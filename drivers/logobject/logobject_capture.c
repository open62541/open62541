/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "logobject_internal.h"

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

#include <open62541/plugin/log_stdout.h>

/* Capture of the server logger into the ServerLog (Part 26, 7.2). The UA_Logger
 * of the server config is rewritten in place. Every component of the server
 * holds a pointer to the same structure, so the EventLoop, the
 * SecurityPolicies, PubSub etc. are captured as well. */

static const char *
logCategoryNames[UA_LOGCATEGORIES] =
    {"Network", "SecureChannel", "Session", "Server", "Client",
     "Application", "Security", "EventLoop", "PubSub", "Discovery"};

/* Map the level of the server logger to the Severity ranges of Part 26,
 * Table 9 */
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

/* The log callback of the wrapped logger. It runs on the thread of the
 * caller, possibly with the server lock held by that thread. It must not take
 * the server lock and must not log itself. */
static void
captureLog(void *context, UA_LogLevel level, UA_LogCategory category,
           const char *msg, va_list args) {
    LogObjectDriver *lod = (LogObjectDriver*)context;
    logObjectAtomicAdd(&lod->captureInFlight, 1);

    /* Forward to the original logger first */
    if(lod->originalLogger.log) {
        va_list args2;
        va_copy(args2, args);
        lod->originalLogger.log(lod->originalLogger.context, level, category, msg, args2);
        va_end(args2);
    }

    UA_UInt16 severity = logLevelToSeverity(level);
    if(!UA_atomic_load(&lod->capturing) ||
       severity < (UA_UInt16)UA_atomic_load(&lod->serverLogMinimumSeverity))
        goto done;

    /* Render the message into a stack buffer. The format string takes the
     * additional specifiers of UA_String_format. Truncated messages are
     * stored as far as they fit, without the terminating zero. */
    char buf[UA_LOGOBJECT_MAXMESSAGELENGTH];
    UA_String text = {UA_LOGOBJECT_MAXMESSAGELENGTH, (UA_Byte*)buf};
    UA_StatusCode res = UA_String_vformat(&text, msg, args);
    if(res == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
        text.length = UA_LOGOBJECT_MAXMESSAGELENGTH - 1;
    else if(res != UA_STATUSCODE_GOOD)
        goto done;

    /* The log category is the SourceName */
    size_t categoryIndex = (size_t)category;
    if(categoryIndex >= UA_LOGCATEGORIES)
        categoryIndex = UA_LOGCATEGORY_APPLICATION;
    UA_String sourceName = UA_STRING((char*)(uintptr_t)logCategoryNames[categoryIndex]);

    UA_LogRecord record;
    UA_LogRecord_init(&record);
    record.time = logObjectNow(lod);
    record.severity = severity;
    record.sourceName = &sourceName;
    record.message.text = text;

    UA_Boolean overflow = false;
    lod->backend.addRecord(lod->driver.server, lod->backend.context,
                           &lod->serverLog.nodeId, &record, record.time, &overflow);
    if(overflow)
        scheduleOverflowEvent(lod, &lod->serverLog);

 done:
    logObjectAtomicAdd(&lod->captureInFlight, (uintptr_t)-1);
}

/* Several servers can share one UA_Logger. Every driver then wraps the capture
 * of the driver started before it: the logger forwards to the last driver,
 * whose originalLogger forwards to the previous driver and so on. The driver
 * whose originalLogger is the capture of another driver is that driver's
 * captureOuter. Returns the driver of a capture, NULL for other loggers.
 * captureLog is only installed here, so its context is always a driver. */
static LogObjectDriver *
capturingDriver(const UA_Logger *logger) {
    return (logger->log == captureLog) ? (LogObjectDriver*)logger->context : NULL;
}

void
installLogCapture(LogObjectDriver *lod) {
    UA_Logger *logger = UA_Server_getConfig(lod->driver.server)->logging;
    if(!logger || !logger->log || lod->capturedLogger)
        return;
    /* The constant stdout logger cannot be rewritten */
    if((const UA_Logger*)logger == UA_Log_Stdout) {
        UA_LOG_WARNING(logger, UA_LOGCATEGORY_SERVER,
                       "LogObject: The constant UA_Log_Stdout logger is not "
                       "captured into the ServerLog");
        return;
    }
    lod->originalLogger = *logger;
    lod->captureOuter = NULL;
    LogObjectDriver *inner = capturingDriver(logger);
    if(inner)
        inner->captureOuter = lod;
    lod->capturedLogger = logger;
    UA_atomic_store(&lod->serverLogMinimumSeverity,
                    (uintptr_t)lod->serverLog.settings.minimumSeverity);
    UA_atomic_store(&lod->capturing, (uintptr_t)1);
    /* The context first: a concurrent caller then never combines captureLog
     * with a foreign context */
    logger->context = lod;
    logger->log = captureLog;
}

void
removeLogCapture(LogObjectDriver *lod) {
    if(!lod->capturedLogger)
        return;
    UA_atomic_store(&lod->capturing, (uintptr_t)0);
    LogObjectDriver *inner = capturingDriver(&lod->originalLogger);
    LogObjectDriver *outer = lod->captureOuter;
    if(outer) {
        outer->originalLogger = lod->originalLogger; /* Splice out */
    } else {
        lod->capturedLogger->log = lod->originalLogger.log; /* Last in the chain */
        lod->capturedLogger->context = lod->originalLogger.context;
    }
    if(inner)
        inner->captureOuter = outer;
    lod->captureOuter = NULL;
    lod->capturedLogger = NULL;

    /* Log calls of other threads may still run in the capture */
    while(UA_atomic_load(&lod->captureInFlight) > 0) {}
}

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */
