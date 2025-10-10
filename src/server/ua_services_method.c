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

static const UA_NodeId hasComponentNodeId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}};

struct LookupMethodNodeContext {
    UA_Server *server;
    UA_QualifiedName withBrowseName;
};

static void *
lookupSpecificMethodNodeCallback(void *context, UA_ReferenceTarget *t) {
    struct LookupMethodNodeContext *ctx = (struct LookupMethodNodeContext*)context;

    /* Get the method node. We only need the nodeClass and executable attribute.
     * Take all forward hasProperty references to get the input/output argument
     * definition variables. */
    const UA_Node *refTarget =
        UA_NODESTORE_GETFROMREF_SELECTIVE(ctx->server, t->targetId,
                                          UA_NODEATTRIBUTESMASK_NODECLASS |
                                          UA_NODEATTRIBUTESMASK_EXECUTABLE,
                                          UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASPROPERTY),
                                          UA_BROWSEDIRECTION_FORWARD);
    if(!refTarget)
        return NULL;
    if(refTarget->head.nodeClass == UA_NODECLASS_METHOD &&
       UA_QualifiedName_equal(&ctx->withBrowseName, &refTarget->head.browseName)) {
        return (void*)(uintptr_t)&refTarget->methodNode;
    }
    UA_NODESTORE_RELEASE(ctx->server, refTarget);
    return NULL;
}

static const UA_MethodNode *
lookupSpecificMethodNode(UA_Server *server, const UA_NodeHead *head,
                         UA_QualifiedName withBrowseName) {
    for(size_t i = 0; i < head->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &head->references[i];
        if(rk->isInverse)
            continue;
        if(rk->referenceTypeIndex != UA_REFERENCETYPEINDEX_HASCOMPONENT)
            continue;
        struct LookupMethodNodeContext ctx;
        ctx.server = server;
        ctx.withBrowseName = withBrowseName;
        return (const UA_MethodNode*)
            UA_NodeReferenceKind_iterate(rk, lookupSpecificMethodNodeCallback, &ctx);
    }
    return NULL;
}

static UA_Boolean
checkMethodReference(const UA_NodeHead *h, UA_ReferenceTypeSet refs,
                     const UA_ExpandedNodeId *methodId) {
    for(size_t i = 0; i < h->referencesSize; i++) {
        const UA_NodeReferenceKind *rk = &h->references[i];
        if(rk->isInverse)
            continue;
        if(!UA_ReferenceTypeSet_contains(&refs, rk->referenceTypeIndex))
            continue;
        if(UA_NodeReferenceKind_findTarget(rk, methodId))
            return true;
    }
    return false;
}

static UA_Boolean
checkMethodReferenceRecursive(UA_Server *server, const UA_NodeHead *h,
                              UA_ReferenceTypeSet refs, const UA_ExpandedNodeId *methodId) {
    if(checkMethodReference(h, refs, methodId))
        return true;

    for(size_t i = 0; i < h->referencesSize; i++) {
        const UA_NodeReferenceKind *rk = &h->references[i];

        if(!rk->isInverse)
            continue;
        if(rk->referenceTypeIndex != UA_REFERENCETYPEINDEX_HASSUBTYPE)
            continue;

        UA_NodeId targetId;
        if(!rk->hasRefTree) {
            if(rk->targetsSize == 0)
                continue;
            targetId = UA_NodePointer_toNodeId(rk->targets.array[0].targetId);
        } else {
            if(!rk->targets.tree.idRoot)
                continue;
            targetId = UA_NodePointer_toNodeId(rk->targets.tree.idRoot->target.targetId);
        }

        const UA_Node *superType = UA_NODESTORE_GET(server, &targetId);
        if(!superType)
            continue;

        UA_Boolean found = checkMethodReferenceRecursive(server, &superType->head, refs, methodId);
        UA_NODESTORE_RELEASE(server, superType);

        if(found)
            return true;
    }
    return false;
}

static void
callWithMethodAndContext(UA_Server *server, UA_Session *session,
                         const UA_CallMethodRequest *request, UA_CallMethodResult *result,
                         const UA_Node *calledMethod, const UA_Node *callContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Verify the context object's NodeClass */
    if(callContext->head.nodeClass != UA_NODECLASS_OBJECT &&
        callContext->head.nodeClass != UA_NODECLASS_OBJECTTYPE) {
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        return;
    }

    /* Verify the method's NodeClass */
    if(calledMethod->head.nodeClass != UA_NODECLASS_METHOD) {
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        return;
    }

    /* Verify method/object relations. Object must have a hasComponent or a
     * subtype of hasComponent reference to the method node. Therefore, check
     * every reference between the parent object and the method node if there is
     * a hasComponent (or subtype) reference */
    UA_ExpandedNodeId methodId = UA_EXPANDEDNODEID_NODEID(request->methodId);
    UA_ReferenceTypeSet hasComponentRefs;
    result->statusCode = referenceTypeIndices(server, &hasComponentNodeId,
                                              &hasComponentRefs, true);
    UA_CHECK_STATUS(result->statusCode, return);
    UA_Boolean found = checkMethodReference(&callContext->head, hasComponentRefs, &methodId);

    /* For object types the given method must be directly referenced!
     * OPC UA 1.05.06 will state in Part 4 - Call Service Parameters
     * the following for parameter "objectId":
     * "In case of an ObjectType the ObjectType shall be the source of a
     *  HasComponent Reference (or subtype of HasComponent Reference) to the
     *  Method specified in methodId" */
    if(!found && (callContext->head.nodeClass == UA_NODECLASS_OBJECT)) {
        /* If the object doesn't have a hasComponent reference to the method node,
         * check its objectType (and its supertypes). Invoked method can be a component
         * of objectType and be invoked on this objectType's instance (or on a instance
         * of one of its subtypes). */
        const UA_Node *objectType = getNodeType(server, &callContext->head);
        if(objectType) {
            found = checkMethodReferenceRecursive(server, &objectType->head,
                                                  hasComponentRefs, &methodId);
            UA_NODESTORE_RELEASE(server, objectType);
        }
    }

    if(!found) {
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
        return;
    }

    /* Resolve to method node in the context
     * OPC UA 1.05 (and earlier) states in Part 3 - Methods:
     * "Methods are “lightweight” functions, whose scope is bound by an owning
     *  (see Note) Object, similar to the methods of a class in object-oriented
     *  programming or an owning ObjectType, similar to static methods of a
     *  class.
     *  NOTE The owning Object or ObjectType is specified in the service call
     *  when invoking a Method."
     * So it is important to resolve this ownership here to allow checking the
     * role permissions at the correct method node and to execute the correct
     * callback. */
    const UA_MethodNode* methodInContext =
        lookupSpecificMethodNode(server, &callContext->head,
                                 calledMethod->head.browseName);

    if(methodInContext == NULL) {
        result->statusCode = UA_STATUSCODE_BADMETHODINVALID;
        return;
    }

    /* Is there a method to execute? */
    if(!methodInContext->method) {
        result->statusCode = UA_STATUSCODE_BADINTERNALERROR;
        UA_NODESTORE_RELEASE(server, (const UA_Node*)methodInContext);
        return;
    }

    /* Verify access rights */
    UA_Boolean executable = methodInContext->executable;
    if(session != &server->adminSession) {
        executable = executable && server->config.accessControl.
            getUserExecutableOnObject(server, &server->config.accessControl,
                                      &session->sessionId, session->context,
                                      &methodInContext->head.nodeId, methodInContext->head.context,
                                      &request->objectId, callContext->head.context);
    }

    if(!executable) {
        result->statusCode = UA_STATUSCODE_BADNOTEXECUTABLE;
        UA_NODESTORE_RELEASE(server, (const UA_Node*)methodInContext);
        return;
    }

    /* The input arguments are const and not changed. We move the input
     * arguments to a secondary array that is mutable. This is used for small
     * adjustments on the type level during the type checking. But it has to be
     * ensured that the original array can still by _clear'ed after the methods
     * call. */
    if(request->inputArgumentsSize > UA_MAX_METHOD_ARGUMENTS) {
        result->statusCode = UA_STATUSCODE_BADTOOMANYARGUMENTS;
        UA_NODESTORE_RELEASE(server, (const UA_Node*)methodInContext);
        return;
    }
    UA_Variant mutableInputArgs[UA_MAX_METHOD_ARGUMENTS];
    if(request->inputArgumentsSize > 0) {
        memcpy(mutableInputArgs, request->inputArguments,
               sizeof(UA_Variant) * request->inputArgumentsSize);
    }

    /* Allocate the inputArgumentResults array */
    result->inputArgumentResults = (UA_StatusCode*)
        UA_Array_new(request->inputArgumentsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!result->inputArgumentResults) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_NODESTORE_RELEASE(server, (const UA_Node*)methodInContext);
        return;
    }
    result->inputArgumentResultsSize = request->inputArgumentsSize;

    /* Type-check the input arguments */
    const UA_VariableNode *inputArguments =
        getArgumentsVariableNode(server, &methodInContext->head, UA_STRING("InputArguments"));
    if(inputArguments) {
        result->statusCode =
            checkAdjustArguments(server, session, inputArguments, request->inputArgumentsSize,
                                 mutableInputArgs, result->inputArgumentResults);
        UA_NODESTORE_RELEASE(server, (const UA_Node*)inputArguments);
    } else if(request->inputArgumentsSize > 0) {
        result->statusCode = UA_STATUSCODE_BADTOOMANYARGUMENTS;
        UA_NODESTORE_RELEASE(server, (const UA_Node*)methodInContext);
        return;
    }

    /* Return inputArgumentResults only for BADINVALIDARGUMENT */
    if(result->statusCode != UA_STATUSCODE_BADINVALIDARGUMENT) {
        UA_Array_delete(result->inputArgumentResults, result->inputArgumentResultsSize,
                        &UA_TYPES[UA_TYPES_STATUSCODE]);
        result->inputArgumentResults = NULL;
        result->inputArgumentResultsSize = 0;
    }

    /* Error during type-checking? */
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_NODESTORE_RELEASE(server, (const UA_Node*)methodInContext);
        return;
    }

    /* Get the output arguments node */
    const UA_VariableNode *outputArguments =
        getArgumentsVariableNode(server, &methodInContext->head, UA_STRING("OutputArguments"));

    /* Allocate the output arguments array. Always allocate memory, hence the
     * +1, even if the length is zero. Because we need a unique outputArguments
     * pointer as the key for async operations. The memory gets deleted in
     * UA_Array_delete even if the outputArgumentsSize is zero. */
    size_t outputArgsSize = 0;
    if(outputArguments)
        outputArgsSize = outputArguments->valueSource.internal.value.value.arrayLength;
    result->outputArguments = (UA_Variant*)
        UA_Array_new(outputArgsSize+1, &UA_TYPES[UA_TYPES_VARIANT]);
    if(!result->outputArguments) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_NODESTORE_RELEASE(server, (const UA_Node*)methodInContext);
        return;
    }
    result->outputArgumentsSize = outputArgsSize;

    /* Release the output arguments node */
    UA_NODESTORE_RELEASE(server, (const UA_Node*)outputArguments);

    /* Call the method. If this is an async method, unlock the server lock for
     * the duration of the (long-running) call. */
    result->statusCode = methodInContext->method(
        server, &session->sessionId, session->context,
        &methodInContext->head.nodeId, methodInContext->head.context,
        &callContext->head.nodeId, callContext->head.context,
        request->inputArgumentsSize, mutableInputArgs,
        result->outputArgumentsSize, result->outputArguments);

    UA_NODESTORE_RELEASE(server, (const UA_Node*)methodInContext);

    /* TODO: Verify Output matches the argument definition */
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
    callWithMethodAndContext(server, session, request, result,
                             method, object);

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
