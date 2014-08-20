#include "open62541.h"
#include "open62541-server.h"
#include "open62541-tcp.h"

#include "open62541-ns0-pico.h" // UA_NamespaceZero_Static

#define PORT 1234
#define MAX_CONNECTIONS 1024

int main(int argc, char ** argv) {
	// Set up UA_Application
	UA_Application *application;
	UA_Application_new(&application);

	// Set up namespace Zero and typical application parameters
	UA_Application_addNamespace(application, 0, &UA_NamespaceZero_Static);

	UA_ApplicationDescription *applicationDescription;
	UA_Application_new(&applicationDescription);
	UA_ApplicationDescription_setApplicationName("Application");
	UA_ApplicationDescription_setApplicationUri("http://open62541.org/api-design/");
	UA_Application_setVariableNodeNS0(application, UA_APPLICATIONDESCRIPTION_NS0, applicationDescription);

	// Set up application specific namespace
	UA_Application_addNamespace(application, 1, UA_NULL);

	UA_Int32 myInteger = 0;
	UA_NodeId myIntegerNode = {1, UA_NODEIDTYPE_NUMERIC, 50};
	UA_Application_addVariableNode(application, &myIntegerNode, UA_INT32, &myInteger);

	// Set up server with network layer and add application
	UA_Server *server;
	UA_Server_new(&server);
	UA_TcpNetworkLayer_new(&server.configuration.networklayer, PORT, MAX_CONNECTIONS);
	UA_Server_addApplication(server, application);

	// Run server
	UA_Server_start(server);

	// Clean up (? first server then application ?)
	UA_Server_delete(server);
	UA_Application_delete(application);

	return 0;
}
