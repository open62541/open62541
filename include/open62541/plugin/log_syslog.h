/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_LOG_SYSLOG_H_
#define UA_LOG_SYSLOG_H_

#include <open62541/plugin/log.h>

_UA_BEGIN_DECLS

/* Syslog-logging is available only for Linux/Unices.
 *
 * open62541 log levels are translated to syslog levels as follows:
 *
 * UA_LOGLEVEL_TRACE   => not available for syslog
 * UA_LOGLEVEL_DEBUG   => LOG_DEBUG
 * UA_LOGLEVEL_INFO    => LOG_INFO
 * UA_LOGLEVEL_WARNING => LOG_WARNING
 * UA_LOGLEVEL_ERROR   => LOG_ERR
 * UA_LOGLEVEL_FATAL   => LOG_CRIT
 */

#if defined(__linux__) || defined(__unix__)

/* Returns a syslog-logger for messages up to the specified level.
 * The programm must call openlog(3) before using this logger. */
UA_EXPORT UA_Logger
UA_Log_Syslog_withLevel(UA_LogLevel minlevel);

/* Allocates memory for the logger. Automatically cleared up via _clear. */
UA_EXPORT UA_Logger *
UA_Log_Syslog_new(UA_LogLevel minlevel);

/* Log all warning levels supported by syslog (no trace-warnings).
 * The programm must call openlog(3) before using this logger. */
UA_EXPORT UA_Logger
UA_Log_Syslog(void);

#endif

_UA_END_DECLS

#endif /* UA_LOG_SYSLOG_H_ */
