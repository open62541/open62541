//This file contains source-code that is discussed in a tutorial located here:
// http://open62541.org/doc/sphinx/tutorial_firstStepsServer.html

#include <stdio.h>
#include <signal.h>

# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"

UA_Boolean running;
UA_Logger logger;

static void stopHandler(int signal) {
    running = UA_FALSE;
}

int main(void) {
    signal(SIGINT,  stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    logger = Logger_Stdout_new();
    UA_Server_setLogger(server, logger);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));
    running = UA_TRUE;
    UA_Server_run(server, 1, &running);
    UA_Server_delete(server);

    printf("Terminated\n");
    return 0;
}
