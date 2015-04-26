#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_util.h"

static UA_StatusCode fillrefdescr(UA_NodeStore *ns, const UA_Node *curr, UA_ReferenceNode *ref,
                                  UA_UInt32 mask, UA_ReferenceDescription *descr)
{
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReferenceDescription_init(descr);
    retval |= UA_NodeId_copy(&curr->nodeId, &descr->nodeId.nodeId);
    //TODO: ExpandedNodeId is mocked up
    descr->nodeId.serverIndex = 0;
    descr->nodeId.namespaceUri.length = -1;

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
            for(UA_Int32 i = 0; i < curr->referencesSize; i++) {
                UA_ReferenceNode *refnode = &curr->references[i];
                if(refnode->referenceTypeId.identifier.numeric != UA_NS0ID_HASTYPEDEFINITION)
                    continue;
                retval |= UA_ExpandedNodeId_copy(&refnode->targetId, &descr->typeDefinition);
                break;
            }
        }
    }

    if(retval)
        UA_ReferenceDescription_deleteMembers(descr);
    return retval;
}

/* Tests if the node is relevant to the browse request and shall be returned. If
   so, it is retrieved from the Nodestore. If not, null is returned. */
static const UA_Node *relevant_node(UA_NodeStore *ns, const UA_BrowseDescription *descr,
                                    UA_Boolean return_all, UA_ReferenceNode *reference,
                                    UA_NodeId *relevant, size_t relevant_count)
{
    if(reference->isInverse == UA_TRUE && descr->browseDirection == UA_BROWSEDIRECTION_FORWARD)
        return UA_NULL;
    else if(reference->isInverse == UA_FALSE && descr->browseDirection == UA_BROWSEDIRECTION_INVERSE)
        return UA_NULL;

    if(!return_all) {
        for(size_t i = 0; i < relevant_count; i++) {
            if(UA_NodeId_equal(&reference->referenceTypeId, &relevant[i]))
                goto is_relevant;
        }
        return UA_NULL;
    }
is_relevant: ;
    const UA_Node *node = UA_NodeStore_get(ns, &reference->targetId.nodeId);
    if(node && descr->nodeClassMask != 0 && (node->nodeClass & descr->nodeClassMask) == 0) {
        UA_NodeStore_release(node);
        return UA_NULL;
    }
    return node;
}

/**
 * We find all subtypes by a single iteration over the array. We start with an array with a single
 * root nodeid at the beginning. When we find relevant references, we add the nodeids to the back of
 * the array and increase the size. Since the hierarchy is not cyclic, we can safely progress in the
 * array to process the newly found referencetype nodeids (emulated recursion).
 */
static UA_StatusCode findsubtypes(UA_NodeStore *ns, const UA_NodeId *root, UA_NodeId **reftypes,
                                  size_t *reftypes_count)
{
    size_t results_size = 20; // probably too big, but saves mallocs
    UA_NodeId *results = UA_malloc(sizeof(UA_NodeId) * results_size);
    if(!results)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    const UA_Node *node = UA_NodeStore_get(ns, root);
    if(!node || node->nodeClass != UA_NODECLASS_REFERENCETYPE)  {
        UA_NodeStore_release(node);
        UA_free(results);
        return UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
    }
    UA_NodeStore_release(node);

    UA_StatusCode retval = UA_NodeId_copy(root, &results[0]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(results);
        return retval;
    }
        
    size_t index = 0; // where are we currently in the array?
    size_t last = 0; // where is the last element in the array?
    do {
        node = UA_NodeStore_get(ns, &results[index]);
        if(!node || node->nodeClass != UA_NODECLASS_REFERENCETYPE)
            continue;
        for(UA_Int32 i = 0; i < node->referencesSize; i++) {
            if(node->references[i].referenceTypeId.identifier.numeric != UA_NS0ID_HASSUBTYPE ||
               node->references[i].isInverse == UA_TRUE)
                continue;

            if(++last >= results_size) { // is the array big enough?
                UA_NodeId *new_results = UA_realloc(results, sizeof(UA_NodeId) * results_size * 2);
                if(!new_results) {
                    retval = UA_STATUSCODE_BADOUTOFMEMORY;
                    break;
                }
                results = new_results;
                results_size *= 2;
            }

            retval = UA_NodeId_copy(&node->references[i].targetId.nodeId, &results[last]);
            if(retval != UA_STATUSCODE_GOOD) {
                last--; // for array_delete
                break;
            }
        }
        UA_NodeStore_release(node);
    } while(++index <= last && retval == UA_STATUSCODE_GOOD);

    if(retval) {
        UA_Array_delete(results, &UA_TYPES[UA_TYPES_NODEID], last);
        return retval;
    }

    *reftypes = results;
    *reftypes_count = last + 1;
    return UA_STATUSCODE_GOOD;
}

/* Results for a single browsedescription. Call this either with an existing continuationpoint from
   which we take the entire context for the search. Or, call with a browsedescription and maxrefs
   value. If we need to create a new continuationpoint, it will be alloced and the new pointer
   stored in *cp.
 */
static void browse(UA_NodeStore *ns, struct ContinuationPointEntry **cp, const UA_BrowseDescription *descr, 
                   UA_UInt32 maxrefs, UA_BrowseResult *result)
{
    UA_UInt32 continuationIndex = 0;
    if(*cp) {
        descr = &(*cp)->browseDescription;
        maxrefs = (*cp)->maxReferences;
        continuationIndex = (*cp)->continuationIndex;
    }

    if(descr->browseDirection != UA_BROWSEDIRECTION_BOTH &&
       descr->browseDirection != UA_BROWSEDIRECTION_FORWARD &&
       descr->browseDirection != UA_BROWSEDIRECTION_INVERSE) {
        result->statusCode = UA_STATUSCODE_BADBROWSEDIRECTIONINVALID;
        return;
    }
    
    size_t relevant_refs_size = 0;
    UA_NodeId *relevant_refs = UA_NULL;
    UA_Boolean all_refs = UA_NodeId_isNull(&descr->referenceTypeId);

    if(!all_refs) {
        if(descr->includeSubtypes) {
            result->statusCode = findsubtypes(ns, &descr->referenceTypeId, &relevant_refs,
                                              &relevant_refs_size);
            if(result->statusCode != UA_STATUSCODE_GOOD)
                return;
        } else {
            const UA_Node *rootRef = UA_NodeStore_get(ns, &descr->referenceTypeId);
            if(!rootRef) {
                result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
                return;
            } else if(rootRef->nodeClass != UA_NODECLASS_REFERENCETYPE) {
                UA_NodeStore_release(rootRef);
                result->statusCode = UA_STATUSCODE_BADREFERENCETYPEIDINVALID;
                return;
            }
            UA_NodeStore_release(rootRef);
            relevant_refs = (UA_NodeId*)(uintptr_t)&descr->referenceTypeId;
            relevant_refs_size = 1;
        }
    }

    // get the node
    const UA_Node *node = UA_NodeStore_get(ns, &descr->nodeId);
    if(!node) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        if(!all_refs && descr->includeSubtypes)
            UA_Array_delete(relevant_refs, &UA_TYPES[UA_TYPES_NODEID], relevant_refs_size);
        return;
    }

    if(node->referencesSize <= 0) {
        result->referencesSize = 0;
        goto cleanup;
    }

    UA_UInt32 real_maxrefs = maxrefs;
    if(real_maxrefs == 0)
        real_maxrefs = node->referencesSize;
    result->references = UA_malloc(sizeof(UA_ReferenceDescription) * real_maxrefs);
    if(!result->references) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    size_t count = 0;
    size_t skipped = 0;
    for(UA_Int32 i = 0; i < node->referencesSize && count < real_maxrefs; i++) {
        const UA_Node *current = relevant_node(ns, descr, all_refs, &node->references[i],
                                               relevant_refs, relevant_refs_size);
        if(!current)
            continue;
        if(skipped < continuationIndex) {
            UA_NodeStore_release(current);
            skipped++;
            continue;
        }
        UA_StatusCode retval = fillrefdescr(ns, current, &node->references[i], descr->resultMask,
                                            &result->references[count]);
        UA_NodeStore_release(current);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Array_delete(result->references, &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], count);
            result->references = UA_NULL;
            count = 0;
            result->statusCode = UA_STATUSCODE_UNCERTAINNOTALLNODESAVAILABLE;
            goto cleanup;
        }
        count++;
    }

    if(*cp) {
        (*cp)->continuationIndex += count;
        if((*cp)->continuationIndex == node->referencesSize) {
            /* remove a finished continuationPoint */
            UA_ByteString_deleteMembers(&(*cp)->identifier);
            UA_BrowseDescription_deleteMembers(&(*cp)->browseDescription);
            LIST_REMOVE(*cp, pointers);
            UA_free(*cp);
            *cp = UA_NULL;
        }
    } else if(maxrefs != 0 && count >= maxrefs) {
        /* create a continuationPoint */
        *cp = UA_malloc(sizeof(struct ContinuationPointEntry));
        UA_BrowseDescription_copy(descr,&(*cp)->browseDescription);
        (*cp)->maxReferences = maxrefs;
        (*cp)->continuationIndex = count;
        UA_Guid *ident = UA_Guid_new();
        UA_UInt32 seed = (uintptr_t)*cp;
        *ident = UA_Guid_random(&seed);
        (*cp)->identifier.data = (UA_Byte*)ident;
        (*cp)->identifier.length = sizeof(UA_Guid);
    }
    
    if(count > 0)
        result->referencesSize = count;
    else {
        UA_free(result->references);
        result->references = UA_NULL;
    }

cleanup:
    UA_NodeStore_release(node);
    if(!all_refs && descr->includeSubtypes)
        UA_Array_delete(relevant_refs, &UA_TYPES[UA_TYPES_NODEID], relevant_refs_size);
}

void Service_Browse(UA_Server *server, UA_Session *session, const UA_BrowseRequest *request,
                    UA_BrowseResponse *response) {
    if(!UA_NodeId_isNull(&request->view.viewId)) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADVIEWIDUNKNOWN;
        return;
    }
    
   if(request->nodesToBrowseSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }
   size_t size = request->nodesToBrowseSize;

   response->results = UA_Array_new(&UA_TYPES[UA_TYPES_BROWSERESULT], size);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* ### Begin External Namespaces */
    UA_Boolean *isExternal = UA_alloca(sizeof(UA_Boolean) * size);
    UA_memset(isExternal, UA_FALSE, sizeof(UA_Boolean) * size);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32) * size);
    for(size_t j = 0; j < server->externalNamespacesSize; j++) {
        size_t indexSize = 0;
        for(size_t i = 0;i < size;i++) {
            if(request->nodesToBrowse[i].nodeId.namespaceIndex != server->externalNamespaces[j].index)
                continue;
            isExternal[i] = UA_TRUE;
            indices[indexSize] = i;
            indexSize++;
        }
        if(indexSize == 0)
            continue;
        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->browseNodes(ens->ensHandle, &request->requestHeader, request->nodesToBrowse, indices, indexSize,
                         request->requestedMaxReferencesPerNode, response->results, response->diagnosticInfos);
    }
    /* ### End External Namespaces */

    response->resultsSize = size;
    for(size_t i = 0; i < size; i++) {
        if(!isExternal[i]) {
            struct ContinuationPointEntry *cp = UA_NULL;
            browse(server->nodestore, &cp, &request->nodesToBrowse[i],
                   request->requestedMaxReferencesPerNode, &response->results[i]);
            if(cp) {
                LIST_INSERT_HEAD(&session->continuationPoints, cp, pointers);
                UA_ByteString_copy(&cp->identifier, &response->results[i].continuationPoint);
            }
        }
    }
}

void Service_BrowseNext(UA_Server *server, UA_Session *session, const UA_BrowseNextRequest *request,
                        UA_BrowseNextResponse *response) {
   if(request->continuationPointsSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
   }
   size_t size = request->continuationPointsSize;
   if(!request->releaseContinuationPoints) {
       response->results = UA_Array_new(&UA_TYPES[UA_TYPES_BROWSERESULT], size);
       if(!response->results) {
           response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
           return;
       }
       response->resultsSize = size;
       for(size_t i = 0; i < size; i++) {
           struct ContinuationPointEntry *cp = UA_NULL;
           struct ContinuationPointEntry *search_cp;
           LIST_FOREACH(search_cp, &session->continuationPoints, pointers) {
               if(UA_ByteString_equal(&search_cp->identifier, &request->continuationPoints[i])) {
                   cp = search_cp;
                   break;
               }
           }
           if(!cp)
               response->results[i].statusCode = UA_STATUSCODE_BADCONTINUATIONPOINTINVALID;
           else
               browse(server->nodestore, &cp, UA_NULL, 0, &response->results[i]);
       }
   } else {
       for(size_t i = 0; i < size; i++) {
           struct ContinuationPointEntry *cp = UA_NULL;
           LIST_FOREACH(cp, &session->continuationPoints, pointers) {
               if(UA_ByteString_equal(&cp->identifier, &request->continuationPoints[i])) {
                   UA_ByteString_deleteMembers(&cp->identifier);
                   UA_BrowseDescription_deleteMembers(&cp->browseDescription);
                   LIST_REMOVE(cp, pointers);
                   UA_free(cp);
                   break;
               }
           }
       }
   }
}

/***********************/
/* TranslateBrowsePath */
/***********************/

static UA_StatusCode
walkBrowsePath(UA_Server *server, UA_Session *session, const UA_Node *node, const UA_RelativePath *path,
               UA_Int32 pathindex, UA_BrowsePathTarget **targets, UA_Int32 *targets_size,
               UA_Int32 *target_count)
{
    const UA_RelativePathElement *elem = &path->elements[pathindex];
    if(elem->targetName.name.length == -1)
        return UA_STATUSCODE_BADBROWSENAMEINVALID;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId *reftypes;
    size_t reftypes_count = 1; // all_refs or no subtypes => 1
    UA_Boolean all_refs = UA_FALSE;
    if(UA_NodeId_isNull(&elem->referenceTypeId))
        all_refs = UA_TRUE;
    else if(!elem->includeSubtypes)
        reftypes = (UA_NodeId*)(uintptr_t)&elem->referenceTypeId; // ptr magic due to const cast
    else
        retval = findsubtypes(server->nodestore, &elem->referenceTypeId, &reftypes, &reftypes_count);

    for(UA_Int32 i = 0; i < node->referencesSize && retval == UA_STATUSCODE_GOOD; i++) {
        UA_Boolean match = all_refs;
        for(size_t j = 0; j < reftypes_count && !match; j++) {
            if(node->references[i].isInverse == elem->isInverse &&
               UA_NodeId_equal(&node->references[i].referenceTypeId, &reftypes[j]))
                match = UA_TRUE;
        }
        if(!match)
            continue;

        // get the node, todo: expandednodeid
        const UA_Node *next = UA_NodeStore_get(server->nodestore, &node->references[i].targetId.nodeId);
        if(!next)
            continue;

        // test the browsename
        if(elem->targetName.namespaceIndex != next->browseName.namespaceIndex ||
           !UA_String_equal(&elem->targetName.name, &next->browseName.name)) {
            UA_NodeStore_release(next);
            continue;
        }

        if(pathindex + 1 < path->elementsSize) {
            // recursion if the path is longer
            retval = walkBrowsePath(server, session, next, path, pathindex + 1,
                                    targets, targets_size, target_count);
            UA_NodeStore_release(next);
        } else {
            // add the browsetarget
            if(*target_count >= *targets_size) {
                UA_BrowsePathTarget *newtargets;
                newtargets = UA_realloc(targets, sizeof(UA_BrowsePathTarget) * (*targets_size) * 2);
                if(!newtargets) {
                    retval = UA_STATUSCODE_BADOUTOFMEMORY;
                    UA_NodeStore_release(next);
                    break;
                }
                *targets = newtargets;
                *targets_size *= 2;
            }

            UA_BrowsePathTarget *res = *targets;
            UA_ExpandedNodeId_init(&res[*target_count].targetId);
            retval = UA_NodeId_copy(&next->nodeId, &res[*target_count].targetId.nodeId);
            UA_NodeStore_release(next);
            if(retval != UA_STATUSCODE_GOOD)
                break;
            res[*target_count].remainingPathIndex = UA_UINT32_MAX;
            *target_count += 1;
        }
    }

    if(!all_refs && elem->includeSubtypes)
        UA_Array_delete(reftypes, &UA_TYPES[UA_TYPES_NODEID], (UA_Int32)reftypes_count);
    return retval;
}

static void translateBrowsePath(UA_Server *server, UA_Session *session, const UA_BrowsePath *path,
                                UA_BrowsePathResult *result) {
    if(path->relativePath.elementsSize <= 0) {
        result->statusCode = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }
        
    UA_Int32 arraySize = 10;
    result->targets = UA_malloc(sizeof(UA_BrowsePathTarget) * arraySize);
    if(!result->targets) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    result->targetsSize = 0;
    const UA_Node *firstNode = UA_NodeStore_get(server->nodestore, &path->startingNode);
    if(!firstNode) {
        result->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        UA_free(result->targets);
        result->targets = UA_NULL;
        return;
    }
    result->statusCode = walkBrowsePath(server, session, firstNode, &path->relativePath, 0,
                                        &result->targets, &arraySize, &result->targetsSize);
    UA_NodeStore_release(firstNode);
    if(result->targetsSize == 0 && result->statusCode == UA_STATUSCODE_GOOD)
        result->statusCode = UA_STATUSCODE_BADNOMATCH;
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_Array_delete(result->targets, &UA_TYPES[UA_TYPES_BROWSEPATHTARGET], result->targetsSize);
        result->targets = UA_NULL;
        result->targetsSize = -1;
    }
}

void Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
                                           const UA_TranslateBrowsePathsToNodeIdsRequest *request,
                                           UA_TranslateBrowsePathsToNodeIdsResponse *response) {
	if(request->browsePathsSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    size_t size = request->browsePathsSize;

    response->results = UA_Array_new(&UA_TYPES[UA_TYPES_BROWSEPATHRESULT], size);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* ### Begin External Namespaces */
    UA_Boolean *isExternal = UA_alloca(sizeof(UA_Boolean) * size);
    UA_memset(isExternal, UA_FALSE, sizeof(UA_Boolean) * size);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32) * size);
    for(size_t j = 0; j < server->externalNamespacesSize; j++) {
    	size_t indexSize = 0;
    	for(size_t i = 0;i < size;i++) {
    		if(request->browsePaths[i].startingNode.namespaceIndex != server->externalNamespaces[j].index)
    			continue;
    		isExternal[i] = UA_TRUE;
    		indices[indexSize] = i;
    		indexSize++;
    	}
    	if(indexSize == 0)
    		continue;
    	UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
    	ens->translateBrowsePathsToNodeIds(ens->ensHandle, &request->requestHeader, request->browsePaths,
    			indices, indexSize, response->results, response->diagnosticInfos);
    }
    /* ### End External Namespaces */

    response->resultsSize = size;
    for(size_t i = 0; i < size; i++){
    	if(!isExternal[i])
    		translateBrowsePath(server, session, &request->browsePaths[i], &response->results[i]);
    }
}

void Service_RegisterNodes(UA_Server *server, UA_Session *session, const UA_RegisterNodesRequest *request,
                           UA_RegisterNodesResponse *response) {
	//TODO: hang the nodeids to the session if really needed
	response->responseHeader.timestamp = UA_DateTime_now();
	response->registeredNodeIdsSize = request->nodesToRegisterSize;
	response->registeredNodeIds = request->nodesToRegister;
	if(request->nodesToRegisterSize==0)
		response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
	if(UA_NodeId_equal(&request->requestHeader.authenticationToken, &UA_NODEID_NULL) ||
       !UA_NodeId_equal(&request->requestHeader.authenticationToken, &session->authenticationToken))
		response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
}

void Service_UnregisterNodes(UA_Server *server, UA_Session *session, const UA_UnregisterNodesRequest *request,
                             UA_UnregisterNodesResponse *response) {
	//TODO: remove the nodeids from the session if really needed
	response->responseHeader.timestamp = UA_DateTime_now();
	if(request->nodesToUnregisterSize==0)
		response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
	if(UA_NodeId_equal(&request->requestHeader.authenticationToken, &UA_NODEID_NULL) ||
       !UA_NodeId_equal(&request->requestHeader.authenticationToken, &session->authenticationToken))
		response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
}
