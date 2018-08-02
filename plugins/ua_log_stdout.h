/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2016 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_LOG_STDOUT_H_
#define UA_LOG_STDOUT_H_

#include "ua_plugin_log.h"

_UA_BEGIN_DECLS

void UA_EXPORT
UA_Log_Stdout(UA_LogLevel level, UA_LogCategory category,
              const char *msg, va_list args);

_UA_END_DECLS

#endif /* UA_LOG_STDOUT_H_ */
