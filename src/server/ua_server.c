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
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_subscription.h"
#endif

#ifdef UA_ENABLE_VALGRIND_INTERACTIVE
#include <valgrind/memcheck.h>
#endif

#define STARTCHANNELID 1
#define STARTTOKENID 1

/**********************/
/* Namespace Handling */
/**********************/

/*
 * The NS1 Uri can be changed by the user to some custom string.
 * This method is called to initialize the NS1 Uri if it is not set before to the default Application URI.
 *
 * This is done as soon as the Namespace Array is read or written via node value read / write services,
 * or UA_Server_addNamespace, UA_Server_getNamespaceByName or UA_Server_run_startup is called.
 *
 * Therefore one has to set the custom NS1 URI before one of the previously mentioned steps.
 */
void setupNs1Uri(UA_Server *server) {
    if (!server->namespaces[1].data) {
        UA_String_copy(&server->config.applicationDescription.applicationUri, &server->namespaces[1]);
    }
}

UA_UInt16 addNamespace(UA_Server *server, const UA_String name) {
    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

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
    UA_LOCK(server->serviceMutex);
    UA_UInt16 retVal = addNamespace(server, nameString);
    UA_UNLOCK(server->serviceMutex);
    return retVal;
}

UA_ServerConfig*
UA_Server_getConfig(UA_Server *server)
{
  if(!server)
    return NULL;

  return &server->config;
}

UA_StatusCode
UA_Server_getNamespaceByName(UA_Server *server, const UA_String namespaceUri,
                             size_t* foundIndex) {
    UA_LOCK(server->serviceMutex);

    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

    for(size_t idx = 0; idx < server->namespacesSize; idx++) {
        if(!UA_String_equal(&server->namespaces[idx], &namespaceUri))
            continue;
        (*foundIndex) = idx;
        UA_UNLOCK(server->serviceMutex);
        return UA_STATUSCODE_GOOD;
    }
    UA_UNLOCK(server->serviceMutex);
    return UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    UA_LOCK(server->serviceMutex);
    const UA_Node *parent = UA_NODESTORE_GET(server, &parentNodeId);
    if(!parent) {
        UA_UNLOCK(server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* TODO: We need to do an ugly copy of the references array since users may
     * delete references from within the callback. In single-threaded mode this
     * changes the same node we point at here. In multi-threaded mode, this
     * creates a new copy as nodes are truly immutable.
     * The callback could remove a node via the regular public API.
     * This can remove a member of the nodes-array we iterate over...
     * */
    UA_Node *parentCopy = UA_Node_copy_alloc(parent);
    if(!parentCopy) {
        UA_NODESTORE_RELEASE(server, parent);
        UA_UNLOCK(server->serviceMutex);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = parentCopy->referencesSize; i > 0; --i) {
        UA_NodeReferenceKind *ref = &parentCopy->references[i - 1];
        for(size_t j = 0; j<ref->refTargetsSize; j++) {
            UA_UNLOCK(server->serviceMutex);
            retval = callback(ref->refTargets[j].targetId.nodeId, ref->isInverse,
                              ref->referenceTypeId, handle);
            UA_LOCK(server->serviceMutex);
            if(retval != UA_STATUSCODE_GOOD)
                goto cleanup;
        }
    }

cleanup:
    UA_Node_clear(parentCopy);
    UA_free(parentCopy);

    UA_NODESTORE_RELEASE(server, parent);
    UA_UNLOCK(server->serviceMutex);
    return retval;
}

/********************/
/* Server Lifecycle */
/********************/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
    /* Delete all internal data */
    UA_Server_deleteSecureChannels(server);
    UA_LOCK(server->serviceMutex);
    session_list_entry *current, *temp;
    LIST_FOREACH_SAFE(current, &server->sessions, pointers, temp) {
        UA_Server_removeSession(server, current, UA_DIAGNOSTICEVENT_CLOSE);
    }
    UA_UNLOCK(server->serviceMutex);
    UA_Array_delete(server->namespaces, server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &server->localMonitoredItems, listEntry, mon_tmp) {
        LIST_REMOVE(mon, listEntry);
        UA_LOCK(server->serviceMutex);
        UA_MonitoredItem_delete(server, mon);
        UA_UNLOCK(server->serviceMutex);
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    UA_ConditionList_delete(server);
#endif//UA_ENABLE_ALARMS_CONDITIONS

#endif

#ifdef UA_ENABLE_PUBSUB
    UA_PubSubManager_delete(server, &server->pubSubManager);
#endif

#ifdef UA_ENABLE_DISCOVERY
    UA_DiscoveryManager_deleteMembers(&server->discoveryManager, server);
#endif

#if UA_MULTITHREADING >= 100
    UA_AsyncManager_clear(&server->asyncManager, server);
#endif

    /* Clean up the Admin Session */
    UA_LOCK(server->serviceMutex);
    UA_Session_deleteMembersCleanup(&server->adminSession, server);
    UA_UNLOCK(server->serviceMutex);

    /* Clean up the work queue */
    UA_WorkQueue_cleanup(&server->workQueue);

    /* Delete the timed work */
    UA_Timer_deleteMembers(&server->timer);

    /* Clean up the config */
    UA_ServerConfig_clean(&server->config);

#if UA_MULTITHREADING >= 100
    UA_LOCK_DESTROY(server->networkMutex)
    UA_LOCK_DESTROY(server->serviceMutex)
#endif

    /* Delete the server itself */
    UA_free(server);
}

/* Recurring cleanup. Removing unused and timed-out channels and sessions */
static void
UA_Server_cleanup(UA_Server *server, void *_) {
    UA_LOCK(server->serviceMutex);
    UA_DateTime nowMonotonic = UA_DateTime_nowMonotonic();
    UA_Server_cleanupSessions(server, nowMonotonic);
    UA_Server_cleanupTimedOutSecureChannels(server, nowMonotonic);
#ifdef UA_ENABLE_DISCOVERY
    UA_Discovery_cleanupTimedOut(server, nowMonotonic);
#endif
    UA_UNLOCK(server->serviceMutex);
}

/********************/
/* Server Lifecycle */
/********************/

static UA_Server *
UA_Server_init(UA_Server *server) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    if(!server->config.nodestore.getNode) {
        UA_LOG_FATAL(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "No Nodestore configured in the server");
        goto cleanup;
    }

    /* Init start time to zero, the actual start time will be sampled in
     * UA_Server_run_startup() */
    server->startTime = 0;

    /* Set a seed for non-cyptographic randomness */
#ifndef UA_ENABLE_DETERMINISTIC_RNG
    UA_random_seed((UA_UInt64)UA_DateTime_now());
#endif

#if UA_MULTITHREADING >= 100
    UA_LOCK_INIT(server->networkMutex)
    UA_LOCK_INIT(server->serviceMutex)
#endif

    /* Initialize the handling of repeated callbacks */
    UA_Timer_init(&server->timer);

    UA_WorkQueue_init(&server->workQueue);

    /* Initialize the adminSession */
    UA_Session_init(&server->adminSession);
    server->adminSession.sessionId.identifierType = UA_NODEIDTYPE_GUID;
    server->adminSession.sessionId.identifier.guid.data1 = 1;
    server->adminSession.validTill = UA_INT64_MAX;

    /* Create Namespaces 0 and 1
     * Ns1 will be filled later with the uri from the app description */
    server->namespaces = (UA_String *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    if(!server->namespaces) {
        UA_Server_delete(server);
        return NULL;
    }
    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    server->namespaces[1] = UA_STRING_NULL;
    server->namespacesSize = 2;

    /* Initialize SecureChannel */
    TAILQ_INIT(&server->channels);
    /* TODO: use an ID that is likely to be unique after a restart */
    server->lastChannelId = STARTCHANNELID;
    server->lastTokenId = STARTTOKENID;

    /* Initialize Session Management */
    LIST_INIT(&server->sessions);
    server->sessionCount = 0;

#if UA_MULTITHREADING >= 100
    UA_AsyncManager_init(&server->asyncManager, server);
#endif

    /* Add a regular callback for cleanup and maintenance. With a 10s interval. */
    UA_Server_addRepeatedCallback(server, (UA_ServerCallback)UA_Server_cleanup, NULL,
                                  10000.0, NULL);

    /* Initialize namespace 0*/
    res = UA_Server_initNS0(server);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Build PubSub information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_Server_initPubSubNS0(server);
#endif

    return server;

 cleanup:
    UA_Server_delete(server);
    return NULL;
}

UA_Server *
UA_Server_newWithConfig(const UA_ServerConfig *config) {
    if(!config)
        return NULL;
    UA_Server *server = (UA_Server *)UA_calloc(1, sizeof(UA_Server));
    if(!server)
        return NULL;
    server->config = *config;
    return UA_Server_init(server);
}

/* Returns if the server should be shut down immediately */
static UA_Boolean
setServerShutdown(UA_Server *server) {
    if(server->endTime != 0)
        return false;
    if(server->config.shutdownDelay == 0)
        return true;
    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                   "Shutting down the server with a delay of %i ms", (int)server->config.shutdownDelay);
    server->endTime = UA_DateTime_now() + (UA_DateTime)(server->config.shutdownDelay * UA_DATETIME_MSEC);
    return false;
}

/*******************/
/* Timed Callbacks */
/*******************/

UA_StatusCode
UA_Server_addTimedCallback(UA_Server *server, UA_ServerCallback callback,
                           void *data, UA_DateTime date, UA_UInt64 *callbackId) {
    UA_LOCK(server->serviceMutex);
    UA_StatusCode retval = UA_Timer_addTimedCallback(&server->timer,
                                                     (UA_ApplicationCallback)callback,
                                                      server, data, date, callbackId);
    UA_UNLOCK(server->serviceMutex);
    return retval;
}

UA_StatusCode
addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                              void *data, UA_Double interval_ms,
                              UA_UInt64 *callbackId) {
    return UA_Timer_addRepeatedCallback(&server->timer,
                                        (UA_ApplicationCallback)callback,
                                         server, data, interval_ms, callbackId);
}

UA_StatusCode
UA_Server_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                              void *data, UA_Double interval_ms,
                              UA_UInt64 *callbackId) {
    UA_LOCK(server->serviceMutex);
    UA_StatusCode retval = addRepeatedCallback(server, callback, data, interval_ms, callbackId);
    UA_UNLOCK(server->serviceMutex);
    return retval;
}

UA_StatusCode
changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                         UA_Double interval_ms) {
    return UA_Timer_changeRepeatedCallbackInterval(&server->timer, callbackId,
                                                   interval_ms);
}

UA_StatusCode
UA_Server_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                         UA_Double interval_ms) {
    UA_LOCK(server->serviceMutex);
    UA_StatusCode retval = changeRepeatedCallbackInterval(server, callbackId, interval_ms);
    UA_UNLOCK(server->serviceMutex);
    return retval;
}

void
removeCallback(UA_Server *server, UA_UInt64 callbackId) {
    UA_Timer_removeCallback(&server->timer, callbackId);
}

void
UA_Server_removeCallback(UA_Server *server, UA_UInt64 callbackId) {
    UA_LOCK(server->serviceMutex);
    removeCallback(server, callbackId);
    UA_UNLOCK(server->serviceMutex);
}

UA_StatusCode
UA_Server_updateCertificate(UA_Server *server,
                            const UA_ByteString *oldCertificate,
                            const UA_ByteString *newCertificate,
                            const UA_ByteString *newPrivateKey,
                            UA_Boolean closeSessions,
                            UA_Boolean closeSecureChannels) {

    if(!server || !oldCertificate || !newCertificate || !newPrivateKey)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(closeSessions) {
        session_list_entry *current;
        LIST_FOREACH(current, &server->sessions, pointers) {
            if(UA_ByteString_equal(oldCertificate,
                                    &current->session.header.channel->securityPolicy->localCertificate)) {
                UA_LOCK(server->serviceMutex);
                UA_Server_removeSessionByToken(server, &current->session.header.authenticationToken,
                                               UA_DIAGNOSTICEVENT_CLOSE);
                UA_UNLOCK(server->serviceMutex);
            }
        }

    }

    if(closeSecureChannels) {
        channel_entry *entry;
        TAILQ_FOREACH(entry, &server->channels, pointers) {
            if(UA_ByteString_equal(&entry->channel.securityPolicy->localCertificate, oldCertificate))
                UA_Server_closeSecureChannel(server, &entry->channel, UA_DIAGNOSTICEVENT_CLOSE);
        }
    }

    size_t i = 0;
    while(i < server->config.endpointsSize) {
        UA_EndpointDescription *ed = &server->config.endpoints[i];
        if(UA_ByteString_equal(&ed->serverCertificate, oldCertificate)) {
            UA_String_deleteMembers(&ed->serverCertificate);
            UA_String_copy(newCertificate, &ed->serverCertificate);
            UA_SecurityPolicy *sp = UA_SecurityPolicy_getSecurityPolicyByUri(server, &server->config.endpoints[i].securityPolicyUri);
            if(!sp)
                return UA_STATUSCODE_BADINTERNALERROR;
            sp->updateCertificateAndPrivateKey(sp, *newCertificate, *newPrivateKey);
        }
        i++;
    }

    return UA_STATUSCODE_GOOD;
}

/***************************/
/* Server lookup functions */
/***************************/

UA_SecurityPolicy *
UA_SecurityPolicy_getSecurityPolicyByUri(const UA_Server *server,
                                         const UA_ByteString *securityPolicyUri) {
    for(size_t i = 0; i < server->config.securityPoliciesSize; i++) {
        UA_SecurityPolicy *securityPolicyCandidate = &server->config.securityPolicies[i];
        if(UA_ByteString_equal(securityPolicyUri, &securityPolicyCandidate->policyUri))
            return securityPolicyCandidate;
    }
    return NULL;
}

#ifdef UA_ENABLE_ENCRYPTION
/* The local ApplicationURI has to match the certificates of the
 * SecurityPolicies */
static UA_StatusCode
verifyServerApplicationURI(const UA_Server *server) {
    for(size_t i = 0; i < server->config.securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &server->config.securityPolicies[i];
        UA_StatusCode retval = server->config.certificateVerification.
            verifyApplicationURI(server->config.certificateVerification.context,
                                 &sp->localCertificate,
                                 &server->config.applicationDescription.applicationUri);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "The configured ApplicationURI \"%.*s\"does not match the "
                         "ApplicationURI specified in the certificate for the "
                         "SecurityPolicy %.*s",
                         (int)server->config.applicationDescription.applicationUri.length,
                         server->config.applicationDescription.applicationUri.data,
                         (int)sp->policyUri.length, sp->policyUri.data);
            return retval;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#endif

UA_ServerStatistics UA_Server_getStatistics(UA_Server *server)
{
   return server->serverStats;
}

/********************/
/* Main Server Loop */
/********************/

#define UA_MAXTIMEOUT 50 /* Max timeout in ms between main-loop iterations */

/* Start: Spin up the workers and the network layer and sample the server's
 *        start time.
 * Iterate: Process repeated callbacks and events in the network layer. This
 *          part can be driven from an external main-loop in an event-driven
 *          single-threaded architecture.
 * Stop: Stop workers, finish all callbacks, stop the network layer, clean up */

UA_StatusCode
UA_Server_run_startup(UA_Server *server) {

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    /* Prominently warn user that fuzzing build is enabled. This will tamper with authentication tokens and other important variables
     * E.g. if fuzzing is enabled, and two clients are connected, subscriptions do not work properly,
     * since the tokens will be overridden to allow easier fuzzing. */
    UA_LOG_FATAL(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Server was built with unsafe fuzzing mode. This should only be used for specific fuzzing builds.");
#endif

    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

    /* write ServerArray with same ApplicationURI value as NamespaceArray */
    UA_StatusCode retVal = writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERARRAY,
                                    &server->config.applicationDescription.applicationUri,
                                    1, &UA_TYPES[UA_TYPES_STRING]);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    if(server->state > UA_SERVERLIFECYCLE_FRESH)
        return UA_STATUSCODE_GOOD;

    /* At least one endpoint has to be configured */
    if(server->config.endpointsSize == 0) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "There has to be at least one endpoint.");
    }

    /* Initialized discovery */
#ifdef UA_ENABLE_DISCOVERY
    UA_DiscoveryManager_init(&server->discoveryManager, server);
#endif

    /* Does the ApplicationURI match the local certificates? */
#ifdef UA_ENABLE_ENCRYPTION
    retVal = verifyServerApplicationURI(server);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;
#endif

    /* Sample the start time and set it to the Server object */
    server->startTime = UA_DateTime_now();
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, &server->startTime, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Server_writeValue(server,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME),
                         var);

    /* Start the networklayers */
    UA_StatusCode result = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < server->config.networkLayersSize; ++i) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        nl->statistics = &server->serverStats.ns;
        result |= nl->start(nl, &server->config.customHostname);
    }

    /* Update the application description to match the previously added discovery urls.
     * We can only do this after the network layer is started since it inits the discovery url */
    if(server->config.applicationDescription.discoveryUrlsSize != 0) {
        UA_Array_delete(server->config.applicationDescription.discoveryUrls,
                        server->config.applicationDescription.discoveryUrlsSize,
                        &UA_TYPES[UA_TYPES_STRING]);
        server->config.applicationDescription.discoveryUrlsSize = 0;
    }
    server->config.applicationDescription.discoveryUrls = (UA_String *)
        UA_Array_new(server->config.networkLayersSize, &UA_TYPES[UA_TYPES_STRING]);
    if(!server->config.applicationDescription.discoveryUrls)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    server->config.applicationDescription.discoveryUrlsSize = server->config.networkLayersSize;
    for(size_t i = 0; i < server->config.applicationDescription.discoveryUrlsSize; i++) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        UA_String_copy(&nl->discoveryUrl, &server->config.applicationDescription.discoveryUrls[i]);
    }

    /* Spin up the worker threads */
#if UA_MULTITHREADING >= 200
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Spinning up %" PRIu16 " worker thread(s)", server->config.nThreads);
    UA_WorkQueue_start(&server->workQueue, server->config.nThreads);
#endif

    /* Start the multicast discovery server */
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.discovery.mdnsEnable)
        startMulticastDiscoveryServer(server);
#endif

    server->state = UA_SERVERLIFECYCLE_FRESH;

    return result;
}

static void
serverExecuteRepeatedCallback(UA_Server *server, UA_ApplicationCallback cb,
                        void *callbackApplication, void *data) {
#if UA_MULTITHREADING >= 200
    UA_WorkQueue_enqueue(&server->workQueue, cb, callbackApplication, data);
#else
    cb(callbackApplication, data);
#endif
}

UA_UInt16
UA_Server_run_iterate(UA_Server *server, UA_Boolean waitInternal) {
    /* Process repeated work */
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime nextRepeated = UA_Timer_process(&server->timer, now,
                     (UA_TimerExecutionCallback)serverExecuteRepeatedCallback, server);
    UA_DateTime latest = now + (UA_MAXTIMEOUT * UA_DATETIME_MSEC);
    if(nextRepeated > latest)
        nextRepeated = latest;

    UA_UInt16 timeout = 0;

    /* round always to upper value to avoid timeout to be set to 0
    * if(nextRepeated - now) < (UA_DATETIME_MSEC/2) */
    if(waitInternal)
        timeout = (UA_UInt16)(((nextRepeated - now) + (UA_DATETIME_MSEC - 1)) / UA_DATETIME_MSEC);

    /* Listen on the networklayer */
    for(size_t i = 0; i < server->config.networkLayersSize; ++i) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        nl->listen(nl, server, timeout);
    }

#if defined(UA_ENABLE_PUBSUB_MQTT)
    /* Listen on the pubsublayer, but only if the yield function is set */
    UA_PubSubConnection *connection;
    TAILQ_FOREACH(connection, &server->pubSubManager.connections, listEntry){
        UA_PubSubConnection *ps = connection;
        if(ps && ps->channel->yield){
            ps->channel->yield(ps->channel, timeout);
        }
    }
#endif
#if defined(UA_ENABLE_DISCOVERY_MULTICAST) && (UA_MULTITHREADING < 200)
    if(server->config.discovery.mdnsEnable) {
        // TODO multicastNextRepeat does not consider new input data (requests)
        // on the socket. It will be handled on the next call. if needed, we
        // need to use select with timeout on the multicast socket
        // server->mdnsSocket (see example in mdnsd library) on higher level.
        UA_DateTime multicastNextRepeat = 0;
        UA_StatusCode hasNext =
            iterateMulticastDiscoveryServer(server, &multicastNextRepeat, true);
        if(hasNext == UA_STATUSCODE_GOOD && multicastNextRepeat < nextRepeated)
            nextRepeated = multicastNextRepeat;
    }
#endif

#if UA_MULTITHREADING < 200
    UA_WorkQueue_manuallyProcessDelayed(&server->workQueue);
#endif

    now = UA_DateTime_nowMonotonic();
    timeout = 0;
    if(nextRepeated > now)
        timeout = (UA_UInt16)((nextRepeated - now) / UA_DATETIME_MSEC);
    return timeout;
}

UA_StatusCode
UA_Server_run_shutdown(UA_Server *server) {
    /* Stop the netowrk layer */
    for(size_t i = 0; i < server->config.networkLayersSize; ++i) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        nl->stop(nl, server);
    }

#if UA_MULTITHREADING >= 200
    /* Shut down the workers */
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Shutting down %u worker thread(s)",
                (int unsigned)server->workQueue.workersSize);
    UA_WorkQueue_stop(&server->workQueue);
#endif

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    /* Stop multicast discovery */
    if(server->config.discovery.mdnsEnable)
        stopMulticastDiscoveryServer(server);
#endif

    /* Execute all delayed callbacks */
    UA_WorkQueue_cleanup(&server->workQueue);

    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
testShutdownCondition(UA_Server *server) {
    if(server->endTime == 0)
        return false;
    return (UA_DateTime_now() > server->endTime);
}

UA_StatusCode
UA_Server_run(UA_Server *server, const volatile UA_Boolean *running) {
    UA_StatusCode retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
#ifdef UA_ENABLE_VALGRIND_INTERACTIVE
    size_t loopCount = 0;
#endif
    while(!testShutdownCondition(server)) {
#ifdef UA_ENABLE_VALGRIND_INTERACTIVE
        if(loopCount == 0) {
            VALGRIND_DO_LEAK_CHECK;
        }
        ++loopCount;
        loopCount %= UA_VALGRIND_INTERACTIVE_INTERVAL;
#endif
        UA_Server_run_iterate(server, true);
        if(!*running) {
            if(setServerShutdown(server))
                break;
        }
    }
    return UA_Server_run_shutdown(server);
}

#ifdef UA_ENABLE_HISTORIZING
/* Allow insert of historical data */
UA_Boolean
UA_Server_AccessControl_allowHistoryUpdateUpdateData(UA_Server *server,
                                                     const UA_NodeId *sessionId, void *sessionContext,
                                                     const UA_NodeId *nodeId,
                                                     UA_PerformUpdateType performInsertReplace,
                                                     const UA_DataValue *value) {
    if(server->config.accessControl.allowHistoryUpdateUpdateData &&
            !server->config.accessControl.allowHistoryUpdateUpdateData(server, &server->config.accessControl,
                                                                       sessionId, sessionContext, nodeId,
                                                                       performInsertReplace, value)) {
        return false;
    }
    return true;
}

/* Allow delete of historical data */
UA_Boolean
UA_Server_AccessControl_allowHistoryUpdateDeleteRawModified(UA_Server *server,
                                                            const UA_NodeId *sessionId, void *sessionContext,
                                                            const UA_NodeId *nodeId,
                                                            UA_DateTime startTimestamp,
                                                            UA_DateTime endTimestamp,
                                                            bool isDeleteModified) {
    if(server->config.accessControl.allowHistoryUpdateDeleteRawModified &&
            !server->config.accessControl.allowHistoryUpdateDeleteRawModified(server, &server->config.accessControl,
                                                                              sessionId, sessionContext, nodeId,
                                                                              startTimestamp, endTimestamp,
                                                                              isDeleteModified)) {
        return false;
    }
    return true;

}
#endif /* UA_ENABLE_HISTORIZING */
