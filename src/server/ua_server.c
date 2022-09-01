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

#ifdef UA_ENABLE_PUBSUB
#include "ua_pubsub_manager.h"
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "ua_subscription.h"
#endif

#ifdef UA_ENABLE_VALGRIND_INTERACTIVE
#include <valgrind/memcheck.h>
#endif

#include "open62541/server_config_default.h"
#include "open62541/plugin/pki_default.h"

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
    if(namespaceIndex > server->namespacesSize)
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
    UA_LOCK_DESTROY(&server->serviceMutex);
#endif

    /* Delete the server itself */
    UA_free(server);
}

/* Regular house-keeping tasks. Removing unused and timed-out channels and
 * sessions. */
static void
serverHouseKeeping(UA_Server *server, void *_) {
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
                   "No Nodestore configured in the server");

    /* Init start time to zero, the actual start time will be sampled in
     * UA_Server_run_startup() */
    server->startTime = 0;

    /* Set a seed for non-cyptographic randomness */
#ifndef UA_ENABLE_DETERMINISTIC_RNG
    UA_random_seed((UA_UInt64)UA_DateTime_now());
#endif

#if UA_MULTITHREADING >= 100
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
        if(server->config.securityPolicies[i].logger == &config->logger)
            server->config.securityPolicies[i].logger = &server->config.logger;

    if(server->config.eventLoop->logger == &config->logger)
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
    el->removeCyclicCallback(el, callbackId);
}

void
UA_Server_removeCallback(UA_Server *server, UA_UInt64 callbackId) {
    UA_LOCK(&server->serviceMutex);
    removeCallback(server, callbackId);
    UA_UNLOCK(&server->serviceMutex);
}

UA_StatusCode
UA_Server_updateCertificate(
	UA_Server *server,
	const UA_NodeId* certificateGroupId,
	const UA_NodeId* certificateTypeId,
    const UA_ByteString* certificate,
    const UA_ByteString* privateKey,
    UA_Boolean closeSessions,
    UA_Boolean closeSecureChannels
) {
	if (server == NULL || certificateGroupId == NULL || certificateTypeId == NULL ||
		certificate == NULL || privateKey == NULL) {
		return UA_STATUSCODE_BADINVALIDARGUMENT;
	}

    /* close sessions on certificate update */
    if(closeSessions) {
        session_list_entry *current;
        LIST_FOREACH(current, &server->sessions, pointers) {
        	UA_PKIStore* pkiStore = current->session.header.channel->endpoint->pkiStore;
        	UA_SecurityPolicy *securityPolicy = current->session.header.channel->endpoint->securityPolicy;

        	/* Check used certificate group */
        	if (!UA_NodeId_equal(certificateGroupId, &pkiStore->certificateGroupId)) {
        		continue;
        	}

        	/* Check used certificate type */
           	if (!UA_NodeId_equal(certificateTypeId, &securityPolicy->certificateTypeId)) {
            	continue;
            }

           	/* Close session */
            UA_LOCK(&server->serviceMutex);
            UA_Server_removeSessionByToken(server, &current->session.header.authenticationToken,
                                           UA_DIAGNOSTICEVENT_CLOSE);
            UA_UNLOCK(&server->serviceMutex);
        }
    }

    /* Close securechannels on certificate update */
    if(closeSecureChannels) {
        channel_entry *entry;
        TAILQ_FOREACH(entry, &server->channels, pointers) {
        	UA_PKIStore* pkiStore = entry->channel.endpoint->pkiStore;
        	UA_SecurityPolicy *securityPolicy = entry->channel.endpoint->securityPolicy;

           	/* Check used certificate group */
        	if (!UA_NodeId_equal(certificateGroupId, &pkiStore->certificateGroupId)) {
            	continue;
            }

            /* Check used certificate type */
            if (!UA_NodeId_equal(certificateTypeId, &securityPolicy->certificateTypeId)) {
                continue;
            }

        	/* Close secure channel */
        	shutdownServerSecureChannel(server, &entry->channel, UA_DIAGNOSTICEVENT_CLOSE);
        }
    }

    /* update certificate and private key */
    for (size_t idx = 0; idx < server->config.pkiStoresSize; idx++) {
    	UA_PKIStore* pkiStore = &server->config.pkiStores[idx];

    	/* Add new certificate */
    	UA_StatusCode retval = pkiStore->storeCertificate(pkiStore, *certificateTypeId, certificate);
    	if (retval != UA_STATUSCODE_GOOD) {
    		return retval;
    	}

    	/* Add new private key */
        retval = pkiStore->storePrivateKey(pkiStore, *certificateTypeId, privateKey);
       	if (retval != UA_STATUSCODE_GOOD) {
        	return retval;
        }
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
// TODO: Move this check to certificate plugin. The plugin should verify the provided pki contents anyways and maybe
// TODO: give some information on the number of trusted certs and so on. this check should be performed on startup
// TODO: The check should also be configured by default to fail the startup if any sanity checks fail.
//static UA_StatusCode
//verifyServerApplicationURI(const UA_Server *server) {
//    const UA_String securityPolicyNoneUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
//    for(size_t i = 0; i < server->config.securityPoliciesSize; i++) {
//        UA_SecurityPolicy *sp = &server->config.securityPolicies[i];
//        if(UA_String_equal(&sp->policyUri, &securityPolicyNoneUri) && (sp->localCertificate.length == 0))
//            continue;
//        UA_StatusCode retval = server->config.certificateManager.
//            verifyApplicationURI(server->config.certificateManager.context,
//                                 &sp->localCertificate,
//                                 &server->config.applicationDescription.applicationUri);
//
//        UA_CHECK_STATUS_ERROR(retval, return retval, &server->config.logger, UA_LOGCATEGORY_SERVER,
//                       "The configured ApplicationURI \"%.*s\"does not match the "
//                       "ApplicationURI specified in the certificate for the "
//                       "SecurityPolicy %.*s",
//                       (int)server->config.applicationDescription.applicationUri.length,
//                       server->config.applicationDescription.applicationUri.data,
//                       (int)sp->policyUri.length, sp->policyUri.data);
//    }
//    return UA_STATUSCODE_GOOD;
//}
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
        UA_KeyValueMap paramsMap;
        paramsMap.map = params;
        paramsMap.mapSize = paramsSize;

        /* Open the server connection */
        res = cm->openConnection(cm, &paramsMap, server, NULL,
                                 UA_Server_networkCallback);
        if(res == UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_StatusCode attemptReverseConnect(UA_Server *server, reverse_connect_context *context) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
    UA_String tcpString = UA_STRING_STATIC("tcp");
    for(UA_EventSource *es = config->eventLoop->eventSources; es != NULL; es = es->next) {
        /* Is this a usable connection manager? */
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(!UA_String_equal(&tcpString, &cm->protocol))
            continue;

        if (es->state != UA_EVENTSOURCESTATE_STARTED)
            return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;

        /* Set up the parameters */
        UA_KeyValueMap params;
        params.mapSize = 0;
        params.map = NULL;
        UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "hostname"), &context->hostname,
                                 &UA_TYPES[UA_TYPES_STRING]);
        UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "port"), &context->port,
                                 &UA_TYPES[UA_TYPES_UINT16]);

        /* Open the server connection */
        res = cm->openConnection(cm, &params, server, context,
                                 UA_Server_reverseConnectCallback);

        UA_KeyValueMap_clear(&params);

        if (res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Failed to create connection for reverse connect: %s\n",
                           UA_StatusCode_name(res));
            context->currentConnection.connectionId = 0;
        }

        if (context->state != UA_SECURECHANNELSTATE_CONNECTING) {
            context->state = UA_SECURECHANNELSTATE_CONNECTING;
            if (context->stateCallback)
                context->stateCallback(server, context->handle, context->state,
                                       context->callbackContext);
        }
    }

    return res;
}

UA_StatusCode UA_Server_addReverseConnect(UA_Server *server, UA_String url,
                                          UA_Server_ReverseConnectStateCallback stateCallback,
                                          void *callbackContext, UA_UInt64 *handle) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_SERVER,
                       "OPC UA URL is invalid: %.*s",
                       (int)url.length, url.data);
        return res;
    }

    if (SLIST_EMPTY(&server->reverseConnects))
        setReverseConnectRetryCallback(server, true);

    reverse_connect_context *newContext = (reverse_connect_context *)calloc(1, sizeof(reverse_connect_context));
    if (newContext == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_String_copy(&hostname, &newContext->hostname);
    newContext->port = port;
    newContext->handle = ++server->lastReverseConnectHandle;
    newContext->stateCallback = stateCallback;
    newContext->callbackContext = callbackContext;
    SLIST_INSERT_HEAD(&server->reverseConnects, newContext, next);

    if (handle)
        *handle = newContext->handle;

    return attemptReverseConnect(server, newContext);
}

static void freeReverseConnectCallback(void *application, void *context) {
    reverse_connect_context *reverseConnect = (reverse_connect_context *)context;

    if (reverseConnect) {
        UA_String_clear(&reverseConnect->hostname);
        free(reverseConnect);
    }

    if (application)
        free(application);
}

UA_StatusCode UA_Server_removeReverseConnect(UA_Server *server, UA_UInt64 handle) {
    reverse_connect_context *rev = NULL;
    reverse_connect_context *temp = NULL;
    UA_StatusCode result = UA_STATUSCODE_BADNOTFOUND;

    SLIST_FOREACH_SAFE(rev, &server->reverseConnects, next, temp) {
        if (rev->handle == handle) {
            SLIST_REMOVE(&server->reverseConnects, rev, reverse_connect_context, next);

            if (rev->currentConnection.connectionId) {
                rev->destruction = true;
                /* Request disconnect and run the event loop for one iteration */
                if (rev->currentConnection.connectionId) {
                    UA_DelayedCallback *freeCallback = (UA_DelayedCallback *)calloc(1, sizeof(UA_DelayedCallback));
                    freeCallback->context = rev;
                    freeCallback->application = freeCallback;
                    freeCallback->callback = freeReverseConnectCallback;

                    server->config.eventLoop->addDelayedCallback(server->config.eventLoop, freeCallback);

                    rev->currentConnection.connectionManager->closeConnection(
                                rev->currentConnection.connectionManager,
                                rev->currentConnection.connectionId);
                }
            } else {
                setReverseConnectState(server, rev, UA_SECURECHANNELSTATE_CLOSED);
                UA_String_clear(&rev->hostname);
                free(rev);
            }
            result = UA_STATUSCODE_GOOD;
            break;
        }
    }

    if (SLIST_EMPTY(&server->reverseConnects))
         setReverseConnectRetryCallback(server, false);

    return result;
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

    /* Add a regular callback for housekeeping tasks. With a 1s interval. */
    if(server->houseKeepingCallbackId == 0) {
        UA_Server_addRepeatedCallback(server, (UA_ServerCallback)serverHouseKeeping,
                                      NULL, 1000.0, &server->houseKeepingCallbackId);
    }

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    /* Start the EventLoop if not already started */
    UA_CHECK_MEM_ERROR(config->eventLoop, return UA_STATUSCODE_BADINTERNALERROR,
                       &config->logger, UA_LOGCATEGORY_SERVER,
                       "eventloop must be set");
    if (config->eventLoop->state != UA_EVENTLOOPSTATE_STARTED) {
        retVal = config->eventLoop->start(config->eventLoop);
        UA_CHECK_STATUS(retVal, return retVal);
    }

    /* Open server sockets */
    UA_Boolean haveServerSocket = false;
    if(config->serverUrlsSize == 0) {
        /* Empty hostname -> listen on all devices */
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "No Server URL configured. Using \"opc.tcp://:4840\" "
                       "to configure the listen socket.");
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
    // TODO: Move this to certificate plugin
//    retVal = verifyServerApplicationURI(server);
//    UA_CHECK_STATUS(retVal, return retVal);
#endif

#ifdef UA_ENABLE_PUBSUB
    /* Initialized PubSubManager */
    UA_PubSubManager_init(server, &server->pubSubManager);
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

#ifdef UA_ENABLE_ENCRYPTION
    /* Register getRejectedListMethode */
    retVal = UA_Server_setMethodNodeCallback(server, UA_NODEID_NUMERIC(0,
    									     UA_NS0ID_SERVERCONFIGURATION_GETREJECTEDLIST),
                                             getRejectedListMethod);
    UA_CHECK_STATUS(retVal, return retVal);

    /* Register updateCertificateMethode */
    retVal = UA_Server_setMethodNodeCallback(server, UA_NODEID_NUMERIC(0,
    									     UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE),
                                             updateCertificateMethod);
    UA_CHECK_STATUS(retVal, return retVal);

    /* Add createSigningRequest callback */
    retVal = UA_Server_setMethodNodeCallback(server, UA_NODEID_NUMERIC(0,
                                          UA_NS0ID_SERVERCONFIGURATION_CREATESIGNINGREQUEST),
                                          createSigningRequest);
    UA_CHECK_STATUS(retVal, return retVal);
#endif

    /* Start the multicast discovery server */
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.mdnsEnabled)
        startMulticastDiscoveryServer(server);
#endif

    /* Update Endpoint description */
    for(size_t i = 0; i < server->config.endpointsSize; ++i) {
        UA_Endpoint_updateApplicationDescription(&server->config.endpoints[i], &server->config.applicationDescription);
    }

    server->state = UA_SERVERLIFECYCLE_FRESH;

    return UA_STATUSCODE_GOOD;
}

UA_UInt16
UA_Server_run_iterate(UA_Server *server, UA_Boolean waitInternal) {
    /* Listen on the pubsublayer, but only if the yield function is set.
     * TODO: Integrate into the EventLoop */
#if defined(UA_ENABLE_PUBSUB_MQTT)
    UA_PubSubConnection *connection;
    TAILQ_FOREACH(connection, &server->pubSubManager.connections, listEntry){
        UA_PubSubConnection *ps = connection;
        if(ps && ps->channel && ps->channel->yield){
            ps->channel->yield(ps->channel, 0);
        }
    }
#endif

    /* Process timed and network events in the EventLoop */
    server->config.eventLoop->run(server->config.eventLoop, UA_MAXTIMEOUT);

#if defined(UA_ENABLE_DISCOVERY_MULTICAST) && (UA_MULTITHREADING < 200)
    UA_LOCK(&server->serviceMutex);
    if(server->config.mdnsEnabled) {
        /* TODO multicastNextRepeat does not consider new input data (requests)
         * on the socket. It will be handled on the next call. if needed, we
         * need to use select with timeout on the multicast socket
         * server->mdnsSocket (see example in mdnsd library) on higher level. */
        UA_DateTime multicastNextRepeat = 0;
        iterateMulticastDiscoveryServer(server, &multicastNextRepeat, true);
    }
    UA_UNLOCK(&server->serviceMutex);
#endif

    /* Return the time until the next scheduled callback */
    return (UA_UInt16)((server->config.eventLoop->nextCyclicTime(server->config.eventLoop)
                        - UA_DateTime_nowMonotonic()) / UA_DATETIME_MSEC);
}

UA_StatusCode
UA_Server_run_shutdown(UA_Server *server) {
    /* Stop the regular housekeeping tasks */
    UA_Server_removeCallback(server, server->houseKeepingCallbackId);
    server->houseKeepingCallbackId = 0;

    /* Mark all reverse connects as destroying */
    reverse_connect_context *rev = NULL;
    SLIST_FOREACH(rev, &server->reverseConnects, next) {
        rev->destruction = true;
        if (rev->currentConnection.connectionId) {
            rev->currentConnection.connectionManager->closeConnection(rev->currentConnection.connectionManager,
                                                                      rev->currentConnection.connectionId);
        }

        setReverseConnectState(server, rev, UA_SECURECHANNELSTATE_CLOSED);
    }

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

    setReverseConnectRetryCallback(server, false);
    reverse_connect_context *next = server->reverseConnects.slh_first;
    while (next) {
        reverse_connect_context *current = next;
        next = current->next.sle_next;
        UA_String_clear(&current->hostname);
        free(current);
    }

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

/* Add a new capability to the server configuration object */
UA_StatusCode
UA_Server_configAddCapability(UA_Server *server, const UA_String *newCapability) {

    /* Valid Capabilities from deps/ua-nodeset/Schema/ServerCapabilities.csv */
	static char validServerConfigCapabilities[][16] = {
	    "NA", "DA", "HD", "AC", "HE", "GDS", "LDS", "DI", "ADI", "FDI",
        "FDIC", "PLC", "S95", "RCP", "PUB", "AUTOID", "MDIS", "CNC", "PLK", "FDT",
        "TMC", "CSPP", "61850", "PACKML", "MTC", "AUTOML", "SERCOS", "MIMOSA", "WITSML", "DEXPI",
        "IOLINK", "VROBOT", "PNO", "PADIM"
    };

	/* check for nullparameter */
	if ((server == NULL) || (newCapability == NULL) || UA_String_equal(newCapability, &UA_STRING_NULL)) {
	    return UA_STATUSCODE_BADINVALIDARGUMENT;
	}

	/* Check for valid capability argument */
	UA_Boolean isValid = false;
	size_t i = 0;
	for (i = 0; i < (sizeof(validServerConfigCapabilities)/sizeof(validServerConfigCapabilities[0])); i++) {
		UA_String str = UA_STRING(validServerConfigCapabilities[i]);
		if (UA_String_equal(newCapability, &str)) {
			isValid = true;
			break;
		}
	}
	if (!isValid) {
		return UA_STATUSCODE_BADINVALIDARGUMENT;
	}

	/* Read capabilities value */
	UA_Variant capabilities;
	UA_Variant_init(&capabilities);
	UA_StatusCode retval = UA_Server_readValue(server,
	                       UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_SERVERCAPABILITIES), &capabilities);
	if (retval != UA_STATUSCODE_GOOD) {
		return retval;  /* reading capabilities failed */
	}

	/* Clear incorrectly allocated array dimensions */
	UA_Array_delete(capabilities.arrayDimensions, capabilities.arrayDimensionsSize, &UA_TYPES[UA_TYPES_UINT32]);
	capabilities.arrayDimensions = NULL;
	capabilities.arrayDimensionsSize = 0;

	/* Check for duplicate capability */
	UA_String *capa = (UA_String *)capabilities.data;
	for (i = 0; i < capabilities.arrayLength; i++) {
		if (UA_String_equal(capa + i, newCapability)) {
			UA_Variant_clear(&capabilities);
			return UA_STATUSCODE_GOOD;  /* already set, nothing to do, no error */
		}
	}

	/* Add new capability */
	retval = UA_Array_resize(&capabilities.data, &capabilities.arrayLength, capabilities.arrayLength + 1,
	                         &UA_TYPES[UA_TYPES_STRING]);
	if (retval != UA_STATUSCODE_GOOD) {
		UA_Variant_clear(&capabilities);
		return retval;
	}

	retval = UA_String_copy(newCapability, ((UA_String *)capabilities.data) + capabilities.arrayLength - 1);
	if (retval != UA_STATUSCODE_GOOD) {
		UA_Variant_clear(&capabilities);
		return retval;
	}

	retval = UA_Server_writeValue(server,
	         UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_SERVERCAPABILITIES), capabilities);
	UA_Variant_clear(&capabilities);
	return retval;
}

/* Add a new private key format to the server configuration */
UA_StatusCode
UA_Server_configAddKeyFormat(UA_Server *server, const UA_String *newKeyFormat) {
    /* Valid private key formats are "PFX" and "PEM" */
	static char validServerConfigKeyFormats[][4] = {"PFX", "PEM"};

	/* Check for null arguments */
	if ((server == NULL) || (newKeyFormat == NULL) || UA_String_equal(newKeyFormat, &UA_STRING_NULL)) {
	    return UA_STATUSCODE_BADINVALIDARGUMENT;
	}

	/* Check for valid key format argument */
	UA_Boolean isValid = false;
	size_t i = 0;
	for (i = 0; i < (sizeof(validServerConfigKeyFormats)/sizeof(validServerConfigKeyFormats[0])); i++) {
	    UA_String str = UA_STRING(validServerConfigKeyFormats[i]);
	    if (UA_String_equal(newKeyFormat, &str)) {
	        isValid = true;
	        break;
	    }
	}
	if (!isValid) {
	    return UA_STATUSCODE_BADINVALIDARGUMENT;
	}

	/* Read key formats already set */
	UA_Variant keyFormats;
	UA_Variant_init(&keyFormats);
	UA_StatusCode retval = UA_Server_readValue(server,
			               UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_SUPPORTEDPRIVATEKEYFORMATS),
						   &keyFormats);
	if (retval != UA_STATUSCODE_GOOD) {
	    return retval;  /* reading key formats failed */
	}

	/* Clear incorrectly allocated array dimensions */
	UA_Array_delete(keyFormats.arrayDimensions, keyFormats.arrayDimensionsSize, &UA_TYPES[UA_TYPES_UINT32]);
	keyFormats.arrayDimensions = NULL;
    keyFormats.arrayDimensionsSize = 0;

    /* Check for duplicate key format */
    UA_String *keyformat = (UA_String *)keyFormats.data;
    for (i = 0; i < keyFormats.arrayLength; i++) {
        if (UA_String_equal(keyformat + i, newKeyFormat)) {
        	UA_Variant_clear(&keyFormats);
            return UA_STATUSCODE_GOOD;  /* already set, nothing to do, no error */
        }
    }

    /* Add new key format */
    if((retval = UA_Array_resize(&keyFormats.data, &keyFormats.arrayLength, keyFormats.arrayLength + 1,
                                 &UA_TYPES[UA_TYPES_STRING]))) {
    	UA_Variant_clear(&keyFormats);
        return retval;
    }
    if((retval = UA_String_copy(newKeyFormat, ((UA_String *)keyFormats.data) + keyFormats.arrayLength - 1))) {
    	UA_Variant_clear(&keyFormats);
        return retval;
    }
    retval = UA_Server_writeValue(server,
             UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_SUPPORTEDPRIVATEKEYFORMATS), keyFormats);

    UA_Variant_clear(&keyFormats);
    return retval;
}

/* Set max trust list size of server configuration */
UA_StatusCode
UA_Server_configSetMaxTrustListSize(UA_Server *server, UA_UInt32 size) {

    /* Check for null pointer argument */
    if (server == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Set the variable MaxTrustListSize of server configuration to size */
    UA_Variant sizeVar;
    UA_Variant_init(&sizeVar);
    UA_Variant_setScalar(&sizeVar, &size, &UA_TYPES[UA_TYPES_UINT32]);

    return UA_Server_writeValue(server,
           UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_MAXTRUSTLISTSIZE), sizeVar);
}

/* Create a Certificate Signing Request (CSR) */
UA_StatusCode
createSigningRequest(UA_Server *server,
                                const UA_NodeId *sessionId, void *sessionHandle,
                                const UA_NodeId *methodId, void *methodContext,
                                const UA_NodeId *objectId, void *objectContext,
                                size_t inputSize, const UA_Variant *input,
                                size_t outputSize, UA_Variant *output) {
	UA_ByteString *csr;

	/* Input arg 1: CertificateGroup object's certificateGroupId */
	UA_NodeId* certificateGroupId = (UA_NodeId *)input[0].data;

	/* Input arg 2: CertificateTypes Property's certificateTypeId, not yet implemented */
	UA_NodeId* certificateTypeId = (UA_NodeId *)input[1].data;

	/* Input arg 3: The subject name to use in the Certificate Request, can be NULL or empty */
	UA_String* subject = (UA_String *)input[2].data;

	/* Input arg 4: regenerateKey should be false (true not yet implemented) */
	/* TODO */

	/* Input arg 5: Nonce, additional entropy, can be NULL or empty, if set at least 32 bytes */
	UA_ByteString* entropy = (UA_ByteString *)input[4].data;
	if ((entropy != NULL) && (entropy->length != 0) && (entropy->length < 32)) {
	    return UA_STATUSCODE_BADINVALIDARGUMENT;
	}

	/* Find PKI Store */
	UA_PKIStore* pkiStore = UA_ServerConfig_PKIStore_get(server, certificateGroupId);
	if (pkiStore == NULL) {
		return UA_STATUSCODE_BADINVALIDARGUMENT;
	}

	/* Build the CSR */
	UA_CertificateManager* certificateManager = &server->config.certificateManager;
	if (certificateManager->createCertificateSigningRequest == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
	}

	UA_StatusCode ret = certificateManager->createCertificateSigningRequest(
		certificateManager, pkiStore, *certificateTypeId, subject, entropy, &csr
	);
	if (ret != UA_STATUSCODE_GOOD) {
	    return ret;
	}

	/* Output arg, the PKCS #10 DER encoded Certificate Request (CSR) */
	UA_Variant_setScalar(output, csr, &UA_TYPES[UA_TYPES_BYTESTRING]);
	return UA_STATUSCODE_GOOD;
}

/* UpdateCertificate method */
UA_StatusCode
updateCertificateMethod(UA_Server *server,
	const UA_NodeId *sessionId, void *sessionHandle,
    const UA_NodeId *methodId, void *methodContext,
    const UA_NodeId *objectId, void *objectContext,
    size_t inputSize, const UA_Variant *input,
    size_t outputSize, UA_Variant *output)
{
	/* Input arg 1: CertificateGroupId */
	UA_NodeId* certificateGroupId = (UA_NodeId *)input[0].data;

	/* Input arg 2: CertificateTypeId */
	UA_NodeId* certificateTypeId = (UA_NodeId *)input[1].data;

	/* Input arg 3: Certificate */
	UA_ByteString* certificate = (UA_ByteString *)input[2].data;

	/* Input arg 4: Issuer Certificates */
	/* Actual not implemented
	UA_ByteString* issuerCertificates = (UA_ByteString *)input[3].data;
	size_t issuerCertificatesLen = input[3].arrayLength; */

	/* Input arg 5: PrivateKeyFormat */
	/* TODO: Actual not implemented
	UA_String* privateKeyFormat = (UA_String*)input[4].data; */

	/* Input arg 6: PrivateKey */
	UA_ByteString* privateKey = (UA_ByteString *)input[5].data;

	/* Update certificate */
	return UA_Server_updateCertificate(server, certificateGroupId, certificateTypeId,
	    certificate, privateKey, true, true
	);
}


UA_StatusCode
UA_Server_getRejectedList(
	UA_Server *server,
	UA_ByteString **list,
	size_t *listSize,
	size_t listSizeMax
)
{
	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	size_t i, j, k = 0;

	/* Check parameter */
	if (server == NULL || list == NULL || listSize == NULL || listSizeMax == 0) {
		return UA_STATUSCODE_BADINVALIDARGUMENT;
	}

	/* Get server configuration */
	UA_ServerConfig *config = UA_Server_getConfig(server);
	if (!config) {
	    return UA_STATUSCODE_BADINTERNALERROR;
	}

	/* Get rejected certificates from all server pki stores */
	*list = (UA_ByteString*)UA_Array_new(listSizeMax, &UA_TYPES[UA_TYPES_STRING]);
	*listSize = 0;
	for (i = 0; i < config->pkiStoresSize; i++) {
		UA_ByteString *rejectedList = NULL;
		size_t rejectedListSize = 0;

		/* Get rejected list */
		UA_PKIStore* pkiStore = &config->pkiStores[i];
		retval = pkiStore->loadRejectedList(pkiStore, &rejectedList, &rejectedListSize);
		if (retval != UA_STATUSCODE_GOOD) {
			*listSize = 0;
			UA_Array_delete(*list, listSizeMax, &UA_TYPES[UA_TYPES_STRING]);
			return retval;
		}

		/* Check duplicate entries and add new entry to list*/
		for (j = 0; j < rejectedListSize; j++) {
		    for (k = 0; k < *listSize; k++) {
				if (UA_ByteString_equal(&rejectedList[j], &(*list)[k])) break;
			}

		    if (k == *listSize) {
		    	UA_ByteString_copy(&rejectedList[j], &(*list)[*listSize]);
		    	(*listSize)++;
		    }

		    if (*listSize >= listSizeMax) {
		    	UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_STRING]);
		    	return UA_STATUSCODE_GOOD;
		    }
		}
		UA_Array_delete(rejectedList, rejectedListSize, &UA_TYPES[UA_TYPES_STRING]);
	}

	size_t newSize = *listSize;
    *listSize = listSizeMax;
	retval = UA_Array_resize((void**)list, listSize, newSize, &UA_TYPES[UA_TYPES_STRING]);
	if (retval != UA_STATUSCODE_GOOD) return retval;
	return UA_STATUSCODE_GOOD;
}

/* GetRejectedList method */
UA_StatusCode
getRejectedListMethod(UA_Server *server,
	const UA_NodeId *sessionId, void *sessionHandle,
    const UA_NodeId *methodId, void *methodContext,
    const UA_NodeId *objectId, void *objectContext,
    size_t inputSize, const UA_Variant *input,
    size_t outputSize, UA_Variant *output)
{
	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	UA_ByteString* list = NULL;
	size_t listSize = 0;

	/* Get server configuration */
	UA_ServerConfig *config = UA_Server_getConfig(server);
	if (!config) {
	    return UA_STATUSCODE_BADINTERNALERROR;
	}
	size_t listSizeMax = config->rejectedListMethodMaxListSize;

	/* Get rejected certificates from certificate store */
	retval = UA_Server_getRejectedList(server, &list, &listSize, listSizeMax);
	if (retval != UA_STATUSCODE_GOOD) {
		return retval;
	}

	UA_Variant_setArray(output, list, listSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
	return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_ENCRYPTION
/* Setup the Certificate Manager */
UA_EXPORT UA_StatusCode
UA_ServerConfig_setupCertificateManager(UA_Server *server) {
    if (server == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_ServerConfig *config = UA_Server_getConfig(server);
    if (config == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if (config->pkiStoresSize < 1 || config->pkiStores == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    return UA_CertificateManager_create(
    		&(config->certificateManager)
	);
}
#endif


