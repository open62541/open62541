/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_PLUGIN_LOG_H_
#define UA_PLUGIN_LOG_H_

#include <open62541/config.h>

#include <stdarg.h>

_UA_BEGIN_DECLS

/**
 * .. _logging:
 *
 * Logging Plugin API
 * ==================
 *
 * Servers and clients define a logger in their configuration. The logger is a
 * plugin. A default plugin that logs to ``stdout`` is provided as an example.
 * The logger plugin is stateful and can point to custom data. So it is possible
 * to keep open file handlers in the logger context.
 *
 * Every log message consists of a log level, a log category and a string
 * message content. The timestamp of the log message is created within the
 * logger. */

typedef enum {
    UA_LOGLEVEL_TRACE   = 100,
    UA_LOGLEVEL_DEBUG   = 200,
    UA_LOGLEVEL_INFO    = 300,
    UA_LOGLEVEL_WARNING = 400,
    UA_LOGLEVEL_ERROR   = 500,
    UA_LOGLEVEL_FATAL   = 600
} UA_LogLevel;

#define UA_LOGCATEGORIES 10

typedef enum {
    UA_LOGCATEGORY_NETWORK = 0,
    UA_LOGCATEGORY_SECURECHANNEL,
    UA_LOGCATEGORY_SESSION,
    UA_LOGCATEGORY_SERVER,
    UA_LOGCATEGORY_CLIENT,
    UA_LOGCATEGORY_USERLAND,
    UA_LOGCATEGORY_SECURITYPOLICY,
    UA_LOGCATEGORY_EVENTLOOP,
    UA_LOGCATEGORY_PUBSUB,
    UA_LOGCATEGORY_DISCOVERY
} UA_LogCategory;

typedef struct UA_Logger {
    /* Log a message. The message string and following varargs are formatted
     * according to the rules of the printf command. Use the convenience macros
     * below that take the minimum log level defined in ua_config.h into
     * account. */
    void (*log)(void *logContext, UA_LogLevel level, UA_LogCategory category,
                const char *msg, va_list args);

    void *context; /* Logger state */

    void (*clear)(struct UA_Logger *logger); /* Clean up the logger plugin */
} UA_Logger;

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_TRACE(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 100
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_TRACE, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_DEBUG(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 200
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_DEBUG, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_INFO(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 300
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_INFO, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_WARNING(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 400
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_WARNING, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_ERROR(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 500
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_ERROR, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

static UA_INLINE UA_FORMAT(3,4) void
UA_LOG_FATAL(const UA_Logger *logger, UA_LogCategory category, const char *msg, ...) {
#if UA_LOGLEVEL <= 600
    if(!logger || !logger->log)
        return;
    va_list args; va_start(args, msg);
    logger->log(logger->context, UA_LOGLEVEL_FATAL, category, msg, args);
    va_end(args);
#else
    (void) logger;
    (void) category;
    (void) msg;
#endif
}

_UA_END_DECLS

#endif /* UA_PLUGIN_LOG_H_ */
