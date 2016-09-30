#ifndef NODESTORE_H_
#define NODESTORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types_generated.h"
#include "../src/server/ua_nodes.h"

/**
 * Nodestore
 * ---------
 * Stores nodes that can be indexed by their NodeId. Internally, it is based on
 * a hash-map implementation. */
struct NodeStore;
typedef struct NodeStore NodeStore;

/**
 * Nodestore Lifecycle
 * ^^^^^^^^^^^^^^^^^^^ */
/* Create a new nodestore */
NodeStore * NodeStore_new(void);

/* Instanciate new NodeStoreInterface for this nodestore, as the open62541 standard nodestore*/
UA_EXPORT UA_NodeStoreInterface UA_NodeStoreInterface_standard(void);

/* Delete the nodestore and all nodes in it. Do not call from a read-side
   critical section (multithreading). */
void NodeStore_delete(NodeStore *ns);

/**
 * Node Lifecycle
 * ^^^^^^^^^^^^^^
 *
 * The following definitions are used to create empty nodes of the different
 * node types. The memory is managed by the nodestore. Therefore, the node has
 * to be removed via a special deleteNode function. (If the new node is not
 * added to the nodestore.) */
/* Create an editable node of the given NodeClass. */
UA_Node * NodeStore_newNode(UA_NodeClass nodeClass);

/* Delete an editable node. */
void NodeStore_deleteNode(UA_Node *node);

/**
 * Insert / Get / Replace / Remove
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
/* Inserts a new node into the nodestore. If the nodeid is zero, then a fresh
 * numeric nodeid from namespace 1 is assigned. If insertion fails, the node is
 * deleted. */
UA_StatusCode NodeStore_insert(NodeStore *ns, UA_Node *node);

/* The returned node is immutable. */
const UA_Node * NodeStore_get(NodeStore *ns, const UA_NodeId *nodeid);

/* Returns an editable copy of a node (needs to be deleted with the deleteNode
   function or inserted / replaced into the nodestore). */
UA_Node * NodeStore_getCopy(NodeStore *ns, const UA_NodeId *nodeid);

/* To replace a node, get an editable copy of the node, edit and replace with
 * this function. If the node was already replaced since the copy was made,
 * UA_STATUSCODE_BADINTERNALERROR is returned. If the nodeid is not found,
 * UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases, the editable
 * node is deleted. */
UA_StatusCode NodeStore_replace(NodeStore *ns, UA_Node *node);

/* Remove a node in the nodestore. */
UA_StatusCode NodeStore_remove(NodeStore *ns, const UA_NodeId *nodeid);

/**
 * Iteration
 * ^^^^^^^^^
 * The following definitions are used to call a callback for every node in the
 * nodestore. */
typedef void (*NodeStore_nodeVisitor)(const UA_Node *node);
void NodeStore_iterate(NodeStore *ns, NodeStore_nodeVisitor visitor);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* NODESTORE_H_ */
