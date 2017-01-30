#include "ua_nodestore_switch.h"
#include "ua_util.h"
#include "ua_server_internal.h"

static UA_Boolean checkNSIndex(UA_Server* server, UA_UInt16 nsIdx){
    return (UA_Boolean) (nsIdx < (UA_UInt16)server->namespacesSize);
}

/*
 * NodestoreInterface Function routing
 */

UA_Node *
UA_NodestoreSwitch_newNode(UA_Server* server, UA_NodeClass nodeClass, UA_UInt16 namespaceIndex) {
    if(!checkNSIndex(server, namespaceIndex)){
        return NULL;
    }
    return server->namespaces[namespaceIndex].nodestore->newNode(nodeClass);
}

void
UA_NodestoreSwitch_deleteNode(UA_Server* server, UA_Node *node){
    if(checkNSIndex(server, node->nodeId.namespaceIndex)){
        server->namespaces[node->nodeId.namespaceIndex].nodestore->deleteNode(node);
    }
}

UA_StatusCode
UA_NodestoreSwitch_insertNode(UA_Server* server, UA_Node *node,
        UA_NodeId *addedNodeId) {
    if(!checkNSIndex(server, node->nodeId.namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    return server->namespaces[node->nodeId.namespaceIndex].nodestore->insertNode(
            server->namespaces[node->nodeId.namespaceIndex].nodestore->handle, node, addedNodeId);
}
const UA_Node *
UA_NodestoreSwitch_getNode(UA_Server* server, const UA_NodeId *nodeId) {
    if(!checkNSIndex(server, nodeId->namespaceIndex)){
        return NULL;
    }
    return server->namespaces[nodeId->namespaceIndex].nodestore->getNode(
            server->namespaces[nodeId->namespaceIndex].nodestore->handle, nodeId);
}
UA_Node *
UA_NodestoreSwitch_getNodeCopy(UA_Server* server, const UA_NodeId *nodeId) {
    if(!checkNSIndex(server, nodeId->namespaceIndex)){
        return NULL;
    }
    return server->namespaces[nodeId->namespaceIndex].nodestore->getNodeCopy(
            server->namespaces[nodeId->namespaceIndex].nodestore->handle, nodeId);
}
UA_StatusCode
UA_NodestoreSwitch_replaceNode(UA_Server* server, UA_Node *node) {
    if(!checkNSIndex(server, node->nodeId.namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    return server->namespaces[node->nodeId.namespaceIndex].nodestore->replaceNode(
            server->namespaces[node->nodeId.namespaceIndex].nodestore->handle, node);
}
UA_StatusCode
UA_NodestoreSwitch_removeNode(UA_Server* server, const UA_NodeId *nodeId) {
    if(!checkNSIndex(server, nodeId->namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    return server->namespaces[nodeId->namespaceIndex].nodestore->removeNode(
            server->namespaces[nodeId->namespaceIndex].nodestore->handle, nodeId);
}
void UA_NodestoreSwitch_releaseNode(UA_Server* server, const UA_Node *node){
    if(node && checkNSIndex(server, node->nodeId.namespaceIndex)){
        server->namespaces[node->nodeId.namespaceIndex].nodestore->releaseNode(
                server->namespaces[node->nodeId.namespaceIndex].nodestore->handle, node);
    }
}
