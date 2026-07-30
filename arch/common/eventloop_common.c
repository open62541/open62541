/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "eventloop_common.h"

UA_StatusCode
UA_KeyValueRestriction_validate(const UA_Logger *logger, const char *logprefix,
                                const UA_KeyValueRestriction *restrictions,
                                size_t restrictionsSize,
                                const UA_KeyValueMap *map) {
    for(size_t i = 0; i < restrictionsSize; i++) {
        const UA_KeyValueRestriction *r = &restrictions[i];
        const UA_Variant *val = UA_KeyValueMap_get(map, r->name);

        /* Value not present but required? */
        if(!val) {
            if(r->required) {
                UA_LOG_WARNING(logger, UA_LOGCATEGORY_EVENTLOOP,
                               "%s\t| Parameter %.*s required but not defined",
                               logprefix, (int)r->name.name.length, (char*)r->name.name.data);
                return UA_STATUSCODE_BADINTERNALERROR;
            }
            continue;
        }

        /* Type matches */
        if(val->type != r->type) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_EVENTLOOP,
                           "%s\t| Parameter %.*s has the wrong type",
                           logprefix, (int)r->name.name.length, (char*)r->name.name.data);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        /* Scalar / array is allowed */
        UA_Boolean scalar = UA_Variant_isScalar(val);
        if(scalar && !r->scalar) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_EVENTLOOP,
                           "%s\t| Parameter %.*s must not be scalar",
                           logprefix, (int)r->name.name.length, (char*)r->name.name.data);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        if(!scalar && !r->array) {
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_EVENTLOOP,
                           "%s\t| Parameter %.*s must not be an array",
                           logprefix, (int)r->name.name.length, (char*)r->name.name.data);
            return UA_STATUSCODE_BADCONNECTIONREJECTED;
        }
    }

    return UA_STATUSCODE_GOOD;
}

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
         * SecureChannel constraint logic in ua_server_binary.c caps the
         * channel to the actual static-buffer size. */
        res = UA_KeyValueMap_setScalar(params, name, &bufSize,
                                       &UA_TYPES[UA_TYPES_UINT32]);
    if(buf->length != bufSize) {
        UA_ByteString_clear(buf);
        res |= UA_ByteString_allocBuffer(buf, bufSize);
    }
    return res;
}
