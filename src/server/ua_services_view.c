/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
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

/**********/
/* Browse */
/**********/

/* Target node on top of the stack */
static UA_StatusCode
fillReferenceDescription(UA_Server *server, const UA_Node *curr,
                         const UA_NodeReferenceKind *ref,
                         UA_UInt32 mask, UA_ReferenceDescription *descr) {
    UA_ReferenceDescription_init(descr);
    UA_StatusCode retval = UA_NodeId_copy(&curr->nodeId, &descr->nodeId.nodeId);
    if(mask & UA_BROWSERESULTMASK_REFERENCETYPEID)
        retval |= UA_NodeId_copy(&ref->referenceTypeId, &descr->referenceTypeId);
    if(mask & UA_BROWSERESULTMASK_ISFORWARD)
        descr->isForward = !ref->isInverse;
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

static void
removeCp(ContinuationPointEntry *cp, UA_Session* session) {
    LIST_REMOVE(cp, pointers);
    UA_ByteString_deleteMembers(&cp->identifier);
    UA_BrowseDescription_deleteMembers(&cp->browseDescription);
    UA_free(cp);
    ++session->availableContinuationPoints;
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

/* Returns whether the node / continuationpoint is done */
static UA_Boolean
browseReferences(UA_Server *server, const UA_Node *node,
                 ContinuationPointEntry *cp, UA_BrowseResult *result) {
    UA_assert(cp != NULL);
    const UA_BrowseDescription *descr = &cp->browseDescription;

    /* If the node has no references, just return */
    if(node->referencesSize == 0) {
        result->referencesSize = 0;
        return true;
    }

    /* Follow all references? */
    UA_Boolean browseAll = UA_NodeId_isNull(&descr->referenceTypeId);

    /* How many references can we return at most? */
    size_t maxrefs = cp->maxReferences;
    if(maxrefs == 0) {
        if(server->config.maxReferencesPerNode != 0) {
            maxrefs = server->config.maxReferencesPerNode;
        } else {
            maxrefs = UA_INT32_MAX;
        }
    } else {
        if(server->config.maxReferencesPerNode != 0 && maxrefs > server->config.maxReferencesPerNode) {
            maxrefs = server->config.maxReferencesPerNode;
        }
    }

    /* Allocate the results array */
    size_t refs_size = 2; /* True size of the array */
    result->references = (UA_ReferenceDescription*)
        UA_Array_new(refs_size, &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION]);
    if(!result->references) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return false;
    }

    size_t referenceKindIndex = cp->referenceKindIndex;
    size_t targetIndex = cp->targetIndex;

    /* Loop over the node's references */
    for(; referenceKindIndex < node->referencesSize; ++referenceKindIndex) {
        UA_NodeReferenceKind *rk = &node->references[referenceKindIndex];

        /* Reference in the right direction? */
        if(rk->isInverse && descr->browseDirection == UA_BROWSEDIRECTION_FORWARD)
            continue;
        if(!rk->isInverse && descr->browseDirection == UA_BROWSEDIRECTION_INVERSE)
            continue;

        /* Is the reference part of the hierarchy of references we look for? */
        if(!browseAll && !relevantReference(server, descr->includeSubtypes,
                                            &descr->referenceTypeId, &rk->referenceTypeId))
            continue;

        /* Loop over the targets */
        for(; targetIndex < rk->targetIdsSize; ++targetIndex) {
            /* Get the node */
            const UA_Node *target =
                UA_Nodestore_getNode(server->nsCtx, &rk->targetIds[targetIndex].nodeId);
            if(!target)
                continue;

            /* Test if the node class matches */
            if(!matchClassMask(target, descr->nodeClassMask)) {
                UA_Nodestore_releaseNode(server->nsCtx, target);
                continue;
            }

            /* A match! Can we return it? */
            if(result->referencesSize >= maxrefs) {
                /* There are references we could not return */
                cp->referenceKindIndex = referenceKindIndex;
                cp->targetIndex = targetIndex;
                UA_Nodestore_releaseNode(server->nsCtx, target);
                return false;
            }

            /* Make enough space in the array */
            if(result->referencesSize >= refs_size) {
                refs_size *= 2;
                if(refs_size > maxrefs)
                    refs_size = maxrefs;
                UA_ReferenceDescription *rd = (UA_ReferenceDescription*)
                    UA_realloc(result->references, sizeof(UA_ReferenceDescription) * refs_size);
                if(!rd) {
                    result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
                    UA_Nodestore_releaseNode(server->nsCtx, target);
                    goto error_recovery;
                }
                result->references = rd;
            }

            /* Copy the node description. Target is on top of the stack */
            result->statusCode =
                fillReferenceDescription(server, target, rk, descr->resultMask,
                                         &result->references[result->referencesSize]);

            UA_Nodestore_releaseNode(server->nsCtx, target);

            if(result->statusCode != UA_STATUSCODE_GOOD)
                goto error_recovery;

            /* Increase the counter */
            result->referencesSize++;
        }

        targetIndex = 0; /* Start at index 0 for the next reference kind */
    }

    /* No relevant references, return array of length zero */
    if(result->referencesSize == 0) {
        UA_free(result->references);
        result->references = (UA_ReferenceDescription*)UA_EMPTY_ARRAY_SENTINEL;
    }

    /* The node is done */
    return true;

 error_recovery:
    if(result->referencesSize == 0)
        UA_free(result->references);
    else
        UA_Array_delete(result->references, result->referencesSize,
                        &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION]);
    result->references = NULL;
    result->referencesSize = 0;
    return false;
}

/* Results for a single browsedescription. This is the inner loop for both
 * Browse and BrowseNext. The ContinuationPoint contains all the data used.
 * Including the BrowseDescription. Returns whether there are remaining
 * references. */
static UA_Boolean
browseWithContinuation(UA_Server *server, UA_Session *session,
                       ContinuationPointEntry *cp, UA_BrowseResult *result) {
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
        const UA_Node *reftype = UA_Nodestore_getNode(server->nsCtx, &descr->referenceTypeId);
        if(!reftype) {
            result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
            return true;
        }

        UA_Boolean isRef = (reftype->nodeClass == UA_NODECLASS_REFERENCETYPE);
        UA_Nodestore_releaseNode(server->nsCtx, reftype);

        if(!isRef) {
            result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
            return true;
        }
    }

    const UA_Node *node = UA_Nodestore_getNode(server->nsCtx, &descr->nodeId);
    if(!node) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return true;
    }

    /* Browse the references */
    UA_Boolean done = browseReferences(server, node, cp, result);
    UA_Nodestore_releaseNode(server->nsCtx, node);
    return done;
}

/* Start to browse with no previous cp */
void
Operation_Browse(UA_Server *server, UA_Session *session, const UA_UInt32 *maxrefs,
                 const UA_BrowseDescription *descr, UA_BrowseResult *result) {
    /* Stack-allocate a temporary cp */
    UA_STACKARRAY(ContinuationPointEntry, cp, 1);
    memset(cp, 0, sizeof(ContinuationPointEntry));
    cp->maxReferences = *maxrefs;
    cp->browseDescription = *descr; /* Shallow copy. Deep-copy later if we persist the cp. */

    UA_Boolean done = browseWithContinuation(server, session, cp, result);

    /* Exit early if done or an error occurred */
    if(done || result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Persist the new continuation point */
    ContinuationPointEntry *cp2 = NULL;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(session->availableContinuationPoints <= 0 ||
       !(cp2 = (ContinuationPointEntry *)UA_malloc(sizeof(ContinuationPointEntry)))) {
        retval = UA_STATUSCODE_BADNOCONTINUATIONPOINTS;
        goto cleanup;
    }
    memset(cp2, 0, sizeof(ContinuationPointEntry));
    cp2->referenceKindIndex = cp->referenceKindIndex;
    cp2->targetIndex = cp->targetIndex;
    cp2->maxReferences = cp->maxReferences;
    retval = UA_BrowseDescription_copy(descr, &cp2->browseDescription);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Create a random bytestring via a Guid */
    UA_Guid *ident = UA_Guid_new();
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
    LIST_INSERT_HEAD(&session->continuationPoints, cp2, pointers);
    --session->availableContinuationPoints;
    return;

 cleanup:
    if(cp2) {
        UA_ByteString_deleteMembers(&cp2->identifier);
        UA_BrowseDescription_deleteMembers(&cp2->browseDescription);
        UA_free(cp2);
    }
    UA_BrowseResult_deleteMembers(result);
    result->statusCode = retval;
}

void Service_Browse(UA_Server *server, UA_Session *session,
                    const UA_BrowseRequest *request, UA_BrowseResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing BrowseRequest");

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

    UA_UInt32 requestedMaxReferencesPerNode = request->requestedMaxReferencesPerNode;
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_Browse,
                                           &requestedMaxReferencesPerNode,
                                           &request->nodesToBrowseSize, &UA_TYPES[UA_TYPES_BROWSEDESCRIPTION],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_BROWSERESULT]);
}

UA_BrowseResult
UA_Server_browse(UA_Server *server, UA_UInt32 maxrefs, const UA_BrowseDescription *descr) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    Operation_Browse(server, &server->adminSession, &maxrefs, descr, &result);
    return result;
}

static void
Operation_BrowseNext(UA_Server *server, UA_Session *session, const UA_Boolean *releaseContinuationPoints,
                     const UA_ByteString *continuationPoint, UA_BrowseResult *result) {
    /* Find the continuation point */
    ContinuationPointEntry *cp;
    LIST_FOREACH(cp, &session->continuationPoints, pointers) {
        if(UA_ByteString_equal(&cp->identifier, continuationPoint))
            break;
    }
    if(!cp) {
        result->statusCode = UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
        return;
    }

    /* Remove the cp */
    if(*releaseContinuationPoints) {
        removeCp(cp, session);
        return;
    }

    /* Continue browsing */
    UA_Boolean done = browseWithContinuation(server, session, cp, result);

    if(done) {
        /* Remove the cp if there are no references left */
        removeCp(cp, session);
    } else {
        /* Return the cp identifier */
        UA_StatusCode retval = UA_ByteString_copy(&cp->identifier, &result->continuationPoint);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_BrowseResult_deleteMembers(result);
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

    if(server->config.maxNodesPerRegisterNodes != 0 &&
       request->nodesToUnregisterSize > server->config.maxNodesPerRegisterNodes) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }
}
