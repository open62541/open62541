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
 *    Copyright (c) 2025 Pilz GmbH & Co. KG, Author: Marcel Patzlaff
 */

#include "ua_services.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_METHODCALLS /* conditional compilation */

#define UA_MAX_METHOD_ARGUMENTS 64

struct GetArgumentsNodeContext {
    UA_Server *server;
    UA_String withBrowseName;
};

static void *
getArgumentsNodeCallback(void *context, UA_ReferenceTarget *t) {
    struct GetArgumentsNodeContext *ctx = (struct GetArgumentsNodeContext*)context;
    const UA_Node *refTarget =
        UA_NODESTORE_GETFROMREF_SELECTIVE(ctx->server, t->targetId,
                                          UA_NODEATTRIBUTESMASK_NODECLASS |
                                          UA_NODEATTRIBUTESMASK_VALUE,
                                          UA_REFERENCETYPESET_NONE,
                                          UA_BROWSEDIRECTION_INVALID);
    if(!refTarget)
        return NULL;
    if(refTarget->head.nodeClass == UA_NODECLASS_VARIABLE &&
       refTarget->head.browseName.namespaceIndex == 0 &&
       UA_String_equal(&ctx->withBrowseName, &refTarget->head.browseName.name)) {
        return (void*)(uintptr_t)&refTarget->variableNode;
    }
    UA_NODESTORE_RELEASE(ctx->server, refTarget);
    return NULL;
}

static const UA_VariableNode *
getArgumentsVariableNode(UA_Server *server, const UA_NodeHead *head,
                         UA_String withBrowseName) {
    for(size_t i = 0; i < head->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &head->references[i];
        if(rk->isInverse)
            continue;
        if(rk->referenceTypeIndex != UA_REFERENCETYPEINDEX_HASPROPERTY)
            continue;
        struct GetArgumentsNodeContext ctx;
        ctx.server = server;
        ctx.withBrowseName = withBrowseName;
        return (const UA_VariableNode*)
            UA_NodeReferenceKind_iterate(rk, getArgumentsNodeCallback, &ctx);
    }
    return NULL;
}

/* inputArgumentResults has the length request->inputArgumentsSize */
static UA_StatusCode
checkAdjustArguments(UA_Server *server, UA_Session *session,
                     const UA_VariableNode *argRequirements, size_t argsSize,
                     UA_Variant *args, UA_StatusCode *inputArgumentResults) {
    /* Verify that we have a Variant containing UA_Argument (scalar or array) in
     * the "InputArguments" node */
    if(argRequirements->valueSourceType != UA_VALUESOURCETYPE_INTERNAL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!argRequirements->valueSource.internal.value.hasValue)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_Variant *argVal = &argRequirements->valueSource.internal.value.value;
    if(argVal->type != &UA_TYPES[UA_TYPES_ARGUMENT])
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Verify the number of arguments. A scalar argument value is interpreted as
     * an array of length 1. */
    size_t argReqsSize = argVal->arrayLength;
    if(UA_Variant_isScalar(argVal))
        argReqsSize = 1;
    if(argReqsSize > argsSize)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(argReqsSize < argsSize)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    /* Type-check every argument against the definition */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Argument *argReqs = (UA_Argument*)argVal->data;
    const char *reason;
    for(size_t i = 0; i < argReqsSize; ++i) {
        /* Incompatible value. Try to correct the type if possible. */
        adjustValueType(server, &args[i], &argReqs[i].dataType);

        /* Check */
        if(!compatibleValue(server, session, &argReqs[i].dataType, argReqs[i].valueRank,
                            argReqs[i].arrayDimensionsSize, argReqs[i].arrayDimensions,
                            &args[i], NULL, &reason)) {
            inputArgumentResults[i] = UA_STATUSCODE_BADTYPEMISMATCH;
            retval = UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    }
    return retval;
}

static UA_StatusCode
checkMethodInTypeHierarchy(UA_Server *server, UA_Session *session,
                           const UA_NodeId *contextId, const UA_NodeId *methodId,
                           const UA_QualifiedName *methodBrowseName) {
    /* Get the type hierarchy "upwards" from the context */
    UA_NodeId *typeHierarchy;
    size_t typeHierarchySize;
    UA_StatusCode res =
        getTypeAndInterfaceHierarchy(server, contextId, false,
                                     &typeHierarchy, &typeHierarchySize);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Check if the method is in the type hierarchy */
    for(size_t i = 0; i < typeHierarchySize; i++) {
        UA_NodeId contained;
        res = findChildByBrowsename(server, session, typeHierarchy[i],
                                    UA_NODECLASS_METHOD,
                                    UA_REFERENCETYPEINDEX_HASCOMPONENT,
                                    UA_NS0ID(HASCOMPONENT),
                                    methodBrowseName, &contained);
        if(res == UA_STATUSCODE_BADNOTFOUND)
            continue;
        if(res != UA_STATUSCODE_GOOD)
            break;
        UA_Boolean found = UA_NodeId_equal(&contained, methodId);
        UA_NodeId_clear(&contained);
        res = (found) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOTFOUND;
        if(found)
            break;
    }

    /* Clean up and return */
    UA_Array_delete(typeHierarchy, typeHierarchySize, &UA_TYPES[UA_TYPES_NODEID]);
    return res;
}

static UA_StatusCode
callWithResolvedMethodAndObject(UA_Server *server, UA_Session *session,
                                const UA_CallMethodRequest *request,
                                UA_CallMethodResult *result,
                                const UA_MethodNode *resolvedMethod,
                                const UA_Node *callContext) {
    /* Is there a method to execute? */
    if(!resolvedMethod->method)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Verify access rights */
    UA_Boolean executable = resolvedMethod->executable;
    if(session != &server->adminSession) {
        executable = executable && server->config.accessControl.
            getUserExecutableOnObject(server, &server->config.accessControl,
                                      &session->sessionId, session->context,
                                      &resolvedMethod->head.nodeId,
                                      resolvedMethod->head.context,
                                      &request->objectId,
                                      callContext->head.context);
    }
    if(!executable)
        return UA_STATUSCODE_BADNOTEXECUTABLE;

    /* The input arguments are const and not changed. We move the input
     * arguments to a secondary array that is mutable. This is used for small
     * adjustments on the type level during the type checking. But it has to be
     * ensured that the original array can still by _clear'ed after the methods
     * call. */
    if(request->inputArgumentsSize > UA_MAX_METHOD_ARGUMENTS)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    UA_Variant mutableInputArgs[UA_MAX_METHOD_ARGUMENTS];
    if(request->inputArgumentsSize > 0)
        memcpy(mutableInputArgs, request->inputArguments,
               sizeof(UA_Variant) * request->inputArgumentsSize);

    /* Allocate the inputArgumentResults array */
    result->inputArgumentResults = (UA_StatusCode*)
        UA_Array_new(request->inputArgumentsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!result->inputArgumentResults)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    result->inputArgumentResultsSize = request->inputArgumentsSize;

    /* Type-check the input arguments */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    const UA_VariableNode *inputArguments =
        getArgumentsVariableNode(server, &resolvedMethod->head,
                                 UA_STRING("InputArguments"));
    if(inputArguments) {
        res = checkAdjustArguments(server, session,
                                   inputArguments, request->inputArgumentsSize,
                                   mutableInputArgs, result->inputArgumentResults);
        UA_NODESTORE_RELEASE(server, (const UA_Node*)inputArguments);
    } else if(request->inputArgumentsSize > 0) {
        res = UA_STATUSCODE_BADTOOMANYARGUMENTS;
    }

    /* Return inputArgumentResults only for BADINVALIDARGUMENT */
    if(res != UA_STATUSCODE_BADINVALIDARGUMENT) {
        UA_Array_delete(result->inputArgumentResults,
                        result->inputArgumentResultsSize,
                        &UA_TYPES[UA_TYPES_STATUSCODE]);
        result->inputArgumentResults = NULL;
        result->inputArgumentResultsSize = 0;
    }

    /* Error during type-checking? */
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Get the output arguments node */
    const UA_VariableNode *outputArguments =
        getArgumentsVariableNode(server, &resolvedMethod->head,
                                 UA_STRING("OutputArguments"));

    /* Allocate the output arguments array. Always allocate memory, hence the
     * +1, even if the length is zero. Because we need a unique outputArguments
     * pointer as the key for async operations. The memory gets deleted in
     * UA_Array_delete even if the outputArgumentsSize is zero. */
    size_t outputArgsSize = 0;
    if(outputArguments)
        outputArgsSize = outputArguments->valueSource.internal.value.value.arrayLength;
    result->outputArguments = (UA_Variant*)
        UA_Array_new(outputArgsSize+1, &UA_TYPES[UA_TYPES_VARIANT]);
    if(!result->outputArguments)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    result->outputArgumentsSize = outputArgsSize;

    /* Release the output arguments node */
    UA_NODESTORE_RELEASE(server, (const UA_Node*)outputArguments);

    /* Call the method. If this is an async method, unlock the server lock for
     * the duration of the (long-running) call. */
    return resolvedMethod->method(server, &session->sessionId, session->context,
                                  &resolvedMethod->head.nodeId,
                                  resolvedMethod->head.context,
                                  &callContext->head.nodeId, callContext->head.context,
                                  request->inputArgumentsSize, mutableInputArgs,
                                  result->outputArgumentsSize, result->outputArguments);

    /* TODO: Verify Output matches the argument definition */
}

static UA_StatusCode
callWithMethodAndObject(UA_Server *server, UA_Session *session,
                        const UA_CallMethodRequest *request,
                        UA_CallMethodResult *result,
                        const UA_Node *callMethod,
                        const UA_Node *callObject) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Verify the context object's NodeClass */
    if(callObject->head.nodeClass != UA_NODECLASS_OBJECT &&
       callObject->head.nodeClass != UA_NODECLASS_OBJECTTYPE)
        return UA_STATUSCODE_BADNODECLASSINVALID;

    /* Verify the method's NodeClass */
    if(callMethod->head.nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADNODECLASSINVALID;

    /* For object types the given method must be directly referenced!
     * OPC UA 1.05.06 will state in Part 4 - Call Service Parameters
     * the following for parameter "objectId":
     * "In case of an ObjectType the ObjectType shall be the source of a
     *  HasComponent Reference (or subtype of HasComponent Reference) to the
     *  Method specified in methodId" */

    /* Find the matching Method of the same BrowseName referenced directly by
     * the calling object */
    UA_NodeId resolvedMethodId;
    UA_StatusCode res =
        findChildByBrowsename(server, session, callObject->head.nodeId,
                              UA_NODECLASS_METHOD, UA_REFERENCETYPEINDEX_HASCOMPONENT,
                              UA_NS0ID(HASCOMPONENT), &callMethod->head.browseName,
                              &resolvedMethodId);
    if(res != UA_STATUSCODE_GOOD)
        return (res == UA_STATUSCODE_BADNOTFOUND) ?
            UA_STATUSCODE_BADMETHODINVALID : res;

    /* The resolved methodId is different */
    const UA_MethodNode *resolvedMethod = (const UA_MethodNode*)callMethod;
    if(!UA_NodeId_equal(&resolvedMethodId, &callMethod->head.nodeId)) {
        /* Check whether the original MethodId comes from an ObjectType that is
         * upwards in the type hierarchy. Downwards in the type hierarchy (or
         * even completely unrelated) is not allowed. */
        res = checkMethodInTypeHierarchy(server, session,
                                         &callObject->head.nodeId,
                                         &callMethod->head.nodeId,
                                         &callMethod->head.browseName);
        if(res != UA_STATUSCODE_GOOD) {
            if(res == UA_STATUSCODE_BADNOTFOUND)
                res = UA_STATUSCODE_BADMETHODINVALID;
            goto errout;
        }

        /* Get the resolved method node */
        resolvedMethod = (const UA_MethodNode*)
            UA_NODESTORE_GET_SELECTIVE(server, &resolvedMethodId,
                                       UA_NODEATTRIBUTESMASK_NODECLASS |
                                       UA_NODEATTRIBUTESMASK_EXECUTABLE,
                                       UA_REFERENCETYPESET_NONE,
                                       UA_BROWSEDIRECTION_INVALID);
        if(!resolvedMethod) {
            res = UA_STATUSCODE_BADINTERNALERROR;
            goto errout;
        }
    }

    /* Run the main call logic */
    res = callWithResolvedMethodAndObject(server, session, request, result,
                                          resolvedMethod, callObject);

 errout:
    /* Release the resolved method node if it is different */
    if(resolvedMethod && (const UA_Node*)resolvedMethod != callMethod)
        UA_NODESTORE_RELEASE(server, (const UA_Node*)resolvedMethod);
    UA_NodeId_clear(&resolvedMethodId);
    return res;
}

UA_Boolean
Operation_CallMethod(UA_Server *server, UA_Session *session,
                     const UA_CallMethodRequest *request,
                     UA_CallMethodResult *result) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Get the method node. We only need the nodeClass and browseName
     * attribute. */
    const UA_Node *method =
        UA_NODESTORE_GET_SELECTIVE(server, &request->methodId,
                                   UA_NODEATTRIBUTESMASK_NODECLASS |
                                   UA_NODEATTRIBUTESMASK_BROWSENAME,
                                   UA_REFERENCETYPESET_NONE,
                                   UA_BROWSEDIRECTION_INVALID);
    if(!method) {
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
        return true;
    }

    /* Get the object node. We only need the NodeClass attribute. But take all
     * references for now.
     *
     * TODO: Which references do we need actually? */
    const UA_Node *object =
        UA_NODESTORE_GET_SELECTIVE(server, &request->objectId,
                                   UA_NODEATTRIBUTESMASK_NODECLASS,
                                   UA_REFERENCETYPESET_ALL,
                                   UA_BROWSEDIRECTION_BOTH);
    if(!object) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        UA_NODESTORE_RELEASE(server, method);
        return true;
    }

    /* Continue with method and object (which can be an ObjectType) as
     * context */
    result->statusCode =
        callWithMethodAndObject(server, session, request,
                                result, method, object);

    /* Release the method and object node */
    UA_NODESTORE_RELEASE(server, method);
    UA_NODESTORE_RELEASE(server, object);

    return (result->statusCode != UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY);
}

UA_CallMethodResult
UA_Server_call(UA_Server *server, const UA_CallMethodRequest *request) {
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    lockServer(server);
    Operation_CallMethod(server, &server->adminSession, request, &result);
    /* Cancel asynchronous responses right away */
    if(result.statusCode == UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY) {
        if(server->config.asyncOperationCancelCallback)
            server->config.asyncOperationCancelCallback(server, result.outputArguments);
        result.statusCode = UA_STATUSCODE_BADWAITINGFORRESPONSE;
    }
    unlockServer(server);
    return result;
}

#endif /* UA_ENABLE_METHODCALLS */
