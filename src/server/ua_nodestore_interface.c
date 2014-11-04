#include "ua_nodestore_interface.h"
#include "ua_util.h"
#include "ua_statuscodes.h"



UA_StatusCode UA_NodeStore_registerAddNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_addNodes addNodes)
{
	if(nodeStore==NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->addNodes = addNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerAddReferencesOperation(UA_NodeStore *nodeStore, UA_NodeStore_addReferences addReferences)
{
	if(nodeStore==NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->addReferences = addReferences;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerDeleteNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_deleteNodes deleteNodes)
{
	if(nodeStore==NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->deleteNodes = deleteNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerDeleteReferencesOperation(UA_NodeStore *nodeStore, UA_NodeStore_deleteReferences deleteReferences)
{
	if(nodeStore==NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->deleteReferences = deleteReferences;
	return UA_STATUSCODE_GOOD;
}


UA_StatusCode UA_NodeStore_registerReadNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_readNodes readNodes)
{
	if(nodeStore==NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->readNodes = readNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerWriteNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_writeNodes writeNodes)
{
	if(nodeStore==NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->writeNodes = writeNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerBrowseNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_browseNodes browseNodes)
{
	if(nodeStore==NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->browseNodes = browseNodes;
	return UA_STATUSCODE_GOOD;
}

//add method to add a 'delete nodestore'





