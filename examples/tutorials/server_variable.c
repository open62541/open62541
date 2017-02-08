/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Adding variables to a server
 * ----------------------------
 * This tutorial shows how to add variable nodes to a server and how these can
 * be connected to a physical process in the background.
 *
 * Variants and Datatypes
 * ^^^^^^^^^^^^^^^^^^^^^^
 * The datatype :ref:`variant` belongs to the built-in datatypes of OPC UA and
 * is used as a container type. A variant can hold any other datatype as a
 * scalar (except variant) or as an array. Array variants can additionally
 * denote the dimensionality of the data (e.g. a 2x3 matrix) in an additional
 * integer array.
 *
 * The `UA_VariableAttributes` type contains a variant member `value`. The
 * command `UA_Variant_setScalar(&attr.value, &myInteger,
 * &UA_TYPES[UA_TYPES_INT32])` sets the variant to point to the integer. Note
 * that this does not make a copy of the integer (for which
 * `UA_Variant_setScalarCopy` can be used). The variant (and its content) is
 * then copied into the newly created node.
 *
 * The above code could have used allocations by making copies of all entries of
 * the attribute variant. Then, of course, the variant content needs to be
 * deleted to prevent memleaks.
 *
 * NodeIds
 * ^^^^^^^
 * A :ref:`nodeid` is a unique identifier in server's context. It contains the
 * number of node's namespace and either a numeric, string or GUID (Globally
 * Unique ID) identifier. The following are some examples for their usage.
 *
 * .. code-block:: c
 *
 *    UA_NodeId id1 = UA_NODEID_NUMERIC(1, 1234);
 *    UA_NodeId id2 = UA_NODEID_STRING(1, "testid"); \/* points to the static string *\/
 *    UA_NodeId id3 = UA_NODEID_STRING_ALLOC(1, "testid"); \/* copy to memory *\/
 *    UA_NodeId_deleteMembers(&id3); \/* free the allocated string *\/
 *
 * This is the code for a server with a single variable node holding an integer.
 * We will take this example to explain some of the fundamental concepts of
 * open62541. */

#include <signal.h>
#include "open62541.h"

UA_Boolean running = true;
static void stopHandler(int sign) {
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);

    /* Create the server */
    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl =
        UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    /* Define the variable */
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");

    /* Add a variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NULL, attr, NULL, NULL);

    /* Run the server */
    UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return 0;
}
