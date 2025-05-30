/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_services.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_QUERY /* conditional compilation */

void Service_QueryFirst(UA_Server *server, UA_Session *session,
                        const UA_QueryFirstRequest *request,
                        UA_QueryFirstResponse *response) {
    response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTIMPLEMENTED;
}

void Service_QueryNext(UA_Server *server, UA_Session *session,
                       const UA_QueryNextRequest *request,
                       UA_QueryNextResponse *response) {
    response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTIMPLEMENTED;
}

UA_StatusCode
UA_Server_query(UA_Server *server,
                size_t nodeTypesSize,
                UA_NodeTypeDescription *nodeTypes,
                UA_ContentFilter filter,
                size_t *outQueryDataSetsSize,
                UA_QueryDataSet **outQueryDataSets) {
    /* Validate the input */
    if(!server || !nodeTypes || !outQueryDataSets || !outQueryDataSetsSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Prepare the request/response pair */
    UA_QueryFirstRequest req;
    UA_QueryFirstRequest_init(&req);
    UA_QueryFirstResponse resp;
    UA_QueryFirstResponse_init(&resp);
    req.nodeTypesSize = nodeTypesSize;
    req.nodeTypes = nodeTypes;
    req.filter = filter;

    /* Call the query service */
    lockServer(server);
    Service_QueryFirst(server, &server->adminSession, &req, &resp);
    unlockServer(server);

    /* The query failed */
    UA_StatusCode res = resp.responseHeader.serviceResult;
    if(res != UA_STATUSCODE_GOOD) {
        UA_QueryFirstResponse_clear(&resp);
        return res;
    }

    /* Move the result to the ouput */
    *outQueryDataSets = resp.queryDataSets;
    *outQueryDataSetsSize = resp.queryDataSetsSize;
    resp.queryDataSets = NULL;
    resp.queryDataSetsSize = 0;
    UA_QueryFirstResponse_clear(&resp);

    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_QUERY */
