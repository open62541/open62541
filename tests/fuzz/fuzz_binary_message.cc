/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) fortiss (Author: Stefan Profanter)
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "ua_server_internal.h"
#include "testing_networklayers.h"

#define RECEIVE_BUFFER_SIZE 65535

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size <= 4)
        return 0;

    /* less debug output */
    UA_ServerConfig initialConfig;
    memset(&initialConfig, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setDefault(&initialConfig);
    initialConfig.allowEmptyVariables = UA_RULEHANDLING_ACCEPT;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(&initialConfig);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not generate the server config");
        return 0;
    }

    UA_Server *server = UA_Server_newWithConfig(&initialConfig);
    if(!server) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create server instance using UA_Server_new");
        return 0;
    }

    // we need to copy the message because it will be freed in the processing function
    UA_ByteString msg = UA_BYTESTRING_NULL;
    retval = UA_ByteString_allocBuffer(&msg, size);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not allocate message buffer");
        return 0;
    }
    memcpy(msg.data, data, size);

    /* Get the binary server components */
    UA_String binStr = UA_STRING((char*)(uintptr_t)"binary");
    UA_ServerComponent *bpm = NULL;
    for(UA_ServerComponent *sc = server->components; sc; sc = sc->next) {
        if(UA_String_equal(&binStr, &sc->name))
            bpm = sc;
    }
    UA_assert(bpm != NULL);

    void *ctx = NULL;
    serverNetworkCallback(&testConnectionManagerTCP, 0, bpm,
                          &ctx, UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, msg);

    // if we got an invalid chunk, the message is not deleted, so delete it here
    UA_ByteString_clear(&msg);
    UA_Server_delete(server);
    return 0;
}
