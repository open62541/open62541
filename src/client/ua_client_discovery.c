/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_client_internal.h"

/* Gets a list of endpoints. Memory is allocated for endpointDescription array */
static UA_StatusCode
UA_Client_getEndpointsInternal(UA_Client *client, const UA_String endpointUrl,
                               size_t *endpointDescriptionsSize,
                               UA_EndpointDescription **endpointDescriptions) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    // assume the endpointurl outlives the service call
    request.endpointUrl = endpointUrl;

    UA_GetEndpointsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_StatusCode retval = response.responseHeader.serviceResult;
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "GetEndpointRequest failed with error code %s",
                     UA_StatusCode_name(retval));
        UA_GetEndpointsResponse_clear(&response);
        return retval;
    }
    *endpointDescriptions = response.endpoints;
    *endpointDescriptionsSize = response.endpointsSize;
    response.endpoints = NULL;
    response.endpointsSize = 0;
    UA_GetEndpointsResponse_clear(&response);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_getEndpoints(UA_Client *client, const char *serverUrl,
                       size_t* endpointDescriptionsSize,
                       UA_EndpointDescription** endpointDescriptions) {
    UA_Boolean connected = (client->channel.state == UA_SECURECHANNELSTATE_OPEN);
    /* Client is already connected to a different server */
    if(connected && strncmp((const char*)client->config.endpoint.endpointUrl.data, serverUrl,
                            client->config.endpoint.endpointUrl.length) != 0) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval;
    const UA_String url = UA_STRING((char*)(uintptr_t)serverUrl);
    if(!connected) {
        retval = UA_Client_connectSecureChannel(client, serverUrl);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }
    retval = UA_Client_getEndpointsInternal(client, url, endpointDescriptionsSize,
                                            endpointDescriptions);

    if(!connected)
        UA_Client_disconnect(client);
    return retval;
}

UA_StatusCode
UA_Client_findServers(UA_Client *client, const char *serverUrl,
                      size_t serverUrisSize, UA_String *serverUris,
                      size_t localeIdsSize, UA_String *localeIds,
                      size_t *registeredServersSize,
                      UA_ApplicationDescription **registeredServers) {
    UA_Boolean connected = (client->channel.state == UA_SECURECHANNELSTATE_OPEN);
    /* Client is already connected to a different server */
    if(connected && strncmp((const char*)client->config.endpoint.endpointUrl.data, serverUrl,
                            client->config.endpoint.endpointUrl.length) != 0) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval;
    if(!connected) {
        retval = UA_Client_connectSecureChannel(client, serverUrl);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Prepare the request */
    UA_FindServersRequest request;
    UA_FindServersRequest_init(&request);
    request.serverUrisSize = serverUrisSize;
    request.serverUris = serverUris;
    request.localeIdsSize = localeIdsSize;
    request.localeIds = localeIds;

    /* Send the request */
    UA_FindServersResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE]);

    /* Process the response */
    retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        *registeredServersSize = response.serversSize;
        *registeredServers = response.servers;
        response.serversSize = 0;
        response.servers = NULL;
    } else {
        *registeredServersSize = 0;
        *registeredServers = NULL;
    }

    /* Clean up */
    UA_FindServersResponse_clear(&response);
    if(!connected)
        UA_Client_disconnect(client);
    return retval;
}

#ifdef UA_ENABLE_DISCOVERY

UA_StatusCode
UA_Client_findServersOnNetwork(UA_Client *client, const char *serverUrl,
                               UA_UInt32 startingRecordId, UA_UInt32 maxRecordsToReturn,
                               size_t serverCapabilityFilterSize, UA_String *serverCapabilityFilter,
                               size_t *serverOnNetworkSize, UA_ServerOnNetwork **serverOnNetwork) {
    UA_Boolean connected = (client->channel.state == UA_SECURECHANNELSTATE_OPEN);
    /* Client is already connected to a different server */
    if(connected && strncmp((const char*)client->config.endpoint.endpointUrl.data, serverUrl,
                            client->config.endpoint.endpointUrl.length) != 0) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval;
    if(!connected) {
        retval = UA_Client_connectSecureChannel(client, serverUrl);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Prepare the request */
    UA_FindServersOnNetworkRequest request;
    UA_FindServersOnNetworkRequest_init(&request);
    request.startingRecordId = startingRecordId;
    request.maxRecordsToReturn = maxRecordsToReturn;
    request.serverCapabilityFilterSize = serverCapabilityFilterSize;
    request.serverCapabilityFilter = serverCapabilityFilter;

    /* Send the request */
    UA_FindServersOnNetworkResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST],
                        &response, &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKRESPONSE]);

    /* Process the response */
    retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        *serverOnNetworkSize = response.serversSize;
        *serverOnNetwork = response.servers;
        response.serversSize = 0;
        response.servers = NULL;
    } else {
        *serverOnNetworkSize = 0;
        *serverOnNetwork = NULL;
    }

    /* Clean up */
    UA_FindServersOnNetworkResponse_clear(&response);
    if(!connected)
        UA_Client_disconnect(client);
    return retval;
}

#endif
