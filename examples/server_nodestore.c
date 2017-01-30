/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Nodestore switch architecture
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * UA_Services            +------------------------------+
 *
 *                        +------------------------------+
 * UA_Nodestore_switch    |  Namespace to Nodestore      |
 *                        +------------------------------+
 *
 * UA_NodeStoreInterfaces +------------+ +------------+
 *                        +------------+ +------------+
 *                        |            | |            |
 * Nodestores             | open62541  | | different  | ...
 *                        | Nodestore  | | Nodestore/ |
 *                        |            | | Repository |
 *                        +------------+ +------------+
 * */
#include <signal.h>

#ifdef UA_NO_AMALGAMATION
#include "ua_types.h"
#include "ua_server.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#include "ua_nodestore_interface.h"
#include "ua_nodestore_standard.h"

#else
#include "open62541.h"
#endif

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

// Define a verry simple nodestore with a cyclic capacity of 100 ObjectNodes as array.
#define NODESTORE_SIZE 100
UA_ObjectNode nodes[NODESTORE_SIZE]; //TODO make nodeStore struct and use as handle --> no static variables in Nodestore functions
size_t nodesCount = 0;
UA_UInt16 nsIdx = 0;

static void Nodestore_delete(UA_ObjectNode* ns){
    for(size_t i=0 ; i < nodesCount && i < NODESTORE_SIZE; i++){
        UA_Node_deleteMembersAnyNodeClass((UA_Node*)&ns[i]);
    }
    nodesCount = 0;
}
static void Nodestore_deleteNode(UA_Node *node){
    UA_Node_deleteMembersAnyNodeClass(node);
    UA_free(node);
}
static UA_Node * Nodestore_newNode(UA_NodeClass nodeClass){
    if(nodeClass != UA_NODECLASS_OBJECT)
        return NULL;
    UA_ObjectNode * node = (UA_ObjectNode*)UA_calloc(1, sizeof(UA_ObjectNode));
    node->nodeClass = UA_NODECLASS_OBJECT;
    return (UA_Node*)node;
}
static const UA_Node * Nodestore_get(UA_ObjectNode *ns, const UA_NodeId *nodeid){
    if(nodeid->identifier.numeric < nodesCount //Node not instanciatet in nodestore
            && (nodeid->identifier.numeric + NODESTORE_SIZE) > nodesCount) //Node already overwritten
        return (UA_Node*) &ns[nodeid->identifier.numeric % NODESTORE_SIZE];
    else
        return NULL;
}
static UA_StatusCode Nodestore_insert(UA_ObjectNode *ns, UA_Node *node, UA_NodeId * addedNodeId){
    if(node->nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_BADINTERNALERROR;
    if( Nodestore_get(ns, &node->nodeId)!= NULL )
        return UA_STATUSCODE_BADNODEIDEXISTS;

    size_t nodeIndex = nodesCount % NODESTORE_SIZE;
    UA_Node_deleteMembersAnyNodeClass((UA_Node*)&ns[nodeIndex]);
    ns[nodeIndex] = *(UA_ObjectNode*)node;
    UA_free(node);
    ns[nodeIndex].nodeId = UA_NODEID_NUMERIC(nsIdx, (UA_UInt32)nodesCount);
    if(addedNodeId)
        UA_NodeId_copy(&ns[nodeIndex].nodeId, addedNodeId);
    nodesCount++;
    return UA_STATUSCODE_GOOD;
}

static UA_Node * Nodestore_getCopy(UA_ObjectNode *ns, const UA_NodeId *nodeid){
    const UA_Node * original = Nodestore_get(ns,nodeid);
    if(!original)
        return NULL;
    UA_ObjectNode * copy = (UA_ObjectNode*)UA_calloc(1, sizeof(UA_ObjectNode));
    copy->nodeClass = UA_NODECLASS_OBJECT;
    if(UA_Node_copyAnyNodeClass(original, (UA_Node*)copy) != UA_STATUSCODE_GOOD){
        return NULL;
    }
    return (UA_Node*)copy;
}
static UA_StatusCode Nodestore_replace(UA_ObjectNode *ns, UA_Node *node){
    if(node->nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    size_t newId = node->nodeId.identifier.numeric;
    if(newId < nodesCount //Node not instanciatet in nodestore
            && (newId + NODESTORE_SIZE) > nodesCount) { //Node already overwritten
        UA_Node_deleteMembersAnyNodeClass((UA_Node*)&ns[newId % NODESTORE_SIZE]);
        ns[newId % NODESTORE_SIZE] = *(UA_ObjectNode*)node;
        UA_free(node);
        return UA_STATUSCODE_GOOD;//(UA_Node*) &ns[nodeid->identifier.numeric % NODESTORE_SIZE];
    }
    else
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
}
static UA_StatusCode Nodestore_remove(UA_ObjectNode *ns, const UA_NodeId *nodeId){
    if( Nodestore_get(ns, nodeId) == NULL ){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    size_t nodeIndex = nodesCount % NODESTORE_SIZE;
    Nodestore_deleteNode((UA_Node*)&ns[nodeIndex]);
    return UA_STATUSCODE_GOOD;
}
//Not implemented/used functions
static void Nodestore_pass(void){};

/* Create new nodestore interface for the example nodestore */
static UA_NodestoreInterface
Nodestore_Example_new(void){
    UA_NodestoreInterface nsi;
    nsi.handle =            &nodes;
    nsi.deleteNodestore =   (UA_NodestoreInterface_deleteNodeStore) Nodestore_delete;
    nsi.newNode =           (UA_NodestoreInterface_newNode)         Nodestore_newNode;
    nsi.deleteNode =        (UA_NodestoreInterface_deleteNode)      Nodestore_deleteNode;
    nsi.insertNode =        (UA_NodestoreInterface_insertNode)      Nodestore_insert;
    nsi.getNode =           (UA_NodestoreInterface_getNode)         Nodestore_get;
    nsi.getNodeCopy =       (UA_NodestoreInterface_getNodeCopy)     Nodestore_getCopy;
    nsi.replaceNode =       (UA_NodestoreInterface_replaceNode)     Nodestore_replace;
    nsi.removeNode =        (UA_NodestoreInterface_removeNode)      Nodestore_remove;

    nsi.releaseNode =       (UA_NodestoreInterface_releaseNode)     Nodestore_pass;
    nsi.iterate =           (UA_NodestoreInterface_iterate)         Nodestore_pass;
    nsi.linkNamespace =     (UA_NodestoreInterface_linkNamespace)   Nodestore_pass;
    nsi.unlinkNamespace =   (UA_NodestoreInterface_unlinkNamespace) Nodestore_pass;
    return nsi;
}

int main(void) {
    signal(SIGINT,  stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl;
    nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;

    //Set the standard nodestore from userland as nodestore for default configured namespace1
    UA_NodestoreInterface nsi = UA_Nodestore_standard();
    config.namespaces[1].nodestore = &nsi;

    UA_Server *server = UA_Server_new(config);

    //Add a new namespace with same nodestore
    UA_Namespace* namespace2 = UA_Namespace_newFromChar("Namespace2_Nodestore_standard");
    namespace2->nodestore = &nsi;
    if(UA_Server_addNamespace_full(server, namespace2) == UA_STATUSCODE_GOOD){
        //Add a test node
        UA_VariableAttributes myVar;
        UA_VariableAttributes_init(&myVar);
        myVar.description = UA_LOCALIZEDTEXT("en_US", "This is a node of the standard nodestore in namespace 2 and resides in nsi.");
        myVar.displayName = UA_LOCALIZEDTEXT("en_US", "Testnode_Namespace2");
        myVar.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        UA_Int32 myInteger = 42;
        UA_Variant_setScalarCopy(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
        const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(namespace2->index, "Testnode_Namespace2");
        const UA_NodeId myIntegerNodeId = UA_NODEID_NUMERIC(namespace2->index, 0);
        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
            myIntegerName, UA_NODEID_NULL, myVar, NULL, NULL);
        UA_Variant_deleteMembers(&myVar.value);
    }
//    //Add a namespace to test the add/delete namespace function
//    UA_Server_addNamespace(server, "TestAddDeleteNamespace");

    //Add the example nodestore
    UA_NodestoreInterface nsi2 = Nodestore_Example_new();
    UA_Namespace* namespace3 = UA_Namespace_newFromChar("Namespace3_Nodestore_example");
    namespace3->nodestore = &nsi2;
    if(UA_Server_addNamespace_full(server, namespace3) == UA_STATUSCODE_GOOD){
        nsIdx = namespace3->index;
        //Create a new node and reference it
        UA_ObjectAttributes object_attr;
        UA_ObjectAttributes_init(&object_attr);
        object_attr.description =
                UA_LOCALIZEDTEXT("en_US","This is the root node of the nodestore example and resides in nsi2.");
        object_attr.displayName =
                UA_LOCALIZEDTEXT("en_US","RootNode_Nodestore_Example");
        UA_NodeId newNodeId;
        UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(nsIdx,0),
                UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(nsIdx,"RootNode"),
                UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, &newNodeId);

        UA_ObjectAttributes_init(&object_attr);
        object_attr.description =
                UA_LOCALIZEDTEXT("en_US","This is the node1 of the nodestore example.");
        object_attr.displayName =
                UA_LOCALIZEDTEXT("en_US","Node1_Nodestore_Example");
        UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(nsIdx,1),
                newNodeId,
                UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(nsIdx,"Node1"),
                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), object_attr, NULL, NULL);
    }

    //Delete the namespace for the test of add/delete namespace function
//    UA_Server_deleteNamespace(server, "TestAddDeleteNamespace");
//    if(UA_Server_addNamespace_full(server,namespace3) == UA_STATUSCODE_GOOD){
//        nsIdx = namespace3->index;
//    }

    //Run and stop the server
    UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    UA_Nodestore_standard_delete(&nsi);
    namespace2->nodestore = NULL;
    namespace3->nodestore = NULL;
    UA_Namespace_deleteMembers(namespace2);
    UA_Namespace_deleteMembers(namespace3);
    UA_free(namespace2);
    UA_free(namespace3);
    return 0;
}
