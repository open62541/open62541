/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_EVENTLOOP_COMMON_H_
#define UA_EVENTLOOP_COMMON_H_

#include <open62541/plugin/eventloop.h>

/* Utility definitions to be used in EventLoop implementations.
 * Not part of the public API. */

_UA_BEGIN_DECLS

/* Typing restrictions for key-value parameters */
typedef struct {
    UA_QualifiedName name;
    const UA_DataType *type;
    UA_Boolean required;
    UA_Boolean scalar;
    UA_Boolean array;
} UA_KeyValueRestriction;

UA_StatusCode
UA_KeyValueRestriction_validate(const UA_Logger *logger,
                                const char *logprefix,
                                const UA_KeyValueRestriction *restrictions,
                                size_t restrictionsSize,
                                const UA_KeyValueMap *map);

/* (Re)allocate a static network buffer sized by the UInt32 parameter `name`.
 * If the parameter is not set, use defaultSize and write it back into the
 * params so that readers of the params (e.g. the SecureChannel constraint
 * logic in ua_server_binary.c) see the size that was actually allocated. */
UA_StatusCode
UA_EventLoopCommon_allocStaticBuffer(UA_KeyValueMap *params,
                                     UA_QualifiedName name,
                                     UA_UInt32 defaultSize,
                                     UA_ByteString *buf);

_UA_END_DECLS

#endif /* UA_EVENTLOOP_COMMON_H_ */
