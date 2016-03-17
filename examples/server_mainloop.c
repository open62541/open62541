/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#define _BSD_SOURCE
#include <signal.h>
#include <unistd.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

UA_Boolean running = true;
UA_Logger logger = Logger_Stdout;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/* In this example, we integrate the server into an external "mainloop". This
   can be for example the event-loop used in GUI toolkits, such as Qt or GTK. */

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.logger = Logger_Stdout;
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    UA_StatusCode retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Should the server networklayer block (with a timeout) until a message
       arrives or should it return immediately? */
    UA_Boolean waitInternal = false;

    while(running) {
        /* timeout is the maximum possible delay (in millisec) until the next
           _iterate call. Otherwise, the server might miss an internal timeout
           or cannot react to messages with the promised responsiveness. */
        UA_UInt16 timeout = UA_Server_run_iterate(server, waitInternal);

        /* now we can use the max timeout to do something else */
        usleep((__useconds_t)(timeout * 1000));
    }
    retval = UA_Server_run_shutdown(server);

 cleanup:
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return (int)retval;
}
