/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "custom_memory_manager.h"

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"


static UA_Boolean tortureEncoding(const uint8_t *data, size_t size, size_t *newOffset) {
    *newOffset = 0;
    if (size <= 2)
        return UA_FALSE;

    // get some random type
    uint16_t typeIndex = (uint16_t)(data[0] | data[1] << 8);
    data += 2;
    size -= 2;

    if (typeIndex >= UA_TYPES_COUNT)
        return UA_FALSE;

    void *dst = UA_new(&UA_TYPES[typeIndex]);

    if (!dst)
        return UA_FALSE;

    const UA_ByteString binary = {
            size, //length
            (UA_Byte *) (void *) data
    };

    UA_StatusCode ret = UA_decodeBinary(&binary, newOffset, dst, &UA_TYPES[typeIndex], NULL);

    if (ret == UA_STATUSCODE_GOOD) {
        // copy the datatype to test
        void *dstCopy = UA_new(&UA_TYPES[typeIndex]);
        if (!dstCopy)
            return UA_FALSE;
        UA_copy(dst, dstCopy, &UA_TYPES[typeIndex]);
        UA_delete(dstCopy, &UA_TYPES[typeIndex]);

        // now also test encoding
        UA_ByteString encoded;
        UA_ByteString_allocBuffer(&encoded, *newOffset);
        const UA_Byte *end = &encoded.data[*newOffset];
        UA_Byte *pos = encoded.data;
        ret = UA_encodeBinary(dst, &UA_TYPES[typeIndex], &pos, &end, NULL, NULL);
        if (ret == UA_STATUSCODE_GOOD) {
            // do nothing
        }
        UA_ByteString_deleteMembers(&encoded);
    }
    UA_delete(dst, &UA_TYPES[typeIndex]);

    return UA_TRUE;
}

static UA_Boolean tortureExtensionObject(const uint8_t *data, size_t size, size_t *newOffset) {
    *newOffset = 0;
    // check if there is still enough data to create an extension object
    // we need at least a nodeid.numeric which equals to 4 bytes
    if (size < 4)
        return UA_FALSE;

    UA_UInt32 identifier = (UA_UInt32)(data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24);
    size-= 4;


    UA_NodeId objectId = UA_NODEID_NUMERIC(0, identifier);

    UA_ExtensionObject obj;
    UA_ExtensionObject_init(&obj);
    obj.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    obj.content.encoded.typeId = objectId;
    obj.content.encoded.body.length = size;
    obj.content.encoded.body.data = (UA_Byte*)(void*)data; // discard const. We are sure that we don't change it

    const UA_DataType *type = UA_findDataTypeByBinary(&obj.content.encoded.typeId);

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if (type) {
        void *dstCopy = UA_new(type);
        if (!dstCopy)
            return UA_FALSE;
        ret = UA_decodeBinary(&obj.content.encoded.body, newOffset, dstCopy, type, NULL);

        if (ret == UA_STATUSCODE_GOOD) {
            UA_Variant var;
            UA_Variant_init(&var);
            UA_Variant_setScalar(&var, dstCopy, type);
        }
        UA_delete(dstCopy, type);
    }
    return ret==UA_STATUSCODE_GOOD;
}

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

    if (!UA_memoryManager_setLimitFromLast4Bytes(data, size))
        return 0;
    size -= 4;

    size_t offset;
    if (!tortureEncoding(data, size, &offset)) {
        return 0;
    }
    if (offset >= size)
        return 0;


    tortureExtensionObject(&data[offset], size-offset, &offset);


    return 0;
}
