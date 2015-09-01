#include "ua_server.h"
#include "ua_server_internal.h"

#define UA_SERVER_DELETENODEALIAS(TYPE) \
UA_StatusCode UA_Server_delete##TYPE##Node(UA_Server *server, UA_NodeId nodeId) { \
  return UA_Server_deleteNode(server, nodeId); \
}

UA_StatusCode 
UA_Server_deleteNode(UA_Server *server, UA_NodeId nodeId) {
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

UA_SERVER_DELETENODEALIAS(Object)
UA_SERVER_DELETENODEALIAS(Variable)
UA_SERVER_DELETENODEALIAS(ReferenceType)
UA_SERVER_DELETENODEALIAS(View)
UA_SERVER_DELETENODEALIAS(VariableType)
UA_SERVER_DELETENODEALIAS(DataType)
#ifdef ENABLE_METHODCALLS
UA_SERVER_DELETENODEALIAS(Method)
#endif

UA_StatusCode UA_Server_getNodeCopy(UA_Server *server, UA_NodeId nodeId, void **copyInto) {
  const UA_Node *node = UA_NodeStore_get(server->nodestore, &nodeId);
  UA_Node **copy = (UA_Node **) copyInto;
  
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  
  if (!node)
    return UA_STATUSCODE_BADNODEIDINVALID;
  
  switch(node->nodeClass) {
    case UA_NODECLASS_DATATYPE:
      *copy = (UA_Node *) UA_VariableNode_new();
      retval |= UA_DataTypeNode_copy((const UA_DataTypeNode *) node, (UA_DataTypeNode *) *copy);
      break;
    case UA_NODECLASS_METHOD:
      *copy =  (UA_Node *) UA_MethodNode_new();
      retval |= UA_MethodNode_copy((const UA_MethodNode *) node, (UA_MethodNode *) *copy);
      break;      
    case UA_NODECLASS_OBJECT:
      *copy =  (UA_Node *) UA_ObjectNode_new();
      retval |= UA_ObjectNode_copy((const UA_ObjectNode *) node, (UA_ObjectNode *) *copy);
      break;      
    case UA_NODECLASS_OBJECTTYPE:
      *copy =  (UA_Node *) UA_ObjectTypeNode_new();
      retval |= UA_ObjectTypeNode_copy((const UA_ObjectTypeNode *) node, (UA_ObjectTypeNode *) *copy);
      break;      
    case UA_NODECLASS_REFERENCETYPE:
      *copy =  (UA_Node *) UA_ReferenceTypeNode_new();
      retval |= UA_ReferenceTypeNode_copy((const UA_ReferenceTypeNode *) node, (UA_ReferenceTypeNode *) *copy);
      break;      
    case UA_NODECLASS_VARIABLE:
      *copy =  (UA_Node *) UA_VariableNode_new();
      retval |= UA_VariableNode_copy((const UA_VariableNode *) node, (UA_VariableNode *) *copy);
      break;      
    case UA_NODECLASS_VARIABLETYPE:
      *copy =  (UA_Node *) UA_VariableTypeNode_new();
      retval |= UA_VariableTypeNode_copy((const UA_VariableTypeNode *) node, (UA_VariableTypeNode *) *copy);
      break;      
    case UA_NODECLASS_VIEW:
      *copy =  (UA_Node *) UA_ViewNode_new();
      retval |= UA_ViewNode_copy((const UA_ViewNode *) node, (UA_ViewNode *) *copy);
      break;      
    default:
      break;
  }
  
  UA_NodeStore_release(node);
  
  return retval;
} 

UA_StatusCode UA_Server_deleteNodeCopy(UA_Server *server, void **nodeptr) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_Node **node = (UA_Node **) nodeptr;
  
  if (!(*node))
    return UA_STATUSCODE_BADNODEIDINVALID;
  
  switch((*node)->nodeClass) {
    case UA_NODECLASS_DATATYPE:
      UA_DataTypeNode_delete((UA_DataTypeNode *) *node);
      break;
    case UA_NODECLASS_METHOD:
      UA_MethodNode_delete((UA_MethodNode *) *node);
      break;      
    case UA_NODECLASS_OBJECT:
      UA_ObjectNode_delete((UA_ObjectNode *) *node);
      break;      
    case UA_NODECLASS_OBJECTTYPE:
      UA_ObjectTypeNode_delete((UA_ObjectTypeNode *) *node);
      break;      
    case UA_NODECLASS_REFERENCETYPE:
      UA_ReferenceTypeNode_delete((UA_ReferenceTypeNode *) *node);
      break;      
    case UA_NODECLASS_VARIABLE:
      UA_VariableNode_delete((UA_VariableNode *) *node);
      break;      
    case UA_NODECLASS_VARIABLETYPE:
      UA_VariableTypeNode_delete((UA_VariableTypeNode *) *node);
      break;      
    case UA_NODECLASS_VIEW:
      UA_ViewNode_delete((UA_ViewNode *) *node);
      break;      
    default:
      break;
  }
  
  return retval;
} 

UA_StatusCode 
UA_Server_forEachChildNodeCall(UA_Server *server, UA_NodeId parentNodeId, UA_NodeIteratorCallback callback, void *handle) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId);
  if (!parent)
    return UA_STATUSCODE_BADNODEIDINVALID;
  
  for(int i=0; i<parent->referencesSize; i++) {
    UA_ReferenceNode *ref = &parent->references[i];
    retval |= callback(ref->targetId.nodeId, ref->isInverse, ref->referenceTypeId, handle);
  }
  
  UA_NodeStore_release(parent);
  return retval;
}

UA_StatusCode
UA_Server_addVariableNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                          UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                          const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
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
UA_Server_addObjectNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                        UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                        const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, 
                        UA_UInt32 writeMask, const UA_ExpandedNodeId typeDefinition, UA_NodeId *createdNodeId) {
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
UA_Server_addDataSourceVariableNode(UA_Server *server, UA_DataSource dataSource,
                                    const UA_QualifiedName browseName, UA_NodeId nodeId,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                    UA_NodeId *createdNodeId) {
    UA_VariableNode *node = UA_VariableNode_new();
    UA_StatusCode retval;
    
    node->valueSource = UA_VALUESOURCE_DATASOURCE;
    node->value.dataSource = dataSource;
    UA_NodeId_copy(&nodeId, &node->nodeId);
    UA_QualifiedName_copy(&browseName, &node->browseName);
    UA_String_copy(&browseName.name, &node->displayName.text);
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
UA_Server_addVariableTypeNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                              UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                              const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                              UA_Variant *value,  UA_Int32 valueRank, UA_Boolean isAbstract, UA_NodeId *createdNodeId) {
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
UA_Server_addDataTypeNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                          UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                          const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                          UA_ExpandedNodeId typeDefinition, UA_Boolean isAbstract, UA_NodeId *createdNodeId) {
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
UA_Server_addViewNode(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
                          UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                          const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                          UA_ExpandedNodeId typeDefinition, UA_NodeId *createdNodeId) {
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

UA_StatusCode UA_Server_addReferenceTypeNode (UA_Server *server, UA_NodeId nodeId, UA_QualifiedName browseName, 
                                              UA_LocalizedText displayName, UA_LocalizedText description, UA_NodeId parentNodeId, 
                                              UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                                              UA_ExpandedNodeId typeDefinition, UA_LocalizedText inverseName, 
                                              UA_NodeId *createdNodeId ){
  UA_ReferenceTypeNode *node = UA_ReferenceTypeNode_new();
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

UA_StatusCode UA_Server_addObjectTypeNode ( UA_Server *server, UA_NodeId nodeId, UA_QualifiedName browseName, 
                                          UA_LocalizedText displayName, UA_LocalizedText description, UA_NodeId parentNodeId, 
                                          UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                                          UA_ExpandedNodeId typeDefinition, UA_Boolean isAbstract, 
                                          UA_NodeId *createdNodeId ){
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
UA_Server_AddMonodirectionalReference(UA_Server *server, UA_NodeId sourceNodeId, UA_ExpandedNodeId targetNodeId,
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
  UA_Node *node = UA_NULL;
  UA_StatusCode retval = UA_Server_getNodeCopy(server, item->sourceNodeId, (void *) &node);
  if (!node)
    return retval;
  
  // Copy all that do not target the targetNode in a new array
  UA_ReferenceNode *newRefs = (UA_ReferenceNode*) UA_malloc(sizeof(UA_ReferenceNode) * node->referencesSize);
  UA_UInt32 newRefCount = 0;
  for(int refn = 0; refn < node->referencesSize; refn++) {
    // Completely different target node:
    if (!UA_NodeId_equal(&item->targetNodeId.nodeId, &node->references[refn].targetId.nodeId)) {
      UA_ReferenceNode_copy(&node->references[refn], &newRefs[newRefCount]);
      newRefCount++;
    }
    else {
      // Correct target node, wrong referenceType
      if (!UA_NodeId_equal(&item->referenceTypeId, &node->references[refn].referenceTypeId)) {
        UA_ReferenceNode_copy(&node->references[refn], &newRefs[newRefCount]);
        newRefCount++;
      }
      else {
        // Correct target, correct referenceType, wrong direction
        // FIXME: Check semantics of deleteBidirectional: does it mean we delete any forward or inverse refs 
        //        in this node (assumed here) OR does it mean we remove Invers refs from target node??
        if (item->deleteBidirectional == UA_FALSE && (item->isForward != node->references[refn].isInverse)) {
          UA_ReferenceNode_copy(&node->references[refn], &newRefs[newRefCount]);
          newRefCount++;
        }
        // else... everything matches, don't copy this node into the new reference array. 
      }
    }
  }
  
  // Reallocate
  UA_ReferenceNode *tmp = UA_realloc(newRefs, sizeof(UA_ReferenceNode) * newRefCount);
  if (!tmp) {
    if (newRefCount > 0) 
      UA_free(newRefs); //realloc with zero size is equal to free!
    UA_Server_deleteNodeCopy(server, (void *) &node);
    return UA_STATUSCODE_BADOUTOFMEMORY;
  }
  newRefs = tmp;
  
  // Swap old references in node for new ones, then free old array
  tmp = node->references;
  node->referencesSize = newRefCount;
  node->references = newRefs;
  if (tmp)
    UA_free(tmp);
  
  const UA_Node *inserted;
  retval |= UA_NodeStore_replace(server->nodestore, (const UA_Node *) node, node, &inserted);
  return retval;
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

/* userland version of addNodeWithSession */
UA_AddNodesResult
UA_Server_addNode(UA_Server *server, UA_Node *node, const UA_ExpandedNodeId parentNodeId,
                  const UA_NodeId referenceTypeId) {
    return UA_Server_addNodeWithSession(server, &adminSession, node, parentNodeId, referenceTypeId);
}

UA_AddNodesResult
UA_Server_addNodeWithSession(UA_Server *server, UA_Session *session, UA_Node *node,
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
UA_Server_addMethodNode(UA_Server* server, UA_NodeId nodeId, const UA_QualifiedName browseName, 
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
    if (method == UA_NULL && inputArguments == UA_NULL && outputArguments == UA_NULL && inputArgumentsSize == 0 && outputArgumentsSize == 0)
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
    if(addRes.statusCode != UA_STATUSCODE_GOOD)
    {
        if(createdNodeId != UA_NULL)
            UA_NodeId_deleteMembers(createdNodeId);    	
        // TODO Remove node
        retval = addRes.statusCode;
    }
    UA_AddNodesResult_deleteMembers(&addRes);
    return retval;
}

#endif

#define SETATTRIBUTE_ASSERTNODECLASS(CLASS) {                   \
if ((anyTypeNode.node->nodeClass & ( CLASS )) == 0) {                     \
    UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);          \
    return UA_STATUSCODE_BADNODECLASSINVALID;                   \
  }                                                             \
}
  
#define SETATTRIBUTE_ASSERTTYPECORRECT(TYPE) { \
  UA_NodeId expectedType = UA_NODEID_NUMERIC(0, UA_NS0ID_##TYPE); \
  if (!UA_NodeId_equal(&dataType->typeId, &expectedType)) {       \
    UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);            \
    return UA_STATUSCODE_BADTYPEMISMATCH;                         \
  }                                                               \
}\
    
UA_StatusCode UA_Server_setAttributeValue(UA_Server *server, UA_NodeId nodeId, UA_AttributeId attributeId, void *value) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  
  if (!value)
    return UA_STATUSCODE_BADNOTHINGTODO;
  
  union {
    UA_Node *node;
    UA_ObjectNode *oObj;
    UA_ObjectTypeNode *otObj;
    UA_VariableNode *vObj;
    UA_VariableTypeNode *vtObj;
    UA_ReferenceTypeNode *rtObj;
    UA_MethodNode *mObj;
    UA_DataTypeNode *dtObj;
    UA_ViewNode *vwObj;
  } anyTypeNode;
  retval = UA_Server_getNodeCopy(server, nodeId, (void **) &anyTypeNode.node);
  if (retval)
    return retval;
  
  UA_UInt32  *nInt;
  UA_Boolean *nBool;
  UA_Byte    *nByte;
  UA_Variant *nVariant;
  UA_Double  *nDouble;
  switch(attributeId) {
    case UA_ATTRIBUTEID_NODEID:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_NODECLASS:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_BROWSENAME:
      UA_QualifiedName_deleteMembers(&anyTypeNode.node->browseName);
      UA_QualifiedName_copy((UA_QualifiedName *) value, &anyTypeNode.node->browseName);
      break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
      UA_LocalizedText_deleteMembers(&anyTypeNode.node->displayName);
      UA_LocalizedText_copy((UA_LocalizedText *) value, &anyTypeNode.node->displayName);
      break;
    case UA_ATTRIBUTEID_DESCRIPTION:
      UA_LocalizedText_deleteMembers(&anyTypeNode.node->description);
      UA_LocalizedText_copy((UA_LocalizedText *) value, &anyTypeNode.node->description);
      break;
    case UA_ATTRIBUTEID_WRITEMASK:
      nInt = (UA_UInt32*) value;
      anyTypeNode.node->writeMask = *nInt;
      break;
    case UA_ATTRIBUTEID_USERWRITEMASK:
      nInt = (UA_UInt32*) value;
      anyTypeNode.node->userWriteMask = *nInt;
      break;    
    case UA_ATTRIBUTEID_ISABSTRACT:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_VARIABLETYPE | UA_NODECLASS_DATATYPE)
      nBool = (UA_Boolean *) value;
      switch(anyTypeNode.node->nodeClass) {
        case UA_NODECLASS_OBJECTTYPE:
          anyTypeNode.otObj->isAbstract = *nBool;
          break;
        case UA_NODECLASS_REFERENCETYPE:
          anyTypeNode.rtObj->isAbstract = *nBool;
          break;
        case UA_NODECLASS_VARIABLETYPE:
          anyTypeNode.vtObj->isAbstract = *nBool;
          break;
        case UA_NODECLASS_DATATYPE:
          anyTypeNode.dtObj->isAbstract = *nBool;
          break;
        default:
          UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
          return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
          break;
      }
      break;
    case UA_ATTRIBUTEID_SYMMETRIC:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_REFERENCETYPE)
      nBool = (UA_Boolean *) value;
      anyTypeNode.rtObj->symmetric = *nBool;
      break;
    case UA_ATTRIBUTEID_INVERSENAME:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_REFERENCETYPE)
      UA_LocalizedText_deleteMembers(&anyTypeNode.rtObj->inverseName);
      UA_LocalizedText_copy((UA_LocalizedText *) value, &anyTypeNode.rtObj->inverseName);
      break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VIEW)
      nBool = (UA_Boolean *) value;
      anyTypeNode.vwObj->containsNoLoops = *nBool;
      break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT)
      nByte = (UA_Byte *) value;
      switch(anyTypeNode.node->nodeClass) {
        case UA_NODECLASS_VIEW:
          anyTypeNode.vwObj->eventNotifier = *nByte;
          break;
        case UA_NODECLASS_OBJECT:
          anyTypeNode.oObj->eventNotifier = *nByte;
          break;
        default:
          UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
          return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
          break;
      }
      break;
    case UA_ATTRIBUTEID_VALUE:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE)
      if(anyTypeNode.vObj->valueSource == UA_VALUESOURCE_DATASOURCE) {
        UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
        return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      }
      nVariant = value;
      UA_Variant_deleteMembers(&anyTypeNode.vObj->value.variant);
      UA_Variant_copy(nVariant, &anyTypeNode.vObj->value.variant);
      break;
    case UA_ATTRIBUTEID_DATATYPE:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_VALUERANK:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE)
      nByte = (UA_Byte*) value;
      anyTypeNode.vObj->accessLevel = *nByte;
      break;
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE)
      nByte = (UA_Byte*) value;
      anyTypeNode.vObj->userAccessLevel = *nByte;
      break;
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE)
      nDouble = (UA_Double *) value;
      anyTypeNode.vObj->minimumSamplingInterval = *nDouble;
      break;
    case UA_ATTRIBUTEID_HISTORIZING:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE)
      nBool = (UA_Boolean *) value;
      anyTypeNode.vObj->historizing= *nBool;
      break;
    case UA_ATTRIBUTEID_EXECUTABLE:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_METHOD)
      nBool = (UA_Boolean *) value;
      anyTypeNode.mObj->executable= *nBool;
      break;
    case UA_ATTRIBUTEID_USEREXECUTABLE:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_METHOD)
      nBool = (UA_Boolean *) value;
      anyTypeNode.mObj->userExecutable= *nBool;
      break;
    default:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
  }
  
  const UA_Node *oldNode = UA_NodeStore_get(server->nodestore, &nodeId);
  const UA_Node **inserted = UA_NULL;
  retval |= UA_NodeStore_replace(server->nodestore, oldNode, anyTypeNode.node, inserted);
  UA_NodeStore_release(oldNode);
  
  // Node Copy deleted by nodeEntryFromNode() during nodestore_replace()!
  //UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
  return retval;
}

#define SERVER_GETATTRIBUTE_COPYTYPEVALUE(TYPE, SRC) \
  *value = (void *) UA_##TYPE##_new();               \
  UA_##TYPE##_copy( SRC, (UA_##TYPE *) *value  );    \
  
UA_StatusCode UA_Server_getAttributeValue(UA_Server *server, UA_NodeId nodeId, UA_AttributeId attributeId, void **value) {
  union {
    UA_Node *node;
    UA_ObjectNode *oObj;
    UA_ObjectTypeNode *otObj;
    UA_VariableNode *vObj;
    UA_VariableTypeNode *vtObj;
    UA_ReferenceTypeNode *rtObj;
    UA_MethodNode *mObj;
    UA_DataTypeNode *dtObj;
    UA_ViewNode *vwObj;
  } anyTypeNode;
  
  UA_StatusCode retval = UA_Server_getNodeCopy(server, nodeId, (void **) &anyTypeNode.node);
  if (retval != UA_STATUSCODE_GOOD)
    return retval;
  
  switch(attributeId) {
    case UA_ATTRIBUTEID_NODEID:
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(NodeId, &anyTypeNode.node->nodeId);
      break;
    case UA_ATTRIBUTEID_NODECLASS:
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(NodeClass, &anyTypeNode.node->nodeClass);
      break;
    case UA_ATTRIBUTEID_BROWSENAME:
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(QualifiedName, &anyTypeNode.node->browseName);
      break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(LocalizedText, &anyTypeNode.node->displayName);
      break;
    case UA_ATTRIBUTEID_DESCRIPTION:
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(LocalizedText, &anyTypeNode.node->description);
      break;
    case UA_ATTRIBUTEID_WRITEMASK:
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(UInt32, &anyTypeNode.node->writeMask);
      break;
    case UA_ATTRIBUTEID_USERWRITEMASK:
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(UInt32, &anyTypeNode.node->userWriteMask);
      break;    
    case UA_ATTRIBUTEID_ISABSTRACT:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_OBJECTTYPE | UA_NODECLASS_REFERENCETYPE | UA_NODECLASS_VARIABLETYPE | UA_NODECLASS_DATATYPE)
      switch(anyTypeNode.node->nodeClass) {
        case UA_NODECLASS_OBJECTTYPE:
          SERVER_GETATTRIBUTE_COPYTYPEVALUE(Boolean, &anyTypeNode.otObj->isAbstract);
          break;
        case UA_NODECLASS_REFERENCETYPE:
          SERVER_GETATTRIBUTE_COPYTYPEVALUE(Boolean, &anyTypeNode.rtObj->isAbstract);
          break;
        case UA_NODECLASS_VARIABLETYPE:
          SERVER_GETATTRIBUTE_COPYTYPEVALUE(Boolean, &anyTypeNode.vtObj->isAbstract);
          break;
        case UA_NODECLASS_DATATYPE:
          SERVER_GETATTRIBUTE_COPYTYPEVALUE(Boolean, &anyTypeNode.dtObj->isAbstract);
          break;
        default:
          UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
          return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
          break;
      }
      break;
    case UA_ATTRIBUTEID_SYMMETRIC:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_REFERENCETYPE)
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(Boolean, &anyTypeNode.rtObj->symmetric);
      break;
    case UA_ATTRIBUTEID_INVERSENAME:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_REFERENCETYPE)
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(LocalizedText, &anyTypeNode.rtObj->inverseName);
      break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VIEW)
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(Boolean, &anyTypeNode.vwObj->containsNoLoops);
      break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VIEW | UA_NODECLASS_OBJECT)
      switch(anyTypeNode.node->nodeClass) {
        case UA_NODECLASS_VIEW:
          SERVER_GETATTRIBUTE_COPYTYPEVALUE(Byte, &anyTypeNode.vwObj->eventNotifier);
          break;
        case UA_NODECLASS_OBJECT:
          SERVER_GETATTRIBUTE_COPYTYPEVALUE(Byte, &anyTypeNode.oObj->eventNotifier);
          break;
        default:
          UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
          return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
          break;
      }
      break;
    case UA_ATTRIBUTEID_VALUE:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE | UA_NODECLASS_VARIABLETYPE)
      switch(anyTypeNode.node->nodeClass) {
        case UA_NODECLASS_VARIABLE:
          *value = UA_Variant_new();
          if(anyTypeNode.vObj->valueSource == UA_VALUESOURCE_VARIANT) {
            retval |= UA_Variant_copy(&anyTypeNode.vObj->value.variant, *value);
          } else {
            UA_DataValue ds;
            UA_DataValue_init(&ds);
            retval |= anyTypeNode.vObj->value.dataSource.read(anyTypeNode.vObj->value.dataSource.handle, UA_FALSE, UA_NULL, &ds);
            UA_Variant_copy(&ds.value, *value);
          }
          break;
        case UA_NODECLASS_VARIABLETYPE:
          // Note: Precicely the same as variableNode above!
          *value = UA_Variant_new();
          if(anyTypeNode.vtObj->valueSource == UA_VALUESOURCE_VARIANT) {
            retval |= UA_Variant_copy(&anyTypeNode.vtObj->value.variant, *value);
          } else {
            UA_DataValue ds;
            UA_DataValue_init(&ds);
            retval |= anyTypeNode.vtObj->value.dataSource.read(anyTypeNode.vtObj->value.dataSource.handle, UA_FALSE, UA_NULL, &ds);
            UA_Variant_copy(&ds.value, *value);
          }
          break;
        default:
          UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
          return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      }
      break;
    case UA_ATTRIBUTEID_DATATYPE:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_VALUERANK:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE)
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(Byte, &anyTypeNode.vObj->accessLevel);
      break;
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE)
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(Byte, &anyTypeNode.vObj->userAccessLevel);
      break;
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE)
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(Double, &anyTypeNode.vObj->minimumSamplingInterval);
      break;
    case UA_ATTRIBUTEID_HISTORIZING:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_VARIABLE)
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(Boolean, &anyTypeNode.vObj->historizing);
      break;
    case UA_ATTRIBUTEID_EXECUTABLE:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_METHOD)
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(Boolean, &anyTypeNode.mObj->executable);
      break;
    case UA_ATTRIBUTEID_USEREXECUTABLE:
      SETATTRIBUTE_ASSERTNODECLASS(UA_NODECLASS_METHOD)
      SERVER_GETATTRIBUTE_COPYTYPEVALUE(Boolean, &anyTypeNode.mObj->userExecutable);
      break;
    default:
      UA_Server_deleteNodeCopy(server, (void **) &anyTypeNode.node);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
  }
  
  return retval;
}


#define arrayOfNodeIds_addNodeId(ARRAYNAME, NODEID) { \
  ARRAYNAME.size++; \
  ARRAYNAME.ids = UA_realloc(ARRAYNAME.ids, sizeof(UA_NodeId) * ARRAYNAME.size); \
  UA_NodeId_copy(&NODEID, &ARRAYNAME.ids[ARRAYNAME.size-1]); \
}

#define arrayOfNodeIds_deleteMembers(ARRAYNAME) { \
  if (ARRAYNAME.size > 0)   \
    UA_free(ARRAYNAME.ids); \
  ARRAYNAME.size = 0;       \
}

#define arrayOfNodeIds_idInArray(ARRAYNAME, NODEID, BOOLEAN, BOOLINIT) { \
  BOOLEAN = BOOLINIT;\
  for (int z=0; z<ARRAYNAME.size; z++) {\
    if (UA_NodeId_equal(&ARRAYNAME.ids[z], &NODEID)) {\
      BOOLEAN = !BOOLINIT; \
      break; \
    } \
  } \
} \

static void UA_Server_addInstanceOf_inheritParentAttributes(UA_Server *server, arrayOfNodeIds *subtypeRefs, arrayOfNodeIds *componentRefs, 
                                                            UA_NodeId objectRoot, UA_InstantiationCallback callback, UA_ObjectTypeNode *typeDefNode,
                                                            arrayOfNodeIds *instantiatedTypes, void *handle) 
{
  UA_Boolean refTypeValid;
  UA_ReferenceNode ref;
  arrayOfNodeIds visitedNodes = (arrayOfNodeIds) {.size=0, .ids = UA_NULL};
  for(int i=0; i<typeDefNode->referencesSize; i++) {
    ref = typeDefNode->references[i];
    if (ref.isInverse == UA_FALSE)
      continue;
    refTypeValid = UA_FALSE;
    arrayOfNodeIds_idInArray((*subtypeRefs), ref.referenceTypeId, refTypeValid, UA_FALSE);
    if (!refTypeValid) 
      continue;
    
    // Check if already tried to inherit from this node (there is such a thing as duplicate refs)
    arrayOfNodeIds_idInArray(visitedNodes, ref.targetId.nodeId, refTypeValid, UA_TRUE);
    if (!refTypeValid) 
      continue;
    // Go ahead and inherit this nodes variables and methods (not objects!)
    arrayOfNodeIds_addNodeId(visitedNodes, ref.targetId.nodeId);
    UA_Server_appendInstanceOfSupertype(server, ref.targetId.nodeId, objectRoot, subtypeRefs, componentRefs, callback, instantiatedTypes, handle);
  } // End check all hassubtype refs
  arrayOfNodeIds_deleteMembers(visitedNodes);
  return;
}

static void UA_Server_addInstanceOf_instatiateChildObject(UA_Server *server, 
                                                          arrayOfNodeIds *subtypeRefs, arrayOfNodeIds *componentRefs, arrayOfNodeIds *typedefRefs,
                                                          UA_Node *objectCopy, UA_NodeId parentId, UA_ExpandedNodeId typeDefinition, UA_NodeId referenceTypeId,
                                                          UA_InstantiationCallback callback, UA_Boolean instantiateObjects, arrayOfNodeIds *instantiatedTypes, 
                                                          void *handle) 
{
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  
  // Refuse to create this node if we detect a circular type definition
  UA_Boolean typeDefRecursion;
  arrayOfNodeIds_idInArray((*instantiatedTypes), typeDefinition.nodeId, typeDefRecursion, UA_FALSE);
  if (typeDefRecursion) 
    return;
  
  UA_Node *typeDefNode;
  UA_Server_getNodeCopy(server, typeDefinition.nodeId, (void *) &typeDefNode);
  
  if (typeDefNode == UA_NULL) {
    return;
  }

  if (typeDefNode->nodeClass != UA_NODECLASS_OBJECTTYPE) {
    UA_Server_deleteNodeCopy(server, (void **) &typeDefNode);
    return;
  }
  
  // Create the object root as specified by the user
  UA_NodeId objectRoot;
  retval |= UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(parentId.namespaceIndex, 0), objectCopy->browseName, objectCopy->displayName, 
                                    objectCopy->description, parentId, referenceTypeId, objectCopy->userWriteMask, objectCopy->writeMask,
                                    typeDefinition, &objectRoot);
  if (retval)
    return;
  
  if (callback != UA_NULL)
    callback(objectRoot, typeDefinition.nodeId, handle);
  
  // (1) If this node is a subtype of any other node, create its things first
  UA_Server_addInstanceOf_inheritParentAttributes(server, subtypeRefs, componentRefs, objectRoot, callback, 
                                                  (UA_ObjectTypeNode *) typeDefNode, instantiatedTypes, handle);
  
  // (2) For each object or variable referenced with hasComponent or hasProperty, create a new node of that
  //     type for this objectRoot
  UA_Server_addInstanceOf_instatiateChildNode(server, subtypeRefs, componentRefs, typedefRefs,
                                              objectRoot, callback, (UA_ObjectTypeNode *) typeDefNode, 
                                              UA_TRUE, instantiatedTypes, handle);
  
  return;
}
  
void UA_Server_addInstanceOf_instatiateChildNode(UA_Server *server, 
                                                arrayOfNodeIds *subtypeRefs, arrayOfNodeIds *componentRefs, arrayOfNodeIds *typedefRefs,
                                                UA_NodeId objectRoot, UA_InstantiationCallback callback, void *typeDefNode,
                                                UA_Boolean instantiateObjects, arrayOfNodeIds *instantiatedTypes, void *handle) 
{
  UA_Boolean refTypeValid;
  UA_NodeClass *refClass;
  UA_Node      *nodeClone;
  UA_ExpandedNodeId *objectRootExpanded = UA_ExpandedNodeId_new();
  UA_VariableNode *newVarNode;
  UA_VariableTypeNode *varTypeNode;
  UA_ReferenceNode ref;
  UA_NodeId_copy(&objectRoot, &objectRootExpanded->nodeId );
  UA_AddNodesResult adres;
  for(int i=0; i< ((UA_ObjectTypeNode *) typeDefNode)->referencesSize; i++) {
    ref = ((UA_ObjectTypeNode *) typeDefNode)->references[i];
    if (ref.isInverse)
      continue;
    arrayOfNodeIds_idInArray((*componentRefs), ref.referenceTypeId, refTypeValid, UA_FALSE);
    if (!refTypeValid) 
      continue;
    
    // What type of node is this?
    refClass = UA_NULL;
    UA_Server_getAttributeValue(server, ref.targetId.nodeId, UA_ATTRIBUTEID_NODECLASS, (void **) &refClass);
    switch (*refClass) {
      case UA_NODECLASS_VARIABLE: // Just clone the variable node with a new nodeId
        UA_Server_getNodeCopy(server, ref.targetId.nodeId, (void **) &nodeClone);
        if (nodeClone == UA_NULL)
          break;
        UA_NodeId_init(&nodeClone->nodeId);
        nodeClone->nodeId.namespaceIndex = objectRoot.namespaceIndex;
        if (nodeClone != UA_NULL) {
          adres = UA_Server_addNode(server, nodeClone,  *objectRootExpanded, ref.referenceTypeId);
          if (callback != UA_NULL)
            callback(adres.addedNodeId, ref.targetId.nodeId, handle);
        }
        break;
      case UA_NODECLASS_VARIABLETYPE: // Convert from a value protoype to a value, then add it
        UA_Server_getNodeCopy(server, ref.targetId.nodeId, (void **) &varTypeNode);
        newVarNode = UA_VariableNode_new();
        newVarNode->nodeId.namespaceIndex = objectRoot.namespaceIndex;
        UA_QualifiedName_copy(&varTypeNode->browseName, &varTypeNode->browseName);
        UA_LocalizedText_copy(&varTypeNode->displayName, &varTypeNode->displayName);
        UA_LocalizedText_copy(&varTypeNode->description, &varTypeNode->description);
        newVarNode->writeMask = varTypeNode->writeMask;
        newVarNode->userWriteMask = varTypeNode->userWriteMask;
        newVarNode->valueRank = varTypeNode->valueRank;
        
        newVarNode->valueSource = varTypeNode->valueSource;
        if (varTypeNode->valueSource == UA_VALUESOURCE_DATASOURCE)
          newVarNode->value.dataSource = varTypeNode->value.dataSource;
        else
          UA_Variant_copy(&varTypeNode->value.variant, &newVarNode->value.variant);
        
        adres = UA_Server_addNode(server, (UA_Node *) newVarNode, *objectRootExpanded, ref.referenceTypeId);
        if (callback != UA_NULL)
          callback(adres.addedNodeId, ref.targetId.nodeId, handle);
        UA_Server_deleteNodeCopy(server, (void **) &newVarNode);
        UA_Server_deleteNodeCopy(server, (void **) &varTypeNode);
        break;
      case UA_NODECLASS_OBJECT: // An object may have it's own inheritance or child nodes
        if (!instantiateObjects)
          break;
        
        UA_Server_getNodeCopy(server, ref.targetId.nodeId, (void **) &nodeClone);
        if (nodeClone == UA_NULL)
          break; // switch
          
          // Retrieve this nodes type definition
          UA_ExpandedNodeId_init(objectRootExpanded); // Slight misuse of an unsused ExpandedNodeId to encode the typeDefinition
          int tidx;
          for(tidx=0; tidx<nodeClone->referencesSize; tidx++) {
            arrayOfNodeIds_idInArray((*typedefRefs), nodeClone->references[tidx].referenceTypeId, refTypeValid, UA_FALSE);
            if (refTypeValid) 
              break;
          } // End iterate over nodeClone refs
          
          if (!refTypeValid) // This may be plain wrong, but since got this far...
            objectRootExpanded->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE);
          else
            UA_ExpandedNodeId_copy(&nodeClone->references[tidx].targetId, objectRootExpanded);
          
          int lastArrayDepth = instantiatedTypes->size;
          arrayOfNodeIds_addNodeId((*instantiatedTypes), ((UA_ObjectTypeNode *) typeDefNode)->nodeId);
          
          UA_Server_addInstanceOf_instatiateChildObject(server, subtypeRefs, componentRefs, typedefRefs, nodeClone,
                                                        objectRoot, *objectRootExpanded, ref.referenceTypeId,
                                                        callback, UA_TRUE, instantiatedTypes, handle);
          instantiatedTypes->size = lastArrayDepth;
          instantiatedTypes->ids = (UA_NodeId *) realloc(instantiatedTypes->ids, lastArrayDepth);
          
          UA_Server_deleteNodeCopy(server, (void **) &nodeClone);
          UA_ExpandedNodeId_deleteMembers(objectRootExpanded); // since we only borrowed this, reset it
          UA_NodeId_copy(&objectRoot, &objectRootExpanded->nodeId );
          break;
      case UA_NODECLASS_METHOD: // Link this method (don't clone the node)
        UA_Server_AddMonodirectionalReference(server, objectRoot, ref.targetId, ref.referenceTypeId, UA_TRUE);
        break;
      default:
        break;
    }
    UA_NodeClass_delete(refClass);
  }
  
  if (objectRootExpanded != UA_NULL)
    UA_ExpandedNodeId_delete(objectRootExpanded);
  return;
}

UA_StatusCode UA_Server_appendInstanceOfSupertype(UA_Server *server, UA_NodeId nodeId, UA_NodeId appendToNodeId, 
                                                  arrayOfNodeIds *subtypeRefs, arrayOfNodeIds *componentRefs, 
                                                  UA_InstantiationCallback callback, arrayOfNodeIds *instantiatedTypes, 
                                                  void *handle)  
{
  UA_StatusCode retval = UA_STATUSCODE_GOOD;

  UA_Node *typeDefNode = UA_NULL;
  UA_Server_getNodeCopy(server, nodeId, (void *) &typeDefNode);
  if (typeDefNode == UA_NULL) {
  return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
  }
<<<<<<< HEAD
  if (typeDefNode->nodeClass != UA_NODECLASS_OBJECTTYPE) {
=======
  if (!(typeDefNode->nodeClass == UA_NODECLASS_OBJECTTYPE)) {
>>>>>>> api_hl_abstractions
  UA_Server_deleteNodeCopy(server, (void **) &typeDefNode);
  return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
  }

  UA_ExpandedNodeId *objectRootExpanded = UA_ExpandedNodeId_new();
  UA_NodeId_copy(&appendToNodeId, &objectRootExpanded->nodeId );
  // (1) If this node is a subtype of any other node, create its things first
  UA_Server_addInstanceOf_inheritParentAttributes(server, subtypeRefs, componentRefs, appendToNodeId, callback, 
                                                  (UA_ObjectTypeNode *) typeDefNode, instantiatedTypes, handle);

  UA_Server_addInstanceOf_instatiateChildNode(server, subtypeRefs, componentRefs, UA_NULL, 
                                              appendToNodeId, callback, (UA_ObjectTypeNode *) typeDefNode, 
                                              UA_FALSE, instantiatedTypes, handle);
  if (objectRootExpanded != UA_NULL)
    UA_ExpandedNodeId_delete(objectRootExpanded);
  return retval;
}

UA_StatusCode UA_Server_addInstanceOf(UA_Server *server, UA_NodeId nodeId, const UA_QualifiedName browseName,
                        UA_LocalizedText displayName, UA_LocalizedText description, const UA_NodeId parentNodeId, 
                        const UA_NodeId referenceTypeId, UA_UInt32 userWriteMask, UA_UInt32 writeMask, 
                        const UA_ExpandedNodeId typeDefinition, UA_InstantiationCallback callback, void *handle, 
                        UA_NodeId *createdNodeId) 
{
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  
  UA_Node *typeDefNode = UA_NULL;
  UA_Server_getNodeCopy(server, typeDefinition.nodeId, (void *) &typeDefNode);
  
  if (typeDefNode == UA_NULL) {
    return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
  }
<<<<<<< HEAD
  if (typeDefNode->nodeClass != UA_NODECLASS_OBJECTTYPE) {
=======
  if (!(typeDefNode->nodeClass == UA_NODECLASS_OBJECTTYPE)) {
>>>>>>> api_hl_abstractions
    UA_Server_deleteNodeCopy(server, (void **) &typeDefNode);
    return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
  }
  
  // Create the object root as specified by the user
  UA_NodeId objectRoot;
  retval |= UA_Server_addObjectNode(server, nodeId, browseName, displayName, description, 
                          parentNodeId, referenceTypeId, userWriteMask, writeMask,
                          typeDefinition, &objectRoot
  );
  if (retval)
    return retval;
  
  // These refs will be examined later. 
  // FIXME: Create these arrays dynamically to include any subtypes as well
  arrayOfNodeIds subtypeRefs = (arrayOfNodeIds) {
    .size  = 1,
    .ids   = (UA_NodeId[]) { UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE)}
  };
  arrayOfNodeIds componentRefs = (arrayOfNodeIds) {
    .size = 2,
    .ids  = (UA_NodeId[]) { UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY)}
  };
  arrayOfNodeIds typedefRefs = (arrayOfNodeIds) {
    .size = 1,
    .ids  = (UA_NodeId[]) { UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION)}
  };
  
  UA_ExpandedNodeId *objectRootExpanded = UA_ExpandedNodeId_new();
  UA_NodeId_copy(&objectRoot, &objectRootExpanded->nodeId );
  
  arrayOfNodeIds instantiatedTypes = (arrayOfNodeIds ) {.size=0, .ids=NULL};
  arrayOfNodeIds_addNodeId(instantiatedTypes, typeDefNode->nodeId);
  
  // (1) If this node is a subtype of any other node, create its things first
  UA_Server_addInstanceOf_inheritParentAttributes(server, &subtypeRefs, &componentRefs, objectRoot, callback, 
                                                  (UA_ObjectTypeNode *) typeDefNode, &instantiatedTypes, handle);
  
  // (2) For each object or variable referenced with hasComponent or hasProperty, create a new node of that
  //     type for this objectRoot
  UA_Server_addInstanceOf_instatiateChildNode(server, &subtypeRefs, &componentRefs, &typedefRefs,
                                              objectRoot, callback, (UA_ObjectTypeNode *) typeDefNode, 
                                              UA_TRUE, &instantiatedTypes, handle);
  arrayOfNodeIds_deleteMembers(instantiatedTypes);
  
  UA_ExpandedNodeId_delete(objectRootExpanded);
  UA_Server_deleteNodeCopy(server, (void **) &typeDefNode);
  return retval;
}
