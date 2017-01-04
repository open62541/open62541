/*
 * ua_nodestore_switch.h
 *
 *  Created on: Sep 30, 2016
 *      Author: julian
 */

#ifndef UA_NODESTORE_SWITCH_H_
#define UA_NODESTORE_SWITCH_H_

#include "ua_types.h"
#include "ua_nodestore_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UA_NodestoreSwitch{
    UA_NodestoreInterface** nodestoreInterfaces;
    UA_UInt16 nodestoreInterfacesSize;
} UA_NodestoreSwitch;

/**
 * Nodestoreswitch Lifecycle
 * ^^^^^^^^^^^^^^^^^^^ */
/* Create a new nodestore */
UA_NodestoreSwitch * UA_NodestoreSwitch_new(void);

/* Delete the nodestore and all nodes in it. Do not call from a read-side
   critical section (multithreading). */
void UA_NodestoreSwitch_delete(UA_NodestoreSwitch* nodestoreSwitch);

/**
 * Change Nodestore
 * ^^^^^^^^^^^^^^^^
 *
 */
UA_Boolean UA_NodestoreSwitch_add(UA_NodestoreSwitch* nodestoreSwitch, UA_NodestoreInterface *nodestoreInterface);
UA_Boolean UA_NodestoreSwitch_change(UA_NodestoreSwitch* nodestoreSwitch, UA_NodestoreInterface *nodestoreInterface, UA_UInt16 nodestoreInterfaceIndex);
UA_NodestoreInterface* UA_NodestoreSwitch_getNodestoreForNamespace(UA_NodestoreSwitch* nodestoreSwitch, UA_UInt16 namespaceIndex);
//TODO Add: Get all Namespaces for a Nodestore. --> Export this Functions

/**
 * Node Lifecycle
 * ^^^^^^^^^^^^^^
 * The memory is managed by the nodestore. Therefore, the node has
 * to be removed via a special deleteNode function. (If the new node is not
 * added to the nodestore.) */
/* Create an editable node of the given NodeClass. */
UA_Node * UA_NodestoreSwitch_newNode(UA_NodestoreSwitch* nodestoreSwitch, UA_NodeClass nodeClass, UA_UInt16 namespaceIndex);
/*
 * The following definitions are used to create empty nodes of the different
 * node types in NameSpace 0.
 */
#define UA_Nodestore_newObjectNode() \
    (UA_ObjectNode*)UA_NodestoreSwitch_newNode(server->nodestoreSwitch, UA_NODECLASS_OBJECT, 0)
#define UA_Nodestore_newVariableNode() \
    (UA_VariableNode*)UA_NodestoreSwitch_newNode(server->nodestoreSwitch, UA_NODECLASS_VARIABLE, 0)
#define UA_Nodestore_newMethodNode() \
    (UA_MethodNode*)UA_NodestoreSwitch_newNode(server->nodestoreSwitch, UA_NODECLASS_METHOD, 0)
#define UA_Nodestore_newObjectTypeNode() \
    (UA_ObjectTypeNode*)UA_NodestoreSwitch_newNode(server->nodestoreSwitch, UA_NODECLASS_OBJECTTYPE, 0)
#define UA_Nodestore_newVariableTypeNode() \
    (UA_VariableTypeNode*)UA_NodestoreSwitch_newNode(server->nodestoreSwitch, UA_NODECLASS_VARIABLETYPE, 0)
#define UA_Nodestore_newReferenceTypeNode() \
    (UA_ReferenceTypeNode*)UA_NodestoreSwitch_newNode(server->nodestoreSwitch, UA_NODECLASS_REFERENCETYPE, 0)
#define UA_Nodestore_newDataTypeNode() \
    (UA_DataTypeNode*)UA_NodestoreSwitch_newNode(server->nodestoreSwitch, UA_NODECLASS_DATATYPE, 0)
#define UA_Nodestore_newViewNode() \
    (UA_ViewNode*)UA_NodestoreSwitch_newNode(server->nodestoreSwitch, UA_NODECLASS_VIEW, 0)

/* Delete an editable node. */
void UA_NodestoreSwitch_deleteNode(UA_NodestoreSwitch* nodestoreSwitch, UA_Node *node);

/**
 * Insert / Get / Replace / Remove
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
/* Inserts a new node into the nodestore. If the nodeid is zero, then a fresh
 * numeric nodeid from namespace 1 is assigned. If insertion fails, the node is
 * deleted. */
UA_StatusCode UA_NodestoreSwitch_insert(UA_NodestoreSwitch* nodestoreSwitch, UA_Node *node,
        const UA_NodeId *parentNodeId, UA_NodeId *addedNodeId);

/* The returned node is immutable. */
const UA_Node * UA_NodestoreSwitch_get(UA_NodestoreSwitch* nodestoreSwitch, const UA_NodeId *nodeId);

/* Returns an editable copy of a node (needs to be deleted with the deleteNode
   function or inserted / replaced into the nodestore). */
UA_Node * UA_NodestoreSwitch_getCopy(UA_NodestoreSwitch* nodestoreSwitch, const UA_NodeId *nodeId);

/* To replace a node, get an editable copy of the node, edit and replace with
 * this function. If the node was already replaced since the copy was made,
 * UA_STATUSCODE_BADINTERNALERROR is returned. If the nodeid is not found,
 * UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases, the editable
 * node is deleted. */
UA_StatusCode UA_NodestoreSwitch_replace(UA_NodestoreSwitch* nodestoreSwitch, UA_Node *node);

/* Remove a node in the nodestore. */
UA_StatusCode UA_NodestoreSwitch_remove(UA_NodestoreSwitch* nodestoreSwitch, const UA_NodeId *nodeId);

/**
 * Iteration
 * ^^^^^^^^^
 * The following definitions are used to call a callback for every node in the
 * nodestore. */
void UA_NodestoreSwitch_iterate(UA_NodestoreSwitch* nodestoreSwitch, UA_Nodestore_nodeVisitor visitor, UA_UInt16 namespaceIndex);

/**
 * Release
 * ^^^^^^^
 * Indicates that the reference to a node, which was fetched from the nodestore via the "get" method, is not used anymore.
 * This is the basis for multithreading capable nodestores. Reference Counting and locks can be freed via this method.
 *  */
void UA_NodestoreSwitch_release(UA_NodestoreSwitch* nodestoreSwitch, const UA_Node *node);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NODESTORE_SWITCH_H_ */
