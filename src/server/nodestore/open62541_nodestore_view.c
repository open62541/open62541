/*
 * service_view_implementation.c
 *
 *  Created on: Oct 27, 2014
 *      Author: opcua
 */
#include "ua_nodestoreExample.h"
#include "../ua_services.h"
#include "open62541_nodestore.h"
#include "../ua_services.h"
#include "ua_namespace_0.h"
#include "ua_util.h"

UA_Int32 Service_Browse_getReferenceDescription(UA_NodeStoreExample *ns, UA_ReferenceNode *reference,
                                                UA_UInt32 nodeClassMask, UA_UInt32 resultMask,
                                                UA_ReferenceDescription *referenceDescription) {
    const UA_Node *foundNode;
    if(UA_NodeStoreExample_get(ns, &reference->targetId.nodeId, &foundNode) != UA_STATUSCODE_GOOD)
    	return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId_copy(&foundNode->nodeId, &referenceDescription->nodeId.nodeId);
    //TODO ExpandedNodeId is a mockup
    referenceDescription->nodeId.serverIndex = 0;
    referenceDescription->nodeId.namespaceUri.length = -1;

    /* UA_UInt32 mask = 0; */
    /* for(mask = 0x01;mask <= 0x40;mask *= 2) { */
    /*     switch(mask & (resultMask)) { */
    if(resultMask & UA_BROWSERESULTMASK_REFERENCETYPEID)
        UA_NodeId_copy(&reference->referenceTypeId, &referenceDescription->referenceTypeId);
    if(resultMask & UA_BROWSERESULTMASK_ISFORWARD)
        referenceDescription->isForward = !reference->isInverse;
    if(resultMask & UA_BROWSERESULTMASK_NODECLASS)
        UA_NodeClass_copy(&foundNode->nodeClass, &referenceDescription->nodeClass);
    if(resultMask & UA_BROWSERESULTMASK_BROWSENAME)
        UA_QualifiedName_copy(&foundNode->browseName, &referenceDescription->browseName);
    if(resultMask & UA_BROWSERESULTMASK_DISPLAYNAME)
        UA_LocalizedText_copy(&foundNode->displayName, &referenceDescription->displayName);
    if(resultMask & UA_BROWSERESULTMASK_TYPEDEFINITION) {
        if(foundNode->nodeClass != UA_NODECLASS_OBJECT &&
           foundNode->nodeClass != UA_NODECLASS_VARIABLE)
            goto end;

        for(UA_Int32 i = 0;i < foundNode->referencesSize;i++) {
            UA_ReferenceNode *ref = &foundNode->references[i];
            if(ref->referenceTypeId.identifier.numeric == 40 /* hastypedefinition */) {
                UA_ExpandedNodeId_copy(&ref->targetId, &referenceDescription->typeDefinition);
                goto end;
            }
        }
    }
 end:
    UA_NodeStoreExample_releaseManagedNode(foundNode);
    return UA_STATUSCODE_GOOD;
}

/* singly-linked list */
struct SubRefTypeId {
    UA_NodeId id;
    SLIST_ENTRY(SubRefTypeId) next;
};
SLIST_HEAD(SubRefTypeIdList, SubRefTypeId);

static UA_UInt32 walkReferenceTree(UA_NodeStoreExample *ns, const UA_ReferenceTypeNode *current,
                                   struct SubRefTypeIdList *list) {
    // insert the current referencetype
    struct SubRefTypeId *element = UA_alloc(sizeof(struct SubRefTypeId));
    element->id = current->nodeId;
    SLIST_INSERT_HEAD(list, element, next);

    UA_UInt32 count = 1; // the current element

    // walk the tree
    for(UA_Int32 i = 0;i < current->referencesSize;i++) {
        if(current->references[i].referenceTypeId.identifier.numeric == 45 /* HasSubtype */ &&
           current->references[i].isInverse == UA_FALSE) {
            const UA_Node *node;
            if(UA_NodeStoreExample_get(ns, &current->references[i].targetId.nodeId, &node) == UA_STATUSCODE_GOOD
               && node->nodeClass == UA_NODECLASS_REFERENCETYPE) {
                count += walkReferenceTree(ns, (UA_ReferenceTypeNode *)node, list);
                UA_NodeStoreExample_releaseManagedNode(node);
            }
        }
    }
    return count;
}

/* We do not search across namespaces so far. The id of the father-referencetype is returned in the array also. */
static UA_Int32 findSubReferenceTypes(UA_NodeStoreExample *ns, UA_NodeId *rootReferenceType,
                                      UA_NodeId **ids, UA_UInt32 *idcount) {
    struct SubRefTypeIdList list;
    UA_UInt32 count;
    SLIST_INIT(&list);

    // walk the tree
    const UA_ReferenceTypeNode *root;

    if(UA_NodeStoreExample_get(ns, rootReferenceType, (const UA_Node **)&root) != UA_STATUSCODE_GOOD ||
       root->nodeClass != UA_NODECLASS_REFERENCETYPE)
        return UA_STATUSCODE_BADINTERNALERROR;
    count = walkReferenceTree(ns, root, &list);
    UA_NodeStoreExample_releaseManagedNode((const UA_Node *)root);

    // copy results into an array
    *ids = UA_alloc(sizeof(UA_NodeId)*count);
    for(UA_UInt32 i = 0;i < count;i++) {
        struct SubRefTypeId *element = SLIST_FIRST(&list);
        UA_NodeId_copy(&element->id, &(*ids)[i]);
        SLIST_REMOVE_HEAD(&list, next);
        UA_free(element);
    }
    *idcount = count;

    return UA_STATUSCODE_GOOD;
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
static void NodeStore_Browse_getBrowseResult(UA_NodeStoreExample         *ns,
                                           UA_BrowseDescription *browseDescription,
                                           UA_UInt32             maxReferences,
                                           UA_BrowseResult      *browseResult) {
    const UA_Node *node;
    UA_NodeId     *relevantReferenceTypes = UA_NULL;
    UA_UInt32      relevantReferenceTypesCount = 0;
    if(UA_NodeStoreExample_get(ns, &browseDescription->nodeId, &node) != UA_STATUSCODE_GOOD) {
        browseResult->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
        return;
    }

    // 0 => unlimited references
    if(maxReferences == 0)
        maxReferences = node->referencesSize;

    // discover the relevant subtypes
    if(!browseDescription->includeSubtypes ||
       findSubReferenceTypes(ns, &browseDescription->referenceTypeId, &relevantReferenceTypes,
                             &relevantReferenceTypesCount) != UA_STATUSCODE_GOOD) {
        if(!(relevantReferenceTypes = UA_alloc(sizeof(UA_NodeId)))) {
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
                                                  browseDescription->resultMask, &browseResult->references[j]) != UA_STATUSCODE_GOOD)
            browseResult->statusCode = UA_STATUSCODE_UNCERTAINNOTALLNODESAVAILABLE;
        j++;
    }

    if(!finished) {
        // Todo. Set the Statuscode and the continuation point.
    }

    UA_NodeStoreExample_releaseManagedNode(node);
    UA_Array_delete(relevantReferenceTypes, relevantReferenceTypesCount, &UA_[UA_NODEID]);
}

UA_Int32 open62541NodeStore_BrowseNodes(UA_BrowseDescription *browseDescriptions,UA_UInt32 *indices,UA_UInt32 indicesSize, UA_UInt32 requestedMaxReferencesPerNode,
		UA_BrowseResult *browseResults,
		UA_DiagnosticInfo *diagnosticInfos){


	for(UA_UInt32 i = 0; i < indicesSize; i++){
		NodeStore_Browse_getBrowseResult(Nodestore_get(),&browseDescriptions[indices[i]],requestedMaxReferencesPerNode, &browseResults[indices[i]]);
	}
	return UA_STATUSCODE_GOOD;
}
