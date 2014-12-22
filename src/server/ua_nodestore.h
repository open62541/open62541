#ifndef UA_NODESTORE_H_
#define UA_NODESTORE_H_

#include "ua_types_generated.h"

/**
 * @ingroup server
 *
 * @defgroup nodestore NodeStore
 *
 * @brief Stores the nodes in the address space. Internally, it is based on a
 * hash-map that maps nodes to their nodeid.
 *
 * Nodes need to be allocated on the heap before adding them to the nodestore
 * with. When adding, the node is copied to a new (managed) location in the
 * nodestore and the original memory is freed. The nodes in the nodestore are
 * immutable. To change the content of a node, it needs to be replaced as a
 * whole.
 *
 * Every node you _get from the nodestore needs to be _released when it is no
 * longer needed. In the background, reference counting is used to know if
 * somebody still uses the node in multi-threaded environments.
 *
 * @{
 */

struct UA_NodeStore;
typedef struct UA_NodeStore UA_NodeStore;

/** Create a new namespace */
UA_NodeStore * UA_NodeStore_new();

/** Delete the namespace and all nodes in it */
void UA_NodeStore_delete(UA_NodeStore *ns);

/**
 * Inserts a new node into the namespace. With the getManaged flag, the node
 * pointer is replaced with the managed pointer. Otherwise, it is set to
 * UA_NULL. If the nodeid is zero, then a fresh numeric nodeid from namespace 1
 * is assigned.
 */
UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, const UA_Node **node, UA_Boolean getManaged);

/**
 * Replace an existing node in the nodestore. With the getManaged flag, the node
 * pointer is replaced with the managed pointer. Otherwise, it is set to
 * UA_NULL. If the return value is UA_STATUSCODE_BADINTERNALERROR, try again.
 * Presumably the oldNode was already replaced by another thread.
 */
UA_StatusCode UA_NodeStore_replace(UA_NodeStore *ns, const UA_Node *oldNode, const UA_Node **node, UA_Boolean getManaged);

/**
 * Remove a node from the namespace. Always succeeds, even if the node was not
 * found.
 */
UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid);

/**
 * Retrieve a node (read-only) from the namespace. Nodes are immutable. They
 * can only be replaced. After the Node is no longer used, the locked entry
 * needs to be released.
 */
const UA_Node * UA_NodeStore_get(const UA_NodeStore *ns, const UA_NodeId *nodeid);

/**
 * Release a managed node. Do never insert a node that isn't stored in a
 * namespace.
 */
void UA_NodeStore_release(const UA_Node *managed);

/**
 * A function that can be evaluated on all entries in a namespace via
 * UA_NodeStore_iterate. Note that the visitor is read-only on the nodes.
 */
typedef void (*UA_NodeStore_nodeVisitor)(const UA_Node *node);

/** Iterate over all nodes in a namespace. */
void UA_NodeStore_iterate(const UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor);

/** @} */

#endif /* UA_NODESTORE_H_ */
