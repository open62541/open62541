#include "ua_client.h"
#include "ua_nodeids.h"
#include "ua_client_highlevel.h"
#include "ua_types_encoding_binary.h"

UA_StatusCode UA_Client_NamespaceGetIndex(UA_Client *client, UA_String *namespaceUri, UA_UInt16 *namespaceIndex){
	UA_ReadRequest ReadRequest;
	UA_ReadResponse ReadResponse;
	UA_StatusCode retval = UA_STATUSCODE_BADUNEXPECTEDERROR;

	UA_ReadRequest_init(&ReadRequest);
    UA_ReadValueId id;
	id.attributeId = UA_ATTRIBUTEID_VALUE;
	id.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY);
	ReadRequest.nodesToRead = &id;
	ReadRequest.nodesToReadSize = 1;

	ReadResponse = UA_Client_Service_read(client, ReadRequest);

    if(ReadResponse.responseHeader.serviceResult != UA_STATUSCODE_GOOD){
        retval = ReadResponse.responseHeader.serviceResult;
        goto cleanup;
    }

    if(ReadResponse.resultsSize != 1 || !ReadResponse.results[0].hasValue){
        retval = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
        goto cleanup;
    }

    if(ReadResponse.results[0].value.type != &UA_TYPES[UA_TYPES_STRING]){
        retval = UA_STATUSCODE_BADTYPEMISMATCH;
        goto cleanup;
    }

    retval = UA_STATUSCODE_BADNOTFOUND;
    for(UA_UInt16 iterator = 0; iterator < ReadResponse.results[0].value.arrayLength; iterator++){
        if(UA_String_equal(namespaceUri, &((UA_String*)ReadResponse.results[0].value.data)[iterator] )){
            *namespaceIndex = iterator;
            retval = UA_STATUSCODE_GOOD;
            break;
        }
    }

cleanup:
    UA_ReadResponse_deleteMembers(&ReadResponse);

	return retval;
}


/*******************/
/* Node Management */
/*******************/

UA_StatusCode UA_EXPORT
UA_Client_addReference(UA_Client *client, const UA_NodeId sourceNodeId, const UA_NodeId referenceTypeId,
                       UA_Boolean isForward, const UA_String targetServerUri,
                       const UA_ExpandedNodeId targetNodeId, UA_NodeClass targetNodeClass) {
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

UA_StatusCode UA_EXPORT
UA_Client_deleteReference(UA_Client *client, const UA_NodeId sourceNodeId, const UA_NodeId referenceTypeId,
                          UA_Boolean isForward, const UA_ExpandedNodeId targetNodeId,
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

UA_StatusCode UA_Client_deleteNode(UA_Client *client, const UA_NodeId nodeId,
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

UA_StatusCode __UA_Client_addNode(UA_Client *client, const UA_NodeClass nodeClass,
                                  const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId,
                                  const UA_NodeId referenceTypeId, const UA_QualifiedName browseName,
                                  const UA_NodeId typeDefinition, const UA_NodeAttributes *attr,
                                  const UA_DataType *attributeType, UA_NodeId *outNewNodeId) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
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
    size_t attributes_length = UA_calcSizeBinary(attr, attributeType);
    item.nodeAttributes.typeId = attributeType->typeId;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
    retval = UA_ByteString_newMembers(&item.nodeAttributes.body, attributes_length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    size_t offset = 0;
    retval = UA_encodeBinary(attr, attributeType, &item.nodeAttributes.body, &offset);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_deleteMembers(&item.nodeAttributes.body);
        return retval;
    }
    request.nodesToAdd = &item;
    request.nodesToAddSize = 1;
    UA_AddNodesResponse response = UA_Client_Service_addNodes(client, request);
    UA_ByteString_deleteMembers(&item.nodeAttributes.body);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        retval = response.responseHeader.serviceResult;
        UA_AddNodesResponse_deleteMembers(&response);
        return retval;
    }
    if(response.resultsSize != 1) {
        UA_AddNodesResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    if(outNewNodeId && response.results[0].statusCode) {
        *outNewNodeId = response.results[0].addedNodeId;
        UA_NodeId_init(&response.results[0].addedNodeId);
    }
    return response.results[0].statusCode;
}

/********/
/* Call */
/********/

UA_StatusCode
UA_Client_call(UA_Client *client, const UA_NodeId objectId, const UA_NodeId methodId,
               UA_Int32 inputSize, const UA_Variant *input, UA_Int32 *outputSize,
               UA_Variant **output) {
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    UA_CallMethodRequest item;
    UA_CallMethodRequest_init(&item);
    item.methodId = methodId;
    item.objectId = objectId;
    item.inputArguments = (void*)(uintptr_t)input; // cast const...
    item.inputArgumentsSize = inputSize;
    request.methodsToCall = &item;
    request.methodsToCallSize = 1;
    UA_CallResponse response = UA_Client_Service_call(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_CallResponse_deleteMembers(&response);
        return retval;
    }
    if(response.resultsSize != 1) {
        UA_CallResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    retval = response.results[0].statusCode;
    if(retval == UA_STATUSCODE_GOOD) {
        *output = response.results[0].outputArguments;
        *outputSize = response.results[0].outputArgumentsSize;
        response.results[0].outputArguments = NULL;
        response.results[0].outputArgumentsSize = -1;
    }
    UA_CallResponse_deleteMembers(&response);
    return retval;
}

/**************/
/* Attributes */
/**************/

UA_StatusCode 
__UA_Client_readAttribute(UA_Client *client, UA_NodeId nodeId, UA_AttributeId attributeId,
                          void *out, const UA_DataType *outDataType) {
    UA_ReadValueId item;
    UA_ReadValueId_init(&item);
    item.nodeId = nodeId;
    item.attributeId = attributeId;
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = &item;
    request.nodesToReadSize = 1;
    UA_ReadResponse response = UA_Client_Service_read(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize != 1)
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_deleteMembers(&response);
        return retval;
    }
    UA_DataValue *res = response.results;
    if(res->hasStatus != UA_STATUSCODE_GOOD)
        retval = res->hasStatus;
    else if(!res->hasValue || !UA_Variant_isScalar(&res->value))
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReadResponse_deleteMembers(&response);
        return retval;
    }
    if(attributeId == UA_ATTRIBUTEID_VALUE) {
        memcpy(out, &res->value, sizeof(UA_Variant));
        UA_Variant_init(&res->value);
    }
    else if(res->value.type != outDataType) {
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    } else {
        memcpy(out, res->value.data, res->value.type->memSize);
        UA_free(res->value.data);
        res->value.data = NULL;
    }
    UA_ReadResponse_deleteMembers(&response);
    return retval;
}
