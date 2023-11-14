/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 */

#ifndef UA_DISCOVERY_MANAGER_H_
#define UA_DISCOVERY_MANAGER_H_

#include "ua_server_internal.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_DISCOVERY

typedef struct registeredServer {
    LIST_ENTRY(registeredServer) pointers;
    UA_RegisteredServer registeredServer;
    UA_DateTime lastSeen;
} registeredServer;

/* Store async register service calls. So we can cancel outstanding requests
 * during shutdown. */
typedef struct {
    UA_DelayedCallback cleanupCallback; /* delayed cleanup */
    UA_Server *server;
    UA_DiscoveryManager *dm;
    UA_Client *client;
    UA_String semaphoreFilePath;
    UA_Boolean unregister;

    UA_Boolean register2;
    UA_Boolean shutdown;
    UA_Boolean connectSuccess;
} asyncRegisterRequest;
#define UA_MAXREGISTERREQUESTS 4

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

#include "mdnsd/libmdnsd/mdnsd.h"
#define UA_MAXMDNSRECVSOCKETS 8

/**
 * TXT record:
 * [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
 *
 * A/AAAA record for all ip addresses:
 * [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
 * [hostname]. A [ip].
 */

typedef struct serverOnNetwork {
    LIST_ENTRY(serverOnNetwork) pointers;
    UA_ServerOnNetwork serverOnNetwork;
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

#endif

struct UA_DiscoveryManager {
    UA_ServerComponent sc;

    UA_UInt64 discoveryCallbackId;

    UA_Server *server; /* backpointer */

    /* Outstanding requests. So they can be cancelled during shutdown. */
    asyncRegisterRequest registerRequests[UA_MAXREGISTERREQUESTS];

    LIST_HEAD(, registeredServer) registeredServers;
    size_t registeredServersSize;
    UA_Server_registerServerCallback registerServerCallback;
    void* registerServerCallbackData;

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    mdns_daemon_t *mdnsDaemon;
    UA_ConnectionManager *cm;
    uintptr_t mdnsSendConnection;
    uintptr_t mdnsRecvConnections[UA_MAXMDNSRECVSOCKETS];
    size_t mdnsRecvConnectionsSize;
    UA_Boolean mdnsMainSrvAdded;

    /* Full Domain Name of server itself. Used to detect if received mDNS
     * message was from itself */
    UA_String selfFqdnMdnsRecord;

    LIST_HEAD(, serverOnNetwork) serverOnNetwork;

    UA_UInt32 serverOnNetworkRecordIdCounter;
    UA_DateTime serverOnNetworkRecordIdLastReset;

    /* hash mapping domain name to serverOnNetwork list entry */
    struct serverOnNetwork_hash_entry* serverOnNetworkHash[SERVER_ON_NETWORK_HASH_SIZE];

    UA_Server_serverOnNetworkCallback serverOnNetworkCallback;
    void *serverOnNetworkCallbackData;

    UA_UInt64 mdnsCallbackId;
# endif /* UA_ENABLE_DISCOVERY_MULTICAST */
};

void
UA_DiscoveryManager_setState(UA_Server *server,
                             UA_DiscoveryManager *dm,
                             UA_LifecycleState state);

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

/* Sends out a new mDNS package for the given server data. This Method is
 * normally called when another server calls the RegisterServer Service on this
 * server. Then this server is responsible to send out a new mDNS package to
 * announce it.
 *
 * Additionally this method also adds the given server to the internal
 * serversOnNetwork list so that a client finds it when calling
 * FindServersOnNetwork. */
void
UA_Discovery_updateMdnsForDiscoveryUrl(UA_DiscoveryManager *dm, const UA_String *serverName,
                                       const UA_MdnsDiscoveryConfiguration *mdnsConfig,
                                       const UA_String *discoveryUrl, UA_Boolean isOnline,
                                       UA_Boolean updateTxt);

void UA_DiscoveryManager_startMulticast(UA_DiscoveryManager *dm);
void UA_DiscoveryManager_stopMulticast(UA_DiscoveryManager *dm);
void UA_DiscoveryManager_sendMulticastMessages(UA_DiscoveryManager *dm);

UA_StatusCode
UA_DiscoveryManager_addEntryToServersOnNetwork(UA_DiscoveryManager *dm,
                                               const char *fqdnMdnsRecord,
                                               const char *serverName,
                                               size_t serverNameLen,
                                               struct serverOnNetwork **addedEntry);

UA_StatusCode
UA_DiscoveryManager_removeEntryFromServersOnNetwork(UA_DiscoveryManager *dm,
                                                    const char *fqdnMdnsRecord,
                                                    const char *serverName,
                                                    size_t serverNameLen);


void mdns_record_received(const struct resource *r, void *data);

void mdns_create_txt(UA_DiscoveryManager *dm, const char *fullServiceDomain,
                     const char *path, const UA_String *capabilites,
                     const size_t capabilitiesSize,
                     void (*conflict)(char *host, int type, void *arg));

void mdns_set_address_record(UA_DiscoveryManager *dm, const char *fullServiceDomain,
                             const char *localDomain);

mdns_record_t *
mdns_find_record(mdns_daemon_t *mdnsDaemon, unsigned short type,
                 const char *host, const char *rdname);

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */

#endif /* UA_ENABLE_DISCOVERY */

_UA_END_DECLS

#endif /* UA_DISCOVERY_MANAGER_H_ */
