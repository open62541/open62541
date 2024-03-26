/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/plugin/log_syslog.h>
#include <open62541/types.h>

#if defined(__linux__) || defined(__unix__)

#include <syslog.h>
#include <stdio.h>

const char *syslogLevelNames[6] = {"trace", "debug", "info",
                                   "warn", "error", "fatal"};
const char *syslogCategoryNames[UA_LOGCATEGORIES] =
    {"network", "channel", "session", "server", "client",
     "userland", "securitypolicy", "eventloop", "pubsub", "discovery"};

#ifdef __clang__
__attribute__((__format__(__printf__, 4 , 0)))
#endif
static void
UA_Log_Syslog_log(void *context, UA_LogLevel level, UA_LogCategory category,
                  const char *msg, va_list args) {
    /* Assume that context is casted to UA_LogLevel */
    if(context != NULL && (UA_LogLevel)(uintptr_t)context > level)
        return;

    int priority = LOG_INFO;
    switch(level) {
    case UA_LOGLEVEL_DEBUG:
        priority = LOG_DEBUG;
        break;
    case UA_LOGLEVEL_INFO:
        priority = LOG_INFO;
        break;
    case UA_LOGLEVEL_WARNING:
        priority = LOG_WARNING;
        break;
    case UA_LOGLEVEL_ERROR:
        priority = LOG_ERR;
        break;
    case UA_LOGLEVEL_FATAL:
        priority = LOG_CRIT;
        break;
    case UA_LOGLEVEL_TRACE:
    default:
        return;
    }

    int logLevelSlot = ((int)level / 100) - 1;
    if(logLevelSlot < 0 || logLevelSlot > 5)
        logLevelSlot = 5; /* Set to fatal if the level is outside the range */

#define LOGBUFSIZE 512
    char logbuf[LOGBUFSIZE];
    int pos = snprintf(logbuf, LOGBUFSIZE, "[%s/%s] ",
                       syslogLevelNames[logLevelSlot],
                       syslogCategoryNames[category]);
    if(pos < 0) {
        syslog(LOG_WARNING, "Log message too long for syslog");
        return;
    }
    pos = vsnprintf(&logbuf[pos], LOGBUFSIZE - (size_t)pos, msg, args);
    if(pos < 0) {
        syslog(LOG_WARNING, "Log message too long for syslog");
        return;
    }

    syslog(priority, "%s", logbuf);
}

static void
UA_Log_Syslog_clear(UA_Logger *logger) {
    /* closelog is optional. We don't use it as several loggers might be
     * instantiated in parallel. */
    /* closelog(); */
    UA_free(logger);
}

UA_Logger
UA_Log_Syslog(void) {
    return UA_Log_Syslog_withLevel(UA_LOGLEVEL_TRACE);
}

UA_Logger
UA_Log_Syslog_withLevel(UA_LogLevel minlevel) {
    UA_Logger logger = {UA_Log_Syslog_log, (void*)(uintptr_t)minlevel, NULL};
    return logger;
}

UA_Logger *
UA_Log_Syslog_new(UA_LogLevel minlevel) {
    UA_Logger *logger = (UA_Logger*)UA_malloc(sizeof(UA_Logger));
    if(!logger)
        return NULL;
    *logger = UA_Log_Syslog_withLevel(minlevel);
    logger->clear = UA_Log_Syslog_clear;
    return logger;
}

#endif
