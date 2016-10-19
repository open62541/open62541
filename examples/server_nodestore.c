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
#include "ua_nodes.h"

#else
#include "open62541.h"
#endif

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

UA_ObjectNode nodes[100];
int nodesCount = 0;
UA_UInt16 nsIdx = 0;
static void Nodestore_delete(UA_ObjectNode *nodestore){

}
static UA_Node * Nodestore_newNode(UA_NodeClass nodeClass){
    if(nodeClass != UA_NODECLASS_OBJECT)
        return NULL;
    nodes[nodesCount].nodeId = UA_NODEID_NUMERIC(nsIdx, (UA_UInt32)nodesCount);
    nodes[nodesCount].nodeClass = UA_NODECLASS_OBJECT;
    return (UA_Node*) &nodes[nodesCount++];
}
static void Nodestore_deleteNode(UA_Node *node){
}
static UA_StatusCode Nodestore_insert(UA_ObjectNode *ns, UA_Node *node){
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}
static const UA_Node * Nodestore_get(UA_ObjectNode *ns, const UA_NodeId *nodeid){
    if(nodeid->identifier.numeric < 100)
        return (UA_Node*) &ns[nodeid->identifier.numeric];
    else
        return NULL;
}
static UA_StatusCode Nodestore_replace(UA_ObjectNode *ns, UA_Node *node){
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}
static UA_StatusCode Nodestore_remove(UA_ObjectNode *ns, const UA_NodeId *nodeid){
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

/* Create new nodestore as array for ObjectNodes */
static UA_NodestoreInterface
Nodestore_Example_new(void){
    UA_NodestoreInterface nsi;
    nsi.handle =        &nodes;
    nsi.deleteNodeStore =        (UA_NodestoreInterface_delete)      Nodestore_delete;
    nsi.newNode =       (UA_NodestoreInterface_newNode)     Nodestore_newNode;
    nsi.deleteNode =    (UA_NodestoreInterface_deleteNode)  Nodestore_deleteNode;
    nsi.insert =        (UA_NodestoreInterface_insert)      Nodestore_insert;
    nsi.get =           (UA_NodestoreInterface_get)         Nodestore_get;
    nsi.getCopy =       (UA_NodestoreInterface_getCopy)     Nodestore_get;
    nsi.replace =       (UA_NodestoreInterface_replace)     Nodestore_replace;
    nsi.remove =        (UA_NodestoreInterface_remove)      Nodestore_remove;
    //nsi.iterate =       (UA_NodestoreInterface_iterate)     Nodestore_iterate;
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

    //Add the default NodeStore as Nodestore for NS0 and NS1
    UA_NodestoreInterface nsi = UA_Nodestore_standard();
    config.nodestore0 = &nsi;
    config.nodestore1 = &nsi;
    UA_Server *server = UA_Server_new(config);

    //Add a new namespace with same nodestore
    UA_Server_addNamespace_Nodestore(server, "Namespace3Nodestore_standard",&nsi);

    //Add another std nodestore
    UA_NodestoreInterface nsi2 = Nodestore_Example_new();
    nsIdx = UA_Server_addNamespace_Nodestore(server, "Namespace4Nodestore_example",&nsi2);
    //Create a new node and reference it
    UA_Node * rootNode = Nodestore_newNode(UA_NODECLASS_OBJECT);
    rootNode->browseName = UA_QUALIFIEDNAME(nsIdx,"RootNode");
    rootNode->displayName = UA_LOCALIZEDTEXT("en_US","RootNode_Nodestore_Example");
    rootNode->description = UA_LOCALIZEDTEXT("en_US","This is the root node of the nodestore example.");
    UA_Server_addReference(server,
            UA_NODEID_NUMERIC(0,UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0,UA_NS0ID_ORGANIZES),
            UA_EXPANDEDNODEID_NUMERIC(nsIdx,0),
            true);

    //Run and stop the server
    UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    UA_Nodestore_standard_delete(&nsi);
    return 0;
}
