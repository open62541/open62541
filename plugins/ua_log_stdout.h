/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef UA_LOG_STDOUT_H_
#define UA_LOG_STDOUT_H_

#include "ua_types.h"
#include "ua_log.h"

#ifdef __cplusplus
extern "C" {
#endif

UA_EXPORT void UA_Log_Stdout(UA_LogLevel level, UA_LogCategory category, const char *msg, ...);

#ifdef __cplusplus
}
#endif

#endif /* UA_LOG_STDOUT_H_ */
