/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <signal.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

UA_Boolean running = 1;
UA_Logger logger = Logger_Stdout;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = 0;
}

static void testCallback(UA_Server *server, void *data) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "testcallback");
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664, logger);
    config.logger = Logger_Stdout;
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    /* add a repeated job to the server */
    UA_Job job = {.type = UA_JOBTYPE_METHODCALL,
                  .job.methodCall = {.method = testCallback, .data = NULL} };
    UA_Server_addRepeatedJob(server, job, 2000, NULL); // call every 2 sec

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return retval;
}
