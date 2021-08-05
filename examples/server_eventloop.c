/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <server/ua_server_internal.h>
#include <ua_util_internal.h>

#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    UA_Boolean waitInternal = false;

    UA_StatusCode rv = UA_Server_run_startup(server);
    if(rv != UA_STATUSCODE_GOOD)
        goto cleanup;

    while(running) {
        /* timeout is the maximum possible delay (in millisec) until the next
           _iterate call. Otherwise, the server might miss an internal timeout
           or cannot react to messages with the promised responsiveness. */
        /* If multicast discovery server is enabled, the timeout does not not consider new input data (requests) on the mDNS socket.
         * It will be handled on the next call, which may be too late for requesting clients.
         * if needed, the select with timeout on the multicast socket server->mdnsSocket (see example in mdnsd library)
         */

        UA_Server_run_iterate(server, waitInternal);
        // UA_UInt16 timeout = UA_Server_run_iterate(server, waitInternal);
    }
    rv = UA_Server_run_shutdown(server);

 cleanup:
    UA_Server_delete(server);
    return rv == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
