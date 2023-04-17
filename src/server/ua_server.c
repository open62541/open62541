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

#ifdef UA_ENABLE_VALGRIND_INTERACTIVE
#include <valgrind/memcheck.h>
#endif

#define STARTCHANNELID 1
#define STARTTOKENID 1

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
    UA_LOCK(&server->serviceMutex);
    UA_UInt16 retVal = addNamespace(server, nameString);
    UA_UNLOCK(&server->serviceMutex);
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
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = getNamespaceByName(server, namespaceUri, foundIndex);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_getNamespaceByIndex(UA_Server *server, const size_t namespaceIndex,
                              UA_String *foundUri) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = getNamespaceByIndex(server, namespaceIndex, foundUri);
    UA_UNLOCK(&server->serviceMutex);
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
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "The server must be fully stopped before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOCK(&server->serviceMutex);

    UA_Server_deleteSecureChannels(server);
    session_list_entry *current, *temp;
    LIST_FOREACH_SAFE(current, &server->sessions, pointers, temp) {
        UA_Server_removeSession(server, current, UA_DIAGNOSTICEVENT_CLOSE);
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

    UA_UNLOCK(&server->serviceMutex); /* The timer has its own mutex */

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
    UA_LOCK(&server->serviceMutex);
    UA_DateTime nowMonotonic = UA_DateTime_nowMonotonic();
    UA_Server_cleanupSessions(server, nowMonotonic);
    UA_Server_cleanupTimedOutSecureChannels(server, nowMonotonic);
    UA_UNLOCK(&server->serviceMutex);
}

/* Some subsystems require regula polling of the network. This is a holdover
 * from before the EventLoop model.
 *
 * TODO: Refactor to use the EventLoop instead of polling. Get rid of this
 * entirely. */
static void
serverPolling(UA_Server *server, void *_) {
    /* Listen on the pubsublayer, but only if the yield function is set. */
#if defined(UA_ENABLE_PUBSUB_MQTT)
    UA_PubSubConnection *connection;
    TAILQ_FOREACH(connection, &server->pubSubManager.connections, listEntry){
        UA_PubSubConnection *ps = connection;
        if(ps && ps->channel && ps->channel->yield){
            ps->channel->yield(ps->channel, 0);
        }
    }
#endif
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
                   &server->config.logger, UA_LOGCATEGORY_SERVER,
                   "No Nodestore configured in the server");

    /* Init start time to zero, the actual start time will be sampled in
     * UA_Server_run_startup() */
    server->startTime = 0;

    /* Set a seed for non-cyptographic randomness */
#ifndef UA_ENABLE_DETERMINISTIC_RNG
    UA_random_seed((UA_UInt64)UA_DateTime_now());
#endif

    UA_LOCK_INIT(&server->serviceMutex);
    UA_LOCK(&server->serviceMutex);

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

    /* Initialized discovery */
#ifdef UA_ENABLE_DISCOVERY
    addServerComponent(server, UA_DiscoveryManager_new(server), NULL);
#endif

    /* Initialize namespace 0*/
    res = initNS0(server);
    UA_CHECK_STATUS(res, goto cleanup);

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

    UA_UNLOCK(&server->serviceMutex);
    return server;

 cleanup:
    UA_UNLOCK(&server->serviceMutex);
    UA_Server_delete(server);
    return NULL;
}

UA_Server *
UA_Server_newWithConfig(UA_ServerConfig *config) {
    UA_CHECK_MEM(config, return NULL);

    UA_CHECK_LOG(config->eventLoop != NULL, return NULL, ERROR,
                 &config->logger, UA_LOGCATEGORY_SERVER, "No EventLoop configured");

    UA_Server *server = (UA_Server *)UA_calloc(1, sizeof(UA_Server));
    UA_CHECK_MEM(server, UA_ServerConfig_clean(config); return NULL);

    server->config = *config;

    /* The config might have been "moved" into the server struct. Ensure that
     * the logger pointer is correct. */
    for(size_t i = 0; i < server->config.securityPoliciesSize; i++)
        if(server->config.securityPolicies[i].logger == &config->logger)
            server->config.securityPolicies[i].logger = &server->config.logger;

    if(server->config.eventLoop->logger == &config->logger)
        server->config.eventLoop->logger = &server->config.logger;

    if((server->config.logging == NULL) ||
       (server->config.logging == &config->logger)) {
        /* re-set the logger pointer */
        server->config.logging = &server->config.logger;
    }
    if(server->config.certificateVerification.logging == &config->logging)
        server->config.certificateVerification.logging = &server->config.logging;

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
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = server->config.eventLoop->
        addTimedCallback(server->config.eventLoop, (UA_Callback)callback,
                         server, data, date, callbackId);
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

UA_StatusCode
addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                    void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    return server->config.eventLoop->
        addCyclicCallback(server->config.eventLoop, (UA_Callback) callback,
                          server, data, interval_ms, NULL,
                          UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME, callbackId);
}

UA_StatusCode
UA_Server_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                              void *data, UA_Double interval_ms,
                              UA_UInt64 *callbackId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = addRepeatedCallback(server, callback, data, interval_ms, callbackId);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                               UA_Double interval_ms) {
    return server->config.eventLoop->
        modifyCyclicCallback(server->config.eventLoop, callbackId, interval_ms,
                             NULL, UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME);
}

UA_StatusCode
UA_Server_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                         UA_Double interval_ms) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval =
        changeRepeatedCallbackInterval(server, callbackId, interval_ms);
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

void
removeCallback(UA_Server *server, UA_UInt64 callbackId) {
    UA_EventLoop *el = server->config.eventLoop;
    if(el) {
        el->removeCyclicCallback(el, callbackId);
    }
}

void
UA_Server_removeCallback(UA_Server *server, UA_UInt64 callbackId) {
    UA_LOCK(&server->serviceMutex);
    removeCallback(server, callbackId);
    UA_UNLOCK(&server->serviceMutex);
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

    if(closeSessions) {
        session_list_entry *current;
        LIST_FOREACH(current, &server->sessions, pointers) {
            if(UA_ByteString_equal(oldCertificate,
                                    &current->session.header.channel->securityPolicy->localCertificate)) {
                UA_LOCK(&server->serviceMutex);
                UA_Server_removeSessionByToken(server, &current->session.header.authenticationToken,
                                               UA_DIAGNOSTICEVENT_CLOSE);
                UA_UNLOCK(&server->serviceMutex);
            }
        }

    }

    if(closeSecureChannels) {
        channel_entry *entry;
        TAILQ_FOREACH(entry, &server->channels, pointers) {
            if(UA_ByteString_equal(&entry->channel.securityPolicy->localCertificate,
                                   oldCertificate))
                shutdownServerSecureChannel(server, &entry->channel, UA_DIAGNOSTICEVENT_CLOSE);
        }
    }

    size_t i = 0;
    while(i < server->config.endpointsSize) {
        UA_EndpointDescription *ed = &server->config.endpoints[i];
        if(UA_ByteString_equal(&ed->serverCertificate, oldCertificate)) {
            UA_String_clear(&ed->serverCertificate);
            UA_String_copy(newCertificate, &ed->serverCertificate);
            UA_SecurityPolicy *sp = getSecurityPolicyByUri(server,
                            &server->config.endpoints[i].securityPolicyUri);
            UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);
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
        UA_StatusCode retval = server->config.certificateVerification.
            verifyApplicationURI(&server->config.certificateVerification,
                                 &sp->localCertificate,
                                 &server->config.applicationDescription.applicationUri);

        UA_CHECK_STATUS_ERROR(retval, return retval, &server->config.logger, UA_LOGCATEGORY_SERVER,
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

static UA_StatusCode
createServerConnection(UA_Server *server, const UA_String *serverUrl) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_ServerConfig *config = &server->config;

    /* Extract the protocol, hostname and port from the url */
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_UInt16 port = 4840; /* default */
    UA_StatusCode res = UA_parseEndpointUrl(serverUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_String tcpString = UA_STRING("tcp");
    for(UA_EventSource *es = config->eventLoop->eventSources;
        es != NULL; es = es->next) {
        /* Is this a usable connection manager? */
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(!UA_String_equal(&tcpString, &cm->protocol))
            continue;

        /* Set up the parameters */
        UA_KeyValuePair params[3];
        size_t paramsSize = 2;

        params[0].key = UA_QUALIFIEDNAME(0, "port");
        UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);

        UA_Boolean listen = true;
        params[1].key = UA_QUALIFIEDNAME(0, "listen");
        UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);

        if(hostname.length > 0) {
            /* The hostname is non-empty */
            params[2].key = UA_QUALIFIEDNAME(0, "address");
            UA_Variant_setArray(&params[2].value, &hostname, 1, &UA_TYPES[UA_TYPES_STRING]);
            paramsSize = 3;
        }

        UA_KeyValueMap paramsMap;
        paramsMap.map = params;
        paramsMap.mapSize = paramsSize;

        /* Open the server connection */
        res = cm->openConnection(cm, &paramsMap, server, NULL, UA_Server_networkCallback);
        if(res == UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_StatusCode
attemptReverseConnect(UA_Server *server, reverse_connect_context *context) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_ServerConfig *config = &server->config;
    UA_EventLoop *el = config->eventLoop;

    /* Find a TCP ConnectionManager */
    UA_String tcpString = UA_STRING_STATIC("tcp");
    for(UA_EventSource *es = el->eventSources; es != NULL; es = es->next) {
        /* Is this a usable connection manager? */
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;

        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(!UA_String_equal(&tcpString, &cm->protocol))
            continue;

        if(es->state != UA_EVENTSOURCESTATE_STARTED)
            continue;

        /* Set up the parameters */
        UA_KeyValuePair params[2];
        params[0].key = UA_QUALIFIEDNAME(0, "address");
        UA_Variant_setScalar(&params[0].value, &context->hostname,
                             &UA_TYPES[UA_TYPES_STRING]);
        params[1].key = UA_QUALIFIEDNAME(0, "port");
        UA_Variant_setScalar(&params[1].value, &context->port,
                             &UA_TYPES[UA_TYPES_UINT16]);
        UA_KeyValueMap kvm = {2, params};

        /* Open the connection */
        UA_StatusCode res = cm->openConnection(cm, &kvm, server, context,
                                               UA_Server_reverseConnectCallback);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Failed to create connection for reverse connect: %s\n",
                           UA_StatusCode_name(res));
        }
        return res;
    }

    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                   "No ConnectionManager found for reverse connect");
    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_StatusCode
UA_Server_addReverseConnect(UA_Server *server, UA_String url,
                            UA_Server_ReverseConnectStateCallback stateCallback,
                            void *callbackContext, UA_UInt64 *handle) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Parse the reverse connect URL */
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_SERVER,
                       "OPC UA URL is invalid: %.*s",
                       (int)url.length, url.data);
        return res;
    }

    /* Set up the reverse connection */
    reverse_connect_context *newContext =
        (reverse_connect_context *)calloc(1, sizeof(reverse_connect_context));
    if(!newContext)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_String_copy(&hostname, &newContext->hostname);
    newContext->port = port;
    newContext->handle = ++server->lastReverseConnectHandle;
    newContext->stateCallback = stateCallback;
    newContext->callbackContext = callbackContext;

    UA_LOCK(&server->serviceMutex);

    /* Register the retry callback */
    if(LIST_EMPTY(&server->reverseConnects))
        setReverseConnectRetryCallback(server, true);

    /* Register the new reverse connection */
    LIST_INSERT_HEAD(&server->reverseConnects, newContext, next);

    if(handle)
        *handle = newContext->handle;

    /* Attempt to connect right away */
    res = attemptReverseConnect(server, newContext);

    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_removeReverseConnect(UA_Server *server, UA_UInt64 handle) {
    UA_StatusCode result = UA_STATUSCODE_BADNOTFOUND;

    UA_LOCK(&server->serviceMutex);

    reverse_connect_context *rev, *temp;
    LIST_FOREACH_SAFE(rev, &server->reverseConnects, next, temp) {
        if(rev->handle != handle)
            continue;

        LIST_REMOVE(rev, next);

        /* Connected -> disconnect, otherwise free immediately */
        if(rev->currentConnection.connectionId) {
            UA_ConnectionManager *cm = rev->currentConnection.connectionManager;
            rev->destruction = true;
            cm->closeConnection(cm, rev->currentConnection.connectionId);
        } else {
            setReverseConnectState(server, rev, UA_SECURECHANNELSTATE_CLOSED);
            UA_String_clear(&rev->hostname);
            free(rev);
        }
        result = UA_STATUSCODE_GOOD;
        break;
    }

    if(LIST_EMPTY(&server->reverseConnects))
        setReverseConnectRetryCallback(server, false);

    UA_UNLOCK(&server->serviceMutex);

    return result;
}

/********************/
/* Main Server Loop */
/********************/

#define UA_MAXTIMEOUT 200 /* Max timeout in ms between main-loop iterations */

void
setServerLifecycleState(UA_Server *server, UA_LifecycleState state) {
    if(server->state == state)
        return;
    server->state = state;
    if(server->config.notifyLifecycleState) {
        UA_UNLOCK(&server->serviceMutex);
        server->config.notifyLifecycleState(server, server->state);
        UA_LOCK(&server->serviceMutex);
    }
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
    UA_LOG_FATAL(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "Server was built with unsafe fuzzing mode. "
                 "This should only be used for specific fuzzing builds.");
#endif

    if(server->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_SERVER,
                       "The server has already been started");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Start the EventLoop if not already started */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_EventLoop *el = config->eventLoop;
    UA_CHECK_MEM_ERROR(el, return UA_STATUSCODE_BADINTERNALERROR,
                       &config->logger, UA_LOGCATEGORY_SERVER,
                       "An EventLoop must be configured");

    if(el->state != UA_EVENTLOOPSTATE_STARTED) {
        retVal = el->start(el);
        UA_CHECK_STATUS(retVal, return retVal); /* Errors are logged internally */
    }

    /* Take the server lock */
    UA_LOCK(&server->serviceMutex);

    /* Does the ApplicationURI match the local certificates? */
#ifdef UA_ENABLE_ENCRYPTION
    retVal = verifyServerApplicationURI(server);
    UA_CHECK_STATUS(retVal, UA_UNLOCK(&server->serviceMutex); return retVal);
#endif

    /* Add a regular callback for housekeeping tasks. With a 1s interval. */
    retVal = addRepeatedCallback(server, serverHouseKeeping,
                                 NULL, 1000.0, &server->houseKeepingCallbackId);
    UA_CHECK_STATUS_ERROR(retVal, UA_UNLOCK(&server->serviceMutex); return retVal,
                          &config->logger, UA_LOGCATEGORY_SERVER,
                          "Could not create the server housekeeping task");

    /* Add a regular callback for network polling tasks. With a 200ms interval.
     *
     * TODO: Move this to the EventLoop model without polling.
     */
    if(server->pollingCallbackId == 0) {
        addRepeatedCallback(server, serverPolling, NULL, 200.0,
                            &server->pollingCallbackId);
    }

    /* <-- The point of no return for starting the server --> */

    /* Open server sockets */
    UA_Boolean haveServerSocket = false;
    if(config->serverUrlsSize == 0) {
        /* Empty hostname -> listen on all devices */
        UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_SERVER,
                       "No Server URL configured. Using \"opc.tcp://:4840\" "
                       "to configure the listen socket.");
        UA_String defaultUrl = UA_STRING("opc.tcp://:4840");
        retVal = createServerConnection(server, &defaultUrl);
        if(retVal == UA_STATUSCODE_GOOD)
            haveServerSocket = true;
    } else {
        for(size_t i = 0; i < config->serverUrlsSize; i++) {
            retVal = createServerConnection(server, &config->serverUrls[i]);
            if(retVal == UA_STATUSCODE_GOOD)
                haveServerSocket = true;
        }
    }

    if(!haveServerSocket) {
        UA_LOG_ERROR(&config->logger, UA_LOGCATEGORY_SERVER,
                     "The server has no server socket");
    }

    /* Update the application description to include the server urls for
     * discovery. Don't add the urls with an empty host (listening on all
     * interfaces) */
    for(size_t i = 0; i < config->serverUrlsSize; i++) {
        UA_String hostname = UA_STRING_NULL;
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;
        retVal = UA_parseEndpointUrl(&config->serverUrls[i],
                                     &hostname, &port, &path);
        if(retVal != UA_STATUSCODE_GOOD || hostname.length == 0)
            continue;

        /* Check if the ServerUrl is already present in the DiscoveryUrl array.
         * Add if not already there. */
        size_t j = 0;
        for(; j < config->applicationDescription.discoveryUrlsSize; j++) {
            if(UA_String_equal(&config->serverUrls[i],
                               &config->applicationDescription.discoveryUrls[j]))
                break;
        }
        if(j == config->applicationDescription.discoveryUrlsSize) {
            retVal =
                UA_Array_appendCopy((void**)&config->applicationDescription.discoveryUrls,
                                    &config->applicationDescription.discoveryUrlsSize,
                                    &config->serverUrls[i], &UA_TYPES[UA_TYPES_STRING]);
            (void)retVal;
        }
    }

    /* Ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

    /* At least one endpoint has to be configured */
    if(config->endpointsSize == 0) {
        UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_SERVER,
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

    /* Set the server to STARTED. From here on, only use
     * UA_Server_run_shutdown(server) to stop the server. */
    setServerLifecycleState(server, UA_LIFECYCLESTATE_STARTED);

    UA_UNLOCK(&server->serviceMutex);
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
    if(server == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_LOCK(&server->serviceMutex);

    if(server->state != UA_LIFECYCLESTATE_STARTED) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "The server is not started, cannot be shut down");
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set to stopping and notify the application */
    setServerLifecycleState(server, UA_LIFECYCLESTATE_STOPPING);

    /* Stop the regular housekeeping tasks */
    if(server->houseKeepingCallbackId != 0) {
        removeCallback(server, server->houseKeepingCallbackId);
        server->houseKeepingCallbackId = 0;
    }

    /* Stop the polling tasks */
    if(server->pollingCallbackId != 0) {
        removeCallback(server, server->pollingCallbackId);
        server->pollingCallbackId = 0;
    }

    /* Stop the regular retry callback */
    setReverseConnectRetryCallback(server, false);

    /* Close or free all reverse connections */
    reverse_connect_context *rev, *rev_tmp;
    LIST_FOREACH_SAFE(rev, &server->reverseConnects, next, rev_tmp) {
        if(rev->currentConnection.connectionId) {
            UA_ConnectionManager *cm = rev->currentConnection.connectionManager;
            rev->destruction = true;
            cm->closeConnection(cm, rev->currentConnection.connectionId);
        } else {
            LIST_REMOVE(rev, next);
            setReverseConnectState(server, rev, UA_SECURECHANNELSTATE_CLOSED);
            UA_String_clear(&rev->hostname);
            free(rev);
        }
    }

    /* Stop all SecureChannels */
    UA_Server_deleteSecureChannels(server);

    /* Stop all ServerComponents */
    ZIP_ITER(UA_ServerComponentTree, &server->serverComponents,
             stopServerComponent, server);

    /* Stop all server sockets */
    for(size_t i = 0; i < UA_MAXSERVERCONNECTIONS; i++) {
        UA_ServerConnection *sc = &server->serverConnections[i];
        UA_ConnectionManager *cm = sc->connectionManager;
        if(sc->connectionId > 0)
            cm->closeConnection(cm, sc->connectionId);
    }

    /* Are we already stopped? */
    if(testStoppedCondition(server)) {
        setServerLifecycleState(server, UA_LIFECYCLESTATE_STOPPED);
    }

    /* Only stop the EventLoop if it is coupled to the server lifecycle  */
    if(server->config.externalEventLoop) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_GOOD;
    }

    /* Iterate the EventLoop until the server is stopped */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_EventLoop *el = server->config.eventLoop;
    while(!testStoppedCondition(server) &&
          res == UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&server->serviceMutex);
        res = el->run(el, 100);
        UA_LOCK(&server->serviceMutex);
    }

    /* Stop the EventLoop. Iterate until stopped. */
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED &&
          el->state != UA_EVENTLOOPSTATE_FRESH &&
          res == UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&server->serviceMutex);
        res = el->run(el, 100);
        UA_LOCK(&server->serviceMutex);
    }

    /* Set server lifecycle state to stopped if not already the case */
    setServerLifecycleState(server, UA_LIFECYCLESTATE_STOPPED);

    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_run(UA_Server *server, const volatile UA_Boolean *running) {
    UA_StatusCode retval = UA_Server_run_startup(server);
    UA_CHECK_STATUS(retval, return retval);

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

