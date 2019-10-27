/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 * based on
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014, 2017 (c) Florian Palm
 *    Copyright 2015 (c) Sten Grüner
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_asyncoperation_manager.h"
#include "ua_server_internal.h"
#include "ua_subscription.h"

#if UA_MULTITHREADING >= 100

/*************************/
/* AsyncOperationManager */
/*************************/

/* Checks queue element timeouts */
void
UA_Server_CheckQueueIntegrity(UA_Server *server, void *_) {
    UA_AsyncOperationManager *amm = &server->asyncMethodManager;

    /* For debugging/testing purposes */
    if(server->config.asyncOperationTimeout <= 0.0) {
        UA_AsyncOperationManager_checkTimeouts(server, amm);
        return;
    }

    /* To prevent a lockup, we remove a maximum 10% of timed out entries */
    /* on small queues, we do at least 3 */
    size_t bMaxRemove = server->config.maxAsyncOperationQueueSize / 10;
    if(bMaxRemove < 3)
        bMaxRemove = 3;
    UA_LOCK(amm->ua_request_queue_lock);
    /* Check ifentry has been in the queue too long time */
    while(bMaxRemove-- && !SIMPLEQ_EMPTY(&amm->ua_method_request_queue)) {
        struct AsyncMethodQueueElement* request_elem = SIMPLEQ_FIRST(&amm->ua_method_request_queue);
        UA_DateTime tNow = UA_DateTime_now();
        UA_DateTime tReq = request_elem->m_tDispatchTime;
        UA_DateTime diff = tNow - tReq;
        /* queue entry is not older than server->nMQTimeoutSecs, so we stop checking */
        if(diff <= (UA_DateTime)(server->config.asyncOperationTimeout * UA_DATETIME_MSEC))
            break;

        /* remove it from the queue */
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "UA_Server_CheckQueueIntegrity: Request #%u was removed due to a timeout (%f)",
                       request_elem->m_nRequestId, server->config.asyncOperationTimeout);
        SIMPLEQ_REMOVE_HEAD(&amm->ua_method_request_queue, next);
        amm->nMQCurSize--;
        /* Notify that we removed this request - e.g. Bad Call Response
         * (UA_STATUSCODE_BADREQUESTTIMEOUT) */
        UA_CallMethodResult* result = &request_elem->m_Response;
        UA_CallMethodResult_clear(result);
        result->statusCode = UA_STATUSCODE_BADREQUESTTIMEOUT;
        UA_Server_InsertMethodResponse(server, request_elem->m_nRequestId,
                                       &request_elem->m_nSessionId,
                                       request_elem->m_nIndex, result);
        UA_CallMethodResult_clear(result);
        UA_Server_DeleteMethodQueueElement(server, request_elem);
    }
    UA_UNLOCK(amm->ua_request_queue_lock);

    /* Clear all pending */
    UA_LOCK(amm->ua_pending_list_lock);
    /* Check ifentry has been in the pendig list too long time */
    while(!SIMPLEQ_EMPTY(&amm->ua_method_pending_list)) {
        struct AsyncMethodQueueElement* request_elem = SIMPLEQ_FIRST(&amm->ua_method_pending_list);
        UA_DateTime tNow = UA_DateTime_now();
        UA_DateTime tReq = request_elem->m_tDispatchTime;
        UA_DateTime diff = tNow - tReq;

        /* list entry is not older than server->nMQTimeoutSecs, so we stop checking */
        if(diff <= (UA_DateTime)(server->config.asyncOperationTimeout * UA_DATETIME_MSEC))
            break;
            
        /* Remove it from the list */
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "UA_Server_CheckQueueIntegrity: Pending request #%u was removed "
                       "due to a timeout (%f)", request_elem->m_nRequestId,
                       server->config.asyncOperationTimeout);
        SIMPLEQ_REMOVE_HEAD(&amm->ua_method_pending_list, next);
        /* Notify that we removed this request - e.g. Bad Call Response
         * (UA_STATUSCODE_BADREQUESTTIMEOUT) */
        UA_CallMethodResult* result = &request_elem->m_Response;
        UA_CallMethodResult_clear(result);
        result->statusCode = UA_STATUSCODE_BADREQUESTTIMEOUT;
        UA_Server_InsertMethodResponse(server, request_elem->m_nRequestId,
                                       &request_elem->m_nSessionId,
                                       request_elem->m_nIndex, result);
        UA_CallMethodResult_clear(result);
        UA_Server_DeleteMethodQueueElement(server, request_elem);
    }
    UA_UNLOCK(amm->ua_pending_list_lock);

    /* Now we check ifwe still have pending CallRequests */
    UA_AsyncOperationManager_checkTimeouts(server, amm);
}

void
UA_AsyncOperationManager_init(UA_AsyncOperationManager *amm, UA_Server *server) {
    memset(amm, 0, sizeof(UA_AsyncOperationManager));
    LIST_INIT(&amm->asyncOperations);

    amm->nMQCurSize = 0;

    SIMPLEQ_INIT(&amm->ua_method_request_queue);
    SIMPLEQ_INIT(&amm->ua_method_response_queue);
    SIMPLEQ_INIT(&amm->ua_method_pending_list);

    UA_LOCK_INIT(amm->ua_request_queue_lock);
    UA_LOCK_INIT(amm->ua_response_queue_lock);
    UA_LOCK_INIT(amm->ua_pending_list_lock);

    /* Add a regular callback for cleanup and maintenance using a 10s interval. */
    UA_Server_addRepeatedCallback(server, (UA_ServerCallback)UA_Server_CheckQueueIntegrity,
                                  NULL, 10000.0, &amm->nCBIdIntegrity);

    /* Add a regular callback for for checking responmses using a 50ms interval. */
    UA_Server_addRepeatedCallback(server, (UA_ServerCallback)UA_Server_CallMethodResponse,
                                  NULL, 50.0, &amm->nCBIdResponse);
}

void
UA_AsyncOperationManager_clear(UA_AsyncOperationManager *amm, UA_Server *server) {
    UA_Server_removeCallback(server, amm->nCBIdResponse);
    UA_Server_removeCallback(server, amm->nCBIdIntegrity);

    /* Clean up request queue */
    UA_LOCK(amm->ua_request_queue_lock);
    while(!SIMPLEQ_EMPTY(&amm->ua_method_request_queue)) {
        struct AsyncMethodQueueElement* request = SIMPLEQ_FIRST(&amm->ua_method_request_queue);
        SIMPLEQ_REMOVE_HEAD(&amm->ua_method_request_queue, next);
        UA_Server_DeleteMethodQueueElement(server, request);
    }
    UA_UNLOCK(amm->ua_request_queue_lock);

    /* Clean up response queue */
    UA_LOCK(amm->ua_response_queue_lock);
    while(!SIMPLEQ_EMPTY(&amm->ua_method_response_queue)) {
        struct AsyncMethodQueueElement* response = SIMPLEQ_FIRST(&amm->ua_method_response_queue);
        SIMPLEQ_REMOVE_HEAD(&amm->ua_method_response_queue, next);
        UA_Server_DeleteMethodQueueElement(server, response);
    }
    UA_UNLOCK(amm->ua_response_queue_lock);

    /* Clear all pending */
    UA_LOCK(amm->ua_pending_list_lock);
    while(!SIMPLEQ_EMPTY(&amm->ua_method_pending_list)) {
        struct AsyncMethodQueueElement* response = SIMPLEQ_FIRST(&amm->ua_method_pending_list);
        SIMPLEQ_REMOVE_HEAD(&amm->ua_method_pending_list, next);
        UA_Server_DeleteMethodQueueElement(server, response);
    }
    UA_UNLOCK(amm->ua_pending_list_lock);

    /* Delete all locks */
    UA_LOCK_DESTROY(amm->ua_response_queue_lock);
    UA_LOCK_DESTROY(amm->ua_request_queue_lock);
    UA_LOCK_DESTROY(amm->ua_pending_list_lock);

    asyncOperationEntry *current, *temp;
    LIST_FOREACH_SAFE(current, &amm->asyncOperations, pointers, temp) {
        UA_AsyncOperationManager_removeEntry(amm, current);
    }
}

asyncOperationEntry *
UA_AsyncOperationManager_getById(UA_AsyncOperationManager *amm, const UA_UInt32 requestId,
                              const UA_NodeId *sessionId) {
    asyncOperationEntry *current = NULL;
    LIST_FOREACH(current, &amm->asyncOperations, pointers) {
        if(current->requestId == requestId &&
           UA_NodeId_equal(&current->sessionId, sessionId))
            return current;
    }
    return NULL;
}

UA_StatusCode
UA_AsyncOperationManager_createEntry(UA_AsyncOperationManager *amm, UA_Server *server,
                                  const UA_NodeId *sessionId, const UA_UInt32 channelId,
                                  const UA_UInt32 requestId, const UA_UInt32 requestHandle,
                                  const UA_AsyncOperationType operationType,
                                  const UA_UInt32 nCountdown) {
    asyncOperationEntry *newentry = (asyncOperationEntry*)
        UA_calloc(1, sizeof(asyncOperationEntry));
    if(!newentry) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "UA_AsyncOperationManager_createEntry: Mem alloc failed.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_StatusCode res = UA_NodeId_copy(sessionId, &newentry->sessionId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "UA_AsyncOperationManager_createEntry: Mem alloc failed.");
        UA_free(newentry);
        return res;
    }

    UA_atomic_addUInt32(&amm->currentCount, 1);
    newentry->requestId = requestId;
    newentry->requestHandle = requestHandle;
    newentry->nCountdown = nCountdown;
    newentry->dispatchTime = UA_DateTime_now();
    UA_CallResponse_init(&newentry->response.callResponse);
    newentry->response.callResponse.results = (UA_CallMethodResult*)
        UA_calloc(nCountdown, sizeof(UA_CallMethodResult));
    newentry->response.callResponse.resultsSize = nCountdown;
    if(newentry->response.callResponse.results == NULL) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "UA_AsyncOperationManager_createEntry: Mem alloc failed.");
        UA_free(newentry);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Set the StatusCode to timeout by default. Will be overwritten when the
     * result is set. */
    for(size_t i = 0; i < nCountdown; i++)
        newentry->response.callResponse.results[i].statusCode = UA_STATUSCODE_BADTIMEOUT;

    LIST_INSERT_HEAD(&amm->asyncOperations, newentry, pointers);

    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "UA_AsyncOperationManager_createEntry: Chan: %u. Req# %u", channelId, requestId);

    return UA_STATUSCODE_GOOD;
}

/* Remove entry and free all allocated data */
void
UA_AsyncOperationManager_removeEntry(UA_AsyncOperationManager *amm,
                                  asyncOperationEntry *current) {
    UA_assert(current);
    LIST_REMOVE(current, pointers);
    UA_atomic_subUInt32(&amm->currentCount, 1);
    UA_CallResponse_clear(&current->response.callResponse);
    UA_NodeId_clear(&current->sessionId);
    UA_free(current);
}

/* Check if CallRequest is waiting way too long (120s) */
void
UA_AsyncOperationManager_checkTimeouts(UA_Server *server, UA_AsyncOperationManager *amm) {
    asyncOperationEntry* current = NULL;
    asyncOperationEntry* current_tmp = NULL;
    LIST_FOREACH_SAFE(current, &amm->asyncOperations, pointers, current_tmp) {
        UA_DateTime tNow = UA_DateTime_now();
        UA_DateTime tReq = current->dispatchTime;
        UA_DateTime diff = tNow - tReq;

        /* The calls are all done or the timeout has not passed */
        if (current->nCountdown == 0 || server->config.asyncCallRequestTimeout <= 0.0 ||
            diff <= server->config.asyncCallRequestTimeout * (UA_DateTime)UA_DATETIME_MSEC)
            continue;

        /* We got an unfinished CallResponse waiting way too long for being finished.
         * Set the remaining StatusCodes and return. */
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "UA_AsyncOperationManager_checkTimeouts: "
                       "RequestCall #%u was removed due to a timeout (120s)", current->requestId);

        /* Get the session */
        UA_LOCK(server->serviceMutex);
        UA_Session* session = UA_SessionManager_getSessionById(&server->sessionManager,
                                                               &current->sessionId);
        UA_UNLOCK(server->serviceMutex);
        if(!session) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "UA_AsyncOperationManager_checkTimeouts: Session is gone");
            goto remove;
        }

        /* Check the channel */
        if(!session->header.channel) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "UA_Server_InsertMethodResponse: Channel is gone");
            goto remove;
        }

        /* Okay, here we go, send the UA_CallResponse */
        sendResponse(session->header.channel, current->requestId, current->requestHandle,
                     (UA_ResponseHeader*)&current->response.callResponse.responseHeader,
                     &UA_TYPES[UA_TYPES_CALLRESPONSE]);
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "UA_Server_SendResponse: Response for Req# %u sent", current->requestId);
    remove:
        UA_AsyncOperationManager_removeEntry(amm, current);
    }
}

/***************/
/* MethodQueue */
/***************/

/* Enqueue next MethodRequest */
UA_StatusCode
UA_Server_SetNextAsyncMethod(UA_Server *server, const UA_UInt32 nRequestId,
                             const UA_NodeId *nSessionId, const UA_UInt32 nIndex,
                             const UA_CallMethodRequest *pRequest) {
    UA_AsyncOperationManager *amm = &server->asyncMethodManager;

    if(server->config.maxAsyncOperationQueueSize != 0 &&
        amm->nMQCurSize >= server->config.maxAsyncOperationQueueSize) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "UA_Server_SetNextAsyncMethod: Queue exceeds limit (%d).",
                       (UA_UInt32)server->config.maxAsyncOperationQueueSize);
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    struct AsyncMethodQueueElement* elem = (struct AsyncMethodQueueElement*)
        UA_calloc(1, sizeof(struct AsyncMethodQueueElement));
    if(!elem) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "UA_Server_SetNextAsyncMethod: Mem alloc failed.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_StatusCode result = UA_CallMethodRequest_copy(pRequest, &elem->m_Request);
    if(result != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "UA_Server_SetAsyncMethodResult: UA_CallMethodRequest_copy failed.");                
        UA_free(elem);
        return result;
    }

    UA_CallMethodResult_init(&elem->m_Response);
    elem->m_nRequestId = nRequestId;
    elem->m_nSessionId = *nSessionId;
    elem->m_nIndex = nIndex;
    elem->m_tDispatchTime = UA_DateTime_now();

    UA_LOCK(amm->ua_request_queue_lock);
    SIMPLEQ_INSERT_TAIL(&amm->ua_method_request_queue, elem, next);
    amm->nMQCurSize++;
    UA_UNLOCK(amm->ua_request_queue_lock);

    if(server->config.asyncOperationNotifyCallback)
        server->config.asyncOperationNotifyCallback(server);

    return UA_STATUSCODE_GOOD;
}

/* Deep delete queue Element - only memory we did allocate */
void
UA_Server_DeleteMethodQueueElement(UA_Server *server, struct AsyncMethodQueueElement *pElem) {
    UA_CallMethodRequest_clear(&pElem->m_Request);
    UA_CallMethodResult_clear(&pElem->m_Response);
    UA_free(pElem);
}

void UA_Server_AddPendingMethodCall(UA_Server* server, struct AsyncMethodQueueElement *pElem) {
    UA_AsyncOperationManager *amm = &server->asyncMethodManager;

    UA_LOCK(amm->ua_pending_list_lock);
    pElem->m_tDispatchTime = UA_DateTime_now(); /* reset timestamp for timeout */
    SIMPLEQ_INSERT_TAIL(&amm->ua_method_pending_list, pElem, next);
    UA_UNLOCK(amm->ua_pending_list_lock);
}

void
UA_Server_RmvPendingMethodCall(UA_Server *server, struct AsyncMethodQueueElement *pElem) {
    UA_AsyncOperationManager *amm = &server->asyncMethodManager;

    /* Remove element from pending list */
    /* Do NOT delete it because we still need it */
    struct AsyncMethodQueueElement* current = NULL;
    struct AsyncMethodQueueElement* tmp_iter = NULL;
    struct AsyncMethodQueueElement* previous = NULL;
    UA_LOCK(amm->ua_pending_list_lock);
    SIMPLEQ_FOREACH_SAFE(current, &amm->ua_method_pending_list, next, tmp_iter) {
        if(pElem == current) {
            if(previous == NULL)
                SIMPLEQ_REMOVE_HEAD(&amm->ua_method_pending_list, next);
            else
                SIMPLEQ_REMOVE_AFTER(&amm->ua_method_pending_list, previous, next);
            break;
        }
        previous = current;
    }
    UA_UNLOCK(amm->ua_pending_list_lock);
    return;
}

UA_Boolean
UA_Server_IsPendingMethodCall(UA_Server *server, struct AsyncMethodQueueElement *pElem) {
    UA_AsyncOperationManager *amm = &server->asyncMethodManager;

    UA_Boolean bRV = UA_FALSE;
    struct AsyncMethodQueueElement* current = NULL;
    struct AsyncMethodQueueElement* tmp_iter = NULL;
    UA_LOCK(amm->ua_pending_list_lock);
    SIMPLEQ_FOREACH_SAFE(current, &amm->ua_method_pending_list, next, tmp_iter) {
        if(pElem == current) {
            bRV = UA_TRUE;
            break;
        }
    }
    UA_UNLOCK(amm->ua_pending_list_lock);
    return bRV;
}

/* ----------------------------------------------------------------- */
/* Public API */

/* Get and remove next Method Call Request */
UA_Boolean
UA_Server_getAsyncOperation(UA_Server *server, UA_AsyncOperationType *type,
                            const UA_AsyncOperationRequest **request,
                            void **context) {
    UA_AsyncOperationManager *amm = &server->asyncMethodManager;

    UA_Boolean bRV = UA_FALSE;
    *type = UA_ASYNCOPERATIONTYPE_INVALID;
    struct AsyncMethodQueueElement *elem = NULL;
    UA_LOCK(amm->ua_request_queue_lock);
    if(!SIMPLEQ_EMPTY(&amm->ua_method_request_queue)) {
        elem = SIMPLEQ_FIRST(&amm->ua_method_request_queue);
        SIMPLEQ_REMOVE_HEAD(&amm->ua_method_request_queue, next);
        amm->nMQCurSize--;
        if(elem) {
            *request = (UA_AsyncOperationRequest*)&elem->m_Request;
            *context = (void*)elem;            
            bRV = UA_TRUE;
        }
        else {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "UA_amm_GetNextAsyncMethod: elem is a NULL-Pointer.");
        }
    }
    UA_UNLOCK(amm->ua_request_queue_lock);

    if(bRV && elem) {
        *type = UA_ASYNCOPERATIONTYPE_CALL;
        UA_Server_AddPendingMethodCall(server, elem);
    }
    return bRV;
}

/* Worker submits Method Call Response */
void
UA_Server_setAsyncOperationResult(UA_Server *server,
                                  const UA_AsyncOperationResponse *response,
                                  void *context) {
    struct AsyncMethodQueueElement* elem = (struct AsyncMethodQueueElement*)context;
    if(!elem || !UA_Server_IsPendingMethodCall(server, elem) ) {
        /* Something went wrong, late call? */
        /* Dismiss response */
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
            "UA_Server_SetAsyncMethodResult: elem is a NULL-Pointer or not valid anymore.");
        return;
    }
    
    /* UA_Server_RmvPendingMethodCall MUST be called outside the lock
     * otherwise we can run into a deadlock */
    UA_Server_RmvPendingMethodCall(server, elem);


    UA_StatusCode result = UA_CallMethodResult_copy(&response->callMethodResult,
                                                    &elem->m_Response);
    if(result != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "UA_Server_SetAsyncMethodResult: UA_CallMethodResult_copy failed.");
        /* Add failed CallMethodResult to response queue */
        UA_CallMethodResult_clear(&elem->m_Response);
        elem->m_Response.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Insert response in queue */
    UA_AsyncOperationManager *amm = &server->asyncMethodManager;
    UA_LOCK(amm->ua_response_queue_lock);
    SIMPLEQ_INSERT_TAIL(&amm->ua_method_response_queue, elem, next);
    UA_UNLOCK(amm->ua_response_queue_lock);
}

/******************/
/* Server Methods */
/******************/

void
UA_Server_InsertMethodResponse(UA_Server *server, const UA_UInt32 nRequestId,
                               const UA_NodeId *nSessionId, const UA_UInt32 nIndex,
                               const UA_CallMethodResult *response) {
    /* Grab the open Request, so we can continue to construct the response */
    asyncOperationEntry *data =
        UA_AsyncOperationManager_getById(&server->asyncMethodManager, nRequestId, nSessionId);
    if(!data) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "UA_Server_InsertMethodResponse: can not find UA_CallRequest/UA_CallResponse "
                       "for Req# %u", nRequestId);
        return;
    }

    /* Add UA_CallMethodResult to UA_CallResponse */
    UA_CallResponse* pResponse = &data->response.callResponse;
    UA_CallMethodResult_copy(response, pResponse->results + nIndex);

    /* Reduce the number of open results. Are we done yet with all requests? */
    data->nCountdown -= 1;
    if(data->nCountdown > 0)
        return;
    
    /* Get the session */
    UA_LOCK(server->serviceMutex);
    UA_Session* session = UA_SessionManager_getSessionById(&server->sessionManager, &data->sessionId);
    UA_UNLOCK(server->serviceMutex);
    if(!session) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "UA_Server_InsertMethodResponse: Session is gone");
        UA_AsyncOperationManager_removeEntry(&server->asyncMethodManager, data);
        return;
    }

    /* Check the channel */
    UA_SecureChannel* channel = session->header.channel;
    if(!channel) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "UA_Server_InsertMethodResponse: Channel is gone");
        UA_AsyncOperationManager_removeEntry(&server->asyncMethodManager, data);
        return;
    }

    /* Okay, here we go, send the UA_CallResponse */
    sendResponse(channel, data->requestId, data->requestHandle,
                 (UA_ResponseHeader*)&data->response.callResponse.responseHeader,
                 &UA_TYPES[UA_TYPES_CALLRESPONSE]);
    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "UA_Server_SendResponse: Response for Req# %u sent", data->requestId);
    /* Remove this job from the UA_AsyncOperationManager */
    UA_AsyncOperationManager_removeEntry(&server->asyncMethodManager, data);
}

/* Get next Method Call Response, user has to call
 * 'UA_DeleteMethodQueueElement(...)' to cleanup memory */
struct AsyncMethodQueueElement *
UA_Server_GetAsyncMethodResult(UA_AsyncOperationManager *amm) {
     struct AsyncMethodQueueElement *elem = NULL;
    UA_LOCK(amm->ua_response_queue_lock);
    if(!SIMPLEQ_EMPTY(&amm->ua_method_response_queue)) {
        elem = SIMPLEQ_FIRST(&amm->ua_method_response_queue);
        SIMPLEQ_REMOVE_HEAD(&amm->ua_method_response_queue, next);
    }
    UA_UNLOCK(amm->ua_response_queue_lock);
    return elem;
}

void
UA_Server_CallMethodResponse(UA_Server *server, void* data) {
    /* Server fetches Result from queue */
    struct AsyncMethodQueueElement* pResponseServer = NULL;
    while((pResponseServer = UA_Server_GetAsyncMethodResult(&server->asyncMethodManager))) {
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "UA_Server_CallMethodResponse: Got Response: OKAY");
        UA_Server_InsertMethodResponse(server, pResponseServer->m_nRequestId,
                                       &pResponseServer->m_nSessionId,
                                       pResponseServer->m_nIndex,
                                       &pResponseServer->m_Response);
        UA_Server_DeleteMethodQueueElement(server, pResponseServer);
    }
}

static UA_StatusCode
setMethodNodeAsync(UA_Server *server, UA_Session *session,
                   UA_Node *node, UA_Boolean *isAsync) {
    UA_MethodNode *method = (UA_MethodNode*)node;
    if(method->nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    method->async = *isAsync;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setMethodNodeAsync(UA_Server *server, const UA_NodeId id,
                             UA_Boolean isAsync) {
    return UA_Server_editNode(server, &server->adminSession, &id,
                              (UA_EditNodeCallback)setMethodNodeAsync, &isAsync);
}

/* this is a copy of the above + contest.nIndex is set :-( Any ideas for a better solution? */
UA_StatusCode
UA_Server_processServiceOperationsAsync(UA_Server *server, UA_Session *session,
    UA_ServiceOperation operationCallback,
    void *context, const size_t *requestOperations,
    const UA_DataType *requestOperationsType,
    size_t *responseOperations,
    const UA_DataType *responseOperationsType) {
    size_t ops = *requestOperations;
    if (ops == 0)
        return UA_STATUSCODE_BADNOTHINGTODO;

    struct AsyncMethodContextInternal* pContext = (struct AsyncMethodContextInternal*)context;

    /* No padding after size_t */
    void **respPos = (void**)((uintptr_t)responseOperations + sizeof(size_t));
    *respPos = UA_Array_new(ops, responseOperationsType);
    if (!(*respPos))
        return UA_STATUSCODE_BADOUTOFMEMORY;

    *responseOperations = ops;
    uintptr_t respOp = (uintptr_t)*respPos;
    /* No padding after size_t */
    uintptr_t reqOp = *(uintptr_t*)((uintptr_t)requestOperations + sizeof(size_t));
    for (size_t i = 0; i < ops; i++) {
        pContext->nIndex = (UA_UInt32)i;
        operationCallback(server, session, context, (void*)reqOp, (void*)respOp);
        reqOp += requestOperationsType->memSize;
        respOp += responseOperationsType->memSize;
    }
    return UA_STATUSCODE_GOOD;
}

#endif
