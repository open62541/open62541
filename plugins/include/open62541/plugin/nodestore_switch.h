/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julian Grothoff, Torben Deppe, Florian Palm
 */

#ifndef UA_NODESTORE_SWITCH_H_
#define UA_NODESTORE_SWITCH_H_

#include <open62541/plugin/nodestore.h>
/*
 * Nodestore interface
 * Holds function pointers to the necessary nodestore functions
 */
typedef struct {
	/* Nodestore context and lifecycle */
	void *context;
	void (*deleteNodestore)(void *nsCtxt);

	/* The following definitions are used to create empty nodes of the different
	 * node types. The memory is managed by the nodestore. Therefore, the node
	 * has to be removed via a special deleteNode function. (If the new node is
	 * not added to the nodestore.) */
	UA_Node * (*newNode)(void *nsCtxt, UA_NodeClass nodeClass);

	void (*deleteNode)(void *nsCtxt, UA_Node *node);

	/* ``Get`` returns a pointer to an immutable node. ``Release`` indicates
	 * that the pointer is no longer accessed afterwards. */
	const UA_Node * (*getNode)(void *nsCtxt, const UA_NodeId *nodeId);

	void (*releaseNode)(void *nsCtxt, const UA_Node *node);

	/* Returns an editable copy of a node (needs to be deleted with the
	 * deleteNode function or inserted / replaced into the nodestore). */
	UA_StatusCode (*getNodeCopy)(void *nsCtxt, const UA_NodeId *nodeId,
			UA_Node **outNode);

	/* Inserts a new node into the nodestore. If the NodeId is zero, then a
	 * fresh numeric NodeId is assigned. If insertion fails, the node is
	 * deleted. */
	UA_StatusCode (*insertNode)(void *nsCtxt, UA_Node *node,
			UA_NodeId *addedNodeId);

	/* To replace a node, get an editable copy of the node, edit and replace
	 * with this function. If the node was already replaced since the copy was
	 * made, UA_STATUSCODE_BADINTERNALERROR is returned. If the NodeId is not
	 * found, UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases,
	 * the editable node is deleted. */
	UA_StatusCode (*replaceNode)(void *nsCtxt, UA_Node *node);

	/* Removes a node from the nodestore. */
	UA_StatusCode (*removeNode)(void *nsCtxt, const UA_NodeId *nodeId);

	/* Execute a callback for every node in the nodestore. */
	void (*iterate)(void *nsCtx, UA_NodestoreVisitor visitor, void *visitorCtx);
} UA_NodestoreInterface;

/*
 * Forward declaration of nodestore switch
 */
struct UA_Nodestore_Switch;
typedef struct UA_Nodestore_Switch UA_Nodestore_Switch;

/*
 * Creates a Nodestore_Switch (like UA_Nodestore_new) but without a default nodestore inside
 * Allocates memory for the switch
 */
UA_StatusCode UA_EXPORT
UA_Nodestore_Switch_newEmpty(void **nsCtx);
/*
 * Creates a new default nodestore and an interface to it
 * Allocates memory for nodestore and the interface
 * The handle to the default nodestore is placed inside the interface context (nsCtx)
 */
UA_StatusCode UA_EXPORT
UA_Nodestore_Default_Interface_new(UA_NodestoreInterface** store);
/*
 * Gets the nodestore interface to an existing switch, which can be created via UA_Nodestore_new or UA_Nodestore_newEmpty
 */
UA_EXPORT UA_NodestoreInterface*
UA_Nodestore_Switch_Interface_get(UA_Nodestore_Switch *storeSwitch);

/*
 * Returns the nodestore interface that is linked to the specified index
 * If useDefault is true, the default nodestore is used if no custom nodestore was linked to the index
 */
UA_EXPORT UA_NodestoreInterface*
UA_Nodestore_Switch_getNodestore(UA_Nodestore_Switch* storeSwitch,
		UA_UInt16 index, UA_Boolean useDefault);
/*
 * Links a nodestore via its nodestore interface to the nodestore switch at the specified namespace index
 * Set with store=NULL is equal to an unlink of the nodestore at the specified index
 * The old store is only unlinked and not deleted
 */
UA_StatusCode UA_EXPORT
UA_Nodestore_Switch_setNodestore(UA_Nodestore_Switch* storeSwitch,
		UA_UInt16 index, UA_NodestoreInterface* store);
/*
 * Links the default nodestore via its nodestore interface to the nodestore switch
 * Set with store=NULL is equal to an unlink of the default nodestore
 * The old store is only unlinked and not deleted
 * The default nodestore is used for every namespace, that has no custom nodestore set
 */
UA_StatusCode UA_EXPORT
UA_Nodestore_Switch_setNodestoreDefault(UA_Nodestore_Switch* storeSwitch,
		UA_NodestoreInterface* store);
/*
 * Replaces a nodestore in all occurances based on the same interface
 * StoreNew=NULL is equal to a complete unlink of the oldNodestore
 */
UA_StatusCode UA_EXPORT
UA_Nodestore_Switch_changeNodestore(UA_Nodestore_Switch* storeSwitch,
		UA_NodestoreInterface *storeOld, UA_NodestoreInterface *storeNew);
/*
 * Gets all namespace indices linked to a nodestore interface in the switch
 * Comparision of nodestores is based on the nodestore interface
 * Returns count of occurances
 * If indices parameter is not NULL an array of namespace indices is returned
 */
UA_StatusCode UA_EXPORT
UA_Nodestore_Switch_getIndices(UA_Nodestore_Switch* storeSwitch,
		UA_NodestoreInterface* store, UA_UInt16* count, UA_UInt16** indices);

/**********************************************************
 * Copy of default nodestore (include/open62541/plugin/nodestore.h),
 * as it is unlinked with UA_ENABLE_CUSTOM_NODESTORE
 * (See https://github.com/open62541/open62541/pull/2748#issuecomment-496834686)
 **********************************************************/

UA_Node *
UA_Nodestore_Default_newNode(void *nsCtx, UA_NodeClass nodeClass);
const UA_Node *
UA_Nodestore_Default_getNode(void *nsCtx, const UA_NodeId *nodeId);
void
UA_Nodestore_Default_deleteNode(void *nsCtx, UA_Node *node);
void
UA_Nodestore_Default_releaseNode(void *nsCtx, const UA_Node *node);
UA_StatusCode
UA_Nodestore_Default_getNodeCopy(void *nsCtx, const UA_NodeId *nodeId,
                         UA_Node **outNode);
UA_StatusCode
UA_Nodestore_Default_insertNode(void *nsCtx, UA_Node *node, UA_NodeId *addedNodeId);
UA_StatusCode
UA_Nodestore_Default_removeNode(void *nsCtx, const UA_NodeId *nodeId);
void
UA_Nodestore_Default_iterate(void *nsCtx, UA_NodestoreVisitor visitor,
                     void *visitorCtx);
UA_StatusCode
UA_Nodestore_Default_replaceNode(void *nsCtx, UA_Node *node);
UA_StatusCode
UA_Nodestore_Default_new(void **nsCtx);
void
UA_Nodestore_Default_delete(void *nsCtx);

#endif /* UA_NODESTORE_SWITCH_H_ */
