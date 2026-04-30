/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <open62541/server_pubsub.h>
#include <open62541/types.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size < 10)
        return 0;

    UA_ByteString msg = {size, (UA_Byte *) (void *) data};
    UA_PubSubConnectionConfig config;
    memset(&config, 0, sizeof(UA_PubSubConnectionConfig));

    /* Decode some fields from the fuzzer input */
    UA_decodeBinary(&msg, &config.name, &UA_TYPES[UA_TYPES_STRING], NULL);
    UA_decodeBinary(&msg, &config.address, &UA_TYPES[UA_TYPES_VARIANT], NULL);

    UA_PubSubConnectionConfig config2;
    UA_StatusCode retval = UA_PubSubConnectionConfig_copy(&config, &config2);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_PubSubConnectionConfig_clear(&config2);
    }
    UA_PubSubConnectionConfig_clear(&config);

    return 0;
}
