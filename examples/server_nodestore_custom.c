/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/*
 * Using the nodestore switch plugin to add a very simple custom nodestore
 * ------------------------
 * Is only compiled with: UA_ENABLE_CUSTOM_NODESTORE and UA_ENABLE_NODESTORE_SWITCH
 * 
 * Adds the nodestore switch as plugin into the server and demonstrates its use with a very simple custom nodestore.
 *
 * nodestore custom:
 * The UA_ENABLE_CUSTOM_NODESTORE compiler option undefines the default nodestore.
 * Without the nodestore switch a custom nodestore has to be implmented, that is capable of all node classes, data types, (multithreading).
 * Even the whole ns0 has to be loaded in it during server startup.
 * So it is easier to use the nodestore switch (which includes a copy of the default nodestore plugin).
 *
 * nodestore switch:
 * The nodestore switch links namespace indices to nodestores. So that every access to a node is redirected, based on its namespace index.
 * The linking between nodestores and and namespaces may be altered during runtime (With the use of UA_Server_run_iterate for example).
 * This enables the user to have persistent and different stores for nodes (for example in a database or file) or to transform objects into OPC UA nodes.
 * Backup scenarios or single nodestores for multiple UA Servers are also possible.
 *
 * nodestore interface:
 * For the nodestore functions of a nodestore interface see open62541/plugin/nodestore_switch.h and open62541/plugin/nodestore.h
 * All nodestore interface functions are implemented in this example, but its not multithreading safe.
 * If you don't need to modify the nodes via opc ua clients, only the getNode function is required.
 * So a even simpler nodestore is possible, that only gets the nodes from a static array an for example refreshes its value attribute on a get (or with a value callback function).
 */

//TODO move myNodestore to separate file?
//TODO fix in open62541: in addNode --> If no requestNewNodeId is specified --> Use NS1 instead of NS0

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/nodestore_switch.h>
#include <open62541/util.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

static const UA_NodeId baseDataVariableType = { 0, UA_NODEIDTYPE_NUMERIC, {
		UA_NS0ID_BASEDATAVARIABLETYPE } };

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
	running = false;
}

/*
 * Very simple, minimal nodestore for some nodes in an static array
 */
#define myNodestoreSize 10
UA_UInt16 myNodestoreNSIndex = 2; //TODO save in nsCtx and add as parameter to myNodestore_new

static UA_StatusCode myNodestore_new(void **nsCtx) {
	//Mark all nodes as null, by setting ther nodeid to Null and initializing via calloc
	UA_VariableNode * myNodestore = (UA_VariableNode *) UA_calloc(myNodestoreSize, sizeof(UA_VariableNode)); //TODO Maybe its easier to safe pointers to nodes.
	if(myNodestore == NULL){
		*nsCtx = NULL;
		return UA_STATUSCODE_BADOUTOFMEMORY;
	}
	//Mark nodeclass of all nodes as VariableNode
	for(size_t i = 0 ; i < myNodestoreSize ; ++i){
		myNodestore[i].nodeClass = UA_NODECLASS_VARIABLE;
	}
	*nsCtx = (void*) myNodestore;
	return UA_STATUSCODE_GOOD;
}

static void myNodestore_delete(void *nsCtx) {
	UA_VariableNode * myNodestore = (UA_VariableNode*) nsCtx;
	for(size_t i = 0 ; i < myNodestoreSize ; ++i){
		UA_Node_deleteMembers((UA_Node*)&myNodestore[i]);
	}
}

static UA_Node *myNodestore_newNode(void *nsCtx, UA_NodeClass nodeClass) {
	if(nodeClass != UA_NODECLASS_VARIABLE)
		return NULL;
	UA_Node* node = (UA_Node*) UA_calloc(1, sizeof(UA_VariableNode));
	if(node)
		node->nodeClass = UA_NODECLASS_VARIABLE;
	return node;
}

static void myNodestore_deleteNode(void *nsCtx, UA_Node *node) {
	UA_Node_deleteMembers(node);
	UA_free(node);
}

static const UA_Node *myNodestore_getNode(void *nsCtx, const UA_NodeId *nodeId) {
	UA_VariableNode * myNodestore = (UA_VariableNode*) nsCtx;
	if(nodeId->identifierType != UA_NODEIDTYPE_NUMERIC ||
			nodeId->identifier.numeric >= (UA_UInt32) myNodestoreSize)
		return NULL;
	if(UA_NodeId_isNull(&myNodestore[nodeId->identifier.numeric].nodeId))
		return NULL;
	return (UA_Node*) &myNodestore[nodeId->identifier.numeric];
}

static void myNodestore_releaseNode(void *nsCtx, const UA_Node *node) {
	//Not needed, as this nodestore is not multithreading safe and has no inPlaceEditAllowed.
	//Otherwise this function can be used to count references and free resources.
}

static UA_StatusCode myNodestore_getNodeCopy(void *nsCtx,
				const UA_NodeId *nodeId, UA_Node **outNode) {
	const UA_Node* original = myNodestore_getNode(nsCtx, nodeId);
	if(original == NULL)
		return UA_STATUSCODE_BADNODEIDUNKNOWN;
	*outNode = myNodestore_newNode(nsCtx, UA_NODECLASS_VARIABLE);
	return UA_Node_copy(original, *outNode);
}

static UA_StatusCode myNodestore_insertNode(void *nsCtx, UA_Node *node,
		UA_NodeId *addedNodeId) {
	UA_VariableNode * myNodestore = (UA_VariableNode*) nsCtx;
	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	//Check if nodeId is specified
	if(!UA_NodeId_isNull(&node->nodeId)){
		UA_UInt32 index = node->nodeId.identifier.numeric;
		//Check that given nodeId is in range
		if(node->nodeId.identifierType != UA_NODEIDTYPE_NUMERIC ||
				index >= (UA_UInt32) myNodestoreSize)
			return UA_STATUSCODE_BADNODEIDINVALID;
		//Check that given nodeId is not already used
		if(UA_NodeId_isNull(&myNodestore[index].nodeId)){
			if(addedNodeId != NULL)
				*addedNodeId = UA_NODEID_NUMERIC(myNodestoreNSIndex, index);
			retval = UA_Node_copy(node, (UA_Node*) &myNodestore[index]);
			UA_Node_deleteMembers(node);
			return retval;
		}
	}
	//Find a free slot to insert node
	for(size_t i = 0 ; i < myNodestoreSize ; ++i){
		if(UA_NodeId_isNull(&myNodestore[i].nodeId)){
			node->nodeId = UA_NODEID_NUMERIC(myNodestoreNSIndex, (UA_UInt32)i);
			if(addedNodeId != NULL)
				UA_NodeId_copy(&node->nodeId ,addedNodeId);
			retval = UA_Node_copy(node, (UA_Node*) &myNodestore[i]);
			UA_Node_deleteMembers(node);
			return retval;
		}
	}
    retval |= UA_STATUSCODE_BADOUTOFMEMORY;
    return retval;
}

static UA_StatusCode myNodestore_removeNode(void *nsCtx,
		const UA_NodeId *nodeId) {
	UA_VariableNode * myNodestore = (UA_VariableNode*) nsCtx;
	if(nodeId->identifierType != UA_NODEIDTYPE_NUMERIC ||
			nodeId->identifier.numeric >= (UA_UInt32) myNodestoreSize)
		return UA_STATUSCODE_BADNODEIDINVALID;
	if(UA_NodeId_isNull(&myNodestore[nodeId->identifier.numeric].nodeId))
		return UA_STATUSCODE_BADNODEIDUNKNOWN;
	UA_Node_deleteMembers((UA_Node*)&myNodestore[nodeId->identifier.numeric]);
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode myNodestore_replaceNode(void *nsCtx, UA_Node *node) {
	UA_StatusCode retval = myNodestore_removeNode(nsCtx, &node->nodeId);
	if(retval == UA_STATUSCODE_GOOD){
		return myNodestore_insertNode(nsCtx, node, NULL);
	}
	return retval;
}

static void myNodestore_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
		void *visitorCtx) {
	UA_VariableNode * myNodestore = (UA_VariableNode*) nsCtx;
	for(size_t i = 0 ; i < myNodestoreSize ; ++i){
		if(!UA_NodeId_isNull(&myNodestore[i].nodeId))
				visitor(visitorCtx, (UA_Node*) &myNodestore[i]);
	}
}

/*
 * Print the node id to the std. log.
 * Has the signature of a nodestore visitor to easily iterate over all nodes in the nodestore.
 */
static void printNode(void* visitorContext, const UA_Node* node) {
	UA_String nodeIdString = UA_STRING_NULL;
	UA_StatusCode result = UA_NodeId_toString(&node->nodeId, &nodeIdString);
	if (result != UA_STATUSCODE_GOOD) {
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"Could not convert nodeId.");
	} else {
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "%s%.*s",
				(char*) visitorContext, UA_PRINTF_STRING_DATA(nodeIdString));
		UA_String_deleteMembers(&nodeIdString);
	}
}

/* add a variable node to the server via public server api*/
static void addVariableNodeExternal(UA_Server* server, UA_UInt16 nsIndex, char* name) {
	UA_VariableAttributes myVar = UA_VariableAttributes_default;
	myVar.description = UA_LOCALIZEDTEXT("en-US",
			"This node was added via public server api.");
	myVar.displayName = UA_LOCALIZEDTEXT("en-US", name);
	myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	myVar.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
	myVar.valueRank = UA_VALUERANK_SCALAR;
	UA_Int32 myInteger = 42;
	UA_Variant_setScalar(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
	const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(nsIndex, name);
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(myNodestoreNSIndex, 0), parentNodeId,
			parentReferenceNodeId, myIntegerName, baseDataVariableType, myVar,
			NULL, NULL);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Added a node via public server api.");
}

/*
 * Add a variable node to the nodestore via its nodestore interface
 * Be careful, this skips all checks, like parent references, types, attribute consistencies.
 *
 * It is also possible to get the nsCtx from the server pointer via
 * UA_Server_getNodestore and UA_Nodestore_Switch_getNodestore.
 */
static void addVariableNodeInteral(UA_Server* server, void* nsCtx, char* name) {
	UA_VariableNode* node = (UA_VariableNode*) myNodestore_newNode(nsCtx, UA_NODECLASS_VARIABLE);
	/* names */
	node->browseName = UA_QUALIFIEDNAME_ALLOC(myNodestoreNSIndex, name);
	node->description = UA_LOCALIZEDTEXT_ALLOC("en-US",
			"This node was added to the nodestore directly.");
	node->displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", name);
    /* Constraints on possible values */
	node->dataType = UA_TYPES[UA_TYPES_INT32].typeId;
	node->valueRank = UA_VALUERANK_SCALAR;
	/* value */
	UA_DataValue_init(&node->value.data.value);
	UA_Int32 myInteger = 42;
	UA_Variant_setScalarCopy(&node->value.data.value.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
	node->value.data.value.hasValue = UA_TRUE;
	node->value.data.value.sourceTimestamp = UA_DateTime_now();
	/* access */
	node->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	node->writeMask = UA_WRITEMASK_VALUEFORVARIABLETYPE;

	/* Insert node into nodestore */
	UA_NodeId newNodeId = UA_NODEID_NULL;
	myNodestore_insertNode(nsCtx,(UA_Node*) node, &newNodeId);
	/* Check that node was added and print its node id */
	const UA_Node* checkNode = myNodestore_getNode(nsCtx, &newNodeId);
	if(checkNode != NULL)
		printNode("Added a node via nodestore api:", checkNode);

	/*
	 * Add references via public server API
	 * ------------------------
 	 * Alternative: Use UA_AddReferencesItem and UA_Node_addReference(node, &ref)
	 * Skips checks, but uses less nodestore accesses.
	 * Often needs other nodestore interfaces for inverse references,
	 * which can be solved via the nodestore switch interface instead of the server
	 */
	//Parent reference
	UA_Server_addReference(server, newNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
			UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_FALSE);
	//Type definition reference
	UA_Server_addReference(server, newNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
			UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), UA_TRUE);
}

/*
 * Main
 */

int main(void) {
	signal(SIGINT, stopHandler);
	signal(SIGTERM, stopHandler);
	UA_StatusCode retval = UA_STATUSCODE_GOOD; //TODO check retvalues

	UA_Server *server = UA_Server_new();
	UA_ServerConfig_setDefault(UA_Server_getConfig(server));

	/*
	 * add myNodestore to the server for NS2
	 * Get the nodestore switch from the server. (Only possible with UA_ENABLE_CUSTOM_NODESTORE)
	 * Can be casted to a nodestoreSwitch if UA_ENABLE_NODESTORE_SWITCH is set, so that the nodestore switch is used as plugin
	 */
	UA_Nodestore_Switch* nodestoreSwitch =
			(UA_Nodestore_Switch*) UA_Server_getNodestore(server);

	/* Create a nodestore interface for myNodestore to easily plug it into the nodestore switch */
	UA_NodestoreInterface * myNodestoreInterface = (UA_NodestoreInterface *) UA_malloc(sizeof(UA_NodestoreInterface));
	myNodestoreInterface->deleteNode = myNodestore_deleteNode;
	myNodestoreInterface->deleteNodestore = myNodestore_delete;
	myNodestoreInterface->getNode = myNodestore_getNode;
	myNodestoreInterface->getNodeCopy = myNodestore_getNodeCopy;
	myNodestoreInterface->insertNode = myNodestore_insertNode;
	myNodestoreInterface->iterate = myNodestore_iterate;
	myNodestoreInterface->newNode = myNodestore_newNode;
	myNodestoreInterface->releaseNode = myNodestore_releaseNode;
	myNodestoreInterface->removeNode = myNodestore_removeNode;
	myNodestoreInterface->replaceNode = myNodestore_replaceNode;
	/* Create a new nodestore and save its context in the nodestore interface.
	 * The nodestore context is passed to every nodestore function. */
	myNodestore_new((void**)&myNodestoreInterface->context);

	// Make the server aware of namespace 2
	myNodestoreNSIndex = UA_Server_addNamespace(server, "MyNamespace");
	// Link the myNodestore to namespace 2, so that all nodes created in namespace 2 reside in myNodestore
	UA_Nodestore_Switch_setNodestore(nodestoreSwitch, myNodestoreNSIndex, myNodestoreInterface);

	// Add some test nodes to namespace 2
	addVariableNodeExternal(server, myNodestoreNSIndex, "TestNode1");
	addVariableNodeExternal(server, myNodestoreNSIndex, "TestNode2");
	addVariableNodeInteral(server, myNodestoreInterface->context, "TestNode3");
	addVariableNodeInteral(server, myNodestoreInterface->context, "TestNode4");

	// Print all nodes in the nodestore
	myNodestoreInterface->iterate(myNodestoreInterface->context,
			(UA_NodestoreVisitor) printNode, "Found Node in NS2: ");

	// Start server and run till SIGINT or SIGTERM
	UA_Server_run(server, &running);

	/* Delete the server including all nodestores */
	UA_Server_delete(server);

	return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
