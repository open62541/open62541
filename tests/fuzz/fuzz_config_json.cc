/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Fuzzer for the JSON5 server-configuration file parser
 * (plugins/ua_config_json.c). Added after GHSA-38g6-5hfj-2fj7: a heap
 * out-of-bounds read reachable via a malformed field name in a nested
 * configuration object. This code path previously had no fuzz coverage --
 * fuzz_json_decode(.*)?.cc only exercise UA_decodeJson() for the OPC UA
 * binary<->JSON type codec, an entirely separate parser.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_config_file_based.h>

#include <string.h>

extern "C" int
LLVMFuzzerTestOneInput(uint8_t *data, size_t size) {
    UA_ByteString buf;
    buf.data = (UA_Byte*)data;
    buf.length = size;

    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setDefault(&config);
    if(retval != UA_STATUSCODE_GOOD)
        return 0;

    UA_ServerConfig_updateFromFile(&config, buf);
    UA_ServerConfig_clean(&config);

    return 0;
}
