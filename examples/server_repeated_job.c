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
UA_Logger logger;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = 0;
}

static void testCallback(UA_Server *server, void *data) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "testcallback");
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    logger = Logger_Stdout_new();
    UA_Server_setLogger(server, logger);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

    /* add a repeated job to the server */
    UA_Job job = {.type = UA_JOBTYPE_METHODCALL,
                  .job.methodCall = {.method = testCallback, .data = NULL} };
    UA_Server_addRepeatedJob(server, job, 2000, NULL); // call every 2 sec

    UA_StatusCode retval = UA_Server_run(server, 1, &running);
    UA_Server_delete(server);
    return retval;
}
