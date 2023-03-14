/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

static void reverseConnectStateCallback(UA_Server *server, UA_UInt64 handle,
                                        UA_SecureChannelState state, void *context) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Reverse connect state callback for %lu with context %p: State %d",
                (unsigned long)handle, context,  state);
}


int main(void) {
    UA_Server *server = UA_Server_new();

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

    UA_Server_runUntilInterrupt(server);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
