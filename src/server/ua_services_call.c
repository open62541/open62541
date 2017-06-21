/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_services.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_METHODCALLS /* conditional compilation */

static const UA_VariableNode *
getArgumentsVariableNode(UA_Server *server, const UA_MethodNode *ofMethod,
                         UA_String withBrowseName) {
    UA_NodeId hasProperty = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    for(size_t i = 0; i < ofMethod->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &ofMethod->references[i];

        if(rk->isInverse != false)
            continue;
        
        if(!UA_NodeId_equal(&hasProperty, &rk->referenceTypeId))
            continue;

        for(size_t j = 0; j < rk->targetIdsSize; ++j) {
            const UA_Node *refTarget =
                UA_NodeStore_get(server->nodestore, &rk->targetIds[j].nodeId);
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

static void
Operation_CallMethod(UA_Server *server, UA_Session *session,
                     const UA_CallMethodRequest *request,
                     UA_CallMethodResult *result) {
    /* Verify the method node */
    const UA_MethodNode *methodCalled = (const UA_MethodNode*)
        UA_NodeStore_get(server->nodestore, &request->methodId);
    if(!methodCalled)
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
    else if(methodCalled->nodeClass != UA_NODECLASS_METHOD)
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
    else if(!methodCalled->method)
        result->statusCode = UA_STATUSCODE_BADINTERNALERROR;
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Verify the object node */
    const UA_ObjectNode *object = (const UA_ObjectNode*)
        UA_NodeStore_get(server->nodestore, &request->objectId);
    if(!object)
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
    else if(object->nodeClass != UA_NODECLASS_OBJECT &&
            object->nodeClass != UA_NODECLASS_OBJECTTYPE)
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Verify access rights */
    UA_Boolean executable = methodCalled->executable;
    if(session != &adminSession)
        executable = executable &&
            server->config.accessControl.getUserExecutableOnObject(&session->sessionId,
                                 session->sessionHandle, &request->objectId, &request->methodId);
    if(!executable) {
        result->statusCode = UA_STATUSCODE_BADNOTWRITABLE; // There is no NOTEXECUTABLE?
        return;
    }

    /* Verify method/object relations. Object must have a hasComponent or a
     * subtype of hasComponent reference to the method node. Therefore, check
     * every reference between the parent object and the method node if there is
     * a hasComponent (or subtype) reference */
    UA_Boolean found = false;
    UA_NodeId hasComponentNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASCOMPONENT);
    UA_NodeId hasSubTypeNodeId = UA_NODEID_NUMERIC(0,UA_NS0ID_HASSUBTYPE);
    for(size_t i = 0; i < object->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &object->references[i];
        if(rk->isInverse)
            continue;
        if(!isNodeInTree(server->nodestore, &rk->referenceTypeId,
                         &hasComponentNodeId, &hasSubTypeNodeId, 1))
            continue;
        for(size_t j = 0; j < rk->targetIdsSize; ++j) {
            if(UA_NodeId_equal(&rk->targetIds[j].nodeId, &request->methodId)) {
                found = true;
                break;
            }
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
        if(request->inputArgumentsSize > 0)
            result->statusCode = UA_STATUSCODE_BADINVALIDARGUMENT;
    } else {
        result->statusCode = argumentsConformsToDefinition(server, inputArguments,
                                                           request->inputArgumentsSize,
                                                           request->inputArguments);
    }
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Allocate the output arguments */
    result->outputArgumentsSize = 0; /* the default */
    const UA_VariableNode *outputArguments =
        getArgumentsVariableNode(server, methodCalled, UA_STRING("OutputArguments"));
    if(outputArguments) {
        result->outputArguments =
            (UA_Variant*)UA_Array_new(outputArguments->value.data.value.value.arrayLength,
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
    result->statusCode =
        methodCalled->method(&methodCalled->nodeId, (void*)(uintptr_t)methodCalled->context,
                             &object->nodeId, (void*)(uintptr_t)&object->context,
                             &session->sessionId, session->sessionHandle,
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
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing CallRequest");

    response->responseHeader.serviceResult = 
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_CallMethod,
                  &request->methodsToCallSize, &UA_TYPES[UA_TYPES_CALLMETHODREQUEST],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_CALLMETHODRESULT]);
}

#endif /* UA_ENABLE_METHODCALLS */
