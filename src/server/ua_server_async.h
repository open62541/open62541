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

#ifndef UA_SERVER_ASYNC_H_
#define UA_SERVER_ASYNC_H_

#include <open62541/server.h>

#include "open62541_queue.h"
#include "util/ua_util_internal.h"

_UA_BEGIN_DECLS

#if UA_MULTITHREADING >= 100

struct UA_AsyncResponse;
typedef struct UA_AsyncResponse UA_AsyncResponse;

/* A single operation (of a larger request) */
typedef struct UA_AsyncOperation {
    TAILQ_ENTRY(UA_AsyncOperation) pointers;
    UA_CallMethodRequest request;
    UA_CallMethodResult	response;
    size_t index;             /* Index of the operation in the array of ops in
                               * request/response */
    UA_AsyncResponse *parent; /* Always non-NULL. The parent is only removed
                               * when its operations are removed */
} UA_AsyncOperation;

struct UA_AsyncResponse {
    TAILQ_ENTRY(UA_AsyncResponse) pointers; /* Insert new at the end */
    UA_UInt32 requestId;
    UA_NodeId sessionId;
    UA_UInt32 requestHandle;
    UA_DateTime	timeout;
    UA_AsyncOperationType operationType;
    union {
        UA_CallResponse callResponse;
        UA_ReadResponse readResponse;
        UA_WriteResponse writeResponse;
    } response;
    UA_UInt32 opCountdown; /* Counter for outstanding operations. The AR can
                            * only be deleted when all have returned. */
};

typedef TAILQ_HEAD(UA_AsyncOperationQueue, UA_AsyncOperation) UA_AsyncOperationQueue;

typedef struct {
    /* Requests / Responses */
    TAILQ_HEAD(, UA_AsyncResponse) asyncResponses;
    size_t asyncResponsesCount;

    /* Operations for the workers. The queues are all FIFO: Put in at the tail,
     * take out at the head.*/
    UA_Lock queueLock; /* Either take this lock free-standing (with no other
                        * locks). Or take server->serviceMutex first and then
                        * the queueLock. Never take the server->serviceMutex
                        * when the queueLock is already acquired (deadlock)! */
    UA_AsyncOperationQueue newQueue;        /* New operations for the workers */
    UA_AsyncOperationQueue dispatchedQueue; /* Operations taken by a worker. When a result is
                                             * returned, we search for the op here to see if it
                                             * is still "alive" (not timed out). */
    UA_AsyncOperationQueue resultQueue;     /* Results to be integrated */
    size_t opsCount; /* How many operations are transient (in one of the three queues)? */

    UA_UInt64 checkTimeoutCallbackId; /* Registered repeated callbacks */
} UA_AsyncManager;

void UA_AsyncManager_init(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_start(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_stop(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_clear(UA_AsyncManager *am, UA_Server *server);

UA_StatusCode
UA_AsyncManager_createAsyncResponse(UA_AsyncManager *am, UA_Server *server,
                                    const UA_NodeId *sessionId,
                                    const UA_UInt32 requestId,
                                    const UA_UInt32 requestHandle,
                                    const UA_AsyncOperationType operationType,
                                    UA_AsyncResponse **outAr);

/* Only remove the AsyncResponse when the operation count is zero */
void
UA_AsyncManager_removeAsyncResponse(UA_AsyncManager *am, UA_AsyncResponse *ar);

UA_StatusCode
UA_AsyncManager_createAsyncOp(UA_AsyncManager *am, UA_Server *server,
                              UA_AsyncResponse *ar, size_t opIndex,
                              const UA_CallMethodRequest *opRequest);

/* Send out the response with status set. Also removes all outstanding
 * operations from the dispatch queue. The queuelock needs to be taken before
 * calling _cancel. */
UA_UInt32
UA_AsyncManager_cancel(UA_Server *server, UA_Session *session, UA_UInt32 requestHandle);

typedef void (*UA_AsyncServiceOperation)(UA_Server *server, UA_Session *session,
                                         UA_UInt32 requestId, UA_UInt32 requestHandle,
                                         size_t opIndex, const void *requestOperation,
                                         void *responseOperation, UA_AsyncResponse **ar);

/* Creates an AsyncResponse in-situ when an async operation is encountered. If
 * that is the case, the sync responses are moved to the AsyncResponse. */
UA_StatusCode
UA_Server_processServiceOperationsAsync(UA_Server *server, UA_Session *session,
                                        UA_UInt32 requestId, UA_UInt32 requestHandle,
                                        UA_AsyncServiceOperation operationCallback,
                                        const size_t *requestOperations,
                                        const UA_DataType *requestOperationsType,
                                        size_t *responseOperations,
                                        const UA_DataType *responseOperationsType,
                                        UA_AsyncResponse **ar)
UA_FUNC_ATTR_WARN_UNUSED_RESULT;

#endif /* UA_MULTITHREADING >= 100 */

_UA_END_DECLS

#endif /* UA_SERVER_ASYNC_H_ */
