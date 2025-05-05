/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2015-2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2016 (c) Chris Iatrou
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Fabian Arndt
 *    Copyright 2018 (c) Peter Rustler, basyskom GmbH
 */

#include "ua_client_internal.h"

UA_StatusCode
UA_Client_NamespaceGetIndex(UA_Client *client, UA_String *namespaceUri,
                            UA_UInt16 *namespaceIndex) {
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId id;
    UA_ReadValueId_init(&id);
    id.attributeId = UA_ATTRIBUTEID_VALUE;
    id.nodeId = UA_NS0ID(SERVER_NAMESPACEARRAY);
    request.nodesToRead = &id;
    request.nodesToReadSize = 1;

    UA_ReadResponse response = UA_Client_Service_read(client, request);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        retval = response.responseHeader.serviceResult;
    else if(response.resultsSize != 1 || !response.results[0].hasValue)
        retval = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
    else if(response.results[0].value.type != &UA_TYPES[UA_TYPES_STRING])
        retval = UA_STATUSCODE_BADTYPEMISMATCH;

    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_clear(&response);
        return retval;
    }

    retval = UA_STATUSCODE_BADNOTFOUND;
    UA_String *ns = (UA_String *)response.results[0].value.data;
    for(size_t i = 0; i < response.results[0].value.arrayLength; ++i) {
        if(UA_String_equal(namespaceUri, &ns[i])) {
            *namespaceIndex = (UA_UInt16)i;
            retval = UA_STATUSCODE_GOOD;
            break;
        }
    }

    UA_ReadResponse_clear(&response);
    return retval;
}

UA_StatusCode
UA_Client_forEachChildNodeCall(UA_Client *client, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    if(!bReq.nodesToBrowse)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    bReq.nodesToBrowseSize = 1;
    UA_NodeId_copy(&parentNodeId, &bReq.nodesToBrowse[0].nodeId);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; //return everything
    bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_BOTH;

    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);

    UA_StatusCode retval = bResp.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < bResp.resultsSize; ++i) {
            for(size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
                UA_ReferenceDescription *ref = &bResp.results[i].references[j];
                retval |= callback(ref->nodeId.nodeId, !ref->isForward,
                                   ref->referenceTypeId, handle);
            }
        }
    }

    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
    return retval;
}

/*******************/
/* Node Management */
/*******************/

UA_StatusCode
UA_Client_addReference(UA_Client *client, const UA_NodeId sourceNodeId,
                       const UA_NodeId referenceTypeId, UA_Boolean isForward,
                       const UA_String targetServerUri,
                       const UA_ExpandedNodeId targetNodeId,
                       UA_NodeClass targetNodeClass) {
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = sourceNodeId;
    item.referenceTypeId = referenceTypeId;
    item.isForward = isForward;
    item.targetServerUri = targetServerUri;
    item.targetNodeId = targetNodeId;
    item.targetNodeClass = targetNodeClass;
    UA_AddReferencesRequest request;
    UA_AddReferencesRequest_init(&request);
    request.referencesToAdd = &item;
    request.referencesToAddSize = 1;
    UA_AddReferencesResponse response = UA_Client_Service_addReferences(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_AddReferencesResponse_clear(&response);
        return retval;
    }
    if(response.resultsSize != 1) {
        UA_AddReferencesResponse_clear(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    retval = response.results[0];
    UA_AddReferencesResponse_clear(&response);
    return retval;
}

UA_StatusCode
UA_Client_deleteReference(UA_Client *client, const UA_NodeId sourceNodeId,
                          const UA_NodeId referenceTypeId, UA_Boolean isForward,
                          const UA_ExpandedNodeId targetNodeId,
                          UA_Boolean deleteBidirectional) {
    UA_DeleteReferencesItem item;
    UA_DeleteReferencesItem_init(&item);
    item.sourceNodeId = sourceNodeId;
    item.referenceTypeId = referenceTypeId;
    item.isForward = isForward;
    item.targetNodeId = targetNodeId;
    item.deleteBidirectional = deleteBidirectional;
    UA_DeleteReferencesRequest request;
    UA_DeleteReferencesRequest_init(&request);
    request.referencesToDelete = &item;
    request.referencesToDeleteSize = 1;
    UA_DeleteReferencesResponse response = UA_Client_Service_deleteReferences(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DeleteReferencesResponse_clear(&response);
        return retval;
    }
    if(response.resultsSize != 1) {
        UA_DeleteReferencesResponse_clear(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    retval = response.results[0];
    UA_DeleteReferencesResponse_clear(&response);
    return retval;
}

UA_StatusCode
UA_Client_deleteNode(UA_Client *client, const UA_NodeId nodeId,
                     UA_Boolean deleteTargetReferences) {
    UA_DeleteNodesItem item;
    UA_DeleteNodesItem_init(&item);
    item.nodeId = nodeId;
    item.deleteTargetReferences = deleteTargetReferences;
    UA_DeleteNodesRequest request;
    UA_DeleteNodesRequest_init(&request);
    request.nodesToDelete = &item;
    request.nodesToDeleteSize = 1;
    UA_DeleteNodesResponse response = UA_Client_Service_deleteNodes(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DeleteNodesResponse_clear(&response);
        return retval;
    }
    if(response.resultsSize != 1) {
        UA_DeleteNodesResponse_clear(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    retval = response.results[0];
    UA_DeleteNodesResponse_clear(&response);
    return retval;
}

UA_StatusCode
__UA_Client_addNode(UA_Client *client, const UA_NodeClass nodeClass,
                    const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId,
                    const UA_NodeId referenceTypeId, const UA_QualifiedName browseName,
                    const UA_NodeId typeDefinition, const UA_NodeAttributes *attr,
                    const UA_DataType *attributeType, UA_NodeId *outNewNodeId) {
    UA_AddNodesRequest request;
    UA_AddNodesRequest_init(&request);
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.parentNodeId.nodeId = parentNodeId;
    item.referenceTypeId = referenceTypeId;
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    item.nodeClass = nodeClass;
    item.typeDefinition.nodeId = typeDefinition;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type = attributeType;
    item.nodeAttributes.content.decoded.data = (void*)(uintptr_t)attr; // hack. is not written into.
    request.nodesToAdd = &item;
    request.nodesToAddSize = 1;
    UA_AddNodesResponse response = UA_Client_Service_addNodes(client, request);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_AddNodesResponse_clear(&response);
        return retval;
    }

    if(response.resultsSize != 1) {
        UA_AddNodesResponse_clear(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    /* Move the id of the created node */
    retval = response.results[0].statusCode;
    if(retval == UA_STATUSCODE_GOOD && outNewNodeId) {
        *outNewNodeId = response.results[0].addedNodeId;
        UA_NodeId_init(&response.results[0].addedNodeId);
    }

    UA_AddNodesResponse_clear(&response);
    return retval;
}

/********/
/* Call */
/********/

UA_StatusCode
UA_Client_call(UA_Client *client, const UA_NodeId objectId,
               const UA_NodeId methodId, size_t inputSize,
               const UA_Variant *input, size_t *outputSize,
               UA_Variant **output) {
    /* Set up the request */
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    UA_CallMethodRequest item;
    UA_CallMethodRequest_init(&item);
    item.methodId = methodId;
    item.objectId = objectId;
    item.inputArguments = (UA_Variant *)(void*)(uintptr_t)input; // cast const...
    item.inputArgumentsSize = inputSize;
    request.methodsToCall = &item;
    request.methodsToCallSize = 1;

    /* Call the service */
    UA_CallResponse response = UA_Client_Service_call(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(response.resultsSize == 1)
            retval = response.results[0].statusCode;
        else
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    if(UA_StatusCode_isBad(retval)) {
        UA_CallResponse_clear(&response);
        return retval;
    }

    /* Move the output arguments */
    if(output != NULL && outputSize != NULL) {
        *output = response.results[0].outputArguments;
        *outputSize = response.results[0].outputArgumentsSize;
        response.results[0].outputArguments = NULL;
        response.results[0].outputArgumentsSize = 0;
    }
    UA_CallResponse_clear(&response);
    return retval;
}

/**********/
/* Browse */
/**********/

UA_BrowseResult
UA_Client_browse(UA_Client *client, const UA_ViewDescription *view,
                 UA_UInt32 requestedMaxReferencesPerNode,
                 const UA_BrowseDescription *nodesToBrowse) {
    UA_BrowseResult res;
    UA_BrowseRequest request;
    UA_BrowseResponse response;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!nodesToBrowse) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto error;
    }

    /* Set up the request */
    UA_BrowseRequest_init(&request);
    if(view)
        request.view = *view;
    request.requestedMaxReferencesPerNode = requestedMaxReferencesPerNode;
    request.nodesToBrowse = (UA_BrowseDescription*)(uintptr_t)nodesToBrowse;
    request.nodesToBrowseSize = 1;

    /* Call the service */
    response = UA_Client_Service_browse(client, request);
    retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize != 1)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(UA_StatusCode_isBad(retval))
        goto error;

    /* Return the result */
    res = response.results[0];
    response.resultsSize = 0;
    UA_BrowseResponse_clear(&response);
    return res;

 error:
    UA_BrowseResponse_clear(&response);
    UA_BrowseResult_init(&res);
    res.statusCode = retval;
    return res;
}

UA_BrowseResult
UA_Client_browseNext(UA_Client *client, UA_Boolean releaseContinuationPoint,
                     UA_ByteString continuationPoint) {
    /* Set up the request */
    UA_BrowseNextRequest request;
    UA_BrowseNextRequest_init(&request);
    request.releaseContinuationPoints = releaseContinuationPoint;
    request.continuationPoints = &continuationPoint;
    request.continuationPointsSize = 1;

    /* Call the service */
    UA_BrowseResult res;
    UA_BrowseNextResponse response = UA_Client_Service_browseNext(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize != 1)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(UA_StatusCode_isBad(retval)) {
        UA_BrowseNextResponse_clear(&response);
        UA_BrowseResult_init(&res);
        res.statusCode = retval;
        return res;
    }

    /* Return the result */
    res = response.results[0];
    response.resultsSize = 0;
    UA_BrowseNextResponse_clear(&response);
    return res;
}

UA_BrowsePathResult
UA_Client_translateBrowsePathToNodeIds(UA_Client *client,
                                       const UA_BrowsePath *browsePath) {
    UA_BrowsePathResult res;
    UA_TranslateBrowsePathsToNodeIdsRequest request;
    UA_TranslateBrowsePathsToNodeIdsResponse response;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!browsePath) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto error;
    }

    /* Set up the request */
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
    request.browsePaths = (UA_BrowsePath*)(uintptr_t)browsePath;
    request.browsePathsSize = 1;

    /* Call the service */
    response = UA_Client_Service_translateBrowsePathsToNodeIds(client, request);
    retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize != 1)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(UA_StatusCode_isBad(retval))
        goto error;

    /* Return the result */
    res = response.results[0];
    response.resultsSize = 0;
    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&response);
    return res;

 error:
    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&response);
    UA_BrowsePathResult_init(&res);
    res.statusCode = retval;
    return res;
}

/********************/
/* Write Attributes */
/********************/

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_write(UA_Client *client, const UA_WriteValue *wv) {
    if(!wv)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Set up the request */
    UA_WriteRequest request;
    UA_WriteRequest_init(&request);
    request.nodesToWrite = (UA_WriteValue*)(uintptr_t)wv;
    request.nodesToWriteSize = 1;

    /* Call the service */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_WriteResponse response = UA_Client_Service_write(client, request);
    retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize != 1)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(UA_StatusCode_isBad(retval)) {
        UA_WriteResponse_clear(&response);
        return retval;
    }

    /* Return the result */
    retval = response.results[0];
    UA_WriteResponse_clear(&response);
    return retval;
}

UA_StatusCode
__UA_Client_writeAttribute(UA_Client *client, const UA_NodeId *nodeId,
                           UA_AttributeId attributeId, const void *in,
                           const UA_DataType *inDataType) {
    if(!in || !inDataType)
      return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    wValue.nodeId = *nodeId;
    wValue.attributeId = attributeId;
    if(attributeId == UA_ATTRIBUTEID_VALUE &&
       inDataType == &UA_TYPES[UA_TYPES_VARIANT]) {
        wValue.value.value = *(const UA_Variant*)in;
        wValue.value.hasValue = true;
    } else if(attributeId == UA_ATTRIBUTEID_VALUE &&
              inDataType == &UA_TYPES[UA_TYPES_DATAVALUE]) {
        wValue.value = *(const UA_DataValue*)in;
    } else {
        /* Hack to get rid of the const annotation.
         * The value is never written into. */
        UA_Variant_setScalar(&wValue.value.value, (void*)(uintptr_t)in, inDataType);
        wValue.value.hasValue = true;
    }
    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = &wValue;
    wReq.nodesToWriteSize = 1;

    UA_WriteResponse wResp = UA_Client_Service_write(client, wReq);

    UA_StatusCode retval = wResp.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(wResp.resultsSize == 1)
            retval = wResp.results[0];
        else
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    UA_WriteResponse_clear(&wResp);
    return retval;
}

UA_StatusCode
UA_Client_writeArrayDimensionsAttribute(UA_Client *client, const UA_NodeId nodeId,
                                        size_t newArrayDimensionsSize,
                                        const UA_UInt32 *newArrayDimensions) {
    if(!newArrayDimensions)
      return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    wValue.nodeId = nodeId;
    wValue.attributeId = UA_ATTRIBUTEID_ARRAYDIMENSIONS;
    UA_Variant_setArray(&wValue.value.value, (void*)(uintptr_t)newArrayDimensions,
                        newArrayDimensionsSize, &UA_TYPES[UA_TYPES_UINT32]);
    wValue.value.hasValue = true;
    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = &wValue;
    wReq.nodesToWriteSize = 1;

    UA_WriteResponse wResp = UA_Client_Service_write(client, wReq);

    UA_StatusCode retval = wResp.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(wResp.resultsSize == 1)
            retval = wResp.results[0];
        else
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    UA_WriteResponse_clear(&wResp);
    return retval;
}

/*******************/
/* Read Attributes */
/*******************/

UA_DataValue
UA_Client_read(UA_Client *client, const UA_ReadValueId *rvi) {
    /* Set up the request */
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    request.nodesToRead = (UA_ReadValueId*)(uintptr_t)rvi;
    request.nodesToReadSize = 1;

    /* Call the service */
    UA_DataValue res;
    UA_ReadResponse response = UA_Client_Service_read(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize != 1)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(UA_StatusCode_isBad(retval)) {
        UA_ReadResponse_clear(&response);
        UA_DataValue_init(&res);
        res.status = retval;
        res.hasStatus = true;
        return res;
    }

    /* Return the result */
    res = response.results[0];
    response.resultsSize = 0;
    UA_ReadResponse_clear(&response);
    return res;
}

UA_StatusCode
__UA_Client_readAttribute(UA_Client *client, const UA_NodeId *nodeId,
                          UA_AttributeId attributeId, void *out,
                          const UA_DataType *outDataType) {
    UA_ReadValueId item;
    UA_ReadValueId_init(&item);
    item.nodeId = *nodeId;
    item.attributeId = attributeId;
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = &item;
    request.nodesToReadSize = 1;
    UA_ReadResponse response = UA_Client_Service_read(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(response.resultsSize == 1)
            retval = response.results[0].status;
        else
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    if(!UA_StatusCode_isEqualTop(retval,UA_STATUSCODE_GOOD)) {
        UA_ReadResponse_clear(&response);
        return retval;
    }

    /* Set the StatusCode */
    UA_DataValue *res = response.results;
    if(res->hasStatus)
        retval = res->status;

    /* Return early of no value is given */
    if(!res->hasValue) {
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
        UA_ReadResponse_clear(&response);
        return retval;
    }

    /* Copy value into out */
    if(attributeId == UA_ATTRIBUTEID_VALUE) {
        memcpy(out, &res->value, sizeof(UA_Variant));
        UA_Variant_init(&res->value);
    } else if(attributeId == UA_ATTRIBUTEID_NODECLASS) {
        memcpy(out, (UA_NodeClass*)res->value.data, sizeof(UA_NodeClass));
    } else if(UA_Variant_isScalar(&res->value) &&
              res->value.type == outDataType) {
        memcpy(out, res->value.data, res->value.type->memSize);
        UA_free(res->value.data);
        res->value.data = NULL;
    } else {
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    UA_ReadResponse_clear(&response);
    return retval;
}

static UA_StatusCode
processReadArrayDimensionsResult(UA_ReadResponse *response,
                                 UA_UInt32 **outArrayDimensions,
                                 size_t *outArrayDimensionsSize) {
    UA_StatusCode retval = response->responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if(response->resultsSize != 1)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    retval = response->results[0].status;
    if(!UA_StatusCode_isEqualTop(retval,UA_STATUSCODE_GOOD))
        return retval;

    UA_DataValue *res = &response->results[0];
    if(!res->hasValue ||
       UA_Variant_isScalar(&res->value) ||
       res->value.type != &UA_TYPES[UA_TYPES_UINT32])
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    /* Move results */
    *outArrayDimensions = (UA_UInt32*)res->value.data;
    *outArrayDimensionsSize = res->value.arrayLength;
    res->value.data = NULL;
    res->value.arrayLength = 0;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_readArrayDimensionsAttribute(UA_Client *client, const UA_NodeId nodeId,
                                       size_t *outArrayDimensionsSize,
                                       UA_UInt32 **outArrayDimensions) {
    UA_ReadValueId item;
    UA_ReadValueId_init(&item);
    item.nodeId = nodeId;
    item.attributeId = UA_ATTRIBUTEID_ARRAYDIMENSIONS;
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = &item;
    request.nodesToReadSize = 1;

    UA_ReadResponse response = UA_Client_Service_read(client, request);
    UA_StatusCode retval = processReadArrayDimensionsResult(&response, outArrayDimensions,
                                                            outArrayDimensionsSize);
    UA_ReadResponse_clear(&response);
    return retval;
}

/*********************/
/* Historical Access */
/*********************/

static UA_HistoryReadResponse
__UA_Client_HistoryRead(UA_Client *client, const UA_NodeId *nodeId,
                        UA_ExtensionObject* details, UA_String indexRange,
                        UA_TimestampsToReturn timestampsToReturn,
                        UA_ByteString continuationPoint, UA_Boolean releaseConti) {

    UA_HistoryReadValueId item;
    UA_HistoryReadValueId_init(&item);

    item.nodeId = *nodeId;
    item.indexRange = indexRange;
    item.continuationPoint = continuationPoint;
    item.dataEncoding = UA_QUALIFIEDNAME(0, NULL);

    UA_HistoryReadRequest request;
    UA_HistoryReadRequest_init(&request);

    request.nodesToRead = &item;
    request.nodesToReadSize = 1;
    request.timestampsToReturn = timestampsToReturn; // Defaults to Source
    request.releaseContinuationPoints = releaseConti; // No values are returned, if true

    /* Build ReadDetails */
    request.historyReadDetails = *details;

    return UA_Client_Service_historyRead(client, request);
}

static UA_StatusCode
__UA_Client_HistoryRead_service(UA_Client *client, const UA_NodeId *nodeId,
                                   const UA_HistoricalIteratorCallback callback,
                                   UA_ExtensionObject *details, UA_String indexRange,
                                   UA_TimestampsToReturn timestampsToReturn,
                                   void *callbackContext) {

    UA_ByteString continuationPoint = UA_BYTESTRING_NULL;
    UA_Boolean continuationAvail = false;
    UA_Boolean fetchMore = false;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    do {
        /* We release the continuation point, if no more data is requested by the user */
        UA_Boolean cleanup = !fetchMore && continuationAvail;
        UA_HistoryReadResponse response =
            __UA_Client_HistoryRead(client, nodeId, details, indexRange, timestampsToReturn, continuationPoint, cleanup);

        if(cleanup) {
            retval = response.responseHeader.serviceResult;
cleanup:    UA_HistoryReadResponse_clear(&response);
            UA_ByteString_clear(&continuationPoint);
            return retval;
        }

        retval = response.responseHeader.serviceResult;
        if(retval == UA_STATUSCODE_GOOD) {
            if(response.resultsSize == 1)
                retval = response.results[0].statusCode;
            else
                retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
        }
        if(!UA_StatusCode_isEqualTop(retval,UA_STATUSCODE_GOOD))
            goto cleanup;

        UA_HistoryReadResult *res = response.results;

        /* Clear old and check / store new continuation point */
        UA_ByteString_clear(&continuationPoint);
        UA_ByteString_copy(&res->continuationPoint, &continuationPoint);
        continuationAvail = !UA_ByteString_equal(&continuationPoint, &UA_BYTESTRING_NULL);

        /* Client callback with possibility to request further values */
        fetchMore = callback(client, nodeId, continuationAvail, &res->historyData, callbackContext);

        /* Regular cleanup */
        UA_HistoryReadResponse_clear(&response);
    } while(continuationAvail);

    return retval;
}

UA_StatusCode
UA_Client_HistoryRead_events(UA_Client *client, const UA_NodeId *nodeId,
                                const UA_HistoricalIteratorCallback callback,
                                UA_DateTime startTime, UA_DateTime endTime,
                                UA_String indexRange, const UA_EventFilter filter, UA_UInt32 numValuesPerNode,
                                UA_TimestampsToReturn timestampsToReturn, void *callbackContext) {

    UA_ReadEventDetails details;
    UA_ReadEventDetails_init(&details);
    details.filter = filter;

    // At least two of the following parameters must be set
    details.numValuesPerNode = numValuesPerNode; // 0 = return all / max server is capable of
    details.startTime = startTime;
    details.endTime = endTime;

    UA_ExtensionObject detailsExtensionObject;
    UA_ExtensionObject_init(&detailsExtensionObject);
    detailsExtensionObject.content.decoded.type = &UA_TYPES[UA_TYPES_READEVENTDETAILS];
    detailsExtensionObject.content.decoded.data = &details;
    detailsExtensionObject.encoding = UA_EXTENSIONOBJECT_DECODED;

    return __UA_Client_HistoryRead_service(client, nodeId, callback, &detailsExtensionObject,
                                              indexRange, timestampsToReturn, callbackContext);
}

static UA_StatusCode
__UA_Client_HistoryRead_service_rawMod(UA_Client *client, const UA_NodeId *nodeId,
                                          const UA_HistoricalIteratorCallback callback,
                                          UA_DateTime startTime,UA_DateTime endTime,
                                          UA_String indexRange, UA_Boolean returnBounds, UA_UInt32 numValuesPerNode,
                                          UA_Boolean readModified, UA_TimestampsToReturn timestampsToReturn,
                                          void *callbackContext) {

    UA_ReadRawModifiedDetails details;
    UA_ReadRawModifiedDetails_init(&details);
    details.isReadModified = readModified; // Return only modified values
    details.returnBounds = returnBounds;   // Return values pre / post given range

    // At least two of the following parameters must be set
    details.numValuesPerNode = numValuesPerNode;   // 0 = return all / max server is capable of
    details.startTime = startTime;
    details.endTime = endTime;

    UA_ExtensionObject detailsExtensionObject;
    UA_ExtensionObject_init(&detailsExtensionObject);
    detailsExtensionObject.content.decoded.type = &UA_TYPES[UA_TYPES_READRAWMODIFIEDDETAILS];
    detailsExtensionObject.content.decoded.data = &details;
    detailsExtensionObject.encoding = UA_EXTENSIONOBJECT_DECODED;

    return __UA_Client_HistoryRead_service(client, nodeId, callback,
                                              &detailsExtensionObject, indexRange,
                                              timestampsToReturn, callbackContext);
}

UA_StatusCode
UA_Client_HistoryRead_raw(UA_Client *client, const UA_NodeId *nodeId,
                             const UA_HistoricalIteratorCallback callback,
                             UA_DateTime startTime, UA_DateTime endTime,
                             UA_String indexRange, UA_Boolean returnBounds, UA_UInt32 numValuesPerNode,
                             UA_TimestampsToReturn timestampsToReturn, void *callbackContext) {

    return __UA_Client_HistoryRead_service_rawMod(client, nodeId, callback, startTime, endTime, indexRange, returnBounds,
                                                     numValuesPerNode, false, timestampsToReturn, callbackContext);
}

UA_StatusCode
UA_Client_HistoryRead_modified(UA_Client *client, const UA_NodeId *nodeId,
                                  const UA_HistoricalIteratorCallback callback,
                                  UA_DateTime startTime, UA_DateTime endTime,
                                  UA_String indexRange, UA_Boolean returnBounds, UA_UInt32 maxItems,
                                  UA_TimestampsToReturn timestampsToReturn, void *callbackContext) {
    return __UA_Client_HistoryRead_service_rawMod(client, nodeId, callback, startTime,
                                                  endTime, indexRange, returnBounds, maxItems,
                                                  true, timestampsToReturn, callbackContext);
}

static UA_HistoryUpdateResponse
__UA_Client_HistoryUpdate(UA_Client *client, void *details, size_t typeId) {
    UA_HistoryUpdateRequest request;
    UA_HistoryUpdateRequest_init(&request);

    UA_ExtensionObject extension;
    UA_ExtensionObject_init(&extension);
    request.historyUpdateDetailsSize = 1;
    request.historyUpdateDetails = &extension;

    extension.encoding = UA_EXTENSIONOBJECT_DECODED;
    extension.content.decoded.type = &UA_TYPES[typeId];
    extension.content.decoded.data = details;

    UA_HistoryUpdateResponse response;
    response = UA_Client_Service_historyUpdate(client, request);
    return response;
}

static UA_StatusCode
__UA_Client_HistoryUpdate_updateData(UA_Client *client, const UA_NodeId *nodeId,
                                     UA_PerformUpdateType type, UA_DataValue *value) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_UpdateDataDetails details;
    UA_UpdateDataDetails_init(&details);

    details.performInsertReplace = type;
    details.updateValuesSize = 1;
    details.updateValues = value;
    UA_NodeId_copy(nodeId, &details.nodeId);

    UA_HistoryUpdateResponse response;
    response = __UA_Client_HistoryUpdate(client, &details, UA_TYPES_UPDATEDATADETAILS);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        ret = response.responseHeader.serviceResult;
        goto cleanup;
    }
    if(response.resultsSize != 1 || response.results[0].operationResultsSize != 1) {
        ret = UA_STATUSCODE_BADUNEXPECTEDERROR;
        goto cleanup;
    }
    if(response.results[0].statusCode != UA_STATUSCODE_GOOD) {
        ret = response.results[0].statusCode;
        goto cleanup;
    }
    ret = response.results[0].operationResults[0];
cleanup:
    UA_HistoryUpdateResponse_clear(&response);
    UA_NodeId_clear(&details.nodeId);
    return ret;
}

UA_StatusCode
UA_Client_HistoryUpdate_insert(UA_Client *client, const UA_NodeId *nodeId,
                               UA_DataValue *value) {
    return __UA_Client_HistoryUpdate_updateData(client, nodeId,
                                                UA_PERFORMUPDATETYPE_INSERT,
                                                value);
}

UA_StatusCode
UA_Client_HistoryUpdate_replace(UA_Client *client, const UA_NodeId *nodeId,
                                UA_DataValue *value) {
    return __UA_Client_HistoryUpdate_updateData(client, nodeId,
                                                UA_PERFORMUPDATETYPE_REPLACE,
                                                value);
}

UA_StatusCode
UA_Client_HistoryUpdate_update(UA_Client *client, const UA_NodeId *nodeId,
                               UA_DataValue *value) {
    return __UA_Client_HistoryUpdate_updateData(client, nodeId,
                                                UA_PERFORMUPDATETYPE_UPDATE,
                                                value);
}

UA_StatusCode
UA_Client_HistoryUpdate_deleteRaw(UA_Client *client, const UA_NodeId *nodeId,
                                  UA_DateTime startTimestamp, UA_DateTime endTimestamp) {
    UA_DeleteRawModifiedDetails details;
    UA_DeleteRawModifiedDetails_init(&details);
    details.isDeleteModified = false;
    details.startTime = startTimestamp;
    details.endTime = endTimestamp;
    UA_NodeId_copy(nodeId, &details.nodeId);

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_HistoryUpdateResponse response;
    response = __UA_Client_HistoryUpdate(client, &details, UA_TYPES_DELETERAWMODIFIEDDETAILS);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        ret = response.responseHeader.serviceResult;
        goto cleanup;
    }
    if(response.resultsSize != 1) {
        ret = UA_STATUSCODE_BADUNEXPECTEDERROR;
        goto cleanup;
    }

    ret = response.results[0].statusCode;

cleanup:
    UA_HistoryUpdateResponse_clear(&response);
    UA_NodeId_clear(&details.nodeId);
    return ret;
}

/*******************/
/* Async Functions */
/*******************/

static UA_StatusCode
__UA_Client_writeAttribute_async(UA_Client *client, const UA_NodeId *nodeId,
                                 UA_AttributeId attributeId, const void *in,
                                 const UA_DataType *inDataType,
                                 UA_ClientAsyncServiceCallback callback,
                                 void *userdata, UA_UInt32 *reqId) {
    if(!in)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    wValue.nodeId = *nodeId;
    wValue.attributeId = attributeId;
    if(attributeId == UA_ATTRIBUTEID_VALUE)
        wValue.value.value = *(const UA_Variant*) in;
    else
        /* hack. is never written into. */
        UA_Variant_setScalar(&wValue.value.value, (void*) (uintptr_t) in,
                inDataType);
    wValue.value.hasValue = true;
    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = &wValue;
    wReq.nodesToWriteSize = 1;

    return __UA_Client_AsyncService(client, &wReq,
                                    &UA_TYPES[UA_TYPES_WRITEREQUEST], callback,
                                    &UA_TYPES[UA_TYPES_WRITERESPONSE], userdata, reqId);
}

UA_StatusCode
UA_Client_call_async(UA_Client *client, const UA_NodeId objectId,
                     const UA_NodeId methodId, size_t inputSize,
                     const UA_Variant *input,
                     UA_ClientAsyncCallCallback callback,
                     void *userdata, UA_UInt32 *reqId) {
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    UA_CallMethodRequest item;
    UA_CallMethodRequest_init(&item);
    item.objectId = objectId;
    item.methodId = methodId;
    item.inputArguments = (UA_Variant *)(uintptr_t)input; /* cast const */
    item.inputArgumentsSize = inputSize;
    request.methodsToCall = &item;
    request.methodsToCallSize = 1;
    return __UA_Client_AsyncService(client, &request,
            &UA_TYPES[UA_TYPES_CALLREQUEST], (UA_ClientAsyncServiceCallback)callback,
            &UA_TYPES[UA_TYPES_CALLRESPONSE], userdata, reqId);
}

/*************************/
/* Read Single Attribute */
/*************************/

typedef struct {
    UA_ClientAsyncOperationCallback userCallback;
    void *userContext;
    const UA_DataType *resultType; /* DataValue -> Value attribute,
                                    * Variant -> ArrayDimensions attribute */
} UA_AttributeReadContext;

static void
AttributeReadCallback(UA_Client *client, void *userdata,
                      UA_UInt32 requestId, UA_ReadResponse *rr) {
    UA_AttributeReadContext *ctx = (UA_AttributeReadContext*)userdata;
    UA_LOG_DEBUG(UA_Client_getConfig(client)->logging, UA_LOGCATEGORY_CLIENT,
                "Async read response for request %" PRIu32, requestId);

    UA_DataValue *dv = NULL;

    /* Check the ServiceResult */
    UA_StatusCode res = rr->responseHeader.serviceResult;
    if(res != UA_STATUSCODE_GOOD)
        goto finish;

    /* Check result array size */
    if(rr->resultsSize != 1) {
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto finish;
    }

    /* A Value attribute */
    dv = &rr->results[0];
    if(ctx->resultType == &UA_TYPES[UA_TYPES_DATAVALUE]) {
        ctx->userCallback(client, ctx->userContext, requestId,
                          UA_STATUSCODE_GOOD, dv);
        goto finish;
    }

    /* An ArrayDimensions attribute. Has to be an array of UInt32. */
    if(ctx->resultType == &UA_TYPES[UA_TYPES_VARIANT]) {
        if(dv->hasValue &&
           UA_Variant_hasArrayType(&dv->value, &UA_TYPES[UA_TYPES_UINT32])) {
            ctx->userCallback(client, ctx->userContext, requestId,
                              UA_STATUSCODE_GOOD, &dv->value);
        } else {
            res = UA_STATUSCODE_BADINTERNALERROR;
        }
        goto finish;
    }

    /* Check we have a value */
    if(!dv->hasValue) {
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto finish;
    }

    /* Check the type. Try to adjust "in situ" if no match. */
    if(!UA_Variant_hasScalarType(&dv->value, ctx->resultType)) {
        /* Remember the old pointer, adjustType can "unwrap" a type but won't
         * free the wrapper. Because the server code still keeps the wrapper. */
        void *oldVal = dv->value.data;
        adjustType(&dv->value, ctx->resultType);
        if(dv->value.data != oldVal)
            UA_free(oldVal);
        if(!UA_Variant_hasScalarType(&dv->value, ctx->resultType)) {
            res = UA_STATUSCODE_BADINTERNALERROR;
            goto finish;
        }
    }

    /* Callback into userland */
    ctx->userCallback(client, ctx->userContext, requestId,
                      UA_STATUSCODE_GOOD, dv->value.data);

 finish:
    if(res != UA_STATUSCODE_GOOD)
        ctx->userCallback(client, ctx->userContext, requestId, res, NULL);
    UA_free(ctx);
}

static UA_StatusCode
readAttribute_async(UA_Client *client, const UA_ReadValueId *rvi,
                    UA_TimestampsToReturn timestampsToReturn,
                    const UA_DataType *resultType, /* For the specialized reads */
                    UA_ClientAsyncOperationCallback callback,
                    void *userdata, UA_UInt32 *requestId) {
    UA_AttributeReadContext *ctx = (UA_AttributeReadContext*)
        UA_malloc(sizeof(UA_AttributeReadContext));
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    ctx->userCallback = callback;
    ctx->userContext = userdata;
    ctx->resultType = resultType;

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = (UA_ReadValueId*)(uintptr_t)rvi; /* hack, treated as const */
    request.nodesToReadSize = 1;
    request.timestampsToReturn = timestampsToReturn;

    UA_StatusCode res =
        __UA_Client_AsyncService(client, &request, &UA_TYPES[UA_TYPES_READREQUEST],
                                 (UA_ClientAsyncServiceCallback)AttributeReadCallback,
                                 &UA_TYPES[UA_TYPES_READRESPONSE], ctx, requestId);
    if(res != UA_STATUSCODE_GOOD)
        UA_free(ctx);
    return res;
}

UA_StatusCode
UA_Client_readAttribute_async(UA_Client *client, const UA_ReadValueId *rvi,
                              UA_TimestampsToReturn timestampsToReturn,
                              UA_ClientAsyncReadAttributeCallback callback,
                              void *userdata, UA_UInt32 *requestId) {
    return readAttribute_async(client, rvi, timestampsToReturn,
                               &UA_TYPES[UA_TYPES_DATAVALUE], /* special handling */
                               (UA_ClientAsyncOperationCallback)callback,
                               userdata, requestId);
}

/* Helper to keep the code short */
static UA_StatusCode
readAttribute_simpleAsync(UA_Client *client, const UA_NodeId *nodeId,
                          UA_AttributeId attributeId, const UA_DataType *resultType,
                          UA_ClientAsyncOperationCallback callback,
                          void *userdata, UA_UInt32 *requestId) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = *nodeId;
    rvi.attributeId = attributeId;
    return readAttribute_async(client, &rvi, UA_TIMESTAMPSTORETURN_NEITHER,
                               resultType, callback, userdata, requestId);
}

UA_StatusCode
UA_Client_readValueAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                   UA_ClientAsyncReadValueAttributeCallback callback,
                                   void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_VALUE,
                                     &UA_TYPES[UA_TYPES_DATAVALUE], /* special hndling */
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readDataTypeAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                      UA_ClientAsyncReadDataTypeAttributeCallback callback,
                                      void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_DATATYPE,
                                     &UA_TYPES[UA_TYPES_NODEID],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readArrayDimensionsAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                             UA_ClientReadArrayDimensionsAttributeCallback callback,
                                             void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_ARRAYDIMENSIONS,
                                     &UA_TYPES[UA_TYPES_VARIANT], /* special handling */
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readNodeClassAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                       UA_ClientAsyncReadNodeClassAttributeCallback callback,
                                       void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_NODECLASS,
                                     &UA_TYPES[UA_TYPES_NODECLASS],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readBrowseNameAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                        UA_ClientAsyncReadBrowseNameAttributeCallback callback,
                                        void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_BROWSENAME,
                                     &UA_TYPES[UA_TYPES_QUALIFIEDNAME],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readDisplayNameAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                         UA_ClientAsyncReadDisplayNameAttributeCallback callback,
                                         void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_DISPLAYNAME,
                                     &UA_TYPES[UA_TYPES_LOCALIZEDTEXT],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readDescriptionAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                         UA_ClientAsyncReadDescriptionAttributeCallback callback,
                                         void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_DESCRIPTION,
                                     &UA_TYPES[UA_TYPES_LOCALIZEDTEXT],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readWriteMaskAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                       UA_ClientAsyncReadWriteMaskAttributeCallback callback,
                                       void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_WRITEMASK,
                                     &UA_TYPES[UA_TYPES_UINT32],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode UA_EXPORT
UA_Client_readUserWriteMaskAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                           UA_ClientAsyncReadUserWriteMaskAttributeCallback callback,
                                           void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_USERWRITEMASK,
                                     &UA_TYPES[UA_TYPES_UINT32],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readIsAbstractAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                        UA_ClientAsyncReadIsAbstractAttributeCallback callback,
                                        void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_ISABSTRACT,
                                     &UA_TYPES[UA_TYPES_BOOLEAN],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readSymmetricAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                       UA_ClientAsyncReadSymmetricAttributeCallback callback,
                                       void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_SYMMETRIC,
                                     &UA_TYPES[UA_TYPES_BOOLEAN],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readInverseNameAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                         UA_ClientAsyncReadInverseNameAttributeCallback callback,
                                         void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_INVERSENAME,
                                     &UA_TYPES[UA_TYPES_LOCALIZEDTEXT],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readContainsNoLoopsAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                             UA_ClientAsyncReadContainsNoLoopsAttributeCallback callback,
                                             void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_CONTAINSNOLOOPS,
                                     &UA_TYPES[UA_TYPES_BOOLEAN],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readEventNotifierAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                           UA_ClientAsyncReadEventNotifierAttributeCallback callback,
                                           void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_EVENTNOTIFIER,
                                     &UA_TYPES[UA_TYPES_BYTE],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readValueRankAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                       UA_ClientAsyncReadValueRankAttributeCallback callback,
                                       void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_VALUERANK,
                                     &UA_TYPES[UA_TYPES_INT32],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readAccessLevelAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                         UA_ClientAsyncReadAccessLevelAttributeCallback callback,
                                         void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_ACCESSLEVEL,
                                     &UA_TYPES[UA_TYPES_BYTE],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readAccessLevelExAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                           UA_ClientAsyncReadAccessLevelExAttributeCallback callback,
                                           void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_ACCESSLEVELEX,
                                     &UA_TYPES[UA_TYPES_UINT32],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readUserAccessLevelAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                             UA_ClientAsyncReadUserAccessLevelAttributeCallback callback,
                                             void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_USERACCESSLEVEL,
                                     &UA_TYPES[UA_TYPES_BYTE],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readMinimumSamplingIntervalAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                                     UA_ClientAsyncReadMinimumSamplingIntervalAttributeCallback callback,
                                                     void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL,
                                     &UA_TYPES[UA_TYPES_DOUBLE],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readHistorizingAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                         UA_ClientAsyncReadHistorizingAttributeCallback callback,
                                         void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_HISTORIZING,
                                     &UA_TYPES[UA_TYPES_BOOLEAN],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readExecutableAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                        UA_ClientAsyncReadExecutableAttributeCallback callback,
                                        void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_EXECUTABLE,
                                     &UA_TYPES[UA_TYPES_BOOLEAN],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

UA_StatusCode
UA_Client_readUserExecutableAttribute_async(UA_Client *client, const UA_NodeId nodeId,
                                            UA_ClientAsyncReadUserExecutableAttributeCallback callback,
                                            void *userdata, UA_UInt32 *requestId) {
    return readAttribute_simpleAsync(client, &nodeId, UA_ATTRIBUTEID_USEREXECUTABLE,
                                     &UA_TYPES[UA_TYPES_BOOLEAN],
                                     (UA_ClientAsyncOperationCallback)callback,
                                     userdata, requestId);
}

static UA_StatusCode
__UA_Client_addNode_async(UA_Client *client, const UA_NodeClass nodeClass,
                          const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId, const UA_QualifiedName browseName,
                          const UA_NodeId typeDefinition, const UA_NodeAttributes *attr,
                          const UA_DataType *attributeType, UA_NodeId *outNewNodeId,
                          UA_ClientAsyncServiceCallback callback, void *userdata,
                          UA_UInt32 *reqId) {
    UA_AddNodesRequest request;
    UA_AddNodesRequest_init(&request);
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.parentNodeId.nodeId = parentNodeId;
    item.referenceTypeId = referenceTypeId;
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    item.nodeClass = nodeClass;
    item.typeDefinition.nodeId = typeDefinition;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type = attributeType;
    item.nodeAttributes.content.decoded.data = (void*) (uintptr_t) attr; // hack. is not written into.
    request.nodesToAdd = &item;
    request.nodesToAddSize = 1;

    return __UA_Client_AsyncService(client, &request,
            &UA_TYPES[UA_TYPES_ADDNODESREQUEST], callback,
            &UA_TYPES[UA_TYPES_ADDNODESRESPONSE], userdata, reqId);

}

UA_StatusCode
UA_Client_addVariableNode_async(UA_Client *client, const UA_NodeId requestedNewNodeId,
                                const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
                                const UA_VariableAttributes attr, UA_NodeId *outNewNodeId,
                                UA_ClientAsyncAddNodesCallback callback, void *userdata,
                                UA_UInt32 *reqId) {
    return __UA_Client_addNode_async(client, UA_NODECLASS_VARIABLE, requestedNewNodeId, parentNodeId,
                                     referenceTypeId, browseName, typeDefinition, (const UA_NodeAttributes *)&attr,
                                     &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES], outNewNodeId,
                                     (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
}

UA_StatusCode
UA_Client_addVariableTypeNode_async(UA_Client *client, const UA_NodeId requestedNewNodeId,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                    const UA_QualifiedName browseName, const UA_VariableTypeAttributes attr,
                                    UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
                                    void *userdata, UA_UInt32 *reqId) {
    return __UA_Client_addNode_async(client, UA_NODECLASS_VARIABLETYPE, requestedNewNodeId, parentNodeId,
                                     referenceTypeId, browseName, UA_NODEID_NULL, (const UA_NodeAttributes *)&attr,
                                     &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES], outNewNodeId,
                                     (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
}

UA_StatusCode
UA_Client_addObjectNode_async(UA_Client *client, const UA_NodeId requestedNewNodeId,
                              const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                              const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
                              const UA_ObjectAttributes attr, UA_NodeId *outNewNodeId,
                              UA_ClientAsyncAddNodesCallback callback, void *userdata,
                              UA_UInt32 *reqId) {
    return __UA_Client_addNode_async(client, UA_NODECLASS_OBJECT, requestedNewNodeId, parentNodeId,
                                     referenceTypeId, browseName, typeDefinition, (const UA_NodeAttributes *)&attr,
                                     &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], outNewNodeId,
                                     (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
}

UA_StatusCode
UA_Client_addObjectTypeNode_async(UA_Client *client, const UA_NodeId requestedNewNodeId,
                                  const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                  const UA_QualifiedName browseName, const UA_ObjectTypeAttributes attr,
                                  UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
                                  void *userdata, UA_UInt32 *reqId) {
    return __UA_Client_addNode_async(client, UA_NODECLASS_OBJECTTYPE, requestedNewNodeId, parentNodeId,
                                     referenceTypeId, browseName, UA_NODEID_NULL, (const UA_NodeAttributes *)&attr,
                                     &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES], outNewNodeId,
                                     (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
}

UA_StatusCode
UA_Client_addViewNode_async(UA_Client *client, const UA_NodeId requestedNewNodeId,
                            const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                            const UA_QualifiedName browseName, const UA_ViewAttributes attr,
                            UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
                            void *userdata, UA_UInt32 *reqId) {
    return __UA_Client_addNode_async(client, UA_NODECLASS_VIEW, requestedNewNodeId, parentNodeId,
                                     referenceTypeId, browseName, UA_NODEID_NULL, (const UA_NodeAttributes *)&attr,
                                     &UA_TYPES[UA_TYPES_VIEWATTRIBUTES], outNewNodeId,
                                     (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
}

UA_StatusCode
UA_Client_addReferenceTypeNode_async(UA_Client *client, const UA_NodeId requestedNewNodeId,
                                     const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                     const UA_QualifiedName browseName, const UA_ReferenceTypeAttributes attr,
                                     UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
                                     void *userdata, UA_UInt32 *reqId) {
    return __UA_Client_addNode_async(client, UA_NODECLASS_REFERENCETYPE, requestedNewNodeId, parentNodeId,
                                     referenceTypeId, browseName, UA_NODEID_NULL,
                                     (const UA_NodeAttributes *)&attr,
                                     &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES], outNewNodeId,
                                     (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
}

UA_StatusCode
UA_Client_addDataTypeNode_async(UA_Client *client, const UA_NodeId requestedNewNodeId,
                                const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                const UA_QualifiedName browseName, const UA_DataTypeAttributes attr,
                                UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
                                void *userdata, UA_UInt32 *reqId) {
    return __UA_Client_addNode_async(client, UA_NODECLASS_DATATYPE, requestedNewNodeId, parentNodeId,
                                     referenceTypeId, browseName, UA_NODEID_NULL, (const UA_NodeAttributes *)&attr,
                                     &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES], outNewNodeId,
                                     (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
}

UA_StatusCode
UA_Client_addMethodNode_async(UA_Client *client, const UA_NodeId requestedNewNodeId,
                              const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                              const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                              UA_NodeId *outNewNodeId, UA_ClientAsyncAddNodesCallback callback,
                              void *userdata, UA_UInt32 *reqId) {
    return __UA_Client_addNode_async(client, UA_NODECLASS_METHOD, requestedNewNodeId, parentNodeId,
                                     referenceTypeId, browseName, UA_NODEID_NULL, (const UA_NodeAttributes *)&attr,
                                     &UA_TYPES[UA_TYPES_METHODATTRIBUTES], outNewNodeId,
                                     (UA_ClientAsyncServiceCallback)callback, userdata, reqId);
}

#define UA_CLIENT_ASYNCWRITE_IMPL(NAME, ATTR_ID, ATTR_TYPE, ATTR_TYPEDESC)                               \
    UA_StatusCode NAME(UA_Client *client, const UA_NodeId nodeId,                                        \
                       const ATTR_TYPE *attr, UA_ClientAsyncWriteCallback callback,                      \
                       void *userdata, UA_UInt32 *reqId) {                                               \
        return __UA_Client_writeAttribute_async(client, &nodeId, UA_ATTRIBUTEID_##ATTR_ID, attr,           \
                                                &UA_TYPES[UA_TYPES_##ATTR_TYPEDESC], \
                                                (UA_ClientAsyncServiceCallback)callback, userdata, reqId); \
}

UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeNodeIdAttribute_async, NODEID, UA_NodeId, NODEID)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeNodeClassAttribute_async, NODECLASS, UA_NodeClass, NODECLASS)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeBrowseNameAttribute_async, BROWSENAME, UA_QualifiedName, QUALIFIEDNAME)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeDisplayNameAttribute_async, DISPLAYNAME, UA_LocalizedText, LOCALIZEDTEXT)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeDescriptionAttribute_async, DESCRIPTION, UA_LocalizedText, LOCALIZEDTEXT)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeWriteMaskAttribute_async, WRITEMASK, UA_UInt32, UINT32)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeIsAbstractAttribute_async, ISABSTRACT, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeSymmetricAttribute_async, SYMMETRIC, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeInverseNameAttribute_async, INVERSENAME, UA_LocalizedText, LOCALIZEDTEXT)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeContainsNoLoopsAttribute_async, CONTAINSNOLOOPS, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeEventNotifierAttribute_async, EVENTNOTIFIER, UA_Byte, BYTE)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeValueAttribute_async, VALUE, UA_Variant, VARIANT)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeDataTypeAttribute_async, DATATYPE, UA_NodeId, NODEID)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeValueRankAttribute_async, VALUERANK, UA_Int32, INT32)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeAccessLevelAttribute_async, ACCESSLEVEL, UA_Byte, BYTE)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeMinimumSamplingIntervalAttribute_async, MINIMUMSAMPLINGINTERVAL, UA_Double, DOUBLE)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeHistorizingAttribute_async, HISTORIZING, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeExecutableAttribute_async, EXECUTABLE, UA_Boolean, BOOLEAN)
UA_CLIENT_ASYNCWRITE_IMPL(UA_Client_writeAccessLevelExAttribute_async, ACCESSLEVELEX, UA_UInt32, UINT32)
