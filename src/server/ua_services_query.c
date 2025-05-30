/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_services.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_QUERY /* conditional compilation */

struct QueryContext {
    UA_Server *server;
    UA_Session *session;
    const UA_ContentFilter *filter;

    size_t dataToReturnSize;
    const UA_QueryDataDescription *dataToReturn;

    UA_NodeId typeDefinitionNode;

    size_t *queryDataSetsSize;
    UA_QueryDataSet **queryDataSets;

    UA_StatusCode status;
};

static UA_StatusCode
fillQueryDataSet(struct QueryContext *ctx, const UA_NodeId nodeId) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_QueryDataSet qds;
    UA_QueryDataSet_init(&qds);
    res |= UA_NodeId_copy(&nodeId, &qds.nodeId.nodeId);
    res |= UA_NodeId_copy(&ctx->typeDefinitionNode, &qds.typeDefinitionNode.nodeId);

    /* Allocate the output array */
    qds.values = (UA_Variant*)
        UA_Array_new(ctx->dataToReturnSize, &UA_TYPES[UA_TYPES_VARIANT]);
    if(!qds.values) {
        UA_QueryDataSet_clear(&qds);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    qds.valuesSize = ctx->dataToReturnSize;

    /* Iterate over the data elements to return */
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    for(size_t i = 0; i < ctx->dataToReturnSize; i++) {
        const UA_QueryDataDescription *qdt = &ctx->dataToReturn[i];

        /* Resolve the BrowsePath to a NodeId */
        UA_BrowsePath bp;
        bp.startingNode = nodeId;
        bp.relativePath = qdt->relativePath;
        UA_BrowsePathResult bpr = translateBrowsePathToNodeIds(ctx->server, &bp);
        if(bpr.statusCode != UA_STATUSCODE_GOOD &&
           bpr.statusCode != UA_STATUSCODE_BADNOMATCH) {
            res = bpr.statusCode;
            UA_BrowsePathResult_clear(&bpr);
            return res;
        }

        /* If not found, return an empty variant */
        if(bpr.targetsSize == 0)
            continue;

        /* Use the first match to read the attribute */
        rvi.nodeId = bpr.targets[0].targetId.nodeId;
        rvi.attributeId = qdt->attributeId;
        rvi.indexRange = qdt->indexRange;
        UA_DataValue v = readWithSession(ctx->server, ctx->session, &rvi,
                                         UA_TIMESTAMPSTORETURN_NEITHER);
        UA_BrowsePathResult_clear(&bpr);

        /* TODO: Error handling */
        qds.values[i] = v.value;
    }

    /* Append results */
    res = UA_Array_append((void**)ctx->queryDataSets, ctx->queryDataSetsSize,
                          &qds, &UA_TYPES[UA_TYPES_QUERYDATASET]);
    if(res != UA_STATUSCODE_GOOD)
        UA_QueryDataSet_clear(&qds);
    return res;
}

static void *
evaluateQueryInstance(void *context, UA_ReferenceTarget *t) {
    /* Currently no cross-server queries are supported */
    if(!UA_NodePointer_isLocal(t->targetId))
        return NULL;

    /* Evaluate the content filter for the instance */
    UA_NodeId target = UA_NodePointer_toNodeId(t->targetId);
    struct QueryContext *ctx = (struct QueryContext*)context;
    UA_StatusCode res = evaluateWhereClause(ctx->server, ctx->session,
                                            &target, ctx->filter, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return NULL;

    /* Generate and append the line item */
    ctx->status = fillQueryDataSet(ctx, target);
    return (ctx->status == UA_STATUSCODE_GOOD) ? NULL : (void*)0x01;
}

static void
Operation_QueryNodeTypeDescription(UA_Server *server, UA_Session *session,
                                   const UA_NodeTypeDescription *ntd,
                                   const UA_ContentFilter *filter,
                                   UA_QueryFirstResponse *response) {
    /* Currently no cross-server queries are supported */
    if(!UA_ExpandedNodeId_isLocal(&ntd->typeDefinitionNode)) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTSUPPORTED;
        return;
    }

    /* Set up the context */
    struct QueryContext ctx;
    ctx.server = server;
    ctx.session = session;
    ctx.filter = filter;
    ctx.dataToReturnSize = ntd->dataToReturnSize;
    ctx.dataToReturn = ntd->dataToReturn;
    ctx.typeDefinitionNode = ntd->typeDefinitionNode.nodeId;
    ctx.queryDataSetsSize = &response->queryDataSetsSize;
    ctx.queryDataSets = &response->queryDataSets;
    ctx.status = UA_STATUSCODE_GOOD;

    /* Get a list of all type nodes (maybe also subtypes) */
    size_t typeNodesSize = 1;
    UA_ExpandedNodeId *typeNodes = (UA_ExpandedNodeId*)(uintptr_t)&ntd->typeDefinitionNode;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(ntd->includeSubTypes) {
        UA_ReferenceTypeSet hasSubTypeRefSet;
        UA_NodeId hasSubType = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
        res = referenceTypeIndices(server, &hasSubType, &hasSubTypeRefSet, true);
        UA_CHECK_STATUS(res, response->responseHeader.serviceResult = res; return);
        res = browseRecursive(server, 1, &ntd->typeDefinitionNode.nodeId,
                              UA_BROWSEDIRECTION_FORWARD, &hasSubTypeRefSet,
                              ~(UA_UInt32)0, true, &typeNodesSize, &typeNodes);
        UA_CHECK_STATUS(res, response->responseHeader.serviceResult = res; return);
    }

    /* Get the HasTypeDefinition reference set */
    UA_ReferenceTypeSet hasTypeDefRefSet;
    UA_NodeId hasTypeDef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    res = referenceTypeIndices(server, &hasTypeDef, &hasTypeDefRefSet, true);
    UA_CHECK_STATUS(res, goto cleanup);

    /* Iterate over all type nodes */
    for(size_t i = 0; i < typeNodesSize; i++) {
        /* Currently no cross-server queries are supported */
        if(!UA_ExpandedNodeId_isLocal(&typeNodes[i]))
            continue;

        /* Get the type node */
        const UA_Node *typeNode =
            UA_NODESTORE_GET_SELECTIVE(server, &typeNodes[i].nodeId,
                                       UA_NODEATTRIBUTESMASK_NONE,
                                       hasTypeDefRefSet,
                                       UA_BROWSEDIRECTION_INVERSE);
        if(!typeNode)
            continue;

        /* Iterate the references */
        const UA_NodeHead *typeHead = &typeNode->head;
        for(size_t j = 0; j < typeHead->referencesSize && !res; j++) {
            UA_NodeReferenceKind *rk = &typeHead->references[j];

            /* Follow only inverse HasTypeDefinition references */
            if(!rk->isInverse)
                continue;
            if(!UA_ReferenceTypeSet_contains(&hasTypeDefRefSet, rk->referenceTypeIndex))
                continue;

            /* Evaluate each instance node */
            UA_NodeReferenceKind_iterate(rk, evaluateQueryInstance, &ctx);
            if(ctx.status != UA_STATUSCODE_GOOD)
                res = ctx.status;
        }

        /* Release the type node */
        UA_NODESTORE_RELEASE(server, typeNode);
    }

 cleanup:
    if(typeNodes != &ntd->typeDefinitionNode)
        UA_Array_delete(typeNodes, typeNodesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    response->responseHeader.serviceResult = res;
}

void
Service_QueryFirst(UA_Server *server, UA_Session *session,
                   const UA_QueryFirstRequest *request,
                   UA_QueryFirstResponse *response) {
    for(size_t i = 0; i < request->nodeTypesSize; i++) {
        Operation_QueryNodeTypeDescription(server, session, &request->nodeTypes[i],
                                           &request->filter, response);
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
            break;
    }
}

void
Service_QueryNext(UA_Server *server, UA_Session *session,
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
