/*
 * Copyright (C) 2014-2016 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#ifndef UA_LOG_H_
#define UA_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include "ua_config.h"

/**
 * Logging
 * -------
 *
 * Servers and clients may contain a logger. Every logger needs to implement the
 * `UA_Logger` signature. An example logger that writes to stdout is provided in
 * the plugins folder.
 *
 * Every log-message consists of a log-level, a log-category and a string
 * message content. The timestamp of the log-message is created within the
 * logger.
 */

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
    UA_LOGCATEGORY_USERLAND
} UA_LogCategory;

/**
 * The signature of the logger. The msg string and following varargs are
 * formatted according to the rules of the printf command.
 *
 * Do not use the logger directly but make use of the following macros that take
 * the minimum log-level defined in ua_config.h into account. */
typedef void (*UA_Logger)(UA_LogLevel level, UA_LogCategory category,
                          const char *msg, va_list args);

static inline void
UA_LOG_TRACE(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 100
    if(logger) {
        va_list args; va_start(args, msg);
        logger(UA_LOGLEVEL_TRACE, category, msg, args);
        va_end(args);
    }
#endif
}

static inline void
UA_LOG_DEBUG(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 200
    if(logger) {
        va_list args; va_start(args, msg);
        logger(UA_LOGLEVEL_DEBUG, category, msg, args);
        va_end(args);
    }
#endif
}

static inline void
UA_LOG_INFO(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 300
    if(logger) {
        va_list args; va_start(args, msg);
        logger(UA_LOGLEVEL_INFO, category, msg, args);
        va_end(args);
    }
#endif
}

static inline void
UA_LOG_WARNING(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 400
    if(logger) {
        va_list args; va_start(args, msg);
        logger(UA_LOGLEVEL_WARNING, category, msg, args);
        va_end(args);
    }
#endif
}

static inline void
UA_LOG_ERROR(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 500
    if(logger) {
        va_list args; va_start(args, msg);
        logger(UA_LOGLEVEL_ERROR, category, msg, args);
        va_end(args);
    }
#endif
}

static inline void
UA_LOG_FATAL(UA_Logger logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 600
    if(logger) {
        va_list args; va_start(args, msg);
        logger(UA_LOGLEVEL_FATAL, category, msg, args);
        va_end(args);
    }
#endif
}

/**
 * Convenience macros for complex types
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
#define UA_PRINTF_GUID_FORMAT "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"
#define UA_PRINTF_GUID_DATA(GUID) (GUID).data1, (GUID).data2, (GUID).data3, \
        (GUID).data4[0], (GUID).data4[1], (GUID).data4[2], (GUID).data4[3], \
        (GUID).data4[4], (GUID).data4[5], (GUID).data4[6], (GUID).data4[7]

#define UA_PRINTF_STRING_FORMAT "\"%.*s\""
#define UA_PRINTF_STRING_DATA(STRING) (STRING).length, (STRING).data

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_LOG_H_ */
