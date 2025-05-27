/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2017 (c) Florian Palm
 *    Copyright 2015-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) LEvertz
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2020 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Martin Lang)
 */

#include "ua_services.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_METHODCALLS /* conditional compilation */

static const UA_VariableNode *
getArgumentsVariableNode(UA_Server *server, const UA_NodeHead *head,
                         UA_String withBrowseName) {
    for(size_t i = 0; i < head->referencesSize; ++i) {
        const UA_NodeReferenceKind *rk = &head->references[i];
        if(rk->isInverse != false)
            continue;
        if(rk->referenceTypeIndex != UA_REFERENCETYPEINDEX_HASPROPERTY)
            continue;
        const UA_ReferenceTarget *t = NULL;
        while((t = UA_NodeReferenceKind_iterate(rk, t))) {
            const UA_Node *refTarget =
                UA_NODESTORE_GETFROMREF(server, t->targetId);
            if(!refTarget)
                continue;
            if(refTarget->head.nodeClass == UA_NODECLASS_VARIABLE &&
               refTarget->head.browseName.namespaceIndex == 0 &&
               UA_String_equal(&withBrowseName, &refTarget->head.browseName.name)) {
                return &refTarget->variableNode;
            }
            UA_NODESTORE_RELEASE(server, refTarget);
        }
    }
    return NULL;
}

/* inputArgumentResults has the length request->inputArgumentsSize */
static UA_StatusCode
typeCheckArguments(UA_Server *server, UA_Session *session,
                   const UA_VariableNode *argRequirements, size_t argsSize,
                   UA_Variant *args, UA_StatusCode *inputArgumentResults) {
    /* Verify that we have a Variant containing UA_Argument (scalar or array) in
     * the "InputArguments" node */
    if(argRequirements->valueSource != UA_VALUESOURCE_DATA)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!argRequirements->value.data.value.hasValue)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(argRequirements->value.data.value.value.type != &UA_TYPES[UA_TYPES_ARGUMENT])
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Verify the number of arguments. A scalar argument value is interpreted as
     * an array of length 1. */
    size_t argReqsSize = argRequirements->value.data.value.value.arrayLength;
    if(UA_Variant_isScalar(&argRequirements->value.data.value.value))
        argReqsSize = 1;
    if(argReqsSize > argsSize)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(argReqsSize < argsSize)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    /* Type-check every argument against the definition */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Argument *argReqs = (UA_Argument*)argRequirements->value.data.value.value.data;
    const char *reason;
    for(size_t i = 0; i < argReqsSize; ++i) {
        if(compatibleValue(server, session, &argReqs[i].dataType, argReqs[i].valueRank,
                           argReqs[i].arrayDimensionsSize, argReqs[i].arrayDimensions,
                           &args[i], NULL, &reason))
            continue;

        /* Incompatible value. Try to correct the type if possible. */
        adjustValueType(server, &args[i], &argReqs[i].dataType);

        /* Recheck */
        if(!compatibleValue(server, session, &argReqs[i].dataType, argReqs[i].valueRank,
                            argReqs[i].arrayDimensionsSize, argReqs[i].arrayDimensions,
                            &args[i], NULL, &reason)) {
            inputArgumentResults[i] = UA_STATUSCODE_BADTYPEMISMATCH;
            retval = UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    }
    return retval;
}

/* inputArgumentResults has the length request->inputArgumentsSize */
static UA_StatusCode
validMethodArguments(UA_Server *server, UA_Session *session, const UA_MethodNode *method,
                     const UA_CallMethodRequest *request,
                     UA_StatusCode *inputArgumentResults) {
    /* Get the input arguments node */
    const UA_VariableNode *inputArguments =
        getArgumentsVariableNode(server, &method->head, UA_STRING("InputArguments"));
    if(!inputArguments) {
        if(request->inputArgumentsSize > 0)
            return UA_STATUSCODE_BADTOOMANYARGUMENTS;
        return UA_STATUSCODE_GOOD;
    }

    /* Verify the request */
    UA_StatusCode retval =
        typeCheckArguments(server, session, inputArguments, request->inputArgumentsSize,
                           request->inputArguments, inputArgumentResults);

    /* Release the input arguments node */
    UA_NODESTORE_RELEASE(server, (const UA_Node*)inputArguments);
    return retval;
}

static const UA_NodeId hasComponentNodeId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}};
static const UA_NodeId organizedByNodeId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ORGANIZES}};
static const UA_String namespaceDiModel = UA_STRING_STATIC("http://opcfoundation.org/UA/DI/");
static const UA_NodeId hasTypeDefinitionNodeId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASTYPEDEFINITION}};
// ns=0 will be replace dynamically. DI-Spec. 1.01: <UAObjectType NodeId="ns=1;i=1005" BrowseName="1:FunctionalGroupType">
static UA_NodeId functionGroupNodeId = {0, UA_NODEIDTYPE_NUMERIC, {1005}};

static void
callWithMethodAndObject(UA_Server *server, UA_Session *session,
                        const UA_CallMethodRequest *request, UA_CallMethodResult *result,
                        const UA_MethodNode *method, const UA_ObjectNode *object) {
    /* Verify the object's NodeClass */
    if(object->head.nodeClass != UA_NODECLASS_OBJECT &&
       object->head.nodeClass != UA_NODECLASS_OBJECTTYPE) {
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        return;
    }

    /* Verify the method's NodeClass */
    if(method->head.nodeClass != UA_NODECLASS_METHOD) {
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        return;
    }

    /* Is there a method to execute? */
    if(!method->method) {
        result->statusCode = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    UA_NodePointer methodP = UA_NodePointer_fromNodeId(&request->methodId);

    /* Verify method/object relations. Object must have a hasComponent or a
     * subtype of hasComponent reference to the method node. Therefore, check
     * every reference between the parent object and the method node if there is
     * a hasComponent (or subtype) reference */
    UA_ReferenceTypeSet hasComponentRefs;
    result->statusCode =
        referenceTypeIndices(server, &hasComponentNodeId, &hasComponentRefs, true);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    UA_Boolean found = false;
    for(size_t i = 0; i < object->head.referencesSize && !found; ++i) {
        const UA_NodeReferenceKind *rk = &object->head.references[i];
        if(rk->isInverse)
            continue;
        if(!UA_ReferenceTypeSet_contains(&hasComponentRefs, rk->referenceTypeIndex))
            continue;
        const UA_ReferenceTarget *t = NULL;
        while((t = UA_NodeReferenceKind_iterate(rk, t))) {
            if(UA_NodePointer_equal(t->targetId, methodP)) {
                found = true;
                break;
            }
        }
    }

    if(!found) {
        /* The following ParentObject evaluation is a workaround only to fulfill
         * the OPC UA Spec. Part 100 - Devices requirements regarding functional
         * groups. Compare OPC UA Spec. Part 100 - Devices, Release 1.02
         *    - 5.4 FunctionalGroupType
         *    - B.1 Functional Group Usages
         * A functional group is a sub-type of the FolderType and is used to
         * organize the Parameters and Methods from the complete set (named
         * ParameterSet and MethodSet) in (Functional) groups for instance
         * Configuration or Identification. The same Property, Parameter or
         * Method can be referenced from more than one FunctionalGroup. */

        /* Check whether the DI namespace is available */
        size_t foundNamespace = 0;
        UA_StatusCode res = getNamespaceByName(server, namespaceDiModel, &foundNamespace);
        if(res != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
            return;
        }
        functionGroupNodeId.namespaceIndex = (UA_UInt16)foundNamespace;

        UA_ReferenceTypeSet hasTypeDefinitionRefs;
        result->statusCode =
            referenceTypeIndices(server, &hasTypeDefinitionNodeId,
                                 &hasTypeDefinitionRefs, true);
        if(result->statusCode != UA_STATUSCODE_GOOD)
            return;

        /* Search for a HasTypeDefinition (or sub-) reference in the parent object */
        for(size_t i = 0; i < object->head.referencesSize && !found; ++i) {
            const UA_NodeReferenceKind *rk = &object->head.references[i];
            if(rk->isInverse)
                continue;
            if(!UA_ReferenceTypeSet_contains(&hasTypeDefinitionRefs, rk->referenceTypeIndex))
                continue;
            
            /* Verify that the HasTypeDefinition is equal to FunctionGroupType
             * (or sub-type) from the DI model */
            const UA_ReferenceTarget *t = NULL;
            while((t = UA_NodeReferenceKind_iterate(rk, t))) {
                if(!UA_NodePointer_isLocal(t->targetId))
                    continue;
                
                UA_NodeId tmpId = UA_NodePointer_toNodeId(t->targetId);
                if(!isNodeInTree_singleRef(server, &tmpId, &functionGroupNodeId,
                                           UA_REFERENCETYPEINDEX_HASSUBTYPE))
                    continue;

                /* Search for the called method with reference Organize (or
                 * sub-type) from the parent object */
                for(size_t k = 0; k < object->head.referencesSize && !found; ++k) {
                    const UA_NodeReferenceKind *rkInner = &object->head.references[k];
                    if(rkInner->isInverse)
                        continue;
                    const UA_NodeId * refId = 
                        UA_NODESTORE_GETREFERENCETYPEID(server, rkInner->referenceTypeIndex);
                    if(!isNodeInTree_singleRef(server, refId, &organizedByNodeId,
                                               UA_REFERENCETYPEINDEX_HASSUBTYPE))
                        continue;
                    
                    const UA_ReferenceTarget *t2 = NULL;
                    while((t2 = UA_NodeReferenceKind_iterate(rkInner, t2))) {
                        if(UA_NodePointer_equal(t2->targetId, methodP)) {
                            found = true;
                            break;
                        }
                    }
                }
            }
        }
        if(!found) {
            result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
            return;
        }
    }

    /* Verify access rights */
    UA_Boolean executable = method->executable;
    if(session != &server->adminSession) {
        UA_UNLOCK(&server->serviceMutex);
        executable = executable && server->config.accessControl.
            getUserExecutableOnObject(server, &server->config.accessControl, &session->sessionId,
                                      session->sessionHandle, &request->methodId, method->head.context,
                                      &request->objectId, object->head.context);
        UA_LOCK(&server->serviceMutex);
    }

    if(!executable) {
        result->statusCode = UA_STATUSCODE_BADNOTEXECUTABLE;
        return;
    }

    /* Allocate the inputArgumentResults array */
    result->inputArgumentResults = (UA_StatusCode*)
        UA_Array_new(request->inputArgumentsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!result->inputArgumentResults) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    result->inputArgumentResultsSize = request->inputArgumentsSize;

    /* Verify Input Arguments */
    result->statusCode = validMethodArguments(server, session, method, request,
                                              result->inputArgumentResults);

    /* Return inputArgumentResults only for BADINVALIDARGUMENT */
    if(result->statusCode != UA_STATUSCODE_BADINVALIDARGUMENT) {
        UA_Array_delete(result->inputArgumentResults, result->inputArgumentResultsSize,
                        &UA_TYPES[UA_TYPES_STATUSCODE]);
        result->inputArgumentResults = NULL;
        result->inputArgumentResultsSize = 0;
    }

    /* Error during type-checking? */
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Get the output arguments node */
    const UA_VariableNode *outputArguments =
        getArgumentsVariableNode(server, &method->head, UA_STRING("OutputArguments"));

    /* Allocate the output arguments array */
    size_t outputArgsSize = 0;
    if(outputArguments)
        outputArgsSize = outputArguments->value.data.value.value.arrayLength;
    result->outputArguments = (UA_Variant*)
        UA_Array_new(outputArgsSize, &UA_TYPES[UA_TYPES_VARIANT]);
    if(!result->outputArguments) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    result->outputArgumentsSize = outputArgsSize;

    /* Release the output arguments node */
    UA_NODESTORE_RELEASE(server, (const UA_Node*)outputArguments);

    /* Call the method */
    UA_UNLOCK(&server->serviceMutex);
    result->statusCode = method->method(server, &session->sessionId, session->sessionHandle,
                                        &method->head.nodeId, method->head.context,
                                        &object->head.nodeId, object->head.context,
                                        request->inputArgumentsSize, request->inputArguments,
                                        result->outputArgumentsSize, result->outputArguments);
    UA_LOCK(&server->serviceMutex);
    /* TODO: Verify Output matches the argument definition */
}

#if UA_MULTITHREADING >= 100

static void
Operation_CallMethodAsync(UA_Server *server, UA_Session *session, UA_UInt32 requestId,
                          UA_UInt32 requestHandle, size_t opIndex,
                          UA_CallMethodRequest *opRequest, UA_CallMethodResult *opResult,
                          UA_AsyncResponse **ar) {
    /* Get the method node */
    const UA_Node *method = UA_NODESTORE_GET(server, &opRequest->methodId);
    if(!method) {
        opResult->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    /* Get the object node */
    const UA_Node *object = UA_NODESTORE_GET(server, &opRequest->objectId);
    if(!object) {
        opResult->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        UA_NODESTORE_RELEASE(server, method);
        return;
    }

    /* Synchronous execution */
    if(!method->methodNode.async) {
        callWithMethodAndObject(server, session, opRequest, opResult,
                                &method->methodNode, &object->objectNode);
        goto cleanup;
    }

    /* Check the NodeClass */
    if(method->head.nodeClass != UA_NODECLASS_METHOD ||
       object->head.nodeClass != UA_NODECLASS_OBJECT) {
        opResult->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        goto cleanup;
    }

    /* Check the access rights */
    UA_Boolean executable = method->methodNode.executable;
    if(session != &server->adminSession) {
        executable = executable && server->config.accessControl.
            getUserExecutableOnObject(server, &server->config.accessControl,
                                      &session->sessionId, session->sessionHandle,
                                      &opRequest->methodId, method->head.context,
                                      &opRequest->objectId, object->head.context);
    }

    if(!executable) {
        opResult->statusCode = UA_STATUSCODE_BADNOTEXECUTABLE;
        goto cleanup;
    }

    /* <-- Async method call --> */

    /* No AsyncResponse allocated so far */
    if(!*ar) {
        opResult->statusCode =
            UA_AsyncManager_createAsyncResponse(&server->asyncManager, server,
                            &session->sessionId, requestId, requestHandle,
                            UA_ASYNCOPERATIONTYPE_CALL, ar);
        if(opResult->statusCode != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Create the Async Request to be taken by workers */
    opResult->statusCode =
        UA_AsyncManager_createAsyncOp(&server->asyncManager,
                                      server, *ar, opIndex, opRequest);

 cleanup:
    /* Release the method and object node */
    UA_NODESTORE_RELEASE(server, method);
    UA_NODESTORE_RELEASE(server, object);
}

void
Service_CallAsync(UA_Server *server, UA_Session *session, UA_UInt32 requestId,
                  const UA_CallRequest *request, UA_CallResponse *response,
                  UA_Boolean *finished) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing CallRequestAsync");
    if(server->config.maxNodesPerMethodCall != 0 &&
        request->methodsToCallSize > server->config.maxNodesPerMethodCall) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    UA_AsyncResponse *ar = NULL;
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperationsAsync(server, session, requestId,
                  request->requestHeader.requestHandle,
                  (UA_AsyncServiceOperation)Operation_CallMethodAsync,
                  &request->methodsToCallSize, &UA_TYPES[UA_TYPES_CALLMETHODREQUEST],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_CALLMETHODRESULT], &ar);

    if(ar) {
        if(ar->opCountdown > 0) {
            /* Move all results to the AsyncResponse. The async operation
             * results will be overwritten when the workers return results. */
            ar->response.callResponse = *response;
            UA_CallResponse_init(response);
            *finished = false;
        } else {
            /* If there is a new AsyncResponse, ensure it has at least one
             * pending operation */
            UA_AsyncManager_removeAsyncResponse(&server->asyncManager, ar);
        }
    }
}
#endif

static void
Operation_CallMethod(UA_Server *server, UA_Session *session, void *context,
                     const UA_CallMethodRequest *request, UA_CallMethodResult *result) {
    /* Get the method node */
    const UA_Node *method = UA_NODESTORE_GET(server, &request->methodId);
    if(!method) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    /* Get the object node */
    const UA_Node *object = UA_NODESTORE_GET(server, &request->objectId);
    if(!object) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        UA_NODESTORE_RELEASE(server, method);
        return;
    }

    /* Continue with method and object as context */
    callWithMethodAndObject(server, session, request, result,
                            &method->methodNode, &object->objectNode);

    /* Release the method and object node */
    UA_NODESTORE_RELEASE(server, method);
    UA_NODESTORE_RELEASE(server, object);
}

void Service_Call(UA_Server *server, UA_Session *session,
                  const UA_CallRequest *request, UA_CallResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing CallRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(server->config.maxNodesPerMethodCall != 0 &&
       request->methodsToCallSize > server->config.maxNodesPerMethodCall) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_CallMethod, NULL,
                  &request->methodsToCallSize, &UA_TYPES[UA_TYPES_CALLMETHODREQUEST],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_CALLMETHODRESULT]);
}

UA_CallMethodResult
UA_Server_call(UA_Server *server, const UA_CallMethodRequest *request) {
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    UA_LOCK(&server->serviceMutex);
    Operation_CallMethod(server, &server->adminSession, NULL, request, &result);
    UA_UNLOCK(&server->serviceMutex);
    return result;
}

#endif /* UA_ENABLE_METHODCALLS */
