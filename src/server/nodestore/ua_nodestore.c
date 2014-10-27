#include "ua_nodestore.h"
#include "ua_util.h"
#include "ua_statuscodes.h"



UA_StatusCode UA_NodeStore_registerAddNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_addNodes addNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->addNodes = addNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerAddReferencesOperation(UA_NodeStore *nodeStore, UA_NodeStore_addReferences addReferences)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->addReferences = addReferences;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerDeleteNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_deleteNodes deleteNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->deleteNodes = deleteNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerDeleteReferencesOperation(UA_NodeStore *nodeStore, UA_NodeStore_deleteReferences deleteReferences)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->deleteReferences = deleteReferences;
	return UA_STATUSCODE_GOOD;
}


UA_StatusCode UA_NodeStore_registerReadNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_readNodes readNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->readNodes = readNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerWriteNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_writeNodes writeNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->writeNodes = writeNodes;
	return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_NodeStore_registerBrowseNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_browseNodes browseNodes)
{
	if(nodeStore==UA_NULL){
		return UA_STATUSCODE_BADNOTFOUND;
	}
	nodeStore->browseNodes = browseNodes;
	return UA_STATUSCODE_GOOD;
}
/*
UA_Boolean UA_NodeStore_nodeExists(UA_NodeStore* nodestore, const UA_NodeId *nodeId)
{
	UA_DataValue *value;
	UA_ReadValueId readValueId;
	readValueId.attributeId = UA_ATTRIBUTEWRITEMASK_NODEID;
	readValueId.nodeId = nodeId;
	readValueId.indexRange.data = UA_NULL;
	readValueId.indexRange.length = 0;
	UA_NodeStore_readNode r;

	if(nodestore->readNodes(nodeId,readValueId,&value) == UA_SUCCESS &&
			value->value.storage.data.dataPtr != UA_NULL){
		return UA_TRUE;
	}
	return UA_FALSE;

}
*/






