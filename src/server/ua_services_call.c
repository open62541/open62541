#include "ua_services.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_METHODCALLS /* conditional compilation */

static const UA_VariableNode *
getArgumentsVariableNode(UA_Server *server, const UA_MethodNode *ofMethod,
                         UA_String withBrowseName) {
    UA_NodeId hasProperty = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    for(size_t i = 0; i < ofMethod->referencesSize; i++) {
        if(ofMethod->references[i].isInverse == false &&
            UA_NodeId_equal(&hasProperty, &ofMethod->references[i].referenceTypeId)) {
            const UA_Node *refTarget =
                UA_NodeStore_get(server->nodestore, &ofMethod->references[i].targetId.nodeId);
            if(!refTarget)
                continue;
            if(refTarget->nodeClass == UA_NODECLASS_VARIABLE &&
                refTarget->browseName.namespaceIndex == 0 &&
                UA_String_equal(&withBrowseName, &refTarget->browseName.name)) {
                return (const UA_VariableNode*) refTarget;
            }
        }
    }
    return NULL;
}

static UA_StatusCode
satisfySignature(UA_Server *server, const UA_Variant *var, const UA_Argument *arg) {
  if(var == NULL || var->type == NULL) 
    return UA_STATUSCODE_BADINVALIDARGUMENT;
  
  if(!UA_NodeId_equal(&var->type->typeId, &arg->dataType)){
        if(!UA_NodeId_equal(&var->type->typeId, &UA_TYPES[UA_TYPES_INT32].typeId))
            return UA_STATUSCODE_BADINVALIDARGUMENT;

        /* enumerations are encoded as int32 -> if provided var is integer, check if arg is an enumeration type */
        UA_NodeId enumerationNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ENUMERATION);
        UA_NodeId hasSubTypeNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASSUBTYPE);
        UA_Boolean found = false;
        UA_StatusCode retval = isNodeInTree(server->nodestore, &arg->dataType, &enumerationNodeId, &hasSubTypeNodeId, 1, 1, &found);
        if(retval != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADINTERNALERROR;
        if(!found)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    // Note: The namespace compiler will compile nodes with their actual array dimensions
    // Todo: Check if this is standard conform for scalars
    if(arg->arrayDimensionsSize > 0 && var->arrayDimensionsSize > 0)
        if(var->arrayDimensionsSize != arg->arrayDimensionsSize)
            return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_Int32 *varDims = var->arrayDimensions;
    size_t varDimsSize = var->arrayDimensionsSize;
    UA_Boolean scalar = UA_Variant_isScalar(var);

    /* The dimension 1 is implicit in the array length */
    UA_Int32 fakeDims;
    if(!scalar && !varDims) {
        fakeDims = (UA_Int32)var->arrayLength;
        varDims = &fakeDims;
        varDimsSize = 1;
    }

    /* ValueRank Semantics
     *  n >= 1: the value is an array with the specified number of dimens*ions.
     *  n = 0: the value is an array with one or more dimensions.
     *  n = -1: the value is a scalar.
     *  n = -2: the value can be a scalar or an array with any number of dimensions.
     *  n = -3:  the value can be a scalar or a one dimensional array. */
    switch(arg->valueRank) {
    case -3:
        if(varDimsSize > 1)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        break;
    case -2:
        break;
    case -1:
        if(!scalar)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        break;
    case 0:
        if(scalar || !varDims)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        break;
    default:
        break;
    }

    /* Do the variants dimensions match? Check only if defined in the argument. */
    if(arg->arrayDimensionsSize > 0) {
        if(arg->arrayDimensionsSize != varDimsSize)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        for(size_t i = 0; i < varDimsSize; i++) {
            if((UA_Int32)arg->arrayDimensions[i] != varDims[i])
                return UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
argConformsToDefinition(UA_Server *server, const UA_VariableNode *argRequirements, size_t argsSize, const UA_Variant *args) {
    if(argRequirements->value.variant.value.type != &UA_TYPES[UA_TYPES_ARGUMENT])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Argument *argReqs = (UA_Argument*)argRequirements->value.variant.value.data;
    size_t argReqsSize = argRequirements->value.variant.value.arrayLength;
    if(argRequirements->valueSource != UA_VALUESOURCE_VARIANT)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(UA_Variant_isScalar(&argRequirements->value.variant.value))
        argReqsSize = 1;
    if(argReqsSize > argsSize)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(argReqsSize != argsSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < argReqsSize; i++)
        retval |= satisfySignature(server, &args[i], &argReqs[i]);
    return retval;
}

void
Service_Call_single(UA_Server *server, UA_Session *session, const UA_CallMethodRequest *request,
                    UA_CallMethodResult *result) {

    /* Get/verify the method node */
    const UA_MethodNode *methodCalled =
        (const UA_MethodNode*)UA_NodeStore_get(server->nodestore, &request->methodId);
    if(!methodCalled) {
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
        return;
    }
    if(methodCalled->nodeClass != UA_NODECLASS_METHOD) {
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        return;
    }
    if(!methodCalled->executable || !methodCalled->userExecutable || !methodCalled->attachedMethod) {
        result->statusCode = UA_STATUSCODE_BADNOTWRITABLE; // There is no NOTEXECUTABLE?
        return;
    }

    /* Get/verify the object node */
    const UA_ObjectNode *withObject =
        (const UA_ObjectNode*)UA_NodeStore_get(server->nodestore, &request->objectId);
    if(!withObject) {
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return;
    }
    if(withObject->nodeClass != UA_NODECLASS_OBJECT && withObject->nodeClass != UA_NODECLASS_OBJECTTYPE) {
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        return;
    }

    /* Verify method/object relations. Object must have a hasComponent or a subtype of hasComponent reference to the method node. */
    /* Therefore, check every reference between the parent object and the method node if there is a hasComponent (or subtype) reference */
    UA_Boolean found = false;
    UA_NodeId hasComponentNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASCOMPONENT);
    UA_NodeId hasSubTypeNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASSUBTYPE);
    for(size_t i=0;i<methodCalled->referencesSize;i++){
        if (methodCalled->references[i].isInverse && UA_NodeId_equal(&methodCalled->references[i].targetId.nodeId,&withObject->nodeId)){
            //TODO adjust maxDepth to needed tree depth (define a variable in config?)
    	    isNodeInTree(server->nodestore, &methodCalled->references[i].referenceTypeId, &hasComponentNodeId,
    	         &hasSubTypeNodeId, 1, 1, &found);
            if(found){
                break;
    	    }
        }
    }
    if(!found)
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Verify Input Argument count, types and sizes */
    //check inputAgruments only if there are any
    if(request->inputArgumentsSize > 0){
        const UA_VariableNode *inputArguments = getArgumentsVariableNode(server, methodCalled, UA_STRING("InputArguments"));

        if(!inputArguments) {
            result->statusCode = UA_STATUSCODE_BADINVALIDARGUMENT;
            return;
        }
            result->statusCode = argConformsToDefinition(server, inputArguments, request->inputArgumentsSize,
                                                     request->inputArguments);
        if(result->statusCode != UA_STATUSCODE_GOOD)
            return;
    }

    /* Allocate the output arguments */
    const UA_VariableNode *outputArguments = getArgumentsVariableNode(server, methodCalled, UA_STRING("OutputArguments"));
    if(!outputArguments) {
        result->outputArgumentsSize=0;
    }else{
        result->outputArguments = UA_Array_new(outputArguments->value.variant.value.arrayLength, &UA_TYPES[UA_TYPES_VARIANT]);
        if(!result->outputArguments) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        result->outputArgumentsSize = outputArguments->value.variant.value.arrayLength;
    }

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
    methodCallSession = session;
#endif

    /* Call the method */
    result->statusCode = methodCalled->attachedMethod(methodCalled->methodHandle, withObject->nodeId,
                                                      request->inputArgumentsSize, request->inputArguments,
                                                      result->outputArgumentsSize, result->outputArguments);

#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
    methodCallSession = NULL;
#endif
    /* TODO: Verify Output Argument count, types and sizes */
}
void Service_Call(UA_Server *server, UA_Session *session, const UA_CallRequest *request,
                  UA_CallResponse *response) {

    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing CallRequest");
    if(request->methodsToCallSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_Array_new(request->methodsToCallSize, &UA_TYPES[UA_TYPES_CALLMETHODRESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->methodsToCallSize;

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    UA_Boolean isExternal[request->methodsToCallSize];
    UA_UInt32 indices[request->methodsToCallSize];
    memset(isExternal, false, sizeof(UA_Boolean) * request->methodsToCallSize);
    for(size_t j = 0;j<server->externalNamespacesSize;j++) {
        size_t indexSize = 0;
        for(size_t i = 0;i < request->methodsToCallSize;i++) {
            if(request->methodsToCall[i].methodId.namespaceIndex != server->externalNamespaces[j].index)
                continue;
            isExternal[i] = true;
            indices[indexSize] = (UA_UInt32)i;
            indexSize++;
        }
        if(indexSize == 0)
            continue;
        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->call(ens->ensHandle, &request->requestHeader, request->methodsToCall,
                       indices, (UA_UInt32)indexSize, response->results);
    }
#endif
	
    for(size_t i = 0; i < request->methodsToCallSize;i++){
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
        if(!isExternal[i])
#endif    
			Service_Call_single(server, session, &request->methodsToCall[i], &response->results[i]);
	}
}

#endif /* UA_ENABLE_METHODCALLS */
