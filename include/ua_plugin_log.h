/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this 
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_PLUGIN_LOG_H_
#define UA_PLUGIN_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include "ua_config.h"

/**
 * Logging Plugin API
 * ==================
 *
 * Servers and clients must define a logger in their configuration. The logger
 * is just a function pointer. Every log-message consists of a log-level, a
 * log-category and a string message content. The timestamp of the log-message
 * is created within the logger. */

typedef enum {
    UA_LOGLEVEL_TRACE,
    UA_LOGLEVEL_DEBUG,
    UA_LOGLEVEL_INFO,
    UA_LOGLEVEL_WARNING,
    UA_LOGLEVEL_ERROR,
    UA_LOGLEVEL_FATAL
} UA_LogLevel;

typedef enum {
    UA_LOGCATEGORY_NETWORK,
    UA_LOGCATEGORY_SECURECHANNEL,
    UA_LOGCATEGORY_SESSION,
    UA_LOGCATEGORY_SERVER,
    UA_LOGCATEGORY_CLIENT,
    UA_LOGCATEGORY_USERLAND,
    UA_LOGCATEGORY_SECURITYPOLICY
} UA_LogCategory;

/**
 * The message string and following varargs are formatted according to the rules
 * of the printf command. Do not call the logger directly. Instead, make use of
 * the convenience macros that take the minimum log-level defined in ua_config.h
 * into account. */

typedef void (*UA_Logger)(UA_LogLevel level, UA_LogCategory category,
                          const char *msg, va_list args);

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_TRACE(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 100
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_TRACE, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_DEBUG(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 200
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_DEBUG, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_INFO(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 300
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_INFO, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_WARNING(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 400
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_WARNING, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_ERROR(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 500
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_ERROR, category, msg, args);
    va_end(args);
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_FATAL(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 600
    va_list args; va_start(args, msg);
    logger(UA_LOGLEVEL_FATAL, category, msg, args);
    va_end(args);
#endif
}

/**
 * Convenience macros for complex types
 * ------------------------------------ */
#define UA_PRINTF_GUID_FORMAT "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define UA_PRINTF_GUID_DATA(GUID) (GUID).data1, (GUID).data2, (GUID).data3, \
        (GUID).data4[0], (GUID).data4[1], (GUID).data4[2], (GUID).data4[3], \
        (GUID).data4[4], (GUID).data4[5], (GUID).data4[6], (GUID).data4[7]

#define UA_PRINTF_STRING_FORMAT "\"%.*s\""
#define UA_PRINTF_STRING_DATA(STRING) (int)(STRING).length, (STRING).data


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_PLUGIN_LOG_H_ */
