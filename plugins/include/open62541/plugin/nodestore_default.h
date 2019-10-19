/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julius Pfrommer, Fraunhofer IOSB
 */

#ifndef UA_NODESTORE_DEFAULT_H_
#define UA_NODESTORE_DEFAULT_H_

#include <open62541/plugin/nodestore.h>

_UA_BEGIN_DECLS

UA_EXPORT UA_StatusCode
UA_Nodestore_ZipTree(UA_Nodestore *ns);

_UA_END_DECLS

#endif /* UA_NODESTORE_DEFAULT_H_ */
