#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_util.h"

void Service_FindServers(UA_Server *server, const UA_FindServersRequest *request, UA_FindServersResponse *response) {
    response->servers = UA_malloc(sizeof(UA_ApplicationDescription));
    if(!response->servers) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    if(UA_ApplicationDescription_copy(&server->description, response->servers) != UA_STATUSCODE_GOOD) {
        UA_free(response->servers);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
	response->serversSize = 1;
}

void Service_GetEndpoints(UA_Server *server, const UA_GetEndpointsRequest *request, UA_GetEndpointsResponse *response) {
    /* test if the supported binary profile shall be returned */
    UA_Boolean *relevant_endpoints = UA_alloca(sizeof(UA_Boolean)*server->endpointDescriptionsSize);
    size_t relevant_count = 0;
    for(UA_Int32 j = 0; j < server->endpointDescriptionsSize; j++) {
        relevant_endpoints[j] = UA_FALSE;
        if(request->profileUrisSize <= 0) {
            relevant_endpoints[j] = UA_TRUE;
            relevant_count++;
            continue;
        }
        for(UA_Int32 i = 0; i < request->profileUrisSize; i++) {
            if(UA_String_equal(&request->profileUris[i], &server->endpointDescriptions->transportProfileUri)) {
                relevant_endpoints[j] = UA_TRUE;
                relevant_count++;
                break;
            }
        }
    }

    if(relevant_count == 0) {
        response->endpointsSize = 0;
        return;
    }

    response->endpoints = UA_malloc(sizeof(UA_EndpointDescription) * relevant_count);
    if(!response->endpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Array_copy(server->endpointDescriptions, (void**)&response->endpoints,
                      &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION], server->endpointDescriptionsSize);
    if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        response->endpointsSize = relevant_count;
}

