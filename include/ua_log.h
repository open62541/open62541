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

#include "ua_config.h"

/**
 * Logging
 * -------
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
typedef void (*UA_Logger)(UA_LogLevel level, UA_LogCategory category, const char *msg, ...);

#if UA_LOGLEVEL <= 100
#define UA_LOG_TRACE(LOGGER, CATEGORY, ...) do { \
        if(LOGGER) LOGGER(UA_LOGLEVEL_TRACE, CATEGORY, __VA_ARGS__); } while(0)
#else
#define UA_LOG_TRACE(LOGGER, CATEGORY, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 200
#define UA_LOG_DEBUG(LOGGER, CATEGORY, ...) do { \
        if(LOGGER) LOGGER(UA_LOGLEVEL_DEBUG, CATEGORY, __VA_ARGS__); } while(0)
#else
#define UA_LOG_DEBUG(LOGGER, CATEGORY, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 300
#define UA_LOG_INFO(LOGGER, CATEGORY, ...) do { \
        if(LOGGER) LOGGER(UA_LOGLEVEL_INFO, CATEGORY, __VA_ARGS__); } while(0)
#else
#define UA_LOG_INFO(LOGGER, CATEGORY, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 400
#define UA_LOG_WARNING(LOGGER, CATEGORY, ...) do { \
        if(LOGGER) LOGGER(UA_LOGLEVEL_WARNING, CATEGORY, __VA_ARGS__); } while(0)
#else
#define UA_LOG_WARNING(LOGGER, CATEGORY, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 500
#define UA_LOG_ERROR(LOGGER, CATEGORY, ...) do { \
        if(LOGGER) LOGGER(UA_LOGLEVEL_ERROR, CATEGORY, __VA_ARGS__); } while(0)
#else
#define UA_LOG_ERROR(LOGGER, CATEGORY, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 600
#define UA_LOG_FATAL(LOGGER, CATEGORY, ...) do { \
        if(LOGGER) LOGGER(UA_LOGLEVEL_FATAL, CATEGORY, __VA_ARGS__); } while(0)
#else
#define UA_LOG_FATAL(LOGGER, CATEGORY, ...) do {} while(0)
#endif

/**
 * Convenience macros for complex types
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define UA_PRINTF_GUID_FORMAT "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"
#define UA_PRINTF_GUID_DATA(GUID) (GUID).identifier.guid.data1, (GUID).identifier.guid.data2, \
        (GUID).identifier.guid.data3, (GUID).identifier.guid.data4[0],  \
        (GUID).identifier.guid.data4[1], (GUID).identifier.guid.data4[2], \
        (GUID).identifier.guid.data4[3], (GUID).identifier.guid.data4[4], \
        (GUID).identifier.guid.data4[5], (GUID).identifier.guid.data4[6], \
        (GUID).identifier.guid.data4[7]

#define UA_PRINTF_STRING_FORMAT "\"%.*s\""
#define UA_PRINTF_STRING_DATA(STRING) (STRING).length, (STRING).data

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_LOG_H_ */
