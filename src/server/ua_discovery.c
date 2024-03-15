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
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include <open62541/client.h>
#include "ua_discovery.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_DISCOVERY

void
UA_DiscoveryManager_setState(UA_Server *server,
                             UA_DiscoveryManager *dm,
                             UA_LifecycleState state) {
    /* Check if open connections remain */
    if(state == UA_LIFECYCLESTATE_STOPPING ||
       state == UA_LIFECYCLESTATE_STOPPED) {
        state = UA_LIFECYCLESTATE_STOPPED;
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
        if(dm->mdnsRecvConnectionsSize != 0 || dm->mdnsSendConnection != 0)
            state = UA_LIFECYCLESTATE_STOPPING;
#endif

        for(size_t i = 0; i < UA_MAXREGISTERREQUESTS; i++) {
            if(dm->registerRequests[i].client != NULL)
                state = UA_LIFECYCLESTATE_STOPPING;
        }
    }

    /* No change */
    if(state == dm->sc.state)
        return;

    /* Set the new state and notify */
    dm->sc.state = state;
    if(dm->sc.notifyState)
        dm->sc.notifyState(server, &dm->sc, state);
}

static UA_StatusCode
UA_DiscoveryManager_free(UA_Server *server,
                         struct UA_ServerComponent *sc) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)sc;

    if(sc->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Cannot delete the DiscoveryManager because "
                     "it is not stopped");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    registeredServer *rs, *rs_tmp;
    LIST_FOREACH_SAFE(rs, &dm->registeredServers, pointers, rs_tmp) {
        LIST_REMOVE(rs, pointers);
        UA_RegisteredServer_clear(&rs->registeredServer);
        UA_free(rs);
    }

# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    serverOnNetwork *son, *son_tmp;
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
UA_DiscoveryManager_cleanupTimedOut(UA_Server *server, void *data) {
    UA_EventLoop *el = server->config.eventLoop;
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)data;

    /* TimedOut gives the last DateTime at which we must have seen the
     * registered server. Otherwise it is timed out. */
    UA_DateTime timedOut = el->dateTime_nowMonotonic(el);
    if(server->config.discoveryCleanupTimeout)
        timedOut -= server->config.discoveryCleanupTimeout * UA_DATETIME_SEC;

    registeredServer *current, *temp;
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
                UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                             "Cannot check registration semaphore. Out of memory");
            }
        }
#endif

        if(semaphoreDeleted ||
           (server->config.discoveryCleanupTimeout &&
            current->lastSeen < timedOut)) {
            if(semaphoreDeleted) {
                UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                            "Registration of server with URI %.*s is removed because "
                            "the semaphore file '%.*s' was deleted",
                            (int)current->registeredServer.serverUri.length,
                            current->registeredServer.serverUri.data,
                            (int)current->registeredServer.semaphoreFilePath.length,
                            current->registeredServer.semaphoreFilePath.data);
            } else {
                // cppcheck-suppress unreadVariable
                UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
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

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    /* Send out multicast */
    UA_DiscoveryManager_sendMulticastMessages(dm);
#endif
}

static UA_StatusCode
UA_DiscoveryManager_start(UA_Server *server,
                          struct UA_ServerComponent *sc) {
    if(sc->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)sc;
    dm->server = server; /* Set the backpointer */

    UA_StatusCode res =
        addRepeatedCallback(server, UA_DiscoveryManager_cleanupTimedOut,
                            dm, 1000.0, &dm->discoveryCallbackId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.mdnsEnabled)
        UA_DiscoveryManager_startMulticast(dm);
#endif

    UA_DiscoveryManager_setState(server, dm, UA_LIFECYCLESTATE_STARTED);
    return UA_STATUSCODE_GOOD;
}

static void
UA_DiscoveryManager_stop(UA_Server *server,
                         struct UA_ServerComponent *sc) {
    if(sc->state != UA_LIFECYCLESTATE_STARTED)
        return;

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)sc;
    removeCallback(server, dm->discoveryCallbackId);

    /* Cancel all outstanding register requests */
    for(size_t i = 0; i < UA_MAXREGISTERREQUESTS; i++) {
        if(dm->registerRequests[i].client == NULL)
            continue;
        UA_Client_disconnectSecureChannelAsync(dm->registerRequests[i].client);
    }

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.mdnsEnabled)
        UA_DiscoveryManager_stopMulticast(dm);
#endif

    UA_DiscoveryManager_setState(server, dm, UA_LIFECYCLESTATE_STOPPED);
}

UA_ServerComponent *
UA_DiscoveryManager_new(UA_Server *server) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        UA_calloc(1, sizeof(UA_DiscoveryManager));
    if(!dm)
        return NULL;

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_EventLoop *el = server->config.eventLoop;
    dm->serverOnNetworkRecordIdLastReset = el->dateTime_now(el);
#endif /* UA_ENABLE_DISCOVERY_MULTICAST */

    dm->sc.name = UA_STRING("discovery");
    dm->sc.start = UA_DiscoveryManager_start;
    dm->sc.stop = UA_DiscoveryManager_stop;
    dm->sc.free = UA_DiscoveryManager_free;
    return &dm->sc;
}

/********************************/
/* Register at Discovery Server */
/********************************/

static void
asyncRegisterRequest_clear(void *app, void *context) {
    UA_Server *server = (UA_Server*)app;
    asyncRegisterRequest *ar = (asyncRegisterRequest*)context;
    UA_DiscoveryManager *dm = ar->dm;

    UA_String_clear(&ar->semaphoreFilePath);
    if(ar->client)
        UA_Client_delete(ar->client);
    memset(ar, 0, sizeof(asyncRegisterRequest));

    /* The Discovery manager is fully stopped? */
    UA_DiscoveryManager_setState(server, dm, dm->sc.state);
}

static void
asyncRegisterRequest_clearAsync(asyncRegisterRequest *ar) {
    UA_Server *server = ar->server;
    UA_ServerConfig *sc = &server->config;
    UA_EventLoop *el = sc->eventLoop;

    ar->cleanupCallback.callback = asyncRegisterRequest_clear;
    ar->cleanupCallback.application = server;
    ar->cleanupCallback.context = ar;
    el->addDelayedCallback(el, &ar->cleanupCallback);
}

static void
setupRegisterRequest(asyncRegisterRequest *ar, UA_RequestHeader *rh,
                     UA_RegisteredServer *rs) {
    UA_ServerConfig *sc = &ar->dm->server->config;

    rh->timeoutHint = 10000;

    rs->isOnline = !ar->unregister;
    rs->serverUri = sc->applicationDescription.applicationUri;
    rs->productUri = sc->applicationDescription.productUri;
    rs->serverType = sc->applicationDescription.applicationType;
    rs->gatewayServerUri = sc->applicationDescription.gatewayServerUri;
    rs->semaphoreFilePath = ar->semaphoreFilePath;

    rs->serverNames = &sc->applicationDescription.applicationName;
    rs->serverNamesSize = 1;

    /* Mirror the discovery URLs from the server config (includes hostnames from
     * the network layers) */
    rs->discoveryUrls = sc->applicationDescription.discoveryUrls;
    rs->discoveryUrlsSize = sc->applicationDescription.discoveryUrlsSize;
}

static void
registerAsyncResponse(UA_Client *client, void *userdata,
                      UA_UInt32 requestId, void *resp) {
    asyncRegisterRequest *ar = (asyncRegisterRequest*)userdata;
    const UA_ServerConfig *sc = &ar->dm->server->config;
    UA_Response *response = (UA_Response*)resp;
    const char *regtype = (ar->register2) ? "RegisterServer2" : "RegisterServer";

    /* Success registering? */
    if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(sc->logging, UA_LOGCATEGORY_SERVER, "%s succeeded", regtype);
        goto done;
    }

    UA_LOG_WARNING(sc->logging, UA_LOGCATEGORY_SERVER,
                   "%s failed with statuscode %s", regtype,
                   UA_StatusCode_name(response->responseHeader.serviceResult));

    /* Try RegisterServer next */
    ar->register2 = false;

    /* Try RegisterServer immediately if we can.
     * Otherwise wait for the next state callback. */
    UA_SecureChannelState ss;
    UA_Client_getState(client, &ss, NULL, NULL);
    if(!ar->shutdown && ss == UA_SECURECHANNELSTATE_OPEN) {
        UA_RegisterServerRequest request;
        UA_RegisterServerRequest_init(&request);
        setupRegisterRequest(ar, &request.requestHeader, &request.server);
        UA_StatusCode res =
            __UA_Client_AsyncService(client, &request,
                                     &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST],
                                     registerAsyncResponse,
                                     &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE], ar, NULL);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR((const UA_Logger *)&sc->logging, UA_LOGCATEGORY_CLIENT,
                         "RegisterServer failed with statuscode %s",
                         UA_StatusCode_name(res));
            goto done;
        }
    }

    return;

 done:
    /* Close the client connection, will be cleaned up in the client state
     * callback when closing is complete */
    ar->shutdown = true;
    UA_Client_disconnectSecureChannelAsync(ar->client);
}

static void
discoveryClientStateCallback(UA_Client *client,
                             UA_SecureChannelState channelState,
                             UA_SessionState sessionState,
                             UA_StatusCode connectStatus) {
    asyncRegisterRequest *ar = (asyncRegisterRequest*)
        UA_Client_getContext(client);
    UA_ServerConfig *sc = &ar->dm->server->config;

    /* Connection failed */
    if(connectStatus != UA_STATUSCODE_GOOD) {
        if(connectStatus != UA_STATUSCODE_BADCONNECTIONCLOSED) {
            UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                         "Could not connect to the Discovery server with error %s",
                         UA_StatusCode_name(connectStatus));
        }

        /* Connection fully closed */
        if(channelState == UA_SECURECHANNELSTATE_CLOSED) {
            if(!ar->connectSuccess || ar->shutdown) {
                asyncRegisterRequest_clearAsync(ar); /* Clean up */
            } else {
                ar->connectSuccess = false;
                __UA_Client_connect(client, true);   /* Reconnect */
            }
        }
        return;
    }

    /* Wait until the SecureChannel is open */
    if(channelState != UA_SECURECHANNELSTATE_OPEN)
        return;

    /* We have at least succeeded to connect */
    ar->connectSuccess = true;

    /* Is this the encrypted SecureChannel already? (We might have to wait for
     * the second connection after the FindServers handshake */
    UA_MessageSecurityMode msm = UA_MESSAGESECURITYMODE_INVALID;
    UA_Client_getConnectionAttribute_scalar(client, UA_QUALIFIEDNAME(0, "securityMode"),
                                            &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE],
                                            &msm);
    if(msm != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return;

    const UA_DataType *reqType;
    const UA_DataType *respType;
    UA_RegisterServerRequest reg1;
    UA_RegisterServer2Request reg2;
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_ExtensionObject mdnsConfig;
#endif
    void *request;

    /* Prepare the request. This does not allocate memory */
    if(ar->register2) {
        UA_RegisterServer2Request_init(&reg2);
        setupRegisterRequest(ar, &reg2.requestHeader, &reg2.server);
        reqType = &UA_TYPES[UA_TYPES_REGISTERSERVER2REQUEST];
        respType = &UA_TYPES[UA_TYPES_REGISTERSERVER2RESPONSE];
        request = &reg2;

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
        /* Set the configuration that is only available for
         * UA_RegisterServer2Request */
        UA_ExtensionObject_setValueNoDelete(&mdnsConfig, &sc->mdnsConfig,
                                            &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION]);
        reg2.discoveryConfigurationSize = 1;
        reg2.discoveryConfiguration = &mdnsConfig;
#endif
    } else {
        UA_RegisterServerRequest_init(&reg1);
        setupRegisterRequest(ar, &reg1.requestHeader, &reg1.server);
        reqType = &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST];
        respType = &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE];
        request = &reg1;
    }

    /* Try to call RegisterServer2 */
    UA_StatusCode res =
        __UA_Client_AsyncService(client, request, reqType, registerAsyncResponse,
                                 respType, ar, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        /* Close the client connection, will be cleaned up in the client state
         * callback when closing is complete */
        UA_Client_disconnectSecureChannelAsync(ar->client);
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_CLIENT,
                     "RegisterServer2 failed with statuscode %s",
                     UA_StatusCode_name(res));
    }
}

static UA_StatusCode
UA_Server_register(UA_Server *server, UA_ClientConfig *cc, UA_Boolean unregister,
                   const UA_String discoveryServerUrl,
                   const UA_String semaphoreFilePath) {
    /* Get the discovery manager */
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        getServerComponentByName(server, UA_STRING("discovery"));
    if(!dm) {
        UA_ClientConfig_clear(cc);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check that the discovery manager is running */
    UA_ServerConfig *sc = &server->config;
    if(dm->sc.state != UA_LIFECYCLESTATE_STARTED) {
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "The server must be started for registering");
        UA_ClientConfig_clear(cc);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Find a free slot for storing the async request information */
    asyncRegisterRequest *ar = NULL;
    for(size_t i = 0; i < UA_MAXREGISTERREQUESTS; i++) {
        if(dm->registerRequests[i].client == NULL) {
            ar = &dm->registerRequests[i];
            break;
        }
    }
    if(!ar) {
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "Too many outstanding register requests. Cannot proceed.");
        UA_ClientConfig_clear(cc);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Use the EventLoop from the server for the client */
    if(cc->eventLoop && !cc->externalEventLoop)
        cc->eventLoop->free(cc->eventLoop);
    cc->eventLoop = sc->eventLoop;
    cc->externalEventLoop = true;

    /* Set the state callback method and context */
    cc->stateCallback = discoveryClientStateCallback;
    cc->clientContext = ar;

    /* Use encryption by default */
#ifdef UA_ENABLE_ENCRYPTION
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
#endif

    /* Open only a SecureChannel */
    cc->noSession = true;

    /* Move the endpoint url */
    UA_String_clear(&cc->endpointUrl);
    UA_String_copy(&discoveryServerUrl, &cc->endpointUrl);

    /* Instantiate the client */
    ar->client = UA_Client_newWithConfig(cc);
    if(!ar->client) {
        UA_ClientConfig_clear(cc);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Zero out the supplied config */
    memset(cc, 0, sizeof(UA_ClientConfig));

    /* Finish setting up the context */
    ar->server = server;
    ar->dm = dm;
    ar->unregister = unregister;
    ar->register2 = true; /* Try register2 first */
    UA_String_copy(&semaphoreFilePath, &ar->semaphoreFilePath);

    /* Connect asynchronously. The register service is called once the
     * connection is open. */
    ar->connectSuccess = false;
    return __UA_Client_connect(ar->client, true);
}

UA_StatusCode
UA_Server_registerDiscovery(UA_Server *server, UA_ClientConfig *cc,
                            const UA_String discoveryServerUrl,
                            const UA_String semaphoreFilePath) {
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "Registering at the DiscoveryServer: %.*s",
                (int)discoveryServerUrl.length, discoveryServerUrl.data);
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res =
        UA_Server_register(server, cc, false, discoveryServerUrl, semaphoreFilePath);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_deregisterDiscovery(UA_Server *server, UA_ClientConfig *cc,
                              const UA_String discoveryServerUrl) {
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "Deregistering at the DiscoveryServer: %.*s",
                (int)discoveryServerUrl.length, discoveryServerUrl.data);
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res =
        UA_Server_register(server, cc, true, discoveryServerUrl, UA_STRING_NULL);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif /* UA_ENABLE_DISCOVERY */
