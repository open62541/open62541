/*
 * ua_namespace_manager.c
 *
 *  Created on: Oct 14, 2014
 *      Author: opcua
 */
#include "ua_util.h"
#include "ua_namespace_manager.h"



struct namespace_list_entry {
    UA_Namespace namespace;
    LIST_ENTRY(namespace_list_entry) pointers;
};

struct UA_NamespaceManager {
    LIST_HEAD(namespace_list, namespace_list_entry) namespaces;
    UA_UInt32    currentNamespaceCount;
};

void UA_NamespaceManager_new(UA_NamespaceManager** namespaceManager)
{
	*namespaceManager = UA_alloc(sizeof(UA_NamespaceManager));
	(*namespaceManager)->currentNamespaceCount = 0;

}

UA_StatusCode UA_NamespaceManager_addNamespace(UA_NamespaceManager *namespaceManager, UA_UInt16 index, UA_NodeStore *nodeStore)
{
	if(namespaceManager->currentNamespaceCount<UA_UINT16_MAX){
		namespaceManager->currentNamespaceCount++;
		struct namespace_list_entry *newentry = UA_alloc(sizeof(struct namespace_list_entry));
		newentry->namespace.index = index;
		newentry->namespace.nodeStore = nodeStore;
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

UA_Int32 UA_NamespaceManager_setNodeStore(UA_NamespaceManager *namespaceManager,UA_UInt16 index, UA_NodeStore *nodeStore)
{
	UA_Namespace *namespace = UA_NULL;
	UA_NamespaceManager_getNamespace(namespaceManager,index,&namespace);
	if(namespace == UA_NULL)
	{
		return UA_STATUSCODE_BADNOTFOUND;
	}
	namespace->nodeStore = nodeStore;
	return UA_STATUSCODE_GOOD;
}
