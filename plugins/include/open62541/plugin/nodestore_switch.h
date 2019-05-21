/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julian Grothoff, Torben Deppe, Florian Palm
 */

#ifndef UA_NODESTORE_SWITCH_H_
#define UA_NODESTORE_SWITCH_H_

#include <open62541/plugin/nodestore.h>
#include <open62541/plugin/nodestore_default.h>

/* Plug in nodestore switch if enabled */
#define UA_Nodestore_Switch_new UA_Nodestore_new
#define UA_Nodestore_Switch_delete UA_Nodestore_delete
#define UA_Nodestore_Switch_newNode UA_Nodestore_newNode
#define UA_Nodestore_Switch_deleteNode UA_Nodestore_deleteNode
#define UA_Nodestore_Switch_getNode UA_Nodestore_getNode
#define UA_Nodestore_Switch_releaseNode UA_Nodestore_releaseNode
#define UA_Nodestore_Switch_getNodeCopy UA_Nodestore_getNodeCopy
#define UA_Nodestore_Switch_insertNode UA_Nodestore_insertNode
#define UA_Nodestore_Switch_replaceNode UA_Nodestore_replaceNode
#define UA_Nodestore_Switch_removeNode UA_Nodestore_removeNode
#define UA_Nodestore_Switch_iterate UA_Nodestore_iterate

/**
 * Nodestore interface and switch definition
 */
typedef struct {
    /* Nodestore context and lifecycle */
    void *context;
    void (*deleteNodestore)(void *nodestoreContext);

    /* The following definitions are used to create empty nodes of the different
     * node types. The memory is managed by the nodestore. Therefore, the node
     * has to be removed via a special deleteNode function. (If the new node is
     * not added to the nodestore.) */
    UA_Node * (*newNode)(void *nodestoreContext, UA_NodeClass nodeClass);

    void (*deleteNode)(void *nodestoreContext, UA_Node *node);

    /* ``Get`` returns a pointer to an immutable node. ``Release`` indicates
     * that the pointer is no longer accessed afterwards. */

    const UA_Node * (*getNode)(void *nodestoreContext, const UA_NodeId *nodeId);

    void (*releaseNode)(void *nodestoreContext, const UA_Node *node);

    /* Returns an editable copy of a node (needs to be deleted with the
     * deleteNode function or inserted / replaced into the nodestore). */
    UA_StatusCode (*getNodeCopy)(void *nodestoreContext, const UA_NodeId *nodeId,
                                 UA_Node **outNode);

    /* Inserts a new node into the nodestore. If the NodeId is zero, then a
     * fresh numeric NodeId is assigned. If insertion fails, the node is
     * deleted. */
    UA_StatusCode (*insertNode)(void *nodestoreContext, UA_Node *node,
                                UA_NodeId *addedNodeId);

    /* To replace a node, get an editable copy of the node, edit and replace
     * with this function. If the node was already replaced since the copy was
     * made, UA_STATUSCODE_BADINTERNALERROR is returned. If the NodeId is not
     * found, UA_STATUSCODE_BADNODEIDUNKNOWN is returned. In both error cases,
     * the editable node is deleted. */
    UA_StatusCode (*replaceNode)(void *nodestoreContext, UA_Node *node);

    /* Removes a node from the nodestore. */
    UA_StatusCode (*removeNode)(void *nodestoreContext, const UA_NodeId *nodeId);

    /* Execute a callback for every node in the nodestore. */
    void (*iterate)(void *nsCtx, UA_NodestoreVisitor visitor,
            void *visitorCtx);
} UA_NodestoreInterface;

typedef struct UA_Nodestore_Switch {
	UA_UInt16 size;
	UA_NodestoreInterface *defaultNodestore;
	UA_NodestoreInterface **nodestoreArray;
}UA_Nodestore_Switch;

UA_StatusCode UA_Nodestore_Switch_newEmpty(void **nsCtx);
UA_StatusCode UA_Nodestore_Default_Interface_new(UA_NodestoreInterface** nsInterface);
void UA_Nodestore_copy(const UA_NodestoreInterface* src, UA_NodestoreInterface* dst);
void UA_Nodestore_Switch_deleteNodestores(void *storeSwitchHandle);


/*
 * Changes of namespace to nodestore mapping.
 */
UA_StatusCode UA_Nodestore_Switch_linkDefaultNodestore(UA_Nodestore_Switch *pSwitch, UA_NodestoreInterface *ns);
UA_StatusCode UA_Nodestore_Switch_changeNodestore(UA_Nodestore_Switch *pSwitch, void *nodestoreHandleOut, UA_NodestoreInterface *nsIn);
UA_StatusCode UA_Nodestore_Switch_linkNodestoreToNamespace(UA_Nodestore_Switch *storeSwitch, UA_NodestoreInterface *ns, UA_UInt16 namespaceindex);
UA_StatusCode UA_Nodestore_Switch_unlinkNodestoreFromNamespace(UA_Nodestore_Switch *storeSwitch, UA_NodestoreInterface *ns);
void UA_Nodestore_Switch_linkSwitchToStore(UA_Nodestore_Switch *storeSwitch ,UA_NodestoreInterface *store);

#endif /* UA_NODESTORE_SWITCH_H_ */
