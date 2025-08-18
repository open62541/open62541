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

_UA_BEGIN_DECLS

struct UA_AsyncResponse;
typedef struct UA_AsyncResponse UA_AsyncResponse;

typedef UA_Boolean (*UA_AsyncServiceOperation)(
    UA_Server *server, UA_Session *session,
    const void *requestOperation, void *responseOperation);

/* The async service description parameterizes the async execution.
 * It should be const-global so that the pointer to it is stable. */
typedef struct {
    const UA_DataType *responseType;
    UA_AsyncServiceOperation operationCallback;
    size_t requestCounterOffset;
    const UA_DataType *requestOperationsType;
    size_t responseCounterOffset;
    const UA_DataType *responseOperationsType;
} UA_AsyncServiceDescription;

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

    /* Always non-NULL */
    union {
        UA_AsyncResponse *response; /* The operation is part of a service request */
    } handling;

    /* The pointer to which the output value is written.
     * The pointer must be */
    union {
        UA_CallMethodResult *call;
        UA_StatusCode *write;
        UA_DataValue *read;
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
    const UA_AsyncServiceDescription *asd;

    UA_UInt32 requestId;
    UA_UInt32 requestHandle;
    UA_DateTime timeout;
    UA_NodeId sessionId;
    UA_UInt32 opCountdown; /* Counter for outstanding operations. The AR can
                            * only be deleted when all have returned. */
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

    /* Async responses that are waiting for operations */
    TAILQ_HEAD(, UA_AsyncResponse) asyncResponses;

    /* Async operations for all async responses */
    TAILQ_HEAD(, UA_AsyncOperation) ops;
    size_t opsCount;

    UA_UInt64 checkTimeoutCallbackId; /* Registered repeated callbacks */

    UA_DelayedCallback dc; /* Delayed callback to have the main thread send out responses */
} UA_AsyncManager;

void UA_AsyncManager_init(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_start(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_stop(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_clear(UA_AsyncManager *am, UA_Server *server);

/* Send out the response with status set. Also removes all outstanding
 * operations from the dispatch queue. The queuelock needs to be taken before
 * calling _cancel. */
UA_UInt32
UA_AsyncManager_cancel(UA_Server *server, UA_Session *session, UA_UInt32 requestHandle);

/* Creates an AsyncResponse with its AsyncOperations as an appendix to the
 * results array. The results array can be "normally" freed once all async
 * operations are processed.
 *
 * If UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY is returned, then the
 * responseOperations array is kept for the AsyncResponse and must not be freed
 * by the calling method. For other returned status codes, the async processing
 * is not visible to the calling method. */
UA_StatusCode
allocProcessServiceOperations_async(UA_Server *server, UA_Session *session,
                                    const UA_AsyncServiceDescription *asDescription,
                                    const void *request, void *response)
UA_FUNC_ATTR_WARN_UNUSED_RESULT;

_UA_END_DECLS

#endif /* UA_SERVER_ASYNC_H_ */
