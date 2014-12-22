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
 * @ingroup server
 *
 * @defgroup logging Logging
 *
 * @brief Custom logging solutions can be "plugged in" with this interface
 */

typedef enum UA_LoggerCategory {
    UA_LOGGINGCATEGORY_CONNECTION,
    UA_LOGGINGCATEGORY_SESSION,
    UA_LOGGINGCATEGORY_SUBSCRIPTION,
    UA_LOGGINGCATEGORY_SERVER
} UA_LoggerCategory;

typedef struct UA_Logger {
    void (*log_trace)(UA_LoggerCategory category, const char *msg, ...);
    void (*log_debug)(UA_LoggerCategory category, const char *msg, ...);
    void (*log_info)(UA_LoggerCategory category, const char *msg, ...);
    void (*log_warning)(UA_LoggerCategory category, const char *msg, ...);
    void (*log_error)(UA_LoggerCategory category, const char *msg, ...);
    void (*log_fatal)(UA_LoggerCategory category, const char *msg, ...);
} UA_Logger;

#if UA_LOGLEVEL <= 100
#define UA_LOG_TRACE(CATEGORY, MSG, ...) do { logger.log_trace(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define UA_LOG_TRACE(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 200
#define UA_LOG_DEBUG(CATEGORY, MSG, ...) do { logger.log_debug(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define UA_LOG_DEBUG(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 300
#define UA_LOG_INFO(CATEGORY, MSG, ...) do { logger.log_info(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define UA_LOG_INFO(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 400
#define UA_LOG_WARNING(CATEGORY, MSG, ...) do { logger.log_warning(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define UA_LOG_WARNING(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 500
#define UA_LOG_ERROR(CATEGORY, MSG, ...) do { logger.log_error(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define UA_LOG_ERROR(CATEGORY, MSG, ...) do {} while(0)
#endif

#if UA_LOGLEVEL <= 600
#define UA_LOG_FATAL(CATEGORY, MSG, ...) do { logger.log_fatal(CATEGORY, MSG, __VA_ARGS__); } while(0)
#else
#define UA_LOG_FATAL(CATEGORY, MSG, ...) do {} while(0)
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_LOG_H_ */
