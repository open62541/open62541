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

/********************/
/* Browse Recursive */
/********************/

/* A RefTree holds a single array for both the NodeIds encountered during
 * recursive browsing and the entries for a tree-structure to check for
 * duplicates. Once the (recursive) browse has finished, the tree-structure part
 * can be simply cut away. A single realloc operation (with some pointer
 * repairing) can be used to increase the capacity of the RefTree.
 *
 * If an ExpandedNodeId is encountered, it has to be processed right away.
 * Remote ExpandedNodeId are not put into the tree, since it is not possible to
 * recurse into them anyway.
 *
 * The layout of the results array is as follows:
 *
 * | Targets [ExpandedNodeId] | Tree [RefEntry] | */

#define UA_BROWSE_INITIAL_SIZE 16

typedef struct RefEntry {
    ZIP_ENTRY(RefEntry) zipfields;
    const UA_ExpandedNodeId *target;
    UA_UInt32 targetHash; /* Hash of the target nodeid */
} RefEntry;
 
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

ZIP_HEAD(RefHead, RefEntry);
typedef struct RefHead RefHead;
ZIP_PROTTYPE(RefHead, RefEntry, RefEntry)
ZIP_IMPL(RefHead, RefEntry, zipfields, RefEntry, zipfields, cmpTarget)

typedef struct {
    UA_ExpandedNodeId *targets;
    RefHead head;
    size_t capacity; /* available space */
    size_t size;     /* used space */
} RefTree;

static UA_StatusCode UA_FUNC_ATTR_WARN_UNUSED_RESULT
RefTree_init(RefTree *rt) {
    rt->size = 0;
    rt->capacity = 0;
    ZIP_INIT(&rt->head);
    size_t space = (sizeof(UA_ExpandedNodeId) + sizeof(RefEntry)) * UA_BROWSE_INITIAL_SIZE;
    rt->targets = (UA_ExpandedNodeId*)UA_malloc(space);
    if(!rt->targets)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    rt->capacity = UA_BROWSE_INITIAL_SIZE;
    return UA_STATUSCODE_GOOD;
}

static void
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

    rt->head.zip_root = (RefEntry*)((uintptr_t)rt->head.zip_root + entrydiff);
    rt->capacity = capacity;
    rt->targets = newTargets;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode UA_FUNC_ATTR_WARN_UNUSED_RESULT
RefTree_add(RefTree *rt, const UA_ExpandedNodeId *target) {
    /* Is the target already in the tree? */
    RefEntry dummy;
    dummy.target = target;
    dummy.targetHash = UA_ExpandedNodeId_hash(target);
    if(ZIP_FIND(RefHead, &rt->head, &dummy))
        return UA_STATUSCODE_GOOD;

    UA_StatusCode s = UA_STATUSCODE_GOOD;
    if(rt->capacity <= rt->size) {
        s = RefTree_double(rt);
        if(s != UA_STATUSCODE_GOOD)
            return s;
    }
    s = UA_ExpandedNodeId_copy(target, &rt->targets[rt->size]);
    if(s != UA_STATUSCODE_GOOD)
        return s;
    RefEntry *re = (RefEntry*)((uintptr_t)rt->targets +
                               (sizeof(UA_ExpandedNodeId) * rt->capacity) +
                               (sizeof(RefEntry) * rt->size));
    re->target = &rt->targets[rt->size];
    re->targetHash = dummy.targetHash;
    ZIP_INSERT(RefHead, &rt->head, re, ZIP_FFS32(UA_UInt32_random()));
    rt->size++;
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
relevantReference(const UA_NodeId *refType, size_t relevantRefsSize,
                  const UA_NodeId *relevantRefs) {
    if(!relevantRefs)
        return true;
    for(size_t i = 0; i < relevantRefsSize; i++) {
        if(UA_NodeId_equal(refType, &relevantRefs[i]))
            return true;
    }
    return false;
}

static UA_StatusCode
addRelevantReferences(UA_Server *server, RefTree *rt, const UA_NodeId *nodeId,
                      size_t refTypesSize, const UA_NodeId *refTypes,
                      UA_BrowseDirection browseDirection) {
    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < node->referencesSize; i++) {
        UA_NodeReferenceKind *rk = &node->references[i];

        /* Reference in the right direction? */
        if(rk->isInverse && browseDirection == UA_BROWSEDIRECTION_FORWARD)
            continue;
        if(!rk->isInverse && browseDirection == UA_BROWSEDIRECTION_INVERSE)
            continue;

        /* Is the reference part of the hierarchy of references we look for? */
        if(!relevantReference(&rk->referenceTypeId, refTypesSize, refTypes))
            continue;

        for(size_t k = 0; k < rk->refTargetsSize; k++) {
            retval = RefTree_add(rt, &rk->refTargets[k].targetId);
            if(retval != UA_STATUSCODE_GOOD)
                goto cleanup;
        }
    }

 cleanup:
    UA_NODESTORE_RELEASE(server, node);
    return retval;
}

UA_StatusCode
browseRecursive(UA_Server *server,
                size_t startNodesSize, const UA_NodeId *startNodes,
                size_t refTypesSize, const UA_NodeId *refTypes,
                UA_BrowseDirection browseDirection, UA_Boolean includeStartNodes,
                size_t *resultsSize, UA_ExpandedNodeId **results) {
    RefTree rt;
    UA_StatusCode retval = RefTree_init(&rt);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Add the start nodes? */
    UA_ExpandedNodeId en = UA_EXPANDEDNODEID_NULL;
    for(size_t i = 0; i < startNodesSize && retval == UA_STATUSCODE_GOOD; i++) {
        if(includeStartNodes) {
            en.nodeId = startNodes[i];
            retval = RefTree_add(&rt, &en);
        } else {
            retval = addRelevantReferences(server, &rt, &startNodes[i],
                                           refTypesSize, refTypes, browseDirection);
        }
    }
    if(retval != UA_STATUSCODE_GOOD) {
        RefTree_clear(&rt);
        return retval;
    }

    /* Loop over the targets we have so far. This recurses, as new targets are
     * added to rt. */
    for(size_t i = 0; i < rt.size; i++) {
        /* Dont recurse into remote nodes */
        if(rt.targets[i].serverIndex > 0)
            continue;
        if(rt.targets[i].namespaceUri.data != NULL)
            continue;

        retval = addRelevantReferences(server, &rt, &rt.targets[i].nodeId,
                                       refTypesSize, refTypes, browseDirection);
        if(retval != UA_STATUSCODE_GOOD) {
            RefTree_clear(&rt);
            return retval;
        }
    }

    if(rt.size > 0) {
        *results = rt.targets;
        *resultsSize = rt.size;
    } else {
        RefTree_clear(&rt);
    }

    return UA_STATUSCODE_GOOD;
}

/* Only if IncludeSubtypes is selected */
UA_StatusCode
referenceSubtypes(UA_Server *server, const UA_NodeId *refType,
                  size_t *refTypesSize, UA_NodeId **refTypes) {
    /* Leave refTypes == NULL */
    if(UA_NodeId_isNull(refType))
        return UA_STATUSCODE_GOOD;

    /* Browse recursive for the hierarchy of sub-references */
    UA_ExpandedNodeId *rt = NULL;
    size_t rtSize = 0;
    UA_NodeId hasSubtype = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    UA_StatusCode retval = browseRecursive(server, 1, refType, 1, &hasSubtype,
                                           UA_BROWSEDIRECTION_FORWARD, true, &rtSize, &rt);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_assert(rtSize > 0);

    /* Allocate space (realloc if non-NULL) */
    UA_NodeId *newRt = NULL;
    if(!*refTypes) {
        newRt = (UA_NodeId*)UA_malloc(rtSize * UA_TYPES[UA_TYPES_NODEID].memSize);
    } else {
        newRt = (UA_NodeId*)UA_realloc(*refTypes, (*refTypesSize + rtSize) *
                                           UA_TYPES[UA_TYPES_NODEID].memSize);
    }
    if(!newRt) {
        UA_Array_delete(rt, rtSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    *refTypes = newRt;

    /* Move NodeIds */
    for(size_t i = 0; i < rtSize; i++) {
        (*refTypes)[*refTypesSize + i] = rt[i].nodeId;
        UA_NodeId_init(&rt[i].nodeId);
    }
    *refTypesSize += rtSize;
    UA_Array_delete(rt, rtSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_browseRecursive(UA_Server *server, const UA_BrowseDescription *bd,
                          size_t *resultsSize, UA_ExpandedNodeId **results) {
    /* Set the list of relevant reference types */
    UA_LOCK(server->serviceMutex);
    UA_NodeId *refTypes = NULL;
    size_t refTypesSize = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!UA_NodeId_isNull(&bd->referenceTypeId)) {
        if(!bd->includeSubtypes) {
            refTypes = (UA_NodeId*)(uintptr_t)&bd->referenceTypeId;
            refTypesSize = 1;
        } else {
            retval = referenceSubtypes(server, &bd->referenceTypeId,
                                       &refTypesSize, &refTypes);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_UNLOCK(server->serviceMutex);
                return retval;
            }
        }
    }

    /* Browse */
    retval = browseRecursive(server, 1, &bd->nodeId, refTypesSize, refTypes,
                             bd->browseDirection, false, resultsSize, results);

    /* Clean up */
    if(refTypes && bd->includeSubtypes)
        UA_Array_delete(refTypes, refTypesSize, &UA_TYPES[UA_TYPES_NODEID]);

    UA_UNLOCK(server->serviceMutex);
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
        UA_Array_new(UA_BROWSE_INITIAL_SIZE, &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION]);
    if(!rr->descr)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    rr->capacity = UA_BROWSE_INITIAL_SIZE;
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
    UA_BrowseDescription browseDescription;
    UA_UInt32 maxReferences;

    size_t relevantReferencesSize;
    UA_NodeId *relevantReferences;

    /* The last point in the node references? */
    size_t referenceKindIndex;
    size_t targetIndex;
};

ContinuationPoint *
ContinuationPoint_clear(ContinuationPoint *cp) {
    UA_ByteString_clear(&cp->identifier);
    UA_BrowseDescription_clear(&cp->browseDescription);
    UA_Array_delete(cp->relevantReferences, cp->relevantReferencesSize,
                    &UA_TYPES[UA_TYPES_NODEID]);
    return cp->next;
}

/* Target node on top of the stack */
static UA_StatusCode UA_FUNC_ATTR_WARN_UNUSED_RESULT
addReferenceDescription(UA_Server *server, RefResult *rr, const UA_NodeReferenceKind *ref,
                        UA_UInt32 mask, const UA_ExpandedNodeId *nodeId, const UA_Node *curr) {
    /* Ensure capacity is left */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(rr->size >= rr->capacity) {
        retval = RefResult_double(rr);
        if(retval != UA_STATUSCODE_GOOD)
           return retval;
    }

    UA_ReferenceDescription *descr = &rr->descr[rr->size];

    /* Fields without access to the actual node */
    retval = UA_ExpandedNodeId_copy(nodeId, &descr->nodeId);
    if(mask & UA_BROWSERESULTMASK_REFERENCETYPEID)
        retval |= UA_NodeId_copy(&ref->referenceTypeId, &descr->referenceTypeId);
    if(mask & UA_BROWSERESULTMASK_ISFORWARD)
        descr->isForward = !ref->isInverse;

    /* Remote references (ExpandedNodeId) are not further looked up here */
    if(!curr) {
        UA_ReferenceDescription_clear(descr);
        return retval;
    }
    
    /* Fields that require the actual node */
    if(mask & UA_BROWSERESULTMASK_NODECLASS)
        retval |= UA_NodeClass_copy(&curr->nodeClass, &descr->nodeClass);
    if(mask & UA_BROWSERESULTMASK_BROWSENAME)
        retval |= UA_QualifiedName_copy(&curr->browseName, &descr->browseName);
    if(mask & UA_BROWSERESULTMASK_DISPLAYNAME)
        retval |= UA_LocalizedText_copy(&curr->displayName, &descr->displayName);
    if(mask & UA_BROWSERESULTMASK_TYPEDEFINITION) {
        if(curr->nodeClass == UA_NODECLASS_OBJECT ||
           curr->nodeClass == UA_NODECLASS_VARIABLE) {
            const UA_Node *type = getNodeType(server, curr);
            if(type) {
                retval |= UA_NodeId_copy(&type->nodeId, &descr->typeDefinition.nodeId);
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

static UA_Boolean
matchClassMask(const UA_Node *node, UA_UInt32 nodeClassMask) {
    if(nodeClassMask != UA_NODECLASS_UNSPECIFIED &&
       (node->nodeClass & nodeClassMask) == 0)
        return false;
    return true;
}

/* Returns whether the node / continuationpoint is done */
static UA_StatusCode
browseReferences(UA_Server *server, const UA_Node *node,
                 ContinuationPoint *cp, RefResult *rr, UA_Boolean *done) {
    UA_assert(cp != NULL);
    const UA_BrowseDescription *bd= &cp->browseDescription;

    size_t referenceKindIndex = cp->referenceKindIndex;
    size_t targetIndex = cp->targetIndex;

    /* Loop over the node's references */
    const UA_Node *target = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(; referenceKindIndex < node->referencesSize; ++referenceKindIndex) {
        UA_NodeReferenceKind *rk = &node->references[referenceKindIndex];

        /* Reference in the right direction? */
        if(rk->isInverse && bd->browseDirection == UA_BROWSEDIRECTION_FORWARD)
            continue;
        if(!rk->isInverse && bd->browseDirection == UA_BROWSEDIRECTION_INVERSE)
            continue;

        /* Is the reference part of the hierarchy of references we look for? */
        if(!relevantReference(&rk->referenceTypeId, cp->relevantReferencesSize,
                              cp->relevantReferences))
            continue;

        /* Loop over the targets */
        for(; targetIndex < rk->refTargetsSize; ++targetIndex) {
            target = NULL;

            /* Get the node if it is not a remote reference */
            if(rk->refTargets[targetIndex].targetId.serverIndex == 0 &&
               rk->refTargets[targetIndex].targetId.namespaceUri.data == NULL) {
                target = UA_NODESTORE_GET(server,
                                          &rk->refTargets[targetIndex].targetId.nodeId);

                /* Test if the node class matches */
                if(target && !matchClassMask(target, bd->nodeClassMask)) {
                    if(target)
                        UA_NODESTORE_RELEASE(server, target);
                    continue;
                }
            }

            /* A match! Did we reach maxrefs? */
            if(rr->size >= cp->maxReferences) {
                cp->referenceKindIndex = referenceKindIndex;
                cp->targetIndex = targetIndex;
                if(target)
                    UA_NODESTORE_RELEASE(server, target);
                return UA_STATUSCODE_GOOD;
            }

            /* Copy the node description. Target is on top of the stack */
            retval = addReferenceDescription(server, rr, rk, bd->resultMask,
                                             &rk->refTargets[targetIndex].targetId, target);
            UA_NODESTORE_RELEASE(server, target);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }

        targetIndex = 0; /* Start at index 0 for the next reference kind */
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
        const UA_Node *reftype = UA_NODESTORE_GET(server, &descr->referenceTypeId);
        if(!reftype) {
            result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
            return true;
        }

        UA_Boolean isRef = (reftype->nodeClass == UA_NODECLASS_REFERENCETYPE);
        UA_NODESTORE_RELEASE(server, reftype);

        if(!isRef) {
            result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
            return true;
        }
    }

    const UA_Node *node = UA_NODESTORE_GET(server, &descr->nodeId);
    if(!node) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return true;
    }

    if(session != &server->adminSession &&
       !server->config.accessControl.allowBrowseNode(server, &server->config.accessControl,
                                                     &session->sessionId, session->sessionHandle,
                                                     &descr->nodeId, node->context)) {
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
    result->statusCode = browseReferences(server, node, cp, &rr, &done);
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
    UA_STACKARRAY(ContinuationPoint, cp, 1);
    memset(cp, 0, sizeof(ContinuationPoint));
    cp->maxReferences = *maxrefs;
    cp->browseDescription = *descr; /* Shallow copy. Deep-copy later if we persist the cp. */

    /* How many references can we return at most? */
    if(cp->maxReferences == 0) {
        if(server->config.maxReferencesPerNode != 0) {
            cp->maxReferences = server->config.maxReferencesPerNode;
        } else {
            cp->maxReferences = UA_INT32_MAX;
        }
    } else {
        if(server->config.maxReferencesPerNode != 0 &&
           cp->maxReferences > server->config.maxReferencesPerNode) {
            cp->maxReferences= server->config.maxReferencesPerNode;
        }
    }

    /* Get the list of relevant reference types */
    if(!UA_NodeId_isNull(&descr->referenceTypeId)) {
        if(!descr->includeSubtypes) {
            cp->relevantReferences = (UA_NodeId*)(uintptr_t)&descr->referenceTypeId;
            cp->relevantReferencesSize = 1;
        } else {
            result->statusCode =
                referenceSubtypes(server, &descr->referenceTypeId,
                                  &cp->relevantReferencesSize, &cp->relevantReferences);
            if(result->statusCode != UA_STATUSCODE_GOOD)
                return;
        }
    }

    UA_Boolean done = browseWithContinuation(server, session, cp, result);

    /* Exit early if done or an error occurred */
    if(done || result->statusCode != UA_STATUSCODE_GOOD) {
        if(descr->includeSubtypes)
            UA_Array_delete(cp->relevantReferences, cp->relevantReferencesSize,
                            &UA_TYPES[UA_TYPES_NODEID]);
        return;
    }

    /* Persist the new continuation point */

    ContinuationPoint *cp2 = NULL;
    UA_Guid *ident = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Enough space for the continuation point? */
    if(session->availableContinuationPoints <= 0) {
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
    cp2->referenceKindIndex = cp->referenceKindIndex;
    cp2->targetIndex = cp->targetIndex;
    cp2->maxReferences = cp->maxReferences;

    if(descr->includeSubtypes) {
        cp2->relevantReferences = cp->relevantReferences;
        cp2->relevantReferencesSize = cp->relevantReferencesSize;
    } else {
        retval = UA_Array_copy(cp->relevantReferences, cp->relevantReferencesSize,
                               (void**)&cp2->relevantReferences, &UA_TYPES[UA_TYPES_NODEID]);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        cp2->relevantReferencesSize = cp->relevantReferencesSize;
    }

    /* Copy the description */
    retval = UA_BrowseDescription_copy(descr, &cp2->browseDescription);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

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
    UA_LOCK_ASSERT(server->serviceMutex, 1);

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
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_Browse,
                                           &request->requestedMaxReferencesPerNode,
                                           &request->nodesToBrowseSize, &UA_TYPES[UA_TYPES_BROWSEDESCRIPTION],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_BROWSERESULT]);
}

UA_BrowseResult
UA_Server_browse(UA_Server *server, UA_UInt32 maxReferences,
                 const UA_BrowseDescription *bd) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    UA_LOCK(server->serviceMutex);
    Operation_Browse(server, &server->adminSession, &maxReferences, bd, &result);
    UA_UNLOCK(server->serviceMutex);
    return result;
}

static void
Operation_BrowseNext(UA_Server *server, UA_Session *session,
                     const UA_Boolean *releaseContinuationPoints,
                     const UA_ByteString *continuationPoint, UA_BrowseResult *result) {
    /* Find the continuation point */
    ContinuationPoint **prev = &session->continuationPoints, *cp;
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
        UA_StatusCode retval = UA_ByteString_copy(&cp->identifier, &result->continuationPoint);
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
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    UA_Boolean releaseContinuationPoints = request->releaseContinuationPoints; /* request is const */
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_BrowseNext,
                                           &releaseContinuationPoints,
                                           &request->continuationPointsSize, &UA_TYPES[UA_TYPES_BYTESTRING],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_BROWSERESULT]);
}

UA_BrowseResult
UA_Server_browseNext(UA_Server *server, UA_Boolean releaseContinuationPoint,
                     const UA_ByteString *continuationPoint) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    UA_LOCK(server->serviceMutex);
    Operation_BrowseNext(server, &server->adminSession, &releaseContinuationPoint,
                         continuationPoint, &result);
    UA_UNLOCK(server->serviceMutex);
    return result;
}

/***********************/
/* TranslateBrowsePath */
/***********************/

static UA_StatusCode
addBrowseTarget(RefTree *next, const UA_ReferenceTarget *rt) {
    UA_assert(rt);
    UA_StatusCode res = RefTree_add(next, &rt->targetId);
    UA_ReferenceTarget *left = ZIP_LEFT(rt, nameTreeFields);
    if(left && left->targetNameHash == rt->targetNameHash)
        res |= addBrowseTarget(next, left);
    UA_ReferenceTarget *right = ZIP_RIGHT(rt, nameTreeFields);
    if(right && right->targetNameHash == rt->targetNameHash)
        res |= addBrowseTarget(next, right);
    return res;
}

static UA_StatusCode
walkBrowsePathElement(UA_Server *server, UA_Session *session,
                      const UA_RelativePath *path, const size_t pathIndex, UA_UInt32 nodeClassMask,
                      const UA_QualifiedName *lastBrowseName, UA_BrowsePathResult *result,
                      RefTree *current, RefTree *next) {
    /* For the next level. Note the difference from lastBrowseName */
    const UA_RelativePathElement *elem = &path->elements[pathIndex];
    UA_UInt32 browseNameHash = UA_QualifiedName_hash(&elem->targetName);

    /* Is the ReferenceType valid? */
    UA_Boolean all_refs = UA_NodeId_isNull(&elem->referenceTypeId);
    if(!all_refs) {
        const UA_Node *rootRef = UA_NODESTORE_GET(server, &elem->referenceTypeId);
        if(!rootRef)
            return UA_STATUSCODE_BADNOMATCH;
        UA_Boolean match = (rootRef->nodeClass == UA_NODECLASS_REFERENCETYPE);
        UA_NODESTORE_RELEASE(server, rootRef);
        if(!match)
            return UA_STATUSCODE_BADNOMATCH;
    }

    /* Loop over all Nodes int the current depth level */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < current->size; i++) {
        /* Remote Node. Immediately add to the results with the RemainingPathIndex set. */
        if(current->targets[i].serverIndex > 0 ||
           current->targets[i].namespaceUri.length > 0) {
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

        /* Local Node. Add to the tree of results at the next depth. */
        const UA_Node *node = UA_NODESTORE_GET(server, &current->targets[i].nodeId);
        if(!node)
            continue;

        /* Test whether the node fits the class mask */
        UA_Boolean skip = !matchClassMask(node, nodeClassMask);

        /* Does the BrowseName match for the current node (not the rerences
         * going out here) */
        skip |= (lastBrowseName && !UA_QualifiedName_equal(lastBrowseName, &node->browseName));

        if(skip) {
            UA_NODESTORE_RELEASE(server, node);
            continue;
        }

        /* Loop over the ReferenceKinds */
        for(size_t j = 0; j < node->referencesSize; j++) {
            UA_NodeReferenceKind *rk = &node->references[j];

            /* Does the direction of the reference match? */
            if(rk->isInverse != elem->isInverse)
                continue;

            /* Does the reference type match? */
            if(!all_refs) {
                if(!elem->includeSubtypes) {
                    if(!UA_NodeId_equal(&rk->referenceTypeId, &elem->referenceTypeId))
                        continue;
                } else {
                    UA_Boolean match =
                        isNodeInTree(server, &rk->referenceTypeId,
                                     &elem->referenceTypeId, &subtypeId, 1);
                    if(!match)
                        continue;
                }
            }

            /* Retrieve by BrowseName hash */
            UA_ReferenceTarget *rt = ZIP_FIND(UA_ReferenceTargetNameTree,
                                              &rk->refTargetsNameTree, &browseNameHash);
            if(!rt)
                continue;

            res = addBrowseTarget(next, rt);
            if(res != UA_STATUSCODE_GOOD)
                break;
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
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    if(path->relativePath.elementsSize <= 0) {
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
    const UA_Node *startingNode = UA_NODESTORE_GET(server, &path->startingNode);
    if(!startingNode) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }
    UA_NODESTORE_RELEASE(server, startingNode);

    /* Create two RefTrees that are alternated between path elements */
    RefTree rt1, rt2, *current = &rt1, *next = &rt2, *tmp;
    result->statusCode |= RefTree_init(&rt1);
    result->statusCode |= RefTree_init(&rt2);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Copy the starting node into next */
    UA_ExpandedNodeId startingNodeId;
    UA_ExpandedNodeId_init(&startingNodeId);
    startingNodeId.nodeId = path->startingNode;
    result->statusCode = RefTree_add(next, &startingNodeId);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Walk the path elements. Retrieve the nodes only once from the NodeStore.
     * Hence the BrowseName is checked with one element "delay". */
    UA_QualifiedName *browseNameFilter = NULL;
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
    UA_BrowsePathTarget *tmpResults = (UA_BrowsePathTarget*)
        UA_realloc(result->targets, sizeof(UA_BrowsePathTarget) *
                   (result->targetsSize + next->size));
    if(!tmpResults) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    result->targets = tmpResults;

    for(size_t k = 0; k < next->size; k++) {
        /* Check the BrowseName. It has been filtered only via its hash so far. */
        const UA_Node *node = UA_NODESTORE_GET(server, &next->targets[k].nodeId);
        if(!node)
            continue;
        UA_Boolean match = UA_QualifiedName_equal(browseNameFilter, &node->browseName);
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
    UA_LOCK_ASSERT(server->serviceMutex, 1);
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
    UA_LOCK(server->serviceMutex);
    UA_BrowsePathResult result = translateBrowsePathToNodeIds(server, browsePath);
    UA_UNLOCK(server->serviceMutex);
    return result;
}

void
Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
                                      const UA_TranslateBrowsePathsToNodeIdsRequest *request,
                                      UA_TranslateBrowsePathsToNodeIdsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing TranslateBrowsePathsToNodeIdsRequest");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

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
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* Construct the BrowsePath */
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = origin;
    UA_STACKARRAY(UA_RelativePathElement, rpe, browsePathSize);
    memset(rpe, 0, sizeof(UA_RelativePathElement) * browsePathSize);
    for(size_t j = 0; j < browsePathSize; j++) {
        rpe[j].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
        rpe[j].includeSubtypes = true;
        rpe[j].targetName = browsePath[j];
    }
    bp.relativePath.elements = rpe;
    bp.relativePath.elementsSize = browsePathSize;

    /* Browse */
    UA_BrowsePathResult bpr;
    UA_BrowsePathResult_init(&bpr);
    UA_UInt32 nodeClassMask = UA_NODECLASS_OBJECT | UA_NODECLASS_VARIABLE;
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    nodeClassMask |= UA_NODECLASS_OBJECTTYPE;
#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */

    Operation_TranslateBrowsePathToNodeIds(server, &server->adminSession, &nodeClassMask, &bp, &bpr);
    return bpr;
}

UA_BrowsePathResult
UA_Server_browseSimplifiedBrowsePath(UA_Server *server, const UA_NodeId origin,
                           size_t browsePathSize, const UA_QualifiedName *browsePath) {
    UA_LOCK(server->serviceMutex);
    UA_BrowsePathResult bpr = browseSimplifiedBrowsePath(server, origin, browsePathSize, browsePath);
    UA_UNLOCK(server->serviceMutex);
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
    UA_LOCK_ASSERT(server->serviceMutex, 1);

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
    UA_LOCK_ASSERT(server->serviceMutex, 1);

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
