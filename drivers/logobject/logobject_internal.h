/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_LOGOBJECT_INTERNAL_H_
#define UA_LOGOBJECT_INTERNAL_H_

#include <open62541/driver/logobject.h>
#include "open62541_queue.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

#define UA_DRIVER_LOGOBJECT_NAME "logobject"

/* Maximum length of a message captured from the server logger. Longer
 * messages are truncated. The buffer lives on the stack, the logging path
 * does not allocate memory. */
#define UA_LOGOBJECT_MAXMESSAGELENGTH 1024

/* Add to an atomic counter. Returns the value before the addition. */
static UA_INLINE uintptr_t
logObjectAtomicAdd(UA_atomic(uintptr_t) *value, uintptr_t delta) {
    uintptr_t current = UA_atomic_load(value);
    for(;;) {
        uintptr_t expected = current;
        UA_atomic_cmpxchg(value, &expected, current + delta);
        if(expected == current)
            return current;
        current = expected;
    }
}

/* A LogObject served by the driver */
typedef struct LogObjectEntry {
    LIST_ENTRY(LogObjectEntry) pointers;
    UA_NodeId nodeId;
    UA_LogObjectSettings settings;
    UA_Boolean isServerLog;
    UA_Boolean logsReferenceAdded;    /* Organizes from the Logs Folder */
    UA_Boolean releaseReferenceAdded; /* HasComponent to the type's
                                       * ReleaseContinuationPoint Method */
    UA_NodeId minimumSeverityId;      /* Writable MinimumSeverity Property, or
                                       * null */
    UA_Byte minimumSeverityAccessLevel; /* AccessLevel before the binding */

    /* LogOverflowEventType Events */
    UA_atomic(uintptr_t) discarded;   /* Records evicted by an overflow since
                                       * the last Event. From any thread. */
    UA_Boolean overflowReported;      /* An overflow Event was emitted */
    UA_DateTime lastOverflowEvent;    /* Monotonic time of the last Event */
} LogObjectEntry;

/* A continuation point of the GetRecords Method. It stores the filter of the
 * original call together with the cursor of the backend. */
typedef struct LogObjectContinuationPoint {
    LIST_ENTRY(LogObjectContinuationPoint) pointers;
    UA_NodeId sessionId;
    UA_Guid identifier; /* Returned to the client as a 16-byte ByteString */
    UA_NodeId logObjectId;
    UA_DateTime startTime;
    UA_DateTime endTime;
    UA_UInt16 minimumSeverity;
    UA_LogRecordMask requestMask;
    UA_UInt32 maxReturnRecords;
    UA_LogObjectCursor cursor;
} LogObjectContinuationPoint;

/* A Method node the driver serves. The previous callback is restored when the
 * driver stops. */
typedef struct {
    UA_NodeId methodId;
    UA_MethodCallback callback;
    UA_MethodCallback previous;
} LogObjectBoundMethod;

/* The lifecycle of the LogObjectType or a subtype. The driver chains to the
 * lifecycle the application had set and restores it when it stops. */
typedef struct {
    UA_NodeId typeId;
    UA_NodeTypeLifecycle application;
} LogObjectTypeLifecycle;

typedef struct LogObjectDriver {
    UA_Driver driver; /* Must be the first member */
    UA_LogObjectBackend backend;

    /* Parameters, read on every start */
    UA_Boolean serverLogEnabled;
    UA_LogObjectSettings serverLogSettings;
    UA_LogObjectSettings defaultSettings; /* For instances without Properties */
    UA_UInt32 maxRecordsPerCall;
    UA_UInt16 maxContinuationPoints;
    UA_Double overflowEventInterval;

    /* The LogObjects. The ServerLog entry is part of the driver. */
    LIST_HEAD(, LogObjectEntry) logObjects;
    LogObjectEntry serverLog;
    UA_Boolean serverLogRegistered;

    /* Nodes created by the driver and removed when it stops */
    UA_Boolean createdServerLog;
    UA_Boolean createdMaxContinuationPoints;

    LogObjectBoundMethod *boundMethods;
    size_t boundMethodsSize;

    /* Lifecycles of the LogObjectType and its subtypes. A lifecycle that the
     * application has chained to the driver's stays installed when the driver
     * stops. */
    LogObjectTypeLifecycle *types;
    size_t typesSize;
    const UA_NodeId *constructing; /* Node in the application constructor */
    const UA_NodeId *destructing;  /* Node in the application destructor */

    /* LogObjects closed when the driver stopped. Their records are deleted at
     * the next start if the LogObject no longer exists. */
    UA_NodeId *closedIds;
    size_t closedIdsSize;

    LIST_HEAD(, LogObjectContinuationPoint) continuationPoints;

    /* Capture of the server logger into the ServerLog */
    UA_Logger *capturedLogger; /* The rewritten logger, NULL without capture */
    UA_Logger originalLogger;  /* Content of the logger before the capture */
    struct LogObjectDriver *captureOuter; /* Driver that captured the shared
                                           * logger after this one */
    UA_atomic(uintptr_t) capturing;
    UA_atomic(uintptr_t) captureInFlight;
    UA_atomic(uintptr_t) serverLogMinimumSeverity;

    /* LogOverflowEventType Events are emitted from the EventLoop. The delayed
     * callback is armed from any thread, also from the logging path. The
     * one-shot timer emits the overflows that were coalesced because the
     * LogObject reported an overflow less than overflowEventInterval ago. */
    UA_DelayedCallback overflowCallback;
    UA_atomic(uintptr_t) overflowCallbackQueued;
    UA_atomic(uintptr_t) overflowTimerArmed;
    UA_UInt64 overflowTimerId;
    UA_DateTime overflowTimerDue; /* Monotonic time the timer fires */
} LogObjectDriver;

/* Identify the driver before casting a generic UA_Driver */
UA_Boolean
isLogObjectDriver(const UA_Driver *driver);

/* The LogObject driver of the server, also when it is not started */
LogObjectDriver *
findLogObjectDriver(UA_Server *server);

UA_DateTime
logObjectNow(LogObjectDriver *lod);

/* A Duration in ms of at least one DateTime tick that fits into an Int64 tick
 * count (rejects NaN and infinity) */
UA_Boolean
logObjectValidDuration(UA_Double ms);

LogObjectEntry *
findLogObjectEntry(LogObjectDriver *lod, const UA_NodeId *logObjectId);

/* Append a record to a LogObject and, per Part 26 7.2, also to the ServerLog.
 * The driver lock must be held. */
UA_StatusCode
addLogObjectRecord(LogObjectDriver *lod, LogObjectEntry *entry,
                   const UA_LogRecord *record);

/* Namespace zero: create the ServerLog and register the LogObjects */
UA_StatusCode
createServerLogNodes(LogObjectDriver *lod);

void
deleteServerLogNodes(LogObjectDriver *lod);

UA_StatusCode
registerLogObjectNode(LogObjectDriver *lod, LogObjectEntry *entry);

void
unregisterLogObjectNode(LogObjectDriver *lod, LogObjectEntry *entry,
                        UA_Boolean deleteRecords);

void
unbindLogObjectMethods(LogObjectDriver *lod);

/* Serve every instance of the LogObjectType and its subtypes: install the
 * lifecycles of the types and register the existing instances */
UA_StatusCode
serveLogObjectInstances(LogObjectDriver *lod);

/* Restore the lifecycles of the types. The ones the application has chained
 * to stay installed. */
void
restoreLogObjectTypeLifecycles(LogObjectDriver *lod);

/* Free the lifecycles that stayed installed. The driver is not called any
 * more. */
void
clearLogObjectTypeLifecycles(LogObjectDriver *lod);

/* Capture the server logger into the ServerLog. The capture is installed last
 * when the driver starts and removed first when it stops. */
void
installLogCapture(LogObjectDriver *lod);

void
removeLogCapture(LogObjectDriver *lod);

UA_UInt16
logLevelToSeverity(UA_LogLevel level);

/* Methods and continuation points */
UA_StatusCode
logObjectGetRecordsMethod(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output);

UA_StatusCode
logObjectReleaseContinuationPointMethod(UA_Server *server, const UA_NodeId *sessionId,
                                        void *sessionContext, const UA_NodeId *methodId,
                                        void *methodContext, const UA_NodeId *objectId,
                                        void *objectContext, size_t inputSize,
                                        const UA_Variant *input, size_t outputSize,
                                        UA_Variant *output);

/* A record of the LogObject was evicted by an overflow. Called from any
 * thread. */
void
scheduleOverflowEvent(LogObjectDriver *lod, LogObjectEntry *entry);

/* Disarm the delayed callback and the timer of the overflow Events */
void
cancelOverflowEvents(LogObjectDriver *lod);

/* Change the MinimumSeverity of a LogObject. The driver lock must be held. */
void
setMinimumSeverity(LogObjectDriver *lod, LogObjectEntry *entry, UA_UInt16 severity);

/* Write the effective settings into the Properties of the LogObject */
void
mirrorLogObjectProperties(LogObjectDriver *lod, LogObjectEntry *entry);

/* The LogObject exposes a MaxStorageDuration Property with a value. Zero is
 * not a valid value (Part 26, 5.2), so its time limit cannot be removed. The
 * ServerLog created by the driver adds and removes the Property itself. */
UA_Boolean
hasMaxStorageDurationValue(LogObjectDriver *lod, LogObjectEntry *entry);

/* Release the continuation points of a Session (NULL for all Sessions) and of
 * a LogObject (NULL for all LogObjects) */
void
releaseContinuationPoints(LogObjectDriver *lod, const UA_NodeId *sessionId,
                          const UA_NodeId *logObjectId);

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */

_UA_END_DECLS

#endif /* UA_LOGOBJECT_INTERNAL_H_ */
