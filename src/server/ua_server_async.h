/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
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
#include "../util/ua_util_internal.h"
#include "ua_session.h"

_UA_BEGIN_DECLS

struct UA_AsyncResponse;
typedef struct UA_AsyncResponse UA_AsyncResponse;

typedef enum {
    UA_ASYNCOPERATIONTYPE_CALL_REQUEST  = 0,
    UA_ASYNCOPERATIONTYPE_READ_REQUEST  = 1,
    UA_ASYNCOPERATIONTYPE_WRITE_REQUEST = 2,
    UA_ASYNCOPERATIONTYPE_CALL_DIRECT   = (0 + 4),
    UA_ASYNCOPERATIONTYPE_READ_DIRECT   = (1 + 4),
    UA_ASYNCOPERATIONTYPE_WRITE_DIRECT  = (2 + 4)
} UA_AsyncOperationType;

/* A single operation (of a larger request) */
typedef struct UA_AsyncOperation {
    TAILQ_ENTRY(UA_AsyncOperation) pointers;
    UA_AsyncOperationType asyncOperationType;

    union {
        /* The operation is part of a service request */
        UA_AsyncResponse *response;

        /* The operation was called directly */
        struct {
            UA_DateTime timeout;
            void *context;
            union {
                UA_ServerAsyncReadResultCallback read;
                UA_ServerAsyncWriteResultCallback write;
                UA_ServerAsyncMethodResultCallback call;
            } method;
        } callback;
    } handling;

    /* For service requests: the pointer to the output value in the response
     * For direct calls: the memory for the output value */
    union {
        UA_CallMethodResult *call;
        UA_StatusCode *write;
        UA_DataValue *read;
        UA_CallMethodResult directCall;
        UA_StatusCode directWrite;
        UA_DataValue directRead;
    } output;

    union {
        /* Forward the pointer to writeValue to the operationCallback. So the
         * pointer is stable and the memory location unique, also when the
         * original request has been freed. But this uses a shallow copy. So
         * don't access the writeValue after the operationCallback. */
        UA_WriteValue writeValue;
    } context;
} UA_AsyncOperation;

struct UA_AsyncResponse {
    TAILQ_ENTRY(UA_AsyncResponse) pointers; /* Insert new at the end */

    UA_UInt32 requestId;
    UA_UInt32 requestHandle;
    UA_DateTime timeout;
    UA_NodeId sessionId;
    UA_UInt32 opCountdown; /* Counter for outstanding operations. The AR can
                            * only be deleted when all have returned. */

    const UA_DataType *responseType;
    union {
        UA_CallResponse callResponse;
        UA_ReadResponse readResponse;
        UA_WriteResponse writeResponse;
    } response;
};

typedef struct {
    /* Forward the request id here as the "UA_Service" method signature does not
     * contain it */
    UA_UInt32 currentRequestId;
    UA_UInt32 currentRequestHandle;

    /* Async responses */
    TAILQ_HEAD(, UA_AsyncResponse) waitingResponses;
    TAILQ_HEAD(, UA_AsyncResponse) readyResponses;

    /* Async operations (some direct, some part of an async response) */
    TAILQ_HEAD(, UA_AsyncOperation) waitingOps;
    TAILQ_HEAD(, UA_AsyncOperation) readyOps;
    size_t opsCount; /* Both waiting and ready */

    UA_UInt64 checkTimeoutCallbackId; /* Registered repeated callbacks */

    UA_DelayedCallback dc; /* Delayed callback to have the main thread handle
                            * ready operations and responses */
} UA_AsyncManager;

void UA_AsyncManager_init(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_start(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_stop(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_clear(UA_AsyncManager *am, UA_Server *server);

/* Cancel all outstanding operations for matching session+requestHandle.
 * Then sends out the responses with a StatusCode. */
UA_UInt32
UA_AsyncManager_cancel(UA_Server *server, UA_Session *session, UA_UInt32 requestHandle);

/* Internal async API */
UA_StatusCode
read_async(UA_Server *server, UA_Session *session, const UA_ReadValueId *operation,
           UA_TimestampsToReturn ttr, UA_ServerAsyncReadResultCallback callback,
           void *context, UA_UInt32 timeout);

UA_StatusCode
write_async(UA_Server *server, UA_Session *session, const UA_WriteValue *operation,
            UA_ServerAsyncWriteResultCallback callback, void *context,
            UA_UInt32 timeout);

UA_StatusCode
call_async(UA_Server *server, UA_Session *session, const UA_CallMethodRequest *operation,
           UA_ServerAsyncMethodResultCallback callback, void *context, UA_UInt32 timeout);

void
async_cancel(UA_Server *server, void *context, UA_StatusCode status,
             UA_Boolean cancelSynchronous);

_UA_END_DECLS

#endif /* UA_SERVER_ASYNC_H_ */
