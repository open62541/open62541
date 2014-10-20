#ifndef UA_NODESTORE_H_
#define UA_NODESTORE_H_

#include "ua_server.h"




UA_Int32 UA_EXPORT UA_NodeStore_registerAddNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_addNodes addNode);
UA_Int32 UA_EXPORT UA_NodeStore_registerAddReferenceOperation(UA_NodeStore *nodeStore, UA_NodeStore_addReferences addReference);
UA_Int32 UA_EXPORT UA_NodeStore_registerDeleteNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_deleteNodes deleteNode);
UA_Int32 UA_EXPORT UA_NodeStore_registerDeleteReferencesOperation(UA_NodeStore *nodeStore, UA_NodeStore_deleteReferences deleteReference);
UA_Int32 UA_EXPORT UA_NodeStore_registerReadNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_readNodes readNode);
UA_Int32 UA_EXPORT UA_NodeStore_registerWriteNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_writeNodes writeNode);
UA_Int32 UA_EXPORT UA_NodeStore_registerBrowseNodesOperation(UA_NodeStore *nodeStore, UA_NodeStore_browseNodes browseNode);

#define UA_NODESTORE_INSERT_UNIQUE 1
#define UA_NODESTORE_INSERT_GETMANAGED 2




UA_Boolean UA_NodeStore_nodeExists(UA_NodeId nodeId);


#endif /* UA_NODESTORE_H_ */
