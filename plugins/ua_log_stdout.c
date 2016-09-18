/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include <stdarg.h>
#include "ua_log_stdout.h"
#include "ua_types_generated.h"
#include "ua_types_generated_handling.h"

const char *LogLevelNames[6] = {"trace", "debug", "info", "warning", "error", "fatal"};
const char *LogCategoryNames[6] = {"network", "channel", "session", "server", "client", "userland"};

#if (defined(__GNUC__) && defined(__GNUC_MINOR__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 6) || defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

void UA_Log_Stdout(UA_LogLevel level, UA_LogCategory category, const char *msg, ...) {
    UA_String t = UA_DateTime_toString(UA_DateTime_now());
    printf("[%.23s] %s/%s\t", t.data, LogLevelNames[level], LogCategoryNames[category]);
    UA_ByteString_deleteMembers(&t);
    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
    printf("\n");
}

#if (defined(__GNUC__) && defined(__GNUC_MINOR__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 6) || defined(__clang__)
# pragma GCC diagnostic pop
#endif
