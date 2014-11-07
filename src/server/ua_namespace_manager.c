/*
 * ua_namespace_manager.c
 *
 *  Created on: Oct 14, 2014
 *      Author: opcua
 */
#include "ua_namespace_manager.h"
#include "ua_util.h"




struct namespace_list_entry {
    UA_Namespace namespace;
    LIST_ENTRY(namespace_list_entry) pointers;
};



UA_StatusCode UA_NamespaceManager_init( UA_NamespaceManager* namespaceManager)
{
	LIST_INIT(&namespaceManager->namespaces);
	namespaceManager->currentNamespaceCount = 0;
	return UA_STATUSCODE_GOOD;
}

void UA_NamespaceManager_deleteMembers(UA_NamespaceManager *namespaceManager)
{
	struct namespace_list_entry *current = LIST_FIRST(&namespaceManager->namespaces);
    while(current) {
        LIST_REMOVE(current, pointers);
        UA_Namespace_deleteMembers(&current->namespace);
        UA_free(current);
        current = LIST_FIRST(&namespaceManager->namespaces);
    }

}

UA_StatusCode UA_NamespaceManager_createNamespace(UA_NamespaceManager *namespaceManager,UA_UInt16 index, UA_NodeStoreInterface *nodeStore)
{
	if(namespaceManager->currentNamespaceCount<UA_UINT16_MAX){

		struct namespace_list_entry *newentry = UA_alloc(sizeof(struct namespace_list_entry));
		UA_Namespace_init(&newentry->namespace);
		newentry->namespace.index = index;
		if(nodeStore != UA_NULL ){

			newentry->namespace.nodeStore = UA_NodeStoreInterface_new();
			UA_NodeStoreInterface_copy(nodeStore,newentry->namespace.nodeStore);
		}
		namespaceManager->currentNamespaceCount++;
	    LIST_INSERT_HEAD(&namespaceManager->namespaces, newentry, pointers);

	    return UA_STATUSCODE_GOOD;

	}
	return UA_STATUSCODE_BADNOTFOUND;



}

UA_StatusCode UA_NamespaceManager_removeNamespace(UA_NamespaceManager *namespaceManager,UA_UInt16 index)
{
	UA_Namespace *namespace;
	UA_NamespaceManager_getNamespace(namespaceManager,index,&namespace);
	if(namespace == UA_NULL)
		return UA_STATUSCODE_BADNOTFOUND;

    struct namespace_list_entry *current = UA_NULL;
    LIST_FOREACH(current, &namespaceManager->namespaces, pointers) {
        if(current->namespace.index  == index)
            break;
    }

    if(!current)
        return UA_STATUSCODE_BADNOTFOUND;
	LIST_REMOVE(current, pointers);

	return UA_STATUSCODE_GOOD;
}

UA_Int32 UA_NamespaceManager_getNamespace(UA_NamespaceManager *namespaceManager, UA_UInt16 index, UA_Namespace **ns)
{

    struct namespace_list_entry *current = UA_NULL;
    LIST_FOREACH(current, &namespaceManager->namespaces, pointers) {
        if(current->namespace.index == index)
            break;
    }
    if(!current) {
        *ns = UA_NULL;
        return UA_STATUSCODE_BADNOTFOUND;
    }
    *ns = &current->namespace;
    return UA_STATUSCODE_GOOD;
}

UA_Int32 UA_NamespaceManager_setNodeStore(UA_NamespaceManager *namespaceManager,UA_UInt16 index, UA_NodeStoreInterface *nodeStore)
{
	UA_Namespace *namespace = UA_NULL;
	UA_NamespaceManager_getNamespace(namespaceManager, index, &namespace);
	if(namespace == UA_NULL)
	{
		return UA_STATUSCODE_BADNOTFOUND;
	}
	if(nodeStore != UA_NULL ){
		if(namespace->nodeStore == UA_NULL){
			namespace->nodeStore = UA_NodeStoreInterface_new();
		}
	}
	namespace->nodeStore = nodeStore;
	return UA_STATUSCODE_GOOD;
}
