#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include "ua_types.h"
#include "ua_types_generated.h"
#include "util/ua_list.h"

/**
   @defgroup namespace Namespace

   @brief The namespace is the central storage for nodes in the UA address
   space. Internally, the namespace is realised as hash-map where nodes are
   stored and retrieved with their nodeid.

   The nodes in the namespace are immutable. To change the content of a node, it
   needs to be replaced as a whole. When a node is inserted into the namespace,
   it gets replaced with a pointer to a managed node. Managed nodes shall never
   be freed by the user. This is done by the namespace when the node is removed
   and no readers (in other threads) access the node.

   @{
 */

/** @brief Namespace datastructure. Mainly a hashmap to UA_Nodes */
struct Namespace;
typedef struct Namespace Namespace;

/** @brief Create a new namespace */
UA_Int32 Namespace_new(Namespace **result, UA_UInt32 namespaceId);

/** @brief Delete the namespace and all nodes in it */
UA_Int32 Namespace_delete(Namespace *ns);

#define NAMESPACE_INSERT_UNIQUE 1
#define NAMESPACE_INSERT_GETMANAGED 2
/** @brief Insert a new node into the namespace

    With the UNIQUE flag, the node is only inserted if the nodeid does not
    already exist. With the GETMANAGED flag, the node pointer is replaced with
    the managed pointer. Otherwise, it is set to UA_NULL. */
UA_Int32 Namespace_insert(Namespace *ns, const UA_Node **node, UA_Byte flags);

/** @brief Remove a node from the namespace. Always succeeds, even if the node
	was not found. */
UA_Int32 Namespace_remove(Namespace *ns, const UA_NodeId *nodeid);

/** @brief Retrieve a node (read-only) from the namespace. Nodes are immutable.
    They can only be replaced. After the Node is no longer used, the locked
    entry needs to be released. */
UA_Int32 Namespace_get(const Namespace *ns, const UA_NodeId *nodeid, const UA_Node **managedNode);

/** @brief Release a managed node. Do never insert a node that isn't stored in a
	namespace. */
void Namespace_releaseManagedNode(const UA_Node *managed);

/** @brief A function that can be evaluated on all entries in a namespace via
	Namespace_iterate. Note that the visitor is read-only on the nodes. */
typedef void (*Namespace_nodeVisitor)(const UA_Node *node);

/** @brief Iterate over all nodes in a namespace. */
UA_Int32 Namespace_iterate(const Namespace *ns, Namespace_nodeVisitor visitor);

/// @} /* end of group */

#endif /* __NAMESPACE_H */
