/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/types.h>
#include <open62541/pubsub.h>
#include "custom_memory_manager.h"

extern "C" int
LLVMFuzzerTestOneInput(uint8_t *data, size_t size) {
    if(size < 4)
        return 0;

    /* Set memory limit from last 4 bytes to test OOM handling */
    if(!UA_memoryManager_setLimitFromLast4Bytes(data, size))
        return 0;
    size -= 4;

    /* Create ByteString from fuzz input */
    UA_ByteString buf;
    buf.data = (UA_Byte*)(void*)data;
    buf.length = size;

    /* Decode the NetworkMessage from binary */
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));

    UA_StatusCode rv = UA_NetworkMessage_decodeBinary(&buf, &nm, NULL, NULL);
    if(rv == UA_STATUSCODE_GOOD) {
        UA_NetworkMessage_clear(&nm);
    }

    return 0;
}
