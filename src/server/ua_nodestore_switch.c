#include "ua_nodestore_switch.h"
#include "ua_util.h"

UA_NodestoreSwitch *nodestoreSwitch;

//returns true if NSIndex is ok.
static UA_Boolean //TODO return the nodeStoreIndex instead (-1 for wrong namespaceindex)
checkNSIndex(const UA_UInt16 nsi_index) {
    return (UA_Boolean) (nodestoreSwitch->nodestoreInterfacesSize > nsi_index);
}

UA_NodestoreSwitch *
UA_NodestoreSwitch_new(){
    nodestoreSwitch = UA_malloc(sizeof(UA_NodestoreSwitch));
    nodestoreSwitch->nodestoreInterfacesSize = 0;
    nodestoreSwitch->nodestoreInterfaces = UA_malloc(2*sizeof(UA_NodestoreInterface*));
    return nodestoreSwitch;
}

void
UA_NodestoreSwitch_delete(){
    UA_UInt16 size = nodestoreSwitch->nodestoreInterfacesSize;
    for(UA_UInt16 i = 0; i < size; i++) {
        nodestoreSwitch->nodestoreInterfaces[i]->deleteNodeStore(
        nodestoreSwitch->nodestoreInterfaces[i]->handle);
    }
    UA_free(nodestoreSwitch->nodestoreInterfaces);
    UA_free(nodestoreSwitch);
}


UA_Boolean
UA_NodestoreSwitch_add(UA_NodestoreInterface *nsi) {
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
UA_NodestoreSwitch_change(UA_NodestoreInterface *nsi, UA_UInt16 nsi_index) {
    if(nsi && checkNSIndex(nsi_index)){
        return UA_FALSE;
    }
    nodestoreSwitch->nodestoreInterfaces[nsi_index] = nsi;
    return UA_TRUE;//UA_STATUSCODE_GOOD;
}
UA_NodestoreInterface *
UA_NodestoreSwitch_getNodestoreForNamespace(UA_UInt16 namespaceIndex){
    if(checkNSIndex(namespaceIndex)){
        return nodestoreSwitch->nodestoreInterfaces[namespaceIndex];
    }
    return NULL;
}

/*
 * NodestoreInterface Function routing
 */

UA_Node *
UA_NodestoreSwitch_newNode(UA_NodeClass nodeClass, UA_UInt16 namespaceIndex) {
    if(!checkNSIndex(namespaceIndex)){
        return NULL;
    }
    return nodestoreSwitch->nodestoreInterfaces[namespaceIndex]->newNode(nodeClass);
}

void
UA_NodestoreSwitch_deleteNode(UA_Node *node){
    if(checkNSIndex(node->nodeId.namespaceIndex)){
        nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->deleteNode(node);
    }
}

UA_StatusCode
UA_NodestoreSwitch_insert(UA_Node *node) {
    if(!checkNSIndex(node->nodeId.namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    return nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->insert(
            nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->handle, node);
}
const UA_Node *
UA_NodestoreSwitch_get(const UA_NodeId *nodeId) {
    if(!checkNSIndex(nodeId->namespaceIndex)){
        return NULL;
    }
    return nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->get(
                    nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->handle, nodeId);
}
UA_Node *
UA_NodestoreSwitch_getCopy(const UA_NodeId *nodeId) {
    if(!checkNSIndex(nodeId->namespaceIndex)){
        return NULL;
    }
    return nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->getCopy(
            nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->handle, nodeId);
}
UA_StatusCode
UA_NodestoreSwitch_replace(UA_Node *node) {
    if(!checkNSIndex(node->nodeId.namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    return nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->replace(
            nodestoreSwitch->nodestoreInterfaces[node->nodeId.namespaceIndex]->handle, node);
}
UA_StatusCode
UA_NodestoreSwitch_remove(const UA_NodeId *nodeId) {
    if(!checkNSIndex(nodeId->namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    return nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->remove(
            nodestoreSwitch->nodestoreInterfaces[nodeId->namespaceIndex]->handle, nodeId);
}
void UA_NodestoreSwitch_iterate(UA_Nodestore_nodeVisitor visitor, UA_UInt16 namespaceIndex){
    if(checkNSIndex(namespaceIndex)){
        nodestoreSwitch->nodestoreInterfaces[namespaceIndex]->iterate(
                nodestoreSwitch->nodestoreInterfaces[namespaceIndex]->handle, visitor);
    }
}
