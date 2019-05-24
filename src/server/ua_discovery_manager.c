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

#include "ua_server_internal.h"

#ifdef UA_ENABLE_DISCOVERY

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

/* Create multicast 224.0.0.251:5353 socket */
static UA_SOCKET
discovery_createMulticastSocket(void) {
    UA_SOCKET s;
    int flag = 1, ittl = 255;
    struct sockaddr_in in;
    struct ip_mreq mc;
    char ttl = (char)255; // publish to complete net, not only subnet. See:
                          // https://docs.oracle.com/cd/E23824_01/html/821-1602/sockets-137.html

    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;
    in.sin_port = htons(5353);
    in.sin_addr.s_addr = 0;

    if((s = UA_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == UA_INVALID_SOCKET)
        return UA_INVALID_SOCKET;

#ifdef SO_REUSEPORT
    UA_setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char *)&flag, sizeof(flag));
#endif
    UA_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag));
    if(UA_bind(s, (struct sockaddr *)&in, sizeof(in))) {
        UA_close(s);
        return UA_INVALID_SOCKET;
    }

    mc.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
    mc.imr_interface.s_addr = htonl(INADDR_ANY);
    UA_setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mc, sizeof(mc));
    UA_setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));
    UA_setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ittl, sizeof(ittl));

    UA_socket_set_nonblocking(s); //TODO: check return value
    return s;
}

static UA_StatusCode
initMulticastDiscoveryServer(UA_DiscoveryManager *dm, UA_Server* server) {
    server->discoveryManager.mdnsDaemon = mdnsd_new(QCLASS_IN, 1000);
    UA_initialize_architecture_network();

    if((server->discoveryManager.mdnsSocket = discovery_createMulticastSocket()) == UA_INVALID_SOCKET) {
        UA_LOG_SOCKET_ERRNO_WRAP(
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Could not create multicast socket. Error: %s", errno_str));
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    mdnsd_register_receive_callback(server->discoveryManager.mdnsDaemon,
                                    mdns_record_received, server);
    return UA_STATUSCODE_GOOD;
}

static void
destroyMulticastDiscoveryServer(UA_DiscoveryManager *dm) {
    if (!dm->mdnsDaemon)
        return;

    mdnsd_shutdown(dm->mdnsDaemon);
    mdnsd_free(dm->mdnsDaemon);

    if(dm->mdnsSocket != UA_INVALID_SOCKET) {
        UA_close(dm->mdnsSocket);
        dm->mdnsSocket = UA_INVALID_SOCKET;
    }
}

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */

void
UA_DiscoveryManager_init(UA_DiscoveryManager *dm, UA_Server *server) {
    LIST_INIT(&dm->registeredServers);
    dm->registeredServersSize = 0;
    LIST_INIT(&dm->periodicServerRegisterCallbacks);
    dm->registerServerCallback = NULL;
    dm->registerServerCallbackData = NULL;

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    dm->mdnsDaemon = NULL;
    dm->mdnsSocket = UA_INVALID_SOCKET;
    dm->mdnsMainSrvAdded = false;
    if(server->config.discovery.mdnsEnable)
        initMulticastDiscoveryServer(dm, server);

    LIST_INIT(&dm->serverOnNetwork);
    dm->serverOnNetworkSize = 0;
    dm->serverOnNetworkRecordIdCounter = 0;
    dm->serverOnNetworkRecordIdLastReset = UA_DateTime_now();
    memset(dm->serverOnNetworkHash, 0,
           sizeof(struct serverOnNetwork_hash_entry*) * SERVER_ON_NETWORK_HASH_PRIME);

    dm->serverOnNetworkCallback = NULL;
    dm->serverOnNetworkCallbackData = NULL;
#endif /* UA_ENABLE_DISCOVERY_MULTICAST */
}

void
UA_DiscoveryManager_deleteMembers(UA_DiscoveryManager *dm, UA_Server *server) {
    registeredServer_list_entry *rs, *rs_tmp;
    LIST_FOREACH_SAFE(rs, &dm->registeredServers, pointers, rs_tmp) {
        LIST_REMOVE(rs, pointers);
        UA_RegisteredServer_deleteMembers(&rs->registeredServer);
        UA_free(rs);
    }
    periodicServerRegisterCallback_entry *ps, *ps_tmp;
    LIST_FOREACH_SAFE(ps, &dm->periodicServerRegisterCallbacks, pointers, ps_tmp) {
        LIST_REMOVE(ps, pointers);
        UA_free(ps->callback);
        UA_free(ps);
    }

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.discovery.mdnsEnable)
        destroyMulticastDiscoveryServer(dm);

    serverOnNetwork_list_entry *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &dm->serverOnNetwork, pointers, son_tmp) {
        LIST_REMOVE(son, pointers);
        UA_ServerOnNetwork_deleteMembers(&son->serverOnNetwork);
        if(son->pathTmp)
            UA_free(son->pathTmp);
        UA_free(son);
    }

    for(size_t i = 0; i < SERVER_ON_NETWORK_HASH_PRIME; i++) {
        serverOnNetwork_hash_entry* currHash = dm->serverOnNetworkHash[i];
        while(currHash) {
            serverOnNetwork_hash_entry* nextHash = currHash->next;
            UA_free(currHash);
            currHash = nextHash;
        }
    }

# endif /* UA_ENABLE_DISCOVERY_MULTICAST */
}

#endif /* UA_ENABLE_DISCOVERY */
