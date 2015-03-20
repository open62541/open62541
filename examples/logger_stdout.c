/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <stdio.h>

#include "logger_stdout.h"
#include "ua_types.h"

static void print_time(void) {
	UA_DateTime now = UA_DateTime_now();
	UA_ByteString str;
	UA_DateTime_toString(now, &str);
	printf("%.27s", str.data); //a bit hacky way not to display nanoseconds
	UA_ByteString_deleteMembers(&str);
}

#define LOG_FUNCTION(LEVEL) \
	static void log_##LEVEL(UA_LoggerCategory category, const char *msg) { \
        printf("[");                                                    \
		print_time();                                                   \
        printf("] " #LEVEL "/%s\t%s\n", UA_LoggerCategoryNames[category], msg); \
	}

LOG_FUNCTION(trace)
LOG_FUNCTION(debug)
LOG_FUNCTION(info)
LOG_FUNCTION(warning)
LOG_FUNCTION(error)
LOG_FUNCTION(fatal)

UA_Logger Logger_Stdout_new(void) {
	return (UA_Logger){
		.log_trace = log_trace,
		.log_debug = log_debug,
		.log_info = log_info,
		.log_warning = log_warning,
		.log_error = log_error,
		.log_fatal = log_fatal
	};
}
