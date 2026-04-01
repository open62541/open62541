/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

static void reverseConnectStateCallback(UA_Server *server, UA_UInt64 handle,
                                        UA_SecureChannelState state, void *context) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Reverse connect state callback for %lu with context %p: State %d",
                (unsigned long)handle, context,  state);
}


int main(void) {
    UA_Server *server = UA_Server_new();

    UA_UInt64 handle = 0;
    UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://localhost:4841"),
                                reverseConnectStateCallback, (void *)123456, &handle);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
