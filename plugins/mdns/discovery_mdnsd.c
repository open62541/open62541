/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/server.h>
#include <open62541/plugin/servercomponent.h>

#ifdef UA_ENABLE_DISCOVERY_MULTICAST_MDNSD

#include "mdnsd/libmdnsd/mdnsd.h"
#ifndef UA_ENABLE_AMALGAMATION
#include "mdnsd/libmdnsd/xht.h"
#include "mdnsd/libmdnsd/sdtxt.h"
#endif

#ifdef UA_ARCHITECTURE_WIN32
/* inet_ntoa is deprecated on MSVC but used for compatibility */
# define _WINSOCK_DEPRECATED_NO_WARNINGS
# include <winsock2.h>
# include <iphlpapi.h>
# include <ws2tcpip.h>
#else
# include <unistd.h>
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

typedef struct ServerOnNetwork {
    struct ServerOnNetwork *next;
    UA_ServerOnNetwork serverOnNetwork;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char* pathTmp;
} ServerOnNetwork;

typedef struct {
    UA_MdnsServerComponent msc;

    UA_Logger *logging; /* Shortcut from msc->server->logging */
    mdns_daemon_t *mdnsDaemon;

    UA_Boolean mdnsMainSrvAdded;

    UA_ConnectionManager *cm;
    uintptr_t mdnsSendConnection;
    uintptr_t mdnsRecvConnections[UA_MAXMDNSRECVSOCKETS];
    size_t mdnsRecvConnectionsSize;

    ServerOnNetwork *serverList;

    /* Name of server itself. Used to detect if received mDNS
     * message was from itself */
    UA_String selfMdnsRecord;

    UA_UInt64 sendCallbackId;
} MdnsdServerComponent;

static UA_StatusCode
MdnsdServerComponent_start(UA_ServerComponent *sc);

static UA_StatusCode
addEntryToServersOnNetwork(MdnsdServerComponent *msc,
                           const char *record, UA_String serverName,
                           ServerOnNetwork **addedEntry);

static ServerOnNetwork *
mdns_record_add_or_get(MdnsdServerComponent *msc, const char *record,
                       UA_String serverName, UA_Boolean createNew) {
    for(ServerOnNetwork *son = msc->serverList; son; son = son->next) {
        if(UA_String_equal(&serverName, &son->serverOnNetwork.serverName))
            return son;
    }

    if(!createNew)
        return NULL;

    ServerOnNetwork *entry = NULL;
    addEntryToServersOnNetwork(msc, record, serverName, &entry);
    return entry;
}

static UA_StatusCode
addEntryToServersOnNetwork(MdnsdServerComponent *msc,
                           const char *record, UA_String serverName,
                           ServerOnNetwork **addedEntry) {
    /* Lookup without creating new */
    ServerOnNetwork *entry = mdns_record_add_or_get(msc, record,
                                                    serverName, false);
    if(entry) {
        if(addedEntry != NULL)
            *addedEntry = entry;
        return UA_STATUSCODE_BADALREADYEXISTS;
    }

    UA_LOG_DEBUG(msc->logging, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: Add entry to ServersOnNetwork: %s (%S)",
                 record, serverName);

    /* Allocate new */
    ServerOnNetwork *listEntry = (ServerOnNetwork*)
        UA_calloc(1, sizeof(ServerOnNetwork));
    if(!listEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Populate the fields */
    UA_StatusCode res = UA_String_copy(&serverName,
                                       &listEntry->serverOnNetwork.serverName);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(listEntry);
        return res;
    }

    /* Add to the server */
    UA_Server *server = msc->msc.serverComponent.server;
    res = UA_Server_registerServerOnNetwork(server, &listEntry->serverOnNetwork, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ServerOnNetwork_clear(&listEntry->serverOnNetwork);
        UA_free(listEntry);
        return res;
    }

    /* Add to the linked list */
    listEntry->next = msc->serverList;
    msc->serverList = listEntry;
    
    /* Return pointer if requested */
    if(addedEntry != NULL)
        *addedEntry = listEntry;
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ARCHITECTURE_WIN32

/* see http://stackoverflow.com/a/10838854/869402 */
static IP_ADAPTER_ADDRESSES *
getInterfaces(MdnsdServerComponent *msc) {
    IP_ADAPTER_ADDRESSES* adapter_addresses = NULL;

    /* Start with a 16 KB buffer and resize if needed - multiple attempts in
     * case interfaces change while we are in the middle of querying them. */
    DWORD adapter_addresses_buffer_size = 16 * 1024;
    for(size_t attempts = 0; attempts != 3; ++attempts) {
        /* todo: malloc may fail: return a statuscode */
        adapter_addresses = (IP_ADAPTER_ADDRESSES*)UA_malloc(adapter_addresses_buffer_size);
        if(!adapter_addresses) {
            UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_DISCOVERY,
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
        UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_SERVER,
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
removeEntryFromServersOnNetwork(MdnsdServerComponent *msc,
                                const char *record, UA_String serverName) {
    UA_LOG_DEBUG(msc->logging, UA_LOGCATEGORY_SERVER,
                 "Multicast DNS: Remove entry from ServersOnNetwork: %s (%S)",
                 record, serverName);

    /* Get and removed the internal entry */
    ServerOnNetwork *entry = mdns_record_add_or_get(msc, record, serverName, false);
    if(entry) {
        for(ServerOnNetwork *son = msc->serverList; son; son = son->next) {
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
    UA_Server *server = msc->msc.serverComponent.server;
    UA_Server_deregisterServerOnNetwork(server, serverName, NULL);

    return UA_STATUSCODE_GOOD;
}

static void
setTxt(MdnsdServerComponent *msc, const struct resource *r,
       ServerOnNetwork *entry) {
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
                if(!entry->pathTmp) {
                    UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_SERVER,
                                 "Cannot alloc memory for mDNS srv path");
                    return;
                }
                memcpy(entry->pathTmp, path, pathLen);
                entry->pathTmp[pathLen] = '\0';
            }
        } else {
            /* SRV already there and discovery URL set. Add path to discovery URL */
            UA_String_append(&entry->serverOnNetwork.discoveryUrl, UA_STRING(path));
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
setSrv(MdnsdServerComponent *msc, const struct resource *r,
       ServerOnNetwork *entry) {
    entry->srvSet = true;

    /* The specification Part 12 says: The hostname maps onto the SRV record
     * target field. If the hostname is an IPAddress then it must be converted
     * to a domain name. If this cannot be done then LDS shall report an
     * error. */

    /* cut off last dot */
    size_t srvNameLen = strlen(r->known.srv.name);
    if(srvNameLen > 0 && r->known.srv.name[srvNameLen - 1] == '.')
        srvNameLen--;
    char *newUrl = (char*)UA_malloc(10 + srvNameLen + 8 + 1);
    if(!newUrl) {
        UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_SERVER,
                     "Cannot allocate char for discovery url. Out of memory.");
        return;
    }

    /* opc.tcp://[servername]:[port][path] */
    UA_String_format(&entry->serverOnNetwork.discoveryUrl, "opc.tcp://%.*s:%d",
                     (int)srvNameLen, r->known.srv.name, r->known.srv.port);

    UA_LOG_INFO(msc->logging, UA_LOGCATEGORY_SERVER,
                "Multicast DNS: found server: %S",
                entry->serverOnNetwork.discoveryUrl);

    if(entry->pathTmp) {
        UA_String_append(&entry->serverOnNetwork.discoveryUrl,
                          UA_STRING(entry->pathTmp));
        UA_free(entry->pathTmp);
        entry->pathTmp = NULL;
    }
}

/* This will be called by the mDNS library on every record which is received */
static void
mdns_record_received(const struct resource *r, void *data) {
    MdnsdServerComponent *msc = (MdnsdServerComponent*)data;

    /* We only need SRV and TXT records */
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
    UA_Boolean isSelfAnnounce = UA_String_equal(&msc->selfMdnsRecord, &recordStr);
    if(isSelfAnnounce)
        return; // ignore itself

    /* Extract the servername */
    size_t servernameLen = (size_t) (opcStr - r->name);
    if(servernameLen == 0)
        return;
    servernameLen--; /* remove point */
    UA_String serverName = {servernameLen, (UA_Byte*)r->name};

    /* Get entry */
    ServerOnNetwork *entry = mdns_record_add_or_get(msc, r->name, serverName, r->ttl > 0);
    if(!entry)
        return;

    /* Check that the ttl is positive */
    if(r->ttl == 0) {
        UA_LOG_INFO(msc->logging, UA_LOGCATEGORY_SERVER,
                    "Multicast DNS: remove server (TTL=0): %S",
                    entry->serverOnNetwork.discoveryUrl);
        removeEntryFromServersOnNetwork(msc, r->name, serverName);
        return;
    }

    /* Add the resources */
    if(r->type == QTYPE_TXT && !entry->txtSet)
        setTxt(msc, r, entry);
    else if (r->type == QTYPE_SRV && !entry->srvSet)
        setSrv(msc, r, entry);
}

static void
mdns_create_txt(MdnsdServerComponent *msc, const char *fullServiceDomain,
                const char *path, const UA_String *capabilites,
                const size_t capabilitiesSize,
                void (*conflict)(char *host, int type, void *arg)) {
    mdns_record_t *r =
        mdnsd_unique(msc->mdnsDaemon, fullServiceDomain,
                     QTYPE_TXT, 600, conflict, msc);

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
                UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_SERVER,
                             "Cannot alloc memory for txt path");
                return;
            }
            memcpy(allocPath, path, pathLen);
            allocPath[pathLen] = '\0';
        } else {
            allocPath = (char*)UA_malloc(pathLen + 2);
            if(!allocPath) {
                UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_SERVER,
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

    int len;
    unsigned char *packet = sd2txt(h, &len);
    if(allocPath)
        UA_free(allocPath);
    if(caps)
        UA_free(caps);
    xht_free(h);
    mdnsd_set_raw(msc->mdnsDaemon, r, (char*)packet, (unsigned short)len);
    MDNSD_free(packet);
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
mdns_set_address_record_if(MdnsdServerComponent *msc, const char *fullServiceDomain,
                           const char *localDomain, char *addr, UA_UInt16 addr_len) {
    /* [servername]-[hostname]._opcua-tcp._tcp.local. A [ip]. */
    mdns_record_t *r = mdnsd_shared(msc->mdnsDaemon, fullServiceDomain, QTYPE_A, 600);
    mdnsd_set_raw(msc->mdnsDaemon, r, addr, addr_len);

    /* [hostname]. A [ip]. */
    r = mdnsd_shared(msc->mdnsDaemon, localDomain, QTYPE_A, 600);
    mdnsd_set_raw(msc->mdnsDaemon, r, addr, addr_len);
}

/* Loop over network interfaces and run set_address_record on each */
#ifdef UA_ARCHITECTURE_WIN32
static void
mdns_set_address_record(MdnsdServerComponent *msc, const char *fullServiceDomain,
                        const char *localDomain) {
    IP_ADAPTER_ADDRESSES* adapter_addresses = getInterfaces(msc);
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
                mdns_set_address_record_if(msc, fullServiceDomain,
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
mdns_set_address_record(MdnsdServerComponent *msc, const char *fullServiceDomain,
                        const char *localDomain) {
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;
    if(getifaddrs(&ifaddr) == -1) {
        UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_SERVER,
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
            mdns_set_address_record_if(msc, fullServiceDomain,
                                       localDomain, (char*)&sa->sin_addr.s_addr, 4);
        }

        /* IPv6 not implemented yet */
    }

    /* Clean up */
    freeifaddrs(ifaddr);
}
#else /* UA_ARCHITECTURE_WIN32 */

void
mdns_set_address_record(MdnsdServerComponent *msc, const char *fullServiceDomain,
                        const char *localDomain) {
    UA_Server *server = msc->msc.serverComponent.server;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    if(config.mdnsIpAddressListSize == 0) {
        UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_SERVER,
                     "If UA_HAS_GETIFADDR is false, config.mdnsIpAddressList must be set");
        return;
    }

    for(size_t i = 0; i < config->mdnsIpAddressListSize; i++) {
        mdns_set_address_record_if(msc, fullServiceDomain, localDomain,
                                   (char*)&config->mdnsIpAddressList[i], 4);
    }
}

#endif /* UA_ARCHITECTURE_WIN32 */

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
addRecord(MdnsdServerComponent *msc, const UA_String servername,
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
removeRecord(MdnsdServerComponent *msc, const UA_String servername,
             const UA_String hostname, UA_UInt16 port);

static void
addConnection(MdnsdServerComponent *msc, uintptr_t connectionId,
              UA_Boolean recv) {
    if(!recv) {
        msc->mdnsSendConnection = connectionId;
        return;
    }
    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        if(msc->mdnsRecvConnections[i] == connectionId)
            return;
    }

    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        if(msc->mdnsRecvConnections[i] != 0)
            continue;
        msc->mdnsRecvConnections[i] = connectionId;
        msc->mdnsRecvConnectionsSize++;
        break;
    }
}

static void
removeConnection(MdnsdServerComponent *msc, uintptr_t connectionId,
                     UA_Boolean recv) {
    if(msc->mdnsSendConnection == connectionId) {
        msc->mdnsSendConnection = 0;
        return;
    }
    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        if(msc->mdnsRecvConnections[i] != connectionId)
            continue;
        msc->mdnsRecvConnections[i] = 0;
        msc->mdnsRecvConnectionsSize--;
        break;
    }
}

static UA_Boolean
allConnectionsClosed(MdnsdServerComponent *msc) {
    if(msc->mdnsSendConnection != 0)
        return false;
    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        if(msc->mdnsRecvConnections[i] != 0)
            return false;
    }
    return true;
}

static void
MulticastDiscoveryCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                           void *_, void **connectionContext,
                           UA_ConnectionState state, const UA_KeyValueMap *params,
                           UA_ByteString msg, UA_Boolean recv) {
    MdnsdServerComponent *msc = (MdnsdServerComponent*)connectionContext;

    if(state == UA_CONNECTIONSTATE_CLOSING) {
        /* Remove closed connction */
        removeConnection(msc, connectionId, recv);

        if(msc->msc.serverComponent.state == UA_LIFECYCLESTATE_STOPPING) {
            /* Stopping? Internally checks if all sockets are closed. */
            if(allConnectionsClosed(msc))
                msc->msc.serverComponent.state = UA_LIFECYCLESTATE_STOPPED;
        } else if(msc->msc.serverComponent.state == UA_LIFECYCLESTATE_STARTED) {
            /* Restart mdns sockets if not shutting down */
            MdnsdServerComponent_start(&msc->msc.serverComponent);
        }
        return;
    }

    addConnection(msc, connectionId, recv);

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
        mdnsd_in(msc->mdnsDaemon, &mm, infoptr->ai_addr,
                 (unsigned short)infoptr->ai_addrlen);
    freeaddrinfo(infoptr);
}

static void
sendMulticastMessages(UA_Server *server, void *data) {
    MdnsdServerComponent *msc = (MdnsdServerComponent*)data;
    (void)server;
    
    UA_ConnectionManager *cm = msc->cm;
    if(!cm || msc->mdnsSendConnection == 0)
        return;

    struct sockaddr ip;
    memset(&ip, 0, sizeof(struct sockaddr));
    ip.sa_family = AF_INET; /* Ipv4 */

    struct message mm;
    memset(&mm, 0, sizeof(struct message));

    unsigned short sport = 0;
    while(mdnsd_out(msc->mdnsDaemon, &mm, &ip, &sport) > 0) {
        int len = message_packet_len(&mm);
        char* buf = (char*)message_packet(&mm);
        if(len <= 0)
            continue;
        UA_ByteString sendBuf = UA_BYTESTRING_NULL;
        UA_StatusCode rv = cm->allocNetworkBuffer(cm, msc->mdnsSendConnection,
                                                  &sendBuf, (size_t)len);
        if(rv != UA_STATUSCODE_GOOD)
            continue;
        memcpy(sendBuf.data, buf, sendBuf.length);
        cm->sendWithConnection(cm, msc->mdnsSendConnection,
                               &UA_KEYVALUEMAP_NULL, &sendBuf);
    }
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
addMdnsRecordForNetworkLayer(MdnsdServerComponent *msc, const UA_String serverName,
                             const UA_String *discoveryUrl) {
    UA_String hostname = UA_STRING_NULL;
    char hoststr[256]; /* check with UA_MAXHOSTNAME_LENGTH */
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode retval =
        UA_parseEndpointUrl(discoveryUrl, &hostname, &port, &path);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url is invalid: %S", *discoveryUrl);
        return retval;
    }

    if(hostname.length == 0) {
        gethostname(hoststr, sizeof(hoststr)-1);
        hoststr[sizeof(hoststr)-1] = '\0';
        hostname.data = (unsigned char *) hoststr;
        hostname.length = strlen(hoststr);
    }

    UA_Server *server = msc->msc.serverComponent.server;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    retval = addRecord(msc, serverName, hostname, port, path, UA_DISCOVERY_TCP,
                       config->mdnsConfig.serverCapabilities,
                       config->mdnsConfig.serverCapabilitiesSize, true);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Cannot add mDNS Record: %s", UA_StatusCode_name(retval));
        return retval;
    }
    return UA_STATUSCODE_GOOD;
}

/* Create multicast 224.0.0.251:5353 socket */
static void
discovery_createMulticastSocket(MdnsdServerComponent *msc) {
    UA_Server *server = msc->msc.serverComponent.server;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Find the UDP connection manager */
    if(!msc->cm) {
        UA_String udpString = UA_STRING("udp");
        UA_EventSource *es = config->eventLoop->eventSources;
        for(; es != NULL; es = es->next) {
            /* Is this a usable connection manager? */
            if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
                continue;
            UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
            if(UA_String_equal(&udpString, &cm->protocol)) {
                msc->cm = cm;
                break;
            }
        }
    }

    if(!msc->cm) {
        UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_DISCOVERY,
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
    if(config->mdnsInterfaceIP.length > 0) {
        params[5].key = UA_QUALIFIEDNAME(0, "interface");
        UA_Variant_setScalar(&params[5].value, &config->mdnsInterfaceIP,
                             &UA_TYPES[UA_TYPES_STRING]);
        paramsSize++;
    }

    /* Open the listen connection */
    UA_KeyValueMap kvm = {paramsSize, params};
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    if(msc->mdnsRecvConnectionsSize == 0) {
        res = msc->cm->openConnection(msc->cm, &kvm, server, msc,
                                      MulticastDiscoveryRecvCallback);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                         "Could not create the mdns UDP multicast listen connection");
    }

    /* Open the send connection */
    listen = false;
    if(msc->mdnsSendConnection == 0) {
        res = msc->cm->openConnection(msc->cm, &kvm, server, msc,
                                     MulticastDiscoverySendCallback);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                         "Could not create the mdns UDP multicast send connection");
    }
}

static void
MdnsdServerComponent_announce(UA_MdnsServerComponent *sc,
                              const UA_ServerOnNetwork *son,
                              const UA_KeyValueMap *params) {
    MdnsdServerComponent *msc = (MdnsdServerComponent*)sc;

    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res =
        UA_parseEndpointUrl(&son->discoveryUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url invalid: %S", son->discoveryUrl);
        return;
    }

    res = addRecord(msc, son->serverName, hostname, port, path,
                    UA_DISCOVERY_TCP, son->serverCapabilities,
                    son->serverCapabilitiesSize, false);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Could not add mDNS record for hostname %S",
                       son->serverName);
}

static void
MdnsdServerComponent_retract(UA_MdnsServerComponent *sc,
                             const UA_String serverName,
                             const UA_String discoveryUrl,
                             const UA_KeyValueMap *params) {
    MdnsdServerComponent *msc = (MdnsdServerComponent*)sc;

    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res =
        UA_parseEndpointUrl(&discoveryUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Server url invalid: %S", discoveryUrl);
        return;
    }

    res = removeRecord(msc, serverName, hostname, port);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Could not remove mDNS record for hostname %S", serverName);
    }
}

/* Log a conflict */
static void
multicastConflict(char *name, int type, void *arg) {
    /* In case logging is disabled */
    (void)name;
    (void)type;

    MdnsdServerComponent *msc = (MdnsdServerComponent*)arg;
    UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                 "Multicast DNS name conflict detected: "
                 "'%s' for type %d", name, type);
}

/* Create a service domain with the format [servername]-[hostname]._opcua-tcp._tcp.local. */
static void
createFullServiceDomain(UA_String *outServiceDomain,
                        UA_String servername, UA_String hostname) {
    /* Can we use hostname and servername with full length? */
    if(hostname.length + servername.length + 1 > outServiceDomain->length) {
        if(servername.length + 2 > outServiceDomain->length) {
            servername.length = outServiceDomain->length;
            hostname.length = 0;
        } else {
            hostname.length = outServiceDomain->length - servername.length - 1;
        }
    }

    if(hostname.length > 0) {
        UA_String_format(outServiceDomain, "%S-%S._opcua-tcp._tcp.local.", servername, hostname);
        /* Replace all dots with minus. Otherwise mDNS is not valid */
        size_t offset = servername.length + hostname.length + 1;
        for(size_t i = servername.length+1; i < offset; i++) {
            if(outServiceDomain->data[i] == '.')
                outServiceDomain->data[i] = '-';
        }
    } else {
        UA_String_format(outServiceDomain, "%S._opcua-tcp._tcp.local.", servername);
    }
}

/* Check if mDNS already has an entry for given hostname and port combination */
static UA_Boolean
recordExists(MdnsdServerComponent *msc, const char* fullServiceDomain,
             unsigned short port, const UA_DiscoveryProtocol protocol) {
    // [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].
    mdns_record_t *r  = mdnsd_get_published(msc->mdnsDaemon, fullServiceDomain);
    while(r) {
        const mdns_answer_t *data = mdnsd_record_data(r);
        if(data->type == QTYPE_SRV && (port == 0 || data->srv.port == port))
            return true;
        r = mdnsd_record_next(r);
    }
    return false;
}

static int
multicastQueryAnswer(mdns_answer_t *a, void *arg) {
    if(a->type != QTYPE_PTR || a->rdname == NULL)
        return 0;

    /* Skip, if we already know about this server */
    MdnsdServerComponent *msc = (MdnsdServerComponent*)arg;
    UA_Boolean exists = recordExists(msc, a->rdname, 0, UA_DISCOVERY_TCP);
    if(exists == true)
        return 0;

    if(mdnsd_has_query(msc->mdnsDaemon, a->rdname))
        return 0;

    UA_LOG_DEBUG(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                 "mDNS send query for: %s SRV&TXT %s", a->name, a->rdname);

    mdnsd_query(msc->mdnsDaemon, a->rdname, QTYPE_SRV, multicastQueryAnswer, msc);
    mdnsd_query(msc->mdnsDaemon, a->rdname, QTYPE_TXT, multicastQueryAnswer, msc);
    return 0;
}

static UA_StatusCode
addRecord(MdnsdServerComponent *msc, const UA_String servername,
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
    if(hostname.length + servername.length + 1 > 63) { /* include dash between servername-hostname */
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    } else if(hostname.length > 63) {
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Hostname length exceeds maximum of 63 chars. "
                       "It will be truncated.");
    }

    if(!msc->mdnsMainSrvAdded) {
        mdns_record_t *r =
            mdnsd_shared(msc->mdnsDaemon, "_services._dns-sd._udp.local.", QTYPE_PTR, 600);
        mdnsd_set_host(msc->mdnsDaemon, r, "_opcua-tcp._tcp.local.");
        msc->mdnsMainSrvAdded = true;
    }

    /* [servername]-[hostname]._opcua-tcp._tcp.local. */
    char fullServiceDomainBuf[63+24];
    UA_String fullServiceDomain = {63+24, (UA_Byte*)fullServiceDomainBuf};
    createFullServiceDomain(&fullServiceDomain, servername, hostname);

    UA_Boolean exists = recordExists(msc, fullServiceDomainBuf, port, protocol);
    if(exists == true)
        return UA_STATUSCODE_GOOD;

    UA_LOG_INFO(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: add record for domain: %S", fullServiceDomain);

    if(isSelf && msc->selfMdnsRecord.length == 0) {
        msc->selfMdnsRecord = UA_STRING_ALLOC(fullServiceDomainBuf);
        if(!msc->selfMdnsRecord.data)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_String serverName = {
        UA_MIN(63, servername.length + hostname.length + 1),
        (UA_Byte*) fullServiceDomainBuf};

    ServerOnNetwork *listEntry;
    /* The servername is servername + hostname. It is the same which we get
     * through mDNS and therefore we need to match servername */
    UA_StatusCode retval =
        addEntryToServersOnNetwork(msc, fullServiceDomainBuf, serverName, &listEntry);
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
        listEntry->srvSet = true;

        if(path.length > 0) {
            UA_String_format(&listEntry->serverOnNetwork.discoveryUrl,
                             "opc.tcp://%S:%d/%S", hostname, port, path);
        } else {
            UA_String_format(&listEntry->serverOnNetwork.discoveryUrl,
                             "opc.tcp://%S:%d", hostname, port);
        }
    }

    /* _services._dns-sd._udp.local. PTR _opcua-tcp._tcp.local */

    /* check if there is already a PTR entry for the given service. */

    /* _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local. */
    mdns_record_t *r =
        mdns_find_record(msc->mdnsDaemon, QTYPE_PTR,
                         "_opcua-tcp._tcp.local.", fullServiceDomainBuf);
    if(!r) {
        r = mdnsd_shared(msc->mdnsDaemon, "_opcua-tcp._tcp.local.", QTYPE_PTR, 600);
        mdnsd_set_host(msc->mdnsDaemon, r, fullServiceDomainBuf);
    }

    /* The first 63 characters of the hostname (or less) and ".local." */
    size_t maxHostnameLen = UA_MIN(hostname.length, 63);
    char localDomain[71];
    memcpy(localDomain, hostname.data, maxHostnameLen);
    strcpy(localDomain + maxHostnameLen, ".local.");

    /* [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].local. */
    r = mdnsd_unique(msc->mdnsDaemon, fullServiceDomainBuf,
                     QTYPE_SRV, 600, multicastConflict, msc);
    mdnsd_set_srv(msc->mdnsDaemon, r, 0, 0, port, localDomain);

    /* A/AAAA record for all ip addresses.
     * [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
     * [hostname].local. A [ip]. */
    mdns_set_address_record(msc, fullServiceDomainBuf, localDomain);

    /* TXT record: [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,... */
    UA_STACKARRAY(char, pathChars, path.length + 1);
    if(path.length > 0)
        memcpy(pathChars, path.data, path.length);
    pathChars[path.length] = 0;
    mdns_create_txt(msc, fullServiceDomainBuf, pathChars, capabilites,
                    capabilitiesSize, multicastConflict);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeRecord(MdnsdServerComponent *msc, const UA_String servername,
             const UA_String hostname, UA_UInt16 port) {
    /* use a limit for the hostname length to make sure full string fits into 63
     * chars (limited by DNS spec) */
    if(hostname.length == 0 || servername.length == 0)
        return UA_STATUSCODE_BADOUTOFRANGE;

    if(hostname.length + servername.length + 1 > 63) { /* include dash between servername-hostname */
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: Combination of hostname+servername exceeds "
                       "maximum of 62 chars. It will be truncated.");
    }

    /* [servername]-[hostname]._opcua-tcp._tcp.local. */
    char fullServiceDomainBuf[63 + 24];
    UA_String fullServiceDomain = {63+24, (UA_Byte*)fullServiceDomainBuf};
    createFullServiceDomain(&fullServiceDomain, servername, hostname);

    UA_LOG_INFO(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                "Multicast DNS: remove record for domain: %S",
                fullServiceDomain);

    UA_String serverName =
        {UA_MIN(63, servername.length + hostname.length + 1),
         (UA_Byte*)fullServiceDomainBuf};

    UA_StatusCode retval =
        removeEntryFromServersOnNetwork(msc, fullServiceDomainBuf, serverName);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local. */
    mdns_record_t *r =
        mdns_find_record(msc->mdnsDaemon, QTYPE_PTR,
                         "_opcua-tcp._tcp.local.", fullServiceDomainBuf);
    if(!r) {
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: could not remove record. "
                       "PTR Record not found for domain: %s", fullServiceDomainBuf);
        return UA_STATUSCODE_BADNOTHINGTODO;
    }
    mdnsd_done(msc->mdnsDaemon, r);

    /* Looks for [servername]-[hostname]._opcua-tcp._tcp.local. 86400 IN SRV 0 5
     * port hostname.local. and TXT record:
     * [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
     * and A record: [servername]-[hostname]._opcua-tcp._tcp.local. A [ip] */
    mdns_record_t *r2 = mdnsd_get_published(msc->mdnsDaemon, fullServiceDomainBuf);
    if(!r2) {
        UA_LOG_WARNING(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                       "Multicast DNS: could not remove record. Record not "
                       "found for domain: %s", fullServiceDomainBuf);
        return UA_STATUSCODE_BADNOTHINGTODO;
    }

    while(r2) {
        const mdns_answer_t *data = mdnsd_record_data(r2);
        mdns_record_t *next = mdnsd_record_next(r2);
        if(data->type == QTYPE_TXT || data->type == QTYPE_A ||
           data->srv.port == port) {
            mdnsd_done(msc->mdnsDaemon, r2);
        }
        r2 = next;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
MdnsdServerComponent_start(UA_ServerComponent *sc) {
    MdnsdServerComponent *msc = (MdnsdServerComponent*)sc;
    if(!sc->server)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ServerConfig *config = UA_Server_getConfig(sc->server);
    msc->logging = config->logging;

    if(sc->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Cannot start multicast discovery that is already running");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(!msc->mdnsDaemon) {
        msc->mdnsDaemon = mdnsd_new(QCLASS_IN, 1000);
        mdnsd_register_receive_callback(msc->mdnsDaemon,
                                        mdns_record_received, msc);
    }

#if defined(UA_ARCHITECTURE_WIN32) || defined(UA_ARCHITECTURE_WEC7)
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    /* Open the mdns listen socket */
    if(msc->mdnsSendConnection == 0)
        discovery_createMulticastSocket(msc);
    if(msc->mdnsSendConnection == 0) {
        UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Could not create multicast socket");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add a repeated callback to send out multicast messages */
    UA_Server_addRepeatedCallback(sc->server, sendMulticastMessages,
                                  msc, 1000.0, &msc->sendCallbackId);

    /* Add record for the server itself */
    UA_String appName = config->mdnsConfig.mdnsServerName;
    for(size_t i = 0; i < config->serverUrlsSize; i++)
        addMdnsRecordForNetworkLayer(msc, appName, &config->serverUrls[i]);

    /* Send a multicast probe to find any other OPC UA server on the network
     * through mDNS */
    mdnsd_query(msc->mdnsDaemon, "_opcua-tcp._tcp.local.",
                QTYPE_PTR, multicastQueryAnswer, msc);

    return UA_STATUSCODE_GOOD;
}

static void
MdnsdServerComponent_stop(UA_ServerComponent *sc) {
    UA_Server *server = sc->server;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    MdnsdServerComponent *msc = (MdnsdServerComponent*)sc;

    /* Already stopped */
    if(sc->state == UA_LIFECYCLESTATE_STOPPED)
        return;

    /* Commence async stopping */
    sc->state = UA_LIFECYCLESTATE_STOPPING;

    /* Remove repeated callback to send out multicast messages */
    UA_Server_removeRepeatedCallback(server, msc->sendCallbackId);
    msc->sendCallbackId = 0;

    for(size_t i = 0; i < config->serverUrlsSize; i++) {
        UA_String hostname = UA_STRING_NULL;
        char hoststr[256]; /* check with UA_MAXHOSTNAME_LENGTH */
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;

        UA_StatusCode retval =
            UA_parseEndpointUrl(&config->serverUrls[i], &hostname, &port, &path);

        if(retval != UA_STATUSCODE_GOOD)
            continue;

        if(hostname.length == 0) {
            gethostname(hoststr, sizeof(hoststr)-1);
            hoststr[sizeof(hoststr)-1] = '\0';
            hostname.data = (unsigned char *) hoststr;
            hostname.length = strlen(hoststr);
        }

        removeRecord(msc, config->mdnsConfig.mdnsServerName, hostname, port);
    }

    /* Close the sockets (async) */
    if(msc->cm) {
        if(msc->mdnsSendConnection)
            msc->cm->closeConnection(msc->cm, msc->mdnsSendConnection);
        for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++)
            if(msc->mdnsRecvConnections[i] != 0)
                msc->cm->closeConnection(msc->cm, msc->mdnsRecvConnections[i]);
    }

    /* Check if already fully closed */
    if(allConnectionsClosed(msc))
        sc->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
MdnsdServerComponent_free(UA_ServerComponent *sc) {
    MdnsdServerComponent *msc = (MdnsdServerComponent*)sc;

    if(sc->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(msc->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Cannot free multicast discovery before it is fully stopped");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Clean up the serverOnNetwork list */
    ServerOnNetwork *son;
    while((son = msc->serverList)) {
        msc->serverList = son->next;
        UA_ServerOnNetwork_clear(&son->serverOnNetwork);
        if(son->pathTmp)
            UA_free(son->pathTmp);
        UA_free(son);
    }

    /* Clean up mdns daemon */
    if(msc->mdnsDaemon) {
        mdnsd_shutdown(msc->mdnsDaemon);
        mdnsd_free(msc->mdnsDaemon);
        msc->mdnsDaemon = NULL;
    }

    UA_String_clear(&msc->selfMdnsRecord);
    UA_free(msc);

    return UA_STATUSCODE_GOOD;
}

UA_MdnsServerComponent *
UA_MdnsServerComponent_Mdnsd(void) {
    UA_MdnsServerComponent *msc = (UA_MdnsServerComponent*)
        UA_calloc(1, sizeof(MdnsdServerComponent));
    if(!msc)
        return NULL;

    msc->serverComponent.serverComponentType = UA_SERVERCOMPONENTTYPE_MDNS;
    msc->serverComponent.name = UA_STRING("discovery-mdns");
    msc->serverComponent.start = MdnsdServerComponent_start;
    msc->serverComponent.stop = MdnsdServerComponent_stop;
    msc->serverComponent.free = MdnsdServerComponent_free;
    msc->announce = MdnsdServerComponent_announce;
    msc->retract = MdnsdServerComponent_retract;

    return msc;
}

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */
