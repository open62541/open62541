/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"
#include "ua_config_standard.h"
#include "ua_log_stdout.h"
#include "ua_plugin_log.h"
#include "testing_networklayers.h"

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

    UA_Connection c = createDummyConnection();
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    // we need to copy the message because it will be freed in the processing function
    UA_ByteString msg = UA_ByteString();
    UA_StatusCode retval = UA_ByteString_allocBuffer(&msg, size);
    if(retval != UA_STATUSCODE_GOOD)
        return (int)retval;
    memcpy(msg.data, data, size);

    UA_Server_processBinaryMessage(server, &c, &msg);
	// if we got an invalid chunk, the message is not deleted, so delete it here
    UA_ByteString_deleteMembers(&msg);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    UA_Connection_deleteMembers(&c);
    return 0;
}
