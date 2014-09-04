/**
 * @brief This files contains helper functions for the UA-Services that are used
 * internally as well (with a simplified API as no access rights are checked).
 */

#include "ua_namespace.h"
#include "ua_types.h"
#include "ua_types_generated.h"

/* @brief Add a reference (and the inverse reference to the target node).
 *
 * @param The node to which the reference shall be added
 * @param The reference itself
 * @param The namespace where the target node is looked up for the reverse reference (this is omitted if targetns is UA_NULL)
 */
UA_Int32 AddReference(UA_Node *node, UA_ReferenceNode *reference, UA_Namespace *targetns);
