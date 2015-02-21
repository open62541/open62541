/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */
#include <time.h>
#include "ua_types.h"

#include <stdio.h>
#include <stdlib.h> 
#include <signal.h>
#include <errno.h> // errno, EINTR

// provided by the open62541 lib
#include "ua_server.h"

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

	UA_Server *server = UA_Server_new();
    UA_Server_addNetworkLayer(server, ServerNetworkLayerUDP_new(UA_ConnectionConfig_standard, 16664));

	// add a variable node to the adresspace
    UA_Int32 *myInteger = UA_Int32_new();
    *myInteger = 42;
    UA_Variant *myIntegerVariant = UA_Variant_new();
    UA_Variant_setValue(myIntegerVariant, myInteger, UA_TYPES_INT32);
    UA_QualifiedName myIntegerName;
    UA_QUALIFIEDNAME_ASSIGN(myIntegerName, "the answer");
    UA_Server_addVariableNode(server, myIntegerVariant, &UA_NODEID_NULL, &myIntegerName,
                              &UA_NODEID_STATIC(UA_NS0ID_OBJECTSFOLDER,0),
                              &UA_NODEID_STATIC(UA_NS0ID_ORGANIZES,0));

    UA_StatusCode retval = UA_Server_run(server, 1, &running);
	UA_Server_delete(server);

	return retval;
}
