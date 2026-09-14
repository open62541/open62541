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
} UA_LogObjectEntry;

typedef LIST_HEAD(UA_LogObjectList, UA_LogObjectEntry) UA_LogObjectList;

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
