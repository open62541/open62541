#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_statuscodes.h"
#include "ua_util.h"
#include "ua_nodestore.h"
#include "ua_methodcall_manager.h"

/* Method Call Manager */
UA_MethodCall_Manager *UA_MethodCallManager_new(void) {
    UA_MethodCall_Manager *manager = (UA_MethodCall_Manager *) UA_malloc(sizeof(UA_MethodCall_Manager));
    LIST_INIT(&manager->attachedMethods);
    return manager;
}

void UA_MethodCallManager_deleteMembers(UA_MethodCall_Manager *manager) {
    UA_NodeAttachedMethod *attMethod;
    
    while (manager->attachedMethods.lh_first != NULL) {
        attMethod = manager->attachedMethods.lh_first;
        LIST_REMOVE(attMethod, listEntry);
        UA_free(attMethod);
    }
    
    return;
}

void UA_MethodCallManager_destroy(UA_MethodCall_Manager *manager) {
    UA_MethodCallManager_deleteMembers(manager);
    UA_free(manager);
    return;
}

/* Method Hook/List management */
UA_NodeAttachedMethod *UA_NodeAttachedMethod_new(void) {
    UA_NodeAttachedMethod *newItem = (UA_NodeAttachedMethod *) UA_malloc(sizeof(UA_NodeAttachedMethod));
    newItem->method = UA_NULL;
    UA_NodeId_init(&newItem->methodNodeId);
    newItem->listEntry.le_next=NULL;
    newItem->listEntry.le_prev=NULL;
    return newItem;
}

UA_NodeAttachedMethod  *UA_MethodCallManager_getMethodByNodeId(UA_MethodCall_Manager *manager, UA_NodeId methodId) {
    UA_NodeAttachedMethod  *am;
    LIST_FOREACH(am, &manager->attachedMethods, listEntry) {
        if(UA_NodeId_equal(&methodId, &am->methodNodeId)) {
            return am;
        }
    }
    return UA_NULL;
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

UA_StatusCode UA_Server_attachMethod_toNode(UA_Server *server, const UA_NodeId methodNodeId, void *method){
    const UA_Node *mNode;
    
    if (server == UA_NULL)
        return UA_STATUSCODE_BADSERVERINDEXINVALID;
    
    if (method == UA_NULL)
        return UA_STATUSCODE_BADMETHODINVALID;
    
    mNode = UA_NodeStore_get(server->nodestore, &methodNodeId);
    if (mNode == UA_NULL)
        return UA_STATUSCODE_BADMETHODINVALID;
    
    if (mNode->nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADMETHODINVALID;
    
    UA_MethodCall_Manager *manager = server->methodCallManager;
    UA_NodeAttachedMethod *newHook = UA_NodeAttachedMethod_new();
    
    UA_NodeId_copy(&methodNodeId, &newHook->methodNodeId);
    newHook->method = method;
    
    LIST_INSERT_HEAD(&manager->attachedMethods, newHook, listEntry);
    return UA_STATUSCODE_GOOD;
}