/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include "ua_server_internal.h"
#include "ua_services.h"

static UA_StatusCode
fillReferenceDescription(UA_NodeStore *ns, const UA_Node *curr, UA_ReferenceNode *ref,
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
    if(mask & UA_BROWSERESULTMASK_TYPEDEFINITION){
        if(curr->nodeClass == UA_NODECLASS_OBJECT || curr->nodeClass == UA_NODECLASS_VARIABLE) {
            for(size_t i = 0; i < curr->referencesSize; ++i) {
                UA_ReferenceNode *refnode = &curr->references[i];
                if(refnode->referenceTypeId.identifier.numeric == UA_NS0ID_HASTYPEDEFINITION) {
                    retval |= UA_ExpandedNodeId_copy(&refnode->targetId, &descr->typeDefinition);
                    break;
                }
            }
        }
    }
    return retval;
}


/* Tests if the node is relevant to the browse request and shall be returned. If
   so, it is retrieved from the Nodestore. If not, null is returned. */
static const UA_Node *
returnRelevantNode(UA_Server *server, const UA_BrowseDescription *descr, UA_Boolean return_all,
                   const UA_ReferenceNode *reference, const UA_NodeId *relevant, size_t relevant_count,
                   UA_Boolean *isExternal) {
    /* reference in the right direction? */
    if(reference->isInverse && descr->browseDirection == UA_BROWSEDIRECTION_FORWARD)
        return NULL;
    if(!reference->isInverse && descr->browseDirection == UA_BROWSEDIRECTION_INVERSE)
        return NULL;

    /* is the reference part of the hierarchy of references we look for? */
    if(!return_all) {
        UA_Boolean is_relevant = false;
        for(size_t i = 0; i < relevant_count; ++i) {
            if(UA_NodeId_equal(&reference->referenceTypeId, &relevant[i])) {
                is_relevant = true;
                break;
            }
        }
        if(!is_relevant)
            return NULL;
    }


    /* return from the internal nodestore */
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &reference->targetId.nodeId);
    if(node && descr->nodeClassMask != 0 && (node->nodeClass & descr->nodeClassMask) == 0)
        return NULL;
    *isExternal = false;
    return node;
}

static void removeCp(struct ContinuationPointEntry *cp, UA_Session* session) {
    LIST_REMOVE(cp, pointers);
    UA_ByteString_deleteMembers(&cp->identifier);
    UA_BrowseDescription_deleteMembers(&cp->browseDescription);
    UA_free(cp);
    ++session->availableContinuationPoints;
}

/* Results for a single browsedescription. This is the inner loop for both
 * Browse and BrowseNext
 *
 * @param session Session to save continuationpoints
 * @param ns The nodstore where the to-be-browsed node can be found
 * @param cp If cp is not null, we continue from here If cp is null, we can add
 *           a new continuation point if possible and necessary.
 * @param descr If no cp is set, we take the browsedescription from there
 * @param maxrefs The maximum number of references the client has requested. If 0,
 *                all matching references are returned at once.
 * @param result The entry in the request */
void
Service_Browse_single(UA_Server *server, UA_Session *session,
                      struct ContinuationPointEntry *cp, const UA_BrowseDescription *descr,
                      UA_UInt32 maxrefs, UA_BrowseResult *result) { 
    size_t referencesCount = 0;
    size_t referencesIndex = 0;
    /* set the browsedescription if a cp is given */
    UA_UInt32 continuationIndex = 0;
    if(cp) {
        descr = &cp->browseDescription;
        maxrefs = cp->maxReferences;
        continuationIndex = cp->continuationIndex;
    }

    /* is the browsedirection valid? */
    if(descr->browseDirection != UA_BROWSEDIRECTION_BOTH &&
       descr->browseDirection != UA_BROWSEDIRECTION_FORWARD &&
       descr->browseDirection != UA_BROWSEDIRECTION_INVERSE) {
        result->statusCode = UA_STATUSCODE_BADBROWSEDIRECTIONINVALID;
        return;
    }
    
    /* get the references that match the browsedescription */
    size_t relevant_refs_size = 0;
    UA_NodeId *relevant_refs = NULL;
    UA_Boolean all_refs = UA_NodeId_isNull(&descr->referenceTypeId);
    if(!all_refs) {
        const UA_Node *rootRef = UA_NodeStore_get(server->nodestore, &descr->referenceTypeId);
        if(!rootRef || rootRef->nodeClass != UA_NODECLASS_REFERENCETYPE) {
            result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
            return;
        }
        if(descr->includeSubtypes) {
            result->statusCode = getTypeHierarchy(server->nodestore, rootRef, false,
                                                  &relevant_refs, &relevant_refs_size);
            if(result->statusCode != UA_STATUSCODE_GOOD)
                return;
        } else {
            relevant_refs = (UA_NodeId*)(uintptr_t)&descr->referenceTypeId;
            relevant_refs_size = 1;
        }
    }

    /* get the node */
    const UA_Node *node = UA_NodeStore_get(server->nodestore, &descr->nodeId);
    if(!node) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        if(!all_refs && descr->includeSubtypes)
            UA_Array_delete(relevant_refs, relevant_refs_size, &UA_TYPES[UA_TYPES_NODEID]);
        return;
    }

    /* if the node has no references, just return */
    if(node->referencesSize == 0) {
        result->referencesSize = 0;
        if(!all_refs && descr->includeSubtypes)
            UA_Array_delete(relevant_refs, relevant_refs_size, &UA_TYPES[UA_TYPES_NODEID]);
        return;
    }

    /* how many references can we return at most? */
    size_t real_maxrefs = maxrefs;
    if(real_maxrefs == 0)
        real_maxrefs = node->referencesSize;
    else if(real_maxrefs > node->referencesSize)
        real_maxrefs = node->referencesSize;
    result->references = UA_Array_new(real_maxrefs, &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION]);
    if(!result->references) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    /* loop over the node's references */
    size_t skipped = 0;
    UA_Boolean isExternal = false;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(; referencesIndex < node->referencesSize && referencesCount < real_maxrefs; ++referencesIndex) {
        isExternal = false;
        const UA_Node *current =
            returnRelevantNode(server, descr, all_refs, &node->references[referencesIndex],
                               relevant_refs, relevant_refs_size, &isExternal);
        if(!current)
            continue;

        if(skipped < continuationIndex) {
            ++skipped;
        } else {
            retval |= fillReferenceDescription(server->nodestore, current,
                                               &node->references[referencesIndex],
                                               descr->resultMask,
                                               &result->references[referencesCount]);
            ++referencesCount;
        }
    }
    result->referencesSize = referencesCount;

    if(referencesCount == 0) {
        UA_free(result->references);
        result->references = NULL;
        result->referencesSize = 0;
    }

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(result->references, result->referencesSize,
                        &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION]);
        result->references = NULL;
        result->referencesSize = 0;
        result->statusCode = retval;
    }

 cleanup:
    if(!all_refs && descr->includeSubtypes)
        UA_Array_delete(relevant_refs, relevant_refs_size, &UA_TYPES[UA_TYPES_NODEID]);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* create, update, delete continuation points */
    if(cp) {
        if(referencesIndex == node->referencesSize) {
            /* all done, remove a finished continuationPoint */
            removeCp(cp, session);
        } else {
            /* update the cp and return the cp identifier */
            cp->continuationIndex += (UA_UInt32)referencesCount;
            UA_ByteString_copy(&cp->identifier, &result->continuationPoint);
        }
    } else if(maxrefs != 0 && referencesCount >= maxrefs) {
        /* create a cp */
        if(session->availableContinuationPoints <= 0 ||
           !(cp = UA_malloc(sizeof(struct ContinuationPointEntry)))) {
            result->statusCode = UA_STATUSCODE_BADNOCONTINUATIONPOINTS;
            return;
        }
        UA_BrowseDescription_copy(descr, &cp->browseDescription);
        cp->maxReferences = maxrefs;
        cp->continuationIndex = (UA_UInt32)referencesCount;
        UA_Guid *ident = UA_Guid_new();
        *ident = UA_Guid_random();
        cp->identifier.data = (UA_Byte*)ident;
        cp->identifier.length = sizeof(UA_Guid);
        UA_ByteString_copy(&cp->identifier, &result->continuationPoint);

        /* store the cp */
        LIST_INSERT_HEAD(&session->continuationPoints, cp, pointers);
        --session->availableContinuationPoints;
    }
}

void Service_Browse(UA_Server *server, UA_Session *session, const UA_BrowseRequest *request,
                    UA_BrowseResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing BrowseRequest");
    if(!UA_NodeId_isNull(&request->view.viewId)) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADVIEWIDUNKNOWN;
        return;
    }

    if(request->nodesToBrowseSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    size_t size = request->nodesToBrowseSize;
    response->results = UA_Array_new(size, &UA_TYPES[UA_TYPES_BROWSERESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = size;


    for(size_t i = 0; i < size; ++i) {
            Service_Browse_single(server, session, NULL, &request->nodesToBrowse[i],
                                  request->requestedMaxReferencesPerNode, &response->results[i]);
    }
}

UA_BrowseResult
UA_Server_browse(UA_Server *server, UA_UInt32 maxrefs, const UA_BrowseDescription *descr) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    UA_RCU_LOCK();
    Service_Browse_single(server, &adminSession, NULL, descr, maxrefs, &result);
    UA_RCU_UNLOCK();
    return result;
}

static void
UA_Server_browseNext_single(UA_Server *server, UA_Session *session, UA_Boolean releaseContinuationPoint,
                            const UA_ByteString *continuationPoint, UA_BrowseResult *result) {
    result->statusCode = UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
    struct ContinuationPointEntry *cp, *temp;
    LIST_FOREACH_SAFE(cp, &session->continuationPoints, pointers, temp) {
        if(UA_ByteString_equal(&cp->identifier, continuationPoint)) {
            result->statusCode = UA_STATUSCODE_GOOD;
            if(!releaseContinuationPoint)
                Service_Browse_single(server, session, cp, NULL, 0, result);
            else
                removeCp(cp, session);
            break;
        }
    }
}

void Service_BrowseNext(UA_Server *server, UA_Session *session, const UA_BrowseNextRequest *request,
                        UA_BrowseNextResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing BrowseNextRequest");
    if(request->continuationPointsSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }
    size_t size = request->continuationPointsSize;
    response->results = UA_Array_new(size, &UA_TYPES[UA_TYPES_BROWSERESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->resultsSize = size;
    for(size_t i = 0; i < size; ++i)
        UA_Server_browseNext_single(server, session, request->releaseContinuationPoints,
                                    &request->continuationPoints[i], &response->results[i]);
}

UA_BrowseResult
UA_Server_browseNext(UA_Server *server, UA_Boolean releaseContinuationPoint,
                     const UA_ByteString *continuationPoint) {
    UA_BrowseResult result;
    UA_BrowseResult_init(&result);
    UA_RCU_LOCK();
    UA_Server_browseNext_single(server, &adminSession, releaseContinuationPoint,
                                continuationPoint, &result);
    UA_RCU_UNLOCK();
    return result;
}

/***********************/
/* TranslateBrowsePath */
/***********************/

static void
walkBrowsePathElementNodeReference(UA_BrowsePathResult *result, size_t *targetsSize,
                                   UA_NodeId **next, size_t *nextSize, size_t *nextCount,
                                   UA_UInt32 elemDepth, UA_Boolean inverse, UA_Boolean all_refs,
                                   const UA_NodeId *reftypes, size_t reftypes_count,
                                   const UA_ReferenceNode *reference) {
    /* Does the direction of the reference match? */
    if(reference->isInverse != inverse)
        return;

    /* Is the node relevant? */
    if(!all_refs) {
        UA_Boolean match = false;
        for(size_t j = 0; j < reftypes_count; ++j) {
            if(UA_NodeId_equal(&reference->referenceTypeId, &reftypes[j])) {
                match = true;
                break;
            }
        }
        if(!match)
            return;
    }

    /* Does the reference point to an external server? Then add to the
     * targets with the right path "depth" */
    if(reference->targetId.serverIndex != 0) {
        UA_BrowsePathTarget *tempTargets =
            UA_realloc(result->targets, sizeof(UA_BrowsePathTarget) * (*targetsSize) * 2);
        if(!tempTargets) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        result->targets = tempTargets;
        (*targetsSize) *= 2;
        result->statusCode = UA_ExpandedNodeId_copy(&reference->targetId,
                                                    &result->targets[result->targetsSize].targetId);
        result->targets[result->targetsSize].remainingPathIndex = elemDepth;
        return;
    }

    /* Add the node to the next array for the following path element */
    if(*nextSize <= *nextCount) {
        UA_NodeId *tempNext = UA_realloc(*next, sizeof(UA_NodeId) * (*nextSize) * 2);
        if(!tempNext) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        *next = tempNext;
        (*nextSize) *= 2;
    }
    result->statusCode = UA_NodeId_copy(&reference->targetId.nodeId,
                                        &(*next)[*nextCount]);
    ++(*nextCount);
}

static void
walkBrowsePathElement(UA_Server *server, UA_Session *session,
                      UA_BrowsePathResult *result, size_t *targetsSize,
                      const UA_RelativePathElement *elem, UA_UInt32 elemDepth,
                      const UA_QualifiedName *targetName,
                      const UA_NodeId *current, const size_t currentCount,
                      UA_NodeId **next, size_t *nextSize, size_t *nextCount) {
    /* Get the full list of relevant referencetypes for this path element */
    UA_NodeId *reftypes = NULL;
    size_t reftypes_count = 1; // all_refs or no subtypes => 1
    UA_Boolean all_refs = false;
    if(UA_NodeId_isNull(&elem->referenceTypeId)) {
        all_refs = true;
    } else if(!elem->includeSubtypes) {
        reftypes = (UA_NodeId*)(uintptr_t)&elem->referenceTypeId; // ptr magic due to const cast
    } else {
        const UA_Node *rootRef = UA_NodeStore_get(server->nodestore, &elem->referenceTypeId);
        if(!rootRef || rootRef->nodeClass != UA_NODECLASS_REFERENCETYPE)
            return;
        UA_StatusCode retval =
            getTypeHierarchy(server->nodestore, rootRef, false, &reftypes, &reftypes_count);
        if(retval != UA_STATUSCODE_GOOD)
            return;
    }

    /* Iterate over all nodes at the current depth-level */
    for(size_t i = 0; i < currentCount; ++i) {
        /* Get the node */
        const UA_Node *node = UA_NodeStore_get(server->nodestore, &current[i]);
        if(!node) {
            /* If we cannot find the node at depth 0, the starting node does not exist */
            if(elemDepth == 0)
                result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
            continue;
        }

        /* Test whether the current node has the target name required in the
         * previous path element */
        if(targetName && (targetName->namespaceIndex != node->browseName.namespaceIndex ||
                          !UA_String_equal(&targetName->name, &node->browseName.name)))
            continue;

        /* Walk over the references in the node */
        /* Loop over the nodes references */
        for(size_t r = 0; r < node->referencesSize &&
                result->statusCode == UA_STATUSCODE_GOOD; ++r) {
            UA_ReferenceNode *reference = &node->references[r];
            walkBrowsePathElementNodeReference(result, targetsSize, next, nextSize, nextCount,
                                               elemDepth, elem->isInverse, all_refs,
                                               reftypes, reftypes_count, reference);
        }
    }

    if(!all_refs && elem->includeSubtypes)
        UA_Array_delete(reftypes, reftypes_count, &UA_TYPES[UA_TYPES_NODEID]);
}

/* This assumes that result->targets has enough room for all currentCount elements */
static void
addBrowsePathTargets(UA_Server *server, UA_Session *session, UA_BrowsePathResult *result,
                     const UA_QualifiedName *targetName, UA_NodeId *current, size_t currentCount) {
    for(size_t i = 0; i < currentCount; i++) {
        /* Get the node */
        const UA_Node *node = UA_NodeStore_get(server->nodestore, &current[i]);
        if(!node) {
            UA_NodeId_deleteMembers(&current[i]);
            continue;
        }

        /* Test whether the current node has the target name required in the
         * previous path element */
        if(targetName->namespaceIndex != node->browseName.namespaceIndex ||
           !UA_String_equal(&targetName->name, &node->browseName.name)) {
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
               UA_BrowsePathResult *result, size_t targetsSize,
               UA_NodeId **current, size_t *currentSize, size_t *currentCount,
               UA_NodeId **next, size_t *nextSize, size_t *nextCount) {
    UA_assert(*currentCount == 1);
    UA_assert(*nextCount == 0);

    /* Points to the targetName of the _previous_ path element */
    const UA_QualifiedName *targetName = NULL;

    /* Iterate over path elements */
    UA_assert(path->relativePath.elementsSize > 0);
    for(UA_UInt32 i = 0; i < path->relativePath.elementsSize; ++i) {
        walkBrowsePathElement(server, session, result, &targetsSize,
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
    addBrowsePathTargets(server, session, result, targetName, *current, *currentCount);
    *currentCount = 0;
}

static void
translateBrowsePathToNodeIds(UA_Server *server, UA_Session *session,
                             const UA_BrowsePath *path, UA_BrowsePathResult *result) {
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
    result->targets = (UA_BrowsePathTarget*)UA_malloc(sizeof(UA_BrowsePathTarget) * targetsSize);
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
    walkBrowsePath(server, session, path, result, targetsSize,
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
    UA_RCU_LOCK();
    translateBrowsePathToNodeIds(server, &adminSession, browsePath, &result);
    UA_RCU_UNLOCK();
    return result;
}

void Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
                                           const UA_TranslateBrowsePathsToNodeIdsRequest *request,
                                           UA_TranslateBrowsePathsToNodeIdsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing TranslateBrowsePathsToNodeIdsRequest");
    if(request->browsePathsSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    size_t size = request->browsePathsSize;
    response->results = UA_Array_new(size, &UA_TYPES[UA_TYPES_BROWSEPATHRESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->resultsSize = size;
    for(size_t i = 0; i < size; ++i)
        translateBrowsePathToNodeIds(server, session, &request->browsePaths[i],
                                     &response->results[i]);

}

void Service_RegisterNodes(UA_Server *server, UA_Session *session, const UA_RegisterNodesRequest *request,
                           UA_RegisterNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing RegisterNodesRequest");
    //TODO: hang the nodeids to the session if really needed
    response->responseHeader.timestamp = UA_DateTime_now();
    if(request->nodesToRegisterSize <= 0)
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
    else {
        response->responseHeader.serviceResult =
            UA_Array_copy(request->nodesToRegister, request->nodesToRegisterSize,
                          (void**)&response->registeredNodeIds, &UA_TYPES[UA_TYPES_NODEID]);
        if(response->responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            response->registeredNodeIdsSize = request->nodesToRegisterSize;
    }
}

void Service_UnregisterNodes(UA_Server *server, UA_Session *session, const UA_UnregisterNodesRequest *request,
                             UA_UnregisterNodesResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing UnRegisterNodesRequest");
    //TODO: remove the nodeids from the session if really needed
    response->responseHeader.timestamp = UA_DateTime_now();
    if(request->nodesToUnregisterSize==0)
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
}
