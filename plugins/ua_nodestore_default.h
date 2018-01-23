/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_NODESTORE_DEFAULT_H_
#define UA_NODESTORE_DEFAULT_H_

#include "ua_plugin_nodestore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes the nodestore, sets the context and function pointers */
UA_StatusCode UA_EXPORT
UA_Nodestore_default_new(UA_Nodestore *ns);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NODESTORE_DEFAULT_H_ */
