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
#include "ua_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Nodestore lifecycle
 * ^^^^^^^^^^^^^^^^
 *
 */
void
UA_NodestoreSwitch_deleteNodestore(UA_Server* server,
                          UA_UInt16 namespaceIndex);
/**
 * Node Lifecycle
 * ^^^^^^^^^^^^^^
 * The memory is managed by the nodestore. Therefore, the node has
 * to be removed via a special deleteNode function. (If the new node is not
 * added to the nodestore.) */
/* Create an editable node of the given NodeClass. */
UA_Node * UA_NodestoreSwitch_newNode(UA_Server* server, UA_NodeClass nodeClass, UA_UInt16 namespaceIndex);
/*
 * The following definitions are used to create empty nodes of the different
 * node types in NameSpace 0.
 */
#define UA_Nodestore_newObjectNode() \
    (UA_ObjectNode*)UA_NodestoreSwitch_newNode(server, UA_NODECLASS_OBJECT, 0)
#define UA_Nodestore_newVariableNode() \
    (UA_VariableNode*)UA_NodestoreSwitch_newNode(server, UA_NODECLASS_VARIABLE, 0)
#define UA_Nodestore_newMethodNode() \
    (UA_MethodNode*)UA_NodestoreSwitch_newNode(server, UA_NODECLASS_METHOD, 0)
#define UA_Nodestore_newObjectTypeNode() \
    (UA_ObjectTypeNode*)UA_NodestoreSwitch_newNode(server, UA_NODECLASS_OBJECTTYPE, 0)
#define UA_Nodestore_newVariableTypeNode() \
    (UA_VariableTypeNode*)UA_NodestoreSwitch_newNode(server, UA_NODECLASS_VARIABLETYPE, 0)
#define UA_Nodestore_newReferenceTypeNode() \
    (UA_ReferenceTypeNode*)UA_NodestoreSwitch_newNode(server, UA_NODECLASS_REFERENCETYPE, 0)
#define UA_Nodestore_newDataTypeNode() \
    (UA_DataTypeNode*)UA_NodestoreSwitch_newNode(server, UA_NODECLASS_DATATYPE, 0)
#define UA_Nodestore_newViewNode() \
    (UA_ViewNode*)UA_NodestoreSwitch_newNode(server, UA_NODECLASS_VIEW, 0)

/* Delete an editable node. */
void UA_NodestoreSwitch_deleteNode(UA_Server* server, UA_Node *node);

/**
 * Insert / Get / Replace / Remove
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
/* Inserts a new node into the nodestore. If the nodeid is zero, then a fresh
 * numeric nodeid from namespace 1 is assigned. If insertion fails, the node is
 * deleted. */
UA_StatusCode UA_NodestoreSwitch_insertNode(UA_Server* server, UA_Node *node,
        UA_NodeId *addedNodeId);

/* The returned node is immutable. */
const UA_Node * UA_NodestoreSwitch_getNode(UA_Server* server, const UA_NodeId *nodeId);

/* Returns an editable copy of a node (needs to be deleted with the deleteNode
   function or inserted / replaced into the nodestore). */
UA_Node * UA_NodestoreSwitch_getNodeCopy(UA_Server* server, const UA_NodeId *nodeId);

/* To replace a node, get an editable copy of the node, edit and replace with
 * this function. If the node was already replaced since the copy was made,
 * UA_STATUSCODE_BADINTERNALERROR is returned. If the nodeid is not found,
 * UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases, the editable
 * node is deleted. */
UA_StatusCode UA_NodestoreSwitch_replaceNode(UA_Server* server, UA_Node *node);

/* Remove a node in the nodestore. */
UA_StatusCode UA_NodestoreSwitch_removeNode(UA_Server* server, const UA_NodeId *nodeId);

/**
 * Release
 * ^^^^^^^
 * Indicates that the reference to a node, which was fetched from the nodestore via the "get" method, is not used anymore.
 * This is the basis for multithreading capable nodestores. Reference Counting and locks can be freed via this method.
 *  */
void UA_NodestoreSwitch_releaseNode(UA_Server* server, const UA_Node *node);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NODESTORE_SWITCH_H_ */
