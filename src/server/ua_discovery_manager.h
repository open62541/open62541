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

#include <open62541/server.h>

#include "open62541_queue.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_DISCOVERY

typedef struct registeredServer_list_entry {
#ifdef UA_ENABLE_MULTITHREADING
    UA_DelayedCallback delayedCleanup;
#endif
    LIST_ENTRY(registeredServer_list_entry) pointers;
    UA_RegisteredServer registeredServer;
    UA_DateTime lastSeen;
} registeredServer_list_entry;

typedef struct periodicServerRegisterCallback_entry {
#ifdef UA_ENABLE_MULTITHREADING
    UA_DelayedCallback delayedCleanup;
#endif
    LIST_ENTRY(periodicServerRegisterCallback_entry) pointers;
    struct PeriodicServerRegisterCallback *callback;
} periodicServerRegisterCallback_entry;

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

#include "mdnsd/libmdnsd/mdnsd.h"

/**
 * TXT record:
 * [servername]-[hostname]._opcua-tcp._tcp.local. TXT path=/ caps=NA,DA,...
 *
 * A/AAAA record for all ip addresses:
 * [servername]-[hostname]._opcua-tcp._tcp.local. A [ip].
 * [hostname]. A [ip].
 */

typedef struct serverOnNetwork_list_entry {
#ifdef UA_ENABLE_MULTITHREADING
    UA_DelayedCallback delayedCleanup;
#endif
    LIST_ENTRY(serverOnNetwork_list_entry) pointers;
    UA_ServerOnNetwork serverOnNetwork;
    UA_DateTime created;
    UA_DateTime lastSeen;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char* pathTmp;
} serverOnNetwork_list_entry;

#define SERVER_ON_NETWORK_HASH_PRIME 1009
typedef struct serverOnNetwork_hash_entry {
    serverOnNetwork_list_entry* entry;
    struct serverOnNetwork_hash_entry* next;
} serverOnNetwork_hash_entry;

#endif

typedef struct {
    LIST_HEAD(, periodicServerRegisterCallback_entry) periodicServerRegisterCallbacks;
    LIST_HEAD(, registeredServer_list_entry) registeredServers;
    size_t registeredServersSize;
    UA_Server_registerServerCallback registerServerCallback;
    void* registerServerCallbackData;

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    mdns_daemon_t *mdnsDaemon;
    UA_SOCKET mdnsSocket;
    UA_Boolean mdnsMainSrvAdded;

    LIST_HEAD(, serverOnNetwork_list_entry) serverOnNetwork;
    size_t serverOnNetworkSize;

    UA_UInt32 serverOnNetworkRecordIdCounter;
    UA_DateTime serverOnNetworkRecordIdLastReset;

    /* hash mapping domain name to serverOnNetwork list entry */
    struct serverOnNetwork_hash_entry* serverOnNetworkHash[SERVER_ON_NETWORK_HASH_PRIME];

    UA_Server_serverOnNetworkCallback serverOnNetworkCallback;
    void* serverOnNetworkCallbackData;

#  ifdef UA_ENABLE_MULTITHREADING
    pthread_t mdnsThread;
    UA_Boolean mdnsRunning;
#  endif
# endif /* UA_ENABLE_DISCOVERY_MULTICAST */
} UA_DiscoveryManager;

void UA_DiscoveryManager_init(UA_DiscoveryManager *dm, UA_Server *server);
void UA_DiscoveryManager_deleteMembers(UA_DiscoveryManager *dm, UA_Server *server);

/* Checks if a registration timed out and removes that registration.
 * Should be called periodically in main loop */
void UA_Discovery_cleanupTimedOut(UA_Server *server, UA_DateTime nowMonotonic);

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

void
UA_Server_updateMdnsForDiscoveryUrl(UA_Server *server, const UA_String *serverName,
                                    const UA_MdnsDiscoveryConfiguration *mdnsConfig,
                                    const UA_String *discoveryUrl,
                                    UA_Boolean isOnline, UA_Boolean updateTxt);

void mdns_record_received(const struct resource *r, void *data);

void mdns_create_txt(UA_Server *server, const char *fullServiceDomain,
                     const char *path, const UA_String *capabilites,
                     const size_t capabilitiesSize,
                     void (*conflict)(char *host, int type, void *arg));

void mdns_set_address_record(UA_Server *server,
                             const char *fullServiceDomain,
                             const char *localDomain);

mdns_record_t *
mdns_find_record(mdns_daemon_t *mdnsDaemon, unsigned short type,
                 const char *host, const char *rdname);

void startMulticastDiscoveryServer(UA_Server *server);

void stopMulticastDiscoveryServer(UA_Server *server);

UA_StatusCode
iterateMulticastDiscoveryServer(UA_Server* server, UA_DateTime *nextRepeat,
                                UA_Boolean processIn);

typedef enum {
    UA_DISCOVERY_TCP,     /* OPC UA TCP mapping */
    UA_DISCOVERY_TLS     /* OPC UA HTTPS mapping */
} UA_DiscoveryProtocol;

/* Send a multicast probe to find any other OPC UA server on the network through mDNS. */
UA_StatusCode
UA_Discovery_multicastQuery(UA_Server* server);

UA_StatusCode
UA_Discovery_addRecord(UA_Server *server, const UA_String *servername,
                       const UA_String *hostname, UA_UInt16 port,
                       const UA_String *path, const UA_DiscoveryProtocol protocol,
                       UA_Boolean createTxt, const UA_String* capabilites,
                       const size_t capabilitiesSize);
UA_StatusCode
UA_Discovery_removeRecord(UA_Server *server, const UA_String *servername,
                          const UA_String *hostname, UA_UInt16 port,
                          UA_Boolean removeTxt);

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */

#endif /* UA_ENABLE_DISCOVERY */

_UA_END_DECLS

#endif /* UA_DISCOVERY_MANAGER_H_ */
