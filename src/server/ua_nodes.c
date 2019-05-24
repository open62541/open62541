/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2015-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015, 2017 (c) Florian Palm
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 */

#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"

/* There is no UA_Node_new() method here. Creating nodes is part of the
 * NodeStore layer */

void UA_Node_deleteMembers(UA_Node *node) {
    /* Delete standard content */
    UA_NodeId_deleteMembers(&node->nodeId);
    UA_QualifiedName_deleteMembers(&node->browseName);
    UA_LocalizedText_deleteMembers(&node->displayName);
    UA_LocalizedText_deleteMembers(&node->description);

    /* Delete references */
    UA_Node_deleteReferences(node);

    /* Delete unique content of the nodeclass */
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        break;
    case UA_NODECLASS_METHOD:
        break;
    case UA_NODECLASS_OBJECTTYPE:
        break;
    case UA_NODECLASS_VARIABLE:
    case UA_NODECLASS_VARIABLETYPE: {
        UA_VariableNode *p = (UA_VariableNode*)node;
        UA_NodeId_deleteMembers(&p->dataType);
        UA_Array_delete(p->arrayDimensions, p->arrayDimensionsSize,
                        &UA_TYPES[UA_TYPES_INT32]);
        p->arrayDimensions = NULL;
        p->arrayDimensionsSize = 0;
        if(p->valueSource == UA_VALUESOURCE_DATA)
            UA_DataValue_deleteMembers(&p->value.data.value);
        break;
    }
    case UA_NODECLASS_REFERENCETYPE: {
        UA_ReferenceTypeNode *p = (UA_ReferenceTypeNode*)node;
        UA_LocalizedText_deleteMembers(&p->inverseName);
        break;
    }
    case UA_NODECLASS_DATATYPE:
        break;
    case UA_NODECLASS_VIEW:
        break;
    default:
        break;
    }
}

static UA_StatusCode
UA_ObjectNode_copy(const UA_ObjectNode *src, UA_ObjectNode *dst) {
    dst->eventNotifier = src->eventNotifier;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_CommonVariableNode_copy(const UA_VariableNode *src, UA_VariableNode *dst) {
    UA_StatusCode retval = UA_Array_copy(src->arrayDimensions,
                                         src->arrayDimensionsSize,
                                         (void**)&dst->arrayDimensions,
                                         &UA_TYPES[UA_TYPES_INT32]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    dst->arrayDimensionsSize = src->arrayDimensionsSize;
    retval = UA_NodeId_copy(&src->dataType, &dst->dataType);
    dst->valueRank = src->valueRank;
    dst->valueSource = src->valueSource;
    if(src->valueSource == UA_VALUESOURCE_DATA) {
        retval |= UA_DataValue_copy(&src->value.data.value,
                                    &dst->value.data.value);
        dst->value.data.callback = src->value.data.callback;
    } else
        dst->value.dataSource = src->value.dataSource;
    return retval;
}

static UA_StatusCode
UA_VariableNode_copy(const UA_VariableNode *src, UA_VariableNode *dst) {
    UA_StatusCode retval = UA_CommonVariableNode_copy(src, dst);
    dst->accessLevel = src->accessLevel;
    dst->minimumSamplingInterval = src->minimumSamplingInterval;
    dst->historizing = src->historizing;
    return retval;
}

static UA_StatusCode
UA_VariableTypeNode_copy(const UA_VariableTypeNode *src,
                         UA_VariableTypeNode *dst) {
    UA_StatusCode retval = UA_CommonVariableNode_copy((const UA_VariableNode*)src,
                                                      (UA_VariableNode*)dst);
    dst->isAbstract = src->isAbstract;
    return retval;
}

static UA_StatusCode
UA_MethodNode_copy(const UA_MethodNode *src, UA_MethodNode *dst) {
    dst->executable = src->executable;
    dst->method = src->method;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ObjectTypeNode_copy(const UA_ObjectTypeNode *src, UA_ObjectTypeNode *dst) {
    dst->isAbstract = src->isAbstract;
    dst->lifecycle = src->lifecycle;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ReferenceTypeNode_copy(const UA_ReferenceTypeNode *src,
                          UA_ReferenceTypeNode *dst) {
    UA_StatusCode retval = UA_LocalizedText_copy(&src->inverseName,
                                                 &dst->inverseName);
    dst->isAbstract = src->isAbstract;
    dst->symmetric = src->symmetric;
    return retval;
}

static UA_StatusCode
UA_DataTypeNode_copy(const UA_DataTypeNode *src, UA_DataTypeNode *dst) {
    dst->isAbstract = src->isAbstract;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ViewNode_copy(const UA_ViewNode *src, UA_ViewNode *dst) {
    dst->containsNoLoops = src->containsNoLoops;
    dst->eventNotifier = src->eventNotifier;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Node_copy(const UA_Node *src, UA_Node *dst) {
    if(src->nodeClass != dst->nodeClass)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Copy standard content */
    UA_StatusCode retval = UA_NodeId_copy(&src->nodeId, &dst->nodeId);
    retval |= UA_QualifiedName_copy(&src->browseName, &dst->browseName);
    retval |= UA_LocalizedText_copy(&src->displayName, &dst->displayName);
    retval |= UA_LocalizedText_copy(&src->description, &dst->description);
    dst->writeMask = src->writeMask;
    dst->context = src->context;
    dst->constructed = src->constructed;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Node_deleteMembers(dst);
        return retval;
    }

    /* Copy the references */
    dst->references = NULL;
    if(src->referencesSize > 0) {
        dst->references = (UA_NodeReferenceKind*)
            UA_calloc(src->referencesSize, sizeof(UA_NodeReferenceKind));
        if(!dst->references) {
            UA_Node_deleteMembers(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->referencesSize = src->referencesSize;

        for(size_t i = 0; i < src->referencesSize; ++i) {
            UA_NodeReferenceKind *srefs = &src->references[i];
            UA_NodeReferenceKind *drefs = &dst->references[i];
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
        }
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Node_deleteMembers(dst);
            return retval;
        }
    }

    /* Copy unique content of the nodeclass */
    switch(src->nodeClass) {
    case UA_NODECLASS_OBJECT:
        retval = UA_ObjectNode_copy((const UA_ObjectNode*)src, (UA_ObjectNode*)dst);
        break;
    case UA_NODECLASS_VARIABLE:
        retval = UA_VariableNode_copy((const UA_VariableNode*)src, (UA_VariableNode*)dst);
        break;
    case UA_NODECLASS_METHOD:
        retval = UA_MethodNode_copy((const UA_MethodNode*)src, (UA_MethodNode*)dst);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        retval = UA_ObjectTypeNode_copy((const UA_ObjectTypeNode*)src, (UA_ObjectTypeNode*)dst);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        retval = UA_VariableTypeNode_copy((const UA_VariableTypeNode*)src, (UA_VariableTypeNode*)dst);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        retval = UA_ReferenceTypeNode_copy((const UA_ReferenceTypeNode*)src, (UA_ReferenceTypeNode*)dst);
        break;
    case UA_NODECLASS_DATATYPE:
        retval = UA_DataTypeNode_copy((const UA_DataTypeNode*)src, (UA_DataTypeNode*)dst);
        break;
    case UA_NODECLASS_VIEW:
        retval = UA_ViewNode_copy((const UA_ViewNode*)src, (UA_ViewNode*)dst);
        break;
    default:
        break;
    }

    if(retval != UA_STATUSCODE_GOOD)
        UA_Node_deleteMembers(dst);

    return retval;
}

UA_Node *
UA_Node_copy_alloc(const UA_Node *src) {
    /* use dstPtr to trick static code analysis in accepting dirty cast */
    size_t nodesize = 0;
    switch(src->nodeClass) {
        case UA_NODECLASS_OBJECT:
            nodesize = sizeof(UA_ObjectNode);
            break;
        case UA_NODECLASS_VARIABLE:
            nodesize = sizeof(UA_VariableNode);
            break;
        case UA_NODECLASS_METHOD:
            nodesize = sizeof(UA_MethodNode);
            break;
        case UA_NODECLASS_OBJECTTYPE:
            nodesize = sizeof(UA_ObjectTypeNode);
            break;
        case UA_NODECLASS_VARIABLETYPE:
            nodesize = sizeof(UA_VariableTypeNode);
            break;
        case UA_NODECLASS_REFERENCETYPE:
            nodesize = sizeof(UA_ReferenceTypeNode);
            break;
        case UA_NODECLASS_DATATYPE:
            nodesize = sizeof(UA_DataTypeNode);
            break;
        case UA_NODECLASS_VIEW:
            nodesize = sizeof(UA_ViewNode);
            break;
        default:
            return NULL;
    }

    UA_Node *dst = (UA_Node*)UA_calloc(1,nodesize);
    if(!dst)
        return NULL;

    dst->nodeClass = src->nodeClass;

    UA_StatusCode retval = UA_Node_copy(src, dst);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(dst);
        return NULL;
    }
    return dst;
}
/******************************/
/* Copy Attributes into Nodes */
/******************************/

static UA_StatusCode
copyStandardAttributes(UA_Node *node, const UA_NodeAttributes *attr) {
    /* retval  = UA_NodeId_copy(&item->requestedNewNodeId.nodeId, &node->nodeId); */
    /* retval |= UA_QualifiedName_copy(&item->browseName, &node->browseName); */

    UA_StatusCode retval;
    /* The new nodeset format has optional display name.
     * See https://github.com/open62541/open62541/issues/2627
     * If display name is NULL, then we take the name part of the browse name */
    if (attr->displayName.text.length == 0) {
        retval = UA_String_copy(&node->browseName.name,
                                       &node->displayName.text);
    } else {
        retval = UA_LocalizedText_copy(&attr->displayName,
                                                     &node->displayName);
        retval |= UA_LocalizedText_copy(&attr->description, &node->description);
    }

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
    retval = UA_NodeId_copy(&attr->dataType, &node->dataType);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    node->valueRank = attr->valueRank;

    /* Copy the value */
    node->valueSource = UA_VALUESOURCE_DATA;
    UA_NodeId extensionObject = UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE);
    /* If we have an extension object which is still encoded (e.g. from the
     * nodeset compiler) return an error.
     * This was used in the old version of the nodeset compiler and is not
     * needed anymore. */
    if(attr->value.type != NULL && UA_NodeId_equal(&attr->value.type->typeId, &extensionObject)) {
        /* Do nothing since we got an empty array of extension objects */
        if(attr->value.data == UA_EMPTY_ARRAY_SENTINEL)
            return UA_STATUSCODE_GOOD;

        const UA_ExtensionObject *obj = (const UA_ExtensionObject *)attr->value.data;
        if(obj && obj->encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING) {
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
    }

    retval = UA_Variant_copy(&attr->value, &node->value.data.value.value);

    node->value.data.value.hasValue = node->value.data.value.value.type != NULL;

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

static UA_StatusCode
copyMethodNodeAttributes(UA_MethodNode *mnode,
                         const UA_MethodAttributes *attr) {
    mnode->executable = attr->executable;
    return UA_STATUSCODE_GOOD;
}

#define CHECK_ATTRIBUTES(TYPE)                           \
    if(attributeType != &UA_TYPES[UA_TYPES_##TYPE]) {    \
        retval = UA_STATUSCODE_BADNODEATTRIBUTESINVALID; \
        break;                                           \
    }

UA_StatusCode
UA_Node_setAttributes(UA_Node *node, const void *attributes,
                      const UA_DataType *attributeType) {
    /* Copy the attributes into the node */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(node->nodeClass) {
    case UA_NODECLASS_OBJECT:
        CHECK_ATTRIBUTES(OBJECTATTRIBUTES);
        retval = copyObjectNodeAttributes((UA_ObjectNode*)node,
                                          (const UA_ObjectAttributes*)attributes);
        break;
    case UA_NODECLASS_VARIABLE:
        CHECK_ATTRIBUTES(VARIABLEATTRIBUTES);
        retval = copyVariableNodeAttributes((UA_VariableNode*)node,
                                            (const UA_VariableAttributes*)attributes);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        CHECK_ATTRIBUTES(OBJECTTYPEATTRIBUTES);
        retval = copyObjectTypeNodeAttributes((UA_ObjectTypeNode*)node,
                                              (const UA_ObjectTypeAttributes*)attributes);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        CHECK_ATTRIBUTES(VARIABLETYPEATTRIBUTES);
        retval = copyVariableTypeNodeAttributes((UA_VariableTypeNode*)node,
                                                (const UA_VariableTypeAttributes*)attributes);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        CHECK_ATTRIBUTES(REFERENCETYPEATTRIBUTES);
        retval = copyReferenceTypeNodeAttributes((UA_ReferenceTypeNode*)node,
                                                 (const UA_ReferenceTypeAttributes*)attributes);
        break;
    case UA_NODECLASS_DATATYPE:
        CHECK_ATTRIBUTES(DATATYPEATTRIBUTES);
        retval = copyDataTypeNodeAttributes((UA_DataTypeNode*)node,
                                            (const UA_DataTypeAttributes*)attributes);
        break;
    case UA_NODECLASS_VIEW:
        CHECK_ATTRIBUTES(VIEWATTRIBUTES);
        retval = copyViewNodeAttributes((UA_ViewNode*)node,
                                        (const UA_ViewAttributes*)attributes);
        break;
    case UA_NODECLASS_METHOD:
        CHECK_ATTRIBUTES(METHODATTRIBUTES);
        retval = copyMethodNodeAttributes((UA_MethodNode*)node,
                                          (const UA_MethodAttributes*)attributes);
        break;
    case UA_NODECLASS_UNSPECIFIED:
    default:
        retval = UA_STATUSCODE_BADNODECLASSINVALID;
    }

    if(retval == UA_STATUSCODE_GOOD)
        retval = copyStandardAttributes(node, (const UA_NodeAttributes*)attributes);
    if(retval != UA_STATUSCODE_GOOD)
        UA_Node_deleteMembers(node);
    return retval;
}

/*********************/
/* Manage References */
/*********************/

static UA_StatusCode
addReferenceTarget(UA_NodeReferenceKind *refs, const UA_ExpandedNodeId *target) {
    UA_ExpandedNodeId *targets =
        (UA_ExpandedNodeId*) UA_realloc(refs->targetIds,
                                        sizeof(UA_ExpandedNodeId) * (refs->targetIdsSize+1));
    if(!targets)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    refs->targetIds = targets;
    UA_StatusCode retval =
        UA_ExpandedNodeId_copy(target, &refs->targetIds[refs->targetIdsSize]);

    if(retval == UA_STATUSCODE_GOOD) {
        refs->targetIdsSize++;
    } else if(refs->targetIdsSize == 0) {
        /* We had zero references before (realloc was a malloc) */
        UA_free(refs->targetIds);
        refs->targetIds = NULL;
    }
    return retval;
}

static UA_StatusCode
addReferenceKind(UA_Node *node, const UA_AddReferencesItem *item) {
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
    retval |= addReferenceTarget(newRef, &item->targetNodeId);

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

UA_StatusCode
UA_Node_addReference(UA_Node *node, const UA_AddReferencesItem *item) {
    UA_NodeReferenceKind *existingRefs = NULL;
    for(size_t i = 0; i < node->referencesSize; ++i) {
        UA_NodeReferenceKind *refs = &node->references[i];
        if(refs->isInverse != item->isForward
                && UA_NodeId_equal(&refs->referenceTypeId, &item->referenceTypeId)) {
            existingRefs = refs;
            break;
        }
    }
    if(existingRefs != NULL) {
        for(size_t i = 0; i < existingRefs->targetIdsSize; i++) {
            if(UA_ExpandedNodeId_equal(&existingRefs->targetIds[i],
                                       &item->targetNodeId)) {
                return UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED;
            }
        }
        return addReferenceTarget(existingRefs, &item->targetNodeId);
    }
    return addReferenceKind(node, item);
}

UA_StatusCode
UA_Node_deleteReference(UA_Node *node, const UA_DeleteReferencesItem *item) {
    for(size_t i = node->referencesSize; i > 0; --i) {
        UA_NodeReferenceKind *refs = &node->references[i-1];
        if(item->isForward == refs->isInverse)
            continue;
        if(!UA_NodeId_equal(&item->referenceTypeId, &refs->referenceTypeId))
            continue;

        for(size_t j = refs->targetIdsSize; j > 0; --j) {
            UA_ExpandedNodeId *target = &refs->targetIds[j-1];
            if(!UA_NodeId_equal(&item->targetNodeId.nodeId, &target->nodeId))
                continue;

            /* Ok, delete the reference */
            UA_ExpandedNodeId_deleteMembers(target);
            refs->targetIdsSize--;

            /* One matching target remaining */
            if(refs->targetIdsSize > 0) {
                if(j-1 != refs->targetIdsSize) // avoid valgrind error: Source
                                               // and destination overlap in
                                               // memcpy
                    *target = refs->targetIds[refs->targetIdsSize];
                return UA_STATUSCODE_GOOD;
            }

            /* No target for the ReferenceType remaining. Remove entry. */
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

            /* No remaining references of any ReferenceType */
            UA_free(node->references);
            node->references = NULL;
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_UNCERTAINREFERENCENOTDELETED;
}

void
UA_Node_deleteReferencesSubset(UA_Node *node, size_t referencesSkipSize,
                               UA_NodeId* referencesSkip) {
    /* Nothing to do */
    if(node->referencesSize == 0 || node->references == NULL)
        return;

    for(size_t i = node->referencesSize; i > 0; --i) {
        UA_NodeReferenceKind *refs = &node->references[i-1];

        /* Shall we keep the references of this type? */
        UA_Boolean skip = false;
        for(size_t j = 0; j < referencesSkipSize; j++) {
            if(UA_NodeId_equal(&refs->referenceTypeId, &referencesSkip[j])) {
                skip = true;
                break;
            }
        }
        if(skip)
            continue;

        /* Remove references */
        UA_Array_delete(refs->targetIds, refs->targetIdsSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
        UA_NodeId_deleteMembers(&refs->referenceTypeId);
        node->referencesSize--;

        /* Move last references-kind entry to this position */
        if(i-1 == node->referencesSize) /* Don't memcpy over the same position */
            continue;
        node->references[i-1] = node->references[node->referencesSize];
    }

    if(node->referencesSize == 0) {
        /* The array is empty. Remove. */
        UA_free(node->references);
        node->references = NULL;
    } else {
        /* Realloc to save memory */
        UA_NodeReferenceKind *refs = (UA_NodeReferenceKind*)
            UA_realloc(node->references, sizeof(UA_NodeReferenceKind) * node->referencesSize);
        if(refs) /* Do nothing if realloc fails */
            node->references = refs;
    }
}

void UA_Node_deleteReferences(UA_Node *node) {
    UA_Node_deleteReferencesSubset(node, 0, NULL);
}
