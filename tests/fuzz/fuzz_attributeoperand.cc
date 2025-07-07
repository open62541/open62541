/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
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

    const UA_String input = {size, (UA_Byte *) (void *) data};
    UA_String out = UA_STRING_NULL;
    UA_String out2 = UA_STRING_NULL;

    UA_AttributeOperand ao;
    UA_AttributeOperand ao2;
    UA_AttributeOperand_init(&ao2);
    UA_StatusCode ret = UA_AttributeOperand_parse(&ao, input);
    if(ret != UA_STATUSCODE_GOOD)
        return 0;

    ret = UA_AttributeOperand_print(&ao, &out);
    if(ret == UA_STATUSCODE_BADOUTOFMEMORY)
        goto cleanup;
    UA_assert(ret == UA_STATUSCODE_GOOD);

    ret = UA_AttributeOperand_parse(&ao2, out);
    if(ret == UA_STATUSCODE_BADOUTOFMEMORY)
        goto cleanup;
    UA_assert(ret == UA_STATUSCODE_GOOD);

    ret = UA_AttributeOperand_print(&ao2, &out2);
    if(ret == UA_STATUSCODE_BADOUTOFMEMORY)
        goto cleanup;
    UA_assert(ret == UA_STATUSCODE_GOOD);

    UA_assert(UA_String_equal(&out, &out2));
    UA_assert(UA_equal(&ao, &ao2, &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND]));

 cleanup:
    UA_String_clear(&out);
    UA_String_clear(&out2);
    UA_AttributeOperand_clear(&ao);
    UA_AttributeOperand_clear(&ao2);
    return 0;
}
