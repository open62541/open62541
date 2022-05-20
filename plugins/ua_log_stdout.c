/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/types.h>

#include <stdio.h>

/* ANSI escape sequences for color output taken from here:
 * https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c*/

#ifdef UA_ENABLE_LOG_COLORS
# define ANSI_COLOR_RED     "\x1b[31m"
# define ANSI_COLOR_GREEN   "\x1b[32m"
# define ANSI_COLOR_YELLOW  "\x1b[33m"
# define ANSI_COLOR_BLUE    "\x1b[34m"
# define ANSI_COLOR_MAGENTA "\x1b[35m"
# define ANSI_COLOR_CYAN    "\x1b[36m"
# define ANSI_COLOR_RESET   "\x1b[0m"
#else
# define ANSI_COLOR_RED     ""
# define ANSI_COLOR_GREEN   ""
# define ANSI_COLOR_YELLOW  ""
# define ANSI_COLOR_BLUE    ""
# define ANSI_COLOR_MAGENTA ""
# define ANSI_COLOR_CYAN    ""
# define ANSI_COLOR_RESET   ""
#endif

const char *logLevelNames[6] = {"trace", "debug",
                                ANSI_COLOR_GREEN "info",
                                ANSI_COLOR_YELLOW "warn",
                                ANSI_COLOR_RED "error",
                                ANSI_COLOR_MAGENTA "fatal"};
const char *logCategoryNames[UA_LOGCATEGORIES] =
    {"network", "channel", "session", "server",
     "client", "userland", "securitypolicy", "eventloop"};

typedef struct {
    UA_LogLevel minlevel;
#if UA_MULTITHREADING >= 100
    UA_Lock lock;
#endif
} LogContext;

#ifdef __clang__
__attribute__((__format__(__printf__, 4 , 0)))
#endif
static void
UA_Log_Stdout_log(void *context, UA_LogLevel level, UA_LogCategory category,
                  const char *msg, va_list args) {
    LogContext *logContext = (LogContext*)context;
    if(logContext) {
        if(logContext->minlevel > level)
            return;
        UA_LOCK(&logContext->lock);
    }

    UA_Int64 tOffset = UA_DateTime_localTimeUtcOffset();
    UA_DateTimeStruct dts = UA_DateTime_toStruct(UA_DateTime_now() + tOffset);

    printf("[%04u-%02u-%02u %02u:%02u:%02u.%03u (UTC%+05d)] %s/%s" ANSI_COLOR_RESET "\t",
           dts.year, dts.month, dts.day, dts.hour, dts.min, dts.sec, dts.milliSec,
           (int)(tOffset / UA_DATETIME_SEC / 36), logLevelNames[level],
           logCategoryNames[category]);
    vprintf(msg, args);
    printf("\n");
    fflush(stdout);

    if(logContext) {
        UA_UNLOCK(&logContext->lock);
    }
}

static void
UA_Log_Stdout_clear(void *context) {
    if(!context)
        return;
    UA_LOCK_DESTROY(&((LogContext*)context)->lock);
    UA_free(context);
}

const UA_Logger UA_Log_Stdout_ = {UA_Log_Stdout_log, NULL, UA_Log_Stdout_clear};
const UA_Logger *UA_Log_Stdout = &UA_Log_Stdout_;

UA_Logger
UA_Log_Stdout_withLevel(UA_LogLevel minlevel) {
    LogContext *context = (LogContext*)UA_calloc(1, sizeof(LogContext));
    if(context) {
        UA_LOCK_INIT(&context->lock);
        context->minlevel = minlevel;
    }

    UA_Logger logger = {UA_Log_Stdout_log, (void*)context, UA_Log_Stdout_clear};
    return logger;
}
