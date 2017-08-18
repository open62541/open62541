/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_util.h"

void Service_FindServers(UA_Server *server, UA_Session *session,
                         const UA_FindServersRequest *request, UA_FindServersResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing FindServersRequest");
    /* copy ApplicationDescription from the config */
    UA_ApplicationDescription *descr = UA_malloc(sizeof(UA_ApplicationDescription));
    if(!descr) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->responseHeader.serviceResult =
        UA_ApplicationDescription_copy(&server->config.applicationDescription, descr);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_free(descr);
        return;
    }

    /* add the discoveryUrls from the networklayers */
    UA_String *disc = UA_realloc(descr->discoveryUrls, sizeof(UA_String) *
                                 (descr->discoveryUrlsSize + server->config.networkLayersSize));
    if(!disc) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_ApplicationDescription_delete(descr);
        return;
    }
    size_t existing = descr->discoveryUrlsSize;
    descr->discoveryUrls = disc;
    descr->discoveryUrlsSize += server->config.networkLayersSize;

    // TODO: Add nl only if discoveryUrl not already present
    for(size_t i = 0; i < server->config.networkLayersSize; ++i) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        UA_String_copy(&nl->discoveryUrl, &descr->discoveryUrls[existing + i]);
    }

    response->servers = descr;
    response->serversSize = 1;
}

void Service_GetEndpoints(UA_Server *server, UA_Session *session, const UA_GetEndpointsRequest *request,
                          UA_GetEndpointsResponse *response) {
    /* If the client expects to see a specific endpointurl, mirror it back. If
       not, clone the endpoints with the discovery url of all networklayers. */
    const UA_String *endpointUrl = &request->endpointUrl;
    if(endpointUrl->length > 0) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing GetEndpointsRequest with endpointUrl " \
                             UA_PRINTF_STRING_FORMAT, UA_PRINTF_STRING_DATA(*endpointUrl));
    } else {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing GetEndpointsRequest with an empty endpointUrl");
    }

    /* test if the supported binary profile shall be returned */
#ifdef NO_ALLOCA
    UA_Boolean relevant_endpoints[server->endpointDescriptionsSize];
#else
    UA_Boolean *relevant_endpoints = UA_alloca(sizeof(UA_Boolean) * server->endpointDescriptionsSize);
#endif
    memset(relevant_endpoints, 0, sizeof(UA_Boolean) * server->endpointDescriptionsSize);
    size_t relevant_count = 0;
    if(request->profileUrisSize == 0) {
        for(size_t j = 0; j < server->endpointDescriptionsSize; ++j)
            relevant_endpoints[j] = true;
        relevant_count = server->endpointDescriptionsSize;
    } else {
        for(size_t j = 0; j < server->endpointDescriptionsSize; ++j) {
            for(size_t i = 0; i < request->profileUrisSize; ++i) {
                if(!UA_String_equal(&request->profileUris[i], &server->endpointDescriptions[j].transportProfileUri))
                    continue;
                relevant_endpoints[j] = true;
                ++relevant_count;
                break;
            }
        }
    }

    if(relevant_count == 0) {
        response->endpointsSize = 0;
        return;
    }

    /* Clone the endpoint for each networklayer? */
    size_t clone_times = 1;
    UA_Boolean nl_endpointurl = false;
    if(endpointUrl->length == 0) {
        clone_times = server->config.networkLayersSize;
        nl_endpointurl = true;
    }

    response->endpoints = UA_Array_new(relevant_count * clone_times, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!response->endpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->endpointsSize = relevant_count * clone_times;

    size_t k = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < clone_times; ++i) {
        if(nl_endpointurl)
            endpointUrl = &server->config.networkLayers[i].discoveryUrl;
        for(size_t j = 0; j < server->endpointDescriptionsSize; ++j) {
            if(!relevant_endpoints[j])
                continue;
            retval |= UA_EndpointDescription_copy(&server->endpointDescriptions[j], &response->endpoints[k]);
            retval |= UA_String_copy(endpointUrl, &response->endpoints[k].endpointUrl);
            ++k;
        }
    }

    if(retval != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = retval;
        UA_Array_delete(response->endpoints, response->endpointsSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        response->endpoints = NULL;
        response->endpointsSize = 0;
        return;
    }
}
