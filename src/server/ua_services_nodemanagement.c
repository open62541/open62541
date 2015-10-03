#include "ua_util.h"
#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_session.h"
#include "ua_types_generated_encoding_binary.h"

/************/
/* Add Node */
/************/

void UA_Server_addExistingNode(UA_Server *server, UA_Session *session, UA_Node *node,
                               const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId,
                               UA_AddNodesResult *result) {
    if(node->nodeId.namespaceIndex >= server->namespacesSize) {
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return;
    }

    const UA_Node *parent = UA_NodeStore_get(server->nodestore, parentNodeId);
    if(!parent) {
        result->statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID;
        return;
    }

    const UA_ReferenceTypeNode *referenceType =
        (const UA_ReferenceTypeNode *)UA_NodeStore_get(server->nodestore, referenceTypeId);
    if(!referenceType) {
        result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        goto ret;
    }

    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        goto ret2;
    }

    if(referenceType->isAbstract == UA_TRUE) {
        result->statusCode = UA_STATUSCODE_BADREFERENCENOTALLOWED;
        goto ret2;
    }

    // todo: test if the referencetype is hierarchical
    // todo: namespace index is assumed to be valid
    const UA_Node *managed = UA_NULL;
    UA_NodeId tempNodeid = node->nodeId;
    tempNodeid.namespaceIndex = 0;
    if(UA_NodeId_isNull(&tempNodeid)) {
        if(UA_NodeStore_insert(server->nodestore, node, &managed) != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }
        result->addedNodeId = managed->nodeId; // cannot fail as unique nodeids are numeric
    } else {
        if(UA_NodeId_copy(&node->nodeId, &result->addedNodeId) != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            goto ret2;
        }

        if(UA_NodeStore_insert(server->nodestore, node, &managed) != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADNODEIDEXISTS;
            UA_NodeId_deleteMembers(&result->addedNodeId);
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
    Service_AddReferences_single(server, session, &item);

    // todo: error handling. remove new node from nodestore
    UA_NodeStore_release(managed);
    
 ret2:
    UA_NodeStore_release((const UA_Node*)referenceType);
 ret:
    UA_NodeStore_release(parent);
}

static void moveStandardAttributes(UA_Node *node, UA_AddNodesItem *item, UA_NodeAttributes *attr) {
    node->nodeId = item->requestedNewNodeId.nodeId;
    UA_NodeId_init(&item->requestedNewNodeId.nodeId);

    node->browseName = item->browseName;
    UA_QualifiedName_init(&item->browseName);

    node->displayName = attr->displayName;
    UA_LocalizedText_init(&attr->displayName);

    node->description = attr->description;
    UA_LocalizedText_init(&attr->description);

    node->writeMask = attr->writeMask;
    node->userWriteMask = attr->userWriteMask;
}

static void
Service_AddNodes_single_fromVariableAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                               UA_VariableAttributes *attr, UA_AddNodesResult *result) {
    UA_VariableNode *vnode = UA_VariableNode_new();
    if(!vnode) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    moveStandardAttributes((UA_Node*)vnode, item, (UA_NodeAttributes*)attr);
    // todo: test if the type / valueRank / value attributes are consistent
    vnode->accessLevel = attr->accessLevel;
    vnode->userAccessLevel = attr->userAccessLevel;
    vnode->historizing = attr->historizing;
    vnode->minimumSamplingInterval = attr->minimumSamplingInterval;
    vnode->valueRank = attr->valueRank;
    vnode->value.variant.value = attr->value;
    UA_Variant_init(&attr->value);

    // don't use extra dimension spec. This comes from the value.
    /* if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_ARRAYDIMENSIONS) { */
    /*     vnode->arrayDimensionsSize = attr.arrayDimensionsSize; */
    /*     vnode->arrayDimensions = attr.arrayDimensions; */
    /*     attr.arrayDimensionsSize = -1; */
    /*     attr.arrayDimensions = UA_NULL; */
    /* } */

    // don't use the extra type id. This comes from the value.
    /* if(attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_DATATYPE || */
    /*    attr.specifiedAttributes & UA_NODEATTRIBUTESMASK_OBJECTTYPEORDATATYPE) { */
    /*     vnode->dataType = attr.dataType; */
    /*     UA_NodeId_init(&attr.dataType); */
    /* } */

    UA_Server_addExistingNode(server, session, (UA_Node*)vnode, &item->parentNodeId.nodeId,
                              &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_VariableNode_delete(vnode);
}

static void
Service_AddNodes_single_fromObjectAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                             UA_ObjectAttributes *attr, UA_AddNodesResult *result) {
    UA_ObjectNode *onode = UA_ObjectNode_new();
    if(!onode) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    moveStandardAttributes((UA_Node*)onode, item, (UA_NodeAttributes*)attr);
    onode->eventNotifier = attr->eventNotifier;

    UA_Server_addExistingNode(server, session, (UA_Node*)onode, &item->parentNodeId.nodeId,
                              &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_ObjectNode_delete(onode);
}

static void
Service_AddNodes_single_fromReferenceTypeAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                                    UA_ReferenceTypeAttributes *attr, UA_AddNodesResult *result) {
    UA_ReferenceTypeNode *rtnode = UA_ReferenceTypeNode_new();
    if(!rtnode) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    
    moveStandardAttributes((UA_Node*)rtnode, item, (UA_NodeAttributes*)&attr);
    rtnode->isAbstract = attr->isAbstract;
    rtnode->symmetric = attr->symmetric;
    rtnode->inverseName = attr->inverseName;
    UA_LocalizedText_init(&attr->inverseName);

    UA_Server_addExistingNode(server, session, (UA_Node*)rtnode, &item->parentNodeId.nodeId,
                              &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_ReferenceTypeNode_delete(rtnode);
}

static void
Service_AddNodes_single_fromObjectTypeAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                                 UA_ObjectTypeAttributes *attr, UA_AddNodesResult *result) {
    UA_ObjectTypeNode *otnode = UA_ObjectTypeNode_new();
    if(!otnode) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    moveStandardAttributes((UA_Node*)otnode, item, (UA_NodeAttributes*)attr);
    otnode->isAbstract = attr->isAbstract;

    UA_Server_addExistingNode(server, session, (UA_Node*)otnode, &item->parentNodeId.nodeId,
                              &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_ObjectTypeNode_delete(otnode);
}

static void
Service_AddNodes_single_fromVariableTypeAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                                   UA_VariableTypeAttributes *attr, UA_AddNodesResult *result) {
    UA_VariableTypeNode *vtnode = UA_VariableTypeNode_new();
    if(!vtnode) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    moveStandardAttributes((UA_Node*)vtnode, item, (UA_NodeAttributes*)attr);
    vtnode->value.variant.value = attr->value;
    UA_Variant_init(&attr->value);
    // datatype is taken from the value
    vtnode->valueRank = attr->valueRank;
    // array dimensions are taken from the value
    vtnode->isAbstract = attr->isAbstract;

    UA_Server_addExistingNode(server, session, (UA_Node*)vtnode, &item->parentNodeId.nodeId,
                              &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_VariableTypeNode_delete(vtnode);
}

static void
Service_AddNodes_single_fromViewAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                           UA_ViewAttributes *attr, UA_AddNodesResult *result) {
    UA_ViewNode *vnode = UA_ViewNode_new();
    if(!vnode) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    moveStandardAttributes((UA_Node*)vnode, item, (UA_NodeAttributes*)attr);
    vnode->containsNoLoops = attr->containsNoLoops;
    vnode->eventNotifier = attr->eventNotifier;

    UA_Server_addExistingNode(server, session, (UA_Node*)vnode, &item->parentNodeId.nodeId,
                              &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_ViewNode_delete(vnode);
}

static void
Service_AddNodes_single_fromDataTypeAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                               UA_DataTypeAttributes *attr, UA_AddNodesResult *result) {
    UA_DataTypeNode *dtnode = UA_DataTypeNode_new();
    if(!dtnode) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    moveStandardAttributes((UA_Node*)dtnode, item, (UA_NodeAttributes*)attr);
    dtnode->isAbstract = attr->isAbstract;

    UA_Server_addExistingNode(server, session, (UA_Node*)dtnode,
                              &item->parentNodeId.nodeId, &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_DataTypeNode_delete(dtnode);
}

void Service_AddNodes_single(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                             UA_NodeAttributes *attr, const UA_DataType *attributeType,
                             UA_AddNodesResult *result) {
    switch(attributeType->typeIndex) {
    case UA_TYPES_OBJECTATTRIBUTES:
        Service_AddNodes_single_fromObjectAttributes(server, session, item, (UA_ObjectAttributes*)attr, result);
        break;
    case UA_TYPES_OBJECTTYPEATTRIBUTES:
        Service_AddNodes_single_fromObjectTypeAttributes(server, session, item, (UA_ObjectTypeAttributes*)attr, result);
        break;
    case UA_TYPES_VARIABLEATTRIBUTES:
        Service_AddNodes_single_fromVariableAttributes(server, session, item, (UA_VariableAttributes*)attr, result);
        break;
    case UA_TYPES_VARIABLETYPEATTRIBUTES:
        Service_AddNodes_single_fromVariableTypeAttributes(server, session, item,
                                                           (UA_VariableTypeAttributes*)attr, result);
        break;
    case UA_TYPES_REFERENCETYPEATTRIBUTES:
        Service_AddNodes_single_fromReferenceTypeAttributes(server, session, item,
                                                            (UA_ReferenceTypeAttributes*)attr, result);
        break;
    case UA_TYPES_VIEWATTRIBUTES:
        Service_AddNodes_single_fromViewAttributes(server, session, item, (UA_ViewAttributes*)attr, result);
        break;
    case UA_TYPES_DATATYPEATTRIBUTES:
        Service_AddNodes_single_fromDataTypeAttributes(server, session, item, (UA_DataTypeAttributes*)attr, result);
        break;
    default:
        result->statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
    }
}

static void Service_AddNodes_single_unparsed(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                                             UA_AddNodesResult *result) {
    /* adding nodes to ns0 is not allowed over the wire */
    if(item->requestedNewNodeId.nodeId.namespaceIndex == 0) {
        result->statusCode = UA_STATUSCODE_BADNODEIDREJECTED;
        return;
    }

    const UA_DataType *attributeType;
    switch (item->nodeClass) {
    case UA_NODECLASS_OBJECT:
        attributeType = &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES];
        break;
    case UA_NODECLASS_OBJECTTYPE:
        attributeType = &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES];
        break;
    case UA_NODECLASS_REFERENCETYPE:
        attributeType = &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES];
        break;
    case UA_NODECLASS_VARIABLE:
        attributeType = &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES];
        break;
    case UA_NODECLASS_VIEW:
        attributeType = &UA_TYPES[UA_TYPES_VIEWATTRIBUTES];
        break;
    case UA_NODECLASS_DATATYPE:
        attributeType = &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES];
        break;
    default:
        result->statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
        return;
    }

    if(!UA_NodeId_equal(&item->nodeAttributes.typeId, &attributeType->typeId)) {
        result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
        return;
    }

    UA_NodeAttributes *attr = UA_alloca(attributeType->memSize);
    size_t pos = 0;
    result->statusCode = UA_decodeBinary(&item->nodeAttributes.body, &pos, &attr, attributeType);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    Service_AddNodes_single(server, session, item, attr, attributeType, result);
    UA_deleteMembers(attr, attributeType);
}

void Service_AddNodes(UA_Server *server, UA_Session *session, const UA_AddNodesRequest *request,
                      UA_AddNodesResponse *response) {
    if(request->nodesToAddSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }
    size_t size = request->nodesToAddSize;

    response->results = UA_Array_new(&UA_TYPES[UA_TYPES_ADDNODESRESULT], size);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    
#ifdef UA_EXTERNAL_NAMESPACES
#ifdef _MSVC_VER
    UA_Boolean *isExternal = UA_alloca(size);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32)*size);
#else
    UA_Boolean isExternal[size];
    UA_UInt32 indices[size];
#endif
    UA_memset(isExternal, UA_FALSE, sizeof(UA_Boolean) * size);
    for(size_t j = 0; j <server->externalNamespacesSize; j++) {
        size_t indexSize = 0;
        for(size_t i = 0;i < size;i++) {
            if(request->nodesToAdd[i].requestedNewNodeId.nodeId.namespaceIndex !=
               server->externalNamespaces[j].index)
                continue;
            isExternal[i] = UA_TRUE;
            indices[indexSize] = i;
            indexSize++;
        }
        if(indexSize == 0)
            continue;
        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->addNodes(ens->ensHandle, &request->requestHeader, request->nodesToAdd,
                      indices, indexSize, response->results, response->diagnosticInfos);
    }
#endif
    
    response->resultsSize = size;
    for(size_t i = 0;i < size;i++) {
#ifdef UA_EXTERNAL_NAMESPACES
        if(!isExternal[i])
#endif
            Service_AddNodes_single_unparsed(server, session, &request->nodesToAdd[i], &response->results[i]);
    }
}

/**************************************************/
/* Add Special Nodes (not possible over the wire) */
/**************************************************/

UA_AddNodesResult
UA_Server_addDataSourceVariableNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                    const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
                                    const UA_VariableAttributes attr, const UA_DataSource dataSource) {
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);

    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    result.statusCode = UA_QualifiedName_copy(&browseName, &item.browseName);
    item.nodeClass = UA_NODECLASS_VARIABLE;
    result.statusCode |= UA_NodeId_copy(&parentNodeId, &item.parentNodeId.nodeId);
    result.statusCode |= UA_NodeId_copy(&referenceTypeId, &item.referenceTypeId);
    result.statusCode |= UA_NodeId_copy(&requestedNewNodeId, &item.requestedNewNodeId.nodeId);
    result.statusCode |= UA_NodeId_copy(&typeDefinition, &item.typeDefinition.nodeId);
    
    UA_VariableAttributes attrCopy;
    result.statusCode |= UA_VariableAttributes_copy(&attr, &attrCopy);
    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_AddNodesItem_deleteMembers(&item);
        UA_VariableAttributes_deleteMembers(&attrCopy);
        return result;
    }

    UA_VariableNode *node = UA_VariableNode_new();
    if(!node) {
        result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_AddNodesItem_deleteMembers(&item);
        UA_VariableAttributes_deleteMembers(&attrCopy);
        return result;
    }

    moveStandardAttributes((UA_Node*)node, &item, (UA_NodeAttributes*)&attrCopy);
    node->valueSource = UA_VALUESOURCE_DATASOURCE;
    node->value.dataSource = dataSource;
    node->accessLevel = attr.accessLevel;
    node->userAccessLevel = attr.userAccessLevel;
    node->historizing = attr.historizing;
    node->minimumSamplingInterval = attr.minimumSamplingInterval;
    node->valueRank = attr.valueRank;

    UA_Server_addExistingNode(server, &adminSession, (UA_Node*)node, &item.parentNodeId.nodeId,
                              &item.referenceTypeId, &result);
    UA_AddNodesItem_deleteMembers(&item);
    UA_VariableAttributes_deleteMembers(&attrCopy);

    if(result.statusCode != UA_STATUSCODE_GOOD)
        UA_VariableNode_delete(node);
    return result;
}

#ifdef ENABLE_METHODCALLS
UA_AddNodesResult UA_EXPORT
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                        UA_MethodCallback method, void *handle,
                        UA_Int32 inputArgumentsSize, const UA_Argument* inputArguments, 
                        UA_Int32 outputArgumentsSize, const UA_Argument* outputArguments) {
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    result.statusCode = UA_QualifiedName_copy(&browseName, &item.browseName);
    item.nodeClass = UA_NODECLASS_METHOD;
    result.statusCode |= UA_NodeId_copy(&parentNodeId, &item.parentNodeId.nodeId);
    result.statusCode |= UA_NodeId_copy(&referenceTypeId, &item.referenceTypeId);
    result.statusCode |= UA_NodeId_copy(&requestedNewNodeId, &item.requestedNewNodeId.nodeId);
    
    UA_MethodAttributes attrCopy;
    result.statusCode |= UA_MethodAttributes_copy(&attr, &attrCopy);
    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_AddNodesItem_deleteMembers(&item);
        UA_MethodAttributes_deleteMembers(&attrCopy);
        return result;
    }

    UA_MethodNode *node = UA_MethodNode_new();
    if(!node) {
        result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_AddNodesItem_deleteMembers(&item);
        UA_MethodAttributes_deleteMembers(&attrCopy);
        return result;
    }
    
    moveStandardAttributes((UA_Node*)node, &item, (UA_NodeAttributes*)&attrCopy);
    node->executable = attrCopy.executable;
    node->userExecutable = attrCopy.executable;
    node->attachedMethod = method;
    node->methodHandle = handle;
    UA_AddNodesItem_deleteMembers(&item);
    UA_MethodAttributes_deleteMembers(&attrCopy);

    UA_Server_addExistingNode(server, &adminSession, (UA_Node*)node, &item.parentNodeId.nodeId,
                              &item.referenceTypeId, &result);
    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_MethodNode_delete(node);
        return result;
    }
    
    /* Only proceed with creating in/outputArguments if the method and both arguments are not
     * UA_NULL; otherwise this is a pretty strong indicator that this node was generated,
     * in which case these arguments will be created later and individually.
     */
    if(method == UA_NULL && inputArguments == UA_NULL && outputArguments == UA_NULL &&
       inputArgumentsSize <= 0 && outputArgumentsSize <= 0)
        return result;

    UA_ExpandedNodeId parent;
    UA_ExpandedNodeId_init(&parent);
    parent.nodeId = result.addedNodeId;
    
    /* create InputArguments */
    UA_VariableNode *inputArgumentsVariableNode = UA_VariableNode_new();
    inputArgumentsVariableNode->nodeId.namespaceIndex = result.addedNodeId.namespaceIndex;
    inputArgumentsVariableNode->browseName = UA_QUALIFIEDNAME_ALLOC(0,"InputArguments");
    inputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", "InputArguments");
    inputArgumentsVariableNode->description = UA_LOCALIZEDTEXT_ALLOC("en_US", "InputArguments");
    inputArgumentsVariableNode->valueRank = 1;
    UA_Variant_setArrayCopy(&inputArgumentsVariableNode->value.variant.value, inputArguments,
                            inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
    UA_AddNodesResult inputAddRes;
    const UA_NodeId hasproperty = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    UA_Server_addExistingNode(server, &adminSession, (UA_Node*)inputArgumentsVariableNode,
                              &parent.nodeId, &hasproperty, &inputAddRes);
    // todo: check if adding succeeded
    UA_AddNodesResult_deleteMembers(&inputAddRes);

    /* create OutputArguments */
    UA_VariableNode *outputArgumentsVariableNode  = UA_VariableNode_new();
    outputArgumentsVariableNode->nodeId.namespaceIndex = result.addedNodeId.namespaceIndex;
    outputArgumentsVariableNode->browseName  = UA_QUALIFIEDNAME_ALLOC(0,"OutputArguments");
    outputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OutputArguments");
    outputArgumentsVariableNode->description = UA_LOCALIZEDTEXT_ALLOC("en_US", "OutputArguments");
    outputArgumentsVariableNode->valueRank = 1;
    UA_Variant_setArrayCopy(&outputArgumentsVariableNode->value.variant.value, outputArguments,
                            outputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
    UA_AddNodesResult outputAddRes;
    UA_Server_addExistingNode(server, &adminSession, (UA_Node*)outputArgumentsVariableNode,
                              &parent.nodeId, &hasproperty, &outputAddRes);
    // todo: check if adding succeeded
    UA_AddNodesResult_deleteMembers(&outputAddRes);

    return result;
}
#endif

/******************/
/* Add References */
/******************/

/* Adds a one-way reference to the local nodestore */
static UA_StatusCode
addOneWayReference(UA_Server *server, UA_Session *session, UA_Node *node, const UA_AddReferencesItem *item) {
	size_t i = node->referencesSize;
	if(node->referencesSize < 0)
		i = 0;
    size_t refssize = (i+1) | 3; // so the realloc is not necessary every time
	UA_ReferenceNode *new_refs = UA_realloc(node->references, sizeof(UA_ReferenceNode) * refssize);
	if(!new_refs)
		return UA_STATUSCODE_BADOUTOFMEMORY;
    node->references = new_refs;
    UA_ReferenceNode_init(&new_refs[i]);
    UA_StatusCode retval = UA_NodeId_copy(&item->referenceTypeId, &new_refs[i].referenceTypeId);
    retval |= UA_ExpandedNodeId_copy(&item->targetNodeId, &new_refs[i].targetId);
    new_refs[i].isInverse = !item->isForward;
    if(retval == UA_STATUSCODE_GOOD) 
        node->referencesSize = i+1;
    else
        UA_ReferenceNode_deleteMembers(&new_refs[i]);
	return retval;
}

UA_StatusCode Service_AddReferences_single(UA_Server *server, UA_Session *session,
                                           const UA_AddReferencesItem *item) {
    if(item->targetServerUri.length > 0)
        return UA_STATUSCODE_BADNOTIMPLEMENTED; // currently no expandednodeids are allowed

    /* cast away the const to loop the call through UA_Server_editNode */
    UA_StatusCode retval = UA_Server_editNode(server, session, &item->sourceNodeId,
                                              (UA_EditNodeCallback)addOneWayReference,
                                              item);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_AddReferencesItem secondItem;
    secondItem = *item;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.isForward = !item->isForward;
    retval = UA_Server_editNode(server, session, &secondItem.sourceNodeId,
                                (UA_EditNodeCallback)addOneWayReference, &secondItem);

    // todo: remove reference if the second direction failed
    return retval;
} 

void Service_AddReferences(UA_Server *server, UA_Session *session, const UA_AddReferencesRequest *request,
                           UA_AddReferencesResponse *response) {
	if(request->referencesToAddSize <= 0) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
		return;
	}
    size_t size = request->referencesToAddSize;
	
    if(!(response->results = UA_malloc(sizeof(UA_StatusCode) * size))) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
		return;
	}
	response->resultsSize = size;

#ifdef UA_EXTERNAL_NAMESPACES
#ifdef NO_ALLOCA
    UA_Boolean isExternal[size];
    UA_UInt32 indices[size];
#else
    UA_Boolean *isExternal = UA_alloca(sizeof(UA_Boolean) * size);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32) * size);
#endif /*NO_ALLOCA */
    UA_memset(isExternal, UA_FALSE, sizeof(UA_Boolean) * size);
	for(size_t j = 0; j < server->externalNamespacesSize; j++) {
		size_t indicesSize = 0;
		for(size_t i = 0;i < size;i++) {
			if(request->referencesToAdd[i].sourceNodeId.namespaceIndex
               != server->externalNamespaces[j].index)
				continue;
			isExternal[i] = UA_TRUE;
			indices[indicesSize] = i;
			indicesSize++;
		}
		if (indicesSize == 0)
			continue;
		UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
		ens->addReferences(ens->ensHandle, &request->requestHeader, request->referencesToAdd,
                           indices, indicesSize, response->results, response->diagnosticInfos);
	}
#endif

	response->resultsSize = size;
	for(UA_Int32 i = 0; i < response->resultsSize; i++) {
#ifdef UA_EXTERNAL_NAMESPACES
		if(!isExternal[i])
#endif
            Service_AddReferences_single(server, session, &request->referencesToAdd[i]);
	}
}

/****************/
/* Delete Nodes */
/****************/

// TODO: Check consistency constraints, remove the references.

UA_StatusCode Service_DeleteNodes_single(UA_Server *server, UA_Session *session, UA_NodeId nodeId,
                                         UA_Boolean deleteReferences) {
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if(deleteReferences == UA_TRUE) {
        UA_DeleteReferencesItem delItem;
        UA_DeleteReferencesItem_init(&delItem);
        delItem.deleteBidirectional = UA_FALSE;
        UA_NodeId_copy(&nodeId, &delItem.targetNodeId.nodeId);
        for(int i = 0; i < node->referencesSize; i++) {
            delItem.sourceNodeId = node->references[i].targetId.nodeId;
            delItem.isForward = node->references[i].isInverse;
            Service_DeleteReferences_single(server, session, &delItem);
        }
        UA_NodeId_init(&delItem.sourceNodeId);
        UA_DeleteReferencesItem_deleteMembers(&delItem);
    }
    UA_NodeStore_release(node);
    return UA_NodeStore_remove(server->nodestore, &nodeId);
}

void Service_DeleteNodes(UA_Server *server, UA_Session *session, const UA_DeleteNodesRequest *request,
                         UA_DeleteNodesResponse *response) {
    if(request->nodesToDeleteSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }
    response->results = UA_malloc(sizeof(UA_StatusCode) * request->nodesToDeleteSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;;
        return;
    }
    response->resultsSize = request->nodesToDeleteSize;
    for(int i=0; i<request->nodesToDeleteSize; i++) {
        UA_DeleteNodesItem *item = &request->nodesToDelete[i];
        response->results[i] = Service_DeleteNodes_single(server, session, item->nodeId, item->deleteTargetReferences);
    }
}

/*********************/
/* Delete References */
/*********************/

static UA_StatusCode
deleteOneWayReference(UA_Server *server, UA_Session *session, UA_Node *node, const UA_DeleteReferencesItem *item) {
    UA_Boolean edited = UA_FALSE;
    for(UA_Int32 i = node->referencesSize - 1; i >= 0; i--) {
        if(!UA_NodeId_equal(&item->targetNodeId.nodeId, &node->references[i].targetId.nodeId))
            continue;
        if(!UA_NodeId_equal(&item->referenceTypeId, &node->references[i].referenceTypeId))
            continue;
        if(item->isForward == node->references[i].isInverse)
            continue;
        /* move the last entry to override the current position */
        UA_ReferenceNode_deleteMembers(&node->references[i]);
        node->references[i] = node->references[node->referencesSize-1];
        node->referencesSize--;
        edited = UA_TRUE;
    }
    if(!edited)
        return UA_STATUSCODE_UNCERTAINREFERENCENOTDELETED;
    /* we removed the last reference */
    if(node->referencesSize <= 0 && node->references)
        UA_free(node->references);
    return UA_STATUSCODE_GOOD;;
}

UA_StatusCode
Service_DeleteReferences_single(UA_Server *server, UA_Session *session, const UA_DeleteReferencesItem *item) {
    UA_StatusCode retval = UA_Server_editNode(server, session, &item->sourceNodeId,
                                       (UA_EditNodeCallback)deleteOneWayReference, item);
    if(!item->deleteBidirectional || item->targetNodeId.serverIndex != 0)
        return retval;
    UA_DeleteReferencesItem secondItem;
    UA_DeleteReferencesItem_init(&secondItem);
    secondItem.isForward = !item->isForward;
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    return UA_Server_editNode(server, session, &secondItem.sourceNodeId,
                              (UA_EditNodeCallback)deleteOneWayReference, &secondItem);
}

void Service_DeleteReferences(UA_Server *server, UA_Session *session, const UA_DeleteReferencesRequest *request,
                              UA_DeleteReferencesResponse *response) {
    if(request->referencesToDeleteSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }
    response->results = UA_malloc(sizeof(UA_StatusCode) * request->referencesToDeleteSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;;
        return;
    }
    response->resultsSize = request->referencesToDeleteSize;
    for(int i=0; i<request->referencesToDeleteSize; i++)
        response->results[i] = Service_DeleteReferences_single(server, session, &request->referencesToDelete[i]);
}
