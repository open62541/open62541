#ifndef UA_NODESTORE_H_
#define UA_NODESTORE_H_

#include "ua_types_generated.h"
#include "ua_nodes.h"

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

/** Create a new nodestore */
UA_NodeStore * UA_NodeStore_new(void);

/** Delete the nodestore and all nodes in it */
void UA_NodeStore_delete(UA_NodeStore *ns);

/**
 * Inserts a new node into the nodestore. If the nodeid is zero, then a fresh
 * numeric nodeid from nodestore 1 is assigned. The memory of the original node
 * is freed and the content is moved to a managed (immutable) node. If inserted
 * is not NULL, then a pointer to the managed node is returned (and must be
 * released).
 */
UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, UA_Node *node, const UA_Node **inserted);

/**
 * Replace an existing node in the nodestore. If the node was already replaced,
 * UA_STATUSCODE_BADINTERNALERROR is returned. If inserted is not NULL, a
 * pointer to the managed (immutable) node is returned.
 */
UA_StatusCode UA_NodeStore_replace(UA_NodeStore *ns, const UA_Node *oldNode, UA_Node *node, const UA_Node **inserted);

/**
 * Remove a node from the nodestore. Always succeeds, even if the node was not
 * found.
 */
UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid);

/**
 * Retrieve a managed node (read-only) from the nodestore. Nodes are reference-
 * counted (for garbage collection) and immutable. They can only be replaced
 * entirely. After the node is no longer used, it needs to be released to decrease
 * the reference count.
 */
const UA_Node * UA_NodeStore_get(const UA_NodeStore *ns, const UA_NodeId *nodeid);

/**
 * Release a managed node. Do never call this with a node that isn't managed by a
 * nodestore.
 */
void UA_NodeStore_release(const UA_Node *managed);

/**
 * A function that can be evaluated on all entries in a nodestore via
 * UA_NodeStore_iterate. Note that the visitor is read-only on the nodes.
 */
typedef void (*UA_NodeStore_nodeVisitor)(const UA_Node *node);

/** Iterate over all nodes in a nodestore. */
void UA_NodeStore_iterate(const UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor);

/** @} */

#endif /* UA_NODESTORE_H_ */
