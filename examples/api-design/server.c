#include "open62541.h"
#include "open62541-server.h"
#include "open62541-tcp.h"

int main(int argc, char ** argv) {
	UA_Server *server;
	UA_Server_new(&server);

	#define PORT 1234
	#define MAX_CONNECTIONS 1024
	UA_TcpNetworkLayer_new(&server.configuration.networklayer, PORT, MAX_CONNECTIONS);

	UA_Application *application;
	UA_Application_new(&application, UA_STRING_STATIC("MyApplication"));
	UA_Application_addNamespace(application, 1);
	UA_Server_addApplication(server, application);

	UA_Int32 myInteger = 0;
	UA_NodeId myIntegerNode = {1, UA_NODEIDTYPE_NUMERIC, 50};
	UA_Application_addVariableNode(application, &myIntegerNode, UA_INT32, &myInteger);

	UA_Server_start(server); // runs a loop until shutdown is triggered
	UA_Application_delete(application);
	UA_Server_delete(server);

	return 0;
}
