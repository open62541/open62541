/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2024 (c) Linutronix GmbH (Author: Vasilij Strassheim)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/server.h>
#include <open62541/driver/mdns.h>

#ifdef UA_ENABLE_DISCOVERY_MULTICAST_AVAHI

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>
#include <avahi-common/address.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/strlst.h>

#include <net/if.h>
#include <stdio.h>

#define UA_AVAHI_POLL_INTERVAL_MS 100
#define UA_AVAHI_MAX_NAME_LENGTH 256

typedef enum {
    AVAHI_RECORD_REMOTE_RECEIVED,
    AVAHI_RECORD_LOCAL_OWNED
} AvahiRecordOrigin;

typedef struct ServerOnNetworkRecord {
    struct ServerOnNetworkRecord *next;
    UA_ServerOnNetwork serverOnNetwork;
    AvahiEntryGroup *group;
    AvahiRecordOrigin origin;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    UA_UInt32 ttl;
} ServerOnNetworkRecord;

typedef struct AddressRecord {
    struct AddressRecord *next;
    UA_String hostname;
    UA_String address;
    unsigned short type;
    AvahiEntryGroup *group;
} AddressRecord;

typedef struct {
    UA_MdnsDriver mdns;
    UA_Logger *logging;
    AvahiClient *client;
    AvahiSimplePoll *simplePoll;
    AvahiServiceBrowser *browser;
    UA_UInt64 pollCallbackId;
    ServerOnNetworkRecord *serverList;
    AddressRecord *addressList;
    UA_Boolean listen;
    UA_Boolean announce;
    UA_Boolean queryPresence;
    UA_Boolean queryDetails;
    UA_UInt32 queryInterval;
    UA_UInt32 announceTTL;
    UA_String interface;
    AvahiIfIndex ifIndex;
} AvahiDriver;

static UA_StatusCode publishLocalRecord(AvahiDriver *md,
                                        ServerOnNetworkRecord *entry);

static UA_UInt32
announceTTL(const AvahiDriver *md) {
    return (md->announceTTL > 0) ? md->announceTTL : 600u;
}

static UA_Boolean
isRemoteReceivedRecord(const ServerOnNetworkRecord *entry) {
    return entry->origin == AVAHI_RECORD_REMOTE_RECEIVED;
}

static UA_Boolean
isLocalOwnedRecord(const ServerOnNetworkRecord *entry) {
    return entry->origin == AVAHI_RECORD_LOCAL_OWNED;
}

static ServerOnNetworkRecord *
findSON(AvahiDriver *md, UA_String serverName) {
    for(ServerOnNetworkRecord *entry = md->serverList; entry; entry = entry->next) {
        if(UA_String_equal(&serverName, &entry->serverOnNetwork.serverName))
            return entry;
    }
    return NULL;
}

static UA_StatusCode
addSON(AvahiDriver *md, const UA_ServerOnNetwork *son,
       AvahiRecordOrigin origin, ServerOnNetworkRecord **out) {
    ServerOnNetworkRecord *entry = (ServerOnNetworkRecord*)
        UA_calloc(1, sizeof(ServerOnNetworkRecord));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode res = UA_ServerOnNetwork_copy(son, &entry->serverOnNetwork);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(entry);
        return res;
    }

    entry->origin = origin;
    entry->next = md->serverList;
    md->serverList = entry;
    if(out)
        *out = entry;
    return UA_STATUSCODE_GOOD;
}

static void
clearEntryGroup(AvahiEntryGroup **group) {
    if(!group || !*group)
        return;
    avahi_entry_group_reset(*group);
    avahi_entry_group_free(*group);
    *group = NULL;
}

static void
removeSONEntry(AvahiDriver *md, ServerOnNetworkRecord *entry,
               UA_Boolean deregisterFromServer, UA_Boolean clearGroup) {
    ServerOnNetworkRecord **prev = &md->serverList;
    while(*prev && *prev != entry)
        prev = &(*prev)->next;
    if(!*prev)
        return;

    *prev = entry->next;
    if(clearGroup)
        clearEntryGroup(&entry->group);

    if(deregisterFromServer)
        UA_Server_deregisterServerOnNetwork(md->mdns.drv.server,
                                            entry->serverOnNetwork.serverName);

    UA_ServerOnNetwork_clear(&entry->serverOnNetwork);
    UA_free(entry);
}

static UA_StatusCode
stringToBuffer(UA_String src, char *dst, size_t dstSize) {
    if(src.length >= dstSize)
        return UA_STATUSCODE_BADOUTOFRANGE;
    memcpy(dst, src.data, src.length);
    dst[src.length] = '\0';
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
appendLocalSuffix(UA_String hostname, char *dst, size_t dstSize) {
    if(hostname.length >= dstSize)
        return UA_STATUSCODE_BADOUTOFRANGE;
    memcpy(dst, hostname.data, hostname.length);
    dst[hostname.length] = '\0';
    if(hostname.length >= 6 &&
       memcmp(hostname.data + hostname.length - 6, ".local", 6) == 0)
        return UA_STATUSCODE_GOOD;
    if(hostname.length >= 7 &&
       memcmp(hostname.data + hostname.length - 7, ".local.", 7) == 0)
        return UA_STATUSCODE_GOOD;
    if(hostname.length + 6 >= dstSize)
        return UA_STATUSCODE_BADOUTOFRANGE;
    memcpy(dst + hostname.length, ".local", 7);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addTxtPath(AvahiStringList **txt, UA_String path) {
    char buf[256];
    if(path.length == 0) {
        *txt = avahi_string_list_add(*txt, "path=/");
        return UA_STATUSCODE_GOOD;
    }
    size_t needed = path.length + 7;
    if(path.data[0] != '/')
        needed++;
    if(needed > sizeof(buf))
        return UA_STATUSCODE_BADOUTOFRANGE;
    size_t pos = 0;
    memcpy(buf + pos, "path=", 5);
    pos += 5;
    if(path.data[0] != '/')
        buf[pos++] = '/';
    memcpy(buf + pos, path.data, path.length);
    pos += path.length;
    buf[pos] = '\0';
    *txt = avahi_string_list_add(*txt, buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addTxtCaps(AvahiStringList **txt, const UA_String *caps, size_t capsSize) {
    if(capsSize == 0) {
        *txt = avahi_string_list_add(*txt, "caps=NA");
        return UA_STATUSCODE_GOOD;
    }
    char buf[256];
    size_t pos = 0;
    memcpy(buf + pos, "caps=", 5);
    pos += 5;
    for(size_t i = 0; i < capsSize; i++) {
        if(i > 0)
            buf[pos++] = ',';
        if(pos + caps[i].length >= sizeof(buf))
            return UA_STATUSCODE_BADOUTOFRANGE;
        memcpy(buf + pos, caps[i].data, caps[i].length);
        pos += caps[i].length;
    }
    buf[pos] = '\0';
    *txt = avahi_string_list_add(*txt, buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
copyCsvPartToString(const char *start, size_t len, UA_String *dst) {
    dst->length = len;
    dst->data = (UA_Byte*)UA_malloc(len + 1);
    if(!dst->data)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(dst->data, start, len);
    dst->data[len] = '\0';
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setCapabilitiesFromCsv(ServerOnNetworkRecord *entry, const char *caps) {
    if(!caps || caps[0] == '\0')
        caps = "NA";
    size_t count = 1;
    for(const char *p = caps; *p; p++) {
        if(*p == ',')
            count++;
    }
    UA_Array_delete(entry->serverOnNetwork.serverCapabilities,
                    entry->serverOnNetwork.serverCapabilitiesSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    entry->serverOnNetwork.serverCapabilities = (UA_String*)
        UA_Array_new(count, &UA_TYPES[UA_TYPES_STRING]);
    if(!entry->serverOnNetwork.serverCapabilities) {
        entry->serverOnNetwork.serverCapabilitiesSize = 0;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    entry->serverOnNetwork.serverCapabilitiesSize = count;
    const char *start = caps;
    for(size_t i = 0; i < count; i++) {
        const char *end = strchr(start, ',');
        size_t len = end ? (size_t)(end - start) : strlen(start);
        UA_StatusCode res =
            copyCsvPartToString(start, len,
                                &entry->serverOnNetwork.serverCapabilities[i]);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        start = end ? end + 1 : start + len;
    }
    return UA_STATUSCODE_GOOD;
}

static const char *
getTxtValue(AvahiStringList *txt, const char *wantedKey) {
    static char valueBuf[256];
    for(AvahiStringList *cur = txt; cur; cur = avahi_string_list_get_next(cur)) {
        char *key = NULL;
        char *value = NULL;
        if(avahi_string_list_get_pair(cur, &key, &value, NULL) < 0)
            continue;
        UA_Boolean match = (strcmp(key, wantedKey) == 0);
        valueBuf[0] = '\0';
        if(match && value)
            snprintf(valueBuf, sizeof(valueBuf), "%s", value);
        avahi_free(key);
        avahi_free(value);
        if(match)
            return valueBuf;
    }
    return NULL;
}

static void
entryGroupCallback(AvahiEntryGroup *group, AvahiEntryGroupState state,
                   void *userdata) {
    AvahiDriver *md = (AvahiDriver*)userdata;
    if(!md)
        return;
    switch(state) {
    case AVAHI_ENTRY_GROUP_ESTABLISHED:
        UA_LOG_INFO(md->logging, UA_LOGCATEGORY_DISCOVERY,
                    "Avahi: Entry group established");
        break;
    case AVAHI_ENTRY_GROUP_COLLISION:
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Avahi: Entry group collision");
        break;
    case AVAHI_ENTRY_GROUP_FAILURE:
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Avahi: Entry group failure: %s",
                     avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(group))));
        break;
    case AVAHI_ENTRY_GROUP_UNCOMMITED:
    case AVAHI_ENTRY_GROUP_REGISTERING:
        break;
    }
}

static UA_StatusCode
publishLocalRecord(AvahiDriver *md, ServerOnNetworkRecord *entry) {
    if(!md->announce)
        return UA_STATUSCODE_GOOD;
    if(!md->client)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_StatusCode res =
        UA_parseEndpointUrl(&entry->serverOnNetwork.discoveryUrl,
                            &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    char serviceName[UA_AVAHI_MAX_NAME_LENGTH];
    res = stringToBuffer(entry->serverOnNetwork.serverName,
                         serviceName, sizeof(serviceName));
    if(res != UA_STATUSCODE_GOOD)
        return res;
    char hostName[UA_AVAHI_MAX_NAME_LENGTH];
    res = appendLocalSuffix(hostname, hostName, sizeof(hostName));
    if(res != UA_STATUSCODE_GOOD)
        return res;
    AvahiStringList *txt = NULL;
    res = addTxtPath(&txt, path);
    if(res == UA_STATUSCODE_GOOD)
        res = addTxtCaps(&txt, entry->serverOnNetwork.serverCapabilities,
                         entry->serverOnNetwork.serverCapabilitiesSize);
    if(res != UA_STATUSCODE_GOOD) {
        avahi_string_list_free(txt);
        return res;
    }
    if(!entry->group) {
        entry->group = avahi_entry_group_new(md->client, entryGroupCallback, md);
        if(!entry->group) {
            avahi_string_list_free(txt);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    } else {
        avahi_entry_group_reset(entry->group);
    }
    int rv = avahi_entry_group_add_service_strlst(entry->group, md->ifIndex,
                                                  AVAHI_PROTO_UNSPEC,
                                                  AVAHI_PUBLISH_USE_MULTICAST,
                                                  serviceName, "_opcua-tcp._tcp",
                                                  NULL, hostName, port, txt);
    avahi_string_list_free(txt);
    if(rv < 0) {
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Avahi: Failed to add service %s: %s", serviceName,
                     avahi_strerror(rv));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    rv = avahi_entry_group_commit(entry->group);
    if(rv < 0) {
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Avahi: Failed to commit service %s: %s", serviceName,
                     avahi_strerror(rv));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static void
removeLocalRecord(AvahiDriver *md, ServerOnNetworkRecord *entry) {
    (void)md;
    clearEntryGroup(&entry->group);
}

static void
registerRemoteIfComplete(AvahiDriver *md, ServerOnNetworkRecord *entry) {
    if(!entry->srvSet || !entry->txtSet)
        return;
    UA_Server_registerServerOnNetwork(md->mdns.drv.server,
                                      &entry->serverOnNetwork,
                                      UA_KEYVALUEMAP_NULL);
}

static void
clientCallback(AvahiClient *client, AvahiClientState state, void *userdata) {
    AvahiDriver *md = (AvahiDriver*)userdata;
    if(!md)
        return;
    UA_LOG_INFO(md->logging, UA_LOGCATEGORY_DISCOVERY,
                "Avahi: Client state changed to %d", state);
    if(state == AVAHI_CLIENT_FAILURE)
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Avahi: Client failure: %s",
                     avahi_strerror(avahi_client_errno(client)));
}

static void
resolveCallback(AvahiServiceResolver *resolver, AvahiIfIndex interface,
                AvahiProtocol protocol, AvahiResolverEvent event,
                const char *name, const char *type, const char *domain,
                const char *hostName, const AvahiAddress *address,
                uint16_t port, AvahiStringList *txt,
                AvahiLookupResultFlags flags, void *userdata) {
    (void)interface;
    (void)protocol;
    (void)type;
    (void)domain;
    (void)address;
    AvahiDriver *md = (AvahiDriver*)userdata;
    if(!md)
        goto cleanup;
    if(event != AVAHI_RESOLVER_FOUND)
        goto cleanup;
    if(flags & AVAHI_LOOKUP_RESULT_LOCAL)
        goto cleanup;
    UA_String serverName = UA_STRING((char*)(uintptr_t)name);
    ServerOnNetworkRecord *entry = findSON(md, serverName);
    if(entry && isLocalOwnedRecord(entry))
        goto cleanup;
    if(!entry) {
        UA_ServerOnNetwork son;
        UA_ServerOnNetwork_init(&son);
        son.serverName = serverName;
        UA_StatusCode res = addSON(md, &son, AVAHI_RECORD_REMOTE_RECEIVED, &entry);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }
    UA_String_clear(&entry->serverOnNetwork.discoveryUrl);
    UA_String_format(&entry->serverOnNetwork.discoveryUrl,
                     "opc.tcp://%s:%u", hostName, port);
    const char *path = getTxtValue(txt, "path");
    if(path && path[0] != '\0' && strcmp(path, "/") != 0)
        UA_String_append(&entry->serverOnNetwork.discoveryUrl,
                         UA_STRING((char*)(uintptr_t)path));
    const char *caps = getTxtValue(txt, "caps");
    if(setCapabilitiesFromCsv(entry, caps) != UA_STATUSCODE_GOOD)
        goto cleanup;
    entry->srvSet = true;
    entry->txtSet = true;
    entry->origin = AVAHI_RECORD_REMOTE_RECEIVED;
    registerRemoteIfComplete(md, entry);
cleanup:
    if(resolver)
        avahi_service_resolver_free(resolver);
}

static void
browseCallback(AvahiServiceBrowser *browser, AvahiIfIndex interface,
               AvahiProtocol protocol, AvahiBrowserEvent event,
               const char *name, const char *type, const char *domain,
               AvahiLookupResultFlags flags, void *userdata) {
    AvahiDriver *md = (AvahiDriver*)userdata;
    if(!md)
        return;
    switch(event) {
    case AVAHI_BROWSER_NEW: {
        if(flags & AVAHI_LOOKUP_RESULT_LOCAL)
            break;
        UA_String serverName = UA_STRING((char*)(uintptr_t)name);
        if(!findSON(md, serverName)) {
            UA_ServerOnNetwork son;
            UA_ServerOnNetwork_init(&son);
            son.serverName = serverName;
            ServerOnNetworkRecord *entry = NULL;
            addSON(md, &son, AVAHI_RECORD_REMOTE_RECEIVED, &entry);
        }
        if(md->queryDetails) {
            AvahiServiceResolver *resolver =
                avahi_service_resolver_new(md->client, interface, protocol,
                                           name, type, domain, AVAHI_PROTO_UNSPEC,
                                           AVAHI_LOOKUP_USE_MULTICAST,
                                           resolveCallback, md);
            if(!resolver)
                UA_LOG_WARNING(md->logging, UA_LOGCATEGORY_DISCOVERY,
                               "Avahi: Failed to resolve service %s: %s", name,
                               avahi_strerror(avahi_client_errno(md->client)));
        }
        break;
    }
    case AVAHI_BROWSER_REMOVE: {
        UA_String serverName = UA_STRING((char*)(uintptr_t)name);
        ServerOnNetworkRecord *entry = findSON(md, serverName);
        if(entry && isRemoteReceivedRecord(entry))
            removeSONEntry(md, entry, true, false);
        break;
    }
    case AVAHI_BROWSER_FAILURE:
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Avahi: Browser failure: %s",
                     avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(browser))));
        break;
    case AVAHI_BROWSER_ALL_FOR_NOW:
    case AVAHI_BROWSER_CACHE_EXHAUSTED:
        break;
    }
}

static void
pollAvahi(UA_Server *server, void *data) {
    (void)server;
    AvahiDriver *md = (AvahiDriver*)data;
    if(!md->simplePoll)
        return;
    int rv = avahi_simple_poll_iterate(md->simplePoll, 0);
    if(rv < 0)
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Avahi: Poll failure: %s", avahi_strerror(rv));
}

static void
AvahiDriverNotificationCallback(UA_Driver *drv,
                                UA_ApplicationNotificationType type,
                                const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY_SERVERONNETWORK)
        return;
    if(drv->state != UA_LIFECYCLESTATE_STARTED)
        return;
    AvahiDriver *md = (AvahiDriver*)drv;
    const UA_ServerOnNetwork *son = (const UA_ServerOnNetwork*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-on-network"),
                                 &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    if(!son)
        return;
    const UA_UInt32 *ttlParam = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "ttl"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 ttl = (ttlParam && *ttlParam > 0) ? *ttlParam : announceTTL(md);
    const UA_Boolean *addedParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-added"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_Boolean *updatedParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-updated"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_Boolean *removedParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-removed"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Boolean added = addedParam ? *addedParam : false;
    UA_Boolean updated = updatedParam ? *updatedParam : false;
    UA_Boolean removed = removedParam ? *removedParam : false;
    ServerOnNetworkRecord *entry = findSON(md, son->serverName);
    if(added || updated) {
        if(entry) {
            UA_ServerOnNetwork sonForCmp = *son;
            sonForCmp.recordId = entry->serverOnNetwork.recordId;
            UA_Order same = UA_order(&sonForCmp, &entry->serverOnNetwork,
                                     &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
            if(same == UA_ORDER_EQ)
                return;
            UA_ServerOnNetwork tmp;
            UA_StatusCode res = UA_ServerOnNetwork_copy(son, &tmp);
            if(res != UA_STATUSCODE_GOOD)
                return;
            UA_ServerOnNetwork_clear(&entry->serverOnNetwork);
            entry->serverOnNetwork = tmp;
        } else {
            UA_StatusCode res = addSON(md, son, AVAHI_RECORD_LOCAL_OWNED, &entry);
            if(res != UA_STATUSCODE_GOOD)
                return;
        }
        entry->origin = AVAHI_RECORD_LOCAL_OWNED;
        entry->ttl = ttl;
        publishLocalRecord(md, entry);
    }
    if(removed) {
        if(!entry)
            return;
        if(isLocalOwnedRecord(entry))
            removeLocalRecord(md, entry);
        removeSONEntry(md, entry, false, false);
    }
}

static UA_StatusCode
AvahiDriver_start(UA_Driver *drv) {
    AvahiDriver *md = (AvahiDriver*)drv;
    if(!drv->server)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_ServerConfig *config = UA_Server_getConfig(drv->server);
    md->logging = config->logging;
    const UA_Boolean *listen = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&drv->params, UA_QUALIFIEDNAME(0, "listen"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    md->listen = listen ? *listen : false;
    const UA_Boolean *announce = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&drv->params, UA_QUALIFIEDNAME(0, "announce"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    md->announce = announce ? *announce : false;
    const UA_Boolean *queryPresence = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&drv->params, UA_QUALIFIEDNAME(0, "query-presence"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    md->queryPresence = queryPresence ? *queryPresence : false;
    const UA_Boolean *queryDetails = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&drv->params, UA_QUALIFIEDNAME(0, "query-details"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    md->queryDetails = queryDetails ? *queryDetails : false;
    const UA_UInt32 *queryInterval = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&drv->params, UA_QUALIFIEDNAME(0, "query-interval"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    md->queryInterval = queryInterval ? *queryInterval : 0;
    const UA_UInt32 *announceTTLParam = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&drv->params, UA_QUALIFIEDNAME(0, "announce-ttl"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    md->announceTTL = announceTTLParam ? *announceTTLParam : 0;
    UA_String_clear(&md->interface);
    const UA_String *interface = (const UA_String*)
        UA_KeyValueMap_getScalar(&drv->params, UA_QUALIFIEDNAME(0, "interface"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(interface)
        UA_String_copy(interface, &md->interface);
    md->ifIndex = AVAHI_IF_UNSPEC;
    if(md->interface.length > 0) {
        char ifName[UA_AVAHI_MAX_NAME_LENGTH];
        if(stringToBuffer(md->interface, ifName, sizeof(ifName)) == UA_STATUSCODE_GOOD) {
            unsigned int idx = if_nametoindex(ifName);
            if(idx > 0) {
                md->ifIndex = (AvahiIfIndex)idx;
            } else {
                UA_LOG_WARNING(md->logging, UA_LOGCATEGORY_DISCOVERY,
                               "Avahi: Interface '%s' is not a local interface "
                               "name; using all interfaces", ifName);
            }
        } else {
            UA_LOG_WARNING(md->logging, UA_LOGCATEGORY_DISCOVERY,
                           "Avahi: Interface parameter too long; using all interfaces");
        }
    }
    if(!md->listen && (md->queryPresence || md->queryDetails)) {
        UA_LOG_WARNING(md->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Avahi: Querying requires listen=true; disabling queries");
        md->queryPresence = false;
        md->queryDetails = false;
        md->queryInterval = 0;
    }
    if(md->queryInterval > 0 && md->queryPresence)
        UA_LOG_INFO(md->logging, UA_LOGCATEGORY_DISCOVERY,
                    "Avahi: query-interval is accepted for configuration parity; "
                    "service discovery remains event-driven");
    md->simplePoll = avahi_simple_poll_new();
    if(!md->simplePoll)
        return UA_STATUSCODE_BADINTERNALERROR;
    int error = 0;
    md->client = avahi_client_new(avahi_simple_poll_get(md->simplePoll),
                                  AVAHI_CLIENT_NO_FAIL, clientCallback,
                                  md, &error);
    if(!md->client) {
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Avahi: Failed to create client: %s", avahi_strerror(error));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(md->listen || md->queryPresence || md->queryDetails) {
        md->browser = avahi_service_browser_new(md->client, md->ifIndex,
                                                AVAHI_PROTO_UNSPEC,
                                                "_opcua-tcp._tcp", NULL,
                                                AVAHI_LOOKUP_USE_MULTICAST,
                                                browseCallback, md);
        if(!md->browser) {
            UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                         "Avahi: Failed to create browser: %s",
                         avahi_strerror(avahi_client_errno(md->client)));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    UA_Server_addRepeatedCallback(drv->server, pollAvahi, md,
                                  UA_AVAHI_POLL_INTERVAL_MS,
                                  &md->pollCallbackId);
    drv->state = UA_LIFECYCLESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
AvahiDriver_stop(UA_Driver *drv) {
    AvahiDriver *md = (AvahiDriver*)drv;
    if(drv->state == UA_LIFECYCLESTATE_STOPPED)
        return;
    drv->state = UA_LIFECYCLESTATE_STOPPING;
    if(md->pollCallbackId) {
        UA_Server_removeRepeatedCallback(drv->server, md->pollCallbackId);
        md->pollCallbackId = 0;
    }
    if(md->browser) {
        avahi_service_browser_free(md->browser);
        md->browser = NULL;
    }
    for(ServerOnNetworkRecord *entry = md->serverList; entry; entry = entry->next) {
        if(isLocalOwnedRecord(entry))
            removeLocalRecord(md, entry);
    }
    for(AddressRecord *ar = md->addressList; ar; ar = ar->next)
        clearEntryGroup(&ar->group);
    if(md->client) {
        avahi_client_free(md->client);
        md->client = NULL;
    }
    if(md->simplePoll) {
        avahi_simple_poll_free(md->simplePoll);
        md->simplePoll = NULL;
    }
    drv->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
AvahiDriver_free(UA_Driver *drv) {
    AvahiDriver *md = (AvahiDriver*)drv;
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;
    ServerOnNetworkRecord *entry = md->serverList;
    while(entry) {
        ServerOnNetworkRecord *next = entry->next;
        clearEntryGroup(&entry->group);
        UA_ServerOnNetwork_clear(&entry->serverOnNetwork);
        UA_free(entry);
        entry = next;
    }
    AddressRecord *ar = md->addressList;
    while(ar) {
        AddressRecord *next = ar->next;
        clearEntryGroup(&ar->group);
        UA_String_clear(&ar->hostname);
        UA_String_clear(&ar->address);
        UA_free(ar);
        ar = next;
    }
    UA_String_clear(&md->interface);
    UA_KeyValueMap_clear(&md->mdns.drv.params);
    UA_free(md);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
publishAddressRecord(UA_MdnsDriver *drv, UA_String hostname, UA_String address,
                     UA_UInt32 ttl, unsigned short type) {
    (void)ttl;
    AvahiDriver *md = (AvahiDriver*)drv;
    if(!md->client)
        return UA_STATUSCODE_BADINTERNALERROR;
    char host[UA_AVAHI_MAX_NAME_LENGTH];
    UA_StatusCode res = stringToBuffer(hostname, host, sizeof(host));
    if(res != UA_STATUSCODE_GOOD)
        return res;
    char addr[AVAHI_ADDRESS_STR_MAX];
    res = stringToBuffer(address, addr, sizeof(addr));
    if(res != UA_STATUSCODE_GOOD)
        return res;
    AvahiProtocol proto = (type == 1) ? AVAHI_PROTO_INET : AVAHI_PROTO_INET6;
    AvahiAddress avahiAddress;
    if(!avahi_address_parse(addr, proto, &avahiAddress))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    AddressRecord *ar = (AddressRecord*)UA_calloc(1, sizeof(AddressRecord));
    if(!ar)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_String_copy(&hostname, &ar->hostname);
    UA_String_copy(&address, &ar->address);
    ar->type = type;
    ar->group = avahi_entry_group_new(md->client, entryGroupCallback, md);
    if(!ar->group) {
        UA_free(ar);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    int rv = avahi_entry_group_add_address(ar->group, md->ifIndex,
                                           AVAHI_PROTO_UNSPEC,
                                           AVAHI_PUBLISH_USE_MULTICAST,
                                           host, &avahiAddress);
    if(rv < 0) {
        clearEntryGroup(&ar->group);
        UA_String_clear(&ar->hostname);
        UA_String_clear(&ar->address);
        UA_free(ar);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    rv = avahi_entry_group_commit(ar->group);
    if(rv < 0) {
        clearEntryGroup(&ar->group);
        UA_String_clear(&ar->hostname);
        UA_String_clear(&ar->address);
        UA_free(ar);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    ar->next = md->addressList;
    md->addressList = ar;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeAddressRecord(UA_MdnsDriver *drv, UA_String hostname, UA_String address,
                    unsigned short type) {
    AvahiDriver *md = (AvahiDriver*)drv;
    AddressRecord **prev = &md->addressList;
    while(*prev) {
        AddressRecord *ar = *prev;
        if(ar->type == type && UA_String_equal(&hostname, &ar->hostname) &&
           UA_String_equal(&address, &ar->address)) {
            *prev = ar->next;
            clearEntryGroup(&ar->group);
            UA_String_clear(&ar->hostname);
            UA_String_clear(&ar->address);
            UA_free(ar);
            return UA_STATUSCODE_GOOD;
        }
        prev = &(*prev)->next;
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
AvahiDriver_addARecord(UA_MdnsDriver *drv, UA_String hostname,
                       UA_String address, UA_UInt32 ttl) {
    return publishAddressRecord(drv, hostname, address, ttl, 1);
}

static UA_StatusCode
AvahiDriver_removeARecord(UA_MdnsDriver *drv, UA_String hostname,
                          UA_String address) {
    return removeAddressRecord(drv, hostname, address, 1);
}

static UA_StatusCode
AvahiDriver_addAAAARecord(UA_MdnsDriver *drv, UA_String hostname,
                          UA_String address, UA_UInt32 ttl) {
    return publishAddressRecord(drv, hostname, address, ttl, 28);
}

static UA_StatusCode
AvahiDriver_removeAAAARecord(UA_MdnsDriver *drv, UA_String hostname,
                             UA_String address) {
    return removeAddressRecord(drv, hostname, address, 28);
}

UA_MdnsDriver *
UA_MdnsDriver_Avahi(const UA_KeyValueMap params) {
    AvahiDriver *md = (AvahiDriver*)UA_calloc(1, sizeof(AvahiDriver));
    if(!md)
        return NULL;
    UA_StatusCode res = UA_KeyValueMap_copy(&params, &md->mdns.drv.params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(md);
        return NULL;
    }
    md->mdns.drv.name = UA_STRING("discovery-mdns-avahi");
    md->mdns.drv.start = AvahiDriver_start;
    md->mdns.drv.stop = AvahiDriver_stop;
    md->mdns.drv.free = AvahiDriver_free;
    md->mdns.drv.notificationCallback = AvahiDriverNotificationCallback;
    md->mdns.drv.notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY;
    md->mdns.addARecord = AvahiDriver_addARecord;
    md->mdns.removeARecord = AvahiDriver_removeARecord;
    md->mdns.addAAAARecord = AvahiDriver_addAAAARecord;
    md->mdns.removeAAAARecord = AvahiDriver_removeAAAARecord;
    return &md->mdns;
}

#endif /* UA_ENABLE_DISCOVERY_MULTICAST_AVAHI */
