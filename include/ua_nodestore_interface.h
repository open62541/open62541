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

#ifndef UA_NODESTORE_INTERFACE_H_
#define UA_NODESTORE_INTERFACE_H_

#include "ua_types_generated.h"
#include "ua_nodes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NodestoreInterface Type Definition
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * Definition of the function signatures for the NodestoreInterface
 */

/**
 * Nodestore Lifecycle
 * ^^^^^^^^^^^^^^^^^^^ */

/* Delete the nodestore and all nodes in it. Do not call from a read-side
   critical section (multithreading). */
typedef void (*UA_NodestoreInterface_delete)(void *handle);


/**
 * Node Lifecycle
 * ^^^^^^^^^^^^^^
 *
 * The following definitions are used to create empty nodes of the different
 * node types. The memory is managed by the nodestore. Therefore, the node has
 * to be removed via a special deleteNode function. (If the new node is not
 * added to the nodestore.) */
/* Create an editable node of the given NodeClass. */
typedef UA_Node * (*UA_NodestoreInterface_newNode)(UA_NodeClass nodeClass);

/* Delete an editable node. */
typedef void (*UA_NodestoreInterface_deleteNode)(UA_Node *node);

/**
 * Insert / Get / Replace / Remove
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
/* Inserts a new node into the nodestore. If the nodeid is zero, then a fresh
 * numeric nodeid from namespace 1 is assigned. If insertion fails, the node is
 * deleted. */
typedef UA_StatusCode (*UA_NodestoreInterface_insert)(void *handle, UA_Node *node);

/* The returned node is immutable. */
typedef const UA_Node * (*UA_NodestoreInterface_get)(void *handle, const UA_NodeId *nodeid);

/* Returns an editable copy of a node (needs to be deleted with the deleteNode
   function or inserted / replaced into the nodestore). */
typedef UA_Node * (*UA_NodestoreInterface_getCopy)(void *handle, const UA_NodeId *nodeid);

/* To replace a node, get an editable copy of the node, edit and replace with
 * this function. If the node was already replaced since the copy was made,
 * UA_STATUSCODE_BADINTERNALERROR is returned. If the nodeid is not found,
 * UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases, the editable
 * node is deleted. */
typedef UA_StatusCode (*UA_NodestoreInterface_replace)(void *handle, UA_Node *node);

/* Remove a node in the nodestore. */
typedef UA_StatusCode (*UA_NodestoreInterface_remove)(void *handle, const UA_NodeId *nodeid);

/**
 * Iteration
 * ^^^^^^^^^
 * The following definitions are used to call a callback for every node in the
 * nodestore. */
typedef void (*UA_Nodestore_nodeVisitor)(const UA_Node *node);
typedef void (*UA_NodestoreInterface_iterate)(void *handle, UA_Nodestore_nodeVisitor visitor);

/**
 * Release
 * ^^^^^^^^
 * Prototype for implementation of multithreading capable nodestores with reference counters instead of RCU LOCK.
 * Indicates that the node is no longer referenced by the caller.
 */
typedef void (*UA_NodestoreInterface_release)(void *handle, const UA_Node *node);


/**
 * NodestoreInterface Type
 * ^^^^^^^^^^^^^^^^^^^^^^^
 * Definition of the NodestoreInterface with function pointers to the nodestore.
 */
typedef struct UA_NodestoreInterface {
    void *                              handle;
    UA_NodestoreInterface_delete        deleteNodeStore;

    UA_NodestoreInterface_newNode       newNode;
    UA_NodestoreInterface_deleteNode    deleteNode;

    UA_NodestoreInterface_insert        insert;
    UA_NodestoreInterface_get           get;
    UA_NodestoreInterface_getCopy       getCopy;
    UA_NodestoreInterface_replace       replace;
    UA_NodestoreInterface_remove        remove;
    UA_NodestoreInterface_iterate       iterate;
    UA_NodestoreInterface_release       release;
}UA_NodestoreInterface;

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NODESTORE_INTERFACE_H_ */
