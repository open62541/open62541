/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/client.h>

#include "ua_server_internal.h"

#ifdef UA_ENABLE_DISCOVERY

static UA_StatusCode
register_server_with_discovery_server(UA_Server *server,
                                      UA_Client *client,
                                      const UA_Boolean isUnregister,
                                      const char* semaphoreFilePath) {
    /* Prepare the request. Do not cleanup the request after the service call,
     * as the members are stack-allocated or point into the server config. */
    UA_RegisterServer2Request request;
    UA_RegisterServer2Request_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;

    request.server.isOnline = !isUnregister;
    request.server.serverUri = server->config.applicationDescription.applicationUri;
    request.server.productUri = server->config.applicationDescription.productUri;
    request.server.serverType = server->config.applicationDescription.applicationType;
    request.server.gatewayServerUri = server->config.applicationDescription.gatewayServerUri;

    if(semaphoreFilePath) {
#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
        request.server.semaphoreFilePath =
            UA_STRING((char*)(uintptr_t)semaphoreFilePath); /* dirty cast */
#else
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Ignoring semaphore file path. open62541 not compiled "
                       "with UA_ENABLE_DISCOVERY_SEMAPHORE=ON");
#endif
    }

    request.server.serverNames = &server->config.applicationDescription.applicationName;
    request.server.serverNamesSize = 1;

    /* Copy the discovery urls from the server config and the network layers*/
    size_t config_discurls = server->config.applicationDescription.discoveryUrlsSize;
    size_t nl_discurls = server->config.networkLayersSize;
    size_t total_discurls = config_discurls + nl_discurls;
    UA_STACKARRAY(UA_String, urlsBuf, total_discurls);
    request.server.discoveryUrls = urlsBuf;
    request.server.discoveryUrlsSize = total_discurls;

    for(size_t i = 0; i < config_discurls; ++i)
        request.server.discoveryUrls[i] = server->config.applicationDescription.discoveryUrls[i];

    /* TODO: Add nl only if discoveryUrl not already present */
    for(size_t i = 0; i < nl_discurls; ++i) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        request.server.discoveryUrls[config_discurls + i] = nl->discoveryUrl;
    }

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    request.discoveryConfigurationSize = 1;
    request.discoveryConfiguration = UA_ExtensionObject_new();
    UA_ExtensionObject_init(&request.discoveryConfiguration[0]);
    // Set to NODELETE so that we can just use a pointer to the mdns config
    request.discoveryConfiguration[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    request.discoveryConfiguration[0].content.decoded.type = &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION];
    request.discoveryConfiguration[0].content.decoded.data = &server->config.discovery.mdns;
#endif

    // First try with RegisterServer2, if that isn't implemented, use RegisterServer
    UA_RegisterServer2Response response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_REGISTERSERVER2REQUEST],
                        &response, &UA_TYPES[UA_TYPES_REGISTERSERVER2RESPONSE]);

    UA_StatusCode serviceResult = response.responseHeader.serviceResult;
    UA_RegisterServer2Response_deleteMembers(&response);
    UA_Array_delete(request.discoveryConfiguration,
                    request.discoveryConfigurationSize,
                    &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    request.discoveryConfiguration = NULL;
    request.discoveryConfigurationSize = 0;

    if(serviceResult == UA_STATUSCODE_BADNOTIMPLEMENTED ||
       serviceResult == UA_STATUSCODE_BADSERVICEUNSUPPORTED) {
        /* Try RegisterServer */
        UA_RegisterServerRequest request_fallback;
        UA_RegisterServerRequest_init(&request_fallback);
        /* Copy from RegisterServer2 request */
        request_fallback.requestHeader = request.requestHeader;
        request_fallback.server = request.server;

        UA_RegisterServerResponse response_fallback;

        __UA_Client_Service(client, &request_fallback,
                            &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST],
                            &response_fallback,
                            &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE]);

        serviceResult = response_fallback.responseHeader.serviceResult;
        UA_RegisterServerResponse_deleteMembers(&response_fallback);
    }

    if(serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_CLIENT,
                     "RegisterServer/RegisterServer2 failed with statuscode %s",
                     UA_StatusCode_name(serviceResult));
    }

    return serviceResult;
}

UA_StatusCode
UA_Server_register_discovery(UA_Server *server, UA_Client *client,
                             const char* semaphoreFilePath) {
    return register_server_with_discovery_server(server, client,
                                                 false, semaphoreFilePath);
}

UA_StatusCode
UA_Server_unregister_discovery(UA_Server *server, UA_Client *client) {
    return register_server_with_discovery_server(server, client,
                                                 true, NULL);
}

#endif /* UA_ENABLE_DISCOVERY */
