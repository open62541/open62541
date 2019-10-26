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

#if UA_MULTITHREADING >= 100

_UA_BEGIN_DECLS

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
    LIST_HEAD(, asyncOperationEntry) asyncOperations;
    UA_UInt32 currentCount;
} UA_AsyncOperationManager;

void
UA_AsyncOperationManager_init(UA_AsyncOperationManager *amm);

/* Deletes all entries */
void UA_AsyncOperationManager_clear(UA_AsyncOperationManager *amm);

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

_UA_END_DECLS

#endif

#endif /* UA_ASYNCOPERATION_MANAGER_H_ */
