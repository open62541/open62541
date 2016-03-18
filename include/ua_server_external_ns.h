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

#ifndef UA_SERVER_EXTERNAL_NS_H_
#define UA_SERVER_EXTERNAL_NS_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "../src/server/ua_nodestore.h"
/**
 * An external application that manages its own data and data model. To plug in
 * outside data sources, one can use
 *
 * - VariableNodes with a data source (functions that are called for read and write access)
 * - An external nodestore that is mapped to specific namespaces
 *
 * If no external nodestore is defined for a nodeid, it is always looked up in
 * the "local" nodestore of open62541. Namespace Zero is always in the local
 * nodestore.
 *
 * @{
 */

typedef UA_Int32 (*UA_ExternalNodeStore_addNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_AddNodesItem *nodesToAdd, UA_UInt32 *indices,
 UA_UInt32 indicesSize, UA_AddNodesResult* addNodesResults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_addReferences)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_AddReferencesItem* referencesToAdd,
 UA_UInt32 *indices,UA_UInt32 indicesSize, UA_StatusCode *addReferencesResults,
 UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_deleteNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_DeleteNodesItem *nodesToDelete, UA_UInt32 *indices,
 UA_UInt32 indicesSize, UA_StatusCode *deleteNodesResults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_deleteReferences)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_DeleteReferencesItem *referenceToDelete,
 UA_UInt32 *indices, UA_UInt32 indicesSize, UA_StatusCode deleteReferencesresults,
 UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_readNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_ReadValueId *readValueIds, UA_UInt32 *indices,
 UA_UInt32 indicesSize,UA_DataValue *readNodesResults, UA_Boolean timeStampToReturn,
 UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_writeNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_WriteValue *writeValues, UA_UInt32 *indices,
 UA_UInt32 indicesSize, UA_StatusCode *writeNodesResults, UA_DiagnosticInfo *diagnosticInfo);

typedef UA_Int32 (*UA_ExternalNodeStore_browseNodes)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_BrowseDescription *browseDescriptions,
 UA_UInt32 *indices, UA_UInt32 indicesSize, UA_UInt32 requestedMaxReferencesPerNode,
 UA_BrowseResult *browseResults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_translateBrowsePathsToNodeIds)
(void *ensHandle, const UA_RequestHeader *requestHeader, UA_BrowsePath *browsePath, UA_UInt32 *indices,
 UA_UInt32 indicesSize, UA_BrowsePathResult *browsePathResults, UA_DiagnosticInfo *diagnosticInfos);

typedef UA_Int32 (*UA_ExternalNodeStore_delete)(void *ensHandle);

typedef struct UA_ExternalNodeStore {
    void *ensHandle;
	UA_ExternalNodeStore_addNodes addNodes;
	UA_ExternalNodeStore_deleteNodes deleteNodes;
	UA_ExternalNodeStore_writeNodes writeNodes;
	UA_ExternalNodeStore_readNodes readNodes;
	UA_ExternalNodeStore_browseNodes browseNodes;
	UA_ExternalNodeStore_translateBrowsePathsToNodeIds translateBrowsePathsToNodeIds;
	UA_ExternalNodeStore_addReferences addReferences;
	UA_ExternalNodeStore_deleteReferences deleteReferences;
	UA_ExternalNodeStore_delete destroy;
} UA_ExternalNodeStore;

UA_StatusCode UA_EXPORT
UA_Server_addExternalNamespace(UA_Server *server, UA_UInt16 namespaceIndex, const UA_String *url,
                               UA_ExternalNodeStore *nodeStore);

#ifdef __cplusplus
}
#endif


/** Delete an editable node. */
void UA_NodeStore_deleteNode(UA_Node *node);

/**
 * Inserts a new node into the nodestore. If the nodeid is zero, then a fresh
 * numeric nodeid from namespace 1 is assigned. If insertion fails, the node is
 * deleted.
 */
typedef UA_StatusCode (*UA_NodestoreInterface_insert)(void *handle, UA_Node *node);
/**
 * To replace, get an editable copy, edit and use this function. If the node was
 * already replaced since the copy was made, UA_STATUSCODE_BADINTERNALERROR is
 * returned. If the nodeid is not found, UA_STATUSCODE_BADNODEIDUNKNOWN is
 * returned. In both error cases, the editable node is deleted.
 */
typedef UA_StatusCode (*UA_NodestoreInterface_replace)(void *handle, UA_Node *node);
/** Remove a node in the nodestore. */
typedef UA_StatusCode (*UA_NodestoreInterface_remove)(void *handle, const UA_NodeId *nodeid);
/**
 * The returned pointer is only valid as long as the node has not been replaced
 * or removed (in the same thread).
 */
typedef const UA_Node * (*UA_NodestoreInterface_get)(void *handle, const UA_NodeId *nodeid);
/** Returns the copy of a node. */
typedef UA_Node * (*UA_NodestoreInterface_getCopy)(void *handle, const UA_NodeId *nodeid);
/**
 * A function that can be evaluated on all entries in a nodestore via
 * UA_NodeStore_iterate. Note that the visitor is read-only on the nodes.
 */
typedef void (*UA_NodestoreInterface_nodeVisitor)(const UA_Node *node);

/** Iterate over all nodes in a nodestore. */
typedef void (*UA_NodestoreInterface_iterate)(void *handle, UA_NodeStore_nodeVisitor visitor);

typedef struct UA_NodestoreInterface {
    void * handle;
    UA_NodestoreInterface_insert insert;
    UA_NodestoreInterface_replace replace;
    UA_NodestoreInterface_remove remove;
    UA_NodestoreInterface_get get;
    UA_NodestoreInterface_getCopy getCopy;
}UA_NodestoreInterface;

UA_StatusCode UA_EXPORT
UA_Server_NodestoreInterface_add(UA_Server *server, UA_String *url,
        UA_NodestoreInterface *nodeStore);

#endif /* UA_SERVER_EXTERNAL_NS_H_ */
