/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2019 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015 (c) LEvertz
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015 (c) Ecosmos
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2016 (c) Lorenz Haas
 *    Copyright 2017 (c) pschoppe
 *    Copyright 2017 (c) Julian Grothoff
 *    Copyright 2017 (c) Henrik Norrman
 */

#include "ua_server_internal.h"
#include "ua_services.h"
#include "ziptree.h"

#define UA_MAX_TREE_RECURSE 50 /* How deep up/down the tree do we recurse at most? */

static UA_UInt32
resultMask2AttributesMask(UA_UInt32 resultMask) {
    UA_UInt32 result = 0;
    if(resultMask & UA_BROWSERESULTMASK_NODECLASS)
        result |= UA_NODEATTRIBUTESMASK_NODECLASS;
    if(resultMask & UA_BROWSERESULTMASK_BROWSENAME)
        result |= UA_NODEATTRIBUTESMASK_BROWSENAME;
    if(resultMask & UA_BROWSERESULTMASK_DISPLAYNAME)
        result |= UA_NODEATTRIBUTESMASK_DISPLAYNAME;
    return result;
}

UA_StatusCode
referenceTypeIndices(UA_Server *server, const UA_NodeId *refType,
                     UA_ReferenceTypeSet *indices, UA_Boolean includeSubtypes) {
    if(UA_NodeId_isNull(refType)) {
        *indices = UA_REFERENCETYPESET_ALL;
        return UA_STATUSCODE_GOOD;
    }

    UA_ReferenceTypeSet_init(indices);

    /* Get the node with only the NodeClass attribute. If it is a
     * ReferenceTypeNode, then the indices are always included, as this is an
     * open62541 specific field (not selectable via the attribute id). */
    const UA_Node *refNode =
        UA_NODESTORE_GET_SELECTIVE(server, refType,
                                   UA_NODEATTRIBUTESMASK_NODECLASS,
                                   UA_REFERENCETYPESET_NONE,
                                   UA_BROWSEDIRECTION_INVALID);

    if(!refNode)
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;

    if(refNode->head.nodeClass != UA_NODECLASS_REFERENCETYPE) {
        UA_NODESTORE_RELEASE(server, refNode);
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }

    if(!includeSubtypes)
        *indices = UA_REFTYPESET(refNode->referenceTypeNode.referenceTypeIndex);
    else
        *indices = refNode->referenceTypeNode.subTypes;

    UA_NODESTORE_RELEASE(server, refNode);
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
matchClassMask(const UA_Node *node, UA_UInt32 nodeClassMask) {
    if(nodeClassMask != UA_NODECLASS_UNSPECIFIED &&
       (node->head.nodeClass & nodeClassMask) == 0)
        return false;
    return true;
}

/****************/
/* IsNodeInTree */
/****************/

/* Internal method to check if a node is already upwards from a leaf node */

static enum ZIP_CMP
cmpRefTarget(const void *a, const void *b) {
    const UA_ReferenceTarget *aa = (const UA_ReferenceTarget*)a;
    const UA_ReferenceTarget *bb = (const UA_ReferenceTarget*)b;
    return (enum ZIP_CMP)UA_NodePointer_order(aa->targetId, bb->targetId);
}

typedef ZIP_HEAD(UA_ParentRefsTree, UA_ReferenceTargetTreeElem) UA_ParentRefsTree;
ZIP_FUNCTIONS(UA_ParentRefsTree, UA_ReferenceTargetTreeElem, idTreeEntry,
              UA_NodePointer, target, cmpRefTarget)

struct IsNodeInTreeContext {
    UA_Server *server;
    UA_NodePointer nodeToFind;
    UA_ParentRefsTree parents;
    UA_ReferenceTypeSet relevantRefs;
    UA_UInt16 depth;
};

static void *
isNodeInTreeIterateCallback(void *context, UA_ReferenceTarget *t) {
    struct IsNodeInTreeContext *tc =
        (struct IsNodeInTreeContext*)context;

    /* Don't follow remote targets */
    if(!UA_NodePointer_isLocal(t->targetId))
        return NULL;

    /* Found the node? -> return non-NULL */
    if(UA_NodePointer_equal(tc->nodeToFind, t->targetId))
        return (void*)0x01;

    /* Prevent endless loop */
    if(ZIP_FIND(UA_ParentRefsTree, &tc->parents, &t->targetId))
        return NULL;

    /* Prevent pathological recursion depth */
    if(tc->depth >= UA_MAX_TREE_RECURSE)
        return NULL;

    /* Get the node without attributes (if the NodeStore supports it) and only
     * the relevant references in inverse direction */
    const UA_Node *node =
        UA_NODESTORE_GETFROMREF_SELECTIVE(tc->server, t->targetId,
                                          UA_NODEATTRIBUTESMASK_NONE,
                                          tc->relevantRefs,
                                          UA_BROWSEDIRECTION_INVERSE);
    if(!node)
        return NULL;

    /* Add current NodeId to parents tree */
    UA_ReferenceTargetTreeElem stackElem;
    stackElem.target = *t;
    ZIP_INSERT(UA_ParentRefsTree, &tc->parents, &stackElem);

    /* Recurse into appropriate references starting from this tree */
    tc->depth++;
    void *res = NULL;
    for(size_t i = 0; i < node->head.referencesSize && !res; i++) {
        UA_NodeReferenceKind *rk = &node->head.references[i];
        /* Search upwards in the tree */
        if(!rk->isInverse)
            continue;

        /* Consider only the indicated reference types */
        if(!UA_ReferenceTypeSet_contains(&tc->relevantRefs, rk->referenceTypeIndex))
            continue;

        res = UA_NodeReferenceKind_iterate(rk, isNodeInTreeIterateCallback, tc);
    }
    tc->depth--;

    /* Clean up */
    UA_NODESTORE_RELEASE(tc->server, node);
    ZIP_REMOVE(UA_ParentRefsTree, &tc->parents, &stackElem);
    return res;
}

UA_Boolean
isNodeInTree(UA_Server *server, const UA_NodeId *leafNode,
             const UA_NodeId *nodeToFind,
             const UA_ReferenceTypeSet *relevantRefs) {
    struct IsNodeInTreeContext ctx;
    memset(&ctx, 0, sizeof(struct IsNodeInTreeContext));
    ctx.server = server;
    ctx.nodeToFind = UA_NodePointer_fromNodeId(nodeToFind);
    ctx.relevantRefs = *relevantRefs;
    UA_ReferenceTarget tmpTarget;
    memset(&tmpTarget, 0, sizeof(UA_ReferenceTarget));
    tmpTarget.targetId = UA_NodePointer_fromNodeId(leafNode);
    return (isNodeInTreeIterateCallback(&ctx, &tmpTarget) != NULL);
}

UA_Boolean
isNodeInTree_singleRef(UA_Server *server, const UA_NodeId *leafNode,
                       const UA_NodeId *nodeToFind, const UA_Byte relevantRefTypeIndex) {
    UA_ReferenceTypeSet reftypes = UA_REFTYPESET(relevantRefTypeIndex);
    return isNodeInTree(server, leafNode, nodeToFind, &reftypes);
}

static enum ZIP_CMP
cmpTarget(const void *a, const void *b) {
    const RefEntry *aa = (const RefEntry*)a;
    const RefEntry *bb = (const RefEntry*)b;
    if(aa->targetHash < bb->targetHash)
        return ZIP_CMP_LESS;
    if(aa->targetHash > bb->targetHash)
        return ZIP_CMP_MORE;
    return (enum ZIP_CMP)UA_ExpandedNodeId_order(aa->target, bb->target);
}

ZIP_FUNCTIONS(RefHead, RefEntry, zipfields, RefEntry, zipfields, cmpTarget)

UA_StatusCode
RefTree_init(RefTree *rt) {
    rt->size = 0;
    rt->capacity = 0;
    ZIP_INIT(&rt->head);
    size_t space = (sizeof(UA_ExpandedNodeId) + sizeof(RefEntry)) * UA_REFTREE_INITIAL_SIZE;
    rt->targets = (UA_ExpandedNodeId*)UA_malloc(space);
    if(!rt->targets)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    rt->capacity = UA_REFTREE_INITIAL_SIZE;
    return UA_STATUSCODE_GOOD;
}

void
RefTree_clear(RefTree *rt) {
    for(size_t i = 0; i < rt->size; i++)
        UA_ExpandedNodeId_clear(&rt->targets[i]);
    if(rt->targets)
        UA_free(rt->targets);
}

/* Double the capacity of the reftree */
static UA_StatusCode UA_FUNC_ATTR_WARN_UNUSED_RESULT
RefTree_double(RefTree *rt) {
    size_t capacity = rt->capacity * 2;
    UA_assert(capacity > 0);
    size_t space = (sizeof(UA_ExpandedNodeId) + sizeof(RefEntry)) * capacity;
    UA_ExpandedNodeId *newTargets = (UA_ExpandedNodeId*)UA_realloc(rt->targets, space);
    if(!newTargets)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Move the entries to the new location  */
    RefEntry *reArray = (RefEntry*)
        ((uintptr_t)newTargets + (capacity * sizeof(UA_ExpandedNodeId)));
    RefEntry *oldReArray = (RefEntry*)
        ((uintptr_t)newTargets + (rt->capacity * sizeof(UA_ExpandedNodeId)));
    memmove(reArray, oldReArray, rt->size * sizeof(RefEntry));

    /* Reinsert all entries into the tree. The new pointer location has changed
     * their ziptree rank. */
    rt->head.root = NULL;
    for(size_t i = 0; i < rt->size; i++) {
        reArray[i].target = &newTargets[i];
        ZIP_INSERT(RefHead, &rt->head, &reArray[i]);
    }

    rt->capacity = capacity;
    rt->targets = newTargets;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
RefTree_add(RefTree *rt, UA_NodePointer target, UA_Boolean *duplicate) {
    UA_ExpandedNodeId en = UA_NodePointer_toExpandedNodeId(target);

    /* Is the target already in the tree? */
    RefEntry dummy;
    memset(&dummy, 0, sizeof(RefEntry));
    dummy.target = &en;
    dummy.targetHash = UA_ExpandedNodeId_hash(&en);
    if(ZIP_FIND(RefHead, &rt->head, &dummy)) {
        if(duplicate)
            *duplicate = true;
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode s = UA_STATUSCODE_GOOD;
    if(rt->capacity <= rt->size) {
        s = RefTree_double(rt);
        if(s != UA_STATUSCODE_GOOD)
            return s;
    }
    s = UA_ExpandedNodeId_copy(&en, &rt->targets[rt->size]);
    if(s != UA_STATUSCODE_GOOD)
        return s;
    RefEntry *re = (RefEntry*)((uintptr_t)rt->targets +
                               (sizeof(UA_ExpandedNodeId) * rt->capacity) +
                               (sizeof(RefEntry) * rt->size));
    re->target = &rt->targets[rt->size];
    re->targetHash = dummy.targetHash;
    ZIP_INSERT(RefHead, &rt->head, re);
    rt->size++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
RefTree_addNodeId(RefTree *rt, const UA_NodeId *target,
                  UA_Boolean *duplicate) {
    return RefTree_add(rt, UA_NodePointer_fromNodeId(target), duplicate);
}

UA_Boolean
RefTree_contains(RefTree *rt, const UA_ExpandedNodeId *target) {
    RefEntry dummy;
    dummy.target = target;
    dummy.targetHash = UA_ExpandedNodeId_hash(target);
    return !!ZIP_FIND(RefHead, &rt->head, &dummy);
}

UA_Boolean
RefTree_containsNodeId(RefTree *rt, const UA_NodeId *target) {
    UA_ExpandedNodeId en;
    en.nodeId = *target;
    en.namespaceUri = UA_STRING_NULL;
    en.serverIndex = 0;
    return RefTree_contains(rt, &en);
}

/********************/
/* Browse Recursive */
/********************/

struct BrowseRecursiveContext {
    UA_Server *server;
    RefTree *rt;
    UA_UInt16 depth;
    UA_BrowseDirection browseDirection;
    UA_ReferenceTypeSet refTypes;
    UA_UInt32 nodeClassMask;
    UA_StatusCode status;
    UA_Boolean includeStartNodes;
};

static void *
browseRecursiveCallback(void *context, UA_ReferenceTarget *t) {
    struct BrowseRecursiveContext *brc =
        (struct BrowseRecursiveContext*)context;

    /* Have we reached the max recursion depth? */
    if(brc->depth >= UA_MAX_TREE_RECURSE)
        return NULL;

    /* Is this a non-local reference? If yes include it in the returned set. */
    if(!UA_NodePointer_isLocal(t->targetId)) {
        brc->status = RefTree_add(brc->rt, t->targetId, NULL);
        return (brc->status == UA_STATUSCODE_GOOD) ? NULL : (void*)0x01;
    }

    /* We only look at the NodeClass attribute and a subset of the references.
     * Get a node with only these elements if the NodeStore supports that. */
    const UA_Node *node =
        UA_NODESTORE_GETFROMREF_SELECTIVE(brc->server, t->targetId,
                                          UA_NODEATTRIBUTESMASK_NODECLASS,
                                          brc->refTypes, brc->browseDirection);
    if(!node)
        return NULL;

    /* Add the current node if we don't want to skip it as a start node and it
     * matches the nodeClassMask filter Recurse into the children in any
     * case. */
    const UA_NodeHead *head = &node->head;
    if((brc->includeStartNodes || brc->depth > 0)  &&
       matchClassMask(node, brc->nodeClassMask)) {
        UA_Boolean duplicate = false;
        brc->status = RefTree_addNodeId(brc->rt, &head->nodeId, &duplicate);
        if(duplicate || brc->status != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Recurse */
    brc->depth++;
    void *res = NULL;
    for(size_t i = 0; i < head->referencesSize && !res; i++) {
        UA_NodeReferenceKind *rk = &head->references[i];

        /* Reference in the right direction? */
        if(rk->isInverse && brc->browseDirection == UA_BROWSEDIRECTION_FORWARD)
            continue;
        if(!rk->isInverse && brc->browseDirection == UA_BROWSEDIRECTION_INVERSE)
            continue;

        /* Is the reference part of the hierarchy of references we look for? */
        if(!UA_ReferenceTypeSet_contains(&brc->refTypes, rk->referenceTypeIndex))
            continue;

        res = UA_NodeReferenceKind_iterate(rk, browseRecursiveCallback, brc);
    }
    brc->depth--;

 cleanup:
    UA_NODESTORE_RELEASE(brc->server, node);
    return (brc->status == UA_STATUSCODE_GOOD) ? NULL : (void*)0x01;
}

UA_StatusCode
browseRecursive(UA_Server *server, size_t startNodesSize, const UA_NodeId *startNodes,
                UA_BrowseDirection browseDirection, const UA_ReferenceTypeSet *refTypes,
                UA_UInt32 nodeClassMask, UA_Boolean includeStartNodes,
                size_t *resultsSize, UA_ExpandedNodeId **results) {
    RefTree rt;
    UA_StatusCode retval = RefTree_init(&rt);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    struct BrowseRecursiveContext brc;
    brc.server = server;
    brc.rt = &rt;
    brc.depth = 0;
    brc.refTypes = *refTypes;
    brc.nodeClassMask = nodeClassMask;
    brc.status = UA_STATUSCODE_GOOD;
    brc.includeStartNodes = includeStartNodes;

    for(size_t i = 0; i < startNodesSize && brc.status == UA_STATUSCODE_GOOD; i++) {
        UA_ReferenceTarget target;
        target.targetId = UA_NodePointer_fromNodeId(&startNodes[i]);

        /* Call the inner recursive browse separately for the search direction.
         * Otherwise we might take one step up and another step down in the
         * search tree. */
        if(browseDirection == UA_BROWSEDIRECTION_FORWARD ||
           browseDirection == UA_BROWSEDIRECTION_BOTH) {
            brc.browseDirection = UA_BROWSEDIRECTION_FORWARD;
            browseRecursiveCallback(&brc, &target);
        }

        if(browseDirection == UA_BROWSEDIRECTION_INVERSE ||
           browseDirection == UA_BROWSEDIRECTION_BOTH) {
            brc.browseDirection = UA_BROWSEDIRECTION_INVERSE;
            browseRecursiveCallback(&brc, &target);
        }
    }

    if(rt.size > 0 && brc.status == UA_STATUSCODE_GOOD) {
        *results = rt.targets;
        *resultsSize = rt.size;
    } else {
        RefTree_clear(&rt);
    }
    return brc.status;
}

UA_StatusCode
UA_Server_browseRecursive(UA_Server *server, const UA_BrowseDescription *bd,
                          size_t *resultsSize, UA_ExpandedNodeId **results) {
    UA_LOCK(&server->serviceMutex);

    /* Set the list of relevant reference types */
    UA_ReferenceTypeSet refTypes;
    UA_StatusCode retval = referenceTypeIndices(server, &bd->referenceTypeId,
                                                &refTypes, bd->includeSubtypes);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&server->serviceMutex);
        return retval;
    }

    /* Browse */
    retval = browseRecursive(server, 1, &bd->nodeId, bd->browseDirection,
                             &refTypes, bd->nodeClassMask, false, resultsSize, results);

    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

/**********/
/* Browse */
/**********/

typedef struct {
    size_t size;
    size_t capacity;
    UA_ReferenceDescription *descr;
} RefResult;

static UA_StatusCode UA_FUNC_ATTR_WARN_UNUSED_RESULT
RefResult_init(RefResult *rr) {
    memset(rr, 0, sizeof(RefResult));
    rr->descr = (UA_ReferenceDescription*)
        UA_Array_new(UA_REFTREE_INITIAL_SIZE, &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION]);
    if(!rr->descr)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    rr->capacity = UA_REFTREE_INITIAL_SIZE;
    rr->size = 0;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode UA_FUNC_ATTR_WARN_UNUSED_RESULT
RefResult_double(RefResult *rr) {
    size_t newSize = rr->capacity * 2;
    UA_ReferenceDescription *rd = (UA_ReferenceDescription*)
        UA_realloc(rr->descr, newSize * sizeof(UA_ReferenceDescription));
    if(!rd)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(&rd[rr->size], 0, sizeof(UA_ReferenceDescription) * (newSize - rr->size));
    rr->descr = rd;
    rr->capacity = newSize;
    return UA_STATUSCODE_GOOD;
}

static void
RefResult_clear(RefResult *rr) {
    UA_assert(rr->descr != NULL);
    for(size_t i = 0; i < rr->size; i++)
        UA_ReferenceDescription_clear(&rr->descr[i]);
    UA_free(rr->descr);
}

struct ContinuationPoint {
    ContinuationPoint *next;
    UA_ByteString identifier;

    /* Parameters of the Browse Request */
    UA_BrowseDescription browseDescription;
    UA_UInt32 maxReferences;
    UA_ReferenceTypeSet relevantReferences;

    /* The last reference target that was transmitted to the client. The results
     * list may be incomplete if the last target is removed or references added
     * between the calls to Browse/BrowseNext. */
    UA_NodePointer lastTarget;
    UA_Byte lastRefKindIndex;
    UA_Boolean lastRefInverse;
};

ContinuationPoint *
ContinuationPoint_clear(ContinuationPoint *cp) {
    UA_ByteString_clear(&cp->identifier);
    UA_BrowseDescription_clear(&cp->browseDescription);
    UA_NodePointer_clear(&cp->lastTarget);
    return cp->next;
}

struct BrowseContext {
    /* Context */
    ContinuationPoint *cp;
    UA_Server *server;
    UA_Session *session;
    UA_NodeReferenceKind *rk;
    UA_ReferenceTypeSet resultRefs; /* With additional references for type
                                     * lookups */
    UA_Boolean activeCP; /* true during "forwarding" to the position of the last
                          * reference target */

    /* Results */
    RefResult rr;
    UA_StatusCode status;
    UA_Boolean done;
};

/* Target node on top of the stack */
static UA_StatusCode
addReferenceDescription(struct BrowseContext *bc, UA_NodePointer nodeP,
                        const UA_Node *curr) {
    UA_assert(curr);
    UA_BrowseDescription *bd = &bc->cp->browseDescription;

    /* Ensure capacity is left */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(bc->rr.size >= bc->rr.capacity) {
        res = RefResult_double(&bc->rr);
        if(res != UA_STATUSCODE_GOOD)
           return res;
    }

    UA_ReferenceDescription *descr = &bc->rr.descr[bc->rr.size];

    /* Fields without access to the actual node */
    UA_ExpandedNodeId en = UA_NodePointer_toExpandedNodeId(nodeP);
    res = UA_ExpandedNodeId_copy(&en, &descr->nodeId);
    if(bd->resultMask & UA_BROWSERESULTMASK_REFERENCETYPEID) {
        const UA_NodeId *refTypeId =
            UA_NODESTORE_GETREFERENCETYPEID(bc->server, bc->rk->referenceTypeIndex);
        res |= UA_NodeId_copy(refTypeId, &descr->referenceTypeId);
    }
    if(bd->resultMask & UA_BROWSERESULTMASK_ISFORWARD)
        descr->isForward = !bc->rk->isInverse;

    /* Create fields that require access to the actual node */
    if(bd->resultMask & UA_BROWSERESULTMASK_NODECLASS)
        descr->nodeClass = curr->head.nodeClass;

    if(bd->resultMask & UA_BROWSERESULTMASK_BROWSENAME)
        res |= UA_QualifiedName_copy(&curr->head.browseName,
                                     &descr->browseName);

    if(bd->resultMask & UA_BROWSERESULTMASK_DISPLAYNAME) {
        UA_LocalizedText displayname =
            UA_Session_getNodeDisplayName(bc->session, &curr->head);
        res |= UA_LocalizedText_copy(&displayname, &descr->displayName);
    }

    if(bd->resultMask & UA_BROWSERESULTMASK_TYPEDEFINITION) {
        if(curr->head.nodeClass == UA_NODECLASS_OBJECT ||
           curr->head.nodeClass == UA_NODECLASS_VARIABLE) {
            const UA_Node *type = getNodeType(bc->server, &curr->head);
            if(type) {
                res |= UA_NodeId_copy(&type->head.nodeId,
                                      &descr->typeDefinition.nodeId);
                UA_NODESTORE_RELEASE(bc->server, type);
            }
        }
    }

    /* Clean up and return */
    if(res != UA_STATUSCODE_GOOD) {
        UA_ReferenceDescription_clear(descr);
        return res;
    }
    bc->rr.size++;
    return UA_STATUSCODE_GOOD;
}

static void *
browseReferencTargetCallback(void *context, UA_ReferenceTarget *t) {
    struct BrowseContext *bc = (struct BrowseContext*)context;
    const UA_BrowseDescription *bd = &bc->cp->browseDescription;
    ContinuationPoint *cp = bc->cp;

    /* Remote references are ignored */
    if(!UA_NodePointer_isLocal(t->targetId))
        return NULL;
    
    /* Get the node. Include only the ReferenceTypes we are interested in,
     * including those for figuring out the TypeDefinition (if that was
     * requested). */
    const UA_Node *target =
        UA_NODESTORE_GETFROMREF_SELECTIVE(bc->server, t->targetId,
                                          resultMask2AttributesMask(bd->resultMask),
                                          bc->resultRefs, bd->browseDirection);
    if(!target)
        return NULL;
    
    /* The node class has to match */
    if(!matchClassMask(target, bd->nodeClassMask)) {
        UA_NODESTORE_RELEASE(bc->server, target);
        return NULL;
    }
    
    /* Reached maxrefs. Return the "abort" signal. */
    if(bc->rr.size >= cp->maxReferences) {
        UA_NODESTORE_RELEASE(bc->server, target);
        return (void*)0x01;
    }

    /* Create the reference description */
    bc->status = addReferenceDescription(bc, t->targetId, target);

    /* Release the node */
    UA_NODESTORE_RELEASE(bc->server, target);

    /* Store as last target. The itarget-id is a shallow copy for now. */
    cp->lastTarget = t->targetId;
    cp->lastRefKindIndex = bc->rk->referenceTypeIndex;
    cp->lastRefInverse = bc->rk->isInverse;

    /* Abort if the status is not good. Also doesn't make a deep-copy of
     * cp->lastTarget after returning from here. */
    if(bc->status != UA_STATUSCODE_GOOD) {
        UA_NodePointer_init(&cp->lastTarget);
        return (void*)0x01;
    }
    return NULL;
}

/* Returns whether the node / continuationpoint is done */
static void
browseWithNode(struct BrowseContext *bc, const UA_NodeHead *head ) {
    ContinuationPoint *cp = bc->cp;
    const UA_BrowseDescription *bd = &cp->browseDescription;

    /* Loop over the ReferenceKinds */
    for(size_t i = 0; i < head->referencesSize && bc->status == UA_STATUSCODE_GOOD; ++i) {
        UA_NodeReferenceKind *rk = &head->references[i];

        /* If the continuation point was previously used, skip forward to the
         * last ReferenceType that was transmitted */
        if(bc->activeCP && rk->referenceTypeIndex != cp->lastRefKindIndex)
            continue;
        if(bc->activeCP && rk->isInverse != cp->lastRefInverse)
            continue;

        /* Reference in the right direction? */
        if(rk->isInverse && bd->browseDirection == UA_BROWSEDIRECTION_FORWARD)
            continue;
        if(!rk->isInverse && bd->browseDirection == UA_BROWSEDIRECTION_INVERSE)
            continue;

        /* Is the reference part of the hierarchy of references we look for? */
        if(!UA_ReferenceTypeSet_contains(&cp->relevantReferences, rk->referenceTypeIndex))
            continue;

        /* We have a matching ReferenceKind */

        /* Skip ahead to the target where the last continuation point stopped.
         * This temporarily modifies rk. */
        UA_ReferenceIdTree left = {NULL}, right = {NULL};
        size_t nextTargetIndex = 0;
        if(bc->activeCP) {
            if(rk->hasRefTree) {
                /* Unzip the tree until the continuation point. All NodeIds
                 * larger than the last target are guaranteed to sit on the
                 * right-hand side. */
                UA_ExpandedNodeId lastEn =
                    UA_NodePointer_toExpandedNodeId(cp->lastTarget);
                UA_ReferenceTargetTreeElem key;
                key.target.targetId = cp->lastTarget;
                key.targetIdHash = UA_ExpandedNodeId_hash(&lastEn);
                ZIP_UNZIP(UA_ReferenceIdTree,
                          (UA_ReferenceIdTree*)&rk->targets.tree.idRoot,
                          &key, &left, &right);
                rk->targets.tree.idRoot = right.root;
            } else {
                /* Iterate over the array to find the match */
                for(; nextTargetIndex < rk->targetsSize; nextTargetIndex++) {
                    UA_ReferenceTarget *t = &rk->targets.array[nextTargetIndex];
                    if(UA_NodePointer_equal(cp->lastTarget, t->targetId))
                        break;
                }
                if(nextTargetIndex == rk->targetsSize) {
                    /* Not found - assume that this reference kind is done */
                    bc->activeCP = false;
                    continue;
                }
                nextTargetIndex++; /* From the last index to the next index */
                rk->targets.array = &rk->targets.array[nextTargetIndex];
                rk->targetsSize -= nextTargetIndex;
            }

            /* Clear cp->lastTarget before it gets overwritten in the following
             * browse steps. */
            UA_NodePointer_clear(&cp->lastTarget);
        }

        /* Iterate over all reference targets */
        bc->rk = rk;
        void *res = UA_NodeReferenceKind_iterate(rk, browseReferencTargetCallback, bc);

        /* Undo the "skipping ahead" for the continuation point */
        if(bc->activeCP) {
            if(rk->hasRefTree) {
                rk->targets.tree.idRoot =
                    ZIP_ZIP(UA_ReferenceIdTree, left.root, right.root);
            } else {
                /* rk->targets.array = rk->targets.array[-nextTargetIndex]; */
                rk->targets.array = rk->targets.array - nextTargetIndex;
                rk->targetsSize += nextTargetIndex;
                UA_assert(rk->targetsSize > 0);
            }
            bc->activeCP = false;
        }

        /* The iteration was aborted */
        if(res != NULL) {
            /* Aborted with status code good -> the maximum number of browse
             * results was reached. Make a deep copy of the last target for the
             * continuation point. */
            if(bc->status == UA_STATUSCODE_GOOD)
                bc->status = UA_NodePointer_copy(cp->lastTarget, &cp->lastTarget);
            return;
        }
    }

    /* Reset last-target to prevent clearing it up */
    UA_NodePointer_init(&cp->lastTarget);

    /* Browsing the node is done */
    bc->done = true;
}

/* Results for a single browsedescription. This is the inner loop for both
 * Browse and BrowseNext. The ContinuationPoint contains all the data used.
 * Including the BrowseDescription. Returns whether there are remaining
 * references. */
static void
browse(struct BrowseContext *bc) {
    /* Is the browsedirection valid? */
    struct ContinuationPoint *cp = bc->cp;
    const UA_BrowseDescription *descr = &cp->browseDescription;
    if(descr->browseDirection != UA_BROWSEDIRECTION_BOTH &&
       descr->browseDirection != UA_BROWSEDIRECTION_FORWARD &&
       descr->browseDirection != UA_BROWSEDIRECTION_INVERSE) {
        bc->status = UA_STATUSCODE_BADBROWSEDIRECTIONINVALID;
        return;
    }

    /* Get node with only the selected references and attributes */
    const UA_Node *node =
        UA_NODESTORE_GET_SELECTIVE(bc->server, &descr->nodeId,
                                   resultMask2AttributesMask(descr->resultMask),
                                   bc->resultRefs, descr->browseDirection);
    if(!node) {
        bc->status = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    /* Check AccessControl rights */
    if(bc->session != &bc->server->adminSession) {
        UA_LOCK_ASSERT(&bc->server->serviceMutex, 1);
        UA_UNLOCK(&bc->server->serviceMutex);
        if(!bc->server->config.accessControl.
           allowBrowseNode(bc->server, &bc->server->config.accessControl,
                           &bc->session->sessionId, bc->session->context,
                           &descr->nodeId, node->head.context)) {
            UA_LOCK(&bc->server->serviceMutex);
            UA_NODESTORE_RELEASE(bc->server, node);
            bc->status = UA_STATUSCODE_BADUSERACCESSDENIED;
            return;
        }
        UA_LOCK(&bc->server->serviceMutex);
    }

    /* Browse the node */
    browseWithNode(bc, &node->head);
    UA_NODESTORE_RELEASE(bc->server, node);

    /* Is the reference type valid? This is very infrequent. So we only test
     * this if browsing came up empty. If the node has references of that type,
     * we know the reftype to be good. */
    if(bc->rr.size == 0 && !UA_NodeId_isNull(&descr->referenceTypeId)) {
        const UA_Node *reftype =
            UA_NODESTORE_GET_SELECTIVE(bc->server, &descr->referenceTypeId,
                                       UA_NODEATTRIBUTESMASK_NODECLASS,
                                       UA_REFERENCETYPESET_NONE,
                                       UA_BROWSEDIRECTION_INVALID);
        if(!reftype) {
            bc->status = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
            return;
        }

        UA_Boolean isRef = (reftype->head.nodeClass == UA_NODECLASS_REFERENCETYPE);
        UA_NODESTORE_RELEASE(bc->server, reftype);

        if(!isRef) {
            bc->status = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
            return;
        }
    }
}

/* Start to browse with no previous cp */
void
Operation_Browse(UA_Server *server, UA_Session *session, const UA_UInt32 *maxrefs,
                 const UA_BrowseDescription *descr, UA_BrowseResult *result) {
    /* Stack-allocate a temporary cp */
    ContinuationPoint cp;
    memset(&cp, 0, sizeof(ContinuationPoint));
    cp.maxReferences = *maxrefs;
    cp.browseDescription = *descr; /* Shallow copy. Deep-copy later if we persist the cp. */

    /* How many references can we return at most? */
    if(cp.maxReferences == 0) {
        if(server->config.maxReferencesPerNode != 0) {
            cp.maxReferences = server->config.maxReferencesPerNode;
        } else {
            cp.maxReferences = UA_INT32_MAX;
        }
    } else {
        if(server->config.maxReferencesPerNode != 0 &&
           cp.maxReferences > server->config.maxReferencesPerNode) {
            cp.maxReferences= server->config.maxReferencesPerNode;
        }
    }

    /* Get the list of relevant reference types */
    result->statusCode =
        referenceTypeIndices(server, &descr->referenceTypeId,
                             &cp.relevantReferences, descr->includeSubtypes);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Prepare the context */
    struct BrowseContext bc;
    bc.cp = &cp;
    bc.server = server;
    bc.session = session;
    bc.status = UA_STATUSCODE_GOOD;
    bc.done = false;
    bc.activeCP = false;
    bc.resultRefs = cp.relevantReferences;
    if(cp.browseDescription.resultMask & UA_BROWSERESULTMASK_TYPEDEFINITION) {
        /* Get the node with additional reference types if we need to lookup the
         * TypeDefinition */
        bc.resultRefs = UA_ReferenceTypeSet_union(bc.resultRefs,
              UA_ReferenceTypeSet_union(UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASTYPEDEFINITION),
                                        UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE)));
    }
    result->statusCode = RefResult_init(&bc.rr);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Perform the browse */
    browse(&bc);

    if(bc.status != UA_STATUSCODE_GOOD || bc.rr.size == 0) {
        /* No relevant references, return array of length zero */
        RefResult_clear(&bc.rr);
        result->references = (UA_ReferenceDescription*)UA_EMPTY_ARRAY_SENTINEL;
        result->statusCode = bc.status;
        return;
    }

    /* Move results */
    result->references = bc.rr.descr;
    result->referencesSize = bc.rr.size;

    /* Exit early if done */
    if(bc.done)
        return;

    /* Persist the continuation point */

    ContinuationPoint *cp2 = NULL;
    UA_Guid *ident = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Enough space for the continuation point? */
    if(session->availableContinuationPoints == 0) {
        retval = UA_STATUSCODE_BADNOCONTINUATIONPOINTS;
        goto cleanup;
    }

    /* Allocate and fill the data structure */
    cp2 = (ContinuationPoint*)UA_calloc(1, sizeof(ContinuationPoint));
    if(!cp2) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    /* The BrowseDescription is only a shallow copy so far */
    retval = UA_BrowseDescription_copy(descr, &cp2->browseDescription);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    cp2->maxReferences = cp.maxReferences;
    cp2->relevantReferences = cp.relevantReferences;
    cp2->lastTarget = cp.lastTarget; /* Move the (deep) copy */
    UA_NodePointer_init(&cp.lastTarget); /* No longer clear below (cleanup) */
    cp2->lastRefKindIndex = cp.lastRefKindIndex;
    cp2->lastRefInverse = cp.lastRefInverse;

    /* Create a random bytestring via a Guid */
    ident = UA_Guid_new();
    if(!ident) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    *ident = UA_Guid_random();
    cp2->identifier.data = (UA_Byte*)ident;
    cp2->identifier.length = sizeof(UA_Guid);

    /* Return the cp identifier */
    retval = UA_ByteString_copy(&cp2->identifier, &result->continuationPoint);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Attach the cp to the session */
    cp2->next = session->continuationPoints;
    session->continuationPoints = cp2;
    --session->availableContinuationPoints;
    return;

 cleanup:
    if(cp2) {
        ContinuationPoint_clear(cp2);
        UA_free(cp2);
    }
    UA_NodePointer_clear(&cp.lastTarget);
    UA_BrowseResult_clear(result);
    result->statusCode = retval;
}

void Service_Browse(UA_Server *server, UA_Session *session,
                    const UA_BrowseRequest *request, UA_BrowseResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session, "Processing BrowseRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Test the number of operations in the request */
    if(server->config.maxNodesPerBrowse != 0 &&
       request->nodesToBrowseSize > server->config.maxNodesPerBrowse) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    /* No views supported at the moment */
    if(!UA_NodeId_isNull(&request->view.viewId)) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADVIEWIDUNKNOWN;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation)Operation_Browse,
                                           &request->requestedMaxReferencesPerNode,
                                           &request->nodesToBrowseSize,
                                           &UA_TYPES[UA_TYPES_BROWSEDESCRIPTION],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_BROWSERESULT]);
}

UA_BrowseResult
UA_Server_browse(UA_Server *server, UA_UInt32 maxReferences,
                 const UA_BrowseDescription *bd) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    UA_LOCK(&server->serviceMutex);
    Operation_Browse(server, &server->adminSession, &maxReferences, bd, &result);
    UA_UNLOCK(&server->serviceMutex);
    return result;
}

static void
Operation_BrowseNext(UA_Server *server, UA_Session *session,
                     const UA_Boolean *releaseContinuationPoints,
                     const UA_ByteString *continuationPoint, UA_BrowseResult *result) {
    /* Find the continuation point */
    ContinuationPoint **prev = &session->continuationPoints;
    ContinuationPoint *cp;
    while((cp = *prev)) {
        if(UA_ByteString_equal(&cp->identifier, continuationPoint))
            break;
        prev = &cp->next;
    }
    if(!cp) {
        result->statusCode = UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
        return;
    }

    /* Remove the cp */
    if(*releaseContinuationPoints) {
        *prev = ContinuationPoint_clear(cp);
        UA_free(cp);
        ++session->availableContinuationPoints;
        return;
    }

    /* Prepare the context */
    struct BrowseContext bc;
    bc.cp = cp;
    bc.server = server;
    bc.session = session;
    bc.status = UA_STATUSCODE_GOOD;
    bc.done = false;
    bc.activeCP = true;
    bc.resultRefs = cp->relevantReferences;
    if(cp->browseDescription.resultMask & UA_BROWSERESULTMASK_TYPEDEFINITION) {
        /* Get the node with additional reference types if we need to lookup the
         * TypeDefinition */
        bc.resultRefs = UA_ReferenceTypeSet_union(bc.resultRefs,
              UA_ReferenceTypeSet_union(UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASTYPEDEFINITION),
                                        UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE)));
    }
    result->statusCode = RefResult_init(&bc.rr);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Continue browsing */
    browse(&bc);

    if(bc.status != UA_STATUSCODE_GOOD || bc.rr.size == 0) {
        /* No relevant references, return array of length zero */
        RefResult_clear(&bc.rr);
        result->references = (UA_ReferenceDescription*)UA_EMPTY_ARRAY_SENTINEL;
        result->statusCode = bc.status;
        goto remove_cp;
    }

    /* Move results */
    result->references = bc.rr.descr;
    result->referencesSize = bc.rr.size;

    if(bc.done)
        goto remove_cp;

    /* Return the cp identifier to signal that there are references left */
    bc.status = UA_ByteString_copy(&cp->identifier, &result->continuationPoint);
    if(bc.status != UA_STATUSCODE_GOOD) {
        UA_BrowseResult_clear(result);
        result->statusCode = bc.status;
    }
    return;

 remove_cp:
    /* Remove the cp */
    *prev = ContinuationPoint_clear(cp);
    UA_free(cp);
    ++session->availableContinuationPoints;
}

void
Service_BrowseNext(UA_Server *server, UA_Session *session,
                   const UA_BrowseNextRequest *request,
                   UA_BrowseNextResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing BrowseNextRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_Boolean releaseContinuationPoints =
        request->releaseContinuationPoints; /* request is const */
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation)Operation_BrowseNext,
                                           &releaseContinuationPoints,
                                           &request->continuationPointsSize,
                                           &UA_TYPES[UA_TYPES_BYTESTRING],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_BROWSERESULT]);
}

UA_BrowseResult
UA_Server_browseNext(UA_Server *server, UA_Boolean releaseContinuationPoint,
                     const UA_ByteString *continuationPoint) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    UA_LOCK(&server->serviceMutex);
    Operation_BrowseNext(server, &server->adminSession, &releaseContinuationPoint,
                         continuationPoint, &result);
    UA_UNLOCK(&server->serviceMutex);
    return result;
}

/***********************/
/* TranslateBrowsePath */
/***********************/

/* Add all entries for the hash. There are possible duplicates due to hash
 * collisions. The full browsename is checked afterwards. */
static void *
addBrowseHashTarget(void *context, UA_ReferenceTargetTreeElem *elem) {
    RefTree *next = (RefTree*)context;
    return (void*)(uintptr_t)RefTree_add(next, elem->target.targetId, NULL);
}

static UA_StatusCode
walkBrowsePathElement(UA_Server *server, UA_Session *session,
                      const UA_RelativePath *path, const size_t pathIndex,
                      UA_UInt32 nodeClassMask, const UA_QualifiedName *lastBrowseName,
                      UA_BrowsePathResult *result, RefTree *current, RefTree *next) {
    /* For the next level. Note the difference from lastBrowseName */
    const UA_RelativePathElement *elem = &path->elements[pathIndex];
    UA_UInt32 browseNameHash = UA_QualifiedName_hash(&elem->targetName);

    /* Get the relevant ReferenceTypes */
    UA_ReferenceTypeSet refTypes;
    UA_StatusCode res =
        referenceTypeIndices(server, &elem->referenceTypeId,
                             &refTypes, elem->includeSubtypes);
    if(res != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADNOMATCH;

    /* Loop over all Nodes in the current depth level */
    for(size_t i = 0; i < current->size; i++) {
        /* Remote Node. Immediately add to the results with the
         * RemainingPathIndex set. */
        if(!UA_ExpandedNodeId_isLocal(&current->targets[i])) {
            /* Increase the size of the results array */
            UA_BrowsePathTarget *tmpResults = (UA_BrowsePathTarget*)
                UA_realloc(result->targets, sizeof(UA_BrowsePathTarget) *
                           (result->targetsSize + 1));
            if(!tmpResults)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            result->targets = tmpResults;

            /* Copy over the result */
            res = UA_ExpandedNodeId_copy(&current->targets[i],
                                         &result->targets[result->targetsSize].targetId);
            result->targets[result->targetsSize].remainingPathIndex = (UA_UInt32)pathIndex;
            result->targetsSize++;
            if(res != UA_STATUSCODE_GOOD)
                break;
            continue;
        }

        /* Local Node. Add to the tree of results at the next depth. Get only
         * the NodeClass + BrowseName attribute and the selected ReferenceTypes
         * if the nodestore supports that. */
        const UA_Node *node =
            UA_NODESTORE_GET_SELECTIVE(server, &current->targets[i].nodeId,
                                       UA_NODEATTRIBUTESMASK_NODECLASS |
                                       UA_NODEATTRIBUTESMASK_BROWSENAME,
                                       refTypes,
                                       elem->isInverse ? UA_BROWSEDIRECTION_INVERSE :
                                                         UA_BROWSEDIRECTION_FORWARD);
        if(!node)
            continue;

        /* Test whether the node fits the class mask */
        UA_Boolean skip = !matchClassMask(node, nodeClassMask);

        /* Does the BrowseName match for the current node (not the references
         * going out here) */
        skip |= (lastBrowseName &&
                 !UA_QualifiedName_equal(lastBrowseName, &node->head.browseName));

        if(skip) {
            UA_NODESTORE_RELEASE(server, node);
            continue;
        }

        /* Loop over the ReferenceKinds */
        UA_ReferenceTarget targetHashKey;
        targetHashKey.targetNameHash = browseNameHash;
        for(size_t j = 0; j < node->head.referencesSize; j++) {
            UA_NodeReferenceKind *rk = &node->head.references[j];

            /* Does the direction of the reference match? */
            if(rk->isInverse != elem->isInverse)
                continue;

            /* Does the reference type match? */
            if(!UA_ReferenceTypeSet_contains(&refTypes, rk->referenceTypeIndex))
                continue;

            /* Retrieve by BrowseName hash. We might have several nodes where
             * the hash matches. The exact BrowseName will be verified in the
             * next iteration of the outer loop. So we only have to retrieve
             * every node just once. */

            if(rk->hasRefTree) {
                res = (UA_StatusCode)(uintptr_t)
                    ZIP_ITER_KEY(UA_ReferenceNameTree,
                                 (UA_ReferenceNameTree*)&rk->targets.tree.nameRoot,
                                 &targetHashKey, addBrowseHashTarget, next);
                if(res != UA_STATUSCODE_GOOD)
                    break;
            } else {
                /* The array entries don't have a BrowseName hash. Add all of
                 * them at this level to be checked with a full string
                 * comparison. */
                for(size_t k = 0; k < rk->targetsSize; k++) {
                    if(rk->targets.array[k].targetNameHash != browseNameHash)
                        continue;
                    res = RefTree_add(next, rk->targets.array[k].targetId, NULL);
                    if(res != UA_STATUSCODE_GOOD)
                        break;
                }
                if(res != UA_STATUSCODE_GOOD)
                    break;
            }
        }

        UA_NODESTORE_RELEASE(server, node);
    }
    return res;
}

static void
Operation_TranslateBrowsePathToNodeIds(UA_Server *server, UA_Session *session,
                                       const UA_UInt32 *nodeClassMask,
                                       const UA_BrowsePath *path,
                                       UA_BrowsePathResult *result) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(path->relativePath.elementsSize == 0) {
        result->statusCode = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    /* RelativePath elements must not have an empty targetName */
    for(size_t i = 0; i < path->relativePath.elementsSize; ++i) {
        if(UA_QualifiedName_isNull(&path->relativePath.elements[i].targetName)) {
            result->statusCode = UA_STATUSCODE_BADBROWSENAMEINVALID;
            return;
        }
    }

    /* Check if the starting node exists */
    const UA_Node *startingNode =
        UA_NODESTORE_GET_SELECTIVE(server, &path->startingNode,
                                   UA_NODEATTRIBUTESMASK_NONE,
                                   UA_REFERENCETYPESET_NONE,
                                   UA_BROWSEDIRECTION_INVALID);
    if(!startingNode) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }
    UA_NODESTORE_RELEASE(server, startingNode);

    /* Create two RefTrees that are alternated between path elements */
    RefTree rt1;
    RefTree rt2;
    RefTree *current = &rt1;
    RefTree *next = &rt2;
    RefTree *tmp;
    result->statusCode |= RefTree_init(&rt1);
    result->statusCode |= RefTree_init(&rt2);
    UA_BrowsePathTarget *tmpResults = NULL;
    UA_QualifiedName *browseNameFilter = NULL;
    if(result->statusCode != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Copy the starting node into next */
    result->statusCode = RefTree_addNodeId(next, &path->startingNode, NULL);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Walk the path elements. Retrieve the nodes only once from the NodeStore.
     * Hence the BrowseName is checked with one element "delay". */
    for(size_t i = 0; i < path->relativePath.elementsSize; i++) {
        /* Switch the trees */
        tmp = current;
        current = next;
        next = tmp;

        /* Clear up current, keep the capacity */
        for(size_t j = 0; j < next->size; j++)
            UA_ExpandedNodeId_clear(&next->targets[j]);
        next->size = 0;
        ZIP_INIT(&next->head);

        /* Do this check after next->size has been set to zero */
        if(current->size == 0)
            break;

        /* Walk element for all NodeIds in the "current" tree.
         * Puts new results in the "next" tree. */
        result->statusCode =
            walkBrowsePathElement(server, session, &path->relativePath, i,
                                  *nodeClassMask, browseNameFilter, result, current, next);
        if(result->statusCode != UA_STATUSCODE_GOOD)
            goto cleanup;

        browseNameFilter = &path->relativePath.elements[i].targetName;
    }

    /* Allocate space for the results array */
    tmpResults = (UA_BrowsePathTarget*)
        UA_realloc(result->targets, sizeof(UA_BrowsePathTarget) *
                   (result->targetsSize + next->size));
    if(!tmpResults && next->size > 0) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    result->targets = tmpResults;

    for(size_t k = 0; k < next->size; k++) {
        /* Check the BrowseName. It has been filtered only via its hash so far.
         * Get only the BrowseName attribute if the nodestore supports that. */
        const UA_Node *node =
            UA_NODESTORE_GET_SELECTIVE(server, &next->targets[k].nodeId,
                                       UA_NODEATTRIBUTESMASK_BROWSENAME,
                                       UA_REFERENCETYPESET_NONE,
                                       UA_BROWSEDIRECTION_INVALID);
        if(!node)
            continue;
        UA_Boolean match = UA_QualifiedName_equal(browseNameFilter, &node->head.browseName);
        UA_NODESTORE_RELEASE(server, node);
        if(!match)
            continue;

        /* Move to the target to the results array */
        result->targets[result->targetsSize].targetId = next->targets[k];
        result->targets[result->targetsSize].remainingPathIndex = UA_UINT32_MAX;
        UA_ExpandedNodeId_init(&next->targets[k]);
        result->targetsSize++;
    }

    /* No results => BadNoMatch status code */
    if(result->targetsSize == 0 && result->statusCode == UA_STATUSCODE_GOOD)
        result->statusCode = UA_STATUSCODE_BADNOMATCH;

    /* Clean up the temporary arrays and the targets */
 cleanup:
    RefTree_clear(&rt1);
    RefTree_clear(&rt2);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < result->targetsSize; ++i)
            UA_BrowsePathTarget_clear(&result->targets[i]);
        if(result->targets)
            UA_free(result->targets);
        result->targets = NULL;
        result->targetsSize = 0;
    }
}

UA_BrowsePathResult
translateBrowsePathToNodeIds(UA_Server *server,
                                       const UA_BrowsePath *browsePath) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_BrowsePathResult result;
    UA_BrowsePathResult_init(&result);
    UA_UInt32 nodeClassMask = 0; /* All node classes */
    Operation_TranslateBrowsePathToNodeIds(server, &server->adminSession, &nodeClassMask,
                                           browsePath, &result);
    return result;
}

UA_BrowsePathResult
UA_Server_translateBrowsePathToNodeIds(UA_Server *server,
                                       const UA_BrowsePath *browsePath) {
    UA_LOCK(&server->serviceMutex);
    UA_BrowsePathResult result = translateBrowsePathToNodeIds(server, browsePath);
    UA_UNLOCK(&server->serviceMutex);
    return result;
}

void
Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
                                      const UA_TranslateBrowsePathsToNodeIdsRequest *request,
                                      UA_TranslateBrowsePathsToNodeIdsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing TranslateBrowsePathsToNodeIdsRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Test the number of operations in the request */
    if(server->config.maxNodesPerTranslateBrowsePathsToNodeIds != 0 &&
       request->browsePathsSize > server->config.maxNodesPerTranslateBrowsePathsToNodeIds) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    UA_UInt32 nodeClassMask = 0; /* All node classes */
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation)Operation_TranslateBrowsePathToNodeIds,
                                           &nodeClassMask,
                                           &request->browsePathsSize, &UA_TYPES[UA_TYPES_BROWSEPATH],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_BROWSEPATHRESULT]);
}

UA_BrowsePathResult
browseSimplifiedBrowsePath(UA_Server *server, const UA_NodeId origin,
                           size_t browsePathSize, const UA_QualifiedName *browsePath) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_BrowsePathResult bpr;
    UA_BrowsePathResult_init(&bpr);
    if(browsePathSize > UA_MAX_TREE_RECURSE) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Simplified Browse Path too long");
        bpr.statusCode = UA_STATUSCODE_BADINTERNALERROR;
        return bpr;
    }

    /* Construct the BrowsePath */
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = origin;

    UA_RelativePathElement rpe[UA_MAX_TREE_RECURSE];
    memset(rpe, 0, sizeof(UA_RelativePathElement) * browsePathSize);
    for(size_t j = 0; j < browsePathSize; j++) {
        rpe[j].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
        rpe[j].includeSubtypes = true;
        rpe[j].targetName = browsePath[j];
    }
    bp.relativePath.elements = rpe;
    bp.relativePath.elementsSize = browsePathSize;

    /* Browse */
    UA_UInt32 nodeClassMask = UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE | UA_NODECLASS_OBJECTTYPE;

    Operation_TranslateBrowsePathToNodeIds(server, &server->adminSession, &nodeClassMask, &bp, &bpr);
    return bpr;
}

UA_BrowsePathResult
UA_Server_browseSimplifiedBrowsePath(UA_Server *server, const UA_NodeId origin,
                           size_t browsePathSize, const UA_QualifiedName *browsePath) {
    UA_LOCK(&server->serviceMutex);
    UA_BrowsePathResult bpr = browseSimplifiedBrowsePath(server, origin, browsePathSize, browsePath);
    UA_UNLOCK(&server->serviceMutex);
    return bpr;
}

/************/
/* Register */
/************/

void Service_RegisterNodes(UA_Server *server, UA_Session *session,
                           const UA_RegisterNodesRequest *request,
                           UA_RegisterNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing RegisterNodesRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    //TODO: hang the nodeids to the session if really needed
    if(request->nodesToRegisterSize == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    /* Test the number of operations in the request */
    if(server->config.maxNodesPerRegisterNodes != 0 &&
       request->nodesToRegisterSize > server->config.maxNodesPerRegisterNodes) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    response->responseHeader.serviceResult =
        UA_Array_copy(request->nodesToRegister, request->nodesToRegisterSize,
                      (void**)&response->registeredNodeIds, &UA_TYPES[UA_TYPES_NODEID]);
    if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        response->registeredNodeIdsSize = request->nodesToRegisterSize;
}

void Service_UnregisterNodes(UA_Server *server, UA_Session *session,
                             const UA_UnregisterNodesRequest *request,
                             UA_UnregisterNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing UnRegisterNodesRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    //TODO: remove the nodeids from the session if really needed
    if(request->nodesToUnregisterSize == 0)
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;

    /* Test the number of operations in the request */
    if(server->config.maxNodesPerRegisterNodes != 0 &&
       request->nodesToUnregisterSize > server->config.maxNodesPerRegisterNodes) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }
}
