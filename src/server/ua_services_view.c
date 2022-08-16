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

/* Keeps track of visited nodes to detect circular references */
struct ref_history {
    struct ref_history *parent; /* the previous element */
    const UA_NodePointer id;    /* the id of the node at this depth */
    UA_UInt16 depth;
};

static UA_Boolean
isNodeInTreeNoCircular(UA_Server *server,
                       UA_NodePointer leafNode,
                       UA_NodePointer nodeToFind,
                       struct ref_history *visitedRefs,
                       const UA_ReferenceTypeSet *relevantRefs) {
    if(UA_NodePointer_equal(nodeToFind, leafNode))
        return true;

    if(visitedRefs->depth >= UA_MAX_TREE_RECURSE)
        return false;

    /* Get the node without attributes (if the NodeStore supports it) and only
     * the relevant references in inverse direction */
    const UA_Node *node =
        UA_NODESTORE_GETFROMREF_SELECTIVE(server, leafNode,
                                          UA_NODEATTRIBUTESMASK_NONE,
                                          *relevantRefs,
                                          UA_BROWSEDIRECTION_INVERSE);
    if(!node)
        return false;

    for(size_t i = 0; i < node->head.referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &node->head.references[i];
        /* Search upwards in the tree */
        if(!rk->isInverse)
            continue;

        /* Consider only the indicated reference types */
        if(!UA_ReferenceTypeSet_contains(relevantRefs, rk->referenceTypeIndex))
            continue;

        /* Match the targets or recurse */
        const UA_ReferenceTarget *t = NULL;
        while((t = UA_NodeReferenceKind_iterate(rk, t))) {
            /* Don't follow remote targets */
            if(!UA_NodePointer_isLocal(t->targetId))
                continue;

            /* Check if we already have seen the referenced node and skip to
             * avoid endless recursion. Do this only at every 5th depth to save
             * effort. Circular dependencies are rare and forbidden for most
             * reference types. */
            if(visitedRefs->depth % 5 == 4) {
                struct ref_history *last = visitedRefs;
                UA_Boolean skip = false;
                while(last) {
                    if(UA_NodePointer_equal(last->id, t->targetId)) {
                        skip = true;
                        break;
                    }
                    last = last->parent;
                }
                if(skip)
                    continue;
            }

            /* Stack-allocate the visitedRefs structure for the next depth */
            struct ref_history nextVisitedRefs = {visitedRefs, t->targetId,
                                                  (UA_UInt16)(visitedRefs->depth+1)};

            /* Recurse */
            UA_Boolean foundRecursive =
                isNodeInTreeNoCircular(server, t->targetId, nodeToFind,
                                       &nextVisitedRefs, relevantRefs);
            if(foundRecursive) {
                UA_NODESTORE_RELEASE(server, node);
                return true;
            }
        }
    }

    UA_NODESTORE_RELEASE(server, node);
    return false;
}

UA_Boolean
isNodeInTree(UA_Server *server, const UA_NodeId *leafNode,
             const UA_NodeId *nodeToFind,
             const UA_ReferenceTypeSet *relevantRefs) {
    UA_NodePointer leafP = UA_NodePointer_fromNodeId(leafNode);
    UA_NodePointer targetP = UA_NodePointer_fromNodeId(nodeToFind);
    struct ref_history visitedRefs = {NULL, leafP, 0};
    return isNodeInTreeNoCircular(server, leafP, targetP,
                                  &visitedRefs, relevantRefs);
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

    /* Repair the pointers for the realloced array+tree  */
    // What is this sorcery?
    // FIXME: This needs some cleaning up or explanation.
    // IMO uintptr could be completely avoided here.
    uintptr_t arraydiff = (uintptr_t)newTargets - (uintptr_t)rt->targets;
    RefEntry *reArray = (RefEntry*)
        ((uintptr_t)newTargets + (capacity * sizeof(UA_ExpandedNodeId)));
    uintptr_t entrydiff = (uintptr_t)reArray -
        ((uintptr_t)rt->targets + (rt->capacity * sizeof(UA_ExpandedNodeId)));
    RefEntry *oldReArray = (RefEntry*)
        ((uintptr_t)newTargets + (rt->capacity * sizeof(UA_ExpandedNodeId)));
    memmove(reArray, oldReArray, rt->size * sizeof(RefEntry));
    for(size_t i = 0; i < rt->size; i++) {
        uintptr_t *left = (uintptr_t*)&ZIP_LEFT(&reArray[i], zipfields);
        uintptr_t *right = (uintptr_t*)&ZIP_RIGHT(&reArray[i], zipfields);
        if(*left != 0)
            *left += entrydiff;
        if(*right != 0)
            *right += entrydiff;
        reArray[i].target = (UA_ExpandedNodeId*)((uintptr_t)reArray[i].target + arraydiff);
    }

    ZIP_ROOT(&rt->head) = (RefEntry*)((uintptr_t)ZIP_ROOT(&rt->head) + entrydiff);
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
    ZIP_INSERT(RefHead, &rt->head, re, UA_UInt32_random());
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

static UA_StatusCode
browseRecursiveInner(UA_Server *server, RefTree *rt, UA_UInt16 depth, UA_Boolean skip,
                     UA_NodePointer nodeP, UA_BrowseDirection browseDirection,
                     const UA_ReferenceTypeSet *refTypes, UA_UInt32 nodeClassMask) {
    /* Have we reached the max recursion depth? */
    if(depth >= UA_MAX_TREE_RECURSE)
        return UA_STATUSCODE_GOOD;

    /* We only look at the NodeClass attribute and a subset of the references.
     * Get a node with only these elements if the NodeStore supports that. */
    const UA_Node *node =
        UA_NODESTORE_GETFROMREF_SELECTIVE(server, nodeP,
                                          UA_NODEATTRIBUTESMASK_NODECLASS,
                                          *refTypes, browseDirection);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    const UA_NodeHead *head = &node->head;

    /* Add the current node to the results if we don't want to skip it (i.e. for
     * includeStartNodes) and it matches the nodeClassMask filter. Process the
     * children also if the nodeClassMask does not match. */
    if(!skip && matchClassMask(node, nodeClassMask)) {
        UA_Boolean duplicate = false;
        retval = RefTree_addNodeId(rt, &head->nodeId, &duplicate);
        if(duplicate || retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    for(size_t i = 0; i < head->referencesSize; i++) {
        UA_NodeReferenceKind *rk = &head->references[i];

        /* Reference in the right direction? */
        if(rk->isInverse && browseDirection == UA_BROWSEDIRECTION_FORWARD)
            continue;
        if(!rk->isInverse && browseDirection == UA_BROWSEDIRECTION_INVERSE)
            continue;

        /* Is the reference part of the hierarchy of references we look for? */
        if(!UA_ReferenceTypeSet_contains(refTypes, rk->referenceTypeIndex))
            continue;

        const UA_ReferenceTarget *target = NULL;
        while((target = UA_NodeReferenceKind_iterate(rk, target))) {
            if(UA_NodePointer_isLocal(target->targetId)) {
                retval = browseRecursiveInner(server, rt, (UA_UInt16)(depth+1), false,
                                              target->targetId, browseDirection,
                                              refTypes, nodeClassMask);
            } else {
                retval = RefTree_add(rt, target->targetId, NULL);
            }
            if(retval != UA_STATUSCODE_GOOD)
                goto cleanup;
        }
    }

 cleanup:
    UA_NODESTORE_RELEASE(server, node);
    return retval;
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

    for(size_t i = 0; i < startNodesSize; i++) {
        /* Call the inner recursive browse separately for the search direction.
         * Otherwise we might take one step up and another step down in the
         * search tree. */
        if(browseDirection == UA_BROWSEDIRECTION_FORWARD ||
           browseDirection == UA_BROWSEDIRECTION_BOTH)
            retval |= browseRecursiveInner(server, &rt, 0, !includeStartNodes,
                                           UA_NodePointer_fromNodeId(&startNodes[i]),
                                           UA_BROWSEDIRECTION_FORWARD,
                                           refTypes, nodeClassMask);
        if(browseDirection == UA_BROWSEDIRECTION_INVERSE ||
           browseDirection == UA_BROWSEDIRECTION_BOTH)
            retval |= browseRecursiveInner(server, &rt, 0, !includeStartNodes,
                                           UA_NodePointer_fromNodeId(&startNodes[i]),
                                           UA_BROWSEDIRECTION_INVERSE,
                                           refTypes, nodeClassMask);
        if(retval != UA_STATUSCODE_GOOD)
            break;
    }

    if(rt.size > 0 && retval == UA_STATUSCODE_GOOD) {
        *results = rt.targets;
        *resultsSize = rt.size;
    } else {
        RefTree_clear(&rt);
    }
    return retval;
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

    /* The next target to be transmitted to the client */
    UA_ExpandedNodeId nextTarget;
    UA_Byte nextRefKindIndex;
};

ContinuationPoint *
ContinuationPoint_clear(ContinuationPoint *cp) {
    UA_ByteString_clear(&cp->identifier);
    UA_BrowseDescription_clear(&cp->browseDescription);
    UA_ExpandedNodeId_clear(&cp->nextTarget);
    return cp->next;
}

/* Target node on top of the stack */
static UA_StatusCode UA_FUNC_ATTR_WARN_UNUSED_RESULT
addReferenceDescription(UA_Server *server, const UA_Session *session, RefResult *rr,
                        const UA_NodeReferenceKind *ref, UA_UInt32 mask,
                        UA_NodePointer nodeP, const UA_Node *curr) {
    /* Ensure capacity is left */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(rr->size >= rr->capacity) {
        retval = RefResult_double(rr);
        if(retval != UA_STATUSCODE_GOOD)
           return retval;
    }

    UA_ReferenceDescription *descr = &rr->descr[rr->size];

    /* Fields without access to the actual node */
    UA_ExpandedNodeId en = UA_NodePointer_toExpandedNodeId(nodeP);
    retval = UA_ExpandedNodeId_copy(&en, &descr->nodeId);
    if(mask & UA_BROWSERESULTMASK_REFERENCETYPEID) {
        const UA_NodeId *refTypeId =
            UA_NODESTORE_GETREFERENCETYPEID(server, ref->referenceTypeIndex);
        retval |= UA_NodeId_copy(refTypeId, &descr->referenceTypeId);
    }
    if(mask & UA_BROWSERESULTMASK_ISFORWARD)
        descr->isForward = !ref->isInverse;

    /* Remote references (ExpandedNodeId) are not further looked up here */
    if(!curr) {
        UA_ReferenceDescription_clear(descr);
        return retval;
    }

    /* Fields that require the actual node */
    if(mask & UA_BROWSERESULTMASK_NODECLASS)
        descr->nodeClass = curr->head.nodeClass;
    if(mask & UA_BROWSERESULTMASK_BROWSENAME)
        retval |= UA_QualifiedName_copy(&curr->head.browseName, &descr->browseName);
    if(mask & UA_BROWSERESULTMASK_DISPLAYNAME) {
        UA_LocalizedText displayname =
            UA_Session_getNodeDisplayName(session, &curr->head);
        retval |= UA_LocalizedText_copy(&displayname, &descr->displayName);
    }
    if(mask & UA_BROWSERESULTMASK_TYPEDEFINITION) {
        if(curr->head.nodeClass == UA_NODECLASS_OBJECT ||
           curr->head.nodeClass == UA_NODECLASS_VARIABLE) {
            const UA_Node *type = getNodeType(server, &curr->head);
            if(type) {
                retval |= UA_NodeId_copy(&type->head.nodeId, &descr->typeDefinition.nodeId);
                UA_NODESTORE_RELEASE(server, type);
            }
        }
    }

    if(retval == UA_STATUSCODE_GOOD)
        rr->size++; /* Increase the counter */
    else
        UA_ReferenceDescription_clear(descr);
    return retval;
}

/* Returns whether the node / continuationpoint is done */
static UA_StatusCode
browseReferences(UA_Server *server, const UA_Session *session, const UA_NodeHead *head,
                 ContinuationPoint *cp, RefResult *rr, UA_Boolean *done) {
    UA_assert(cp);
    const UA_BrowseDescription *bd = &cp->browseDescription;

    size_t i = 0;
    const UA_ReferenceTarget *ref = NULL;

    /* If the cp was previously used, skip forward to the next ReferenceType to
     * be transmitted. */
    if(cp->identifier.length > 0) {
        for(; i < head->referencesSize; ++i) {
            UA_NodeReferenceKind *rk = &head->references[i];

            /* Was this the last transmitted ReferenceType? */
            if(head->references[i].referenceTypeIndex != cp->nextRefKindIndex)
                continue;

            /* Reference in the right direction? */
            if(rk->isInverse && bd->browseDirection == UA_BROWSEDIRECTION_FORWARD)
                continue;
            if(!rk->isInverse && bd->browseDirection == UA_BROWSEDIRECTION_INVERSE)
                continue;

            /* Get the next target */
            ref = UA_NodeReferenceKind_findTarget(rk, &cp->nextTarget);
            if(ref)
                break;

            /* The target no longer exists for this ReferenceType (and
             * direction). Continue to iterate for the case that a nodestore has
             * a duplicate UA_NodeReferenceKind (should not happen though). */
        }

        /* Fail with an error if the reference no longer exists. */
        if(!ref)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Get the node with additional reference types if we need to lookup the
     * TypeDefinition */
    UA_ReferenceTypeSet resultRefs = cp->relevantReferences;
    if(bd->resultMask & UA_BROWSERESULTMASK_TYPEDEFINITION) {
        resultRefs = UA_ReferenceTypeSet_union(resultRefs,
                                               UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASTYPEDEFINITION));
        resultRefs = UA_ReferenceTypeSet_union(resultRefs,
                                               UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASSUBTYPE));
    }

    /* Loop over the ReferenceTypes */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    for(; i < head->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &head->references[i];

        /* Reference in the right direction? */
        if(rk->isInverse && bd->browseDirection == UA_BROWSEDIRECTION_FORWARD)
            continue;
        if(!rk->isInverse && bd->browseDirection == UA_BROWSEDIRECTION_INVERSE)
            continue;

        /* Is the reference part of the hierarchy of references we look for? */
        if(!UA_ReferenceTypeSet_contains(&cp->relevantReferences,
                                         rk->referenceTypeIndex))
            continue;

        /* We have a matching ReferenceType! */

        /* Loop over the targets for the ReferenceType. Start with the first
         * entry if we don't have a known entry-point from the cp. */
        if(!ref)
            ref = UA_NodeReferenceKind_iterate(rk, ref);
        for(;ref; ref = UA_NodeReferenceKind_iterate(rk, ref)) {
            /* Get the node (NULL if is a remote reference). Include only the
             * ReferenceTypes we are interested in, including those for figuring
             * out the TypeDefinition (if that was requested). */
            const UA_Node *target =
                UA_NODESTORE_GETFROMREF_SELECTIVE(server, ref->targetId,
                                                  resultMask2AttributesMask(bd->resultMask),
                                                  resultRefs, bd->browseDirection);

            /* Test if the node class matches */
            if(target && !matchClassMask(target, bd->nodeClassMask)) {
                UA_NODESTORE_RELEASE(server, target);
                continue;
            }

            /* We have a matching target! */

            /* Reached maxrefs. Update the cp and bail. */
            if(rr->size >= cp->maxReferences) {
                if(target)
                    UA_NODESTORE_RELEASE(server, target);
                cp->nextRefKindIndex = rk->referenceTypeIndex;
                /* Make a deep copy */
                UA_ExpandedNodeId tmpEn =
                    UA_NodePointer_toExpandedNodeId(ref->targetId);
                return UA_ExpandedNodeId_copy(&tmpEn, &cp->nextTarget);
            }

            /* Copy the node description. Target is on top of the stack */
            retval = addReferenceDescription(server, session, rr, rk, bd->resultMask,
                                             ref->targetId, target);
            if(target)
                UA_NODESTORE_RELEASE(server, target);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }

    /* The node is done */
    *done = true;
    return UA_STATUSCODE_GOOD;
}

/* Results for a single browsedescription. This is the inner loop for both
 * Browse and BrowseNext. The ContinuationPoint contains all the data used.
 * Including the BrowseDescription. Returns whether there are remaining
 * references. */
static UA_Boolean
browseWithContinuation(UA_Server *server, UA_Session *session,
                       ContinuationPoint *cp, UA_BrowseResult *result) {
    UA_assert(cp);
    const UA_BrowseDescription *descr = &cp->browseDescription;

    /* Is the browsedirection valid? */
    if(descr->browseDirection != UA_BROWSEDIRECTION_BOTH &&
       descr->browseDirection != UA_BROWSEDIRECTION_FORWARD &&
       descr->browseDirection != UA_BROWSEDIRECTION_INVERSE) {
        result->statusCode = UA_STATUSCODE_BADBROWSEDIRECTIONINVALID;
        return true;
    }

    /* Is the reference type valid? */
    if(!UA_NodeId_isNull(&descr->referenceTypeId)) {
        const UA_Node *reftype =
            UA_NODESTORE_GET_SELECTIVE(server, &descr->referenceTypeId,
                                       UA_NODEATTRIBUTESMASK_NODECLASS,
                                       UA_REFERENCETYPESET_NONE,
                                       UA_BROWSEDIRECTION_INVALID);
        if(!reftype) {
            result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
            return true;
        }

        UA_Boolean isRef = (reftype->head.nodeClass == UA_NODECLASS_REFERENCETYPE);
        UA_NODESTORE_RELEASE(server, reftype);

        if(!isRef) {
            result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
            return true;
        }
    }

    /* Get node with only the selected references and attributes */
    const UA_Node *node =
        UA_NODESTORE_GET_SELECTIVE(server, &descr->nodeId,
                                   resultMask2AttributesMask(descr->resultMask),
                                   cp->relevantReferences, descr->browseDirection);
    if(!node) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return true;
    }

    if(session != &server->adminSession &&
       !server->config.accessControl.
         allowBrowseNode(server, &server->config.accessControl,
                         &session->sessionId, session->sessionHandle,
                         &descr->nodeId, node->head.context)) {
        result->statusCode = UA_STATUSCODE_BADUSERACCESSDENIED;
        UA_NODESTORE_RELEASE(server, node);
        return true;
    }

    RefResult rr;
    result->statusCode = RefResult_init(&rr);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_NODESTORE_RELEASE(server, node);
        return true;
    }

    /* Browse the references */
    UA_Boolean done = false;
    result->statusCode = browseReferences(server, session, &node->head, cp, &rr, &done);
    UA_NODESTORE_RELEASE(server, node);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        RefResult_clear(&rr);
        return true;
    }

    /* Move results */
    if(rr.size > 0) {
        result->references = rr.descr;
        result->referencesSize = rr.size;
    } else {
        /* No relevant references, return array of length zero */
        RefResult_clear(&rr);
        result->references = (UA_ReferenceDescription*)UA_EMPTY_ARRAY_SENTINEL;
    }

    return done;
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

    UA_Boolean done = browseWithContinuation(server, session, &cp, result);

    /* Exit early if done or an error occurred */
    if(done || result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Persist the new continuation point */

    ContinuationPoint *cp2 = NULL;
    UA_Guid *ident = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Enough space for the continuation point? */
    if(session->availableContinuationPoints == 0) {
        retval = UA_STATUSCODE_BADNOCONTINUATIONPOINTS;
        goto cleanup;
    }

    /* Allocate and fill the data structure */
    cp2 = (ContinuationPoint*)UA_malloc(sizeof(ContinuationPoint));
    if(!cp2) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    memset(cp2, 0, sizeof(ContinuationPoint));
    /* The BrowseDescription is only a shallow copy in cp */
    retval = UA_BrowseDescription_copy(descr, &cp2->browseDescription);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    cp2->maxReferences = cp.maxReferences;
    cp2->relevantReferences = cp.relevantReferences;
    cp2->nextTarget = cp.nextTarget;
    cp2->nextRefKindIndex = cp.nextRefKindIndex;

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
    UA_BrowseResult_clear(result);
    result->statusCode = retval;
}

void Service_Browse(UA_Server *server, UA_Session *session,
                    const UA_BrowseRequest *request, UA_BrowseResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing BrowseRequest");
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

    /* Continue browsing */
    UA_Boolean done = browseWithContinuation(server, session, cp, result);

    if(done) {
        /* Remove the cp if there are no references left */
        *prev = ContinuationPoint_clear(cp);
        UA_free(cp);
        ++session->availableContinuationPoints;
    } else {
        /* Return the cp identifier */
        UA_StatusCode retval =
            UA_ByteString_copy(&cp->identifier, &result->continuationPoint);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_BrowseResult_clear(result);
            result->statusCode = retval;
        }
    }
}

void
Service_BrowseNext(UA_Server *server, UA_Session *session,
                   const UA_BrowseNextRequest *request,
                   UA_BrowseNextResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
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

/* Find all entries for that hash. There are duplicate for the possible hash
 * collisions. The exact browsename is checked afterwards. */
static UA_StatusCode
recursiveAddBrowseHashTarget(RefTree *results, struct aa_head *head,
                             const UA_ReferenceTarget *rt) {
    UA_assert(rt);
    UA_StatusCode res = RefTree_add(results, rt->targetId, NULL);
    UA_ReferenceTarget *prev = (UA_ReferenceTarget*)aa_prev(head, rt);
    while(prev && prev->targetNameHash == rt->targetNameHash) {
        res |= RefTree_add(results, prev->targetId, NULL);
        prev = (UA_ReferenceTarget*)aa_prev(head, prev);
    }
    UA_ReferenceTarget *next= (UA_ReferenceTarget*)aa_next(head, rt);
    while(next && next->targetNameHash == rt->targetNameHash) {
        res |= RefTree_add(results, next->targetId, NULL);
        next = (UA_ReferenceTarget*)aa_next(head, next);
    }
    return res;
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

    struct aa_head _refNameTree = refNameTree;

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
        for(size_t j = 0; j < node->head.referencesSize; j++) {
            UA_NodeReferenceKind *rk = &node->head.references[j];

            /* Does the direction of the reference match? */
            if(rk->isInverse != elem->isInverse)
                continue;

            /* Does the reference type match? */
            if(!UA_ReferenceTypeSet_contains(&refTypes, rk->referenceTypeIndex))
                continue;

            if(rk->hasRefTree) {
                /* Retrieve by BrowseName hash. We might have several nodes where
                 * the hash matches. The exact BrowseName will be verified in the
                 * next iteration of the outer loop. So we only have to retrieve
                 * every node just once. */
                _refNameTree.root = rk->targets.tree.nameTreeRoot;
                UA_ReferenceTarget *rt = (UA_ReferenceTarget*)
                    aa_find(&_refNameTree, &browseNameHash);
                if(!rt)
                    continue;

                res = recursiveAddBrowseHashTarget(next, &_refNameTree, rt);
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
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
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
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
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
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
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
