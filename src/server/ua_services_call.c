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
static UA_StatusCode isNodeInTree(UA_NodeStore *ns, const UA_NodeId *rootNode,const UA_NodeId *nodeToFind, const UA_NodeId *referenceTypeId,size_t *maxDepth, UA_Boolean *found){
    *maxDepth = *maxDepth-1;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    const UA_Node * node = UA_NodeStore_get(ns,rootNode);
    if(node==NULL){
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    *found = false;
    for(size_t i=0; i<node->referencesSize;i++){
        if(UA_NodeId_equal(&node->references[i].referenceTypeId, referenceTypeId) &&
                node->references[i].isInverse == false){
           if(UA_NodeId_equal(&node->references[i].targetId.nodeId, nodeToFind)){
               *found = true;
               return UA_STATUSCODE_GOOD;
           }
           if(*maxDepth==0){
               continue;
           }
           retval = isNodeInTree(ns,&node->references[i].targetId.nodeId,nodeToFind,referenceTypeId,maxDepth,found);

           if(*found){
               break;
           }
        }

    }
    *maxDepth=*maxDepth+1;
    return retval;
}
static UA_StatusCode
satisfySignature(UA_Server *server, const UA_Variant *var, const UA_Argument *arg) {

    if(!UA_NodeId_equal(&var->type->typeId, &arg->dataType)){
        if(!UA_NodeId_equal(&var->type->typeId, &UA_TYPES[UA_TYPES_INT32].typeId))
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        //enumerations are encoded as int32 -> if provided var is integer, check if a an enumeration type
        UA_NodeId ENUMERATION_NODEID_NS0 = UA_NODEID_NUMERIC(0,29);
        UA_NodeId hasSubTypeNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASSUBTYPE);
        UA_Boolean found = false;
        size_t maxDepth = 1;
        if(isNodeInTree(server->nodestore, &ENUMERATION_NODEID_NS0, &arg->dataType, &hasSubTypeNodeId, &maxDepth, &found)!=UA_STATUSCODE_GOOD){
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        if(!found){
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        }
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

    /* do the array dimensions match? */
    if(arg->arrayDimensionsSize != varDimsSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    for(size_t i = 0; i < varDimsSize; i++) {
        if((UA_Int32)arg->arrayDimensions[i] != varDims[i])
            return UA_STATUSCODE_BADINVALIDARGUMENT;
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
    const UA_MethodNode *methodCalled =
        (const UA_MethodNode*)UA_NodeStore_get(server->nodestore, &request->methodId);
    if(!methodCalled) {
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
        return;
    }

    const UA_ObjectNode *withObject =
        (const UA_ObjectNode*)UA_NodeStore_get(server->nodestore, &request->objectId);
    if(!withObject) {
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return;
    }

    if(methodCalled->nodeClass != UA_NODECLASS_METHOD) {
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        return;
    }

    if(withObject->nodeClass != UA_NODECLASS_OBJECT && withObject->nodeClass != UA_NODECLASS_OBJECTTYPE) {
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        return;
    }

    /* Verify method/object relations */
    // Object must have a hasComponent reference (or any inherited referenceType from sayd reference)
    // to be valid for a methodCall...
    result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
    UA_Boolean found = false;
    size_t maxDepth = 10;
    UA_NodeId hasSubTypeNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASSUBTYPE);
    UA_NodeId hasComponentNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASCOMPONENT);
    for(size_t i = 0; i < withObject->referencesSize; i++) {
        if(UA_NodeId_equal(&withObject->references[i].targetId.nodeId, &methodCalled->nodeId)) {
            if(UA_NodeId_equal(&withObject->references[i].referenceTypeId, &hasComponentNodeId)){
                result->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            UA_StatusCode retval = isNodeInTree(server->nodestore,&hasComponentNodeId,&withObject->references[i].referenceTypeId,&hasSubTypeNodeId,&maxDepth,&found);
            if(retval == UA_STATUSCODE_GOOD && found){
                result->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            return;
        }
    }

    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Verify method executable */
    if(!methodCalled->executable || !methodCalled->userExecutable) {
        result->statusCode = UA_STATUSCODE_BADNOTWRITABLE; // There is no NOTEXECUTABLE?
        return;
    }

    /* Verify Input Argument count, types and sizes */
    const UA_VariableNode *inputArguments;
    inputArguments = getArgumentsVariableNode(server, methodCalled, UA_STRING("InputArguments"));
    if(inputArguments) {
        result->statusCode = argConformsToDefinition(server, inputArguments, request->inputArgumentsSize,
                                                     request->inputArguments);
        if(result->statusCode != UA_STATUSCODE_GOOD)
            return;
    } else if(request->inputArgumentsSize > 0) {
        result->statusCode = UA_STATUSCODE_BADINVALIDARGUMENT;
        return;
    }

    const UA_VariableNode *outputArguments =
        getArgumentsVariableNode(server, methodCalled, UA_STRING("OutputArguments"));
    if(!outputArguments) {
        // A MethodNode must have an OutputArguments variable (which may be empty)
        result->statusCode = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* Call method if available */
    if(methodCalled->attachedMethod) {
        result->outputArguments = UA_Array_new(outputArguments->value.variant.value.arrayLength,
                                               &UA_TYPES[UA_TYPES_VARIANT]);
        result->outputArgumentsSize = outputArguments->value.variant.value.arrayLength;
        result->statusCode = methodCalled->attachedMethod(methodCalled->methodHandle, withObject->nodeId,
                                                          request->inputArgumentsSize, request->inputArguments,
                                                          result->outputArgumentsSize, result->outputArguments);
    }
    else
        result->statusCode = UA_STATUSCODE_BADNOTWRITABLE; // There is no NOTEXECUTABLE?
    /* TODO: Verify Output Argument count, types and sizes */
}
void Service_Call(UA_Server *server, UA_Session *session, const UA_CallRequest *request,
                  UA_CallResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing CallRequest");
    if(request->methodsToCallSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_Array_new(request->methodsToCallSize,
                                     &UA_TYPES[UA_TYPES_CALLMETHODRESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->methodsToCallSize;

    for(size_t i = 0; i < request->methodsToCallSize;i++)
        Service_Call_single(server, session, &request->methodsToCall[i], &response->results[i]);
}

#endif /* UA_ENABLE_METHODCALLS */
