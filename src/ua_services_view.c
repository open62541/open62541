#include "ua_services.h"
#include "ua_statuscodes.h"

UA_Int32 Service_Browse_getReferenceDescription(Namespace *ns,UA_ReferenceNode* reference,
		UA_UInt32 nodeClassMask ,UA_UInt32 resultMask, UA_ReferenceDescription* referenceDescription)
{
	const UA_Node* foundNode = UA_NULL;
	Namespace_Entry_Lock *lock;
	if(Namespace_get(ns,&reference->targetId.nodeId,&foundNode, &lock)==UA_SUCCESS && foundNode != UA_NULL)
	{
		UA_UInt32 mask = 0;
		referenceDescription->resultMask = 0;
		for (mask = 0x01; mask <= 0x40; mask *= 2) {
			switch (mask & (resultMask)) {
			case UA_BROWSERESULTMASK_REFERENCETYPEID:
				UA_NodeId_copy(
						&reference->referenceTypeId,
						&referenceDescription->referenceTypeId);
				referenceDescription->resultMask |= UA_BROWSERESULTMASK_REFERENCETYPEID;
				break;
			case UA_BROWSERESULTMASK_ISFORWARD:
				referenceDescription->isForward = !reference->isInverse;
				referenceDescription->resultMask |= UA_BROWSERESULTMASK_ISFORWARD;
				break;
			case UA_BROWSERESULTMASK_NODECLASS:
				UA_NodeClass_copy(&foundNode->nodeClass,
						&referenceDescription->nodeClass);
				referenceDescription->resultMask |= UA_BROWSERESULTMASK_NODECLASS;
				break;
			case UA_BROWSERESULTMASK_BROWSENAME:
				UA_QualifiedName_copy(&foundNode->browseName,
						&referenceDescription->browseName);
				referenceDescription->resultMask |= UA_BROWSERESULTMASK_BROWSENAME;
				break;
			case UA_BROWSERESULTMASK_DISPLAYNAME:
				UA_LocalizedText_copy(&foundNode->displayName,
						&referenceDescription->displayName);
				referenceDescription->resultMask |= UA_BROWSERESULTMASK_DISPLAYNAME;
				break;
			case UA_BROWSERESULTMASK_TYPEDEFINITION:
				if (referenceDescription->nodeClass == UA_NODECLASS_OBJECT
						|| referenceDescription->nodeClass
								== UA_NODECLASS_VARIABLE) {
					UA_NodeId_copy(&foundNode->nodeId,
							&referenceDescription->typeDefinition.nodeId);
					//TODO mockup
					referenceDescription->typeDefinition.serverIndex = 0;
					referenceDescription->typeDefinition.namespaceUri.length = 0;

					referenceDescription->resultMask |= UA_BROWSERESULTMASK_TYPEDEFINITION;
				}
				break;
			}
		}
		return UA_SUCCESS;
	}
	return UA_ERROR;
}
UA_Boolean Service_Browse_returnReference(UA_BrowseDescription *browseDescription, UA_ReferenceNode* reference)
{
	UA_Boolean c = UA_FALSE;

	c = c || ((reference->isInverse == UA_TRUE) && (browseDescription->browseDirection == UA_BROWSEDIRECTION_INVERSE));
	c = c || ((reference->isInverse == UA_FALSE) && (browseDescription->browseDirection == UA_BROWSEDIRECTION_FORWARD));
	c = c || (browseDescription->browseDirection == UA_BROWSEDIRECTION_BOTH);
	c = c && &browseDescription->referenceTypeId == UA_NULL;
	c = c || UA_NodeId_equal(&browseDescription->referenceTypeId, &reference->referenceTypeId);
	//TODO subtypes
	return c;
}
UA_Int32 Service_Browse_getBrowseResult(Namespace *ns,UA_BrowseDescription *browseDescription,UA_UInt32 requestedMaxReferencesPerNode, UA_BrowseResult *browseResult)
{
	UA_Int32 retval = UA_SUCCESS;
	const UA_Node* foundNode = UA_NULL;
	Namespace_Entry_Lock *lock;
	if(Namespace_get(ns, &browseDescription->nodeId, &foundNode, &lock) == UA_SUCCESS && foundNode)
	{
		UA_Int32 i = 0;
		UA_list_List referenceDescriptionList;
		UA_list_init(&referenceDescriptionList);
		for(i = 0; i < (foundNode->referencesSize) && ((UA_UInt32)referenceDescriptionList.size < requestedMaxReferencesPerNode); i++)
		{
			if(Service_Browse_returnReference(browseDescription, &foundNode->references[i]) == UA_TRUE)
			{
				UA_ReferenceDescription *referenceDescription = UA_NULL;
				UA_ReferenceDescription_new(&referenceDescription);

				retval |= Service_Browse_getReferenceDescription(ns, &foundNode->references[i],
					browseDescription->nodeClassMask,browseDescription->resultMask,referenceDescription);
				if(retval == UA_SUCCESS){
					retval |= UA_list_addPayloadToBack(&referenceDescriptionList,referenceDescription);
				}
			}
		}
		//create result array and copy all data from list into it
		UA_Array_new((void**) &browseResult->references, referenceDescriptionList.size,
				&UA_.types[UA_REFERENCEDESCRIPTION]);

		browseResult->referencesSize = referenceDescriptionList.size;
		UA_list_Element *element = referenceDescriptionList.first;
		UA_Int32 l = 0;

		for (l = 0; l < referenceDescriptionList.size; l++) {
			UA_ReferenceDescription_copy(
					(UA_ReferenceDescription*) element->payload,
					&browseResult->references[l]);
			element = element->next;
		}
		UA_list_destroy(&referenceDescriptionList,
				(UA_list_PayloadVisitor) UA_ReferenceDescription_delete);
		return retval;
	}
	return UA_ERROR;
}
UA_Int32 Service_Browse(SL_Channel *channel, const UA_BrowseRequest *request,
		UA_BrowseResponse *response) {
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE(UA_NodeId_printf("BrowseService - view=", &request->view.viewId));
	UA_Int32 i = 0;
	Namespace *ns = UA_indexedList_findValue(
			channel->session->application->namespaces,
			request->nodesToBrowse[i].nodeId.namespace);
	//TODO request->view not used atm
	if(ns)
	{
		UA_Array_new((void**) &(response->results), request->nodesToBrowseSize,
				&UA_.types[UA_BROWSERESULT]);
		response->resultsSize = request->nodesToBrowseSize;

		for(i=0; i < request->nodesToBrowseSize; i++)
		{
			retval |= Service_Browse_getBrowseResult(ns,
					request->nodesToBrowse,
					request->requestedMaxReferencesPerNode,
					&response->results[i]);
		}
		return retval;
		response->diagnosticInfosSize = 0;
		response->diagnosticInfos = UA_NULL;
		//TODO fill Diagnostic info array
	}
	return UA_ERROR;
}
/*
UA_Int32 Service_Browse(SL_Channel *channel, const UA_BrowseRequest *request,
		UA_BrowseResponse *response) {
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE(UA_NodeId_printf("BrowseService - view=", &request->view.viewId));
	UA_Int32 i = 0;

	UA_Array_new((void**) &(response->results), request->nodesToBrowseSize,
			&UA_.types[UA_BROWSERESULT]);
	response->resultsSize = request->nodesToBrowseSize;

	for (i = 0;
			request->nodesToBrowseSize > 0 && i < request->nodesToBrowseSize;
			i++) {
		UA_list_List referencesToReturn;
		UA_list_init(&referencesToReturn);
		UA_UInt32 j = 0;
		Namespace *ns = UA_indexedList_findValue(
				channel->session->application->namespaces,
				request->nodesToBrowse[i].nodeId.namespace);
		const UA_Node *node;
		Namespace_Entry_Lock *lock;
		Namespace_get(ns, &request->nodesToBrowse[i].nodeId, (&node), &lock);

		if (request->requestedMaxReferencesPerNode > 0) {
			UA_Int32 k = 0;
			while (j < request->requestedMaxReferencesPerNode
					&& k < node->referencesSize) {
				//TODO include subtypes missing how to determine a subtype?
				if (((node->references[k].isInverse)
						&& ((request->nodesToBrowse->browseDirection
								== UA_BROWSEDIRECTION_INVERSE)
								|| (request->nodesToBrowse->browseDirection
										== UA_BROWSEDIRECTION_BOTH)))
						|| (!node->references[k].isInverse
								&& ((request->nodesToBrowse->browseDirection
										== UA_BROWSEDIRECTION_BOTH)
										|| (request->nodesToBrowse->browseDirection
												== UA_BROWSEDIRECTION_FORWARD)))) {
					UA_ReferenceDescription *rd;
					UA_ReferenceDescription_new(&rd);
					const UA_Node* targetNode;
					Namespace_Entry_Lock *lock1;
					UA_Int32 ret = Namespace_get(ns, &node->references[k].targetId.nodeId,
							&targetNode, &lock1);

					if (ret==UA_SUCCESS &&targetNode
							&& ((request->nodesToBrowse->nodeClassMask
									& targetNode->nodeClass) || (request->nodesToBrowse->nodeClassMask == 0))) {
						UA_UInt32 mask = 0;
						rd->resultMask = 0;
						for (mask = 0x01; mask <= 0x40; mask *= 2) {
							switch (mask & (request->nodesToBrowse->resultMask)) {
							case 1:
								UA_NodeId_copy(
										&node->references[k].referenceTypeId,
										&rd->referenceTypeId);
								rd->resultMask |= (1 << 0);
								break;
							case 2:
								rd->isForward = !node->references[k].isInverse;
								rd->resultMask |= (1 << 1);
								break;
							case 4:
								UA_NodeClass_copy(&targetNode->nodeClass,
										&rd->nodeClass);
								rd->resultMask |= (1 << 2);
								break;
							case 8:
								UA_QualifiedName_copy(&targetNode->browseName,
										&rd->browseName);
								rd->resultMask |= (1 << 3);
								break;
							case 16:
								UA_LocalizedText_copy(&targetNode->displayName,
										&rd->displayName);
								rd->resultMask |= (1 << 4);
								break;
							case 32:
								if (rd->nodeClass == UA_NODECLASS_OBJECT
										|| rd->nodeClass
												== UA_NODECLASS_VARIABLE) {
									UA_NodeId_copy(&targetNode->nodeId,
											&rd->typeDefinition);
									rd->resultMask |= (1 << 5);
								}
								break;
							}
						}
						UA_list_addPayloadToBack(&referencesToReturn, rd);
						j++;
					}
				}
				k++;
			}
		}
		UA_Array_new((void**) &response->results[i].references, j,
				&UA_.types[UA_REFERENCEDESCRIPTION]);
		response->results[i].referencesSize = j;
		UA_list_Element *element = referencesToReturn.first;
		UA_UInt32 l = 0;

		for (l = 0; l < j; l++) {
			UA_ReferenceDescription_copy(
					(UA_ReferenceDescription*) element->payload,
					&response->results[i].references[l]);
			element = element->next;
		}
		UA_list_destroy(&referencesToReturn,
				(UA_list_PayloadVisitor) UA_ReferenceDescription_delete);
	}
	response->diagnosticInfosSize = 0;

	return retval;

}
*/
//UA_BrowseResult br;
//UA_ReferenceNode rn;

//UA_Array_new(&response->results,j,UA_BROWSERESULT_NS0)
//UA_NodeId_printf("BrowseService - nodesToBrowse=", &request->nodesToBrowse[i].nodeId);

UA_Int32 Service_TranslateBrowsePathsToNodeIds(SL_Channel *channel,
		const UA_TranslateBrowsePathsToNodeIdsRequest *request,
		UA_TranslateBrowsePathsToNodeIdsResponse *response) {
	UA_Int32 retval = UA_SUCCESS;

	DBG_VERBOSE(printf("TranslateBrowsePathsToNodeIdsService - %i path(s)", request->browsePathsSize));
//Allocate space for a correct answer
	UA_Array_new((void**) &response->results, request->browsePathsSize,
			&UA_.types[UA_BROWSEPATHRESULT]);

	response->resultsSize = request->browsePathsSize;

	for (UA_Int32 i = 0; i < request->browsePathsSize; i++) {
		UA_BrowsePathResult_init(&response->results[i]);
//FIXME: implement
		response->results[i].statusCode = UA_STATUSCODE_BADQUERYTOOCOMPLEX;
	}

	return retval;
}
