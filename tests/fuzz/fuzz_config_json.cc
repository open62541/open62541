/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Fuzzer for JSON configuration parsing (UA_Server_newFromFile).
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size < 1)
        return 0;

    UA_ByteString buf;
    buf.data = (UA_Byte*)(uintptr_t)data;
    buf.length = size;

    /* Fuzz server config parsing from JSON */
    UA_Server *server = UA_Server_newFromFile(buf);
    if(server)
        UA_Server_delete(server);

    return 0;
}
