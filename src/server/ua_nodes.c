/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2015-2018, 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015, 2017 (c) Florian Palm
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Julian Grothoff
 */

#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"

/*********************/
/* ReferenceType Set */
/*********************/

#define UA_REFTYPES_ALL_MASK (~(UA_UInt32)0)
#define UA_REFTYPES_ALL_MASK2 UA_REFTYPES_ALL_MASK, UA_REFTYPES_ALL_MASK
#define UA_REFTYPES_ALL_MASK4 UA_REFTYPES_ALL_MASK2, UA_REFTYPES_ALL_MASK2
#if (UA_REFERENCETYPESET_MAX) / 32 > 8
# error Adjust macros to support than 256 reference types
#elif (UA_REFERENCETYPESET_MAX) / 32 == 8
# define UA_REFTYPES_ALL_ARRAY UA_REFTYPES_ALL_MASK4, UA_REFTYPES_ALL_MASK4
#elif (UA_REFERENCETYPESET_MAX) / 32 == 7
# define UA_REFTYPES_ALL_ARRAY                                          \
    UA_REFTYPES_ALL_MASK4, UA_REFTYPES_ALL_MASK2, UA_REFTYPES_ALL_MASK
#elif (UA_REFERENCETYPESET_MAX) / 32 == 6
# define UA_REFTYPES_ALL_ARRAY UA_REFTYPES_ALL_MASK4, UA_REFTYPES_ALL_MASK2
#elif (UA_REFERENCETYPESET_MAX) / 32 == 5
# define UA_REFTYPES_ALL_ARRAY UA_REFTYPES_ALL_MASK4, UA_REFTYPES_ALL_MASK
#elif (UA_REFERENCETYPESET_MAX) / 32 == 4
# define UA_REFTYPES_ALL_ARRAY UA_REFTYPES_ALL_MASK4
#elif (UA_REFERENCETYPESET_MAX) / 32 == 3
# define UA_REFTYPES_ALL_ARRAY UA_REFTYPES_ALL_MASK2, UA_REFTYPES_ALL_MASK
#elif (UA_REFERENCETYPESET_MAX) / 32 == 2
# define UA_REFTYPES_ALL_ARRAY UA_REFTYPES_ALL_MASK2
#else
# define UA_REFTYPES_ALL_ARRAY UA_REFTYPES_ALL_MASK
#endif

const UA_ReferenceTypeSet UA_REFERENCETYPESET_NONE = {{0}};
const UA_ReferenceTypeSet UA_REFERENCETYPESET_ALL = {{UA_REFTYPES_ALL_ARRAY}};

/*****************/
/* Node Pointers */
/*****************/

#define UA_NODEPOINTER_MASK 0x03
#define UA_NODEPOINTER_TAG_IMMEDIATE 0x00
#define UA_NODEPOINTER_TAG_NODEID 0x01
#define UA_NODEPOINTER_TAG_EXPANDEDNODEID 0x02
#define UA_NODEPOINTER_TAG_NODE 0x03

void
UA_NodePointer_clear(UA_NodePointer *np) {
    switch(np->immediate & UA_NODEPOINTER_MASK) {
    case UA_NODEPOINTER_TAG_NODEID:
        np->immediate &= ~(uintptr_t)UA_NODEPOINTER_MASK;
        UA_NodeId_delete((UA_NodeId*)(uintptr_t)np->id);
        break;
    case UA_NODEPOINTER_TAG_EXPANDEDNODEID:
        np->immediate &= ~(uintptr_t)UA_NODEPOINTER_MASK;
        UA_ExpandedNodeId_delete((UA_ExpandedNodeId*)(uintptr_t)
                                 np->expandedId);
        break;
    default:
        break;
    }
    UA_NodePointer_init(np);
}

UA_StatusCode
UA_NodePointer_copy(UA_NodePointer in, UA_NodePointer *out) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_Byte tag = in.immediate & UA_NODEPOINTER_MASK;
    in.immediate &= ~(uintptr_t)UA_NODEPOINTER_MASK;
    switch(tag) {
    case UA_NODEPOINTER_TAG_NODE:
        in.id = &in.node->nodeId;
        goto nodeid; /* fallthrough */
    case UA_NODEPOINTER_TAG_NODEID:
    nodeid:
        out->id = UA_NodeId_new();
        if(!out->id)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        res = UA_NodeId_copy(in.id, (UA_NodeId*)(uintptr_t)out->id);
        if(res != UA_STATUSCODE_GOOD) {
            UA_free((void*)out->immediate);
            out->immediate = 0;
            break;
        }
        out->immediate |= UA_NODEPOINTER_TAG_NODEID;
        break;
    case UA_NODEPOINTER_TAG_EXPANDEDNODEID:
        out->expandedId = UA_ExpandedNodeId_new();
        if(!out->expandedId)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        res = UA_ExpandedNodeId_copy(in.expandedId,
                                     (UA_ExpandedNodeId*)(uintptr_t)
                                     out->expandedId);
        if(res != UA_STATUSCODE_GOOD) {
            UA_free((void*)out->immediate);
            out->immediate = 0;
            break;
        }
        out->immediate |= UA_NODEPOINTER_TAG_EXPANDEDNODEID;
        break;
    default:
    case UA_NODEPOINTER_TAG_IMMEDIATE:
        *out = in;
        break;
    }
    return res;
}

UA_Boolean
UA_NodePointer_isLocal(UA_NodePointer np) {
    UA_Byte tag = np.immediate & UA_NODEPOINTER_MASK;
    return (tag != UA_NODEPOINTER_TAG_EXPANDEDNODEID);
}

UA_Order
UA_NodePointer_order(UA_NodePointer p1, UA_NodePointer p2) {
    if(p1.immediate == p2.immediate)
        return UA_ORDER_EQ;

    /* Extract the tag and resolve pointers to nodes */
    UA_Byte tag1 = p1.immediate & UA_NODEPOINTER_MASK;
    if(tag1 == UA_NODEPOINTER_TAG_NODE) {
        p1 = UA_NodePointer_fromNodeId(&p1.node->nodeId);
        tag1 = p1.immediate & UA_NODEPOINTER_MASK;
    }
    UA_Byte tag2 = p2.immediate & UA_NODEPOINTER_MASK;
    if(tag2 == UA_NODEPOINTER_TAG_NODE) {
        p2 = UA_NodePointer_fromNodeId(&p2.node->nodeId);
        tag2 = p2.immediate & UA_NODEPOINTER_MASK;
    }

    /* Different tags, cannot be identical */
    if(tag1 != tag2)
        return (tag1 > tag2) ? UA_ORDER_MORE : UA_ORDER_LESS;

    /* Immediate */
    if(UA_LIKELY(tag1 == UA_NODEPOINTER_TAG_IMMEDIATE))
        return (p1.immediate > p2.immediate) ?
            UA_ORDER_MORE : UA_ORDER_LESS;

    /* Compare from pointers */
    p1.immediate &= ~(uintptr_t)UA_NODEPOINTER_MASK;
    p2.immediate &= ~(uintptr_t)UA_NODEPOINTER_MASK;
    if(tag1 == UA_NODEPOINTER_TAG_EXPANDEDNODEID)
        return UA_ExpandedNodeId_order(p1.expandedId, p2.expandedId);
    return UA_NodeId_order(p1.id, p2.id);
}

UA_NodePointer
UA_NodePointer_fromNodeId(const UA_NodeId *id) {
    UA_NodePointer np;
    if(id->identifierType != UA_NODEIDTYPE_NUMERIC) {
        np.id = id;
        np.immediate |= UA_NODEPOINTER_TAG_NODEID;
        return np;
    }

#if SIZE_MAX > UA_UINT32_MAX
    /* 64bit: 4 Byte for the numeric identifier + 2 Byte for the namespaceIndex
     *        + 1 Byte for the tagging bit (zero) */
    np.immediate  = ((uintptr_t)id->identifier.numeric) << 32;
    np.immediate |= ((uintptr_t)id->namespaceIndex) << 8;
#else
    /* 32bit: 3 Byte for the numeric identifier + 6 Bit for the namespaceIndex
     *        + 2 Bit for the tagging bit (zero) */
    if(id->namespaceIndex < (0x01 << 6) &&
       id->identifier.numeric < (0x01 << 24)) {
        np.immediate  = ((uintptr_t)id->identifier.numeric) << 8;
        np.immediate |= ((uintptr_t)id->namespaceIndex) << 2;
    } else {
        np.id = id;
        np.immediate |= UA_NODEPOINTER_TAG_NODEID;
    }
#endif
    return np;
}

UA_NodeId
UA_NodePointer_toNodeId(UA_NodePointer np) {
    UA_Byte tag = np.immediate & UA_NODEPOINTER_MASK;
    np.immediate &= ~(uintptr_t)UA_NODEPOINTER_MASK;
    switch(tag) {
    case UA_NODEPOINTER_TAG_NODE:
        return np.node->nodeId;
    case UA_NODEPOINTER_TAG_NODEID:
        return *np.id;
    case UA_NODEPOINTER_TAG_EXPANDEDNODEID:
        return np.expandedId->nodeId;
    default:
    case UA_NODEPOINTER_TAG_IMMEDIATE:
        break;
    }

    UA_NodeId id;
    id.identifierType = UA_NODEIDTYPE_NUMERIC;
#if SIZE_MAX > UA_UINT32_MAX /* 64bit */
    id.namespaceIndex = (UA_UInt16)(np.immediate >> 8);
    id.identifier.numeric = (UA_UInt32)(np.immediate >> 32);
#else                        /* 32bit */
    id.namespaceIndex = ((UA_Byte)np.immediate) >> 2;
    id.identifier.numeric = np.immediate >> 8;
#endif
    return id;
}

UA_NodePointer
UA_NodePointer_fromExpandedNodeId(const UA_ExpandedNodeId *id) {
    if(!UA_ExpandedNodeId_isLocal(id)) {
        UA_NodePointer np;
        np.expandedId = id;
        np.immediate |= UA_NODEPOINTER_TAG_EXPANDEDNODEID;
        return np;
    }
    return UA_NodePointer_fromNodeId(&id->nodeId);
}

UA_ExpandedNodeId
UA_NodePointer_toExpandedNodeId(UA_NodePointer np) {
    /* Resolve node pointer to get the NodeId */
    UA_Byte tag = np.immediate & UA_NODEPOINTER_MASK;
    if(tag == UA_NODEPOINTER_TAG_NODE) {
        np = UA_NodePointer_fromNodeId(&np.node->nodeId);
        tag = np.immediate & UA_NODEPOINTER_MASK;
    }

    /* ExpandedNodeId, make a shallow copy */
    if(tag == UA_NODEPOINTER_TAG_EXPANDEDNODEID) {
        np.immediate &= ~(uintptr_t)UA_NODEPOINTER_MASK;
        return *np.expandedId;
    }

    /* NodeId, either immediate or via a pointer */
    UA_ExpandedNodeId en;
    UA_ExpandedNodeId_init(&en);
    en.nodeId = UA_NodePointer_toNodeId(np);
    return en;
}

/**************/
/* References */
/**************/

static UA_StatusCode
addReferenceTarget(UA_NodeReferenceKind *refs, UA_NodePointer target,
                   UA_UInt32 targetNameHash);

static UA_StatusCode
addReferenceTargetToTree(UA_NodeReferenceKind *rk, UA_NodePointer targetId,
                         UA_UInt32 targetIdHash, UA_UInt32 targetNameHash);

enum ZIP_CMP
cmpRefTargetId(const void *a, const void *b) {
    const UA_ReferenceTargetTreeElem *aa = (const UA_ReferenceTargetTreeElem*)a;
    const UA_ReferenceTargetTreeElem *bb = (const UA_ReferenceTargetTreeElem*)b;
    if(aa->targetIdHash < bb->targetIdHash)
        return ZIP_CMP_LESS;
    if(aa->targetIdHash > bb->targetIdHash)
        return ZIP_CMP_MORE;
    return (enum ZIP_CMP)UA_NodePointer_order(aa->target.targetId,
                                              bb->target.targetId);
}

enum ZIP_CMP
cmpRefTargetName(const void *a, const void *b) {
    const UA_ReferenceTargetTreeElem *aa = (const UA_ReferenceTargetTreeElem*)a;
    const UA_ReferenceTargetTreeElem *bb = (const UA_ReferenceTargetTreeElem*)b;
    if(aa->target.targetNameHash == bb->target.targetNameHash)
        return ZIP_CMP_EQ;
    return (aa->target.targetNameHash < bb->target.targetNameHash) ?
        ZIP_CMP_LESS : ZIP_CMP_MORE;
}

/* Move to the array in-order, also deletes the tree elements */
static void
moveTreeToArray(UA_ReferenceTarget *array, size_t *pos,
                UA_ReferenceTargetTreeElem *elem) {
    if(!elem)
        return;
    moveTreeToArray(array, pos, elem->idTreeEntry.left);
    array[*pos] = elem->target;
    (*pos)++;
    moveTreeToArray(array, pos, elem->idTreeEntry.right);
    UA_free(elem);
}

static void *
removeTreeEntry(void *context, UA_ReferenceTargetTreeElem *elem) {
    (void)context;
    UA_NodePointer_clear(&elem->target.targetId);
    UA_free(elem);
    return NULL;
}

UA_StatusCode
UA_NodeReferenceKind_switch(UA_NodeReferenceKind *rk) {
    UA_assert(rk->targetsSize > 0);

    if(rk->hasRefTree) {
        /* From tree to array */
        UA_ReferenceTarget *array = (UA_ReferenceTarget*)
            UA_malloc(sizeof(UA_ReferenceTarget) * rk->targetsSize);
        if(!array)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        size_t pos = 0;
        moveTreeToArray(array, &pos, rk->targets.tree.idRoot);
        rk->targets.array = array;
        rk->hasRefTree = false;
        return UA_STATUSCODE_GOOD;
    }

    /* From array to tree */
    UA_NodeReferenceKind newRk = *rk;
    newRk.hasRefTree = true;
    newRk.targets.tree.idRoot = NULL;
    newRk.targets.tree.nameRoot = NULL;
    newRk.targetsSize = 0;
    for(size_t i = 0; i < rk->targetsSize; i++) {
        UA_StatusCode res =
            addReferenceTarget(&newRk, rk->targets.array[i].targetId,
                               rk->targets.array[i].targetNameHash);
        if(res != UA_STATUSCODE_GOOD) {
            ZIP_ITER(UA_ReferenceIdTree,
                     (UA_ReferenceIdTree*)&newRk.targets.tree.idRoot,
                     removeTreeEntry, NULL);
            return res;
        }
    }
    for(size_t i = 0; i < rk->targetsSize; i++)
        UA_NodePointer_clear(&rk->targets.array[i].targetId);
    UA_free(rk->targets.array);
    *rk = newRk;
    return UA_STATUSCODE_GOOD;
}

void *
UA_NodeReferenceKind_iterate(UA_NodeReferenceKind *rk,
                             UA_NodeReferenceKind_iterateCallback callback,
                             void *context) {
    if(rk->hasRefTree)
        return ZIP_ITER(UA_ReferenceIdTree,
                        (UA_ReferenceIdTree*)&rk->targets.tree.idRoot,
                        (UA_ReferenceIdTree_cb)callback, context);
    for(size_t i = 0; i < rk->targetsSize; i++) {
        void *res = callback(context, &rk->targets.array[i]);
        if(res)
            return res;
    }
    return NULL;
}

const UA_ReferenceTarget *
UA_NodeReferenceKind_findTarget(const UA_NodeReferenceKind *rk,
                                const UA_ExpandedNodeId *targetId) {
    UA_NodePointer targetP = UA_NodePointer_fromExpandedNodeId(targetId);
    if(rk->hasRefTree) {
        /* Return from the tree */
        UA_ReferenceTargetTreeElem tmpTarget;
        tmpTarget.target.targetId = targetP;
        tmpTarget.targetIdHash = UA_ExpandedNodeId_hash(targetId);
        UA_ReferenceTargetTreeElem *result =
            ZIP_FIND(UA_ReferenceIdTree, (UA_ReferenceIdTree*)
                     (uintptr_t)&rk->targets.tree.idRoot, &tmpTarget);
        if(result)
            return &result->target;
    } else {
        /* Return from the array */
        for(size_t i = 0; i < rk->targetsSize; i++) {
            if(UA_NodePointer_equal(targetP, rk->targets.array[i].targetId))
                return &rk->targets.array[i];
        }
    }
    return NULL;
}

/* General node handling methods. There is no UA_Node_new() method here.
 * Creating nodes is part of the Nodestore layer */

void UA_Node_clear(UA_Node *node) {
    /* Delete references */
    UA_Node_deleteReferences(node);

    /* Delete other head content */
    UA_NodeHead *head = &node->head;
    UA_NodeId_clear(&head->nodeId);
    UA_QualifiedName_clear(&head->browseName);

    UA_LocalizedTextListEntry *lt;

    while((lt = head->displayName)) {
        head->displayName = lt->next;
        UA_LocalizedText_clear(&lt->localizedText);
        UA_free(lt);
    }

    while((lt = head->description)) {
        head->description = lt->next;
        UA_LocalizedText_clear(&lt->localizedText);
        UA_free(lt);
    }

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
    dst->valueBackend = src->valueBackend;
    dst->valueSource = src->valueSource;
    if(src->valueSource == UA_VALUESOURCE_DATA) {
        retval |= UA_DataValue_copy(&src->value.data.value,
                                    &dst->value.data.value);
        dst->value.data.callback = src->value.data.callback;
    } else {
        dst->value.dataSource = src->value.dataSource;
    }
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

static void *
copyTarget(void *context, UA_ReferenceTargetTreeElem *elm) {
    UA_NodeReferenceKind *drefs = (UA_NodeReferenceKind*)context;
    return (void*)(uintptr_t)
        addReferenceTargetToTree(drefs, elm->target.targetId,
                                 elm->targetIdHash,
                                 elm->target.targetNameHash);
}

UA_StatusCode
UA_Node_copy(const UA_Node *src, UA_Node *dst) {
    const UA_NodeHead *srchead = &src->head;
    UA_NodeHead *dsthead = &dst->head;
    if(srchead->nodeClass != dsthead->nodeClass)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Copy standard content */
    UA_StatusCode retval = UA_NodeId_copy(&srchead->nodeId, &dsthead->nodeId);
    retval |= UA_QualifiedName_copy(&srchead->browseName, &dsthead->browseName);

    /* Copy the display name in several languages */
    for(UA_LocalizedTextListEntry *lt = srchead->displayName; lt != NULL; lt = lt->next) {
        UA_LocalizedTextListEntry *newEntry = (UA_LocalizedTextListEntry *)
            UA_calloc(1, sizeof(UA_LocalizedTextListEntry));
        if(!newEntry) {
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
            break;
        }
        retval |= UA_LocalizedText_copy(&lt->localizedText, &newEntry->localizedText);

        /* Add to the linked list possibly in reverse order */
        newEntry->next = dsthead->displayName;
        dsthead->displayName = newEntry;
    }

    /* Copy the description in several languages */
    for(UA_LocalizedTextListEntry *lt = srchead->description; lt != NULL; lt = lt->next) {
        UA_LocalizedTextListEntry *newEntry = (UA_LocalizedTextListEntry *)
            UA_calloc(1, sizeof(UA_LocalizedTextListEntry));
        if(!newEntry) {
            retval |= UA_STATUSCODE_BADOUTOFMEMORY;
            break;
        }
        retval |= UA_LocalizedText_copy(&lt->localizedText, &newEntry->localizedText);

        /* Add to the linked list possibly in reverse order */
        newEntry->next = dsthead->description;
        dsthead->description= newEntry;
    }

    dsthead->writeMask = srchead->writeMask;
    dsthead->context = srchead->context;
    dsthead->constructed = srchead->constructed;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    dsthead->monitoredItems = srchead->monitoredItems;
#endif
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
            drefs->hasRefTree = srefs->hasRefTree; /* initially empty */

            /* Copy all the targets */
            if(!srefs->hasRefTree) {
                drefs->targets.array = (UA_ReferenceTarget*)
                    UA_malloc(sizeof(UA_ReferenceTarget) * srefs->targetsSize);
                if(!drefs->targets.array) {
                    UA_Node_clear(dst);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                for(size_t j = 0; j < srefs->targetsSize; j++) {
                    drefs->targets.array[j].targetNameHash =
                        srefs->targets.array[j].targetNameHash;
                    retval = UA_NodePointer_copy(srefs->targets.array[j].targetId,
                                                 &drefs->targets.array[j].targetId);
                    drefs->targetsSize++; /* avoid that targetsSize == 0 in error case */
                    if(retval != UA_STATUSCODE_GOOD) {
                        UA_Node_clear(dst);
                        return retval;
                    }
                }
            } else {
                void *res = ZIP_ITER(UA_ReferenceIdTree,
                                     (UA_ReferenceIdTree*)&srefs->targets.tree.idRoot,
                                     copyTarget, drefs);
                if(res != NULL) {
                    UA_Node_clear(dst);
                    return (UA_StatusCode)(uintptr_t)res;
                }
            }

            UA_assert(srefs->targetsSize > 0);
            UA_assert(srefs->targetsSize == drefs->targetsSize);
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
    UA_StatusCode retval = UA_Node_insertOrUpdateDescription(head, &attr->description);
    /* The new nodeset format has optional display names:
     * https://github.com/open62541/open62541/issues/2627. If the display name
     * is NULL, take the name part of the browse name */
    if(attr->displayName.text.length == 0) {
        UA_LocalizedText lt;
        UA_LocalizedText_init(&lt);
        lt.text = head->browseName.name;
        retval |= UA_Node_insertOrUpdateDisplayName(head, &lt);
    } else
        retval |= UA_Node_insertOrUpdateDisplayName(head, &attr->displayName);
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
addReferenceTargetToTree(UA_NodeReferenceKind *rk, UA_NodePointer targetId,
                         UA_UInt32 targetIdHash, UA_UInt32 targetNameHash) {
    UA_ReferenceTargetTreeElem *entry = (UA_ReferenceTargetTreeElem*)
        UA_malloc(sizeof(UA_ReferenceTargetTreeElem));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode res = UA_NodePointer_copy(targetId, &entry->target.targetId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(entry);
        return res;
    }

    entry->targetIdHash = targetIdHash;
    entry->target.targetNameHash = targetNameHash;

    ZIP_INSERT(UA_ReferenceIdTree,
               (UA_ReferenceIdTree*)&rk->targets.tree.idRoot, entry);
    ZIP_INSERT(UA_ReferenceNameTree,
               (UA_ReferenceNameTree*)&rk->targets.tree.nameRoot, entry);

    rk->targetsSize++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addReferenceTarget(UA_NodeReferenceKind *rk, UA_NodePointer targetId,
                   UA_UInt32 targetNameHash) {
    /* Insert into tree */
    if(rk->hasRefTree) {
        UA_ExpandedNodeId en = UA_NodePointer_toExpandedNodeId(targetId);
        return addReferenceTargetToTree(rk, targetId, UA_ExpandedNodeId_hash(&en),
                                        targetNameHash);
    }

    /* Insert to the array */
    UA_ReferenceTarget *newRefs = (UA_ReferenceTarget*)
        UA_realloc(rk->targets.array,
                   sizeof(UA_ReferenceTarget) * (rk->targetsSize + 1));
    if(!newRefs)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    rk->targets.array = newRefs;

    UA_StatusCode retval =
        UA_NodePointer_copy(targetId,
                            &rk->targets.array[rk->targetsSize].targetId);
    rk->targets.array[rk->targetsSize].targetNameHash = targetNameHash;
    if(retval != UA_STATUSCODE_GOOD) {
        if(rk->targetsSize == 0) {
            UA_free(rk->targets.array);
            rk->targets.array = NULL;
        }
        return retval;
    }

    rk->targetsSize++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addReferenceKind(UA_NodeHead *head, UA_Byte refTypeIndex, UA_Boolean isForward,
                 const UA_NodePointer target, UA_UInt32 targetBrowseNameHash) {
    UA_NodeReferenceKind *refs = (UA_NodeReferenceKind*)
        UA_realloc(head->references,
                   sizeof(UA_NodeReferenceKind) * (head->referencesSize+1));
    if(!refs)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    head->references = refs;

    UA_NodeReferenceKind *newRef = &refs[head->referencesSize];
    memset(newRef, 0, sizeof(UA_NodeReferenceKind));
    newRef->referenceTypeIndex = refTypeIndex;
    newRef->isInverse = !isForward;
    UA_StatusCode res = addReferenceTarget(newRef, target, targetBrowseNameHash);
    if(res != UA_STATUSCODE_GOOD) {
        if(head->referencesSize == 0) {
            UA_free(head->references);
            head->references = NULL;
        }
        return res;
    }

    head->referencesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Node_addReference(UA_Node *node, UA_Byte refTypeIndex, UA_Boolean isForward,
                     const UA_ExpandedNodeId *targetNodeId,
                     UA_UInt32 targetBrowseNameHash) {
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
        const UA_ReferenceTarget *found =
            UA_NodeReferenceKind_findTarget(refs, targetNodeId);
        if(found)
            return UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED;

        /* Add to existing ReferenceKind */
        return addReferenceTarget(refs, UA_NodePointer_fromExpandedNodeId(targetNodeId),
                                  targetBrowseNameHash);
    }

    /* Add new ReferenceKind for the target */
    return addReferenceKind(&node->head, refTypeIndex, isForward,
                            UA_NodePointer_fromExpandedNodeId(targetNodeId),
                            targetBrowseNameHash);

}

UA_StatusCode
UA_Node_deleteReference(UA_Node *node, UA_Byte refTypeIndex, UA_Boolean isForward,
                        const UA_ExpandedNodeId *targetNodeId) {
    UA_NodeHead *head = &node->head;
    for(size_t i = 0; i < head->referencesSize; i++) {
        UA_NodeReferenceKind *refs = &head->references[i];
        if(isForward == refs->isInverse)
            continue;
        if(refTypeIndex != refs->referenceTypeIndex)
            continue;

        /* Cast out the const qualifier (hack!) */
        UA_ReferenceTarget *target = (UA_ReferenceTarget*)(uintptr_t)
            UA_NodeReferenceKind_findTarget(refs, targetNodeId);
        if(!target)
            continue;

        /* Ok, delete the reference. Cannot fail */
        refs->targetsSize--;

        if(!refs->hasRefTree) {
            /* Remove from array */
            UA_NodePointer_clear(&target->targetId);

            /* Elements remaining. Realloc. */
            if(refs->targetsSize > 0) {
                if(target != &refs->targets.array[refs->targetsSize])
                    *target = refs->targets.array[refs->targetsSize];
                UA_ReferenceTarget *newRefs = (UA_ReferenceTarget*)
                    UA_realloc(refs->targets.array,
                               sizeof(UA_ReferenceTarget) * refs->targetsSize);
                if(newRefs)
                    refs->targets.array = newRefs;
                return UA_STATUSCODE_GOOD; /* Realloc allowed to fail */
            }

            /* Remove the last target. Remove the ReferenceKind below */
            UA_free(refs->targets.array);
        } else {
            UA_ReferenceTargetTreeElem *elem = (UA_ReferenceTargetTreeElem*)target;
            ZIP_REMOVE(UA_ReferenceIdTree,
                       (UA_ReferenceIdTree*)&refs->targets.tree.idRoot, elem);
            ZIP_REMOVE(UA_ReferenceNameTree,
                       (UA_ReferenceNameTree*)&refs->targets.tree.nameRoot, elem);
            UA_NodePointer_clear(&target->targetId);
            UA_free(target);
            if(refs->targetsSize > 0)
                return UA_STATUSCODE_GOOD;
        }

        /* No targets remaining. Remove the ReferenceKind. */
        head->referencesSize--;
        if(head->referencesSize > 0) {
            /* No target for the ReferenceType remaining. Remove and shrink down
             * allocated buffer. Ignore errors in case memory buffer could not
             * be shrinked down. */
            if(i != head->referencesSize)
                head->references[i] = head->references[node->head.referencesSize];
            UA_NodeReferenceKind *newRefs = (UA_NodeReferenceKind*)
                UA_realloc(head->references,
                           sizeof(UA_NodeReferenceKind) * head->referencesSize);
            if(newRefs)
                head->references = newRefs;
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
    for(size_t i = 0; i < head->referencesSize; i++) {
        /* Keep the references of this type? */
        UA_NodeReferenceKind *refs = &head->references[i];
        if(UA_ReferenceTypeSet_contains(keepSet, refs->referenceTypeIndex))
            continue;

        /* Remove all target entries. Don't remove entries from browseName tree.
         * The entire ReferenceKind will be removed anyway. */
        if(!refs->hasRefTree) {
            for(size_t j = 0; j < refs->targetsSize; j++)
                UA_NodePointer_clear(&refs->targets.array[j].targetId);
            UA_free(refs->targets.array);
        } else {
            ZIP_ITER(UA_ReferenceIdTree,
                     (UA_ReferenceIdTree*)&refs->targets.tree.idRoot,
                     removeTreeEntry, NULL);
        }

        /* Move last references-kind entry to this position. Don't memcpy over
         * the same position. Decrease i to repeat at this location. */
        head->referencesSize--;
        if(i != head->referencesSize) {
            head->references[i] = head->references[head->referencesSize];
            i--;
        }
    }

    if(head->referencesSize > 0) {
        /* Realloc to save memory. Ignore if realloc fails. */
        UA_NodeReferenceKind *refs = (UA_NodeReferenceKind*)
            UA_realloc(head->references,
                       sizeof(UA_NodeReferenceKind) * head->referencesSize);
        if(refs)
            head->references = refs;
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

static UA_StatusCode
UA_Node_insertOrUpdateLocale(UA_LocalizedTextListEntry **root,
                             const UA_LocalizedText *value) {
    UA_StatusCode res;
    UA_LocalizedTextListEntry *lt, *prev = NULL;
    for(lt = *root; lt != NULL; prev = lt, lt = lt->next) {
        if(!UA_String_equal(&value->locale, &lt->localizedText.locale))
            continue;

        /* No text -> remove the entry for this locale */
        if(value->text.length == 0) {
            if(prev == NULL)
                *root = lt->next;
            else
                prev->next = lt->next;
            UA_LocalizedText_clear(&lt->localizedText);
            UA_free(lt);
            return UA_STATUSCODE_GOOD;
        }

        /* First make a copy of the text, if this succeeds replace the old
         * version */
        UA_String tmp;
        res = UA_String_copy(&value->text, &tmp);
        if(res != UA_STATUSCODE_GOOD)
            return res;

        UA_String_clear(&lt->localizedText.text);
        lt->localizedText.text = tmp;
        return UA_STATUSCODE_GOOD;
    }

    /* The locale does not exist so far */

    /* Do nothing if a non-existing locale should be removed */
    if(value->text.length == 0)
        return UA_STATUSCODE_GOOD;

    /* Add a new localized text */
    lt = (UA_LocalizedTextListEntry *)UA_malloc(sizeof(UA_LocalizedTextListEntry));
    if(!lt)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    res = UA_LocalizedText_copy(value, &lt->localizedText);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(lt);
        return res;
    }

    lt->next = *root;
    *root = lt;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Node_insertOrUpdateDisplayName(UA_NodeHead *head,
                                  const UA_LocalizedText *value) {
    return UA_Node_insertOrUpdateLocale(&head->displayName, value);
}

UA_StatusCode
UA_Node_insertOrUpdateDescription(UA_NodeHead *head,
                                  const UA_LocalizedText *value) {
    return UA_Node_insertOrUpdateLocale(&head->description, value);
}
