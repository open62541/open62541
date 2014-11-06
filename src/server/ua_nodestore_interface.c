#include "ua_nodestore_interface.h"
#include "ua_util.h"
#include "ua_statuscodes.h"



UA_StatusCode UA_NodeStoreInterface_registerAddNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_addNodes addNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->addNodes = addNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerAddReferencesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_addReferences addReferences)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->addReferences = addReferences;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStoreInterface_registerDeleteNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_deleteNodes deleteNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->deleteNodes = deleteNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStoreInterface_registerDeleteReferencesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_deleteReferences deleteReferences)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->deleteReferences = deleteReferences;
	return UA_STATUSCODE_GOOD;
}


UA_StatusCode UA_NodeStoreInterface_registerReadNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_readNodes readNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->readNodes = readNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStoreInterface_registerWriteNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_writeNodes writeNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->writeNodes = writeNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStoreInterface_registerBrowseNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_browseNodes browseNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->browseNodes = browseNodes;
	return UA_STATUSCODE_GOOD;
}

UA_NodeStoreInterface* UA_NodeStore_new(){
	return UA_alloc(sizeof(UA_NodeStoreInterface));
}

UA_StatusCode UA_NodeStore_copy(const UA_NodeStoreInterface *src,UA_NodeStoreInterface *dst){
	if(src!=UA_NULL){
		if(dst!=UA_NULL){
			dst->addNodes = src->addNodes;
			dst->addReferences = src->addReferences;
			dst->browseNodes = src->browseNodes;
			dst->deleteNodes = src->deleteNodes;
			dst->deleteReferences = src->deleteReferences;
			dst->readNodes = src->readNodes;
			dst->writeNodes = src->writeNodes;
		return UA_STATUSCODE_GOOD;
		}
	}
	return UA_STATUSCODE_BADINTERNALERROR;
}
void UA_NodeStore_delete(UA_NodeStoreInterface *nodestore){
	UA_free(nodestore);
}
//add method to add a 'delete nodestore'





