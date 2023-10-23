/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include <open62541/client.h>

#include "ua_discovery_manager.h"

#ifdef UA_ENABLE_DISCOVERY

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
registerAsyncResponse(UA_Client *client, void *userdata,
                      UA_UInt32 requestId, void *resp) {
    asyncRegisterRequest *ar = (asyncRegisterRequest*)userdata;
    const UA_ServerConfig *sc = ar->dm->serverConfig;
    UA_RegisterServerResponse *response = (UA_RegisterServerResponse*)resp;
    if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&sc->logger, UA_LOGCATEGORY_SERVER,
                    "RegisterServer succeeded");
    } else {
        UA_LOG_WARNING(&sc->logger, UA_LOGCATEGORY_SERVER,
                       "RegisterServer failed with statuscode %s",
                       UA_StatusCode_name(response->responseHeader.serviceResult));
    }

    /* Close the client connection, will be cleaned up in the client state
     * callback when closing is complete */
    UA_Client_disconnectSecureChannelAsync(ar->client);
}

static void
setupRegisterRequest(asyncRegisterRequest *ar, UA_RequestHeader *rh,
                     UA_RegisteredServer *rs) {
    UA_ServerConfig *sc = ar->dm->serverConfig;

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
register2AsyncResponse(UA_Client *client, void *userdata,
                       UA_UInt32 requestId, void *resp) {
    asyncRegisterRequest *ar = (asyncRegisterRequest*)userdata;
    const UA_ServerConfig *sc = ar->dm->serverConfig;
    UA_RegisterServer2Response *response = (UA_RegisterServer2Response*)resp;

    /* Success */
    UA_StatusCode serviceResult = response->responseHeader.serviceResult;
    if(serviceResult == UA_STATUSCODE_GOOD) {
        /* Close the client connection, will be cleaned up in the client state
         * callback when closing is complete */
        UA_Client_disconnectSecureChannelAsync(ar->client);
        UA_LOG_INFO(&sc->logger, UA_LOGCATEGORY_SERVER,
                    "RegisterServer succeeded");
        return;
    }

    /* Unrecoverable error */
    if(serviceResult != UA_STATUSCODE_BADNOTIMPLEMENTED &&
       serviceResult != UA_STATUSCODE_BADSERVICEUNSUPPORTED) {
        /* Close the client connection, will be cleaned up in the client state
         * callback when closing is complete */
        UA_Client_disconnectSecureChannelAsync(ar->client);
        UA_LOG_WARNING(&sc->logger, UA_LOGCATEGORY_SERVER,
                       "RegisterServer failed with error %s",
                       UA_StatusCode_name(serviceResult));
        return;
    }

    /* Try RegisterServer */
    UA_RegisterServerRequest request;
    UA_RegisterServerRequest_init(&request);
    setupRegisterRequest(ar, &request.requestHeader, &request.server);
    UA_StatusCode res =
        __UA_Client_AsyncService(client, &request,
                                 &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST],
                                 registerAsyncResponse,
                                 &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE],
                                 ar, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        /* Close the client connection, will be cleaned up in the client state
         * callback when closing is complete */
        UA_Client_disconnectSecureChannelAsync(ar->client);
        UA_LOG_ERROR(&sc->logger, UA_LOGCATEGORY_CLIENT,
                     "RegisterServer2 failed with statuscode %s",
                     UA_StatusCode_name(res));
    }
}

static void
discoveryClientStateCallback(UA_Client *client,
                             UA_SecureChannelState channelState,
                             UA_SessionState sessionState,
                             UA_StatusCode connectStatus) {
    asyncRegisterRequest *ar = (asyncRegisterRequest*)
        UA_Client_getContext(client);
    UA_ServerConfig *sc = ar->dm->serverConfig;

    /* Connection failed */
    if(connectStatus != UA_STATUSCODE_GOOD) {
        if(connectStatus != UA_STATUSCODE_BADCONNECTIONCLOSED) {
            UA_LOG_ERROR(&sc->logger, UA_LOGCATEGORY_SERVER,
                         "Could not connect to the Discovery server with error %s",
                         UA_StatusCode_name(connectStatus));
        }
        /* If fully closed, delete the client and clean up */
        if(channelState == UA_SECURECHANNELSTATE_CLOSED)
            asyncRegisterRequest_clearAsync(ar);
        return;
    }

    /* Wait until the SecureChannel is open */
    if(channelState != UA_SECURECHANNELSTATE_OPEN)
        return;

    /* Prepare the request. This does not allocate memory */
    UA_RegisterServer2Request request;
    UA_RegisterServer2Request_init(&request);
    setupRegisterRequest(ar, &request.requestHeader, &request.server);

    /* Set the configuration that is only available for UA_RegisterServer2Request */
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_ExtensionObject mdnsConfig;
    UA_ExtensionObject_setValueNoDelete(ar->request.discoveryConfiguration,
                                        &sc->mdnsConfig,
                                        &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION]);
    ar->request.discoveryConfigurationSize = 1;
    ar->request.discoveryConfiguration = &mdnsConfig;
#endif

    /* Try to call RegisterServer2 */
    UA_StatusCode res =
        __UA_Client_AsyncService(client, &request,
                                 &UA_TYPES[UA_TYPES_REGISTERSERVER2REQUEST],
                                 register2AsyncResponse,
                                 &UA_TYPES[UA_TYPES_REGISTERSERVER2RESPONSE],
                                 ar, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        /* Close the client connection, will be cleaned up in the client state
         * callback when closing is complete */
        UA_Client_disconnectSecureChannelAsync(ar->client);
        UA_LOG_ERROR(&sc->logger, UA_LOGCATEGORY_CLIENT,
                     "RegisterServer2 failed with statuscode %s",
                     UA_StatusCode_name(res));
    }
}

static UA_StatusCode
UA_Server_register(UA_Server *server, UA_ClientConfig *cc, UA_Boolean unregister,
                   const UA_String discoveryServerUrl,
                   const UA_String  semaphoreFilePath) {
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
        UA_LOG_ERROR(&sc->logger, UA_LOGCATEGORY_SERVER,
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
        UA_LOG_ERROR(&sc->logger, UA_LOGCATEGORY_SERVER,
                     "Too many outstanding register requests. Cannot proceed.");
        UA_ClientConfig_clear(cc);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Use the EventLoop from the server for the client */
    if(cc->eventLoop && !cc->externalEventLoop)
        cc->eventLoop->free(cc->eventLoop);
    cc->eventLoop = sc->eventLoop;
    cc->externalEventLoop = true;

    /* Use the logging from the server */
    if(cc->logging->clear)
        cc->logging->clear(&cc->logging);
    cc->logging = sc->logging;

    /* Set the state callback method and context */
    cc->stateCallback = discoveryClientStateCallback;
    cc->clientContext = ar;

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
    UA_String_copy(&semaphoreFilePath, &ar->semaphoreFilePath);

    /* Connect asynchronously. The register service is called once the
     * connection is open. */
    return __UA_Client_connect(ar->client, true);
}

UA_StatusCode
UA_Server_registerDiscovery(UA_Server *server, UA_ClientConfig *cc,
                            const UA_String discoveryServerUrl,
                            const UA_String semaphoreFilePath) {
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Deregistering at the DiscoveryServer: %.*s",
                (int)discoveryServerUrl.length, discoveryServerUrl.data);
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res =
        UA_Server_register(server, cc, true, discoveryServerUrl, UA_STRING_NULL);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif /* UA_ENABLE_DISCOVERY */
