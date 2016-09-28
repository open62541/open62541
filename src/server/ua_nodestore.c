#include "ua_nodestore.h"
#include "../../include/ua_nodestore_interface.h"
#include "ua_util.h"
//#include "ua_server_internal.h"
//#include "ua_util.h"

UA_NodeStore *nodeStore; //TODO Change signatures -> remove *ns, remove server->nodestore

//returns true if NSIndex is ok.
static UA_Boolean
checkNSIndex(UA_NodeStore *ns, const UA_UInt16 nsi_index) {
	return (UA_Boolean) (ns->nodeStoreInterfacesSize > nsi_index);
}

UA_NodeStore *
UA_NodeStore_new(){
    nodeStore = UA_malloc(sizeof(UA_NodeStore));
    nodeStore->nodeStoreInterfacesSize = 0;
    nodeStore->nodeStoreInterfaces = UA_malloc(2*sizeof(UA_NodeStoreInterface));
    return nodeStore;
}

void
UA_NodeStore_delete(){
    UA_UInt16 size = nodeStore->nodeStoreInterfacesSize;
    for(UA_UInt16 i = 0; i < size; i++) {
    	nodeStore->nodeStoreInterfaces[i]->delete(nodeStore->nodeStoreInterfaces[i]);
    }
    UA_free(nodeStore->nodeStoreInterfaces);
    UA_free(nodeStore);
}


UA_Boolean
UA_NodeStore_add(UA_NodeStore *ns, UA_NodeStoreInterface *nsi) {
    if(!nsi){
        return UA_FALSE;
    }
    size_t size = ns->nodeStoreInterfacesSize;
    UA_NodeStoreInterface** new_nsis =
    UA_realloc(ns->nodeStoreInterfaces,
               sizeof(UA_NodeStoreInterface*) * (size + 1));
    if(!new_nsis) {
        return UA_FALSE;//UA_STATUSCODE_BADOUTOFMEMORY;
    }
    ns->nodeStoreInterfaces = new_nsis;
    ns->nodeStoreInterfaces[size] = nsi;
    ns->nodeStoreInterfacesSize++;
    return UA_TRUE;//UA_STATUSCODE_GOOD;
}

UA_Boolean
UA_NodeStore_change(UA_NodeStore *ns, UA_NodeStoreInterface *nsi, UA_UInt16 nsi_index) {
    if(nsi && checkNSIndex(ns,nsi_index)){
        return UA_FALSE;
    }
    ns->nodeStoreInterfaces[nsi_index] = nsi;
    return UA_TRUE;//UA_STATUSCODE_GOOD;
}

/*
 * NodeStoreInterface Function routing
 */

UA_Node *
UA_NodeStore_newNode(UA_NodeClass nodeClass, UA_UInt16 nodeStoreIndex) {
	if(!checkNSIndex(nodeStore,nodeStoreIndex)){
		return NULL;
	}
    return nodeStore->nodeStoreInterfaces[nodeStoreIndex]->newNode(nodeClass);
}

void
UA_NodeStore_deleteNode(UA_Node *node){
	if(checkNSIndex(nodeStore,node->nodeId.namespaceIndex)){
		nodeStore->nodeStoreInterfaces[node->nodeId.namespaceIndex]->deleteNode(node);
	}
}

UA_StatusCode
UA_NodeStore_insert(UA_NodeStore *ns, UA_Node *node) {
	if(!checkNSIndex(ns,node->nodeId.namespaceIndex)){
	    return UA_STATUSCODE_BADNODEIDUNKNOWN;
	}
    return ns->nodeStoreInterfaces[node->nodeId.namespaceIndex]->insert(
            ns->nodeStoreInterfaces[node->nodeId.namespaceIndex]->handle, node);
}
const UA_Node *
UA_NodeStore_get(UA_NodeStore *ns, const UA_NodeId *nodeId) {
	if(!checkNSIndex(ns,nodeId->namespaceIndex)){
		return NULL;
	}
	return ns->nodeStoreInterfaces[nodeId->namespaceIndex]->get(
	                ns->nodeStoreInterfaces[nodeId->namespaceIndex]->handle, nodeId);
}
UA_Node *
UA_NodeStore_getCopy(UA_NodeStore *ns, const UA_NodeId *nodeId) {
	if(!checkNSIndex(ns,nodeId->namespaceIndex)){
	    return NULL;
	}
    return ns->nodeStoreInterfaces[nodeId->namespaceIndex]->getCopy(
            ns->nodeStoreInterfaces[nodeId->namespaceIndex]->handle, nodeId);
}
UA_StatusCode
UA_NodeStore_replace(UA_NodeStore *ns, UA_Node *node) {
	if(!checkNSIndex(ns,node->nodeId.namespaceIndex)){
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
	}
    return ns->nodeStoreInterfaces[node->nodeId.namespaceIndex]->replace(
            ns->nodeStoreInterfaces[node->nodeId.namespaceIndex]->handle, node);
}
UA_StatusCode
UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeId) {
	if(!checkNSIndex(ns,nodeId->namespaceIndex)){
		return UA_STATUSCODE_BADNODEIDUNKNOWN;
	}
	return ns->nodeStoreInterfaces[nodeId->namespaceIndex]->remove(
	                ns->nodeStoreInterfaces[nodeId->namespaceIndex]->handle, nodeId);
}
