/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdint>
#include <cstddef>

#include <open62541/config.h>

#include <open62541/types.h>
#include <open62541/util.h>

#include <cstring>

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size == 0)
        return 0;

    UA_ByteString content;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&content, size);
    if(retval != UA_STATUSCODE_GOOD)
        return 0;
    memcpy(content.data, data, size);

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    retval = UA_EventFilter_parse(&filter, content, NULL);
    UA_EventFilter_clear(&filter);

    UA_ByteString_clear(&content);
    return 0;
}

