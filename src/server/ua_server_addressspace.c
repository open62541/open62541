#include "ua_server.h"
#include "ua_services.h"
#include "ua_server_internal.h"

UA_StatusCode UA_Server_deleteNode(UA_Server *server, UA_NodeId nodeId) {
  union nodeUnion {
    const UA_Node *delNodeConst;
    UA_Node *delNode;
  } ptrs;
  ptrs.delNodeConst = UA_NodeStore_get(server->nodestore, &nodeId);
  
  if (!ptrs.delNodeConst)
    return UA_STATUSCODE_BADNODEIDINVALID;
  UA_NodeStore_release(ptrs.delNodeConst);
  
  // Remove the node from the hashmap/slot
  UA_NodeStore_remove(server->nodestore, &nodeId);
  
  /* 
   * FIXME: Delete unreachable child nodes???
   */
  
  return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId,
                                             UA_NodeIteratorCallback callback, void *handle) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId);
    if(!parent)
        return UA_STATUSCODE_BADNODEIDINVALID;
    
    for(int i=0; i<parent->referencesSize; i++) {
        UA_ReferenceNode *ref = &parent->references[i];
        retval |= callback(ref->targetId.nodeId, ref->isInverse, ref->referenceTypeId, handle);
    }
    
    UA_NodeStore_release(parent);
    return retval;
}

UA_StatusCode
UA_Server_addVariableNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                          const UA_LocalizedText displayName, const UA_LocalizedText description,
                          const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                          const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                          UA_Variant *value, UA_NodeId *createdNodeId) {
    UA_VariableNode *node = UA_VariableNode_new();
    UA_StatusCode retval;
    node->value.variant = *value; // copy content
    UA_NodeId_copy(&nodeId, &node->nodeId);
    UA_QualifiedName_copy(&browseName, &node->browseName);
    UA_LocalizedText_copy(&displayName, &node->displayName);
    UA_LocalizedText_copy(&description, &node->description);
    node->writeMask = writeMask;
    node->userWriteMask = userWriteMask;
    UA_ExpandedNodeId parentId;
    UA_ExpandedNodeId_init(&parentId);
    UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
    UA_AddNodesResult res = UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node,
                                                         parentId, referenceTypeId);
    UA_Server_addReference(server, res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                           UA_EXPANDEDNODEID_NUMERIC(0, value->type->typeId.identifier.numeric));
    
    if(res.statusCode != UA_STATUSCODE_GOOD) {
        UA_Variant_init(&node->value.variant);
        UA_VariableNode_delete(node);
    } else 
        UA_free(value);
    retval = res.statusCode;
    if (createdNodeId != UA_NULL)
      UA_NodeId_copy(&res.addedNodeId, createdNodeId);
    UA_AddNodesResult_deleteMembers(&res);
    UA_ExpandedNodeId_deleteMembers(&parentId);
    return retval;
}

UA_StatusCode
UA_Server_addObjectNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                        const UA_LocalizedText displayName, const UA_LocalizedText description,
                        const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_ExpandedNodeId typeDefinition, UA_NodeId *createdNodeId){
    UA_ObjectNode *node = UA_ObjectNode_new();
    UA_StatusCode retval;
    
    UA_NodeId_copy(&nodeId, &node->nodeId);
    UA_QualifiedName_copy(&browseName, &node->browseName);
    UA_LocalizedText_copy(&displayName, &node->displayName);
    UA_LocalizedText_copy(&description, &node->description);
    node->writeMask = writeMask;
    node->userWriteMask = userWriteMask;
    UA_ExpandedNodeId parentId; // we need an expandednodeid
    UA_ExpandedNodeId_init(&parentId);
    UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
    UA_AddNodesResult res = UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node,
                                                         parentId, referenceTypeId);
    if(res.statusCode != UA_STATUSCODE_GOOD)
        UA_ObjectNode_delete(node);
    retval = res.statusCode;
    if (createdNodeId != UA_NULL)
      UA_NodeId_copy(&res.addedNodeId, createdNodeId);
    UA_AddNodesResult_deleteMembers(&res);

    if(!UA_NodeId_isNull(&typeDefinition.nodeId)) {
      UA_Server_addReference(server, res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION), typeDefinition);
    }
    return retval;
}

UA_StatusCode
UA_Server_addDataSourceVariableNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                                    const UA_LocalizedText displayName, const UA_LocalizedText description,
                                    const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                    const UA_DataSource dataSource, UA_NodeId *createdNodeId) {
    UA_VariableNode *node = UA_VariableNode_new();
    UA_StatusCode retval;
    
    node->valueSource = UA_VALUESOURCE_DATASOURCE;
    node->value.dataSource = dataSource;
    UA_NodeId_copy(&nodeId, &node->nodeId);
    UA_QualifiedName_copy(&browseName, &node->browseName);
    UA_LocalizedText_copy(&displayName, &node->displayName);
    UA_LocalizedText_copy(&description, &node->description);
    node->writeMask = writeMask;
    node->userWriteMask = userWriteMask;
    UA_ExpandedNodeId parentId; // dummy exapndednodeid
    UA_ExpandedNodeId_init(&parentId);
    UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
    UA_AddNodesResult res = UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node,
                                                         parentId, referenceTypeId);
    UA_Server_addReference(server, res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE));
    if(res.statusCode != UA_STATUSCODE_GOOD)
        UA_VariableNode_delete(node);
    retval = res.statusCode;
    if (createdNodeId != UA_NULL)
      UA_NodeId_copy(&res.addedNodeId, createdNodeId);
    UA_AddNodesResult_deleteMembers(&res);
    return retval;
}

                                    
UA_StatusCode
UA_Server_addVariableTypeNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                              const UA_LocalizedText displayName, const UA_LocalizedText description,
                              const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                              const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                              UA_Variant *value, const UA_Int32 valueRank, const UA_Boolean isAbstract,
                              UA_NodeId *createdNodeId) {
    UA_VariableTypeNode *node = UA_VariableTypeNode_new();
    UA_StatusCode retval;
    node->value.variant = *value; // copy content
    UA_NodeId_copy(&nodeId, &node->nodeId);
    UA_QualifiedName_copy(&browseName, &node->browseName);
    UA_LocalizedText_copy(&displayName, &node->displayName);
    UA_LocalizedText_copy(&description, &node->description);
    node->writeMask = writeMask;
    node->userWriteMask = userWriteMask;
    UA_ExpandedNodeId parentId;
    UA_ExpandedNodeId_init(&parentId);
    UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
    node->isAbstract = isAbstract;
    node->valueRank  = valueRank;
    UA_AddNodesResult res = UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node,
                                                         parentId, referenceTypeId);
    UA_Server_addReference(server, res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION),
                           UA_EXPANDEDNODEID_NUMERIC(0, value->type->typeId.identifier.numeric));
    
    if(res.statusCode != UA_STATUSCODE_GOOD) {
        UA_Variant_init(&node->value.variant);
        UA_VariableTypeNode_delete(node);
    } else 
        UA_free(value);
    retval = res.statusCode;
    if (createdNodeId != UA_NULL)
      UA_NodeId_copy(&res.addedNodeId, createdNodeId);
    UA_AddNodesResult_deleteMembers(&res);
    return retval ;
}

UA_StatusCode
UA_Server_addDataTypeNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                          const UA_LocalizedText displayName, const UA_LocalizedText description,
                          const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                          const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                          const UA_ExpandedNodeId typeDefinition, const UA_Boolean isAbstract,
                          UA_NodeId *createdNodeId) {
  UA_DataTypeNode *node = UA_DataTypeNode_new();
  UA_StatusCode retval;
  
  UA_NodeId_copy(&nodeId, &node->nodeId);
  UA_QualifiedName_copy(&browseName, &node->browseName);
  UA_LocalizedText_copy(&displayName, &node->displayName);
  UA_LocalizedText_copy(&description, &node->description);
  node->writeMask = writeMask;
  node->userWriteMask = userWriteMask;
  UA_ExpandedNodeId parentId; // we need an expandednodeid
  UA_ExpandedNodeId_init(&parentId);
  UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
  node->isAbstract = isAbstract;
  UA_AddNodesResult res = UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node,
                                                       parentId, referenceTypeId);
  if(res.statusCode != UA_STATUSCODE_GOOD)
    UA_DataTypeNode_delete(node);
  retval = res.statusCode;
  if (createdNodeId != UA_NULL)
    UA_NodeId_copy(&res.addedNodeId, createdNodeId);
  UA_AddNodesResult_deleteMembers(&res);
  
  if(!UA_NodeId_isNull(&typeDefinition.nodeId)) {
    UA_Server_addReference(server, res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION), typeDefinition);
  }
  
  return retval;
}

UA_StatusCode
UA_Server_addViewNode(UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                      const UA_LocalizedText displayName, const UA_LocalizedText description,
                      const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                      const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                      const UA_ExpandedNodeId typeDefinition, UA_NodeId *createdNodeId) {
  UA_ViewNode *node = UA_ViewNode_new();
  UA_StatusCode retval;
  
  UA_NodeId_copy(&nodeId, &node->nodeId);
  UA_QualifiedName_copy(&browseName, &node->browseName);
  UA_LocalizedText_copy(&displayName, &node->displayName);
  UA_LocalizedText_copy(&description, &node->description);
  node->writeMask = writeMask;
  node->userWriteMask = userWriteMask;
  UA_ExpandedNodeId parentId; // we need an expandednodeid
  UA_ExpandedNodeId_init(&parentId);
  UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
  node->containsNoLoops = UA_TRUE;
  node->eventNotifier = 0;
  UA_AddNodesResult res = UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node,
                                                       parentId, referenceTypeId);
  if(res.statusCode != UA_STATUSCODE_GOOD)
    UA_ViewNode_delete(node);
  retval = res.statusCode;
  if (createdNodeId != UA_NULL)
    UA_NodeId_copy(&res.addedNodeId, createdNodeId);
  UA_AddNodesResult_deleteMembers(&res);
  
  if(!UA_NodeId_isNull(&typeDefinition.nodeId)) {
    UA_Server_addReference(server, res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION), typeDefinition);
  }
  
  return retval;
}

UA_StatusCode UA_Server_addReferenceTypeNode (UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                                              const UA_LocalizedText displayName, const UA_LocalizedText description,
                                              const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                                              const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                              const UA_ExpandedNodeId typeDefinition, const UA_LocalizedText inverseName,
                                              UA_NodeId *createdNodeId) {
  UA_ReferenceTypeNode *node = UA_ReferenceTypeNode_new();
  UA_StatusCode retval;
  
  UA_NodeId_copy(&nodeId, &node->nodeId);
  UA_QualifiedName_copy(&browseName, &node->browseName);
  UA_LocalizedText_copy(&displayName, &node->displayName);
  UA_LocalizedText_copy(&description, &node->description);
  UA_LocalizedText_copy(&inverseName, &node->inverseName);
  node->writeMask = writeMask;
  node->userWriteMask = userWriteMask;
  UA_ExpandedNodeId parentId; // we need an expandednodeid
  UA_ExpandedNodeId_init(&parentId);
  UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
  UA_AddNodesResult res = UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node,
                                                       parentId, referenceTypeId);
  if(res.statusCode != UA_STATUSCODE_GOOD)
    UA_ReferenceTypeNode_delete(node);
  retval = res.statusCode;
  if (createdNodeId != UA_NULL)
    UA_NodeId_copy(&res.addedNodeId, createdNodeId);
  UA_AddNodesResult_deleteMembers(&res);
  
  if(!UA_NodeId_isNull(&typeDefinition.nodeId)) {
    UA_Server_addReference(server, res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION), typeDefinition);
  }
  
  return retval;
}

UA_StatusCode UA_Server_addObjectTypeNode (UA_Server *server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                                           const UA_LocalizedText displayName, const UA_LocalizedText description,
                                           const UA_UInt32 userWriteMask, const UA_UInt32 writeMask,
                                           const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                           const UA_ExpandedNodeId typeDefinition, const UA_Boolean isAbstract,
                                           UA_NodeId *createdNodeId) {
  UA_ObjectTypeNode *node = UA_ObjectTypeNode_new();
  UA_StatusCode retval;
  
  UA_NodeId_copy(&nodeId, &node->nodeId);
  UA_QualifiedName_copy(&browseName, &node->browseName);
  UA_LocalizedText_copy(&displayName, &node->displayName);
  UA_LocalizedText_copy(&description, &node->description);
  node->writeMask = writeMask;
  node->userWriteMask = userWriteMask;
  UA_ExpandedNodeId parentId; // we need an expandednodeid
  UA_ExpandedNodeId_init(&parentId);
  UA_NodeId_copy(&parentNodeId, &parentId.nodeId);
  node->isAbstract = isAbstract;
  UA_AddNodesResult res = UA_Server_addNodeWithSession(server, &adminSession, (UA_Node*)node,
                                                       parentId, referenceTypeId);
  if(res.statusCode != UA_STATUSCODE_GOOD)
    UA_ObjectTypeNode_delete(node);
  retval = res.statusCode;
  if (createdNodeId != UA_NULL)
    UA_NodeId_copy(&res.addedNodeId, createdNodeId);
  UA_AddNodesResult_deleteMembers(&res);
  
  if(!UA_NodeId_isNull(&typeDefinition.nodeId)) {
    UA_Server_addReference(server, res.addedNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION), typeDefinition);
  }
  
  return retval;
}

/* Userspace Version of addOneWayReferenceWithSession*/
UA_StatusCode
UA_Server_addMonodirectionalReference(UA_Server *server, UA_NodeId sourceNodeId, UA_ExpandedNodeId targetNodeId,
                                      UA_NodeId referenceTypeId, UA_Boolean isforward) {
    UA_AddReferencesItem ref;
    UA_AddReferencesItem_init(&ref);
    UA_StatusCode retval = UA_NodeId_copy(&sourceNodeId, &ref.sourceNodeId);
    retval |= UA_ExpandedNodeId_copy(&targetNodeId, &ref.targetNodeId);
    retval |= UA_NodeId_copy(&referenceTypeId, &ref.referenceTypeId);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    
    const UA_Node *target = UA_NodeStore_get(server->nodestore, &ref.targetNodeId.nodeId);
    if(!target) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto cleanup;
    }

    if(isforward == UA_TRUE)
        ref.isForward = UA_TRUE;
    ref.targetNodeClass = target->nodeClass;
    UA_NodeStore_release(target);
    retval = addOneWayReferenceWithSession(server, (UA_Session *) UA_NULL, &ref);

 cleanup:
    UA_AddReferencesItem_deleteMembers(&ref);
    return retval;
}
    

/* Adds a one-way reference to the local nodestore */
UA_StatusCode
addOneWayReferenceWithSession(UA_Server *server, UA_Session *session, const UA_AddReferencesItem *item) {
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &item->sourceNodeId);
    if(!node)
        return UA_STATUSCODE_BADINTERNALERROR;
	UA_StatusCode retval = UA_STATUSCODE_GOOD;
#ifndef UA_MULTITHREADING
	size_t i = node->referencesSize;
	if(node->referencesSize < 0)
		i = 0;
    size_t refssize = (i+1) | 3; // so the realloc is not necessary every time
	UA_ReferenceNode *new_refs = UA_realloc(node->references, sizeof(UA_ReferenceNode) * refssize);
	if(!new_refs)
		retval = UA_STATUSCODE_BADOUTOFMEMORY;
	else {
		UA_ReferenceNode_init(&new_refs[i]);
		retval = UA_NodeId_copy(&item->referenceTypeId, &new_refs[i].referenceTypeId);
		new_refs[i].isInverse = !item->isForward;
		retval |= UA_ExpandedNodeId_copy(&item->targetNodeId, &new_refs[i].targetId);
		/* hack. be careful! possible only in the single-threaded case. */
		UA_Node *mutable_node = (UA_Node*)(uintptr_t)node;
		mutable_node->references = new_refs;
		if(retval != UA_STATUSCODE_GOOD) {
			UA_NodeId_deleteMembers(&new_refs[node->referencesSize].referenceTypeId);
			UA_ExpandedNodeId_deleteMembers(&new_refs[node->referencesSize].targetId);
		} else
			mutable_node->referencesSize = i+1;
	}
	UA_NodeStore_release(node);
	return retval;
#else
    UA_Node *newNode = UA_NULL;
    void (*deleteNode)(UA_Node*) = UA_NULL;
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        newNode = (UA_Node*)UA_ObjectNode_new();
        UA_ObjectNode_copy((const UA_ObjectNode*)node, (UA_ObjectNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_ObjectNode_delete;
        break;
    case UA_NODECLASS_VARIABLE:
        newNode = (UA_Node*)UA_VariableNode_new();
        UA_VariableNode_copy((const UA_VariableNode*)node, (UA_VariableNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_VariableNode_delete;
        break;
    case UA_NODECLASS_METHOD:
        newNode = (UA_Node*)UA_MethodNode_new();
        UA_MethodNode_copy((const UA_MethodNode*)node, (UA_MethodNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_MethodNode_delete;
        break;
    case UA_NODECLASS_OBJECTTYPE:
        newNode = (UA_Node*)UA_ObjectTypeNode_new();
        UA_ObjectTypeNode_copy((const UA_ObjectTypeNode*)node, (UA_ObjectTypeNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_ObjectTypeNode_delete;
        break;
    case UA_NODECLASS_VARIABLETYPE:
        newNode = (UA_Node*)UA_VariableTypeNode_new();
        UA_VariableTypeNode_copy((const UA_VariableTypeNode*)node, (UA_VariableTypeNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_VariableTypeNode_delete;
        break;
    case UA_NODECLASS_REFERENCETYPE:
        newNode = (UA_Node*)UA_ReferenceTypeNode_new();
        UA_ReferenceTypeNode_copy((const UA_ReferenceTypeNode*)node, (UA_ReferenceTypeNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_ReferenceTypeNode_delete;
        break;
    case UA_NODECLASS_DATATYPE:
        newNode = (UA_Node*)UA_DataTypeNode_new();
        UA_DataTypeNode_copy((const UA_DataTypeNode*)node, (UA_DataTypeNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_DataTypeNode_delete;
        break;
    case UA_NODECLASS_VIEW:
        newNode = (UA_Node*)UA_ViewNode_new();
        UA_ViewNode_copy((const UA_ViewNode*)node, (UA_ViewNode*)newNode);
        deleteNode = (void (*)(UA_Node*))UA_ViewNode_delete;
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Int32 count = node->referencesSize;
    if(count < 0)
        count = 0;
    UA_ReferenceNode *old_refs = newNode->references;
    UA_ReferenceNode *new_refs = UA_malloc(sizeof(UA_ReferenceNode)*(count+1));
    if(!new_refs) {
        deleteNode(newNode);
        UA_NodeStore_release(node);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    // insert the new reference
    UA_memcpy(new_refs, old_refs, sizeof(UA_ReferenceNode)*count);
    UA_ReferenceNode_init(&new_refs[count]);
    retval = UA_NodeId_copy(&item->referenceTypeId, &new_refs[count].referenceTypeId);
    new_refs[count].isInverse = !item->isForward;
    retval |= UA_ExpandedNodeId_copy(&item->targetNodeId, &new_refs[count].targetId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(new_refs, &UA_TYPES[UA_TYPES_REFERENCENODE], ++count);
        newNode->references = UA_NULL;
        newNode->referencesSize = 0;
        deleteNode(newNode);
        UA_NodeStore_release(node);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_free(old_refs);
    newNode->references = new_refs;
    newNode->referencesSize = ++count;
    retval = UA_NodeStore_replace(server->nodestore, node, newNode, UA_NULL);
	UA_NodeStore_release(node);
	if(retval == UA_STATUSCODE_BADINTERNALERROR) {
		/* presumably because the node was replaced and an old version was updated at the same time.
           just try again */
		deleteNode(newNode);
		return addOneWayReferenceWithSession(server, session, item);
	}
	return retval;
#endif
}

UA_StatusCode deleteOneWayReferenceWithSession(UA_Server *server, UA_Session *session, const UA_DeleteReferencesItem *item) {
    const UA_Node *orig;
 repeat_deleteref_oneway:
    orig = UA_NodeStore_get(server->nodestore, &item->sourceNodeId);
    if(!orig)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

#ifndef UA_MULTITHREADING
    /* We cheat if multithreading is not enabled and treat the node as mutable. */
    UA_Node *editable = (UA_Node*)(uintptr_t)orig;
#else
    UA_Node *editable = UA_Node_copyAnyNodeClass(orig);
    UA_Boolean edited = UA_FALSE;;
#endif

    for(UA_Int32 i = editable->referencesSize - 1; i >= 0; i--) {
        if(!UA_NodeId_equal(&item->targetNodeId.nodeId, &editable->references[i].targetId.nodeId))
            continue;
        if(!UA_NodeId_equal(&item->referenceTypeId, &editable->references[i].referenceTypeId))
            continue;
        if(item->isForward == editable->references[i].isInverse)
            continue;
        /* move the last entry to override the current position */
        UA_ReferenceNode_deleteMembers(&editable->references[i]);
        editable->references[i] = editable->references[editable->referencesSize-1];
        editable->referencesSize--;

#ifdef UA_MULTITHREADING
        edited = UA_TRUE;
#endif
    }

    /* we removed the last reference */
    if(editable->referencesSize <= 0 && editable->references)
        UA_free(editable->references);
    
#ifdef UA_MULTITHREADING
    if(!edited) {
        UA_Node_deleteAnyNodeClass(editable);
    } else if(UA_NodeStore_replace(server->nodestore, orig, editable, UA_NULL) != UA_STATUSCODE_GOOD) {
        /* the node was changed by another thread. repeat. */
        UA_Node_deleteAnyNodeClass(editable);
        UA_NodeStore_release(orig);
        goto repeat_deleteref_oneway;
    }
#endif

    UA_NodeStore_release(orig);
    return UA_STATUSCODE_GOOD;;
}

/* userland version of addReferenceWithSession */
UA_StatusCode UA_Server_addReference(UA_Server *server, const UA_NodeId sourceId, const UA_NodeId refTypeId,
                                     const UA_ExpandedNodeId targetId) {
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = sourceId;
    item.referenceTypeId = refTypeId;
    item.isForward = UA_TRUE;
    item.targetNodeId = targetId;
    return UA_Server_addReferenceWithSession(server, &adminSession, &item);
}

UA_StatusCode UA_Server_addReferenceWithSession(UA_Server *server, UA_Session *session,
                                                const UA_AddReferencesItem *item) {
    if(item->targetServerUri.length > 0)
        return UA_STATUSCODE_BADNOTIMPLEMENTED; // currently no expandednodeids are allowed

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

#ifdef UA_EXTERNAL_NAMESPACES
    UA_ExternalNodeStore *ensFirst = UA_NULL;
    UA_ExternalNodeStore *ensSecond = UA_NULL;
    for(size_t j = 0;j<server->externalNamespacesSize && (!ensFirst || !ensSecond);j++) {
        if(item->sourceNodeId.namespaceIndex == server->externalNamespaces[j].index)
            ensFirst = &server->externalNamespaces[j].externalNodeStore;
        if(item->targetNodeId.nodeId.namespaceIndex == server->externalNamespaces[j].index)
            ensSecond = &server->externalNamespaces[j].externalNodeStore;
    }

    if(ensFirst) {
        // todo: use external nodestore
    } else
#endif
        retval = addOneWayReferenceWithSession(server, session, item);

    if(retval)
        return retval;

    UA_AddReferencesItem secondItem;
    secondItem = *item;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.isForward = !item->isForward;
#ifdef UA_EXTERNAL_NAMESPACES
    if(ensSecond) {
        // todo: use external nodestore
    } else
#endif
        retval = addOneWayReferenceWithSession (server, session, &secondItem);

    // todo: remove reference if the second direction failed
    return retval;
} 

UA_AddNodesResult UA_Server_addNode(UA_Server *server, UA_Node *node, const UA_ExpandedNodeId parentNodeId,
                                    const UA_NodeId referenceTypeId) {
    return UA_Server_addNodeWithSession(server, &adminSession, node, parentNodeId, referenceTypeId);
}

UA_AddNodesResult UA_Server_addNodeWithSession(UA_Server *server, UA_Session *session, UA_Node *node,
                                               const UA_ExpandedNodeId parentNodeId, const UA_NodeId referenceTypeId) {
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);

    if(node->nodeId.namespaceIndex >= server->namespacesSize) {
        result.statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return result;
    }

    const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId.nodeId);
    if(!parent) {
        result.statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID;
        return result;
    }

    const UA_ReferenceTypeNode *referenceType =
        (const UA_ReferenceTypeNode *)UA_NodeStore_get(server->nodestore, &referenceTypeId);
    if(!referenceType) {
        result.statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        goto ret;
    }

    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        result.statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        goto ret2;
    }

    if(referenceType->isAbstract == UA_TRUE) {
        result.statusCode = UA_STATUSCODE_BADREFERENCENOTALLOWED;
        goto ret2;
    }

    // todo: test if the referencetype is hierarchical
    //FIXME: a bit dirty workaround of preserving namespace
    //namespace index is assumed to be valid
    const UA_Node *managed = UA_NULL;
    UA_NodeId tempNodeid;
    UA_NodeId_init(&tempNodeid);
    UA_NodeId_copy(&node->nodeId, &tempNodeid);
    tempNodeid.namespaceIndex = 0;
    if(UA_NodeId_isNull(&tempNodeid)) {
        if(UA_NodeStore_insert(server->nodestore, node, &managed) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }
        result.addedNodeId = managed->nodeId; // cannot fail as unique nodeids are numeric
    } else {
        if(UA_NodeId_copy(&node->nodeId, &result.addedNodeId) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }

        if(UA_NodeStore_insert(server->nodestore, node, &managed) != UA_STATUSCODE_GOOD) {
            result.statusCode = UA_STATUSCODE_BADNODEIDEXISTS;  // todo: differentiate out of memory
            UA_NodeId_deleteMembers(&result.addedNodeId);
            goto ret2;
        }
    }
    
    // reference back to the parent
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = managed->nodeId;
    item.referenceTypeId = referenceType->nodeId;
    item.isForward = UA_FALSE;
    item.targetNodeId.nodeId = parent->nodeId;
    UA_Server_addReferenceWithSession(server, session, &item);

    // todo: error handling. remove new node from nodestore
    UA_NodeStore_release(managed);
    
 ret2:
    UA_NodeId_deleteMembers(&tempNodeid);
    UA_NodeStore_release((const UA_Node*)referenceType);
 ret:
    UA_NodeStore_release(parent);
    return result;
}

#ifdef ENABLE_METHODCALLS
UA_StatusCode
UA_Server_addMethodNode(UA_Server* server, const UA_NodeId nodeId, const UA_QualifiedName browseName,
                        UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                        const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                        UA_MethodCallback method, void *handle, UA_Int32 inputArgumentsSize, const UA_Argument* inputArguments, 
                        UA_Int32 outputArgumentsSize, const UA_Argument* outputArguments, 
                        UA_NodeId* createdNodeId) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_MethodNode *newMethod = UA_MethodNode_new();
    UA_NodeId_copy(&nodeId, &newMethod->nodeId);
    UA_QualifiedName_copy(&browseName, &newMethod->browseName);
    UA_LocalizedText_copy(&displayName, &newMethod->displayName);
    UA_LocalizedText_copy(&description, &newMethod->description);
    newMethod->writeMask = writeMask;
    newMethod->userWriteMask = userWriteMask;
    newMethod->attachedMethod = method;
    newMethod->methodHandle   = handle;
    newMethod->executable = UA_TRUE;
    newMethod->userExecutable = UA_TRUE;
    
    UA_ExpandedNodeId parentExpandedNodeId;
    UA_ExpandedNodeId_init(&parentExpandedNodeId);
    UA_NodeId_copy(&parentNodeId, &parentExpandedNodeId.nodeId);
    UA_AddNodesResult addRes = UA_Server_addNode(server, (UA_Node*)newMethod, parentExpandedNodeId, referenceTypeId);
    retval |= addRes.statusCode;
    if(retval!= UA_STATUSCODE_GOOD) {
        UA_MethodNode_delete(newMethod);
        return retval;
    }
    
    UA_ExpandedNodeId methodExpandedNodeId;
    UA_ExpandedNodeId_init(&methodExpandedNodeId);
    UA_NodeId_copy(&addRes.addedNodeId, &methodExpandedNodeId.nodeId);
    
    if (createdNodeId != UA_NULL)
      UA_NodeId_copy(&addRes.addedNodeId, createdNodeId);
    UA_AddNodesResult_deleteMembers(&addRes);
    
    /* Only proceed with creating in/outputArguments if the method and both arguments are not
     * UA_NULL; otherwise this is a pretty strong indicator that this node was generated,
     * in which case these arguments will be created later and individually.
     */
    if (method == UA_NULL && inputArguments == UA_NULL && outputArguments == UA_NULL &&
        inputArgumentsSize == 0 && outputArgumentsSize == 0)
      return retval;
    
    /* create InputArguments */
    UA_NodeId argId = UA_NODEID_NUMERIC(nodeId.namespaceIndex, 0); 
    UA_VariableNode *inputArgumentsVariableNode  = UA_VariableNode_new();
    retval |= UA_NodeId_copy(&argId, &inputArgumentsVariableNode->nodeId);
    inputArgumentsVariableNode->browseName = UA_QUALIFIEDNAME_ALLOC(0,"InputArguments");
    inputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", "InputArguments");
    inputArgumentsVariableNode->description = UA_LOCALIZEDTEXT_ALLOC("en_US", "InputArguments");
    inputArgumentsVariableNode->valueRank = 1;
    UA_Variant_setArrayCopy(&inputArgumentsVariableNode->value.variant, inputArguments,
                            inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
    addRes = UA_Server_addNode(server, (UA_Node*)inputArgumentsVariableNode, methodExpandedNodeId,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY));
    if(addRes.statusCode != UA_STATUSCODE_GOOD) {
        UA_ExpandedNodeId_deleteMembers(&methodExpandedNodeId);
        if(createdNodeId != UA_NULL)
            UA_NodeId_deleteMembers(createdNodeId);    	
        // TODO Remove node
        return addRes.statusCode;
    }
    UA_AddNodesResult_deleteMembers(&addRes);

    /* create OutputArguments */
    argId = UA_NODEID_NUMERIC(nodeId.namespaceIndex, 0);
    UA_VariableNode *outputArgumentsVariableNode  = UA_VariableNode_new();
    retval |= UA_NodeId_copy(&argId, &outputArgumentsVariableNode->nodeId);
    outputArgumentsVariableNode->browseName  = UA_QUALIFIEDNAME_ALLOC(0,"OutputArguments");
    outputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OutputArguments");
    outputArgumentsVariableNode->description = UA_LOCALIZEDTEXT_ALLOC("en_US", "OutputArguments");
    outputArgumentsVariableNode->valueRank = 1;
    UA_Variant_setArrayCopy(&outputArgumentsVariableNode->value.variant, outputArguments,
                            outputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
    addRes = UA_Server_addNode(server, (UA_Node*)outputArgumentsVariableNode, methodExpandedNodeId,
                               UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY));
    UA_ExpandedNodeId_deleteMembers(&methodExpandedNodeId);
    if(addRes.statusCode != UA_STATUSCODE_GOOD) {
        if(createdNodeId != UA_NULL)
            UA_NodeId_deleteMembers(createdNodeId);    	
        // TODO Remove node
        retval = addRes.statusCode;
    }
    UA_AddNodesResult_deleteMembers(&addRes);
    return retval;
}
#endif
    
UA_StatusCode UA_Server_setNodeAttribute(UA_Server *server, const UA_NodeId nodeId,
                                         const UA_AttributeId attributeId, const UA_Variant value) {
    UA_WriteValue wvalue;
    UA_WriteValue_init(&wvalue);
    wvalue.nodeId = nodeId;
    wvalue.attributeId = attributeId;
    wvalue.value.value = value;
    wvalue.value.hasValue = UA_TRUE;
    return Service_Write_single(server, &adminSession, &wvalue);
}

#ifdef ENABLE_METHODCALLS
/* Allow userspace to attach a method to one defined via XML or to switch an attached method for another */
UA_StatusCode
UA_Server_setNodeAttribute_method(UA_Server *server, UA_NodeId methodNodeId, UA_MethodCallback method, void *handle) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  const UA_Node *attachToMethod = UA_NULL;
  UA_MethodNode *replacementMethod = UA_NULL;
  
  if (!method)
    return UA_STATUSCODE_BADMETHODINVALID;

  if (!server)
    return UA_STATUSCODE_BADSERVERINDEXINVALID;
  
  attachToMethod =  UA_NodeStore_get(server->nodestore, &methodNodeId);
  if (!attachToMethod)
    return UA_STATUSCODE_BADNODEIDINVALID;
  
  if (attachToMethod->nodeClass != UA_NODECLASS_METHOD){
    UA_NodeStore_release(attachToMethod);
    return UA_STATUSCODE_BADNODEIDINVALID;
  }
  
  replacementMethod = UA_MethodNode_new();

  UA_MethodNode_copy((const UA_MethodNode *) attachToMethod, replacementMethod);
  
  replacementMethod->attachedMethod = method;
  replacementMethod->methodHandle   = handle;
  UA_NodeStore_replace(server->nodestore, attachToMethod, (UA_Node *) replacementMethod, UA_NULL);
  UA_NodeStore_release(attachToMethod);

  return retval;
}
#endif

UA_StatusCode
UA_Server_setNodeAttribute_valueDataSource(UA_Server *server, const UA_NodeId nodeId, UA_DataSource dataSource) {
    const UA_Node *orig;
 retrySetDataSource:
    orig = UA_NodeStore_get(server->nodestore, &nodeId);
    if(!orig)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    if(orig->nodeClass != UA_NODECLASS_VARIABLE &&
       orig->nodeClass != UA_NODECLASS_VARIABLETYPE) {
        UA_NodeStore_release(orig);
        return UA_STATUSCODE_BADNODECLASSINVALID;
    }
    
#ifndef UA_MULTITHREADING
    /* We cheat if multithreading is not enabled and treat the node as mutable. */
    UA_VariableNode *editable = (UA_VariableNode*)(uintptr_t)orig;
#else
    UA_VariableNode *editable = (UA_VariableNode*)UA_Node_copyAnyNodeClass(orig);
    if(!editable) {
        UA_NodeStore_release(orig);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
#endif

    if(editable->valueSource == UA_VALUESOURCE_VARIANT)
        UA_Variant_deleteMembers(&editable->value.variant);
    editable->value.dataSource = dataSource;
    editable->valueSource = UA_VALUESOURCE_DATASOURCE;
  
#ifdef UA_MULTITHREADING
    UA_StatusCode retval = UA_NodeStore_replace(server->nodestore, orig, (UA_Node*)editable, UA_NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        /* The node was replaced in the background */
        UA_NodeStore_release(orig);
        goto retrySetDataSource;
    }
#endif
    UA_NodeStore_release(orig);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getNodeAttribute(UA_Server *server, const UA_NodeId nodeId,
                           UA_AttributeId attributeId, UA_Variant *v) {
    const UA_ReadValueId rvi = {.nodeId = nodeId, .attributeId = attributeId, .indexRange = UA_STRING_NULL,
                                .dataEncoding = UA_QUALIFIEDNAME(0, "DefaultBinary")};
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rvi, &dv);
    if(dv.hasStatus && dv.status != UA_STATUSCODE_GOOD)
        return dv.status;
    *v = dv.value; // The caller needs to free the content eventually
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_getNodeAttributeUnpacked(UA_Server *server, const UA_NodeId nodeId, const UA_AttributeId attributeId, void *v) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode retval = UA_Server_getNodeAttribute(server, nodeId, attributeId, &out); 
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(attributeId == UA_ATTRIBUTEID_VALUE)
        UA_memcpy(v, &out, sizeof(UA_Variant));
    else {
        UA_memcpy(v, out.data, out.type->memSize);
        out.data = UA_NULL;
        out.arrayLength = -1;
        UA_Variant_deleteMembers(&out);
    }
    return UA_STATUSCODE_GOOD;
}

#ifdef ENABLE_METHODCALLS
UA_StatusCode
UA_Server_getNodeAttribute_method(UA_Server *server, UA_NodeId nodeId, UA_MethodCallback *method) {
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    
    if(node.anyNode->nodeClass != UA_NODECLASS_METHOD) {
        UA_NodeStore_release(node);
        return UA_STATUSCODE_BADNODECLASSINVALID;
    }

    *method = ((UA_MethodNode*)node)->attachToMethod;
    UA_NodeStore_release(node);
    return UA_STATUSCODE_GOOD;
}
#endif

UA_StatusCode
UA_Server_getNodeAttribute_valueDataSource(UA_Server *server, UA_NodeId nodeId, UA_DataSource *dataSource) {
    const UA_VariableNode *node = (const UA_VariableNode*)UA_NodeStore_get(server->nodestore, &nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    if(node->nodeClass != UA_NODECLASS_VARIABLE &&
       node->nodeClass != UA_NODECLASS_VARIABLETYPE) {
        UA_NodeStore_release((const UA_Node*)node);
        return UA_STATUSCODE_BADNODECLASSINVALID;
    }

    if(node->valueSource != UA_VALUESOURCE_DATASOURCE) {
        UA_NodeStore_release((const UA_Node*)node);
        return UA_STATUSCODE_BADNODECLASSINVALID;
    }

    *dataSource = node->value.dataSource;
    UA_NodeStore_release((const UA_Node*)node);
    return UA_STATUSCODE_GOOD;
}
