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

typedef struct LogObjectDriver {
    UA_Driver driver; /* Must be the first member */
    UA_LogObjectBackend backend;

    /* Parameters, read on every start */
    UA_Boolean serverLogEnabled;
    UA_LogObjectSettings serverLogSettings;
    UA_UInt32 maxRecordsPerCall;
    UA_UInt16 maxContinuationPoints;

    /* The LogObjects. The ServerLog entry is part of the driver. */
    LIST_HEAD(, LogObjectEntry) logObjects;
    LogObjectEntry serverLog;
    UA_Boolean serverLogRegistered;

    /* Nodes created by the driver and removed when it stops */
    UA_Boolean createdServerLog;
    UA_Boolean createdMaxContinuationPoints;

    LogObjectBoundMethod *boundMethods;
    size_t boundMethodsSize;

    LIST_HEAD(, LogObjectContinuationPoint) continuationPoints;

    /* Capture of the server logger into the ServerLog */
    UA_Logger *capturedLogger; /* The rewritten logger, NULL without capture */
    UA_Logger originalLogger;  /* Content of the logger before the capture */
    struct LogObjectDriver *captureOuter; /* Driver that captured the shared
                                           * logger after this one */
    UA_atomic(uintptr_t) capturing;
    UA_atomic(uintptr_t) captureInFlight;
    UA_atomic(uintptr_t) serverLogMinimumSeverity;
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

/* Release the continuation points of a Session (NULL for all Sessions) and of
 * a LogObject (NULL for all LogObjects) */
void
releaseContinuationPoints(LogObjectDriver *lod, const UA_NodeId *sessionId,
                          const UA_NodeId *logObjectId);

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */

_UA_END_DECLS

#endif /* UA_LOGOBJECT_INTERNAL_H_ */
