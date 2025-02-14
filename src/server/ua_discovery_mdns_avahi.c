/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2024 (c) Linutronix GmbH (Author: Vasilij Strassheim)
 */

#include "ua_discovery.h"
#include "ua_server_internal.h"
#include <stdio.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/strlst.h>
#include <avahi-common/address.h>

#ifdef UA_ENABLE_DISCOVERY_MULTICAST_AVAHI

#include "../deps/mp_printf.h"

typedef struct serverOnNetwork {
    LIST_ENTRY(serverOnNetwork) pointers;
    UA_ServerOnNetwork serverOnNetwork;
    AvahiEntryGroup *group;
    UA_DateTime created;
    UA_DateTime lastSeen;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char* pathTmp;
} serverOnNetwork;

#define SERVER_ON_NETWORK_HASH_SIZE 1000
typedef struct serverOnNetwork_hash_entry {
    serverOnNetwork *entry;
    struct serverOnNetwork_hash_entry* next;
} serverOnNetwork_hash_entry;

typedef struct avahiPrivate {
    AvahiClient *client;
    AvahiSimplePoll *simple_poll;
    AvahiServiceBrowser *browser;
    UA_Server *server;
    /* hash mapping domain name to serverOnNetwork list entry */
    struct serverOnNetwork_hash_entry* serverOnNetworkHash[SERVER_ON_NETWORK_HASH_SIZE];
    LIST_HEAD(, serverOnNetwork) serverOnNetwork;
    /* Name of server itself. Used to detect if received mDNS
     * message was from itself */
    UA_String selfMdnsRecord;
    UA_UInt32 serverOnNetworkRecordIdCounter;
    UA_DateTime serverOnNetworkRecordIdLastReset;
} avahiPrivate;

static
avahiPrivate mdnsPrivateData;

static UA_StatusCode
UA_DiscoveryManager_addEntryToServersOnNetwork(UA_DiscoveryManager *dm,
                                               UA_String serverName,
                                               struct serverOnNetwork **addedEntry);

static void
UA_Discovery_multicastConflict(char *name, UA_DiscoveryManager *dm) {
    /* In case logging is disabled */
    (void)name;
    UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                 "Multicast DNS name conflict detected: '%s'", name);
}

static struct serverOnNetwork *
mdns_record_add_or_get(UA_DiscoveryManager *dm, UA_String serverName,
                       UA_Boolean createNew) {
    UA_UInt32 hashIdx = UA_ByteString_hash(0, serverName.data,
                                           serverName.length) % SERVER_ON_NETWORK_HASH_SIZE;
    struct serverOnNetwork_hash_entry *hash_entry = mdnsPrivateData.serverOnNetworkHash[hashIdx];

    while(hash_entry) {
        size_t maxLen = serverName.length;
        if(maxLen > hash_entry->entry->serverOnNetwork.serverName.length)
            maxLen = hash_entry->entry->serverOnNetwork.serverName.length;

        if(strncmp((char*)hash_entry->entry->serverOnNetwork.serverName.data,
                   (char*)serverName.data, maxLen) == 0)
            return hash_entry->entry;
        hash_entry = hash_entry->next;
    }

    if(!createNew)
        return NULL;

    struct serverOnNetwork *listEntry;
    UA_StatusCode res =
        UA_DiscoveryManager_addEntryToServersOnNetwork(dm, serverName, &listEntry);
    if(res != UA_STATUSCODE_GOOD)
        return NULL;

    return listEntry;
}

static UA_StatusCode
UA_DiscoveryManager_addEntryToServersOnNetwork(UA_DiscoveryManager *dm,
                                               UA_String serverName,
                                               struct serverOnNetwork **addedEntry) {
    struct serverOnNetwork *entry =
            mdns_record_add_or_get(dm, serverName, false);
    if(entry) {
        if(addedEntry != NULL)
            *addedEntry = entry;
        return UA_STATUSCODE_BADALREADYEXISTS;
    }

    UA_LOG_DEBUG(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                 "Multicast DNS: Add entry to ServersOnNetwork: (%S)", serverName);

    struct serverOnNetwork *listEntry = (serverOnNetwork*)
            UA_malloc(sizeof(struct serverOnNetwork));
    if(!listEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;


    UA_EventLoop *el = dm->sc.server->config.eventLoop;
    listEntry->created = el->dateTime_now(el);
    listEntry->pathTmp = NULL;
    listEntry->txtSet = false;
    listEntry->srvSet = false;
    UA_ServerOnNetwork_init(&listEntry->serverOnNetwork);
    listEntry->serverOnNetwork.recordId = mdnsPrivateData.serverOnNetworkRecordIdCounter;
    UA_StatusCode res = UA_String_copy(&serverName, &listEntry->serverOnNetwork.serverName);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(listEntry);
        return res;
    }
    mdnsPrivateData.serverOnNetworkRecordIdCounter++;
    if(mdnsPrivateData.serverOnNetworkRecordIdCounter == 0)
        UA_DiscoveryManager_resetServerOnNetworkRecordCounter(dm);
    listEntry->lastSeen = el->dateTime_nowMonotonic(el);

    /* add to hash */
    UA_UInt32 hashIdx = UA_ByteString_hash(0, serverName.data, serverName.length) %
                        SERVER_ON_NETWORK_HASH_SIZE;
    struct serverOnNetwork_hash_entry *newHashEntry = (struct serverOnNetwork_hash_entry*)
            UA_malloc(sizeof(struct serverOnNetwork_hash_entry));
    if(!newHashEntry) {
        UA_String_clear(&listEntry->serverOnNetwork.serverName);
        UA_free(listEntry);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    newHashEntry->next = mdnsPrivateData.serverOnNetworkHash[hashIdx];
    mdnsPrivateData.serverOnNetworkHash[hashIdx] = newHashEntry;
    newHashEntry->entry = listEntry;

    LIST_INSERT_HEAD(&mdnsPrivateData.serverOnNetwork, listEntry, pointers);
    if(addedEntry != NULL)
        *addedEntry = listEntry;

    /* call callback for every entry we receive. */
    if(dm->serverOnNetworkCallback) {
        dm->serverOnNetworkCallback(&listEntry->serverOnNetwork, true, listEntry->txtSet,
                                    dm->serverOnNetworkCallbackData);
    }

    return UA_STATUSCODE_GOOD;
}

static void
entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state,
                     AVAHI_GCC_UNUSED void *userdata) {
    bool groupFound = false;
    if(!userdata)
        return;
    UA_DiscoveryManager *dm = (UA_DiscoveryManager *)userdata;
    if(!g) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "AvahiEntryGroup or userdata is NULL");
        return;
    }
    /* Called whenever the entry group state changes */
    /* Find the registered service on network */
    serverOnNetwork *current;
    LIST_FOREACH(current, &mdnsPrivateData.serverOnNetwork, pointers) {
        if(current->group == g) {
            groupFound = true;
            break;
        }
    }
    switch(state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED:
            UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                        "Entry group established.");
            break;
        case AVAHI_ENTRY_GROUP_COLLISION: {
            if(!groupFound) {
                UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                             "Entry group collision for unknown group.");
                break;
            }
            char *name = strndup((const char *)current->serverOnNetwork.serverName.data,
                                 current->serverOnNetwork.serverName.length);
            UA_Discovery_multicastConflict(name, dm);
            break;
        }
        case AVAHI_ENTRY_GROUP_FAILURE:
            UA_LOG_ERROR(
                dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                "Entry group failure: %s",
                avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

            /* Some kind of failure happened while we were registering our services */
            avahi_simple_poll_quit(mdnsPrivateData.simple_poll);
            break;
        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            break;
        default:
            UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                         "Unknown entry group state");
            break;
    }
}

static UA_StatusCode
UA_DiscoveryManager_removeEntryFromServersOnNetwork(UA_DiscoveryManager *dm,
                                                    UA_String serverName) {
    UA_LOG_DEBUG(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                 "Multicast DNS: Remove entry from ServersOnNetwork: %S",serverName);

    struct serverOnNetwork *entry =
            mdns_record_add_or_get(dm, serverName, false);
    if(!entry)
        return UA_STATUSCODE_BADNOTFOUND;

    /* remove from hash */
    UA_UInt32 hashIdx = UA_ByteString_hash(0, (const UA_Byte*)serverName.data,
                                           serverName.length) % SERVER_ON_NETWORK_HASH_SIZE;
    struct serverOnNetwork_hash_entry *hash_entry = mdnsPrivateData.serverOnNetworkHash[hashIdx];
    struct serverOnNetwork_hash_entry *prevEntry = hash_entry;
    while(hash_entry) {
        if(hash_entry->entry == entry) {
            if(mdnsPrivateData.serverOnNetworkHash[hashIdx] == hash_entry)
                mdnsPrivateData.serverOnNetworkHash[hashIdx] = hash_entry->next;
            else if(prevEntry)
                prevEntry->next = hash_entry->next;
            break;
        }
        prevEntry = hash_entry;
        hash_entry = hash_entry->next;
    }
    UA_free(hash_entry);

    if(dm->serverOnNetworkCallback &&
        !UA_String_equal(&mdnsPrivateData.selfMdnsRecord, &serverName))
        dm->serverOnNetworkCallback(&entry->serverOnNetwork, false,
                                    entry->txtSet,
                                    dm->serverOnNetworkCallbackData);

    /* Remove from list */
    LIST_REMOVE(entry, pointers);
    UA_ServerOnNetwork_clear(&entry->serverOnNetwork);
    if(entry->pathTmp) {
        UA_free(entry->pathTmp);
        entry->pathTmp = NULL;
    }
    UA_free(entry);
    return UA_STATUSCODE_GOOD;
}


UA_StatusCode
UA_DiscoveryManager_clearServerOnNetwork(UA_DiscoveryManager *dm) {
    if(!dm) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "DiscoveryManager is NULL");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    serverOnNetwork *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &mdnsPrivateData.serverOnNetwork, pointers, son_tmp) {
        LIST_REMOVE(son, pointers);
        UA_ServerOnNetwork_clear(&son->serverOnNetwork);
        if(son->pathTmp)
            UA_free(son->pathTmp);
        UA_free(son);
    }

    UA_String_clear(&mdnsPrivateData.selfMdnsRecord);

    for(size_t i = 0; i < SERVER_ON_NETWORK_HASH_SIZE; i++) {
        serverOnNetwork_hash_entry* currHash = mdnsPrivateData.serverOnNetworkHash[i];
        while(currHash) {
            serverOnNetwork_hash_entry* nextHash = currHash->next;
            UA_free(currHash);
            currHash = nextHash;
        }
    }

    return UA_STATUSCODE_GOOD;
}

UA_ServerOnNetwork*
UA_DiscoveryManager_getServerOnNetworkList(void) {
    serverOnNetwork* entry = LIST_FIRST(&mdnsPrivateData.serverOnNetwork);
    return entry ? &entry->serverOnNetwork : NULL;
}

UA_ServerOnNetwork*
UA_DiscoveryManager_getNextServerOnNetworkRecord(UA_ServerOnNetwork *current) {
    serverOnNetwork *entry = NULL;
    LIST_FOREACH(entry, &mdnsPrivateData.serverOnNetwork, pointers) {
        if(&entry->serverOnNetwork == current) {
            entry = LIST_NEXT(entry, pointers);
            break;
        }
    }
    return entry ? &entry->serverOnNetwork : NULL;
}


UA_UInt32
UA_DiscoveryManager_getServerOnNetworkRecordIdCounter(UA_DiscoveryManager *dm) {
    if(!dm) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "DiscoveryManager is NULL");
        return 0;
    }
    return mdnsPrivateData.serverOnNetworkRecordIdCounter;
}

UA_StatusCode
UA_DiscoveryManager_resetServerOnNetworkRecordCounter(UA_DiscoveryManager *dm) {
    if(!dm) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "DiscoveryManager is NULL");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    mdnsPrivateData.serverOnNetworkRecordIdCounter = 0;
    mdnsPrivateData.serverOnNetworkRecordIdLastReset = dm->sc.server->config.eventLoop->dateTime_now(
        dm->sc.server->config.eventLoop);
    return UA_STATUSCODE_GOOD;
}

UA_DateTime
UA_DiscoveryManager_getServerOnNetworkCounterResetTime(UA_DiscoveryManager *dm) {
    if(!dm) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "DiscoveryManager is NULL");
        return 0;
    }
    return mdnsPrivateData.serverOnNetworkRecordIdLastReset;
}

typedef enum {
    UA_DISCOVERY_TCP,    /* OPC UA TCP mapping */
    UA_DISCOVERY_TLS     /* OPC UA HTTPS mapping */
} UA_DiscoveryProtocol;

/* Create a mDNS Record for the given server info and adds it to the mDNS output
 * queue.
 *
 * Additionally this method also adds the given server to the internal
 * serversOnNetwork list so that a client finds it when calling
 * FindServersOnNetwork. */
static UA_StatusCode
UA_Discovery_addRecord(UA_DiscoveryManager *dm, const UA_String servername,
                       const UA_String hostname, UA_UInt16 port,
                       const UA_String path, const UA_DiscoveryProtocol protocol,
                       UA_Boolean createTxt, const UA_String* capabilites,
                       const size_t capabilitiesSize,
                       UA_Boolean isSelf);

/* Create a mDNS Record for the given server info with TTL=0 and adds it to the
 * mDNS output queue.
 *
 * Additionally this method also removes the given server from the internal
 * serversOnNetwork list so that a client gets the updated data when calling
 * FindServersOnNetwork. */
static UA_StatusCode
UA_Discovery_removeRecord(UA_DiscoveryManager *dm, const UA_String servername,
                          const UA_String hostname, UA_UInt16 port,
                          UA_Boolean removeTxt);

static UA_StatusCode
addMdnsRecordForNetworkLayer(UA_DiscoveryManager *dm, const UA_String serverName,
                             const UA_String *discoveryUrl) {
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode retval =
        UA_parseEndpointUrl(discoveryUrl, &hostname, &port, &path);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url is invalid: %S", *discoveryUrl);
        return retval;
    }

    if(hostname.length == 0) {
        /* get host name used by avahi */
        const char *hoststr = avahi_client_get_host_name(mdnsPrivateData.client);
        if(!hoststr) {
            UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                           "Cannot get hostname from avahi");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        hostname = UA_String_fromChars(hoststr);
    }
    retval = UA_Discovery_addRecord(dm, serverName, hostname, port, path, UA_DISCOVERY_TCP, true,
                                    dm->sc.server->config.mdnsConfig.serverCapabilities,
                                    dm->sc.server->config.mdnsConfig.serverCapabilitiesSize, true);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                       "Cannot add mDNS Record: %s", UA_StatusCode_name(retval));
        return retval;
    }
    return UA_STATUSCODE_GOOD;
}

#ifndef IN_ZERONET
#define IN_ZERONET(addr) ((addr & IN_CLASSA_NET) == 0)
#endif

/* Callback when the client state changes */
static
void client_callback(AvahiClient *c, AvahiClientState state, void *userdata) {
    /* Handle state changes if necessary */
    UA_LOG_INFO(((UA_DiscoveryManager *)userdata)->sc.server->config.logging,
                UA_LOGCATEGORY_DISCOVERY, "Avahi client state changed to %d", state);
}

/* Called whenever a service has been resolved successfully or timed out */
static void
resolve_callback(AvahiServiceResolver *r, AVAHI_GCC_UNUSED AvahiIfIndex interface,
                 AVAHI_GCC_UNUSED AvahiProtocol protocol, AvahiResolverEvent event,
                 const char *name, const char *type, const char *domain,
                 const char *host_name, const AvahiAddress *address, uint16_t port,
                 AvahiStringList *txt, AvahiLookupResultFlags flags,
                 AVAHI_GCC_UNUSED void *userdata) {

    UA_DiscoveryManager *dm = (UA_DiscoveryManager *)userdata;

    switch(event) {
        case AVAHI_RESOLVER_FAILURE:
            UA_LOG_ERROR(
                dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                "Failed to resolve service '%s' of type '%s' in domain '%s': %s", name,
                type, domain,
                avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
            break;

        case AVAHI_RESOLVER_FOUND: {
            char a[AVAHI_ADDRESS_STR_MAX];
            int discoveryLength = 0;
            char *path = NULL;
            UA_String serverName = UA_String_fromChars(name);
            avahi_address_snprint(a, sizeof(a), address);

            /* Ignore own name */
            if(UA_String_equal(&mdnsPrivateData.selfMdnsRecord, &serverName)) {
                UA_String_clear(&serverName);
                return;
            }
            struct serverOnNetwork *listEntry;
            UA_StatusCode res = UA_DiscoveryManager_addEntryToServersOnNetwork(
                dm, serverName, &listEntry);
            if(res != UA_STATUSCODE_GOOD && res != UA_STATUSCODE_BADALREADYEXISTS) {
                UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                             "Failed to add server to ServersOnNetwork: %s",
                             UA_StatusCode_name(res));
                UA_String_clear(&serverName);
                return;
            } else if(res == UA_STATUSCODE_BADALREADYEXISTS) {
                UA_LOG_DEBUG(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                             "Server already in ServersOnNetwork: %s", name);
                UA_String_clear(&serverName);
                return;
            }

            UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                        "Service '%s' of type '%s' in domain '%s':", name, type, domain);

            UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                        "  %s:%u (%s)", host_name, port, a);
            /* Add discoveryUrl opc.tcp://[servername]:[port][path] */
            discoveryLength = strlen("opc.tcp://") + strlen(host_name) + 5 + 1;
            int listSize = avahi_string_list_length(txt);
            for(int i = 0; i < listSize; i++) {
                char *value = NULL;
                char *key = NULL;
                if(avahi_string_list_get_pair(txt, &key, &value, NULL) < 0)
                    continue;
                /* Add path if the is more then just a single slash */
                if(strcmp(key, "path") == 0) {
                    /* Add path to discovery URL */
                    discoveryLength += strlen(value);
                    path = (char *)UA_malloc(strlen(value) + 1);
                    if(!path) {
                        UA_LOG_ERROR(dm->sc.server->config.logging,
                                     UA_LOGCATEGORY_DISCOVERY,
                                     "Failed to allocate memory for path");
                        UA_String_clear(&serverName);
                        return;
                    }
                    if(strlen(value) > 1) {
                        sprintf(path, "%s", value);
                    } else { /* Ignore empty path */
                        path[0] = '\0';
                    }
                }

                UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                            "  %s = %s", key, value);
                avahi_free(key);
                avahi_free(value);
                txt = avahi_string_list_get_next(txt);
            }

            listEntry->lastSeen = dm->sc.server->config.eventLoop->dateTime_nowMonotonic(
                dm->sc.server->config.eventLoop);
            listEntry->serverOnNetwork.discoveryUrl.length = discoveryLength;
            listEntry->serverOnNetwork.discoveryUrl.data =
                (UA_Byte *)UA_malloc(discoveryLength);
            if(!listEntry->serverOnNetwork.discoveryUrl.data) {
                UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                             "Failed to allocate memory for discoveryUrl");
                UA_free(path);
                UA_String_clear(&serverName);
                return;
            }
            sprintf((char *)listEntry->serverOnNetwork.discoveryUrl.data,
                    "opc.tcp://%s:%u%s", host_name, port, path);
            UA_free(path);
            UA_String_clear(&serverName);
            break;
        }
        default:
            UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                        "Invalid avahi resolver event %d", event);
            break;

    }
    avahi_service_resolver_free(r);
}

static void
browse_callback(AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol,
                AvahiBrowserEvent event, const char *name, const char *type,
                const char *domain, AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
                void *userdata) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager *)userdata;
    switch(event) {
        case AVAHI_BROWSER_FAILURE:
            UA_LOG_ERROR(
                dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                "Browser failure: %s",
                avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
            avahi_simple_poll_quit(mdnsPrivateData.simple_poll);
            return;
        case AVAHI_BROWSER_NEW:
            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */
            if(!(avahi_service_resolver_new(
                   mdnsPrivateData.client, interface, protocol, name, type, domain, AVAHI_PROTO_INET,
                   AVAHI_LOOKUP_USE_MULTICAST, resolve_callback, dm))) {
                UA_LOG_INFO(
                    dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                    "Failed to resolve service '%s' of type '%s' in domain '%s': %s",
                    name, type, domain, avahi_strerror(avahi_client_errno(mdnsPrivateData.client)));
            }
            break;
        case AVAHI_BROWSER_REMOVE:
            UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                        "Service '%s' of type '%s' in domain '%s' removed", name, type,
                        domain);
            UA_String nameStr = UA_STRING_ALLOC(name);
            UA_DiscoveryManager_removeEntryFromServersOnNetwork(dm, nameStr);
            UA_String_clear(&nameStr);
            break;
        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                        "Browser %s",
                        event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED"
                                                               : "ALL_FOR_NOW");
            break;
    }
}

void
UA_DiscoveryManager_startMulticast(UA_DiscoveryManager *dm) {
    int error;
    mdnsPrivateData.simple_poll = avahi_simple_poll_new();
    if(!mdnsPrivateData.simple_poll) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "Failed to create avahi simple poll");
        return;
    }

    mdnsPrivateData.client = avahi_client_new(avahi_simple_poll_get(mdnsPrivateData.simple_poll),
                                  AVAHI_CLIENT_NO_FAIL, client_callback, dm, &error);
    if(!mdnsPrivateData.client) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "Failed to create avahi client: %s", avahi_strerror(error));
        return;
    }
    /* Add record for the server itself */
    UA_String appName = dm->sc.server->config.mdnsConfig.mdnsServerName;
    for(size_t i = 0; i < dm->sc.server->config.serverUrlsSize; i++)
        addMdnsRecordForNetworkLayer(dm, appName, &dm->sc.server->config.serverUrls[i]);

    mdnsPrivateData.browser = avahi_service_browser_new(mdnsPrivateData.client, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET,
                              "_opcua-tcp._tcp", NULL, AVAHI_LOOKUP_USE_MULTICAST,
                              browse_callback, dm);
}

void
UA_DiscoveryManager_stopMulticast(UA_DiscoveryManager *dm) {
    UA_Server *server = dm->sc.server;
    for(size_t i = 0; i < server->config.serverUrlsSize; i++) {
        UA_String hostname = UA_STRING_NULL;
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;

        UA_StatusCode retval =
            UA_parseEndpointUrl(&server->config.serverUrls[i],
                                &hostname, &port, &path);

        if(retval != UA_STATUSCODE_GOOD)
            continue;
        if(hostname.length == 0) {
            /* get host name used by avahi */
            const char *hoststr = avahi_client_get_host_name(mdnsPrivateData.client);
            if(!hoststr) {
                UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                               "Cannot get hostname from avahi");
                continue;
            }
            hostname = UA_String_fromChars(hoststr);
        }

        UA_Discovery_removeRecord(dm, server->config.mdnsConfig.mdnsServerName,
                                  hostname, port, true);
    }
    /* clean up avahi resources */
    if(mdnsPrivateData.browser)
        avahi_service_browser_free(mdnsPrivateData.browser);
    if(mdnsPrivateData.client)
        avahi_client_free(mdnsPrivateData.client);
    if(mdnsPrivateData.simple_poll)
        avahi_simple_poll_free(mdnsPrivateData.simple_poll);
}

void
UA_DiscoveryManager_clearMdns(void) {
    /* Clean up the serverOnNetwork list */
    serverOnNetwork *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &mdnsPrivateData.serverOnNetwork, pointers, son_tmp) {
        LIST_REMOVE(son, pointers);
        UA_ServerOnNetwork_clear(&son->serverOnNetwork);
        if(son->pathTmp)
            UA_free(son->pathTmp);
        UA_free(son);
    }

    UA_String_clear(&mdnsPrivateData.selfMdnsRecord);

    for(size_t i = 0; i < SERVER_ON_NETWORK_HASH_SIZE; i++) {
        serverOnNetwork_hash_entry* currHash = mdnsPrivateData.serverOnNetworkHash[i];
        while(currHash) {
            serverOnNetwork_hash_entry* nextHash = currHash->next;
            UA_free(currHash);
            currHash = nextHash;
        }
    }
}

void
UA_Discovery_updateMdnsForDiscoveryUrl(UA_DiscoveryManager *dm, const UA_String serverName,
                                       const UA_MdnsDiscoveryConfiguration *mdnsConfig,
                                       const UA_String discoveryUrl,
                                       UA_Boolean isOnline, UA_Boolean updateTxt) {
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode retval =
        UA_parseEndpointUrl(&discoveryUrl, &hostname, &port, &path);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url invalid: %S", discoveryUrl);
        return;
    }

    if(!isOnline) {
        UA_StatusCode removeRetval =
                UA_Discovery_removeRecord(dm, serverName, hostname,
                                          port, updateTxt);
        if(removeRetval != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                           "Could not remove mDNS record for hostname %S", serverName);
        return;
    }

    UA_String *capabilities = NULL;
    size_t capabilitiesSize = 0;
    if(mdnsConfig) {
        capabilities = mdnsConfig->serverCapabilities;
        capabilitiesSize = mdnsConfig->serverCapabilitiesSize;
    }

    UA_StatusCode addRetval =
        UA_Discovery_addRecord(dm, serverName, hostname,
                               port, path, UA_DISCOVERY_TCP, updateTxt,
                               capabilities, capabilitiesSize, false);
    if(addRetval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                       "Could not add mDNS record for hostname %S", serverName);
}

void
UA_Server_setServerOnNetworkCallback(UA_Server *server,
                                     UA_Server_serverOnNetworkCallback cb,
                                     void* data) {
    lockServer(server);
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(dm) {
        dm->serverOnNetworkCallback = cb;
        dm->serverOnNetworkCallbackData = data;
    }
    unlockServer(server);
}

void
UA_DiscoveryManager_mdnsCyclicTimer(UA_Server *server, void *data) {
    int ret = avahi_simple_poll_iterate(mdnsPrivateData.simple_poll, 0);
    if(ret < 0) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "Error in avahi_simple_poll_iterate: %s", avahi_strerror(ret));
    }
}

/* Create a service domain with the format [servername]-[hostname]._opcua-tcp._tcp.local. */
static void
createServiceDomain(char *outServiceDomain, size_t maxLen,
                        UA_String servername, UA_String hostname) {

    /* Can we use hostname and servername with full length? */
    if(hostname.length + servername.length + 1 > maxLen) {
        if(servername.length + 2 > maxLen) {
            servername.length = maxLen;
            hostname.length = 0;
        } else {
            hostname.length = maxLen - servername.length - 1;
        }
    }

    size_t offset = 0;
    if(hostname.length > 0) {
        mp_snprintf(outServiceDomain, maxLen + 1, "%S-%S", servername, hostname);
        offset = servername.length + hostname.length + 1;
        //replace all dots with minus. Otherwise mDNS is not valid
        for(size_t i = servername.length+1; i < offset; i++) {
            if(outServiceDomain[i] == '.')
                outServiceDomain[i] = '-';
        }
    } else {
        mp_snprintf(outServiceDomain, maxLen + 1, "%S", servername);
    }
}

/* Check if mDNS already has an entry for given hostname and port combination */
static UA_Boolean
UA_Discovery_recordExists(UA_DiscoveryManager *dm, const char* serviceDomain,
                          unsigned short port, const UA_DiscoveryProtocol protocol) {
    struct serverOnNetwork *current;
    LIST_FOREACH(current, &mdnsPrivateData.serverOnNetwork, pointers) {
        if(strcmp((char*)current->serverOnNetwork.serverName.data, serviceDomain) == 0)
            return true;
    }
    return false;
}

static UA_StatusCode
handle_path(AvahiStringList **txt, const UA_String path) {
    if (!path.data || path.length == 0) {
        *txt = avahi_string_list_add(*txt, "path=/");
    } else {
        char *allocPath = NULL;
        if (path.data[0] == '/') {
            allocPath = avahi_strdup((const char*) path.data);
        } else {
            size_t pLen = path.length + 2;
            allocPath = (char *) avahi_malloc(pLen);
            if(!allocPath)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            snprintf(allocPath, pLen, "/%s", path.data);
        }
        size_t pathLen = strlen("path=") + strlen(allocPath) + 1;
        char *path_kv = (char *) avahi_malloc(pathLen);
        if(!path_kv) {
            avahi_free(allocPath);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        snprintf(path_kv, pathLen, "path=%s", allocPath);
        *txt = avahi_string_list_add(*txt, path_kv);
        avahi_free(allocPath);
        avahi_free(path_kv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
handle_capabilities(AvahiStringList **txt, const UA_String *capabilities,
                    const size_t capabilitiesSize) {
    size_t capsLen = 0;
    for (size_t i = 0; i < capabilitiesSize; i++) {
        capsLen += capabilities[i].length + 1; // +1 for comma or null terminator
    }
    char *caps = (char *) avahi_malloc(capsLen);
    if(!caps)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    caps[0] = '\0';
    for (size_t i = 0; i < capabilitiesSize; i++) {
        strncat(caps, (const char*)capabilities[i].data, capabilities[i].length);
        if (i < capabilitiesSize - 1) {
            if (strcat(caps, ",") == NULL) {
                avahi_free(caps);
                return UA_STATUSCODE_BADINTERNALERROR;
            }
        }
    }
    size_t caps_kv_len = strlen("caps=") + strlen(caps) + 1;
    char *caps_kv = (char *)avahi_malloc(caps_kv_len);
    if(!caps_kv) {
        avahi_free(caps);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    snprintf(caps_kv, caps_kv_len, "caps=%s", caps);
    *txt = avahi_string_list_add(*txt, caps_kv);
    avahi_free(caps);
    avahi_free(caps_kv);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Discovery_addRecord(UA_DiscoveryManager *dm, const UA_String servername,
                       const UA_String hostname, UA_UInt16 port,
                       const UA_String path, const UA_DiscoveryProtocol protocol,
                       UA_Boolean createTxt, const UA_String* capabilites,
                       const size_t capabilitiesSize,
                       UA_Boolean isSelf) {
    /* We assume that the hostname is not an IP address, but a valid domain
     * name. It is required by the OPC UA spec (see Part 12, DiscoveryURL to DNS
     * SRV mapping) to always use the hostname instead of the IP address. */

    if(capabilitiesSize > 0 && !capabilites)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(hostname.length == 0 || servername.length == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    /* Use a limit for the hostname length to make sure full string fits into 63
     * chars (limited by DNS spec) */
    if(hostname.length + servername.length + 1 > 63) { /* include dash between servername-hostname */
        UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    } else if(hostname.length > 63) {
        UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Hostname length exceeds maximum of 63 chars. "
                       "It will be truncated.");
    }

    if(!dm->mdnsMainSrvAdded) {
        dm->mdnsMainSrvAdded = true;
    }

    /* [servername]-[hostname] */
    char serviceDomain[63];
    createServiceDomain(serviceDomain, 63, servername, hostname);

    UA_Boolean exists = UA_Discovery_recordExists(dm, serviceDomain,
                                                  port, protocol);
    if(exists == true)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: add record for domain: %s", serviceDomain);

    UA_String serverName = UA_String_fromChars(serviceDomain);
    if(isSelf && mdnsPrivateData.selfMdnsRecord.length == 0) {
        mdnsPrivateData.selfMdnsRecord = serverName;
        if(!mdnsPrivateData.selfMdnsRecord.data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }


    struct serverOnNetwork *listEntry;
    /* The servername is servername + hostname. It is the same which we get
     * through mDNS and therefore we need to match servername */
    UA_StatusCode retval =
        UA_DiscoveryManager_addEntryToServersOnNetwork(dm, serverName, &listEntry);
    if(retval != UA_STATUSCODE_GOOD &&
       retval != UA_STATUSCODE_BADALREADYEXISTS)
        return retval;

    /* If entry is already in list, skip initialization of capabilities and txt+srv */
    if(retval != UA_STATUSCODE_BADALREADYEXISTS) {
        listEntry->group = avahi_entry_group_new(mdnsPrivateData.client, entry_group_callback, dm);
        if(!listEntry->group) {
            UA_free(listEntry);
            return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
        }
        /* if capabilitiesSize is 0, then add default cap 'NA' */
        listEntry->serverOnNetwork.serverCapabilitiesSize = UA_MAX(1, capabilitiesSize);
        listEntry->serverOnNetwork.serverCapabilities = (UA_String *)
            UA_Array_new(listEntry->serverOnNetwork.serverCapabilitiesSize,
                         &UA_TYPES[UA_TYPES_STRING]);
        if(!listEntry->serverOnNetwork.serverCapabilities)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        if(capabilitiesSize == 0) {
            UA_String na;
            na.length = 2;
            na.data = (UA_Byte *) (uintptr_t) "NA";
            UA_String_copy(&na, &listEntry->serverOnNetwork.serverCapabilities[0]);
        } else {
            for(size_t i = 0; i < capabilitiesSize; i++)
                UA_String_copy(&capabilites[i],
                               &listEntry->serverOnNetwork.serverCapabilities[i]);
        }

        listEntry->txtSet = true;

        const size_t newUrlSize = strlen("opc.tcp://") + hostname.length + strlen(".local") + strlen(":port") + path.length + 1;
        UA_STACKARRAY(char, newUrl, newUrlSize);
        memset(newUrl, 0, newUrlSize);
        if(path.length > 0) {
            mp_snprintf(newUrl, newUrlSize, "opc.tcp://%S.local:%d/%S", hostname, port, path);
        } else {
            mp_snprintf(newUrl, newUrlSize, "opc.tcp://%S.local:%d", hostname, port);
        }
        listEntry->serverOnNetwork.discoveryUrl = UA_String_fromChars(newUrl);
        listEntry->srvSet = true;
    }

    /* Prepare the TXT records */
    AvahiStringList *txt = NULL;
    /* Handle 'path' */
    if(handle_path(&txt, path) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "Failed to add TXT record for %S", serverName);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    /* Handle 'caps' */
    if (capabilitiesSize > 0) {
        if(handle_capabilities(&txt, capabilites, capabilitiesSize) != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                         "Failed to add TXT record for %s", serviceDomain);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    } else {
        txt = avahi_string_list_add(txt, "caps=NA");
    }

    /* prepare hostname for avahi */
    size_t hostnameLen = hostname.length + strlen(".local");
    char *hostnameStr = (char *)avahi_malloc(hostnameLen + 1);
    if(!hostnameStr) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "Failed to add TXT record for %s", serviceDomain);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    memcpy(hostnameStr, hostname.data, hostname.length);
    memcpy(hostnameStr + hostname.length, ".local", strlen(".local"));
    hostnameStr[hostnameLen] = '\0';

    /*Add the service with TXT records using default host and domain */
    int ret;
    if((ret = avahi_entry_group_add_service_strlst(
            listEntry->group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
            AVAHI_PUBLISH_USE_MULTICAST, serviceDomain, "_opcua-tcp._tcp", NULL, hostnameStr,
            port, txt)) < 0) {
        if(ret == AVAHI_ERR_COLLISION) {
            UA_Discovery_multicastConflict(serviceDomain, dm);
        } else {
            UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                         "Failed to add TXT record for %s", serviceDomain);
            goto cleanup;
        }
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "Failed to add TXT record for %s", serviceDomain);
        goto cleanup;
    }
    avahi_free(hostnameStr);

    if (avahi_entry_group_commit(listEntry->group) < 0) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                        "Failed to commit entry group for %s", serviceDomain);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;

cleanup:
    avahi_free(hostnameStr);
    return UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
UA_Discovery_removeRecord(UA_DiscoveryManager *dm, const UA_String servername,
                          const UA_String hostname, UA_UInt16 port,
                          UA_Boolean removeTxt) {
    /* use a limit for the hostname length to make sure full string fits into 63
     * chars (limited by DNS spec) */
    if(hostname.length == 0 || servername.length == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    if(hostname.length + servername.length + 1 > 63) { /* include dash between servername-hostname */
        UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    }

    /* [servername]-[hostname] */
    char serviceDomain[63];
    createServiceDomain(serviceDomain, 63, servername, hostname);

    UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: remove record for domain: %s",
                serviceDomain);

    UA_String serverName = UA_String_fromChars(serviceDomain);
    UA_StatusCode retval =
        UA_DiscoveryManager_removeEntryFromServersOnNetwork(dm, serverName);
    return retval;
}

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */
