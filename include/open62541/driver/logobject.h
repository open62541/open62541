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

/**
 * Parameters
 * ~~~~~~~~~~
 * Set ``driver->params`` before the driver starts. The parameters are read on
 * every start, all keys use namespace 0. A known key with another type than
 * listed, or an invalid value, fails the start with ``Bad_InvalidArgument``.
 * Numeric types with the same encoding are accepted as well, e.g. a Double
 * for a Duration.
 *
 * 0:server-log [Boolean]
 *    Expose the well-known ServerLog (Part 26, 7.2) below the Server Object
 *    (default: true).
 * 0:server-log-max-records [UInt32]
 *    MaxRecords of the ServerLog, at least 1 (default: 1000).
 * 0:server-log-max-storage-duration [Duration]
 *    MaxStorageDuration of the ServerLog in milliseconds. 0 disables the
 *    time-based expiry and the Property is not exposed (default: 0).
 * 0:server-log-minimum-severity [UInt16]
 *    MinimumSeverity of the ServerLog, 1..1000 (default: 51, Information and
 *    above, Part 26 Table 9).
 * 0:default-max-records [UInt32]
 *    MaxRecords of the LogObjects in the information model without a valid
 *    MaxRecords Property, at least 1 (default: 1000).
 * 0:default-max-storage-duration [Duration]
 *    MaxStorageDuration in milliseconds of the LogObjects without a valid
 *    MaxStorageDuration Property. 0 disables the time-based expiry (default:
 *    0).
 * 0:default-minimum-severity [UInt16]
 *    MinimumSeverity of the LogObjects without a valid MinimumSeverity
 *    Property, 1..1000 (default: 1).
 * 0:max-records-per-call [UInt32]
 *    Server-imposed limit for the LogRecords returned by one GetRecords call.
 *    0 = only the MaxReturnRecords argument of the client applies (default:
 *    1000).
 * 0:max-continuation-points [UInt16]
 *    Continuation points of the GetRecords Method per Session. 0 = no limit
 *    (default: 32). Mirrored into the MaxLogObjectContinuationPoints
 *    Property of the ServerCapabilities.
 *
 * ServerLog and GetRecords
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 * The started driver creates the ServerLog with the NodeIds of the standard
 * nodeset and removes it again when it stops. Every LogObject is referenced
 * from the Logs Folder (Part 26, 7.3). The GetRecords and
 * ReleaseContinuationPoint Methods are served by the driver. The continuation
 * points belong to the Session and are released when the Session is closed.
 *
 * The ServerLog contains security related information and should only be
 * accessible to authorised users (Part 26, 5.11). Access to the Methods and
 * to the writable MinimumSeverity is controlled by the AccessControl plugin
 * (``getUserExecutableOnObject``, ``getUserAccessLevel``) or by role
 * permissions. The default AccessControl allows all users. The records are
 * not filtered per user.
 *
 * Capture of the Server Logger
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * While it runs, the driver captures every message written to the server
 * logger (``config->logging``) into the ServerLog as a LogRecord:
 *
 * - Time: the current time of the EventLoop clock
 * - Severity: mapped from the log level (TRACE 10, DEBUG 40, INFO 80,
 *   WARNING 180, ERROR 230, FATAL 500; see Part 26, Table 9)
 * - Message: the formatted log message, without a locale
 * - SourceName: the log category (Network, SecureChannel, Session, Server,
 *   Client, Application, Security, EventLoop, PubSub, Discovery)
 *
 * Messages below the MinimumSeverity of the ServerLog are not stored.
 * Messages that are removed at compile time by ``UA_LOGLEVEL`` never reach the
 * logger. The capture is installed when the driver starts and removed when it
 * stops, so messages written before ``UA_Server_run_startup`` are not captured.
 *
 * The capture rewrites the ``UA_Logger`` structure that ``config->logging``
 * points to in place (the original log callback is still called). The
 * structure must be writable: the constant ``UA_Log_Stdout`` is not captured.
 * Several servers can share one logger: each of their drivers captures the
 * messages and they can be stopped in any order. Starting and stopping the
 * driver must not run concurrently with log calls through the same logger
 * from other threads.
 *
 * LogObjects in the Information Model
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Every Object of the LogObjectType or one of its subtypes is a LogObject of
 * the driver: Objects added with ``UA_Server_addObjectNode``, created by the
 * instantiation of an ObjectType (e.g. the log of a device) or loaded from the
 * nodeset of a companion specification. The InstanceDeclarations below
 * ObjectTypes are not LogObjects. The driver installs the node lifecycle of
 * the LogObjectType and its subtypes, chained to the lifecycle the application
 * had set, and registers the existing instances when it starts. An
 * application that sets the lifecycle of such a type while the driver runs
 * chains to the driver's lifecycle (``UA_Server_getNodeTypeLifecycle``). That
 * lifecycle then stays installed when the driver stops and must be restored
 * by the application before the driver is removed.
 *
 * The settings of a LogObject come from its MaxRecords, MaxStorageDuration and
 * MinimumSeverity Properties if they are present and valid, otherwise from
 * the default parameters. The Properties are read when the LogObject is
 * registered: when the driver starts or when the Object is finished. Add them
 * between ``UA_Server_addNode_begin`` and ``UA_Server_addNode_finish`` or
 * change the settings later with ``UA_LogObjectDriver_setSettings``. The
 * effective settings are written back into the Properties. Deleting the
 * Object deletes its records. The records of every LogObject are also added
 * to the ServerLog (Part 26, 7.2).
 *
 * The Logs Folder references every LogObject (Part 26, 7.3). A LogObject that
 * is the component of another Object, e.g. of a device, therefore has a second
 * hierarchical parent and is not deleted together with that Object. Delete it
 * explicitly.
 *
 * Subtypes of the LogObjectType that are added after the driver started, e.g.
 * by a nodeset that is loaded at runtime, are served after a refresh. Returns
 * ``Bad_InvalidState`` if the driver is not started. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_LogObjectDriver_refresh(UA_Driver *driver);

/**
 * Settings
 * ~~~~~~~~
 * Clients can write the MinimumSeverity Property of a LogObject (Part 26,
 * 5.2). The value must be in 1..1000 (``Bad_OutOfRange`` otherwise). The
 * change does not affect the records that are already stored. While the
 * driver runs, the Property is a callback value source of the driver. When it
 * stops, the Property gets an internal value with the current setting: a
 * value source or notifications that the application had set are not
 * restored.
 *
 * The application reads and changes all settings of a LogObject of the started
 * driver with the functions below. A smaller MaxRecords keeps the newest
 * records. Changing MaxRecords or MaxStorageDuration requires the
 * ``updateLogObject`` callback of the backend (``Bad_NotSupported`` without
 * it). The Properties are updated. Zero is not a valid value of the
 * MaxStorageDuration Property (Part 26, 5.2): a LogObject whose Property has
 * a value keeps a time limit (``Bad_InvalidArgument`` for zero). The settings of the ServerLog are taken
 * from the parameters again when the driver restarts, those of the other
 * LogObjects from their Properties. */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_LogObjectDriver_getSettings(UA_Driver *driver, const UA_NodeId logObjectId,
                               UA_LogObjectSettings *settings);

UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_LogObjectDriver_setSettings(UA_Driver *driver, const UA_NodeId logObjectId,
                               const UA_LogObjectSettings *settings);

/**
 * Application Records
 * ~~~~~~~~~~~~~~~~~~~
 * Add a record to a LogObject of the started driver. The record is copied by
 * the storage backend, the caller keeps the ownership. The Severity must be in
 * 1..1000. A Time of zero is replaced by the current time. Records with a
 * Severity below the MinimumSeverity of the LogObject are dropped. Every
 * record is also added to the ServerLog (Part 26, 7.2). */
UA_EXPORT UA_THREADSAFE UA_StatusCode
UA_LogObjectDriver_addRecord(UA_Driver *driver, const UA_NodeId logObjectId,
                             const UA_LogRecord *record);

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */

_UA_END_DECLS

#endif /* UA_DRIVER_LOGOBJECT_H_ */
