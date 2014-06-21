#include "ua_services.h"
#include "ua_statuscodes.h"

UA_Int32 Service_Browse_getReferenceDescription(Namespace *ns, UA_ReferenceNode* reference, UA_UInt32 nodeClassMask,
												UA_UInt32 resultMask, UA_ReferenceDescription* referenceDescription) {
	const UA_Node* foundNode;
	Namespace_Entry_Lock *lock;
	if(Namespace_get(ns,&reference->targetId.nodeId,&foundNode, &lock) != UA_SUCCESS)
		return UA_ERROR;

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
			if (referenceDescription->nodeClass == UA_NODECLASS_OBJECT ||
				referenceDescription->nodeClass == UA_NODECLASS_VARIABLE) {
				UA_NodeId_copy(&foundNode->nodeId, &referenceDescription->typeDefinition.nodeId);
				//TODO mockup
				referenceDescription->typeDefinition.serverIndex = 0;
				referenceDescription->typeDefinition.namespaceUri.length = 0;
			}
			break;
		}
	}
	
	Namespace_Entry_Lock_release(lock);
	return UA_SUCCESS;
}

static inline UA_Boolean Service_Browse_returnReference(UA_BrowseDescription *browseDescription, UA_ReferenceNode* reference) {
	UA_Boolean c = UA_FALSE;

	c = c || ((reference->isInverse == UA_TRUE) && (browseDescription->browseDirection == UA_BROWSEDIRECTION_INVERSE));
	c = c || ((reference->isInverse == UA_FALSE) && (browseDescription->browseDirection == UA_BROWSEDIRECTION_FORWARD));
	c = c || (browseDescription->browseDirection == UA_BROWSEDIRECTION_BOTH);
	c = c && &browseDescription->referenceTypeId == UA_NULL;
	c = c || UA_NodeId_equal(&browseDescription->referenceTypeId, &reference->referenceTypeId);
	//TODO subtypes
	return c;
}

static void Service_Browse_getBrowseResult(Namespace *ns,UA_BrowseDescription *browseDescription,
										   UA_UInt32 maxReferences, UA_BrowseResult *browseResult) {
	const UA_Node* node;
	Namespace_Entry_Lock *lock;

	if(Namespace_get(ns, &browseDescription->nodeId, &node, &lock) != UA_SUCCESS) {
		browseResult->statusCode = UA_STATUSCODE_BADNODEIDUNKNOWN;
		return;
	}

	// 0 => unlimited references
	if(maxReferences == 0)
		maxReferences = node->referencesSize;

	/* We do not use a linked list but traverse the nodes references list twice
	 * (once for counting, once for generating the referencedescriptions). That
	 * is much faster than using a linked list, since the references are
	 * allocated in a continuous blob and RAM access is predictible/does not
	 * miss cache lines so often. */
	UA_UInt32 refs = 0;
	for(UA_Int32 i=0;i < node->referencesSize && refs <= maxReferences;i++) {
		if(Service_Browse_returnReference(browseDescription, &node->references[i]))
			refs++;
	}

	// can we return all relevant references in one go?
	UA_Boolean finished = UA_TRUE;
	if(refs > maxReferences) {
		refs--;
		finished = UA_FALSE;
	}
	
	browseResult->referencesSize = refs;
	UA_Array_new((void**) &browseResult->references, refs, &UA_.types[UA_REFERENCEDESCRIPTION]);
	
	for(UA_UInt32 i = 0, j=0; j < refs; i++) {
		if(!Service_Browse_returnReference(browseDescription, &node->references[i]))
			continue;
		
		if(Service_Browse_getReferenceDescription(ns, &node->references[i], browseDescription->nodeClassMask,
												  browseDescription->resultMask, &browseResult->references[j]) != UA_SUCCESS)
			browseResult->statusCode = UA_STATUSCODE_UNCERTAINNOTALLNODESAVAILABLE;
		j++;
	}

	if(!finished) {
		// Todo. Set the Statuscode and the continuation point.
	}
	
	Namespace_Entry_Lock_release(lock);
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
