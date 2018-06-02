/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <signal.h>
#include "open62541.h"

/* Files example_namespace.h and example_namespace.c are created from server_nodeset.xml in the
 * /src_generated directory by CMake */
#include "example_nodeset.h"
#include "example_nodeset_ids.h"

UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    UA_StatusCode retval;
    /* create nodes from nodeset */
    if(example_nodeset(server) != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Could not add the example nodeset. "
        "Check previous output for any error.");
        retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    } else {

        // Do some additional stuff with the nodes

        // this will just get the namespace index, since it is already added to the server
        UA_UInt16 nsIdx = UA_Server_addNamespace(server, "http://yourorganisation.org/test/");

        UA_NodeId testInstanceId = UA_NODEID_NUMERIC(nsIdx, UA_EXAMPLE_NSID_TESTINSTANCE);

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The testInstance has ns=%d;id=%d",
                    testInstanceId.namespaceIndex, testInstanceId.identifier.numeric);

        retval = UA_Server_run(server, &running);
    }

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}
