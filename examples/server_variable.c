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

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    logger = Logger_Stdout_new();
    UA_Server_setLogger(server, logger);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

    /* add a variable node to the adresspace */
    UA_Variant *myIntegerVariant = UA_Variant_new();
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(myIntegerVariant, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer"); /* UA_NODEID_NULL would assign a random free nodeid */
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerVariant, myIntegerName,
                              myIntegerNodeId, parentNodeId, parentReferenceNodeId);

    UA_StatusCode retval = UA_Server_run(server, 1, &running);
    UA_Server_delete(server);

    return retval;
}
