/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_NODESTORE_DEFAULT_H_
#define UA_NODESTORE_DEFAULT_H_

#include <open62541/plugin/nodestore.h>

_UA_BEGIN_DECLS

/* Initializes the nodestore, sets the context and function pointers */
UA_StatusCode UA_EXPORT
UA_Nodestore_default_new(UA_Nodestore *ns);

_UA_END_DECLS

#endif /* UA_NODESTORE_DEFAULT_H_ */
