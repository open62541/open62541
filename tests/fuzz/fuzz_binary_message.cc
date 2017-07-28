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
    UA_ServerConfig *config = UA_ServerConfig_standard_new();
    UA_Server *server = UA_Server_new(*config);
    UA_ByteString msg = {
			size, //length
			const_cast<UA_Byte*>(data) //data
	};
    UA_Boolean reallocated = UA_FALSE;
    UA_StatusCode retval = UA_Connection_completeChunks(&c, &msg, &reallocated);
    if(retval == UA_STATUSCODE_GOOD && msg.length > 0)
        UA_Server_processBinaryMessage(server, &c, &msg);
    UA_Server_delete(server);
    UA_Connection_deleteMembers(&c);
    UA_ServerConfig_standard_delete(config);
    return 0;
}
