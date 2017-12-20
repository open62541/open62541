/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include "ua_services.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_METHODCALLS /* conditional compilation */

static const UA_VariableNode *
getArgumentsVariableNode(UA_Server *server, const UA_MethodNode *ofMethod,
                         UA_String withBrowseName) {
    UA_NodeId hasProperty = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    for(size_t i = 0; i < ofMethod->referencesSize; ++i) {
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
argumentsConformsToDefinition(UA_Server *server, const UA_VariableNode *argRequirements,
                              size_t argsSize, UA_Variant *args) {
    if(argRequirements->value.data.value.value.type != &UA_TYPES[UA_TYPES_ARGUMENT])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Argument *argReqs = (UA_Argument*)argRequirements->value.data.value.value.data;
    size_t argReqsSize = argRequirements->value.data.value.value.arrayLength;
    if(argRequirements->valueSource != UA_VALUESOURCE_DATA)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(UA_Variant_isScalar(&argRequirements->value.data.value.value))
        argReqsSize = 1;
    if(argReqsSize > argsSize)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(argReqsSize != argsSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < argReqsSize; ++i)
        retval |= typeCheckValue(server, &argReqs[i].dataType, argReqs[i].valueRank,
                                 argReqs[i].arrayDimensionsSize, argReqs[i].arrayDimensions,
                                 &args[i], NULL, &args[i]);
    return retval;
}

void
Service_Call_single(UA_Server *server, UA_Session *session,
                    const UA_CallMethodRequest *request,
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

    /* Verify method/object relations. Object must have a hasComponent or a
     * subtype of hasComponent reference to the method node. Therefore, check
     * every reference between the parent object and the method node if there is
     * a hasComponent (or subtype) reference */
    UA_Boolean found = false;
    UA_NodeId hasComponentNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASCOMPONENT);
    UA_NodeId hasSubTypeNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASSUBTYPE);
    for(size_t i = 0; i < methodCalled->referencesSize; ++i) {
        if(methodCalled->references[i].isInverse &&
           UA_NodeId_equal(&methodCalled->references[i].targetId.nodeId, &withObject->nodeId)) {
            found = isNodeInTree(server->nodestore, &methodCalled->references[i].referenceTypeId,
                                 &hasComponentNodeId, &hasSubTypeNodeId, 1);
            if(found)
                break;
        }
    }
    if(!found) {
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
        return;
    }

    /* Verify Input Argument count, types and sizes */
    const UA_VariableNode *inputArguments =
        getArgumentsVariableNode(server, methodCalled, UA_STRING("InputArguments"));

    if(!inputArguments) {
        if(request->inputArgumentsSize > 0) {
            result->statusCode = UA_STATUSCODE_BADINVALIDARGUMENT;
            return;
        }
    } else {
        result->statusCode = argumentsConformsToDefinition(server, inputArguments,
                                                           request->inputArgumentsSize,
                                                           request->inputArguments);
        if(result->statusCode != UA_STATUSCODE_GOOD)
            return;
    }

    /* Allocate the output arguments */
    result->outputArgumentsSize = 0; /* the default */
    const UA_VariableNode *outputArguments =
        getArgumentsVariableNode(server, methodCalled, UA_STRING("OutputArguments"));
    if(outputArguments) {
        result->outputArguments = UA_Array_new(outputArguments->value.data.value.value.arrayLength,
                                               &UA_TYPES[UA_TYPES_VARIANT]);
        if(!result->outputArguments) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        result->outputArgumentsSize = outputArguments->value.data.value.value.arrayLength;
    }

    /* Call the method */
#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
    methodCallSession = session;
#endif
    result->statusCode = methodCalled->attachedMethod(methodCalled->methodHandle, withObject->nodeId,
                                                      request->inputArgumentsSize, request->inputArguments,
                                                      result->outputArgumentsSize, result->outputArguments);
#if defined(UA_ENABLE_METHODCALLS) && defined(UA_ENABLE_SUBSCRIPTIONS)
    methodCallSession = NULL;
#endif

    /* TODO: Verify Output matches the argument definition */
}

void Service_Call(UA_Server *server, UA_Session *session,
                  const UA_CallRequest *request,
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

    
    for(size_t i = 0; i < request->methodsToCallSize;++i){
            Service_Call_single(server, session, &request->methodsToCall[i], &response->results[i]);
    }
}

UA_CallMethodResult UA_EXPORT
UA_Server_call(UA_Server *server, const UA_CallMethodRequest *request) {
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    Service_Call_single(server, &adminSession,
                        request, &result);
    return result;
}

#endif /* UA_ENABLE_METHODCALLS */
