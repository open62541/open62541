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

#include "ua_pubsub_manager.h"

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
 * read / write services, or UA_Server_addNamespace,
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
UA_Server_getNamespaceByName(UA_Server *server, const UA_String namespaceUri,
                             size_t *foundIndex) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = getNamespaceByName(server, namespaceUri, foundIndex);
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

/********************/
/* Server Lifecycle */
/********************/

/* The server needs to be stopped before it can be deleted */
void UA_Server_delete(UA_Server *server) {
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

#ifdef UA_ENABLE_DISCOVERY
    UA_DiscoveryManager_clear(&server->discoveryManager, server);
#endif

#if UA_MULTITHREADING >= 100
    UA_AsyncManager_clear(&server->asyncManager, server);
#endif

    /* Stop the EventLoop and iterate until stopped or an error occurs */
    if(server->config.eventLoop->state == UA_EVENTLOOPSTATE_STARTED)
        server->config.eventLoop->stop(server->config.eventLoop);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    while(res == UA_STATUSCODE_GOOD &&
          (server->config.eventLoop->state != UA_EVENTLOOPSTATE_FRESH &&
           server->config.eventLoop->state != UA_EVENTLOOPSTATE_STOPPED)) {

        UA_UNLOCK(&server->serviceMutex);
        res = server->config.eventLoop->run(server->config.eventLoop, 100);
        UA_LOCK(&server->serviceMutex);
    }

    /* Clean up the Admin Session */
    UA_Session_clear(&server->adminSession, server);

    UA_UNLOCK(&server->serviceMutex); /* The timer has its own mutex */

    /* Clean up the config */
    UA_ServerConfig_clean(&server->config);

#if UA_MULTITHREADING >= 100
    UA_LOCK_DESTROY(&server->networkMutex);
    UA_LOCK_DESTROY(&server->serviceMutex);
#endif

    /* Delete the server itself */
    UA_free(server);
}

/* Recurring cleanup. Removing unused and timed-out channels and sessions */
static void
UA_Server_cleanup(UA_Server *server, void *_) {
    UA_LOCK(&server->serviceMutex);
    UA_DateTime nowMonotonic = UA_DateTime_nowMonotonic();
    UA_Server_cleanupSessions(server, nowMonotonic);
    UA_Server_cleanupTimedOutSecureChannels(server, nowMonotonic);
#ifdef UA_ENABLE_DISCOVERY
    UA_Discovery_cleanupTimedOut(server, nowMonotonic);
#endif
    UA_UNLOCK(&server->serviceMutex);
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
                    "No Nodestore configured in the server"
                   );

    /* Init start time to zero, the actual start time will be sampled in
     * UA_Server_run_startup() */
    server->startTime = 0;

    /* Set a seed for non-cyptographic randomness */
#ifndef UA_ENABLE_DETERMINISTIC_RNG
    UA_random_seed((UA_UInt64)UA_DateTime_now());
#endif

#if UA_MULTITHREADING >= 100
    UA_LOCK_INIT(&server->networkMutex);
    UA_LOCK_INIT(&server->serviceMutex);
#endif

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

    /* Add a regular callback for cleanup and maintenance. With a 10s interval. */
    UA_Server_addRepeatedCallback(server, (UA_ServerCallback)UA_Server_cleanup, NULL,
                                  10000.0, NULL);

    /* Initialize namespace 0*/
    res = UA_Server_initNS0(server);
    UA_CHECK_STATUS(res, goto cleanup);

#ifdef UA_ENABLE_PUBSUB
    /* Build PubSub information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    UA_Server_initPubSubNS0(server);
#endif

#ifdef UA_ENABLE_PUBSUB_MONITORING
    /* setup default PubSub monitoring callbacks */
    res = UA_PubSubManager_setDefaultMonitoringCallbacks(&server->config.pubSubConfig.monitoringInterface);
    UA_CHECK_STATUS(res, goto cleanup);
#endif /* UA_ENABLE_PUBSUB_MONITORING */
#endif /* UA_ENABLE_PUBSUB */
    return server;

 cleanup:
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
        server->config.securityPolicies[i].logger = &server->config.logger;

    server->config.eventLoop->logger = &server->config.logger;

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
    UA_StatusCode retval =
        addRepeatedCallback(server, callback, data, interval_ms, callbackId);
    UA_UNLOCK(&server->serviceMutex);
    return retval;
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
    server->config.eventLoop->removeCyclicCallback(server->config.eventLoop,
                                                   callbackId);
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
            if(UA_ByteString_equal(&entry->channel.securityPolicy->localCertificate, oldCertificate))
                UA_Server_closeSecureChannel(server, &entry->channel, UA_DIAGNOSTICEVENT_CLOSE);
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
            verifyApplicationURI(server->config.certificateVerification.context,
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
    stat.ns = server->networkStatistics;
    stat.scs = server->secureChannelStatistics;

    stat.ss.currentSessionCount = server->activeSessionCount;
    stat.ss.cumulatedSessionCount =
        server->serverDiagnosticsSummary.cumulatedSessionCount;
    stat.ss.securityRejectedSessionCount =
        server->serverDiagnosticsSummary.securityRejectedSessionCount;
    stat.ss.rejectedSessionCount =
        server->serverDiagnosticsSummary.rejectedSessionCount;
    stat.ss.sessionTimeoutCount =
        server->serverDiagnosticsSummary.sessionTimeoutCount;
    stat.ss.sessionAbortCount =
        server->serverDiagnosticsSummary.sessionAbortCount;
    
    return stat;
}

/* Callback of a TCP socket (server socket or an active connection) */
static void
UA_Server_networkCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                          void *application, void **connectionContext,
                          UA_StatusCode state,
                          size_t paramsSize, const UA_KeyValuePair *params,
                          UA_ByteString msg) {
    UA_Server *server = (UA_Server*)application;

    /* A server socket that is not registered in the server */
    if(*connectionContext == NULL) {
        if(state != UA_STATUSCODE_GOOD)
            return; /* Closing an unregistered server socket */

        /* Cannot register */
        if(server->serverConnectionsSize >= UA_MAXSERVERCONNECTIONS) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Cannot register server socket - too many already open");
            cm->closeConnection(cm, connectionId);
            return;
        }

        /* Find and use a free connection slot */
        server->serverConnectionsSize++;
        UA_ServerConnection *sc = server->serverConnections;
        while(sc->connectionId != 0)
            sc++;
        sc->state = UA_SERVERCONNECTIONSTATE_STARTED;
        sc->connectionId = connectionId;
        sc->connectionManager = cm;
        *connectionContext = (void*)sc; /* Set the context pointer in the connection */
        return;
    }

    UA_ServerConnection *sc = (UA_ServerConnection*)*connectionContext;
    UA_Connection *conn = (UA_Connection*)*connectionContext;
    UA_Boolean serverSocket = (sc >= server->serverConnections &&
                               sc < &server->serverConnections[UA_MAXSERVERCONNECTIONS]);

    if(state != UA_STATUSCODE_GOOD) {
        if(serverSocket) {
            /* A server socket is closing */
            sc->state = UA_SERVERCONNECTIONSTATE_FRESH;
            sc->connectionId = 0;
            server->serverConnectionsSize--;
        } else {
            /* A normal connection is closing */
            if(conn->channel)
                UA_SecureChannel_close(conn->channel);
            UA_free(conn);
        }
        return;
    }

    /* A new connection is opening - still has the pointer to the serversocket */
    if(serverSocket) {
        conn = (UA_Connection*)UA_malloc(sizeof(UA_Connection));
        if(!conn) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Could not accept the connection - out of memory");
            cm->closeConnection(cm, connectionId);
            return;
        }

        conn->state = UA_CONNECTIONSTATE_OPENING;
        conn->channel = NULL;
        conn->sockfd = (UA_SOCKET)connectionId;
        conn->handle = cm;
        conn->getSendBuffer = UA_Server_Connection_getSendBuffer;
        conn->releaseSendBuffer = UA_Server_Connection_releaseBuffer;
        conn->send = UA_Server_Connection_send;
        conn->recv = NULL;
        conn->releaseRecvBuffer = UA_Server_Connection_releaseBuffer;
        conn->close = UA_Server_Connection_close;
        conn->free = NULL;

        *connectionContext = (void*)conn;
        return;
    }

    /* Received a message on a normal connection */
    UA_Server_processBinaryMessage(server, conn, &msg);
}

static UA_StatusCode
UA_Server_createServerConnection(UA_Server *server, const UA_String *serverUrl) {
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

        params[0].key = UA_QUALIFIEDNAME(0, "listen-port");
        UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);

        UA_UInt32 bufSize = config->tcpBufSize;
        if(bufSize == 0)
            bufSize = 1 << 16; /* 64kB */
        params[1].key = UA_QUALIFIEDNAME(0, "recv-bufsize");
        UA_Variant_setScalar(&params[1].value, &bufSize, &UA_TYPES[UA_TYPES_UINT32]);

        /* If the hostname is non-empty */
        if(hostname.length > 0) {
            params[2].key = UA_QUALIFIEDNAME(0, "listen-hostnames");
            UA_Variant_setArray(&params[2].value, &hostname, 1, &UA_TYPES[UA_TYPES_STRING]);
            paramsSize = 3;
        }

        /* Open the server connection */
        res = cm->openConnection(cm, paramsSize, params, server, NULL,
                                 UA_Server_networkCallback);
        if(res == UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_BADINTERNALERROR;
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

    /* Start the EventLoop */
    UA_StatusCode retVal = config->eventLoop->start(config->eventLoop);
    UA_CHECK_STATUS(retVal, return retVal);

    /* Open server sockets */
    UA_Boolean haveServerSocket = false;
    if(config->serverUrlsSize == 0) {
        /* Empty hostname -> listen on all devices */
        UA_String defaultUrl = UA_STRING("opc.tcp://:4840");
        retVal = UA_Server_createServerConnection(server, &defaultUrl);
        if(retVal == UA_STATUSCODE_GOOD)
            haveServerSocket = true;
    } else {
        for(size_t i = 0; i < config->serverUrlsSize; i++) {
            retVal = UA_Server_createServerConnection(server, &config->serverUrls[i]);
            if(retVal == UA_STATUSCODE_GOOD)
                haveServerSocket = true;
        }
    }

    /* Warn if no socket available */
    if(!haveServerSocket) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "The server has no server socket");
    }

    /* ensure that the uri for ns1 is set up from the app description */
    setupNs1Uri(server);

    /* write ServerArray with same ApplicationURI value as NamespaceArray */
    retVal = writeNs0VariableArray(server, UA_NS0ID_SERVER_SERVERARRAY,
                                   &server->config.applicationDescription.applicationUri,
                                   1, &UA_TYPES[UA_TYPES_STRING]);
    UA_CHECK_STATUS(retVal, return retVal);

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
    UA_CHECK_STATUS(retVal, return retVal);
#endif

    /* Sample the start time and set it to the Server object */
    server->startTime = UA_DateTime_now();
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, &server->startTime, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Server_writeValue(server,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME),
                         var);

    /* Update the application description to include the server urls for
     * discovery. Don't add the urls with an empty host (listening on all
     * interfaces) */
    for(size_t i = 0; i < server->config.serverUrlsSize; i++) {
        UA_String hostname = UA_STRING_NULL;
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = 0;
        retVal = UA_parseEndpointUrl(&server->config.serverUrls[i],
                                     &hostname, &port, &path);
        if(retVal != UA_STATUSCODE_GOOD || hostname.length == 0)
            continue;
        retVal =
            UA_Array_appendCopy((void**)&server->config.applicationDescription.discoveryUrls,
                                &server->config.applicationDescription.discoveryUrlsSize,
                                &server->config.serverUrls[i], &UA_TYPES[UA_TYPES_STRING]);
        UA_CHECK_STATUS(retVal, return retVal);
    }

    /* Start the multicast discovery server */
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.mdnsEnabled)
        startMulticastDiscoveryServer(server);
#endif

    /* Update Endpoint description */
    for(size_t i = 0; i < server->config.endpointsSize; ++i) {
        UA_ApplicationDescription_clear(&server->config.endpoints[i].server);
        UA_ApplicationDescription_copy(&server->config.applicationDescription,
                                       &server->config.endpoints[i].server);
    }

    server->state = UA_SERVERLIFECYCLE_FRESH;

    return UA_STATUSCODE_GOOD;
}

UA_UInt16
UA_Server_run_iterate(UA_Server *server, UA_Boolean waitInternal) {

    UA_UInt16 timeout = 0;

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
    /* Process repeated work */
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime nextRepeated =
        server->config.eventLoop->nextCyclicTime(server->config.eventLoop);
    UA_DateTime latest = now + (UA_MAXTIMEOUT * UA_DATETIME_MSEC);
    if(nextRepeated > latest)
        nextRepeated = latest;
    /* round always to upper value to avoid timeout to be set to 0
    * if(nextRepeated - now) < (UA_DATETIME_MSEC/2) */
    if(waitInternal)
        timeout = (UA_UInt16)(((nextRepeated - now) + (UA_DATETIME_MSEC - 1)) / UA_DATETIME_MSEC);

    server->config.eventLoop->run(server->config.eventLoop, timeout);

#if defined(UA_ENABLE_DISCOVERY_MULTICAST) && (UA_MULTITHREADING < 200)

    UA_LOCK(&server->serviceMutex);
    if(server->config.mdnsEnabled) {

        /* TODO multicastNextRepeat does not consider new input data (requests)
         * on the socket. It will be handled on the next call. if needed, we
         * need to use select with timeout on the multicast socket
         * server->mdnsSocket (see example in mdnsd library) on higher level. */
        UA_DateTime multicastNextRepeat = 0;
        UA_StatusCode hasNext =
            iterateMulticastDiscoveryServer(server, &multicastNextRepeat, true);
        if(hasNext == UA_STATUSCODE_GOOD && multicastNextRepeat < nextRepeated)
            nextRepeated = multicastNextRepeat;
    }
    UA_UNLOCK(&server->serviceMutex);
#endif

    return timeout;
}

UA_StatusCode
UA_Server_run_shutdown(UA_Server *server) {
    /* Stop all SecureChannels */
    UA_Server_deleteSecureChannels(server);

    /* Stop all server sockets */
    for(size_t i = 0; i < UA_MAXSERVERCONNECTIONS; i++) {
        UA_ServerConnection *sc = &server->serverConnections[i];
        if(sc->connectionId > 0)
            sc->connectionManager->
                closeConnection(sc->connectionManager, sc->connectionId);
    }

    UA_EventLoop *el = server->config.eventLoop;
    if(server->config.externalEventLoop) {
        el->run(el, 0); /* Run one iteration of the eventloop with a zero
                         * timeout. This closes the connections fully. */
    } else {
        el->stop(el);
        UA_StatusCode res = UA_STATUSCODE_GOOD;
        while(el->state != UA_EVENTLOOPSTATE_STOPPED &&
              el->state != UA_EVENTLOOPSTATE_FRESH &&
              res == UA_STATUSCODE_GOOD)
            res = el->run(el, 100); /* Iterate until stopped */
    }

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    /* Stop multicast discovery */
    if(server->config.mdnsEnabled)
        stopMulticastDiscoveryServer(server);
#endif

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

