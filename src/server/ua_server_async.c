/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 *    Copyright 2019, 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_server_async.h"
#include "open62541/types.h"
#include "ua_server_internal.h"

/* Service and local operations share a kind and the same result layout. */
static const struct {
    UA_UInt16 requestType, resultType, responseType, serviceType;
} operationTypes[] = {
    {UA_TYPES_CALLMETHODREQUEST, UA_TYPES_CALLMETHODRESULT, UA_TYPES_CALLRESPONSE, UA_TYPES_CALLREQUEST},
    {UA_TYPES_READVALUEID, UA_TYPES_DATAVALUE, UA_TYPES_READRESPONSE, UA_TYPES_READREQUEST},
    {UA_TYPES_WRITEVALUE, UA_TYPES_STATUSCODE, UA_TYPES_WRITERESPONSE, UA_TYPES_WRITEREQUEST}
};

static UA_AsyncOperationType
operationKind(UA_AsyncOperationType type) {
    return (UA_AsyncOperationType)(type & 3);
}

static UA_Boolean
isRequestOp(const UA_AsyncOperation *op) {
    return op->asyncOperationType < UA_ASYNCOPERATIONTYPE_CALL_DIRECT;
}

static const UA_DataType *
resultType(const UA_AsyncOperation *op) {
    return &UA_TYPES[operationTypes[operationKind(op->asyncOperationType)].resultType];
}

static const UA_DataType *
responseType(const UA_AsyncResponse *ar) {
    return &UA_TYPES[operationTypes[ar->kind].responseType];
}

static UA_Boolean
acceptsAsyncRequests(const UA_Server *server) {
    return server->state == UA_LIFECYCLESTATE_STARTED &&
        server->asyncManager.driver.state == UA_LIFECYCLESTATE_STARTED;
}

static UA_StatusCode
asyncAdmissionStatus(const UA_Server *server) {
    if(!acceptsAsyncRequests(server))
        return UA_STATUSCODE_BADSHUTDOWN;
    size_t limit = server->config.maxAsyncOperationQueueSize;
    if(limit != 0 && server->asyncManager.trackedOpsCount >= limit)
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;
    return UA_STATUSCODE_GOOD;
}

static void
asyncManagerCheckStopped(UA_AsyncManager *am) {
    if(am->driver.state != UA_LIFECYCLESTATE_STOPPING ||
       am->trackedOpsCount != 0 || am->activeDispatch != 0 ||
       !TAILQ_EMPTY(&am->responses))
        return;
    UA_EventLoop *el = am->driver.server->config.eventLoop;
    am->driver.state = UA_LIFECYCLESTATE_STOPPED;
    el->cancel(el);
}

static void
finishDispatch(UA_AsyncManager *am) {
    UA_assert(am->activeDispatch > 0);
    am->activeDispatch--;
    asyncManagerCheckStopped(am);
}

static void
moveResult(void *src, void *dst, const UA_DataType *type) {
    memcpy(dst, src, type->memSize);
    memset(src, 0, type->memSize);
}

static void *
responseResult(UA_AsyncResponse *ar, size_t index) {
    if(index == SIZE_MAX)
        return NULL;
    UA_assert(ar && index < ar->response.readResponse.resultsSize);
    switch(ar->kind) {
    case UA_ASYNCOPERATIONTYPE_CALL_REQUEST:
        return &ar->response.callResponse.results[index];
    case UA_ASYNCOPERATIONTYPE_READ_REQUEST:
        return &ar->response.readResponse.results[index];
    case UA_ASYNCOPERATIONTYPE_WRITE_REQUEST:
        return &ar->response.writeResponse.results[index];
    default: UA_assert(false); return NULL;
    }
}

static void
setResultStatus(void *result, const UA_DataType *type, UA_StatusCode status) {
    if(type == &UA_TYPES[UA_TYPES_DATAVALUE]) {
        UA_DataValue *dv = (UA_DataValue*)result;
        dv->hasStatus = true;
        dv->status = status;
    } else if(type == &UA_TYPES[UA_TYPES_CALLMETHODRESULT]) {
        ((UA_CallMethodResult*)result)->statusCode = status;
    } else {
        UA_assert(type == &UA_TYPES[UA_TYPES_STATUSCODE]);
        *(UA_StatusCode*)result = status;
    }
}

/* Return the stable identifier without reading any application-owned contents. */
static const void *
operationId(const UA_AsyncOperation *op) {
    switch(operationKind(op->asyncOperationType)) {
    case UA_ASYNCOPERATIONTYPE_READ_REQUEST:
        return &op->output.read;
    case UA_ASYNCOPERATIONTYPE_WRITE_REQUEST:
        return &op->context.writeValue.value;
    case UA_ASYNCOPERATIONTYPE_CALL_REQUEST:
        return op->output.call.outputArguments;
    default: UA_assert(false); return NULL;
    }
}

/* Compare opaque identifiers numerically, without dereferencing application
 * memory or requiring an allocation for a lookup key. */
static enum ZIP_CMP
compareOperationId(const uintptr_t *a, const uintptr_t *b) {
    return (*a < *b) ? ZIP_CMP_LESS : ((*a > *b) ? ZIP_CMP_MORE : ZIP_CMP_EQ);
}

ZIP_FUNCTIONS(UA_AsyncOperationTree, UA_AsyncOperation, index, uintptr_t, id,
              compareOperationId)

/* Called once by delivery. Synchronous facades may notify inline, but must not
 * access op afterwards: the application may return ownership in the callback. */
static void
notifyCanceledOperation(UA_Server *server, UA_AsyncOperation *op) {
    if(op->id && op->resultIndex == SIZE_MAX &&
       server->config.asyncOperationCancelCallback)
        server->config.asyncOperationCancelCallback(server, (const void*)op->id);
}

static void
releaseOperation(UA_AsyncManager *am, UA_AsyncOperation *op, void *destination) {
    UA_assert(op->id == 0);
    if(destination)
        memcpy(destination, &op->output, resultType(op)->memSize);
    else
        UA_clear(&op->output, resultType(op));
    /* Retain a small warm pool, not the peak concurrent allocation. */
    if(am->freeOpsSize >= 16) {
        UA_free(op);
        return;
    }
    memset(op, 0, sizeof(*op));
    op->index.left = am->freeOps;
    am->freeOps = op;
    am->freeOpsSize++;
}

static void
releaseResponse(UA_AsyncManager *am, UA_AsyncResponse *ar) {
    UA_assert(ar->response.readResponse.resultsSize == 0 && ar->pendingResults == 0);
    UA_assert(LIST_EMPTY(&ar->operations));
    TAILQ_REMOVE(&am->responses, ar, pointers);
    if(am->freeResponsesSize >= 16) {
        UA_free(ar);
        return;
    }
    memset(ar, 0, sizeof(*ar));
    ar->pointers.tqe_next = am->freeResponses;
    am->freeResponses = ar;
    am->freeResponsesSize++;
}

static UA_AsyncResponse *
acquireResponse(UA_AsyncManager *am) {
    UA_AsyncResponse *ar = am->freeResponses;
    if(ar) {
        am->freeResponses = TAILQ_NEXT(ar, pointers);
        am->freeResponsesSize--;
    } else
        ar = (UA_AsyncResponse*)UA_calloc(1, sizeof(*ar));
    if(ar)
        TAILQ_INSERT_TAIL(&am->responses, ar, pointers);
    return ar;
}

static void
sendAsyncResponse(UA_Server *server, UA_AsyncResponse *ar) {
    UA_assert(ar->pendingResults == 0);
    UA_Session *session = ar->session;
    UA_assert(session);
    UA_SecureChannel *channel = session->channel;

    /* Notify that processing the service has ended */
    notifyService(server, UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_END,
                  channel ? channel->securityToken.channelId : 0, session->sessionId,
                  ar->uacpRequestId, UA_TYPES[operationTypes[ar->kind].serviceType].typeId);

    /* Session cleanup follows response delivery, but the Session may already
     * be logically closed. Do not send a response in that case. */
    if(session->state == UA_SESSIONSTATE_CLOSED)
        return;

    if(ar->abandoned) {
        UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Async response for closed transport carrier token %"
                     PRIu64 " was abandoned", ar->responseToken);
        return;
    }

    /* Notifications can change the Session's channel or advance its expiry. */
    if(session->channel != channel)
        return;

    UA_EventLoop *el = server->config.eventLoop;
    if(session != &server->adminSession &&
       el->dateTime_nowMonotonic(el) > session->validTill) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "Async Service: Session has timed out");
        return;
    }

    /* Check the channel */
    if(!channel) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "Async Service Response cannot be sent. "
                               "No SecureChannel for the session.");
        return;
    }

    /* Send the Response */
    UA_StatusCode res = sendResponse(server, channel, ar->responseToken,
                                     (UA_Response*)&ar->response, responseType(ar));
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "Async response for token %" PRIu64 " failed "
                               "with StatusCode %s", ar->responseToken,
                               UA_StatusCode_name(res));
    }
}

static void
directOpCallback(UA_Server *server, UA_AsyncOperation *op) {
    UA_AsyncOperationOutput canceled;
    const UA_AsyncOperationOutput *output = &op->output;
    if(op->resultIndex == SIZE_MAX) {
        memset(&canceled, 0, sizeof(canceled));
        setResultStatus(&canceled, resultType(op), op->handling.callback.cancellationStatus);
        output = &canceled;
    }
    void *context = op->handling.callback.context;
    switch(operationKind(op->asyncOperationType)) {
    case UA_ASYNCOPERATIONTYPE_READ_REQUEST:
        op->handling.callback.method.read(server, context, &output->read); break;
    case UA_ASYNCOPERATIONTYPE_WRITE_REQUEST:
        op->handling.callback.method.write(server, context, output->write); break;
    case UA_ASYNCOPERATIONTYPE_CALL_REQUEST:
        op->handling.callback.method.call(server, context, &output->call); break;
    default: UA_assert(false); break;
    }
}

/* A canceled operation stays indexed until ownership returns, even during its
 * result callback. The queued/executing callback keeps the storage alive. */
static void
finishDirectOp(void *application, void *context) {
    UA_Server *server = (UA_Server*)application;
    UA_AsyncOperation *op = (UA_AsyncOperation*)context;
    lockServer(server);
    UA_AsyncManager *am = &server->asyncManager;
    am->activeDispatch++;
    /* Leave the delayed callback installed as a lifetime pin until both
     * application callbacks have returned. */
    directOpCallback(server, op);
    notifyCanceledOperation(server, op);
    op->handling.callback.dc.callback = NULL;
    if(!op->id) {
        am->trackedOpsCount--;
        releaseOperation(am, op, NULL);
    }
    finishDispatch(am);
    unlockServer(server);
}

static void
scheduleCallback(UA_Server *server, UA_DelayedCallback *dc,
                  UA_Callback callback, void *context) {
    UA_assert(!dc->callback || dc->callback == callback);
    dc->callback = callback;
    dc->application = server;
    dc->context = context;
    UA_EventLoop *el = server->config.eventLoop;
    el->addDelayedCallback(el, dc);
    el->cancel(el);
}

static void
finishResponse(UA_Server *server, UA_AsyncResponse *ar) {
    if(ar->response.readResponse.resultsSize > 0) {
        sendAsyncResponse(server, ar);
        UA_clear(&ar->response, responseType(ar));
        ar->session = NULL;
    }
    /* Pop before notifying: callbacks may complete and unlink any other
     * operation. Re-fetching the response's head is safe and constant-time. */
    UA_AsyncOperation *op;
    while((op = LIST_FIRST(&ar->operations))) {
        UA_assert(op->resultIndex == SIZE_MAX);
        LIST_REMOVE(op, handling.service.pointers);
        op->handling.service.response = NULL;
        notifyCanceledOperation(server, op);
    }
}

/* Called from the EventLoop via a delayed callback */
static void
processResponse(void *application, void *context) {
    UA_Server *server = (UA_Server*)application;
    UA_AsyncResponse *ar = (UA_AsyncResponse*)context;
    lockServer(server);
    UA_AsyncManager *am = &server->asyncManager;
    am->activeDispatch++;
    finishResponse(server, ar);
    releaseResponse(am, ar);
    finishDispatch(am);
    unlockServer(server);
}

static void
scheduleResponse(UA_Server *server, UA_AsyncResponse *ar) {
    UA_assert(!ar->dc.callback);
    scheduleCallback(server, &ar->dc, processResponse, ar);
}

static void
completeResponseOperation(UA_Server *server, UA_AsyncResponse *ar) {
    UA_assert(ar && ar->pendingResults > 0);
    /* Completion during initiation is handled by the service return. */
    if(--ar->pendingResults == 0 && ar->session)
        scheduleResponse(server, ar);
}

/* All three setters use the same ownership transition. Canceled operations
 * remain discoverable until the application returns ownership. */
static UA_StatusCode
setAsyncResult(UA_Server *server, const void *id,
                UA_AsyncOperationType kind, UA_StatusCode status) {
    lockServer(server);
    UA_AsyncManager *am = &server->asyncManager;
    uintptr_t key = (uintptr_t)id;
    UA_AsyncOperation *op = ZIP_FIND(UA_AsyncOperationTree, &am->operations, &key);
    if(!op || operationKind(op->asyncOperationType) != kind) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    am->activeDispatch++;
    /* Take ownership, then publish or discard the operation's result. */
    ZIP_REMOVE(UA_AsyncOperationTree, &am->operations, op);
    op->id = 0;
    if(op->resultIndex != SIZE_MAX) {
        if(kind == UA_ASYNCOPERATIONTYPE_READ_REQUEST)
            Operation_Read_complete(server, &op->output.read, op->context.read.timestamps,
                                    UA_ATTRIBUTEID_VALUE, op->context.read.nonNullable);
        else
            setResultStatus(&op->output, resultType(op), status);
    }
    if(isRequestOp(op)) {
        UA_AsyncResponse *ar = op->handling.service.response;
        if(ar)
            LIST_REMOVE(op, handling.service.pointers);
        if(op->resultIndex != SIZE_MAX)
            completeResponseOperation(server, ar);
        am->trackedOpsCount--;
        releaseOperation(am, op, responseResult(ar, op->resultIndex));
    } else if(op->resultIndex != SIZE_MAX) {
        scheduleCallback(server, &op->handling.callback.dc, finishDirectOp, op);
    } else if(!op->handling.callback.dc.callback) {
        am->trackedOpsCount--;
        releaseOperation(am, op, NULL);
    }
    finishDispatch(am);
    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

/* Invalidate the result index and arrange requester delivery, without invoking
 * application code. Delivery owns the subsequent cancellation notification. */
static void
cancelOperation(UA_Server *server, UA_AsyncOperation *op, UA_StatusCode status) {
    UA_assert(status != UA_STATUSCODE_GOOD);
    if(op->resultIndex == SIZE_MAX)
        return;
    if(isRequestOp(op)) {
        UA_AsyncResponse *ar = op->handling.service.response;
        setResultStatus(responseResult(ar, op->resultIndex),
                        resultType(op), status);
        completeResponseOperation(server, ar);
    } else {
        op->handling.callback.cancellationStatus = status;
        scheduleCallback(server, &op->handling.callback.dc, finishDirectOp, op);
    }
    op->resultIndex = SIZE_MAX;
}

static void
cancelResponseOperations(UA_Server *server, UA_AsyncResponse *ar,
                         UA_StatusCode status) {
    /* Only indexed operations have returned from their initiating callback. */
    UA_AsyncOperation *op;
    LIST_FOREACH(op, &ar->operations, handling.service.pointers)
        cancelOperation(server, op, status);
    UA_assert(ar->pendingResults == 0);
}

typedef struct {
    UA_Server *server;
    UA_DateTime now;
    UA_StatusCode status;
} AsyncCancelContext;

/* Cancellation does not invoke application code or mutate the index. */
static void *
cancelIndexedOperation(void *context, UA_AsyncOperation *op) {
    AsyncCancelContext *cc = (AsyncCancelContext*)context;
    if(op->resultIndex == SIZE_MAX)
        return NULL;
    /* Service operations share a deadline; local operations have their own. */
    UA_AsyncResponse *ar = isRequestOp(op) ? op->handling.service.response : NULL;
    if(cc->status == UA_STATUSCODE_BADTIMEOUT) {
        if(ar && !ar->session) /* Still dispatching */
            return NULL;
        if(cc->now <= (ar ? ar->timeout : op->handling.callback.timeout))
            return NULL;
    }
    cancelOperation(cc->server, op, cc->status);
    if(cc->status == UA_STATUSCODE_BADTIMEOUT && (!ar || ar->pendingResults == 0))
        UA_LOG_WARNING(cc->server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Async operation timed out");
    return NULL;
}

/* Check if any operations have timed out. */
static void
checkTimeouts(UA_Server *server, void *context) {
    lockServer(server);
    UA_AsyncManager *am = (UA_AsyncManager*)context;
    am->activeDispatch++;
    UA_EventLoop *el = server->config.eventLoop;
    AsyncCancelContext cc = {server, el->dateTime_nowMonotonic(el), UA_STATUSCODE_BADTIMEOUT};
    ZIP_ITER(UA_AsyncOperationTree, &am->operations, cancelIndexedOperation, &cc);
    finishDispatch(am);
    unlockServer(server);
}

static UA_StatusCode
UA_AsyncManager_start(UA_Driver *driver) {
    UA_AsyncManager *am = (UA_AsyncManager*)driver;
    UA_Server *server = driver->server;
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(driver->state == UA_LIFECYCLESTATE_STARTED)
        return UA_STATUSCODE_GOOD;
    /* Startup may reopen admission with canceled pre-start trampolines still
     * outstanding. Restarting a driver during shutdown is not allowed. */
    if(server->state == UA_LIFECYCLESTATE_STOPPING ||
       (driver->state == UA_LIFECYCLESTATE_STOPPING &&
        server->state != UA_LIFECYCLESTATE_STOPPED) || am->activeDispatch != 0)
        return UA_STATUSCODE_BADINVALIDSTATE;
    /* Check service and local operation deadlines once per second. */
    UA_StatusCode res = addRepeatedCallback(server, (UA_ServerCallback)checkTimeouts,
                    am, 1000.0, &am->checkTimeoutCallbackId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "Failed to register async timeout callback. StatusCode: %s",
                    UA_StatusCode_name(res));
        am->checkTimeoutCallbackId = 0;
    } else
        driver->state = UA_LIFECYCLESTATE_STARTED;
    return res;
}

static void
UA_AsyncManager_stop(UA_Driver *driver) {
    UA_AsyncManager *am = (UA_AsyncManager*)driver;
    UA_Server *server = driver->server;
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(driver->state != UA_LIFECYCLESTATE_STARTED) {
        asyncManagerCheckStopped(am);
        return;
    }
    driver->state = UA_LIFECYCLESTATE_STOPPING;
    am->activeDispatch++;
    removeCallback(server, am->checkTimeoutCallbackId);
    am->checkTimeoutCallbackId = 0;

    /* Cancellation only marks records; delivery and notifications run later. */
    AsyncCancelContext cc = {server, 0, UA_STATUSCODE_BADSHUTDOWN};
    ZIP_ITER(UA_AsyncOperationTree, &am->operations, cancelIndexedOperation, &cc);
    finishDispatch(am);
}

static UA_StatusCode
freeAsyncManager(UA_Driver *driver) {
    UA_AsyncManager *am = (UA_AsyncManager*)driver;
    if(driver->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINVALIDSTATE;
    UA_assert(!ZIP_ROOT(&am->operations) && TAILQ_EMPTY(&am->responses));
    UA_assert(am->checkTimeoutCallbackId == 0);
    UA_AsyncOperation *op;
    while((op = am->freeOps)) {
        am->freeOps = op->index.left;
        UA_free(op);
    }
    am->freeOpsSize = 0;
    UA_AsyncResponse *ar;
    while((ar = am->freeResponses)) {
        am->freeResponses = TAILQ_NEXT(ar, pointers);
        UA_free(ar);
    }
    am->freeResponsesSize = 0;
    UA_KeyValueMap_clear(&driver->params);
    driver->server = NULL;
    /* The manager itself is embedded in UA_Server. */
    return UA_STATUSCODE_GOOD;
}

void
UA_AsyncManager_init(UA_AsyncManager *am, UA_Server *server) {
    memset(am, 0, sizeof(*am));
    am->driver.server = server;
    am->driver.name = UA_STRING("async");
    am->driver.start = UA_AsyncManager_start;
    am->driver.stop = UA_AsyncManager_stop;
    am->driver.free = freeAsyncManager;
    TAILQ_INIT(&am->responses);
    ZIP_INIT(&am->operations);
}

/* Services, local APIs and synchronous facades use stable, reusable storage. Queue
 * admission belongs to the async API, not allocation: synchronous operations
 * must keep working while canceled operations occupy the async queue. */
static UA_StatusCode
acquireOperation(UA_Server *server, UA_AsyncOperationType type,
                  UA_AsyncOperation **out) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_AsyncManager *am = &server->asyncManager;
    if(!am->driver.server)
        return UA_STATUSCODE_BADSHUTDOWN;
    UA_AsyncOperation *op = am->freeOps;
    if(op) {
        am->freeOps = op->index.left;
        am->freeOpsSize--;
    } else
        op = (UA_AsyncOperation*)UA_calloc(1, sizeof(*op));
    if(!op)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    /* Even a synchronous facade may leave a canceled operation outstanding. */
    if(am->driver.state == UA_LIFECYCLESTATE_STOPPED)
        am->driver.state = UA_LIFECYCLESTATE_STOPPING;
    op->asyncOperationType = type;
    *out = op;
    return UA_STATUSCODE_GOOD;
}

/* Register only after initiation returns. Admission is rechecked by async
 * callers; synchronous facades always reject deferred completion. */
static UA_StatusCode
persistAsyncOperation(UA_Server *server, UA_AsyncOperation *op, UA_StatusCode status) {
    UA_AsyncManager *am = &server->asyncManager;
    am->trackedOpsCount++;
    op->id = (uintptr_t)operationId(op);
    UA_assert(op->id != 0);
    ZIP_INSERT(UA_AsyncOperationTree, &am->operations, op);
    if(isRequestOp(op)) {
        UA_AsyncResponse *ar = op->handling.service.response;
        LIST_INSERT_HEAD(&ar->operations, op, handling.service.pointers);
        ar->pendingResults++;
        if(status != UA_STATUSCODE_GOOD)
            cancelOperation(server, op, status);
    } else if(status != UA_STATUSCODE_GOOD) {
        /* Rejected local calls have no result callback. Notification may
         * immediately return ownership and recycle op. */
        op->resultIndex = SIZE_MAX;
        notifyCanceledOperation(server, op);
    }
    return status;
}

static UA_AsyncOperation *
beginNoAsync(UA_Server *server, UA_AsyncOperationType type, UA_StatusCode *status) {
    UA_AsyncOperation *op = NULL;
    *status = acquireOperation(server, type, &op);
    if(op)
        server->asyncManager.activeDispatch++;
    return op;
}

static void
finishNoAsync(UA_Server *server, UA_AsyncOperation *op,
              UA_Boolean done, void *result) {
    if(done) {
        releaseOperation(&server->asyncManager, op, result);
    } else {
        setResultStatus(result, resultType(op), UA_STATUSCODE_BADWAITINGFORRESPONSE);
        persistAsyncOperation(server, op, UA_STATUSCODE_BADWAITINGFORRESPONSE);
    }
    finishDispatch(&server->asyncManager);
}

/* node may be NULL to resolve the NodeId through the nodestore. */
void
readNoAsync(UA_Server *server, UA_Session *session, const UA_Node *node,
            UA_TimestampsToReturn ttr, const UA_ReadValueId *rvi,
            UA_DataValue *result) {
    UA_AsyncOperation *op = beginNoAsync(server, UA_ASYNCOPERATIONTYPE_READ_DIRECT,
                                        &result->status);
    if(!op) {
        result->hasStatus = true;
        return;
    }
    UA_Boolean done = node ?
        Operation_ReadWithNode(server, session, node, ttr, rvi, &op->output.read, NULL) :
        Operation_Read(server, session, ttr, rvi, &op->output.read, NULL);
    finishNoAsync(server, op, done, result);
}

/* Internal value reads intentionally bypass attribute access checks. */
UA_StatusCode
readValueAttribute(UA_Server *server, UA_Session *session,
                   const UA_VariableNode *node, UA_DataValue *result) {
    UA_StatusCode res;
    UA_AsyncOperation *op = beginNoAsync(server, UA_ASYNCOPERATIONTYPE_READ_DIRECT, &res);
    if(!op)
        return res;
    res = readValueAttributeRaw(server, session, node, &op->output.read);
    UA_Boolean done = (res != UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY);
    finishNoAsync(server, op, done, result);
    return done ? res : UA_STATUSCODE_BADWAITINGFORRESPONSE;
}

UA_StatusCode
writeNoAsync(UA_Server *server, UA_Session *session, const UA_WriteValue *value) {
    UA_StatusCode res;
    UA_AsyncOperation *op = beginNoAsync(server, UA_ASYNCOPERATIONTYPE_WRITE_DIRECT, &res);
    if(!op)
        return res;
    op->context.writeValue = *value;
    UA_Boolean done = Operation_Write(server, session, &op->context.writeValue, &op->output.write);
    finishNoAsync(server, op, done, &res);
    return res;
}

#ifdef UA_ENABLE_METHODCALLS
void
callNoAsync(UA_Server *server, UA_Session *session,
            const UA_CallMethodRequest *request, UA_CallMethodResult *result) {
    UA_AsyncOperation *op = beginNoAsync(server, UA_ASYNCOPERATIONTYPE_CALL_DIRECT,
                                        &result->statusCode);
    if(!op)
        return;
    UA_Boolean done = Operation_CallMethod(server, session, request, &op->output.call);
    finishNoAsync(server, op, done, result);
}
#endif

void
UA_AsyncManager_cancelSession(UA_Server *server, UA_Session *session) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /* Cancellation queues delivery without calling application code. Ready or
     * currently delivering responses already precede Session cleanup. */
    UA_AsyncResponse *ar;
    TAILQ_FOREACH(ar, &server->asyncManager.responses, pointers) {
        if(ar->session == session && ar->pendingResults > 0)
            cancelResponseOperations(server, ar, UA_STATUSCODE_BADSESSIONCLOSED);
    }
}

UA_UInt32
UA_AsyncManager_cancel(UA_Server *server, UA_Session *session, UA_UInt32 requestHandle) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Cancel matching requests, not individual operations. */
    UA_UInt32 count = 0;
    UA_AsyncManager *am = &server->asyncManager;
    UA_AsyncResponse *match;
    TAILQ_FOREACH(match, &am->responses, pointers) {
        if(match->pendingResults == 0 ||
           match->response.callResponse.responseHeader.requestHandle != requestHandle ||
           match->session != session)
            continue;

        count++;
        match->response.callResponse.responseHeader.serviceResult =
            UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT;
        cancelResponseOperations(server, match,
                                UA_STATUSCODE_BADOPERATIONABANDONED);
    }

    return count;
}

void
UA_AsyncManager_abandon(UA_Server *server, UA_SecureChannel *channel,
                        UA_UInt64 responseToken) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_AsyncManager *am = &server->asyncManager;
    UA_AsyncResponse *ar;
    TAILQ_FOREACH(ar, &am->responses, pointers) {
        if(!ar->session || ar->responseToken != responseToken)
            continue;
        if(ar->session->channel == channel)
            ar->abandoned = true;
    }
}

static UA_StatusCode
prepareDirectOperation(UA_Server *server, UA_AsyncOperationType type,
                        void *context, UA_UInt32 timeout, UA_AsyncOperation **out) {
    UA_StatusCode res = asyncAdmissionStatus(server);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_AsyncOperation *op = NULL;
    res = acquireOperation(server, type, &op);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    op->handling.callback.context = context;
    op->handling.callback.timeout = UA_INT64_MAX;
    if(timeout > 0) {
        UA_EventLoop *el = server->config.eventLoop;
        op->handling.callback.timeout = el->dateTime_nowMonotonic(el) +
            (UA_DateTime)timeout * UA_DATETIME_MSEC;
    }
    *out = op;
    server->asyncManager.activeDispatch++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
finishLocalOperation(UA_Server *server, UA_AsyncOperation *op, UA_Boolean done) {
    UA_AsyncManager *am = &server->asyncManager;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(done) {
        directOpCallback(server, op);
        releaseOperation(am, op, NULL);
    } else {
        res = persistAsyncOperation(server, op, asyncAdmissionStatus(server));
    }
    finishDispatch(am);
    return res;
}

static UA_StatusCode
checkOperationCount(size_t size, UA_UInt32 limit) {
    if(limit != 0 && size > limit)
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;
    return size == 0 ? UA_STATUSCODE_BADNOTHINGTODO : UA_STATUSCODE_GOOD;
}

/* The service owns validation and result allocation; this iterator only
 * dispatches operations and retains the response when completion is deferred. */
static UA_Boolean
serviceOperations(UA_Server *server, UA_Session *session, UA_AsyncOperationType kind,
                   size_t size, const void *requests,
                   UA_TimestampsToReturn ttr, UA_Response *response) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_ResponseHeader *header = &response->responseHeader;
    UA_AsyncManager *am = &server->asyncManager;
    UA_AsyncResponse *ar = acquireResponse(am);
    if(!ar) {
        header->serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return true;
    }
    const UA_DataType *resultsType = &UA_TYPES[operationTypes[kind].resultType];
    ar->kind = kind;
    /* Make result slots available before callbacks can complete earlier ops. */
    moveResult(response, &ar->response, responseType(ar));
    header = &ar->response.readResponse.responseHeader;
    const UA_DataType *requestType = &UA_TYPES[operationTypes[kind].requestType];
    am->activeDispatch++;
    for(size_t i = 0; i < size; i++) {
        void *destination = responseResult(ar, i);
        UA_AsyncOperation *op = NULL;
        UA_StatusCode res = acquireOperation(server, kind, &op);
        if(res != UA_STATUSCODE_GOOD) {
            setResultStatus(destination, resultsType, res);
            continue;
        }
        op->handling.service.response = ar;
        op->resultIndex = i;
        const void *request = (const UA_Byte*)requests + i * requestType->memSize;
        UA_Boolean done;
        switch(kind) {
        case UA_ASYNCOPERATIONTYPE_READ_REQUEST:
            op->context.read.timestamps = ttr;
            done = Operation_Read(server, session, ttr, (const UA_ReadValueId*)request,
                                  &op->output.read, &op->context.read.nonNullable);
            break;
        case UA_ASYNCOPERATIONTYPE_WRITE_REQUEST:
            op->context.writeValue = *(const UA_WriteValue*)request;
            done = Operation_Write(server, session, &op->context.writeValue, &op->output.write);
            break;
#ifdef UA_ENABLE_METHODCALLS
        case UA_ASYNCOPERATIONTYPE_CALL_REQUEST:
            done = Operation_CallMethod(server, session, (const UA_CallMethodRequest*)request,
                                        &op->output.call);
            break;
#endif
        default: UA_assert(false); done = true; break;
        }
        if(done) {
            releaseOperation(am, op, destination);
        } else
            persistAsyncOperation(server, op, asyncAdmissionStatus(server));
        if(session->state == UA_SESSIONSTATE_CLOSED)
            header->serviceResult = UA_STATUSCODE_BADSESSIONCLOSED;
        else if(!acceptsAsyncRequests(server))
            header->serviceResult = UA_STATUSCODE_BADSHUTDOWN;
        else
            continue;
        cancelResponseOperations(server, ar, header->serviceResult);
        break;
    }
    UA_Boolean done = (ar->pendingResults == 0);
    if(done) {
        moveResult(&ar->response, response, responseType(ar));
        if(LIST_EMPTY(&ar->operations)) {
            releaseResponse(am, ar);
        } else {
            /* Notify after the caller has sent the synchronous response. */
            scheduleResponse(server, ar);
        }
    } else {
        /* Retain delivery and the transport correlation supplied at dispatch. */
        ar->responseToken = am->currentResponseToken;
        ar->uacpRequestId = am->currentUacpRequestId;
        ar->session = session;
        ar->timeout = UA_INT64_MAX;
        UA_EventLoop *el = server->config.eventLoop;
        if(server->config.asyncOperationTimeout > 0.0)
            ar->timeout = el->dateTime_nowMonotonic(el) + (UA_DateTime)
                (server->config.asyncOperationTimeout * (UA_DateTime)UA_DATETIME_MSEC);
    }
    finishDispatch(am);
    return done;
}

/********/
/* Read */
/********/

UA_Boolean
Service_Read(UA_Server *server, UA_Session *session, const void *request_, void *response_) {
    const UA_ReadRequest *request = (const UA_ReadRequest*)request_;
    UA_ReadResponse *response = (UA_ReadResponse*)response_;
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_DEBUG_SESSION(server->config.logging, session, "Processing ReadRequest");
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;
    if(!acceptsAsyncRequests(server)) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSHUTDOWN;
        return true;
    }
    if(request->timestampsToReturn > UA_TIMESTAMPSTORETURN_NEITHER) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
        return true;
    }
    if(request->maxAge < 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADMAXAGEINVALID;
        return true;
    }
    response->responseHeader.serviceResult =
        checkOperationCount(request->nodesToReadSize, server->config.maxNodesPerRead);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return true;
    response->results = (UA_DataValue*)UA_Array_new(request->nodesToReadSize,
                                                   &UA_TYPES[UA_TYPES_DATAVALUE]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return true;
    }
    response->resultsSize = request->nodesToReadSize;
    return serviceOperations(server, session, UA_ASYNCOPERATIONTYPE_READ_REQUEST,
                             request->nodesToReadSize, request->nodesToRead,
                             request->timestampsToReturn, (UA_Response*)response);
}

UA_StatusCode
read_async(UA_Server *server, UA_Session *session, const UA_ReadValueId *operation,
           UA_TimestampsToReturn ttr, UA_ServerAsyncReadResultCallback callback,
           void *context, UA_UInt32 timeout) {
    UA_AsyncOperation *op = NULL;
    UA_StatusCode res = prepareDirectOperation(server, UA_ASYNCOPERATIONTYPE_READ_DIRECT,
                                               context, timeout, &op);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    op->handling.callback.method.read = callback;
    op->context.read.timestamps = ttr;
    UA_Boolean done = Operation_Read(server, session, ttr, operation, &op->output.read,
                                     &op->context.read.nonNullable);
    return finishLocalOperation(server, op, done);
}

UA_StatusCode
UA_Server_read_async(UA_Server *server, const UA_ReadValueId *operation,
                     UA_TimestampsToReturn ttr, UA_ServerAsyncReadResultCallback callback,
                     void *context, UA_UInt32 timeout) {
    lockServer(server);
    UA_StatusCode res = read_async(server, &server->adminSession, operation,
                                   ttr, callback, context, timeout);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_setAsyncReadResult(UA_Server *server, UA_DataValue *result) {
    return setAsyncResult(server, result, UA_ASYNCOPERATIONTYPE_READ_REQUEST, UA_STATUSCODE_GOOD);
}

/*********/
/* Write */
/*********/

UA_Boolean
Service_Write(UA_Server *server, UA_Session *session, const void *request_, void *response_) {
    const UA_WriteRequest *request = (const UA_WriteRequest*)request_;
    UA_WriteResponse *response = (UA_WriteResponse*)response_;
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_DEBUG_SESSION(server->config.logging, session, "Processing WriteRequest");
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;
    if(!acceptsAsyncRequests(server)) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSHUTDOWN;
        return true;
    }
    response->responseHeader.serviceResult =
        checkOperationCount(request->nodesToWriteSize, server->config.maxNodesPerWrite);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return true;
    response->results = (UA_StatusCode*)UA_Array_new(request->nodesToWriteSize,
                                                    &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return true;
    }
    response->resultsSize = request->nodesToWriteSize;
    return serviceOperations(server, session, UA_ASYNCOPERATIONTYPE_WRITE_REQUEST,
                             request->nodesToWriteSize, request->nodesToWrite,
                             UA_TIMESTAMPSTORETURN_NEITHER, (UA_Response*)response);
}

UA_StatusCode
UA_Server_write_async(UA_Server *server, const UA_WriteValue *operation,
                      UA_ServerAsyncWriteResultCallback callback,
                      void *context, UA_UInt32 timeout) {
    lockServer(server);
    UA_AsyncOperation *op = NULL;
    UA_StatusCode res = prepareDirectOperation(server, UA_ASYNCOPERATIONTYPE_WRITE_DIRECT,
                                               context, timeout, &op);
    if(res == UA_STATUSCODE_GOOD) {
        op->handling.callback.method.write = callback;
        op->context.writeValue = *operation;
        UA_Boolean done = Operation_Write(server, &server->adminSession,
                                          &op->context.writeValue, &op->output.write);
        res = finishLocalOperation(server, op, done);
    }
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_setAsyncWriteResult(UA_Server *server, const UA_DataValue *value,
                              UA_StatusCode result) {
    return setAsyncResult(server, value, UA_ASYNCOPERATIONTYPE_WRITE_REQUEST, result);
}

/********/
/* Call */
/********/

#ifdef UA_ENABLE_METHODCALLS
UA_Boolean
Service_Call(UA_Server *server, UA_Session *session, const void *request_, void *response_) {
    const UA_CallRequest *request = (const UA_CallRequest*)request_;
    UA_CallResponse *response = (UA_CallResponse*)response_;
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_DEBUG_SESSION(server->config.logging, session, "Processing CallRequest");
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;
    if(!acceptsAsyncRequests(server)) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSHUTDOWN;
        return true;
    }
    response->responseHeader.serviceResult =
        checkOperationCount(request->methodsToCallSize, server->config.maxNodesPerMethodCall);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return true;
    response->results = (UA_CallMethodResult*)UA_Array_new(request->methodsToCallSize,
                                                         &UA_TYPES[UA_TYPES_CALLMETHODRESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return true;
    }
    response->resultsSize = request->methodsToCallSize;
    return serviceOperations(server, session, UA_ASYNCOPERATIONTYPE_CALL_REQUEST,
                             request->methodsToCallSize, request->methodsToCall,
                             UA_TIMESTAMPSTORETURN_NEITHER, (UA_Response*)response);
}

UA_StatusCode
UA_Server_call_async(UA_Server *server, const UA_CallMethodRequest *operation,
                     UA_ServerAsyncMethodResultCallback callback,
                     void *context, UA_UInt32 timeout) {
    lockServer(server);
    UA_AsyncOperation *op = NULL;
    UA_StatusCode res = prepareDirectOperation(server, UA_ASYNCOPERATIONTYPE_CALL_DIRECT,
                                               context, timeout, &op);
    if(res == UA_STATUSCODE_GOOD) {
        op->handling.callback.method.call = callback;
        UA_Boolean done = Operation_CallMethod(server, &server->adminSession,
                                               operation, &op->output.call);
        res = finishLocalOperation(server, op, done);
    }
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_setAsyncCallMethodResult(UA_Server *server, UA_Variant *output,
                                   UA_StatusCode result) {
    return setAsyncResult(server, output, UA_ASYNCOPERATIONTYPE_CALL_REQUEST, result);
}
#endif
