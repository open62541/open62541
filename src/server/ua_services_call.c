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
satisfySignature(const UA_Variant *var, const UA_Argument *arg) {
    if(!UA_NodeId_equal(&var->type->typeId, &arg->dataType) )
        return UA_STATUSCODE_BADINVALIDARGUMENT;

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
argConformsToDefinition(const UA_VariableNode *argRequirements, size_t argsSize, const UA_Variant *args) {
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
        retval |= satisfySignature(&args[i], &argReqs[i]);
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
    for(size_t i = 0; i < withObject->referencesSize; i++) {
        if(withObject->references[i].referenceTypeId.identifier.numeric == UA_NS0ID_HASCOMPONENT) {
            // FIXME: Not checking any subtypes of HasComponent at the moment
            if(UA_NodeId_equal(&withObject->references[i].targetId.nodeId, &methodCalled->nodeId)) {
                result->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
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
        result->statusCode = argConformsToDefinition(inputArguments, request->inputArgumentsSize,
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
