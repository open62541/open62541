/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

static void reverseConnectStateCallback(UA_Server *server, UA_UInt64 handle,
                                        UA_ReverseConnectState state, void *context) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Reverse connect state callback for %lu with context %p: State %d", handle, context,  state);
}


int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    /*UA_Server_run_startup(server); */

    UA_UInt64 handle = 0;
    UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://localhost:4841"),
                                reverseConnectStateCallback, (void *)123456, &handle);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Test remove */
    /*for (int i = 0; i < 10000; ++i) {
        retval = UA_Server_run_iterate(server, true);

        if (i == 8000)
            UA_Server_removeReverseConnect(server, handle);

        if (!running)
            break;
    }

    UA_Server_run_shutdown(server);*/

    UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
