/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

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
    UA_StatusCode res =
        sendResponse(server, channel, ar->requestId, (UA_Response*)&ar->response,
                     ar->asyncServiceDescription->responseType);
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
    TAILQ_REMOVE(&am->ops, op, pointers);

    UA_AsyncResponse *ar = op->parent;
    ar->opCountdown -= 1;
    am->opsCount--;

    /* Trigger the main server thread to send out responses */
    if(ar->opCountdown == 0 && am->dc.callback == NULL) {
        UA_EventLoop *el = server->config.eventLoop;
        am->dc.callback = UA_AsyncManager_sendAsyncResponses;
        am->dc.application = server;
        am->dc.context = am;
        el->addDelayedCallback(el, &am->dc);
        el->cancel(el); /* Wake up the EventLoop if currently waiting in select() */
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
        const UA_AsyncServiceDescription *asd = op->parent->asyncServiceDescription;
        setResultStatus(op->opResult, asd->responseOperationsType, UA_STATUSCODE_BADTIMEOUT);
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

    /* Clean up queues. The operations get deleted together with the
     * AsyncResponse below. */
    UA_AsyncOperation *op, *op_tmp;
    TAILQ_FOREACH_SAFE(op, &am->ops, pointers, op_tmp) {
        TAILQ_REMOVE(&am->ops, op, pointers);
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

    /* Clean up the results array last. Because the rseults array contains more
     * data, including the ar. */
    const UA_AsyncServiceDescription *asd = ar->asyncServiceDescription;
    uintptr_t resultsSizePos = ((uintptr_t)&ar->response) + asd->responseCounterOffset;
    size_t *resultsSize = (size_t*)resultsSizePos;
    void **results = (void**)(resultsSizePos + sizeof(size_t));
    size_t origResultsSize = *resultsSize;
    void *origResults = *results;
    *resultsSize = 0;
    *results = NULL;

    UA_CallResponse_clear(&ar->response.callResponse);
    UA_Array_delete(origResults, origResultsSize, asd->responseOperationsType);
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
        if(((UA_CallMethodResult*)op->opResult)->outputArguments == output)
            break;
    }
    if(!op) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "UA_Server_SetAsyncCallMethodResult: The operation has timed out");
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

UA_StatusCode
allocProcessServiceOperations_async(UA_Server *server, UA_Session *session,
                                    const UA_AsyncServiceDescription *asd,
                                    const void *request, void *response) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_AsyncManager *am = &server->asyncManager;

    uintptr_t requestCountPos = ((uintptr_t)request) + asd->requestCounterOffset;
    size_t reqSize = *(size_t*)requestCountPos;
    const void *reqArray = *(const void**)(requestCountPos + sizeof(size_t));
    if(reqSize == 0)
        return UA_STATUSCODE_BADNOTHINGTODO;

    /* Get the location of the response array */
    uintptr_t responseCountPos = ((uintptr_t)response) + asd->responseCounterOffset;
    size_t *respSizePtr = (size_t*)responseCountPos;
    void **respArrayPtr = (void**)(responseCountPos + sizeof(size_t));

    /* Allocate the response array. The AsyncResponse and AsyncOperations are
     * added to the back. So they get cleaned up automatically. */
    size_t opsLen = asd->responseOperationsType->memSize * reqSize;
    size_t len = opsLen + sizeof(UA_AsyncResponse) + (sizeof(UA_AsyncOperation) * reqSize);
    void *respArray = UA_malloc(len);
    if(!respArray)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(respArray, 0, len);
    *respSizePtr = reqSize;
    *respArrayPtr = respArray;

    /* Execute the operations */
    uintptr_t reqOp = (uintptr_t)reqArray;
    uintptr_t respOp = (uintptr_t)respArray;
    uintptr_t aop = respOp + opsLen + sizeof(UA_AsyncResponse);
    UA_AsyncResponse *ar = (UA_AsyncResponse*)(respOp + opsLen);
    for(size_t i = 0; i < reqSize; i++) {
        UA_Boolean done = asd->operationCallback(server, session, (void*)reqOp, (void*)respOp);
        if(!done) {
            if(server->config.maxAsyncOperationQueueSize != 0 &&
               am->opsCount >= server->config.maxAsyncOperationQueueSize) {
                UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                               "Cannot create an async operation: Queue exceeds limit (%d).",
                               (int unsigned)server->config.maxAsyncOperationQueueSize);
                setResultStatus((void*)respOp, asd->responseOperationsType,
                                UA_STATUSCODE_BADTOOMANYOPERATIONS);
            } else {
                /* Enqueue the asyncop in the async manager */
                UA_AsyncOperation *op = (UA_AsyncOperation*)aop;
                op->parent = ar;
                op->opResult = (void*)respOp;
                ar->opCountdown++;
                am->opsCount++;
                TAILQ_INSERT_TAIL(&am->ops, op, pointers);
            }
        }
        reqOp += asd->requestOperationsType->memSize;
        respOp += asd->responseOperationsType->memSize;
        aop += sizeof(UA_AsyncOperation);
    }

    /* Pending results, attach the AsyncResponse to the AsyncManager */
    if(ar->opCountdown > 0) {
        /* RequestId and -Handle are set in the AsyncManager before processing
         * the request */
        ar->asyncServiceDescription = asd;
        ar->requestId = am->currentRequestId;
        ar->requestHandle = am->currentRequestHandle;
        ar->sessionId = session->sessionId;
        ar->timeout = UA_INT64_MAX;

        UA_EventLoop *el = server->config.eventLoop;
        if(server->config.asyncOperationTimeout > 0.0)
            ar->timeout = el->dateTime_nowMonotonic(el) + (UA_DateTime)
                (server->config.asyncOperationTimeout * (UA_DateTime)UA_DATETIME_MSEC);

        /* Move the response content to the AsyncResponse */
        memcpy(&ar->response, response, asd->responseType->memSize);
        UA_init(response, asd->responseType);

        /* Enqueue the ar */
        TAILQ_INSERT_TAIL(&am->asyncResponses, ar, pointers);
    }

    /* Signal in the status whether async operations are pending */
    return (ar->opCountdown == 0) ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

UA_UInt32
UA_AsyncManager_cancel(UA_Server *server, UA_Session *session, UA_UInt32 requestHandle) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Loop over all queued operations */
    UA_UInt32 count = 0;
    UA_AsyncOperation *op, *op_tmp;
    UA_AsyncManager *am = &server->asyncManager;
    TAILQ_FOREACH_SAFE(op, &am->ops, pointers, op_tmp) {
        UA_AsyncResponse *ar = op->parent;
        if(ar->requestHandle != requestHandle ||
           !UA_NodeId_equal(&session->sessionId, &ar->sessionId))
            continue;

        count++; /* Found the matching request */

        /* Notify that the async operations is removed
         * TODO: Enable other async operation types */
        const UA_AsyncServiceDescription *asd = ar->asyncServiceDescription;
        if(server->config.asyncOperationCancelCallback) {
            if(asd->responseType == &UA_TYPES[UA_TYPES_CALLMETHODRESULT]) {
                UA_CallMethodResult *res = (UA_CallMethodResult*)op->opResult;
                server->config.asyncOperationCancelCallback(server, res->outputArguments);
            } else {
                server->config.asyncOperationCancelCallback(server, op->opResult);
            }
        }

        /* Set the status of the overall response */
        ar->response.callResponse.responseHeader.serviceResult =
            UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT;

        /* Set operation status and integrate */
        setResultStatus(op->opResult, asd->responseType,
                        UA_STATUSCODE_BADOPERATIONABANDONED);
        integrateResult(server, am, op);
    }

    return count;
}
