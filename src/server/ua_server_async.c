/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

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

    /* Reset the delayed callback */
    UA_atomic_xchg((void**)&am->dc.callback, NULL);

    /* Send out ready responses */
    UA_AsyncResponse *ar, *temp;
    TAILQ_FOREACH_SAFE(ar, &am->asyncResponses, pointers, temp) {
        if(ar->opCountdown == 0)
            sendAsyncResponse(server, am, ar);
    }

    UA_UNLOCK(&server->serviceMutex);
}

static void
integrateResult(UA_Server *server, UA_AsyncManager *am, UA_AsyncOperation *op) {
    UA_AsyncResponse *ar = op->parent;
    ar->opCountdown -= 1;
    am->opsCount--;

    TAILQ_REMOVE(&am->ops, op, pointers);

    /* Delete the request information embedded in the op  */
    UA_CallMethodRequest_clear(&op->request);

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

    lockServer(server);

    /* Loop over the queue of dispatched ops */
    UA_AsyncOperation *op = NULL, *op_tmp = NULL;
    TAILQ_FOREACH_SAFE(op, &am->ops, pointers, op_tmp) {
        /* The timeout has not passed. Also for all elements following in the queue. */
        if(tNow <= op->parent->timeout)
            break;

        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Operation was removed due to a timeout");

        /* Mark operation as timed out integrate */
        op->response->statusCode = UA_STATUSCODE_BADTIMEOUT;
        integrateResult(server, am, op);
    }

    unlockServer(server);
}

void
UA_AsyncManager_init(UA_AsyncManager *am, UA_Server *server) {
    memset(am, 0, sizeof(UA_AsyncManager));
    TAILQ_INIT(&am->asyncResponses);
    TAILQ_INIT(&am->ops);
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
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Clean up queues */
    UA_AsyncOperation *op, *op_tmp;
    TAILQ_FOREACH_SAFE(op, &am->ops, pointers, op_tmp) {
        TAILQ_REMOVE(&am->ops, op, pointers);
        UA_CallMethodRequest_clear(&op->request);
    }

    /* Remove responses */
    UA_AsyncResponse *ar, *ar_tmp;
    TAILQ_FOREACH_SAFE(ar, &am->asyncResponses, pointers, ar_tmp) {
        UA_AsyncManager_removeAsyncResponse(am, ar);
    }
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

UA_StatusCode
UA_Server_setAsyncCallMethodResult(UA_Server *server, UA_StatusCode operationStatus,
                                   UA_Variant *output) {
    lockServer(server);

    UA_AsyncManager *am = &server->asyncManager;

    /* See if the operation is still in the dispatched queue. Otherwise it has
     * been removed due to a timeout.
     *
     * TODO: Add a tree-structure for the dispatch queue. The linear lookup does
     * not scale. */
    UA_AsyncOperation *op = NULL;
    TAILQ_FOREACH(op, &am->ops, pointers) {
        /* If the output length is zero, we get a unique pointer directly to the
         * response structure */
        if(op->response->outputArguments == output || (UA_Variant*)op->response == output)
            break;
    }
    if(!op) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "UA_Server_SetAsyncMethodResult: The operation has timed out");
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;;
    }

    integrateResult(server, am, op);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

/******************/
/* Server Methods */
/******************/

static void
setResultStatus(void *resp, const UA_DataType *responseOperationsType,
                UA_StatusCode status) {
    if(responseOperationsType == &UA_TYPES[UA_TYPES_CALLMETHODRESULT]) {
        UA_CallMethodResult *cmr = (UA_CallMethodResult*)resp;
        cmr->statusCode = status;
    } else if(responseOperationsType == &UA_TYPES[UA_TYPES_STATUSCODE]) {
        UA_StatusCode *s = (UA_StatusCode*)resp;
        *s = status;
    } else if(responseOperationsType == &UA_TYPES[UA_TYPES_DATAVALUE]) {
        UA_DataValue *dv = (UA_DataValue*)resp;
        dv->hasStatus = true;
        dv->status = status;
    }
}

UA_StatusCode
allocProcessServiceOperations_async(UA_Server *server, UA_Session *session,
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

    /* Execute the operations */
    uintptr_t respOp = (uintptr_t)*respPos;
    uintptr_t reqOp = *(uintptr_t*)(((uintptr_t)requestOperations) + sizeof(size_t));
    for(size_t i = 0; i < ops; i++) {
        UA_AsyncOperation *op = (UA_AsyncOperation*)aop;
        UA_Boolean done = operationCallback(server, session, (void*)reqOp, (void*)respOp);
        if(!done) {
            if(server->config.maxAsyncOperationQueueSize != 0 &&
               am->opsCount >= server->config.maxAsyncOperationQueueSize) {
                UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                               "Cannot create an async operation: Queue exceeds limit (%d).",
                               (int unsigned)server->config.maxAsyncOperationQueueSize);
                setResultStatus((void*)respOp, responseOperationsType,
                                UA_STATUSCODE_BADTOOMANYOPERATIONS);
            } else {
                /* Enqueue the asyncop in the async manager */
                op->parent = ar;
                op->response = (UA_CallMethodResult*)respOp;
                UA_copy((void*)reqOp, &op->request, requestOperationsType);
                ar->opCountdown++;
                am->opsCount++;
                TAILQ_INSERT_TAIL(&am->ops, op, pointers);
            }
        }
        reqOp += requestOperationsType->memSize;
        respOp += responseOperationsType->memSize;
        aop += sizeof(UA_AsyncOperation);
    }

    /* Pending results, attach the AsyncResponse to the AsyncManager */
    if(ar->opCountdown > 0) {
        /* RequestId and -Handle are set in the AsyncManager before processing
         * the request */
        ar->requestId = am->currentRequestId;
        ar->requestHandle = am->currentRequestHandle;

        ar->sessionId = session->sessionId;
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

        TAILQ_INSERT_TAIL(&am->asyncResponses, ar, pointers);
    }

    /* Signal in the status whether async operations are pending */
    return (ar->opCountdown == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

UA_UInt32
UA_AsyncManager_cancel(UA_Server *server, UA_Session *session, UA_UInt32 requestHandle) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_AsyncManager *am = &server->asyncManager;

    /* Loop over the newQueue, then the dispatchedQueue */
    UA_UInt32 count = 0;
    UA_AsyncOperation *op = NULL, *op_tmp = NULL;
    TAILQ_FOREACH_SAFE(op, &am->ops, pointers, op_tmp) {
        UA_AsyncResponse *ar = op->parent;
        if(ar->requestHandle != requestHandle ||
           !UA_NodeId_equal(&session->sessionId, &ar->sessionId))
            continue;

        /* Found the matching request */
        count++;

        /* Notify that the async operations is removed
         * TODO: Enable other async operation types */
        if(server->config.asyncOperationCancelCallback) {
            if(op->response->outputArgumentsSize> 0)
                server->config.asyncOperationCancelCallback(server, op->response->outputArguments);
            else
                server->config.asyncOperationCancelCallback(server, op->response);
        }

        /* Set the status of the overall response */
        ar->response.callResponse.responseHeader.serviceResult =
            UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT;

        /* Set operation status and integrate */
        op->response->statusCode = UA_STATUSCODE_BADOPERATIONABANDONED;
        integrateResult(server, am, op);
    }

    return count;
}
