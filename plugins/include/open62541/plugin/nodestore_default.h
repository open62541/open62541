/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2019 (c) Julian Grothoff, Torben Deppe, Florian Palm
 */

#ifndef UA_NODESTORE_DEFAULT_H_
#define UA_NODESTORE_DEFAULT_H_

#include <open62541/plugin/nodestore.h>

/* Plug in default nodestore if custom nodestores are disabled */
#ifndef UA_ENABLE_CUSTOM_NODESTORE
#define UA_Nodestore_Default_new UA_Nodestore_new
#define UA_Nodestore_Default_delete UA_Nodestore_delete
#define UA_Nodestore_Default_newNode UA_Nodestore_newNode
#define UA_Nodestore_Default_deleteNode UA_Nodestore_deleteNode
#define UA_Nodestore_Default_getNode UA_Nodestore_getNode
#define UA_Nodestore_Default_releaseNode UA_Nodestore_releaseNode
#define UA_Nodestore_Default_getNodeCopy UA_Nodestore_getNodeCopy
#define UA_Nodestore_Default_insertNode UA_Nodestore_insertNode
#define UA_Nodestore_Default_replaceNode UA_Nodestore_replaceNode
#define UA_Nodestore_Default_removeNode UA_Nodestore_removeNode
#define UA_Nodestore_Default_iterate UA_Nodestore_iterate
#else
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
#endif /* UA_ENABLE_CUSTOM_NODESTORE */

#endif /* UA_NODESTORE_DEFAULT_H_ */
