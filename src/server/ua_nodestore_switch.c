#include "ua_nodestore_switch.h"
#include "ua_util.h"

//returns true if NSIndex is ok.
static UA_Boolean //TODO return the nodeStoreIndex instead (-1 for wrong namespaceindex)
checkNSIndex(UA_NodestoreSwitch* nodestoreSwitch, const UA_UInt16 nsi_index) {
    return (UA_Boolean) (nodestoreSwitch->nodestoreInterfacesSize > nsi_index);
}

UA_NodestoreSwitch *
UA_NodestoreSwitch_new(){
    UA_NodestoreSwitch* nodestoreSwitch = UA_malloc(sizeof(UA_NodestoreSwitch));
    nodestoreSwitch->nodestoreInterfacesSize = 0;
    nodestoreSwitch->nodestoreInterfaces = NULL;
    return nodestoreSwitch;
}

void
UA_NodestoreSwitch_delete(UA_NodestoreSwitch* nodestoreSwitch){
    UA_UInt16 size = nodestoreSwitch->nodestoreInterfacesSize;
    for(UA_UInt16 i = 0; i < size; i++) {
        nodestoreSwitch->nodestoreInterfaces[i]->deleteNodeStore(
        nodestoreSwitch->nodestoreInterfaces[i]->handle);
    }
    UA_free(nodestoreSwitch->nodestoreInterfaces);
    UA_free(nodestoreSwitch);
}


UA_Boolean
UA_NodestoreSwitch_add(UA_NodestoreSwitch* nodestoreSwitch, UA_NodestoreInterface *nsi) {
    if(!nsi){
        return UA_FALSE;
    }
    size_t size = nodestoreSwitch->nodestoreInterfacesSize;
    UA_NodestoreInterface** new_nsis =
            UA_realloc(nodestoreSwitch->nodestoreInterfaces,
               sizeof(UA_NodestoreInterface*) * (size + 1));
    if(!new_nsis) {
        return UA_FALSE;//UA_STATUSCODE_BADOUTOFMEMORY;
    }
    nodestoreSwitch->nodestoreInterfaces = new_nsis;
    nodestoreSwitch->nodestoreInterfaces[size] = nsi;
    nodestoreSwitch->nodestoreInterfacesSize++;
    return UA_TRUE;//UA_STATUSCODE_GOOD;
}

UA_Boolean
UA_NodestoreSwitch_change(UA_NodestoreSwitch* nodestoreSwitch, UA_NodestoreInterface *nsi, UA_UInt16 nsi_index) {
    if(nsi && checkNSIndex(nodestoreSwitch, nsi_index)){
        return UA_FALSE;
    }
    nodestoreSwitch->nodestoreInterfaces[nsi_index] = nsi;
    return UA_TRUE;//UA_STATUSCODE_GOOD;
}
UA_NodestoreInterface *
UA_NodestoreSwitch_getNodestoreForNamespace(UA_NodestoreSwitch* nodestoreSwitch, UA_UInt16 namespaceIndex){
    if(checkNSIndex(nodestoreSwitch, namespaceIndex)){
        return nodestoreSwitch->nodestoreInterfaces[namespaceIndex];
    }
    return NULL;
}

/*
 * NodestoreInterface Function routing
 */

UA_Node *
UA_NodestoreSwitch_newNode(UA_NodestoreSwitch* nodestoreSwitch, UA_NodeClass nodeClass, UA_UInt16 namespaceIndex) {
    if(!checkNSIndex(nodestoreSwitch, namespaceIndex)){
        return NULL;
    }
    return nodestoreSwitch->nodestoreInterfaces[namespaceIndex]->newNode(nodeClass);
}

void
UA_NodestoreSwitch_deleteNode(UA_NodestoreSwitch* nodestoreSwitch, UA_Node *node){
    if(checkNSIndex(nodestoreSwitch, node->nodeId.namespaceIndex)){
        nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->deleteNode(node);
    }
}

UA_StatusCode
UA_NodestoreSwitch_insert(UA_NodestoreSwitch* nodestoreSwitch, UA_Node *node,
        const UA_NodeId *parentNodeId, UA_NodeId *addedNodeId) {
    if(!checkNSIndex(nodestoreSwitch, node->nodeId.namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    return nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->insert(
            nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->handle, node, parentNodeId, addedNodeId);
}
const UA_Node *
UA_NodestoreSwitch_get(UA_NodestoreSwitch* nodestoreSwitch, const UA_NodeId *nodeId) {
    if(!checkNSIndex(nodestoreSwitch, nodeId->namespaceIndex)){
        return NULL;
    }
    return nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->get(
                    nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->handle, nodeId);
}
UA_Node *
UA_NodestoreSwitch_getCopy(UA_NodestoreSwitch* nodestoreSwitch, const UA_NodeId *nodeId) {
    if(!checkNSIndex(nodestoreSwitch, nodeId->namespaceIndex)){
        return NULL;
    }
    return nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->getCopy(
            nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->handle, nodeId);
}
UA_StatusCode
UA_NodestoreSwitch_replace(UA_NodestoreSwitch* nodestoreSwitch, UA_Node *node) {
    if(!checkNSIndex(nodestoreSwitch, node->nodeId.namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    return nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->replace(
            nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->handle, node);
}
UA_StatusCode
UA_NodestoreSwitch_remove(UA_NodestoreSwitch* nodestoreSwitch, const UA_NodeId *nodeId) {
    if(!checkNSIndex(nodestoreSwitch, nodeId->namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    return nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->remove(
            nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->handle, nodeId);
}
void UA_NodestoreSwitch_iterate(UA_NodestoreSwitch* nodestoreSwitch, UA_Nodestore_nodeVisitor visitor, UA_UInt16 namespaceIndex){
    if(checkNSIndex(nodestoreSwitch, namespaceIndex)){
        nodestoreSwitch->nodestoreInterfaces[namespaceIndex]->iterate(
                nodestoreSwitch->nodestoreInterfaces[namespaceIndex]->handle, visitor);
    }
}
void UA_NodestoreSwitch_release(UA_NodestoreSwitch* nodestoreSwitch, const UA_Node *node){
    if(node && checkNSIndex(nodestoreSwitch, node->nodeId.namespaceIndex)){
        nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->release(
                nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->handle, node);
    }
}
