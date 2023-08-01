/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

#include <signal.h>

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/* In this example, we integrate the server into an external "mainloop". This
 * can be for example the event-loop used in GUI toolkits, such as Qt or GTK. */

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_Server_run_startup(server);

    /* Should the server networklayer block (with a timeout) until a message
       arrives or should it return immediately? */
    UA_Boolean waitInternal = true;
    while(running) {
        UA_Server_run_iterate(server, waitInternal);
    }

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    return 0;
}
