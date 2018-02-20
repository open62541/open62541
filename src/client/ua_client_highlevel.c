/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2015-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Florian Palm
 *    Copyright 2016 (c) Chris Iatrou
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_client.h"
#include "ua_client_internal.h"
#include "ua_client_highlevel.h"
#include "ua_client_highlevel_async.h"
#include "ua_util.h"

UA_StatusCode
UA_Client_NamespaceGetIndex(UA_Client *client, UA_String *namespaceUri,
                            UA_UInt16 *namespaceIndex) {
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId id;
    UA_ReadValueId_init(&id);
    id.attributeId = UA_ATTRIBUTEID_VALUE;
    id.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY);
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
        UA_ReadResponse_deleteMembers(&response);
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

    UA_ReadResponse_deleteMembers(&response);
    return retval;
}

UA_StatusCode
UA_Client_forEachChildNodeCall(UA_Client *client, UA_NodeId parentNodeId,
                               UA_NodeIteratorCallback callback, void *handle) {
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
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

    UA_BrowseRequest_deleteMembers(&bReq);
    UA_BrowseResponse_deleteMembers(&bResp);
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
        UA_AddReferencesResponse_deleteMembers(&response);
        return retval;
    }
    if(response.resultsSize != 1) {
        UA_AddReferencesResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    retval = response.results[0];
    UA_AddReferencesResponse_deleteMembers(&response);
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
        UA_DeleteReferencesResponse_deleteMembers(&response);
        return retval;
    }
    if(response.resultsSize != 1) {
        UA_DeleteReferencesResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    retval = response.results[0];
    UA_DeleteReferencesResponse_deleteMembers(&response);
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
        UA_DeleteNodesResponse_deleteMembers(&response);
        return retval;
    }
    if(response.resultsSize != 1) {
        UA_DeleteNodesResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    retval = response.results[0];
    UA_DeleteNodesResponse_deleteMembers(&response);
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
        UA_AddNodesResponse_deleteMembers(&response);
        return retval;
    }

    if(response.resultsSize != 1) {
        UA_AddNodesResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    /* Move the id of the created node */
    retval = response.results[0].statusCode;
    if(retval == UA_STATUSCODE_GOOD && outNewNodeId) {
        *outNewNodeId = response.results[0].addedNodeId;
        UA_NodeId_init(&response.results[0].addedNodeId);
    }

    UA_AddNodesResponse_deleteMembers(&response);
    return retval;
}

/********/
/* Call */
/********/

#ifdef UA_ENABLE_METHODCALLS

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
    if(retval != UA_STATUSCODE_GOOD) {
        UA_CallResponse_deleteMembers(&response);
        return retval;
    }

    /* Move the output arguments */
    if(output != NULL && outputSize != NULL) {
        *output = response.results[0].outputArguments;
        *outputSize = response.results[0].outputArgumentsSize;
        response.results[0].outputArguments = NULL;
        response.results[0].outputArgumentsSize = 0;
    }
    UA_CallResponse_deleteMembers(&response);
    return retval;
}

#endif

/********************/
/* Write Attributes */
/********************/

UA_StatusCode
__UA_Client_writeAttribute(UA_Client *client, const UA_NodeId *nodeId,
                           UA_AttributeId attributeId, const void *in,
                           const UA_DataType *inDataType) {
    if(!in)
      return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    wValue.nodeId = *nodeId;
    wValue.attributeId = attributeId;
    if(attributeId == UA_ATTRIBUTEID_VALUE)
        wValue.value.value = *(const UA_Variant*)in;
    else
        /* hack. is never written into. */
        UA_Variant_setScalar(&wValue.value.value, (void*)(uintptr_t)in, inDataType);
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

    UA_WriteResponse_deleteMembers(&wResp);
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
    UA_WriteResponse_deleteMembers(&wResp);
    return retval;
}

/*******************/
/* Read Attributes */
/*******************/

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
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_deleteMembers(&response);
        return retval;
    }

    /* Set the StatusCode */
    UA_DataValue *res = response.results;
    if(res->hasStatus)
        retval = res->status;

    /* Return early of no value is given */
    if(!res->hasValue) {
        if(retval == UA_STATUSCODE_GOOD)
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
        UA_ReadResponse_deleteMembers(&response);
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

    UA_ReadResponse_deleteMembers(&response);
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
    if(retval != UA_STATUSCODE_GOOD)
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
    UA_ReadResponse_deleteMembers(&response);
    return retval;
}

/* Async Functions */

static
void ValueAttributeRead(UA_Client *client, void *userdata,
                        UA_UInt32 requestId, void *response) {
    if(!response)
        return;

    /* Find the callback for the response */
    CustomCallback *cc;
    LIST_FOREACH(cc, &client->customCallbacks, pointers) {
        if(cc->callbackId == requestId)
            break;
    }
    if(!cc)
        return;

    UA_ReadResponse *rr = (UA_ReadResponse *) response;
    UA_DataValue *res = rr->results;
    UA_Boolean done = false;
    if(rr->resultsSize == 1 && res != NULL && res->hasValue) {
        if(cc->attributeId == UA_ATTRIBUTEID_VALUE) {
            /* Call directly with the variant */
            cc->callback(client, userdata, requestId, &res->value);
            done = true;
        } else if(UA_Variant_isScalar(&res->value) &&
                  res->value.type == cc->outDataType) {
            /* Unpack the value */
            UA_STACKARRAY(UA_Byte, value, cc->outDataType->memSize);
            memcpy(&value, res->value.data, cc->outDataType->memSize);
            cc->callback(client, userdata, requestId, &value);
            done = true;
        }
    }

    /* Could not process, delete the callback anyway */
    if(!done)
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Cannot process the response to the async read "
                    "request %u", requestId);

    LIST_REMOVE(cc, pointers);
    UA_free(cc);
}

/*Read Attributes*/
UA_StatusCode __UA_Client_readAttribute_async(UA_Client *client,
        const UA_NodeId *nodeId, UA_AttributeId attributeId,
        const UA_DataType *outDataType, UA_ClientAsyncServiceCallback callback,
        void *userdata, UA_UInt32 *reqId) {
    UA_ReadValueId item;
    UA_ReadValueId_init(&item);
    item.nodeId = *nodeId;
    item.attributeId = attributeId;
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = &item;
    request.nodesToReadSize = 1;

    __UA_Client_AsyncService(client, &request, &UA_TYPES[UA_TYPES_READREQUEST],
                             ValueAttributeRead, &UA_TYPES[UA_TYPES_READRESPONSE],
                             userdata, reqId);

    CustomCallback *cc = (CustomCallback*) UA_malloc(sizeof(CustomCallback));
    if (!cc)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    cc->callback = callback;
    cc->callbackId = *reqId;

    cc->attributeId = attributeId;
    cc->outDataType = outDataType;

    LIST_INSERT_HEAD(&client->customCallbacks, cc, pointers);

    return UA_STATUSCODE_GOOD;
}

/*Write Attributes*/
UA_StatusCode __UA_Client_writeAttribute_async(UA_Client *client,
        const UA_NodeId *nodeId, UA_AttributeId attributeId, const void *in,
        const UA_DataType *inDataType, UA_ClientAsyncServiceCallback callback,
        void *userdata, UA_UInt32 *reqId) {
    if (!in)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    wValue.nodeId = *nodeId;
    wValue.attributeId = attributeId;
    if (attributeId == UA_ATTRIBUTEID_VALUE)
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

/*Node Management*/

UA_StatusCode UA_EXPORT
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

/* Misc Highlevel Functions */
#ifdef UA_ENABLE_METHODCALLS
UA_StatusCode __UA_Client_call_async(UA_Client *client,
        const UA_NodeId objectId, const UA_NodeId methodId, size_t inputSize,
        const UA_Variant *input, UA_ClientAsyncServiceCallback callback,
        void *userdata, UA_UInt32 *reqId) {

    UA_CallRequest request;
    UA_CallRequest_init(&request);
    UA_CallMethodRequest item;
    UA_CallMethodRequest_init(&item);
    item.methodId = methodId;
    item.objectId = objectId;
    item.inputArguments = (UA_Variant *) (void*) (uintptr_t) input; // cast const...
    item.inputArgumentsSize = inputSize;
    request.methodsToCall = &item;
    request.methodsToCallSize = 1;

    return __UA_Client_AsyncService(client, &request,
            &UA_TYPES[UA_TYPES_CALLREQUEST], callback,
            &UA_TYPES[UA_TYPES_CALLRESPONSE], userdata, reqId);
}
#endif

UA_StatusCode __UA_Client_translateBrowsePathsToNodeIds_async(UA_Client *client,
        char *paths[], UA_UInt32 ids[], size_t pathSize,
        UA_ClientAsyncServiceCallback callback, void *userdata,
        UA_UInt32 *reqId) {

    UA_BrowsePath browsePath;
    UA_BrowsePath_init(&browsePath);
    browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    browsePath.relativePath.elements = (UA_RelativePathElement*) UA_Array_new(
            pathSize, &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
    if (!browsePath.relativePath.elements)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    browsePath.relativePath.elementsSize = pathSize;

    UA_TranslateBrowsePathsToNodeIdsRequest request;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
    request.browsePaths = &browsePath;
    request.browsePathsSize = 1;

    UA_StatusCode retval = __UA_Client_AsyncService(client, &request,
            &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST], callback,
            &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE], userdata,
            reqId);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(browsePath.relativePath.elements,
                browsePath.relativePath.elementsSize,
                &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
        return retval;
    }
    UA_BrowsePath_deleteMembers(&browsePath);
    return retval;
}

/*********************/
/* Historical Access */
/*********************/

// FIXME: Function for ReadProcessedDetails is missing
UA_HistoryReadResponse
__UA_Client_readHistorical(UA_Client *client, const UA_NodeId nodeId,
                           void* details, const UA_TimestampsToReturn timestampsToReturn,
                           const UA_ByteString continuationPoint, const UA_Boolean releaseConti) {

    UA_HistoryReadValueId item;
    UA_HistoryReadValueId_init(&item);

    item.nodeId = nodeId;
    item.indexRange = UA_STRING_NULL; // TODO: NumericRange (?)
    item.continuationPoint = continuationPoint;
    item.dataEncoding = UA_QUALIFIEDNAME(0, "Default Binary");

    UA_HistoryReadRequest request;
    UA_HistoryReadRequest_init(&request);

    request.nodesToRead = &item;
    request.nodesToReadSize = 1;
    request.timestampsToReturn = timestampsToReturn; // Defaults to Source
    request.releaseContinuationPoints = releaseConti; // No values are returned, if true

    /* Build ReadDetails */
    request.historyReadDetails.content.decoded.type = &UA_TYPES[UA_TYPES_READRAWMODIFIEDDETAILS];
    request.historyReadDetails.content.decoded.data = details;
    request.historyReadDetails.encoding = UA_EXTENSIONOBJECT_DECODED;

    return UA_Client_Service_history_read(client, request);
}

UA_StatusCode
__UA_Client_readHistorical_service(UA_Client *client, const UA_NodeId nodeId,
                                   const UA_HistoricalIteratorCallback callback,
                                   void *details, const UA_TimestampsToReturn timestampsToReturn,
                                   UA_Boolean isInverse, UA_UInt32 maxItems, void *handle) {

    UA_ByteString continuationPoint = UA_BYTESTRING_NULL;
    UA_Boolean continuationAvail = UA_FALSE;
    UA_Boolean fetchMore = UA_FALSE;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    do {
        /* We release the continuation point, if no more data is requested by the user */
        UA_Boolean cleanup = !fetchMore && continuationAvail;
        UA_HistoryReadResponse response =
            __UA_Client_readHistorical(client, nodeId, details, timestampsToReturn, continuationPoint, cleanup);

        if (cleanup) {
            retval = response.responseHeader.serviceResult;
cleanup:    UA_HistoryReadResponse_deleteMembers(&response);
            return retval;
        }

        retval = response.responseHeader.serviceResult;
        if (retval == UA_STATUSCODE_GOOD) {
            if (response.resultsSize == 1)
                retval = response.results[0].statusCode;
            else
                retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
        }
        if (retval != UA_STATUSCODE_GOOD)
            goto cleanup;

        UA_HistoryReadResult *res = response.results;
        UA_HistoryData *data = (UA_HistoryData*)res->historyData.content.decoded.data;

        /* We should never receive more values, than requested */
        if (maxItems && data->dataValuesSize > maxItems) {
            retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
            goto cleanup;
        }

        /* Clear old and check / store new continuation point */
        UA_ByteString_deleteMembers(&continuationPoint);
        UA_ByteString_copy(&res->continuationPoint, &continuationPoint);
        continuationAvail = !UA_ByteString_equal(&continuationPoint, &UA_BYTESTRING_NULL);

        /* Client callback with posibility to request further values */
        fetchMore = callback(nodeId, isInverse, continuationAvail, data, handle);

        /* Regular cleanup */
        UA_HistoryReadResponse_deleteMembers(&response);
    } while (continuationAvail);

    return retval;
}

UA_StatusCode
UA_Client_readHistorical_events(UA_Client *client, const UA_NodeId nodeId,
                                const UA_HistoricalIteratorCallback callback,
                                const UA_DateTime startTime, const UA_DateTime endTime,
                                const UA_EventFilter filter, const UA_UInt32 maxItems,
                                const UA_TimestampsToReturn timestampsToReturn, void* handle) {

    // TODO: ReadProcessedDetails / ReadAtTimeDetails
    UA_ReadEventDetails details;
    UA_ReadEventDetails_init(&details);
    details.filter = filter;

    // At least two of the following parameters must be set
    details.numValuesPerNode = maxItems; // 0 = return all / max server is capable of
    details.startTime = startTime;
    details.endTime = endTime;

    UA_Boolean isInverse = !startTime || (endTime && (startTime > endTime));
    return __UA_Client_readHistorical_service(client, nodeId, callback, &details,
                                              timestampsToReturn, isInverse, 0, handle);
}

UA_StatusCode
__UA_Client_readHistorical_service_rawMod(UA_Client *client, const UA_NodeId nodeId,
                                          const UA_HistoricalIteratorCallback callback,
                                          const UA_DateTime startTime, const UA_DateTime endTime,
                                          const UA_Boolean returnBounds, const UA_UInt32 maxItems,
                                          const UA_Boolean readModified, const UA_TimestampsToReturn timestampsToReturn,
                                          void *handle) {

    // TODO: ReadProcessedDetails / ReadAtTimeDetails
    UA_ReadRawModifiedDetails details;
    UA_ReadRawModifiedDetails_init(&details);
    details.isReadModified = readModified; // Return only modified values
    details.returnBounds = returnBounds;   // Return values pre / post given range

    // At least two of the following parameters must be set
    details.numValuesPerNode = maxItems;   // 0 = return all / max server is capable of
    details.startTime = startTime;
    details.endTime = endTime;

    UA_Boolean isInverse = !startTime || (endTime && (startTime > endTime));
    return __UA_Client_readHistorical_service(client, nodeId, callback, &details,
                                              timestampsToReturn, isInverse, maxItems, handle);
}

UA_StatusCode
UA_Client_readHistorical_raw(UA_Client *client, const UA_NodeId nodeId,
                             const UA_HistoricalIteratorCallback callback,
                             const UA_DateTime startTime, const UA_DateTime endTime,
                             const UA_Boolean returnBounds, const UA_UInt32 maxItems,
                             const UA_TimestampsToReturn timestampsToReturn, void *handle) {

    return __UA_Client_readHistorical_service_rawMod(client, nodeId, callback, startTime, endTime, returnBounds,
                                                     maxItems, UA_FALSE, timestampsToReturn, handle);
}

UA_StatusCode
UA_Client_readHistorical_modified(UA_Client *client, const UA_NodeId nodeId,
                                  const UA_HistoricalIteratorCallback callback,
                                  const UA_DateTime startTime, const UA_DateTime endTime,
                                  const UA_Boolean returnBounds, const UA_UInt32 maxItems,
                                  const UA_TimestampsToReturn timestampsToReturn, void *handle) {

    return __UA_Client_readHistorical_service_rawMod(client, nodeId, callback, startTime, endTime, returnBounds,
                                                     maxItems, UA_TRUE, timestampsToReturn, handle);
}
