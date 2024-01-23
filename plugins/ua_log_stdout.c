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

static
const char *logLevelNames[6] = {"trace", "debug",
                                ANSI_COLOR_GREEN "info",
                                ANSI_COLOR_YELLOW "warn",
                                ANSI_COLOR_RED "error",
                                ANSI_COLOR_MAGENTA "fatal"};
static
const char *logCategoryNames[7] = {"network", "channel", "session", "server",
                                   "client", "userland", "securitypolicy"};

typedef struct {
    UA_LogLevel minlevel;
#if UA_MULTITHREADING >= 100
    UA_Lock lock;
#endif
} LogContext;

#ifdef __clang__
__attribute__((__format__(__printf__, 5 , 0)))
#endif
void UA_Log_Stdout_log(void *context, UA_LogLevel level, UA_LogCategory category, unsigned int line, 
                  const char *msg, va_list args) 
{
	extern void dk_trace_v(PNIO_UINT32 ModId, PNIO_UINT32 Line, PNIO_UINT32 subsys, PNIO_UINT32 level, const PNIO_INT8 * fmt, va_list argptr);

	static const BYTE dk_levels[] = { LSA_TRACE_LEVEL_CHAT,  /* UA_LOGLEVEL_TRACE */
			LSA_TRACE_LEVEL_NOTE,               /* UA_LOGLEVEL_DEBUG */
			LSA_TRACE_LEVEL_NOTE_HIGH,          /* UA_LOGLEVEL_INFO */
			LSA_TRACE_LEVEL_WARN,               /* UA_LOGLEVEL_WARNING */
			LSA_TRACE_LEVEL_ERROR,              /* UA_LOGLEVEL_ERROR */
			LSA_TRACE_LEVEL_FATAL};             /* UA_LOGLEVEL_FATAL */
	
	static const BYTE dk_categories[] = { TRACE_SUBSYS_UA_NETWORK,  /* UA_LOGCATEGORY_NETWORK */
			TRACE_SUBSYS_UA_SECURECHANNEL,             /* UA_LOGCATEGORY_SECURECHANNEL */
			TRACE_SUBSYS_UA_SESSION,                   /* UA_LOGCATEGORY_SESSION */
			TRACE_SUBSYS_UA_SERVER,                    /* UA_LOGCATEGORY_SERVER */
			TRACE_SUBSYS_UA_CLIENT,                    /* UA_LOGCATEGORY_CLIENT */
			TRACE_SUBSYS_UA_USERLAND,                  /* UA_LOGCATEGORY_USERLAND */
			TRACE_SUBSYS_UA_SECURITYPOLICY};           /* UA_LOGCATEGORY_SECURITYPOLICY */

	LSA_TRACE_V(dk_categories[category], dk_levels[level], line, msg, args);
}

void UA_Log_Stdout_clear(void *context) 
{
	if(!context)
        return;
#if UA_MULTITHREADING >= 100
    UA_LOCK_DESTROY(&((LogContext*)context)->lock);
#endif
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