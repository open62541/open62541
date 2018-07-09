
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015 (c) LEvertz
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) Julian Grothoff
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2016 (c) Lorenz Haas
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2018 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Martin Lang)
 */

#include "ua_types.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_subscription.h"
#endif

/**********************/
/* Namespace Handling */
/**********************/

UA_UInt16 addNamespace(UA_Server *server, const UA_String name) {
    /* Check if the namespace already exists in the server's namespace array */
    for(UA_UInt16 i = 0; i < server->namespacesSize; ++i) {
        if(UA_String_equal(&name, &server->namespaces[i]))
            return i;
    }

    /* Make the array bigger */
    UA_String *newNS = (UA_String*)UA_realloc(server->namespaces,
                                              sizeof(UA_String) * (server->namespacesSize + 1));
    if(!newNS)
        return 0;
    server->namespaces = newNS;

    /* Copy the namespace string */
    UA_StatusCode retval = UA_String_copy(&name, &server->namespaces[server->namespacesSize]);
    if(retval != UA_STATUSCODE_GOOD)
        return 0;

    /* Announce the change (otherwise, the array appears unchanged) */
    ++server->namespacesSize;
    return (UA_UInt16)(server->namespacesSize - 1);
}

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* name) {
    /* Override const attribute to get string (dirty hack) */
    UA_String nameString;
    nameString.length = strlen(name);
    nameString.data = (UA_Byte*)(uintptr_t)name;
    return addNamespace(server, nameString);
}

UA_StatusCode
UA_Server_getNamespaceByName(UA_Server *server, const UA_String namespaceUri,
                             size_t* foundIndex) {
  for(size_t idx = 0; idx < server->namespacesSize; idx++)
  {
    if(UA_String_equal(&server->namespaces[idx], &namespaceUri) == true)
    {
      (*foundIndex) = idx;
      return UA_STATUSCODE_GOOD;
    }
  }

  return UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    const UA_Node *parent =
        server->config.nodestore.getNode(server->config.nodestore.context,
                                         &parentNodeId);
    if(!parent)
        return UA_STATUSCODE_BADNODEIDINVALID;

    /* TODO: We need to do an ugly copy of the references array since users may
     * delete references from within the callback. In single-threaded mode this
     * changes the same node we point at here. In multi-threaded mode, this
     * creates a new copy as nodes are truly immutable.
     * The callback could remove a node via the regular public API.
     * This can remove a member of the nodes-array we iterate over...
     * */
    UA_Node *parentCopy = UA_Node_copy_alloc(parent);
    if(!parentCopy) {
        server->config.nodestore.releaseNode(server->config.nodestore.context, parent);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = parentCopy->referencesSize; i > 0; --i) {
        UA_NodeReferenceKind *ref = &parentCopy->references[i - 1];
        for(size_t j = 0; j<ref->targetIdsSize; j++)
            retval |= callback(ref->targetIds[j].nodeId, ref->isInverse,
                               ref->referenceTypeId, handle);
    }
    UA_Node_deleteMembers(parentCopy);
    UA_free(parentCopy);

    server->config.nodestore.releaseNode(server->config.nodestore.context, parent);
    return retval;
}

/********************/
/* Server Lifecycle */
/********************/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
    /* Delete all internal data */
    UA_SecureChannelManager_deleteMembers(&server->secureChannelManager);
    UA_SessionManager_deleteMembers(&server->sessionManager);
    UA_Array_delete(server->namespaces, server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &server->localMonitoredItems, listEntry, mon_tmp) {
        LIST_REMOVE(mon, listEntry);
        UA_MonitoredItem_delete(server, mon);
    }
#endif

#ifdef UA_ENABLE_PUBSUB
    UA_PubSubManager_delete(server, &server->pubSubManager);
#endif

#ifdef UA_ENABLE_DISCOVERY
    registeredServer_list_entry *rs, *rs_tmp;
    LIST_FOREACH_SAFE(rs, &server->registeredServers, pointers, rs_tmp) {
        LIST_REMOVE(rs, pointers);
        UA_RegisteredServer_deleteMembers(&rs->registeredServer);
        UA_free(rs);
    }
    periodicServerRegisterCallback_entry *ps, *ps_tmp;
    LIST_FOREACH_SAFE(ps, &server->periodicServerRegisterCallbacks, pointers, ps_tmp) {
        LIST_REMOVE(ps, pointers);
        UA_free(ps->callback);
        UA_free(ps);
    }

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER)
        destroyMulticastDiscoveryServer(server);

    serverOnNetwork_list_entry *son, *son_tmp;
    LIST_FOREACH_SAFE(son, &server->serverOnNetwork, pointers, son_tmp) {
        LIST_REMOVE(son, pointers);
        UA_ServerOnNetwork_deleteMembers(&son->serverOnNetwork);
        if(son->pathTmp)
            UA_free(son->pathTmp);
        UA_free(son);
    }

    for(size_t i = 0; i < SERVER_ON_NETWORK_HASH_PRIME; i++) {
        serverOnNetwork_hash_entry* currHash = server->serverOnNetworkHash[i];
        while(currHash) {
            serverOnNetwork_hash_entry* nextHash = currHash->next;
            UA_free(currHash);
            currHash = nextHash;
        }
    }
# endif

#endif

#ifdef UA_ENABLE_MULTITHREADING
    /* Process new delayed callbacks from the cleanup */
    UA_Server_cleanupDispatchQueue(server);
    pthread_mutex_destroy(&server->dispatchQueue_accessMutex);
    pthread_cond_destroy(&server->dispatchQueue_condition);
    pthread_mutex_destroy(&server->dispatchQueue_conditionMutex);
#else
    /* Process new delayed callbacks from the cleanup */
    UA_Server_cleanupDelayedCallbacks(server);
#endif

    /* Delete the timed work */
    UA_Timer_deleteMembers(&server->timer);

    /* Delete the server itself */
    UA_free(server);
}

/* Recurring cleanup. Removing unused and timed-out channels and sessions */
static void
UA_Server_cleanup(UA_Server *server, void *_) {
    UA_DateTime nowMonotonic = UA_DateTime_nowMonotonic();
    UA_SessionManager_cleanupTimedOut(&server->sessionManager, nowMonotonic);
    UA_SecureChannelManager_cleanupTimedOut(&server->secureChannelManager, nowMonotonic);
#ifdef UA_ENABLE_DISCOVERY
    UA_Discovery_cleanupTimedOut(server, nowMonotonic);
#endif
}

/********************/
/* Server Lifecycle */
/********************/

UA_Server *
UA_Server_new(const UA_ServerConfig *config) {
    /* A config is required */
    if(!config)
        return NULL;

    /* At least one endpoint has to be configured */
    if(config->endpointsSize == 0) {
        UA_LOG_FATAL(config->logger, UA_LOGCATEGORY_SERVER,
                     "There has to be at least one endpoint.");
        return NULL;
    }

    /* Allocate the server */
    UA_Server *server = (UA_Server *)UA_calloc(1, sizeof(UA_Server));
    if(!server)
        return NULL;

    /* Set the config */
    server->config = *config;

    /* Init start time to zero, the actual start time will be sampled in
     * UA_Server_run_startup() */
    server->startTime = 0;

    /* Set a seed for non-cyptographic randomness */
#ifndef UA_ENABLE_DETERMINISTIC_RNG
    UA_random_seed((UA_UInt64)UA_DateTime_now());
#endif

    /* Initialize the handling of repeated callbacks */
    UA_Timer_init(&server->timer);

    /* Initialized the linked list for delayed callbacks */
#ifndef UA_ENABLE_MULTITHREADING
    SLIST_INIT(&server->delayedCallbacks);
#endif

    /* Initialized the dispatch queue for worker threads */
#ifdef UA_ENABLE_MULTITHREADING
    SIMPLEQ_INIT(&server->dispatchQueue);
#endif

    /* Create Namespaces 0 and 1 */
    server->namespaces = (UA_String *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    UA_String_copy(&server->config.applicationDescription.applicationUri, &server->namespaces[1]);
    server->namespacesSize = 2;

    /* Initialized SecureChannel and Session managers */
    UA_SecureChannelManager_init(&server->secureChannelManager, server);
    UA_SessionManager_init(&server->sessionManager, server);

    /* Add a regular callback for cleanup and maintenance */
    UA_Server_addRepeatedCallback(server, (UA_ServerCallback)UA_Server_cleanup, NULL,
                                  10000, NULL);

    /* Initialized discovery database */
#ifdef UA_ENABLE_DISCOVERY
    LIST_INIT(&server->registeredServers);
    server->registeredServersSize = 0;
    LIST_INIT(&server->periodicServerRegisterCallbacks);
    server->registerServerCallback = NULL;
    server->registerServerCallbackData = NULL;
#endif

    /* Initialize multicast discovery */
#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)
    server->mdnsDaemon = NULL;
    server->mdnsSocket = UA_INVALID_SOCKET;
    server->mdnsMainSrvAdded = UA_FALSE;
    if(server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER)
        initMulticastDiscoveryServer(server);

    LIST_INIT(&server->serverOnNetwork);
    server->serverOnNetworkSize = 0;
    server->serverOnNetworkRecordIdCounter = 0;
    server->serverOnNetworkRecordIdLastReset = UA_DateTime_now();
    memset(server->serverOnNetworkHash, 0,
           sizeof(struct serverOnNetwork_hash_entry*) * SERVER_ON_NETWORK_HASH_PRIME);

    server->serverOnNetworkCallback = NULL;
    server->serverOnNetworkCallbackData = NULL;
#endif

    /* Initialize namespace 0*/
    UA_StatusCode retVal = UA_Server_initNS0(server);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(config->logger, UA_LOGCATEGORY_SERVER,
                     "Namespace 0 could not be bootstrapped with error %s. "
                     "Shutting down the server.",
                     UA_StatusCode_name(retVal));
        UA_Server_delete(server);
        return NULL;
    }
    /* Build PubSub information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_Server_initPubSubNS0(server);
#endif

    return server;
}

/*****************/
/* Repeated Jobs */
/*****************/

UA_StatusCode
UA_Server_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                              void *data, UA_UInt32 interval,
                              UA_UInt64 *callbackId) {
    return UA_Timer_addRepeatedCallback(&server->timer, (UA_TimerCallback)callback,
                                        data, interval, callbackId);
}

UA_StatusCode
UA_Server_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                         UA_UInt32 interval) {
    return UA_Timer_changeRepeatedCallbackInterval(&server->timer, callbackId, interval);
}

UA_StatusCode
UA_Server_removeRepeatedCallback(UA_Server *server, UA_UInt64 callbackId) {
    return UA_Timer_removeRepeatedCallback(&server->timer, callbackId);
}
