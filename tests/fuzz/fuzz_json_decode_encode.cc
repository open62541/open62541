/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>
#include "ua_types_encoding_json.h"

/* Decode a message, then encode, decode, encode.
 * The two encodings must be bit-equal. */
extern "C" int
LLVMFuzzerTestOneInput(uint8_t *data, size_t size) {
    UA_ByteString buf;
    buf.data = (UA_Byte*)data;
    buf.length = size;

    UA_Variant value;
    UA_Variant_init(&value);

    UA_StatusCode retval = UA_decodeJson(&buf, &value, &UA_TYPES[UA_TYPES_VARIANT]);
    if(retval != UA_STATUSCODE_GOOD)
        return 0;

    size_t jsonSize = UA_calcSizeJson(&value, &UA_TYPES[UA_TYPES_VARIANT],
                                      NULL, 0, NULL, 0, true);

    UA_ByteString buf2 = UA_BYTESTRING_NULL;
    retval = UA_ByteString_allocBuffer(&buf2, jsonSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Variant_deleteMembers(&value);
        return 0;
    }

    uint8_t *bufPos = buf2.data;
    const uint8_t *bufEnd = &buf2.data[buf2.length];
    retval = UA_encodeJson(&value, &UA_TYPES[UA_TYPES_VARIANT],
                           &bufPos, &bufEnd, NULL, 0, NULL, 0, true);
	UA_Variant_deleteMembers(&value);
	if(retval != UA_STATUSCODE_GOOD || bufPos != bufEnd) {
		return 0;
	}

    UA_Variant value2;
    UA_Variant_init(&value2);

    retval = UA_decodeJson(&buf2, &value2, &UA_TYPES[UA_TYPES_VARIANT]);
    if(retval != UA_STATUSCODE_GOOD) {
		return 0;
	}

    UA_ByteString buf3 = UA_BYTESTRING_NULL;
    retval = UA_ByteString_allocBuffer(&buf3, jsonSize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Variant_deleteMembers(&value2);
        UA_ByteString_deleteMembers(&buf2);
        return 0;
    }

    bufPos = buf3.data;
    bufEnd = &buf3.data[buf3.length];
    retval = UA_encodeJson(&value2, &UA_TYPES[UA_TYPES_VARIANT],
                           &bufPos, &bufEnd, NULL, 0, NULL, 0, true);
	UA_Variant_deleteMembers(&value2);
	if(retval != UA_STATUSCODE_GOOD) {
		UA_ByteString_deleteMembers(&buf2);
		UA_ByteString_deleteMembers(&buf3);
		return 0;
	}
    if (memcmp(buf2.data, buf3.data, buf.length) != 0) {
    	// ignore
    }

    UA_ByteString_deleteMembers(&buf2);
    UA_ByteString_deleteMembers(&buf3);

    return 0;
}
