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

UA_EXPORT UA_Logger
UA_Log_Stdout_withLevel(UA_LogLevel minlevel);

_UA_END_DECLS

#endif /* UA_LOG_STDOUT_H_ */
