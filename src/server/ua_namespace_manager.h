/*
 * ua_namespace_manager.h
 *
 *  Created on: Oct 14, 2014
 *      Author: opcua
 */

#ifndef UA_NAMESPACE_MANAGER_H_
#define UA_NAMESPACE_MANAGER_H_
#include "ua_server.h"
#include "ua_nodestore_interface.h"


UA_NamespaceManager* UA_NamespaceManager_new();

UA_Int32  UA_NamespaceManager_addNamespace(UA_NamespaceManager *namespaceManager, UA_UInt16 index, UA_NodeStoreInterface *nodeStore);

UA_Int32  UA_NamespaceManager_removeNamespace(UA_NamespaceManager *namespaceManager,UA_UInt16 index);

UA_Int32  UA_NamespaceManager_getNamespace(UA_NamespaceManager *namespaceManager, UA_UInt16 index, UA_Namespace **ns);

UA_Int32  UA_NamespaceManager_setNodeStore(UA_NamespaceManager *namespaceManager,UA_UInt16 index, UA_NodeStoreInterface *nodeStore);



#endif /* UA_NAMESPACE_MANAGER_H_ */
