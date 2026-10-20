/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#ifndef UA_DRIVER_LOGOBJECT_H_
#define UA_DRIVER_LOGOBJECT_H_

#include <open62541/server.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

/**
 * LogObject Driver (Experimental)
 * -------------------------------
 *
 * OPC UA Part 26 (LogObject Model) defines the LogObjectType: an Object that
 * collects LogRecords and returns them with its GetRecords Method. The
 * LogObject driver implements the LogObjects of a server. Create it with
 * ``UA_LogObjectDriver_new`` and attach it with ``UA_Server_addDriver``. At
 * most one LogObject driver can be added to a server.
 *
 * Storage Backend
 * ~~~~~~~~~~~~~~~
 * Part 26 leaves the storage of the LogRecords to the implementation. The
 * driver owns the LogObject nodes, the GetRecords and ReleaseContinuationPoint
 * Methods and the continuation points. The records are stored by a backend.
 * ``UA_LogObjectBackend_memory`` creates an in-memory ring buffer backend.
 *
 * Every LogObject is registered with the backend together with its settings.
 * Records are appended per LogObject and read back in the order of their Time
 * via a cursor. The driver keeps the continuation points of the GetRecords
 * Method and hands the cursor stored in a continuation point back to the
 * backend unchanged. */

/* The settings of a LogObject (OPC UA Part 26, 5.2). They are mirrored into the
 * MaxRecords, MaxStorageDuration and MinimumSeverity Properties. */
typedef struct {
    UA_UInt32 maxRecords;         /* Hard limit (> 0). The oldest records are
                                   * evicted when the limit is reached. */
    UA_Double maxStorageDuration; /* Soft limit in ms. Older records may be
                                   * removed. 0 = no time-based expiry. */
    UA_UInt16 minimumSeverity;    /* Records with a lower Severity are not
                                   * stored (1..1000). Enforced by the driver
                                   * before addRecord is called. */
} UA_LogObjectSettings;

/* Position in the records of one LogObject. The cursor is opaque to the
 * driver, which stores the nextCursor of getRecords in a continuation point
 * and hands it back unchanged. The cursor 0 starts with the oldest record. A
 * read with the nextCursor continues after the records that were returned,
 * also when records were evicted or added (also with an older Time) in the
 * meantime. */
typedef UA_UInt64 UA_LogObjectCursor;

typedef struct UA_LogObjectBackend UA_LogObjectBackend;

struct UA_LogObjectBackend {
    void *context;

    /* Release the resources of the backend. Called when the driver is freed. */
    void (*clear)(UA_LogObjectBackend *backend);

    /* Register a LogObject before records are appended to it. Called from the
     * driver under the server lock.
     *
     * A LogObject that was unregistered without deleting its records is opened
     * again with its records. A persistent backend opens the stored records of
     * the LogObject here and may drop the records older than
     * settings.maxStorageDuration (Part 26, 5.2).
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

    /* Change the settings of a registered LogObject. A smaller maxRecords keeps
     * the newest records. Can be NULL, then the settings cannot be changed
     * while the LogObject is registered. */
    UA_StatusCode
    (*updateLogObject)(UA_Server *server, void *backendContext,
                       const UA_NodeId *logObjectId,
                       const UA_LogObjectSettings *settings);

    /* Unregister a LogObject. Called under the server lock.
     *
     * deleteRecords is true when the LogObject was deleted. Then its records
     *               are removed. It is false when the driver stops. Then a
     *               later registration opens the records again. */
    void
    (*unregisterLogObject)(UA_Server *server, void *backendContext,
                           const UA_NodeId *logObjectId, UA_Boolean deleteRecords);

    /* Append a record to a LogObject. The record is deep-copied by the
     * backend, the caller keeps the ownership.
     *
     * This callback may be called from ANY thread, with or without the server
     * lock held, because the server logger is captured into the ServerLog.
     * The implementation must be thread-safe, must not block on the server
     * lock and must not log.
     *
     * now is the current time as seen by the driver. Records older than
     *     settings.maxStorageDuration may be dropped.
     * The record keeps its Time, also if it is older than records that
     * were added before.
     *
     * overflow is set to true if a record that had not yet expired was
     *          discarded because settings.maxRecords was reached. The driver
     *          then emits a LogOverflowEventType Event. */
    UA_StatusCode
    (*addRecord)(UA_Server *server, void *backendContext,
                 const UA_NodeId *logObjectId, const UA_LogRecord *record,
                 UA_DateTime now, UA_Boolean *overflow);

    /* Read records of a LogObject. Called under the server lock.
     *
     * Returns up to maxRecords (never 0) records with
     * startTime <= Time <= endTime and Severity >= minimumSeverity in the
     * order of their Time, oldest first, continuing at cursor. Records older
     * than settings.maxStorageDuration may be dropped before the read. The
     * records are complete, the driver removes the optional fields the client
     * did not request.
     *
     * records is allocated with UA_Array_new and owned by the caller. It is
     *         NULL if no record matches.
     * nextCursor continues after the returned records.
     * moreAvailable is set to true if further matching records exist after
     *               the returned ones. The driver then creates a continuation
     *               point that stores nextCursor. */
    UA_StatusCode
    (*getRecords)(UA_Server *server, void *backendContext,
                  const UA_NodeId *logObjectId,
                  UA_DateTime startTime, UA_DateTime endTime,
                  UA_UInt16 minimumSeverity, UA_LogObjectCursor cursor, size_t maxRecords, UA_DateTime now,
                  size_t *recordsSize, UA_LogRecord **records,
                  UA_LogObjectCursor *nextCursor, UA_Boolean *moreAvailable);
};

/* In-memory storage for LogObjects. Every registered LogObject gets a ring
 * buffer of settings.maxRecords records, kept in the order of their Time. A
 * record with an older Time than the newest one is inserted at its position.
 * The oldest record is evicted when the ring is full (reported as overflow to
 * the driver). Records older than settings.maxStorageDuration are dropped
 * lazily whenever records are appended or read. The records of a LogObject
 * that is unregistered without deleting its records are kept in memory until
 * it is registered again or the backend is cleared. The cursor of this backend
 * is the sequence number of the last record that was read.
 *
 * The backend is thread-safe: addRecord may be called concurrently from any
 * thread. */
UA_EXPORT UA_StatusCode
UA_LogObjectBackend_memory(UA_LogObjectBackend *backend);

/**
 * Construction
 * ~~~~~~~~~~~~
 * The driver takes over the backend: on success the backend structure is
 * copied and its context is owned by the driver, which clears the backend when
 * it is freed. On failure the caller keeps the backend. A NULL backend selects
 * the in-memory backend.
 *
 * Before ``UA_Server_addDriver`` succeeded, the caller has to free the driver.
 * Afterwards the server owns it and frees it with the server. */
UA_EXPORT UA_StatusCode
UA_LogObjectDriver_new(const UA_LogObjectBackend *backend, UA_Driver **outDriver);

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */

_UA_END_DECLS

#endif /* UA_DRIVER_LOGOBJECT_H_ */
