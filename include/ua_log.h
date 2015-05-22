/*
 * Copyright (C) 2014 the contributors as stated in the AUTHORS file
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
 * @defgroup logging Logging
 *
 * @brief Custom logging solutions can be "plugged in" with this interface
 *
 * @{
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
    UA_LOGCATEGORY_COMMUNICATION,
    UA_LOGCATEGORY_SERVER,
    UA_LOGCATEGORY_CLIENT,
    UA_LOGCATEGORY_USERLAND
} UA_LogCategory;
    
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

/** @} */

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_LOG_H_ */
