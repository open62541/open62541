/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"
#include "ua_services.h"

/*********************/
/* Edit Node Context */
/*********************/

UA_StatusCode
UA_Server_getNodeContext(UA_Server *server, UA_NodeId nodeId,
                             void **nodeContext) {
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    *nodeContext = node->context;
    return UA_STATUSCODE_GOOD;
}

struct SetNodeContext {
    void *context;
    UA_Boolean setConstructed;
};

static UA_StatusCode
editNodeContext(UA_Server *server, UA_Session* session,
                UA_Node* node, struct SetNodeContext *ctx) {
    node->context = ctx->context;
    if(ctx->setConstructed)
        node->constructed = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setNodeContext(UA_Server *server, UA_NodeId nodeId,
                         void *nodeContext) {
    struct SetNodeContext ctx;
    ctx.context = nodeContext;
    ctx.setConstructed = false; /* Only "constructNode" can set this */
    UA_RCU_LOCK();
    UA_StatusCode retval =
        UA_Server_editNode(server, &adminSession, &nodeId,
                           (UA_EditNodeCallback)editNodeContext, &ctx);
    UA_RCU_UNLOCK();
    return retval;
}

/**********************/
/* Consistency Checks */
/**********************/

/* Check if the requested parent node exists, has the right node class and is
 * referenced with an allowed (hierarchical) reference type. For "type" nodes,
 * only hasSubType references are allowed. */
static UA_StatusCode
checkParentReference(UA_Server *server, UA_Session *session, UA_NodeClass nodeClass,
                     const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId) {
    /* Objects do not need a parent (e.g. mandatory/optional modellingrules) */
    if(nodeClass == UA_NODECLASS_OBJECT && UA_NodeId_isNull(parentNodeId) &&
       UA_NodeId_isNull(referenceTypeId))
        return UA_STATUSCODE_GOOD;

/* See
    ifthe parent exists */
    const UA_Node *parent = UA_NodeStore_get(server->nodestore, parentNodeId);
    if(!parent) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: Parent node not found");

        returnUA_STATUSCODE_BADPARENTNODEIDINVALID;
    }

    /* Check the referencetype exists */
    const UA_ReferenceTypeNode *referenceType =
        (const UA_ReferenceTypeNode*)UA_NodeStore_get(server->nodestore, referenceTypeId);
    if(!referenceType) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: Reference type to the parent not found");

        returnUA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    /* Check if the referencetype is a reference type node */
    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: Reference type to the parent invalid");

        returnUA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    /* Check that the reference type is not abstract */
    if(referenceType->isAbstract == true) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: Abstract reference type to the parent not allowed");
        returnUA_STATUSCODE_BADREFERENCENOTALLOWED;
    }

    /* Check hassubtype relation for type nodes */
    if(nodeClass == UA_NODECLASS_DATATYPE ||
       nodeClass == UA_NODECLASS_VARIABLETYPE ||
       nodeClass == UA_NODECLASS_OBJECTTYPE ||
       nodeClass == UA_NODECLASS_REFERENCETYPE) {
        /* type needs hassubtype reference to the supertype */
        if(!UA_NodeId_equal(referenceTypeId, &subtypeId)) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: New type node need to have a "
                                "HasSubType reference");
            return UA_STATUSCODE_BADREFERENCENOTALLOWED;
        }
        /* supertype needs to be of the same node type  */
        if(parent->nodeClass != nodeClass) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: New type node needs to be of the same "
                                "node type as the parent");
            return UA_STATUSCODE_BADPARENTNODEIDINVALID;
        }
        return UA_STATUSCODE_GOOD;
    }

    /* Test if the referencetype is hierarchical */
    const UA_NodeId hierarchicalReference =
            UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    if(!isNodeInTree(server->nodestore, referenceTypeId,
                     &hierarchicalReference, &subtypeId, 1)) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Reference type is not hierarchical");
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }



    returnUA_STATUSCODE_GOOD;
}

static UA_StatusCode
typeCheckVariableNode(UA_Server *server, UA_Session *session,
                      const UA_NodeId *nodeId, const UA_VariableTypeNode *vt) {
    /* Get the node */
    const UA_VariableNode *node = (const UA_VariableNode*)
        UA_NodeStore_get(server->nodestore, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* The value might come from a datasource, so we perform a
     * regular read. */
    UA_DataValue value;
    UA_DataValue_init(&value);
    UA_StatusCode retval = readValueAttribute(server, session, node, &value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Check the datatype against the vt */
    if(!compatibleDataType(server, &node->dataType, &vt->dataType))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Get the array dimensions */
    size_t arrayDims = node->arrayDimensionsSize;
    if(arrayDims == 0 && value.hasValue && value.value.type &&
       !UA_Variant_isScalar(&value.value)) {
        arrayDims = 1; /* No array dimensions on an array implies one dimension */
    }

    /* Check valueRank against array dimensions */
    retval = compatibleValueRankArrayDimensions(node->valueRank, arrayDims);
        if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Check valueRank against the vt */
    retval = compatibleValueRanks(node->valueRank, vt->valueRank);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Check array dimensions against the vt */
    retval = compatibleArrayDimensions(vt->arrayDimensionsSize, vt->arrayDimensions,
                                       node->arrayDimensionsSize, node->arrayDimensions);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Typecheck the value */
    if(value.hasValue) {
        retval = typeCheckValue(server, &node->dataType, node->valueRank,
                                node->arrayDimensionsSize, node->arrayDimensions,
                                &value.value, NULL, NULL);
        /* The type-check failed. Write the same value again. The write-service
         * tries to convert to the correct type... */
        if(retval != UA_STATUSCODE_GOOD) {
            UA_RCU_UNLOCK();
            retval = UA_Server_writeValue(server, node->nodeId, value.value);
            UA_RCU_LOCK();
        }
        UA_DataValue_deleteMembers(&value);
    }
    return retval;
}

/********************/
/* Instantiate Node */
/********************/

static UA_StatusCode
fillVariableNodeAttributes(UA_Server *server, UA_Session *session,
                           const UA_NodeId *nodeId,
                           const UA_VariableTypeNode *vt) {
    /* Get the node */
    const UA_VariableNode *node = (const UA_VariableNode*)
        UA_NodeStore_get(server->nodestore, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* Is this a variable? */
    if(node->nodeClass != UA_NODECLASS_VARIABLE &&
       node->nodeClass != UA_NODECLASS_VARIABLETYPE)
        return UA_STATUSCODE_BADNODECLASSINVALID;

    /* Is the variable type abstract? */
    if(node->nodeClass == UA_NODECLASS_VARIABLE && vt->isAbstract)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;

    /* The value might come from a datasource, so we perform a
     * regular read. */
    UA_DataValue value;
    UA_DataValue_init(&value);
    UA_StatusCode retval = readValueAttribute(server, session, node, &value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* If no value is set, see if the vt provides one and copy it. This needs to
     * be done before copying the datatype from the vt, as setting the datatype
     * triggers a typecheck. */
    if(!value.hasValue || !value.value.type) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "AddNodes: "
                            "No value given; Copy the value from the TypeDefinition");
        UA_DataValue vt_value;
        UA_DataValue_init(&vt_value);
        retval = readValueAttribute(server, session, (const UA_VariableNode*)vt, &vt_value);
        if(retval == UA_STATUSCODE_GOOD && value.hasValue && value.value.type) {
            UA_RCU_UNLOCK();
            retval = UA_Server_writeValue(server, node->nodeId, value.value);
            UA_RCU_LOCK();
        }
        UA_DataValue_deleteMembers(&vt_value);
    }

    /* If no datatype is given, use the datatype of the vt */
    if(retval == UA_STATUSCODE_GOOD && UA_NodeId_isNull(&node->dataType)) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: "
                            "No datatype given; Copy the datatype attribute "
                            "from the TypeDefinition");
        UA_RCU_UNLOCK();
        retval = UA_Server_writeDataType(server, node->nodeId, vt->dataType);
        UA_RCU_LOCK();
    }

    UA_DataValue_deleteMembers(&value);
    return retval;
}

static UA_StatusCode
instantiateVariableNodeAttributes(UA_Server *server, UA_Session *session,
                                  const UA_NodeId *nodeId,
                                  const UA_NodeId *typeDef) {
    /* Get the variable type */
    const UA_VariableTypeNode *vt =
        (const UA_VariableTypeNode*)UA_NodeStore_get(server->nodestore, typeDef);
    if(!vt || vt->nodeClass != UA_NODECLASS_VARIABLETYPE)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;

    /* Set attributes defined in the variable type */
    UA_StatusCode retval = fillVariableNodeAttributes(server, session, nodeId, vt);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Perform the type check */
    return typeCheckVariableNode(server, session, nodeId, vt);
}

/* Search for an instance of "browseName" in node searchInstance Used during
 * copyChildNodes to find overwritable/mergable nodes */
static UA_StatusCode
instanceFindAggregateByBrowsename(UA_Server *server, UA_Session *session,
                                  const UA_NodeId *searchInstance,
                                  const UA_QualifiedName *browseName,
                                  UA_NodeId *outInstanceNodeId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *searchInstance;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES);
    bd.includeSubtypes = true;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE | UA_NODECLASS_METHOD;
    bd.resultMask = UA_BROWSERESULTMASK_NODECLASS | UA_BROWSERESULTMASK_BROWSENAME;

    UA_BrowseResult br;
    UA_BrowseResult_init(&br);
    Service_Browse_single(server, session, NULL, &bd, 0, &br);
    if(br.statusCode != UA_STATUSCODE_GOOD)
        return br.statusCode;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < br.referencesSize; ++i) {
        UA_ReferenceDescription *rd = &br.references[i];
        if(rd->browseName.namespaceIndex == browseName->namespaceIndex &&
           UA_String_equal(&rd->browseName.name, &browseName->name)) {
            retval = UA_NodeId_copy(&rd->nodeId.nodeId, outInstanceNodeId);
            break;
        }
    }

    UA_BrowseResult_deleteMembers(&br);
    return retval;
}

static const UA_NodeId mandatoryId =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_MODELLINGRULE_MANDATORY}};
static const UA_NodeId hasModellingRuleId =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASMODELLINGRULE}};

static UA_Boolean
isMandatoryChild(UA_Server *server, UA_Session *session,
                 const UA_NodeId *childNodeId) {
    /* Get the child */
    const UA_Node *child = UA_NodeStore_get(server->nodestore, childNodeId);
    if(!child)
        return false;

    /* Look for the reference making the child mandatory */
    for(size_t i = 0; i < child->referencesSize; ++i) {
        UA_NodeReferenceKind *refs = &child->references[i];
        if(!UA_NodeId_equal(&hasModellingRuleId, &refs->referenceTypeId))
            continue;
        if(refs->isInverse)
            continue;
        for(size_t j = 0; j < refs->targetIdsSize; ++j) {
            if(UA_NodeId_equal(&mandatoryId, &refs->targetIds[j].nodeId))
                return true;
        }
    }
    return false;
}

static UA_StatusCode
copyChildNodes(UA_Server *server, UA_Session *session,
               const UA_NodeId *sourceNodeId,
               const UA_NodeId *destinationNodeId);

static UA_StatusCode
Service_AddNode_finish(UA_Server *server, UA_Session *session,
                       const UA_NodeId *nodeId, const UA_NodeId *parentNodeId,
                       const UA_NodeId *referenceTypeId, const UA_NodeId *typeDefinition);

static void
addReference(UA_Server *server, UA_Session *session,
             const UA_AddReferencesItem *item, UA_StatusCode *retval);

static UA_StatusCode
copyChildNode(UA_Server *server, UA_Session *session,
              const UA_NodeId *destinationNodeId,
              const UA_ReferenceDescription *rd) {
    UA_NodeId existingChild = UA_NODEID_NULL;
    UA_StatusCode retval =
        instanceFindAggregateByBrowsename(server, session, destinationNodeId,
                                          &rd->browseName, &existingChild);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Have a child with that browseName. Try to deep-copy missing members. */
    if(!UA_NodeId_isNull(&existingChild)) {
        if(rd->nodeClass == UA_NODECLASS_VARIABLE ||
           rd->nodeClass == UA_NODECLASS_OBJECT)
            retval = copyChildNodes(server, session, &rd->nodeId.nodeId, &existingChild);
        UA_NodeId_deleteMembers(&existingChild);
        return retval;
    }

    /* Is the child mandatory? If not, skip */
    if(!isMandatoryChild(server, session, &rd->nodeId.nodeId))
        return UA_STATUSCODE_GOOD;

    /* No existing child with that browsename. Create it. */
    if(rd->nodeClass == UA_NODECLASS_METHOD) {
        /* Add a reference to the method in the objecttype */
        UA_AddReferencesItem newItem;
        UA_AddReferencesItem_init(&newItem);
        newItem.sourceNodeId = *destinationNodeId;
        newItem.referenceTypeId = rd->referenceTypeId;
        newItem.isForward = true;
        newItem.targetNodeId = rd->nodeId;
        newItem.targetNodeClass = UA_NODECLASS_METHOD;
        addReference(server, session, &newItem, &retval);
        return retval;
    }

    /* Node exists and is a variable or object. Instantiate missing mandatory
     * children */
    if(rd->nodeClass == UA_NODECLASS_VARIABLE ||
       rd->nodeClass == UA_NODECLASS_OBJECT) {
        /* Get the node */
        const UA_Node *node = UA_NodeStore_get(server->nodestore, &rd->nodeId.nodeId);
        if(!node)
            return UA_STATUSCODE_BADNODEIDINVALID;

        /* Get the type */
        const UA_NodeId *typeId = getNodeType(server, node);

        /* Get a copy of the node */
        UA_Node *node_copy = UA_NodeStore_getCopy(server->nodestore, &rd->nodeId.nodeId);
        if(!node_copy)
            return UA_STATUSCODE_BADNODEIDINVALID;

        /* Reset the NodeId (random numeric id will be assigned in the nodestore) */
        UA_NodeId_deleteMembers(&node_copy->nodeId);
        node_copy->nodeId.namespaceIndex = destinationNodeId->namespaceIndex;

        /* Remove references, they are re-created from scratch in addnode_finish */
        /* TODO: Be more clever in removing references that are re-added during
         * addnode_finish. That way, we can call addnode_finish also on children that were
         * manually added by the user during addnode_begin and addnode_finish. */
        UA_Node_deleteReferences(node_copy);

        /* Add the node to the nodestore */
        retval = UA_NodeStore_insert(server->nodestore, node_copy);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        /* Call addnode_finish, this recursively adds members, the type
         * definition and so on */
        retval = Service_AddNode_finish(server, session, &node->nodeId, destinationNodeId,
                                        &rd->referenceTypeId, typeId);
    }
    return retval;
}

/* Copy any children of Node sourceNodeId to another node destinationNodeId. */
static UA_StatusCode
copyChildNodes(UA_Server *server, UA_Session *session,
               const UA_NodeId *sourceNodeId, const UA_NodeId *destinationNodeId) {
    /* Browse to get all children of the source */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *sourceNodeId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES);
    bd.includeSubtypes = true;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE | UA_NODECLASS_METHOD;
    bd.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID | UA_BROWSERESULTMASK_NODECLASS |
        UA_BROWSERESULTMASK_BROWSENAME;

    UA_BrowseResult br;
    UA_BrowseResult_init(&br);
    Service_Browse_single(server, session, NULL, &bd, 0, &br);
    if(br.statusCode != UA_STATUSCODE_GOOD)
        return br.statusCode;

    /* Copy all children from source to destination */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < br.referencesSize; ++i) {
        UA_ReferenceDescription *rd = &br.references[i];
        retval |= copyChildNode(server, session, destinationNodeId, rd);
    }
    UA_BrowseResult_deleteMembers(&br);
    return retval;
}

/* The node is deleted in the caller when the instantiation fails here */
static UA_StatusCode
constructNode(UA_Server *server, UA_Session *session,
              const UA_Node *node, const UA_NodeId *typeId) {
    /* Currently, only variables and objects are instantiated */
    if(node->nodeClass != UA_NODECLASS_VARIABLE &&
       node->nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_GOOD;

    /* Get the type node */
    UA_ASSERT_RCU_LOCKED();
    const UA_Node *typenode = UA_NodeStore_get(server->nodestore, typeId);
    if(!typenode)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;

    /* See if the type has the correct node class */
    if(node->nodeClass == UA_NODECLASS_VARIABLE) {
        if(typenode->nodeClass != UA_NODECLASS_VARIABLETYPE ||
           ((const UA_VariableTypeNode*)typenode)->isAbstract)
            return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    } else { /* nodeClass == UA_NODECLASS_OBJECT */
        if(typenode->nodeClass != UA_NODECLASS_OBJECTTYPE ||
           ((const UA_ObjectTypeNode*)typenode)->isAbstract)
            return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    }

    /* Get the hierarchy of the type and all its supertypes */
    UA_NodeId *hierarchy = NULL;
    size_t hierarchySize = 0;
    UA_StatusCode retval = getTypeHierarchy(server->nodestore, typeId,
                                            &hierarchy, &hierarchySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Copy members of the type and supertypes (and instantiate them) */
    for(size_t i = 0; i < hierarchySize; ++i)
        retval |= copyChildNodes(server, session, &hierarchy[i], &node->nodeId);
    UA_Array_delete(hierarchy, hierarchySize, &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Add a hasTypeDefinition reference */
    UA_AddReferencesItem addref;
    UA_AddReferencesItem_init(&addref);
    addref.sourceNodeId = node->nodeId;
    addref.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    addref.isForward = true;
    addref.targetNodeId.nodeId = *typeId;
    addReference(server, session, &addref, &retval);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Get the node type constructor */
    const UA_NodeTypeLifecycle *lifecycle = NULL;
    if(node->nodeClass == UA_NODECLASS_OBJECT) {
        const UA_ObjectTypeNode *ot = (const UA_ObjectTypeNode*)typenode;
        lifecycle = &ot->lifecycle;
    } else if(node->nodeClass == UA_NODECLASS_VARIABLE) {
        const UA_VariableTypeNode *vt = (const UA_VariableTypeNode*)typenode;
        lifecycle = &vt->lifecycle;
    }

    /* Call the global constructor */
    void *context = node->context;
    if(server->config.nodeLifecycle.constructor)
        retval = server->config.nodeLifecycle.constructor(server, &session->sessionId,
                                                          session->sessionHandle,
                                                          &node->nodeId, &context);

    /* Call the type constructor */
    if(retval == UA_STATUSCODE_GOOD && lifecycle && lifecycle->constructor)
        retval = lifecycle->constructor(server, &session->sessionId,
                                        session->sessionHandle, &typenode->nodeId,
                                        typenode->context, &node->nodeId, &context);

    /* Set the context *and* mark the node as constructed */
    if(retval == UA_STATUSCODE_GOOD) {
        struct SetNodeContext ctx;
        ctx.context = context;
        ctx.setConstructed = true;
        UA_RCU_LOCK();
        retval = UA_Server_editNode(server, &adminSession, &node->nodeId,
                                    (UA_EditNodeCallback)editNodeContext, &ctx);
        UA_RCU_UNLOCK();
    }

    /* Destruct the node. (It will be deleted outside of this function) */
    if(retval != UA_STATUSCODE_GOOD && server->config.nodeLifecycle.destructor)
        server->config.nodeLifecycle.destructor(server, &session->sessionId,
                                                session->sessionHandle, &node->nodeId, context);

    return retval;
}

/************/
/* Add Node */
/************/

static void
Service_AddNode_begin(UA_Server *server, UA_Session *session,
                      const UA_AddNodesItem *item, UA_AddNodesResult *result,
                      void *context) {
    /* Check the namespaceindex */
    if(item->requestedNewNodeId.nodeId.namespaceIndex >= server->namespacesSize) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Namespace invalid");
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return;
    }

    /* Add the node to the nodestore */
    UA_Node *node = NULL;
    result->statusCode = UA_Node_createFromAttributes(item, &node);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node could not create a node "
                            "with error code %s",
                            UA_StatusCode_name(result->statusCode));
        return;
    }

    node->context = context;
    result->statusCode = UA_NodeStore_insert(server->nodestore, node);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node could not add the new node "
                            "to the nodestore with error code %s",
                            UA_StatusCode_name(result->statusCode));
        return;
    }

    /* Copy the nodeid of the new node */
    result->statusCode = UA_NodeId_copy(&node->nodeId, &result->addedNodeId);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Could not copy the nodeid");
        UA_Server_deleteNode(server, node->nodeId, true);
    }
}

static const UA_NodeId baseDataVariableType =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_BASEDATAVARIABLETYPE}};
static const UA_NodeId baseObjectType =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_BASEOBJECTTYPE}};

static UA_StatusCode
Service_AddNode_finish(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                       const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId,
                       const UA_NodeId *typeDefinition) {
    /* Get the node */
    const UA_Node *node = UA_NodeStore_get(server->nodestore, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* Use the typeDefinition as parent for type-nodes */
    const UA_NodeId hasSubtype = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    if(node->nodeClass == UA_NODECLASS_VARIABLETYPE ||
       node->nodeClass == UA_NODECLASS_OBJECTTYPE ||
       node->nodeClass == UA_NODECLASS_REFERENCETYPE ||
       node->nodeClass == UA_NODECLASS_DATATYPE) {
        referenceTypeId = &hasSubtype;
        typeDefinition = parentNodeId;
    }

    /* Replace empty typeDefinition with the most permissive default */
    if((node->nodeClass == UA_NODECLASS_VARIABLE ||
        node->nodeClass == UA_NODECLASS_OBJECT) && UA_NodeId_isNull(typeDefinition)) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: No TypeDefinition; "
                            "Use the default TypeDefinition for the Variable/Object");
        if(node->nodeClass == UA_NODECLASS_VARIABLE)
            typeDefinition = &baseDataVariableType;
        else
            typeDefinition = &baseObjectType;
    }

    /* Check parent reference. Objects may have no parent. */
    UA_StatusCode retval = checkParentReference(server, session, node->nodeClass,
                                                parentNodeId, referenceTypeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: The parent reference is invalid");
        UA_Server_deleteNode(server, *nodeId, true);
        return retval;
    }

    /* For variables, perform attribute-checks and -instantiation */
    if(node->nodeClass == UA_NODECLASS_VARIABLE ||
       node->nodeClass == UA_NODECLASS_VARIABLETYPE) {
        retval = instantiateVariableNodeAttributes(server, session, nodeId, typeDefinition);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Type checking failed with error code %s",
                                UA_StatusCode_name(retval));
            UA_Server_deleteNode(server, *nodeId, true);
            return retval;
        }
    }

    /* Add children and call the constructor */
    retval = constructNode(server, session, node, typeDefinition);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node instantiation failed "
                            "with status code %s", UA_StatusCode_name(retval));
        UA_Server_deleteNode(server, *nodeId, true);
        return retval;
    }

    /* Add parent reference */
    if(!UA_NodeId_isNull(parentNodeId)) {
        UA_AddReferencesItem ref_item;
        UA_AddReferencesItem_init(&ref_item);
        ref_item.sourceNodeId = *nodeId;
        ref_item.referenceTypeId = *referenceTypeId;
        ref_item.isForward = false;
        ref_item.targetNodeId.nodeId = *parentNodeId;
        addReference(server, session, &ref_item, &retval);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Adding reference to parent failed");
            UA_Server_deleteNode(server, *nodeId, true);
            return retval;
        }
    }

    return UA_STATUSCODE_GOOD;

}

static void
Service_AddNodes_single(UA_Server *server, UA_Session *session,
                        const UA_AddNodesItem *item, UA_AddNodesResult *result,
                        void *nodeContext) {
    /* AddNodes_begin */
    Service_AddNode_begin(server, session, item, result, nodeContext);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* AddNodes_finish */
    result->statusCode =
        Service_AddNode_finish(server, session, &result->addedNodeId,
                               &item->parentNodeId.nodeId, &item->referenceTypeId,
                               &item->typeDefinition.nodeId);

    /* If finishing failed, don't even return a NodeId of the added node */
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_NodeId_deleteMembers(&result->addedNodeId);
}

void Service_AddNodes(UA_Server *server, UA_Session *session,
                      const UA_AddNodesRequest *request,
                      UA_AddNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing AddNodesRequest");

    if(request->nodesToAddSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    size_t size = request->nodesToAddSize;
    response->results =
        (UA_AddNodesResult*)UA_Array_new(size, &UA_TYPES[UA_TYPES_ADDNODESRESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->resultsSize = size;
    for(size_t i = 0; i < size; ++i) {
            Service_AddNodes_single(server, session, &request->nodesToAdd[i],
                                    &response->results[i], NULL);
    }
}

UA_StatusCode
__UA_Server_addNode(UA_Server *server, const UA_NodeClass nodeClass,
                    const UA_NodeId *requestedNewNodeId,
                    const UA_NodeId *parentNodeId,
                    const UA_NodeId *referenceTypeId,
                    const UA_QualifiedName browseName,
                    const UA_NodeId *typeDefinition,
                    const UA_NodeAttributes *attr,
                    const UA_DataType *attributeType,
                    void *nodeContext, UA_NodeId *outNewNodeId) {
    /* Create the AddNodesItem */
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.requestedNewNodeId.nodeId = *requestedNewNodeId;
    item.browseName = browseName;
    item.nodeClass = nodeClass;
    item.parentNodeId.nodeId = *parentNodeId;
    item.referenceTypeId = *referenceTypeId;
    item.typeDefinition.nodeId = *typeDefinition;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type =attributeType;
    item.nodeAttributes.content.decoded.data = (void*)(uintptr_t)attr;

    /* Call the normal addnodes service */
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    UA_RCU_LOCK();
    Service_AddNodes_single(server, &adminSession, &item, &result, nodeContext);
    UA_RCU_UNLOCK();
    if(outNewNodeId)
        *outNewNodeId = result.addedNodeId;
    else
        UA_NodeId_deleteMembers(&result.addedNodeId);
    return result.statusCode;
}

UA_StatusCode
__UA_Server_addNode_begin(UA_Server *server, const UA_NodeClass nodeClass,
                          const UA_NodeId *requestedNewNodeId,
                          const UA_QualifiedName *browseName,
                          const UA_NodeAttributes *attr,
                          const UA_DataType *attributeType,
                          void *nodeContext, UA_NodeId *outNewNodeId) {
    /* Create the item */
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.requestedNewNodeId.nodeId = *requestedNewNodeId;
    item.browseName = *browseName;
    item.nodeClass = nodeClass;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type = attributeType;
    item.nodeAttributes.content.decoded.data = (void*)(uintptr_t)attr;

    /* Add the node without checks or instantiation */
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    UA_RCU_LOCK();
    Service_AddNode_begin(server, &adminSession, &item, &result, nodeContext);
    if(outNewNodeId)
        *outNewNodeId = result.addedNodeId;
    else
        UA_NodeId_deleteMembers(&result.addedNodeId);
    UA_RCU_UNLOCK();
    return result.statusCode;
}

UA_StatusCode
UA_Server_addNode_finish(UA_Server *server, const UA_NodeId nodeId,
                         const UA_NodeId parentNodeId,
                         const UA_NodeId referenceTypeId,
                         const UA_NodeId typeDefinition) {
    UA_RCU_LOCK();
    UA_StatusCode retval =
        Service_AddNode_finish(server, &adminSession, &nodeId, &parentNodeId,
                               &referenceTypeId, &typeDefinition);
    UA_RCU_UNLOCK();
    return retval;
}

/****************/
/* Delete Nodes */
/****************/

static void
deleteReference(UA_Server *server, UA_Session *session,
                const UA_DeleteReferencesItem *item,
                UA_StatusCode *retval);

/* Remove references to this node (in the other nodes) */
static void
removeIncomingReferences(UA_Server *server, UA_Session *session,
                     const UA_Node *node) {
    UA_DeleteReferencesItem item;
    UA_DeleteReferencesItem_init(&item);
    item.targetNodeId.nodeId = node->nodeId;
    item.deleteBidirectional = false;
    UA_StatusCode dummy;
    for(size_t i = 0; i < node->referencesSize; ++i) {
        UA_NodeReferenceKind *refs = &node->references[i];
        item.isForward = refs->isInverse;
        item.referenceTypeId = refs->referenceTypeId;
        for(size_t j = 0; j < refs->targetIdsSize; ++j) {
            item.sourceNodeId = refs->targetIds[j].nodeId;
            deleteReference(server, session, &item, &dummy);
        }
    }
}

static UA_StatusCode
deleteNode(UA_Server *server, UA_Session *session,
           const UA_DeleteNodesItem *item, UA_StatusCode *result) {
    UA_RCU_LOCK();
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &item->nodeId);
    UA_RCU_UNLOCK();
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* TODO: Check if the information model consistency is violated */
    /* TODO: Check if the node is a mandatory child of an object */

    if(UA_Node_hasSubTypeOrInstances(node)) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "Delete Nodes: Cannot delete a node with active instances");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Call the node type destructor if the constructor has been run */
    if(node->constructed) {
        void *context = node->context; /* No longer needed after this function */
        UA_RCU_LOCK();
        if(node->nodeClass == UA_NODECLASS_OBJECT) {
            /* Call the destructor from the object type */
            const UA_ObjectTypeNode *typenode =
                getObjectNodeType(server, (const UA_ObjectNode*)node);
            if(typenode && typenode->lifecycle.destructor)
                typenode->lifecycle.destructor(server, &session->sessionId,
                                               session->sessionHandle, &typenode->nodeId,
                                               typenode->context, &item->nodeId, &context);
        } else if(node->nodeClass == UA_NODECLASS_VARIABLE) {
            const UA_VariableTypeNode *typenode =
                getVariableNodeType(server, (const UA_VariableNode*)node);
            if(typenode && typenode->lifecycle.destructor)
                typenode->lifecycle.destructor(server, &session->sessionId,
                                               session->sessionHandle, &typenode->nodeId,
                                               typenode->context, &item->nodeId, &context);
        }
        UA_RCU_UNLOCK();

        /* Call the global destructor */
        if(server->config.nodeLifecycle.destructor)
            server->config.nodeLifecycle.destructor(server, &session->sessionId,
                                                    session->sessionHandle,
                                                    &item->nodeId, context);
    }

    /* Remove references to the node (not the references in the node that will
     * be deleted anyway) */
    if(item->deleteTargetReferences)
        removeIncomingReferences(server, session, node);

    /* Remove the node in the nodestore */
    UA_RCU_LOCK();
    UA_StatusCode retval = UA_NodeStore_remove(server->nodestore, &item->nodeId);
    UA_RCU_UNLOCK();

    return retval;
}

void Service_DeleteNodes(UA_Server *server, UA_Session *session,
                         const UA_DeleteNodesRequest *request,
                         UA_DeleteNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing DeleteNodesRequest");
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation)deleteNode,
                                           &request->nodesToDeleteSize,
                                           &UA_TYPES[UA_TYPES_DELETENODESITEM],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_STATUSCODE]);
}

UA_StatusCode
UA_Server_deleteNode(UA_Server *server, const UA_NodeId nodeId,
                     UA_Boolean deleteReferences) {
    UA_DeleteNodesItem item;
    item.deleteTargetReferences = deleteReferences;
    item.nodeId = nodeId;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = deleteNode(server, &adminSession, &item, &retval);
    return retval;
}


/******************/
/* Add References */
/******************/

static UA_StatusCode
addOneWayReference(UA_Server *server, UA_Session *session,
             UA_Node *node, const UA_AddReferencesItem *item) {
    return UA_Node_addReference(node, item);
}

static UA_StatusCode
deleteOneWayReference(UA_Server *server, UA_Session *session, UA_Node *node,
                      const UA_DeleteReferencesItem *item) {
    return UA_Node_deleteReference(node, item);
}

static void
addReference(UA_Server *server, UA_Session *session,
             const UA_AddReferencesItem *item, UA_StatusCode *retval) {
    /* Currently no expandednodeids are allowed */
    if(item->targetServerUri.length > 0) {
        *retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
        return;
    }

    /* Add the first direction */
    UA_RCU_UNLOCK();
    *retval =
        UA_Server_editNode(server, session, &item->sourceNodeId,
                           (UA_EditNodeCallback)addOneWayReference, item);
    UA_RCU_LOCK();
    if(*retval != UA_STATUSCODE_GOOD)
        return;

    /* Add the second direction */
    UA_AddReferencesItem secondItem;
    UA_AddReferencesItem_init(&secondItem);
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.referenceTypeId = item->referenceTypeId;
    secondItem.isForward = !item->isForward;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    /* keep default secondItem.targetNodeClass = UA_NODECLASS_UNSPECIFIED */
    UA_RCU_UNLOCK();
    *retval =
        UA_Server_editNode(server, session, &secondItem.sourceNodeId,
                           (UA_EditNodeCallback)addOneWayReference, &secondItem);
    UA_RCU_LOCK();

    /* remove reference if the second direction failed */
    if(*retval != UA_STATUSCODE_GOOD) {
        UA_DeleteReferencesItem deleteItem;
        deleteItem.sourceNodeId = item->sourceNodeId;
        deleteItem.referenceTypeId = item->referenceTypeId;
        deleteItem.isForward = item->isForward;
        deleteItem.targetNodeId = item->targetNodeId;
        deleteItem.deleteBidirectional = false;
        /* ignore returned status code */
        UA_RCU_UNLOCK();
        UA_Server_editNode(server, session, &item->sourceNodeId,
                           (UA_EditNodeCallback)deleteOneWayReference, &deleteItem);
        UA_RCU_LOCK();
    }
}

void Service_AddReferences(UA_Server *server, UA_Session *session,
                           const UA_AddReferencesRequest *request,
                           UA_AddReferencesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing AddReferencesRequest");
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation) addReference,
                                           &request->referencesToAddSize,
                                           &UA_TYPES[UA_TYPES_ADDREFERENCESITEM],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_STATUSCODE]);
}

UA_StatusCode
UA_Server_addReference(UA_Server *server, const UA_NodeId sourceId,
                       const UA_NodeId refTypeId,
                       const UA_ExpandedNodeId targetId,
                       UA_Boolean isForward) {
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = sourceId;
    item.referenceTypeId = refTypeId;
    item.isForward = isForward;
    item.targetNodeId = targetId;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_RCU_LOCK();
    addReference(server, &adminSession, &item, &retval);
    UA_RCU_UNLOCK();
    return retval;
}

/*********************/
/* Delete References */
/*********************/

static void
deleteReference(UA_Server *server, UA_Session *session,
                const UA_DeleteReferencesItem *item,
                UA_StatusCode *retval) {
    // TODO: Check consistency constraints, remove the references.
    *retval = UA_Server_editNode(server, session, &item->sourceNodeId,
                                 (UA_EditNodeCallback)deleteOneWayReference, item);
    if(*retval != UA_STATUSCODE_GOOD)
        return;

    if(!item->deleteBidirectional || item->targetNodeId.serverIndex != 0)
        return;

    UA_DeleteReferencesItem secondItem;
    UA_DeleteReferencesItem_init(&secondItem);
    secondItem.isForward = !item->isForward;
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    secondItem.referenceTypeId = item->referenceTypeId;
    *retval = UA_Server_editNode(server, session, &secondItem.sourceNodeId,
                                 (UA_EditNodeCallback)deleteOneWayReference,
                                 &secondItem);
}

void
Service_DeleteReferences(UA_Server *server, UA_Session *session,
                         const UA_DeleteReferencesRequest *request,
                         UA_DeleteReferencesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing DeleteReferencesRequest");
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation)deleteReference,
                                           &request->referencesToDeleteSize,
                                           &UA_TYPES[UA_TYPES_DELETEREFERENCESITEM],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_STATUSCODE]);
}

UA_StatusCode
UA_Server_deleteReference(UA_Server *server, const UA_NodeId sourceNodeId,
                          const UA_NodeId referenceTypeId, UA_Boolean isForward,
                          const UA_ExpandedNodeId targetNodeId,
                          UA_Boolean deleteBidirectional) {
    UA_DeleteReferencesItem item;
    item.sourceNodeId = sourceNodeId;
    item.referenceTypeId = referenceTypeId;
    item.isForward = isForward;
    item.targetNodeId = targetNodeId;
    item.deleteBidirectional = deleteBidirectional;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_RCU_LOCK();
    deleteReference(server, &adminSession, &item, &retval);
    UA_RCU_UNLOCK();
    return retval;
}


/**********************/
/* Set Value Callback */
/**********************/

static UA_StatusCode
setValueCallback(UA_Server *server, UA_Session *session,
                 UA_VariableNode *node, UA_ValueCallback *callback) {
    if(node->nodeClass != UA_NODECLASS_VARIABLE)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    node->value.data.callback = *callback;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setVariableNode_valueCallback(UA_Server *server,
                                        const UA_NodeId nodeId,
                                        const UA_ValueCallback callback) {
    UA_RCU_LOCK();
    UA_StatusCode retval =
        UA_Server_editNode(server, &adminSession, &nodeId,
                           (UA_EditNodeCallback)setValueCallback, &callback);
    UA_RCU_UNLOCK();
    return retval;
}

/***************************************************/
/* Special Handling of Variables with Data Sources */
/***************************************************/

UA_StatusCode
UA_Server_addDataSourceVariableNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                                    const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                    const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
                                    const UA_VariableAttributes attr, const UA_DataSource dataSource,
                                    void *nodeContext, UA_NodeId *outNewNodeId) {
    UA_NodeId newNodeId;
    UA_Boolean deleteNodeId = UA_FALSE;
    if(!outNewNodeId) {
        newNodeId = UA_NODEID_NULL;
        outNewNodeId = &newNodeId;
        deleteNodeId = UA_TRUE;
    }
    UA_StatusCode retval =
        UA_Server_addVariableNode_begin(server, requestedNewNodeId, browseName,
                                        attr, nodeContext, outNewNodeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    retval = UA_Server_setVariableNode_dataSource(server, *outNewNodeId, dataSource);
    if(retval == UA_STATUSCODE_GOOD)
        retval = UA_Server_addNode_finish(server, *outNewNodeId, parentNodeId,
                                          referenceTypeId, typeDefinition);
    if(retval != UA_STATUSCODE_GOOD || deleteNodeId)
        UA_NodeId_deleteMembers(outNewNodeId);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setDataSource(UA_Server *server, UA_Session *session,
              UA_VariableNode* node, UA_DataSource *dataSource) {
    if(node->nodeClass != UA_NODECLASS_VARIABLE)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    if(node->valueSource == UA_VALUESOURCE_DATA)
        UA_DataValue_deleteMembers(&node->value.data.value);
    node->value.dataSource = *dataSource;
    node->valueSource = UA_VALUESOURCE_DATASOURCE;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setVariableNode_dataSource(UA_Server *server, const UA_NodeId nodeId,
                                     const UA_DataSource dataSource) {
    UA_RCU_LOCK();
    UA_StatusCode retval =
        UA_Server_editNode(server, &adminSession, &nodeId,
                           (UA_EditNodeCallback)setDataSource,
                           &dataSource);
    UA_RCU_UNLOCK();
    return retval;
}

/************************************/
/* Special Handling of Method Nodes */
/************************************/

#ifdef UA_ENABLE_METHODCALLS

static const UA_NodeId hasproperty =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASPROPERTY}};
static const UA_NodeId propertytype =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_PROPERTYTYPE}};

UA_StatusCode
UA_Server_addMethodNode_finish(UA_Server *server, const UA_NodeId nodeId,
                               const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                               UA_MethodCallback method,
                               size_t inputArgumentsSize, const UA_Argument* inputArguments,
                               size_t outputArgumentsSize, const UA_Argument* outputArguments) {
    /* Browse to see which argument nodes exist */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = nodeId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    bd.includeSubtypes = false;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_VARIABLE;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;

    UA_BrowseResult br;
    UA_BrowseResult_init(&br);
    UA_RCU_LOCK();
    Service_Browse_single(server, &adminSession, NULL, &bd, 0, &br);
    UA_RCU_UNLOCK();

    UA_StatusCode retval = br.statusCode;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_RCU_LOCK();
        UA_Server_deleteNode(server, nodeId, true);
        UA_RCU_UNLOCK();
        UA_BrowseResult_deleteMembers(&br);
        return retval;
    }

    /* Filter out the argument nodes */
    UA_NodeId inputArgsId = UA_NODEID_NULL;
    UA_NodeId outputArgsId = UA_NODEID_NULL;
    const UA_NodeId newArgsId = UA_NODEID_NUMERIC(nodeId.namespaceIndex, 0);
    const UA_QualifiedName inputArgsName = UA_QUALIFIEDNAME(0, "InputArguments");
    const UA_QualifiedName outputArgsName = UA_QUALIFIEDNAME(0, "OutputArguments");
    for(size_t i = 0; i < br.referencesSize; i++) {
        UA_ReferenceDescription *rd = &br.references[i];
        if(rd->browseName.namespaceIndex == 0 &&
           UA_String_equal(&rd->browseName.name, &inputArgsName.name))
            inputArgsId = rd->nodeId.nodeId;
        else if(rd->browseName.namespaceIndex == 0 &&
                UA_String_equal(&rd->browseName.name, &outputArgsName.name))
            outputArgsId = rd->nodeId.nodeId;
    }

    /* Add the Input Arguments VariableNode */
    if(inputArgumentsSize > 0 && UA_NodeId_isNull(&inputArgsId)) {
        UA_VariableAttributes inputargs;
        UA_VariableAttributes_init(&inputargs);
        inputargs.displayName = UA_LOCALIZEDTEXT("", "InputArguments");
        /* UAExpert creates a monitoreditem on inputarguments ... */
        inputargs.minimumSamplingInterval = 100000.0f;
        inputargs.valueRank = 1;
        inputargs.accessLevel = UA_ACCESSLEVELMASK_READ;
        inputargs.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
        /* dirty-cast, but is treated as const ... */
        UA_Variant_setArray(&inputargs.value, (void*)(uintptr_t)inputArguments,
                            inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
        retval = UA_Server_addVariableNode(server, newArgsId, nodeId, hasproperty,
                                           inputArgsName, propertytype, inputargs,
                                           NULL, &inputArgsId);
    }

    /* Add the Output Arguments VariableNode */
    if(outputArgumentsSize > 0 && UA_NodeId_isNull(&outputArgsId)) {
        UA_VariableAttributes outputargs;
        UA_VariableAttributes_init(&outputargs);
        outputargs.displayName = UA_LOCALIZEDTEXT("", "OutputArguments");
        /* UAExpert creates a monitoreditem on outputarguments ... */
        outputargs.minimumSamplingInterval = 100000.0f;
        outputargs.valueRank = 1;
        outputargs.accessLevel = UA_ACCESSLEVELMASK_READ;
        outputargs.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
        /* dirty-cast, but is treated as const ... */
        UA_Variant_setArray(&outputargs.value, (void*)(uintptr_t)outputArguments,
                            outputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
        retval |= UA_Server_addVariableNode(server, newArgsId, nodeId, hasproperty,
                                            outputArgsName, propertytype, outputargs,
                                            NULL, &outputArgsId);
    }

    retval |= UA_Server_setMethodNode_callback(server, nodeId, method);

    /* Call finish to add the parent reference */
    UA_RCU_LOCK();
    retval |= Service_AddNode_finish(server, &adminSession, &nodeId, &parentNodeId,
                                     &referenceTypeId, &UA_NODEID_NULL);
    UA_RCU_UNLOCK();

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, nodeId, true);
        UA_Server_deleteNode(server, inputArgsId, true);
        UA_Server_deleteNode(server, outputArgsId, true);
    }
    UA_BrowseResult_deleteMembers(&br);
    return retval;
}

UA_StatusCode
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                        UA_MethodCallback method,
                        size_t inputArgumentsSize, const UA_Argument* inputArguments,
                        size_t outputArgumentsSize, const UA_Argument* outputArguments,
                        void *nodeContext, UA_NodeId *outNewNodeId) {
    UA_NodeId newId;
    if(!outNewNodeId) {
        UA_NodeId_init(&newId);
        outNewNodeId = &newId;
    }

    UA_StatusCode retval =
        UA_Server_addMethodNode_begin(server, requestedNewNodeId,
                                      browseName, attr, nodeContext, outNewNodeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_Server_addMethodNode_finish(server, *outNewNodeId,
                                            parentNodeId, referenceTypeId, method,
                                            inputArgumentsSize, inputArguments,
                                            outputArgumentsSize, outputArguments);

    if(outNewNodeId == &newId)
        UA_NodeId_deleteMembers(&newId);
    return retval;
}

static UA_StatusCode
editMethodCallback(UA_Server *server, UA_Session* session,
                   UA_Node* node, void* handle) {
    if(node->nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    UA_MethodNode *mnode = (UA_MethodNode*) node;
    mnode->method = (UA_MethodCallback)(uintptr_t)handle;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setMethodNode_callback(UA_Server *server,
                                 const UA_NodeId methodNodeId,
                                 UA_MethodCallback methodCallback) {
    UA_RCU_LOCK();
    UA_StatusCode retval =
        UA_Server_editNode(server, &adminSession, &methodNodeId,
                           (UA_EditNodeCallback)editMethodCallback,
                           (void*)(uintptr_t)methodCallback);
    UA_RCU_UNLOCK();
    return retval;
}

#endif

/************************/
/* Lifecycle Management */
/************************/

static UA_StatusCode
setNodeTypeLifecycle(UA_Server *server, UA_Session *session,
                     UA_Node* node, UA_NodeTypeLifecycle *lifecycle) {
    if(node->nodeClass == UA_NODECLASS_OBJECTTYPE) {
        UA_ObjectTypeNode *ot = (UA_ObjectTypeNode*)node;
        ot->lifecycle = *lifecycle;
        return UA_STATUSCODE_GOOD;
    }

    if(node->nodeClass == UA_NODECLASS_VARIABLETYPE) {
        UA_VariableTypeNode *vt = (UA_VariableTypeNode*)node;
        vt->lifecycle = *lifecycle;
        return UA_STATUSCODE_GOOD;
    }

    return UA_STATUSCODE_BADNODECLASSINVALID;
}

UA_StatusCode
UA_Server_setNodeTypeLifecycle(UA_Server *server, UA_NodeId nodeId,
                               UA_NodeTypeLifecycle lifecycle) {
    UA_RCU_LOCK();
    UA_StatusCode retval =
        UA_Server_editNode(server, &adminSession, &nodeId,
                           (UA_EditNodeCallback)setNodeTypeLifecycle,
                           &lifecycle);
    UA_RCU_UNLOCK();
    return retval;
}
