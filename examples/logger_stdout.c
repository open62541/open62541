/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <stdio.h>
#include <stdarg.h>
#include "logger_stdout.h"

static void print_time(void) {
	UA_DateTime now = UA_DateTime_now();
	UA_ByteString str;
	UA_DateTime_toString(now, &str);
	printf("%.27s", str.data); //a bit hacky way not to display nanoseconds
	UA_ByteString_deleteMembers(&str);
}

const char *LogLevelNames[6] = {"trace", "debug", "info", "warning", "error", "fatal"};
const char *LogCategoryNames[4] = {"communication", "server", "client", "userland"};

#if ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4 || defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
static void Logger_Stdout(UA_LogLevel level, UA_LogCategory category, const char *msg, ...) {
    printf("[");
    print_time();
    va_list ap;
    va_start(ap, msg);
    printf("] %s/%s\t", LogLevelNames[level], LogCategoryNames[category]);
    vprintf(msg, ap);
    printf("\n");
    va_end(ap);
}
#if ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4 || defined(__clang__))
#pragma GCC diagnostic pop
#endif

UA_Logger Logger_Stdout_new(void) {
	return Logger_Stdout;
}
