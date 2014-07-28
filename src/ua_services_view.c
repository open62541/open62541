#include "ua_services.h"
#include "ua_statuscodes.h"

UA_Int32 Service_Browse_getReferenceDescription(Namespace *ns, UA_ReferenceNode* reference, UA_UInt32 nodeClassMask,
												UA_UInt32 resultMask, UA_ReferenceDescription* referenceDescription) {
	const UA_Node* foundNode;
	if(Namespace_get(ns,&reference->targetId.nodeId,&foundNode) != UA_SUCCESS)
		return UA_ERROR;

	UA_NodeId_copy(&foundNode->nodeId, &referenceDescription->nodeId.nodeId);
	//TODO ExpandedNodeId is a mockup
	referenceDescription->nodeId.serverIndex = 0;
	referenceDescription->nodeId.namespaceUri.length = -1;

	UA_UInt32 mask = 0;
	for (mask = 0x01; mask <= 0x40; mask *= 2) {
		switch (mask & (resultMask)) {
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
			if (foundNode->nodeClass != UA_NODECLASS_OBJECT &&
				foundNode->nodeClass != UA_NODECLASS_VARIABLE)
				break;

			for(UA_Int32 i = 0;i<foundNode->referencesSize;i++) {
				UA_ReferenceNode *ref = &foundNode->references[i];
				if(ref->referenceTypeId.identifier.numeric == 40 /* hastypedefinition */) {
					UA_ExpandedNodeId_copy(&ref->targetId, &referenceDescription->typeDefinition);
					break;
				}
			}
			break;
		}
	}
	
	Namespace_releaseManagedNode(foundNode);
	return UA_SUCCESS;
}

/* singly-linked list */
struct SubRefTypeId {
	UA_NodeId id;
	UA_SLIST_ENTRY(SubRefTypeId) next;
};
UA_SLIST_HEAD(SubRefTypeIdList, SubRefTypeId);

UA_UInt32 walkReferenceTree(Namespace *ns, const UA_ReferenceTypeNode *current, struct SubRefTypeIdList *list) {
	// insert the current referencetype
	struct SubRefTypeId *element;
	UA_alloc((void**)&element, sizeof(struct SubRefTypeId));
	element->id = current->nodeId;
	UA_SLIST_INSERT_HEAD(list, element, next);

	UA_UInt32 count = 1; // the current element

	// walk the tree
	for(UA_Int32 i = 0;i < current->referencesSize;i++) {
		if(current->references[i].referenceTypeId.identifier.numeric == 45 /* HasSubtype */ &&
		   current->references[i].isInverse == UA_FALSE) {
			const UA_Node *node;
			if(Namespace_get(ns, &current->references[i].targetId.nodeId, &node) == UA_SUCCESS &&
			   node->nodeClass == UA_NODECLASS_REFERENCETYPE) {
				count += walkReferenceTree(ns,(UA_ReferenceTypeNode*)node, list);
				Namespace_releaseManagedNode(node);
			}
		}
	}
	return count;
}

/* We do not search across namespaces so far. The id of the father-referencetype is returned in the array also. */
static UA_Int32 findSubReferenceTypes(Namespace *ns, UA_NodeId *rootReferenceType, UA_NodeId **ids, UA_UInt32 *idcount) {
	struct SubRefTypeIdList list;
	UA_SLIST_INIT(&list);

	// walk the tree
	const UA_ReferenceTypeNode *root;
	if(Namespace_get(ns, rootReferenceType, (const UA_Node**)&root) != UA_SUCCESS ||
	   root->nodeClass != UA_NODECLASS_REFERENCETYPE)
		return UA_ERROR;
	UA_UInt32 count = walkReferenceTree(ns, root, &list);
	Namespace_releaseManagedNode((const UA_Node*) root);

	// copy results into an array
	UA_alloc((void**) ids, sizeof(UA_NodeId)*count);
	for(UA_UInt32 i = 0; i < count;i++) {
		struct SubRefTypeId *element = UA_SLIST_FIRST(&list);
		UA_NodeId_copy(&element->id, &(*ids)[i]);
		UA_SLIST_REMOVE_HEAD(&list, next);
		UA_free(element);
	}
	*idcount = count;

	return UA_SUCCESS;
}

/* is this a relevant reference? */
static inline UA_Boolean Service_Browse_returnReference(UA_BrowseDescription *browseDescription, UA_ReferenceNode* reference,
														UA_NodeId *relevantRefTypes, UA_UInt32 relevantRefTypesCount) {
	if (reference->isInverse == UA_TRUE && browseDescription->browseDirection == UA_BROWSEDIRECTION_FORWARD)
		return UA_FALSE;
	else if (reference->isInverse == UA_FALSE && browseDescription->browseDirection == UA_BROWSEDIRECTION_INVERSE)
		return UA_FALSE;
	for(UA_UInt32 i = 0; i<relevantRefTypesCount;i++) {
		if(UA_NodeId_equal(&browseDescription->referenceTypeId, &relevantRefTypes[i]) == UA_EQUAL)
			return UA_TRUE;
	}
	return UA_FALSE;
}

/* Return results to a single browsedescription. */
static void Service_Browse_getBrowseResult(Namespace *ns, UA_BrowseDescription *browseDescription,
										   UA_UInt32 maxReferences, UA_BrowseResult *browseResult) {
	const UA_Node* node;
	if(Namespace_get(ns, &browseDescription->nodeId, &node) != UA_SUCCESS) {
		browseResult->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return;
	}

	// 0 => unlimited references
	if(maxReferences == 0)
		maxReferences = node->referencesSize;

	// discover the relevant subtypes
	UA_NodeId *relevantReferenceTypes;
	UA_UInt32 relevantReferenceTypesCount=0;
	if(!browseDescription->includeSubtypes ||
	   findSubReferenceTypes(ns, &browseDescription->referenceTypeId, &relevantReferenceTypes, &relevantReferenceTypesCount) != UA_SUCCESS) {
		UA_alloc((void**)&relevantReferenceTypes, sizeof(UA_NodeId));
		UA_NodeId_copy(&browseDescription->referenceTypeId, relevantReferenceTypes);
		relevantReferenceTypesCount = 1;
	}

	/* We do not use a linked list but traverse the nodes references list twice
	 * (once for counting, once for generating the referencedescriptions). That
	 * is much faster than using a linked list, since the references are
	 * allocated in a continuous blob and RAM access is predictible/does not
	 * miss cache lines so often. TODO: measure with some huge objects! */
	UA_UInt32 refs = 0;
	for(UA_Int32 i=0;i < node->referencesSize && refs <= maxReferences;i++) {
		if(Service_Browse_returnReference(browseDescription, &node->references[i], relevantReferenceTypes, relevantReferenceTypesCount))
			refs++;
	}

	// can we return all relevant references at once?
	UA_Boolean finished = UA_TRUE;
	if(refs > maxReferences) {
		refs--;
		finished = UA_FALSE;
	}
	
	browseResult->referencesSize = refs;
	UA_Array_new((void**) &browseResult->references, refs, &UA_.types[UA_REFERENCEDESCRIPTION]);
	
	for(UA_UInt32 i = 0, j=0; j < refs; i++) {
		if(!Service_Browse_returnReference(browseDescription, &node->references[i], relevantReferenceTypes, relevantReferenceTypesCount))
			continue;
		
		if(Service_Browse_getReferenceDescription(ns, &node->references[i], browseDescription->nodeClassMask,
												  browseDescription->resultMask, &browseResult->references[j]) != UA_SUCCESS)
			browseResult->statusCode = UA_STATUSCODE_UNCERTAINNOTALLNODESAVAILABLE;
		j++;
	}

	if(!finished) {
		// Todo. Set the Statuscode and the continuation point.
	}
	
	Namespace_releaseManagedNode(node);
	UA_Array_delete(relevantReferenceTypes, relevantReferenceTypesCount, &UA_.types[UA_NODEID]);
}

UA_Int32 Service_Browse(SL_Channel *channel, const UA_BrowseRequest *request, UA_BrowseResponse *response) {
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE(UA_NodeId_printf("BrowseService - view=", &request->view.viewId));

	//TODO request->view not used atm
	UA_Array_new((void**) &(response->results), request->nodesToBrowseSize, &UA_.types[UA_BROWSERESULT]);
	response->resultsSize = request->nodesToBrowseSize;

	for(UA_Int32 i=0; i < request->nodesToBrowseSize; i++) {
		Namespace *ns = UA_indexedList_findValue(channel->session->application->namespaces,
												 request->nodesToBrowse[i].nodeId.namespace);
		if(ns == UA_NULL) {
			response->results[i].statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
			continue;
		}
		
		// Service_Browse_getBrowseResult has no return value. All errors are resolved internally.
		Service_Browse_getBrowseResult(ns, &request->nodesToBrowse[i],
									   request->requestedMaxReferencesPerNode, &response->results[i]);
	}

	//TODO fill Diagnostic info array
	response->diagnosticInfosSize = 0;
	response->diagnosticInfos = UA_NULL;
	return retval;
}

UA_Int32 Service_TranslateBrowsePathsToNodeIds(SL_Channel *channel, const UA_TranslateBrowsePathsToNodeIdsRequest *request,
											   UA_TranslateBrowsePathsToNodeIdsResponse *response) {
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE(printf("TranslateBrowsePathsToNodeIdsService - %i path(s)", request->browsePathsSize));

	// Allocate space for a correct answer
	response->resultsSize = request->browsePathsSize;
	// _init of the elements is done in Array_new
	UA_Array_new((void**) &response->results, request->browsePathsSize, &UA_.types[UA_BROWSEPATHRESULT]);

	for (UA_Int32 i = 0; i < request->browsePathsSize; i++) {
		//FIXME: implement
		response->results[i].statusCode = UA_STATUSCODE_BADNOMATCH;
	}

	return retval;
}
