/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_PLUGIN_LOGOBJECT_H_
#define UA_PLUGIN_LOGOBJECT_H_

#include <open62541/util.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_LOGOBJECT

/**
 * .. _logobject-backend:
 *
 * LogObject Storage Backend
 * =========================
 *
 * OPC UA Part 26 (LogObject Model) leaves the storage of LogRecords to the
 * implementation. The server core owns the LogObject Nodes, the GetRecords and
 * ReleaseContinuationPoint Methods and the continuation points. The storage of
 * the records is delegated to this plugin. An in-memory ring buffer
 * implementation ships with the plugins (``UA_LogObjectBackend_Memory``).
 *
 * Every LogObject -- the well-known ServerLog and the LogObjects created with
 * ``UA_Server_addLogObject`` -- is registered with the backend together with
 * its settings. Records are appended per LogObject and read back in
 * chronological order via a cursor. The core keeps the continuation points of
 * the GetRecords Method and hands the cursor stored in a continuation point
 * back to the backend unchanged. */

/* The limits of a LogObject (OPC UA Part 26, 5.2). They are mirrored into the
 * MaxRecords, MaxStorageDuration and MinimumSeverity Properties. */
typedef struct {
    UA_UInt32 maxRecords;         /* Hard limit (> 0). The oldest records are
                                   * evicted when the limit is reached. */
    UA_Double maxStorageDuration; /* Soft limit in ms. Older records may be
                                   * removed. 0 = no time-based expiry. */
    UA_UInt16 minimumSeverity;    /* Records with a lower Severity are not
                                   * stored (1..1000). Enforced by the core
                                   * before addRecord is called. */
} UA_LogObjectSettings;

/* Position in the record sequence of one LogObject. The cursor 0 addresses the
 * oldest record. Cursors are monotonically increasing: the nextCursor returned
 * by getRecords addresses the first record that was not returned, also after
 * older records have been evicted in the meantime. A cursor that is older than
 * the oldest retained record is clamped to the oldest record. */
typedef UA_UInt64 UA_LogObjectCursor;

typedef struct UA_LogObjectBackend UA_LogObjectBackend;

struct UA_LogObjectBackend {
    void *context;

    /* Release the resources of the backend. Called from
     * UA_ServerConfig_clear. */
    void (*clear)(UA_LogObjectBackend *backend);

    /* Register a LogObject before records are appended to it. Called under
     * the server lock from the EventLoop thread.
     *
     * server is the server that owns the LogObject. It is NULL when the
     *        backend is used without a server.
     * backendContext is the context of the UA_LogObjectBackend.
     * logObjectId is the NodeId of the LogObject.
     * settings are the limits of the LogObject. They stay valid only for the
     *          duration of the call. */
    UA_StatusCode
    (*registerLogObject)(UA_Server *server, void *backendContext,
                         const UA_NodeId *logObjectId,
                         const UA_LogObjectSettings *settings);

    /* Remove a LogObject and all of its records. Called under the server
     * lock. */
    void
    (*unregisterLogObject)(UA_Server *server, void *backendContext,
                           const UA_NodeId *logObjectId);

    /* Append a record to a LogObject. The record is deep-copied by the
     * backend, the caller keeps the ownership.
     *
     * This callback may be called from ANY thread, with or without the server
     * lock held, because the server logger is captured into the ServerLog.
     * The implementation must be thread-safe, must not block on the server
     * lock and must not log.
     *
     * now is the current time as seen by the core. Records older than
     *     settings.maxStorageDuration may be dropped.
     * overflow is set to true if a record that had not yet expired was
     *          evicted because settings.maxRecords was reached. The core
     *          then emits a LogOverflowEventType Event. */
    UA_StatusCode
    (*addRecord)(UA_Server *server, void *backendContext,
                 const UA_NodeId *logObjectId, const UA_LogRecord *record,
                 UA_DateTime now, UA_Boolean *overflow);

    /* Read records of a LogObject. Called under the server lock.
     *
     * Returns up to maxRecords (never 0) records with
     * startTime <= Time <= endTime and Severity >= minimumSeverity, oldest
     * first, beginning at cursor. Records older than
     * settings.maxStorageDuration may be dropped before the read.
     *
     * requestMask names the optional fields of the LogRecord (EventType,
     *             SourceNode, SourceName, TraceContext, AdditionalData) that
     *             the client asked for. The core removes unrequested optional
     *             fields from the result in any case, the backend may leave
     *             them out already.
     * records is allocated with UA_Array_new and owned by the caller. It is
     *         NULL if no record matches.
     * nextCursor addresses the first record that was not returned.
     * moreAvailable is set to true if further matching records exist after
     *               the returned ones. The core then creates a continuation
     *               point that stores nextCursor. */
    UA_StatusCode
    (*getRecords)(UA_Server *server, void *backendContext,
                  const UA_NodeId *logObjectId,
                  UA_DateTime startTime, UA_DateTime endTime,
                  UA_UInt16 minimumSeverity, UA_LogRecordMask requestMask,
                  UA_LogObjectCursor cursor, size_t maxRecords, UA_DateTime now,
                  size_t *recordsSize, UA_LogRecord **records,
                  UA_LogObjectCursor *nextCursor, UA_Boolean *moreAvailable);
};

#endif /* UA_ENABLE_LOGOBJECT */

_UA_END_DECLS

#endif /* UA_PLUGIN_LOGOBJECT_H_ */
