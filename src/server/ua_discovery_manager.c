/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015-2016, 2019 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_DISCOVERY

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
    dm->selfFqdnMdnsRecord = UA_STRING_NULL;

    LIST_INIT(&dm->serverOnNetwork);
    dm->serverOnNetworkRecordIdCounter = 0;
    dm->serverOnNetworkRecordIdLastReset = UA_DateTime_now();
    memset(dm->serverOnNetworkHash, 0,
           sizeof(struct serverOnNetwork_hash_entry*) * SERVER_ON_NETWORK_HASH_SIZE);

    dm->serverOnNetworkCallback = NULL;
    dm->serverOnNetworkCallbackData = NULL;
#endif /* UA_ENABLE_DISCOVERY_MULTICAST */
}

void
UA_DiscoveryManager_clear(UA_DiscoveryManager *dm, UA_Server *server) {
    registeredServer_list_entry *rs, *rs_tmp;
    LIST_FOREACH_SAFE(rs, &dm->registeredServers, pointers, rs_tmp) {
        LIST_REMOVE(rs, pointers);
        UA_RegisteredServer_clear(&rs->registeredServer);
        UA_free(rs);
    }
    periodicServerRegisterCallback_entry *ps, *ps_tmp;
    LIST_FOREACH_SAFE(ps, &dm->periodicServerRegisterCallbacks, pointers, ps_tmp) {
        LIST_REMOVE(ps, pointers);
        if(ps->callback->discovery_server_url)
            UA_free(ps->callback->discovery_server_url);
        UA_free(ps->callback);
        UA_free(ps);
    }

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    serverOnNetwork_list_entry *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &dm->serverOnNetwork, pointers, son_tmp) {
        LIST_REMOVE(son, pointers);
        UA_ServerOnNetwork_clear(&son->serverOnNetwork);
        if(son->pathTmp)
            UA_free(son->pathTmp);
        UA_free(son);
    }

    UA_String_clear(&dm->selfFqdnMdnsRecord);

    for(size_t i = 0; i < SERVER_ON_NETWORK_HASH_SIZE; i++) {
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
