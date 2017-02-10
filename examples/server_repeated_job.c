/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <signal.h>
#include "open62541.h"

UA_Boolean running = true;
UA_Logger logger = UA_Log_Stdout;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static void testCallback(UA_Server *server, void *data) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "testcallback");
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    /* add a repeated job to the server */
    UA_Job job;
    job.type = UA_JOBTYPE_METHODCALL;
    job.job.methodCall.data = NULL;
    job.job.methodCall.method = testCallback;
    UA_Server_addRepeatedJob(server, job, 2000, NULL); // call every 2 sec

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return (int)retval;
}
