/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) fortiss (Author: Stefan Profanter)
 */

#include "custom_memory_manager.h"

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size <= 6)
        return 0;

    // set the available memory
    if(!UA_memoryManager_setLimitFromLast4Bytes(data, size))
        return 0;

    data += 4;
    size -= 4;

    // get some random type
    uint16_t typeIndex = (uint16_t)(data[0] | data[1] << 8);
    data += 2;
    size -= 2;

    if(typeIndex >= UA_TYPES_COUNT)
        return UA_FALSE;

    void *dst = UA_new(&UA_TYPES[typeIndex]);
    if(!dst)
        return 0;

    const UA_ByteString binary = {
            size, //length
            (UA_Byte *) (void *) data
    };

    UA_StatusCode ret = UA_decodeBinary(&binary, dst, &UA_TYPES[typeIndex], NULL);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_delete(dst, &UA_TYPES[typeIndex]);
        return 0;
    }

    // copy the datatype to test
    void *dstCopy = UA_new(&UA_TYPES[typeIndex]);
    if(!dstCopy) {
        UA_delete(dst, &UA_TYPES[typeIndex]);
        return 0;
    }
    ret = UA_copy(dst, dstCopy, &UA_TYPES[typeIndex]);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_delete(dst, &UA_TYPES[typeIndex]);
        UA_delete(dstCopy, &UA_TYPES[typeIndex]);
        return 0;
    }
    
    // compare with copy
    UA_assert(UA_order(dst, dstCopy, &UA_TYPES[typeIndex]) == UA_ORDER_EQ);
    UA_delete(dstCopy, &UA_TYPES[typeIndex]);
    
    // now also test encoding
    size_t encSize = UA_calcSizeBinary(dst, &UA_TYPES[typeIndex]);
    UA_ByteString encoded;
    ret = UA_ByteString_allocBuffer(&encoded, encSize);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_delete(dst, &UA_TYPES[typeIndex]);
        return 0;
    }

    ret = UA_encodeBinary(dst, &UA_TYPES[typeIndex], &encoded);
    UA_assert(ret == UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&encoded);
    UA_delete(dst, &UA_TYPES[typeIndex]);
    return 0;
}
