#include "ua_server_internal.h"
#include "ua_services.h"

/********************/
/* Helper Functions */
/********************/

/* Returns the type and all subtypes. We start with an array with a single root nodeid. When a relevant
 * reference is found, we add the nodeids to the back of the array and increase the size. Since the hierarchy
 * is not cyclic, we can safely progress in the array to process the newly found referencetype nodeids. */
UA_StatusCode
getTypeHierarchy(UA_NodeStore *ns, const UA_NodeId *root, UA_NodeId **typeHierarchy, size_t *typeHierarchySize) {
    const UA_Node *node = UA_NodeStore_get(ns, root);
    if(!node)
        return UA_STATUSCODE_BADNOMATCH;
    if(node->nodeClass != UA_NODECLASS_REFERENCETYPE)
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;

    size_t results_size = 20; // probably too big, but saves mallocs
    UA_NodeId *results = UA_malloc(sizeof(UA_NodeId) * results_size);
    if(!results)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval = UA_NodeId_copy(root, &results[0]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(results);
        return retval;
    }

    size_t idx = 0; // where are we currently in the array?
    size_t last = 0; // where is the last element in the array?
    const UA_NodeId hasSubtypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    do {
        node = UA_NodeStore_get(ns, &results[idx]);
        if(!node || node->nodeClass != UA_NODECLASS_REFERENCETYPE)
            continue;
        for(size_t i = 0; i < node->referencesSize; i++) {
            if(node->references[i].isInverse == true ||
               !UA_NodeId_equal(&hasSubtypeNodeId, &node->references[i].referenceTypeId))
                continue;

            if(++last >= results_size) { // is the array big enough?
                UA_NodeId *new_results = UA_realloc(results, sizeof(UA_NodeId) * results_size * 2);
                if(!new_results) {
                    retval = UA_STATUSCODE_BADOUTOFMEMORY;
                    break;
                }
                results = new_results;
                results_size *= 2;
            }

            retval = UA_NodeId_copy(&node->references[i].targetId.nodeId, &results[last]);
            if(retval != UA_STATUSCODE_GOOD) {
                last--; // for array_delete
                break;
            }
        }
    } while(++idx <= last && retval == UA_STATUSCODE_GOOD);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(results, last, &UA_TYPES[UA_TYPES_NODEID]);
        return retval;
    }

    *typeHierarchy = results;
    *typeHierarchySize = last + 1;
    return UA_STATUSCODE_GOOD;
}

/* Recursively searches "upwards" in the tree following specific reference types */
UA_StatusCode
isNodeInTree(UA_NodeStore *ns, const UA_NodeId *rootNode, const UA_NodeId *nodeToFind,
             const UA_NodeId *referenceTypeIds, size_t referenceTypeIdsSize,
             size_t maxDepth, UA_Boolean *found) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(UA_NodeId_equal(rootNode, nodeToFind)) {
        *found = true;
        return UA_STATUSCODE_GOOD;
    }

    *found = false;
    const UA_Node *node = UA_NodeStore_get(ns,rootNode);
    if(!node)
        return UA_STATUSCODE_BADINTERNALERROR;

    maxDepth = maxDepth - 1;
    for(size_t i = 0; i < node->referencesSize; i++) {
        /* Search only upwards */
        if(!node->references[i].isInverse)
            continue;

        /* Go up only for some reference types */
        UA_Boolean reftype_found = false;
        for(size_t j = 0; j < referenceTypeIdsSize; j++) {
            if(UA_NodeId_equal(&node->references[i].referenceTypeId, &referenceTypeIds[j])) {
                reftype_found = true;
                break;
            }
        }
        if(!reftype_found)
            continue;

        /* Is the it node we seek? */
        if(UA_NodeId_equal(&node->references[i].targetId.nodeId, nodeToFind)) {
            *found = true;
            return UA_STATUSCODE_GOOD;
        }

        /* Recurse */
        if(maxDepth > 0) {
            retval = isNodeInTree(ns, &node->references[i].targetId.nodeId, nodeToFind,
                                  referenceTypeIds, referenceTypeIdsSize, maxDepth, found);
            if(*found || retval != UA_STATUSCODE_GOOD)
                break;
        }
    }
    return retval;
}

/************/
/* Add Node */
/************/

static UA_StatusCode
instantiateVariableNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                        const UA_NodeId *typeId, UA_InstantiationCallback *instantiationCallback);
static UA_StatusCode
instantiateObjectNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                      const UA_NodeId *typeId, UA_InstantiationCallback *instantiationCallback);

void
Service_AddNodes_existing(UA_Server *server, UA_Session *session, UA_Node *node,
                          const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId,
                          const UA_NodeId *typeDefinition, UA_InstantiationCallback *instantiationCallback,
                          UA_AddNodesResult *result) {
    if(node->nodeId.namespaceIndex >= server->namespacesSize) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Namespace invalid");
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        UA_NodeStore_deleteNode(node);
        return;
    }

    const UA_Node *parent = UA_NodeStore_get(server->nodestore, parentNodeId);
    if(!parent) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Parent node not found");
        result->statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID;
        UA_NodeStore_deleteNode(node);
        return;
    }

    const UA_ReferenceTypeNode *referenceType =
        (const UA_ReferenceTypeNode *)UA_NodeStore_get(server->nodestore, referenceTypeId);
    if(!referenceType) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Reference type to the parent not found");
        result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        UA_NodeStore_deleteNode(node);
        return;
    }
    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Reference type to the parent invalid");
        result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
        UA_NodeStore_deleteNode(node);
        return;
    }
    if(referenceType->isAbstract == true) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Abstract feference type to the parent invalid");
        result->statusCode = UA_STATUSCODE_BADREFERENCENOTALLOWED;
        UA_NodeStore_deleteNode(node);
        return;
    }

    // todo: test if the referencetype is hierarchical
    // todo: namespace index is assumed to be valid
    result->statusCode = UA_NodeStore_insert(server->nodestore, node);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Node could not be added to the nodestore with error code 0x%08x",
                             result->statusCode);
        return;
    }
    result->statusCode = UA_NodeId_copy(&node->nodeId, &result->addedNodeId);

    /* Hierarchical reference back to the parent */
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = node->nodeId;
    item.referenceTypeId = *referenceTypeId;
    item.isForward = false;
    item.targetNodeId.nodeId = *parentNodeId;
    result->statusCode = Service_AddReferences_single(server, session, &item);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Could not add the reference to the parent");
        goto remove_node;
    }

    const UA_NodeId hassubtype = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    if(node->nodeClass == UA_NODECLASS_OBJECT) {
        const UA_NodeId baseobjtype = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE);
        if(!typeDefinition || UA_NodeId_equal(typeDefinition, &UA_NODEID_NULL)) {
            /* Objects must have a type reference. Default to BaseObjectType */
            UA_NodeId *writableTypeDef = UA_alloca(sizeof(UA_NodeId));
            *writableTypeDef = baseobjtype;
            typeDefinition = writableTypeDef;
        } else {
            /* Check if the supplied type is a subtype of BaseObjectType */
            UA_Boolean found = false;
            result->statusCode = isNodeInTree(server->nodestore, typeDefinition, &baseobjtype, &hassubtype, 1, 10, &found);
            if(!found)
                result->statusCode = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            if(result->statusCode != UA_STATUSCODE_GOOD) {
                UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: The object if not derived from BaseObjectType");
                goto remove_node;
            }
        }
        result->statusCode = instantiateObjectNode(server, session, &result->addedNodeId,
                                                   typeDefinition, instantiationCallback);
        if(result->statusCode != UA_STATUSCODE_GOOD)
            goto remove_node;
    } else if(node->nodeClass == UA_NODECLASS_VARIABLE) {
        const UA_NodeId basevartype = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE);
        if(!typeDefinition || UA_NodeId_equal(typeDefinition, &UA_NODEID_NULL)) {
            /* Variables must have a type reference. Default to BaseVariableType */
            UA_NodeId *writableTypeDef = UA_alloca(sizeof(UA_NodeId));
            *writableTypeDef = basevartype;
            typeDefinition = writableTypeDef;
        } else {
            /* Check if the supplied type is a subtype of BaseVariableType */
            UA_Boolean found = false;
            result->statusCode = isNodeInTree(server->nodestore, typeDefinition, &basevartype, &hassubtype, 1, 10, &found);
            if(!found)
                result->statusCode = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            if(result->statusCode != UA_STATUSCODE_GOOD) {
                UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: The variable if not derived from BaseVariableType");
                goto remove_node;
            }
        }
        result->statusCode = instantiateVariableNode(server, session, &result->addedNodeId,
                                                     typeDefinition, instantiationCallback);
        if(result->statusCode != UA_STATUSCODE_GOOD)
            goto remove_node;
    }

    /* Custom callback */
    if(instantiationCallback)
        instantiationCallback->method(result->addedNodeId, *typeDefinition, instantiationCallback->handle);

    return;

 remove_node:
    Service_DeleteNodes_single(server, &adminSession, &result->addedNodeId, true);
    UA_AddNodesResult_deleteMembers(result);
}

/* copy an existing variable under the given parent. then instantiate the
   variable for all hastypedefinitions of the original version. */
static UA_StatusCode
copyExistingVariable(UA_Server *server, UA_Session *session, const UA_NodeId *variable,
                     const UA_NodeId *referenceType, const UA_NodeId *parent,
                     UA_InstantiationCallback *instantiationCallback) {
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
    Service_AddNodes_single(server, session, &item, &res, instantiationCallback);
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
        UA_StatusCode retval = instantiateVariableNode(server, session, &res.addedNodeId, &rn->targetId.nodeId, instantiationCallback);
        if(retval != UA_STATUSCODE_GOOD) {
            Service_DeleteNodes_single(server, &adminSession, &res.addedNodeId, true);
            UA_AddNodesResult_deleteMembers(&res);
            return retval;
        }
    }

    if(instantiationCallback)
        instantiationCallback->method(res.addedNodeId, node->nodeId, instantiationCallback->handle);

    UA_AddNodesResult_deleteMembers(&res);
    return UA_STATUSCODE_GOOD;
}

/* copy an existing object under the given parent. then instantiate the
   variable for all hastypedefinitions of the original version. */
static UA_StatusCode
copyExistingObject(UA_Server *server, UA_Session *session, const UA_NodeId *variable,
                   const UA_NodeId *referenceType, const UA_NodeId *parent,
                   UA_InstantiationCallback *instantiationCallback) {
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
    Service_AddNodes_single(server, session, &item, &res, instantiationCallback);
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
        UA_StatusCode retval = instantiateObjectNode(server, session, &res.addedNodeId, &rn->targetId.nodeId, instantiationCallback);
        if(retval != UA_STATUSCODE_GOOD) {
            Service_DeleteNodes_single(server, &adminSession, &res.addedNodeId, true);
            UA_AddNodesResult_deleteMembers(&res);
            return retval;
        }
    }

    if(instantiationCallback)
        instantiationCallback->method(res.addedNodeId, node->nodeId, instantiationCallback->handle);

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
instantiateObjectNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                      const UA_NodeId *typeId, UA_InstantiationCallback *instantiationCallback) {
    const UA_ObjectTypeNode *typenode = (const UA_ObjectTypeNode*)UA_NodeStore_get(server->nodestore, typeId);
    if(!typenode) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Could not instantiate an object from an unknown nodeid");
        return UA_STATUSCODE_BADNODEIDINVALID;
    }
    if(typenode->nodeClass != UA_NODECLASS_OBJECTTYPE) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Could not instantiate an object from a non-objecttype node");
        return UA_STATUSCODE_BADNODECLASSINVALID;
    }

    /* Add all the child nodes */
    UA_BrowseDescription browseChildren;
    UA_BrowseDescription_init(&browseChildren);
    browseChildren.nodeId = *typeId;
    browseChildren.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES);
    browseChildren.includeSubtypes = true;
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
            item.isForward = true;
            item.targetNodeId = rd->nodeId;
            item.targetNodeClass = UA_NODECLASS_METHOD;
            Service_AddReferences_single(server, session, &item);
        } else if(rd->nodeClass == UA_NODECLASS_VARIABLE)
          copyExistingVariable(server, session, &rd->nodeId.nodeId,
                               &rd->referenceTypeId, nodeId, instantiationCallback);
        else if(rd->nodeClass == UA_NODECLASS_OBJECT)
          copyExistingObject(server, session, &rd->nodeId.nodeId,
                             &rd->referenceTypeId, nodeId, instantiationCallback);
    }

    /* add a hastypedefinition reference */
    UA_AddReferencesItem addref;
    UA_AddReferencesItem_init(&addref);
    addref.sourceNodeId = *nodeId;
    addref.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    addref.isForward = true;
    addref.targetNodeId.nodeId = *typeId;
    addref.targetNodeClass = UA_NODECLASS_OBJECTTYPE;
    Service_AddReferences_single(server, session, &addref);

    /* call the constructor */
    const UA_ObjectLifecycleManagement *olm = &typenode->lifecycleManagement;
    if(olm->constructor)
        UA_Server_editNode(server, session, nodeId,
                           (UA_EditNodeCallback)setObjectInstanceHandle, olm->constructor(*nodeId));
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
instantiateVariableNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                        const UA_NodeId *typeId, UA_InstantiationCallback *instantiationCallback) {
    const UA_ObjectTypeNode *typenode = (const UA_ObjectTypeNode*)UA_NodeStore_get(server->nodestore, typeId);
    if(!typenode) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Could not instantiate a variable from an unknown nodeid");
        return UA_STATUSCODE_BADNODEIDINVALID;
    }
    if(typenode->nodeClass != UA_NODECLASS_VARIABLETYPE) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: Could not instantiate a variable from a non-variabletype node");
        return UA_STATUSCODE_BADNODECLASSINVALID;
    }

    /* get the references to child properties */
    UA_BrowseDescription browseChildren;
    UA_BrowseDescription_init(&browseChildren);
    browseChildren.nodeId = *typeId;
    browseChildren.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    browseChildren.includeSubtypes = true;
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
        copyExistingVariable(server, session, &rd->nodeId.nodeId,
                             &rd->referenceTypeId, nodeId, instantiationCallback);
    }

    /* add a hastypedefinition reference */
    UA_AddReferencesItem addref;
    UA_AddReferencesItem_init(&addref);
    addref.sourceNodeId = *nodeId;
    addref.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    addref.isForward = true;
    addref.targetNodeId.nodeId = *typeId;
    addref.targetNodeClass = UA_NODECLASS_OBJECTTYPE;
    Service_AddReferences_single(server, session, &addref);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
copyStandardAttributes(UA_Node *node, const UA_AddNodesItem *item, const UA_NodeAttributes *attr) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_NodeId_copy(&item->requestedNewNodeId.nodeId, &node->nodeId);
    retval |= UA_QualifiedName_copy(&item->browseName, &node->browseName);
    retval |= UA_LocalizedText_copy(&attr->displayName, &node->displayName);
    retval |= UA_LocalizedText_copy(&attr->description, &node->description);
    node->writeMask = attr->writeMask;
    node->userWriteMask = attr->userWriteMask;
    return retval;
}

static UA_Node *
variableNodeFromAttributes(const UA_AddNodesItem *item, const UA_VariableAttributes *attr) {
    UA_VariableNode *vnode = UA_NodeStore_newVariableNode();
    if(!vnode)
        return NULL;
    UA_StatusCode retval = copyStandardAttributes((UA_Node*)vnode, item, (const UA_NodeAttributes*)attr);
    // todo: test if the type / valueRank / value attributes are consistent
    vnode->accessLevel = attr->accessLevel;
    vnode->userAccessLevel = attr->userAccessLevel;
    vnode->historizing = attr->historizing;
    vnode->minimumSamplingInterval = attr->minimumSamplingInterval;
    vnode->valueRank = attr->valueRank;
    retval |= UA_Variant_copy(&attr->value, &vnode->value.variant.value);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)vnode);
        return NULL;
    }
    return (UA_Node*)vnode;
}

static UA_Node *
objectNodeFromAttributes(const UA_AddNodesItem *item, const UA_ObjectAttributes *attr) {
    UA_ObjectNode *onode = UA_NodeStore_newObjectNode();
    if(!onode)
        return NULL;
    UA_StatusCode retval = copyStandardAttributes((UA_Node*)onode, item, (const UA_NodeAttributes*)attr);
    onode->eventNotifier = attr->eventNotifier;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)onode);
        return NULL;
    }
    return (UA_Node*)onode;
}

static UA_Node *
referenceTypeNodeFromAttributes(const UA_AddNodesItem *item, const UA_ReferenceTypeAttributes *attr) {
    UA_ReferenceTypeNode *rtnode = UA_NodeStore_newReferenceTypeNode();
    if(!rtnode)
        return NULL;
    UA_StatusCode retval = copyStandardAttributes((UA_Node*)rtnode, item, (const UA_NodeAttributes*)attr);
    rtnode->isAbstract = attr->isAbstract;
    rtnode->symmetric = attr->symmetric;
    retval |= UA_LocalizedText_copy(&attr->inverseName, &rtnode->inverseName);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)rtnode);
        return NULL;
    }
    return (UA_Node*)rtnode;
}

static UA_Node *
objectTypeNodeFromAttributes(const UA_AddNodesItem *item, const UA_ObjectTypeAttributes *attr) {
    UA_ObjectTypeNode *otnode = UA_NodeStore_newObjectTypeNode();
    if(!otnode)
        return NULL;
    UA_StatusCode retval = copyStandardAttributes((UA_Node*)otnode, item, (const UA_NodeAttributes*)attr);
    otnode->isAbstract = attr->isAbstract;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)otnode);
        return NULL;
    }
    return (UA_Node*)otnode;
}

static UA_Node *
variableTypeNodeFromAttributes(const UA_AddNodesItem *item, const UA_VariableTypeAttributes *attr) {
    UA_VariableTypeNode *vtnode = UA_NodeStore_newVariableTypeNode();
    if(!vtnode)
        return NULL;
    UA_StatusCode retval = copyStandardAttributes((UA_Node*)vtnode, item, (const UA_NodeAttributes*)attr);
    UA_Variant_copy(&attr->value, &vtnode->value.variant.value);
    // datatype is taken from the value
    vtnode->valueRank = attr->valueRank;
    // array dimensions are taken from the value
    vtnode->isAbstract = attr->isAbstract;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)vtnode);
        return NULL;
    }
    return (UA_Node*)vtnode;
}

static UA_Node *
viewNodeFromAttributes(const UA_AddNodesItem *item, const UA_ViewAttributes *attr) {
    UA_ViewNode *vnode = UA_NodeStore_newViewNode();
    if(!vnode)
        return NULL;
    UA_StatusCode retval = copyStandardAttributes((UA_Node*)vnode, item, (const UA_NodeAttributes*)attr);
    vnode->containsNoLoops = attr->containsNoLoops;
    vnode->eventNotifier = attr->eventNotifier;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)vnode);
        return NULL;
    }
    return (UA_Node*)vnode;
}

static UA_Node *
dataTypeNodeFromAttributes(const UA_AddNodesItem *item, const UA_DataTypeAttributes *attr) {
    UA_DataTypeNode *dtnode = UA_NodeStore_newDataTypeNode();
    if(!dtnode)
        return NULL;
    UA_StatusCode retval = copyStandardAttributes((UA_Node*)dtnode, item, (const UA_NodeAttributes*)attr);
    dtnode->isAbstract = attr->isAbstract;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)dtnode);
        return NULL;
    }
    return (UA_Node*)dtnode;
}

void Service_AddNodes_single(UA_Server *server, UA_Session *session, const UA_AddNodesItem *item,
                             UA_AddNodesResult *result, UA_InstantiationCallback *instantiationCallback) {
    if(item->nodeAttributes.encoding < UA_EXTENSIONOBJECT_DECODED ||
       !item->nodeAttributes.content.decoded.type) {
        result->statusCode = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;
        return;
    }

    /* Create the node */
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
        if(item->nodeAttributes.content.decoded.type != &UA_TYPES[UA_TYPES_VARIABLETYPEATTRIBUTES]) {
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
    Service_AddNodes_existing(server, session, node, &item->parentNodeId.nodeId,
                              &item->referenceTypeId, &item->typeDefinition.nodeId,
                              instantiationCallback, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        Service_DeleteNodes_single(server, session, &result->addedNodeId, true);
}

void Service_AddNodes(UA_Server *server, UA_Session *session, const UA_AddNodesRequest *request,
                      UA_AddNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing AddNodesRequest");
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

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
#ifdef _MSC_VER
    UA_Boolean *isExternal = UA_alloca(size);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32)*size);
#else
    UA_Boolean isExternal[size];
    UA_UInt32 indices[size];
#endif
    memset(isExternal, false, sizeof(UA_Boolean) * size);
    for(size_t j = 0; j <server->externalNamespacesSize; j++) {
        size_t indexSize = 0;
        for(size_t i = 0;i < size;i++) {
            if(request->nodesToAdd[i].requestedNewNodeId.nodeId.namespaceIndex !=
               server->externalNamespaces[j].index)
                continue;
            isExternal[i] = true;
            indices[indexSize] = (UA_UInt32)i;
            indexSize++;
        }
        if(indexSize == 0)
            continue;
        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->addNodes(ens->ensHandle, &request->requestHeader, request->nodesToAdd,
                      indices, (UA_UInt32)indexSize, response->results, response->diagnosticInfos);
    }
#endif

    response->resultsSize = size;
    for(size_t i = 0; i < size; i++) {
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
        if(!isExternal[i])
#endif
            Service_AddNodes_single(server, session, &request->nodesToAdd[i], &response->results[i], NULL);
    }
}

/**************************************************/
/* Add Special Nodes (not possible over the wire) */
/**************************************************/

UA_StatusCode
UA_Server_addDataSourceVariableNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                    const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
                                    const UA_VariableAttributes attr, const UA_DataSource dataSource,
                                    UA_NodeId *outNewNodeId) {
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

    UA_VariableNode *node = UA_NodeStore_newVariableNode();
    if(!node) {
        UA_AddNodesItem_deleteMembers(&item);
        UA_VariableAttributes_deleteMembers(&attrCopy);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    copyStandardAttributes((UA_Node*)node, &item, (UA_NodeAttributes*)&attrCopy);
    node->valueSource = UA_VALUESOURCE_DATASOURCE;
    node->value.dataSource = dataSource;
    node->accessLevel = attr.accessLevel;
    node->userAccessLevel = attr.userAccessLevel;
    node->historizing = attr.historizing;
    node->minimumSamplingInterval = attr.minimumSamplingInterval;
    node->valueRank = attr.valueRank;
    UA_RCU_LOCK();
    Service_AddNodes_existing(server, &adminSession, (UA_Node*)node, &item.parentNodeId.nodeId,
                              &item.referenceTypeId, &item.typeDefinition.nodeId, NULL, &result);
    UA_RCU_UNLOCK();
    UA_AddNodesItem_deleteMembers(&item);
    UA_VariableAttributes_deleteMembers(&attrCopy);

    if(outNewNodeId && result.statusCode == UA_STATUSCODE_GOOD)
        *outNewNodeId = result.addedNodeId;
    else
        UA_AddNodesResult_deleteMembers(&result);
    return result.statusCode;
}

#ifdef UA_ENABLE_METHODCALLS

UA_StatusCode
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                        UA_MethodCallback method, void *handle,
                        size_t inputArgumentsSize, const UA_Argument* inputArguments,
                        size_t outputArgumentsSize, const UA_Argument* outputArguments,
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

    UA_MethodNode *node = UA_NodeStore_newMethodNode();
    if(!node) {
        result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_AddNodesItem_deleteMembers(&item);
        UA_MethodAttributes_deleteMembers(&attrCopy);
        return result.statusCode;
    }

    copyStandardAttributes((UA_Node*)node, &item, (UA_NodeAttributes*)&attrCopy);
    node->executable = attrCopy.executable;
    node->userExecutable = attrCopy.executable;
    node->attachedMethod = method;
    node->methodHandle = handle;
    UA_AddNodesItem_deleteMembers(&item);
    UA_MethodAttributes_deleteMembers(&attrCopy);

    UA_RCU_LOCK();
    Service_AddNodes_existing(server, &adminSession, (UA_Node*)node, &item.parentNodeId.nodeId,
                              &item.referenceTypeId, &item.typeDefinition.nodeId, NULL, &result);
    UA_RCU_UNLOCK();
    if(result.statusCode != UA_STATUSCODE_GOOD)
        return result.statusCode;

    UA_ExpandedNodeId parent;
    UA_ExpandedNodeId_init(&parent);
    parent.nodeId = result.addedNodeId;

    const UA_NodeId hasproperty = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);

    if(inputArgumentsSize > 0){
        UA_VariableNode *inputArgumentsVariableNode = UA_NodeStore_newVariableNode();
        inputArgumentsVariableNode->nodeId.namespaceIndex = result.addedNodeId.namespaceIndex;
        inputArgumentsVariableNode->browseName = UA_QUALIFIEDNAME_ALLOC(0, "InputArguments");
        inputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", "InputArguments");
        inputArgumentsVariableNode->description = UA_LOCALIZEDTEXT_ALLOC("en_US", "InputArguments");
        inputArgumentsVariableNode->valueRank = 1;
        //TODO: 0.3 work item: the addMethodNode API does not have the possibility to set nodeIDs
        //actually we need to change the signature to pass UA_NS0ID_SERVER_GETMONITOREDITEMS_INPUTARGUMENTS
        //and UA_NS0ID_SERVER_GETMONITOREDITEMS_OUTPUTARGUMENTS into the function :/
        if(result.addedNodeId.namespaceIndex == 0 &&
           result.addedNodeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
           result.addedNodeId.identifier.numeric == UA_NS0ID_SERVER_GETMONITOREDITEMS){
            inputArgumentsVariableNode->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS_INPUTARGUMENTS);
        }
        UA_Variant_setArrayCopy(&inputArgumentsVariableNode->value.variant.value, inputArguments,
                                inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
        UA_AddNodesResult inputAddRes;
        UA_RCU_LOCK();
        Service_AddNodes_existing(server, &adminSession, (UA_Node*)inputArgumentsVariableNode,
                                  &parent.nodeId, &hasproperty, &UA_NODEID_NULL, NULL, &inputAddRes);
        UA_RCU_UNLOCK();
        // todo: check if adding succeeded
        UA_AddNodesResult_deleteMembers(&inputAddRes);
    }

    if(outputArgumentsSize > 0){
        /* create OutputArguments */
        UA_VariableNode *outputArgumentsVariableNode  = UA_NodeStore_newVariableNode();
        outputArgumentsVariableNode->nodeId.namespaceIndex = result.addedNodeId.namespaceIndex;
        outputArgumentsVariableNode->browseName  = UA_QUALIFIEDNAME_ALLOC(0, "OutputArguments");
        outputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OutputArguments");
        outputArgumentsVariableNode->description = UA_LOCALIZEDTEXT_ALLOC("en_US", "OutputArguments");
        outputArgumentsVariableNode->valueRank = 1;
        //FIXME: comment in line 882
        if(result.addedNodeId.namespaceIndex == 0 &&
           result.addedNodeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
           result.addedNodeId.identifier.numeric == UA_NS0ID_SERVER_GETMONITOREDITEMS){
            outputArgumentsVariableNode->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS_OUTPUTARGUMENTS);
        }
        UA_Variant_setArrayCopy(&outputArgumentsVariableNode->value.variant.value, outputArguments,
                                outputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
        UA_AddNodesResult outputAddRes;
        UA_RCU_LOCK();
        Service_AddNodes_existing(server, &adminSession, (UA_Node*)outputArgumentsVariableNode,
                                  &parent.nodeId, &hasproperty, &UA_NODEID_NULL, NULL, &outputAddRes);
        UA_RCU_UNLOCK();
        // todo: check if adding succeeded
        UA_AddNodesResult_deleteMembers(&outputAddRes);
    }

    if(outNewNodeId)
        *outNewNodeId = result.addedNodeId; // don't deleteMember the result
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

UA_StatusCode
Service_AddReferences_single(UA_Server *server, UA_Session *session, const UA_AddReferencesItem *item) {
UA_StatusCode retval = UA_STATUSCODE_GOOD;
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    UA_Boolean handledExternally = UA_FALSE;
#endif
    if(item->targetServerUri.length > 0)
        return UA_STATUSCODE_BADNOTIMPLEMENTED; // currently no expandednodeids are allowed

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    for(size_t j = 0; j <server->externalNamespacesSize; j++) {
        if(item->sourceNodeId.namespaceIndex != server->externalNamespaces[j].index) {
                continue;
        } else {
            UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
            retval = ens->addOneWayReference(ens->ensHandle, item);
            handledExternally = UA_TRUE;
            break;
        }
    }
    if(handledExternally == UA_FALSE) {
#endif
    /* cast away the const to loop the call through UA_Server_editNode beware
        the "if" above for external nodestores */
    retval = UA_Server_editNode(server, session, &item->sourceNodeId,
                                              (UA_EditNodeCallback)addOneWayReference, item);
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    }
#endif

    if(retval != UA_STATUSCODE_GOOD){
        return retval;
    }

    UA_AddReferencesItem secondItem;
    secondItem = *item;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.isForward = !item->isForward;
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    handledExternally = UA_FALSE;
    for(size_t j = 0; j <server->externalNamespacesSize; j++) {
        if(secondItem.sourceNodeId.namespaceIndex != server->externalNamespaces[j].index) {
                continue;
        } else {
            UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
            retval = ens->addOneWayReference(ens->ensHandle, &secondItem);
            handledExternally = UA_TRUE;
            break;
        }

    }
    if(handledExternally == UA_FALSE) {
#endif
    retval = UA_Server_editNode(server, session, &secondItem.sourceNodeId,
                                (UA_EditNodeCallback)addOneWayReference, &secondItem);
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
    }
#endif
    // todo: remove reference if the second direction failed
    return retval;
}

void Service_AddReferences(UA_Server *server, UA_Session *session, const UA_AddReferencesRequest *request,
                           UA_AddReferencesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing AddReferencesRequest");
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

#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
#ifdef NO_ALLOCA
    UA_Boolean isExternal[size];
    UA_UInt32 indices[size];
#else
    UA_Boolean *isExternal = UA_alloca(sizeof(UA_Boolean) * size);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32) * size);
#endif /*NO_ALLOCA */
    memset(isExternal, false, sizeof(UA_Boolean) * size);
    for(size_t j = 0; j < server->externalNamespacesSize; j++) {
        size_t indicesSize = 0;
        for(size_t i = 0;i < size;i++) {
            if(request->referencesToAdd[i].sourceNodeId.namespaceIndex
               != server->externalNamespaces[j].index)
                continue;
            isExternal[i] = true;
            indices[indicesSize] = (UA_UInt32)i;
            indicesSize++;
        }
        if (indicesSize == 0)
            continue;
        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->addReferences(ens->ensHandle, &request->requestHeader, request->referencesToAdd,
                           indices, (UA_UInt32)indicesSize, response->results, response->diagnosticInfos);
    }
#endif

    for(size_t i = 0; i < response->resultsSize; i++) {
#ifdef UA_ENABLE_EXTERNAL_NAMESPACES
        if(!isExternal[i])
#endif
            response->results[i] = Service_AddReferences_single(server, session, &request->referencesToAdd[i]);
    }
}

/****************/
/* Delete Nodes */
/****************/

// TODO: Check consistency constraints, remove the references.

UA_StatusCode
Service_DeleteNodes_single(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                           UA_Boolean deleteReferences) {
    const UA_Node *node = UA_NodeStore_get(server->nodestore, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if(deleteReferences == true) {
        UA_DeleteReferencesItem delItem;
        UA_DeleteReferencesItem_init(&delItem);
        delItem.deleteBidirectional = false;
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
        bd.includeSubtypes = true;
        bd.nodeClassMask = UA_NODECLASS_OBJECTTYPE;

        /* browse type definitions with admin rights */
        UA_BrowseResult result;
        UA_BrowseResult_init(&result);
        Service_Browse_single(server, &adminSession, NULL, &bd, UA_UINT32_MAX, &result);
        for(size_t i = 0; i < result.referencesSize; i++) {
            /* call the destructor */
            UA_ReferenceDescription *rd = &result.references[i];
            const UA_ObjectTypeNode *typenode =
                (const UA_ObjectTypeNode*)UA_NodeStore_get(server->nodestore, &rd->nodeId.nodeId);
            if(!typenode)
                continue;
            if(typenode->nodeClass != UA_NODECLASS_OBJECTTYPE || !typenode->lifecycleManagement.destructor)
                continue;

            /* if there are several types with lifecycle management, call all the destructors */
            typenode->lifecycleManagement.destructor(*nodeId, ((const UA_ObjectNode*)node)->instanceHandle);
        }
        UA_BrowseResult_deleteMembers(&result);
    }

    return UA_NodeStore_remove(server->nodestore, nodeId);
}

void Service_DeleteNodes(UA_Server *server, UA_Session *session, const UA_DeleteNodesRequest *request,
                         UA_DeleteNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing DeleteNodesRequest");
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
    for(size_t i = 0; i < request->nodesToDeleteSize; i++) {
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
    UA_Boolean edited = false;
    for(size_t i = node->referencesSize-1; ; i--) {
        if(i > node->referencesSize)
            break; /* underflow after i == 0 */
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
        edited = true;
        break;
    }
    if(!edited)
        return UA_STATUSCODE_UNCERTAINREFERENCENOTDELETED;
    /* we removed the last reference */
    if(node->referencesSize == 0 && node->references)
        UA_free(node->references);
    return UA_STATUSCODE_GOOD;;
}

UA_StatusCode
Service_DeleteReferences_single(UA_Server *server, UA_Session *session,
                                const UA_DeleteReferencesItem *item) {
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

void
Service_DeleteReferences(UA_Server *server, UA_Session *session,
                         const UA_DeleteReferencesRequest *request,
                         UA_DeleteReferencesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing DeleteReferencesRequest");
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
        response->results[i] =
            Service_DeleteReferences_single(server, session, &request->referencesToDelete[i]);
}
