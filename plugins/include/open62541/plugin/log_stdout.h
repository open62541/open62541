/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016, 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_LOG_STDOUT_H_
#define UA_LOG_STDOUT_H_

#include <open62541/plugin/log.h>

_UA_BEGIN_DECLS

extern UA_EXPORT const UA_Logger UA_Log_Stdout_; /* Logger structure */
extern UA_EXPORT const UA_Logger *UA_Log_Stdout; /* Shorthand pointer */

/* Don't use these definitions. They are only exported as long as the client
 * config is static and required compile-time  */
UA_EXPORT void
UA_Log_Stdout_log(void *_, UA_LogLevel level, UA_LogCategory category,
                  const char *msg, va_list args);
UA_EXPORT void
UA_Log_Stdout_clear(void *logContext);

_UA_END_DECLS

#endif /* UA_LOG_STDOUT_H_ */
