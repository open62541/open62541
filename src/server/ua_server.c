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
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_subscription.h"
#endif

#ifdef UA_ENABLE_NODESET_INJECTOR
#include "open62541/nodesetinjector.h"
#endif

/**********************/
/* Namespace Handling */
/**********************/

/* The NS1 Uri can be changed by the user to some custom string. This method is
 * called to initialize the NS1 Uri if it is not set before to the default
 * Application URI.
 *
 * This is done as soon as the Namespace Array is read or written via node value
 * read / write services, or UA_Server_addNamespace, or UA_Server_getNamespaceByIndex
 * UA_Server_getNamespaceByName or UA_Server_run_startup is called.
 *
 * Therefore one has to set the custom NS1 URI before one of the previously
 * mentioned steps. */

void
setupNs1Uri(UA_Server *server) {
    if(!server->namespaces[1].data) {
        UA_String_copy(&server->config.applicationDescription.applicationUri,
                       &server->namespaces[1]);
    }
}

UA_UInt16 addNamespace(UA_Server *server, const UA_String name) {
    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

    /* Check if the namespace already exists in the server's namespace array */
    for(size_t i = 0; i < server->namespacesSize; ++i) {
        if(UA_String_equal(&name, &server->namespaces[i]))
            return (UA_UInt16) i;
    }

    /* Make the array bigger */
    UA_String *newNS = (UA_String*)UA_realloc(server->namespaces,
                                              sizeof(UA_String) * (server->namespacesSize + 1));
    UA_CHECK_MEM(newNS, return 0);

    server->namespaces = newNS;

    /* Copy the namespace string */
    UA_StatusCode retval = UA_String_copy(&name, &server->namespaces[server->namespacesSize]);
    UA_CHECK_STATUS(retval, return 0);

    /* Announce the change (otherwise, the array appears unchanged) */
    ++server->namespacesSize;
    return (UA_UInt16)(server->namespacesSize - 1);
}

UA_UInt16 UA_Server_addNamespace(UA_Server *server, const char* name) {
    /* Override const attribute to get string (dirty hack) */
    UA_String nameString;
    nameString.length = strlen(name);
    nameString.data = (UA_Byte*)(uintptr_t)name;
    lockServer(server);
    UA_UInt16 retVal = addNamespace(server, nameString);
    unlockServer(server);
    return retVal;
}

UA_ServerConfig*
UA_Server_getConfig(UA_Server *server) {
    UA_CHECK_MEM(server, return NULL);
    return &server->config;
}

UA_StatusCode
getNamespaceByName(UA_Server *server, const UA_String namespaceUri,
                   size_t *foundIndex) {
    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    for(size_t idx = 0; idx < server->namespacesSize; idx++) {
        if(UA_String_equal(&server->namespaces[idx], &namespaceUri)) {
            (*foundIndex) = idx;
            res = UA_STATUSCODE_GOOD;
            break;
        }
    }
    return res;
}

UA_StatusCode
getNamespaceByIndex(UA_Server *server, const size_t namespaceIndex,
                   UA_String *foundUri) {
    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    if(namespaceIndex >= server->namespacesSize)
        return res;
    res = UA_String_copy(&server->namespaces[namespaceIndex], foundUri);
    return res;
}

UA_StatusCode
UA_Server_getNamespaceByName(UA_Server *server, const UA_String namespaceUri,
                             size_t *foundIndex) {
    lockServer(server);
    UA_StatusCode res = getNamespaceByName(server, namespaceUri, foundIndex);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getNamespaceByIndex(UA_Server *server, const size_t namespaceIndex,
                              UA_String *foundUri) {
    lockServer(server);
    UA_StatusCode res = getNamespaceByIndex(server, namespaceIndex, foundUri);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = parentNodeId;
    bd.browseDirection = UA_BROWSEDIRECTION_BOTH;
    bd.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID | UA_BROWSERESULTMASK_ISFORWARD;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    UA_StatusCode res = br.statusCode;
    UA_CHECK_STATUS(res, goto cleanup);

    for(size_t i = 0; i < br.referencesSize; i++) {
        if(!UA_ExpandedNodeId_isLocal(&br.references[i].nodeId))
            continue;
        res = callback(br.references[i].nodeId.nodeId, !br.references[i].isForward,
                       br.references[i].referenceTypeId, handle);
        UA_CHECK_STATUS(res, goto cleanup);
    }
cleanup:
    UA_BrowseResult_clear(&br);
    return res;
}

/*********************/
/* Server Components */
/*********************/

enum ZIP_CMP
cmpServerComponent(const UA_UInt64 *a, const UA_UInt64 *b) {
    if(*a == *b)
        return ZIP_CMP_EQ;
    return (*a < *b) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
}

void
addServerComponent(UA_Server *server, UA_ServerComponent *sc,
                   UA_UInt64 *identifier) {
    if(!sc)
        return;

    sc->identifier = ++server->serverComponentIds;
    ZIP_INSERT(UA_ServerComponentTree, &server->serverComponents, sc);

    /* Start the component if the server is started */
    if(server->state == UA_LIFECYCLESTATE_STARTED && sc->start)
        sc->start(server, sc);

    if(identifier)
        *identifier = sc->identifier;
}

static void *
findServerComponent(void *context, UA_ServerComponent *sc) {
    UA_String *name = (UA_String*)context;
    return (UA_String_equal(&sc->name, name)) ? sc : NULL;
}

UA_ServerComponent *
getServerComponentByName(UA_Server *server, UA_String name) {
    return (UA_ServerComponent*)
        ZIP_ITER(UA_ServerComponentTree, &server->serverComponents,
                 findServerComponent, &name);
}

static void *
removeServerComponent(void *application, UA_ServerComponent *sc) {
    UA_assert(sc->state == UA_LIFECYCLESTATE_STOPPED);
    sc->free((UA_Server*)application, sc);
    return NULL;
}

static void *
startServerComponent(void *application, UA_ServerComponent *sc) {
    sc->start((UA_Server*)application, sc);
    return NULL;
}

static void *
stopServerComponent(void *application, UA_ServerComponent *sc) {
    sc->stop((UA_Server*)application, sc);
    return NULL;
}

/* ZIP_ITER returns NULL only if all components are stopped */
static void *
checkServerComponent(void *application, UA_ServerComponent *sc) {
    return (sc->state == UA_LIFECYCLESTATE_STOPPED) ? NULL : (void*)0x01;
}

/********************/
/* Server Lifecycle */
/********************/

/* The server needs to be stopped before it can be deleted */
UA_StatusCode
UA_Server_delete(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(server->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "The server must be fully stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    lockServer(server);

    session_list_entry *current, *temp;
    LIST_FOREACH_SAFE(current, &server->sessions, pointers, temp) {
        UA_Server_removeSession(server, current, UA_SHUTDOWNREASON_CLOSE);
    }
    UA_Array_delete(server->namespaces, server->namespacesSize, &UA_TYPES[UA_TYPES_STRING]);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &server->localMonitoredItems, listEntry, mon_tmp) {
        LIST_REMOVE(mon, listEntry);
        UA_MonitoredItem_delete(server, mon);
    }

    /* Remove subscriptions without a session */
    UA_Subscription *sub, *sub_tmp;
    LIST_FOREACH_SAFE(sub, &server->subscriptions, serverListEntry, sub_tmp) {
        UA_Subscription_delete(server, sub);
    }
    UA_assert(server->monitoredItemsSize == 0);
    UA_assert(server->subscriptionsSize == 0);

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    UA_ConditionList_delete(server);
#endif

#endif

#ifdef UA_ENABLE_PUBSUB
    UA_PubSubManager_delete(server, &server->pubSubManager);
#endif

#if UA_MULTITHREADING >= 100
    UA_AsyncManager_clear(&server->asyncManager, server);
#endif

    /* Clean up the Admin Session */
    UA_Session_clear(&server->adminSession, server);

    /* Remove all remaining server components (must be all stopped) */
    ZIP_ITER(UA_ServerComponentTree, &server->serverComponents,
             removeServerComponent, server);

    unlockServer(server); /* The timer has its own mutex */

    /* Clean up the config */
    UA_ServerConfig_clean(&server->config);

#if UA_MULTITHREADING >= 100
    UA_LOCK_DESTROY(&server->serviceMutex);
#endif

    /* Delete the server itself and return */
    UA_free(server);
    return UA_STATUSCODE_GOOD;
}

/* Regular house-keeping tasks. Removing unused and timed-out channels and
 * sessions. */
static void
serverHouseKeeping(UA_Server *server, void *_) {
    lockServer(server);
    UA_DateTime nowMonotonic = UA_DateTime_nowMonotonic();
    UA_Server_cleanupSessions(server, nowMonotonic);
    unlockServer(server);
}

/********************/
/* Server Lifecycle */
/********************/

static
UA_INLINE
UA_Boolean UA_Server_NodestoreIsConfigured(UA_Server *server) {
    return server->config.nodestore.getNode != NULL;
}

static UA_Server *
UA_Server_init(UA_Server *server) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_CHECK_FATAL(UA_Server_NodestoreIsConfigured(server), goto cleanup,
                   server->config.logging, UA_LOGCATEGORY_SERVER,
                   "No Nodestore configured in the server");

    /* Init start time to zero, the actual start time will be sampled in
     * UA_Server_run_startup() */
    server->startTime = 0;

    /* Set a seed for non-cyptographic randomness */
#ifndef UA_ENABLE_DETERMINISTIC_RNG
    UA_random_seed((UA_UInt64)UA_DateTime_now());
#endif

    UA_LOCK_INIT(&server->serviceMutex);
    lockServer(server);

    /* Initialize the adminSession */
    UA_Session_init(&server->adminSession);
    server->adminSession.sessionId.identifierType = UA_NODEIDTYPE_GUID;
    server->adminSession.sessionId.identifier.guid.data1 = 1;
    server->adminSession.validTill = UA_INT64_MAX;
    server->adminSession.sessionName = UA_STRING_ALLOC("Administrator");

    /* Create Namespaces 0 and 1
     * Ns1 will be filled later with the uri from the app description */
    server->namespaces = (UA_String *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    UA_CHECK_MEM(server->namespaces, goto cleanup);

    server->namespaces[0] = UA_STRING_ALLOC("http://opcfoundation.org/UA/");
    server->namespaces[1] = UA_STRING_NULL;
    server->namespacesSize = 2;

    /* Initialize Session Management */
    LIST_INIT(&server->sessions);
    server->sessionCount = 0;

#if UA_MULTITHREADING >= 100
    UA_AsyncManager_init(&server->asyncManager, server);
#endif

    /* Initialize the binay protocol support */
    addServerComponent(server, UA_BinaryProtocolManager_new(server), NULL);

    /* Initialized discovery */
#ifdef UA_ENABLE_DISCOVERY
    addServerComponent(server, UA_DiscoveryManager_new(server), NULL);
#endif

    /* Initialize namespace 0*/
    res = initNS0(server);
    UA_CHECK_STATUS(res, goto cleanup);

#ifdef UA_ENABLE_NODESET_INJECTOR
    res = UA_Server_injectNodesets(server);
    UA_CHECK_STATUS(res, goto cleanup);
#endif

#ifdef UA_ENABLE_PUBSUB
    /* Initialized PubSubManager */
    UA_PubSubManager_init(server, &server->pubSubManager);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /* Build PubSub information model */
    initPubSubNS0(server);
#endif

#ifdef UA_ENABLE_PUBSUB_MONITORING
    /* setup default PubSub monitoring callbacks */
    res = UA_PubSubManager_setDefaultMonitoringCallbacks(&server->config.pubSubConfig.monitoringInterface);
    UA_CHECK_STATUS(res, goto cleanup);
#endif /* UA_ENABLE_PUBSUB_MONITORING */
#endif /* UA_ENABLE_PUBSUB */

    unlockServer(server);
    return server;

 cleanup:
    unlockServer(server);
    UA_Server_delete(server);
    return NULL;
}

UA_Server *
UA_Server_newWithConfig(UA_ServerConfig *config) {
    UA_CHECK_MEM(config, return NULL);

    UA_CHECK_LOG(config->eventLoop != NULL, return NULL, ERROR,
                 config->logging, UA_LOGCATEGORY_SERVER, "No EventLoop configured");

    UA_Server *server = (UA_Server *)UA_calloc(1, sizeof(UA_Server));
    UA_CHECK_MEM(server, UA_ServerConfig_clean(config); return NULL);

    server->config = *config;

    /* If not defined, set logging to what the server has */
    if(!server->config.secureChannelPKI.logging)
        server->config.secureChannelPKI.logging = server->config.logging;
    if(!server->config.sessionPKI.logging)
        server->config.sessionPKI.logging = server->config.logging;

    /* Reset the old config */
    memset(config, 0, sizeof(UA_ServerConfig));
    return UA_Server_init(server);
}

/* Returns if the server should be shut down immediately */
static UA_Boolean
setServerShutdown(UA_Server *server) {
    if(server->endTime != 0)
        return false;
    if(server->config.shutdownDelay == 0)
        return true;
    UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
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
    lockServer(server);
    UA_StatusCode retval = server->config.eventLoop->
        addTimedCallback(server->config.eventLoop, (UA_Callback)callback,
                         server, data, date, callbackId);
    unlockServer(server);
    return retval;
}

UA_StatusCode
addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                    void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    return server->config.eventLoop->
        addCyclicCallback(server->config.eventLoop, (UA_Callback) callback,
                          server, data, interval_ms, NULL,
                          UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME, callbackId);
}

UA_StatusCode
UA_Server_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                              void *data, UA_Double interval_ms,
                              UA_UInt64 *callbackId) {
    lockServer(server);
    UA_StatusCode res = addRepeatedCallback(server, callback, data, interval_ms, callbackId);
    unlockServer(server);
    return res;
}

UA_StatusCode
changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                               UA_Double interval_ms) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    return server->config.eventLoop->
        modifyCyclicCallback(server->config.eventLoop, callbackId, interval_ms,
                             NULL, UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME);
}

UA_StatusCode
UA_Server_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                         UA_Double interval_ms) {
    lockServer(server);
    UA_StatusCode retval =
        changeRepeatedCallbackInterval(server, callbackId, interval_ms);
    unlockServer(server);
    return retval;
}

void
removeCallback(UA_Server *server, UA_UInt64 callbackId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_EventLoop *el = server->config.eventLoop;
    if(el) {
        el->removeCyclicCallback(el, callbackId);
    }
}

void
UA_Server_removeCallback(UA_Server *server, UA_UInt64 callbackId) {
    lockServer(server);
    removeCallback(server, callbackId);
    unlockServer(server);
}

static void
notifySecureChannelsStopped(UA_Server *server, struct UA_ServerComponent *sc,
                            UA_LifecycleState state) {
    if(sc->state == UA_LIFECYCLESTATE_STOPPED &&
       server->state == UA_LIFECYCLESTATE_STARTED) {
        sc->notifyState = NULL; /* remove the callback */
        sc->start(server, sc);
    }
}

UA_StatusCode
UA_Server_updateCertificate(UA_Server *server,
                            const UA_ByteString *oldCertificate,
                            const UA_ByteString *newCertificate,
                            const UA_ByteString *newPrivateKey,
                            UA_Boolean closeSessions,
                            UA_Boolean closeSecureChannels) {
    UA_CHECK(server && oldCertificate && newCertificate && newPrivateKey,
             return UA_STATUSCODE_BADINTERNALERROR);

    lockServer(server);

    if(closeSessions) {
        session_list_entry *current;
        LIST_FOREACH(current, &server->sessions, pointers) {
            UA_SessionHeader *header = &current->session.header;
            if(UA_ByteString_equal(oldCertificate, &header->channel->securityPolicy->localCertificate))
                UA_Server_removeSessionByToken(server, &header->authenticationToken,
                                               UA_SHUTDOWNREASON_CLOSE);
        }
    }

    /* Gracefully close all SecureChannels. And restart the
     * BinaryProtocolManager once it has fully stopped. */
    if(closeSecureChannels) {
        UA_ServerComponent *binaryProtocolManager =
            getServerComponentByName(server, UA_STRING("binary"));
        if(binaryProtocolManager) {
            binaryProtocolManager->notifyState = notifySecureChannelsStopped;
            binaryProtocolManager->stop(server, binaryProtocolManager);
        }
    }

    size_t i = 0;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    while(i < server->config.endpointsSize) {
        UA_EndpointDescription *ed = &server->config.endpoints[i];
        if(UA_ByteString_equal(&ed->serverCertificate, oldCertificate)) {
            UA_String_clear(&ed->serverCertificate);
            UA_String_copy(newCertificate, &ed->serverCertificate);
            UA_SecurityPolicy *sp = getSecurityPolicyByUri(server,
                            &server->config.endpoints[i].securityPolicyUri);
            if(!sp) {
                res = UA_STATUSCODE_BADINTERNALERROR;
                break;
            }
            sp->updateCertificateAndPrivateKey(sp, *newCertificate, *newPrivateKey);
        }
        i++;
    }

    unlockServer(server);

    return res;
}

/***************************/
/* Server lookup functions */
/***************************/

UA_SecurityPolicy *
getSecurityPolicyByUri(const UA_Server *server, const UA_ByteString *securityPolicyUri) {
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
    const UA_String securityPolicyNoneUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    for(size_t i = 0; i < server->config.securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &server->config.securityPolicies[i];
        if(UA_String_equal(&sp->policyUri, &securityPolicyNoneUri) && (sp->localCertificate.length == 0))
            continue;
        UA_StatusCode retval = server->config.secureChannelPKI.
            verifyApplicationURI(&server->config.secureChannelPKI,
                                 &sp->localCertificate,
                                 &server->config.applicationDescription.applicationUri);

        UA_CHECK_STATUS_ERROR(retval, return retval, server->config.logging, UA_LOGCATEGORY_SERVER,
                       "The configured ApplicationURI \"%.*s\"does not match the "
                       "ApplicationURI specified in the certificate for the "
                       "SecurityPolicy %.*s",
                       (int)server->config.applicationDescription.applicationUri.length,
                       server->config.applicationDescription.applicationUri.data,
                       (int)sp->policyUri.length, sp->policyUri.data);
    }
    return UA_STATUSCODE_GOOD;
}
#endif

UA_ServerStatistics
UA_Server_getStatistics(UA_Server *server) {
    UA_ServerStatistics stat;
    stat.scs = server->secureChannelStatistics;
    UA_ServerDiagnosticsSummaryDataType *sds = &server->serverDiagnosticsSummary;
    stat.ss.currentSessionCount = server->activeSessionCount;
    stat.ss.cumulatedSessionCount = sds->cumulatedSessionCount;
    stat.ss.securityRejectedSessionCount = sds->securityRejectedSessionCount;
    stat.ss.rejectedSessionCount = sds->rejectedSessionCount;
    stat.ss.sessionTimeoutCount = sds->sessionTimeoutCount;
    stat.ss.sessionAbortCount = sds->sessionAbortCount;
    return stat;
}

/********************/
/* Main Server Loop */
/********************/

#define UA_MAXTIMEOUT 200 /* Max timeout in ms between main-loop iterations */

void
setServerLifecycleState(UA_Server *server, UA_LifecycleState state) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if(server->state == state)
        return;
    server->state = state;
    if(server->config.notifyLifecycleState)
        server->config.notifyLifecycleState(server, server->state);
}

UA_LifecycleState
UA_Server_getLifecycleState(UA_Server *server) {
    return server->state;
}

/* Start: Spin up the workers and the network layer and sample the server's
 *        start time.
 * Iterate: Process repeated callbacks and events in the network layer. This
 *          part can be driven from an external main-loop in an event-driven
 *          single-threaded architecture.
 * Stop: Stop workers, finish all callbacks, stop the network layer, clean up */

UA_StatusCode
UA_Server_run_startup(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_ServerConfig *config = &server->config;

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    /* Prominently warn user that fuzzing build is enabled. This will tamper
     * with authentication tokens and other important variables E.g. if fuzzing
     * is enabled, and two clients are connected, subscriptions do not work
     * properly, since the tokens will be overridden to allow easier fuzzing. */
    UA_LOG_FATAL(server->config.logging, UA_LOGCATEGORY_SERVER,
                 "Server was built with unsafe fuzzing mode. "
                 "This should only be used for specific fuzzing builds.");
#endif

    if(server->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                       "The server has already been started");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check if UserIdentityTokens are defined */
    bool hasUserIdentityTokens = false;
    for(size_t i = 0; i < config->endpointsSize; i++) {
        if(config->endpoints[i].userIdentityTokensSize > 0) {
            hasUserIdentityTokens = true;
            break;
        }
    }
    if(config->accessControl.userTokenPoliciesSize == 0 && hasUserIdentityTokens == false) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "The server has no userIdentificationPolicies defined.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Start the EventLoop if not already started */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_EventLoop *el = config->eventLoop;
    UA_CHECK_MEM_ERROR(el, return UA_STATUSCODE_BADINTERNALERROR,
                       config->logging, UA_LOGCATEGORY_SERVER,
                       "An EventLoop must be configured");

    if(el->state != UA_EVENTLOOPSTATE_STARTED) {
        retVal = el->start(el);
        UA_CHECK_STATUS(retVal, return retVal); /* Errors are logged internally */
    }

    /* Take the server lock */
    lockServer(server);

    /* Does the ApplicationURI match the local certificates? */
#ifdef UA_ENABLE_ENCRYPTION
    retVal = verifyServerApplicationURI(server);
    UA_CHECK_STATUS(retVal, unlockServer(server); return retVal);
#endif

    /* Are there enough SecureChannels possible for the max number of sessions? */
    if(config->maxSecureChannels != 0 &&
       (config->maxSessions == 0 || config->maxSessions > config->maxSecureChannels)) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                       "Maximum SecureChannels count not enough for the "
                       "maximum Sessions count");
    }

    /* Add a regular callback for housekeeping tasks. With a 1s interval. */
    retVal = addRepeatedCallback(server, serverHouseKeeping,
                                 NULL, 1000.0, &server->houseKeepingCallbackId);
    UA_CHECK_STATUS_ERROR(retVal, unlockServer(server); return retVal,
                          config->logging, UA_LOGCATEGORY_SERVER,
                          "Could not create the server housekeeping task");

    /* Ensure that the uri for ns1 is set up from the app description */
    UA_String_clear(&server->namespaces[1]);
    setupNs1Uri(server);

    /* At least one endpoint has to be configured */
    if(config->endpointsSize == 0) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                       "There has to be at least one endpoint.");
    }

    /* Update Endpoint description */
    for(size_t i = 0; i < config->endpointsSize; ++i) {
        UA_ApplicationDescription_clear(&config->endpoints[i].server);
        UA_ApplicationDescription_copy(&config->applicationDescription,
                                       &config->endpoints[i].server);
    }

    /* Write ServerArray with same ApplicationUri value as NamespaceArray */
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, &config->applicationDescription.applicationUri,
                        1, &UA_TYPES[UA_TYPES_STRING]);
    UA_NodeId serverArray = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERARRAY);
    writeValueAttribute(server, serverArray, &var);

    /* Sample the start time and set it to the Server object */
    server->startTime = UA_DateTime_now();
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, &server->startTime, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_NodeId startTime =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME);
    writeValueAttribute(server, startTime, &var);

    /* Start all ServerComponents */
    ZIP_ITER(UA_ServerComponentTree, &server->serverComponents,
             startServerComponent, server);

    /* Check that the binary protocol support component have been started */
    UA_ServerComponent *binaryProtocolManager =
        getServerComponentByName(server, UA_STRING("binary"));
    if(binaryProtocolManager->state != UA_LIFECYCLESTATE_STARTED) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                       "The binary protocol support component could not been started.");
        /* Stop all server components that have already been started */
        ZIP_ITER(UA_ServerComponentTree, &server->serverComponents,
                 stopServerComponent, server);
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set the server to STARTED. From here on, only use
     * UA_Server_run_shutdown(server) to stop the server. */
    setServerLifecycleState(server, UA_LIFECYCLESTATE_STARTED);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_UInt16
UA_Server_run_iterate(UA_Server *server, UA_Boolean waitInternal) {
    /* Make sure an EventLoop is configured */
    UA_EventLoop *el = server->config.eventLoop;
    if(!el)
        return 0;

    /* Process timed and network events in the EventLoop */
    UA_UInt32 timeout = (waitInternal) ? UA_MAXTIMEOUT : 0;
    el->run(el, timeout);

    /* Return the time until the next scheduled callback */
    UA_DateTime now = el->dateTime_nowMonotonic(el);
    UA_DateTime nextTimeout = (el->nextCyclicTime(el) - now) / UA_DATETIME_MSEC;
    if(nextTimeout < 0)
        nextTimeout = 0;
    if(nextTimeout > UA_UINT16_MAX)
        nextTimeout = UA_UINT16_MAX;
    return (UA_UInt16)nextTimeout;
}

static UA_Boolean
testShutdownCondition(UA_Server *server) {
    /* Was there a wait time until the shutdown configured? */
    if(server->endTime == 0)
        return false;
    return (UA_DateTime_now() > server->endTime);
}

static UA_Boolean
testStoppedCondition(UA_Server *server) {
    /* Check if there are remaining server components that did not fully stop */
    if(ZIP_ITER(UA_ServerComponentTree, &server->serverComponents,
                checkServerComponent, server) != NULL)
        return false;
    return true;
}

UA_StatusCode
UA_Server_run_shutdown(UA_Server *server) {
    if(server == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    if(server->state != UA_LIFECYCLESTATE_STARTED) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "The server is not started, cannot be shut down");
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set to stopping and notify the application */
    setServerLifecycleState(server, UA_LIFECYCLESTATE_STOPPING);

    /* Stop the regular housekeeping tasks */
    if(server->houseKeepingCallbackId != 0) {
        removeCallback(server, server->houseKeepingCallbackId);
        server->houseKeepingCallbackId = 0;
    }

    /* Stop PubSub */
#ifdef UA_ENABLE_PUBSUB
    UA_PubSubManager_shutdown(server, &server->pubSubManager);
#endif

    /* Stop all ServerComponents */
    ZIP_ITER(UA_ServerComponentTree, &server->serverComponents,
             stopServerComponent, server);

    /* Are we already stopped? */
    if(testStoppedCondition(server)) {
        setServerLifecycleState(server, UA_LIFECYCLESTATE_STOPPED);
    }

    /* Only stop the EventLoop if it is coupled to the server lifecycle  */
    if(server->config.externalEventLoop) {
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }

    /* Iterate the EventLoop until the server is stopped */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_EventLoop *el = server->config.eventLoop;
    while(!testStoppedCondition(server) &&
          res == UA_STATUSCODE_GOOD) {
        res = el->run(el, 100);
    }

    /* Stop the EventLoop. Iterate until stopped. */
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED &&
          el->state != UA_EVENTLOOPSTATE_FRESH &&
          res == UA_STATUSCODE_GOOD) {
        res = el->run(el, 100);
    }

    /* Set server lifecycle state to stopped if not already the case */
    setServerLifecycleState(server, UA_LIFECYCLESTATE_STOPPED);

    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_run(UA_Server *server, const volatile UA_Boolean *running) {
    UA_StatusCode retval = UA_Server_run_startup(server);
    UA_CHECK_STATUS(retval, return retval);

    while(!testShutdownCondition(server)) {
        UA_Server_run_iterate(server, true);
        if(!*running) {
            if(setServerShutdown(server))
                break;
        }
    }
    return UA_Server_run_shutdown(server);
}

void lockServer(UA_Server *server) {
    if(UA_LIKELY(server->config.eventLoop && server->config.eventLoop->lock))
        server->config.eventLoop->lock(server->config.eventLoop);
    UA_LOCK(&server->serviceMutex);
}

void unlockServer(UA_Server *server) {
    if(UA_LIKELY(server->config.eventLoop && server->config.eventLoop->unlock))
        server->config.eventLoop->unlock(server->config.eventLoop);
    UA_UNLOCK(&server->serviceMutex);
}
