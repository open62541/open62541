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
        (void)overflow;
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

    /* The log level is kept in the AdditionalData */
    UA_Int32 levelValue = (UA_Int32)level;
    UA_NameValuePair levelPair;
    UA_NameValuePair_init(&levelPair);
    levelPair.name = UA_STRING("LogLevel");
    UA_Variant_setScalar(&levelPair.value, &levelValue, &UA_TYPES[UA_TYPES_INT32]);

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
    record.additionalDataSize = 1;
    record.additionalData = &levelPair;

    UA_LogObjectBackend *backend = &server->config.logObjectBackend;
    UA_Boolean overflow = false;
    backend->addRecord(server, backend->context, &entry->nodeId,
                       &record, record.time, &overflow);
    (void)overflow;
}

/* The config is cleared without the server having restored the original
 * logger before. Restore it now and delegate. */
static void
captureLogClear(UA_Logger *logger) {
    UA_Server *server = (UA_Server*)logger->context;
    UA_Logger original = server->originalLogger;
    *logger = original;
    server->logObjectLoggerInstalled = false;
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
    logger->log = captureLog;
    logger->context = server;
    logger->clear = captureLogClear;
    server->logObjectLoggerInstalled = true;
}

static void
removeLogCapture(UA_Server *server) {
    if(!server->logObjectLoggerInstalled)
        return;
    *server->config.logging = server->originalLogger;
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

    /* Sanitize the configuration */
    if(config->serverLog.maxRecords == 0)
        config->serverLog.maxRecords = 1000;
    if(config->serverLog.minimumSeverity < 1)
        config->serverLog.minimumSeverity = 1;
    if(config->serverLog.minimumSeverity > 1000)
        config->serverLog.minimumSeverity = 1000;
    if(config->serverLog.maxStorageDuration < 0.0)
        config->serverLog.maxStorageDuration = 0.0;

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
