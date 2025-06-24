/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

#if UA_MULTITHREADING >= 100

static void
sendAsyncResponse(UA_Server *server, UA_AsyncManager *am,
                  UA_AsyncResponse *ar) {
    UA_assert(ar->opCountdown == 0);

    /* Get the session */
    UA_Session *session = getSessionById(server, &ar->sessionId);
    if(!session) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Async Service: Session %N no longer exists", ar->sessionId);
        UA_AsyncManager_removeAsyncResponse(&server->asyncManager, ar);
        return;
    }

    /* Check the channel */
    UA_SecureChannel *channel = session->channel;
    if(!channel) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "Async Service Response cannot be sent. "
                               "No SecureChannel for the session.");
        UA_AsyncManager_removeAsyncResponse(&server->asyncManager, ar);
        return;
    }

    /* Set the request handle */
    UA_ResponseHeader *responseHeader = (UA_ResponseHeader*)
        &ar->response.callResponse.responseHeader;
    responseHeader->requestHandle = ar->requestHandle;

    /* Send the Response */
    UA_StatusCode res = sendResponse(server, channel, ar->requestId,
                                     (UA_Response*)&ar->response,
                                     &UA_TYPES[UA_TYPES_CALLRESPONSE]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "Async Response for Req# %" PRIu32 " failed "
                               "with StatusCode %s", ar->requestId,
                               UA_StatusCode_name(res));
    }
    UA_AsyncManager_removeAsyncResponse(&server->asyncManager, ar);
}

static void
UA_AsyncManager_sendAsyncResponses(void *s, void *a) {
    UA_Server *server = (UA_Server*)s;
    UA_AsyncManager *am = (UA_AsyncManager*)a;

    UA_LOCK(&server->serviceMutex);
    UA_LOCK(&am->queueLock);

    /* Reset the delayed callback */
    UA_atomic_xchg((void**)&am->dc.callback, NULL);

    /* Send out ready responses */
    UA_AsyncResponse *ar, *temp;
    TAILQ_FOREACH_SAFE(ar, &am->asyncResponses, pointers, temp) {
        if(ar->opCountdown == 0)
            sendAsyncResponse(server, am, ar);
    }

    UA_UNLOCK(&am->queueLock);
    UA_UNLOCK(&server->serviceMutex);
}

static void
integrateResult(UA_Server *server, UA_AsyncManager *am,
                UA_AsyncOperation *ao) {
    UA_AsyncResponse *ar = ao->parent;
    ar->opCountdown -= 1;
    am->opsCount--;

    /* Delete the request information embedded in the ao */
    UA_CallMethodRequest_clear(&ao->request);

    /* Trigger the main server thread to send out responses */
    if(ar->opCountdown == 0 && am->dc.callback == NULL) {
        UA_EventLoop *el = server->config.eventLoop;

        am->dc.callback = UA_AsyncManager_sendAsyncResponses;
        am->dc.application = server;
        am->dc.context = am;
        el->addDelayedCallback(el, &am->dc);

        /* Wake up the EventLoop */
        el->cancel(el);
    }
}

/* Check if any operations have timed out */
static void
checkTimeouts(UA_Server *server, void *_) {
    /* Timeouts are not configured */
    if(server->config.asyncOperationTimeout <= 0.0)
        return;

    UA_EventLoop *el = server->config.eventLoop;
    UA_AsyncManager *am = &server->asyncManager;
    const UA_DateTime tNow = el->dateTime_nowMonotonic(el);

    UA_LOCK(&am->queueLock);

    /* Loop over the queue of dispatched ops */
    UA_AsyncOperation *op = NULL, *op_tmp = NULL;
    UA_AsyncOperationQueue *q = &am->dispatchedQueue;

 iterate:
    TAILQ_FOREACH_SAFE(op, q, pointers, op_tmp) {
        /* The timeout has not passed. Also for all elements following in the queue. */
        if(tNow <= op->parent->timeout)
            break;

        TAILQ_REMOVE(q, op, pointers);
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Operation was removed due to a timeout");

        /* Mark operation as timed out integrate */
        op->response->statusCode = UA_STATUSCODE_BADTIMEOUT;
        integrateResult(server, am, op);
    }

    if(q == &am->dispatchedQueue) {
        q = &am->newQueue;
        goto iterate;
    }

    UA_UNLOCK(&am->queueLock);
}

void
UA_AsyncManager_init(UA_AsyncManager *am, UA_Server *server) {
    memset(am, 0, sizeof(UA_AsyncManager));
    TAILQ_INIT(&am->asyncResponses);
    TAILQ_INIT(&am->newQueue);
    TAILQ_INIT(&am->dispatchedQueue);
    UA_LOCK_INIT(&am->queueLock);
}

void UA_AsyncManager_start(UA_AsyncManager *am, UA_Server *server) {
    /* Add a regular callback for cleanup and sending finished responses at a
     * 1s interval. */
    addRepeatedCallback(server, (UA_ServerCallback)checkTimeouts,
                        NULL, 1000.0, &am->checkTimeoutCallbackId);
}

void UA_AsyncManager_stop(UA_AsyncManager *am, UA_Server *server) {
    /* Add a regular callback for checking timeouts and sending finished
     * responses at a 100ms interval. */
    removeCallback(server, am->checkTimeoutCallbackId);
    if(am->dc.callback) {
        UA_EventLoop *el = server->config.eventLoop;
        el->removeDelayedCallback(el, &am->dc);
    }
}

void
UA_AsyncManager_clear(UA_AsyncManager *am, UA_Server *server) {
    UA_AsyncOperation *ar, *ar_tmp;

    /* Clean up queues */
    UA_LOCK(&am->queueLock);
    TAILQ_FOREACH_SAFE(ar, &am->newQueue, pointers, ar_tmp) {
        TAILQ_REMOVE(&am->newQueue, ar, pointers);
        UA_CallMethodRequest_clear(&ar->request);
    }
    TAILQ_FOREACH_SAFE(ar, &am->dispatchedQueue, pointers, ar_tmp) {
        TAILQ_REMOVE(&am->dispatchedQueue, ar, pointers);
        UA_CallMethodRequest_clear(&ar->request);
    }
    UA_UNLOCK(&am->queueLock);

    /* Remove responses */
    UA_AsyncResponse *current, *temp;
    TAILQ_FOREACH_SAFE(current, &am->asyncResponses, pointers, temp) {
        UA_AsyncManager_removeAsyncResponse(am, current);
    }

    /* Delete all locks */
    UA_LOCK_DESTROY(&am->queueLock);
}

void
UA_AsyncManager_removeAsyncResponse(UA_AsyncManager *am, UA_AsyncResponse *ar) {
    TAILQ_REMOVE(&am->asyncResponses, ar, pointers);
    UA_NodeId_clear(&ar->sessionId);

    /* Clean up the results array last. Because the rseults array has the ar
     * attached. */
    size_t resultsSize = *ar->resultsSize;
    void *results = *ar->results;
    const UA_DataType *resultsType = ar->resultsType;
    *ar->resultsSize = 0;
    *ar->results = NULL;

    UA_CallResponse_clear(&ar->response.callResponse);
    UA_Array_delete(results, resultsSize, resultsType);
}

/* Get and remove next Method Call Request */
UA_Boolean
UA_Server_getAsyncOperationNonBlocking(UA_Server *server, UA_AsyncOperationType *type,
                                       const UA_AsyncOperationRequest **request,
                                       void **context, UA_DateTime *timeout) {
    UA_AsyncManager *am = &server->asyncManager;

    UA_Boolean bRV = false;
    *type = UA_ASYNCOPERATIONTYPE_INVALID;
    UA_LOCK(&am->queueLock);
    UA_AsyncOperation *ao = TAILQ_FIRST(&am->newQueue);
    if(ao) {
        TAILQ_REMOVE(&am->newQueue, ao, pointers);
        TAILQ_INSERT_TAIL(&am->dispatchedQueue, ao, pointers);
        *type = UA_ASYNCOPERATIONTYPE_CALL;
        *request = (UA_AsyncOperationRequest*)&ao->request;
        *context = (void*)ao;
        if(timeout)
            *timeout = ao->parent->timeout;
        bRV = true;
    }
    UA_UNLOCK(&am->queueLock);

    return bRV;
}

/* Worker submits Method Call Response */
void
UA_Server_setAsyncOperationResult(UA_Server *server,
                                  const UA_AsyncOperationResponse *response,
                                  void *context) {
    UA_AsyncManager *am = &server->asyncManager;

    UA_AsyncOperation *ao = (UA_AsyncOperation*)context;
    if(!ao) {
        /* Something went wrong. Not a good AsyncOp. */
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "UA_Server_SetAsyncMethodResult: Invalid context");
        return;
    }

    UA_LOCK(&am->queueLock);

    /* See if the operation is still in the dispatched queue. Otherwise it has
     * been removed due to a timeout.
     *
     * TODO: Add a tree-structure for the dispatch queue. The linear lookup does
     * not scale. */
    UA_AsyncOperation *op = NULL;
    TAILQ_FOREACH(op, &am->dispatchedQueue, pointers) {
        if(op == ao)
            break;
    }
    if(!op) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "UA_Server_SetAsyncMethodResult: The operation has timed out");
        UA_UNLOCK(&am->queueLock);
        return;
    }

    /* Copy the result into the internal AsyncOperation */
    UA_StatusCode result =
        UA_CallMethodResult_copy(&response->callMethodResult, ao->response);
    if(result != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "UA_Server_SetAsyncMethodResult: UA_CallMethodResult_copy failed.");
        ao->response->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Remove from the dispatch queue */
    TAILQ_REMOVE(&am->dispatchedQueue, ao, pointers);

    UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                 "Return result in the server thread with %" PRIu32 " ops remaining",
                 op->parent->opCountdown);

    integrateResult(server, am, op);

    UA_UNLOCK(&am->queueLock);
}

/******************/
/* Server Methods */
/******************/

UA_StatusCode
UA_Server_setMethodNodeAsync(UA_Server *server, const UA_NodeId id,
                             UA_Boolean isAsync) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    lockServer(server);
    UA_Node *node =
        UA_NODESTORE_GET_EDIT_SELECTIVE(server, &id, UA_NODEATTRIBUTESMASK_NONE,
                                        UA_REFERENCETYPESET_NONE,
                                        UA_BROWSEDIRECTION_INVALID);
    if(node) {
        if(node->head.nodeClass == UA_NODECLASS_METHOD)
            node->methodNode.async = isAsync;
        else
            res = UA_STATUSCODE_BADNODECLASSINVALID;
        UA_NODESTORE_RELEASE(server, node);
    } else {
        res = UA_STATUSCODE_BADNODEIDINVALID;
    }
    unlockServer(server);
    return res;
}

UA_StatusCode
allocProcessServiceOperations_async(UA_Server *server, UA_Session *session,
                                    UA_UInt32 requestId, UA_UInt32 requestHandle,
                                    UA_AsyncServiceOperation operationCallback,
                                    const size_t *requestOperations,
                                    const UA_DataType *requestOperationsType,
                                    size_t *responseOperations,
                                    const UA_DataType *responseOperationsType) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_AsyncManager *am = &server->asyncManager;

    size_t ops = *requestOperations;
    if(ops == 0)
        return UA_STATUSCODE_BADNOTHINGTODO;

    /* Get the location of the response array */
    void **respPos = (void**)(((uintptr_t)responseOperations) + sizeof(size_t));

    /* Allocate the response array. The AsyncResponse and AsyncOperations are
     * added to the back. So they get cleaned up automatically. */
    size_t opsLen = responseOperationsType->memSize * ops;
    size_t len = opsLen + sizeof(UA_AsyncResponse) + (sizeof(UA_AsyncOperation) * ops);
    *respPos = UA_malloc(len);
    if(!*respPos)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(*respPos, 0, len);
    *responseOperations = ops;

    UA_AsyncResponse *ar = (UA_AsyncResponse*)(((uintptr_t)*respPos) + opsLen);
    uintptr_t aop = ((uintptr_t)ar) + sizeof(UA_AsyncResponse);

    /* Finish / dispatch the operations. This may allocate a new AsyncResponse internally */
    uintptr_t respOp = (uintptr_t)*respPos;
    uintptr_t reqOp = *(uintptr_t*)(((uintptr_t)requestOperations) + sizeof(size_t));
    for(size_t i = 0; i < ops; i++) {
        UA_AsyncOperation *op = (UA_AsyncOperation*)aop;
        UA_Boolean done = operationCallback(server, session, requestId, requestHandle,
                                            (void*)reqOp, (void*)respOp);
        if(!done) {
            if(server->config.maxAsyncOperationQueueSize != 0 &&
               am->opsCount >= server->config.maxAsyncOperationQueueSize) {
                UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                               "Cannot create an async operation: Queue exceeds limit (%d).",
                               (int unsigned)server->config.maxAsyncOperationQueueSize);
                /* TODO: Return a status code */
            } else {
                /* Enqueue the asyncop in the async manager */
                op->parent = ar;
                op->response = (UA_CallMethodResult*)respOp;
                UA_copy((void*)reqOp, &op->request, requestOperationsType);
                ar->opCountdown++;
                am->opsCount++;
                UA_LOCK(&am->queueLock);
                TAILQ_INSERT_TAIL(&am->newQueue, op, pointers);
                UA_UNLOCK(&am->queueLock);
            }
        }
        reqOp += requestOperationsType->memSize;
        respOp += responseOperationsType->memSize;
        aop += sizeof(UA_AsyncOperation);
    }

    /* Pending results, attach the AsyncResponse to the AsyncManager */
    if(ar->opCountdown > 0) {
        ar->requestId = requestId;
        ar->sessionId = session->sessionId;
        ar->requestHandle = requestHandle;
        ar->timeout = UA_INT64_MAX;
        UA_EventLoop *el = server->config.eventLoop;
        if(server->config.asyncOperationTimeout > 0.0)
            ar->timeout = el->dateTime_nowMonotonic(el) + (UA_DateTime)
                (server->config.asyncOperationTimeout * (UA_DateTime)UA_DATETIME_MSEC);

        /* Move the results array to the AsyncResponse.
         * It must not be freed from the calling method then. */

        /* TODO: Handle more response types */
        ar->response.callResponse.results = (UA_CallMethodResult*)*respPos;
        ar->response.callResponse.resultsSize = ops;
        ar->resultsSize = &ar->response.callResponse.resultsSize;
        ar->results = (void**)&ar->response.callResponse.results;
        ar->resultsType = responseOperationsType;
        *respPos = NULL;
        *responseOperations = 0;

        UA_LOCK(&am->queueLock);
        TAILQ_INSERT_TAIL(&am->asyncResponses, ar, pointers);
        UA_UNLOCK(&am->queueLock);

        /* Notify the server that async operations have been queued */
        if(server->config.asyncOperationNotifyCallback)
            server->config.asyncOperationNotifyCallback(server);
    }

    /* Signal in the status whether async operations are pending */
    return (ar->opCountdown == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

UA_UInt32
UA_AsyncManager_cancel(UA_Server *server, UA_Session *session, UA_UInt32 requestHandle) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_AsyncManager *am = &server->asyncManager;

    UA_LOCK(&am->queueLock);

    /* Loop over the newQueue, then the dispatchedQueue */
    UA_UInt32 count = 0;
    UA_AsyncOperation *op = NULL, *op_tmp = NULL;
    UA_AsyncOperationQueue *q = &am->dispatchedQueue;

 iterate:
    TAILQ_FOREACH_SAFE(op, q, pointers, op_tmp) {
        UA_AsyncResponse *ar = op->parent;
        if(ar->requestHandle != requestHandle ||
           !UA_NodeId_equal(&session->sessionId, &ar->sessionId))
            continue;

        /* Found the matching request */
        TAILQ_REMOVE(q, op, pointers);
        count++;

        /* Set the status of the overall response */
        ar->response.callResponse.responseHeader.serviceResult =
            UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT;

        /* Set operation status and integrate */
        op->response->statusCode = UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT;
        integrateResult(server, am, op);
    }

    if(q == &am->dispatchedQueue) {
        q = &am->newQueue;
        goto iterate;
    }

    UA_UNLOCK(&am->queueLock);
    return count;
}

#endif
