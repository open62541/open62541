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
    LIST_ENTRY(registeredServer_list_entry) pointers;
    UA_RegisteredServer registeredServer;
    UA_DateTime lastSeen;
} registeredServer_list_entry;

struct PeriodicServerRegisterCallback {
    UA_UInt64 id;
    UA_Double this_interval;
    UA_Double default_interval;
    UA_Boolean registered;
    struct UA_Client* client;
    char* discovery_server_url;
};

typedef struct periodicServerRegisterCallback_entry {
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
    LIST_ENTRY(serverOnNetwork_list_entry) pointers;
    UA_ServerOnNetwork serverOnNetwork;
    UA_DateTime created;
    UA_DateTime lastSeen;
    UA_Boolean txtSet;
    UA_Boolean srvSet;
    char* pathTmp;
} serverOnNetwork_list_entry;

#define SERVER_ON_NETWORK_HASH_SIZE 1000
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

    /* Full Domain Name of server itself. Used to detect if received mDNS
     * message was from itself */
    UA_String selfFqdnMdnsRecord;

    LIST_HEAD(, serverOnNetwork_list_entry) serverOnNetwork;

    UA_UInt32 serverOnNetworkRecordIdCounter;
    UA_DateTime serverOnNetworkRecordIdLastReset;

    /* hash mapping domain name to serverOnNetwork list entry */
    struct serverOnNetwork_hash_entry* serverOnNetworkHash[SERVER_ON_NETWORK_HASH_SIZE];

    UA_Server_serverOnNetworkCallback serverOnNetworkCallback;
    void *serverOnNetworkCallbackData;

    UA_UInt64 mdnsCallbackId;
# endif /* UA_ENABLE_DISCOVERY_MULTICAST */
} UA_DiscoveryManager;

void UA_DiscoveryManager_init(UA_DiscoveryManager *dm);
void UA_DiscoveryManager_clear(UA_DiscoveryManager *dm);

/* Checks if a registration timed out and removes that registration.
 * Should be called periodically in main loop */
void UA_Discovery_cleanupTimedOut(UA_Server *server, UA_DateTime nowMonotonic);

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
UA_Server_updateMdnsForDiscoveryUrl(UA_Server *server, const UA_String *serverName,
                                    const UA_MdnsDiscoveryConfiguration *mdnsConfig,
                                    const UA_String *discoveryUrl, UA_Boolean isOnline,
                                    UA_Boolean updateTxt);

void mdns_record_received(const struct resource *r, void *data);

void mdns_create_txt(UA_Server *server, const char *fullServiceDomain,
                     const char *path, const UA_String *capabilites,
                     const size_t capabilitiesSize,
                     void (*conflict)(char *host, int type, void *arg));

void mdns_set_address_record(UA_Server *server, const char *fullServiceDomain,
                             const char *localDomain);

mdns_record_t *
mdns_find_record(mdns_daemon_t *mdnsDaemon, unsigned short type,
                 const char *host, const char *rdname);

UA_StatusCode
initMulticastDiscoveryServer(UA_DiscoveryManager *dm, UA_Server* server);

void startMulticastDiscoveryServer(UA_Server *server);
void stopMulticastDiscoveryServer(UA_Server *server);

UA_StatusCode
UA_DiscoveryManager_addEntryToServersOnNetwork(UA_Server *server, const char *fqdnMdnsRecord,
                                               const char *serverName, size_t serverNameLen,
                                               struct serverOnNetwork_list_entry **addedEntry);

UA_StatusCode
UA_DiscoveryManager_removeEntryFromServersOnNetwork(UA_Server *server, const char *fqdnMdnsRecord,
                                                    const char *serverName, size_t serverNameLen);

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */

#endif /* UA_ENABLE_DISCOVERY */

_UA_END_DECLS

#endif /* UA_DISCOVERY_MANAGER_H_ */
