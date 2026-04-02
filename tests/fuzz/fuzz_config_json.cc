/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Fuzzer for JSON configuration parsing (UA_ServerConfig_loadFromFile
 * and UA_ClientConfig_loadFromFile).
 */

#include <open62541/server.h>
#include <open62541/client.h>
#include <open62541/server_config_default.h>
#include <open62541/client_config_default.h>

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    UA_ByteString buf;
    buf.data = (UA_Byte*)(uintptr_t)data;
    buf.length = size;

    /* Use the first byte to decide which parser to exercise */
    if(size < 1)
        return 0;

    if(data[0] % 2 == 0) {
        /* Fuzz server config parsing */
        UA_Server *server = UA_Server_newFromFile(buf);
        if(server)
            UA_Server_delete(server);
    } else {
        /* Fuzz client config parsing */
        UA_Client *client = UA_Client_newFromFile(buf);
        if(client)
            UA_Client_delete(client);
    }

    return 0;
}
