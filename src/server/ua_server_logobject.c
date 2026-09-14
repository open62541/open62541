/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_LOGOBJECT

/* OPC UA Part 26 LogObjects. The server core keeps the registry of the
 * LogObjects and captures the server logger into the ServerLog. The records
 * are stored by the config.logObjectBackend plugin. */

static const char *
logCategoryNames[UA_LOGCATEGORIES] =
    {"Network", "SecureChannel", "Session", "Server", "Client",
     "Application", "Security", "EventLoop", "PubSub", "Discovery"};

UA_UInt16
logLevelToSeverity(UA_LogLevel level) {
    switch(level) {
    case UA_LOGLEVEL_TRACE:   return 10;  /* Debug 1-50 */
    case UA_LOGLEVEL_DEBUG:   return 40;
    case UA_LOGLEVEL_INFO:    return 80;  /* Information 51-100 */
    case UA_LOGLEVEL_WARNING: return 180; /* Warning 151-200 */
    case UA_LOGLEVEL_ERROR:   return 230; /* Error 201-250 */
    case UA_LOGLEVEL_FATAL:   return 500; /* Emergency 401-1000 */
    default:                  return 80;
    }
}

static UA_DateTime
logObjectNow(UA_Server *server) {
    UA_EventLoop *el = server->config.eventLoop;
    if(el && el->dateTime_now)
        return el->dateTime_now(el);
    return UA_DateTime_now();
}

/******************/
/* Overflow Event */
/******************/

/* Emit a LogOverflowEventType Event for the LogObject (Part 26, 6.4). The
 * server lock must be held. */
static void
emitOverflowEvent(UA_Server *server, UA_LogObjectEntry *entry,
                  uintptr_t discarded) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    UA_String sourceName = UA_STRING("LogObject/Overflow");
    UA_KeyValuePair field;
    field.key = UA_QUALIFIEDNAME(0, "/SourceName");
    UA_Variant_setScalar(&field.value, &sourceName, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap fields = {1, &field};

    /* The LogOverflowEventType has no field for the number of discarded
     * records. It is reported in the Message. */
    char buf[96];
    UA_String text = {sizeof(buf), (UA_Byte*)buf};
    if(UA_String_format(&text, "LogObject discarded %lu records (MaxRecords reached)",
                        (unsigned long)discarded) != UA_STATUSCODE_GOOD)
        text = UA_STRING("LogObject discarded records (MaxRecords reached)");

    UA_EventDescription ed;
    memset(&ed, 0, sizeof(UA_EventDescription));
    ed.sourceNode = entry->nodeId;
    ed.eventType = UA_NS0ID(LOGOVERFLOWEVENTTYPE);
    ed.severity = 500;
    ed.message.text = text;
    ed.eventFields = &fields;
    /* A failure is not logged: the message would be captured into the
     * ServerLog and could trigger the next overflow */
    (void)createEvent(server, &ed, NULL);
#else
    (void)server;
    (void)entry;
    (void)discarded;
#endif
}

static void overflowTimerCallback(void *application, void *context);

/* Emit the pending overflow Events. A LogObject emits at most one Event per
 * logOverflowEventInterval. A ring buffer at its limit evicts a record for
 * every new one, so without the limit every cycle that logs would emit an
 * Event. And the emission itself logs, which would overflow again in the next
 * cycle. The overflows within the interval are reported with the next Event
 * when the one-shot timer fires. Runs in the EventLoop, the server lock must be
 * held. */
static void
processOverflowEvents(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_EventLoop *el = server->config.eventLoop;
    if(!el || server->state != UA_LIFECYCLESTATE_STARTED)
        return;

    UA_DateTime now = el->dateTime_nowMonotonic(el);
    UA_DateTime interval = (UA_DateTime)
        (server->config.logOverflowEventInterval * UA_DATETIME_MSEC);
    UA_DateTime nextDue = UA_INT64_MAX;
    UA_LogObjectEntry *entry;
    LIST_FOREACH(entry, &server->logObjects, pointers) {
        if(UA_atomic_load(&entry->discarded) == 0)
            continue;
        if(entry->overflowReported && now - entry->lastOverflowEvent < interval) {
            UA_DateTime due = entry->lastOverflowEvent + interval;
            if(due < nextDue)
                nextDue = due;
            continue;
        }
        uintptr_t discarded = 0;
        UA_atomic_xchg(&entry->discarded, (uintptr_t)0, &discarded);
        entry->overflowReported = true;
        entry->lastOverflowEvent = now;
        emitOverflowEvent(server, entry, discarded);
    }

    /* Wake up when the next coalesced Event is due */
    if(nextDue == UA_INT64_MAX || UA_atomic_load(&server->overflowTimerArmed))
        return;
    UA_Double delay = (UA_Double)(nextDue - now) / UA_DATETIME_MSEC;
    UA_StatusCode res =
        el->addTimer(el, overflowTimerCallback, server, NULL, delay, NULL,
                     UA_TIMERPOLICY_ONCE, &server->overflowTimerId);
    if(res == UA_STATUSCODE_GOOD)
        UA_atomic_store(&server->overflowTimerArmed, (uintptr_t)1);
}

/* Runs in the EventLoop. Emits the Events of the LogObjects that overflowed
 * since the last cycle. */
static void
overflowDelayedCallback(void *application, void *context) {
    UA_Server *server = (UA_Server*)application;
    lockServer(server);
    UA_atomic_store(&server->overflowCallbackQueued, (uintptr_t)0);
    processOverflowEvents(server);
    unlockServer(server);
}

/* The one-shot timer is removed by the EventLoop after it fired */
static void
overflowTimerCallback(void *application, void *context) {
    UA_Server *server = (UA_Server*)application;
    lockServer(server);
    UA_atomic_store(&server->overflowTimerArmed, (uintptr_t)0);
    processOverflowEvents(server);
    unlockServer(server);
}

/* Called from any thread. Counts the discarded record and arms the delayed
 * callback once. The callback structure is embedded in the server, so nothing
 * is leaked if the EventLoop never runs it. While the timer is armed, the
 * timer picks up the count. The count is incremented before the timer flag is
 * read and the callbacks clear the flag before they read the counts. So no
 * overflow is lost. */
static void
scheduleOverflowEvent(UA_Server *server, UA_LogObjectEntry *entry) {
    uintptr_t count = UA_atomic_load(&entry->discarded);
    for(;;) {
        uintptr_t expected = count;
        UA_atomic_cmpxchg(&entry->discarded, &expected, count + 1);
        if(expected == count)
            break;
        count = expected;
    }
    if(UA_atomic_load(&server->overflowTimerArmed))
        return; /* The timer emits the Event */
    uintptr_t queued = 0;
    UA_atomic_cmpxchg(&server->overflowCallbackQueued, &queued, (uintptr_t)1);
    if(queued != 0)
        return; /* Already armed */
    UA_EventLoop *el = server->config.eventLoop;
    if(!el) {
        UA_atomic_store(&server->overflowCallbackQueued, (uintptr_t)0);
        return;
    }
    el->addDelayedCallback(el, &server->overflowCallback);
}

UA_LogObjectEntry *
getLogObjectEntry(UA_Server *server, const UA_NodeId *logObjectId) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LogObjectEntry *entry;
    LIST_FOREACH(entry, &server->logObjects, pointers) {
        if(UA_NodeId_equal(&entry->nodeId, logObjectId))
            return entry;
    }
    return NULL;
}

UA_StatusCode
addLogRecord(UA_Server *server, UA_LogObjectEntry *entry,
             const UA_LogRecord *record) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LogObjectBackend *backend = &server->config.logObjectBackend;
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* The MinimumSeverity is checked when the record is generated (Part 26,
     * 5.2). Records below the limit are dropped silently. */
    if(record->severity >= entry->settings.minimumSeverity) {
        UA_Boolean overflow = false;
        res = backend->addRecord(server, backend->context, &entry->nodeId,
                                 record, record->time, &overflow);
        if(overflow)
            scheduleOverflowEvent(server, entry);
    }

    /* The ServerLog contains the records of all LogObjects (Part 26, 7.2) */
    if(res == UA_STATUSCODE_GOOD && !entry->isServerLog && server->serverLog)
        res = addLogRecord(server, server->serverLog, record);
    return res;
}

UA_StatusCode
UA_Server_addLogRecord(UA_Server *server, const UA_NodeId logObjectId,
                       const UA_LogRecord *record) {
    if(!server || !record)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(record->severity < 1 || record->severity > 1000)
        return UA_STATUSCODE_BADOUTOFRANGE;

    lockServer(server);
    if(!server->config.logObjectsEnabled || !server->serverLog) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }
    UA_LogObjectEntry *entry = getLogObjectEntry(server, &logObjectId);
    if(!entry) {
        unlockServer(server);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Shallow copy to fill in the time. The backend deep-copies the record. */
    UA_LogRecord r = *record;
    if(r.time == 0)
        r.time = logObjectNow(server);
    UA_StatusCode res = addLogRecord(server, entry, &r);
    unlockServer(server);
    return res;
}

/***********************/
/* Continuation points */
/***********************/

#define UA_LOGRECORDMASK_ALL                                            \
    (UA_LOGRECORDMASK_EVENTTYPE | UA_LOGRECORDMASK_SOURCENODE |         \
     UA_LOGRECORDMASK_SOURCENAME | UA_LOGRECORDMASK_TRACECONTEXT |      \
     UA_LOGRECORDMASK_ADDITIONALDATA)

static void
LogObjectContinuationPoint_delete(UA_LogObjectContinuationPoint *cp) {
    UA_NodeId_clear(&cp->logObjectId);
    UA_free(cp);
}

void
UA_LogObjectCPQueue_clear(UA_LogObjectCPQueue *queue) {
    UA_LogObjectContinuationPoint *cp, *tmp;
    TAILQ_FOREACH_SAFE(cp, queue, pointers, tmp) {
        TAILQ_REMOVE(queue, cp, pointers);
        LogObjectContinuationPoint_delete(cp);
    }
}

static UA_LogObjectContinuationPoint *
findContinuationPoint(UA_Session *session, const UA_ByteString *identifier) {
    if(identifier->length != sizeof(UA_Guid))
        return NULL;
    UA_LogObjectContinuationPoint *cp;
    TAILQ_FOREACH(cp, &session->logObjectCPs, pointers) {
        if(memcmp(identifier->data, &cp->identifier, sizeof(UA_Guid)) == 0)
            return cp;
    }
    return NULL;
}

static void
removeContinuationPoint(UA_Session *session, UA_LogObjectContinuationPoint *cp) {
    TAILQ_REMOVE(&session->logObjectCPs, cp, pointers);
    session->logObjectCPsSize--;
    LogObjectContinuationPoint_delete(cp);
}

/* Remove the optional fields that were not requested (Part 26, 5.8) */
static void
applyRequestMask(UA_LogRecord *r, UA_LogRecordMask mask) {
    if(!(mask & UA_LOGRECORDMASK_EVENTTYPE) && r->eventType) {
        UA_NodeId_delete(r->eventType);
        r->eventType = NULL;
    }
    if(!(mask & UA_LOGRECORDMASK_SOURCENODE) && r->sourceNode) {
        UA_NodeId_delete(r->sourceNode);
        r->sourceNode = NULL;
    }
    if(!(mask & UA_LOGRECORDMASK_SOURCENAME) && r->sourceName) {
        UA_String_delete(r->sourceName);
        r->sourceName = NULL;
    }
    if(!(mask & UA_LOGRECORDMASK_TRACECONTEXT) && r->traceContext) {
        UA_TraceContextDataType_delete(r->traceContext);
        r->traceContext = NULL;
    }
    if(!(mask & UA_LOGRECORDMASK_ADDITIONALDATA) && r->additionalData) {
        UA_Array_delete(r->additionalData, r->additionalDataSize,
                        &UA_TYPES[UA_TYPES_NAMEVALUEPAIR]);
        r->additionalData = NULL;
        r->additionalDataSize = 0;
    }
}

static UA_Boolean
scalarInput(const UA_Variant *v, const UA_DataType *type) {
    return v->data != NULL && UA_Variant_hasScalarType(v, type);
}

UA_StatusCode
logObjectGetRecordsMethod(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output) {
    UA_StatusCode res = checkMethodOutputArguments(outputSize, 2);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(inputSize < 6)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    /* The Call service checked the argument types. The RequestMask arrives as
     * LogRecordMask or UInt32, both are backed by UInt32. */
    if(!scalarInput(&input[0], &UA_TYPES[UA_TYPES_DATETIME]) ||
       !scalarInput(&input[1], &UA_TYPES[UA_TYPES_DATETIME]) ||
       !scalarInput(&input[2], &UA_TYPES[UA_TYPES_UINT32]) ||
       !scalarInput(&input[3], &UA_TYPES[UA_TYPES_UINT16]) ||
       !input[4].data || !UA_Variant_isScalar(&input[4]) ||
       input[4].type->typeKind != UA_DATATYPEKIND_UINT32)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_DateTime startTime = *(UA_DateTime*)input[0].data;
    UA_DateTime endTime = *(UA_DateTime*)input[1].data;
    UA_UInt32 maxReturnRecords = *(UA_UInt32*)input[2].data;
    UA_UInt16 minimumSeverity = *(UA_UInt16*)input[3].data;
    UA_LogRecordMask requestMask = *(UA_UInt32*)input[4].data;
    const UA_ByteString *cpIn = NULL;
    if(input[5].data && UA_Variant_hasScalarType(&input[5], &UA_TYPES[UA_TYPES_BYTESTRING]))
        cpIn = (const UA_ByteString*)input[5].data;
    UA_Boolean continuation = (cpIn && cpIn->length > 0);

    /* Validate the arguments of a new request (Part 26, Table 3). A
     * continuation uses the arguments of the original call. */
    if(!continuation) {
        if(minimumSeverity < 1 || minimumSeverity > 1000)
            return UA_STATUSCODE_BADOUTOFRANGE;
        if(startTime != 0 && endTime != 0 && startTime > endTime)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        if(requestMask & ~(UA_LogRecordMask)UA_LOGRECORDMASK_ALL)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        /* Zero denotes an unbounded time range */
        if(startTime == 0)
            startTime = UA_INT64_MIN;
        if(endTime == 0)
            endTime = UA_INT64_MAX;
    }

    lockServer(server);

    UA_Session *session = getSessionById(server, sessionId);
    if(!session) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_LogObjectEntry *entry = getLogObjectEntry(server, objectId);
    if(!entry) {
        unlockServer(server);
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Continue a previous call */
    UA_LogObjectCursor cursor = 0;
    if(continuation) {
        UA_LogObjectContinuationPoint *cp = findContinuationPoint(session, cpIn);
        if(!cp || !UA_NodeId_equal(&cp->logObjectId, objectId)) {
            unlockServer(server);
            return UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
        }
        startTime = cp->startTime;
        endTime = cp->endTime;
        minimumSeverity = cp->minimumSeverity;
        requestMask = cp->requestMask;
        maxReturnRecords = cp->maxReturnRecords;
        cursor = cp->cursor;
        removeContinuationPoint(session, cp);
    }

    /* The server can impose a limit below MaxReturnRecords (Part 26, 5.3) */
    size_t limit = maxReturnRecords;
    UA_UInt32 cap = server->config.maxLogRecordsPerCall;
    if(cap > 0 && (limit == 0 || limit > cap))
        limit = cap;
    if(limit == 0)
        limit = entry->settings.maxRecords;

    /* Read the records from the backend */
    UA_LogObjectBackend *backend = &server->config.logObjectBackend;
    size_t recordsSize = 0;
    UA_LogRecord *records = NULL;
    UA_LogObjectCursor nextCursor = cursor;
    UA_Boolean moreAvailable = false;
    res = backend->getRecords(server, backend->context, &entry->nodeId,
                              startTime, endTime, minimumSeverity, requestMask,
                              cursor, limit, logObjectNow(server),
                              &recordsSize, &records, &nextCursor, &moreAvailable);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    /* The core is authoritative for the RequestMask */
    for(size_t i = 0; i < recordsSize; i++)
        applyRequestMask(&records[i], requestMask);

    /* Allocate the output arguments */
    UA_LogRecordsDataType *results = UA_LogRecordsDataType_new();
    UA_ByteString *cpOut = UA_ByteString_new();
    if(!results || !cpOut) {
        if(results)
            UA_LogRecordsDataType_delete(results);
        if(cpOut)
            UA_ByteString_delete(cpOut);
        UA_Array_delete(records, recordsSize, &UA_TYPES[UA_TYPES_LOGRECORD]);
        unlockServer(server);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Keep a continuation point for the remaining records. The Session limit
     * is enforced without evicting older points (Part 26, Table 3). */
    if(moreAvailable) {
        UA_UInt16 maxCPs = server->config.maxLogObjectContinuationPoints;
        UA_LogObjectContinuationPoint *cp = NULL;
        if(maxCPs > 0 && session->logObjectCPsSize >= maxCPs) {
            res = UA_STATUSCODE_BADNOCONTINUATIONPOINTS;
        } else {
            cp = (UA_LogObjectContinuationPoint*)
                UA_calloc(1, sizeof(UA_LogObjectContinuationPoint));
            if(!cp)
                res = UA_STATUSCODE_BADOUTOFMEMORY;
        }
        if(res == UA_STATUSCODE_GOOD)
            res = UA_NodeId_copy(objectId, &cp->logObjectId);
        if(res == UA_STATUSCODE_GOOD)
            res = UA_ByteString_allocBuffer(cpOut, sizeof(UA_Guid));
        if(res != UA_STATUSCODE_GOOD) {
            if(cp)
                LogObjectContinuationPoint_delete(cp);
            UA_LogRecordsDataType_delete(results);
            UA_ByteString_delete(cpOut);
            UA_Array_delete(records, recordsSize, &UA_TYPES[UA_TYPES_LOGRECORD]);
            unlockServer(server);
            return res;
        }
        cp->identifier = UA_Guid_random();
        memcpy(cpOut->data, &cp->identifier, sizeof(UA_Guid));
        cp->startTime = startTime;
        cp->endTime = endTime;
        cp->minimumSeverity = minimumSeverity;
        cp->requestMask = requestMask;
        cp->maxReturnRecords = maxReturnRecords;
        cp->cursor = nextCursor;
        TAILQ_INSERT_TAIL(&session->logObjectCPs, cp, pointers);
        session->logObjectCPsSize++;
    }

    /* Set the output arguments. An empty result is an empty array. */
    results->logRecordArraySize = recordsSize;
    results->logRecordArray = records;
    if(!records)
        results->logRecordArray = (UA_LogRecord*)UA_EMPTY_ARRAY_SENTINEL;
    UA_Variant_setScalar(&output[0], results, &UA_TYPES[UA_TYPES_LOGRECORDSDATATYPE]);
    UA_Variant_setScalar(&output[1], cpOut, &UA_TYPES[UA_TYPES_BYTESTRING]);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
logObjectReleaseContinuationPointMethod(UA_Server *server, const UA_NodeId *sessionId,
                                        void *sessionContext, const UA_NodeId *methodId,
                                        void *methodContext, const UA_NodeId *objectId,
                                        void *objectContext, size_t inputSize,
                                        const UA_Variant *input, size_t outputSize,
                                        UA_Variant *output) {
    UA_StatusCode res = checkMethodOutputArguments(outputSize, 0);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(inputSize < 1)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(!scalarInput(&input[0], &UA_TYPES[UA_TYPES_BYTESTRING]))
        return UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
    const UA_ByteString *identifier = (const UA_ByteString*)input[0].data;

    lockServer(server);
    UA_Session *session = getSessionById(server, sessionId);
    if(!session) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_LogObjectContinuationPoint *cp = findContinuationPoint(session, identifier);
    if(!cp || !UA_NodeId_equal(&cp->logObjectId, objectId)) {
        unlockServer(server);
        return UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
    }
    removeContinuationPoint(session, cp);
    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

/**********************/
/* Capture the logger */
/**********************/

/* The log callback of the wrapped logger. It runs on the thread of the
 * caller, possibly with the server lock held by that thread. It must not take
 * the server lock and must not log itself. */
static void
captureLog(void *context, UA_LogLevel level, UA_LogCategory category,
           const char *msg, va_list args) {
    UA_Server *server = (UA_Server*)context;

    /* Forward to the original logger first */
    if(server->originalLogger.log) {
        va_list args2;
        va_copy(args2, args);
        server->originalLogger.log(server->originalLogger.context,
                                   level, category, msg, args2);
        va_end(args2);
    }

    UA_LogObjectEntry *entry = server->serverLog;
    if(!entry)
        return;
    UA_UInt16 severity = logLevelToSeverity(level);
    if(severity < entry->settings.minimumSeverity)
        return;

    /* Render the message into a stack buffer. The format string takes the
     * additional specifiers of UA_String_format. Truncated messages are
     * stored as far as they fit. */
    char buf[UA_LOGOBJECT_MAXMESSAGELENGTH];
    UA_String text = {UA_LOGOBJECT_MAXMESSAGELENGTH, (UA_Byte*)buf};
    UA_StatusCode res = UA_String_vformat(&text, msg, args);
    if(res != UA_STATUSCODE_GOOD && res != UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
        return;

    /* The log category is the SourceName */
    size_t categoryIndex = (size_t)category;
    if(categoryIndex >= UA_LOGCATEGORIES)
        categoryIndex = UA_LOGCATEGORY_APPLICATION;
    UA_String sourceName =
        UA_STRING((char*)(uintptr_t)logCategoryNames[categoryIndex]);

    UA_LogRecord record;
    UA_LogRecord_init(&record);
    record.time = logObjectNow(server);
    record.severity = severity;
    record.sourceName = &sourceName;
    record.message.text = text;

    UA_LogObjectBackend *backend = &server->config.logObjectBackend;
    UA_Boolean overflow = false;
    backend->addRecord(server, backend->context, &entry->nodeId,
                       &record, record.time, &overflow);
    if(overflow)
        scheduleOverflowEvent(server, entry);
}

/* Several servers can share one UA_Logger. Every server then wraps the
 * capture of the server created before it: the logger forwards to the last
 * server, whose originalLogger forwards to the previous server and so on. The
 * server whose originalLogger is the capture of another server is that
 * server's logCaptureOuter. Returns the server wrapped by the given logger, or
 * NULL for a logger that is not a capture. captureLog is only installed here,
 * so its context is always a server. */
static UA_Server *
capturedServer(const UA_Logger *logger) {
    return (logger->log == captureLog) ? (UA_Server*)logger->context : NULL;
}

/* The config is cleared without the server having restored the original
 * logger before. The logger points to the last server of the chain. Restore
 * its original and delegate, which unwinds the remaining chain. */
static void
captureLogClear(UA_Logger *logger) {
    UA_Server *server = (UA_Server*)logger->context;
    UA_Logger original = server->originalLogger;
    *logger = original;
    server->logObjectLoggerInstalled = false;
    UA_Server *inner = capturedServer(&original);
    if(inner)
        inner->logCaptureOuter = NULL;
    if(original.clear)
        original.clear(logger);
}

/* Rewrite the UA_Logger of the config in place. Every component of the server
 * holds a pointer to the same structure, so the EventLoop, the
 * SecurityPolicies, PubSub etc. are captured as well. */
static void
installLogCapture(UA_Server *server) {
    UA_Logger *logger = server->config.logging;
    if(!logger || !logger->log || server->logObjectLoggerInstalled)
        return;
    server->originalLogger = *logger;
    server->logCaptureOuter = NULL;
    UA_Server *inner = capturedServer(logger);
    if(inner)
        inner->logCaptureOuter = server;
    logger->log = captureLog;
    logger->context = server;
    /* A logger without clear is not owned by the config. It is typically
     * shared, e.g. with other servers. Then the config of every server calls
     * clear when it is deleted, which must not unwind the captures of the
     * other servers. */
    if(server->originalLogger.clear)
        logger->clear = captureLogClear;
    server->logObjectLoggerInstalled = true;
}

/* Remove the server from the chain of captures. The servers that share the
 * logger can be deleted in any order. */
static void
removeLogCapture(UA_Server *server) {
    if(!server->logObjectLoggerInstalled)
        return;
    UA_Server *inner = capturedServer(&server->originalLogger);
    UA_Server *outer = server->logCaptureOuter;
    if(outer)
        outer->originalLogger = server->originalLogger; /* Splice out */
    else
        *server->config.logging = server->originalLogger; /* Last in the chain */
    if(inner)
        inner->logCaptureOuter = outer;
    server->logCaptureOuter = NULL;
    server->logObjectLoggerInstalled = false;
}

/************************/
/* Lifecycle of the API */
/************************/

UA_StatusCode
UA_Server_initLogObjects(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_ServerConfig *config = &server->config;
    LIST_INIT(&server->logObjects);
    server->serverLog = NULL;
    memset(&server->overflowCallback, 0, sizeof(UA_DelayedCallback));
    server->overflowCallback.callback = overflowDelayedCallback;
    server->overflowCallback.application = server;
    UA_atomic_store(&server->overflowCallbackQueued, (uintptr_t)0);
    UA_atomic_store(&server->overflowTimerArmed, (uintptr_t)0);
    server->overflowTimerId = 0;

    /* Sanitize the configuration */
    if(config->serverLog.maxRecords == 0)
        config->serverLog.maxRecords = 1000;
    if(config->serverLog.minimumSeverity < 1)
        config->serverLog.minimumSeverity = 1;
    if(config->serverLog.minimumSeverity > 1000)
        config->serverLog.minimumSeverity = 1000;
    if(config->serverLog.maxStorageDuration < 0.0)
        config->serverLog.maxStorageDuration = 0.0;
    if(!(config->logOverflowEventInterval > 0.0))
        config->logOverflowEventInterval = 60000.0;

    if(config->logObjectsEnabled &&
       (!config->logObjectBackend.registerLogObject ||
        !config->logObjectBackend.addRecord ||
        !config->logObjectBackend.getRecords)) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                       "LogObjects: No storage backend configured. "
                       "The LogObjects are disabled.");
        config->logObjectsEnabled = false;
    }

    if(!config->logObjectsEnabled)
        return initNS0LogObject(server);

    /* Create the ServerLog */
    UA_LogObjectEntry *entry = (UA_LogObjectEntry*)
        UA_calloc(1, sizeof(UA_LogObjectEntry));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    entry->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERLOG);
    entry->settings = config->serverLog;
    entry->isServerLog = true;
    UA_StatusCode res = config->logObjectBackend.
        registerLogObject(server, config->logObjectBackend.context,
                          &entry->nodeId, &entry->settings);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(entry);
        return res;
    }
    LIST_INSERT_HEAD(&server->logObjects, entry, pointers);
    server->serverLog = entry;

    res = initNS0LogObject(server);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Capture the server logger. This is done last so that no record is
     * generated before the ServerLog is ready. */
    installLogCapture(server);
    return UA_STATUSCODE_GOOD;
}

void
UA_Server_cleanupLogObjects(UA_Server *server) {
    removeLogCapture(server);

    /* The EventLoop is freed after this point. Disarm the delayed callback. */
    uintptr_t queued = 1;
    UA_atomic_cmpxchg(&server->overflowCallbackQueued, &queued, (uintptr_t)0);
    if(queued == 1 && server->config.eventLoop)
        server->config.eventLoop->removeDelayedCallback(server->config.eventLoop,
                                                         &server->overflowCallback);
    uintptr_t armed = 1;
    UA_atomic_cmpxchg(&server->overflowTimerArmed, &armed, (uintptr_t)0);
    if(armed == 1 && server->config.eventLoop)
        server->config.eventLoop->removeTimer(server->config.eventLoop,
                                              server->overflowTimerId);
    UA_LogObjectBackend *backend = &server->config.logObjectBackend;
    UA_LogObjectEntry *entry, *tmp;
    LIST_FOREACH_SAFE(entry, &server->logObjects, pointers, tmp) {
        LIST_REMOVE(entry, pointers);
        if(backend->unregisterLogObject)
            backend->unregisterLogObject(server, backend->context, &entry->nodeId);
        UA_NodeId_clear(&entry->nodeId);
        UA_free(entry);
    }
    server->serverLog = NULL;
}

#endif /* UA_ENABLE_LOGOBJECT */
