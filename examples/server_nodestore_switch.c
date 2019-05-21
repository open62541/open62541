/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Using the nodestore switch plugin
 * ------------------------
 * Is only compiled with: UA_ENABLE_CUSTOM_NODESTORE and UA_ENABLE_NODESTORE_SWITCH
 * 
 * Adds the nodestore switch as plugin into the server and demonstrates its use with a second default nodestore.
 *
 * The nodestore switch links namespace indices to nodestores. So that every access to a node is redirected, based on its namespace index.
 * The linking between nodestores and and namespaces may be altered during runtime (With the use of UA_Server_run_iterate for example).
 * This enables the user to have a persistent and different stores for nodes (for example in a database or file) or to transform objects into OPC UA nodes.
 * Backup scenarios are also possible.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/nodestore_switch.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

static const UA_NodeId baseDataVariableType = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_BASEDATAVARIABLETYPE}};


static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    /* add a second default nodestore for NS1 */
    // Get the nodestore switch from the server. (Only possible with UA_ENABLE_CUSTOM_NODESTORE)
    // Can be casted to a nodestoreSwitch if UA_ENABLE_NODESTORE_SWITCH is set, so that the nodestore switch is used as plugin
    UA_Nodestore_Switch* nodestoreSwitch = (UA_Nodestore_Switch*) UA_Server_getNodestore(server);
    // Create a default nodestore as an own nodestore for namespace 1 (application namespace) and remember a pointer to its interface
    UA_NodestoreInterface * ns1Nodestore = NULL;
	UA_StatusCode retval = UA_Nodestore_Default_Interface_new(&ns1Nodestore);
	if(retval != UA_STATUSCODE_GOOD){
	    UA_Server_delete(server);
	    return EXIT_FAILURE;
	}
	// Link the ns1Nodestore to namespace 1, so that all nodes created in namespace 1 reside in ns1Nodestore
	UA_Nodestore_Switch_linkNodestoreToNamespace(nodestoreSwitch,ns1Nodestore, 1);

    /* add a static variable node to the server */
    UA_VariableAttributes myVar = UA_VariableAttributes_default;
    myVar.description = UA_LOCALIZEDTEXT("en-US", "This node lives in a second default nodestore.");
    myVar.displayName = UA_LOCALIZEDTEXT("en-US", "A node in nodestore 1");
    myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    myVar.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    myVar.valueRank = UA_VALUERANK_SCALAR;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "A node in nodestore 1");
    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "Nodestore1Node");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
                              myIntegerName, baseDataVariableType, myVar, NULL, NULL);

    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
