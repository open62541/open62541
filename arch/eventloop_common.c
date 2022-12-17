/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "eventloop_common.h"

UA_StatusCode
UA_KeyValueRestriction_validate(const UA_KeyValueRestriction *restrictions, size_t restrictionsSize,
                                const UA_KeyValueMap *map) {
    for(size_t i = 0; i < restrictionsSize; i++) {
        const UA_KeyValueRestriction *r = &restrictions[i];
        const UA_Variant *val = UA_KeyValueMap_get(map, r->name);

        /* Value not present but required? */
        if(!val) {
            if(r->required)
                return UA_STATUSCODE_BADINTERNALERROR;
            continue;
        }

        /* Type matches */
        if(val->type != r->type)
            return UA_STATUSCODE_BADINTERNALERROR;

        /* Scalar / array is allowed */
        UA_Boolean scalar = UA_Variant_isScalar(val);
        if(scalar && !r->scalar)
            return UA_STATUSCODE_BADINTERNALERROR;
        if(!scalar && !r->array)
            return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    return UA_STATUSCODE_GOOD;
}
