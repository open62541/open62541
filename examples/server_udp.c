/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h> 
#include <signal.h>

// provided by the open62541 lib
# include "ua_types.h"
# include "ua_server.h"

// provided by the user, implementations available in the /examples folder
#include "logger_stdout.h"
#include "networklayer_udp.h"

UA_Boolean running = 1;

static void stopHandler(int sign) {
    printf("Received Ctrl-C\n");
    running = 0;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    UA_ServerNetworkLayer *nl = ServerNetworkLayerUDP_new(UA_ConnectionConfig_standard, 16664);
    UA_Server_addNetworkLayer(server, nl);

    // add a variable node to the adresspace
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NULL, attr, NULL);

    UA_StatusCode retval = UA_Server_run(server, 1, &running);
    UA_Server_delete(server);

    return (int) retval;
}
