/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifndef LOGGER_STDOUT_H_
#define LOGGER_STDOUT_H_

#include "ua_types.h"
#include "ua_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialises the logger for the current thread. */
UA_EXPORT UA_Logger Logger_Stdout_new(void);

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_STDOUT_H_ */
