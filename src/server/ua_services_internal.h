/**
 * @brief This files contains helper functions for the UA-Services that are used
 * internally as well (with a simplified API as no access rights are checked).
 */

#include "ua_session.h"
#include "ua_nodestore.h"
#include "ua_types_generated.h"

/* @brief Add a reference (and the inverse reference to the target node).
 *
 * @param The node to which the reference shall be added
 * @param The reference itself
 * @param The namespace where the target node is looked up for the reverse reference (this is omitted if targetns is UA_NULL)
 */
UA_Int32 AddReference(UA_NodeStoreExample *nodestore, UA_Node *node, UA_ReferenceNode *reference);
UA_AddNodesResult AddNode(UA_Server *server, UA_Session *session, UA_Node **node,
                          UA_ExpandedNodeId *parentNodeId, UA_NodeId *referenceTypeId);
