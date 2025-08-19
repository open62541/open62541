/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 *    Copyright 2019, 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

static void
UA_AsyncOperation_cancel(UA_Server *server, UA_AsyncOperation *op, UA_StatusCode status) {
    UA_AsyncOperationType type = op->asyncOperationType | 0x04;
    switch(type) {
    case UA_ASYNCOPERATIONTYPE_CALL_DIRECT:
        /* The method out array is always an allocated pointer. Also if the length is zero. */
        if(server->config.asyncOperationCancelCallback)
            server->config.asyncOperationCancelCallback(server, op->output.call->outputArguments);
        op->output.call->statusCode = status;
        break;
    case UA_ASYNCOPERATIONTYPE_READ_DIRECT:
    default:
        if(server->config.asyncOperationCancelCallback)
            server->config.asyncOperationCancelCallback(server, op->output.read);
        op->output.read->hasStatus = true;
        op->output.read->status = status;
        break;
    case UA_ASYNCOPERATIONTYPE_WRITE_DIRECT:
        if(server->config.asyncOperationCancelCallback)
            server->config.asyncOperationCancelCallback(server, &op->context.writeValue.value);
        *op->output.write = status;
        break;
    }
}

static void
UA_AsyncManager_removeAsyncResponse(UA_AsyncManager *am, UA_AsyncResponse *ar) {
    TAILQ_REMOVE(&am->asyncResponses, ar, pointers);
    UA_NodeId_clear(&ar->sessionId);

    /* Clean up the results array last. Because the results array memory also
     * includes ar. */
    void *arr = NULL;
    size_t arrSize = 0;
    const UA_DataType *arrType;
    if(ar->responseType == &UA_TYPES[UA_TYPES_CALLRESPONSE]) {
        arr = ar->response.callResponse.results;
        arrSize = ar->response.callResponse.resultsSize;
        ar->response.callResponse.results = NULL;
        ar->response.callResponse.resultsSize = 0;
        arrType = &UA_TYPES[UA_TYPES_CALLMETHODRESULT];
    } else if(ar->responseType == &UA_TYPES[UA_TYPES_READRESPONSE]) {
        arr = ar->response.readResponse.results;
        arrSize = ar->response.readResponse.resultsSize;
        ar->response.readResponse.results = NULL;
        ar->response.readResponse.resultsSize = 0;
        arrType = &UA_TYPES[UA_TYPES_DATAVALUE];
    } else /* if(ar->responseType == &UA_TYPES[UA_TYPES_WRITERESPONSE]) */ {
        UA_assert(ar->responseType == &UA_TYPES[UA_TYPES_WRITERESPONSE]);
        arr = ar->response.writeResponse.results;
        arrSize = ar->response.writeResponse.resultsSize;
        ar->response.writeResponse.results = NULL;
        ar->response.writeResponse.resultsSize = 0;
        arrType = &UA_TYPES[UA_TYPES_STATUSCODE];
    }
    UA_clear(&ar->response.callResponse, ar->responseType);
    UA_Array_delete(arr, arrSize, arrType);
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
    UA_StatusCode res = sendResponse(server, channel, ar->requestId,
                                     (UA_Response*)&ar->response, ar->responseType);
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

    UA_AsyncResponse *ar = op->handling.response;
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
        if(tNow <= op->handling.response->timeout)
            break;

        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Operation was removed due to a timeout");

        /* Mark operation as timed out integrate */
        UA_AsyncOperation_cancel(server, op, UA_STATUSCODE_BADTIMEOUT);
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
        UA_AsyncOperation_cancel(server, op, UA_STATUSCODE_BADSHUTDOWN);
        TAILQ_REMOVE(&am->ops, op, pointers);
    }

    /* Remove responses */
    UA_AsyncResponse *ar, *ar_tmp;
    TAILQ_FOREACH_SAFE(ar, &am->asyncResponses, pointers, ar_tmp) {
        UA_AsyncManager_removeAsyncResponse(am, ar);
    }
}

static void
persistAsyncResponse(UA_Server *server, UA_Session *session,
                     void *response, UA_AsyncResponse *ar) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_AsyncManager *am = &server->asyncManager;

    /* Pending results, attach the AsyncResponse to the AsyncManager. RequestId
     * and -Handle are set in the AsyncManager before processing the request. */
    ar->requestId = am->currentRequestId;
    ar->requestHandle = am->currentRequestHandle;
    ar->sessionId = session->sessionId;
    ar->timeout = UA_INT64_MAX;

    UA_EventLoop *el = server->config.eventLoop;
    if(server->config.asyncOperationTimeout > 0.0)
        ar->timeout = el->dateTime_nowMonotonic(el) + (UA_DateTime)
            (server->config.asyncOperationTimeout * (UA_DateTime)UA_DATETIME_MSEC);

    /* Move the response content to the AsyncResponse */
    memcpy(&ar->response, response, ar->responseType->memSize);
    UA_init(response, ar->responseType);

    /* Enqueue the ar */
    TAILQ_INSERT_TAIL(&am->asyncResponses, ar, pointers);
}

UA_Boolean
Service_Read(UA_Server *server, UA_Session *session,
             const UA_ReadRequest *request, UA_ReadResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session, "Processing ReadRequest");
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Check if the timestampstoreturn is valid */
    if(request->timestampsToReturn > UA_TIMESTAMPSTORETURN_NEITHER) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
        return true;
    }

    /* Check if maxAge is valid */
    if(request->maxAge < 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADMAXAGEINVALID;
        return true;
    }

    /* Check if there are too many operations */
    if(server->config.maxNodesPerRead != 0 &&
       request->nodesToReadSize > server->config.maxNodesPerRead) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return true;
    }

    /* Check if there are no operations */
    if(request->nodesToReadSize == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return true;
    }

    /* Allocate the response array. The AsyncResponse and AsyncOperations are
     * added to the back. So they get cleaned up automatically if none of the
     * calls are async. */
    size_t opsLen = sizeof(UA_DataValue) * request->nodesToReadSize;
    size_t len = opsLen + sizeof(UA_AsyncResponse) +
        (sizeof(UA_AsyncOperation) * request->nodesToReadSize);
    response->results = (UA_DataValue*)UA_calloc(1, len);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return true;
    }
    response->resultsSize = request->nodesToReadSize;

    UA_AsyncResponse *ar = (UA_AsyncResponse*)&response->results[response->resultsSize];
    UA_AsyncOperation *aopArray = (UA_AsyncOperation*)&ar[1];

    /* Execute the operations */
    UA_AsyncManager *am = &server->asyncManager;
    for(size_t i = 0; i < request->nodesToReadSize; i++) {
        UA_Boolean done = Operation_Read(server, session,
                                         request->timestampsToReturn,
                                         &request->nodesToRead[i],
                                         &response->results[i]);
        if(done)
            continue;

        /* Set up the async operation */
        UA_AsyncOperation *op = &aopArray[i];
        op->asyncOperationType = UA_ASYNCOPERATIONTYPE_READ_REQUEST;
        op->handling.response = ar;
        op->output.read = &response->results[i];

        /* Not enough resources to store the async operation */
        if(server->config.maxAsyncOperationQueueSize != 0 &&
           am->opsCount >= server->config.maxAsyncOperationQueueSize) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Cannot create an async read operation: Queue exceeds limit (%d).",
                           (int unsigned)server->config.maxAsyncOperationQueueSize);
            UA_AsyncOperation_cancel(server, op, UA_STATUSCODE_BADTOOMANYOPERATIONS);
            continue;
        }

        /* Enqueue the asyncop in the async manager */
        ar->opCountdown++;
        am->opsCount++;
        TAILQ_INSERT_TAIL(&am->ops, op, pointers);
    }

    /* If async operations are pending, persist them and signal the service is
     * not done */
    if(ar->opCountdown > 0) {
        ar->responseType = &UA_TYPES[UA_TYPES_READRESPONSE];
        persistAsyncResponse(server, session, response, ar);
    }
    return (ar->opCountdown == 0);
}

UA_Boolean
Service_Write(UA_Server *server, UA_Session *session, const UA_WriteRequest *request,
              UA_WriteResponse *response) {
    UA_assert(session != NULL);
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing WriteRequest");
    UA_LOCK_ASSERT(&server->serviceMutex);

    if(server->config.maxNodesPerWrite != 0 &&
       request->nodesToWriteSize > server->config.maxNodesPerWrite) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return true;
    }

    if(request->nodesToWriteSize == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return true;
    }

    /* Allocate the response array. The AsyncResponse and AsyncOperations are
     * added to the back. So they get cleaned up automatically if none of the
     * calls are async. */
    size_t opsLen = sizeof(UA_StatusCode) * request->nodesToWriteSize;
    size_t len = opsLen + sizeof(UA_AsyncResponse) +
        (sizeof(UA_AsyncOperation) * request->nodesToWriteSize);
    response->results = (UA_StatusCode*)UA_calloc(1, len);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return true;
    }
    response->resultsSize = request->nodesToWriteSize;

    UA_AsyncResponse *ar = (UA_AsyncResponse*)&response->results[response->resultsSize];
    UA_AsyncOperation *aopArray = (UA_AsyncOperation*)&ar[1];

    /* Execute the operations */
    UA_AsyncManager *am = &server->asyncManager;
    for(size_t i = 0; i < request->nodesToWriteSize; i++) {
        UA_Boolean done = Operation_Write(server, session,
                                         &request->nodesToWrite[i],
                                         &response->results[i]);
        if(done)
            continue;

        /* Set up the async operation */
        UA_AsyncOperation *op = &aopArray[i];
        op->asyncOperationType = UA_ASYNCOPERATIONTYPE_WRITE_REQUEST;
        op->handling.response = ar;
        op->output.write = &response->results[i];

        /* Not enough resources to store the async operation */
        if(server->config.maxAsyncOperationQueueSize != 0 &&
           am->opsCount >= server->config.maxAsyncOperationQueueSize) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Cannot create an async write operation: Queue exceeds limit (%d).",
                           (int unsigned)server->config.maxAsyncOperationQueueSize);
            UA_AsyncOperation_cancel(server, op, UA_STATUSCODE_BADTOOMANYOPERATIONS);
            continue;
        }

        /* Enqueue the asyncop in the async manager */
        ar->opCountdown++;
        am->opsCount++;
        TAILQ_INSERT_TAIL(&am->ops, op, pointers);
    }

    /* If async operations are pending, persist them and signal the service is
     * not done */
    if(ar->opCountdown > 0) {
        ar->responseType = &UA_TYPES[UA_TYPES_WRITERESPONSE];
        persistAsyncResponse(server, session, response, ar);
    }
    return (ar->opCountdown == 0);
}

#ifdef UA_ENABLE_METHODCALLS
UA_Boolean
Service_Call(UA_Server *server, UA_Session *session,
             const UA_CallRequest *request, UA_CallResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session, "Processing CallRequest");
    UA_LOCK_ASSERT(&server->serviceMutex);

    if(server->config.maxNodesPerMethodCall != 0 &&
        request->methodsToCallSize > server->config.maxNodesPerMethodCall) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return true;
    }

    if(request->methodsToCallSize == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return true;
    }

    /* Allocate the response array. The AsyncResponse and AsyncOperations are
     * added to the back. So they get cleaned up automatically if none of the
     * calls are async. */
    size_t opsLen = sizeof(UA_CallMethodResult) * request->methodsToCallSize;
    size_t len = opsLen + sizeof(UA_AsyncResponse) +
        (sizeof(UA_AsyncOperation) * request->methodsToCallSize);
    response->results = (UA_CallMethodResult*)UA_calloc(1, len);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return true;
    }
    response->resultsSize = request->methodsToCallSize;

    UA_AsyncResponse *ar = (UA_AsyncResponse*)&response->results[response->resultsSize];
    UA_AsyncOperation *aopArray = (UA_AsyncOperation*)&ar[1];

    /* Execute the operations */
    UA_AsyncManager *am = &server->asyncManager;
    for(size_t i = 0; i < request->methodsToCallSize; i++) {
        UA_Boolean done = Operation_CallMethod(server, session, &request->methodsToCall[i],
                                               &response->results[i]);
        if(done)
            continue;

        /* Set up the async operation */
        UA_AsyncOperation *op = &aopArray[i];
        op->asyncOperationType = UA_ASYNCOPERATIONTYPE_CALL_REQUEST;
        op->handling.response = ar;
        op->output.call = &response->results[i];

        /* Not enough resources to store the async operation */
        if(server->config.maxAsyncOperationQueueSize != 0 &&
           am->opsCount >= server->config.maxAsyncOperationQueueSize) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Cannot create an async call operation: Queue exceeds limit (%d).",
                           (int unsigned)server->config.maxAsyncOperationQueueSize);
            UA_AsyncOperation_cancel(server, op, UA_STATUSCODE_BADTOOMANYOPERATIONS);
            continue;
        }

        /* Enqueue the asyncop in the async manager */
        ar->opCountdown++;
        am->opsCount++;
        TAILQ_INSERT_TAIL(&am->ops, op, pointers);
    }

    /* If async operations are pending, persist them and signal the service is
     * not done */
    if(ar->opCountdown > 0) {
        ar->responseType = &UA_TYPES[UA_TYPES_CALLRESPONSE];
        persistAsyncResponse(server, session, response, ar);
    }
    return (ar->opCountdown == 0);
}
#endif

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

    uintptr_t reqOp = (uintptr_t)reqArray;
    uintptr_t respOp = (uintptr_t)respArray;
    uintptr_t aop = respOp + opsLen + sizeof(UA_AsyncResponse);
    UA_AsyncResponse *ar = (UA_AsyncResponse*)(respOp + opsLen);
    ar->responseType = asd->responseType;

    /* Execute the operations */
    for(size_t i = 0; i < reqSize; i++) {
        void *reqOpPtr = (void*)reqOp;
        void *respOpPtr = (void*)respOp;

        UA_AsyncOperation *op = (UA_AsyncOperation*)aop;
        if(asd->responseType == &UA_TYPES[UA_TYPES_CALLRESPONSE]) {
            op->asyncOperationType = UA_ASYNCOPERATIONTYPE_CALL_REQUEST;
        } else if(asd->responseType == &UA_TYPES[UA_TYPES_READRESPONSE]) {
            op->asyncOperationType = UA_ASYNCOPERATIONTYPE_READ_REQUEST;
        } else if(asd->responseType == &UA_TYPES[UA_TYPES_WRITERESPONSE]) {
            op->asyncOperationType = UA_ASYNCOPERATIONTYPE_WRITE_REQUEST;
            /* Ensure a stable pointer for the writevalue. Doesn't get written to,
             * just used for the lookup */
            op->context.writeValue = *(UA_WriteValue*)reqOp;
            reqOpPtr = &op->context.writeValue;
        }

        /* Call the async operation */
        UA_Boolean done = asd->operationCallback(server, session, reqOpPtr, respOpPtr);
        if(!done) {
            /* Set up the async operation */
            op->handling.response = ar;
            op->output.read = (UA_DataValue*)respOpPtr; /* Applies for all response types */

            if(server->config.maxAsyncOperationQueueSize != 0 &&
               am->opsCount >= server->config.maxAsyncOperationQueueSize) {
                /* Not enough resources to store the async operation */
                UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                               "Cannot create an async operation: Queue exceeds limit (%d).",
                               (int unsigned)server->config.maxAsyncOperationQueueSize);
                UA_AsyncOperation_cancel(server, op, UA_STATUSCODE_BADTOOMANYOPERATIONS);
            } else {
                /* Enqueue the asyncop in the async manager */
                ar->opCountdown++;
                am->opsCount++;
                TAILQ_INSERT_TAIL(&am->ops, op, pointers);
            }
        }

        /* Forward the pointer to the next operation */
        reqOp += asd->requestOperationsType->memSize;
        respOp += asd->responseOperationsType->memSize;
        aop += sizeof(UA_AsyncOperation);
    }

    /* No async operations, this is done */
    if(ar->opCountdown == 0)
        return UA_STATUSCODE_GOOD;

    /* Persist the response as an async response and signal in the status that
     * async operations are pending */
    persistAsyncResponse(server, session, response, ar);
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

UA_UInt32
UA_AsyncManager_cancel(UA_Server *server, UA_Session *session, UA_UInt32 requestHandle) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Loop over all queued operations */
    UA_UInt32 count = 0;
    UA_AsyncOperation *op, *op_tmp;
    UA_AsyncManager *am = &server->asyncManager;
    TAILQ_FOREACH_SAFE(op, &am->ops, pointers, op_tmp) {
        UA_AsyncResponse *ar = op->handling.response;
        if(ar->requestHandle != requestHandle ||
           !UA_NodeId_equal(&session->sessionId, &ar->sessionId))
            continue;

        count++; /* Found a matching request */

        /* Set the status of the overall response */
        ar->response.callResponse.responseHeader.serviceResult =
            UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT;

        /* Notify, set operation status and integrate */
        UA_AsyncOperation_cancel(server, op, UA_STATUSCODE_BADOPERATIONABANDONED);
        integrateResult(server, am, op);
    }

    return count;
}

UA_StatusCode
UA_Server_setAsyncReadResult(UA_Server *server, UA_DataValue *result) {
    lockServer(server);
    UA_AsyncManager *am = &server->asyncManager;
    UA_AsyncOperation *op = NULL;
    TAILQ_FOREACH(op, &am->ops, pointers) {
        if(op->output.read == result) {
            integrateResult(server, am, op);
            break;
        }
    }
    unlockServer(server);
    return (op) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_Server_setAsyncWriteResult(UA_Server *server,
                              const UA_DataValue *value,
                              UA_StatusCode result) {
    lockServer(server);
    UA_AsyncManager *am = &server->asyncManager;
    UA_AsyncOperation *op = NULL;
    TAILQ_FOREACH(op, &am->ops, pointers) {
        if(&op->context.writeValue.value == value) {
            *op->output.write = result;
            integrateResult(server, am, op);
            break;
        }
    }
    unlockServer(server);
    return (op) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_Server_setAsyncCallMethodResult(UA_Server *server,
                                   UA_Variant *output,
                                   UA_StatusCode result) {
    lockServer(server);
    UA_AsyncManager *am = &server->asyncManager;
    UA_AsyncOperation *op = NULL;
    TAILQ_FOREACH(op, &am->ops, pointers) {
        if(op->output.call->outputArguments == output) {
            op->output.call->statusCode = result;
            integrateResult(server, am, op);
            break;
        }
    }
    unlockServer(server);
    return (op) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOTFOUND;
}
