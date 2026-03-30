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
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/client.h>
#include <open62541/client_highlevel_async.h>

#include "ua_discovery.h"

#ifdef UA_ENABLE_DISCOVERY

/**************************/
/* Client-Based Discovery */
/**************************/

/* The DiscoveryManager holds the state of clients that register at a
 * DiscoveryServer */

void
UA_DiscoveryManager_setState(UA_DiscoveryManager *dm,
                             UA_LifecycleState state) {
    /* Check if open connections remain */
    if(state == UA_LIFECYCLESTATE_STOPPING ||
       state == UA_LIFECYCLESTATE_STOPPED) {
        state = UA_LIFECYCLESTATE_STOPPED;
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
}

static UA_StatusCode
UA_DiscoveryManager_free(struct UA_ServerComponent *sc) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)sc;

    if(sc->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(sc->server->config.logging, UA_LOGCATEGORY_SERVER,
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

    UA_free(sc);

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
                            "Registration of server with URI %S is removed because "
                            "the semaphore file '%S' was deleted",
                            current->registeredServer.serverUri,
                            current->registeredServer.semaphoreFilePath);
            } else {
                // cppcheck-suppress unreadVariable
                UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                            "Registration of server with URI %S has timed out "
                            "and is removed", current->registeredServer.serverUri);
            }
            LIST_REMOVE(current, pointers);
            UA_RegisteredServer_clear(&current->registeredServer);
            UA_free(current);
            dm->registeredServersSize--;
        }
    }
}

static UA_StatusCode
UA_DiscoveryManager_start(struct UA_ServerComponent *sc) {
    /* Check that the server backpointer is set */
    UA_Server *server = sc->server;
    if(!server)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Cannot start an already started DiscoveryManager */
    if(sc->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)sc;

    UA_StatusCode res =
        addRepeatedCallback(server, UA_DiscoveryManager_cleanupTimedOut,
                            dm, 1000.0, &dm->discoveryCallbackId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_DiscoveryManager_setState(dm, UA_LIFECYCLESTATE_STARTED);
    return UA_STATUSCODE_GOOD;
}

static void
UA_DiscoveryManager_stop(struct UA_ServerComponent *sc) {
    if(sc->state != UA_LIFECYCLESTATE_STARTED)
        return;

    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)sc;
    removeCallback(dm->sc.server, dm->discoveryCallbackId);

    /* Cancel all outstanding register requests */
    for(size_t i = 0; i < UA_MAXREGISTERREQUESTS; i++) {
        if(dm->registerRequests[i].client == NULL)
            continue;
        UA_Client_disconnectSecureChannelAsync(dm->registerRequests[i].client);
    }

    UA_DiscoveryManager_setState(dm, UA_LIFECYCLESTATE_STOPPED);
}

UA_ServerComponent *
UA_DiscoveryManager_new(void) {
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)
        UA_calloc(1, sizeof(UA_DiscoveryManager));
    if(!dm)
        return NULL;

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
asyncRegisterRequest_clear(void *_, void *context) {
    asyncRegisterRequest *ar = (asyncRegisterRequest*)context;
    UA_DiscoveryManager *dm = ar->dm;

    UA_String_clear(&ar->semaphoreFilePath);
    if(ar->client)
        UA_Client_delete(ar->client);
    memset(ar, 0, sizeof(asyncRegisterRequest));

    /* The Discovery manager is fully stopped? */
    UA_DiscoveryManager_setState(dm, dm->sc.state);
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
    UA_ServerConfig *sc = &ar->dm->sc.server->config;

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
    const UA_ServerConfig *sc = &ar->dm->sc.server->config;
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
    UA_ServerConfig *sc = &ar->dm->sc.server->config;

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
                UA_Client_connectAsync(client, NULL);   /* Reconnect */
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
#ifdef UA_ENABLE_ENCRYPTION
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    if(cc->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT &&
       msm != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return;
#endif

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    UA_ExtensionObject mdnsConfig;
#endif
    const UA_DataType *reqType;
    const UA_DataType *respType;
    UA_RegisterServerRequest reg1;
    UA_RegisterServer2Request reg2;
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
    UA_DiscoveryManager *dm = (UA_DiscoveryManager*)server->discoverySC;
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

    /* If it's not already set, use encryption by default */
    if(cc->securityMode == UA_MESSAGESECURITYMODE_INVALID) {
#ifdef UA_ENABLE_ENCRYPTION
        cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
#else
        cc->securityMode = UA_MESSAGESECURITYMODE_NONE;
#endif
    }

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
    return UA_Client_connectAsync(ar->client, NULL);
}

UA_StatusCode
UA_Server_registerDiscovery(UA_Server *server, UA_ClientConfig *cc,
                            const UA_String discoveryServerUrl,
                            const UA_String semaphoreFilePath) {
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "Registering at the DiscoveryServer: %S", discoveryServerUrl);
    lockServer(server);
    UA_StatusCode res =
        UA_Server_register(server, cc, false, discoveryServerUrl, semaphoreFilePath);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_deregisterDiscovery(UA_Server *server, UA_ClientConfig *cc,
                              const UA_String discoveryServerUrl) {
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "Deregistering at the DiscoveryServer: %S", discoveryServerUrl);
    lockServer(server);
    UA_StatusCode res =
        UA_Server_register(server, cc, true, discoveryServerUrl, UA_STRING_NULL);
    unlockServer(server);
    return res;
}

#ifdef UA_ENABLE_DISCOVERY_MULTICAST

/***************************/
/* Multicast DNS Discovery */
/***************************/

/* API for the mDNS ServerComponent plugins */

static void
notifyServerOnNetwork(UA_Server *server,
                      const UA_ServerOnNetwork *serverOnNetwork,
                      UA_Boolean added, UA_Boolean removed,
                      UA_Boolean updated) {
    if(!server->config.discoveryNotificationCallback &&
       !server->config.globalNotificationCallback)
        return;

    static UA_KeyValuePair kvp[4] = {
        {{0, UA_STRING_STATIC("server-on-network")}, {0}},
        {{0, UA_STRING_STATIC("server-added")}, {0}},
        {{0, UA_STRING_STATIC("server-removed")}, {0}},
        {{0, UA_STRING_STATIC("server-updated")}, {0}},
    };
    UA_Variant_setScalar(&kvp[0].value, (void*)(uintptr_t)serverOnNetwork,
                         &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    UA_Variant_setScalar(&kvp[1].value, &added, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&kvp[2].value, &removed, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&kvp[3].value, &updated, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {4, kvp};

    if(server->config.discoveryNotificationCallback)
        server->config.discoveryNotificationCallback(server,
            UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY_SERVERONNETWORK, kvm);
    if(server->config.globalNotificationCallback)
        server->config.globalNotificationCallback(server,
            UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY_SERVERONNETWORK, kvm);
}

static void
resetDiscoveryResetIds(UA_Server *server) {
    /* Reset the counter */
    UA_EventLoop *el = server->config.eventLoop;
    server->lastCounterResetTime = el->dateTime_now(el);
    server->serversOnNetworkRecordCounter = 1;

    /* Set RecordIds in increasing order */
    for(size_t i = 0; i < server->serversOnNetworkSize; i++) {
        UA_ServerOnNetwork *son = &server->serversOnNetwork[i];
        son->recordId = server->serversOnNetworkRecordCounter++;
    }
}

static void
announceServerOnNetwork(UA_Server *server,
                        const UA_ServerOnNetwork *son,
                        const UA_KeyValueMap *params) {
    /* Was announcing requested? */
    if(!params)
        return;
    const UA_Boolean *announce = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "announce"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(!announce || !(*announce))
        return;

    /* Find the MdnsServerComponent and trigger the announcement */
    for(UA_ServerComponent *sc = server->components; sc; sc = sc->next) {
        if(sc->serverComponentType != UA_SERVERCOMPONENTTYPE_MDNS)
            continue;
        UA_MdnsServerComponent *msc = (UA_MdnsServerComponent*)sc;
        msc->announce(msc, son, params);
    }
}

UA_StatusCode
UA_Server_registerServerOnNetwork(UA_Server *server,
                                  const UA_ServerOnNetwork *newSon,
                                  const UA_KeyValueMap *params) {
    if(!newSon)
        return UA_STATUSCODE_BADINTERNALERROR;

    lockServer(server);

    /* Does the entry already exist? Then update it. */
    UA_StatusCode res;
    UA_ServerOnNetwork *son;
    for(size_t i = 0; i < server->serversOnNetworkSize; i++) {
        /* Check matching record */
        son = &server->serversOnNetwork[i];
        if(!UA_String_equal(&newSon->serverName, &son->serverName))
            continue;

        /* Check if the entry is identical (modulo the RecordId we set
         * locally) */
        UA_ServerOnNetwork tmp = *newSon;
        tmp.recordId = son->recordId;
        UA_Order match =
            UA_order(&tmp, son, &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
        if(match == UA_ORDER_EQ) {
            unlockServer(server);
            return UA_STATUSCODE_GOOD;
        }

        /* Make a copy */
        res = UA_ServerOnNetwork_copy(newSon, &tmp);
        if(res != UA_STATUSCODE_GOOD) {
            unlockServer(server);
            return res;
        }

        /* Replace the previous entry */
        UA_ServerOnNetwork_clear(son);
        *son = tmp;

        /* Notify the application about an updated entry */
        notifyServerOnNetwork(server, son, false, false, true);

        /* All new RecordIds in increasing order */
        resetDiscoveryResetIds(server);

        /* If requested, send out an mDNS announcement */
        announceServerOnNetwork(server, son, params);

        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }

    /* Append to the list */
    res = UA_Array_appendCopy((void**)&server->serversOnNetwork,
                              &server->serversOnNetworkSize, newSon,
                              &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    /* Notify the application about an added entry */
    notifyServerOnNetwork(server, son, true, false, false);

    /* Increase the record id counter and use it */
    son = &server->serversOnNetwork[server->serversOnNetworkSize-1];
    son->recordId = server->serversOnNetworkRecordCounter++;

    /* If requested, send out an mDNS announcement */
    announceServerOnNetwork(server, son, params);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

static void
retractServerOnNetwork(UA_Server *server,
                       const UA_ServerOnNetwork *son,
                       const UA_KeyValueMap *params) {
    /* Was a retraction requested? */
    if(!params)
        return;
    const UA_Boolean *retract = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "retract"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(!retract || !(*retract))
        return;

    /* Find the MdnsServerComponent and trigger the retraction */
    for(UA_ServerComponent *sc = server->components; sc; sc = sc->next) {
        if(sc->serverComponentType != UA_SERVERCOMPONENTTYPE_MDNS)
            continue;
        UA_MdnsServerComponent *msc = (UA_MdnsServerComponent*)sc;
        msc->retract(msc, son->serverName, son->discoveryUrl, params);
    }
}

UA_StatusCode
UA_Server_deregisterServerOnNetwork(UA_Server *server,
                                    const UA_String serverName,
                                    const UA_KeyValueMap *params) {
    lockServer(server);

    /* Update the reset time */
    UA_EventLoop *el = server->config.eventLoop;
    server->lastCounterResetTime = el->dateTime_now(el);

    /* Find the entry */
    for(size_t i = 0; i < server->serversOnNetworkSize; i++) {
        UA_ServerOnNetwork *son = &server->serversOnNetwork[i];
        if(!UA_String_equal(&serverName, &son->serverName))
            continue;

        /* Notify the application about removed entry */
        notifyServerOnNetwork(server, son, false, true, false);

        /* If requested, send out an mDNS retraction */
        retractServerOnNetwork(server, son, params);

        /* Move the last entry in the current position */
        UA_ServerOnNetwork_clear(son);
        server->serversOnNetworkSize--;
        if(i != server->serversOnNetworkSize) {
            server->serversOnNetwork[i] =
                server->serversOnNetwork[server->serversOnNetworkSize];
        }

        /* Free the array if the last entry was removed */
        if(server->serversOnNetworkSize == 0) {
            UA_free(server->serversOnNetwork);
            server->serversOnNetwork = NULL;
        }

        /* All new RecordIds in increasing order */
        resetDiscoveryResetIds(server);
        break;
    }

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_DISCOVERY_MULTICAST */

#endif /* UA_ENABLE_DISCOVERY */
