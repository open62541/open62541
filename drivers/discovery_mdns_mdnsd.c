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
#include <open62541/driver/mdns.h>
#include "open62541_queue.h"

#ifdef UA_ENABLE_DISCOVERY_MULTICAST_MDNSD

#include "mdnsd/mdnsd.h"
#ifndef UA_ENABLE_AMALGAMATION
#include "xht.h"
#include "sdtxt.h"
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

// TODO: Query

#define UA_MAXMDNSRECVSOCKETS 8
#define UA_MDNS_POLL_INTERVAL_MS 1000

/***************************/
/* Multicast DNS Discovery */
/***************************/

/* open62541 splits mDNS into two layers:
 *
 * 1) The mDNS driver handles the wire protocol. It sends and receives multicast
 *    DNS packets, maps OPC UA discovery information to DNS records and parses
 *    incoming records from other servers on the network.
 *
 * 2) The implementation here manages the discovery state in the server. The
 *    plugin reports discovered servers via UA_Server_registerServerOnNetwork()
 *    and UA_Server_deregisterServerOnNetwork(). The resulting
 *    UA_ServerOnNetwork entries back FindServersOnNetwork and the discovery
 *    notification callbacks.
 *
 * A typical OPC UA mDNS announcement consists of:
 *
 * - PTR:
 *   _opcua-tcp._tcp.local. -> [servername]-[hostname]._opcua-tcp._tcp.local.
 *   This advertises that an OPC UA TCP service instance exists.
 *
 * - SRV:
 *   [servername]-[hostname]._opcua-tcp._tcp.local. -> host + port
 *   This gives the target host and port for the discovery URL.
 *
 * - TXT:
 *   [servername]-[hostname]._opcua-tcp._tcp.local. -> path=..., caps=...
 *   This carries the discovery path and server capabilities.
 *
 * - A / AAAA:
 *   host.local. -> IP address
 *   This resolves the target host to an address.
 *
 * Lifecycle:
 *
 * - Announcement:
 *   The local server (or a registered remote server) is announced by the mDNS
 *   plugin. The plugin publishes PTR/SRV/TXT/A(AAAA) records and stores the
 *   normalized result here as a UA_ServerOnNetwork entry.
 *
 * - Update:
 *   A repeated announce or a changed SRV/TXT payload refreshes the existing
 *   UA_ServerOnNetwork entry. Depending on the packet order, TXT may arrive
 *   before SRV or vice versa; the plugin combines both into one normalized
 *   discovery URL plus capabilities and then reports the added/updated entry
 *   here.
 *
 * - Removal:
 *   A server can be retracted locally, or it can disappear remotely via a
 *   "goodbye" packet with TTL=0. The plugin translates that into
 *   UA_Server_deregisterServerOnNetwork(), which removes the normalized entry
 *   and emits a removal notification.
 */

typedef struct ServerOnNetworkRecord {
    LIST_ENTRY(ServerOnNetworkRecord) listPointers;
    UA_ServerOnNetwork serverOnNetwork;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char *pathTmp;
    UA_Boolean received;    /* Received over the network or locally created */
    UA_DateTime nextAction; /* If received: Remove after TTL
                             * If not received: Announce again */
    UA_UInt32 ttl;          /* If not received, the interval before sending
                             * again */
} ServerOnNetworkRecord;

typedef struct {
    UA_Driver drv;

    UA_Logger *logging; /* Shortcut from md->server->logging */

    mdns_daemon_t *mdnsDaemon;

    UA_ConnectionManager *cm;
    uintptr_t mdnsSendConnection;
    uintptr_t mdnsRecvConnections[UA_MAXMDNSRECVSOCKETS];
    size_t mdnsRecvConnectionsSize;
    UA_UInt64 sendCallbackId;

    LIST_HEAD(, ServerOnNetworkRecord) serverList;

    /* Configuration parameters */
    UA_Boolean listen;
    UA_Boolean announce;
    UA_UInt32 announceTTL;
    UA_String interface;
} MdnsdDriver;

static void
ServerOnNetworkRecord_delete(ServerOnNetworkRecord *son) {
    UA_ServerOnNetwork_clear(&son->serverOnNetwork);
    if(son->pathTmp)
        UA_free(son->pathTmp);
    UA_free(son);
}

static ServerOnNetworkRecord *
findSON(MdnsdDriver *md, UA_String serverName) {
    ServerOnNetworkRecord *son;
    LIST_FOREACH(son, &md->serverList, listPointers) {
        if(UA_String_equal(&serverName, &son->serverOnNetwork.serverName))
            return son;
    }
    return NULL;
}

static UA_StatusCode
addSON(MdnsdDriver *md, const UA_ServerOnNetwork *son,
       ServerOnNetworkRecord **addedEntry) {
    UA_LOG_INFO(md->logging, UA_LOGCATEGORY_DISCOVERY,
                "mDNS: Add record for %S", son->serverName);

    /* Allocate new */
    ServerOnNetworkRecord *entry = (ServerOnNetworkRecord*)
        UA_calloc(1, sizeof(ServerOnNetworkRecord));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Copy the SON */
    UA_StatusCode res =
        UA_ServerOnNetwork_copy(son, &entry->serverOnNetwork);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(entry);
        return res;
    }

    /* Add to the linked list */
    LIST_INSERT_HEAD(&md->serverList, entry, listPointers);
    
    /* Return pointer if requested */
    if(addedEntry != NULL)
        *addedEntry = entry;
    return UA_STATUSCODE_GOOD;
}

static void
removeSON(MdnsdDriver *md, UA_String serverName) {
    UA_LOG_INFO(md->logging, UA_LOGCATEGORY_DISCOVERY,
                "mDNS: Remove record for %S", serverName);

    /* Get and removed the internal entry */
    ServerOnNetworkRecord *entry = findSON(md, serverName);
    if(!entry)
        return;

    /* Deregister received entries. Locally created entries are getting removed
     * via a notification from inside UA_Server_deregisterServerOnNetwork.*/
    if(entry->received)
        UA_Server_deregisterServerOnNetwork(md->drv.server,
                                            entry->serverOnNetwork.serverName);

    /* Remove from linked list and clean up */
    LIST_REMOVE(entry, listPointers);
    ServerOnNetworkRecord_delete(entry);
}

/****************************/
/* Process Received Records */
/****************************/

static UA_StatusCode
processTxt(ServerOnNetworkRecord *entry, const struct resource *r) {
    /* Parse the record */
    xht_t *x = txt2sd(r->rdata, r->rdlength);
    if(!x)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Process the path */
    char *path = (char *) xht_get(x, "path");
    size_t pathLen = path ? strlen(path) : 0;
    if(pathLen > 1) {
        if(!entry->srvSet) {
            /* TXT arrived before SRV. Cache path entry for assembly of the full
             * DiscoveryUrl */
            if(!entry->pathTmp) {
                entry->pathTmp = (char*)UA_malloc(pathLen+1);
                if(!entry->pathTmp) {
                    xht_free(x);
                    return UA_STATUSCODE_BADOUTOFMEMORY;;
                }
                memcpy(entry->pathTmp, path, pathLen);
                entry->pathTmp[pathLen] = '\0';
            }
        } else {
            /* SRV already there and discovery URL set. Add path to discovery URL */
            UA_String_append(&entry->serverOnNetwork.discoveryUrl, UA_STRING(path));
        }
    }

    /* Process the capabilities */
    char *caps = (char*)xht_get(x, "caps");
    if(caps && strlen(caps) > 0) {
        /* Count comma in caps */
        size_t capsCount = 1;
        for(size_t i = 0; caps[i]; i++) {
            if(caps[i] == ',')
                capsCount++;
        }

        /* Delete the old capabilities array */
        UA_Array_delete(entry->serverOnNetwork.serverCapabilities,
                        entry->serverOnNetwork.serverCapabilitiesSize,
                        &UA_TYPES[UA_TYPES_STRING]);

        /* Allocate capabilities array */
        entry->serverOnNetwork.serverCapabilities = (UA_String *)
            UA_Array_new(capsCount, &UA_TYPES[UA_TYPES_STRING]);
        if(!entry->serverOnNetwork.serverCapabilities) {
            entry->serverOnNetwork.serverCapabilitiesSize = 0;
            xht_free(x);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        entry->serverOnNetwork.serverCapabilitiesSize = capsCount;

        /* Add capabilities to the array */
        char *nextStr = strchr(caps, ',');
        for(size_t i = 0; i < capsCount; i++, caps = nextStr + 1) {
            if(nextStr)
                *nextStr = '\0';
            entry->serverOnNetwork.serverCapabilities[i] =
                UA_STRING_ALLOC(caps);
        }
    }

    xht_free(x);
    entry->txtSet = true;
    return UA_STATUSCODE_GOOD;
}

/* Example: _opcua-tcp._tcp.[servername]. 86400 IN SRV 0 5 4840 [hostname]. */
static void
processSrv(ServerOnNetworkRecord *entry, const struct resource *r) {
    /* The specification Part 12 says: The hostname maps onto the SRV record
     * target field. If the hostname is an IP-address then it must be converted
     * to a domain name. If this cannot be done then LDS shall report an
     * error. */

    /* Cut off the last dot from the hostname */
    UA_String hostname = {strlen(r->known.srv.name), (UA_Byte*)r->known.srv.name};
    if(hostname.length > 0 && hostname.data[hostname.length - 1] == '.')
        hostname.length--;

    /* Prepare the DiscoveryUrl as opc.tcp://[servername]:[port][path] */
    UA_String_format(&entry->serverOnNetwork.discoveryUrl,
                     "opc.tcp://%S:%d", hostname, r->known.srv.port);
    if(entry->pathTmp) {
        UA_String_append(&entry->serverOnNetwork.discoveryUrl,
                         UA_STRING(entry->pathTmp));
        UA_free(entry->pathTmp);
        entry->pathTmp = NULL;
    }

    entry->srvSet = true;
}

/* Called by the mDNS library on every received record */
static void
processRecord(const struct resource *r, void *data) {
    MdnsdDriver *md = (MdnsdDriver*)data;

    /* Are we even listening? */
    if(!md->listen)
        return;

    /* We only need SRV and TXT records */
    /* TODO: remove magic number */
    if((r->clazz != QCLASS_IN && r->clazz != QCLASS_IN + 32768) ||
       (r->type != QTYPE_SRV && r->type != QTYPE_TXT))
        return;

    /* Only handle '._opcua-tcp._tcp.' records */
    char *opcStr = strstr(r->name, "._opcua-tcp._tcp.");
    if(!opcStr)
        return;

    /* Extract the servername */
    UA_String serverName = {(size_t)(opcStr - r->name), (UA_Byte*)r->name};
    if(serverName.length == 0)
        return;

    /* Find an existing entry or create a new one */
    ServerOnNetworkRecord *entry = findSON(md, serverName);
    if(!entry) {
        if(r->ttl == 0)
            return;
        UA_ServerOnNetwork son;
        UA_ServerOnNetwork_init(&son);
        son.serverName = serverName;
        UA_StatusCode res = addSON(md, &son, &entry);
        if(res != UA_STATUSCODE_GOOD)
            return;
    }

    /* We have sent out the entry ourselves */
    if(!entry->received)
        return;

    /* If TTL == 0, the entry is retracted. */
    if(r->ttl == 0) {
        removeSON(md, serverName);
        return;
    }

    /* Process the record. These can arrive in any order. */
    if(r->type == QTYPE_TXT)
        processTxt(entry, r);
    else if(r->type == QTYPE_SRV)
        processSrv(entry, r);

    /* Set TTL until we require the next received value */
    if(r->ttl > entry->ttl)
        entry->ttl = r->ttl;
       
    /* Update nextAction. If no update is received until then the record is
     * deleted */
    UA_EventLoop *el = UA_Server_getConfig(md->drv.server)->eventLoop;
    entry->nextAction =
        el->dateTime_nowMonotonic(el) + (UA_DATETIME_SEC * entry->ttl);

    /* Received over mDNS (instead of locally created) */
    entry->received = true;

    /* If the entry is complete, forward it to the server */
    if(entry->srvSet && entry->txtSet)
        UA_Server_registerServerOnNetwork(md->drv.server,
                                          &entry->serverOnNetwork,
                                          UA_KEYVALUEMAP_NULL);
}

/*******************/
/* Announce Record */
/*******************/

static void
flushMulticastMessages(MdnsdDriver *md) {
    UA_ConnectionManager *cm = md->cm;
    if(!cm || md->mdnsSendConnection == 0)
        return;

    struct in_addr ip;
    memset(&ip, 0, sizeof(struct in_addr));

    struct message mm;
    memset(&mm, 0, sizeof(struct message));

    unsigned short sport = 0;
    while(mdnsd_out(md->mdnsDaemon, &mm, &ip, &sport) > 0) {
        int len = message_packet_len(&mm);
        char* buf = (char*)message_packet(&mm);
        if(len <= 0)
            continue;
        UA_ByteString sendBuf = UA_BYTESTRING_NULL;
        UA_StatusCode rv = cm->allocNetworkBuffer(cm, md->mdnsSendConnection,
                                                  &sendBuf, (size_t)len);
        if(rv != UA_STATUSCODE_GOOD)
            continue;
        memcpy(sendBuf.data, buf, sendBuf.length);
        cm->sendWithConnection(cm, md->mdnsSendConnection,
                               &UA_KEYVALUEMAP_NULL, &sendBuf);
    }
}

/* Log a conflict */
static void
multicastConflict(char *name, int type, void *arg) {
    (void)name;
    (void)type;
    MdnsdDriver *md = (MdnsdDriver*)arg;
    UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                 "mDNS: Name conflict for '%s' of type %d", name, type);
}

static UA_UInt32
announceTTL(const MdnsdDriver *md) {
    return (md->announceTTL > 0) ? md->announceTTL : 600u;
}

static mdns_record_t *
findRecord(mdns_daemon_t *mdnsDaemon, unsigned short type,
           const char *host, const char *rdname) {
    for(mdns_record_t *r = mdnsd_get_published(mdnsDaemon, host);
        r != NULL; r = mdnsd_record_next(r)) {
        const mdns_answer_t *data = mdnsd_record_data(r);
        if(data->type == type && strcmp(data->rdname, rdname) == 0)
            return r;
    }
    return NULL;
}

/* PTR record: _opcua-tcp._tcp.local. PTR [servername]-[hostname]._opcua-tcp._tcp.local. */
static UA_StatusCode
announcePTR(MdnsdDriver *md, const char *serviceDomain,
            const UA_ServerOnNetwork *son, UA_UInt32 ttl) {
    mdns_record_t *r = findRecord(md->mdnsDaemon, QTYPE_PTR,
                                  "_opcua-tcp._tcp.local.", serviceDomain);
    if(!r)
        r = mdnsd_shared(md->mdnsDaemon, "_opcua-tcp._tcp.local.",
                         QTYPE_PTR, ttl);
    if(!r)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mdnsd_set_host(md->mdnsDaemon, r, serviceDomain);
    return UA_STATUSCODE_GOOD;
}

/* SRV record: [servername]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port [hostname].local.
 * The first 63 characters of the hostname (or less) and ".local." */
static UA_StatusCode
announceSVR(MdnsdDriver *md, UA_String hostname, UA_UInt16 port,
            const char *serviceDomain, const UA_ServerOnNetwork *son,
            UA_UInt32 ttl) {
    size_t maxHostnameLen = UA_MIN(hostname.length, 63);
    char localDomain[71];
    memcpy(localDomain, hostname.data, maxHostnameLen);
    strcpy(localDomain + maxHostnameLen, ".local.");
    mdns_record_t *r =
        mdnsd_unique(md->mdnsDaemon, serviceDomain, QTYPE_SRV,
                     ttl, multicastConflict, md);

    if(!r)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mdnsd_set_srv(md->mdnsDaemon, r, 0, 0, port, localDomain);
    return UA_STATUSCODE_GOOD;
}

/* TXT record: [servername]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,... */
static UA_StatusCode
announceTXT(MdnsdDriver *md, UA_String path, const char *serviceDomain,
            const UA_ServerOnNetwork *son, UA_UInt32 ttl) {
    /* Create the table for the TXT payload */
    xht_t *h = xht_new(11);
    if(!h)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Buffers for the path and caps values. They are referenced by xht and
     * have to live until sd2txt has copied their contents out (and ideally
     * stay alive in the function frame to keep ASan happy). */
    char pathBuf[256];
    char caps[256];

    /* Add the path */
    if(path.length >= 254) {
        xht_free(h);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(path.length == 0) {
        xht_set(h, "path", "/");
    } else {
        size_t pathLen = path.length;
        if(path.data[0] != '/') {
            /* Prepend missing slash */
            pathBuf[0] = '/';
            memcpy(&pathBuf[1], path.data, path.length);
            pathLen++;
        } else {
            memcpy(pathBuf, path.data, path.length);
        }

        pathBuf[pathLen] = '\0';
        xht_set(h, "path", pathBuf);
    }

    /* Calculate string length for the capabilities */
    size_t capsLen = 0;
    for(size_t i = 0; i < son->serverCapabilitiesSize; i++) {
        capsLen += son->serverCapabilities[i].length + 1;
    }
    if(capsLen >= 256) {
        xht_free(h);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add the capabilities */
    if(son->serverCapabilitiesSize > 0) {
        size_t idx = 0;
        for(size_t i = 0; i < son->serverCapabilitiesSize; i++) {
            memcpy(caps + idx, son->serverCapabilities[i].data,
                   son->serverCapabilities[i].length);
            idx += son->serverCapabilities[i].length + 1;
            caps[idx - 1] = ',';
        }
        caps[capsLen] = '\0';
        xht_set(h, "caps", caps);
    } else {
        xht_set(h, "caps", "NA"); /* NA - Not Available */
    }

    /* Encode the packet and set in the MDNSD daemon */
    int len;
    unsigned char *packet = sd2txt(h, &len);
    xht_free(h);
    if(!packet)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Create the record and set the payload */
    mdns_record_t *r =
        mdnsd_unique(md->mdnsDaemon, serviceDomain,
                     QTYPE_TXT, ttl, multicastConflict, md);
    if(!r) {
        UA_free(packet);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    mdnsd_set_raw(md->mdnsDaemon, r, (char*)packet, (unsigned short)len);
    UA_free(packet);

    return UA_STATUSCODE_GOOD;
}

/* Create a mDNS Record for the given server info and adds it to the mDNS output
 * queue.
 *
 * We assume that the hostname is not an IP address, but a valid domain name. It
 * is required by the OPC UA spec (see Part 12, DiscoveryURL to DNS SRV mapping)
 * to always use the hostname instead of the IP address. */
static UA_StatusCode
announceRecord(MdnsdDriver *md, const ServerOnNetworkRecord *entry) {
    if(!md->announce)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_ServerOnNetwork *son = &entry->serverOnNetwork;

    /* Use a limit for the ServerName to make sure it fits into 63 chars
     * (limited by DNS spec) */
    if(son->serverName.length > 63) {
        UA_LOG_WARNING(md->logging, UA_LOGCATEGORY_DISCOVERY,
                       "mDNS: ServerName of %S exceeds maximum of 63 chars",
                       son->serverName);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Extract hostname, port and path form the DiscoveryUrl */
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_StatusCode res =
        UA_parseEndpointUrl(&son->discoveryUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the service name [servername]._opcua-tcp._tcp.local. */
    char serviceDomainBuf[63+25];
    UA_String serviceDomain = {63+24, (UA_Byte*)serviceDomainBuf};
    UA_String_format(&serviceDomain, "%S._opcua-tcp._tcp.local.", son->serverName);
    serviceDomain.data[serviceDomain.length] = '\0';

    UA_LOG_INFO(md->logging, UA_LOGCATEGORY_DISCOVERY,
                "mDNS: Announcing record for %S", serviceDomain);

    /* _services._dns-sd._udp.local. PTR _opcua-tcp._tcp.local */
    /* if(!md->mdnsMainSrvAdded) { */
    /*     mdns_record_t *r = mdnsd_shared(md->mdnsDaemon, "_services._dns-sd._udp.local.", QTYPE_PTR, ttl); */
    /*     mdnsd_set_host(md->mdnsDaemon, r, "_opcua-tcp._tcp.local."); */
    /*     md->mdnsMainSrvAdded = true; */
    /* } */

    /* /\* A/AAAA record: For all IP-addresses */
    /*  * [servername]._opcua-tcp._tcp.local. A [ip]. */
    /*  * [hostname].local. A [ip]. *\/ */
    /* mdns_set_address_record(md, serviceDomainBuf, localDomain, ttl); */

    /* Announce the different mDNS records */
    res |= announcePTR(md, serviceDomainBuf, son, entry->ttl);
    res |= announceSVR(md, hostname, port, serviceDomainBuf, son, entry->ttl);
    res |= announceTXT(md, path, serviceDomainBuf, son, entry->ttl);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(md->logging, UA_LOGCATEGORY_DISCOVERY,
                    "mDNS: Could not create record for %S with StatusCode %s",
                    serviceDomain, UA_StatusCode_name(res));
    }

    /* Send out the queued messages */
    flushMulticastMessages(md);

    return UA_STATUSCODE_GOOD;
}

/******************/
/* Retract Record */
/******************/

static UA_StatusCode
retractRecord(MdnsdDriver *md, const UA_ServerOnNetwork *son) {
    if(!md->announce)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Use a limit for the ServerName to make sure it fits into 63 chars
     * (limited by DNS spec) */
    if(son->serverName.length > 63) {
        UA_LOG_WARNING(md->logging, UA_LOGCATEGORY_DISCOVERY,
                       "mDNS: ServerName of %S exceeds maximum of 63 chars",
                       son->serverName);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Extract hostname, port and path form the DiscoveryUrl */
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_UInt16 port = 4840;
    UA_StatusCode res =
        UA_parseEndpointUrl(&son->discoveryUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the service name [servername]._opcua-tcp._tcp.local. */
    char serviceDomainBuf[63+25];
    UA_String serviceDomain = {63+24, (UA_Byte*)serviceDomainBuf};
    UA_String_format(&serviceDomain, "%S._opcua-tcp._tcp.local.", son->serverName);
    serviceDomain.data[serviceDomain.length] = '\0';

    UA_LOG_INFO(md->logging, UA_LOGCATEGORY_DISCOVERY,
                "mDNS: Retracting record for %S", serviceDomain);

    /* _opcua-tcp._tcp.local. PTR [servername]._opcua-tcp._tcp.local. */
    mdns_record_t *r =
        findRecord(md->mdnsDaemon, QTYPE_PTR, "_opcua-tcp._tcp.local.", serviceDomainBuf);
    if(r)
        mdnsd_done(md->mdnsDaemon, r);

    /* SRV record: [servername]._opcua-tcp._tcp.local. 86400 IN SRV 0 5 port hostname.local.
     * TXT record: [servername]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
     * A record:   [servername]._opcua-tcp._tcp.local. A [ip] */
    mdns_record_t *r2 = mdnsd_get_published(md->mdnsDaemon, serviceDomainBuf);
    while(r2) {
        const mdns_answer_t *data = mdnsd_record_data(r2);
        mdns_record_t *next = mdnsd_record_next(r2);
        if(data->type == QTYPE_TXT || data->type == QTYPE_A || data->srv.port == port) {
            mdnsd_done(md->mdnsDaemon, r2);
        }
        r2 = next;
    }

    return UA_STATUSCODE_GOOD;
}

/* #ifdef UA_ARCHITECTURE_WIN32 */

/* /\* see http://stackoverflow.com/a/10838854/869402 *\/ */
/* static IP_ADAPTER_ADDRESSES * */
/* getInterfaces(MdnsdDriver *md) { */
/*     IP_ADAPTER_ADDRESSES* adapter_addresses = NULL; */

/*     /\* Start with a 16 KB buffer and resize if needed - multiple attempts in */
/*      * case interfaces change while we are in the middle of querying them. *\/ */
/*     DWORD adapter_addresses_buffer_size = 16 * 1024; */
/*     for(size_t attempts = 0; attempts != 3; ++attempts) { */
/*         /\* todo: malloc may fail: return a statuscode *\/ */
/*         adapter_addresses = (IP_ADAPTER_ADDRESSES*)UA_malloc(adapter_addresses_buffer_size); */
/*         if(!adapter_addresses) { */
/*             UA_LOG_ERROR(dm->logging, UA_LOGCATEGORY_DISCOVERY, */
/*                          "GetAdaptersAddresses out of memory"); */
/*             adapter_addresses = NULL; */
/*             break; */
/*         } */
/*         DWORD error = GetAdaptersAddresses(AF_UNSPEC, */
/*                                            GAA_FLAG_SKIP_ANYCAST | */
/*                                            GAA_FLAG_SKIP_DNS_SERVER | */
/*                                            GAA_FLAG_SKIP_FRIENDLY_NAME, */
/*                                            NULL, adapter_addresses, */
/*                                            &adapter_addresses_buffer_size); */

/*         if(ERROR_SUCCESS == error) { */
/*             break; */
/*         } else if (ERROR_BUFFER_OVERFLOW == error) { */
/*             /\* Try again with the new size *\/ */
/*             UA_free(adapter_addresses); */
/*             adapter_addresses = NULL; */
/*             continue; */
/*         } */

/*         /\* Unexpected error *\/ */
/*         UA_LOG_ERROR(dm->logging, UA_LOGCATEGORY_SERVER, */
/*                      "GetAdaptersAddresses returned an unexpected error. " */
/*                      "Not setting mDNS A records."); */
/*         UA_free(adapter_addresses); */
/*         adapter_addresses = NULL; */
/*         break; */
/*     } */

/*     return adapter_addresses; */
/* } */

/* #endif /\* UA_ARCHITECTURE_WIN32 *\/ */

/* /\* set record in the given interface *\/ */
/* static void */
/* mdns_set_address_record_if(MdnsdDriver *md, const char *fullServiceDomain, */
/*                            const char *localDomain, char *addr, UA_UInt16 addr_len, */
/*                            UA_UInt32 ttl) { */
/*     /\* [servername]-[hostname]._opcua-tcp._tcp.local. A [ip]. *\/ */
/*     mdns_record_t *r = mdnsd_shared(md->mdnsDaemon, fullServiceDomain, QTYPE_A, */
/*                                     ttl); */
/*     mdnsd_set_raw(md->mdnsDaemon, r, addr, addr_len); */

/*     /\* [hostname]. A [ip]. *\/ */
/*     r = mdnsd_shared(md->mdnsDaemon, localDomain, QTYPE_A, ttl); */
/*     mdnsd_set_raw(md->mdnsDaemon, r, addr, addr_len); */
/* } */

/* /\* Loop over network interfaces and run set_address_record on each *\/ */
/* #ifdef UA_ARCHITECTURE_WIN32 */
/* static void */
/* mdns_set_address_record(MdnsdDriver *md, const char *fullServiceDomain, */
/*                         const char *localDomain, UA_UInt32 ttl) { */
/*     IP_ADAPTER_ADDRESSES* adapter_addresses = getInterfaces(md); */
/*     if(!adapter_addresses) */
/*         return; */

/*     /\* Iterate through all of the adapters *\/ */
/*     IP_ADAPTER_ADDRESSES* adapter = adapter_addresses; */
/*     for(; adapter != NULL; adapter = adapter->Next) { */
/*         /\* Skip loopback adapters *\/ */
/*         if(IF_TYPE_SOFTWARE_LOOPBACK == adapter->IfType) */
/*             continue; */

/*         /\* Parse all IPv4 and IPv6 addresses *\/ */
/*         IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; */
/*         for(; NULL != address; address = address->Next) { */
/*             int family = address->Address.lpSockaddr->sa_family; */
/*             if(AF_INET == family) { */
/*                 SOCKADDR_IN* ipv4 = (SOCKADDR_IN*)(address->Address.lpSockaddr); /\* IPv4 *\/ */
/*                 mdns_set_address_record_if(md, fullServiceDomain, */
/*                                            localDomain, (char *)&ipv4->sin_addr, */
/*                                            4, ttl); */
/*             } else if(AF_INET6 == family) { */
/*                 /\* IPv6 *\/ */
/* #if 0 */
/*                 SOCKADDR_IN6* ipv6 = (SOCKADDR_IN6*)(address->Address.lpSockaddr); */

/*                 char str_buffer[INET6_ADDRSTRLEN] = {0}; */
/*                 inet_ntop(AF_INET6, &(ipv6->sin6_addr), str_buffer, INET6_ADDRSTRLEN); */

/*                 std::string ipv6_str(str_buffer); */

/*                 /\* Detect and skip non-external addresses *\/ */
/*                 UA_Boolean is_link_local(false); */
/*                 UA_Boolean is_special_use(false); */

/*                 if(0 == ipv6_str.find("fe")) { */
/*                     char c = ipv6_str[2]; */
/*                     if(c == '8' || c == '9' || c == 'a' || c == 'b') */
/*                         is_link_local = true; */
/*                 } else if (0 == ipv6_str.find("2001:0:")) { */
/*                     is_special_use = true; */
/*                 } */

/*                 if(!(is_link_local || is_special_use)) */
/*                     ipAddrs.mIpv6.push_back(ipv6_str); */
/* #endif */
/*             } */
/*         } */
/*     } */

/*     /\* Cleanup *\/ */
/*     UA_free(adapter_addresses); */
/*     adapter_addresses = NULL; */
/* } */

/* #elif defined(UA_HAS_GETIFADDR) */

/* static void */
/* mdns_set_address_record(MdnsdDriver *md, const char *fullServiceDomain, */
/*                         const char *localDomain, UA_UInt32 ttl) { */
/*     struct ifaddrs *ifaddr; */
/*     struct ifaddrs *ifa; */
/*     if(getifaddrs(&ifaddr) == -1) { */
/*         UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_SERVER, */
/*                      "getifaddrs returned an unexpected error. Not setting mDNS A records."); */
/*         return; */
/*     } */

/*     /\* Walk through linked list, maintaining head pointer so we can free list later *\/ */
/*     int n; */
/*     for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) { */
/*         if(!ifa->ifa_addr) */
/*             continue; */

/*         if((strcmp("lo", ifa->ifa_name) == 0) || */
/*            !(ifa->ifa_flags & (IFF_RUNNING))|| */
/*            !(ifa->ifa_flags & (IFF_MULTICAST))) */
/*             continue; */

/*         /\* IPv4 *\/ */
/*         if(ifa->ifa_addr->sa_family == AF_INET) { */
/*             struct sockaddr_in* sa = (struct sockaddr_in*) ifa->ifa_addr; */
/*             mdns_set_address_record_if(md, fullServiceDomain, */
/*                                        localDomain, (char*)&sa->sin_addr.s_addr, */
/*                                        4, ttl); */
/*         } */

/*         /\* IPv6 not implemented yet *\/ */
/*     } */

/*     /\* Clean up *\/ */
/*     freeifaddrs(ifaddr); */
/* } */
/* #else /\* UA_ARCHITECTURE_WIN32 *\/ */

/* void */
/* mdns_set_address_record(UA_DiscoveryManager *dm, const char *fullServiceDomain, */
/*                         const char *localDomain) { */
/*     if(md->drv.server->config.mdnsIpAddressListSize == 0) { */
/*         UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_SERVER, */
/*                      "If UA_HAS_GETIFADDR is false, config.mdnsIpAddressList must be set"); */
/*         return; */
/*     } */

/*     for(size_t i=0; i< md->drv.server->config.mdnsIpAddressListSize; i++) { */
/*         mdns_set_address_record_if(dm, fullServiceDomain, localDomain, */
/*                                    (char*)&md->drv.server->config.mdnsIpAddressList[i], 4); */
/*     } */
/* } */

/* #endif /\* UA_ARCHITECTURE_WIN32 *\/ */

static void
addConnection(MdnsdDriver *md, uintptr_t connectionId,
              UA_Boolean recv) {
    if(!recv) {
        md->mdnsSendConnection = connectionId;
        return;
    }
    size_t freeIdx = UA_MAXMDNSRECVSOCKETS;
    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        uintptr_t recvConn = md->mdnsRecvConnections[i];
        if(recvConn == connectionId)
            return;
        if(recvConn == 0 && freeIdx == UA_MAXMDNSRECVSOCKETS)
            freeIdx = i;
    }

    if(freeIdx == UA_MAXMDNSRECVSOCKETS)
        return;
    md->mdnsRecvConnections[freeIdx] = connectionId;
    md->mdnsRecvConnectionsSize++;
}

static void
removeConnection(MdnsdDriver *md, uintptr_t connectionId) {
    if(md->mdnsSendConnection == connectionId) {
        md->mdnsSendConnection = 0;
        return;
    }
    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        if(md->mdnsRecvConnections[i] != connectionId)
            continue;
        md->mdnsRecvConnections[i] = 0;
        md->mdnsRecvConnectionsSize--;
        break;
    }
}

static UA_Boolean
allConnectionsClosed(MdnsdDriver *md) {
    if(md->mdnsSendConnection != 0)
        return false;
    for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++) {
        if(md->mdnsRecvConnections[i] != 0)
            return false;
    }
    return true;
}

static void
MulticastDiscoveryCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                           void *_, void **connectionContext,
                           UA_ConnectionState state, const UA_KeyValueMap *params,
                           UA_ByteString msg, UA_Boolean recv);

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

/* Create multicast 224.0.0.251:5353 socket */
static void
createMulticastSocket(MdnsdDriver *md) {
    UA_Server *server = md->drv.server;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Find the UDP connection manager */
    if(!md->cm) {
        UA_String udpString = UA_STRING("udp");
        UA_EventSource *es = config->eventLoop->eventSources;
        for(; es != NULL; es = es->next) {
            /* Is this a usable connection manager? */
            if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
                continue;
            UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
            if(UA_String_equal(&udpString, &cm->protocol)) {
                md->cm = cm;
                break;
            }
        }
    }

    if(!md->cm) {
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "mDNS: No UDP ConnectionManager found");
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
    if(md->interface.length > 0) {
        params[5].key = UA_QUALIFIEDNAME(0, "interface");
        UA_Variant_setScalar(&params[5].value, &md->interface,
                             &UA_TYPES[UA_TYPES_STRING]);
        paramsSize++;
    }

    /* Open the listen connection */
    UA_KeyValueMap kvm = {paramsSize, params};
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    if(md->listen && md->mdnsRecvConnectionsSize == 0) {
        res = md->cm->openConnection(md->cm, &kvm, md->drv.server, md,
                                     MulticastDiscoveryRecvCallback);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                         "mDNS: Could not create the UDP multicast listen connection");
    }

    /* Open the send connection */
    if(md->announce && md->mdnsSendConnection == 0) {
        res = md->cm->openConnection(md->cm, &kvm, md->drv.server, md,
                                     MulticastDiscoverySendCallback);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                         "mDNS: Could not create the UDP multicast send connection");
    }
}

static void
MulticastDiscoveryCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                           void *_, void **connectionContext,
                           UA_ConnectionState state, const UA_KeyValueMap *params,
                           UA_ByteString msg, UA_Boolean recv) {
    MdnsdDriver *md = *(MdnsdDriver**)connectionContext;
    if(!md)
        return;

    if(state == UA_CONNECTIONSTATE_CLOSING) {
        /* Remove closed connection */
        removeConnection(md, connectionId);
        UA_Boolean allClosed = allConnectionsClosed(md);

        if(md->drv.state == UA_LIFECYCLESTATE_STOPPING) {
            if(allClosed)
                md->drv.state = UA_LIFECYCLESTATE_STOPPED;
        } else if(md->drv.state == UA_LIFECYCLESTATE_STARTED) {
            /* Restart mdns sockets if not shutting down */
            createMulticastSocket(md);
            if(md->mdnsSendConnection == 0)
                UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                             "mDNS: Could not create UDP multicast socket");
        }
        return;
    }

    addConnection(md, connectionId, recv);

    /* Expect packet between 0 and 512 bytes */
    if(msg.length == 0)
        return;
    if(msg.length >= 512)
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

    /* Zero-terminated buffer */
    unsigned char buf[512];
    memcpy(buf, msg.data, msg.length);
    buf[msg.length] = 0;

    /* Parse and process the message */
    struct message mm;
    memset(&mm, 0, sizeof(struct message));
    int rr = message_parse(&mm, buf);
    if(rr == 0) {
        struct sockaddr_in *addr = (struct sockaddr_in *)infoptr->ai_addr;
        mdnsd_in(md->mdnsDaemon, &mm, addr->sin_addr, addr->sin_port);
    }
    freeaddrinfo(infoptr);
}

/* Loop over all SON and perform required actions */
static void
MdnsdHouseKeeping(UA_Server *server, void *drv) {
    (void)server;
    MdnsdDriver *md = (MdnsdDriver*)drv;

    UA_EventLoop *el = UA_Server_getConfig(md->drv.server)->eventLoop;
    UA_DateTime now = el->dateTime_nowMonotonic(el);

    ServerOnNetworkRecord *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &md->serverList, listPointers, son_tmp) {
        /* Remove if stale */
        if(son->nextAction > now)
            continue;
        if(son->received) {
            removeSON(md, son->serverOnNetwork.serverName);
            continue;
        }
        /* Announce the record again.
         * Update nextAction to a timestamp before the TTL runs out. */
        announceRecord(md, son);
        son->nextAction = now + ((son->ttl * 0.9) * UA_DATETIME_SEC);
    }

    /* Send out queued messages */
    flushMulticastMessages(md);
}

/* Notifications from the server to the driver */
static void
MdnsdDriverNotificationCallback(UA_Driver *drv,
                                UA_ApplicationNotificationType type,
                                const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY_SERVERONNETWORK)
        return;

    if(drv->state != UA_LIFECYCLESTATE_STARTED)
        return;

    MdnsdDriver *md = (MdnsdDriver*)drv;
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* Extract the notification parameters */
    const UA_ServerOnNetwork *son = (const UA_ServerOnNetwork*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-on-network"),
                                 &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    if(!son)
        return;

    const UA_UInt32 *_ttl = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "ttl"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    UA_UInt32 ttl = (_ttl) ? *_ttl : announceTTL(md);

    const UA_Boolean *_added = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-added"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Boolean added = (_added) ? *_added : false;

    const UA_Boolean *_updated = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-updated"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Boolean updated = (_updated) ? *_updated : false;

    const UA_Boolean *_removed = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "server-removed"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Boolean removed = (_removed) ? *_removed : false;

    /* Find existing entry for the ServerName */
    ServerOnNetworkRecord *entry = findSON(md, son->serverName);

    /* Add or update record */
    if(added || updated) {
        if(!entry) {
            /* Create a new entry if none exists */
            res = addSON(md, son, &entry);
            if(res != UA_STATUSCODE_GOOD)
                return;
        } else {
            /* Abort if identical to what we have already */
            UA_Order same =
                UA_order(son, &entry->serverOnNetwork,
                         &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
            if(same == UA_ORDER_EQ)
                return;
            UA_ServerOnNetwork tmp;
            res = UA_ServerOnNetwork_copy(son, &tmp);
            if(res != UA_STATUSCODE_GOOD)
                return;
            UA_ServerOnNetwork_clear(&entry->serverOnNetwork);
            entry->serverOnNetwork = tmp;
        }

        /* Locally created and not received over the network */
        entry->received = false;
        entry->ttl = ttl;

        /* Announce right away */
        announceRecord(md, entry);

        /* Next time the entry is announced */
        UA_EventLoop *el = UA_Server_getConfig(md->drv.server)->eventLoop;
        UA_DateTime now = el->dateTime_nowMonotonic(el);
        entry->nextAction = now + ((entry->ttl * 0.9) * UA_DATETIME_SEC);
    }

    /* Remove record */
    if(removed) {
        /* Retract if we haven't received the entry the entry from mDNS */
        if((!entry || !entry->received))
            retractRecord(md, son);
        removeSON(md, entry->serverOnNetwork.serverName); /* Remove entry */
    }
}

static void
MdnsdDriver_stop(UA_Driver *drv) {
    UA_Server *server = drv->server;
    MdnsdDriver *md = (MdnsdDriver*)drv;

    /* Already stopped */
    if(drv->state == UA_LIFECYCLESTATE_STOPPED)
        return;

    /* Commence async stopping */
    drv->state = UA_LIFECYCLESTATE_STOPPING;

    /* Remove repeated callback to send out multicast messages */
    UA_Server_removeRepeatedCallback(server, md->sendCallbackId);
    md->sendCallbackId = 0;

    /* Flush queued goodbye packets before the UDP sockets are closed. This is
     * the last point where the component still owns live connections but no
     * further periodic queries will be scheduled. */
    flushMulticastMessages(md);

    /* Close the sockets (async) */
    if(md->cm) {
        if(md->mdnsSendConnection)
            md->cm->closeConnection(md->cm, md->mdnsSendConnection);
        for(size_t i = 0; i < UA_MAXMDNSRECVSOCKETS; i++)
            if(md->mdnsRecvConnections[i] != 0)
                md->cm->closeConnection(md->cm, md->mdnsRecvConnections[i]);
    }

    /* Check if already fully closed */
    if(allConnectionsClosed(md))
        drv->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
MdnsdDriver_start(UA_Driver *drv) {
    /* Check that the server has been set */
    MdnsdDriver *md = (MdnsdDriver*)drv;
    if(!drv->server)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Initialize networking on win32 */
#if defined(UA_ARCHITECTURE_WIN32) || defined(UA_ARCHITECTURE_WEC7)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    /* Set the logging shortcut */
    UA_ServerConfig *config = UA_Server_getConfig(drv->server);
    md->logging = config->logging;

    /* Check the state */
    if(drv->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "mDNS: Cannot start driver that is already running");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    drv->state = UA_LIFECYCLESTATE_STARTED;

    /* Initialize the list of server-on-network records. The list head is
     * embedded in the driver struct (zeroed by calloc) so the initial state
     * is an empty list. Re-initialize in case the driver is restarted. */
    LIST_INIT(&md->serverList);

    /* Extract the configuration parameters from the key-value map */
    const UA_Boolean *listen = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&drv->params,
                                 UA_QUALIFIEDNAME(0, "listen"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    md->listen = listen ? *listen : false;

    const UA_Boolean *announce = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&drv->params,
                                 UA_QUALIFIEDNAME(0, "announce"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    md->announce = announce ? *announce : false;

    const UA_UInt32 *announceTTL = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&drv->params,
                                 UA_QUALIFIEDNAME(0, "announce-ttl"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    md->announceTTL = announceTTL ? *announceTTL : 0;

    UA_String_clear(&md->interface);
    const UA_String *interface = (const UA_String*)
        UA_KeyValueMap_getScalar(&drv->params,
                                 UA_QUALIFIEDNAME(0, "interface"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(interface)
        UA_String_copy(interface, &md->interface);

    /* Create mDNS daemon */
    if(!md->mdnsDaemon) {
        md->mdnsDaemon = mdnsd_new(QCLASS_IN, 1000);
        if(!md->mdnsDaemon) {
            UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                         "mDNS: Could not create mDNS daemon");
            MdnsdDriver_stop(&md->drv);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        mdnsd_register_receive_callback(md->mdnsDaemon, processRecord, md);
    }

    /* Open the UDP sockets if needed */
    if(md->listen || md->announce) {
        if(md->mdnsSendConnection == 0)
            createMulticastSocket(md);
        if(md->mdnsSendConnection == 0) {
            UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                         "mDNS: Could not create UDP multicast socket");
            MdnsdDriver_stop(&md->drv);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    /* Add a repeated callback to process actions and send out multicast
     * messages */
    UA_Server_addRepeatedCallback(md->drv.server, MdnsdHouseKeeping,
                                  md, UA_MDNS_POLL_INTERVAL_MS,
                                  &md->sendCallbackId);

    /* /\* Send an initial multicast probe if active presence queries are enabled *\/ */
    /* if(md->msc.queryPresence > 0) { */
    /*     UA_EventLoop *el = config->eventLoop; */
    /*     mdnsd_query(md->mdnsDaemon, "_opcua-tcp._tcp.local.", */
    /*                 QTYPE_PTR, multicastQueryAnswer, md); */
    /*     md->lastPresenceQuery = el->dateTime_nowMonotonic(el); */
    /* } */

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
MdnsdDriver_free(UA_Driver *drv) {
    MdnsdDriver *md = (MdnsdDriver*)drv;

    if(drv->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                     "Cannot free multicast discovery before it is fully stopped");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Clean up the serverOnNetwork list */
    ServerOnNetworkRecord *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &md->serverList, listPointers, son_tmp) {
        LIST_REMOVE(son, listPointers);
        ServerOnNetworkRecord_delete(son);
    }

    /* Clean up mdns daemon */
    if(md->mdnsDaemon) {
        mdnsd_shutdown(md->mdnsDaemon);
        mdnsd_free(md->mdnsDaemon);
        md->mdnsDaemon = NULL;
    }

    /* Clean up the configuration */
    UA_String_clear(&md->interface);

    UA_free(md);

    return UA_STATUSCODE_GOOD;
}

UA_Driver *
UA_MdnsDriver_Mdnsd(const UA_KeyValueMap params) {
    /* Allocate the memory */
    MdnsdDriver *md = (MdnsdDriver*)
        UA_calloc(1, sizeof(MdnsdDriver));
    if(!md)
        return NULL;

    /* Copy over the parameters */
    UA_StatusCode res =
        UA_KeyValueMap_copy(&params, &md->drv.params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(md);
        return NULL;
    }

    /* Set the name and function pointers */
    md->drv.name = UA_STRING("discovery-mdns");
    md->drv.start = MdnsdDriver_start;
    md->drv.stop = MdnsdDriver_stop;
    md->drv.free = MdnsdDriver_free;

    /* Callback for server notifications */
    md->drv.notificationCallback = MdnsdDriverNotificationCallback;
    md->drv.notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY;

    return &md->drv;
}

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */
