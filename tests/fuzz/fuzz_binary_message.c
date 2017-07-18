/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "fuzz_common.h"

UA_Connection c;
UA_ServerConfig config;
UA_Server *server = NULL;
UA_ByteString msg;

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (server == NULL) {
        c = createDummyConnection();
        config = UA_ServerConfig_standard;
        config.logger = UA_Log_Stdout;

        // no freeing needed, fuzzer is killed or shuts down due to exception
        server = UA_Server_new(config);
    }

    config.logger = UA_Log_Stdout;
    msg.length = size;
    msg.data = data;
    UA_Boolean reallocated = UA_FALSE;
    UA_StatusCode retval = UA_Connection_completeMessages(&c, &msg, &reallocated);
    if(retval == UA_STATUSCODE_GOOD && msg.length > 0)
        UA_Server_processBinaryMessage(server, &c, &msg);
    return 0;
}
