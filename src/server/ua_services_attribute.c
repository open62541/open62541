#include "ua_services.h"
#include "nodestore/ua_nodestoreExample.h"
#include "ua_statuscodes.h"
#include "nodestore/ua_nodestore.h"
#include "ua_namespace_manager.h"
#include "ua_namespace_0.h"
#include "ua_util.h"


#define CHECK_NODECLASS(CLASS)                                 \
    if(!(node->nodeClass & (CLASS))) {                         \
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE; \
        v.status       = UA_STATUSCODE_BADNOTREADABLE;         \
        break;                                                 \
    }                                                          \

static UA_DataValue service_read_node(UA_Server *server, const UA_ReadValueId *id) {
    UA_DataValue v;
    UA_DataValue_init(&v);

    UA_Node const *node   = UA_NULL;
    UA_Int32       result = UA_NodeStoreExample_get(server->nodestore, &(id->nodeId), &node);
    if(result != UA_STATUSCODE_GOOD || node == UA_NULL) {
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
        v.status       = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return v;
    }
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    switch(id->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_NODEID], &node->nodeId);
        break;

    case UA_ATTRIBUTEID_NODECLASS:
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_INT32], &node->nodeClass);
        break;

    case UA_ATTRIBUTEID_BROWSENAME:
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_QUALIFIEDNAME], &node->browseName);
        break;

    case UA_ATTRIBUTEID_DISPLAYNAME:
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_LOCALIZEDTEXT],
                                          &node->displayName);
        break;

    case UA_ATTRIBUTEID_DESCRIPTION:
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_LOCALIZEDTEXT],
                                          &node->description);
        break;

    case UA_ATTRIBUTEID_WRITEMASK:
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_UINT32], &node->writeMask);
        break;

    case UA_ATTRIBUTEID_USERWRITEMASK:
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_UINT32], &node->userWriteMask);
        break;

    case UA_ATTRIBUTEID_ISABSTRACT:
        CHECK_NODECLASS(
            UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_VARIABLETYPE |
            UA_NODECLASS_DATATYPE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |=
            UA_Variant_copySetValue(&v.value, &UA_[UA_BOOLEAN],
                                    &((UA_ReferenceTypeNode *)node)->isAbstract);
        break;

    case UA_ATTRIBUTEID_SYMMETRIC:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_BOOLEAN],
                                          &((UA_ReferenceTypeNode *)node)->symmetric);
        break;

    case UA_ATTRIBUTEID_INVERSENAME:
        CHECK_NODECLASS(UA_NODECLASS_REFERENCETYPE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_LOCALIZEDTEXT],
                                          &((UA_ReferenceTypeNode *)node)->inverseName);
        break;

    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        CHECK_NODECLASS(UA_NODECLASS_VIEW);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_BOOLEAN],
                                          &((UA_ViewNode *)node)->containsNoLoops);
        break;

    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        CHECK_NODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_BYTE],
                                          &((UA_ViewNode *)node)->eventNotifier);
        break;

    case UA_ATTRIBUTEID_VALUE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copy(&((UA_VariableNode *)node)->value, &v.value); // todo: zero-copy
        break;

    case UA_ATTRIBUTEID_DATATYPE:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_NODEID],
                                          &((UA_VariableTypeNode *)node)->dataType);
        break;

    case UA_ATTRIBUTEID_VALUERANK:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_INT32],
                                          &((UA_VariableTypeNode *)node)->valueRank);
        break;

    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        UA_Variant_copySetArray(&v.value, &UA_[UA_UINT32],
                                ((UA_VariableTypeNode *)node)->arrayDimensionsSize,
                                &((UA_VariableTypeNode *)node)->arrayDimensions);
        break;

    case UA_ATTRIBUTEID_ACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_BYTE],
                                          &((UA_VariableNode *)node)->accessLevel);
        break;

    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_BYTE],
                                          &((UA_VariableNode *)node)->userAccessLevel);
        break;

    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_DOUBLE],
                                          &((UA_VariableNode *)node)->minimumSamplingInterval);
        break;

    case UA_ATTRIBUTEID_HISTORIZING:
        CHECK_NODECLASS(UA_NODECLASS_VARIABLE);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_BOOLEAN],
                                          &((UA_VariableNode *)node)->historizing);
        break;

    case UA_ATTRIBUTEID_EXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_BOOLEAN],
                                          &((UA_MethodNode *)node)->executable);
        break;

    case UA_ATTRIBUTEID_USEREXECUTABLE:
        CHECK_NODECLASS(UA_NODECLASS_METHOD);
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_VARIANT;
        retval |= UA_Variant_copySetValue(&v.value, &UA_[UA_BOOLEAN],
                                          &((UA_MethodNode *)node)->userExecutable);
        break;

    default:
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
        v.status       = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;
    }

    UA_NodeStoreExample_releaseManagedNode(node);

    if(retval != UA_STATUSCODE_GOOD) {
        v.encodingMask = UA_DATAVALUE_ENCODINGMASK_STATUSCODE;
        v.status       = UA_STATUSCODE_BADNOTREADABLE;
    }


    return v;
}

void Service_Read(UA_Server *server, UA_Session *session,
                  const UA_ReadRequest *request, UA_ReadResponse *response) {
    UA_assert(server != UA_NULL && session != UA_NULL && request != UA_NULL && response != UA_NULL);

    if(request->nodesToReadSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    if(UA_Array_new((void **)&response->results, request->nodesToReadSize, &UA_[UA_DATAVALUE]) != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    if(UA_Array_new((void **)&response->diagnosticInfos, request->nodesToReadSize, &UA_[UA_DIAGNOSTICINFO]) != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->nodesToReadSize;

    UA_Int32 *numberOfFoundIndices;
    UA_UInt16 *associatedIndices;
    UA_UInt32 differentNamespaceIndexCount = 0;
    if(UA_Array_new((void **)&numberOfFoundIndices,request->nodesToReadSize,&UA_[UA_UINT32]) != UA_STATUSCODE_GOOD){
    	response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
    	return ;
    }

    if(UA_Array_new((void **)&associatedIndices,request->nodesToReadSize,&UA_[UA_UINT16]) != UA_STATUSCODE_GOOD){
    	response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
    	return ;
    }
    // find out count of different namespace indices

   for(UA_Int32 i = 0; i<request->nodesToReadSize; i++){

    	for(UA_UInt32 j = 0; j <= differentNamespaceIndexCount; j++){
    		if(associatedIndices[j] == request->nodesToRead[i].nodeId.namespaceIndex){
    			if(j==0){
    				differentNamespaceIndexCount++;
    			}
				numberOfFoundIndices[j]++;
				break;
    		}
    		else if(j == (differentNamespaceIndexCount - 1)){
    			associatedIndices[j] = request->nodesToRead[i].nodeId.namespaceIndex;
    			associatedIndices[j] = 1;
    			differentNamespaceIndexCount++;
    		}
    	}
    }

	UA_UInt32 *readValueIdIndices;
    if(UA_Array_new((void **)&readValueIdIndices,request->nodesToReadSize,&UA_[UA_UINT32]) != UA_STATUSCODE_GOOD){
    	response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
    	return ;
    }

    for(UA_UInt32 i = 0; i < differentNamespaceIndexCount; i++){
    	UA_Namespace *tmpNamespace;
    	UA_NamespaceManager_getNamespace(server->namespaceManager,associatedIndices[i],&tmpNamespace);
    	if(tmpNamespace != UA_NULL){

    	    //build up index array for each read operation onto a different namespace
    	    UA_UInt32 n = 0;
    	    for(UA_Int32 j = 0; j < request->nodesToReadSize; j++){
    	    	if(request->nodesToRead[j].nodeId.namespaceIndex == associatedIndices[i]){
    	    		readValueIdIndices[n] = j;
    	    	}
    	    }
    	    //call read for every namespace
    		tmpNamespace->nodeStore->readNodes(request->nodesToRead,
    				readValueIdIndices,
    				numberOfFoundIndices[i],
    				response->results,
    				request->timestampsToReturn,
    				&response->diagnosticInfos[i]);

			//	response->results[i] = service_read_node(server, &request->nodesToRead[i]);
    	}
    }
    UA_free(readValueIdIndices);
    UA_free(numberOfFoundIndices);
    UA_free(associatedIndices);
    /*
    for(UA_Int32 i = 0;i < response->resultsSize;i++){
				response->results[i] = service_read_node(server, &request->nodesToRead[i]);
    	}
    }
    */


}

static UA_StatusCode Service_Write_writeNode(UA_Server *server, UA_WriteValue *writeValue) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    const UA_Node *node;
    retval = UA_NodeStoreExample_get(server->nodestore, &writeValue->nodeId, &node);
    if(retval)
        return retval;

    switch(writeValue->attributeId) {
    case UA_ATTRIBUTEID_NODEID:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_NODECLASS:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){ } */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_BROWSENAME:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_DISPLAYNAME:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_DESCRIPTION:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_WRITEMASK:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_USERWRITEMASK:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_ISABSTRACT:

        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_SYMMETRIC:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_INVERSENAME:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_VALUE:
        if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT) {
            retval |= UA_Variant_copy(&writeValue->value.value, &((UA_VariableNode *)node)->value); // todo: zero-copy
        }
        break;


    case UA_ATTRIBUTEID_DATATYPE:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_VALUERANK:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_ACCESSLEVEL:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_HISTORIZING:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_EXECUTABLE:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    case UA_ATTRIBUTEID_USEREXECUTABLE:
        /* if(writeValue->value.encodingMask == UA_DATAVALUE_ENCODINGMASK_VARIANT){} */
        retval = UA_STATUSCODE_BADWRITENOTSUPPORTED;
        break;

    default:
        retval = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
        break;
    }

    UA_NodeStoreExample_releaseManagedNode(node);
    return retval;

}

void Service_Write(UA_Server *server, UA_Session *session,
                   const UA_WriteRequest *request, UA_WriteResponse *response) {
    UA_assert(server != UA_NULL && session != UA_NULL && request != UA_NULL && response != UA_NULL);

    if(UA_Array_new((void **)&response->results, request->nodesToWriteSize, &UA_[UA_STATUSCODE])) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    
    response->resultsSize = request->nodesToWriteSize;
    for(UA_Int32 i = 0;i < request->nodesToWriteSize;i++)
        response->results[i] = Service_Write_writeNode(server, &request->nodesToWrite[i]);
}
