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
# include <arpa/inet.h>
# include <netinet/in.h> // for struct ip_mreq
# if defined(UA_HAS_GETIFADDR)
#  include <ifaddrs.h>
# endif /* UA_HAS_GETIFADDR */
# include <net/if.h> /* for IFF_RUNNING */
# include <netdb.h> // for recvfrom in cygwin
#endif

#define UA_MAXMDNSRECVSOCKETS 8
#define UA_MDNS_POLL_INTERVAL_MS 1000
#define UA_MDNS_MAX_RECORD_NAME_LENGTH 256
#define UA_MDNS_OPCUA_TCP_LOCAL "_opcua-tcp._tcp.local."

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
 * Lifecycle:
 *
 * - Announcement:
 *   The local server (or a registered remote server) is announced by the mDNS
 *   plugin. The plugin publishes PTR/SRV/TXT records and stores the normalized
 *   result here as a UA_ServerOnNetwork entry. The UDP event loop owns the
 *   multicast socket mechanics such as interface selection, group membership,
 *   send-interface selection and packet TTL.
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

typedef enum {
    SERVER_ON_NETWORK_RECORD_REMOTE_RECEIVED,
    SERVER_ON_NETWORK_RECORD_LOCAL_OWNED
} ServerOnNetworkRecordOrigin;

typedef struct ServerOnNetworkRecord {
    LIST_ENTRY(ServerOnNetworkRecord) listPointers;
    UA_ServerOnNetwork serverOnNetwork;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char *pathTmp;
    ServerOnNetworkRecordOrigin origin;
    UA_DateTime nextAction; /* Remote: Remove after TTL
                             * Local: Announce again */
    UA_UInt32 ttl;          /* Local: the interval before sending again */
} ServerOnNetworkRecord;

typedef struct {
    UA_MdnsDriver mdns;

    UA_Logger *logging; /* Shortcut from md->server->logging */

    mdns_daemon_t *mdnsDaemon;

    UA_ConnectionManager *cm;
    uintptr_t mdnsSendConnection;
    uintptr_t mdnsRecvConnections[UA_MAXMDNSRECVSOCKETS];
    size_t mdnsRecvConnectionsSize;
    UA_UInt64 sendCallbackId;
    UA_UInt64 presenceQueryCallbackId;

    LIST_HEAD(, ServerOnNetworkRecord) serverList;

    /* Configuration parameters */
    UA_Boolean listen;
    UA_Boolean announce;
    UA_Boolean queryPresence;
    UA_Boolean queryDetails;
    UA_UInt32 queryInterval;
    UA_UInt32 announceTTL;
    UA_String interface;
} MdnsdDriver;

static void
flushMulticastMessages(MdnsdDriver *md);

static void
stopRecordDetailsQueryForEntry(MdnsdDriver *md,
                               const ServerOnNetworkRecord *entry);

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
removeSONEntry(MdnsdDriver *md, ServerOnNetworkRecord *entry,
               UA_Boolean deregisterFromServer) {
    UA_LOG_INFO(md->logging, UA_LOGCATEGORY_DISCOVERY,
                "mDNS: Remove record for %S", entry->serverOnNetwork.serverName);

    /* Remove from linked list first to break the recursion. The driver's
     * notification callback can be triggered by UA_Server_deregisterServerOnNetwork
     * and would otherwise re-enter with the same entry. */
    LIST_REMOVE(entry, listPointers);

    if(deregisterFromServer)
        UA_Server_deregisterServerOnNetwork(md->mdns.drv.server,
                                            entry->serverOnNetwork.serverName);

    ServerOnNetworkRecord_delete(entry);
}

static void
removeReceivedSON(MdnsdDriver *md, ServerOnNetworkRecord *entry) {
    /* This record came from the network. Removing it is a cache update and must
     * not emit a multicast goodbye for somebody else's service. Deregistering
     * from the server updates FindServersOnNetwork and notifies the application. */
    stopRecordDetailsQueryForEntry(md, entry);
    removeSONEntry(md, entry, true);
}

static void
removeLocalSON(MdnsdDriver *md, ServerOnNetworkRecord *entry) {
    /* The server notification already removed the locally-owned record from the
     * server's public discovery table. Only remove the driver's bookkeeping. */
    removeSONEntry(md, entry, false);
}

static UA_Boolean
isRemoteReceivedSON(const ServerOnNetworkRecord *entry) {
    return entry->origin == SERVER_ON_NETWORK_RECORD_REMOTE_RECEIVED;
}

static UA_Boolean
isLocalOwnedSON(const ServerOnNetworkRecord *entry) {
    return entry->origin == SERVER_ON_NETWORK_RECORD_LOCAL_OWNED;
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
        for(size_t i = 0; i < capsCount; i++) {
            char *nextStr = strchr(caps, ',');
            if(nextStr)
                *nextStr = '\0';
            entry->serverOnNetwork.serverCapabilities[i] =
                UA_STRING_ALLOC(caps);
            if(nextStr)
                caps = nextStr + 1;
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

static int
multicastQueryAnswer(mdns_answer_t *a, void *arg) {
    (void)a;
    (void)arg;
    return 0;
}

static UA_Boolean
serviceInstanceToServerName(char *serviceInstance, UA_String *serverName) {
    char *opcStr = strstr(serviceInstance, "." UA_MDNS_OPCUA_TCP_LOCAL);
    if(!opcStr)
        return false;

    serverName->length = (size_t)(opcStr - serviceInstance);
    serverName->data = (UA_Byte*)serviceInstance;
    return serverName->length > 0;
}

static void
queryRecordDetails(MdnsdDriver *md, const char *serviceInstance,
                   UA_Boolean querySrv, UA_Boolean queryTxt) {
    if(!md->queryDetails || !serviceInstance || !md->mdnsDaemon)
        return;

    if(querySrv)
        mdnsd_query(md->mdnsDaemon, serviceInstance, QTYPE_SRV,
                    multicastQueryAnswer, md);
    if(queryTxt)
        mdnsd_query(md->mdnsDaemon, serviceInstance, QTYPE_TXT,
                    multicastQueryAnswer, md);
    if(querySrv || queryTxt)
        flushMulticastMessages(md);
}

static void
stopRecordDetailsQuery(MdnsdDriver *md, const char *serviceInstance) {
    if(!md->queryDetails || !serviceInstance || !md->mdnsDaemon)
        return;

    mdnsd_query(md->mdnsDaemon, serviceInstance, QTYPE_SRV, NULL, NULL);
    mdnsd_query(md->mdnsDaemon, serviceInstance, QTYPE_TXT, NULL, NULL);
}

static void
stopRecordDetailsQueryForEntry(MdnsdDriver *md,
                               const ServerOnNetworkRecord *entry) {
    const UA_String *serverName = &entry->serverOnNetwork.serverName;
    const char suffix[] = "." UA_MDNS_OPCUA_TCP_LOCAL;
    char serviceInstance[UA_MDNS_MAX_RECORD_NAME_LENGTH];
    if(serverName->length + sizeof(suffix) > sizeof(serviceInstance))
        return;

    memcpy(serviceInstance, serverName->data, serverName->length);
    memcpy(serviceInstance + serverName->length, suffix, sizeof(suffix));
    stopRecordDetailsQuery(md, serviceInstance);
}

static void
queryPresence(MdnsdDriver *md) {
    if(!md->queryPresence || !md->mdnsDaemon)
        return;

    mdnsd_query(md->mdnsDaemon, UA_MDNS_OPCUA_TCP_LOCAL, QTYPE_PTR,
                multicastQueryAnswer, md);
    flushMulticastMessages(md);
    mdnsd_query(md->mdnsDaemon, UA_MDNS_OPCUA_TCP_LOCAL, QTYPE_PTR,
                NULL, NULL);
}

static void
MdnsdPresenceQueryCallback(UA_Server *server, void *drv) {
    (void)server;
    queryPresence((MdnsdDriver*)drv);
}

/* Called by the mDNS library on every received record */
static void
processRecord(const struct resource *r, void *data) {
    MdnsdDriver *md = (MdnsdDriver*)data;

    /* Are we even listening? */
    if(!md->listen)
        return;

    /* We only need PTR, SRV and TXT records */
    /* TODO: remove magic number */
    if((r->clazz != QCLASS_IN && r->clazz != QCLASS_IN + 32768) ||
       (r->type != QTYPE_PTR && r->type != QTYPE_SRV && r->type != QTYPE_TXT))
        return;

    char *serviceInstance = r->name;
    if(r->type == QTYPE_PTR) {
        if(strcmp(r->name, UA_MDNS_OPCUA_TCP_LOCAL) != 0)
            return;
        serviceInstance = r->known.ptr.name;
        if(!serviceInstance)
            return;
    }

    /* Only handle '._opcua-tcp._tcp.' records */
    UA_String serverName;
    if(!serviceInstanceToServerName(serviceInstance, &serverName))
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
        entry->origin = SERVER_ON_NETWORK_RECORD_REMOTE_RECEIVED;
    } else {
        /* Local entries are already owned by this driver. Ignore matching
         * network traffic so our own announcements are not mirrored back into
         * the local discovery table. */
        if(isLocalOwnedSON(entry))
            return;
    }

    /* If TTL == 0, the entry is retracted. */
    if(r->ttl == 0) {
        removeReceivedSON(md, entry);
        return;
    }

    /* Process the record. These can arrive in any order. */
    if(r->type == QTYPE_PTR)
        queryRecordDetails(md, serviceInstance, !entry->srvSet, !entry->txtSet);
    else if(r->type == QTYPE_TXT)
        processTxt(entry, r);
    else if(r->type == QTYPE_SRV)
        processSrv(entry, r);

    if(r->type == QTYPE_TXT && !entry->srvSet)
        queryRecordDetails(md, serviceInstance, true, false);
    else if(r->type == QTYPE_SRV && !entry->txtSet)
        queryRecordDetails(md, serviceInstance, false, true);

    /* Set TTL until we require the next received value */
    if(r->ttl > entry->ttl)
        entry->ttl = r->ttl;

    /* Update nextAction. If no update is received until then the record is
     * deleted */
    UA_EventLoop *el = UA_Server_getConfig(md->mdns.drv.server)->eventLoop;
    entry->nextAction =
        el->dateTime_nowMonotonic(el) + (UA_DATETIME_SEC * entry->ttl);

    /* Received over mDNS (instead of locally created) */
    entry->origin = SERVER_ON_NETWORK_RECORD_REMOTE_RECEIVED;

    /* If the entry is complete, forward it to the server */
    if(entry->srvSet && entry->txtSet) {
        stopRecordDetailsQuery(md, serviceInstance);
        UA_Server_registerServerOnNetwork(md->mdns.drv.server,
                                          &entry->serverOnNetwork,
                                          UA_KEYVALUEMAP_NULL);
    }
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
    mdns_record_t *r = mdnsd_find(md->mdnsDaemon, serviceDomain, QTYPE_SRV);
    if(!r)
        r = mdnsd_shared(md->mdnsDaemon, serviceDomain, QTYPE_SRV, ttl);

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
            if(i > 0)
                caps[idx++] = ',';
            memcpy(caps + idx, son->serverCapabilities[i].data,
                   son->serverCapabilities[i].length);
            idx += son->serverCapabilities[i].length;
        }
        caps[idx] = '\0';
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
    mdns_record_t *r = mdnsd_find(md->mdnsDaemon, serviceDomain, QTYPE_TXT);
    if(!r)
        r = mdnsd_shared(md->mdnsDaemon, serviceDomain, QTYPE_TXT, ttl);
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
     * TXT record: [servername]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,... */
    mdns_record_t *r2 = mdnsd_get_published(md->mdnsDaemon, serviceDomainBuf);
    while(r2) {
        const mdns_answer_t *data = mdnsd_record_data(r2);
        mdns_record_t *next = mdnsd_record_next(r2);
        if(data->type == QTYPE_TXT ||
           (data->type == QTYPE_SRV && data->srv.port == port)) {
            mdnsd_done(md->mdnsDaemon, r2);
        }
        r2 = next;
    }

    return UA_STATUSCODE_GOOD;
}

/**************************/
/* Manual Address Records */
/**************************/

static UA_StatusCode
MdnsdDriver_addARecord(UA_MdnsDriver *drv, UA_String hostname,
                       UA_String address, UA_UInt32 ttl) {
    MdnsdDriver *md = (MdnsdDriver*)drv;
    if(!md->mdnsDaemon)
        return UA_STATUSCODE_BADINTERNALERROR;

    char host[UA_MDNS_MAX_RECORD_NAME_LENGTH];
    if(hostname.length >= sizeof(host))
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(host, hostname.data, hostname.length);
    host[hostname.length] = '\0';

    char addr[INET6_ADDRSTRLEN];
    if(address.length >= sizeof(addr))
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(addr, address.data, address.length);
    addr[address.length] = '\0';

    struct in_addr ip;
    int rv = inet_pton(AF_INET, addr, &ip);
    if(rv != 1)
        return UA_STATUSCODE_BADINTERNALERROR;

    mdns_record_t *r = mdnsd_shared(md->mdnsDaemon, host, QTYPE_A, ttl);
    if(!r)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mdnsd_set_ip(md->mdnsDaemon, r, ip);

    flushMulticastMessages(md);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
MdnsdDriver_addAAAARecord(UA_MdnsDriver *drv, UA_String hostname,
                          UA_String address, UA_UInt32 ttl) {
    MdnsdDriver *md = (MdnsdDriver*)drv;
    if(!md->mdnsDaemon)
        return UA_STATUSCODE_BADINTERNALERROR;

    char host[UA_MDNS_MAX_RECORD_NAME_LENGTH];
    if(hostname.length >= sizeof(host))
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(host, hostname.data, hostname.length);
    host[hostname.length] = '\0';

    char addr[INET6_ADDRSTRLEN];
    if(address.length >= sizeof(addr))
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(addr, address.data, address.length);
    addr[address.length] = '\0';

    struct in6_addr ip6;
    int rv = inet_pton(AF_INET6, addr, &ip6);
    if(rv != 1)
        return UA_STATUSCODE_BADINTERNALERROR;

    mdns_record_t *r = mdnsd_shared(md->mdnsDaemon, host, QTYPE_AAAA, ttl);
    if(!r)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mdnsd_set_ipv6(md->mdnsDaemon, r, ip6);

    flushMulticastMessages(md);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
MdnsdDriver_removeAddressRecord(UA_MdnsDriver *drv, UA_String hostname,
                                UA_String address, unsigned short type) {
    MdnsdDriver *md = (MdnsdDriver*)drv;
    if(!md->mdnsDaemon)
        return UA_STATUSCODE_BADINTERNALERROR;

    char host[UA_MDNS_MAX_RECORD_NAME_LENGTH];
    if(hostname.length >= sizeof(host))
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(host, hostname.data, hostname.length);
    host[hostname.length] = '\0';

    char addr[INET6_ADDRSTRLEN];
    if(address.length >= sizeof(addr))
        return UA_STATUSCODE_BADINTERNALERROR;
    memcpy(addr, address.data, address.length);
    addr[address.length] = '\0';

    struct in_addr ip;
    struct in6_addr ip6;
    int rv = (type == QTYPE_A) ? inet_pton(AF_INET, addr, &ip) :
        inet_pton(AF_INET6, addr, &ip6);
    if(rv != 1)
        return UA_STATUSCODE_BADINTERNALERROR;

    mdns_record_t *r = mdnsd_get_published(md->mdnsDaemon, host);
    while(r) {
        const mdns_answer_t *data = mdnsd_record_data(r);
        mdns_record_t *next = mdnsd_record_next(r);
        if(data->type == type) {
            UA_Boolean match = (type == QTYPE_A) ?
                (memcmp(&data->ip, &ip, sizeof(ip)) == 0) :
                (memcmp(&data->ip6, &ip6, sizeof(ip6)) == 0);
            if(match)
                mdnsd_done(md->mdnsDaemon, r);
        }
        r = next;
    }

    flushMulticastMessages(md);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
MdnsdDriver_removeARecord(UA_MdnsDriver *drv, UA_String hostname,
                          UA_String address) {
    return MdnsdDriver_removeAddressRecord(drv, hostname, address, QTYPE_A);
}

static UA_StatusCode
MdnsdDriver_removeAAAARecord(UA_MdnsDriver *drv, UA_String hostname,
                             UA_String address) {
    return MdnsdDriver_removeAddressRecord(drv, hostname, address, QTYPE_AAAA);
}

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
    UA_Server *server = md->mdns.drv.server;
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
        res = md->cm->openConnection(md->cm, &kvm, md->mdns.drv.server, md,
                                     MulticastDiscoveryRecvCallback);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                         "mDNS: Could not create the UDP multicast listen connection");
    }

    /* Open the send connection */
    if((md->announce || md->queryPresence || md->queryDetails) &&
       md->mdnsSendConnection == 0) {
        res = md->cm->openConnection(md->cm, &kvm, md->mdns.drv.server, md,
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

        if(md->mdns.drv.state == UA_LIFECYCLESTATE_STOPPING) {
            if(allClosed)
                md->mdns.drv.state = UA_LIFECYCLESTATE_STOPPED;
        } else if(md->mdns.drv.state == UA_LIFECYCLESTATE_STARTED) {
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

    UA_EventLoop *el = UA_Server_getConfig(md->mdns.drv.server)->eventLoop;
    UA_DateTime now = el->dateTime_nowMonotonic(el);

    ServerOnNetworkRecord *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &md->serverList, listPointers, son_tmp) {
        /* Remove if stale */
        if(son->nextAction > now)
            continue;
        if(isRemoteReceivedSON(son)) {
            removeReceivedSON(md, son);
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
    /* A TTL of zero means "unspecified"; fall back to the driver's default
     * (e.g. configured announce-ttl, or 600s). The notification always
     * carries a ttl key (possibly zero), so the only way to detect
     * "unspecified" is to check for zero explicitly. */
    UA_UInt32 ttl = (_ttl && *_ttl > 0) ? *_ttl : announceTTL(md);

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
            /* Abort if the mDNS-relevant fields are identical. The recordId is
             * bumped server-side (e.g. by resetDiscoveryResetIds) and must
             * not be part of the equality check. Use a shallow copy on the
             * stack so the string pointers of `son` stay valid. */
            UA_ServerOnNetwork sonForCmp = *son;
            sonForCmp.recordId = entry->serverOnNetwork.recordId;
            UA_Order same =
                UA_order(&sonForCmp, &entry->serverOnNetwork,
                         &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
            if(same == UA_ORDER_EQ) {
                /* If this is an entry received from mDNS, this notification is
                 * just the server table echo of our cache update. Keep the
                 * remote origin and do not announce it back out. */
                return;
            }
            UA_ServerOnNetwork tmp;
            res = UA_ServerOnNetwork_copy(son, &tmp);
            if(res != UA_STATUSCODE_GOOD)
                return;
            UA_ServerOnNetwork_clear(&entry->serverOnNetwork);
            entry->serverOnNetwork = tmp;
        }

        /* Locally created and not received over the network */
        entry->origin = SERVER_ON_NETWORK_RECORD_LOCAL_OWNED;
        entry->ttl = ttl;

        /* Announce right away */
        announceRecord(md, entry);

        /* Next time the entry is announced */
        UA_EventLoop *el = UA_Server_getConfig(md->mdns.drv.server)->eventLoop;
        UA_DateTime now = el->dateTime_nowMonotonic(el);
        entry->nextAction = now + ((entry->ttl * 0.9) * UA_DATETIME_SEC);
    }

    /* Remove record */
    if(removed) {
        if(!entry)
            return;

        if(isLocalOwnedSON(entry)) {
            /* Locally owned record: the API removal means we own the goodbye. */
            retractRecord(md, son);
            /* Send the goodbye message immediately so the
             * public-api tests observe the deregister packet. */
            flushMulticastMessages(md);
            removeLocalSON(md, entry);
        } else {
            /* Received record: the server notification was caused by a remote
             * goodbye or TTL expiry. Do not mirror a goodbye back out. */
            removeSONEntry(md, entry, false);
        }
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
    if(md->presenceQueryCallbackId) {
        UA_Server_removeRepeatedCallback(server, md->presenceQueryCallbackId);
        md->presenceQueryCallbackId = 0;
    }

    /* Retract all locally-announced records so that goodbyes (TTL=0) are
     * flushed before the UDP sockets are closed. Only retract entries we
     * created locally; received entries will be cleaned up by the server
     * via the normal deregister path. */
    ServerOnNetworkRecord *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &md->serverList, listPointers, son_tmp) {
        if(isLocalOwnedSON(son))
            retractRecord(md, &son->serverOnNetwork);
    }

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

    const UA_Boolean *queryPresenceParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&drv->params,
                                 UA_QUALIFIEDNAME(0, "query-presence"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    md->queryPresence = queryPresenceParam ? *queryPresenceParam : false;

    const UA_Boolean *queryDetailsParam = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&drv->params,
                                 UA_QUALIFIEDNAME(0, "query-details"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    md->queryDetails = queryDetailsParam ? *queryDetailsParam : false;

    const UA_UInt32 *queryIntervalParam = (const UA_UInt32*)
        UA_KeyValueMap_getScalar(&drv->params,
                                 UA_QUALIFIEDNAME(0, "query-interval"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    md->queryInterval = queryIntervalParam ? *queryIntervalParam : 0;

    if(!md->listen && (md->queryPresence || md->queryDetails)) {
        UA_LOG_WARNING(md->logging, UA_LOGCATEGORY_DISCOVERY,
                       "mDNS: Querying requires listen=true; disabling queries");
        md->queryPresence = false;
        md->queryDetails = false;
        md->queryInterval = 0;
    }

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
            MdnsdDriver_stop(&md->mdns.drv);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        mdnsd_register_receive_callback(md->mdnsDaemon, processRecord, md);
    }

    /* Open the UDP sockets if needed */
    if(md->listen || md->announce || md->queryPresence || md->queryDetails) {
        if(md->mdnsSendConnection == 0)
            createMulticastSocket(md);
        if(md->mdnsSendConnection == 0) {
            UA_LOG_ERROR(md->logging, UA_LOGCATEGORY_DISCOVERY,
                         "mDNS: Could not create UDP multicast socket");
            MdnsdDriver_stop(&md->mdns.drv);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    /* Add a repeated callback to process actions and send out multicast
     * messages */
    UA_Server_addRepeatedCallback(md->mdns.drv.server, MdnsdHouseKeeping,
                                  md, UA_MDNS_POLL_INTERVAL_MS,
                                  &md->sendCallbackId);

    if(md->queryPresence) {
        queryPresence(md);
        if(md->queryInterval > 0) {
            UA_Server_addRepeatedCallback(md->mdns.drv.server,
                                          MdnsdPresenceQueryCallback, md,
                                          md->queryInterval * 1000,
                                          &md->presenceQueryCallbackId);
        }
    }

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

UA_MdnsDriver *
UA_MdnsDriver_Mdnsd(const UA_KeyValueMap params) {
    /* Allocate the memory */
    MdnsdDriver *md = (MdnsdDriver*)
        UA_calloc(1, sizeof(MdnsdDriver));
    if(!md)
        return NULL;

    /* Copy over the parameters */
    UA_StatusCode res =
        UA_KeyValueMap_copy(&params, &md->mdns.drv.params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(md);
        return NULL;
    }

    /* Set the name and function pointers */
    md->mdns.drv.name = UA_STRING("discovery-mdns");
    md->mdns.drv.start = MdnsdDriver_start;
    md->mdns.drv.stop = MdnsdDriver_stop;
    md->mdns.drv.free = MdnsdDriver_free;

    /* Callback for server notifications */
    md->mdns.drv.notificationCallback = MdnsdDriverNotificationCallback;
    md->mdns.drv.notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY;

    /* Manual address records */
    md->mdns.addARecord = MdnsdDriver_addARecord;
    md->mdns.removeARecord = MdnsdDriver_removeARecord;
    md->mdns.addAAAARecord = MdnsdDriver_addAAAARecord;
    md->mdns.removeAAAARecord = MdnsdDriver_removeAAAARecord;

    return &md->mdns;
}

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */
