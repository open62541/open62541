/*
 * Copyright (C) 2014 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#ifndef UA_SERVER_H_
#define UA_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_connection.h"
#include "ua_log.h"

/** @defgroup server Server */

struct UA_NodeStore;
typedef struct UA_NodeStore UA_NodeStoreInterface;

struct UA_Server;
typedef struct UA_Server UA_Server;
struct open62541NodeStore;
typedef struct open62541NodeStore open62541NodeStore;


struct UA_NamespaceManager;
typedef struct UA_NamespaceManager UA_NamespaceManager;

typedef UA_Int32 (*UA_NodeStore_addNodes)(const UA_RequestHeader *requestHeader, UA_AddNodesItem *nodesToAdd,UA_UInt32 *indices,UA_UInt32 indicesSize, UA_AddNodesResult* addNodesResults, UA_DiagnosticInfo *diagnosticInfos);
typedef UA_Int32 (*UA_NodeStore_addReferences)(const UA_RequestHeader *requestHeader,UA_AddReferencesItem* referencesToAdd,UA_UInt32 *indices,UA_UInt32 indicesSize, UA_StatusCode *addReferencesResults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_NodeStore_deleteNodes)(const UA_RequestHeader *requestHeader,UA_DeleteNodesItem *nodesToDelete,UA_UInt32 *indices,UA_UInt32 indicesSize, UA_StatusCode *deleteNodesResults, UA_DiagnosticInfo *diagnosticInfos);
typedef UA_Int32 (*UA_NodeStore_deleteReferences)(const UA_RequestHeader *requestHeader,UA_DeleteReferencesItem *referenceToDelete,UA_UInt32 *indices, UA_UInt32 indicesSize,UA_StatusCode deleteReferencesresults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_NodeStore_readNodes)(const UA_RequestHeader *requestHeader,UA_ReadValueId *readValueIds,UA_UInt32 *indices,UA_UInt32 indicesSize,UA_DataValue *readNodesResults, UA_Boolean timeStampToReturn, UA_DiagnosticInfo *diagnosticInfos);
typedef UA_Int32 (*UA_NodeStore_writeNodes)(const UA_RequestHeader *requestHeader,UA_WriteValue *writeValues,UA_UInt32 *indices ,UA_UInt32 indicesSize, UA_StatusCode *writeNodesResults, UA_DiagnosticInfo *diagnosticInfo);
typedef UA_Int32 (*UA_NodeStore_browseNodes)(const UA_RequestHeader *requestHeader,UA_BrowseDescription *browseDescriptions,UA_UInt32 *indices,UA_UInt32 indicesSize, UA_UInt32 requestedMaxReferencesPerNode, UA_BrowseResult *browseResults, UA_DiagnosticInfo *diagnosticInfos);

struct UA_NodeStore {
	//new, set, get, remove,
	UA_NodeStore_addNodes addNodes;
	UA_NodeStore_deleteNodes deleteNodes;
	UA_NodeStore_writeNodes writeNodes;
	UA_NodeStore_readNodes readNodes;
	UA_NodeStore_browseNodes browseNodes;
	UA_NodeStore_addReferences addReferences;
	UA_NodeStore_deleteReferences deleteReferences;
};


UA_Server UA_EXPORT * UA_Server_new(UA_String *endpointUrl, UA_ByteString *serverCertificate, UA_NodeStoreInterface *ns0Nodestore,UA_Boolean useOpen62541NodeStore);

void UA_EXPORT UA_Server_delete(UA_Server *server);
void UA_EXPORT UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg);

/* Services for local use */
void UA_EXPORT UA_Server_addScalarVariableNode(UA_Server *server, UA_QualifiedName *browseName, void *value,
                                                  const UA_VTable_Entry *vt, UA_ExpandedNodeId *parentNodeId,
                                                  UA_NodeId *referenceTypeId );
//UA_AddNodesResult UA_EXPORT UA_Server_addNode(UA_Server *server, UA_Node **node, UA_ExpandedNodeId *parentNodeId,
//		UA_NodeId *referenceTypeId);
//void UA_EXPORT UA_Server_addReferences(UA_Server *server, const UA_AddReferencesRequest *request,
//		UA_AddReferencesResponse *response);

UA_Int32 UA_EXPORT UA_Server_addNamespace(UA_Server *server, UA_UInt16 namespaceIndex, UA_NodeStoreInterface *nodeStore);

UA_Int32 UA_EXPORT UA_Server_removeNamespace(UA_Server *server, UA_UInt16 namespaceIndex);

UA_Int32 UA_EXPORT UA_Server_setNodeStore(UA_Server *server, UA_UInt16 namespaceIndex, UA_NodeStoreInterface *nodeStore);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_H_ */
