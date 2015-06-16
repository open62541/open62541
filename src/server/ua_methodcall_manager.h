#ifndef UA_METHODCALL_H
#define UA_METHODCALL_H

#include "ua_util.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_nodes.h"

typedef struct UA_NodeAttachedMethod_s {
    UA_Node methodNodeId;
    UA_Variant* (*method)(UA_Node *object, UA_UInt32 InputArgumentsSize, UA_Variant *InputArguments, UA_UInt32 *OutputArgumentsSize);
    LIST_ENTRY(UA_NodeAttachedMethod_s) listEntry;
} UA_NodeAttachedMethod;

typedef struct UA_MethodCall_Manager_s {
    LIST_HEAD(UA_ListOfUAAttachedMethods, UA_NodeAttachedMethod_s) attachedMethods;
} UA_MethodCall_Manager;

UA_MethodCall_Manager *UA_MethodCallManager_new(void);
void UA_MethodCallManager_deleteMembers(UA_MethodCall_Manager *manager);
void UA_MethodCallManager_destroy(UA_MethodCall_Manager *manager);

UA_StatusCode UA_Server_detachMethod_fromNode(UA_Server *server, UA_NodeId methodNodeId);
UA_StatusCode UA_Server_attachMethod_toNode(UA_Server *server, UA_NodeId methodNodeId, UA_Variant* *method);
#endif