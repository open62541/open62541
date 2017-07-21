/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_types_generated.h"
#include "ua_types_generated_handling.h"

/************************/
/* Forward Declarations */
/************************/

static UA_StatusCode
copyChildNodes(UA_Server *server, UA_Session *session, 
               const UA_NodeId *sourceNodeId, const UA_NodeId *destinationNodeId, 
               UA_InstantiationCallback *instantiationCallback);

static UA_StatusCode
Service_AddNode_finish(UA_Server *server, UA_Session *session,
                       const UA_NodeId *nodeId, const UA_NodeId *parentNodeId,
                       const UA_NodeId *referenceTypeId,
                       const UA_NodeId *typeDefinition,
                       UA_InstantiationCallback *instantiationCallback);

static UA_StatusCode
deleteNode(UA_Server *server, UA_Session *session,
           const UA_NodeId *nodeId, UA_Boolean deleteReferences);

static void
addReference(UA_Server *server, UA_Session *session,
             const UA_AddReferencesItem *item,
             UA_StatusCode *retval);

static UA_StatusCode
deleteOneWayReference(UA_Server *server, UA_Session *session, UA_Node *node,
                      const UA_DeleteReferencesItem *item);

static void
deleteReference(UA_Server *server, UA_Session *session,
                const UA_DeleteReferencesItem *item, UA_StatusCode *retval);

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

    /* See if the parent exists */
    const UA_Node *parent = UA_NodeStore_get(server->nodestore, parentNodeId);
    if(!parent) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Parent node not found", NULL);
        return UA_STATUSCODE_BADPARENTNODEIDINVALID;
    }

    /* Check the referencetype exists */
    const UA_ReferenceTypeNode *referenceType =
        (const UA_ReferenceTypeNode*)UA_NodeStore_get(server->nodestore, referenceTypeId);
    if(!referenceType) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Reference type to the parent not found", NULL);
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    /* Check if the referencetype is a reference type node */
    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Reference type to the parent invalid", NULL);
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    /* Check that the reference type is not abstract */
    if(referenceType->isAbstract == true) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Abstract reference type to the parent not allowed", NULL);
        return UA_STATUSCODE_BADREFERENCENOTALLOWED;
    }

    /* Check hassubtype relation for type nodes */
    const UA_NodeId subtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    if(nodeClass == UA_NODECLASS_DATATYPE ||
       nodeClass == UA_NODECLASS_VARIABLETYPE ||
       nodeClass == UA_NODECLASS_OBJECTTYPE ||
       nodeClass == UA_NODECLASS_REFERENCETYPE) {
        /* type needs hassubtype reference to the supertype */
        if(!UA_NodeId_equal(referenceTypeId, &subtypeId)) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: New type node need to have a "
                                "HasSubType reference", NULL);
            return UA_STATUSCODE_BADREFERENCENOTALLOWED;
        }
        /* supertype needs to be of the same node type  */
        if(parent->nodeClass != nodeClass) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: New type node needs to be of the same "
                                "node type as the parent", NULL);
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
                            "AddNodes: Reference type is not hierarchical", NULL);
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
typeCheckVariableNodeWithValue(UA_Server *server, UA_Session *session,
                               const UA_VariableNode *node,
                               const UA_VariableTypeNode *vt,
                               UA_DataValue *value) {
    /* Check the datatype against the vt */
    if(!compatibleDataType(server, &node->dataType, &vt->dataType))
        return UA_STATUSCODE_BADTYPEMISMATCH;

    /* Get the array dimensions */
    size_t arrayDims = node->arrayDimensionsSize;
    if(arrayDims == 0 && value->hasValue && value->value.type &&
       !UA_Variant_isScalar(&value->value)) {
        arrayDims = 1; /* No array dimensions on an array implies one dimension */
    }

    /* Check valueRank against array dimensions */
    UA_StatusCode retval = compatibleValueRankArrayDimensions(node->valueRank, arrayDims);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Check valueRank against the vt */
    retval = compatibleValueRanks(node->valueRank, vt->valueRank);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Check array dimensions against the vt */
    retval = compatibleArrayDimensions(vt->arrayDimensionsSize, vt->arrayDimensions,
                                       node->arrayDimensionsSize, node->arrayDimensions);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Typecheck the value */
    if(value->hasValue) {
        retval = typeCheckValue(server, &node->dataType, node->valueRank,
                                node->arrayDimensionsSize, node->arrayDimensions,
                                &value->value, NULL, NULL);
        /* The type-check failed. Write the same value again. The write-service
         * tries to convert to the correct type... */
        if(retval != UA_STATUSCODE_GOOD) {
            UA_RCU_UNLOCK();
            retval = UA_Server_writeValue(server, node->nodeId, value->value);
            UA_RCU_LOCK();
        }
    }
    return retval;
}

/* Check the consistency of the variable (or variable type) attributes data
 * type, value rank, array dimensions internally and against the parent variable
 * type. */
static UA_StatusCode
typeCheckVariableNode(UA_Server *server, UA_Session *session,
                      const UA_VariableNode *node,
                      const UA_NodeId *typeDef) {
    /* Get the variable type */
    const UA_VariableTypeNode *vt =
        (const UA_VariableTypeNode*)UA_NodeStore_get(server->nodestore, typeDef);
    if(!vt || vt->nodeClass != UA_NODECLASS_VARIABLETYPE)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    if(node->nodeClass == UA_NODECLASS_VARIABLE && vt->isAbstract)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;

    /* We need the value for some checks. Might come from a datasource, so we perform a
     * regular read. */
    UA_DataValue value;
    UA_DataValue_init(&value);
    UA_StatusCode retval = readValueAttribute(server, node, &value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Fix the variable: Set a sane valueRank if required (the most permissive -2) */
    if(node->valueRank == 0 &&
       (!value.hasValue || !value.value.type || UA_Variant_isScalar(&value.value))) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Use a default ValueRank of -2", NULL);
        UA_RCU_UNLOCK();
        retval = UA_Server_writeValueRank(server, node->nodeId, -2);
        UA_RCU_LOCK();
        if(retval != UA_STATUSCODE_GOOD) {
            UA_DataValue_deleteMembers(&value);
            return retval;
        }
    }

    /* If no value is set, see if the vt provides one and copy that. This needs
     * to be done before copying the datatype from the vt, as setting the datatype
     * triggers a typecheck. */
    if(!value.hasValue || !value.value.type) {
        retval = readValueAttribute(server, (const UA_VariableNode*)vt, &value);
        if(retval == UA_STATUSCODE_GOOD && value.hasValue && value.value.type) {
            UA_RCU_UNLOCK();
            retval = UA_Server_writeValue(server, node->nodeId, value.value);
            UA_RCU_LOCK();
        }
        if(retval != UA_STATUSCODE_GOOD) {
            UA_DataValue_deleteMembers(&value);
            return retval;
        }
    }

    /* Fix the variable: If no datatype is given, use the datatype of the vt */
    if(UA_NodeId_isNull(&node->dataType)) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: "
                            "Use a default DataType (from the TypeDefinition)", NULL);
        UA_RCU_UNLOCK();
        retval = UA_Server_writeDataType(server, node->nodeId, vt->dataType);
        UA_RCU_LOCK();
        if(retval != UA_STATUSCODE_GOOD) {
            UA_DataValue_deleteMembers(&value);
            return retval;
        }
    }

#ifdef UA_ENABLE_MULTITHREADING
    /* Re-read the node to get the changes */
    node = (const UA_VariableNode*)UA_NodeStore_get(server->nodestore, &node->nodeId);
#endif

    retval = typeCheckVariableNodeWithValue(server, session, node, vt, &value);
    UA_DataValue_deleteMembers(&value);
    return retval;
}

/********************/
/* Instantiate Node */
/********************/

/* Returns an array with all subtype nodeids (including the root). Supertypes
 * need to have the same nodeClass as root and are (recursively) related with a
 * hasSubType reference. Since multi-inheritance is possible, we test for
 * duplicates and return evey nodeid at most once. */
static UA_StatusCode
getTypeHierarchy(UA_NodeStore *ns, const UA_Node *rootRef,
                 UA_NodeId **typeHierarchy, size_t *typeHierarchySize) {
    size_t results_size = 20; // probably too big, but saves mallocs
    UA_NodeId *results = (UA_NodeId*)UA_malloc(sizeof(UA_NodeId) * results_size);
    if(!results)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval = UA_NodeId_copy(&rootRef->nodeId, &results[0]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(results);
        return retval;
    }

    const UA_Node *node = rootRef;
    size_t idx = 0; /* Current index (contains NodeId of node) */
    size_t last = 0; /* Index of the last element in the array */
    const UA_NodeId hasSubtypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    while(true) {
        for(size_t i = 0; i < node->referencesSize; ++i) {
            UA_NodeReferenceKind *refs = &node->references[i];
            /* Are the references relevant? */
            if(!refs->isInverse)
                continue;
            if(!UA_NodeId_equal(&hasSubtypeNodeId, &refs->referenceTypeId))
                continue;

            for(size_t j = 0; j < refs->targetIdsSize; ++j) {
                UA_NodeId *targetId = &refs->targetIds[j].nodeId;

                /* Is the target already considered? (multi-inheritance) */
                UA_Boolean duplicate = false;
                for(size_t k = 0; k <= last; ++k) {
                    if(UA_NodeId_equal(targetId, &results[k])) {
                        duplicate = true;
                        break;
                    }
                }
                if(duplicate)
                    continue;

                /* Increase array length if necessary */
                if(last + 1 >= results_size) {
                    UA_NodeId *new_results =
                        (UA_NodeId*)UA_realloc(results, sizeof(UA_NodeId) * results_size * 2);
                    if(!new_results) {
                        UA_Array_delete(results, last, &UA_TYPES[UA_TYPES_NODEID]);
                        return UA_STATUSCODE_BADOUTOFMEMORY;
                    }
                    results = new_results;
                    results_size *= 2;
                }

                /* Copy new nodeid to the end of the list */
                retval = UA_NodeId_copy(targetId, &results[++last]);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_Array_delete(results, last, &UA_TYPES[UA_TYPES_NODEID]);
                    return retval;
                }
            }
        }

        /* Get the next node */
    next:
        ++idx;
        if(idx > last || retval != UA_STATUSCODE_GOOD)
            break;
        node = UA_NodeStore_get(ns, &results[idx]);
        if(!node || node->nodeClass != rootRef->nodeClass)
            goto next;
    }

    *typeHierarchy = results;
    *typeHierarchySize = last + 1;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setObjectInstanceHandle(UA_Server *server, UA_Session *session, UA_ObjectNode* node,
                        void * (*constructor)(const UA_NodeId instance)) {
    if(node->nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    if(!node->instanceHandle)
        node->instanceHandle = constructor(node->nodeId);
    return UA_STATUSCODE_GOOD;
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

static UA_Boolean
isMandatoryChild(UA_Server *server, UA_Session *session, const UA_NodeId *childNodeId) {
    const UA_NodeId mandatoryId = UA_NODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY);
    const UA_NodeId hasModellingRuleId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE);

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
copyChildNode(UA_Server *server, UA_Session *session,
              const UA_NodeId *destinationNodeId, 
              const UA_ReferenceDescription *rd,
              UA_InstantiationCallback *instantiationCallback) {
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
            retval = copyChildNodes(server, session, &rd->nodeId.nodeId,
                                    &existingChild, instantiationCallback);
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
    } else if(rd->nodeClass == UA_NODECLASS_VARIABLE ||
              rd->nodeClass == UA_NODECLASS_OBJECT) {
        /* Copy the node */
        UA_Node *node = UA_NodeStore_getCopy(server->nodestore, &rd->nodeId.nodeId);
        if(!node)
            return UA_STATUSCODE_BADNODEIDINVALID;

        /* Get the type */
        UA_NodeId typeId;
        getNodeType(server, node, &typeId);

        /* Reset the NodeId (random numeric id will be assigned in the nodestore) */
        UA_NodeId_deleteMembers(&node->nodeId);
        node->nodeId.namespaceIndex = destinationNodeId->namespaceIndex;

        /* Remove references, they are re-created from scratch in addnode_finish */
        /* TODO: Be more clever in removing references that are re-added during
         * addnode_finish. That way, we can call addnode_finish also on children that were
         * manually added by the user during addnode_begin and addnode_finish. */
        UA_Node_deleteReferences(node);
                
        /* Add the node to the nodestore */
        retval = UA_NodeStore_insert(server->nodestore, node);

        /* Call addnode_finish, this recursively adds members, the type definition and so on */
        if(retval == UA_STATUSCODE_GOOD)
            retval = Service_AddNode_finish(server, session, &node->nodeId,
                                            destinationNodeId, &rd->referenceTypeId,
                                            &typeId, instantiationCallback);

        /* Clean up */
        UA_NodeId_deleteMembers(&typeId);
    }
    return retval;
}

/* Copy any children of Node sourceNodeId to another node destinationNodeId. */
static UA_StatusCode
copyChildNodes(UA_Server *server, UA_Session *session, 
               const UA_NodeId *sourceNodeId, const UA_NodeId *destinationNodeId, 
               UA_InstantiationCallback *instantiationCallback) {
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
        retval |= copyChildNode(server, session, destinationNodeId, 
                                rd, instantiationCallback);
    }
    UA_BrowseResult_deleteMembers(&br);
    return retval;
}

static UA_StatusCode
instantiateNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                UA_NodeClass nodeClass, const UA_NodeId *typeId,
                UA_InstantiationCallback *instantiationCallback) {
    /* Currently, only variables and objects are instantiated */
    if(nodeClass != UA_NODECLASS_VARIABLE &&
       nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_GOOD;
        
    /* Get the type node */
    UA_ASSERT_RCU_LOCKED();
    const UA_Node *typenode = UA_NodeStore_get(server->nodestore, typeId);
    if(!typenode)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;

    /* See if the type has the correct node class */
    if(nodeClass == UA_NODECLASS_VARIABLE) {
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
    UA_StatusCode retval = getTypeHierarchy(server->nodestore, typenode,
                                            &hierarchy, &hierarchySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    
    /* Copy members of the type and supertypes */
    for(size_t i = 0; i < hierarchySize; ++i)
        retval |= copyChildNodes(server, session, &hierarchy[i],
                                 nodeId, instantiationCallback);
    UA_Array_delete(hierarchy, hierarchySize, &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Call the object constructor */
    if(typenode->nodeClass == UA_NODECLASS_OBJECTTYPE) {
        const UA_ObjectLifecycleManagement *olm =
            &((const UA_ObjectTypeNode*)typenode)->lifecycleManagement;
        if(olm->constructor) {
            UA_RCU_UNLOCK();
            UA_Server_editNode(server, session, nodeId,
                               (UA_EditNodeCallback)setObjectInstanceHandle,
                               (void*)(uintptr_t)olm->constructor);
            UA_RCU_LOCK();
        }
    }

    /* Add a hasType reference */
    UA_AddReferencesItem addref;
    UA_AddReferencesItem_init(&addref);
    addref.sourceNodeId = *nodeId;
    addref.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    addref.isForward = true;
    addref.targetNodeId.nodeId = *typeId;
    addReference(server, session, &addref, &retval);

    /* Call custom callback */
    if(retval == UA_STATUSCODE_GOOD && instantiationCallback)
        instantiationCallback->method(*nodeId, *typeId, instantiationCallback->handle);
    return retval;
}

/***************************************/
/* Set node from attribute description */
/***************************************/

static UA_StatusCode
copyStandardAttributes(UA_Node *node, const UA_AddNodesItem *item,
                       const UA_NodeAttributes *attr) {
    UA_StatusCode retval;
    retval  = UA_NodeId_copy(&item->requestedNewNodeId.nodeId, &node->nodeId);
    retval |= UA_QualifiedName_copy(&item->browseName, &node->browseName);
    retval |= UA_LocalizedText_copy(&attr->displayName, &node->displayName);
    retval |= UA_LocalizedText_copy(&attr->description, &node->description);
    node->writeMask = attr->writeMask;
    return retval;
}

static UA_StatusCode
copyCommonVariableAttributes(UA_VariableNode *node,
                             const UA_VariableAttributes *attr) {
    /* Copy the array dimensions */
    UA_StatusCode retval =
        UA_Array_copy(attr->arrayDimensions, attr->arrayDimensionsSize,
                      (void**)&node->arrayDimensions, &UA_TYPES[UA_TYPES_UINT32]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    node->arrayDimensionsSize = attr->arrayDimensionsSize;

    /* Data type and value rank */
    retval |= UA_NodeId_copy(&attr->dataType, &node->dataType);
    node->valueRank = attr->valueRank;

    /* Copy the value */
    node->valueSource = UA_VALUESOURCE_DATA;
    retval |= UA_Variant_copy(&attr->value, &node->value.data.value.value);
    node->value.data.value.hasValue = true;

    return retval;
}

static UA_StatusCode
copyVariableNodeAttributes(UA_VariableNode *vnode,
                           const UA_VariableAttributes *attr) {
    vnode->accessLevel = attr->accessLevel;
    vnode->historizing = attr->historizing;
    vnode->minimumSamplingInterval = attr->minimumSamplingInterval;
    return copyCommonVariableAttributes(vnode, attr);
}

static UA_StatusCode
copyVariableTypeNodeAttributes(UA_VariableTypeNode *vtnode,
                               const UA_VariableTypeAttributes *attr) {
    vtnode->isAbstract = attr->isAbstract;
    return copyCommonVariableAttributes((UA_VariableNode*)vtnode,
                                        (const UA_VariableAttributes*)attr);
}

static UA_StatusCode
copyObjectNodeAttributes(UA_ObjectNode *onode, const UA_ObjectAttributes *attr) {
    onode->eventNotifier = attr->eventNotifier;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
copyReferenceTypeNodeAttributes(UA_ReferenceTypeNode *rtnode,
                                const UA_ReferenceTypeAttributes *attr) {
    rtnode->isAbstract = attr->isAbstract;
    rtnode->symmetric = attr->symmetric;
    return UA_LocalizedText_copy(&attr->inverseName, &rtnode->inverseName);
}

static UA_StatusCode
copyObjectTypeNodeAttributes(UA_ObjectTypeNode *otnode,
                             const UA_ObjectTypeAttributes *attr) {
    otnode->isAbstract = attr->isAbstract;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
copyViewNodeAttributes(UA_ViewNode *vnode, const UA_ViewAttributes *attr) {
    vnode->containsNoLoops = attr->containsNoLoops;
    vnode->eventNotifier = attr->eventNotifier;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
copyDataTypeNodeAttributes(UA_DataTypeNode *dtnode,
                           const UA_DataTypeAttributes *attr) {
    dtnode->isAbstract = attr->isAbstract;
    return UA_STATUSCODE_GOOD;
}

#define CHECK_ATTRIBUTES(TYPE)                                          \
    if(item->nodeAttributes.content.decoded.type != &UA_TYPES[UA_TYPES_##TYPE]) { \
        retval = UA_STATUSCODE_BADNODEATTRIBUTESINVALID;                \
        break;                                                          \
    }

/* Copy the attributes into a new node. On success, newNode points to the
 * created node */
static UA_StatusCode
createNodeFromAttributes(const UA_AddNodesItem *item, UA_Node **newNode) {
    /* Check that we can read the attributes */
    if(item->nodeAttributes.encoding < UA_EXTENSIONOBJECT_DECODED ||
       !item->nodeAttributes.content.decoded.type)
        return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;

    /* Create the node */
    // todo: error case where the nodeclass is faulty should return a different
    // status code
    UA_Node *node = UA_NodeStore_newNode(item->nodeClass);
    if(!node)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Copy the attributes into the node */
    void *data = item->nodeAttributes.content.decoded.data;
    UA_StatusCode retval = copyStandardAttributes(node, item,
                                                  (const UA_NodeAttributes*)data);
    switch(item->nodeClass) {
    case UA_NODECLASS_OBJECT:
        CHECK_ATTRIBUTES(OBJECTATTRIBUTES);
        retval |= copyObjectNodeAttributes((UA_ObjectNode*)node,
                                           (const UA_ObjectAttributes*)data);
        break;
    case UA_NODECLASS_VARIABLE:
        CHECK_ATTRIBUTES(VARIABLEATTRIBUTES);
        retval |= copyVariableNodeAttributes((UA_VariableNode*)node,
                                             (const UA_VariableAttributes*)data);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        CHECK_ATTRIBUTES(OBJECTTYPEATTRIBUTES);
        retval |= copyObjectTypeNodeAttributes((UA_ObjectTypeNode*)node,
                                               (const UA_ObjectTypeAttributes*)data);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        CHECK_ATTRIBUTES(VARIABLETYPEATTRIBUTES);
        retval |= copyVariableTypeNodeAttributes((UA_VariableTypeNode*)node,
                                                 (const UA_VariableTypeAttributes*)data);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        CHECK_ATTRIBUTES(REFERENCETYPEATTRIBUTES);
        retval |= copyReferenceTypeNodeAttributes((UA_ReferenceTypeNode*)node,
                                                  (const UA_ReferenceTypeAttributes*)data);
        break;
    case UA_NODECLASS_DATATYPE:
        CHECK_ATTRIBUTES(DATATYPEATTRIBUTES);
        retval |= copyDataTypeNodeAttributes((UA_DataTypeNode*)node,
                                             (const UA_DataTypeAttributes*)data);
        break;
    case UA_NODECLASS_VIEW:
        CHECK_ATTRIBUTES(VIEWATTRIBUTES);
        retval |= copyViewNodeAttributes((UA_ViewNode*)node,
                                         (const UA_ViewAttributes*)data);
        break;
    case UA_NODECLASS_METHOD:
    case UA_NODECLASS_UNSPECIFIED:
    default:
        retval = UA_STATUSCODE_BADNODECLASSINVALID;
    }

    if(retval == UA_STATUSCODE_GOOD)
        *newNode = (UA_Node*)node;
    else
        UA_NodeStore_deleteNode(node);

    return retval;
}

/************/
/* Add Node */
/************/

static void
Service_AddNode_begin(UA_Server *server, UA_Session *session,
                      const UA_AddNodesItem *item, UA_AddNodesResult *result) {
    /* Check the namespaceindex */
    if(item->requestedNewNodeId.nodeId.namespaceIndex >= server->namespacesSize) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Namespace invalid", NULL);
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return;
    }

    /* Add the node to the nodestore */
    UA_Node *node = NULL;
    result->statusCode = createNodeFromAttributes(item, &node);
    if(result->statusCode == UA_STATUSCODE_GOOD)
        result->statusCode = UA_NodeStore_insert(server->nodestore, node);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node could not be added to the "
                            "nodestore with error code %s",
                            UA_StatusCode_name(result->statusCode));
        return;
    }

    /* Copy the nodeid of the new node */
    result->statusCode = UA_NodeId_copy(&node->nodeId, &result->addedNodeId);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Could not copy the nodeid", NULL);
        deleteNode(server, &adminSession, &node->nodeId, true);
    }
}

static UA_StatusCode
Service_AddNode_finish(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                       const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId,
                       const UA_NodeId *typeDefinition,
                       UA_InstantiationCallback *instantiationCallback) {
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

    /* Workaround: Replace empty typeDefinition with the most permissive default */
    const UA_NodeId baseDataVariableType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    const UA_NodeId baseObjectType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE);
    if((node->nodeClass == UA_NODECLASS_VARIABLE || node->nodeClass == UA_NODECLASS_OBJECT) &&
       UA_NodeId_isNull(typeDefinition)) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: Use a default "
                            "TypeDefinition for the Variable/Object", NULL);
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
                            "AddNodes: The parent reference is invalid", NULL);
        deleteNode(server, &adminSession, nodeId, true);
        return retval;
    }

    /* Instantiate node. We need the variable type for type checking (e.g. when
     * writing into attributes) */
    retval = instantiateNode(server, session, nodeId, node->nodeClass,
                             typeDefinition, instantiationCallback);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node instantiation failed "
                            "with code %s", UA_StatusCode_name(retval));
        deleteNode(server, &adminSession, nodeId, true);
        return retval;
    }
    
    if(node->nodeClass == UA_NODECLASS_VARIABLE ||
       node->nodeClass == UA_NODECLASS_VARIABLETYPE) {
        /* Type check node */
        retval = typeCheckVariableNode(server, session, (const UA_VariableNode*)node, typeDefinition);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Type checking failed with error code %s",
                                UA_StatusCode_name(retval));
            deleteNode(server, &adminSession, nodeId, true);
            return retval;
        }

        if(node->nodeClass == UA_NODECLASS_VARIABLE) {
            /* Set AccessLevel to readable */
            const UA_VariableNode *vn = (const UA_VariableNode*)node;
            if(!(vn->accessLevel & (UA_ACCESSLEVELMASK_READ))) {
                UA_LOG_INFO_SESSION(server->config.logger, session,
                                    "AddNodes: Set the AccessLevel to readable by default", NULL);
                UA_Byte readable = vn->accessLevel | (UA_ACCESSLEVELMASK_READ);
                UA_Server_writeAccessLevel(server, vn->nodeId, readable);
            }
        }
    }

    /* Add parent reference */
    if(!UA_NodeId_isNull(parentNodeId)) {
        UA_AddReferencesItem ref_item;
        UA_AddReferencesItem_init(&ref_item);
        ref_item.sourceNodeId = node->nodeId;
        ref_item.referenceTypeId = *referenceTypeId;
        ref_item.isForward = false;
        ref_item.targetNodeId.nodeId = *parentNodeId;
        addReference(server, session, &ref_item, &retval);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Adding reference to parent failed", NULL);
            deleteNode(server, &adminSession, nodeId, true);
            return retval;
        }
    }

    return UA_STATUSCODE_GOOD;

}

static void
Service_AddNodes_single(UA_Server *server, UA_Session *session,
                        const UA_AddNodesItem *item, UA_AddNodesResult *result,
                        UA_InstantiationCallback *instantiationCallback) {
    /* AddNodes_begin */
    Service_AddNode_begin(server, session, item, result);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* AddNodes_finish */
    result->statusCode =
        Service_AddNode_finish(server, session, &result->addedNodeId,
                               &item->parentNodeId.nodeId, &item->referenceTypeId,
                               &item->typeDefinition.nodeId, instantiationCallback);

    /* If finishing failed, don't even return a NodeId of the added node */
    if(result->statusCode != UA_STATUSCODE_GOOD)
        UA_NodeId_deleteMembers(&result->addedNodeId);
}

void Service_AddNodes(UA_Server *server, UA_Session *session,
                      const UA_AddNodesRequest *request,
                      UA_AddNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing AddNodesRequest", NULL);

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
                    UA_InstantiationCallback *instantiationCallback,
                    UA_NodeId *outNewNodeId) {
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
    Service_AddNodes_single(server, &adminSession, &item, &result, instantiationCallback);
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
                          UA_NodeId *outNewNodeId) {
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
    Service_AddNode_begin(server, &adminSession, &item, &result);
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
                         const UA_NodeId typeDefinition,
                         UA_InstantiationCallback *instantiationCallback) {
    UA_RCU_LOCK();
    UA_StatusCode retval = Service_AddNode_finish(server, &adminSession, &nodeId, &parentNodeId,
                                                  &referenceTypeId, &typeDefinition,
                                                  instantiationCallback);
    UA_RCU_UNLOCK();
    return retval;
}

/**************************************************/
/* Add Special Nodes (not possible over the wire) */
/**************************************************/

UA_StatusCode
UA_Server_addDataSourceVariableNode(UA_Server *server,
                                    const UA_NodeId requestedNewNodeId,
                                    const UA_NodeId parentNodeId,
                                    const UA_NodeId referenceTypeId,
                                    const UA_QualifiedName browseName,
                                    const UA_NodeId typeDefinition,
                                    const UA_VariableAttributes attr,
                                    const UA_DataSource dataSource,
                                    UA_NodeId *outNewNodeId) {
    UA_NodeId newNodeId;
    UA_Boolean  deleteNodeId = UA_FALSE;
    if(!outNewNodeId) {
        newNodeId = UA_NODEID_NULL;
        outNewNodeId = &newNodeId;
        deleteNodeId = UA_TRUE;
    }
    UA_StatusCode retval = UA_Server_addVariableNode_begin(server, requestedNewNodeId,
                                                           browseName, attr, outNewNodeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    retval = UA_Server_setVariableNode_dataSource(server, *outNewNodeId, dataSource);
    if(retval == UA_STATUSCODE_GOOD)
        retval = UA_Server_addNode_finish(server, *outNewNodeId,
                                          parentNodeId, referenceTypeId,
                                          typeDefinition, NULL);
    if(retval != UA_STATUSCODE_GOOD || deleteNodeId)
        UA_NodeId_deleteMembers(outNewNodeId);
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_METHODCALLS

UA_StatusCode
UA_Server_addMethodNode_begin(UA_Server *server, const UA_NodeId requestedNewNodeId,
                              const UA_QualifiedName browseName,
                              const UA_MethodAttributes attr,
                              UA_MethodCallback method, void *handle,
                              UA_NodeId *outNewNodeId) {
    /* Create the node */
    UA_MethodNode *node = (UA_MethodNode*)UA_NodeStore_newNode(UA_NODECLASS_METHOD);
    if(!node)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set the node attributes */
    node->executable = attr.executable;
    node->attachedMethod = method;
    node->methodHandle = handle;
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    UA_StatusCode retval =
        copyStandardAttributes((UA_Node*)node, &item, (const UA_NodeAttributes*)&attr);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)node);
        return retval;
    }

    /* Add the node to the nodestore */
    UA_RCU_LOCK();
    retval = UA_NodeStore_insert(server->nodestore, (UA_Node*)node);
    if(outNewNodeId) {
        retval = UA_NodeId_copy(&node->nodeId, outNewNodeId);
        if(retval != UA_STATUSCODE_GOOD)
            UA_NodeStore_remove(server->nodestore, &node->nodeId);
    }
    UA_RCU_UNLOCK();
    return retval;
}

UA_StatusCode
UA_Server_addMethodNode_finish(UA_Server *server, const UA_NodeId nodeId,
                               const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                               size_t inputArgumentsSize, const UA_Argument* inputArguments,
                               size_t outputArgumentsSize, const UA_Argument* outputArguments) {
    const UA_NodeId hasproperty = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    const UA_NodeId propertytype = UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE);
    const UA_NodeId argsId = UA_NODEID_NUMERIC(nodeId.namespaceIndex, 0);

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
        deleteNode(server, &adminSession, &nodeId, true);
        UA_RCU_UNLOCK();
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
        UA_VariableAttributes inputargs;
        UA_VariableAttributes_init(&inputargs);
        inputargs.displayName = UA_LOCALIZEDTEXT("en_US", "InputArguments");
        /* UAExpert creates a monitoreditem on inputarguments ... */
        inputargs.minimumSamplingInterval = 100000.0f;
        inputargs.valueRank = 1;
        inputargs.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
        /* dirty-cast, but is treated as const ... */
        UA_Variant_setArray(&inputargs.value, (void*)(uintptr_t)inputArguments,
                            inputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
        retval = UA_Server_addVariableNode(server, argsId, nodeId, hasproperty,
                                           inputArgsName, propertytype, inputargs,
                                           NULL, &inputArgsId);
    }

    /* Add the Output Arguments VariableNode */
    if(outputArgumentsSize > 0 && UA_NodeId_isNull(&outputArgsId)) {
        UA_VariableAttributes outputargs;
        UA_VariableAttributes_init(&outputargs);
        outputargs.displayName = UA_LOCALIZEDTEXT("en_US", "OutputArguments");
        /* UAExpert creates a monitoreditem on outputarguments ... */
        outputargs.minimumSamplingInterval = 100000.0f;
        outputargs.valueRank = 1;
        outputargs.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
        /* dirty-cast, but is treated as const ... */
        UA_Variant_setArray(&outputargs.value, (void*)(uintptr_t)outputArguments,
                            outputArgumentsSize, &UA_TYPES[UA_TYPES_ARGUMENT]);
        retval |= UA_Server_addVariableNode(server, argsId, nodeId, hasproperty,
                                            outputArgsName, propertytype, outputargs,
                                            NULL, &outputArgsId);
    }

    /* Call finish to add the parent reference */
    UA_RCU_LOCK();
    retval |= Service_AddNode_finish(server, &adminSession, &nodeId, &parentNodeId,
                                     &referenceTypeId, &UA_NODEID_NULL, NULL);
    UA_RCU_UNLOCK();

    if(retval != UA_STATUSCODE_GOOD) {
        deleteNode(server, &adminSession, &nodeId, true);
        deleteNode(server, &adminSession, &inputArgsId, true);
        deleteNode(server, &adminSession, &outputArgsId, true);
    }
    UA_BrowseResult_deleteMembers(&br);
    return retval;
}

UA_StatusCode
UA_Server_addMethodNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                        const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                        const UA_QualifiedName browseName, const UA_MethodAttributes attr,
                        UA_MethodCallback method, void *handle,
                        size_t inputArgumentsSize, const UA_Argument* inputArguments, 
                        size_t outputArgumentsSize, const UA_Argument* outputArguments,
                        UA_NodeId *outNewNodeId) {
    UA_NodeId newId;
    if(!outNewNodeId) {
        UA_NodeId_init(&newId);
        outNewNodeId = &newId;
    }

    /* Call begin */
    UA_StatusCode retval =
        UA_Server_addMethodNode_begin(server, requestedNewNodeId, browseName,
                                      attr, method, handle, outNewNodeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Call finish */
    retval = UA_Server_addMethodNode_finish(server, *outNewNodeId,
                                            parentNodeId, referenceTypeId,
                                            inputArgumentsSize, inputArguments,
                                            outputArgumentsSize, outputArguments);

    if(outNewNodeId == &newId)
        UA_NodeId_deleteMembers(&newId);
    return retval;
}

#endif

/******************/
/* Add References */
/******************/

static UA_StatusCode
addOneWayTarget(UA_NodeReferenceKind *refs, const UA_ExpandedNodeId *target) {
    UA_ExpandedNodeId *targets =
        (UA_ExpandedNodeId*) UA_realloc(refs->targetIds,
                                        sizeof(UA_ExpandedNodeId) * (refs->targetIdsSize+1));
    if(!targets)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    refs->targetIds = targets;

    UA_StatusCode retval = UA_ExpandedNodeId_copy(target, &refs->targetIds[refs->targetIdsSize]);
    if(retval != UA_STATUSCODE_GOOD && refs->targetIds == 0) {
        UA_free(refs->targetIds);
        refs->targetIds = NULL;
        return retval;
    }

    refs->targetIdsSize++;
    return retval;
}

static UA_StatusCode
addOneWayNodeReferences(UA_Node *node, const UA_AddReferencesItem *item) {
    UA_NodeReferenceKind *refs =
        (UA_NodeReferenceKind*)UA_realloc(node->references,
                                          sizeof(UA_NodeReferenceKind) * (node->referencesSize+1));
    if(!refs)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    node->references = refs;
    UA_NodeReferenceKind *newRef = &refs[node->referencesSize];
    memset(newRef, 0, sizeof(UA_NodeReferenceKind));

    newRef->isInverse = !item->isForward;
    UA_StatusCode retval = UA_NodeId_copy(&item->referenceTypeId, &newRef->referenceTypeId);
    retval |= addOneWayTarget(newRef, &item->targetNodeId);

    if(retval == UA_STATUSCODE_GOOD) {
        node->referencesSize++;
    } else {
        UA_NodeId_deleteMembers(&newRef->referenceTypeId);
        if(node->referencesSize == 0) {
            UA_free(node->references);
            node->references = NULL;
        }
    }
    return retval;
}

/* Adds a one-way reference to the local nodestore */
static UA_StatusCode
addOneWayReference(UA_Server *server, UA_Session *session,
                   UA_Node *node, const UA_AddReferencesItem *item) {
    for(size_t i = 0; i < node->referencesSize; ++i) {
        UA_NodeReferenceKind *refs = &node->references[i];
        if(refs->isInverse == item->isForward)
            continue;
        if(!UA_NodeId_equal(&refs->referenceTypeId, &item->referenceTypeId))
            continue;
        return addOneWayTarget(refs, &item->targetNodeId);
    }
    return addOneWayNodeReferences(node, item);
}

static void
addReference(UA_Server *server, UA_Session *session,
             const UA_AddReferencesItem *item,
             UA_StatusCode *retval) {
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
                         "Processing AddReferencesRequest", NULL);
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

/****************/
/* Delete Nodes */
/****************/

static void
removeReferences(UA_Server *server, UA_Session *session,
                 const UA_Node *node) {
    UA_DeleteReferencesItem item;
    UA_DeleteReferencesItem_init(&item);
    item.targetNodeId.nodeId = node->nodeId;
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
           const UA_NodeId *nodeId, UA_Boolean deleteReferences) {
    UA_RCU_LOCK();
    const UA_Node *node = UA_NodeStore_get(server->nodestore, nodeId);
    UA_RCU_UNLOCK();
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* TODO: check if the information model consistency is violated */
    /* TODO: Check if the node is a mandatory child of an object */

    /* Destroy an object before removing it */
    if(node->nodeClass == UA_NODECLASS_OBJECT) {
        /* Call the destructor from the object type */
        UA_RCU_LOCK();
        const UA_ObjectTypeNode *typenode =
            getObjectNodeType(server, (const UA_ObjectNode*)node);
        UA_RCU_UNLOCK();
        if(typenode && typenode->lifecycleManagement.destructor) {
            const UA_ObjectNode *on = (const UA_ObjectNode*)node;
            typenode->lifecycleManagement.destructor(*nodeId, on->instanceHandle);
        }
    }

    /* Remove references to the node (not the references in the node that will
     * be deleted anyway) */
    if(deleteReferences)
        removeReferences(server, session, node);

    UA_RCU_LOCK();
    UA_StatusCode retval = UA_NodeStore_remove(server->nodestore, nodeId);
    UA_RCU_UNLOCK();

    return retval;
}

void Service_DeleteNodes(UA_Server *server, UA_Session *session,
                         const UA_DeleteNodesRequest *request,
                         UA_DeleteNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing DeleteNodesRequest", NULL);

    if(request->nodesToDeleteSize == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results =
        (UA_StatusCode*)UA_malloc(sizeof(UA_StatusCode) * request->nodesToDeleteSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;;
        return;
    }
    response->resultsSize = request->nodesToDeleteSize;

    for(size_t i = 0; i < request->nodesToDeleteSize; ++i) {
        UA_DeleteNodesItem *item = &request->nodesToDelete[i];
        response->results[i] = deleteNode(server, session, &item->nodeId,
                                          item->deleteTargetReferences);
    }
}

UA_StatusCode
UA_Server_deleteNode(UA_Server *server, const UA_NodeId nodeId,
                     UA_Boolean deleteReferences) {
    UA_StatusCode retval = deleteNode(server, &adminSession,
                                      &nodeId, deleteReferences);
    return retval;
}

/*********************/
/* Delete References */
/*********************/

// TODO: Check consistency constraints, remove the references.
static UA_StatusCode
deleteOneWayReference(UA_Server *server, UA_Session *session, UA_Node *node,
                      const UA_DeleteReferencesItem *item) {
    for(size_t i = node->referencesSize; i > 0; --i) {
        UA_NodeReferenceKind *refs = &node->references[i-1];
        if(item->isForward == refs->isInverse)
            continue;
        if(!UA_NodeId_equal(&item->referenceTypeId, &refs->referenceTypeId))
            continue;

        for(size_t j = refs->targetIdsSize; j > 0; --j) {
            if(!UA_NodeId_equal(&item->targetNodeId.nodeId, &refs->targetIds[j-1].nodeId))
                continue;

            /* Ok, delete the reference */
            UA_ExpandedNodeId_deleteMembers(&refs->targetIds[j-1]);
            refs->targetIdsSize--;

            /* One matching target remaining */
            if(refs->targetIdsSize > 0) {
                if(j-1 != refs->targetIdsSize) // avoid valgrind error: Source
                                               // and destination overlap in
                                               // memcpy
                    refs->targetIds[j-1] = refs->targetIds[refs->targetIdsSize];
                return UA_STATUSCODE_GOOD;
            }

            /* Remove refs */
            UA_free(refs->targetIds);
            UA_NodeId_deleteMembers(&refs->referenceTypeId);
            node->referencesSize--;
            if(node->referencesSize > 0) {
                if(i-1 != node->referencesSize) // avoid valgrind error: Source
                                                // and destination overlap in
                                                // memcpy
                    node->references[i-1] = node->references[node->referencesSize];
                return UA_STATUSCODE_GOOD;
            }

            /* Remove the node references */
            UA_free(node->references);
            node->references = NULL;
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_UNCERTAINREFERENCENOTDELETED;
}

static void
deleteReference(UA_Server *server, UA_Session *session,
                const UA_DeleteReferencesItem *item,
                UA_StatusCode *retval) {
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
                         "Processing DeleteReferencesRequest", NULL);
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
UA_Server_setVariableNode_valueCallback(UA_Server *server, const UA_NodeId nodeId,
                                        const UA_ValueCallback callback) {
    UA_RCU_LOCK();
    UA_StatusCode retval =
        UA_Server_editNode(server, &adminSession, &nodeId,
                           (UA_EditNodeCallback)setValueCallback, &callback);
    UA_RCU_UNLOCK();
    return retval;
}

/******************/
/* Set DataSource */
/******************/

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
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession, &nodeId,
                                              (UA_EditNodeCallback)setDataSource,
                                              &dataSource);
    return retval;
}

/****************************/
/* Set Lifecycle Management */
/****************************/

static UA_StatusCode
setOLM(UA_Server *server, UA_Session *session,
       UA_ObjectTypeNode* node, UA_ObjectLifecycleManagement *olm) {
    if(node->nodeClass != UA_NODECLASS_OBJECTTYPE)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    node->lifecycleManagement = *olm;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setObjectTypeNode_lifecycleManagement(UA_Server *server, UA_NodeId nodeId,
                                                UA_ObjectLifecycleManagement olm) {
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession, &nodeId,
                                              (UA_EditNodeCallback)setOLM, &olm);
    return retval;
}

/***********************/
/* Set Method Callback */
/***********************/

#ifdef UA_ENABLE_METHODCALLS

typedef struct {
    UA_MethodCallback callback;
    void *handle;
} addMethodCallback;

static UA_StatusCode
editMethodCallback(UA_Server *server, UA_Session* session,
                   UA_Node* node, const void* handle) {
    if(node->nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    const addMethodCallback *newCallback = (const addMethodCallback *)handle;
    UA_MethodNode *mnode = (UA_MethodNode*) node;
    mnode->attachedMethod = newCallback->callback;
    mnode->methodHandle   = newCallback->handle;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setMethodNode_callback(UA_Server *server, const UA_NodeId methodNodeId,
                                 UA_MethodCallback method, void *handle) {
    addMethodCallback cb;
    cb.callback = method;
    cb.handle = handle;

    UA_RCU_LOCK();
    UA_StatusCode retval =
        UA_Server_editNode(server, &adminSession,
                           &methodNodeId, editMethodCallback, &cb);
    UA_RCU_UNLOCK();
    return retval;
}

#endif
