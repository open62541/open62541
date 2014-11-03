#ifndef UA_OPEN62541_NODESTORE_H_
#define UA_OPEN62541_NODESTORE_H_

#include "ua_server.h"
#include "ua_config.h"
/**
 @ingroup server

 @defgroup nodestore NodeStore

 @brief The nodestore is the central storage for nodes in the UA address
 space. Internally, the nodestore is realised as hash-map where nodes are
 stored and retrieved with their nodeid.

 The nodes in the nodestore are immutable. To change the content of a node, it
 needs to be replaced as a whole. When a node is inserted into the namespace,
 it gets replaced with a pointer to a managed node. Managed nodes shall never
 be freed by the user. This is done by the namespace when the node is removed
 and no readers (in other threads) access the node.

 @{
 */

void UA_EXPORT open62541NodeStore_setNodeStore(open62541NodeStore *nodestore);
open62541NodeStore UA_EXPORT *open62541NodeStore_getNodeStore();

/** @brief Create a new namespace */
UA_StatusCode UA_EXPORT open62541NodeStore_new(open62541NodeStore **result);

/** @brief Delete the namespace and all nodes in it */
void open62541NodeStore_delete(open62541NodeStore *ns);

#define UA_NODESTORE_INSERT_UNIQUE 1
#define UA_NODESTORE_INSERT_GETMANAGED 2
/** @brief Insert a new node into the namespace

 With the UNIQUE flag, the node is only inserted if the nodeid does not
 already exist. With the GETMANAGED flag, the node pointer is replaced with
 the managed pointer. Otherwise, it is set to UA_NULL. */
UA_StatusCode open62541NodeStore_insert(open62541NodeStore *ns, UA_Node **node,
		UA_Byte flags);

/** @brief Remove a node from the namespace. Always succeeds, even if the node
 was not found. */
UA_StatusCode open62541NodeStore_remove(open62541NodeStore *ns,
		const UA_NodeId *nodeid);

/** @brief Retrieve a node (read-only) from the namespace. Nodes are immutable.
 They can only be replaced. After the Node is no longer used, the locked
 entry needs to be released. */
UA_StatusCode open62541NodeStore_get(const open62541NodeStore *ns,
		const UA_NodeId *nodeid, const UA_Node **managedNode);

/** @brief Release a managed node. Do never insert a node that isn't stored in a
 namespace. */
void open62541NodeStore_releaseManagedNode(const UA_Node *managed);

/** @brief A function that can be evaluated on all entries in a namespace via
 UA_NodeStore_iterate. Note that the visitor is read-only on the nodes. */
typedef void (*UA_NodeStore_nodeVisitor)(const UA_Node *node);

/** @brief Iterate over all nodes in a namespace. */
void open62541NodeStore_iterate(const open62541NodeStore *ns,
		UA_NodeStore_nodeVisitor visitor);

/// @} /* end of group */

//service implementations
UA_Int32 UA_EXPORT open62541NodeStore_ReadNodes(UA_ReadValueId *readValueIds,
		UA_UInt32 *indices, UA_UInt32 indicesSize,
		UA_DataValue *readNodesResults, UA_Boolean timeStampToReturn,
		UA_DiagnosticInfo *diagnosticInfos);

UA_Int32 UA_EXPORT open62541NodeStore_BrowseNodes(
		UA_BrowseDescription *browseDescriptions, UA_UInt32 *indices,
		UA_UInt32 indicesSize, UA_UInt32 requestedMaxReferencesPerNode,
		UA_BrowseResult *browseResults, UA_DiagnosticInfo *diagnosticInfos);

UA_Int32 UA_EXPORT open62541NodeStore_AddNodes(UA_AddNodesItem *nodesToAdd,
		UA_UInt32 *indices, UA_UInt32 indicesSize,
		UA_AddNodesResult* addNodesResults, UA_DiagnosticInfo *diagnosticInfos);

#endif /* UA_OPEN62541_NODESTORE_H_ */
