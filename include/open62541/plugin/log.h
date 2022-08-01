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
    UA_LOGLEVEL_TRACE = 0,
    UA_LOGLEVEL_DEBUG,
    UA_LOGLEVEL_INFO,
    UA_LOGLEVEL_WARNING,
    UA_LOGLEVEL_ERROR,
    UA_LOGLEVEL_FATAL
} UA_LogLevel;

#define UA_LOGCATEGORIES 8

typedef enum {
    UA_LOGCATEGORY_NETWORK = 0,
    UA_LOGCATEGORY_SECURECHANNEL,
    UA_LOGCATEGORY_SESSION,
    UA_LOGCATEGORY_SERVER,
    UA_LOGCATEGORY_CLIENT,
    UA_LOGCATEGORY_USERLAND,
    UA_LOGCATEGORY_SECURITYPOLICY,
    UA_LOGCATEGORY_EVENTLOOP
} UA_LogCategory;

typedef struct {
    /* Log a message. The message string and following varargs are formatted
     * according to the rules of the printf command. Use the convenience macros
     * below that take the minimum log level defined in ua_config.h into
     * account. */
    void (*log)(void *logContext, UA_LogLevel level, UA_LogCategory category, const char* file, const char* function, uint_least32_t line, 
                const char *msg, va_list args);

    void *context; /* Logger state */

    void (*clear)(void *context); /* Clean up the logger plugin */
} UA_Logger;


#define UA_SOURCE_LOCATION 1
#ifdef UA_SOURCE_LOCATION
inline void Log( const UA_Logger *logger, UA_LogLevel level, UA_LogCategory category, const char* file, const char* function, uint_least32_t line, const char *msg, ... )
{
    va_list args; va_start( args, msg );
    //va_list args2; va_copy( args2, args );
    //size_t l1 = strlen( msg );
    //int len = vsnprintf( 0, 0, msg, args2 );
    //size_t len = 1023;
    //char* x = new char[len+1];
    //vsnprintf( x, len + 1, msg, args );
    logger->log( logger->context, level, category, file, function, line, msg, args );
    va_end(args);
}
#define UA_LOG_TRACE0( logger, category, msg ) Log( logger, UA_LOGLEVEL_TRACE, category, __FILE__, __FUNCTION__, __LINE__, msg )
#define UA_LOG_TRACE( logger, category, msg, ... ) Log( logger, UA_LOGLEVEL_TRACE, category, __FILE__, __FUNCTION__, __LINE__, msg, __VA_ARGS__ )
#else
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
#endif

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

#ifdef UA_SOURCE_LOCATION
    #define UA_LOG_INFO0( logger, category, msg ) Log( logger, UA_LOGLEVEL_INFO, category, __FILE__,__FUNCTION__,__LINE__, msg )
    #define UA_LOG_INFO( logger, category, msg, ... ) Log( logger, UA_LOGLEVEL_INFO, category, __FILE__,__FUNCTION__,__LINE__, msg, __VA_ARGS__ )
#else
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
#endif
#define UA_SOURCE_LOCATION 1
#ifdef UA_SOURCE_LOCATION
#define UA_LOG_WARNING0( logger, category, msg ) Log( logger, UA_LOGLEVEL_WARNING, category, __FILE__,__FUNCTION__,__LINE__,  msg )
#define UA_LOG_WARNING( logger, category, msg, ... ) Log( logger, UA_LOGLEVEL_WARNING, category, __FILE__,__FUNCTION__,__LINE__,  msg, __VA_ARGS__ )
#else
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
#endif

#ifdef UA_SOURCE_LOCATION
#define UA_LOG_ERROR0( logger, category, msg ) Log( logger, UA_LOGLEVEL_INFO, category, __FILE__,__FUNCTION__,__LINE__,  msg )
#define UA_LOG_ERROR( logger, category, msg, ... ) Log( logger, UA_LOGLEVEL_INFO, category, __FILE__,__FUNCTION__,__LINE__,  msg, __VA_ARGS__ )
#else
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
#endif

#ifdef UA_SOURCE_LOCATION
#define UA_LOG_FATAL0( logger, category, msg ) Log( logger, UA_LOGLEVEL_FATAL, category, __FILE__, __FUNCTION__, __LINE__,  msg )
#define UA_LOG_FATAL( logger, category, msg, ... ) Log( logger, UA_LOGLEVEL_FATAL, category, __FILE__, __FUNCTION__, __LINE__,  msg, __VA_ARGS__ )
#else
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
#endif

_UA_END_DECLS

#endif /* UA_PLUGIN_LOG_H_ */
