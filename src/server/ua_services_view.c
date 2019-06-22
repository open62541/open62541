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

/***********/
/* RefTree */
/***********/

/* References are browsed with a minimum number of copy operations. A RefTree
 * holds a single array for both the NodeIds encountered during (recursive)
 * browsing and the entries for a tree-structure to check for duplicates. Once
 * the (recursive) browse has finished, the tree-structure part can be simply
 * cut away. A single realloc operation (with some pointer repairing) can be
 * used to increase the capacity of the RefTree.
 *
 * If an ExpandedNodeId is encountered, it has to be processed right away.
 * Remote ExpandedNodeId are not put into the tree, since it is not possible to
 * recurse into them anyway.
 *
 * The layout of the results array is as follows:
 *
 * | Targets [NodeId] | Tree [RefEntry] | */

#define UA_BROWSE_INITIAL_SIZE 16

typedef struct RefEntry {
    ZIP_ENTRY(RefEntry) zipfields;
    const UA_NodeId *target;
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
    return (enum ZIP_CMP)UA_NodeId_order(aa->target, bb->target);
}

ZIP_HEAD(RefHead, RefEntry);
typedef struct RefHead RefHead;
ZIP_PROTTYPE(RefHead, RefEntry, RefEntry)
ZIP_IMPL(RefHead, RefEntry, zipfields, RefEntry, zipfields, cmpTarget)

typedef struct {
    UA_NodeId *targets;
    RefHead head;
    size_t capacity; /* available space */
    size_t size;     /* used space */
} RefTree;

static UA_StatusCode
RefTree_init(RefTree *rt) {
    size_t space = (sizeof(UA_NodeId) + sizeof(RefEntry)) * UA_BROWSE_INITIAL_SIZE;
    rt->targets = (UA_NodeId*)UA_malloc(space);
    if(!rt->targets)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    rt->capacity = UA_BROWSE_INITIAL_SIZE;
    rt->size = 0;
    ZIP_INIT(&rt->head);
    return UA_STATUSCODE_GOOD;
}

static
void RefTree_clear(RefTree *rt) {
    for(size_t i = 0; i < rt->size; i++)
        UA_NodeId_deleteMembers(&rt->targets[i]);
    UA_free(rt->targets);
}

/* Double the capacity of the reftree */
static UA_StatusCode
RefTree_double(RefTree *rt) {
    size_t capacity = rt->capacity * 2;
    UA_assert(capacity > 0);
    size_t space = (sizeof(UA_NodeId) + sizeof(RefEntry)) * capacity;
    UA_NodeId *newTargets = (UA_NodeId*)UA_realloc(rt->targets, space);
    if(!newTargets)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Repair the pointers for the realloced array+tree  */
    uintptr_t arraydiff = (uintptr_t)newTargets - (uintptr_t)rt->targets;
    RefEntry *reArray = (RefEntry*)
        ((uintptr_t)newTargets + (capacity * sizeof(UA_NodeId)));
    uintptr_t entrydiff = (uintptr_t)reArray -
        ((uintptr_t)rt->targets + (rt->capacity * sizeof(UA_NodeId)));
    RefEntry *oldReArray = (RefEntry*)
        ((uintptr_t)newTargets + (rt->capacity * sizeof(UA_NodeId)));
    memmove(reArray, oldReArray, rt->size * sizeof(RefEntry));
    for(size_t i = 0; i < rt->size; i++) {
        if(reArray[i].zipfields.zip_left)
            *(uintptr_t*)&reArray[i].zipfields.zip_left += entrydiff;
        if(reArray[i].zipfields.zip_right)
            *(uintptr_t*)&reArray[i].zipfields.zip_right += entrydiff;
        *(uintptr_t*)&reArray[i].target += arraydiff;
    }

    rt->head.zip_root = (RefEntry*)((uintptr_t)rt->head.zip_root + entrydiff);
    rt->capacity = capacity;
    rt->targets = newTargets;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
RefTree_add(RefTree *rt, const UA_NodeId *target) {
    UA_StatusCode s = UA_STATUSCODE_GOOD;
    if(rt->capacity <= rt->size) {
        s = RefTree_double(rt);
        if(s != UA_STATUSCODE_GOOD)
            return s;
    }
    s = UA_NodeId_copy(target, &rt->targets[rt->size]);
    if(s != UA_STATUSCODE_GOOD)
        return s;
    RefEntry *re = (RefEntry*)((uintptr_t)rt->targets +
                               (sizeof(UA_NodeId) * rt->capacity) +
                               (sizeof(RefEntry) * rt->size));
    re->target = &rt->targets[rt->size];
    re->targetHash = UA_NodeId_hash(target);
    ZIP_INSERT(RefHead, &rt->head, re, ZIP_FFS32(UA_UInt32_random()));
    rt->size++;
    return UA_STATUSCODE_GOOD;
}

/*********************/
/* ContinuationPoint */
/*********************/

struct ContinuationPoint {
    ContinuationPoint *next;
    UA_Guid identifier;
    UA_UInt32 maxReferences;
    UA_BrowseDescription bd;
    UA_Boolean recursive;

    RefTree rt;
    size_t n;    /* Index of the node in the rt that is currently visited */
    size_t nk;   /* Index of the ReferenceKind in the node that is visited */
    size_t nki;  /* Index of the reference in the ReferenceKind that is visited */
};

static UA_StatusCode
ContinuationPoint_init(ContinuationPoint *cp, UA_UInt32 maxRefs,
                       UA_Boolean recursive) {
    memset(cp, 0, sizeof(ContinuationPoint));
    cp->identifier = UA_Guid_random();
    cp->maxReferences = maxRefs;
    cp->recursive = recursive;
    return RefTree_init(&cp->rt);
}

ContinuationPoint *
ContinuationPoint_clear(ContinuationPoint *cp) {
    UA_BrowseDescription_clear(&cp->bd);
    RefTree_clear(&cp->rt);
    return cp->next;
}

/**********/
/* Browse */
/**********/

/* The RefTree structure is kept across calls to Browse(Next). The RefResult
 * structure is valid only for the current service call. Note that we can call
 * Browse internally without a RefResult. Then, only the RefTree is set up with
 * the target identifiers. */

typedef struct {
    size_t size;
    size_t capacity;
    UA_ReferenceDescription *descr;
} RefResult;

static UA_StatusCode
RefResult_init(RefResult *rr, UA_UInt32 maxRefs) {
    UA_UInt32 initialRes = UA_BROWSE_INITIAL_SIZE;
    if(initialRes > maxRefs)
        initialRes = maxRefs;
    memset(rr, 0, sizeof(RefResult));
    rr->descr = (UA_ReferenceDescription*)
        UA_Array_new(initialRes, &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION]);
    if(!rr->descr)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    rr->capacity = initialRes;
    rr->size = 0;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
RefResult_double(RefResult *rr, UA_UInt32 maxSize) {
    size_t newSize = rr->capacity * 2;
    if(newSize > maxSize)
        newSize = maxSize;
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

/* Target node on top of the stack */
static UA_StatusCode
fillReferenceDescription(UA_Server *server, const UA_NodeReferenceKind *ref, UA_UInt32 mask,
                         const UA_ExpandedNodeId *nodeId, const UA_Node *curr,
                         UA_ReferenceDescription *descr) {
    /* Fields without access to the actual node */
    UA_StatusCode retval = UA_ExpandedNodeId_copy(nodeId, &descr->nodeId);
    if(mask & UA_BROWSERESULTMASK_REFERENCETYPEID)
        retval |= UA_NodeId_copy(&ref->referenceTypeId, &descr->referenceTypeId);
    if(mask & UA_BROWSERESULTMASK_ISFORWARD)
        descr->isForward = !ref->isInverse;

    /* Remote references (ExpandedNodeId) are not further looked up here */
    if(!curr)
        return retval;
    
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
                UA_Nodestore_releaseNode(server->nsCtx, type);
            }
        }
    }
    return retval;
}

static UA_Boolean
relevantReference(UA_Server *server, UA_Boolean includeSubtypes,
                  const UA_NodeId *rootRef, const UA_NodeId *testRef) {
    if(!includeSubtypes)
        return UA_NodeId_equal(rootRef, testRef);

    const UA_NodeId hasSubType = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    return isNodeInTree(server->nsCtx, testRef, rootRef, &hasSubType, 1);
}

static UA_Boolean
matchClassMask(const UA_Node *node, UA_UInt32 nodeClassMask) {
    if(nodeClassMask != UA_NODECLASS_UNSPECIFIED &&
       (node->nodeClass & nodeClassMask) == 0)
        return false;
    return true;
}

static UA_StatusCode
browseNodeRefKind(UA_Server *server, UA_Session *session, ContinuationPoint *cp,
                  RefResult *rr, UA_Boolean *maxed, const UA_NodeReferenceKind *rk,
                  const UA_ExpandedNodeId *target) {
    /* Is the target already in the tree? */
    RefEntry re;
    re.target = &target->nodeId;
    re.targetHash = UA_NodeId_hash(&target->nodeId);
    if(ZIP_FIND(RefHead, &cp->rt.head, &re)) {
        cp->nki++;
        return UA_STATUSCODE_GOOD;
    }

    /* Get the target if it is not a remote node */
    const UA_Node *node = NULL;
    if(target->serverIndex == 0) {
        node = UA_Nodestore_getNode(server->nsCtx, &target->nodeId);
        if(node) {
            /* Test if the node class matches */
            UA_Boolean match = matchClassMask(node, cp->bd.nodeClassMask);
            if(!match) {
                UA_Nodestore_releaseNode(server->nsCtx, node);
                cp->nki++;
                return UA_STATUSCODE_GOOD;
            }
        }
    }

    /* A match! */

    /* Don't return detailed results if not required */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!rr)
        goto cleanup;

    /* Increase the capacity of the results array/tree if required */
    if(rr->capacity <= rr->size) {
        retval = RefResult_double(rr, cp->maxReferences);
        /* Reached MaxReferences; Redo for this target */
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        if(rr->size == rr->capacity) {
            *maxed = true;
            goto cleanup;
        }
    }

    /* Fill the detailed results */
    retval = fillReferenceDescription(server, rk, cp->bd.resultMask, target,
                                      node, &rr->descr[rr->size]);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    rr->size++;

 cleanup:
    if(!(*maxed) && retval == UA_STATUSCODE_GOOD) {
        /* Proceed to the next target */
        if(node)
            retval = RefTree_add(&cp->rt, &target->nodeId);
        cp->nki++;
    }
    if(node)
        UA_Nodestore_releaseNode(server->nsCtx, node);
    return retval;
}

static UA_StatusCode
browseNode(UA_Server *server, UA_Session *session,
           ContinuationPoint *cp, RefResult *rr, UA_Boolean *maxed,
           size_t referenceTypesSize, const UA_NodeId *referenceTypes,
           const UA_Node *node) {
    for(; cp->nk < node->referencesSize; cp->nk++, cp->nki = 0) {
        UA_NodeReferenceKind *rk = &node->references[cp->nk];

        /* Reference in the right direction? */
        if(rk->isInverse && cp->bd.browseDirection == UA_BROWSEDIRECTION_FORWARD)
            continue;
        if(!rk->isInverse && cp->bd.browseDirection == UA_BROWSEDIRECTION_INVERSE)
            continue;

        /* Is the reference part of the hierarchy of references we look for? */
        UA_Boolean relevant = (referenceTypes == NULL);
        for(size_t i = 0; i < referenceTypesSize && !relevant; i++)
            relevant = UA_NodeId_equal(&rk->referenceTypeId, &referenceTypes[i]);
        if(!relevant)
            continue;

        /* cp->nki is increaed in browseNodeRefKind upon success */
        for(; cp->nki < rk->targetIdsSize; ) {
            UA_StatusCode retval = browseNodeRefKind(server, session, cp, rr, maxed,
                                                     rk, &rk->targetIds[cp->nki]);
            if(*maxed || retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }

    return UA_STATUSCODE_GOOD;
}

/* Results for a single browsedescription. This is the inner loop for both
 * Browse and BrowseNext. The ContinuationPoint contains all the data used.
 * Including the BrowseDescription. Returns whether there are remaining
 * references. */
/* Results for a single browsedescription. Sets all NodeIds for the RefTree. */
static UA_StatusCode
browseWithCp(UA_Server *server, UA_Session *session, ContinuationPoint *cp,
             RefResult *rr, UA_Boolean *maxed) {
    /* Is the browsedirection valid? */
    if(cp->bd.browseDirection != UA_BROWSEDIRECTION_BOTH &&
       cp->bd.browseDirection != UA_BROWSEDIRECTION_FORWARD &&
       cp->bd.browseDirection != UA_BROWSEDIRECTION_INVERSE) {
        return UA_STATUSCODE_BADBROWSEDIRECTIONINVALID;
    }

    /* Set the list of references to check */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId *referenceTypes = NULL; /* NULL -> all reference types are relevant */
    size_t referenceTypesSize = 0;
    if(!UA_NodeId_isNull(&cp->bd.referenceTypeId)) {
        const UA_Node *reftype = UA_Nodestore_getNode(server->nsCtx, &cp->bd.referenceTypeId);
        if(!reftype)
            return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;

        UA_Boolean isRef = (reftype->nodeClass == UA_NODECLASS_REFERENCETYPE);
        UA_Nodestore_releaseNode(server->nsCtx, reftype);

        if(!isRef)
            return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;

        if(!UA_NodeId_isNull(&cp->bd.referenceTypeId)) {
            if(cp->bd.includeSubtypes) {
                retval = getLocalRecursiveHierarchy(server, &cp->bd.referenceTypeId, 1, &subtypeId, 1,
                                                    true, &referenceTypes, &referenceTypesSize);
                if(retval != UA_STATUSCODE_GOOD)
                    return retval;
                UA_assert(referenceTypesSize > 0); /* The original referenceTypeId has been mirrored back */
            } else {
                referenceTypes = &cp->bd.referenceTypeId;
                referenceTypesSize = 1;
            }
        }
    }

    for(; cp->n < cp->rt.size; cp->n++, cp->nk = 0, cp->nki = 0) {
        /* Get the node */
        const UA_Node *node = UA_Nodestore_getNode(server->nsCtx, &cp->rt.targets[cp->n]);
        if(!node)
            continue;

        /* Browse the references in the current node */
        retval = browseNode(server, session, cp, rr, maxed, referenceTypesSize, referenceTypes, node);
        UA_Nodestore_releaseNode(server->nsCtx, node);
        if(retval != UA_STATUSCODE_GOOD)
            break;
        if(!cp->recursive || *maxed)
            break;
    }

    if(referenceTypes != &cp->bd.referenceTypeId)
        UA_Array_delete(referenceTypes, referenceTypesSize, &UA_TYPES[UA_TYPES_NODEID]);

    return retval;
}

/* Start to browse with no previous cp */
void
Operation_Browse(UA_Server *server, UA_Session *session, const struct BrowseOpts *bo,
                 const UA_BrowseDescription *descr, UA_BrowseResult *result) {
    /* How many references can we return at most? */
    UA_UInt32 maxRefs = bo->maxReferences;
    if(maxRefs == 0) {
        if(server->config.maxReferencesPerNode != 0) {
            maxRefs = server->config.maxReferencesPerNode;
        } else {
            maxRefs = UA_INT32_MAX;
        }
    } else {
        if(server->config.maxReferencesPerNode != 0 &&
           maxRefs > server->config.maxReferencesPerNode) {
            maxRefs = server->config.maxReferencesPerNode;
        }
    }

    /* Create the results array */
    RefResult rr;
    result->statusCode = RefResult_init(&rr, maxRefs);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    ContinuationPoint cp;
    result->statusCode = ContinuationPoint_init(&cp, maxRefs, bo->recursive);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        RefResult_clear(&rr);
        return;
    }
    cp.bd = *descr; /* Deep-copy only when the cp is persisted in the session */

    /* Add the initial node to the RefTree */
    result->statusCode = RefTree_add(&cp.rt, &descr->nodeId);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        RefTree_clear(&cp.rt);
        RefResult_clear(&rr);
        return;
    }
    
    /* Recurse to get all references */
    UA_Boolean maxed = false;
    result->statusCode = browseWithCp(server, session, &cp, &rr, &maxed);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        RefTree_clear(&cp.rt);
        RefResult_clear(&rr);
        return;
    }

    /* No results */
    if(rr.size == 0) {
        result->references = (UA_ReferenceDescription*)UA_EMPTY_ARRAY_SENTINEL;
        RefTree_clear(&cp.rt);
        UA_free(rr.descr);
        return;
    }

    /* Move results to the output */
    result->references = rr.descr;
    result->referencesSize = rr.size;

    /* Nothing left for BrowseNext */
    if(!maxed) {
        RefTree_clear(&cp.rt);
        return;
    }

    /* Create a new continuation point. */
    ContinuationPoint *newCp = (ContinuationPoint*)UA_malloc(sizeof(ContinuationPoint));
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ByteString tmp;
    if(!newCp) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    *newCp = cp;

    /* Make a deep copy of the BrowseDescription */
    retval = UA_BrowseDescription_copy(descr, &newCp->bd);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Return the cp identifier */
    tmp.length = sizeof(UA_Guid);
    tmp.data = (UA_Byte*)&newCp->identifier;
    retval = UA_ByteString_copy(&tmp, &result->continuationPoint);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Remove the oldest continuation point if required */
    if(session->availableContinuationPoints <= 0) {
        struct ContinuationPoint **prev = &session->continuationPoints;
        struct ContinuationPoint *cp2 = session->continuationPoints;
        while(cp2 && cp2->next) {
            prev = &cp2->next;
            cp2 = cp2->next;
        }
        if(cp2) {
            *prev = NULL;
            ContinuationPoint_clear(cp2);
            UA_free(cp2);
            ++session->availableContinuationPoints;
        }
    }

    /* Attach the cp to the session */
    newCp->next = session->continuationPoints;
    session->continuationPoints = newCp;
    --session->availableContinuationPoints;
    return;

 cleanup:
    UA_BrowseResult_deleteMembers(result); /* Holds the content that was in rr before */
    if(newCp) {
        ContinuationPoint_clear(newCp);
        UA_free(newCp);
    }
    result->statusCode = retval;
}

void Service_Browse(UA_Server *server, UA_Session *session,
                    const UA_BrowseRequest *request, UA_BrowseResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing BrowseRequest");

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

    struct BrowseOpts bo;
    bo.maxReferences = request->requestedMaxReferencesPerNode;
    bo.recursive = false;
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_Browse, &bo,
                                           &request->nodesToBrowseSize, &UA_TYPES[UA_TYPES_BROWSEDESCRIPTION],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_BROWSERESULT]);
}

UA_BrowseResult
UA_Server_browse(UA_Server *server, UA_UInt32 maxReferences,
                 const UA_BrowseDescription *bd) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    struct BrowseOpts bo;
    bo.maxReferences = maxReferences;
    bo.recursive = false;
    Operation_Browse(server, &server->adminSession, &bo, bd, &result);
    return result;
}

UA_BrowseResult
UA_Server_browseRecursive(UA_Server *server, UA_UInt32 maxReferences,
                          const UA_BrowseDescription *bd) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    struct BrowseOpts bo;
    bo.maxReferences = maxReferences;
    bo.recursive = true;
    Operation_Browse(server, &server->adminSession, &bo, bd, &result);
    return result;
}

static void
Operation_BrowseNext(UA_Server *server, UA_Session *session,
                     const UA_Boolean *releaseContinuationPoints,
                     const UA_ByteString *continuationPoint, UA_BrowseResult *result) {
    /* Find the continuation point */
    ContinuationPoint **prev = &session->continuationPoints, *cp;
    UA_ByteString identifier;
    while((cp = *prev)) {
        identifier.length = sizeof(UA_Guid);
        identifier.data = (UA_Byte*)&cp->identifier;
        if(UA_ByteString_equal(&identifier, continuationPoint))
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

    /* Allocate the results array */
    RefResult rr;
    result->statusCode = RefResult_init(&rr, cp->maxReferences);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;
    
    /* Recurse to get all references */
    UA_Boolean maxed = false;
    result->statusCode = browseWithCp(server, session, cp, &rr, &maxed);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        goto removeCp;

    /* No results */
    if(rr.size == 0) {
        result->references = (UA_ReferenceDescription*)UA_EMPTY_ARRAY_SENTINEL;
        goto removeCp;
    }

    if(maxed) {
        /* Keep the cp */
        result->statusCode = UA_ByteString_copy(&identifier, &result->continuationPoint);
        if(result->statusCode != UA_STATUSCODE_GOOD)
            goto removeCp;
    } else {
        /* All done, remove the cp */
        *prev = ContinuationPoint_clear(cp);
        UA_free(cp);
        ++session->availableContinuationPoints;
    }

    /* Move results to the output */
    result->references = rr.descr;
    result->referencesSize = rr.size;
    return;

 removeCp:
    *prev = cp->next;
    ContinuationPoint_clear(cp);
    UA_free(cp);
    session->availableContinuationPoints++;
    RefResult_clear(&rr);
}

void
Service_BrowseNext(UA_Server *server, UA_Session *session,
                   const UA_BrowseNextRequest *request,
                   UA_BrowseNextResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing BrowseNextRequest");
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
    Operation_BrowseNext(server, &server->adminSession, &releaseContinuationPoint,
                         continuationPoint, &result);
    return result;
}

UA_StatusCode
getLocalRecursiveHierarchy(UA_Server *server, const UA_NodeId *startNodes, size_t startNodesSize,
                           const UA_NodeId *refTypes, size_t refTypesSize, UA_Boolean walkDownwards,
                           UA_NodeId **results, size_t *resultsSize) {
    ContinuationPoint cp;
    UA_StatusCode retval = ContinuationPoint_init(&cp, 0, true);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    for(size_t i = 0; i < startNodesSize; i++)
        retval |= RefTree_add(&cp.rt, &startNodes[i]);
    if(retval != UA_STATUSCODE_GOOD) {
        ContinuationPoint_clear(&cp);
        return retval;
    }

    if(!walkDownwards)
        cp.bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;

    UA_Boolean maxed = false;
    for(; cp.n < cp.rt.size; cp.n++, cp.nk = 0, cp.nki = 0) {
        /* Get the node */
        const UA_Node *node = UA_Nodestore_getNode(server->nsCtx, &cp.rt.targets[cp.n]);
        if(!node)
            continue;

        /* Browse the references in the current node */
        retval = browseNode(server, &server->adminSession, &cp, NULL,
                            &maxed, refTypesSize, refTypes, node);
        UA_Nodestore_releaseNode(server->nsCtx, node);
        if(retval != UA_STATUSCODE_GOOD)
            break;
    }
    
    if(retval == UA_STATUSCODE_GOOD) {
        *results = cp.rt.targets;
        *resultsSize = cp.rt.size;
        cp.rt.targets = NULL;
        cp.rt.size = 0;
    }
    ContinuationPoint_clear(&cp);
    return retval;
}

/***********************/
/* TranslateBrowsePath */
/***********************/

static void
walkBrowsePathElementReferenceTargets(UA_BrowsePathResult *result, size_t *targetsSize,
                                      UA_NodeId **next, size_t *nextSize, size_t *nextCount,
                                      UA_UInt32 elemDepth, const UA_NodeReferenceKind *rk) {
    /* Loop over the targets */
    for(size_t i = 0; i < rk->targetIdsSize; i++) {
        UA_ExpandedNodeId *targetId = &rk->targetIds[i];

        /* Does the reference point to an external server? Then add to the
         * targets with the right path depth. */
        if(targetId->serverIndex != 0) {
            UA_BrowsePathTarget *tempTargets =
                (UA_BrowsePathTarget*)UA_realloc(result->targets,
                             sizeof(UA_BrowsePathTarget) * (*targetsSize) * 2);
            if(!tempTargets) {
                result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
                return;
            }
            result->targets = tempTargets;
            (*targetsSize) *= 2;
            result->statusCode = UA_ExpandedNodeId_copy(targetId,
                       &result->targets[result->targetsSize].targetId);
            result->targets[result->targetsSize].remainingPathIndex = elemDepth;
            continue;
        }

        /* Can we store the node in the array of candidates for deep-search? */
        if(*nextSize <= *nextCount) {
            UA_NodeId *tempNext =
                (UA_NodeId*)UA_realloc(*next, sizeof(UA_NodeId) * (*nextSize) * 2);
            if(!tempNext) {
                result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
                return;
            }
            *next = tempNext;
            (*nextSize) *= 2;
        }

        /* Add the node to the next array for the following path element */
        result->statusCode = UA_NodeId_copy(&targetId->nodeId,
                                            &(*next)[*nextCount]);
        if(result->statusCode != UA_STATUSCODE_GOOD)
            return;
        ++(*nextCount);
    }
}

static void
walkBrowsePathElement(UA_Server *server, UA_Session *session, UA_UInt32 nodeClassMask,
                      UA_BrowsePathResult *result, size_t *targetsSize,
                      const UA_RelativePathElement *elem, UA_UInt32 elemDepth,
                      const UA_QualifiedName *targetName,
                      const UA_NodeId *current, const size_t currentCount,
                      UA_NodeId **next, size_t *nextSize, size_t *nextCount) {
    /* Return all references? */
    UA_Boolean all_refs = UA_NodeId_isNull(&elem->referenceTypeId);
    if(!all_refs) {
        const UA_Node *rootRef = UA_Nodestore_getNode(server->nsCtx, &elem->referenceTypeId);
        if(!rootRef)
            return;
        UA_Boolean match = (rootRef->nodeClass == UA_NODECLASS_REFERENCETYPE);
        UA_Nodestore_releaseNode(server->nsCtx, rootRef);
        if(!match)
            return;
    }

    /* Iterate over all nodes at the current depth-level */
    for(size_t i = 0; i < currentCount; ++i) {
        /* Get the node */
        const UA_Node *node = UA_Nodestore_getNode(server->nsCtx, &current[i]);
        if(!node) {
            /* If we cannot find the node at depth 0, the starting node does not exist */
            if(elemDepth == 0)
                result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
            continue;
        }

        /* Test whether the node fits the class mask */
        if(!matchClassMask(node, nodeClassMask)) {
            UA_Nodestore_releaseNode(server->nsCtx, node);
            continue;
        }

        /* Test whether the node has the target name required in the previous
         * path element */
        if(targetName && (targetName->namespaceIndex != node->browseName.namespaceIndex ||
                          !UA_String_equal(&targetName->name, &node->browseName.name))) {
            UA_Nodestore_releaseNode(server->nsCtx, node);
            continue;
        }

        /* Loop over the nodes references */
        for(size_t r = 0; r < node->referencesSize &&
                result->statusCode == UA_STATUSCODE_GOOD; ++r) {
            UA_NodeReferenceKind *rk = &node->references[r];

            /* Does the direction of the reference match? */
            if(rk->isInverse != elem->isInverse)
                continue;

            /* Is the node relevant? */
            if(!all_refs && !relevantReference(server, elem->includeSubtypes,
                                               &elem->referenceTypeId, &rk->referenceTypeId))
                continue;

            /* Walk over the reference targets */
            walkBrowsePathElementReferenceTargets(result, targetsSize, next, nextSize,
                                                  nextCount, elemDepth, rk);
        }

        UA_Nodestore_releaseNode(server->nsCtx, node);
    }
}

/* This assumes that result->targets has enough room for all currentCount elements */
static void
addBrowsePathTargets(UA_Server *server, UA_Session *session, UA_UInt32 nodeClassMask,
                     UA_BrowsePathResult *result, const UA_QualifiedName *targetName,
                     UA_NodeId *current, size_t currentCount) {
    for(size_t i = 0; i < currentCount; i++) {
        const UA_Node *node = UA_Nodestore_getNode(server->nsCtx, &current[i]);
        if(!node) {
            UA_NodeId_deleteMembers(&current[i]);
            continue;
        }

        /* Test whether the node fits the class mask */
        UA_Boolean skip = !matchClassMask(node, nodeClassMask);

        /* Test whether the node has the target name required in the
         * previous path element */
        if(targetName->namespaceIndex != node->browseName.namespaceIndex ||
           !UA_String_equal(&targetName->name, &node->browseName.name))
            skip = true;

        UA_Nodestore_releaseNode(server->nsCtx, node);

        if(skip) {
            UA_NodeId_deleteMembers(&current[i]);
            continue;
        }

        /* Move the nodeid to the target array */
        UA_BrowsePathTarget_init(&result->targets[result->targetsSize]);
        result->targets[result->targetsSize].targetId.nodeId = current[i];
        result->targets[result->targetsSize].remainingPathIndex = UA_UINT32_MAX;
        ++result->targetsSize;
    }
}

static void
walkBrowsePath(UA_Server *server, UA_Session *session, const UA_BrowsePath *path,
               UA_UInt32 nodeClassMask, UA_BrowsePathResult *result, size_t targetsSize,
               UA_NodeId **current, size_t *currentSize, size_t *currentCount,
               UA_NodeId **next, size_t *nextSize, size_t *nextCount) {
    UA_assert(*currentCount == 1);
    UA_assert(*nextCount == 0);

    /* Points to the targetName of the _previous_ path element */
    const UA_QualifiedName *targetName = NULL;

    /* Iterate over path elements */
    UA_assert(path->relativePath.elementsSize > 0);
    for(UA_UInt32 i = 0; i < path->relativePath.elementsSize; ++i) {
        walkBrowsePathElement(server, session, nodeClassMask, result, &targetsSize,
                              &path->relativePath.elements[i], i, targetName,
                              *current, *currentCount, next, nextSize, nextCount);

        /* Clean members of current */
        for(size_t j = 0; j < *currentCount; j++)
            UA_NodeId_deleteMembers(&(*current)[j]);
        *currentCount = 0;

        /* When no targets are left or an error occurred. None of next's
         * elements will be copied to result->targets */
        if(*nextCount == 0 || result->statusCode != UA_STATUSCODE_GOOD) {
            UA_assert(*currentCount == 0);
            UA_assert(*nextCount == 0);
            return;
        }

        /* Exchange current and next for the next depth */
        size_t tSize = *currentSize; size_t tCount = *currentCount; UA_NodeId *tT = *current;
        *currentSize = *nextSize; *currentCount = *nextCount; *current = *next;
        *nextSize = tSize; *nextCount = tCount; *next = tT;

        /* Store the target name of the previous path element */
        targetName = &path->relativePath.elements[i].targetName;
    }

    UA_assert(targetName != NULL);
    UA_assert(*nextCount == 0);

    /* After the last BrowsePathElement, move members from current to the
     * result targets */

    /* Realloc if more space is needed */
    if(targetsSize < result->targetsSize + (*currentCount)) {
        UA_BrowsePathTarget *newTargets =
            (UA_BrowsePathTarget*)UA_realloc(result->targets, sizeof(UA_BrowsePathTarget) *
                                             (result->targetsSize + (*currentCount)));
        if(!newTargets) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            for(size_t i = 0; i < *currentCount; ++i)
                UA_NodeId_deleteMembers(&(*current)[i]);
            *currentCount = 0;
            return;
        }
        result->targets = newTargets;
    }

    /* Move the elements of current to the targets */
    addBrowsePathTargets(server, session, nodeClassMask, result, targetName, *current, *currentCount);
    *currentCount = 0;
}

static void
Operation_TranslateBrowsePathToNodeIds(UA_Server *server, UA_Session *session,
                                       const UA_UInt32 *nodeClassMask, const UA_BrowsePath *path,
                                       UA_BrowsePathResult *result) {
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

    /* Allocate memory for the targets */
    size_t targetsSize = 10; /* When to realloc; the member count is stored in
                              * result->targetsSize */
    result->targets =
        (UA_BrowsePathTarget*)UA_malloc(sizeof(UA_BrowsePathTarget) * targetsSize);
    if(!result->targets) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* Allocate memory for two temporary arrays. One with the results for the
     * previous depth of the path. The other for the new results at the current
     * depth. The two arrays alternate as we descend down the tree. */
    size_t currentSize = 10; /* When to realloc */
    size_t currentCount = 0; /* Current elements */
    UA_NodeId *current = (UA_NodeId*)UA_malloc(sizeof(UA_NodeId) * currentSize);
    if(!current) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_free(result->targets);
        return;
    }
    size_t nextSize = 10; /* When to realloc */
    size_t nextCount = 0; /* Current elements */
    UA_NodeId *next = (UA_NodeId*)UA_malloc(sizeof(UA_NodeId) * nextSize);
    if(!next) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_free(result->targets);
        UA_free(current);
        return;
    }

    /* Copy the starting node into current */
    result->statusCode = UA_NodeId_copy(&path->startingNode, &current[0]);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_free(result->targets);
        UA_free(current);
        UA_free(next);
        return;
    }
    currentCount = 1;

    /* Walk the path elements */
    walkBrowsePath(server, session, path, *nodeClassMask, result, targetsSize,
                   &current, &currentSize, &currentCount,
                   &next, &nextSize, &nextCount);

    UA_assert(currentCount == 0);
    UA_assert(nextCount == 0);

    /* No results => BadNoMatch status code */
    if(result->targetsSize == 0 && result->statusCode == UA_STATUSCODE_GOOD)
        result->statusCode = UA_STATUSCODE_BADNOMATCH;

    /* Clean up the temporary arrays and the targets */
    UA_free(current);
    UA_free(next);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < result->targetsSize; ++i)
            UA_BrowsePathTarget_deleteMembers(&result->targets[i]);
        UA_free(result->targets);
        result->targets = NULL;
        result->targetsSize = 0;
    }
}

UA_BrowsePathResult
UA_Server_translateBrowsePathToNodeIds(UA_Server *server,
                                       const UA_BrowsePath *browsePath) {
    UA_BrowsePathResult result;
    UA_BrowsePathResult_init(&result);
    UA_UInt32 nodeClassMask = 0; /* All node classes */
    Operation_TranslateBrowsePathToNodeIds(server, &server->adminSession, &nodeClassMask,
                                           browsePath, &result);
    return result;
}

void
Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
                                      const UA_TranslateBrowsePathsToNodeIdsRequest *request,
                                      UA_TranslateBrowsePathsToNodeIdsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing TranslateBrowsePathsToNodeIdsRequest");

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
UA_Server_browseSimplifiedBrowsePath(UA_Server *server, const UA_NodeId origin,
                                     size_t browsePathSize, const UA_QualifiedName *browsePath) {
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
    Operation_TranslateBrowsePathToNodeIds(server, &server->adminSession, &nodeClassMask, &bp, &bpr);
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
