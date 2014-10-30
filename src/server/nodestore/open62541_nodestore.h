/*
 * open62541_nodestore.h
 *
 *  Created on: Oct 27, 2014
 *      Author: opcua
 */

#ifndef OPEN62541_NODESTORE_H_
#define OPEN62541_NODESTORE_H_




#include "ua_server.h"


void UA_EXPORT Nodestore_set(UA_NodeStoreExample *nodestore);
UA_NodeStoreExample UA_EXPORT *Nodestore_get();


UA_Int32 UA_EXPORT open62541NodeStore_ReadNodes(UA_ReadValueId *readValueIds,UA_UInt32 *indices,UA_UInt32 indicesSize,UA_DataValue *readNodesResults, UA_Boolean timeStampToReturn, UA_DiagnosticInfo *diagnosticInfos);
UA_Int32 UA_EXPORT open62541NodeStore_BrowseNodes(UA_BrowseDescription *browseDescriptions,UA_UInt32 *indices,UA_UInt32 indicesSize, UA_UInt32 requestedMaxReferencesPerNode,
		UA_BrowseResult *browseResults,
		UA_DiagnosticInfo *diagnosticInfos);
UA_Int32 UA_EXPORT open62541Nodestore_addNodes(UA_AddNodesItem *nodesToAdd,UA_UInt32 *indices,
		UA_UInt32 indicesSize, UA_AddNodesResult* addNodesResults,
		UA_DiagnosticInfo *diagnosticInfos);
#endif /* OPEN62541_NODESTORE_H_ */
