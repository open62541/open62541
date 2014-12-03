#ifndef UA_NODESTORE_H_
#define UA_NODESTORE_H_

#include "ua_types_generated.h"
#include "ua_util.h"

/**
   @ingroup server

   @defgroup nodestore NodeStore

   @brief The nodestore contains the nodes in the UA address space. Internally,
   it is based on a hash-map that maps nodes to their nodeid.

   ATTENTION! You need to allocate single nodes on the heap (with _new) before
   adding them to the nodestore with _insert or _replace. The node is then
   copied to a new (managed) location in the nodestore and the original memory
   is freed. The nodes in the nodestore are immutable. To change the content of
   a node, it needs to be replaced as a whole.

   ATTENTION! Every node you _get from the nodestore needs to be _released when
   it is no longer needed. Otherwise, we can't know if somebody still uses it
   (especially in multi-threaded environments).
 */

struct UA_NodeStore;
typedef struct UA_NodeStore UA_NodeStore;

/** @brief Create a new namespace */
UA_NodeStore * UA_NodeStore_new();

/** @brief Delete the namespace and all nodes in it */
void UA_NodeStore_delete(UA_NodeStore *ns);

/** @brief Insert a new node into the namespace

    With the getManaged flag, the node pointer is replaced with the managed
    pointer. Otherwise, it is set to UA_NULL.

    If the nodeid is zero, then a fresh numeric nodeid from namespace 1 is
    assigned. */
UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, const UA_Node **node, UA_Boolean getManaged);

/** @brief Replace an existing node in the nodestore

    With the getManaged flag, the node pointer is replaced with the managed
    pointer. Otherwise, it is set to UA_NULL.

    If the return value is UA_STATUSCODE_BADINTERNALERROR, try again. Presumably
    the oldNode was already replaced by another thread.
*/
UA_StatusCode UA_NodeStore_replace(UA_NodeStore *ns, const UA_Node *oldNode, const UA_Node **node, UA_Boolean getManaged);

/** @brief Remove a node from the namespace. Always succeeds, even if the node
    was not found. */
UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid);

/** @brief Retrieve a node (read-only) from the namespace. Nodes are immutable.
    They can only be replaced. After the Node is no longer used, the locked
    entry needs to be released. */
const UA_Node * UA_NodeStore_get(const UA_NodeStore *ns, const UA_NodeId *nodeid);

/** @brief Release a managed node. Do never insert a node that isn't stored in a
    namespace. */
void UA_NodeStore_release(const UA_Node *managed);

/** @brief A function that can be evaluated on all entries in a namespace via
    UA_NodeStore_iterate. Note that the visitor is read-only on the nodes. */
typedef void (*UA_NodeStore_nodeVisitor)(const UA_Node *node);

/** @brief Iterate over all nodes in a namespace. */
void UA_NodeStore_iterate(const UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor);

#endif /* UA_NODESTORE_H_ */
