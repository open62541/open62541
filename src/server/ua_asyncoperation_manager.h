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

#ifndef UA_ASYNCOPERATION_MANAGER_H_
#define UA_ASYNCOPERATION_MANAGER_H_

#include <open62541/server.h>

#include "open62541_queue.h"
#include "ua_session.h"
#include "ua_util_internal.h"
#include "ua_workqueue.h"

_UA_BEGIN_DECLS

#if UA_MULTITHREADING >= 100

struct AsyncMethodQueueElement {
    SIMPLEQ_ENTRY(AsyncMethodQueueElement) next;
    UA_CallMethodRequest m_Request;
    UA_CallMethodResult	m_Response;
    UA_DateTime	m_tDispatchTime;
    UA_UInt32	m_nRequestId;
    UA_NodeId	m_nSessionId;
    UA_UInt32	m_nIndex;
};
    
/* Internal Helper to transfer info */
struct AsyncMethodContextInternal {
    UA_UInt32 nRequestId;
    UA_NodeId nSessionId;
    UA_UInt32 nIndex;
    const UA_CallRequest* pRequest;
    UA_SecureChannel* pChannel;
};

typedef struct asyncOperationEntry {
    LIST_ENTRY(asyncOperationEntry) pointers;
    UA_UInt32 requestId;
    UA_NodeId sessionId;
    UA_UInt32 requestHandle;
    UA_DateTime	dispatchTime;       /* Creation time */
    UA_UInt32 nCountdown;			/* Counter for open UA_CallResults */
    UA_AsyncOperationType operationType;
    union {
        UA_CallResponse callResponse;
        UA_ReadResponse readResponse;
        UA_WriteResponse writeResponse;
    } response;
} asyncOperationEntry;

typedef struct UA_AsyncOperationManager {
    /* Requests / Responses */
    LIST_HEAD(, asyncOperationEntry) asyncOperations;
    UA_UInt32 currentCount;

    /* Operations belonging to a request */
    UA_UInt32	nMQCurSize;		/* actual size of queue */
    UA_UInt64	nCBIdIntegrity;	/* id of callback queue check callback  */
    UA_UInt64	nCBIdResponse;	/* id of callback check for a response  */

    UA_LOCK_TYPE(ua_request_queue_lock)
    UA_LOCK_TYPE(ua_response_queue_lock)
    UA_LOCK_TYPE(ua_pending_list_lock)

    SIMPLEQ_HEAD(, AsyncMethodQueueElement) ua_method_request_queue;    
    SIMPLEQ_HEAD(, AsyncMethodQueueElement) ua_method_response_queue;
    SIMPLEQ_HEAD(, AsyncMethodQueueElement) ua_method_pending_list;
} UA_AsyncOperationManager;

void
UA_AsyncOperationManager_init(UA_AsyncOperationManager *amm, UA_Server *server);

/* Deletes all entries */
void UA_AsyncOperationManager_clear(UA_AsyncOperationManager *amm, UA_Server *server);

UA_StatusCode
UA_AsyncOperationManager_createEntry(UA_AsyncOperationManager *amm, UA_Server *server,
                                  const UA_NodeId *sessionId, const UA_UInt32 channelId,
                                  const UA_UInt32 requestId, const UA_UInt32 requestHandle,
                                  const UA_AsyncOperationType operationType,
                                  const UA_UInt32 nCountdown);

/* The pointers amm and current must not be NULL */
void
UA_AsyncOperationManager_removeEntry(UA_AsyncOperationManager *amm, asyncOperationEntry *current);

asyncOperationEntry *
UA_AsyncOperationManager_getById(UA_AsyncOperationManager *amm, const UA_UInt32 requestId,
                              const UA_NodeId *sessionId);

void
UA_AsyncOperationManager_checkTimeouts(UA_Server *server, UA_AsyncOperationManager *amm);

/* Internal definitions for the unit tests */
struct AsyncMethodQueueElement *
UA_AsyncOperationManager_getAsyncMethodResult(UA_AsyncOperationManager *amm);
void deleteMethodQueueElement(struct AsyncMethodQueueElement *pElem);
void UA_AsyncOperationManager_addPendingMethodCall(UA_AsyncOperationManager *amm,
                                                   struct AsyncMethodQueueElement *pElem);
void UA_AsyncOperationManager_rmvPendingMethodCall(UA_AsyncOperationManager *amm,
                                                   struct AsyncMethodQueueElement *pElem);
UA_Boolean
UA_AsyncOperationManager_isPendingMethodCall(UA_AsyncOperationManager *amm,
                                             struct AsyncMethodQueueElement *pElem);
UA_StatusCode
UA_Server_SetNextAsyncMethod(UA_Server *server, const UA_UInt32 nRequestId,
                             const UA_NodeId *nSessionId, const UA_UInt32 nIndex,
                             const UA_CallMethodRequest* pRequest);
void UA_Server_CheckQueueIntegrity(UA_Server *server, void *_);

#endif /* UA_MULTITHREADING >= 100 */

_UA_END_DECLS

#endif /* UA_ASYNCOPERATION_MANAGER_H_ */
