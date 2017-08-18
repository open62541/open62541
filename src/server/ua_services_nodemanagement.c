/* This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include "ua_server_internal.h"
#include "ua_services.h"

/************************/
/* Forward Declarations */
/************************/

static UA_StatusCode
addReference(UA_Server *server, UA_Session *session,
             const UA_AddReferencesItem *item);

static UA_StatusCode
deleteReference(UA_Server *server, UA_Session *session,
                const UA_DeleteReferencesItem *item);

static UA_StatusCode
deleteNode(UA_Server *server, UA_Session *session,
           const UA_NodeId *nodeId, UA_Boolean deleteReferences);

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
                            "AddNodes: Parent node not found");
        return UA_STATUSCODE_BADPARENTNODEIDINVALID;
    }

    /* Check the referencetype exists */
    const UA_ReferenceTypeNode *referenceType =
        (const UA_ReferenceTypeNode*)UA_NodeStore_get(server->nodestore, referenceTypeId);
    if(!referenceType) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Reference type to the parent not found");
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    /* Check if the referencetype is a reference type node */
    if(referenceType->nodeClass != UA_NODECLASS_REFERENCETYPE) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Reference type to the parent invalid");
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    /* Check that the reference type is not abstract */
    if(referenceType->isAbstract == true) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Abstract reference type to the parent not allowed");
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

    return UA_STATUSCODE_GOOD;
}

/************/
/* Add Node */
/************/

static void
Service_AddNodes_single(UA_Server *server, UA_Session *session,
                        const UA_AddNodesItem *item, UA_AddNodesResult *result,
                        UA_InstantiationCallback *instantiationCallback);

static UA_StatusCode
copyChildNodesToNode(UA_Server *server, UA_Session *session,
                     const UA_NodeId *sourceNodeId, const UA_NodeId *destinationNodeId,
                     UA_InstantiationCallback *instantiationCallback);

/* copy an existing variable under the given parent. then instantiate the
 * variable for its type */
static UA_StatusCode
copyExistingVariable(UA_Server *server, UA_Session *session, const UA_NodeId *variable,
                     const UA_NodeId *referenceType, const UA_NodeId *parent,
                     UA_InstantiationCallback *instantiationCallback) {
    const UA_VariableNode *node =
        (const UA_VariableNode*)UA_NodeStore_get(server->nodestore, variable);
    if(!node)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if(node->nodeClass != UA_NODECLASS_VARIABLE)
        return UA_STATUSCODE_BADNODECLASSINVALID;

    /* Get the current value */
    UA_DataValue value;
    UA_DataValue_init(&value);
    UA_StatusCode retval = readValueAttribute(server, node, &value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Prepare the variable description */
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    attr.displayName = node->displayName;
    attr.description = node->description;
    attr.writeMask = node->writeMask;
    attr.userWriteMask = node->userWriteMask;
    attr.value = value.value;
    attr.dataType = node->dataType;
    attr.valueRank = node->valueRank;
    attr.arrayDimensionsSize = node->arrayDimensionsSize;
    attr.arrayDimensions = node->arrayDimensions;
    attr.accessLevel = node->accessLevel;
    attr.userAccessLevel = node->userAccessLevel;
    attr.minimumSamplingInterval = node->minimumSamplingInterval;
    attr.historizing = node->historizing;

    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.nodeClass = UA_NODECLASS_VARIABLE;
    item.parentNodeId.nodeId = *parent;
    item.referenceTypeId = *referenceType;
    item.browseName = node->browseName;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type = &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES];
    item.nodeAttributes.content.decoded.data = &attr;
    const UA_VariableTypeNode *vt = (const UA_VariableTypeNode*)getNodeType(server, (const UA_Node*)node);
    if(!vt || vt->nodeClass != UA_NODECLASS_VARIABLETYPE || vt->isAbstract) {
        retval = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
        goto cleanup;
    }
    item.typeDefinition.nodeId = vt->nodeId;

    /* Add the variable and instantiate the children */
    UA_AddNodesResult res;
    UA_AddNodesResult_init(&res);
    Service_AddNodes_single(server, session, &item, &res, instantiationCallback);
    if(res.statusCode != UA_STATUSCODE_GOOD) {
        retval = res.statusCode;
        goto cleanup;
    }
    retval = copyChildNodesToNode(server, session, &node->nodeId,
                                  &res.addedNodeId, instantiationCallback);

    if(retval == UA_STATUSCODE_GOOD && instantiationCallback)
        instantiationCallback->method(res.addedNodeId, node->nodeId,
                                      instantiationCallback->handle);

    UA_NodeId_deleteMembers(&res.addedNodeId);
 cleanup:
    if(value.hasValue && value.value.storageType == UA_VARIANT_DATA)
        UA_Variant_deleteMembers(&value.value);
    return retval;
}

/* Copy an existing object under the given parent. Then instantiate for all
 * hastypedefinitions of the original version. */
static UA_StatusCode
copyExistingObject(UA_Server *server, UA_Session *session, const UA_NodeId *object,
                   const UA_NodeId *referenceType, const UA_NodeId *parent,
                   UA_InstantiationCallback *instantiationCallback) {
    const UA_ObjectNode *node =
        (const UA_ObjectNode*)UA_NodeStore_get(server->nodestore, object);
    if(!node)
        return UA_STATUSCODE_BADNODEIDINVALID;
    if(node->nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_BADNODECLASSINVALID;

    /* Prepare the item */
    UA_ObjectAttributes attr;
    UA_ObjectAttributes_init(&attr);
    attr.displayName = node->displayName;
    attr.description = node->description;
    attr.writeMask = node->writeMask;
    attr.userWriteMask = node->userWriteMask;
    attr.eventNotifier = node->eventNotifier;

    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.nodeClass = UA_NODECLASS_OBJECT;
    item.parentNodeId.nodeId = *parent;
    item.referenceTypeId = *referenceType;
    item.browseName = node->browseName;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.nodeAttributes.content.decoded.type = &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES];
    item.nodeAttributes.content.decoded.data = &attr;
    const UA_ObjectTypeNode *objtype = (const UA_ObjectTypeNode*)getNodeType(server, (const UA_Node*)node);
    if(!objtype || objtype->nodeClass != UA_NODECLASS_OBJECTTYPE || objtype->isAbstract)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    item.typeDefinition.nodeId = objtype->nodeId;

    /* add the new object */
    UA_AddNodesResult res;
    UA_AddNodesResult_init(&res);
    Service_AddNodes_single(server, session, &item, &res, instantiationCallback);
    if(res.statusCode != UA_STATUSCODE_GOOD)
        return res.statusCode;

    /* Copy any aggregated/nested variables/methods/subobjects this object contains
     * These objects may not be part of the nodes type. */
    UA_StatusCode retval = copyChildNodesToNode(server, session, &node->nodeId,
                                                &res.addedNodeId, instantiationCallback);
    if(retval == UA_STATUSCODE_GOOD && instantiationCallback)
        instantiationCallback->method(res.addedNodeId, node->nodeId,
                                      instantiationCallback->handle);

    UA_NodeId_deleteMembers(&res.addedNodeId);
    return retval;
}

static UA_StatusCode
setObjectInstanceHandle(UA_Server *server, UA_Session *session,
                        UA_ObjectNode* node, void * (*constructor)(const UA_NodeId instance)) {
    if(node->nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    if(!node->instanceHandle)
        node->instanceHandle = constructor(node->nodeId);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
instantiateNode(UA_Server *server, UA_Session *session, const UA_NodeId *nodeId,
                UA_NodeClass nodeClass, const UA_NodeId *typeId,
                UA_InstantiationCallback *instantiationCallback) {
    /* see if the type node is correct */
    const UA_Node *typenode = UA_NodeStore_get(server->nodestore, typeId);
    if(!typenode)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    if(nodeClass == UA_NODECLASS_VARIABLE) {
        if(typenode->nodeClass != UA_NODECLASS_VARIABLETYPE ||
           ((const UA_VariableTypeNode*)typenode)->isAbstract)
            return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    } else if(nodeClass == UA_NODECLASS_OBJECT) {
        if(typenode->nodeClass != UA_NODECLASS_OBJECTTYPE ||
           ((const UA_ObjectTypeNode*)typenode)->isAbstract)
            return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    } else {
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    }

    /* Get the hierarchy of the type and all its supertypes */
    UA_NodeId *hierarchy = NULL;
    size_t hierarchySize = 0;
    UA_StatusCode retval =
        getTypeHierarchy(server->nodestore, typenode, true, &hierarchy, &hierarchySize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Copy members of the type and supertypes */
    for(size_t i = 0; i < hierarchySize; ++i)
        retval |= copyChildNodesToNode(server, session, &hierarchy[i], nodeId, instantiationCallback);
    UA_Array_delete(hierarchy, hierarchySize, &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Call the object constructor */
    if(typenode->nodeClass == UA_NODECLASS_OBJECTTYPE) {
        const UA_ObjectLifecycleManagement *olm =
            &((const UA_ObjectTypeNode*)typenode)->lifecycleManagement;
        if(olm->constructor)
            UA_Server_editNode(server, session, nodeId,
                               (UA_EditNodeCallback)setObjectInstanceHandle,
                               olm->constructor);
    }

    /* Add a hasType reference */
    UA_AddReferencesItem addref;
    UA_AddReferencesItem_init(&addref);
    addref.sourceNodeId = *nodeId;
    addref.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    addref.isForward = true;
    addref.targetNodeId.nodeId = *typeId;
    return addReference(server, session, &addref);
}

/* Search for an instance of "browseName" in node searchInstance
 * Used during copyChildNodes to find overwritable/mergable nodes */
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
mandatoryChild(UA_Server *server, UA_Session *session, const UA_NodeId *childNodeId) {
    const UA_NodeId mandatoryId = UA_NODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY);
    const UA_NodeId hasModellingRuleId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE);

    /* Get the child */
    const UA_Node *child = UA_NodeStore_get(server->nodestore, childNodeId);
    if(!child)
        return false;

    /* Look for the reference making the child mandatory */
    for(size_t i = 0; i < child->referencesSize; ++i) {
        UA_ReferenceNode *ref = &child->references[i];
        if(!UA_NodeId_equal(&hasModellingRuleId, &ref->referenceTypeId))
            continue;
        if(!UA_NodeId_equal(&mandatoryId, &ref->targetId.nodeId))
            continue;
        if(ref->isInverse)
            continue;
        return true;
    }
    return false;
}

/* Copy any children of Node sourceNodeId to another node destinationNodeId
 * Used at 2 places:
 *  (1) During instantiation, when any children of the Type are copied
 *  (2) During instantiation to copy any *nested* instances to the new node
 *      (2.1) Might call instantiation of a type first
 *      (2.2) *Should* then overwrite nested contents in definition --> this scenario is currently not handled!
 */
static UA_StatusCode
copyChildNodesToNode(UA_Server* server, UA_Session* session,
                     const UA_NodeId* sourceNodeId, const UA_NodeId* destinationNodeId,
                     UA_InstantiationCallback* instantiationCallback) {
    /* Browse to get all children */
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

    /* Copy all children */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId existingChild = UA_NODEID_NULL;
    for(size_t i = 0; i < br.referencesSize; ++i) {
        UA_ReferenceDescription *rd = &br.references[i];

        /* Is the child mandatory? If not, skip */
        if(!mandatoryChild(server, session, &rd->nodeId.nodeId))
            continue;

        /* TODO: If a child is optional, check whether optional children that
         * were manually added fit the constraints. */

        /* Check for deduplication */
        retval = instanceFindAggregateByBrowsename(server, session, destinationNodeId,
                                                   &rd->browseName, &existingChild);
        if(retval != UA_STATUSCODE_GOOD)
            break;

        if(UA_NodeId_equal(&UA_NODEID_NULL, &existingChild)) {
            /* New node in child */
            if(rd->nodeClass == UA_NODECLASS_METHOD) {
                /* add a reference to the method in the objecttype */
                UA_AddReferencesItem newItem;
                UA_AddReferencesItem_init(&newItem);
                newItem.sourceNodeId = *destinationNodeId;
                newItem.referenceTypeId = rd->referenceTypeId;
                newItem.isForward = true;
                newItem.targetNodeId = rd->nodeId;
                newItem.targetNodeClass = UA_NODECLASS_METHOD;
                retval = addReference(server, session, &newItem);
            } else if(rd->nodeClass == UA_NODECLASS_VARIABLE)
                retval = copyExistingVariable(server, session, &rd->nodeId.nodeId,
                                              &rd->referenceTypeId, destinationNodeId,
                                              instantiationCallback);
            else if(rd->nodeClass == UA_NODECLASS_OBJECT)
                retval = copyExistingObject(server, session, &rd->nodeId.nodeId,
                                            &rd->referenceTypeId, destinationNodeId,
                                            instantiationCallback);
        } else {
            /* Preexistent node in child
             * General strategy if we meet an already existing node:
             * - Preexistent variable contents always 'win' overwriting anything
             *   supertypes would instantiate
             * - Always copy contents of template *into* existant node (merge
             *   contents of e.g. Folders like ParameterSet) */
            if(rd->nodeClass == UA_NODECLASS_METHOD) {
                /* Do nothing, existent method wins */
            } else if(rd->nodeClass == UA_NODECLASS_VARIABLE ||
                      rd->nodeClass == UA_NODECLASS_OBJECT) {
                if(!UA_NodeId_equal(&rd->nodeId.nodeId, &existingChild))
                    retval = copyChildNodesToNode(server, session, &rd->nodeId.nodeId,
                                                  &existingChild, instantiationCallback);
            }
            UA_NodeId_deleteMembers(&existingChild);
        }
        if(retval != UA_STATUSCODE_GOOD)
            break;
    }
    UA_BrowseResult_deleteMembers(&br);
    return retval;
}

UA_StatusCode
Service_AddNodes_existing(UA_Server *server, UA_Session *session, UA_Node *node,
                          const UA_NodeId *parentNodeId, const UA_NodeId *referenceTypeId,
                          const UA_NodeId *typeDefinition,
                          UA_InstantiationCallback *instantiationCallback,
                          UA_NodeId *addedNodeId) {
    UA_ASSERT_RCU_LOCKED();

    /* Check the namespaceindex */
    if(node->nodeId.namespaceIndex >= server->namespacesSize) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "AddNodes: Namespace invalid");
        UA_NodeStore_deleteNode(node);
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Check the reference to the parent */
    UA_StatusCode retval = checkParentReference(server, session, node->nodeClass,
                                                parentNodeId, referenceTypeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Checking the reference to the parent returned "
                            "error code %s", UA_StatusCode_name(retval));
        UA_NodeStore_deleteNode(node);
        return retval;
    }

    /* Add the node to the nodestore */
    retval = UA_NodeStore_insert(server->nodestore, node);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "AddNodes: Node could not be added to the nodestore "
                            "with error code %s", UA_StatusCode_name(retval));
        return retval;
    }

    /* Copy the nodeid if needed */
    if(addedNodeId) {
        retval = UA_NodeId_copy(&node->nodeId, addedNodeId);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Could not copy the nodeid");
            goto remove_node;
        }
    }

    /* Hierarchical reference back to the parent */
    if(!UA_NodeId_isNull(parentNodeId)) {
        UA_AddReferencesItem item;
        UA_AddReferencesItem_init(&item);
        item.sourceNodeId = node->nodeId;
        item.referenceTypeId = *referenceTypeId;
        item.isForward = false;
        item.targetNodeId.nodeId = *parentNodeId;
        retval = addReference(server, session, &item);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Could not add the reference to the parent"
                                "with error code %s", UA_StatusCode_name(retval));
            goto remove_node;
        }
    }

    /* Fall back to a default typedefinition for variables and objects */
    const UA_NodeId basedatavariabletype = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    const UA_NodeId baseobjecttype = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE);
    if(node->nodeClass == UA_NODECLASS_VARIABLE ||
       node->nodeClass == UA_NODECLASS_OBJECT) {
        if(!typeDefinition || UA_NodeId_isNull(typeDefinition)) {
            if(node->nodeClass == UA_NODECLASS_VARIABLE)
                typeDefinition = &basedatavariabletype;
            else
                typeDefinition = &baseobjecttype;
        }

        /* Instantiate variables and objects */
        retval = instantiateNode(server, session, &node->nodeId, node->nodeClass,
                                 typeDefinition, instantiationCallback);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "AddNodes: Could not instantiate the node with"
                                "error code %s", UA_StatusCode_name(retval));
            goto remove_node;
        }
    }

    /* Custom callback */
    if(instantiationCallback)
        instantiationCallback->method(node->nodeId, *typeDefinition,
                                      instantiationCallback->handle);
    return UA_STATUSCODE_GOOD;

 remove_node:
    deleteNode(server, &adminSession, &node->nodeId, true);
    return retval;
}

/*******************************************/
/* Create nodes from attribute description */
/*******************************************/

static UA_StatusCode
copyStandardAttributes(UA_Node *node, const UA_AddNodesItem *item,
                       const UA_NodeAttributes *attr) {
    UA_StatusCode retval;
    retval  = UA_NodeId_copy(&item->requestedNewNodeId.nodeId, &node->nodeId);
    retval |= UA_QualifiedName_copy(&item->browseName, &node->browseName);
    retval |= UA_LocalizedText_copy(&attr->displayName, &node->displayName);
    retval |= UA_LocalizedText_copy(&attr->description, &node->description);
    node->writeMask = attr->writeMask;
    node->userWriteMask = attr->userWriteMask;
    return retval;
}

static UA_StatusCode
copyCommonVariableAttributes(UA_Server *server, UA_VariableNode *node,
                             const UA_AddNodesItem *item,
                             const UA_VariableAttributes *attr) {
    const UA_NodeId basevartype = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE);
    const UA_NodeId basedatavartype = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    const UA_NodeId *typeDef = &item->typeDefinition.nodeId;
    if(UA_NodeId_isNull(typeDef)) /* workaround when the variabletype is undefined */
        typeDef = &basedatavartype;

    /* Make sure we can instantiate the basetypes themselves */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(UA_NodeId_equal(&node->nodeId, &basevartype) ||
       UA_NodeId_equal(&node->nodeId, &basedatavartype)) {
        node->dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);
        node->valueRank = -2;
        return retval;
    }

    const UA_VariableTypeNode *vt =
        (const UA_VariableTypeNode*)UA_NodeStore_get(server->nodestore, typeDef);
    if(!vt || vt->nodeClass != UA_NODECLASS_VARIABLETYPE)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    if(node->nodeClass == UA_NODECLASS_VARIABLE && vt->isAbstract)
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;

    /* Set the datatype */
    if(!UA_NodeId_isNull(&attr->dataType))
        retval  = writeDataTypeAttribute(server, node, &attr->dataType, &vt->dataType);
    else /* workaround common error where the datatype is left as NA_NODEID_NULL */
        retval = UA_NodeId_copy(&vt->dataType, &node->dataType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the array dimensions. Check only against the vt. */
    retval = compatibleArrayDimensions(vt->arrayDimensionsSize, vt->arrayDimensions,
                                       attr->arrayDimensionsSize, attr->arrayDimensions);
    if(retval == UA_STATUSCODE_GOOD) {
        retval = UA_Array_copy(attr->arrayDimensions, attr->arrayDimensionsSize,
                               (void**)&node->arrayDimensions, &UA_TYPES[UA_TYPES_UINT32]);
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Array dimensions incompatible with the VariableType "
                    "with error code %s", UA_StatusCode_name(retval));
        return retval;
    }
    node->arrayDimensionsSize = attr->arrayDimensionsSize;

    /* Set the valuerank */
    if(attr->valueRank != 0 || !UA_Variant_isScalar(&attr->value))
        retval = writeValueRankAttribute(server, node, attr->valueRank, vt->valueRank);
    else /* workaround common error where the valuerank is left as 0 */
        node->valueRank = vt->valueRank;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Value Rank incompatible with the VariableType "
                    "with error code %s", UA_StatusCode_name(retval));
        return retval;
    }

    /* Set the value */
    UA_DataValue value;
    UA_DataValue_init(&value);
    value.hasValue = true;
    value.value = attr->value;
    value.value.storageType = UA_VARIANT_DATA_NODELETE;

    /* Use the default value from the vt if none is defined */
    if(!value.value.type) {
        retval = readValueAttribute(server, (const UA_VariableNode*)vt, &value);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                        "Could not read the value of the variable type "
                        "with error code %s", UA_StatusCode_name(retval));
            return retval;
        }
    }

    /* Write the value to the node */
    retval = writeValueAttribute(server, node, &value, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Could not set the value of the new node "
                    "with error code %s", UA_StatusCode_name(retval));
    }
    UA_DataValue_deleteMembers(&value);
    return retval;
}

static UA_StatusCode
copyVariableNodeAttributes(UA_Server *server, UA_VariableNode *vnode,
                           const UA_AddNodesItem *item,
                           const UA_VariableAttributes *attr) {
    vnode->accessLevel = attr->accessLevel;
    vnode->userAccessLevel = attr->userAccessLevel;
    vnode->historizing = attr->historizing;
    vnode->minimumSamplingInterval = attr->minimumSamplingInterval;
    return copyCommonVariableAttributes(server, vnode, item, attr);
}

static UA_StatusCode
copyVariableTypeNodeAttributes(UA_Server *server, UA_VariableTypeNode *vtnode,
                               const UA_AddNodesItem *item,
                               const UA_VariableTypeAttributes *attr) {
    vtnode->isAbstract = attr->isAbstract;
    return copyCommonVariableAttributes(server, (UA_VariableNode*)vtnode, item,
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

static UA_StatusCode
createNodeFromAttributes(UA_Server *server, const UA_AddNodesItem *item, UA_Node **newNode) {
    /* Check that we can read the attributes */
    if(item->nodeAttributes.encoding < UA_EXTENSIONOBJECT_DECODED ||
       !item->nodeAttributes.content.decoded.type)
        return UA_STATUSCODE_BADNODEATTRIBUTESINVALID;

    /* Create the node */
    // todo: error case where the nodeclass is faulty
    void *node = UA_NodeStore_newNode(item->nodeClass);
    if(!node)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Copy the attributes into the node */
    void *data = item->nodeAttributes.content.decoded.data;
    UA_StatusCode retval = copyStandardAttributes(node, item, data);
    switch(item->nodeClass) {
    case UA_NODECLASS_OBJECT:
        CHECK_ATTRIBUTES(OBJECTATTRIBUTES);
        retval |= copyObjectNodeAttributes(node, data);
        break;
    case UA_NODECLASS_VARIABLE:
        CHECK_ATTRIBUTES(VARIABLEATTRIBUTES);
        retval |= copyVariableNodeAttributes(server, node, item, data);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        CHECK_ATTRIBUTES(OBJECTTYPEATTRIBUTES);
        retval |= copyObjectTypeNodeAttributes(node, data);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        CHECK_ATTRIBUTES(VARIABLETYPEATTRIBUTES);
        retval |= copyVariableTypeNodeAttributes(server, node, item, data);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        CHECK_ATTRIBUTES(REFERENCETYPEATTRIBUTES);
        retval |= copyReferenceTypeNodeAttributes(node, data);
        break;
    case UA_NODECLASS_DATATYPE:
        CHECK_ATTRIBUTES(DATATYPEATTRIBUTES);
        retval |= copyDataTypeNodeAttributes(node, data);
        break;
    case UA_NODECLASS_VIEW:
        CHECK_ATTRIBUTES(VIEWATTRIBUTES);
        retval |= copyViewNodeAttributes(node, data);
        break;
    case UA_NODECLASS_METHOD:
    case UA_NODECLASS_UNSPECIFIED:
    default:
        retval = UA_STATUSCODE_BADNODECLASSINVALID;
    }

    if(retval == UA_STATUSCODE_GOOD)
        *newNode = node;
    else
        UA_NodeStore_deleteNode(node);
    return retval;
}

static void
Service_AddNodes_single(UA_Server *server, UA_Session *session,
                        const UA_AddNodesItem *item, UA_AddNodesResult *result,
                        UA_InstantiationCallback *instantiationCallback) {
    /* Create the node from the attributes*/
    UA_Node *node = NULL;
    result->statusCode = createNodeFromAttributes(server, item, &node);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "Could not add node with error code %s",
                            UA_StatusCode_name(result->statusCode));
        return;
    }

    /* Run consistency checks and add the node */
    UA_assert(node != NULL);
    result->statusCode = Service_AddNodes_existing(server, session, node, &item->parentNodeId.nodeId,
                                                   &item->referenceTypeId, &item->typeDefinition.nodeId,
                                                   instantiationCallback, &result->addedNodeId);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "Could not add node with error code %s",
                            UA_StatusCode_name(result->statusCode));
    }
}

void Service_AddNodes(UA_Server *server, UA_Session *session,
                      const UA_AddNodesRequest *request,
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


    response->resultsSize = size;
    for(size_t i = 0; i < size; ++i) {
            Service_AddNodes_single(server, session, &request->nodesToAdd[i],
                                    &response->results[i], NULL);
    }
}

UA_StatusCode
__UA_Server_addNode(UA_Server *server, const UA_NodeClass nodeClass,
                    const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId,
                    const UA_NodeId referenceTypeId, const UA_QualifiedName browseName,
                    const UA_NodeId typeDefinition, const UA_NodeAttributes *attr,
                    const UA_DataType *attributeType,
                    UA_InstantiationCallback *instantiationCallback, UA_NodeId *outNewNodeId) {
    /* prepare the item */
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.parentNodeId.nodeId = parentNodeId;
    item.referenceTypeId = referenceTypeId;
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    item.nodeClass = nodeClass;
    item.typeDefinition.nodeId = typeDefinition;
    item.nodeAttributes = (UA_ExtensionObject){
        .encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE,
        .content.decoded = {attributeType, (void*)(uintptr_t)attr}};

    /* run the service */
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    UA_RCU_LOCK();
    Service_AddNodes_single(server, &adminSession, &item, &result, instantiationCallback);
    UA_RCU_UNLOCK();

    /* prepare the output */
    if(outNewNodeId && result.statusCode == UA_STATUSCODE_GOOD)
        *outNewNodeId = result.addedNodeId;
    else
        UA_NodeId_deleteMembers(&result.addedNodeId);
    return result.statusCode;
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
    /* Create the new node */
    UA_VariableNode *node = UA_NodeStore_newVariableNode();
    if(!node)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Read the current value (to do typechecking) */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_VariableAttributes editAttr = attr;
    UA_DataValue value;
    UA_DataValue_init(&value);
    if(dataSource.read)
        retval = dataSource.read(dataSource.handle, requestedNewNodeId,
                                 false, NULL, &value);
    else
        retval = UA_STATUSCODE_BADTYPEMISMATCH;
    editAttr.value = value.value;

    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)node);
        return retval;
    }

    /* Copy attributes into node */
    UA_RCU_LOCK();
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    item.typeDefinition.nodeId = typeDefinition;
    item.parentNodeId.nodeId = parentNodeId;
    retval |= copyStandardAttributes((UA_Node*)node, &item, (const UA_NodeAttributes*)&editAttr);
    retval |= copyVariableNodeAttributes(server, node, &item, &editAttr);
    UA_DataValue_deleteMembers(&node->value.data.value);
    node->valueSource = UA_VALUESOURCE_DATASOURCE;
    node->value.dataSource = dataSource;
    UA_DataValue_deleteMembers(&value);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_NodeStore_deleteNode((UA_Node*)node);
        UA_RCU_UNLOCK();
        return retval;
    }

    /* Add the node */
    UA_AddNodesResult result;
    UA_AddNodesResult_init(&result);
    retval = Service_AddNodes_existing(server, &adminSession, (UA_Node*)node, &parentNodeId,
                                       &referenceTypeId, &typeDefinition, NULL, outNewNodeId);
    UA_RCU_UNLOCK();
    return retval;
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
    UA_MethodNode *node = UA_NodeStore_newMethodNode();
    if(!node)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    copyStandardAttributes((UA_Node*)node, &item, (const UA_NodeAttributes*)&attr);
    node->executable = attr.executable;
    node->userExecutable = attr.userExecutable;
    node->attachedMethod = method;
    node->methodHandle = handle;

    /* Add the node */
    UA_NodeId newMethodId;
    UA_NodeId_init(&newMethodId);
    UA_RCU_LOCK();
    UA_StatusCode retval = Service_AddNodes_existing(server, &adminSession, (UA_Node*)node, &parentNodeId,
                                                     &referenceTypeId, &UA_NODEID_NULL, NULL, &newMethodId);
    UA_RCU_UNLOCK();
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    const UA_NodeId hasproperty = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    const UA_NodeId propertytype = UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE);

    if(inputArgumentsSize > 0) {
        UA_VariableNode *inputArgumentsVariableNode = UA_NodeStore_newVariableNode();
        inputArgumentsVariableNode->nodeId.namespaceIndex = newMethodId.namespaceIndex;
        inputArgumentsVariableNode->browseName = UA_QUALIFIEDNAME_ALLOC(0, "InputArguments");
        inputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", "InputArguments");
        inputArgumentsVariableNode->description = UA_LOCALIZEDTEXT_ALLOC("en_US", "InputArguments");
        inputArgumentsVariableNode->valueRank = 1;

        /* UAExpert creates a monitoreditem on inputarguments ... */
        inputArgumentsVariableNode->minimumSamplingInterval = 10000.0f;

        //TODO: 0.3 work item: the addMethodNode API does not have the possibility to set nodeIDs
        //actually we need to change the signature to pass UA_NS0ID_SERVER_GETMONITOREDITEMS_INPUTARGUMENTS
        //and UA_NS0ID_SERVER_GETMONITOREDITEMS_OUTPUTARGUMENTS into the function :/
        if(newMethodId.namespaceIndex == 0 &&
           newMethodId.identifierType == UA_NODEIDTYPE_NUMERIC &&
           newMethodId.identifier.numeric == UA_NS0ID_SERVER_GETMONITOREDITEMS) {
            inputArgumentsVariableNode->nodeId =
                UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS_INPUTARGUMENTS);
        }
        UA_Variant_setArrayCopy(&inputArgumentsVariableNode->value.data.value.value,
                                inputArguments, inputArgumentsSize,
                                &UA_TYPES[UA_TYPES_ARGUMENT]);
        inputArgumentsVariableNode->value.data.value.hasValue = true;
        UA_RCU_LOCK();
        // todo: check if adding succeeded
        Service_AddNodes_existing(server, &adminSession, (UA_Node*)inputArgumentsVariableNode,
                                  &newMethodId, &hasproperty, &propertytype, NULL, NULL);
        UA_RCU_UNLOCK();
    }

    if(outputArgumentsSize > 0) {
        /* create OutputArguments */
        UA_VariableNode *outputArgumentsVariableNode  = UA_NodeStore_newVariableNode();
        outputArgumentsVariableNode->nodeId.namespaceIndex = newMethodId.namespaceIndex;
        outputArgumentsVariableNode->browseName  = UA_QUALIFIEDNAME_ALLOC(0, "OutputArguments");
        outputArgumentsVariableNode->displayName = UA_LOCALIZEDTEXT_ALLOC("en_US", "OutputArguments");
        outputArgumentsVariableNode->description = UA_LOCALIZEDTEXT_ALLOC("en_US", "OutputArguments");
        outputArgumentsVariableNode->valueRank = 1;
        //FIXME: comment in line 882
        if(newMethodId.namespaceIndex == 0 &&
           newMethodId.identifierType == UA_NODEIDTYPE_NUMERIC &&
           newMethodId.identifier.numeric == UA_NS0ID_SERVER_GETMONITOREDITEMS) {
            outputArgumentsVariableNode->nodeId =
                UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS_OUTPUTARGUMENTS);
        }
        UA_Variant_setArrayCopy(&outputArgumentsVariableNode->value.data.value.value,
                                outputArguments, outputArgumentsSize,
                                &UA_TYPES[UA_TYPES_ARGUMENT]);
        outputArgumentsVariableNode->value.data.value.hasValue = true;
        UA_RCU_LOCK();
        // todo: check if adding succeeded
        Service_AddNodes_existing(server, &adminSession, (UA_Node*)outputArgumentsVariableNode,
                                  &newMethodId, &hasproperty, &propertytype, NULL, NULL);
        UA_RCU_UNLOCK();
    }

    if(outNewNodeId)
        *outNewNodeId = newMethodId;
    else
        UA_NodeId_deleteMembers(&newMethodId);
    return retval;
}

#endif

/******************/
/* Add References */
/******************/

static UA_StatusCode
deleteOneWayReference(UA_Server *server, UA_Session *session, UA_Node *node,
                      const UA_DeleteReferencesItem *item);

/* Adds a one-way reference to the local nodestore */
static UA_StatusCode
addOneWayReference(UA_Server *server, UA_Session *session,
                   UA_Node *node, const UA_AddReferencesItem *item) {
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

static UA_StatusCode
addReference(UA_Server *server, UA_Session *session,
             const UA_AddReferencesItem *item) {
    /* Currently no expandednodeids are allowed */
    if(item->targetServerUri.length > 0)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Add the first direction */
    UA_StatusCode retval = UA_Server_editNode(server, session, &item->sourceNodeId,
                                              (UA_EditNodeCallback)addOneWayReference,
                                              item);


    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Add the second direction */
    UA_AddReferencesItem secondItem;
    UA_AddReferencesItem_init(&secondItem);
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.referenceTypeId = item->referenceTypeId;
    secondItem.isForward = !item->isForward;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    /* keep default secondItem.targetNodeClass = UA_NODECLASS_UNSPECIFIED */

    retval = UA_Server_editNode(server, session, &secondItem.sourceNodeId,
                                (UA_EditNodeCallback)addOneWayReference, &secondItem);

    /* remove reference if the second direction failed */
    if(retval != UA_STATUSCODE_GOOD) {
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
    return retval;
}

void Service_AddReferences(UA_Server *server, UA_Session *session,
                           const UA_AddReferencesRequest *request,
                           UA_AddReferencesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing AddReferencesRequest");
    if(request->referencesToAddSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_malloc(sizeof(UA_StatusCode) * request->referencesToAddSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->referencesToAddSize;

    for(size_t i = 0; i < response->resultsSize; ++i)
        response->results[i] =
            addReference(server, session, &request->referencesToAdd[i]);
}

UA_StatusCode
UA_Server_addReference(UA_Server *server, const UA_NodeId sourceId,
                       const UA_NodeId refTypeId, const UA_ExpandedNodeId targetId,
                       UA_Boolean isForward) {
    UA_AddReferencesItem item;
    UA_AddReferencesItem_init(&item);
    item.sourceNodeId = sourceId;
    item.referenceTypeId = refTypeId;
    item.isForward = isForward;
    item.targetNodeId = targetId;
    UA_RCU_LOCK();
    UA_StatusCode retval = addReference(server, &adminSession, &item);
    UA_RCU_UNLOCK();
    return retval;
}

/****************/
/* Delete Nodes */
/****************/

static void
removeReferences(UA_Server *server, UA_Session *session, const UA_Node *node) {
    UA_DeleteReferencesItem item;
    UA_DeleteReferencesItem_init(&item);
    item.targetNodeId.nodeId = node->nodeId;
    for(size_t i = 0; i < node->referencesSize; ++i) {
        item.isForward = node->references[i].isInverse;
        item.sourceNodeId = node->references[i].targetId.nodeId;
        item.referenceTypeId = node->references[i].referenceTypeId;
        deleteReference(server, session, &item);
    }
}

static UA_StatusCode
deleteNode(UA_Server *server, UA_Session *session,
           const UA_NodeId *nodeId, UA_Boolean deleteReferences) {
    const UA_Node *node = UA_NodeStore_get(server->nodestore, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    /* TODO: check if the information model consistency is violated */
    /* TODO: Check if the node is a mandatory child of an object */

    /* Destroy an object before removing it */
    if(node->nodeClass == UA_NODECLASS_OBJECT) {
        /* Call the destructor from the object type */
        const UA_ObjectTypeNode *typenode = getObjectNodeType(server, (const UA_ObjectNode*)node);
        if(typenode && typenode->lifecycleManagement.destructor)
            typenode->lifecycleManagement.destructor(*nodeId, ((const UA_ObjectNode*)node)->instanceHandle);
    }

    /* Remove references to the node (not the references in the node that will
     * be deleted anyway) */
    if(deleteReferences)
        removeReferences(server, session, node);

    return UA_NodeStore_remove(server->nodestore, nodeId);
}

void Service_DeleteNodes(UA_Server *server, UA_Session *session,
                         const UA_DeleteNodesRequest *request,
                         UA_DeleteNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing DeleteNodesRequest");
    if(request->nodesToDeleteSize == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_malloc(sizeof(UA_StatusCode) * request->nodesToDeleteSize);
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
    UA_RCU_LOCK();
    UA_StatusCode retval = deleteNode(server, &adminSession,
                                      &nodeId, deleteReferences);
    UA_RCU_UNLOCK();
    return retval;
}

/*********************/
/* Delete References */
/*********************/

// TODO: Check consistency constraints, remove the references.
static UA_StatusCode
deleteOneWayReference(UA_Server *server, UA_Session *session, UA_Node *node,
                      const UA_DeleteReferencesItem *item) {
    UA_Boolean edited = false;
    for(size_t i = node->referencesSize; i > 0; --i) {
        UA_ReferenceNode *ref = &node->references[i-1];
        if(!UA_NodeId_equal(&item->targetNodeId.nodeId, &ref->targetId.nodeId))
            continue;
        if(!UA_NodeId_equal(&item->referenceTypeId, &ref->referenceTypeId))
            continue;
        if(item->isForward == ref->isInverse)
            continue;
        UA_ReferenceNode_deleteMembers(ref);
        /* move the last entry to override the current position */
        node->references[i-1] = node->references[node->referencesSize-1];
        --node->referencesSize;
        edited = true;
        break;
    }
    if(!edited)
        return UA_STATUSCODE_UNCERTAINREFERENCENOTDELETED;
    /* we removed the last reference */
    if(node->referencesSize == 0 && node->references) {
        UA_free(node->references);
        node->references = NULL;
    }
    return UA_STATUSCODE_GOOD;;
}

static UA_StatusCode
deleteReference(UA_Server *server, UA_Session *session,
                const UA_DeleteReferencesItem *item) {
    UA_StatusCode retval = UA_Server_editNode(server, session, &item->sourceNodeId,
                                              (UA_EditNodeCallback)deleteOneWayReference, item);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(!item->deleteBidirectional || item->targetNodeId.serverIndex != 0)
        return retval;
    UA_DeleteReferencesItem secondItem;
    UA_DeleteReferencesItem_init(&secondItem);
    secondItem.isForward = !item->isForward;
    secondItem.sourceNodeId = item->targetNodeId.nodeId;
    secondItem.targetNodeId.nodeId = item->sourceNodeId;
    secondItem.referenceTypeId = item->referenceTypeId;
    return UA_Server_editNode(server, session, &secondItem.sourceNodeId,
                              (UA_EditNodeCallback)deleteOneWayReference, &secondItem);
}

void
Service_DeleteReferences(UA_Server *server, UA_Session *session,
                         const UA_DeleteReferencesRequest *request,
                         UA_DeleteReferencesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing DeleteReferencesRequest");
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

    for(size_t i = 0; i < request->referencesToDeleteSize; ++i)
        response->results[i] =
            deleteReference(server, session, &request->referencesToDelete[i]);
}

UA_StatusCode
UA_Server_deleteReference(UA_Server *server, const UA_NodeId sourceNodeId,
                          const UA_NodeId referenceTypeId,
                          UA_Boolean isForward, const UA_ExpandedNodeId targetNodeId,
                          UA_Boolean deleteBidirectional) {
    UA_DeleteReferencesItem item;
    item.sourceNodeId = sourceNodeId;
    item.referenceTypeId = referenceTypeId;
    item.isForward = isForward;
    item.targetNodeId = targetNodeId;
    item.deleteBidirectional = deleteBidirectional;
    UA_RCU_LOCK();
    UA_StatusCode retval = deleteReference(server, &adminSession, &item);
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
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession, &nodeId,
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
    UA_RCU_LOCK();
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession, &nodeId,
                                              (UA_EditNodeCallback)setDataSource, &dataSource);
    UA_RCU_UNLOCK();
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
    UA_RCU_LOCK();
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession, &nodeId,
                                              (UA_EditNodeCallback)setOLM, &olm);
    UA_RCU_UNLOCK();
    return retval;
}

/***********************/
/* Set Method Callback */
/***********************/

#ifdef UA_ENABLE_METHODCALLS

struct addMethodCallback {
    UA_MethodCallback callback;
    void *handle;
};

static UA_StatusCode
editMethodCallback(UA_Server *server, UA_Session* session,
                   UA_Node* node, const void* handle) {
    if(node->nodeClass != UA_NODECLASS_METHOD)
        return UA_STATUSCODE_BADNODECLASSINVALID;
    const struct addMethodCallback *newCallback = handle;
    UA_MethodNode *mnode = (UA_MethodNode*) node;
    mnode->attachedMethod = newCallback->callback;
    mnode->methodHandle   = newCallback->handle;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setMethodNode_callback(UA_Server *server, const UA_NodeId methodNodeId,
                                 UA_MethodCallback method, void *handle) {
    struct addMethodCallback cb = { method, handle };
    UA_RCU_LOCK();
    UA_StatusCode retval = UA_Server_editNode(server, &adminSession,
                                              &methodNodeId, editMethodCallback, &cb);
    UA_RCU_UNLOCK();
    return retval;
}

#endif
