/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Thomas Stalder, Blue Time Concept SA
 */

#include <stdio.h>
#include "ua_log_stdout.h"
#include "ua_types_generated.h"
#include "ua_types_generated_handling.h"

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

#ifdef UA_ENABLE_MULTITHREADING
#include <pthread.h>
static pthread_mutex_t printf_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

const char *logLevelNames[6] = {"trace", "debug",
                                ANSI_COLOR_GREEN "info",
                                ANSI_COLOR_YELLOW "warn",
                                ANSI_COLOR_RED "error",
                                ANSI_COLOR_MAGENTA "fatal"};
const char *logCategoryNames[7] = {"network", "channel", "session", "server",
                                   "client", "userland", "securitypolicy"};

#ifdef __clang__
__attribute__((__format__(__printf__, 3 , 0)))
#endif
void
UA_Log_Stdout(UA_LogLevel level, UA_LogCategory category,
              const char *msg, va_list args) {
    UA_Int64 tOffset = UA_DateTime_localTimeUtcOffset();
    UA_DateTimeStruct dts = UA_DateTime_toStruct(UA_DateTime_now() + tOffset);

#ifdef UA_ENABLE_MULTITHREADING
    pthread_mutex_lock(&printf_mutex);
#endif

    printf("[%04u-%02u-%02u %02u:%02u:%02u.%03u (UTC%+05d)] %s/%s" ANSI_COLOR_RESET "\t",
           dts.year, dts.month, dts.day, dts.hour, dts.min, dts.sec, dts.milliSec,
           (int)(tOffset / UA_DATETIME_SEC / 36), logLevelNames[level], logCategoryNames[category]);
    vprintf(msg, args);
    printf("\n");
    fflush(stdout);

#ifdef UA_ENABLE_MULTITHREADING
    pthread_mutex_unlock(&printf_mutex);
#endif
}
