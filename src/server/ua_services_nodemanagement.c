#include "ua_util.h"
#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_session.h"
#include "ua_types_generated_encoding_binary.h"

/**
 * Information Model Consistency
 *
 * The following consistency assertions *always* hold:
 *
 * - There are no directed cycles of hierarchical references
 * - All nodes have a hierarchical relation to at least one father node
 * - Variables and Objects contain all mandatory children according to their type
 *
 * The following consistency assertions *eventually* hold:
 *
 * - All references (except those pointing to external servers with an expandednodeid) are two-way
 *   (present in the source and the target node)
 * - The target of all references exists in the information model
 *
 */

// TOOD: Ensure that the consistency guarantuees hold until v0.2

/************/
/* Add Node */
/************/

void Service_AddNodes_single(UA_Server *server, UA_Session *session, UA_Node *node,
                             const UA_ExpandedNodeId *parentNodeId,
                             const UA_NodeId *referenceTypeId, UA_AddNodesResult *result) {
    if(node->nodeId.namespaceIndex >= server->namespacesSize) {
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return;
    }

    const UA_Node *parent = UA_NodeStore_get(server->nodestore, &parentNodeId->nodeId);
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
    //FIXME: a bit dirty workaround of preserving namespace
    //namespace index is assumed to be valid
    const UA_Node *managed = UA_NULL;
    UA_NodeId tempNodeid;
    UA_NodeId_init(&tempNodeid);
    UA_NodeId_copy(&node->nodeId, &tempNodeid);
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
            result->statusCode = UA_STATUSCODE_BADNODEIDEXISTS;  // todo: differentiate out of memory
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
    UA_NodeId_deleteMembers(&tempNodeid);
    UA_NodeStore_release((const UA_Node*)referenceType);
 ret:
    UA_NodeStore_release(parent);
}

static void moveStandardAttributes(UA_Node *node, UA_AddNodesItem *item, UA_NodeAttributes *attr) {
    node->nodeId = item->requestedNewNodeId.nodeId;
    UA_NodeId_init(&item->requestedNewNodeId.nodeId);

    node->browseName = item->browseName;
    UA_QualifiedName_deleteMembers(&item->browseName);

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

    Service_AddNodes_single(server, session, (UA_Node*)vnode, &item->parentNodeId, &item->referenceTypeId, result);
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

    Service_AddNodes_single(server, session, (UA_Node*)onode, &item->parentNodeId, &item->referenceTypeId, result);
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

    Service_AddNodes_single(server, session, (UA_Node*)rtnode, &item->parentNodeId, &item->referenceTypeId, result);
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

    Service_AddNodes_single(server, session, (UA_Node*)otnode, &item->parentNodeId, &item->referenceTypeId, result);
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

    Service_AddNodes_single(server, session, (UA_Node*)vtnode, &item->parentNodeId, &item->referenceTypeId, result);
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

    Service_AddNodes_single(server, session, (UA_Node*)vnode, &item->parentNodeId, &item->referenceTypeId, result);
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

    Service_AddNodes_single(server, session, (UA_Node*)dtnode, &item->parentNodeId, &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_DataTypeNode_delete(dtnode);
}

void Service_AddNodes_single_fromAttributes(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
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
        Service_AddNodes_single_fromVariableTypeAttributes(server, session, item, (UA_VariableTypeAttributes*)attr, result);
        break;
    case UA_TYPES_REFERENCETYPEATTRIBUTES:
        Service_AddNodes_single_fromReferenceTypeAttributes(server, session, item, (UA_ReferenceTypeAttributes*)attr, result);
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

    Service_AddNodes_single_fromAttributes(server, session, item, attr, attributeType, result);
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
UA_Server_addDataSourceVariableNode(UA_Server *server, const UA_ExpandedNodeId requestedNewNodeId,
                                    const UA_ExpandedNodeId parentNodeId, const UA_NodeId referenceTypeId,
                                    const UA_QualifiedName browseName, const UA_ExpandedNodeId typeDefinition,
                                    const UA_VariableAttributes attr, const UA_DataSource dataSource) {
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);

    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    result.statusCode = UA_QualifiedName_copy(&browseName, &item.browseName);
    item.nodeClass = UA_NODECLASS_METHOD;
    result.statusCode |= UA_ExpandedNodeId_copy(&parentNodeId, &item.parentNodeId);
    result.statusCode |= UA_NodeId_copy(&referenceTypeId, &item.referenceTypeId);
    result.statusCode |= UA_ExpandedNodeId_copy(&requestedNewNodeId, &item.requestedNewNodeId);
    result.statusCode |= UA_ExpandedNodeId_copy(&typeDefinition, &item.typeDefinition);
    
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

    Service_AddNodes_single(server, &adminSession, (UA_Node*)node, &item.parentNodeId,
                            &item.referenceTypeId, &result);
    UA_AddNodesItem_deleteMembers(&item);
    UA_VariableAttributes_deleteMembers(&attrCopy);

    if(result.statusCode != UA_STATUSCODE_GOOD)
        UA_VariableNode_delete(node);
    return result;
}

#ifdef ENABLE_METHODCALLS
UA_AddNodesResult UA_EXPORT
UA_Server_addMethodNode(UA_Server *server, const UA_ExpandedNodeId requestedNewNodeId,
                        const UA_ExpandedNodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_NodeAttributes attr,
                        UA_MethodCallback method, void *handle,
                        UA_Int32 inputArgumentsSize, const UA_Argument* inputArguments, 
                        UA_Int32 outputArgumentsSize, const UA_Argument* outputArguments) {
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    result.statusCode = UA_QualifiedName_copy(&browseName, &item.browseName);
    item.nodeClass = UA_NODECLASS_METHOD;
    result.statusCode |= UA_ExpandedNodeId_copy(&parentNodeId, &item.parentNodeId);
    result.statusCode |= UA_NodeId_copy(&referenceTypeId, &item.referenceTypeId);
    result.statusCode |= UA_ExpandedNodeId_copy(&requestedNewNodeId, &item.requestedNewNodeId);
    
    UA_NodeAttributes attrCopy;
    result.statusCode |= UA_NodeAttributes_copy(&attr, &attrCopy);
    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_AddNodesItem_deleteMembers(&item);
        UA_NodeAttributes_deleteMembers(&attrCopy);
        return result;
    }

    UA_MethodNode *node = UA_MethodNode_new();
    if(!node) {
        result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_AddNodesItem_deleteMembers(&item);
        UA_NodeAttributes_deleteMembers(&attrCopy);
        return result;
    }
    
    moveStandardAttributes((UA_Node*)node, &item, &attrCopy);
    node->executable = UA_TRUE;
    node->userExecutable = UA_TRUE;
    UA_AddNodesItem_deleteMembers(&item);
    UA_NodeAttributes_deleteMembers(&attrCopy);

    Service_AddNodes_single(server, &adminSession, (UA_Node*)node, &item.parentNodeId,
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
    Service_AddNodes_single(server, &adminSession, (UA_Node*)inputArgumentsVariableNode, &parent,
                            &UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), &inputAddRes);
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
    Service_AddNodes_single(server, &adminSession, (UA_Node*)inputArgumentsVariableNode, &parent,
                            &UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), &outputAddRes);
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
    UA_Node *newNode = UA_Node_copyAnyNodeClass(node);
    if(!newNode) {
        UA_NodeStore_release(node);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Int32 count = node->referencesSize;
    if(count < 0)
        count = 0;
    UA_ReferenceNode *old_refs = newNode->references;
    UA_ReferenceNode *new_refs = UA_malloc(sizeof(UA_ReferenceNode)*(count+1));
    if(!new_refs) {
        UA_Node_deleteAnyNodeClass(newNode);
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
        UA_Node_deleteAnyNodeClass(newNode);
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
        UA_Node_deleteAnyNodeClass(newNode);
		return addOneWayReferenceWithSession(server, session, item);
	}
	return retval;
#endif
}

UA_StatusCode Service_AddReferences_single(UA_Server *server, UA_Session *session,
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
	UA_memset(response->results, UA_STATUSCODE_GOOD, sizeof(UA_StatusCode) * size);

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
  const UA_Node *delNode = UA_NodeStore_get(server->nodestore, &nodeId);
  if (!delNode)
    return UA_STATUSCODE_BADNODEIDINVALID;
  
  // Find and remove all References to this node if so requested.
  if(deleteReferences == UA_TRUE) {
    UA_DeleteReferencesItem *delItem = UA_DeleteReferencesItem_new();
    delItem->deleteBidirectional = UA_TRUE; // WARNING: Current semantics in deleteOneWayReference is 'delete forward or inverse'
    UA_NodeId_copy(&nodeId, &delItem->targetNodeId.nodeId);
    
    for(int i=0; i<delNode->referencesSize; i++) {
        UA_NodeId_copy(&delNode->references[i].targetId.nodeId, &delItem->sourceNodeId);
        UA_NodeId_deleteMembers(&delItem->sourceNodeId);
    }
    UA_DeleteReferencesItem_delete(delItem);
  }
  
  UA_NodeStore_release(delNode);
  return UA_NodeStore_remove(server->nodestore, &nodeId);
}

void Service_DeleteNodes(UA_Server *server, UA_Session *session, const UA_DeleteNodesRequest *request,
                         UA_DeleteNodesResponse *response) {
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

// TODO: Add to service

static UA_StatusCode
deleteOneWayReference(UA_Server *server, UA_Session *session, const UA_DeleteReferencesItem *item) {
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


void Service_DeleteReferences(UA_Server *server, UA_Session *session, const UA_DeleteReferencesRequest *request,
                              UA_DeleteReferencesResponse *response) {
    UA_StatusCode retval = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
    response->responseHeader.serviceResult = retval;
}
