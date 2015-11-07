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

void
UA_Server_addExistingNode(UA_Server *server, UA_Session *session, UA_Node *node,
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
        return;
    }

    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        return;
    }

    if(referenceType->isAbstract == UA_TRUE) {
        result->statusCode = UA_STATUSCODE_BADREFERENCENOTALLOWED;
        return;
    }

    // todo: test if the referencetype is hierarchical
    // todo: namespace index is assumed to be valid
    UA_MT_CONST UA_Node *managed = NULL;
    UA_NodeId tempNodeid = node->nodeId;
    tempNodeid.namespaceIndex = 0;
    if(UA_NodeId_isNull(&tempNodeid)) {
        if(UA_NodeStore_insert(server->nodestore, node, &managed) != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        result->addedNodeId = managed->nodeId; // cannot fail as unique nodeids are numeric
    } else {
        if(UA_NodeId_copy(&node->nodeId, &result->addedNodeId) != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }

        if(UA_NodeStore_insert(server->nodestore, node, &managed) != UA_STATUSCODE_GOOD) {
            result->statusCode = UA_STATUSCODE_BADNODEIDEXISTS;
            UA_NodeId_deleteMembers(&result->addedNodeId);
            return;
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
}

static UA_StatusCode
instantiateVariableNode(UA_Server *server, UA_Session *session,
                        const UA_NodeId *nodeId, const UA_NodeId *typeId);
static UA_StatusCode
instantiateObjectNode(UA_Server *server, UA_Session *session,
                      const UA_NodeId *nodeId, const UA_NodeId *typeId);

/* copy an existing variable under the given parent. then instantiate the
   variable for all hastypedefinitions of the original version. */
static UA_StatusCode
copyExistingVariable(UA_Server *server, UA_Session *session, const UA_NodeId *variable,
                     const UA_NodeId *referenceType, const UA_NodeId *parent) {
    const UA_VariableNode *node = (const UA_VariableNode*)UA_NodeStore_get(server->nodestore, variable);
    if(!node)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if(node->nodeClass != UA_NODECLASS_VARIABLE)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    
    // copy the variable attributes
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_LocalizedText_copy(&node->displayName, &attr.displayName);
    UA_LocalizedText_copy(&node->description, &attr.description);
    attr.writeMask = node->writeMask;
    attr.userWriteMask = node->userWriteMask;
    // todo: handle data sources!!!!
    UA_Variant_copy(&node->value.variant.value, &attr.value);
    // datatype is taken from the value
    // valuerank is taken from the value
    // array dimensions are taken from the value
    attr.accessLevel = node->accessLevel;
    attr.userAccessLevel = node->userAccessLevel;
    attr.minimumSamplingInterval = node->minimumSamplingInterval;
    attr.historizing = node->historizing;

    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    UA_NodeId_copy(parent, &item.parentNodeId.nodeId);
    UA_NodeId_copy(referenceType, &item.referenceTypeId);
    UA_QualifiedName_copy(&node->browseName, &item.browseName);
    item.nodeClass = UA_NODECLASS_VARIABLE;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type = &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES];
    item.nodeAttributes.content.decoded.data = &attr;
    // don't add a typedefinition here.

    // add the new variable
    UA_AddNodesResult res;
    UA_AddNodesResult_init(&res);
    Service_AddNodes_single(server, session, &item, &res);
    UA_VariableAttributes_deleteMembers(&attr);
    UA_AddNodesItem_deleteMembers(&item);

    // now instantiate the variable for all hastypedefinition references
    for(size_t i = 0; i < node->referencesSize; i++) {
        UA_ReferenceNode *rn = &node->references[i];
        if(rn->isInverse)
            continue;
        const UA_NodeId hasTypeDef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
        if(!UA_NodeId_equal(&rn->referenceTypeId, &hasTypeDef))
            continue;
        instantiateVariableNode(server, session, &res.addedNodeId, &rn->targetId.nodeId);
    }

    UA_AddNodesResult_deleteMembers(&res);
    return UA_STATUSCODE_GOOD;
}

/* copy an existing object under the given parent. then instantiate the
   variable for all hastypedefinitions of the original version. */
static UA_StatusCode
copyExistingObject(UA_Server *server, UA_Session *session, const UA_NodeId *variable,
                   const UA_NodeId *referenceType, const UA_NodeId *parent) {
    const UA_ObjectNode *node = (const UA_ObjectNode*)UA_NodeStore_get(server->nodestore, variable);
    if(!node)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if(node->nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_BADNODECLASSINVALID;

    // copy the variable attributes
    UA_ObjectAttributes attr;
    UA_ObjectAttributes_init(&attr);
    UA_LocalizedText_copy(&node->displayName, &attr.displayName);
    UA_LocalizedText_copy(&node->description, &attr.description);
    attr.writeMask = node->writeMask;
    attr.userWriteMask = node->userWriteMask;
    attr.eventNotifier = node->eventNotifier;

    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    UA_NodeId_copy(parent, &item.parentNodeId.nodeId);
    UA_NodeId_copy(referenceType, &item.referenceTypeId);
    UA_QualifiedName_copy(&node->browseName, &item.browseName);
    item.nodeClass = UA_NODECLASS_OBJECT;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type = &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES];
    item.nodeAttributes.content.decoded.data = &attr;
    // don't add a typedefinition here.

    // add the new object
    UA_AddNodesResult res;
    UA_AddNodesResult_init(&res);
    Service_AddNodes_single(server, session, &item, &res);
    UA_ObjectAttributes_deleteMembers(&attr);
    UA_AddNodesItem_deleteMembers(&item);

    // now instantiate the object for all hastypedefinition references
    for(size_t i = 0; i < node->referencesSize; i++) {
        UA_ReferenceNode *rn = &node->references[i];
        if(rn->isInverse)
            continue;
        const UA_NodeId hasTypeDef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
        if(!UA_NodeId_equal(&rn->referenceTypeId, &hasTypeDef))
            continue;
        instantiateObjectNode(server, session, &res.addedNodeId, &rn->targetId.nodeId);
    }

    UA_AddNodesResult_deleteMembers(&res);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setObjectInstanceHandle(UA_Server *server, UA_Session *session, UA_ObjectNode* node, void *handle) {
    if(node->nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    node->instanceHandle = handle;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
instantiateObjectNode(UA_Server *server, UA_Session *session,
                      const UA_NodeId *nodeId, const UA_NodeId *typeId) {
    const UA_ObjectTypeNode *type = (const UA_ObjectTypeNode*)UA_NodeStore_get(server->nodestore, typeId);
    if(!type)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if(type->nodeClass != UA_NODECLASS_OBJECTTYPE)
        return UA_STATUSCODE_BADNODECLASSINVALID;

    /* Add all the child nodes */
    UA_BrowseDescription browseChildren;
    UA_BrowseDescription_init(&browseChildren);
    browseChildren.nodeId = *typeId;
    browseChildren.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES);
    browseChildren.includeSubtypes = UA_TRUE;
    browseChildren.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    browseChildren.nodeClassMask = UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE | UA_NODECLASS_METHOD;
    browseChildren.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID | UA_BROWSERESULTMASK_NODECLASS;

    UA_BrowseResult browseResult;
    UA_BrowseResult_init(&browseResult);
    // todo: continuation points if there are too many results
    Service_Browse_single(server, session, NULL, &browseChildren, 100, &browseResult);

    for(size_t i = 0; i < browseResult.referencesSize; i++) {
        UA_ReferenceDescription *rd = &browseResult.references[i];
        if(rd->nodeClass == UA_NODECLASS_METHOD) {
            /* add a reference to the method in the objecttype */
            UA_AddReferencesItem item;
            UA_AddReferencesItem_init(&item);
            item.sourceNodeId = *nodeId;
            item.referenceTypeId = rd->referenceTypeId;
            item.isForward = UA_TRUE;
            item.targetNodeId = rd->nodeId;
            item.targetNodeClass = UA_NODECLASS_METHOD;
            Service_AddReferences_single(server, session, &item);
        } else if(rd->nodeClass == UA_NODECLASS_VARIABLE)
            copyExistingVariable(server, session, &rd->nodeId.nodeId, &rd->referenceTypeId, nodeId);
        else if(rd->nodeClass == UA_NODECLASS_OBJECT)
            copyExistingObject(server, session, &rd->nodeId.nodeId, &rd->referenceTypeId, nodeId);
    }

    /* add a hastypedefinition reference */
    UA_AddReferencesItem addref;
    UA_AddReferencesItem_init(&addref);
    addref.sourceNodeId = *nodeId;
    addref.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    addref.isForward = UA_TRUE;
    addref.targetNodeId.nodeId = *typeId;
    addref.targetNodeClass = UA_NODECLASS_OBJECTTYPE;
    Service_AddReferences_single(server, session, &addref);

    /* call the constructor */
    const UA_ObjectLifecycleManagement *olm = &type->lifecycleManagement;
    if(olm->constructor)
        UA_Server_editNode(server, session, nodeId,
                           (UA_EditNodeCallback)setObjectInstanceHandle, olm->constructor(*nodeId));
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
instantiateVariableNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                        const UA_NodeId *typeId) {
    const UA_ObjectTypeNode *type = (const UA_ObjectTypeNode*)UA_NodeStore_get(server->nodestore, typeId);
    if(!type)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if(type->nodeClass != UA_NODECLASS_VARIABLETYPE)
        return UA_STATUSCODE_BADNODECLASSINVALID;

    /* get the references to child properties */
    UA_BrowseDescription browseChildren;
    UA_BrowseDescription_init(&browseChildren);
    browseChildren.nodeId = *typeId;
    browseChildren.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    browseChildren.includeSubtypes = UA_TRUE;
    browseChildren.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    browseChildren.nodeClassMask = UA_NODECLASS_VARIABLE;
    browseChildren.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID | UA_BROWSERESULTMASK_NODECLASS;

    UA_BrowseResult browseResult;
    UA_BrowseResult_init(&browseResult);
    // todo: continuation points if there are too many results
    Service_Browse_single(server, session, NULL, &browseChildren, 100, &browseResult);

    /* add the child properties */
    for(size_t i = 0; i < browseResult.referencesSize; i++) {
        UA_ReferenceDescription *rd = &browseResult.references[i];
        copyExistingVariable(server, session, &rd->nodeId.nodeId, &rd->referenceTypeId, nodeId);
    }

    /* add a hastypedefinition reference */
    UA_AddReferencesItem addref;
    UA_AddReferencesItem_init(&addref);
    addref.sourceNodeId = *nodeId;
    addref.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    addref.isForward = UA_TRUE;
    addref.targetNodeId.nodeId = *typeId;
    addref.targetNodeClass = UA_NODECLASS_OBJECTTYPE;
    Service_AddReferences_single(server, session, &addref);

    return UA_STATUSCODE_GOOD;
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

static UA_Node *
variableNodeFromAttributes(UA_AddNodesItem *item, UA_VariableAttributes *attr) {
    UA_VariableNode *vnode = UA_VariableNode_new();
    if(!vnode)
        return NULL;
    moveStandardAttributes((UA_Node*)vnode, item, (UA_NodeAttributes*)attr);
    // todo: test if the type / valueRank / value attributes are consistent
    vnode->accessLevel = attr->accessLevel;
    vnode->userAccessLevel = attr->userAccessLevel;
    vnode->historizing = attr->historizing;
    vnode->minimumSamplingInterval = attr->minimumSamplingInterval;
    vnode->valueRank = attr->valueRank;
    vnode->value.variant.value = attr->value;
    UA_Variant_init(&attr->value);
    return (UA_Node*)vnode;
}

static UA_Node *
objectNodeFromAttributes(UA_AddNodesItem *item, UA_ObjectAttributes *attr) {
    UA_ObjectNode *onode = UA_ObjectNode_new();
    if(!onode)
        return NULL;
    moveStandardAttributes((UA_Node*)onode, item, (UA_NodeAttributes*)attr);
    onode->eventNotifier = attr->eventNotifier;
    return (UA_Node*)onode;
}

static UA_Node *
referenceTypeNodeFromAttributes(UA_AddNodesItem *item, UA_ReferenceTypeAttributes *attr) {
    UA_ReferenceTypeNode *rtnode = UA_ReferenceTypeNode_new();
    if(!rtnode)
        return NULL;
    moveStandardAttributes((UA_Node*)rtnode, item, (UA_NodeAttributes*)attr);
    rtnode->isAbstract = attr->isAbstract;
    rtnode->symmetric = attr->symmetric;
    rtnode->inverseName = attr->inverseName;
    UA_LocalizedText_init(&attr->inverseName);
    return (UA_Node*)rtnode;
}

static UA_Node *
objectTypeNodeFromAttributes(UA_AddNodesItem *item, UA_ObjectTypeAttributes *attr) {
    UA_ObjectTypeNode *otnode = UA_ObjectTypeNode_new();
    if(!otnode)
        return NULL;
    moveStandardAttributes((UA_Node*)otnode, item, (UA_NodeAttributes*)attr);
    otnode->isAbstract = attr->isAbstract;
    return (UA_Node*)otnode;
}

static UA_Node *
variableTypeNodeFromAttributes(UA_AddNodesItem *item, UA_VariableTypeAttributes *attr) {
    UA_VariableTypeNode *vtnode = UA_VariableTypeNode_new();
    if(!vtnode)
        return NULL;
    moveStandardAttributes((UA_Node*)vtnode, item, (UA_NodeAttributes*)attr);
    vtnode->value.variant.value = attr->value;
    UA_Variant_init(&attr->value);
    // datatype is taken from the value
    vtnode->valueRank = attr->valueRank;
    // array dimensions are taken from the value
    vtnode->isAbstract = attr->isAbstract;
    return (UA_Node*)vtnode;
}

static UA_Node *
viewNodeFromAttributes(UA_AddNodesItem *item, UA_ViewAttributes *attr) {
    UA_ViewNode *vnode = UA_ViewNode_new();
    if(!vnode)
        return NULL;
    moveStandardAttributes((UA_Node*)vnode, item, (UA_NodeAttributes*)attr);
    vnode->containsNoLoops = attr->containsNoLoops;
    vnode->eventNotifier = attr->eventNotifier;
    return (UA_Node*)vnode;
}

static UA_Node *
dataTypeNodeFromAttributes(UA_AddNodesItem *item, UA_DataTypeAttributes *attr) {
    UA_DataTypeNode *dtnode = UA_DataTypeNode_new();
    if(!dtnode)
        return NULL;
    moveStandardAttributes((UA_Node*)dtnode, item, (UA_NodeAttributes*)attr);
    dtnode->isAbstract = attr->isAbstract;
    return (UA_Node*)dtnode;
}

void Service_AddNodes_single(UA_Server *server, UA_Session *session, UA_AddNodesItem *item,
                             UA_AddNodesResult *result) {
    if(item->nodeAttributes.encoding < UA_EXTENSIONOBJECT_DECODED ||
       !item->nodeAttributes.content.decoded.type) {
        result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
        return;
    }
    
    /* create the node */
    UA_Node *node;
    switch(item->nodeClass) {
    case UA_NODECLASS_OBJECT:
        if(item->nodeAttributes.content.decoded.type != &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES]) {
            result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
            return;
        }
        node = objectNodeFromAttributes(item, item->nodeAttributes.content.decoded.data);
        break;
    case UA_NODECLASS_VARIABLE:
        if(item->nodeAttributes.content.decoded.type != &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES]) {
            result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
            return;
        }
        node = variableNodeFromAttributes(item, item->nodeAttributes.content.decoded.data);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        if(item->nodeAttributes.content.decoded.type != &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES]) {
            result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
            return;
        }
        node = objectTypeNodeFromAttributes(item, item->nodeAttributes.content.decoded.data);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        if(item->nodeAttributes.content.decoded.type != &UA_TYPES[UA_TYPES_OBJECTTYPEATTRIBUTES]) {
            result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
            return;
        }
        node = variableTypeNodeFromAttributes(item, item->nodeAttributes.content.decoded.data);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        if(item->nodeAttributes.content.decoded.type != &UA_TYPES[UA_TYPES_REFERENCETYPEATTRIBUTES]) {
            result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
            return;
        }
        node = referenceTypeNodeFromAttributes(item, item->nodeAttributes.content.decoded.data);
        break;
    case UA_NODECLASS_DATATYPE:
        if(item->nodeAttributes.content.decoded.type != &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES]) {
            result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
            return;
        }
        node = dataTypeNodeFromAttributes(item, item->nodeAttributes.content.decoded.data);
        break;
    case UA_NODECLASS_VIEW:
        if(item->nodeAttributes.content.decoded.type != &UA_TYPES[UA_TYPES_VIEWATTRIBUTES]) {
            result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
            return;
        }
        node = viewNodeFromAttributes(item, item->nodeAttributes.content.decoded.data);
        break;
    case UA_NODECLASS_METHOD:
    case UA_NODECLASS_UNSPECIFIED:
    default:
        result->statusCode = UA_STATUSCODE_BADNODECLASSINVALID;
        return;
    }

    if(!node) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* add it to the server */
    UA_Server_addExistingNode(server, session, node, &item->parentNodeId.nodeId,
                              &item->referenceTypeId, result);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_Node_deleteAnyNodeClass(node);
        return;
    }

    /* instantiate if it has a type */
    if(!UA_NodeId_isNull(&item->typeDefinition.nodeId)) {
        if(item->nodeClass == UA_NODECLASS_OBJECT)
            result->statusCode = instantiateObjectNode(server, session, &result->addedNodeId,
                                                       &item->typeDefinition.nodeId);
        else if(item->nodeClass == UA_NODECLASS_VARIABLE)
            result->statusCode = instantiateVariableNode(server, session, &result->addedNodeId,
                                                         &item->typeDefinition.nodeId);
    }

    /* if instantiation failed, remove the node */
    if(result->statusCode != UA_STATUSCODE_GOOD)
        Service_DeleteNodes_single(server, session, &result->addedNodeId, UA_TRUE);
}

void Service_AddNodes(UA_Server *server, UA_Session *session, const UA_AddNodesRequest *request,
                      UA_AddNodesResponse *response) {
    if(request->nodesToAddSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }
    size_t size = request->nodesToAddSize;

    response->results = UA_Array_new(size, &UA_TYPES[UA_TYPES_ADDNODESRESULT]);
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
    memset(isExternal, UA_FALSE, sizeof(UA_Boolean) * size);
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
    for(size_t i = 0; i < size; i++) {
#ifdef UA_EXTERNAL_NAMESPACES
        if(!isExternal[i])
#endif
            Service_AddNodes_single(server, session, &request->nodesToAdd[i], &response->results[i]);
    }
}

/**************************************************/
/* Add Special Nodes (not possible over the wire) */
/**************************************************/

UA_StatusCode UA_EXPORT
UA_Server_addDataSourceVariableNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                    const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
                                    const UA_VariableAttributes attr, const UA_DataSource dataSource, UA_NodeId *outNewNodeId) {
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
        return result.statusCode;
    }

    UA_VariableNode *node = UA_VariableNode_new();
    if(!node) {
        UA_AddNodesItem_deleteMembers(&item);
        UA_VariableAttributes_deleteMembers(&attrCopy);
        return UA_STATUSCODE_BADOUTOFMEMORY;
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

    if(outNewNodeId && result.statusCode == UA_STATUSCODE_GOOD)
        *outNewNodeId = result.addedNodeId;
    else
        UA_AddNodesResult_deleteMembers(&result);
    return result.statusCode;
}

#ifdef ENABLE_METHODCALLS
UA_StatusCode UA_EXPORT
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                        UA_MethodCallback method, void *handle,
                        UA_Int32 inputArgumentsSize, const UA_Argument* inputArguments, 
                        UA_Int32 outputArgumentsSize, const UA_Argument* outputArguments,
                        UA_NodeId *outNewNodeId) {
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
        return result.statusCode;
    }

    UA_MethodNode *node = UA_MethodNode_new();
    if(!node) {
        result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_AddNodesItem_deleteMembers(&item);
        UA_MethodAttributes_deleteMembers(&attrCopy);
        return result.statusCode;
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
    if(result.statusCode != UA_STATUSCODE_GOOD)
        UA_MethodNode_delete(node);
    
    if(result.statusCode == UA_STATUSCODE_GOOD && method != NULL) {
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
    }

    if(outNewNodeId && result.statusCode == UA_STATUSCODE_GOOD)
        *outNewNodeId = result.addedNodeId;
    else
        UA_AddNodesResult_deleteMembers(&result);
    return result.statusCode;
}
#endif

/******************/
/* Add References */
/******************/

/* Adds a one-way reference to the local nodestore */
static UA_StatusCode
addOneWayReference(UA_Server *server, UA_Session *session, UA_Node *node, const UA_AddReferencesItem *item) {
	size_t i = node->referencesSize;
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
    memset(isExternal, UA_FALSE, sizeof(UA_Boolean) * size);
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
	for(size_t i = 0; i < response->resultsSize; i++) {
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

UA_StatusCode Service_DeleteNodes_single(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                                         UA_Boolean deleteReferences) {
    UA_MT_CONST UA_Node *node = UA_NodeStore_get(server->nodestore, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if(deleteReferences == UA_TRUE) {
        UA_DeleteReferencesItem delItem;
        UA_DeleteReferencesItem_init(&delItem);
        delItem.deleteBidirectional = UA_FALSE;
        delItem.targetNodeId.nodeId = *nodeId;
        for(size_t i = 0; i < node->referencesSize; i++) {
            delItem.sourceNodeId = node->references[i].targetId.nodeId;
            delItem.isForward = node->references[i].isInverse;
            Service_DeleteReferences_single(server, session, &delItem);
        }
    }

    /* destroy an object before removing it */
    if(node->nodeClass == UA_NODECLASS_OBJECT) {
        /* find the object type(s) */
        UA_BrowseDescription bd;
        UA_BrowseDescription_init(&bd);
        bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
        bd.nodeId = *nodeId;
        bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
        bd.includeSubtypes = UA_TRUE;
        bd.nodeClassMask = UA_NODECLASS_OBJECTTYPE;
        
        /* browse type definitions with admin rights */
        UA_BrowseResult result;
        UA_BrowseResult_init(&result);
        Service_Browse_single(server, &adminSession, NULL, &bd, UA_UINT32_MAX, &result);
        for(size_t i = 0; i < result.referencesSize; i++) {
            /* call the destructor */
            UA_ReferenceDescription *rd = &result.references[i];
            const UA_ObjectTypeNode *type =
                (const UA_ObjectTypeNode*)UA_NodeStore_get(server->nodestore, &rd->nodeId.nodeId);
            if(!type)
                continue;
            if(type->nodeClass != UA_NODECLASS_OBJECTTYPE || !type->lifecycleManagement.destructor)
                continue;

            /* if there are several types with lifecycle management, call all the destructors */
            type->lifecycleManagement.destructor(*nodeId, ((const UA_ObjectNode*)node)->instanceHandle);
        }
        UA_BrowseResult_deleteMembers(&result);
    }
    
    return UA_NodeStore_remove(server->nodestore, nodeId);
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
    for(size_t i=0; i<request->nodesToDeleteSize; i++) {
        UA_DeleteNodesItem *item = &request->nodesToDelete[i];
        response->results[i] = Service_DeleteNodes_single(server, session, &item->nodeId,
                                                          item->deleteTargetReferences);
    }
}

/*********************/
/* Delete References */
/*********************/

static UA_StatusCode
deleteOneWayReference(UA_Server *server, UA_Session *session, UA_Node *node,
                      const UA_DeleteReferencesItem *item) {
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
    if(node->referencesSize == 0 && node->references)
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

void Service_DeleteReferences(UA_Server *server, UA_Session *session,
                              const UA_DeleteReferencesRequest *request,
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
    for(size_t i = 0; i < request->referencesToDeleteSize; i++)
        response->results[i] = Service_DeleteReferences_single(server, session,
                                                               &request->referencesToDelete[i]);
}
