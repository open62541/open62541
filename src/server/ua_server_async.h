/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Fraunhofer IOSB (Author: Klaus Schick)
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
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
#include "ziptree.h"

_UA_BEGIN_DECLS

struct UA_Session;
struct UA_SecureChannel;
typedef struct UA_AsyncResponse UA_AsyncResponse;

/* Responses own their results arrays. Service and local operations share a
 * pool of independent allocations, valid until ownership returns. */

/* The low two bits identify Call/Read/Write; bit 2 selects a local C API call. */
typedef enum {
    UA_ASYNCOPERATIONTYPE_CALL_REQUEST  = 0,
    UA_ASYNCOPERATIONTYPE_READ_REQUEST  = 1,
    UA_ASYNCOPERATIONTYPE_WRITE_REQUEST = 2,
    UA_ASYNCOPERATIONTYPE_CALL_DIRECT   = (0 + 4),
    UA_ASYNCOPERATIONTYPE_READ_DIRECT   = (1 + 4),
    UA_ASYNCOPERATIONTYPE_WRITE_DIRECT  = (2 + 4)
} UA_AsyncOperationType;

typedef union {
    UA_CallMethodResult call;
    UA_StatusCode write;
    UA_DataValue read;
} UA_AsyncOperationOutput;

/* A single operation (of a larger request) */
typedef struct UA_AsyncOperation {
    /* Indexed by the completion identifier until ownership returns. The left
     * pointer doubles as the operation free-list link after completion. */
    ZIP_ENTRY(UA_AsyncOperation) index;
    uintptr_t id; /* Zero when not indexed */
    /* Service result index; zero for local operations. SIZE_MAX means canceled,
     * without changing the application-owned output storage. */
    size_t resultIndex;
    UA_AsyncOperationType asyncOperationType;
    union {
        /* Linked until completion or the cancellation notification. */
        struct {
            LIST_ENTRY(UA_AsyncOperation) pointers;
            UA_AsyncResponse *response;
        } service;

        /* The operation was called directly */
        struct {
            UA_DelayedCallback dc;
            UA_StatusCode cancellationStatus; /* Local result while the application owns output */
            UA_DateTime timeout;
            void *context;
            union {
                UA_ServerAsyncReadResultCallback read;
                UA_ServerAsyncWriteResultCallback write;
                UA_ServerAsyncMethodResultCallback call;
            } method;
        } callback;
    } handling;

    /* Stable output storage, shared by service and local operations */
    UA_AsyncOperationOutput output;

    union {
        struct {
            UA_TimestampsToReturn timestamps;
            UA_Boolean nonNullable; /* Snapshot while the Value's node is held */
        } read;
        /* Shallow copy: &writeValue.value is a stable completion identifier.
         * After the initiating callback, use only its address, not its contents. */
        UA_WriteValue writeValue;
    } context;
} UA_AsyncOperation;

ZIP_HEAD(UA_AsyncOperationTree, UA_AsyncOperation);

struct UA_AsyncResponse {
    TAILQ_ENTRY(UA_AsyncResponse) pointers; /* Insert new at the end */
    LIST_HEAD(, UA_AsyncOperation) operations;

    /* Queued once when ready; delivery recycles the response. Session cleanup
     * is queued after its responses, keeping their context alive. */
    UA_DelayedCallback dc;
    UA_UInt64 responseToken;
    UA_UInt32 uacpRequestId; /* Zero for transports without a UACP RequestId */
    UA_DateTime timeout;
    /* Session removal queues responses before delayed Session cleanup.
     * NULL for records that only owe cancellation notifications. */
    struct UA_Session *session;
    UA_UInt32 pendingResults; /* Results still needed before the response is ready */
    UA_Boolean abandoned;  /* The transport carrier closed before completion */

    UA_AsyncOperationType kind; /* Call, Read or Write */
    /* Own the response during dispatch, then return it to the synchronous
     * caller or retain it for delivery. All three types begin with responseHeader
     * and resultsSize; zero resultsSize means only notifications remain. */
    union {
        UA_CallResponse callResponse;
        UA_ReadResponse readResponse;
        UA_WriteResponse writeResponse;
    } response;
};

typedef struct {
    /* STARTED admits async work. STOPPING retains outstanding operation storage
     * and callbacks; STOPPED means completely drained. */
    UA_Driver driver;

    /* Forward the transport response token here as the "UA_Service" method
     * signature does not contain it. */
    UA_UInt64 currentResponseToken;
    UA_UInt32 currentUacpRequestId;

    /* Responses remain listed through dispatch and delivery. */
    TAILQ_HEAD(, UA_AsyncResponse) responses;

    /* Index of operations whose ownership has not yet returned. */
    struct UA_AsyncOperationTree operations;
    size_t trackedOpsCount; /* Also counts local results awaiting delivery */
    size_t activeDispatch; /* Stack-owned operations and executing callbacks */

    /* Reusable operation storage shared by services, local APIs and facades.
     * Only completed operations enter this pool. Push/pop at the head. */
    UA_AsyncOperation *freeOps;
    size_t freeOpsSize;

    /* Completed response records, also reused in LIFO order. */
    UA_AsyncResponse *freeResponses;
    size_t freeResponsesSize;

    UA_UInt64 checkTimeoutCallbackId; /* Registered repeated callbacks */

} UA_AsyncManager;

void UA_AsyncManager_init(UA_AsyncManager *am, UA_Server *server);

/* Queue canceled service responses before the caller queues Session cleanup. */
void
UA_AsyncManager_cancelSession(UA_Server *server, struct UA_Session *session);

/* Cancel all outstanding operations for matching session+requestHandle.
 * Then sends out the responses with a StatusCode. */
UA_UInt32
UA_AsyncManager_cancel(UA_Server *server, struct UA_Session *session, UA_UInt32 requestHandle);

/* Abandon an asynchronous response whose transport carrier has closed. */
void
UA_AsyncManager_abandon(UA_Server *server, struct UA_SecureChannel *channel,
                        UA_UInt64 responseToken);

/* Internal async API */
UA_StatusCode
read_async(UA_Server *server, struct UA_Session *session, const UA_ReadValueId *operation,
           UA_TimestampsToReturn ttr, UA_ServerAsyncReadResultCallback callback,
           void *context, UA_UInt32 timeout);

_UA_END_DECLS

#endif /* UA_SERVER_ASYNC_H_ */
