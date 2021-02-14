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
#include "aa_tree.h"

static enum aa_cmp
cmpRefTargetId(const void *a, const void *b) {
    const UA_ReferenceTarget *aa = (const UA_ReferenceTarget*)a;
    const UA_ReferenceTarget *bb = (const UA_ReferenceTarget*)b;
    if(aa->targetIdHash < bb->targetIdHash)
        return AA_CMP_LESS;
    if(aa->targetIdHash > bb->targetIdHash)
        return AA_CMP_MORE;
    return (enum aa_cmp)UA_ExpandedNodeId_order(&aa->targetId, &bb->targetId);
}

static enum aa_cmp
cmpRefTargetName(const void *a, const void *b) {
    const UA_UInt32 *nameHashA = (const UA_UInt32*)a;
    const UA_UInt32 *nameHashB = (const UA_UInt32*)b;
    if(*nameHashA < *nameHashB)
        return AA_CMP_LESS;
    if(*nameHashA > *nameHashB)
        return AA_CMP_MORE;
    return AA_CMP_EQ;
}

/* Reusable binary search tree "heads". Just switch out the root pointer. */
static const struct aa_head idTreeHead =
    { NULL, cmpRefTargetId, offsetof(UA_ReferenceTarget, idTreeEntry), 0 };
const struct aa_head nameTreeHead =
    { NULL, cmpRefTargetName, offsetof(UA_ReferenceTarget, nameTreeEntry),
      offsetof(UA_ReferenceTarget, targetNameHash) };

/* General node handling methods. There is no UA_Node_new() method here.
 * Creating nodes is part of the Nodestore layer */

void UA_Node_clear(UA_Node *node) {
    /* Delete references */
    UA_Node_deleteReferences(node);

    /* Delete other head content */
    UA_NodeHead *head = &node->head;
    UA_NodeId_clear(&head->nodeId);
    UA_QualifiedName_clear(&head->browseName);
    UA_LocalizedText_clear(&head->displayName);
    UA_LocalizedText_clear(&head->description);

    /* Delete unique content of the nodeclass */
    switch(head->nodeClass) {
    case UA_NODECLASS_OBJECT:
        break;
    case UA_NODECLASS_METHOD:
        break;
    case UA_NODECLASS_OBJECTTYPE:
        break;
    case UA_NODECLASS_VARIABLE:
    case UA_NODECLASS_VARIABLETYPE: {
        UA_VariableNode *p = &node->variableNode;
        UA_NodeId_clear(&p->dataType);
        UA_Array_delete(p->arrayDimensions, p->arrayDimensionsSize,
                        &UA_TYPES[UA_TYPES_INT32]);
        p->arrayDimensions = NULL;
        p->arrayDimensionsSize = 0;
        if(p->valueSource == UA_VALUESOURCE_DATA)
            UA_DataValue_clear(&p->value.data.value);
        break;
    }
    case UA_NODECLASS_REFERENCETYPE: {
        UA_ReferenceTypeNode *p = &node->referenceTypeNode;
        UA_LocalizedText_clear(&p->inverseName);
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
    UA_StatusCode retval =
        UA_Array_copy(src->arrayDimensions, src->arrayDimensionsSize,
                      (void**)&dst->arrayDimensions, &UA_TYPES[UA_TYPES_INT32]);
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
    dst->accessLevel = src->accessLevel;
    dst->minimumSamplingInterval = src->minimumSamplingInterval;
    dst->historizing = src->historizing;
    dst->isDynamic = src->isDynamic;
    return UA_CommonVariableNode_copy(src, dst);
}

static UA_StatusCode
UA_VariableTypeNode_copy(const UA_VariableTypeNode *src,
                         UA_VariableTypeNode *dst) {
    dst->isAbstract = src->isAbstract;
    return UA_CommonVariableNode_copy((const UA_VariableNode*)src, (UA_VariableNode*)dst);
}

static UA_StatusCode
UA_MethodNode_copy(const UA_MethodNode *src, UA_MethodNode *dst) {
    dst->executable = src->executable;
    dst->method = src->method;
#if UA_MULTITHREADING >= 100
    dst->async = src->async;
#endif
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
    dst->isAbstract = src->isAbstract;
    dst->symmetric = src->symmetric;
    dst->referenceTypeIndex = src->referenceTypeIndex;
    dst->subTypes = src->subTypes;
    return UA_LocalizedText_copy(&src->inverseName, &dst->inverseName);
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

static UA_StatusCode
addReferenceTarget(UA_NodeReferenceKind *refs, const UA_ExpandedNodeId *target,
                   UA_UInt32 targetIdHash, UA_UInt32 targetNameHash);

UA_StatusCode
UA_Node_copy(const UA_Node *src, UA_Node *dst) {
    const UA_NodeHead *srchead = &src->head;
    UA_NodeHead *dsthead = &dst->head;
    if(srchead->nodeClass != dsthead->nodeClass)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Copy standard content */
    UA_StatusCode retval = UA_NodeId_copy(&srchead->nodeId, &dsthead->nodeId);
    retval |= UA_QualifiedName_copy(&srchead->browseName, &dsthead->browseName);
    retval |= UA_LocalizedText_copy(&srchead->displayName, &dsthead->displayName);
    retval |= UA_LocalizedText_copy(&srchead->description, &dsthead->description);
    dsthead->writeMask = srchead->writeMask;
    dsthead->context = srchead->context;
    dsthead->constructed = srchead->constructed;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Node_clear(dst);
        return retval;
    }

    /* Copy the references */
    dsthead->references = NULL;
    if(srchead->referencesSize > 0) {
        dsthead->references = (UA_NodeReferenceKind*)
            UA_calloc(srchead->referencesSize, sizeof(UA_NodeReferenceKind));
        if(!dsthead->references) {
            UA_Node_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dsthead->referencesSize = srchead->referencesSize;

        for(size_t i = 0; i < srchead->referencesSize; ++i) {
            UA_NodeReferenceKind *srefs = &srchead->references[i];

            UA_NodeReferenceKind *drefs = &dsthead->references[i];
            drefs->referenceTypeIndex = srefs->referenceTypeIndex;
            drefs->isInverse = srefs->isInverse;
            TAILQ_INIT(&drefs->queueHead);
            drefs->idTreeRoot = NULL;
            drefs->nameTreeRoot = NULL;

            UA_ReferenceTarget *sTarget;
            TAILQ_FOREACH(sTarget, &srefs->queueHead, queuePointers) {
                retval = addReferenceTarget(drefs, &sTarget->targetId,
                                            sTarget->targetIdHash, sTarget->targetNameHash);
                if(retval != UA_STATUSCODE_GOOD)
                    break;
            }
            if(retval != UA_STATUSCODE_GOOD)
                break;
        }

        if(retval != UA_STATUSCODE_GOOD) {
            UA_Node_clear(dst);
            return retval;
        }
    }

    /* Copy unique content of the nodeclass */
    switch(src->head.nodeClass) {
    case UA_NODECLASS_OBJECT:
        retval = UA_ObjectNode_copy(&src->objectNode, &dst->objectNode);
        break;
    case UA_NODECLASS_VARIABLE:
        retval = UA_VariableNode_copy(&src->variableNode, &dst->variableNode);
        break;
    case UA_NODECLASS_METHOD:
        retval = UA_MethodNode_copy(&src->methodNode, &dst->methodNode);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        retval = UA_ObjectTypeNode_copy(&src->objectTypeNode, &dst->objectTypeNode);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        retval = UA_VariableTypeNode_copy(&src->variableTypeNode, &dst->variableTypeNode);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        retval = UA_ReferenceTypeNode_copy(&src->referenceTypeNode, &dst->referenceTypeNode);
        break;
    case UA_NODECLASS_DATATYPE:
        retval = UA_DataTypeNode_copy(&src->dataTypeNode, &dst->dataTypeNode);
        break;
    case UA_NODECLASS_VIEW:
        retval = UA_ViewNode_copy(&src->viewNode, &dst->viewNode);
        break;
    default:
        break;
    }

    if(retval != UA_STATUSCODE_GOOD)
        UA_Node_clear(dst);

    return retval;
}

UA_Node *
UA_Node_copy_alloc(const UA_Node *src) {
    size_t nodesize = 0;
    switch(src->head.nodeClass) {
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

    UA_Node *dst = (UA_Node*)UA_calloc(1, nodesize);
    if(!dst)
        return NULL;

    dst->head.nodeClass = src->head.nodeClass;

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
copyStandardAttributes(UA_NodeHead *head, const UA_NodeAttributes *attr) {
    /* UA_NodeId_copy(&item->requestedNewNodeId.nodeId, &node->nodeId); */
    /* UA_QualifiedName_copy(&item->browseName, &node->browseName); */

    head->writeMask = attr->writeMask;
    UA_StatusCode retval = UA_LocalizedText_copy(&attr->description, &head->description);
    /* The new nodeset format has optional display names:
     * https://github.com/open62541/open62541/issues/2627. If the display name
     * is NULL, take the name part of the browse name */
    if(attr->displayName.text.length == 0)
        retval |= UA_String_copy(&head->browseName.name, &head->displayName.text);
    else
        retval |= UA_LocalizedText_copy(&attr->displayName, &head->displayName);
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
    retval = UA_Variant_copy(&attr->value, &node->value.data.value.value);
    node->valueSource = UA_VALUESOURCE_DATA;
    node->value.data.value.hasValue = (node->value.data.value.value.type != NULL);

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
UA_Node_setAttributes(UA_Node *node, const void *attributes, const UA_DataType *attributeType) {
    /* Copy the attributes into the node */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    switch(node->head.nodeClass) {
    case UA_NODECLASS_OBJECT:
        CHECK_ATTRIBUTES(OBJECTATTRIBUTES);
        retval = copyObjectNodeAttributes(&node->objectNode,
                                          (const UA_ObjectAttributes*)attributes);
        break;
    case UA_NODECLASS_VARIABLE:
        CHECK_ATTRIBUTES(VARIABLEATTRIBUTES);
        retval = copyVariableNodeAttributes(&node->variableNode,
                                            (const UA_VariableAttributes*)attributes);
        break;
    case UA_NODECLASS_OBJECTTYPE:
        CHECK_ATTRIBUTES(OBJECTTYPEATTRIBUTES);
        retval = copyObjectTypeNodeAttributes(&node->objectTypeNode,
                                              (const UA_ObjectTypeAttributes*)attributes);
        break;
    case UA_NODECLASS_VARIABLETYPE:
        CHECK_ATTRIBUTES(VARIABLETYPEATTRIBUTES);
        retval = copyVariableTypeNodeAttributes(&node->variableTypeNode,
                                                (const UA_VariableTypeAttributes*)attributes);
        break;
    case UA_NODECLASS_REFERENCETYPE:
        CHECK_ATTRIBUTES(REFERENCETYPEATTRIBUTES);
        retval = copyReferenceTypeNodeAttributes(&node->referenceTypeNode,
                                                 (const UA_ReferenceTypeAttributes*)attributes);
        break;
    case UA_NODECLASS_DATATYPE:
        CHECK_ATTRIBUTES(DATATYPEATTRIBUTES);
        retval = copyDataTypeNodeAttributes(&node->dataTypeNode,
                                            (const UA_DataTypeAttributes*)attributes);
        break;
    case UA_NODECLASS_VIEW:
        CHECK_ATTRIBUTES(VIEWATTRIBUTES);
        retval = copyViewNodeAttributes(&node->viewNode, (const UA_ViewAttributes*)attributes);
        break;
    case UA_NODECLASS_METHOD:
        CHECK_ATTRIBUTES(METHODATTRIBUTES);
        retval = copyMethodNodeAttributes(&node->methodNode, (const UA_MethodAttributes*)attributes);
        break;
    case UA_NODECLASS_UNSPECIFIED:
    default:
        retval = UA_STATUSCODE_BADNODECLASSINVALID;
    }

    if(retval == UA_STATUSCODE_GOOD)
        retval = copyStandardAttributes(&node->head, (const UA_NodeAttributes*)attributes);
    if(retval != UA_STATUSCODE_GOOD)
        UA_Node_clear(node);
    return retval;
}

/*********************/
/* Manage References */
/*********************/

static UA_StatusCode
addReferenceTarget(UA_NodeReferenceKind *refs, const UA_ExpandedNodeId *target,
                   UA_UInt32 targetIdHash, UA_UInt32 targetNameHash) {
    UA_ReferenceTarget *entry = (UA_ReferenceTarget*)
        UA_malloc(sizeof(UA_ReferenceTarget));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval = UA_ExpandedNodeId_copy(target, &entry->targetId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(entry);
        return retval;
    }

    entry->targetIdHash = targetIdHash;
    entry->targetNameHash = targetNameHash;
    TAILQ_INSERT_TAIL(&refs->queueHead, entry, queuePointers);

    /* Insert to the id lookup binary search tree. Only the root is kept in refs
     * to save space. */
    struct aa_head _idTreeHead = idTreeHead;
    _idTreeHead.root = refs->idTreeRoot;
    aa_insert(&_idTreeHead, entry);
    refs->idTreeRoot = _idTreeHead.root;

    /* Insert to the name lookup binary search tree */
    struct aa_head _nameTreeHead = nameTreeHead;
    _nameTreeHead.root = refs->nameTreeRoot;
    aa_insert(&_nameTreeHead, entry);
    refs->nameTreeRoot = _nameTreeHead.root;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addReferenceKind(UA_NodeHead *head, UA_Byte refTypeIndex, UA_Boolean isForward,
                 const UA_ExpandedNodeId *targetNodeId, UA_UInt32 targetBrowseNameHash) {
    UA_NodeReferenceKind *refs = (UA_NodeReferenceKind*)
        UA_realloc(head->references, sizeof(UA_NodeReferenceKind) * (head->referencesSize+1));
    if(!refs)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Repair the backpointer in the queue */
    for(size_t i = 0; i < head->referencesSize; i++)
        TAILQ_FIRST(&refs[i].queueHead)->queuePointers.tqe_prev = &refs[i].queueHead.tqh_first;
    head->references = refs;

    UA_NodeReferenceKind *newRef = &refs[head->referencesSize];
    newRef->referenceTypeIndex = refTypeIndex;
    newRef->isInverse = !isForward;
    TAILQ_INIT(&newRef->queueHead);
    newRef->idTreeRoot = NULL;
    newRef->nameTreeRoot = NULL;
    UA_StatusCode retval =
        addReferenceTarget(newRef, targetNodeId,
                           UA_ExpandedNodeId_hash(targetNodeId),
                           targetBrowseNameHash);
    if(retval != UA_STATUSCODE_GOOD) {
        if(head->referencesSize == 0) {
            UA_free(head->references);
            head->references = NULL;
        }
        return retval;
    }

    head->referencesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Node_addReference(UA_Node *node, UA_Byte refTypeIndex, UA_Boolean isForward,
                     const UA_ExpandedNodeId *targetNodeId,
                     UA_UInt32 targetBrowseNameHash) {
    struct aa_head _idTreeHead = idTreeHead;

    /* Find the matching reference kind */
    for(size_t i = 0; i < node->head.referencesSize; ++i) {
        UA_NodeReferenceKind *refs = &node->head.references[i];

        /* Reference direction does not match */
        if(refs->isInverse == isForward)
            continue;

        /* Reference type does not match */
        if(refs->referenceTypeIndex != refTypeIndex)
            continue;

        /* Does an identical reference already exist? */
        UA_ReferenceTarget tmpTarget;
        tmpTarget.targetId = *targetNodeId;
        tmpTarget.targetIdHash = UA_ExpandedNodeId_hash(targetNodeId);
        _idTreeHead.root = refs->idTreeRoot;
        UA_ReferenceTarget *found = (UA_ReferenceTarget*)
            aa_find(&_idTreeHead, &tmpTarget);
        if(found)
            return UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED;

        /* Add to existing ReferenceKind */
        return addReferenceTarget(refs, targetNodeId, tmpTarget.targetIdHash,
                                  targetBrowseNameHash);
    }

    /* Add new ReferenceKind for the target */
    return addReferenceKind(&node->head, refTypeIndex, isForward,
                            targetNodeId, targetBrowseNameHash);

}

UA_StatusCode
UA_Node_deleteReference(UA_Node *node, UA_Byte refTypeIndex, UA_Boolean isForward,
                        const UA_ExpandedNodeId *targetNodeId) {
    struct aa_head _idTreeHead = idTreeHead;
    struct aa_head _nameTreeHead = nameTreeHead;

    UA_NodeHead *head = &node->head;
    for(size_t i = head->referencesSize; i > 0; --i) {
        UA_NodeReferenceKind *refs = &head->references[i-1];
        if(isForward == refs->isInverse)
            continue;
        if(refTypeIndex != refs->referenceTypeIndex)
            continue;

        _idTreeHead.root = refs->idTreeRoot;
        _nameTreeHead.root = refs->nameTreeRoot;

        UA_ReferenceTarget tmpTarget;
        tmpTarget.targetId = *targetNodeId;
        tmpTarget.targetIdHash = UA_ExpandedNodeId_hash(targetNodeId);
        UA_ReferenceTarget *target = (UA_ReferenceTarget*)
            aa_find(&_idTreeHead, &tmpTarget);
        if(!target)
            continue;

        /* Ok, delete the reference */
        TAILQ_REMOVE(&refs->queueHead, target, queuePointers);
        aa_remove(&_idTreeHead, target);
        aa_remove(&_nameTreeHead, target);
        UA_ExpandedNodeId_clear(&target->targetId);
        UA_free(target);

        refs->idTreeRoot = _idTreeHead.root;
        refs->nameTreeRoot = _nameTreeHead.root;

        if(!TAILQ_EMPTY(&refs->queueHead))
            return UA_STATUSCODE_GOOD; /* At least one target remains for the refkind */

        head->referencesSize--;
        if(head->referencesSize > 0) {
            /* No target for the ReferenceType remaining. Remove and shrink down
             * allocated buffer. */
            if(i-1 != head->referencesSize)
                head->references[i-1] = head->references[node->head.referencesSize];
            UA_NodeReferenceKind *newRefs = (UA_NodeReferenceKind*)
                UA_realloc(head->references, sizeof(UA_NodeReferenceKind) * head->referencesSize);
            /* Ignore errors in case memory buffer could not be shrinked down */
            if(newRefs) {
                /* Repair the backpointer in the queue */
                for(size_t j = 0; j < head->referencesSize; j++)
                    TAILQ_FIRST(&newRefs[j].queueHead)->queuePointers.tqe_prev =
                        &newRefs[j].queueHead.tqh_first;
                head->references = newRefs;
            }
        } else {
            /* No remaining references of any ReferenceType */
            UA_free(head->references);
            head->references = NULL;
        }
        return UA_STATUSCODE_GOOD;
    }
    return UA_STATUSCODE_UNCERTAINREFERENCENOTDELETED;
}

void
UA_Node_deleteReferencesSubset(UA_Node *node, const UA_ReferenceTypeSet *keepSet) {
    UA_NodeHead *head = &node->head;
    for(size_t i = head->referencesSize; i > 0; --i) {
        /* Keep the references of this type? */
        UA_NodeReferenceKind *refs = &head->references[i-1];
        if(UA_ReferenceTypeSet_contains(keepSet, refs->referenceTypeIndex))
            continue;

        /* Remove all target entries. Don't remove entries from the tree. The
         * entire ReferenceKind will be removed anyway. */
        UA_ReferenceTarget *target, *tmp;
        TAILQ_FOREACH_SAFE(target, &refs->queueHead, queuePointers, tmp) {
            TAILQ_REMOVE(&refs->queueHead, target, queuePointers);
            UA_ExpandedNodeId_clear(&target->targetId);
            UA_free(target);
        }
        head->referencesSize--;

        /* Move last references-kind entry to this position */
        if(i-1 != head->referencesSize) /* Don't memcpy over the same position */
            head->references[i-1] = head->references[head->referencesSize];
    }

    if(head->referencesSize > 0) {
        /* Realloc to save memory */
        UA_NodeReferenceKind *refs = (UA_NodeReferenceKind*)
            UA_realloc(head->references, sizeof(UA_NodeReferenceKind) * head->referencesSize);
        /* Ignore if realloc fails */
        if(refs) {
            /* Repair the backpointer in the queue */
            for(size_t i = 0; i < head->referencesSize; i++)
                TAILQ_FIRST(&refs[i].queueHead)->queuePointers.tqe_prev =
                    &refs[i].queueHead.tqh_first;
            head->references = refs;
        }
    } else {
        /* The array is empty. Remove. */
        UA_free(head->references);
        head->references = NULL;
    }
}

void UA_Node_deleteReferences(UA_Node *node) {
    UA_ReferenceTypeSet noRefs;
    UA_ReferenceTypeSet_init(&noRefs);
    UA_Node_deleteReferencesSubset(node, &noRefs);
}
