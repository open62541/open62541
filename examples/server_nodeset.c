/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <signal.h>
#include <stdlib.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

/* files nodeset.h and nodeset.c are created from server_nodeset.xml in the /src_generated directory by CMake */
#include "nodeset.h"

UA_Boolean running = UA_TRUE;
UA_Logger logger = Logger_Stdout;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = 0;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    /* initialize the server */
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    UA_Server_setLogger(server, logger);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

    /* create nodes from nodeset */
    nodeset(server);

    /* start server */
    UA_StatusCode retval = UA_Server_run(server, 1, &running); //blocks until running=false

    /* ctrl-c received -> clean up */
    UA_Server_delete(server);
    return retval;
}
