#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_util.h"

void Service_GetEndpoints(UA_Server                    *server,
                          const UA_GetEndpointsRequest *request,
                          UA_GetEndpointsResponse      *response) {
    response->endpoints = UA_malloc(sizeof(UA_EndpointDescription));
    if(!response->endpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    if(UA_EndpointDescription_copy(server->endpointDescriptions, response->endpoints) != UA_STATUSCODE_GOOD) {
        UA_free(response->endpoints);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    UA_String_deleteMembers(&response->endpoints->endpointUrl);
    if(UA_String_copy(&request->endpointUrl, &response->endpoints->endpointUrl) != UA_STATUSCODE_GOOD) {
        UA_EndpointDescription_delete(response->endpoints);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->endpointsSize = 1;
}
