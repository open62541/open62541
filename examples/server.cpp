/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */
#include <iostream>

// provided by the open62541 lib
#include "ua_server.h"

// provided by the user, implementations available in the /examples folder
#include "logger_stdout.h"
#include "networklayer_tcp.h"

/**
 * Build Instructions (Linux)
 *
 * To build this C++ server, first compile the open62541 library. Then, compile the network layer and logging with a C compiler.
 * - gcc -std=c99 -c networklayer_tcp.c -I../include -I../build/src_generated
 * - gcc -std=c99 -c logger_stdout.c -I../include -I../build/src_generated
 * Lastly, compile and link the C++ server with
 * - g++ server.cpp networklayer_tcp.o logger_stdout.o ../build/libopen62541.a -I../include -I../build/src_generated -o server
 */

using namespace std;

int main()
{
	UA_Server *server = UA_Server_new();
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

	// add a variable node to the adresspace
    UA_Variant *myIntegerVariant = UA_Variant_new();
    UA_Int32 myInteger = 42;
    UA_Variant_copySetScalar(myIntegerVariant, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_QualifiedName myIntegerName;
    UA_QUALIFIEDNAME_ASSIGN(myIntegerName, "the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_NULL; /* assign a random free nodeid */
    UA_NodeId parentNodeId = UA_NODEID_STATIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_STATIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerVariant, myIntegerName,
                              myIntegerNodeId, parentNodeId, parentReferenceNodeId);

    UA_Boolean running = UA_TRUE;
    UA_StatusCode retval = UA_Server_run(server, 1, &running);
	UA_Server_delete(server);

	return retval;
}
