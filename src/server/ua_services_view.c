#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_util.h"

/* Releases the current node, even if it was supplied as an argument. */
static UA_StatusCode fillReferenceDescription(UA_NodeStore *ns, const UA_Node *currentNode, UA_ReferenceNode *reference,
                                              UA_UInt32 resultMask, UA_ReferenceDescription *referenceDescription) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReferenceDescription_init(referenceDescription);
    retval |= UA_NodeId_copy(&currentNode->nodeId, &referenceDescription->nodeId.nodeId);
    //TODO: ExpandedNodeId is mocked up
    referenceDescription->nodeId.serverIndex = 0;
    referenceDescription->nodeId.namespaceUri.length = -1;

    if(resultMask & UA_BROWSERESULTMASK_REFERENCETYPEID)
        retval |= UA_NodeId_copy(&reference->referenceTypeId, &referenceDescription->referenceTypeId);
    if(resultMask & UA_BROWSERESULTMASK_ISFORWARD)
        referenceDescription->isForward = !reference->isInverse;
    if(resultMask & UA_BROWSERESULTMASK_NODECLASS)
        retval |= UA_NodeClass_copy(&currentNode->nodeClass, &referenceDescription->nodeClass);
    if(resultMask & UA_BROWSERESULTMASK_BROWSENAME)
        retval |= UA_QualifiedName_copy(&currentNode->browseName, &referenceDescription->browseName);
    if(resultMask & UA_BROWSERESULTMASK_DISPLAYNAME)
        retval |= UA_LocalizedText_copy(&currentNode->displayName, &referenceDescription->displayName);
    if(resultMask & UA_BROWSERESULTMASK_TYPEDEFINITION ) {
        for(UA_Int32 i = 0;i < currentNode->referencesSize;i++) {
            UA_ReferenceNode *ref = &currentNode->references[i];
            if(ref->referenceTypeId.identifier.numeric == 40 /* hastypedefinition */) {
                retval |= UA_ExpandedNodeId_copy(&ref->targetId, &referenceDescription->typeDefinition);
                break;
            }
        }
    }

    if(currentNode)
        UA_NodeStore_release(currentNode);
    if(retval)
        UA_ReferenceDescription_deleteMembers(referenceDescription);
    return retval;
}

/* Tests if the node is relevant to the browse request and shall be returned. If
   so, it is retrieved from the Nodestore. If not, null is returned. */
static const UA_Node *
getRelevantTargetNode(UA_NodeStore *ns, const UA_BrowseDescription *browseDescription, UA_Boolean returnAll,
                      UA_ReferenceNode *reference, UA_NodeId *relevantRefTypes, UA_UInt32 relevantRefTypesCount) {
    if(reference->isInverse == UA_TRUE && browseDescription->browseDirection == UA_BROWSEDIRECTION_FORWARD)
        return UA_NULL;

    else if(reference->isInverse == UA_FALSE && browseDescription->browseDirection == UA_BROWSEDIRECTION_INVERSE)
        return UA_NULL;

    UA_Boolean isRelevant = returnAll;
    if(!isRelevant) {
        for(UA_UInt32 i = 0;i < relevantRefTypesCount;i++) {
            if(UA_NodeId_equal(&reference->referenceTypeId, &relevantRefTypes[i]))
                isRelevant = UA_TRUE;
        }
        if(!isRelevant)
            return UA_NULL;
    }

    const UA_Node *node = UA_NodeStore_get(ns, &reference->targetId.nodeId);
    if(!node)
        return UA_NULL;

    if(browseDescription->nodeClassMask != 0 && (node->nodeClass & browseDescription->nodeClassMask) == 0) {
        UA_NodeStore_release(node);
        return UA_NULL;
    }

    return node;
}

/* We do not search across namespaces so far. The id of the root-referencetype
   is returned in the array also. */
static UA_StatusCode findRelevantReferenceTypes(UA_NodeStore *ns, const UA_NodeId *rootReferenceType,
                                                UA_NodeId **referenceTypes, UA_UInt32 *referenceTypesSize) {
    /* The references form a tree. We walk the tree by adding new nodes to the end of the array. */
    UA_UInt32 currentIndex = 0;
    UA_UInt32 currentLastIndex = 0;
    UA_UInt32 currentArraySize = 20; // should be more than enough. if not, increase the array size.
    UA_NodeId *typeArray = UA_malloc(sizeof(UA_NodeId) * currentArraySize);
    if(!typeArray)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_NodeId_copy(rootReferenceType, &typeArray[0]);
    if(retval) {
        UA_free(typeArray);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
        
    do {
        const UA_ReferenceTypeNode *node =
            (const UA_ReferenceTypeNode *)UA_NodeStore_get(ns, &typeArray[currentIndex]);
        if(!node)
            break;
        // subtypes of referencestypes are always referencestypes?
        if(node->nodeClass != UA_NODECLASS_REFERENCETYPE) 
            continue;

        // Find subtypes of the current referencetype
        for(UA_Int32 i = 0; i < node->referencesSize && retval == UA_STATUSCODE_GOOD; i++) {
            if(node->references[i].referenceTypeId.identifier.numeric != 45 /* HasSubtype */ ||
               node->references[i].isInverse == UA_TRUE)
                continue;

            if(currentLastIndex + 1 >= currentArraySize) {
                // we need to resize the array
                UA_NodeId *newArray = UA_malloc(sizeof(UA_NodeId) * currentArraySize * 2);
                if(newArray) {
                    UA_memcpy(newArray, typeArray, sizeof(UA_NodeId) * currentArraySize);
                    currentArraySize *= 2;
                    UA_free(typeArray);
                    typeArray = newArray;
                } else {
                    retval = UA_STATUSCODE_BADOUTOFMEMORY;
                    break;
                }
            }

            // ok, we have space to add the new referencetype.
            retval |= UA_NodeId_copy(&node->references[i].targetId.nodeId, &typeArray[++currentLastIndex]);
            if(retval)
                currentLastIndex--; // undo if we need to delete the typeArray
        }
        UA_NodeStore_release((const UA_Node*)node);
    } while(++currentIndex <= currentLastIndex && retval == UA_STATUSCODE_GOOD);

    if(retval)
        UA_Array_delete(typeArray, &UA_TYPES[UA_TYPES_NODEID], currentLastIndex);
    else {
        *referenceTypes = typeArray;
        *referenceTypesSize = currentLastIndex + 1;
    }
    
    return retval;
}

/* Results for a single browsedescription. */
static void getBrowseResult(UA_NodeStore *ns, const UA_BrowseDescription *browseDescription,
                            UA_UInt32 maxReferences, UA_BrowseResult *browseResult) {
    UA_UInt32  relevantReferenceTypesSize = 0;
    UA_NodeId *relevantReferenceTypes = UA_NULL;

    // if the referencetype is null, all referencetypes are returned
    UA_Boolean returnAll = UA_NodeId_isNull(&browseDescription->referenceTypeId);
    if(!returnAll) {
        if(browseDescription->includeSubtypes) {
            browseResult->statusCode = findRelevantReferenceTypes(ns, &browseDescription->referenceTypeId,
                                                                  &relevantReferenceTypes,
                                                                  &relevantReferenceTypesSize);
            if(browseResult->statusCode != UA_STATUSCODE_GOOD)
                return;
        } else {
            relevantReferenceTypes = UA_NodeId_new();
            UA_NodeId_copy(&browseDescription->referenceTypeId, relevantReferenceTypes);
            relevantReferenceTypesSize = 1;
        }
    }

    const UA_Node *parentNode = UA_NodeStore_get(ns, &browseDescription->nodeId);
    if(!parentNode) {
        browseResult->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        if(!returnAll)
            UA_Array_delete(relevantReferenceTypes, &UA_TYPES[UA_TYPES_NODEID], relevantReferenceTypesSize);
        return;
    }

    maxReferences = parentNode->referencesSize;
    // 0 => unlimited references
    if(maxReferences <= 0 || maxReferences > UA_INT32_MAX ||
       (UA_Int32)maxReferences > parentNode->referencesSize) {
        if(parentNode->referencesSize < 0)
            maxReferences = 0;
        else
            maxReferences = parentNode->referencesSize;
    }

    /* We allocate an array that is probably too big. But since most systems
       have more than enough memory, this has zero impact on speed and
       performance. Call Array_delete with the actual content size! */
    browseResult->references = UA_malloc(sizeof(UA_ReferenceDescription) * maxReferences);
    if(!browseResult->references) {
        browseResult->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
    } else {
        UA_UInt32 currentRefs = 0;
        for(UA_Int32 i = 0;i < parentNode->referencesSize && currentRefs < maxReferences;i++) {
            // 1) Is the node relevant? If yes, the node is retrieved from the nodestore.
            const UA_Node *currentNode = getRelevantTargetNode(ns, browseDescription, returnAll,
                                                               &parentNode->references[i],
                                                               relevantReferenceTypes,
                                                               relevantReferenceTypesSize);
            if(!currentNode)
                continue;

            // 2) Fill the reference description. This also releases the current node.
            if(fillReferenceDescription(ns, currentNode, &parentNode->references[i],
                                        browseDescription->resultMask,
                                        &browseResult->references[currentRefs]) != UA_STATUSCODE_GOOD) {
                UA_Array_delete(browseResult->references, &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION], currentRefs);
                currentRefs = 0;
                browseResult->references = UA_NULL;
                browseResult->statusCode = UA_STATUSCODE_UNCERTAINNOTALLNODESAVAILABLE;
                break;
            }
            currentRefs++;
        }
        if(currentRefs != 0)
            browseResult->referencesSize = currentRefs;
        else {
            UA_free(browseResult->references);
            browseResult->references = UA_NULL;
        }
    }

    UA_NodeStore_release(parentNode);
    if(!returnAll)
        UA_Array_delete(relevantReferenceTypes, &UA_TYPES[UA_TYPES_NODEID], relevantReferenceTypesSize);
}

void Service_Browse(UA_Server *server, UA_Session *session, const UA_BrowseRequest *request,
                    UA_BrowseResponse *response) {
   if(request->nodesToBrowseSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

   response->results = UA_Array_new(&UA_TYPES[UA_TYPES_BROWSERESULT], request->nodesToBrowseSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* ### Begin External Namespaces */
    UA_Boolean *isExternal = UA_alloca(sizeof(UA_Boolean) * request->nodesToBrowseSize);
    UA_memset(isExternal, UA_FALSE, sizeof(UA_Boolean) * request->nodesToBrowseSize);
    UA_UInt32 *indices = UA_alloca(sizeof(UA_UInt32) * request->nodesToBrowseSize);
    for(UA_Int32 j = 0;j<server->externalNamespacesSize;j++) {
        UA_UInt32 indexSize = 0;
        for(UA_Int32 i = 0;i < request->nodesToBrowseSize;i++) {
            if(request->nodesToBrowse[i].nodeId.namespaceIndex != server->externalNamespaces[j].index)
                continue;
            isExternal[i] = UA_TRUE;
            indices[indexSize] = i;
            indexSize++;
        }
        if(indexSize == 0)
            continue;

        UA_ExternalNodeStore *ens = &server->externalNamespaces[j].externalNodeStore;
        ens->browseNodes(ens->ensHandle, &request->requestHeader, request->nodesToBrowse,
                       indices, indexSize, request->requestedMaxReferencesPerNode, response->results, response->diagnosticInfos);
    }
    /* ### End External Namespaces */


    response->resultsSize = request->nodesToBrowseSize;
    for(UA_Int32 i = 0;i < request->nodesToBrowseSize;i++){
        if(!isExternal[i]) {
            getBrowseResult(server->nodestore, &request->nodesToBrowse[i],
                        request->requestedMaxReferencesPerNode, &response->results[i]);
        }
    }
}

void Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
                                           const UA_TranslateBrowsePathsToNodeIdsRequest *request,
                                           UA_TranslateBrowsePathsToNodeIdsResponse *response) {
    if(request->browsePathsSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_Array_new(&UA_TYPES[UA_TYPES_BROWSEPATHRESULT], request->browsePathsSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->resultsSize = request->browsePathsSize;
    for(UA_Int32 i = 0;i < response->resultsSize;i++)
        response->results[i].statusCode = UA_STATUSCODE_BADNOMATCH; //FIXME: implement
}
