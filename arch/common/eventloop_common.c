/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "eventloop_common.h"

UA_StatusCode
UA_EventLoopCommon_allocStaticBuffer(UA_KeyValueMap *params,
                                     UA_QualifiedName name,
                                     UA_UInt32 defaultSize,
                                     UA_ByteString *buf) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_UInt32 bufSize = defaultSize;
    const UA_UInt32 *configBufSize = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(params, name, &UA_TYPES[UA_TYPES_UINT32]);
    if(configBufSize)
        bufSize = *configBufSize;
    else
        /* Write the resolved default back into the params so the
         * SecureChannel constraint logic in ua_transport_tcp.c caps the
         * channel to the actual static-buffer size. */
        res = UA_KeyValueMap_setScalar(params, name, &bufSize,
                                       &UA_TYPES[UA_TYPES_UINT32]);
    if(buf->length != bufSize) {
        UA_ByteString_clear(buf);
        res |= UA_ByteString_allocBuffer(buf, bufSize);
    }
    return res;
}
