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
#include <open62541/plugin/servercomponent.h>

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

typedef struct ServerOnNetwork {
    struct ServerOnNetwork *next;
    UA_ServerOnNetwork serverOnNetwork;
    AvahiEntryGroup *group;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char* pathTmp;
} ServerOnNetwork;

typedef struct {
    UA_MdnsServerComponent msc;

    UA_Logger *logging; /* Shortcut from msc->server->logging */

    UA_Boolean mdnsMainSrvAdded;

    AvahiClient *client;
    AvahiSimplePoll *simple_poll;
    AvahiServiceBrowser *browser;

    ServerOnNetwork *serverList;

    /* Name of server itself. Used to detect if received mDNS
     * message was from itself */
    UA_String selfMdnsRecord;

    UA_UInt64 pollCallbackId;
} AvahiServerComponent;

static UA_StatusCode
addEntryToServersOnNetwork(AvahiServerComponent *asc,
                           UA_String serverName,
                           ServerOnNetwork **addedEntry);

static void
multicastConflict(UA_String name, AvahiServerComponent *asc) {
    /* In case logging is disabled */
    (void)name;
    UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                 "Multicast DNS name conflict detected: '%S'", name);
}

static ServerOnNetwork *
mdns_record_add_or_get(AvahiServerComponent *asc, UA_String serverName,
                       UA_Boolean createNew) {
    for(ServerOnNetwork *son = asc->serverList; son; son = son->next) {
        if(UA_String_equal(&serverName, &son->serverOnNetwork.serverName))
            return son;
    }

    if(!createNew)
        return NULL;

    ServerOnNetwork *entry = NULL;
    addEntryToServersOnNetwork(asc, serverName, &entry);
    return entry;
}

static UA_StatusCode
addEntryToServersOnNetwork(AvahiServerComponent *asc, UA_String serverName,
                           ServerOnNetwork **addedEntry) {
    /* Lookup without creating new */
    ServerOnNetwork *entry = mdns_record_add_or_get(asc, serverName, false);
    if(entry) {
        if(addedEntry != NULL)
            *addedEntry = entry;
        return UA_STATUSCODE_BADALREADYEXISTS;
    }

    UA_LOG_DEBUG(asc->logging, UA_LOGCATEGORY_SERVER,
                 "Multicast DNS: Add entry to ServersOnNetwork: (%S)",
                 serverName);

    ServerOnNetwork *listEntry = (ServerOnNetwork*)
        UA_calloc(1, sizeof(ServerOnNetwork));
    if(!listEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode res = UA_String_copy(&serverName,
                                       &listEntry->serverOnNetwork.serverName);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(listEntry);
        return res;
    }

    /* Add to the server */
    UA_Server *server = asc->msc.serverComponent.server;
    res = UA_Server_registerServerOnNetwork(server, &listEntry->serverOnNetwork, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ServerOnNetwork_clear(&listEntry->serverOnNetwork);
        UA_free(listEntry);
        return res;
    }

    /* Add to the linked list */
    listEntry->next = asc->serverList;
    asc->serverList = listEntry;

    /* Return pointer if requested */
    if(addedEntry != NULL)
        *addedEntry = listEntry;
    return UA_STATUSCODE_GOOD;
}

static void
entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state,
                     AVAHI_GCC_UNUSED void *userdata) {
    if(!userdata)
        return;

    AvahiServerComponent *asc = (AvahiServerComponent*)userdata;
    if(!g) {
        UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "AvahiEntryGroup or userdata is NULL");
        return;
    }

    /* Called whenever the entry group state changes */
    /* Find the registered service on network */
    ServerOnNetwork *current = asc->serverList;
    while(current && current->group != g)
        current = current->next;

    int err;
    switch(state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED:
            UA_LOG_INFO(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                        "Entry group established.");
            break;
        case AVAHI_ENTRY_GROUP_COLLISION: {
            if(!current) {
                UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                             "Entry group collision for unknown group.");
                break;
            }
            multicastConflict(current->serverOnNetwork.serverName, asc);
            break;
        }
        case AVAHI_ENTRY_GROUP_FAILURE:
            err = avahi_client_errno(avahi_entry_group_get_client(g));
            UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                         "Entry group failure: %s", avahi_strerror(err));

            /* Some kind of failure happened while we were registering our
             * services */
            avahi_simple_poll_quit(asc->simple_poll);
            break;
        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            break;
        default:
            UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                         "Unknown entry group state");
            break;
    }
}

static UA_StatusCode
removeEntryFromServersOnNetwork(AvahiServerComponent *asc,
                                UA_String serverName) {
    UA_LOG_DEBUG(asc->logging, UA_LOGCATEGORY_SERVER,
                 "Multicast DNS: Remove entry from ServersOnNetwork: %S",
                 serverName);

    /* Remove from list */
    ServerOnNetwork *entry = mdns_record_add_or_get(asc, serverName, false);
    if(entry) {
        for(ServerOnNetwork *son = asc->serverList; son; son = son->next) {
            if(son->next == entry) {
                son->next = entry->next;
                break;
            }
        }
        UA_ServerOnNetwork_clear(&entry->serverOnNetwork);
        if(entry->pathTmp)
            UA_free(entry->pathTmp);
        UA_free(entry);
    }

    /* Remove from the server */
    UA_Server *server = asc->msc.serverComponent.server;
    UA_Server_deregisterServerOnNetwork(server, serverName, NULL);

    return UA_STATUSCODE_GOOD;
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
addRecord(AvahiServerComponent *asc, const UA_String servername,
          const UA_String hostname, UA_UInt16 port,
          const UA_String path, const UA_DiscoveryProtocol protocol,
          const UA_String* capabilites, const size_t capabilitiesSize,
          UA_Boolean isSelf);

/* Create a mDNS Record for the given server info with TTL=0 and adds it to the
 * mDNS output queue.
 *
 * Additionally this method also removes the given server from the internal
 * serversOnNetwork list so that a client gets the updated data when calling
 * FindServersOnNetwork. */
static UA_StatusCode
removeRecord(AvahiServerComponent *asc, const UA_String servername,
             const UA_String hostname, UA_UInt16 port);

static UA_StatusCode
addMdnsRecordForNetworkLayer(AvahiServerComponent *asc, const UA_String serverName,
                             const UA_String *discoveryUrl) {
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode retval =
        UA_parseEndpointUrl(discoveryUrl, &hostname, &port, &path);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url is invalid: %S", *discoveryUrl);
        return retval;
    }

    UA_Server *server = asc->msc.serverComponent.server;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    if(hostname.length == 0) {
        /* get host name used by avahi */
        const char *hoststr = avahi_client_get_host_name(asc->client);
        if(!hoststr) {
            UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                           "Cannot get hostname from avahi");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        hostname = UA_String_fromChars(hoststr);
    }
    retval = addRecord(asc, serverName, hostname, port,
                       path, UA_DISCOVERY_TCP,
                       config->mdnsConfig.serverCapabilities,
                       config->mdnsConfig.serverCapabilitiesSize, true);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Cannot add mDNS Record: %s",
                       UA_StatusCode_name(retval));
        return retval;
    }
    return UA_STATUSCODE_GOOD;
}

/* Callback when the client state changes */
static
void client_callback(AvahiClient *c, AvahiClientState state, void *userdata) {
    /* Handle state changes if necessary */
    AvahiServerComponent *asc = (AvahiServerComponent *)userdata;
    UA_LOG_INFO(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                "Avahi client state changed to %d", state);
}

/* Called whenever a service has been resolved successfully or timed out */
static void
resolve_callback(AvahiServiceResolver *r, AVAHI_GCC_UNUSED AvahiIfIndex interface,
                 AVAHI_GCC_UNUSED AvahiProtocol protocol, AvahiResolverEvent event,
                 const char *name, const char *type, const char *domain,
                 const char *host_name, const AvahiAddress *address, uint16_t port,
                 AvahiStringList *txt, AvahiLookupResultFlags flags,
                 AVAHI_GCC_UNUSED void *userdata) {

    AvahiServerComponent *asc = (AvahiServerComponent *)userdata;

    int err;
    switch(event) {
        case AVAHI_RESOLVER_FAILURE:
            err = avahi_client_errno(avahi_service_resolver_get_client(r));
            UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                         "Failed to resolve service '%s' of type '%s' in "
                         "domain '%s': %s", name, type, domain,
                         avahi_strerror(err));
            break;

        case AVAHI_RESOLVER_FOUND: {
            char a[AVAHI_ADDRESS_STR_MAX];
            avahi_address_snprint(a, sizeof(a), address);

            /* Ignore own name */
            UA_String serverName = UA_STRING((char*)(uintptr_t)name);
            if(UA_String_equal(&asc->selfMdnsRecord, &serverName))
                return;

            ServerOnNetwork *listEntry;
            UA_StatusCode res =
                addEntryToServersOnNetwork(asc, serverName, &listEntry);
            if(res == UA_STATUSCODE_BADALREADYEXISTS) {
                UA_LOG_DEBUG(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                             "Server already in ServersOnNetwork: %s", name);
                return;
            }
            if(res != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                             "Failed to add server to ServersOnNetwork: %s",
                             UA_StatusCode_name(res));
                return;
            }

            UA_LOG_INFO(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                        "Service '%s' of type '%s' in domain '%s':",
                        name, type, domain);

            UA_LOG_INFO(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                        "  %s:%u (%s)", host_name, port, a);

            /* Add discoveryUrl opc.tcp://[servername]:[port][path] */
            UA_String path = UA_STRING_NULL;
            for(; txt; txt = avahi_string_list_get_next(txt)) {
                char *value = NULL;
                char *key = NULL;
                if(avahi_string_list_get_pair(txt, &key, &value, NULL) < 0)
                    break;

                /* Add path if it is more then just a single slash */
                if(strcmp(key, "path") == 0 && strlen(value) > 1)
                    path = UA_STRING_ALLOC(value);

                avahi_free(key);
                avahi_free(value);
                if(path.length > 0)
                    break;
            }

            UA_String_clear(&listEntry->serverOnNetwork.discoveryUrl);
            UA_String_format(&listEntry->serverOnNetwork.discoveryUrl,
                             "opc.tcp://%s:%u%S", host_name, port, path);
            UA_String_clear(&path);
            break;
        }

        default:
            UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                        "Invalid avahi resolver event %d", event);
            break;

    }
    avahi_service_resolver_free(r);
}

static void
browse_callback(AvahiServiceBrowser *b, AvahiIfIndex interface,
                AvahiProtocol protocol, AvahiBrowserEvent event,
                const char *name, const char *type, const char *domain,
                AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void *userdata) {
    AvahiServerComponent *asc = (AvahiServerComponent*)userdata;
    int err;
    switch(event) {
        case AVAHI_BROWSER_FAILURE:
            err = avahi_client_errno(avahi_service_browser_get_client(b));
            UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                         "Browser failure: %s", avahi_strerror(err));
            avahi_simple_poll_quit(asc->simple_poll);
            return;
        case AVAHI_BROWSER_NEW:
            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */
            if(!(avahi_service_resolver_new(asc->client, interface, protocol,
                                            name, type, domain, AVAHI_PROTO_INET,
                                            AVAHI_LOOKUP_USE_MULTICAST,
                                            resolve_callback, asc))) {
                err = avahi_client_errno(asc->client);
                UA_LOG_INFO(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                            "Failed to resolve service '%s' of type '%s' in "
                            "domain '%s': %s", name, type, domain,
                            avahi_strerror(err));
            }
            break;
        case AVAHI_BROWSER_REMOVE:
            UA_LOG_INFO(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                        "Service '%s' of type '%s' in domain '%s' removed",
                        name, type, domain);
            UA_String nameStr = UA_STRING_ALLOC(name);
            removeEntryFromServersOnNetwork(asc, nameStr);
            UA_String_clear(&nameStr);
            break;
        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED: {
            char *b = (event == AVAHI_BROWSER_CACHE_EXHAUSTED) ?
                "CACHE_EXHAUSTED" : "ALL_FOR_NOW";
            UA_LOG_INFO(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                        "Browser %s", b);
            break;
        }
    }
}

static void
AvahiServerComponent_announce(UA_MdnsServerComponent *msc,
                              const UA_ServerOnNetwork *son,
                              const UA_KeyValueMap *params) {
    AvahiServerComponent *asc = (AvahiServerComponent*)msc;
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res =
        UA_parseEndpointUrl(&son->discoveryUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url invalid: %S", son->discoveryUrl);
        return;
    }

    res = addRecord(asc, son->serverName, hostname, port, path, UA_DISCOVERY_TCP,
                    son->serverCapabilities, son->serverCapabilitiesSize, false);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Could not add mDNS record for hostname %S",
                       son->serverName);
}

static void
AvahiServerComponent_retract(UA_MdnsServerComponent *msc,
                             const UA_String serverName,
                             const UA_String discoveryUrl,
                             const UA_KeyValueMap *params) {
    AvahiServerComponent *asc = (AvahiServerComponent*)msc;
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res =
        UA_parseEndpointUrl(&discoveryUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url invalid: %S", discoveryUrl);
        return;
    }

    res = removeRecord(asc, serverName, hostname, port);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Could not remove mDNS record for hostname %S",
                       serverName);
}

static void
pollAvahiMdns(UA_Server *server, void *data) {
    AvahiServerComponent *asc = (AvahiServerComponent*)data;
    int ret = avahi_simple_poll_iterate(asc->simple_poll, 0);
    if(ret < 0) {
        UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Error in avahi_simple_poll_iterate: %s",
                     avahi_strerror(ret));
    }
}

/* Create a service domain with the format [servername]-[hostname]._opcua-tcp._tcp.local. */
static void
createServiceDomain(UA_String *outServiceDomain,
                    UA_String servername,
                    UA_String hostname) {
    /* Can we use hostname and servername with full length? */
    size_t maxLen = outServiceDomain->length - 1;
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
        UA_String_format(outServiceDomain, "%S-%S", servername, hostname);
        offset = servername.length + hostname.length + 1;
        /* Replace all dots with minus. Otherwise mDNS is not valid */
        for(size_t i = servername.length+1; i < offset; i++) {
            if(outServiceDomain->data[i] == '.')
                outServiceDomain->data[i] = '-';
        }
    } else {
        *outServiceDomain = servername;
    }
}

/* Check if mDNS already has an entry for given hostname and port combination */
static UA_Boolean
recordExists(AvahiServerComponent *asc, UA_String serviceDomain,
             unsigned short port, const UA_DiscoveryProtocol protocol) {
    for(ServerOnNetwork *son = asc->serverList; son; son = son->next) {
        if(UA_String_equal(&son->serverOnNetwork.serverName, &serviceDomain))
            return true;
    }
    return false;
}

static UA_StatusCode
handle_path(AvahiStringList **txt, const UA_String path) {
    if(!path.data || path.length == 0) {
        *txt = avahi_string_list_add(*txt, "path=/");
        return UA_STATUSCODE_GOOD;
    }

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
addRecord(AvahiServerComponent *asc, const UA_String servername,
          const UA_String hostname, UA_UInt16 port,
          const UA_String path, const UA_DiscoveryProtocol protocol,
          const UA_String* capabilites, const size_t capabilitiesSize,
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
    if(hostname.length + servername.length + 1 > 63) {
        UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    } else if(hostname.length > 63) {
        UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Hostname length exceeds maximum "
                       "of 63 chars. It will be truncated.");
    }

    if(!asc->mdnsMainSrvAdded)
        asc->mdnsMainSrvAdded = true;

    /* Include dash between servername-hostname: [servername]-[hostname] */
    UA_Byte serviceDomainBuf[64];
    UA_String serviceDomain = {64, serviceDomainBuf};
    createServiceDomain(&serviceDomain, servername, hostname);

    UA_Boolean exists = recordExists(asc, serviceDomain, port, protocol);
    if(exists == true)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: add record for domain: %S", serviceDomain);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(isSelf && asc->selfMdnsRecord.length == 0) {
        retval = UA_String_copy(&serviceDomain, &asc->selfMdnsRecord);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* The servername is servername + hostname. It is the same which we get
     * through mDNS and therefore we need to match servername */
    ServerOnNetwork *entry;
    retval = addEntryToServersOnNetwork(asc, serviceDomain, &entry);
    if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADALREADYEXISTS)
        return retval;

    /* If entry is already in list, skip initialization of capabilities and
     * txt+srv */
    if(retval != UA_STATUSCODE_BADALREADYEXISTS) {
        entry->group =
            avahi_entry_group_new(asc->client, entry_group_callback, asc);
        if(!entry->group) {
            UA_free(entry);
            return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
        }

        /* If capabilitiesSize is 0, then add default cap 'NA' */
        entry->serverOnNetwork.serverCapabilitiesSize =
            UA_MAX(1, capabilitiesSize);
        entry->serverOnNetwork.serverCapabilities = (UA_String *)
            UA_Array_new(entry->serverOnNetwork.serverCapabilitiesSize,
                         &UA_TYPES[UA_TYPES_STRING]);
        if(!entry->serverOnNetwork.serverCapabilities)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        if(capabilitiesSize == 0) {
            UA_String na;
            na.length = 2;
            na.data = (UA_Byte *) (uintptr_t) "NA";
            UA_String_copy(&na, &entry->serverOnNetwork.serverCapabilities[0]);
        } else {
            for(size_t i = 0; i < capabilitiesSize; i++)
                UA_String_copy(&capabilites[i],
                               &entry->serverOnNetwork.serverCapabilities[i]);
        }

        entry->txtSet = true;

        if(path.length > 0) {
            UA_String_format(&entry->serverOnNetwork.discoveryUrl,
                             "opc.tcp://%S.local:%d/%S", hostname, port, path);
        } else {
            UA_String_format(&entry->serverOnNetwork.discoveryUrl,
                             "opc.tcp://%S.local:%d", hostname, port);
        }
        entry->srvSet = true;
    }

    /* Prepare the TXT records */
    AvahiStringList *txt = NULL;

    /* Handle 'path' */
    if(handle_path(&txt, path) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Failed to add TXT record for %S", serviceDomain);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Handle 'caps' */
    if(capabilitiesSize > 0) {
        retval = handle_capabilities(&txt, capabilites, capabilitiesSize);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                         "Failed to add TXT record for %s", serviceDomain);
            return retval;
        }
    } else {
        txt = avahi_string_list_add(txt, "caps=NA");
    }

    /* prepare hostname for avahi */
    size_t hostnameLen = hostname.length + strlen(".local");
    char *hostnameStr = (char *)avahi_malloc(hostnameLen + 1);
    if(!hostnameStr)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memcpy(hostnameStr, hostname.data, hostname.length);
    memcpy(hostnameStr + hostname.length, ".local", strlen(".local"));
    hostnameStr[hostnameLen] = '\0';

    /* Add the service with TXT records using default host and domain */
    int ret =
        avahi_entry_group_add_service_strlst(entry->group, AVAHI_IF_UNSPEC,
                                             AVAHI_PROTO_UNSPEC,
                                             AVAHI_PUBLISH_USE_MULTICAST,
                                             (char*)serviceDomainBuf,
                                             "_opcua-tcp._tcp",
                                             NULL, hostnameStr, port, txt);
    avahi_free(hostnameStr);
    if(ret < 0) {
        if(ret == AVAHI_ERR_COLLISION) {
            multicastConflict(serviceDomain, asc);
        } else {
            UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                         "Failed to add TXT record for %s", serviceDomain);
        }
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Commit to the multicast group */
    ret = avahi_entry_group_commit(entry->group);
    if(ret < 0) {
        UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Failed to commit entry group for %s",
                     serviceDomain);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeRecord(AvahiServerComponent *asc, const UA_String servername,
             const UA_String hostname, UA_UInt16 port) {
    /* Use a limit for the hostname length to make sure full string fits into 63
     * chars (limited by DNS spec) */
    if(hostname.length == 0 || servername.length == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    /* Include dash between servername-hostname */
    if(hostname.length + servername.length + 1 > 63) {
        UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Combination of hostname+servername "
                       "exceeds maximum of 62 chars. It will be truncated.");
    }

    /* [servername]-[hostname] */
    UA_Byte serviceDomainBuf[64];
    UA_String serviceDomain = {64, serviceDomainBuf};
    createServiceDomain(&serviceDomain, servername, hostname);

    UA_LOG_INFO(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: remove record for domain: %S", serviceDomain);

    return removeEntryFromServersOnNetwork(asc, serviceDomain);
}

static UA_StatusCode
AvahiServerComponent_start(UA_ServerComponent *sc) {
    AvahiServerComponent *asc = (AvahiServerComponent*)sc;

    UA_ServerConfig *config = UA_Server_getConfig(sc->server);
    asc->logging = config->logging;

    if(sc->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Cannot start multicast discovery that is already running");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    asc->simple_poll = avahi_simple_poll_new();
    if(!asc->simple_poll) {
        UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Failed to create avahi simple poll");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    int error;
    asc->client =
        avahi_client_new(avahi_simple_poll_get(asc->simple_poll),
                         AVAHI_CLIENT_NO_FAIL, client_callback,
                         asc, &error);
    if(!asc->client) {
        UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Failed to create avahi client: %s",
                     avahi_strerror(error));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add record for the server itself */
    UA_String appName = config->mdnsConfig.mdnsServerName;
    for(size_t i = 0; i < config->serverUrlsSize; i++)
        addMdnsRecordForNetworkLayer(asc, appName, &config->serverUrls[i]);

    asc->browser =
        avahi_service_browser_new(asc->client, AVAHI_IF_UNSPEC,
                                  AVAHI_PROTO_INET, "_opcua-tcp._tcp",
                                  NULL, AVAHI_LOOKUP_USE_MULTICAST,
                                  browse_callback, asc);

    /* Add a repeated callback to send out multicast messages */
    UA_Server_addRepeatedCallback(sc->server, pollAvahiMdns,
                                  asc, 1000.0, &asc->pollCallbackId);

    return UA_STATUSCODE_GOOD;
}

static void
AvahiServerComponent_stop(UA_ServerComponent *sc) {
    UA_Server *server = sc->server;
    AvahiServerComponent *asc = (AvahiServerComponent*)sc;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Already stopped */
    if(sc->state == UA_LIFECYCLESTATE_STOPPED)
        return;

    /* Commence async stopping */
    sc->state = UA_LIFECYCLESTATE_STOPPING;

    /* Add a repeated callback to send out multicast messages */
    UA_Server_removeRepeatedCallback(sc->server, asc->pollCallbackId);
    asc->pollCallbackId = 0;

    for(size_t i = 0; i < config->serverUrlsSize; i++) {
        UA_String hostname = UA_STRING_NULL;
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;

        UA_StatusCode retval =
            UA_parseEndpointUrl(&config->serverUrls[i],
                                &hostname, &port, &path);

        if(retval != UA_STATUSCODE_GOOD)
            continue;

        /* Get host name used by avahi */
        if(hostname.length == 0) {
            const char *hoststr = avahi_client_get_host_name(asc->client);
            if(!hoststr) {
                UA_LOG_WARNING(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                               "Cannot get hostname from avahi");
                continue;
            }
            hostname = UA_String_fromChars(hoststr);
        }

        removeRecord(asc, config->mdnsConfig.mdnsServerName, hostname, port);
    }

    /* Clean up avahi resources */
    if(asc->browser)
        avahi_service_browser_free(asc->browser);
    if(asc->client)
        avahi_client_free(asc->client);
    if(asc->simple_poll)
        avahi_simple_poll_free(asc->simple_poll);

    /* Stopping avahi is synchronous */
    sc->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
AvahiServerComponent_free(UA_ServerComponent *sc) {
    AvahiServerComponent *asc = (AvahiServerComponent*)sc;

    if(sc->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(asc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Cannot free multicast discovery that is not stopped");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Clean up the serverOnNetwork list */
    ServerOnNetwork *son;
    while((son = asc->serverList)) {
        asc->serverList = son->next;
        UA_ServerOnNetwork_clear(&son->serverOnNetwork);
        if(son->pathTmp)
            UA_free(son->pathTmp);
        UA_free(son);
    }

    UA_String_clear(&asc->selfMdnsRecord);

    return UA_STATUSCODE_GOOD;
}

UA_MdnsServerComponent *
UA_MdnsServerComponent_Avahi(void) {
    UA_MdnsServerComponent *msc = (UA_MdnsServerComponent*)
        UA_calloc(1, sizeof(AvahiServerComponent));
    if(!msc)
        return NULL;

    msc->serverComponent.serverComponentType = UA_SERVERCOMPONENTTYPE_MDNS;
    msc->serverComponent.name = UA_STRING("discovery-avahi");
    msc->serverComponent.start = AvahiServerComponent_start;
    msc->serverComponent.stop = AvahiServerComponent_stop;
    msc->serverComponent.free = AvahiServerComponent_free;
    msc->announce = AvahiServerComponent_announce;
    msc->retract = AvahiServerComponent_retract;

    return msc;
}

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */
