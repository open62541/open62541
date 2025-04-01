/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/types.h>

/* Decode a message, then encode, decode, encode.
 * The two encodings must be bit-equal. */
extern "C" int
LLVMFuzzerTestOneInput(uint8_t *data, size_t size) {
    UA_ByteString buf;
    buf.data = (UA_Byte*)data;
    buf.length = size;

    UA_Variant value;
    UA_Variant_init(&value);

    UA_StatusCode retval = UA_decodeXml(&buf, &value, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    if(retval != UA_STATUSCODE_GOOD)
        return 0;

    /* This can fail for now. For example length limits are not always computed
     * 100% identical between encoding and decoding. */
    size_t xmlSize = UA_calcSizeXml(&value, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    if(xmlSize == 0) {
        UA_Variant_clear(&value);
        return 0;
    }

    UA_ByteString buf2 = UA_BYTESTRING_NULL;
    retval = UA_ByteString_allocBuffer(&buf2, xmlSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Variant_clear(&value);
        return 0;
    }

    retval = UA_encodeXml(&value, &UA_TYPES[UA_TYPES_VARIANT], &buf2, NULL);
    UA_assert(retval == UA_STATUSCODE_GOOD);

    UA_Variant value2;
    UA_Variant_init(&value2);
    retval = UA_decodeXml(&buf2, &value2, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    if(retval == UA_STATUSCODE_BADOUTOFMEMORY) {
        UA_Variant_clear(&value);
        UA_ByteString_clear(&buf2);
        return 0;
    }
    UA_assert(retval == UA_STATUSCODE_GOOD);

    UA_assert(UA_order(&value, &value2, &UA_TYPES[UA_TYPES_VARIANT]) == UA_ORDER_EQ);

    UA_ByteString buf3 = UA_BYTESTRING_NULL;
    retval = UA_ByteString_allocBuffer(&buf3, xmlSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Variant_clear(&value);
        UA_Variant_clear(&value2);
        UA_ByteString_clear(&buf2);
        return 0;
    }

    retval = UA_encodeXml(&value2, &UA_TYPES[UA_TYPES_VARIANT], &buf3, NULL);
    UA_assert(retval == UA_STATUSCODE_GOOD);

    UA_assert(buf2.length == buf3.length);
    UA_assert(memcmp(buf2.data, buf3.data, buf2.length) == 0);

    UA_Variant_clear(&value);
    UA_Variant_clear(&value2);
    UA_ByteString_clear(&buf2);
    UA_ByteString_clear(&buf3);
    return 0;
}
