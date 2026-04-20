/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cstdint>
#include <cstddef>
#include <cstring>

#include <open62541/config.h>
#include <open62541/types.h>
#include <open62541/util.h>

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size <= 1)
        return 0;

    /* Use the first byte as a selector to choose which parsing function to fuzz */
    uint8_t selector = data[0];
    const uint8_t *payload = data + 1;
    size_t payload_size = size - 1;

    UA_String str;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&str, payload_size);
    if(retval != UA_STATUSCODE_GOOD)
        return 0;
    memcpy(str.data, payload, payload_size);

    switch(selector % 5) {
    case 0: {
        UA_NodeId id;
        UA_NodeId_init(&id);
        UA_NodeId_parse(&id, str);
        UA_NodeId_clear(&id);
        break;
    }
    case 1: {
        UA_DateTime dt;
        UA_DateTime_parse(&dt, str);
        break;
    }
    case 2: {
        UA_Guid guid;
        UA_Guid_init(&guid);
        UA_Guid_parse(&guid, str);
        break;
    }
    case 3: {
        UA_ExpandedNodeId enid;
        UA_ExpandedNodeId_init(&enid);
        UA_ExpandedNodeId_parse(&enid, str);
        UA_ExpandedNodeId_clear(&enid);
        break;
    }
    case 4: {
        UA_QualifiedName qn;
        UA_QualifiedName_init(&qn);
        UA_QualifiedName_parse(&qn, str);
        UA_QualifiedName_clear(&qn);
        break;
    }
    default: break;
    }

    UA_String_clear(&str);
    return 0;
}
