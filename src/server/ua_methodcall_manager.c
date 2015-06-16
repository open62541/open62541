#include "ua_methodcall_manager.h"

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

UA_StatusCode UA_Server_detachMethod_fromNode(UA_Server *server, UA_NodeId methodNodeId) {
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_attachMethod_toNode(UA_Server *server, UA_NodeId methodNodeId, UA_Variant* *method){
    return UA_STATUSCODE_GOOD;
}