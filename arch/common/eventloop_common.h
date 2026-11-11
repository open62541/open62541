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

/* Validate EventLoop parameters against the restrictions (see
 * UA_KeyValueMap_validate). Parameters without restriction are ignored. */
static UA_INLINE UA_StatusCode
UA_KeyValueRestriction_validate(const UA_Logger *logger,
                                const char *logprefix,
                                const UA_KeyValueRestriction *restrictions,
                                size_t restrictionsSize,
                                const UA_KeyValueMap *map) {
    return UA_KeyValueMap_validate(map, restrictions, restrictionsSize, false,
                                   logger, UA_LOGCATEGORY_EVENTLOOP, logprefix);
}

/* (Re)allocate a static network buffer sized by the UInt32 parameter `name`.
 * If the parameter is not set, use defaultSize and write it back into the
 * params so that readers of the params (e.g. the SecureChannel constraint
 * logic in ua_transport_tcp.c) see the size that was actually allocated. */
UA_StatusCode
UA_EventLoopCommon_allocStaticBuffer(UA_KeyValueMap *params,
                                     UA_QualifiedName name,
                                     UA_UInt32 defaultSize,
                                     UA_ByteString *buf);

_UA_END_DECLS

#endif /* UA_EVENTLOOP_COMMON_H_ */
