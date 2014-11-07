/*
 * ua_namespace.c
 *
 *  Created on: Nov 6, 2014
 *      Author: opcua
 */
#include "ua_namespace.h"
#include "ua_nodestore_interface.h"


UA_Namespace *UA_Namespace_new()
{
	UA_Namespace *n = UA_alloc(sizeof(UA_Namespace));
    if(n) UA_Namespace_init(n);
    return n;
}

void UA_Namespace_init(UA_Namespace *namespace)
{
	namespace->index = 0;
	namespace->nodeStore = UA_NULL;
	UA_String_init(&namespace->url);
}
void UA_Namespace_delete(UA_Namespace *namespace){
	UA_Namespace_deleteMembers(namespace);
	UA_free(namespace);
}
void UA_Namespace_deleteMembers(UA_Namespace *namespace){
	UA_String_deleteMembers(&namespace->url);
	UA_NodeStoreInterface_delete(namespace->nodeStore);
}
