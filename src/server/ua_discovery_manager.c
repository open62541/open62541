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

#include "ua_discovery_manager.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_DISCOVERY

static UA_StatusCode
UA_DiscoveryManager_free(UA_Server *server,
                         struct UA_ServerComponent *sc) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)sc;

    if(sc->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Cannot delete the DiscoveryManager because "
                     "it is not stopped");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

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

    UA_free(dm);
    return UA_STATUSCODE_GOOD;
}

/* Cleanup server registration: If the semaphore file path is set, then it just
 * checks the existence of the file. When it is deleted, the registration is
 * removed. If there is no semaphore file, then the registration will be removed
 * if it is older than 60 minutes. */
static void
UA_DiscoveryManager_cleanupTimedOut(UA_Server *server,
                                    void *data) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)data;

    /* TimedOut gives the last DateTime at which we must have seen the
     * registered server. Otherwise it is timed out. */
    UA_DateTime timedOut = UA_DateTime_nowMonotonic();
    if(server->config.discoveryCleanupTimeout)
        timedOut -= server->config.discoveryCleanupTimeout * UA_DATETIME_SEC;

    registeredServer_list_entry *current, *temp;
    LIST_FOREACH_SAFE(current, &dm->registeredServers, pointers, temp) {
        UA_Boolean semaphoreDeleted = false;

#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
        if(current->registeredServer.semaphoreFilePath.length) {
            size_t fpSize = current->registeredServer.semaphoreFilePath.length+1;
            char* filePath = (char *)UA_malloc(fpSize);
            if(filePath) {
                memcpy(filePath, current->registeredServer.semaphoreFilePath.data,
                       current->registeredServer.semaphoreFilePath.length );
                filePath[current->registeredServer.semaphoreFilePath.length] = '\0';
                semaphoreDeleted = UA_fileExists(filePath) == false;
                UA_free(filePath);
            } else {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                             "Cannot check registration semaphore. Out of memory");
            }
        }
#endif

        if(semaphoreDeleted ||
           (server->config.discoveryCleanupTimeout &&
            current->lastSeen < timedOut)) {
            if(semaphoreDeleted) {
                UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "Registration of server with URI %.*s is removed because "
                            "the semaphore file '%.*s' was deleted",
                            (int)current->registeredServer.serverUri.length,
                            current->registeredServer.serverUri.data,
                            (int)current->registeredServer.semaphoreFilePath.length,
                            current->registeredServer.semaphoreFilePath.data);
            } else {
                // cppcheck-suppress unreadVariable
                UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "Registration of server with URI %.*s has timed out "
                            "and is removed",
                            (int)current->registeredServer.serverUri.length,
                            current->registeredServer.serverUri.data);
            }
            LIST_REMOVE(current, pointers);
            UA_RegisteredServer_clear(&current->registeredServer);
            UA_free(current);
            dm->registeredServersSize--;
        }
    }
}

static UA_StatusCode
UA_DiscoveryManager_start(UA_Server *server,
                          struct UA_ServerComponent *sc) {
    if(sc->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)sc;
    UA_StatusCode res = addRepeatedCallback(server, UA_DiscoveryManager_cleanupTimedOut,
                                            dm, 1000.0, &dm->discoveryCallbackId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    dm->logging = &server->config.logger;
    dm->serverConfig = &server->config;

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.mdnsEnabled)
        startMulticastDiscoveryServer(server);
#endif

    sc->state = UA_LIFECYCLESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
UA_DiscoveryManager_stop(UA_Server *server,
                         struct UA_ServerComponent *sc) {
    if(sc->state != UA_LIFECYCLESTATE_STARTED)
        return;

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)sc;
    removeCallback(server, dm->discoveryCallbackId);

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.mdnsEnabled)
        stopMulticastDiscoveryServer(server);
#endif

    sc->state = UA_LIFECYCLESTATE_STOPPED;
}

UA_ServerComponent *
UA_DiscoveryManager_new(UA_Server *server) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        UA_calloc(1, sizeof(UA_DiscoveryManager));
    if(!dm)
        return NULL;

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

    dm->sc.name = UA_STRING("discovery");
    dm->sc.start = UA_DiscoveryManager_start;
    dm->sc.stop = UA_DiscoveryManager_stop;
    dm->sc.free = UA_DiscoveryManager_free;
    return &dm->sc;
}

#endif /* UA_ENABLE_DISCOVERY */
