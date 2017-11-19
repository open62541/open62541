/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include <stdarg.h>
#include "ua_log_stdout.h"
#include "ua_types_generated.h"
#include "ua_types_generated_handling.h"

#ifdef UA_ENABLE_MULTITHREADING
#include <pthread.h>
static pthread_mutex_t printf_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

const char *logLevelNames[6] = {"trace", "debug", "info", "warn", "error", "fatal"};
const char *logCategoryNames[7] = {"network", "channel", "session", "server",
                                   "client", "userland", "securitypolicy"};

#ifdef __clang__
__attribute__((__format__(__printf__, 3 , 0)))
#endif
void
UA_Log_Stdout(UA_LogLevel level, UA_LogCategory category,
              const char *msg, va_list args) {
    UA_String t = UA_DateTime_toString(UA_DateTime_now());
#ifdef UA_ENABLE_MULTITHREADING
    pthread_mutex_lock(&printf_mutex);
#endif
    printf("[%.23s] %s/%s\t", t.data, logLevelNames[level], logCategoryNames[category]);
    vprintf(msg, args);
    printf("\n");
    fflush(stdout);
#ifdef UA_ENABLE_MULTITHREADING
    pthread_mutex_unlock(&printf_mutex);
#endif
    UA_ByteString_deleteMembers(&t);
}
