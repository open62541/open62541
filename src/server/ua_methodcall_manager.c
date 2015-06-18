#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_statuscodes.h"
#include "ua_util.h"
#include "ua_nodestore.h"
#include "ua_methodcall_manager.h"

/* Method Hook/List management */
UA_NodeAttachedMethod *UA_NodeAttachedMethod_new(void) {
    UA_NodeAttachedMethod *newItem = (UA_NodeAttachedMethod *) UA_malloc(sizeof(UA_NodeAttachedMethod));
    newItem->method = UA_NULL;
    //UA_NodeId_init(&newItem->methodNodeId);
    //newItem->listEntry.le_next=NULL;
    //newItem->listEntry.le_prev=NULL;
    return newItem;
}

UA_ArgumentsList *UA_ArgumentsList_new(UA_UInt32 statusSize, UA_UInt32 argumentsSize){
    UA_ArgumentsList *newAList = (UA_ArgumentsList *) UA_malloc(sizeof(UA_ArgumentsList));
    newAList->argumentsSize = argumentsSize;
    newAList->statusSize = statusSize;
    newAList->callResult = UA_STATUSCODE_GOOD;
    if (statusSize > 0) {
        newAList->status = (UA_StatusCode *) UA_malloc(sizeof(UA_StatusCode) * statusSize);
        for (unsigned int i=0; i<statusSize; i++)
            UA_StatusCode_init(&newAList->status[i]);
    }
    if (argumentsSize > 0) {
        newAList->arguments = (UA_Variant *) UA_malloc(sizeof(UA_Variant) * argumentsSize);
        for (unsigned int i=0; i<argumentsSize; i++)
            UA_Variant_init(&newAList->arguments[i]);
    }
    return newAList;
}

void UA_ArgumentsList_deleteMembers(UA_ArgumentsList *value) {
    if (value->statusSize > 0) {
        UA_free(value->status);
    }
    if (value->argumentsSize > 0) {
        for (unsigned int i=0; i<value->argumentsSize; i++)
            UA_Variant_deleteMembers(&value->arguments[i]);
        UA_free(value->arguments);
    }
}

void UA_ArgumentsList_destroy(UA_ArgumentsList *value) {
    UA_ArgumentsList_deleteMembers(value);
    UA_free(value);
}

/* User facing functions */
UA_StatusCode UA_Server_detachMethod_fromNode(UA_Server *server, const UA_NodeId methodNodeId) {
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_attachMethod_toNode(UA_Server *server, UA_NodeId methodNodeID, void *method) {    
    const UA_Node *methodNode = UA_NodeStore_get(server->nodestore, (const UA_NodeId *) &methodNodeID);
    
    if (server == UA_NULL)
        return UA_STATUSCODE_BADSERVERINDEXINVALID;
    
    if (method == UA_NULL)
        return UA_STATUSCODE_BADMETHODINVALID;
    
    if (methodNode == UA_NULL)
        return UA_STATUSCODE_BADNODEIDINVALID;
    
    if ( methodNode->nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADMETHODINVALID;
    
    UA_MethodNode *replacement = UA_MethodNode_new();
    if (!replacement) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_MethodNode_copy((const UA_MethodNode *) methodNode, replacement);
    if( retval != UA_STATUSCODE_GOOD) {
        return retval;
    }
    replacement->attachedMethod.method = method;
    
    retval |= UA_NodeStore_replace(server->nodestore, methodNode, (UA_Node *) replacement, UA_NULL);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_MethodNode_delete(replacement);
        return retval;
    }
    UA_NodeStore_release(methodNode);
    
    // Cannot change a constant node.
    //((UA_MethodNode *) methodNode)->executable = UA_TRUE;
    // FIXME: This should be set by the namespace/server, not this handler.
    //((UA_MethodNode *) methodNode)->userExecutable = UA_TRUE;
    return retval;
}