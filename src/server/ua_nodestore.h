#ifndef UA_NODESTORE_H_
#define UA_NODESTORE_H_

#include "ua_server.h"

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

/** @brief Create a new namespace */
UA_StatusCode UA_NodeStore_new(UA_NodeStore **result);

/** @brief Delete the namespace and all nodes in it */
void UA_NodeStore_delete(UA_NodeStore *ns);

#define UA_NODESTORE_INSERT_UNIQUE 1
#define UA_NODESTORE_INSERT_GETMANAGED 2
/** @brief Insert a new node into the namespace

    With the UNIQUE flag, the node is only inserted if the nodeid does not
    already exist. With the GETMANAGED flag, the node pointer is replaced with
    the managed pointer. Otherwise, it is set to UA_NULL. */
UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, UA_Node **node, UA_Byte flags);

/** @brief Remove a node from the namespace. Always succeeds, even if the node
    was not found. */
UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid);

/** @brief Retrieve a node (read-only) from the namespace. Nodes are immutable.
    They can only be replaced. After the Node is no longer used, the locked
    entry needs to be released. */
UA_StatusCode UA_NodeStore_get(const UA_NodeStore *ns, const UA_NodeId *nodeid,
                               const UA_Node **managedNode);

/** @brief Release a managed node. Do never insert a node that isn't stored in a
    namespace. */
void UA_NodeStore_releaseManagedNode(const UA_Node *managed);

/** @brief A function that can be evaluated on all entries in a namespace via
    UA_NodeStore_iterate. Note that the visitor is read-only on the nodes. */
typedef void (*UA_NodeStore_nodeVisitor)(const UA_Node *node);

/** @brief Iterate over all nodes in a namespace. */
void UA_NodeStore_iterate(const UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor);

/// @} /* end of group */

#endif /* UA_NODESTORE_H_ */
