/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2020 (c) SPE UGPA (Author: Andranik Simonian)
 */

#include "ua_server_async.h"
#include "ua_server_internal.h"
#include "open62541_queue.h"

#if UA_MULTITHREADING >= 100

/* TODO: Should the following defenitions be moved to ua_architecture.h? */
#ifndef _WIN32
#define UA_COND_TYPE(condName) pthread_cond_t condName
#define UA_COND_INIT(condName) do { pthread_cond_init(&condName, NULL); } while(0)
#define UA_COND_DESTROY(condName) do { pthread_cond_destroy(&condName); } while(0)
#define UA_COND_BROADCAST(condName) do { pthread_cond_broadcast(&condName); } while(0)
#define UA_COND_SIGNAL(condName) do { pthread_cond_signal(&condName); } while(0)
#define UA_COND_WAIT(condName, mutexName)         \
    do {                                          \
        UA_assert(--(mutexName##Counter) == 0);   \
        pthread_cond_wait(&condName, &mutexName); \
        UA_assert(++(mutexName##Counter) == 1);   \
    } while(0)
#else
/* Win32 impl is not tested
 * TODO: Test, fix, and delete this comment and the warning */
#warning "Condvars for _WIN32 have not been tested"
#define UA_COND_TYPE(condName) CONDITION_VARIABLE condName
#define UA_COND_INIT(condName) do { InitializeConditionVariable(&condName; } while(0)
#define UA_COND_DESTROY(condName) do { } while(0)
#define UA_COND_BROADCAST(condName) do { WakeAllConditionVariable(&condName); } while(0)
#define UA_COND_SIGNAL(condName) do { WakeConditionVariable(&condName); } while(0)
#define UA_COND_WAIT(condName, mutexName)                          \
    do {                                                           \
        UA_assert(--(mutexName##Counter) == 0);                    \
        SleepConditionVariableCS(&condName, &mutexName, INFINITE); \
        UA_assert(++(mutexName##Counter) == 1);                    \
    } while(0)
#endif

/******************/
/* Async Services */
/******************/

UA_Service_async
UA_getAsyncService(const UA_DataType *requestType) {
    if(requestType->typeId.identifierType != UA_NODEIDTYPE_NUMERIC ||
            requestType->typeId.namespaceIndex != 0)
        return NULL;

    switch(requestType->typeId.identifier.numeric) {
#ifdef UA_ENABLE_METHODCALLS
    case UA_NS0ID_CALLREQUEST:
        return Service_Call_async;
#endif
#ifdef UA_ENABLE_HISTORIZING
    case UA_NS0ID_HISTORYREADREQUEST:
        return Service_HistoryRead_async;
#endif
    
    default:
        return NULL;
    }
}

/*****************/
/* Async Manager */
/*****************/

struct UA_AsyncTask {
    TAILQ_ENTRY(UA_AsyncTask) entry;
    UA_NodeId sessionId;
    UA_UInt32 requestId;
    const UA_DataType *requestType;
    UA_RequestHeader *request;
    const UA_DataType *responseType;
    UA_ResponseHeader *response;
    _UA_Service service;
    bool hasTimedOut;
    UA_DateTime timeout;
};

typedef struct UA_AsyncTask UA_AsyncTask;
typedef TAILQ_HEAD(UA_AsyncTaskQueue, UA_AsyncTask) UA_AsyncTaskQueue;

static void
AsyncTask_delete(UA_AsyncTask *task);

struct UA_AsyncManager {
    UA_Server *server;
    UA_AsyncTaskQueue pendingTasks;
    UA_AsyncTaskQueue dispatchedTasks;
    UA_AsyncTaskQueue finishedTasks;
    UA_AsyncTaskQueue timedoutTasks;
    bool isStopping;
    UA_LOCK_TYPE(mutex)
    UA_COND_TYPE(cond);
    UA_UInt64 serviceCallbackId;
};

static void
AsyncManager_serviceCallback(UA_Server *server, void *);

UA_AsyncManager *
UA_AsyncManager_new(UA_Server *server) {
    UA_AsyncManager *am = (UA_AsyncManager*)UA_calloc(1, sizeof(UA_AsyncManager));
    am->server = server;
    TAILQ_INIT(&am->pendingTasks);
    TAILQ_INIT(&am->dispatchedTasks);
    TAILQ_INIT(&am->finishedTasks);
    TAILQ_INIT(&am->timedoutTasks);
    UA_LOCK_INIT(am->mutex);
    UA_COND_INIT(am->cond);
    /* Add a regular callback for cleanup and sending finished responses at a
     * 100s interval. */
    UA_Server_addRepeatedCallback(server, AsyncManager_serviceCallback,
                                  NULL, 100, &am->serviceCallbackId);
    return am;
}

void
UA_AsyncManager_stop(UA_AsyncManager *am) {
    UA_Server_removeRepeatedCallback(am->server, am->serviceCallbackId);
    UA_LOCK(am->mutex);
    am->isStopping = true;
    UA_UNLOCK(am->mutex);
    UA_COND_BROADCAST(am->cond);
}

static void
UA_AsyncTaskQueue_clear(UA_AsyncTaskQueue *queue) {
    UA_AsyncTask *task = NULL, *task_tmp = NULL;
    TAILQ_FOREACH_SAFE(task, queue, entry, task_tmp) {
        TAILQ_REMOVE(queue, task, entry);
        AsyncTask_delete(task);
    }
}

void
UA_AsyncManager_delete(UA_AsyncManager *am) {
    UA_LOCK_DESTROY(am->mutex);
    UA_COND_DESTROY(am->cond);
    UA_AsyncTaskQueue_clear(&am->pendingTasks);
    UA_AsyncTaskQueue_clear(&am->dispatchedTasks);
    UA_AsyncTaskQueue_clear(&am->finishedTasks);
    UA_AsyncTaskQueue_clear(&am->timedoutTasks);
    UA_free(am);
}

UA_StatusCode
UA_AsyncManager_addAsyncTask(UA_AsyncManager *am,
                             const UA_Session *session,
                             UA_UInt32 requestId,
                             const UA_DataType *requestType,
                             const UA_RequestHeader *request,
                             const UA_DataType *responseType,
                             UA_ResponseHeader *response,
                             _UA_Service service) {
    UA_AsyncTask *asyncTask = (UA_AsyncTask*)UA_calloc(1, sizeof(UA_AsyncTask));
    UA_NodeId_copy(&session->sessionId, &asyncTask->sessionId);
    asyncTask->requestId = requestId;
    asyncTask->service = service;
    if(am->server->config.asyncOperationTimeout > 0) {
        asyncTask->timeout = UA_DateTime_now() +
            (UA_DateTime)(am->server->config.asyncOperationTimeout * UA_DATETIME_MSEC);
    }
    /* Copying request because it is const */
    asyncTask->requestType = requestType;
    asyncTask->request = (UA_RequestHeader*)UA_new(requestType);
    UA_copy(request, asyncTask->request, requestType);
    /* We can just move response only serviceResult should be retained in original */
    asyncTask->responseType = responseType;
    asyncTask->response = (UA_ResponseHeader*)UA_new(responseType);
    memcpy(asyncTask->response, response, responseType->memSize);
    UA_init(response, responseType);
    response->serviceResult = asyncTask->response->serviceResult;

    UA_LOCK(am->mutex);
    TAILQ_INSERT_TAIL(&am->pendingTasks, asyncTask, entry);
    UA_UNLOCK(am->mutex);

    UA_COND_SIGNAL(am->cond);

    return UA_STATUSCODE_GOOD;
}

static void
AsyncManager_sendResponse(UA_AsyncManager *am, UA_AsyncTask *task) {
    UA_LOCK(am->server->serviceMutex);
    UA_Session* session = UA_Server_getSessionById(am->server, &task->sessionId);
    UA_UNLOCK(am->server->serviceMutex);
    if(!session) {
        UA_LOG_WARNING(&am->server->config.logger, UA_LOGCATEGORY_SERVER,
                       "(Async) Cannot respond to request: session is gone");
    }
    UA_SecureChannel* channel = session->header.channel;
    if(!channel) {
        UA_LOG_WARNING(&am->server->config.logger, UA_LOGCATEGORY_SERVER,
                       "(Async) Cannot respond to request: channel is gone");
    }
    UA_StatusCode retval =
        sendResponse(channel, task->requestId, task->request->requestHandle,
                     task->response, task->responseType);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&am->server->config.logger, UA_LOGCATEGORY_SERVER,
            "(Async) Cannot send response to req# %" PRIu32 " : %s",
            task->requestId,
            UA_StatusCode_name(retval));
    }
}

static void
AsyncTask_delete(UA_AsyncTask *task) {
    UA_NodeId_clear(&task->sessionId);
    UA_delete(task->request, task->requestType);
    UA_delete(task->response, task->responseType);
    UA_free(task);
}

static void
AsyncManager_Queue_controlTimeouts(UA_AsyncManager *am, UA_AsyncTaskQueue *queue,
                                   UA_DateTime now) {
    UA_AsyncTask *task = NULL, *task_tmp = NULL;
    TAILQ_FOREACH_SAFE(task, queue, entry, task_tmp) {
        if(task->timeout == 0 || now < task->timeout)
            /* The timeout has not passed. Also for all elements following in the queue. */
            break;
        UA_LOG_WARNING(&am->server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Operation was removed due to a timeout");
        task->hasTimedOut = true;
        /* Change serviceResult from UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY to
         * UA_STATUSCODE_GOOD before sending back to client */
        task->response->serviceResult = UA_STATUSCODE_GOOD;
        AsyncManager_sendResponse(am, task);
        TAILQ_REMOVE(queue, task, entry);
        TAILQ_INSERT_TAIL(&am->timedoutTasks, task, entry);
    }
}

static void
AsyncManager_Queue_sendOutAndDelete(UA_AsyncManager *am, UA_AsyncTaskQueue *queue) {
    UA_AsyncTask *task = NULL, *task_tmp = NULL;
    TAILQ_FOREACH_SAFE(task, queue, entry, task_tmp) {
        AsyncManager_sendResponse(am, task);
        TAILQ_REMOVE(queue, task, entry);
        AsyncTask_delete(task);
    }
}

/* Checks timeouts and sends timed out and finishsed task */
static void
AsyncManager_serviceCallback(UA_Server *server, void *_) {
    UA_AsyncManager *am = server->asyncManager;
    UA_DateTime now = UA_DateTime_now();
    UA_LOCK(am->mutex);
    AsyncManager_Queue_controlTimeouts(am, &am->pendingTasks, now);
    AsyncManager_Queue_controlTimeouts(am, &am->dispatchedTasks, now);
    AsyncManager_Queue_sendOutAndDelete(am, &am->finishedTasks);
    UA_UNLOCK(am->mutex);
}

/* Moves a task to the dispatched queue and returnes it
 * Always returns NULL if AsyncManager is shutting down */
static UA_AsyncTask *
AsyncManager_getAsyncRequest(UA_AsyncManager *am) {
    UA_LOCK(am->mutex)
    if(am->isStopping)
        return NULL;
    UA_AsyncTask *task = NULL;
    if(!TAILQ_EMPTY(&am->pendingTasks)) {
        task = TAILQ_FIRST(&am->pendingTasks);
        TAILQ_REMOVE(&am->pendingTasks, task, entry);
        TAILQ_INSERT_TAIL(&am->dispatchedTasks, task, entry);
    }
    UA_UNLOCK(am->mutex)
    return task;
}

/* Marks a task as finished and deletes it if it has timed out */
static void
AsyncManager_finalizeTask(UA_AsyncManager *am, UA_AsyncTask *task) {
    bool hasTimedOut;
    UA_LOCK(am->mutex);
    hasTimedOut = task->hasTimedOut;
    if(hasTimedOut) {
        TAILQ_REMOVE(&am->timedoutTasks, task, entry);
    } else {
        TAILQ_REMOVE(&am->dispatchedTasks, task, entry);
        TAILQ_INSERT_TAIL(&am->finishedTasks, task, entry);
    }
    UA_UNLOCK(am->mutex);
    if(hasTimedOut)
        AsyncTask_delete(task);
}

/******************/
/* Server Methods */
/******************/

void
UA_Server_runAsync(UA_Server *server) {
    UA_AsyncManager *am = server->asyncManager;
    UA_AsyncTask *task;
    while(true) {
        UA_LOCK(am->mutex);
        /* Check if we need to exit before waiting */
        if(am->isStopping) {
            UA_UNLOCK(am->mutex);
            break;
        }
        UA_COND_WAIT(am->cond, am->mutex);
        /* Have we been woken up because we need to exit? */
        if(am->isStopping) {
            UA_UNLOCK(am->mutex);
            break;
        }
        UA_UNLOCK(am->mutex);
        while(true) {
            task = AsyncManager_getAsyncRequest(am);
            if(!task)
                break;
            UA_LOCK(server->serviceMutex);
            UA_Session *session = UA_Server_getSessionById(server, &task->sessionId);
            if(session)
                task->service(server, session, task->request, task->response);
            UA_UNLOCK(server->serviceMutex);
            AsyncManager_finalizeTask(am, task);
        }
    }
}

void
UA_Server_stopAsync(UA_Server *server) {
    UA_AsyncManager_stop(server->asyncManager);
}

UA_StatusCode
UA_Server_processServiceOperationsAsync(UA_Server *server, UA_Session *session,
                                        UA_ServiceOperation operationCallback,
                                        const void *context, const size_t *requestOperations,
                                        const UA_DataType *requestOperationsType,
                                        size_t *responseOperations,
                                        const UA_DataType *responseOperationsType) {
    size_t ops = *requestOperations;
    if(ops == 0)
        return UA_STATUSCODE_BADNOTHINGTODO;

    /* No padding after size_t */
    void **respPos = (void**)((uintptr_t)responseOperations + sizeof(size_t));
    if(!(*respPos)) {
        *respPos = UA_Array_new(ops, responseOperationsType);
        if(!(*respPos))
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    *responseOperations = ops;
    uintptr_t respOp = (uintptr_t)*respPos;
    /* No padding after size_t */
    uintptr_t reqOp = *(uintptr_t*)((uintptr_t)requestOperations + sizeof(size_t));
    for(size_t i = 0; i < ops; i++) {
        operationCallback(server, session, context, (void*)reqOp, (void*)respOp);
        reqOp += requestOperationsType->memSize;
        respOp += responseOperationsType->memSize;
    }
    return UA_STATUSCODE_GOOD;
}

#endif
