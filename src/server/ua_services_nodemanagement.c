/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2016 (c) LEvertz
 *    Copyright 2016 (c) Lorenz Haas
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017-2018 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Christian von Arnim
 *    Copyright 2017 (c) Henrik Norrman
 */

#include "ua_server_internal.h"
#include "ua_services.h"

/*********************/
/* Edit Node Context */
/*********************/

UA_StatusCode
UA_Server_getNodeContext(UA_Server *server, UA_NodeId nodeId,
                         void **nodeContext) {
    const UA_Node *node = UA_Nodestore_get(server, &nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    *nodeContext = node->context;
    UA_Nodestore_release(server, node);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
editNodeContext(UA_Server *server, UA_Session* session,
                UA_Node* node, void *context) {
    node->context = context;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setNodeContext(UA_Server *server, UA_NodeId nodeId,
                         void *nodeContext) {
    UA_StatusCode retval =
        UA_Server_editNode(server, &adminSession, &nodeId,
                           (UA_EditNodeCallback)editNodeContext, nodeContext);
    return retval;
}

/**********************/
/* Consistency Checks */
/**********************/


#define UA_PARENT_REFERENCES_COUNT 2

const UA_NodeId parentReferences[UA_PARENT_REFERENCES_COUNT] = {
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASSUBTYPE}},
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}}
};

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

    /* See if the parent exists */
    const UA_Node *parent = UA_Nodestore_get(server, parentNodeId);
    if(!parent) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Parent node not found");
        return UA_STATUSCODE_BADPARENTNODEIDINVALID;
    }

    UA_NodeClass parentNodeClass = parent->nodeClass;
    UA_Nodestore_release(server, parent);

    /* Check the referencetype exists */
    const UA_ReferenceTypeNode *referenceType = (const UA_ReferenceTypeNode*)
        UA_Nodestore_get(server, referenceTypeId);
    if(!referenceType) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Reference type to the parent not found");
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    /* Check if the referencetype is a reference type node */
    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Reference type to the parent not a ReferenceTypeNode");
        UA_Nodestore_release(server, (const UA_Node*)referenceType);
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    UA_Boolean referenceTypeIsAbstract = referenceType->isAbstract;
    UA_Nodestore_release(server, (const UA_Node*)referenceType);
    /* Check that the reference type is not abstract */
    if(referenceTypeIsAbstract == true) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Abstract reference type to the parent not allowed");
        return UA_STATUSCODE_BADREFERENCENOTALLOWED;
    }

    /* Check hassubtype relation for type nodes */
    if(nodeClass == UA_NODECLASS_DATATYPE ||
       nodeClass == UA_NODECLASS_VARIABLETYPE ||
       nodeClass == UA_NODECLASS_OBJECTTYPE ||
       nodeClass == UA_NODECLASS_REFERENCETYPE) {
        /* type needs hassubtype reference to the supertype */
        if(!UA_NodeId_equal(referenceTypeId, &subtypeId)) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Type nodes need to have a HasSubType reference to the parent");
            return UA_STATUSCODE_BADREFERENCENOTALLOWED;
        }
        /* supertype needs to be of the same node type  */
        if(parentNodeClass != nodeClass) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Type nodes needs to be of the same node type as their parent");
            return UA_STATUSCODE_BADPARENTNODEIDINVALID;
        }
        return UA_STATUSCODE_GOOD;
    }

    /* Test if the referencetype is hierarchical */
    if(!isNodeInTree(&server->config.nodestore, referenceTypeId,
                     &hierarchicalReferences, &subtypeId, 1)) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Reference type to the parent is not hierarchical");
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
typeCheckVariableNode(UA_Server *server, UA_Session *session,
                      const UA_VariableNode *node,
                      const UA_VariableTypeNode *vt) {
    /* The value might come from a datasource, so we perform a
     * regular read. */
    UA_DataValue value;
    UA_DataValue_init(&value);
    UA_StatusCode retval = readValueAttribute(server, session, node, &value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Check the datatype against the vt */
    if(!compatibleDataType(server, &node->dataType, &vt->dataType, false)) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: The value is incompatible with the datatype of the VariableType");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* Check valueRank against array dimensions */
    if(!compatibleValueRankArrayDimensions(server, session, node->valueRank, node->arrayDimensionsSize))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Check valueRank against the vt */
    if(!compatibleValueRanks(node->valueRank, vt->valueRank)) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: The value rank is incomatible with the value rank of the VariableType");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* Check array dimensions against the vt */
    if(!compatibleArrayDimensions(vt->arrayDimensionsSize, vt->arrayDimensions,
                                  node->arrayDimensionsSize, node->arrayDimensions)) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: The array dimensions are "
                            "incomatible with the array dimensions of the VariableType");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    /* Typecheck the value */
    if(value.hasValue) {
        /* If the type-check failed write the same value again. The
         * write-service tries to convert to the correct type... */
        if(!compatibleValue(server, session, &node->dataType, node->valueRank,
                            node->arrayDimensionsSize, node->arrayDimensions,
                            &value.value, NULL))
            retval = UA_Server_writeValue(server, node->nodeId, value.value);
        UA_DataValue_deleteMembers(&value);
    }
    return retval;
}

/********************/
/* Instantiate Node */
/********************/

static const UA_NodeId baseDataVariableType =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_BASEDATAVARIABLETYPE}};
static const UA_NodeId baseObjectType =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_BASEOBJECTTYPE}};
static const UA_NodeId hasTypeDefinition =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASTYPEDEFINITION}};

/* Use attributes from the variable type wherever required. Reload the node if
 * changes were made. */
static UA_StatusCode
useVariableTypeAttributes(UA_Server *server, UA_Session *session,
                          const UA_VariableNode **node_ptr, const UA_VariableTypeNode *vt) {
    const UA_VariableNode *node = *node_ptr;
    UA_Boolean modified = false;

    /* If no value is set, see if the vt provides one and copy it. This needs to
     * be done before copying the datatype from the vt, as setting the datatype
     * triggers a typecheck. */
    UA_DataValue orig;
    UA_DataValue_init(&orig);
    UA_StatusCode retval = readValueAttribute(server, session, node, &orig);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if(orig.value.type) {
        /* A value is present */
        UA_DataValue_deleteMembers(&orig);
    } else {
        UA_LOG_DEBUG_SESSION(server->config.logger, session,
                             "AddNodes: No value given; Copy the value "
                             "from the TypeDefinition");
        UA_WriteValue v;
        UA_WriteValue_init(&v);
        retval = readValueAttribute(server, session, (const UA_VariableNode*)vt, &v.value);
        if(retval == UA_STATUSCODE_GOOD && v.value.hasValue) {
            v.nodeId = node->nodeId;
            v.attributeId = UA_ATTRIBUTEID_VALUE;
            retval = UA_Server_writeWithSession(server, session, &v);
            modified = true;
        }
        UA_DataValue_deleteMembers(&v.value);

        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* If no datatype is given, use the datatype of the vt */
    if(UA_NodeId_isNull(&node->dataType)) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: "
                            "No datatype given; Copy the datatype attribute "
                            "from the TypeDefinition");
        UA_WriteValue v;
        UA_WriteValue_init(&v);
        v.nodeId = node->nodeId;
        v.attributeId = UA_ATTRIBUTEID_DATATYPE;
        v.value.hasValue = true;
        UA_Variant_setScalar(&v.value.value, (void*)(uintptr_t)&vt->dataType, &UA_TYPES[UA_TYPES_NODEID]);
        retval = UA_Server_writeWithSession(server, session, &v);
        modified = true;

        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Use the ArrayDimensions of the vt */
    if(node->arrayDimensionsSize == 0 && vt->arrayDimensionsSize > 0) {
        UA_WriteValue v;
        UA_WriteValue_init(&v);
        v.nodeId = node->nodeId;
        v.attributeId = UA_ATTRIBUTEID_ARRAYDIMENSIONS;
        v.value.hasValue = true;
        UA_Variant_setArray(&v.value.value, vt->arrayDimensions, vt->arrayDimensionsSize,
                            &UA_TYPES[UA_TYPES_UINT32]);
        retval = UA_Server_writeWithSession(server, session, &v);
        modified = true;

        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* If the node was modified, update the pointer to the new version */
    if(modified) {
        const UA_VariableNode *updated = (const UA_VariableNode*)
            UA_Nodestore_get(server, &node->nodeId);

        if(!updated)
            return UA_STATUSCODE_BADINTERNALERROR;

        UA_Nodestore_release(server, (const UA_Node*)node);
        *node_ptr = updated;
    }

    return UA_STATUSCODE_GOOD;
}

/* Search for an instance of "browseName" in node searchInstance. Used during
 * copyChildNodes to find overwritable/mergable nodes. Does not touch
 * outInstanceNodeId if no child is found. */
static UA_StatusCode
findChildByBrowsename(UA_Server *server, UA_Session *session,
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
    UA_UInt32 maxrefs = 0;
    Operation_Browse(server, session, &maxrefs, &bd, &br);
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
    const UA_Node *child = UA_Nodestore_get(server, childNodeId);
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
            if(UA_NodeId_equal(&mandatoryId, &refs->targetIds[j].nodeId)) {
                UA_Nodestore_release(server, child);
                return true;
            }
        }
    }

    UA_Nodestore_release(server, child);
    return false;
}

static UA_StatusCode
copyChildNodes(UA_Server *server, UA_Session *session,
               const UA_NodeId *sourceNodeId,
               const UA_NodeId *destinationNodeId);

static void
Operation_addReference(UA_Server *server, UA_Session *session, void *context,
                       const UA_AddReferencesItem *item, UA_StatusCode *retval);


/*
 * This method only deletes references from the node which are not matching any type in the given array.
 * Could be used to e.g. delete all the references, except 'HASMODELINGRULE'
 */
static void deleteReferencesSubset(UA_Node *node, size_t referencesSkipSize, UA_NodeId* referencesSkip) {
    if(referencesSkipSize == 0) {
        UA_Node_deleteReferences(node);
        return;
    }

    /* Let's count if there are references left. If not just delete all the references.
     * It's faster */
    size_t newSize = 0;
    for(size_t i = 0; i < node->referencesSize; ++i) {
        for(size_t j = 0; j < referencesSkipSize; j++) {
            if(UA_NodeId_equal(&node->references[i].referenceTypeId, &referencesSkip[j])) {
                newSize++;
            }
        }
    }
    if(newSize == 0) {
        UA_Node_deleteReferences(node);
        return;
    }

    /* Now copy the remaining references to a new array */
    UA_NodeReferenceKind *newReferences = (UA_NodeReferenceKind *)UA_malloc(sizeof(UA_NodeReferenceKind) * (newSize));
    size_t curr = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < node->referencesSize && retval == UA_STATUSCODE_GOOD; ++i) {
        for(size_t j = 0; j < referencesSkipSize; j++) {
            if(!UA_NodeId_equal(&node->references[i].referenceTypeId, &referencesSkip[j]))
                continue;

            // copy the reference
            UA_NodeReferenceKind *srefs = &node->references[i];
            UA_NodeReferenceKind *drefs = &newReferences[curr++];
            drefs->isInverse = srefs->isInverse;
            retval = UA_NodeId_copy(&srefs->referenceTypeId, &drefs->referenceTypeId);
            if(retval != UA_STATUSCODE_GOOD)
                break;
            retval = UA_Array_copy(srefs->targetIds, srefs->targetIdsSize,
                                   (void**)&drefs->targetIds,
                                   &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
            if(retval != UA_STATUSCODE_GOOD)
                break;
            drefs->targetIdsSize = srefs->targetIdsSize;
            break;
        }
        if(retval != UA_STATUSCODE_GOOD) {
            for(size_t k=0; k<i; k++) {
                UA_NodeReferenceKind *refs = &newReferences[i];
                for(size_t j = 0; j < refs->targetIdsSize; ++j)
                    UA_ExpandedNodeId_deleteMembers(&refs->targetIds[j]);
                UA_Array_delete(refs->targetIds, refs->targetIdsSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
                UA_NodeId_deleteMembers(&refs->referenceTypeId);
            }
        }
    }

    UA_Node_deleteReferences(node);
    if(retval == UA_STATUSCODE_GOOD) {
        node->references = newReferences;
        node->referencesSize = newSize;
    } else {
        UA_free(newReferences);
    }
}

static UA_StatusCode
copyChildNode(UA_Server *server, UA_Session *session,
              const UA_NodeId *destinationNodeId,
              const UA_ReferenceDescription *rd) {
    UA_NodeId existingChild = UA_NODEID_NULL;
    UA_StatusCode retval =
        findChildByBrowsename(server, session, destinationNodeId,
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
        Operation_addReference(server, session, NULL, &newItem, &retval);
        return retval;
    }

    /* Node exists and is a variable or object. Instantiate missing mandatory
     * children */
    if(rd->nodeClass == UA_NODECLASS_VARIABLE ||
       rd->nodeClass == UA_NODECLASS_OBJECT) {
        /* Get the node */
        UA_Node *node;
        retval = UA_Nodestore_getCopy(server, &rd->nodeId.nodeId, &node);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        /* Get the type */
        const UA_Node *type = getNodeType(server, node);
        const UA_NodeId *typeId;
        if(type)
            typeId = &type->nodeId;
        else
            typeId = &UA_NODEID_NULL;

        /* Reset the NodeId (random numeric id will be assigned in the nodestore) */
        UA_NodeId_deleteMembers(&node->nodeId);
        node->nodeId.namespaceIndex = destinationNodeId->namespaceIndex;

        /* Remove references, they are re-created from scratch in addnode_finish */
        /* TODO: Be more clever in removing references that are re-added during
         * addnode_finish. That way, we can call addnode_finish also on children that were
         * manually added by the user during addnode_begin and addnode_finish. */
        /* For now we keep all the modelling rule references and delete all others */
        UA_NodeId modellingRuleReferenceId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE);
        deleteReferencesSubset(node, 1, &modellingRuleReferenceId);

        /* Add the node to the nodestore */
        UA_NodeId newNodeId;
        retval = UA_Nodestore_insert(server, node, &newNodeId);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Nodestore_release(server, type);
            return retval;
        }

        /* Add all the children of this child to the new child node to make sure we take
         * the values from the nearest inherited object first.
         * The call to addNode_finish will then only add the children from the type and
         * thus skip the direct children of rd->nodeId.nodeId */
        copyChildNodes(server, session, &rd->nodeId.nodeId, &newNodeId);

        /* Add the parent reference */
        retval = AddNode_addRefs(server, session, &newNodeId, destinationNodeId,
                                 &rd->referenceTypeId, typeId);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Nodestore_remove(server, &node->nodeId);
            UA_Nodestore_release(server, type);
            return retval;
        }

        /* Call addnode_finish, this recursively adds additional members, the type
         * definition and so on of the base type of this child, if they are not yet
         * in the destination */
        retval |= AddNode_finish(server, session, &newNodeId);
        UA_NodeId_deleteMembers(&newNodeId);
        UA_Nodestore_release(server, type);
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
    UA_UInt32 maxrefs = 0;
    Operation_Browse(server, session, &maxrefs, &bd, &br);
    if(br.statusCode != UA_STATUSCODE_GOOD)
        return br.statusCode;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < br.referencesSize; ++i) {
        UA_ReferenceDescription *rd = &br.references[i];
        retval |= copyChildNode(server, session, destinationNodeId, rd);
    }

    UA_BrowseResult_deleteMembers(&br);
    return retval;
}

static UA_StatusCode
addChildren(UA_Server *server, UA_Session *session,
            const UA_Node *node, const UA_Node *type) {
    /* Get the hierarchy of the type and all its supertypes */
    UA_NodeId *hierarchy = NULL;
    size_t hierarchySize = 0;
    UA_StatusCode retval = getTypeHierarchy(&server->config.nodestore, &type->nodeId,
                                            &hierarchy, &hierarchySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Copy members of the type and supertypes (and instantiate them) */
    for(size_t i = 0; i < hierarchySize; ++i)
        retval |= copyChildNodes(server, session, &hierarchy[i], &node->nodeId);
    UA_Array_delete(hierarchy, hierarchySize, &UA_TYPES[UA_TYPES_NODEID]);
    return retval;
}

/* Calls the global destructor internally of the global constructor succeeds and
 * the type-level constructor fails. */
static UA_StatusCode callConstructors(UA_Server *server, UA_Session *session,
                                      const UA_Node *node, const UA_Node *type) {
    /* Get the node type constructor */
    const UA_NodeTypeLifecycle *lifecycle = NULL;
    if(node->nodeClass == UA_NODECLASS_OBJECT) {
        const UA_ObjectTypeNode *ot = (const UA_ObjectTypeNode*)type;
        lifecycle = &ot->lifecycle;
    } else if(node->nodeClass == UA_NODECLASS_VARIABLE) {
        const UA_VariableTypeNode *vt = (const UA_VariableTypeNode*)type;
        lifecycle = &vt->lifecycle;
    }

    /* Call the global constructor */
    void *context = node->context;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(server->config.nodeLifecycle.constructor)
        retval = server->config.nodeLifecycle.constructor(server, &session->sessionId,
                                                          session->sessionHandle,
                                                          &node->nodeId, &context);

    /* Call the type constructor */
    if(retval == UA_STATUSCODE_GOOD && lifecycle && lifecycle->constructor)
        retval = lifecycle->constructor(server, &session->sessionId,
                                        session->sessionHandle, &type->nodeId,
                                        type->context, &node->nodeId, &context);

    /* Set the context *and* mark the node as constructed */
    if(retval == UA_STATUSCODE_GOOD)
        retval = UA_Server_editNode(server, &adminSession, &node->nodeId,
                                    (UA_EditNodeCallback)editNodeContext,
                                    context);

    /* Fail. Call the global destructor. */
    if(retval != UA_STATUSCODE_GOOD && server->config.nodeLifecycle.destructor)
        server->config.nodeLifecycle.destructor(server, &session->sessionId,
                                                session->sessionHandle,
                                                &node->nodeId, context);

    return retval;
}

static UA_StatusCode
addRef(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
       const UA_NodeId *referenceTypeId, const UA_NodeId *parentNodeId,
       UA_Boolean forward) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_AddReferencesItem ref_item;
    UA_AddReferencesItem_init(&ref_item);
    ref_item.sourceNodeId = *nodeId;
    ref_item.referenceTypeId = *referenceTypeId;
    ref_item.isForward = forward;
    ref_item.targetNodeId.nodeId = *parentNodeId;
    Operation_addReference(server, session, NULL, &ref_item, &retval);
    return retval;
}

/************/
/* Add Node */
/************/

static void
removeDeconstructedNode(UA_Server *server, UA_Session *session,
                        const UA_Node *node, UA_Boolean removeTargetRefs);

static const UA_NodeId hasSubtype = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASSUBTYPE}};

UA_StatusCode
AddNode_addRefs(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId,
                const UA_NodeId *typeDefinitionId) {
    /* Get the node */
    const UA_Node *type = NULL;
    const UA_Node *node = UA_Nodestore_get(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* Use the typeDefinition as parent for type-nodes */
    if(node->nodeClass == UA_NODECLASS_VARIABLETYPE ||
       node->nodeClass == UA_NODECLASS_OBJECTTYPE ||
       node->nodeClass == UA_NODECLASS_REFERENCETYPE ||
       node->nodeClass == UA_NODECLASS_DATATYPE) {
        if(UA_NodeId_equal(referenceTypeId, &UA_NODEID_NULL))
            referenceTypeId = &hasSubtype;
        const UA_Node *parentNode = UA_Nodestore_get(server, parentNodeId);
        if(parentNode) {
            if(parentNode->nodeClass == node->nodeClass)
                typeDefinitionId = parentNodeId;
            UA_Nodestore_release(server, parentNode);
        }
    }

    /* Check parent reference. Objects may have no parent. */
    UA_StatusCode retval = checkParentReference(server, session, node->nodeClass,
                                                parentNodeId, referenceTypeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: The parent reference is invalid "
                            "with status code %s", UA_StatusCode_name(retval));
        goto cleanup;
    }

    /* Replace empty typeDefinition with the most permissive default */
    if((node->nodeClass == UA_NODECLASS_VARIABLE ||
        node->nodeClass == UA_NODECLASS_OBJECT) &&
       UA_NodeId_isNull(typeDefinitionId)) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: No TypeDefinition; Use the default "
                            "TypeDefinition for the Variable/Object");
        if(node->nodeClass == UA_NODECLASS_VARIABLE)
            typeDefinitionId = &baseDataVariableType;
        else
            typeDefinitionId = &baseObjectType;
    }

    /* Get the node type. There must be a typedefinition for variables, objects
     * and type-nodes. See the above checks. */
    if(!UA_NodeId_isNull(typeDefinitionId)) {
        /* Get the type node */
        type = UA_Nodestore_get(server, typeDefinitionId);
        if(!type) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Node type not found");
            retval = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            goto cleanup;
        }

        UA_Boolean typeOk = UA_FALSE;
        switch(node->nodeClass) {
            case UA_NODECLASS_DATATYPE:
                typeOk = type->nodeClass == UA_NODECLASS_DATATYPE;
                break;
            case UA_NODECLASS_METHOD:
                typeOk = type->nodeClass == UA_NODECLASS_METHOD;
                break;
            case UA_NODECLASS_OBJECT:
                typeOk = type->nodeClass == UA_NODECLASS_OBJECTTYPE;
                break;
            case UA_NODECLASS_OBJECTTYPE:
                typeOk = type->nodeClass == UA_NODECLASS_OBJECTTYPE;
                break;
            case UA_NODECLASS_REFERENCETYPE:
                typeOk = type->nodeClass == UA_NODECLASS_REFERENCETYPE;
                break;
            case UA_NODECLASS_VARIABLE:
                typeOk = type->nodeClass == UA_NODECLASS_VARIABLETYPE;
                break;
            case UA_NODECLASS_VARIABLETYPE:
                typeOk = type->nodeClass == UA_NODECLASS_VARIABLETYPE;
                break;
            case UA_NODECLASS_VIEW:
                typeOk = type->nodeClass == UA_NODECLASS_VIEW;
                break;
            default:
                typeOk = UA_FALSE;
        }
        if(!typeOk) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Type does not match node class");
            retval = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            goto cleanup;
        }

        /* See if the type has the correct node class. For type-nodes, we know
         * that type has the same nodeClass from checkParentReference. */
        if(node->nodeClass == UA_NODECLASS_VARIABLE) {
            if(((const UA_VariableTypeNode*)type)->isAbstract) {
                /* Abstract variable is allowed if parent is a children of a base data variable */
                const UA_NodeId variableTypes = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
                /* A variable may be of an object type which again is below BaseObjectType */
                const UA_NodeId objectTypes = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE);
                // TODO handle subtypes of parent reference types
                if(!isNodeInTree(&server->config.nodestore, parentNodeId, &variableTypes,
                                 parentReferences, UA_PARENT_REFERENCES_COUNT) &&
                   !isNodeInTree(&server->config.nodestore, parentNodeId, &objectTypes,
                                 parentReferences, UA_PARENT_REFERENCES_COUNT)) {
                    UA_LOG_INFO_SESSION(server->config.logger, session,
                                        "AddNodes: Type of variable node must "
                                        "be VariableType and not cannot be abstract");
                    retval = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
                    goto cleanup;
                }
            }
        }

        if(node->nodeClass == UA_NODECLASS_OBJECT) {
            if(((const UA_ObjectTypeNode*)type)->isAbstract) {
                /* Object node created of an abstract ObjectType. Only allowed
                 * if within BaseObjectType folder */
                const UA_NodeId objectTypes = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE);
                // TODO handle subtypes of parent reference types
                if(!isNodeInTree(&server->config.nodestore, parentNodeId, &objectTypes,
                                 parentReferences, UA_PARENT_REFERENCES_COUNT)) {
                    UA_LOG_INFO_SESSION(server->config.logger, session,
                                        "AddNodes: Type of object node must "
                                        "be ObjectType and not be abstract");
                    retval = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
                    goto cleanup;
                }
            }
        }
    }

    /* Add reference to the parent */
    if(!UA_NodeId_isNull(parentNodeId)) {
        if(UA_NodeId_isNull(referenceTypeId)) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Reference to parent cannot be null");
            retval = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            goto cleanup;
        }

        retval = addRef(server, session, &node->nodeId, referenceTypeId, parentNodeId, false);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Adding reference to parent failed");
            goto cleanup;
        }
    }

    /* Add a hasTypeDefinition reference */
    if(node->nodeClass == UA_NODECLASS_VARIABLE ||
       node->nodeClass == UA_NODECLASS_OBJECT) {
        UA_assert(type != NULL); /* see above */
        retval = addRef(server, session, &node->nodeId, &hasTypeDefinition, &type->nodeId, true);
        if(retval != UA_STATUSCODE_GOOD)
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Adding a reference to the type "
                                "definition failed with error code %s",
                                UA_StatusCode_name(retval));
    }

 cleanup:
    UA_Nodestore_release(server, node);
    if(type)
        UA_Nodestore_release(server, type);
    return retval;
}

/* Create the node and add it to the nodestore. But don't typecheck and add
 * references so far */
UA_StatusCode
AddNode_raw(UA_Server *server, UA_Session *session, void *nodeContext,
            const UA_AddNodesItem *item, UA_NodeId *outNewNodeId) {
    /* Do not check access for server */
    if(session != &adminSession && server->config.accessControl.allowAddNode &&
       !server->config.accessControl.allowAddNode(server, &server->config.accessControl,
                                                  &session->sessionId, session->sessionHandle, item)) {
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    }

    /* Check the namespaceindex */
    if(item->requestedNewNodeId.nodeId.namespaceIndex >= server->namespacesSize) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Namespace invalid");
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    if(item->nodeAttributes.encoding != UA_EXTENSIONOBJECT_DECODED &&
       item->nodeAttributes.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node attributes invalid");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create a node */
    UA_Node *node = UA_Nodestore_new(server, item->nodeClass);
    if(!node) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node could not create a node "
                            "in the nodestore");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Fill the node attributes */
    node->context = nodeContext;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_NodeId_copy(&item->requestedNewNodeId.nodeId, &node->nodeId);
    retval |= UA_QualifiedName_copy(&item->browseName, &node->browseName);
    retval |= UA_Node_setAttributes(node, item->nodeAttributes.content.decoded.data,
                                    item->nodeAttributes.content.decoded.type);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node could not create a node "
                            "with error code %s", UA_StatusCode_name(retval));
        UA_Nodestore_delete(server, node);
        return retval;
    }

    /* Add the node to the nodestore */
    retval = UA_Nodestore_insert(server, node, outNewNodeId);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node could not add the new node "
                            "to the nodestore with error code %s",
                            UA_StatusCode_name(retval));
    return retval;
}

/* Prepare the node, then add it to the nodestore */
static UA_StatusCode
Operation_addNode_begin(UA_Server *server, UA_Session *session, void *nodeContext,
                        const UA_AddNodesItem *item, const UA_NodeId *parentNodeId,
                        const UA_NodeId *referenceTypeId, UA_NodeId *outNewNodeId) {
    /* Create a temporary NodeId if none is returned */
    UA_NodeId newId;
    if(!outNewNodeId) {
        UA_NodeId_init(&newId);
        outNewNodeId = &newId;
    }

    /* Create the node and add it to the nodestore */
    UA_StatusCode retval = AddNode_raw(server, session, nodeContext, item, outNewNodeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Typecheck and add references to parent and type definition */
    retval = AddNode_addRefs(server, session, outNewNodeId, parentNodeId,
                             referenceTypeId, &item->typeDefinition.nodeId);
    if(retval != UA_STATUSCODE_GOOD)
        UA_Server_deleteNode(server, *outNewNodeId, UA_TRUE);

    if(outNewNodeId == &newId)
        UA_NodeId_deleteMembers(&newId);
    return retval;
}

/* Children, references, type-checking, constructors. */
UA_StatusCode
AddNode_finish(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Get the node */
    const UA_Node *node = UA_Nodestore_get(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    const UA_Node *type = NULL;

    /* Instantiate variables and objects */
    if(node->nodeClass == UA_NODECLASS_VARIABLE ||
       node->nodeClass == UA_NODECLASS_VARIABLETYPE ||
       node->nodeClass == UA_NODECLASS_OBJECT) {
        /* Get the type node */
        type = getNodeType(server, node);
        if(!type) {
            if(server->bootstrapNS0)
                goto constructor;
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Node type not found");
            retval = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            goto cleanup;
        }

        /* Use attributes from the type. The value and value constraints are the
         * same for the variable and variabletype attribute structs. */
        if(node->nodeClass == UA_NODECLASS_VARIABLE ||
           node->nodeClass == UA_NODECLASS_VARIABLETYPE) {
            retval = useVariableTypeAttributes(server, session,
                                               (const UA_VariableNode**)&node,
                                               (const UA_VariableTypeNode*)type);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO_SESSION(server->config.logger, session,
                                    "AddNodes: Using attributes from the variable type "
                                    "failed with error code %s", UA_StatusCode_name(retval));
                goto cleanup;
            }

            /* Check if all attributes hold the constraints of the type now. The initial
             * attributes must type-check. The constructor might change the attributes
             * again. Then, the changes are type-checked by the normal write service. */
            retval = typeCheckVariableNode(server, session, (const UA_VariableNode*)node,
                                           (const UA_VariableTypeNode*)type);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO_SESSION(server->config.logger, session,
                                    "AddNodes: Type-checking the variable node "
                                    "failed with error code %s", UA_StatusCode_name(retval));
                goto cleanup;
            }
        }

        /* Add (mandatory) child nodes from the type definition */
        if(node->nodeClass == UA_NODECLASS_VARIABLE ||
           node->nodeClass == UA_NODECLASS_OBJECT) {
            retval = addChildren(server, session, node, type);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO_SESSION(server->config.logger, session,
                                    "AddNodes: Adding child nodes failed with error code %s",
                                    UA_StatusCode_name(retval));
                goto cleanup;
            }
        }
    }

    /* Call the constructor(s) */
 constructor:
    retval = callConstructors(server, session, node, type);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Calling the node constructor(s) failed "
                            "with status code %s", UA_StatusCode_name(retval));
    }

 cleanup:
    if(type)
        UA_Nodestore_release(server, type);
    if(retval != UA_STATUSCODE_GOOD)
        removeDeconstructedNode(server, session, node, true);
    UA_Nodestore_release(server, node);
    return retval;
}

static void
Operation_addNode(UA_Server *server, UA_Session *session, void *nodeContext,
                  const UA_AddNodesItem *item, UA_AddNodesResult *result) {
    result->statusCode =
        Operation_addNode_begin(server, session, nodeContext, item, &item->parentNodeId.nodeId,
                                &item->referenceTypeId, &result->addedNodeId);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* AddNodes_finish */
    result->statusCode =
        AddNode_finish(server, session, &result->addedNodeId);

    /* If finishing failed, the node was deleted */
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_NodeId_deleteMembers(&result->addedNodeId);
}

void
Service_AddNodes(UA_Server *server, UA_Session *session,
                      const UA_AddNodesRequest *request,
                      UA_AddNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing AddNodesRequest");

    if(server->config.maxNodesPerNodeManagement != 0 &&
       request->nodesToAddSize > server->config.maxNodesPerNodeManagement) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_addNode, NULL,
                                           &request->nodesToAddSize, &UA_TYPES[UA_TYPES_ADDNODESITEM],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_ADDNODESRESULT]);
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
    item.nodeClass = nodeClass;
    item.requestedNewNodeId.nodeId = *requestedNewNodeId;
    item.browseName = browseName;
    item.parentNodeId.nodeId = *parentNodeId;
    item.referenceTypeId = *referenceTypeId;
    item.typeDefinition.nodeId = *typeDefinition;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type = attributeType;
    item.nodeAttributes.content.decoded.data = (void*)(uintptr_t)attr;

    /* Call the normal addnodes service */
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    Operation_addNode(server, &adminSession, nodeContext, &item, &result);
    if(outNewNodeId)
        *outNewNodeId = result.addedNodeId;
    else
        UA_NodeId_deleteMembers(&result.addedNodeId);
    return result.statusCode;
}

UA_StatusCode
UA_Server_addNode_begin(UA_Server *server, const UA_NodeClass nodeClass,
                        const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId,
                        const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName,
                        const UA_NodeId typeDefinition,
                        const void *attr, const UA_DataType *attributeType,
                        void *nodeContext, UA_NodeId *outNewNodeId) {
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.nodeClass = nodeClass;
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    item.typeDefinition.nodeId = typeDefinition;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type = attributeType;
    item.nodeAttributes.content.decoded.data = (void*)(uintptr_t)attr;
    return Operation_addNode_begin(server, &adminSession, nodeContext, &item,
                                   &parentNodeId, &referenceTypeId, outNewNodeId);
}

UA_StatusCode
UA_Server_addNode_finish(UA_Server *server, const UA_NodeId nodeId) {
    return AddNode_finish(server, &adminSession, &nodeId);
}

/****************/
/* Delete Nodes */
/****************/

static void
Operation_deleteReference(UA_Server *server, UA_Session *session, void *context,
                          const UA_DeleteReferencesItem *item, UA_StatusCode *retval);

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
            Operation_deleteReference(server, session, NULL, &item, &dummy);
        }
    }
}

static void
deconstructNode(UA_Server *server, UA_Session *session,
                const UA_Node *node) {
    /* Call the type-level destructor */
    void *context = node->context; /* No longer needed after this function */
    if(node->nodeClass == UA_NODECLASS_OBJECT ||
       node->nodeClass == UA_NODECLASS_VARIABLE) {
        const UA_Node *type = getNodeType(server, node);
        if(type) {
            const UA_NodeTypeLifecycle *lifecycle;
            if(node->nodeClass == UA_NODECLASS_OBJECT)
                lifecycle = &((const UA_ObjectTypeNode*)type)->lifecycle;
            else
                lifecycle = &((const UA_VariableTypeNode*)type)->lifecycle;
            if(lifecycle->destructor)
                lifecycle->destructor(server,
                                      &session->sessionId, session->sessionHandle,
                                      &type->nodeId, type->context,
                                      &node->nodeId, &context);
            UA_Nodestore_release(server, type);
        }
    }

    /* Call the global destructor */
    if(server->config.nodeLifecycle.destructor)
        server->config.nodeLifecycle.destructor(server, &session->sessionId,
                                                session->sessionHandle,
                                                &node->nodeId, context);
}

static void
deleteNodeOperation(UA_Server *server, UA_Session *session, void *context,
                    const UA_DeleteNodesItem *item, UA_StatusCode *result);

static void
removeChildren(UA_Server *server, UA_Session *session,
               const UA_Node *node) {
    /* Browse to get all children of the node */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = node->nodeId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_AGGREGATES);
    bd.includeSubtypes = true;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE | UA_NODECLASS_METHOD;
    bd.resultMask = UA_BROWSERESULTMASK_NONE;

    UA_BrowseResult br;
    UA_BrowseResult_init(&br);
    UA_UInt32 maxrefs = 0;
    Operation_Browse(server, session, &maxrefs, &bd, &br);
    if(br.statusCode != UA_STATUSCODE_GOOD)
        return;

    UA_DeleteNodesItem item;
    item.deleteTargetReferences = true;

    /* Remove every child */
    for(size_t i = 0; i < br.referencesSize; ++i) {
        UA_ReferenceDescription *rd = &br.references[i];
        // check for self-reference to avoid endless loop
        if(UA_NodeId_equal(&node->nodeId, &rd->nodeId.nodeId))
            continue;
        item.nodeId = rd->nodeId.nodeId;
        UA_StatusCode retval;
        deleteNodeOperation(server, session, NULL, &item, &retval);
    }

    UA_BrowseResult_deleteMembers(&br);
}

static void
removeDeconstructedNode(UA_Server *server, UA_Session *session,
                        const UA_Node *node, UA_Boolean removeTargetRefs) {
    /* Remove all children of the node */
    removeChildren(server, session, node);

    /* Remove references to the node (not the references going out, as the node
     * will be deleted anyway) */
    if(removeTargetRefs)
        removeIncomingReferences(server, session, node);

    /* Remove the node in the nodestore */
    UA_Nodestore_remove(server, &node->nodeId);
}

static void
deleteNodeOperation(UA_Server *server, UA_Session *session, void *context,
                    const UA_DeleteNodesItem *item, UA_StatusCode *result) {
    /* Do not check access for server */
    if(session != &adminSession && server->config.accessControl.allowDeleteNode &&
       !server->config.accessControl.allowDeleteNode(server, &server->config.accessControl,
                                                     &session->sessionId, session->sessionHandle, item)) {
        *result = UA_STATUSCODE_BADUSERACCESSDENIED;
        return;
    }

    const UA_Node *node = UA_Nodestore_get(server, &item->nodeId);
    if(!node) {
        *result = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    if(UA_Node_hasSubTypeOrInstances(node)) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "Delete Nodes: Cannot delete a type node "
                            "with active instances or subtypes");
        UA_Nodestore_release(server, node);
        *result = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* TODO: Check if the information model consistency is violated */
    /* TODO: Check if the node is a mandatory child of a parent */

    deconstructNode(server, session, node);
    removeDeconstructedNode(server, session, node, item->deleteTargetReferences);
    UA_Nodestore_release(server, node);
}

void Service_DeleteNodes(UA_Server *server, UA_Session *session,
                         const UA_DeleteNodesRequest *request,
                         UA_DeleteNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing DeleteNodesRequest");

    if(server->config.maxNodesPerNodeManagement != 0 &&
       request->nodesToDeleteSize > server->config.maxNodesPerNodeManagement) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)deleteNodeOperation, NULL,
                                           &request->nodesToDeleteSize, &UA_TYPES[UA_TYPES_DELETENODESITEM],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

UA_StatusCode
UA_Server_deleteNode(UA_Server *server, const UA_NodeId nodeId,
                     UA_Boolean deleteReferences) {
    UA_DeleteNodesItem item;
    item.deleteTargetReferences = deleteReferences;
    item.nodeId = nodeId;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    deleteNodeOperation(server, &adminSession, NULL, &item, &retval);
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
Operation_addReference(UA_Server *server, UA_Session *session, void *context,
                       const UA_AddReferencesItem *item, UA_StatusCode *retval) {
    /* Do not check access for server */
    if(session != &adminSession && server->config.accessControl.allowAddReference &&
       !server->config.accessControl.allowAddReference(server, &server->config.accessControl,
                                                       &session->sessionId, session->sessionHandle, item)) {
        *retval = UA_STATUSCODE_BADUSERACCESSDENIED;
        return;
    }

    /* Currently no expandednodeids are allowed */
    if(item->targetServerUri.length > 0) {
        *retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
        return;
    }

    /* Add the first direction */
    *retval = UA_Server_editNode(server, session, &item->sourceNodeId,
                                 (UA_EditNodeCallback)addOneWayReference,
                                 /* cast away const because callback uses const anyway */
                                 (UA_AddReferencesItem *)(uintptr_t)item);
    UA_Boolean firstExisted = UA_FALSE;
    if(*retval == UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED) {
        *retval = UA_STATUSCODE_GOOD;
        firstExisted = UA_TRUE;
    } else if(*retval != UA_STATUSCODE_GOOD)
        return;

    /* Add the second direction */
    UA_AddReferencesItem secondItem;
    UA_AddReferencesItem_init(&secondItem);
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.referenceTypeId = item->referenceTypeId;
    secondItem.isForward = !item->isForward;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    /* keep default secondItem.targetNodeClass = UA_NODECLASS_UNSPECIFIED */
    *retval = UA_Server_editNode(server, session, &secondItem.sourceNodeId,
                                 (UA_EditNodeCallback)addOneWayReference, &secondItem);

    /* remove reference if the second direction failed */
    UA_Boolean secondExisted = UA_FALSE;
    if(*retval == UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED) {
        *retval = UA_STATUSCODE_GOOD;
        secondExisted = UA_TRUE;
    } else if(*retval != UA_STATUSCODE_GOOD && !firstExisted) {
        UA_DeleteReferencesItem deleteItem;
        deleteItem.sourceNodeId = item->sourceNodeId;
        deleteItem.referenceTypeId = item->referenceTypeId;
        deleteItem.isForward = item->isForward;
        deleteItem.targetNodeId = item->targetNodeId;
        deleteItem.deleteBidirectional = false;
        /* ignore returned status code */
        UA_Server_editNode(server, session, &item->sourceNodeId,
                           (UA_EditNodeCallback)deleteOneWayReference, &deleteItem);
    }

    /* Calculate common duplicate reference not allowed result and set bad result
     * if BOTH directions already existed */
    if(firstExisted && secondExisted)
        *retval = UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED;
}

void Service_AddReferences(UA_Server *server, UA_Session *session,
                           const UA_AddReferencesRequest *request,
                           UA_AddReferencesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing AddReferencesRequest");

    if(server->config.maxNodesPerNodeManagement != 0 &&
       request->referencesToAddSize > server->config.maxNodesPerNodeManagement) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_addReference, NULL,
                                           &request->referencesToAddSize, &UA_TYPES[UA_TYPES_ADDREFERENCESITEM],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
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
    Operation_addReference(server, &adminSession, NULL, &item, &retval);
    return retval;
}

/*********************/
/* Delete References */
/*********************/

static void
Operation_deleteReference(UA_Server *server, UA_Session *session, void *context,
                          const UA_DeleteReferencesItem *item, UA_StatusCode *retval) {
    /* Do not check access for server */
    if(session != &adminSession && server->config.accessControl.allowDeleteReference &&
       !server->config.accessControl.allowDeleteReference(server, &server->config.accessControl,
                                                          &session->sessionId, session->sessionHandle, item)) {
        *retval = UA_STATUSCODE_BADUSERACCESSDENIED;
        return;
    }

    // TODO: Check consistency constraints, remove the references.
    *retval = UA_Server_editNode(server, session, &item->sourceNodeId,
                                 (UA_EditNodeCallback)deleteOneWayReference,
                                 /* cast away const qualifier because callback uses it anyway */
                                 (UA_DeleteReferencesItem *)(uintptr_t)item);
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

    if(server->config.maxNodesPerNodeManagement != 0 &&
       request->referencesToDeleteSize > server->config.maxNodesPerNodeManagement) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_deleteReference, NULL,
                                           &request->referencesToDeleteSize, &UA_TYPES[UA_TYPES_DELETEREFERENCESITEM],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
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
    Operation_deleteReference(server, &adminSession, NULL, &item, &retval);
    return retval;
}

/**********************/
/* Set Value Callback */
/**********************/

static UA_StatusCode
setValueCallback(UA_Server *server, UA_Session *session,
                 UA_VariableNode *node, const UA_ValueCallback *callback) {
    if(node->nodeClass != UA_NODECLASS_VARIABLE)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    node->value.data.callback = *callback;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setVariableNode_valueCallback(UA_Server *server,
                                        const UA_NodeId nodeId,
                                        const UA_ValueCallback callback) {
    return UA_Server_editNode(server, &adminSession, &nodeId,
                              (UA_EditNodeCallback)setValueCallback,
                              /* cast away const because callback uses const anyway */
                              (UA_ValueCallback *)(uintptr_t) &callback);
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
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.nodeClass = UA_NODECLASS_VARIABLE;
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    UA_ExpandedNodeId typeDefinitionId;
    UA_ExpandedNodeId_init(&typeDefinitionId);
    typeDefinitionId.nodeId = typeDefinition;
    item.typeDefinition = typeDefinitionId;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.data = (void*)(uintptr_t)&attr;
    item.nodeAttributes.content.decoded.type = &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES];
    UA_NodeId newNodeId;
    if(!outNewNodeId) {
        newNodeId = UA_NODEID_NULL;
        outNewNodeId = &newNodeId;
    }

    /* Create the node and add it to the nodestore */
    UA_StatusCode retval = AddNode_raw(server, &adminSession, nodeContext, &item, outNewNodeId);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Set the data source */
    retval = UA_Server_setVariableNode_dataSource(server, *outNewNodeId, dataSource);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Typecheck and add references to parent and type definition */
    retval = AddNode_addRefs(server, &adminSession, outNewNodeId, &parentNodeId,
                             &referenceTypeId, &typeDefinition);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Call the constructors */
    retval = AddNode_finish(server, &adminSession, outNewNodeId);

 cleanup:
    if(outNewNodeId == &newNodeId)
        UA_NodeId_deleteMembers(&newNodeId);

    return retval;
}

static UA_StatusCode
setDataSource(UA_Server *server, UA_Session *session,
              UA_VariableNode* node, const UA_DataSource *dataSource) {
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
    return UA_Server_editNode(server, &adminSession, &nodeId,
                              (UA_EditNodeCallback)setDataSource,
                            /* casting away const because callback casts it back anyway */
                              (UA_DataSource *) (uintptr_t)&dataSource);
}

/************************************/
/* Special Handling of Method Nodes */
/************************************/

#ifdef UA_ENABLE_METHODCALLS

static const UA_NodeId hasproperty = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASPROPERTY}};
static const UA_NodeId propertytype = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_PROPERTYTYPE}};

static UA_StatusCode
UA_Server_addMethodNodeEx_finish(UA_Server *server, const UA_NodeId nodeId,
                                 UA_MethodCallback method,
                                 const size_t inputArgumentsSize, const UA_Argument *inputArguments,
                                 const UA_NodeId inputArgumentsRequestedNewNodeId,
                                 UA_NodeId *inputArgumentsOutNewNodeId,
                                 const size_t outputArgumentsSize, const UA_Argument *outputArguments,
                                 const UA_NodeId outputArgumentsRequestedNewNodeId,
                                 UA_NodeId *outputArgumentsOutNewNodeId) {
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
    UA_UInt32 maxrefs = 0;
    Operation_Browse(server, &adminSession, &maxrefs, &bd, &br);

    UA_StatusCode retval = br.statusCode;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, nodeId, true);
        UA_BrowseResult_deleteMembers(&br);
        return retval;
    }

    /* Filter out the argument nodes */
    UA_NodeId inputArgsId = UA_NODEID_NULL;
    UA_NodeId outputArgsId = UA_NODEID_NULL;
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
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        char *name = "InputArguments";
        attr.displayName = UA_LOCALIZEDTEXT("", name);
        attr.dataType = UA_TYPES[UA_TYPES_ARGUMENT].typeId;
        attr.valueRank = 1;
        UA_UInt32 inputArgsSize32 = (UA_UInt32)inputArgumentsSize;
        attr.arrayDimensions = &inputArgsSize32;
        attr.arrayDimensionsSize = 1;
        UA_Variant_setArray(&attr.value, (void*)(uintptr_t) inputArguments,
                            inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
        retval |= UA_Server_addVariableNode(server, inputArgumentsRequestedNewNodeId, nodeId,
                                            hasproperty, UA_QUALIFIEDNAME(0, name),
                                            propertytype, attr, NULL, &inputArgsId);
    }

    /* Add the Output Arguments VariableNode */
    if(outputArgumentsSize > 0 && UA_NodeId_isNull(&outputArgsId)) {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        char *name = "OutputArguments";
        attr.displayName = UA_LOCALIZEDTEXT("", name);
        attr.dataType = UA_TYPES[UA_TYPES_ARGUMENT].typeId;
        attr.valueRank = 1;
        UA_UInt32 outputArgsSize32 = (UA_UInt32)outputArgumentsSize;
        attr.arrayDimensions = &outputArgsSize32;
        attr.arrayDimensionsSize = 1;
        UA_Variant_setArray(&attr.value, (void*)(uintptr_t) outputArguments,
                            outputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
        retval |= UA_Server_addVariableNode(server, outputArgumentsRequestedNewNodeId, nodeId,
                                            hasproperty, UA_QUALIFIEDNAME(0, name),
                                            propertytype, attr, NULL, &outputArgsId);
    }

    retval |= UA_Server_setMethodNode_callback(server, nodeId, method);

    /* Call finish to add the parent reference */
    retval |= AddNode_finish(server, &adminSession, &nodeId);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, nodeId, true);
        UA_Server_deleteNode(server, inputArgsId, true);
        UA_Server_deleteNode(server, outputArgsId, true);
    } else {
        if(inputArgumentsOutNewNodeId != NULL) {
            UA_NodeId_copy(&inputArgsId, inputArgumentsOutNewNodeId);
    }
        if(outputArgumentsOutNewNodeId != NULL) {
            UA_NodeId_copy(&outputArgsId, outputArgumentsOutNewNodeId);
        }
    }
    UA_BrowseResult_deleteMembers(&br);
    return retval;
}

UA_StatusCode
UA_Server_addMethodNode_finish(UA_Server *server, const UA_NodeId nodeId,
                               UA_MethodCallback method,
                               size_t inputArgumentsSize, const UA_Argument* inputArguments,
                               size_t outputArgumentsSize, const UA_Argument* outputArguments) {
    return UA_Server_addMethodNodeEx_finish(server, nodeId, method,
                                            inputArgumentsSize, inputArguments, UA_NODEID_NULL, NULL,
                                            outputArgumentsSize, outputArguments, UA_NODEID_NULL, NULL);
}

UA_StatusCode
UA_Server_addMethodNodeEx(UA_Server *server, const UA_NodeId requestedNewNodeId,
                          const UA_NodeId parentNodeId,
                          const UA_NodeId referenceTypeId,
                          const UA_QualifiedName browseName,
                          const UA_MethodAttributes attr, UA_MethodCallback method,
                          size_t inputArgumentsSize, const UA_Argument *inputArguments,
                          const UA_NodeId inputArgumentsRequestedNewNodeId,
                          UA_NodeId *inputArgumentsOutNewNodeId,
                          size_t outputArgumentsSize, const UA_Argument *outputArguments,
                          const UA_NodeId outputArgumentsRequestedNewNodeId,
                          UA_NodeId *outputArgumentsOutNewNodeId,
                          void *nodeContext, UA_NodeId *outNewNodeId) {
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.nodeClass = UA_NODECLASS_METHOD;
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.data = (void*)(uintptr_t)&attr;
    item.nodeAttributes.content.decoded.type = &UA_TYPES[UA_TYPES_METHODATTRIBUTES];

    UA_NodeId newId;
    if(!outNewNodeId) {
        UA_NodeId_init(&newId);
        outNewNodeId = &newId;
    }

    UA_StatusCode retval = Operation_addNode_begin(server, &adminSession, nodeContext,
                                                   &item, &parentNodeId, &referenceTypeId, outNewNodeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_Server_addMethodNodeEx_finish(server, *outNewNodeId, method,
                                              inputArgumentsSize, inputArguments,
                                              inputArgumentsRequestedNewNodeId,
                                              inputArgumentsOutNewNodeId,
                                              outputArgumentsSize, outputArguments,
                                              outputArgumentsRequestedNewNodeId,
                                              outputArgumentsOutNewNodeId);

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
    return UA_Server_editNode(server, &adminSession, &methodNodeId,
                              (UA_EditNodeCallback)editMethodCallback,
                              (void*)(uintptr_t)methodCallback);
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
    return UA_Server_editNode(server, &adminSession, &nodeId,
                              (UA_EditNodeCallback)setNodeTypeLifecycle,
                              &lifecycle);
}
