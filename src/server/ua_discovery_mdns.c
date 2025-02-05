/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

#include "ua_discovery.h"
#include "ua_server_internal.h"
#include "mdnsd/libmdnsd/mdnsd.h"
#ifdef UA_ENABLE_DISCOVERY_MULTICAST_MDNSD

#ifndef UA_ENABLE_AMALGAMATION
#include "mdnsd/libmdnsd/xht.h"
#include "mdnsd/libmdnsd/sdtxt.h"
#endif

#include "../deps/mp_printf.h"

#ifdef UA_ARCHITECTURE_WIN32
/* inet_ntoa is deprecated on MSVC but used for compatibility */
# define _WINSOCK_DEPRECATED_NO_WARNINGS
# include <winsock2.h>
# include <iphlpapi.h>
# include <ws2tcpip.h>
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/time.h> // for struct timeval
# include <netinet/in.h> // for struct ip_mreq
# if defined(UA_HAS_GETIFADDR)
#  include <ifaddrs.h>
# endif /* UA_HAS_GETIFADDR */
# include <net/if.h> /* for IFF_RUNNING */
# include <netdb.h> // for recvfrom in cygwin
#endif

#define UA_MAXMDNSRECVSOCKETS 8
#define SERVER_ON_NETWORK_HASH_SIZE 1000
typedef struct serverOnNetwork {
    LIST_ENTRY(serverOnNetwork) pointers;
    UA_ServerOnNetwork serverOnNetwork;
    UA_DateTime created;
    UA_DateTime lastSeen;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char* pathTmp;
} serverOnNetwork;

typedef struct serverOnNetwork_hash_entry {
    serverOnNetwork *entry;
    struct serverOnNetwork_hash_entry* next;
} serverOnNetwork_hash_entry;

typedef struct mdnsPrivate {
    mdns_daemon_t *mdnsDaemon;
    uintptr_t mdnsSendConnection;
    uintptr_t mdnsRecvConnections[UA_MAXMDNSRECVSOCKETS];
    size_t mdnsRecvConnectionsSize;
    /* hash mapping domain name to serverOnNetwork list entry */
    struct serverOnNetwork_hash_entry* serverOnNetworkHash[SERVER_ON_NETWORK_HASH_SIZE];
    LIST_HEAD(, serverOnNetwork) serverOnNetwork;
    /* Name of server itself. Used to detect if received mDNS
     * message was from itself */
    UA_String selfMdnsRecord;

    UA_UInt32 serverOnNetworkRecordIdCounter;
    UA_DateTime serverOnNetworkRecordIdLastReset;
} mdnsPrivate;


static UA_StatusCode
UA_DiscoveryManager_addEntryToServersOnNetwork(UA_DiscoveryManager *dm,
                                               const char *fqdnMdnsRecord,
                                               UA_String serverName,
                                               struct serverOnNetwork **addedEntry);


static mdnsPrivate mdnsPrivateData;

static struct serverOnNetwork *
mdns_record_add_or_get(UA_DiscoveryManager *dm, const char *record,
                       UA_String serverName, UA_Boolean createNew) {
    UA_UInt32 hashIdx = UA_ByteString_hash(0, (const UA_Byte*)record,
                                           strlen(record)) % SERVER_ON_NETWORK_HASH_SIZE;
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
        UA_DiscoveryManager_addEntryToServersOnNetwork(dm, record, serverName, &listEntry);
    if(res != UA_STATUSCODE_GOOD)
        return NULL;

    return listEntry;
}


static UA_StatusCode
UA_DiscoveryManager_addEntryToServersOnNetwork(UA_DiscoveryManager *dm,
                                               const char *fqdnMdnsRecord,
                                               UA_String serverName,
                                               struct serverOnNetwork **addedEntry) {
    struct serverOnNetwork *entry =
            mdns_record_add_or_get(dm, fqdnMdnsRecord, serverName, false);
    if(entry) {
        if(addedEntry != NULL)
            *addedEntry = entry;
        return UA_STATUSCODE_BADALREADYEXISTS;
    }

    UA_LOG_DEBUG(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: Add entry to ServersOnNetwork: %s (%S)",
                 fqdnMdnsRecord, serverName);

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
        mdnsPrivateData.serverOnNetworkRecordIdLastReset = el->dateTime_now(el);
    listEntry->lastSeen = el->dateTime_nowMonotonic(el);

    /* add to hash */
    UA_UInt32 hashIdx = UA_ByteString_hash(0, (const UA_Byte*)fqdnMdnsRecord,
                                           strlen(fqdnMdnsRecord)) % SERVER_ON_NETWORK_HASH_SIZE;
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

    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ARCHITECTURE_WIN32

/* see http://stackoverflow.com/a/10838854/869402 */
static IP_ADAPTER_ADDRESSES *
getInterfaces(UA_DiscoveryManager *dm) {
    IP_ADAPTER_ADDRESSES* adapter_addresses = NULL;

    /* Start with a 16 KB buffer and resize if needed - multiple attempts in
     * case interfaces change while we are in the middle of querying them. */
    DWORD adapter_addresses_buffer_size = 16 * 1024;
    for(size_t attempts = 0; attempts != 3; ++attempts) {
        /* todo: malloc may fail: return a statuscode */
        adapter_addresses = (IP_ADAPTER_ADDRESSES*)UA_malloc(adapter_addresses_buffer_size);
        if(!adapter_addresses) {
            UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
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
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                     "GetAdaptersAddresses returned an unexpected error. "
                     "Not setting mDNS A records.");
        UA_free(adapter_addresses);
        adapter_addresses = NULL;
        break;
    }
    return adapter_addresses;
}

#endif /* UA_ARCHITECTURE_WIN32 */

static UA_StatusCode
UA_DiscoveryManager_removeEntryFromServersOnNetwork(UA_DiscoveryManager *dm,
                                                    const char *fqdnMdnsRecord,
                                                    UA_String serverName) {
    UA_LOG_DEBUG(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                 "Multicast DNS: Remove entry from ServersOnNetwork: %s (%S)",
                 fqdnMdnsRecord, serverName);

    struct serverOnNetwork *entry =
            mdns_record_add_or_get(dm, fqdnMdnsRecord, serverName, false);
    if(!entry)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_String recordStr;
    // Cast away const because otherwise the pointer cannot be assigned.
    // Be careful what you do with recordStr!
    recordStr.data = (UA_Byte*)(uintptr_t)fqdnMdnsRecord;
    recordStr.length = strlen(fqdnMdnsRecord);

    /* remove from hash */
    UA_UInt32 hashIdx = UA_ByteString_hash(0, (const UA_Byte*)recordStr.data,
                                           recordStr.length) % SERVER_ON_NETWORK_HASH_SIZE;
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
        !UA_String_equal(&mdnsPrivateData.selfMdnsRecord, &recordStr))
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

static void
mdns_append_path_to_url(UA_String *url, const char *path) {
    size_t pathLen = strlen(path);
    size_t newUrlLen = url->length + pathLen; //size of the new url string incl. the path 
    /* todo: malloc may fail: return a statuscode */
    char *newUrl = (char *)UA_malloc(url->length + pathLen);
    memcpy(newUrl, url->data, url->length);
    memcpy(newUrl + url->length, path, pathLen);
    UA_String_clear(url);
    url->length = newUrlLen;
    url->data = (UA_Byte *) newUrl;
}

static void
setTxt(UA_DiscoveryManager *dm, const struct resource *r,
       struct serverOnNetwork *entry) {
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
                    UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "Cannot alloc memory for mDNS srv path");
                    return;
                }
                memcpy(entry->pathTmp, path, pathLen);
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
setSrv(UA_DiscoveryManager *dm, const struct resource *r,
       struct serverOnNetwork *entry) {
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
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Cannot allocate char for discovery url. Out of memory.");
        return;
    }

    mp_snprintf(newUrl, 10 + srvNameLen + 8, "opc.tcp://%.*s:%d",
                (int)srvNameLen, r->known.srv.name, r->known.srv.port);

    entry->serverOnNetwork.discoveryUrl = UA_String_fromChars(newUrl);
    UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: found server: %S",
                entry->serverOnNetwork.discoveryUrl);
    UA_free(newUrl);

    if(entry->pathTmp) {
        mdns_append_path_to_url(&entry->serverOnNetwork.discoveryUrl, entry->pathTmp);
        UA_free(entry->pathTmp);
        entry->pathTmp = NULL;
    }
}

/* This will be called by the mDNS library on every record which is received */
static void
mdns_record_received(const struct resource *r, void *data) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*) data;

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
    UA_Boolean isSelfAnnounce = UA_String_equal(&mdnsPrivateData.selfMdnsRecord, &recordStr);
    if(isSelfAnnounce)
        return; // ignore itself

    /* Extract the servername */
    size_t servernameLen = (size_t) (opcStr - r->name);
    if(servernameLen == 0)
        return;
    servernameLen--; /* remove point */
    UA_String serverName = {servernameLen, (UA_Byte*)r->name};

    /* Get entry */
    struct serverOnNetwork *entry =
        mdns_record_add_or_get(dm, r->name, serverName, r->ttl > 0);
    if(!entry)
        return;

    /* Check that the ttl is positive */
    if(r->ttl == 0) {
        UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                    "Multicast DNS: remove server (TTL=0): %S",
                    entry->serverOnNetwork.discoveryUrl);
        UA_DiscoveryManager_removeEntryFromServersOnNetwork(dm, r->name, serverName);
        return;
    }

    /* Update lastSeen */
    UA_EventLoop *el = dm->sc.server->config.eventLoop;
    entry->lastSeen = el->dateTime_nowMonotonic(el);

    /* TXT and SRV are already set */
    if(entry->txtSet && entry->srvSet) {
        // call callback for every mdns package we received.
        // This will also call the callback multiple times
        if(dm->serverOnNetworkCallback)
            dm->serverOnNetworkCallback(&entry->serverOnNetwork, true, entry->txtSet,
                                        dm->serverOnNetworkCallbackData);
        return;
    }

    /* Add the resources */
    if(r->type == QTYPE_TXT && !entry->txtSet)
        setTxt(dm, r, entry);
    else if (r->type == QTYPE_SRV && !entry->srvSet)
        setSrv(dm, r, entry);

    /* Call callback to announce a new server */
    if(entry->srvSet && dm->serverOnNetworkCallback)
        dm->serverOnNetworkCallback(&entry->serverOnNetwork, true, entry->txtSet,
                                    dm->serverOnNetworkCallbackData);
}

static void
mdns_create_txt(UA_DiscoveryManager *dm, const char *fullServiceDomain, const char *path,
                const UA_String *capabilites, const size_t capabilitiesSize,
                void (*conflict)(char *host, int type, void *arg)) {
    mdns_record_t *r = mdnsd_unique(mdnsPrivateData.mdnsDaemon, fullServiceDomain,
                                    QTYPE_TXT, 600, conflict, dm);
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
                UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                             "Cannot alloc memory for txt path");
                return;
            }
            memcpy(allocPath, path, pathLen);
            allocPath[pathLen] = '\0';
        } else {
            allocPath = (char*)UA_malloc(pathLen + 2);
            if(!allocPath) {
                UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
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
    mdnsd_set_raw(mdnsPrivateData.mdnsDaemon, r, (char *) packet,
                  (unsigned short) txtRecordLength);
    UA_free(packet);
}

static mdns_record_t *
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
    mdns_record_t *r = mdnsd_shared(mdnsPrivateData.mdnsDaemon, fullServiceDomain, QTYPE_A, 600);
    mdnsd_set_raw(mdnsPrivateData.mdnsDaemon, r, addr, addr_len);

    /* [hostname]. A [ip]. */
    r = mdnsd_shared(mdnsPrivateData.mdnsDaemon, localDomain, QTYPE_A, 600);
    mdnsd_set_raw(mdnsPrivateData.mdnsDaemon, r, addr, addr_len);
}

/* Loop over network interfaces and run set_address_record on each */
#ifdef UA_ARCHITECTURE_WIN32
static void
mdns_set_address_record(UA_DiscoveryManager *dm, const char *fullServiceDomain,
                        const char *localDomain) {
    IP_ADAPTER_ADDRESSES* adapter_addresses = getInterfaces(dm);
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
                mdns_set_address_record_if(dm, fullServiceDomain,
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

static void
mdns_set_address_record(UA_DiscoveryManager *dm, const char *fullServiceDomain,
                        const char *localDomain) {
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    if(getifaddrs(&ifaddr) == -1) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
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
            mdns_set_address_record_if(dm, fullServiceDomain,
                                       localDomain, (char*)&sa->sin_addr.s_addr, 4);
        }

        /* IPv6 not implemented yet */
    }

    /* Clean up */
    freeifaddrs(ifaddr);
}
#else /* UA_ARCHITECTURE_WIN32 */

void
mdns_set_address_record(UA_DiscoveryManager *dm, const char *fullServiceDomain,
                        const char *localDomain) {
    if(dm->sc.server->config.mdnsIpAddressListSize == 0) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                     "If UA_HAS_GETIFADDR is false, config.mdnsIpAddressList must be set");
        return;
    }

    for(size_t i=0; i< dm->sc.server->config.mdnsIpAddressListSize; i++) {
        mdns_set_address_record_if(dm, fullServiceDomain, localDomain,
                                   (char*)&dm->sc.server->config.mdnsIpAddressList[i], 4);
    }
}

#endif /* UA_ARCHITECTURE_WIN32 */

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
UA_DiscoveryManager_getServerOnNetworkList(UA_DiscoveryManager *dm) {
    serverOnNetwork* entry = LIST_FIRST(&mdnsPrivateData.serverOnNetwork);
    return entry ? &entry->serverOnNetwork : NULL;
}

UA_ServerOnNetwork*
UA_DiscoveryManager_getNextServerOnNetworkRecord(UA_DiscoveryManager *dm,
                                   UA_ServerOnNetwork *current) {
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

static int
discovery_multicastQueryAnswer(mdns_answer_t *a, void *arg);

static void
mdnsAddConnection(UA_DiscoveryManager *dm, uintptr_t connectionId,
                  UA_Boolean recv) {
    if(!recv) {
        mdnsPrivateData.mdnsSendConnection = connectionId;
        return;
    }
    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        if(mdnsPrivateData.mdnsRecvConnections[i] == connectionId)
            return;
    }

    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        if(mdnsPrivateData.mdnsRecvConnections[i] != 0)
            continue;
        mdnsPrivateData.mdnsRecvConnections[i] = connectionId;
        mdnsPrivateData.mdnsRecvConnectionsSize++;
        break;
    }
}

static void
mdnsRemoveConnection(UA_DiscoveryManager *dm, uintptr_t connectionId,
                     UA_Boolean recv) {
    if(mdnsPrivateData.mdnsSendConnection == connectionId) {
        mdnsPrivateData.mdnsSendConnection = 0;
        return;
    }
    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        if(mdnsPrivateData.mdnsRecvConnections[i] != connectionId)
            continue;
        mdnsPrivateData.mdnsRecvConnections[i] = 0;
        mdnsPrivateData.mdnsRecvConnectionsSize--;
        break;
    }
}

static void
MulticastDiscoveryCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                           void *_, void **connectionContext,
                           UA_ConnectionState state, const UA_KeyValueMap *params,
                           UA_ByteString msg, UA_Boolean recv) {
    UA_DiscoveryManager *dm = *(UA_DiscoveryManager**)connectionContext;

    if(state == UA_CONNECTIONSTATE_CLOSING) {
        mdnsRemoveConnection(dm, connectionId, recv);

        /* Fully stopped? Internally checks if all sockets are closed. */
        UA_DiscoveryManager_setState(dm, dm->sc.state);

        /* Restart mdns sockets if not shutting down */
        if(dm->sc.state == UA_LIFECYCLESTATE_STARTED)
            UA_DiscoveryManager_startMulticast(dm);

        return;
    }

    mdnsAddConnection(dm, connectionId, recv);

    if(msg.length == 0)
        return;

    /* Prepare the sockaddrinfo */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "remote-port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    const UA_String *address = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "remote-address"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!port || !address)
        return;

    char portStr[16];
    UA_UInt16 myPort = *port;
    for(size_t i = 0; i < 16; i++) {
        if(myPort == 0) {
            portStr[i] = 0;
            break;
        }
        unsigned char rem = (unsigned char)(myPort % 10);
        portStr[i] = (char)(rem + 48); /* to ascii */
        myPort = myPort / 10;
    }

    struct addrinfo *infoptr;
    int res = getaddrinfo((const char*)address->data, portStr, NULL, &infoptr);
    if(res != 0)
        return;

    /* Parse and process the message */
    struct message mm;
    memset(&mm, 0, sizeof(struct message));
    UA_Boolean rr = message_parse(&mm, (unsigned char*)msg.data, msg.length);
    if(rr)
        mdnsd_in(mdnsPrivateData.mdnsDaemon, &mm, infoptr->ai_addr,
                 (unsigned short)infoptr->ai_addrlen);
    freeaddrinfo(infoptr);
}

static void
UA_DiscoveryManager_sendMulticastMessages(UA_DiscoveryManager *dm) {
    UA_ConnectionManager *cm = dm->cm;
    if(!dm->cm || mdnsPrivateData.mdnsSendConnection == 0)
        return;

    struct sockaddr ip;
    memset(&ip, 0, sizeof(struct sockaddr));
    ip.sa_family = AF_INET; /* Ipv4 */

    struct message mm;
    memset(&mm, 0, sizeof(struct message));

    unsigned short sport = 0;
    while(mdnsd_out(mdnsPrivateData.mdnsDaemon, &mm, &ip, &sport) > 0) {
        int len = message_packet_len(&mm);
        char* buf = (char*)message_packet(&mm);
        if(len <= 0)
            continue;
        UA_ByteString sendBuf = UA_BYTESTRING_NULL;
        UA_StatusCode rv = cm->allocNetworkBuffer(cm, mdnsPrivateData.mdnsSendConnection,
                                                  &sendBuf, (size_t)len);
        if(rv != UA_STATUSCODE_GOOD)
            continue;
        memcpy(sendBuf.data, buf, sendBuf.length);
        cm->sendWithConnection(cm, mdnsPrivateData.mdnsSendConnection,
                               &UA_KEYVALUEMAP_NULL, &sendBuf);
    }
}

void UA_DiscoveryManager_mdnsCyclicTimer(UA_Server *server, void *data) {
    UA_DiscoveryManager_sendMulticastMessages((UA_DiscoveryManager*)data);
}

static void
MulticastDiscoveryRecvCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                               void *application, void **connectionContext,
                               UA_ConnectionState state, const UA_KeyValueMap *params,
                               UA_ByteString msg) {
    MulticastDiscoveryCallback(cm, connectionId, application, connectionContext,
                               state, params, msg, true);
}

static void
MulticastDiscoverySendCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                               void *application, void **connectionContext,
                               UA_ConnectionState state, const UA_KeyValueMap *params,
                               UA_ByteString msg) {
    MulticastDiscoveryCallback(cm, connectionId, application, connectionContext,
                               state, params, msg, false);
}

static UA_StatusCode
addMdnsRecordForNetworkLayer(UA_DiscoveryManager *dm, const UA_String serverName,
                             const UA_String *discoveryUrl) {
    UA_String hostname = UA_STRING_NULL;
    char hoststr[256]; /* check with UA_MAXHOSTNAME_LENGTH */
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
        gethostname(hoststr, sizeof(hoststr)-1);
        hoststr[sizeof(hoststr)-1] = '\0';
        hostname.data = (unsigned char *) hoststr;
        hostname.length = strlen(hoststr);
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

/* Create multicast 224.0.0.251:5353 socket */
static void
discovery_createMulticastSocket(UA_DiscoveryManager *dm) {
    /* Find the connection manager */
    if(!dm->cm) {
        UA_String udpString = UA_STRING("udp");
        for(UA_EventSource *es = dm->sc.server->config.eventLoop->eventSources;
            es != NULL; es = es->next) {
            /* Is this a usable connection manager? */
            if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
                continue;
            UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
            if(UA_String_equal(&udpString, &cm->protocol)) {
                dm->cm = cm;
                break;
            }
        }
    }

    if(!dm->cm) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "No UDP communication supported");
        return;
    }

    /* Set up the parameters */
    UA_KeyValuePair params[6];
    size_t paramsSize = 5;

    UA_UInt16 port = 5353;
    UA_String address = UA_STRING("224.0.0.251");
    UA_UInt32 ttl = 255;
    UA_Boolean reuse = true;
    UA_Boolean listen = true;

    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[1].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    params[2].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[2].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[3].key = UA_QUALIFIEDNAME(0, "reuse");
    UA_Variant_setScalar(&params[3].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[4].key = UA_QUALIFIEDNAME(0, "ttl");
    UA_Variant_setScalar(&params[4].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);
    if(dm->sc.server->config.mdnsInterfaceIP.length > 0) {
        params[5].key = UA_QUALIFIEDNAME(0, "interface");
        UA_Variant_setScalar(&params[5].value, &dm->sc.server->config.mdnsInterfaceIP,
                             &UA_TYPES[UA_TYPES_STRING]);
        paramsSize++;
    }

    /* Open the listen connection */
    UA_KeyValueMap kvm = {paramsSize, params};
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    if(mdnsPrivateData.mdnsRecvConnectionsSize == 0) {
        res = dm->cm->openConnection(dm->cm, &kvm, dm->sc.server, dm,
                                     MulticastDiscoveryRecvCallback);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                         "Could not create the mdns UDP multicast listen connection");
    }

    /* Open the send connection */
    listen = false;
    if(mdnsPrivateData.mdnsSendConnection == 0) {
        res = dm->cm->openConnection(dm->cm, &kvm, dm->sc.server, dm,
                                     MulticastDiscoverySendCallback);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                         "Could not create the mdns UDP multicast send connection");
    }
}

void
UA_DiscoveryManager_startMulticast(UA_DiscoveryManager *dm) {
    if(!mdnsPrivateData.mdnsDaemon) {
        mdnsPrivateData.mdnsDaemon = mdnsd_new(QCLASS_IN, 1000);
        mdnsd_register_receive_callback(mdnsPrivateData.mdnsDaemon, mdns_record_received, dm);
    }

#if defined(UA_ARCHITECTURE_WIN32) || defined(UA_ARCHITECTURE_WEC7)
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    /* Open the mdns listen socket */
    if(mdnsPrivateData.mdnsSendConnection == 0)
        discovery_createMulticastSocket(dm);
    if(mdnsPrivateData.mdnsSendConnection == 0) {
        UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                     "Could not create multicast socket");
        return;
    }

    /* Add record for the server itself */
    UA_String appName = dm->sc.server->config.mdnsConfig.mdnsServerName;
    for(size_t i = 0; i < dm->sc.server->config.serverUrlsSize; i++)
        addMdnsRecordForNetworkLayer(dm, appName, &dm->sc.server->config.serverUrls[i]);

    /* Send a multicast probe to find any other OPC UA server on the network
     * through mDNS */
    mdnsd_query(mdnsPrivateData.mdnsDaemon, "_opcua-tcp._tcp.local.",
                QTYPE_PTR,discovery_multicastQueryAnswer, dm->sc.server);
}

void
UA_DiscoveryManager_stopMulticast(UA_DiscoveryManager *dm) {
    UA_Server *server = dm->sc.server;
    for(size_t i = 0; i < server->config.serverUrlsSize; i++) {
        UA_String hostname = UA_STRING_NULL;
        char hoststr[256]; /* check with UA_MAXHOSTNAME_LENGTH */
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;

        UA_StatusCode retval =
            UA_parseEndpointUrl(&server->config.serverUrls[i],
                                &hostname, &port, &path);

        if(retval != UA_STATUSCODE_GOOD)
            continue;

        if(hostname.length == 0) {
            gethostname(hoststr, sizeof(hoststr)-1);
            hoststr[sizeof(hoststr)-1] = '\0';
            hostname.data = (unsigned char *) hoststr;
            hostname.length = strlen(hoststr);
        }

        UA_Discovery_removeRecord(dm, server->config.mdnsConfig.mdnsServerName,
                                  hostname, port, true);
    }

    /* Close the socket */
    if(dm->cm) {
        if(mdnsPrivateData.mdnsSendConnection)
            dm->cm->closeConnection(dm->cm, mdnsPrivateData.mdnsSendConnection);
        for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++)
            if(mdnsPrivateData.mdnsRecvConnections[i] != 0)
                dm->cm->closeConnection(dm->cm, mdnsPrivateData.mdnsRecvConnections[i]);
    }
}

void
UA_DiscoveryManager_clearMdns(UA_DiscoveryManager *dm) {
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

    /* Clean up mdns daemon */
    if(mdnsPrivateData.mdnsDaemon) {
        mdnsd_shutdown(mdnsPrivateData.mdnsDaemon);
        mdnsd_free(mdnsPrivateData.mdnsDaemon);
        mdnsPrivateData.mdnsDaemon = NULL;
    }
}

UA_UInt32
UA_DiscoveryManager_getMdnsConnectionCount(void) {
    return mdnsPrivateData.mdnsRecvConnectionsSize + (mdnsPrivateData.mdnsSendConnection != 0);
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

static void
UA_Discovery_multicastConflict(char *name, int type, void *arg) {
    /* In case logging is disabled */
    (void)name;
    (void)type;

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*) arg;
    UA_LOG_ERROR(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                 "Multicast DNS name conflict detected: "
                 "'%s' for type %d", name, type);
}

/* Create a service domain with the format [servername]-[hostname]._opcua-tcp._tcp.local. */
static void
createFullServiceDomain(char *outServiceDomain, size_t maxLen,
                        UA_String servername, UA_String hostname) {
    maxLen -= 24; /* the length we have remaining before the opc ua postfix and
                   * the trailing zero */

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
        offset = servername.length;
    }
    mp_snprintf(&outServiceDomain[offset], 24, "._opcua-tcp._tcp.local.");
}

/* Check if mDNS already has an entry for given hostname and port combination */
static UA_Boolean
UA_Discovery_recordExists(UA_DiscoveryManager *dm, const char* fullServiceDomain,
                          unsigned short port, const UA_DiscoveryProtocol protocol) {
    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    mdns_record_t *r  = mdnsd_get_published(mdnsPrivateData.mdnsDaemon, fullServiceDomain);
    while(r) {
        const mdns_answer_t *data = mdnsd_record_data(r);
        if(data->type == QTYPE_SRV && (port == 0 || data->srv.port == port))
            return true;
        r = mdnsd_record_next(r);
    }
    return false;
}

static int
discovery_multicastQueryAnswer(mdns_answer_t *a, void *arg) {
    UA_Server *server = (UA_Server*) arg;
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(!dm)
        return 0;

    if(a->type != QTYPE_PTR)
        return 0;

    if(a->rdname == NULL)
        return 0;

    /* Skip, if we already know about this server */
    UA_Boolean exists =
        UA_Discovery_recordExists(dm, a->rdname, 0, UA_DISCOVERY_TCP);
    if(exists == true)
        return 0;

    if(mdnsd_has_query(mdnsPrivateData.mdnsDaemon, a->rdname))
        return 0;

    UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                 "mDNS send query for: %s SRV&TXT %s", a->name, a->rdname);

    mdnsd_query(mdnsPrivateData.mdnsDaemon, a->rdname, QTYPE_SRV,
                discovery_multicastQueryAnswer, server);
    mdnsd_query(mdnsPrivateData.mdnsDaemon, a->rdname, QTYPE_TXT,
                discovery_multicastQueryAnswer, server);
    return 0;
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
        mdns_record_t *r =
            mdnsd_shared(mdnsPrivateData.mdnsDaemon, "_services._dns-sd._udp.local.",
                         QTYPE_PTR, 600);
        mdnsd_set_host(mdnsPrivateData.mdnsDaemon, r, "_opcua-tcp._tcp.local.");
        dm->mdnsMainSrvAdded = true;
    }

    /* [servername]-[hostname]._opcua-tcp._tcp.local. */
    char fullServiceDomain[63+24];
    createFullServiceDomain(fullServiceDomain, 63+24, servername, hostname);

    UA_Boolean exists = UA_Discovery_recordExists(dm, fullServiceDomain,
                                                  port, protocol);
    if(exists == true)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: add record for domain: %s", fullServiceDomain);

    if(isSelf && mdnsPrivateData.selfMdnsRecord.length == 0) {
        mdnsPrivateData.selfMdnsRecord = UA_STRING_ALLOC(fullServiceDomain);
        if(!mdnsPrivateData.selfMdnsRecord.data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_String serverName = {
        UA_MIN(63, servername.length + hostname.length + 1),
        (UA_Byte*) fullServiceDomain};

    struct serverOnNetwork *listEntry;
    /* The servername is servername + hostname. It is the same which we get
     * through mDNS and therefore we need to match servername */
    UA_StatusCode retval =
        UA_DiscoveryManager_addEntryToServersOnNetwork(dm, fullServiceDomain,
                                                       serverName, &listEntry);
    if(retval != UA_STATUSCODE_GOOD &&
       retval != UA_STATUSCODE_BADALREADYEXISTS)
        return retval;

    /* If entry is already in list, skip initialization of capabilities and txt+srv */
    if(retval != UA_STATUSCODE_BADALREADYEXISTS) {
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

        const size_t newUrlSize = 10 + hostname.length + 8 + path.length + 1;
        UA_STACKARRAY(char, newUrl, newUrlSize);
        memset(newUrl, 0, newUrlSize);
        if(path.length > 0) {
            mp_snprintf(newUrl, newUrlSize, "opc.tcp://%S:%d/%S", hostname, port, path);
        } else {
            mp_snprintf(newUrl, newUrlSize, "opc.tcp://%S:%d", hostname, port);
        }
        listEntry->serverOnNetwork.discoveryUrl = UA_String_fromChars(newUrl);
        listEntry->srvSet = true;
    }

    /* _services._dns-sd._udp.local. PTR _opcua-tcp._tcp.local */

    /* check if there is already a PTR entry for the given service. */

    /* _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local. */
    mdns_record_t *r =
        mdns_find_record(mdnsPrivateData.mdnsDaemon, QTYPE_PTR,
                         "_opcua-tcp._tcp.local.", fullServiceDomain);
    if(!r) {
        r = mdnsd_shared(mdnsPrivateData.mdnsDaemon, "_opcua-tcp._tcp.local.",
                         QTYPE_PTR, 600);
        mdnsd_set_host(mdnsPrivateData.mdnsDaemon, r, fullServiceDomain);
    }

    /* The first 63 characters of the hostname (or less) */
    size_t maxHostnameLen = UA_MIN(hostname.length, 63);
    char localDomain[71];
    memcpy(localDomain, hostname.data, maxHostnameLen);
    strcpy(localDomain + maxHostnameLen, ".local.");

    /* [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname]. */
    r = mdnsd_unique(mdnsPrivateData.mdnsDaemon, fullServiceDomain,
                     QTYPE_SRV, 600, UA_Discovery_multicastConflict, dm);
    mdnsd_set_srv(mdnsPrivateData.mdnsDaemon, r, 0, 0, port, localDomain);

    /* A/AAAA record for all ip addresses.
     * [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
     * [hostname]. A [ip]. */
    mdns_set_address_record(dm, fullServiceDomain, localDomain);

    /* TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,... */
    UA_STACKARRAY(char, pathChars, path.length + 1);
    if(createTxt) {
        if(path.length > 0)
            memcpy(pathChars, path.data, path.length);
        pathChars[path.length] = 0;
        mdns_create_txt(dm, fullServiceDomain, pathChars, capabilites,
                        capabilitiesSize, UA_Discovery_multicastConflict);
    }

    return UA_STATUSCODE_GOOD;
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

    /* [servername]-[hostname]._opcua-tcp._tcp.local. */
    char fullServiceDomain[63 + 24];
    createFullServiceDomain(fullServiceDomain, 63+24, servername, hostname);

    UA_LOG_INFO(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: remove record for domain: %s",
                fullServiceDomain);

    UA_String serverName =
        {UA_MIN(63, servername.length + hostname.length + 1), (UA_Byte*)fullServiceDomain};

    UA_StatusCode retval =
        UA_DiscoveryManager_removeEntryFromServersOnNetwork(dm, fullServiceDomain, serverName);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local. */
    mdns_record_t *r =
        mdns_find_record(mdnsPrivateData.mdnsDaemon, QTYPE_PTR,
                         "_opcua-tcp._tcp.local.", fullServiceDomain);
    if(!r) {
        UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: could not remove record. "
                       "PTR Record not found for domain: %s", fullServiceDomain);
        return UA_STATUSCODE_BADNOTHINGTODO;
    }
    mdnsd_done(mdnsPrivateData.mdnsDaemon, r);

    /* looks for [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5
     * port hostname.local. and TXT record:
     * [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
     * and A record: [servername]-[hostname]._opcua-tcp._tcp.local. A [ip] */
    mdns_record_t *r2 =
        mdnsd_get_published(mdnsPrivateData.mdnsDaemon, fullServiceDomain);
    if(!r2) {
        UA_LOG_WARNING(dm->sc.server->config.logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: could not remove record. Record not "
                       "found for domain: %s", fullServiceDomain);
        return UA_STATUSCODE_BADNOTHINGTODO;
    }

    while(r2) {
        const mdns_answer_t *data = mdnsd_record_data(r2);
        mdns_record_t *next = mdnsd_record_next(r2);
        if((removeTxt && data->type == QTYPE_TXT) ||
           (removeTxt && data->type == QTYPE_A) ||
           data->srv.port == port) {
            mdnsd_done(mdnsPrivateData.mdnsDaemon, r2);
        }
        r2 = next;
    }

    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */
