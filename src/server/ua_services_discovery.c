#include "ua_services.h"
#include "ua_namespace_0.h"
#include "ua_util.h"

void Service_GetEndpoints(UA_Server                    *server,
                          const UA_GetEndpointsRequest *request,
                          UA_GetEndpointsResponse      *response) {
    UA_GetEndpointsResponse_init(response);
    response->endpointsSize = 1;
    response->endpoints = UA_alloc(sizeof(UA_EndpointDescription));
    if(!response->endpoints) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    if(UA_EndpointDescription_copy(server->endpointDescriptions, response->endpoints) != UA_STATUSCODE_GOOD) {
        UA_free(response->endpoints);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
    }
}
