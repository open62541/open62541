 /*
 * Copyright (C) 2014 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#ifndef UA_NODESTORE_H_
#define UA_NODESTORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_nodes.h"
#include "ua_types.h"

struct UA_NodeStoreInterface;
typedef struct UA_NodeStoreInterface UA_NodeStoreInterface;

typedef struct UA_NodeStore{
	UA_NodeStoreInterface** nodeStoreInterfaces;
	UA_UInt16 nodeStoreInterfacesSize;
} UA_NodeStore;

/**
 * Nodestore Lifecycle
 * ^^^^^^^^^^^^^^^^^^^ */
/* Create a new nodestore */
UA_NodeStore * UA_NodeStore_new(void);

/* Delete the nodestore and all nodes in it. Do not call from a read-side
   critical section (multithreading). */
void UA_NodeStore_delete(void);

/**
 * Change Nodestore
 * ^^^^^^^^^^^^^^^^
 *
 */
UA_Boolean UA_NodeStore_add(UA_NodeStore *ns, UA_NodeStoreInterface *nodeStoreInterface);
UA_Boolean UA_NodeStore_change(UA_NodeStore *ns, UA_NodeStoreInterface *nodeStoreInterface, UA_UInt16 nodeStoreIndex);
//TODO Add: Get all Namespaces for a NodeStore. --> Export this Functions

/**
 * Node Lifecycle
 * ^^^^^^^^^^^^^^
 * The memory is managed by the nodestore. Therefore, the node has
 * to be removed via a special deleteNode function. (If the new node is not
 * added to the nodestore.) */
/* Create an editable node of the given NodeClass. */
UA_Node * UA_NodeStore_newNode(UA_NodeClass nodeClass, UA_UInt16 nodeStoreIndex);
/*
 * The following definitions are used to create empty nodes of the different
 * node types in NameSpace 0.
 */
#define UA_NodeStore_newObjectNode_0() \
    (UA_ObjectNode*)UA_NodeStore_newNode(UA_NODECLASS_OBJECT, 0)
#define UA_NodeStore_newVariableNode_0() \
    (UA_VariableNode*)UA_NodeStore_newNode(UA_NODECLASS_VARIABLE, 0)
#define UA_NodeStore_newMethodNode_0() \
    (UA_MethodNode*)UA_NodeStore_newNode(UA_NODECLASS_METHOD, 0)
#define UA_NodeStore_newObjectTypeNode_0() \
    (UA_ObjectTypeNode*)UA_NodeStore_newNode(UA_NODECLASS_OBJECTTYPE, 0)
#define UA_NodeStore_newVariableTypeNode_0() \
    (UA_VariableTypeNode*)UA_NodeStore_newNode(UA_NODECLASS_VARIABLETYPE, 0)
#define UA_NodeStore_newReferenceTypeNode_0() \
    (UA_ReferenceTypeNode*)UA_NodeStore_newNode(UA_NODECLASS_REFERENCETYPE, 0)
#define UA_NodeStore_newDataTypeNode_0() \
    (UA_DataTypeNode*)UA_NodeStore_newNode(UA_NODECLASS_DATATYPE, 0)
#define UA_NodeStore_newViewNode_0() \
    (UA_ViewNode*)UA_NodeStore_newNode(UA_NODECLASS_VIEW, 0)

/* Delete an editable node. */
void UA_NodeStore_deleteNode(UA_Node *node);

/**
 * Insert / Get / Replace / Remove
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
/* Inserts a new node into the nodestore. If the nodeid is zero, then a fresh
 * numeric nodeid from namespace 1 is assigned. If insertion fails, the node is
 * deleted. */
UA_StatusCode UA_NodeStore_insert(UA_NodeStore *ns, UA_Node *node);

/* The returned node is immutable. */
const UA_Node * UA_NodeStore_get(UA_NodeStore *ns, const UA_NodeId *nodeId);

/* Returns an editable copy of a node (needs to be deleted with the deleteNode
   function or inserted / replaced into the nodestore). */
UA_Node * UA_NodeStore_getCopy(UA_NodeStore *ns, const UA_NodeId *nodeId);

/* To replace a node, get an editable copy of the node, edit and replace with
 * this function. If the node was already replaced since the copy was made,
 * UA_STATUSCODE_BADINTERNALERROR is returned. If the nodeid is not found,
 * UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases, the editable
 * node is deleted. */
UA_StatusCode UA_NodeStore_replace(UA_NodeStore *ns, UA_Node *node);

/* Remove a node in the nodestore. */
UA_StatusCode UA_NodeStore_remove(UA_NodeStore *ns, const UA_NodeId *nodeId);

/**
 * Iteration
 * ^^^^^^^^^
 * The following definitions are used to call a callback for every node in the
 * nodestore. */
typedef void (*UA_NodeStore_nodeVisitor)(const UA_Node *node);
void UA_NodeStore_iterate(UA_NodeStore *ns, UA_NodeStore_nodeVisitor visitor);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NODESTORE_H_ */
