/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_SERVER_LOGOBJECT_H_
#define UA_SERVER_LOGOBJECT_H_

#include <open62541/server.h>
#include "open62541_queue.h"

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_LOGOBJECT

/* Maximum length of a message captured from the server logger. Longer
 * messages are truncated. The buffer lives on the stack, the logging path
 * does not allocate memory. */
#define UA_LOGOBJECT_MAXMESSAGELENGTH 1024

/* A LogObject known to the server (OPC UA Part 26). The ServerLog is created
 * during the server initialization, further LogObjects are added with
 * UA_Server_addLogObject. */
typedef struct UA_LogObjectEntry {
    LIST_ENTRY(UA_LogObjectEntry) pointers;
    UA_NodeId nodeId;
    UA_LogObjectSettings settings;
    UA_Boolean isServerLog;
    UA_atomic(uintptr_t) overflowPending; /* Set from any thread */
} UA_LogObjectEntry;

typedef LIST_HEAD(UA_LogObjectList, UA_LogObjectEntry) UA_LogObjectList;

/* A continuation point of the GetRecords Method. It is owned by the Session
 * and stores the filter of the original call together with the cursor of the
 * storage backend. */
typedef struct UA_LogObjectContinuationPoint {
    TAILQ_ENTRY(UA_LogObjectContinuationPoint) pointers;
    UA_Guid identifier; /* Returned to the client as a 16-byte ByteString */
    UA_NodeId logObjectId;
    UA_DateTime startTime;
    UA_DateTime endTime;
    UA_UInt16 minimumSeverity;
    UA_LogRecordMask requestMask;
    UA_UInt32 maxReturnRecords;
    UA_LogObjectCursor cursor;
} UA_LogObjectContinuationPoint;

/* Callback of the GetRecords Method (Part 26, 5.3). Attached to the Method of
 * the LogObjectType, which serves the instantiated LogObjects, and to the
 * GetRecords Method of the ServerLog. */
UA_StatusCode
logObjectGetRecordsMethod(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output);

/* Callback of the ReleaseContinuationPoint Method (Part 26, 5.4). Attached to
 * the Method of the LogObjectType, which is referenced from every LogObject. */
UA_StatusCode
logObjectReleaseContinuationPointMethod(UA_Server *server, const UA_NodeId *sessionId,
                                        void *sessionContext, const UA_NodeId *methodId,
                                        void *methodContext, const UA_NodeId *objectId,
                                        void *objectContext, size_t inputSize,
                                        const UA_Variant *input, size_t outputSize,
                                        UA_Variant *output);

/* Create the ServerLog, wire the NS0 nodes and capture the server logger.
 * Called from UA_Server_init with the server lock held after the namespace
 * zero is set up. */
UA_StatusCode UA_Server_initLogObjects(UA_Server *server);

/* Restore the original logger and release the LogObjects. Called from
 * UA_Server_delete before the config is cleared. */
void UA_Server_cleanupLogObjects(UA_Server *server);

/* Wire the NS0 nodes: removes the LogObject nodes when the feature is
 * disabled at runtime, otherwise writes the Property values, attaches the
 * Method callbacks and creates the MaxLogObjectContinuationPoints Property
 * of the ServerCapabilities. The server lock must be held. */
UA_StatusCode initNS0LogObject(UA_Server *server);

/* Find a LogObject. The server lock must be held. */
UA_LogObjectEntry *
getLogObjectEntry(UA_Server *server, const UA_NodeId *logObjectId);

/* Append a record to a LogObject and, per Part 26 7.2, also to the ServerLog
 * if the entry is not the ServerLog itself. The server lock must be held. */
UA_StatusCode
addLogRecord(UA_Server *server, UA_LogObjectEntry *entry,
             const UA_LogRecord *record);

/* Map the level of the server logger to the Severity ranges of OPC UA Part
 * 26, Table 9 */
UA_UInt16 logLevelToSeverity(UA_LogLevel level);

#endif /* UA_ENABLE_LOGOBJECT */

_UA_END_DECLS

#endif /* UA_SERVER_LOGOBJECT_H_ */
