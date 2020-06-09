/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/plugin/log_syslog.h>
#include <open62541/types.h>

#if defined(__linux__) || defined(__unix__)

#include <syslog.h>

const char *syslogLevelNames[6] = {"trace", "debug", "info",
                                   "warn", "error", "fatal"};
const char *syslogCategoryNames[7] = {"network", "channel", "session", "server",
                                      "client", "userland", "securitypolicy"};

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

#define LOGBUFSIZE 512
    char logbuf[LOGBUFSIZE];
    int pos = snprintf(logbuf, LOGBUFSIZE, "[%s/%s] ",
                       syslogLevelNames[level], syslogCategoryNames[category]);
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
UA_Log_Syslog_clear(void *logContext) {
    /* closelog is optional. We don't use it as several loggers might be
     * instantiated in parallel. */
    /* closelog(); */
}

UA_Logger
UA_Log_Syslog(void) {
    return UA_Log_Syslog_withLevel(UA_LOGLEVEL_TRACE);
}

UA_Logger
UA_Log_Syslog_withLevel(UA_LogLevel minlevel) {
    UA_Logger logger = {UA_Log_Syslog_log, (void*)minlevel, UA_Log_Syslog_clear};
    return logger;
}

#endif
