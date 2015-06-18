#ifndef UA_METHODCALL_H
#define UA_METHODCALL_H

#include "ua_util.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#include "ua_server.h"

typedef struct UA_ArgumentsList_s {
    UA_StatusCode callResult;
    UA_UInt32     statusSize;
    UA_StatusCode *status;
    UA_UInt32     argumentsSize;
    UA_Variant    *arguments;
}
UA_ArgumentsList;

typedef struct UA_NodeAttachedMethod_s {
    //UA_NodeId methodNodeId;
    void* (*method)(const void *object, const UA_ArgumentsList *InputArguments, UA_ArgumentsList *OutputArguments);
    //LIST_ENTRY(UA_NodeAttachedMethod_s) listEntry;
} UA_NodeAttachedMethod;

/* Method Hook/List management */
UA_NodeAttachedMethod *UA_NodeAttachedMethod_new(void);

UA_ArgumentsList UA_EXPORT *UA_ArgumentsList_new(UA_UInt32 statusSize, UA_UInt32 argumentsSize);
void             UA_EXPORT UA_ArgumentsList_deleteMembers(UA_ArgumentsList *value);
void             UA_EXPORT UA_ArgumentsList_destroy(UA_ArgumentsList *value);

/* User facing functions */
UA_StatusCode UA_EXPORT UA_Server_detachMethod_fromNode(UA_Server *server, UA_NodeId const methodNodeId);
//UA_StatusCode UA_EXPORT UA_Server_attachMethod_toNode(UA_Server *server, UA_NodeId const methodNodeId, void* method);
UA_StatusCode UA_EXPORT UA_Server_attachMethod_toNode(UA_Server *server, UA_NodeId methodNodeID, void *method) ;
#endif