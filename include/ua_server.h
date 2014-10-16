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

//identifier numbers are different for XML and binary, so we have to substract an offset for comparison
#define UA_ENCODINGOFFSET_XML 1
#define UA_ENCODINGOFFSET_BINARY 2

struct UA_SecureChannelManager;
typedef struct UA_SecureChannelManager UA_SecureChannelManager;

struct UA_SessionManager;
typedef struct UA_SessionManager UA_SessionManager;




struct UA_NodeStore;
typedef struct UA_NodeStore UA_NodeStore;

struct UA_NodeStoreExample;
typedef struct UA_NodeStoreExample UA_NodeStoreExample;


//struct UA_Namespace;
//typedef struct UA_Namespace UA_Namespace;
typedef struct UA_Namespace
{
	UA_UInt16 index;
	UA_NodeStore *nodeStore;
}UA_Namespace;

struct UA_NamespaceManager;
typedef struct UA_NamespaceManager UA_NamespaceManager;


typedef UA_Int32 (*UA_NodeStore_addNodes)(UA_AddNodesItem *nodesToAdd,UA_UInt32 sizeNodesToAdd, UA_AddNodesResult* result, UA_DiagnosticInfo *diagnosticInfo);
typedef UA_Int32 (*UA_NodeStore_addReferences)(UA_AddReferencesItem* referencesToAdd,UA_UInt32 sizeReferencesToAdd, UA_StatusCode *result, UA_DiagnosticInfo diagnosticInfo);

typedef UA_Int32 (*UA_NodeStore_deleteNodes)(UA_DeleteNodesItem *nodesToDelete,UA_UInt32 sizeNodesToDelete, UA_StatusCode *result, UA_DiagnosticInfo *diagnosticInfo);
typedef UA_Int32 (*UA_NodeStore_deleteReferences)(UA_DeleteReferencesItem referenceToDelete, UA_UInt32 sizeReferencesToDelete,UA_StatusCode result, UA_DiagnosticInfo diagnosticInfo);


typedef UA_Int32 (*UA_NodeStore_readNodes)(UA_ReadValueId *readValueIds,UA_UInt32 sizeReadValueIds, UA_DataValue *value, UA_Boolean timeStampToReturn, UA_DiagnosticInfo *diagnosticInfo);
typedef UA_Int32 (*UA_NodeStore_writeNodes)(UA_WriteValue *writeValues, UA_UInt32 sizeWriteValues, UA_StatusCode *result, UA_DiagnosticInfo *diagnosticInfo);
typedef UA_Int32 (*UA_NodeStore_browseNodes)(UA_UInt32 requestedMaxReferencesPerNode, UA_BrowseDescription *browseDescriptions,UA_UInt32 sizeBrowseDescriptions, UA_BrowseResult *browseResult, UA_DiagnosticInfo *diagnosticInfo);




struct  UA_NodeStore{
	//new, set, get, remove,
	UA_NodeStore_addNodes addNodes;
	UA_NodeStore_deleteNodes deleteNodes;
	UA_NodeStore_writeNodes writeNodes;
	UA_NodeStore_readNodes readNodes;
	UA_NodeStore_browseNodes browseNodes;
	UA_NodeStore_addReferences addReferences;
	UA_NodeStore_deleteReferences deleteReferences;
};




typedef struct UA_Server {
    UA_ApplicationDescription description;
    UA_SecureChannelManager *secureChannelManager;
    UA_SessionManager *sessionManager;
    UA_NamespaceManager* namespaceManager;
    UA_NodeStoreExample *nodestore;
    UA_Logger logger;
    UA_ByteString serverCertificate;


    // todo: move these somewhere sane
    UA_ExpandedNodeId objectsNodeId;
    UA_NodeId hasComponentReferenceTypeId;


} UA_Server;

void UA_EXPORT UA_Server_init(UA_Server *server, UA_String *endpointUrl);
UA_Int32 UA_EXPORT UA_Server_deleteMembers(UA_Server *server);
UA_Int32 UA_EXPORT UA_Server_processBinaryMessage(UA_Server *server, UA_Connection *connection, const UA_ByteString *msg);

/* Services for local use */
UA_AddNodesResult UA_EXPORT UA_Server_addScalarVariableNode(UA_Server *server, UA_String *browseName, void *value,
                                                            const UA_VTable_Entry *vt, UA_ExpandedNodeId *parentNodeId,
                                                            UA_NodeId *referenceTypeId );
UA_AddNodesResult UA_EXPORT UA_Server_addNode(UA_Server *server, UA_Node **node, UA_ExpandedNodeId *parentNodeId,
                                              UA_NodeId *referenceTypeId);
void UA_EXPORT UA_Server_addReferences(UA_Server *server, const UA_AddReferencesRequest *request,
                                       UA_AddReferencesResponse *response);


UA_Int32 UA_Server_addNamespace(UA_Server *server, UA_UInt16 namespaceIndex, UA_NodeStore *nodeStore);

UA_Int32 UA_Server_removeNamespace(UA_Server *server, UA_UInt16 namespaceIndex);

UA_Int32 UA_Server_setNodeStore(UA_Server *server, UA_UInt16 namespaceIndex, UA_NodeStore *nodeStore);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_SERVER_H_ */
