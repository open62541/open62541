#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_nodestore.h"
#include "ua_namespace_0.h"
#include "ua_util.h"

UA_Int32 Service_Browse_getReferenceDescription(UA_NodeStore *ns, UA_ReferenceNode *reference,
                                                UA_UInt32 nodeClassMask, UA_UInt32 resultMask,
                                                UA_ReferenceDescription *referenceDescription) {
    const UA_Node *foundNode;
    if(UA_NodeStore_get(ns, &reference->targetId.nodeId, &foundNode) != UA_SUCCESS)
        return UA_ERROR;

    UA_NodeId_copy(&foundNode->nodeId, &referenceDescription->nodeId.nodeId);
    //TODO ExpandedNodeId is a mockup
    referenceDescription->nodeId.serverIndex = 0;
    referenceDescription->nodeId.namespaceUri.length = -1;

    UA_UInt32 mask = 0;
    for(mask = 0x01;mask <= 0x40;mask *= 2) {
        switch(mask & (resultMask)) {
        case UA_BROWSERESULTMASK_REFERENCETYPEID:
            UA_NodeId_copy(&reference->referenceTypeId, &referenceDescription->referenceTypeId);
            break;

        case UA_BROWSERESULTMASK_ISFORWARD:
            referenceDescription->isForward = !reference->isInverse;
            break;

        case UA_BROWSERESULTMASK_NODECLASS:
            UA_NodeClass_copy(&foundNode->nodeClass, &referenceDescription->nodeClass);
            break;

        case UA_BROWSERESULTMASK_BROWSENAME:
            UA_QualifiedName_copy(&foundNode->browseName, &referenceDescription->browseName);
            break;

        case UA_BROWSERESULTMASK_DISPLAYNAME:
            UA_LocalizedText_copy(&foundNode->displayName, &referenceDescription->displayName);
            break;

        case UA_BROWSERESULTMASK_TYPEDEFINITION:
            if(foundNode->nodeClass != UA_NODECLASS_OBJECT &&
               foundNode->nodeClass != UA_NODECLASS_VARIABLE)
                break;

            for(UA_Int32 i = 0;i < foundNode->referencesSize;i++) {
                UA_ReferenceNode *ref = &foundNode->references[i];
                if(ref->referenceTypeId.identifier.numeric == 40 /* hastypedefinition */) {
                    UA_ExpandedNodeId_copy(&ref->targetId, &referenceDescription->typeDefinition);
                    break;
                }
            }
            break;
        }
    }

    UA_NodeStore_releaseManagedNode(foundNode);
    return UA_SUCCESS;
}

/* singly-linked list */
struct SubRefTypeId {
    UA_NodeId id;
    SLIST_ENTRY(SubRefTypeId) next;
};
SLIST_HEAD(SubRefTypeIdList, SubRefTypeId);

static UA_UInt32 walkReferenceTree(UA_NodeStore *ns, const UA_ReferenceTypeNode *current,
                                   struct SubRefTypeIdList *list) {
    // insert the current referencetype
    struct SubRefTypeId *element;
    UA_alloc((void **)&element, sizeof(struct SubRefTypeId));
    element->id = current->nodeId;
    SLIST_INSERT_HEAD(list, element, next);

    UA_UInt32 count = 1; // the current element

    // walk the tree
    for(UA_Int32 i = 0;i < current->referencesSize;i++) {
        if(current->references[i].referenceTypeId.identifier.numeric == 45 /* HasSubtype */ &&
           current->references[i].isInverse == UA_FALSE) {
            const UA_Node *node;
            if(UA_NodeStore_get(ns, &current->references[i].targetId.nodeId, &node) == UA_SUCCESS
               && node->nodeClass == UA_NODECLASS_REFERENCETYPE) {
                count += walkReferenceTree(ns, (UA_ReferenceTypeNode *)node, list);
                UA_NodeStore_releaseManagedNode(node);
            }
        }
    }
    return count;
}

/* We do not search across namespaces so far. The id of the father-referencetype is returned in the array also. */
static UA_Int32 findSubReferenceTypes(UA_NodeStore *ns, UA_NodeId *rootReferenceType,
                                      UA_NodeId **ids, UA_UInt32 *idcount) {
    struct SubRefTypeIdList list;
    UA_UInt32 count;
    SLIST_INIT(&list);

    // walk the tree
    const UA_ReferenceTypeNode *root;
    if(UA_NodeStore_get(ns, rootReferenceType, (const UA_Node **)&root) != UA_SUCCESS ||
       root->nodeClass != UA_NODECLASS_REFERENCETYPE)
        return UA_ERROR;
    count = walkReferenceTree(ns, root, &list);
    UA_NodeStore_releaseManagedNode((const UA_Node *)root);

    // copy results into an array
    UA_alloc((void **)ids, sizeof(UA_NodeId)*count);
    for(UA_UInt32 i = 0;i < count;i++) {
        struct SubRefTypeId *element = SLIST_FIRST(&list);
        UA_NodeId_copy(&element->id, &(*ids)[i]);
        SLIST_REMOVE_HEAD(&list, next);
        UA_free(element);
    }
    *idcount = count;

    return UA_SUCCESS;
}

/* is this a relevant reference? */
static INLINE UA_Boolean Service_Browse_returnReference(UA_BrowseDescription *browseDescription,
                                                        UA_ReferenceNode     *reference,
                                                        UA_NodeId            *relevantRefTypes,
                                                        UA_UInt32             relevantRefTypesCount) {
    if(reference->isInverse == UA_TRUE &&
       browseDescription->browseDirection == UA_BROWSEDIRECTION_FORWARD)
        return UA_FALSE;
    else if(reference->isInverse == UA_FALSE &&
            browseDescription->browseDirection == UA_BROWSEDIRECTION_INVERSE)
        return UA_FALSE;
    for(UA_UInt32 i = 0;i < relevantRefTypesCount;i++) {
        if(UA_NodeId_equal(&browseDescription->referenceTypeId, &relevantRefTypes[i]) == UA_EQUAL)
            return UA_TRUE;
    }
    return UA_FALSE;
}

/* Return results to a single browsedescription. */
static void Service_Browse_getBrowseResult(UA_NodeStore         *ns,
                                           UA_BrowseDescription *browseDescription,
                                           UA_UInt32             maxReferences,
                                           UA_BrowseResult      *browseResult) {
    const UA_Node *node;
    UA_NodeId     *relevantReferenceTypes = UA_NULL;
    UA_UInt32      relevantReferenceTypesCount = 0;
    if(UA_NodeStore_get(ns, &browseDescription->nodeId, &node) != UA_SUCCESS) {
        browseResult->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    // 0 => unlimited references
    if(maxReferences == 0)
        maxReferences = node->referencesSize;

    // discover the relevant subtypes
    if(!browseDescription->includeSubtypes ||
       findSubReferenceTypes(ns, &browseDescription->referenceTypeId, &relevantReferenceTypes,
                             &relevantReferenceTypesCount) != UA_SUCCESS) {
        if(UA_alloc((void **)&relevantReferenceTypes, sizeof(UA_NodeId)) != UA_SUCCESS) {

            return;
        }
        UA_NodeId_copy(&browseDescription->referenceTypeId, relevantReferenceTypes);
        relevantReferenceTypesCount = 1;
    }

    /* We do not use a linked list but traverse the nodes references list twice
     * (once for counting, once for generating the referencedescriptions). That
     * is much faster than using a linked list, since the references are
     * allocated in a continuous blob and RAM access is predictible/does not
     * miss cache lines so often. TODO: measure with some huge objects! */
    UA_UInt32 refs = 0;
    for(UA_Int32 i = 0;i < node->referencesSize && refs <= maxReferences;i++) {
        if(Service_Browse_returnReference(browseDescription, &node->references[i], relevantReferenceTypes,
                                          relevantReferenceTypesCount))
            refs++;
    }

    // can we return all relevant references at once?
    UA_Boolean finished = UA_TRUE;
    if(refs > maxReferences) {
        refs--;
        finished = UA_FALSE;
    }

    browseResult->referencesSize = refs;
    UA_Array_new((void **)&browseResult->references, refs, &UA_[UA_REFERENCEDESCRIPTION]);

    for(UA_UInt32 i = 0, j = 0;j < refs;i++) {
        if(!Service_Browse_returnReference(browseDescription, &node->references[i], relevantReferenceTypes,
                                           relevantReferenceTypesCount))
            continue;

        if(Service_Browse_getReferenceDescription(ns, &node->references[i], browseDescription->nodeClassMask,
                                                  browseDescription->resultMask,
                                                  &browseResult->references[j]) != UA_SUCCESS)
            browseResult->statusCode = UA_STATUSCODE_UNCERTAINNOTALLNODESAVAILABLE;
        j++;
    }

    if(!finished) {
        // Todo. Set the Statuscode and the continuation point.
    }

    UA_NodeStore_releaseManagedNode(node);
    UA_Array_delete(relevantReferenceTypes, relevantReferenceTypesCount, &UA_[UA_NODEID]);
}

void Service_Browse(UA_Server *server, UA_Session *session,
                    const UA_BrowseRequest *request, UA_BrowseResponse *response) {
    UA_assert(server != UA_NULL && session != UA_NULL && request != UA_NULL && response != UA_NULL);

    if(request->nodesToBrowseSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    if(UA_Array_new((void **)&(response->results), request->nodesToBrowseSize, &UA_[UA_BROWSERESULT])
       != UA_SUCCESS) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
        
    response->resultsSize = request->nodesToBrowseSize;
    for(UA_Int32 i = 0;i < request->nodesToBrowseSize;i++)
        Service_Browse_getBrowseResult(server->nodestore, &request->nodesToBrowse[i],
                                       request->requestedMaxReferencesPerNode, &response->results[i]);
}


void Service_TranslateBrowsePathsToNodeIds(UA_Server *server, UA_Session *session,
                                           const UA_TranslateBrowsePathsToNodeIdsRequest *request,
                                           UA_TranslateBrowsePathsToNodeIdsResponse *response) {
    UA_assert(server != UA_NULL && session != UA_NULL && request != UA_NULL && response != UA_NULL);

    if(request->browsePathsSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    // Allocate space for a correct answer
    response->resultsSize = request->browsePathsSize;
    // _init of the elements is done in Array_new
    if(UA_Array_new((void **)&response->results, request->browsePathsSize, &UA_[UA_BROWSEPATHRESULT])
       != UA_SUCCESS) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    for(UA_Int32 i = 0;i < request->browsePathsSize;i++)
        response->results[i].statusCode = UA_STATUSCODE_BADNOMATCH; //FIXME: implement
}
