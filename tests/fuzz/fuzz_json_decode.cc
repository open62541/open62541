/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 */

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(uint8_t *data, size_t size) {
    UA_ByteString buf;
    buf.data = (UA_Byte*)data;
    buf.length = size;

    UA_Variant out;
    UA_Variant_init(&out);

    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    if(retval == UA_STATUSCODE_GOOD)
        UA_Variant_clear(&out);

    return 0;
}
