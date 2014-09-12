/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <stdio.h>
#include <stdarg.h>

#include "logger_stdout.h"
#include "ua_types.h"

void print_time() {
	UA_DateTime now = UA_DateTime_now();
	UA_ByteString str;
	UA_DateTime_toString(now, &str);
	printf("\"%.*s\"}", str.length, str.data);
	UA_ByteString_deleteMembers(&str);
}

#define LOG_FUNCTION(LEVEL) \
	void log_##LEVEL(UA_LoggerCategory category, const char *msg, ...) { \
		va_list args;												   \
		puts("##LEVEL - ");											   \
		print_time();												   \
		puts(" - ");												   \
		va_start(args, msg);										   \
		vprintf(msg, args);											   \
		puts("\n");													   \
		va_end(args);												   \
	}

LOG_FUNCTION(trace)
LOG_FUNCTION(debug)
LOG_FUNCTION(info)
LOG_FUNCTION(warning)
LOG_FUNCTION(error)
LOG_FUNCTION(fatal)

void Logger_Stdout_init(UA_Logger *logger) {
	*logger = (UA_Logger){
		.log_trace = log_trace,
		.log_debug = log_debug,
		.log_info = log_info,
		.log_warning = log_warning,
		.log_error = log_error,
		.log_fatal = log_fatal
	};
}
