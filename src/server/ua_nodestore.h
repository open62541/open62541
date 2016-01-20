#ifndef UA_NODESTORE_H_
#define UA_NODESTORE_H_

#include "ua_types_generated.h"
#include "ua_nodes.h"

/**
 * Stores the nodes in the address space. Internally, it is based on a hash-map
 * that maps nodes to their nodeid.
 */

struct UA_NodeStore;
typedef struct UA_NodeStore UA_NodeStore;

/** Create a new nodestore */
UA_NodeStore * UA_NodeStore_new(void);

/** Delete the nodestore and all nodes in it. Do not call from a read-side
    critical section (multithreading). */
void UA_NodeStore_delete(UA_NodeStore *ns);

/** Create an editable node of the given NodeClass. */
UA_Node * UA_NodeStore_newNode(UA_NodeClass class);
#define UA_NodeStore_newObjectNode() (UA_ObjectNode*)UA_NodeStore_newNode(UA_NODECLASS_OBJECT)
#define UA_NodeStore_newVariableNode() (UA_VariableNode*)UA_NodeStore_newNode(UA_NODECLASS_VARIABLE)
#define UA_NodeStore_newMethodNode() (UA_MethodNode*)UA_NodeStore_newNode(UA_NODECLASS_METHOD)
#define UA_NodeStore_newObjectTypeNode() (UA_ObjectTypeNode*)UA_NodeStore_newNode(UA_NODECLASS_OBJECTTYPE)
#define UA_NodeStore_newVariableTypeNode() (UA_VariableTypeNode*)UA_NodeStore_newNode(UA_NODECLASS_VARIABLETYPE)
#define UA_NodeStore_newReferenceTypeNode() (UA_ReferenceTypeNode*)UA_NodeStore_newNode(UA_NODECLASS_REFERENCETYPE)
#define UA_NodeStore_newDataTypeNode() (UA_DataTypeNode*)UA_NodeStore_newNode(UA_NODECLASS_DATATYPE)
#define UA_NodeStore_newViewNode() (UA_ViewNode*)UA_NodeStore_newNode(UA_NODECLASS_VIEW)

/** Delete an editable node. */
void UA_NodeStore_deleteNode(UA_Node *node);

/**
 * Inserts a new node into the nodestore. If the nodeid is zero, then a fresh
 * numeric nodeid from namespace 1 is assigned. If insertion fails, the node is
 * deleted.
 */
UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, UA_Node *node);

/**
 * To replace, get an editable copy, edit and use this function. If the node was
 * already replaced since the copy was made, UA_STATUSCODE_BADINTERNALERROR is
 * returned. If the nodeid is not found, UA_STATUSCODE_BADNODEIDUNKNOWN is
 * returned. In both error cases, the editable node is deleted.
 */
UA_StatusCode UA_NodeStore_replace(UA_NodeStore *ns, UA_Node *node);

/** Remove a node in the nodestore. */
UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeid);

/**
 * The returned pointer is only valid as long as the node has not been replaced
 * or removed (in the same thread).
 */
const UA_Node * UA_NodeStore_get(UA_NodeStore *ns, const UA_NodeId *nodeid);

/** Returns the copy of a node. */
UA_Node * UA_NodeStore_getCopy(UA_NodeStore *ns, const UA_NodeId *nodeid);

/**
 * A function that can be evaluated on all entries in a nodestore via
 * UA_NodeStore_iterate. Note that the visitor is read-only on the nodes.
 */
typedef void (*UA_NodeStore_nodeVisitor)(const UA_Node *node);

/** Iterate over all nodes in a nodestore. */
void UA_NodeStore_iterate(UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor);

#endif /* UA_NODESTORE_H_ */
