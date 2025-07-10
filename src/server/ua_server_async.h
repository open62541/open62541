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

/* A single operation (of a larger request) */
typedef struct UA_AsyncOperation {
    TAILQ_ENTRY(UA_AsyncOperation) pointers;
    void *opResult;
    UA_AsyncResponse *parent; /* Always non-NULL. The parent is only removed
                               * when its operations are removed */
} UA_AsyncOperation;

struct UA_AsyncResponse {
    TAILQ_ENTRY(UA_AsyncResponse) pointers; /* Insert new at the end */
    UA_UInt32 requestId;
    UA_UInt32 requestHandle;
    UA_DateTime timeout;
    UA_NodeId sessionId;
    UA_UInt32 opCountdown; /* Counter for outstanding operations. The AR can
                            * only be deleted when all have returned. */
    const UA_AsyncServiceDescription *asyncServiceDescription;
    union {
        UA_CallResponse callResponse;
        UA_ReadResponse readResponse;
        UA_WriteResponse writeResponse;
    } response;
};

typedef TAILQ_HEAD(UA_AsyncOperationQueue, UA_AsyncOperation) UA_AsyncOperationQueue;

typedef struct {
    /* Forward the request id here as the "UA_Service" method signature does not
     * contain it */
    UA_UInt32 currentRequestId;
    UA_UInt32 currentRequestHandle;

    TAILQ_HEAD(, UA_AsyncResponse) asyncResponses;

    /* Operations for the workers. The queues are all FIFO: Put in at the tail,
     * take out at the head.*/
    UA_AsyncOperationQueue ops; /* New operations for the workers */
    size_t opsCount;            /* How many operations are transient (in one of the three queues)? */

    UA_UInt64 checkTimeoutCallbackId; /* Registered repeated callbacks */

    UA_DelayedCallback dc; /* Delayed callback to have the main thread send out responses */
} UA_AsyncManager;

void UA_AsyncManager_init(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_start(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_stop(UA_AsyncManager *am, UA_Server *server);
void UA_AsyncManager_clear(UA_AsyncManager *am, UA_Server *server);

/* Only remove the AsyncResponse when the operation count is zero */
void
UA_AsyncManager_removeAsyncResponse(UA_AsyncManager *am, UA_AsyncResponse *ar);

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
