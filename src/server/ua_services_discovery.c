#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_util.h"

void Service_FindServers(UA_Server                    *server,
                          const UA_FindServersRequest *request,
                          UA_FindServersResponse      *response) {
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

void Service_GetEndpoints(UA_Server                    *server,
                          const UA_GetEndpointsRequest *request,
                          UA_GetEndpointsResponse      *response) {
    /* test if the supported binary profile shall be returned */
    UA_Boolean returnBinary = request->profileUrisSize == 0;
    for(UA_Int32 i=0;i<request->profileUrisSize;i++) {
        if(UA_String_equal(&request->profileUris[i], &server->endpointDescriptions->transportProfileUri)) {
            returnBinary = UA_TRUE;
            break;
        }
    }

    if(!returnBinary) {
        response->endpointsSize = 0;
        return;
    }

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
