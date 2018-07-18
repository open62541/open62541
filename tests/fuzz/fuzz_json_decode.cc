/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ua_types.h>
#include "ua_server_internal.h"
#include "ua_config_default.h"
#include "ua_log_stdout.h"
#include "ua_types_encoding_json.h"

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    /* TODO:
    
    UA_ByteString buf;
    buf.data = (UA_Byte*)Data;
    buf.length = Size;

    UA_Variant *out = UA_Variant_new();
    UA_Variant_init(out);

    UA_decodeJson(&buf, out, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_Variant_delete(out);
    */
    return 0;
}

