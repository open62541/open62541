#ifndef UA_NODESTORE_H_
#define UA_NODESTORE_H_



#include "ua_server.h"


UA_StatusCode UA_EXPORT UA_NodeStoreInterface_registerAddNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_addNodes addNodes);
UA_StatusCode UA_EXPORT UA_NodeStoreInterface_registerAddReferenceOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_addReferences addReference);
UA_StatusCode UA_EXPORT UA_NodeStoreInterface_registerDeleteNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_deleteNodes deleteNode);
UA_StatusCode UA_EXPORT UA_NodeStoreInterface_registerDeleteReferencesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_deleteReferences deleteReference);
UA_StatusCode UA_EXPORT UA_NodeStoreInterface_registerReadNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_readNodes readNode);
UA_StatusCode UA_EXPORT UA_NodeStoreInterface_registerWriteNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_writeNodes writeNode);
UA_StatusCode UA_EXPORT UA_NodeStoreInterface_registerBrowseNodesOperation(UA_NodeStoreInterface *nodeStore, UA_NodeStore_browseNodes browseNode);

#define UA_NODESTORE_INSERT_UNIQUE 1
#define UA_NODESTORE_INSERT_GETMANAGED 2



UA_NodeStoreInterface* UA_NodeStore_new();
UA_StatusCode UA_NodeStore_copy(const UA_NodeStoreInterface *src,UA_NodeStoreInterface *dst);
void UA_NodeStore_delete(UA_NodeStoreInterface *nodestore);

UA_Boolean UA_NodeStore_nodeExists(UA_NodeId nodeId);


#endif /* UA_NODESTORE_H_ */
