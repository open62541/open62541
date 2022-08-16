/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

#ifndef UA_ENABLE_AMALGAMATION
#include "mdnsd/libmdnsd/xht.h"
#include "mdnsd/libmdnsd/sdtxt.h"
#endif

#ifdef _WIN32
/* inet_ntoa is deprecated on MSVC but used for compatibility */
# define _WINSOCK_DEPRECATED_NO_WARNINGS
# include <winsock2.h>
# include <iphlpapi.h>
# include <ws2tcpip.h>
#else
# include <sys/time.h> // for struct timeval
# include <netinet/in.h> // for struct ip_mreq
# if defined(UA_HAS_GETIFADDR)
#  include <ifaddrs.h>
# endif /* UA_HAS_GETIFADDR */
# include <net/if.h> /* for IFF_RUNNING */
# include <netdb.h> // for recvfrom in cygwin
#endif

static struct serverOnNetwork_list_entry *
mdns_record_add_or_get(UA_Server *server, const char *record, const char *serverName,
                       size_t serverNameLen, UA_Boolean createNew) {
    UA_UInt32 hashIdx = UA_ByteString_hash(0, (const UA_Byte*)record, strlen(record)) % SERVER_ON_NETWORK_HASH_SIZE;
    struct serverOnNetwork_hash_entry *hash_entry = server->discoveryManager.serverOnNetworkHash[hashIdx];

    while(hash_entry) {
        size_t maxLen;
        if(serverNameLen > hash_entry->entry->serverOnNetwork.serverName.length)
            maxLen = hash_entry->entry->serverOnNetwork.serverName.length;
        else
            maxLen = serverNameLen;

        if(strncmp((char *) hash_entry->entry->serverOnNetwork.serverName.data,
                   serverName, maxLen) == 0)
            return hash_entry->entry;
        hash_entry = hash_entry->next;
    }

    if(!createNew)
        return NULL;

    struct serverOnNetwork_list_entry *listEntry;
    UA_StatusCode retval = UA_DiscoveryManager_addEntryToServersOnNetwork(server, record, serverName, serverNameLen, &listEntry);
    if (retval != UA_STATUSCODE_GOOD)
        return NULL;

    return listEntry;
}


UA_StatusCode
UA_DiscoveryManager_addEntryToServersOnNetwork(UA_Server *server, const char *fqdnMdnsRecord, const char *serverName,
                                               size_t serverNameLen, struct serverOnNetwork_list_entry **addedEntry) {

    struct serverOnNetwork_list_entry *entry =
            mdns_record_add_or_get(server, fqdnMdnsRecord, serverName,
                                   serverNameLen, UA_FALSE);
    if (entry) {
        if (addedEntry != NULL)
            *addedEntry = entry;
        return UA_STATUSCODE_BADALREADYEXISTS;
    }


    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: Add entry to ServersOnNetwork: %s (%*.s)", fqdnMdnsRecord, (int)serverNameLen, serverName);

    struct serverOnNetwork_list_entry *listEntry = (serverOnNetwork_list_entry*)
            UA_malloc(sizeof(struct serverOnNetwork_list_entry));
    if (!listEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    listEntry->created = UA_DateTime_now();
    listEntry->pathTmp = NULL;
    listEntry->txtSet = false;
    listEntry->srvSet = false;
    UA_ServerOnNetwork_init(&listEntry->serverOnNetwork);
    listEntry->serverOnNetwork.recordId = server->discoveryManager.serverOnNetworkRecordIdCounter;
    listEntry->serverOnNetwork.serverName.data = (UA_Byte*)UA_malloc(serverNameLen);
    if (!listEntry->serverOnNetwork.serverName.data) {
        UA_free(listEntry);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    listEntry->serverOnNetwork.serverName.length = serverNameLen;
    memcpy(listEntry->serverOnNetwork.serverName.data, serverName, serverNameLen);
    server->discoveryManager.serverOnNetworkRecordIdCounter++;
    if(server->discoveryManager.serverOnNetworkRecordIdCounter == 0)
        server->discoveryManager.serverOnNetworkRecordIdLastReset = UA_DateTime_now();
    listEntry->lastSeen = UA_DateTime_nowMonotonic();

    /* add to hash */
    UA_UInt32 hashIdx = UA_ByteString_hash(0, (const UA_Byte*)fqdnMdnsRecord, strlen(fqdnMdnsRecord)) % SERVER_ON_NETWORK_HASH_SIZE;
    struct serverOnNetwork_hash_entry *newHashEntry = (struct serverOnNetwork_hash_entry*)
            UA_malloc(sizeof(struct serverOnNetwork_hash_entry));
    if (!newHashEntry) {
        UA_free(listEntry->serverOnNetwork.serverName.data);
        UA_free(listEntry);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    newHashEntry->next = server->discoveryManager.serverOnNetworkHash[hashIdx];
    server->discoveryManager.serverOnNetworkHash[hashIdx] = newHashEntry;
    newHashEntry->entry = listEntry;

    LIST_INSERT_HEAD(&server->discoveryManager.serverOnNetwork, listEntry, pointers);

    if (addedEntry != NULL)
        *addedEntry = listEntry;

    return UA_STATUSCODE_GOOD;
}

#ifdef _WIN32

/* see http://stackoverflow.com/a/10838854/869402 */
static IP_ADAPTER_ADDRESSES *
getInterfaces(const UA_Server *server) {
    IP_ADAPTER_ADDRESSES* adapter_addresses = NULL;

    /* Start with a 16 KB buffer and resize if needed - multiple attempts in
     * case interfaces change while we are in the middle of querying them. */
    DWORD adapter_addresses_buffer_size = 16 * 1024;
    for(size_t attempts = 0; attempts != 3; ++attempts) {
        /* todo: malloc may fail: return a statuscode */
        adapter_addresses = (IP_ADAPTER_ADDRESSES*)UA_malloc(adapter_addresses_buffer_size);
        if(!adapter_addresses) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "GetAdaptersAddresses out of memory");
            adapter_addresses = NULL;
            break;
        }
        DWORD error = GetAdaptersAddresses(AF_UNSPEC,
                                           GAA_FLAG_SKIP_ANYCAST |
                                           GAA_FLAG_SKIP_DNS_SERVER |
                                           GAA_FLAG_SKIP_FRIENDLY_NAME,
                                           NULL, adapter_addresses,
                                           &adapter_addresses_buffer_size);

        if(ERROR_SUCCESS == error) {
            break;
        } else if (ERROR_BUFFER_OVERFLOW == error) {
            /* Try again with the new size */
            UA_free(adapter_addresses);
            adapter_addresses = NULL;
            continue;
        }

        /* Unexpected error */
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "GetAdaptersAddresses returned an unexpected error. "
                     "Not setting mDNS A records.");
        UA_free(adapter_addresses);
        adapter_addresses = NULL;
        break;
    }
    return adapter_addresses;
}

#endif /* _WIN32 */

UA_StatusCode
UA_DiscoveryManager_removeEntryFromServersOnNetwork(UA_Server *server, const char *fqdnMdnsRecord, const char *serverName,
                                                    size_t serverNameLen) {

    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "Multicast DNS: Remove entry from ServersOnNetwork: %s (%*.s)", fqdnMdnsRecord, (int)serverNameLen, serverName);

    struct serverOnNetwork_list_entry *entry =
            mdns_record_add_or_get(server, fqdnMdnsRecord, serverName,
                                   serverNameLen, UA_FALSE);
    if (!entry)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_String recordStr;
    // Cast away const because otherwise the pointer cannot be assigned.
    // Be careful what you do with recordStr!
    recordStr.data = (UA_Byte*)(uintptr_t)fqdnMdnsRecord;
    recordStr.length = strlen(fqdnMdnsRecord);

    /* remove from hash */
    UA_UInt32 hashIdx = UA_ByteString_hash(0, (const UA_Byte*)recordStr.data, recordStr.length) % SERVER_ON_NETWORK_HASH_SIZE;
    struct serverOnNetwork_hash_entry *hash_entry = server->discoveryManager.serverOnNetworkHash[hashIdx];
    struct serverOnNetwork_hash_entry *prevEntry = hash_entry;
    while(hash_entry) {
        if(hash_entry->entry == entry) {
            if(server->discoveryManager.serverOnNetworkHash[hashIdx] == hash_entry)
                server->discoveryManager.serverOnNetworkHash[hashIdx] = hash_entry->next;
            else if(prevEntry)
                prevEntry->next = hash_entry->next;
            break;
        }
        prevEntry = hash_entry;
        hash_entry = hash_entry->next;
    }
    UA_free(hash_entry);

    if(server->discoveryManager.serverOnNetworkCallback &&
        !UA_String_equal(&server->discoveryManager.selfFqdnMdnsRecord, &recordStr))
        server->discoveryManager.serverOnNetworkCallback(&entry->serverOnNetwork, false,
                                    entry->txtSet, server->discoveryManager.serverOnNetworkCallbackData);

    /* Remove from list */
    LIST_REMOVE(entry, pointers);
    UA_ServerOnNetwork_clear(&entry->serverOnNetwork);
    if(entry->pathTmp)
        UA_free(entry->pathTmp);
    UA_free(entry);
    return UA_STATUSCODE_GOOD;
}

static void
mdns_append_path_to_url(UA_String *url, const char *path) {
    size_t pathLen = strlen(path);
    /* todo: malloc may fail: return a statuscode */
    char *newUrl = (char *)UA_malloc(url->length + pathLen);
    memcpy(newUrl, url->data, url->length);
    memcpy(newUrl + url->length, path, pathLen);
    UA_String_clear(url);
    url->length = url->length + pathLen;
    url->data = (UA_Byte *) newUrl;
}

static void
setTxt(UA_Server *server, const struct resource *r,
       struct serverOnNetwork_list_entry *entry) {
    entry->txtSet = true;
    xht_t *x = txt2sd(r->rdata, r->rdlength);
    char *path = (char *) xht_get(x, "path");
    char *caps = (char *) xht_get(x, "caps");

    size_t pathLen = path ? strlen(path) : 0;

    if(path && pathLen > 1) {
        if(!entry->srvSet) {
            /* txt arrived before SRV, thus cache path entry */
            if (!entry->pathTmp) {
                entry->pathTmp = (char*)UA_malloc(pathLen+1);
                if (!entry->pathTmp) {
                    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Cannot alloc memory for mDNS srv path");
                    return;
                }
                memcpy(&(entry->pathTmp), &path, pathLen);
                entry->pathTmp[pathLen] = '\0';
            }
        } else {
            /* SRV already there and discovery URL set. Add path to discovery URL */
            mdns_append_path_to_url(&entry->serverOnNetwork.discoveryUrl, path);
        }
    }

    if(caps && strlen(caps) > 0) {
        /* count comma in caps */
        size_t capsCount = 1;
        for(size_t i = 0; caps[i]; i++) {
            if(caps[i] == ',')
                capsCount++;
        }

        /* set capabilities */
        entry->serverOnNetwork.serverCapabilitiesSize = capsCount;
        entry->serverOnNetwork.serverCapabilities =
            (UA_String *) UA_Array_new(capsCount, &UA_TYPES[UA_TYPES_STRING]);

        for(size_t i = 0; i < capsCount; i++) {
            char *nextStr = strchr(caps, ',');
            size_t len = nextStr ? (size_t) (nextStr - caps) : strlen(caps);
            entry->serverOnNetwork.serverCapabilities[i].length = len;
            /* todo: malloc may fail: return a statuscode */
            entry->serverOnNetwork.serverCapabilities[i].data = (UA_Byte*)UA_malloc(len);
            memcpy(entry->serverOnNetwork.serverCapabilities[i].data, caps, len);
            if(nextStr)
                caps = nextStr + 1;
            else
                break;
        }
    }
    xht_free(x);
}

/* [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname]. */
static void
setSrv(UA_Server *server, const struct resource *r,
       struct serverOnNetwork_list_entry *entry) {
    entry->srvSet = true;


    /* The specification Part 12 says: The hostname maps onto the SRV record
     * target field. If the hostname is an IPAddress then it must be converted
     * to a domain name. If this cannot be done then LDS shall report an
     * error. */

    size_t srvNameLen = strlen(r->known.srv.name);
    if(srvNameLen > 0 && r->known.srv.name[srvNameLen - 1] == '.')
        /* cut off last dot */
        srvNameLen--;
    /* opc.tcp://[servername]:[port][path] */
    char *newUrl = (char*)UA_malloc(10 + srvNameLen + 8 + 1);
    if (!newUrl) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Cannot allocate char for discovery url. Out of memory.");
        return;
    }
    UA_snprintf(newUrl, 10 + srvNameLen + 8, "opc.tcp://%.*s:%d", (int) srvNameLen,
             r->known.srv.name, r->known.srv.port);

    entry->serverOnNetwork.discoveryUrl = UA_String_fromChars(newUrl);
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: found server: %.*s", (int)entry->serverOnNetwork.discoveryUrl.length, (char*)entry->serverOnNetwork.discoveryUrl.data);
    UA_free(newUrl);

    if(entry->pathTmp) {
        mdns_append_path_to_url(&entry->serverOnNetwork.discoveryUrl, entry->pathTmp);
        UA_free(entry->pathTmp);
    }
}

/* This will be called by the mDNS library on every record which is received */
void
mdns_record_received(const struct resource *r, void *data) {
    UA_Server *server = (UA_Server *) data;
    /* we only need SRV and TXT records */
    /* TODO: remove magic number */
    if((r->clazz != QCLASS_IN && r->clazz != QCLASS_IN + 32768) ||
       (r->type != QTYPE_SRV && r->type != QTYPE_TXT))
        return;

    /* we only handle '_opcua-tcp._tcp.' records */
    char *opcStr = strstr(r->name, "_opcua-tcp._tcp.");
    if(!opcStr)
        return;

    UA_String recordStr;
    recordStr.data = (UA_Byte*)r->name;
    recordStr.length = strlen(r->name);
    UA_Boolean isSelfAnnounce = UA_String_equal(&server->discoveryManager.selfFqdnMdnsRecord, &recordStr);
    if (isSelfAnnounce)
        // ignore itself
        return;

    /* Compute the length of the servername */
    size_t servernameLen = (size_t) (opcStr - r->name);
    if(servernameLen == 0)
        return;
    servernameLen--; /* remove point */

    /* Get entry */
    struct serverOnNetwork_list_entry *entry =
            mdns_record_add_or_get(server, r->name, r->name,
                                   servernameLen, r->ttl > 0);
    if(!entry)
        return;

    /* Check that the ttl is positive */
    if(r->ttl == 0) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Multicast DNS: remove server (TTL=0): %.*s",
                    (int)entry->serverOnNetwork.discoveryUrl.length,
                    entry->serverOnNetwork.discoveryUrl.data);
        UA_DiscoveryManager_removeEntryFromServersOnNetwork(server, r->name, r->name, servernameLen);
        return;
    }

    /* Update lastSeen */
    entry->lastSeen = UA_DateTime_nowMonotonic();

    /* TXT and SRV are already set */
    if(entry->txtSet && entry->srvSet) {
        // call callback for every mdns package we received.
        // This will also call the callback multiple times
        if (server->discoveryManager.serverOnNetworkCallback)
            server->discoveryManager.
                serverOnNetworkCallback(&entry->serverOnNetwork, true, entry->txtSet,
                                        server->discoveryManager.serverOnNetworkCallbackData);
        return;
    }

    /* Add the resources */
    if(r->type == QTYPE_TXT && !entry->txtSet)
        setTxt(server, r, entry);
    else if (r->type == QTYPE_SRV && !entry->srvSet)
        setSrv(server, r, entry);

    /* Call callback to announce a new server */
    if(entry->srvSet && server->discoveryManager.serverOnNetworkCallback)
        server->discoveryManager.
            serverOnNetworkCallback(&entry->serverOnNetwork, true, entry->txtSet,
                                    server->discoveryManager.serverOnNetworkCallbackData);
}

void
mdns_create_txt(UA_Server *server, const char *fullServiceDomain, const char *path,
                const UA_String *capabilites, const size_t capabilitiesSize,
                void (*conflict)(char *host, int type, void *arg)) {
    mdns_record_t *r = mdnsd_unique(server->discoveryManager.mdnsDaemon, fullServiceDomain,
                                    QTYPE_TXT, 600, conflict, server);
    xht_t *h = xht_new(11);
    char *allocPath = NULL;
    if(!path || strlen(path) == 0) {
        xht_set(h, "path", "/");
    } else {
        /* path does not contain slash, so add it here */
        size_t pathLen = strlen(path);
        if(path[0] == '/') {
            allocPath = (char*)UA_malloc(pathLen+1);
            if(!allocPath) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                             "Cannot alloc memory for txt path");
                return;
            }
            memcpy(allocPath, path, pathLen);
            allocPath[pathLen] = '\0';
        } else {
            allocPath = (char*)UA_malloc(pathLen + 2);
            if(!allocPath) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                             "Cannot alloc memory for txt path");
                return;
            }
            allocPath[0] = '/';
            memcpy(allocPath + 1, path, pathLen);
            allocPath[pathLen + 1] = '\0';
        }
        xht_set(h, "path", allocPath);
    }

    /* calculate max string length: */
    size_t capsLen = 0;
    for(size_t i = 0; i < capabilitiesSize; i++) {
        /* add comma or last \0 */
        capsLen += capabilites[i].length + 1;
    }

    char *caps = NULL;
    if(capsLen) {
        /* freed when xht_free is called */
        /* todo: malloc may fail: return a statuscode */
        caps = (char*)UA_malloc(sizeof(char) * capsLen);
        size_t idx = 0;
        for(size_t i = 0; i < capabilitiesSize; i++) {
            memcpy(caps + idx, (const char *) capabilites[i].data, capabilites[i].length);
            idx += capabilites[i].length + 1;
            caps[idx - 1] = ',';
        }
        caps[idx - 1] = '\0';

        xht_set(h, "caps", caps);
    } else {
        xht_set(h, "caps", "NA");
    }

    int txtRecordLength;
    unsigned char *packet = sd2txt(h, &txtRecordLength);
    if(allocPath)
        UA_free(allocPath);
    if(caps)
        UA_free(caps);
    xht_free(h);
    mdnsd_set_raw(server->discoveryManager.mdnsDaemon, r, (char *) packet,
                  (unsigned short) txtRecordLength);
    UA_free(packet);
}

mdns_record_t *
mdns_find_record(mdns_daemon_t *mdnsDaemon, unsigned short type,
                 const char *host, const char *rdname) {
    mdns_record_t *r = mdnsd_get_published(mdnsDaemon, host);
    if(!r)
        return NULL;

    /* search for the record with the correct ptr hostname */
    while(r) {
        const mdns_answer_t *data = mdnsd_record_data(r);
        if(data->type == type && strcmp(data->rdname, rdname) == 0)
            return r;
        r = mdnsd_record_next(r);
    }
    return NULL;
}

/* set record in the given interface */
static void
mdns_set_address_record_if(UA_DiscoveryManager *dm, const char *fullServiceDomain,
                           const char *localDomain, char *addr, UA_UInt16 addr_len) {
    /* [servername]-[hostname]._opcua-tcp._tcp.local. A [ip]. */
    mdns_record_t *r = mdnsd_shared(dm->mdnsDaemon, fullServiceDomain, QTYPE_A, 600);
    mdnsd_set_raw(dm->mdnsDaemon, r, addr, addr_len);

    /* [hostname]. A [ip]. */
    r = mdnsd_shared(dm->mdnsDaemon, localDomain, QTYPE_A, 600);
    mdnsd_set_raw(dm->mdnsDaemon, r, addr, addr_len);
}

/* Loop over network interfaces and run set_address_record on each */
#ifdef _WIN32

void mdns_set_address_record(UA_Server *server, const char *fullServiceDomain,
                             const char *localDomain) {
    IP_ADAPTER_ADDRESSES* adapter_addresses = getInterfaces(server);
    if(!adapter_addresses)
        return;

    /* Iterate through all of the adapters */
    IP_ADAPTER_ADDRESSES* adapter = adapter_addresses;
    for(; adapter != NULL; adapter = adapter->Next) {
        /* Skip loopback adapters */
        if(IF_TYPE_SOFTWARE_LOOPBACK == adapter->IfType)
            continue;

        /* Parse all IPv4 and IPv6 addresses */
        IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress;
        for(; NULL != address; address = address->Next) {
            int family = address->Address.lpSockaddr->sa_family;
            if(AF_INET == family) {
                SOCKADDR_IN* ipv4 = (SOCKADDR_IN*)(address->Address.lpSockaddr); /* IPv4 */
                mdns_set_address_record_if(&server->discoveryManager, fullServiceDomain,
                                           localDomain, (char *)&ipv4->sin_addr, 4);
            } else if(AF_INET6 == family) {
                /* IPv6 */
#if 0
                SOCKADDR_IN6* ipv6 = (SOCKADDR_IN6*)(address->Address.lpSockaddr);

                char str_buffer[INET6_ADDRSTRLEN] = {0};
                inet_ntop(AF_INET6, &(ipv6->sin6_addr), str_buffer, INET6_ADDRSTRLEN);

                std::string ipv6_str(str_buffer);

                /* Detect and skip non-external addresses */
                UA_Boolean is_link_local(false);
                UA_Boolean is_special_use(false);

                if(0 == ipv6_str.find("fe")) {
                    char c = ipv6_str[2];
                    if(c == '8' || c == '9' || c == 'a' || c == 'b')
                        is_link_local = true;
                } else if (0 == ipv6_str.find("2001:0:")) {
                    is_special_use = true;
                }

                if(!(is_link_local || is_special_use))
                    ipAddrs.mIpv6.push_back(ipv6_str);
#endif
            }
        }
    }

    /* Cleanup */
    UA_free(adapter_addresses);
    adapter_addresses = NULL;
}

#elif defined(UA_HAS_GETIFADDR)

void
mdns_set_address_record(UA_Server *server, const char *fullServiceDomain,
                        const char *localDomain) {
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    if(getifaddrs(&ifaddr) == -1) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "getifaddrs returned an unexpected error. Not setting mDNS A records.");
        return;
    }

    /* Walk through linked list, maintaining head pointer so we can free list later */
    int n;
    for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
        if(!ifa->ifa_addr)
            continue;

        if((strcmp("lo", ifa->ifa_name) == 0) ||
           !(ifa->ifa_flags & (IFF_RUNNING))||
           !(ifa->ifa_flags & (IFF_MULTICAST)))
            continue;

        /* IPv4 */
        if(ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* sa = (struct sockaddr_in*) ifa->ifa_addr;
            mdns_set_address_record_if(&server->discoveryManager, fullServiceDomain,
                                       localDomain, (char*)&sa->sin_addr.s_addr, 4);
        }

        /* IPv6 not implemented yet */
    }

    /* Clean up */
    freeifaddrs(ifaddr);
}
#else /* _WIN32 */

void
mdns_set_address_record(UA_Server *server, const char *fullServiceDomain,
                        const char *localDomain) {

    if (server->config.mdnsIpAddressListSize == 0) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "If UA_HAS_GETIFADDR is false, config.mdnsIpAddressList must be set");
        return;
    }

    for(size_t i=0; i<server->config.mdnsIpAddressListSize; i++) {
        mdns_set_address_record_if(&server->discoveryManager, fullServiceDomain,
                                   localDomain, (char*)&server->config.mdnsIpAddressList[i], 4);
    }
}

#endif /* _WIN32 */

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */
