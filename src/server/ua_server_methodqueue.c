/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
*    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
*/

#include "ua_server_methodqueue.h"
#include <open62541/types_generated_handling.h>
#include "ua_server_internal.h"
#include <open62541/plugin/log.h>

#if UA_MULTITHREADING >= 100

/* Initialize Request Queue */
void
UA_Server_MethodQueues_init(UA_Server *server) {
    server->nMQCurSize = 0;

    UA_LOCK_INIT(server->ua_request_queue_lock);
    SIMPLEQ_INIT(&server->ua_method_request_queue);

    UA_LOCK_INIT(server->ua_response_queue_lock);
    SIMPLEQ_INIT(&server->ua_method_response_queue);

    UA_LOCK_INIT(server->ua_pending_list_lock);
    SIMPLEQ_INIT(&server->ua_method_pending_list);

    /* Add a regular callback for cleanup and maintenance using a 10s interval. */
    UA_Server_addRepeatedCallback(server, (UA_ServerCallback)UA_Server_CheckQueueIntegrity,
        NULL, 10000.0, &server->nCBIdIntegrity);
}

/* Cleanup and terminate queues */
void
UA_Server_MethodQueues_delete(UA_Server *server) {
    UA_Server_removeCallback(server, server->nCBIdIntegrity);

    /* Clean up request queue */
    UA_LOCK(server->ua_request_queue_lock);
    while (!SIMPLEQ_EMPTY(&server->ua_method_request_queue)) {
        struct AsyncMethodQueueElement* request = SIMPLEQ_FIRST(&server->ua_method_request_queue);
        SIMPLEQ_REMOVE_HEAD(&server->ua_method_request_queue, next);
        UA_Server_DeleteMethodQueueElement(server, request);
    }
    UA_UNLOCK(server->ua_request_queue_lock);

    /* Clean up response queue */
    UA_LOCK(server->ua_response_queue_lock);
    while (!SIMPLEQ_EMPTY(&server->ua_method_response_queue)) {
        struct AsyncMethodQueueElement* response = SIMPLEQ_FIRST(&server->ua_method_response_queue);
        SIMPLEQ_REMOVE_HEAD(&server->ua_method_response_queue, next);
        UA_Server_DeleteMethodQueueElement(server, response);
    }
    UA_UNLOCK(server->ua_response_queue_lock);

    /* Clear all pending */
    UA_LOCK(server->ua_pending_list_lock);
    while (!SIMPLEQ_EMPTY(&server->ua_method_pending_list)) {
        struct AsyncMethodQueueElement* response = SIMPLEQ_FIRST(&server->ua_method_pending_list);
        SIMPLEQ_REMOVE_HEAD(&server->ua_method_pending_list, next);
        UA_Server_DeleteMethodQueueElement(server, response);
    }
    UA_UNLOCK(server->ua_pending_list_lock);

    /* delete all locks */
    /* TODO KS: actually we should make sure the worker is not 'hanging' on this lock anymore */
    Sleep(100);
    UA_LOCK_DESTROY(server->ua_response_queue_lock);
    UA_LOCK_DESTROY(server->ua_request_queue_lock);
    UA_LOCK_DESTROY(server->ua_pending_list_lock);
}

/* Checks queue element timeouts */
void
UA_Server_CheckQueueIntegrity(UA_Server *server, void *data) {
    /* for debugging/testing purposes */
    if(server->config.asyncOperationTimeout <= 0.0) {
        UA_AsyncMethodManager_checkTimeouts(server, &server->asyncMethodManager);
        return;
    }

    /* To prevent a lockup, we remove a maximum 10% of timed out entries */
    /* on small queues, we do at least 3 */
    size_t bMaxRemove = server->config.maxAsyncOperationQueueSize / 10;
    if(bMaxRemove < 3)
        bMaxRemove = 3;
    UA_Boolean bCheckQueue = UA_TRUE;
    UA_LOCK(server->ua_request_queue_lock);
    /* Check if entry has been in the queue too long time */
    while(bCheckQueue && bMaxRemove-- && !SIMPLEQ_EMPTY(&server->ua_method_request_queue)) {
        struct AsyncMethodQueueElement* request_elem = SIMPLEQ_FIRST(&server->ua_method_request_queue);
        if (request_elem) {
            UA_DateTime tNow = UA_DateTime_now();
            UA_DateTime tReq = request_elem->m_tDispatchTime;
            UA_DateTime diff = tNow - tReq;
            if(diff > (UA_DateTime)(server->config.asyncOperationTimeout * UA_DATETIME_MSEC)) {
                /* remove it from the queue */
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                    "UA_Server_CheckQueueIntegrity: Request #%u was removed due to a timeout (%f)",
                    request_elem->m_nRequestId, server->config.asyncOperationTimeout);
                SIMPLEQ_REMOVE_HEAD(&server->ua_method_request_queue, next);
                server->nMQCurSize--;
                /* Notify that we removed this request - e.g. Bad Call Response (UA_STATUSCODE_BADREQUESTTIMEOUT) */
                UA_CallMethodResult* result = &request_elem->m_Response;
                UA_CallMethodResult_clear(result);
                result->statusCode = UA_STATUSCODE_BADREQUESTTIMEOUT;
                UA_Server_InsertMethodResponse(server, request_elem->m_nRequestId, &request_elem->m_nSessionId, request_elem->m_nIndex, result);
                UA_CallMethodResult_clear(result);
                UA_Server_DeleteMethodQueueElement(server, request_elem);
            }
            else {
                /* queue entry is not older than server->nMQTimeoutSecs, so we stop checking */
                bCheckQueue = UA_FALSE;
            }
        }
    }
    UA_UNLOCK(server->ua_request_queue_lock);

    /* Clear all pending */
    bCheckQueue = UA_TRUE;
    UA_LOCK(server->ua_pending_list_lock);
    /* Check if entry has been in the pendig list too long time */
    while(bCheckQueue && !SIMPLEQ_EMPTY(&server->ua_method_pending_list)) {
        struct AsyncMethodQueueElement* request_elem = SIMPLEQ_FIRST(&server->ua_method_pending_list);
        if (request_elem) {
            UA_DateTime tNow = UA_DateTime_now();
            UA_DateTime tReq = request_elem->m_tDispatchTime;
            UA_DateTime diff = tNow - tReq;
            if (diff > (UA_DateTime)(server->config.asyncOperationTimeout * UA_DATETIME_MSEC)) {
                /* remove it from the list */
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                    "UA_Server_CheckQueueIntegrity: Pending request #%u was removed due to a timeout (%f)",
                    request_elem->m_nRequestId, server->config.asyncOperationTimeout);
                SIMPLEQ_REMOVE_HEAD(&server->ua_method_pending_list, next);
                /* Notify that we removed this request - e.g. Bad Call Response (UA_STATUSCODE_BADREQUESTTIMEOUT) */
                UA_CallMethodResult* result = &request_elem->m_Response;
                UA_CallMethodResult_clear(result);
                result->statusCode = UA_STATUSCODE_BADREQUESTTIMEOUT;
                UA_Server_InsertMethodResponse(server, request_elem->m_nRequestId, &request_elem->m_nSessionId, request_elem->m_nIndex, result);
                UA_CallMethodResult_clear(result);
                UA_Server_DeleteMethodQueueElement(server, request_elem);
            }
            else {
                /* list entry is not older than server->nMQTimeoutSecs, so we stop checking */
                bCheckQueue = UA_FALSE;
            }
        }
    }
    UA_UNLOCK(server->ua_pending_list_lock);

    /* Now we check if we still have pending CallRequests */
    UA_AsyncMethodManager_checkTimeouts(server, &server->asyncMethodManager);
}

/* Enqueue next MethodRequest */
UA_StatusCode
UA_Server_SetNextAsyncMethod(UA_Server *server,
    const UA_UInt32 nRequestId,
    const UA_NodeId *nSessionId,
    const UA_UInt32 nIndex,
    const UA_CallMethodRequest *pRequest) {
    UA_StatusCode result = UA_STATUSCODE_GOOD;

    if (server->config.maxAsyncOperationQueueSize == 0 ||
        server->nMQCurSize < server->config.maxAsyncOperationQueueSize) {
        struct AsyncMethodQueueElement* elem = (struct AsyncMethodQueueElement*)UA_calloc(1, sizeof(struct AsyncMethodQueueElement));
        if (elem)
        {
            UA_CallMethodRequest_init(&elem->m_Request);
            result = UA_CallMethodRequest_copy(pRequest, &elem->m_Request);
            if (result != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                    "UA_Server_SetAsyncMethodResult: UA_CallMethodRequest_copy failed.");                
            }

            elem->m_nRequestId = nRequestId;
            elem->m_nSessionId = *nSessionId;
            elem->m_nIndex = nIndex;
            UA_CallMethodResult_clear(&elem->m_Response);
            elem->m_tDispatchTime = UA_DateTime_now();
            UA_LOCK(server->ua_request_queue_lock);
            SIMPLEQ_INSERT_TAIL(&server->ua_method_request_queue, elem, next);
            server->nMQCurSize++;
            if(server->config.asyncOperationNotifyCallback)
                server->config.asyncOperationNotifyCallback(server);
            UA_UNLOCK(server->ua_request_queue_lock);
        }
        else {
            /* notify about error */
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "UA_Server_SetNextAsyncMethod: Mem alloc failed.");
            result = UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    else {
        /* issue warning */
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
            "UA_Server_SetNextAsyncMethod: Queue exceeds limit (%d).",
                       (UA_UInt32)server->config.maxAsyncOperationQueueSize);
        result = UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return result;
}

/* Private API */
/* Get next Method Call Response */
UA_Boolean
UA_Server_GetAsyncMethodResult(UA_Server *server, struct AsyncMethodQueueElement **pResponse) {
    UA_Boolean bRV = UA_FALSE;    
    UA_LOCK(server->ua_response_queue_lock);
    if (!SIMPLEQ_EMPTY(&server->ua_method_response_queue)) {
        *pResponse = SIMPLEQ_FIRST(&server->ua_method_response_queue);
        SIMPLEQ_REMOVE_HEAD(&server->ua_method_response_queue, next);
        bRV = UA_TRUE;
    }
    UA_UNLOCK(server->ua_response_queue_lock);
    return bRV;
}

/* Deep delete queue Element - only memory we did allocate */
void
UA_Server_DeleteMethodQueueElement(UA_Server *server, struct AsyncMethodQueueElement *pElem) {
    UA_CallMethodRequest_clear(&pElem->m_Request);
    UA_CallMethodResult_clear(&pElem->m_Response);
    UA_free(pElem);
}

void UA_Server_AddPendingMethodCall(UA_Server* server, struct AsyncMethodQueueElement *pElem) {
    UA_LOCK(server->ua_pending_list_lock);
    pElem->m_tDispatchTime = UA_DateTime_now(); /* reset timestamp for timeout */
    SIMPLEQ_INSERT_TAIL(&server->ua_method_pending_list, pElem, next);
    UA_UNLOCK(server->ua_pending_list_lock);
}

void UA_Server_RmvPendingMethodCall(UA_Server *server, struct AsyncMethodQueueElement *pElem) {
/* Remove element from pending list */
/* Do NOT delete it because we still need it */
    struct AsyncMethodQueueElement* current = NULL;
    struct AsyncMethodQueueElement* tmp_iter = NULL;
    struct AsyncMethodQueueElement* previous = NULL;
    UA_LOCK(server->ua_pending_list_lock);
    SIMPLEQ_FOREACH_SAFE(current, &server->ua_method_pending_list, next, tmp_iter) {
        if (pElem == current) {
            if (previous == NULL)
                SIMPLEQ_REMOVE_HEAD(&server->ua_method_pending_list, next);
            else
                SIMPLEQ_REMOVE_AFTER(&server->ua_method_pending_list, previous, next);
            break;
        }
        previous = current;
    }
    UA_UNLOCK(server->ua_pending_list_lock);
    return;
}

UA_Boolean UA_Server_IsPendingMethodCall(UA_Server *server, struct AsyncMethodQueueElement *pElem) {
    UA_Boolean bRV = UA_FALSE;
    struct AsyncMethodQueueElement* current = NULL;
    struct AsyncMethodQueueElement* tmp_iter = NULL;
    UA_LOCK(server->ua_pending_list_lock);
    SIMPLEQ_FOREACH_SAFE(current, &server->ua_method_pending_list, next, tmp_iter) {
        if (pElem == current) {
            bRV = UA_TRUE;
            break;
        }
    }
    UA_UNLOCK(server->ua_pending_list_lock);
    return bRV;
}

/* ----------------------------------------------------------------- */
/* Public API */

/* Get and remove next Method Call Request */
UA_Boolean
UA_Server_getAsyncOperation(UA_Server *server, UA_AsyncOperationType *type,
                            const UA_AsyncOperationRequest **request,
                            void **context) {
    UA_Boolean bRV = UA_FALSE;
    *type = UA_ASYNCOPERATIONTYPE_INVALID;
    struct AsyncMethodQueueElement *elem = NULL;
    UA_LOCK(server->ua_request_queue_lock);
    if (!SIMPLEQ_EMPTY(&server->ua_method_request_queue)) {
        elem = SIMPLEQ_FIRST(&server->ua_method_request_queue);
        SIMPLEQ_REMOVE_HEAD(&server->ua_method_request_queue, next);
        server->nMQCurSize--;
        if (elem) {
            *request = (UA_AsyncOperationRequest*)&elem->m_Request;
            *context = (void*)elem;            
            bRV = UA_TRUE;
        }
        else {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "UA_Server_GetNextAsyncMethod: elem is a NULL-Pointer.");
        }
    }
    UA_UNLOCK(server->ua_request_queue_lock);
    if (bRV && elem) {
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
    if (!elem || !UA_Server_IsPendingMethodCall(server, elem) ) {
        /* Something went wrong, late call? */
        /* Dismiss response */
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
            "UA_Server_SetAsyncMethodResult: elem is a NULL-Pointer or not valid anymore.");
    }
    else {
        /* UA_Server_RmvPendingMethodCall MUST be called outside the lock
        * otherwise we can run into a deadlock */
        UA_Server_RmvPendingMethodCall(server, elem);

        UA_StatusCode result = UA_CallMethodResult_copy(&response->callMethodResult,
                                                        &elem->m_Response);
        if (result != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "UA_Server_SetAsyncMethodResult: UA_CallMethodResult_copy failed.");
            /* Add failed CallMethodResult to response queue */
            UA_CallMethodResult_clear(&elem->m_Response);
            elem->m_Response.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        }
        /* Insert response in queue */
        UA_LOCK(server->ua_response_queue_lock);
        SIMPLEQ_INSERT_TAIL(&server->ua_method_response_queue, elem, next);
        UA_UNLOCK(server->ua_response_queue_lock);
    }
}

#endif /* !UA_MULTITHREADING >= 100 */
