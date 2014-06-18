#include "ua_services.h"
#include "ua_statuscodes.h"

UA_Int32 Service_Browse(SL_Channel *channel, const UA_BrowseRequest *request,
		UA_BrowseResponse *response) {
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE(UA_NodeId_printf("BrowseService - view=", &request->view.viewId));
	UA_Int32 i = 0;

	UA_Array_new((void**)&(response->results), request->nodesToBrowseSize, &UA_.types[UA_BROWSERESULT]);
	response->resultsSize = request->nodesToBrowseSize;

	for (i = 0;
			request->nodesToBrowseSize > 0 && i < request->nodesToBrowseSize;
			i++)
	{
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
					const UA_Node* foundNode;
						Namespace_Entry_Lock *lock1;
						Namespace_get(ns, &node->references[k].targetId.nodeId,
								&foundNode, &lock1);

					if (foundNode && (request->nodesToBrowse->nodeClassMask & foundNode->nodeClass)) {
						if (request->nodesToBrowse->resultMask & (1 << 0)) {
							UA_NodeId_copy(&node->references[k].referenceTypeId,
									&rd->referenceTypeId);
						}
						if (request->nodesToBrowse->resultMask & (1 << 1)) {
							rd->isForward = !node->references[k].isInverse;
						}

						UA_ExpandedNodeId_copy(&node->references[k].targetId,
								&rd->nodeId);


						if (request->nodesToBrowse->resultMask & (1 << 2)) {
							UA_NodeClass_copy(&foundNode->nodeClass,
									&rd->nodeClass);
						}
						if (request->nodesToBrowse->resultMask & (1 << 3)) {
							UA_QualifiedName_copy(&foundNode->browseName,
									&rd->browseName);
						}
						if (request->nodesToBrowse->resultMask & (1 << 4)) {
							UA_LocalizedText_copy(&foundNode->displayName,
									&rd->displayName);
						}
						if (rd->nodeClass == UA_NODECLASS_OBJECT
								|| rd->nodeClass == UA_NODECLASS_VARIABLE) {
							//FIXME correct? to use that nodeId for typedefinition?
							//UA_ExpandedNodeId_copy(&foundNode->references->, &rd->typeDefinition);
						} else {
							//rd->typeDefinition = UA_NULL;
						}
						UA_list_addPayloadToBack(&referencesToReturn,
								&node->references[k]);

						j++;
					}
				}
				k++;
			}
		}
		UA_Array_new((void**)&response->results[i].references,j,&UA_.types[UA_REFERENCEDESCRIPTION]);
		response->results[i].referencesSize = j;
		UA_list_Element *element = referencesToReturn.first;
		UA_UInt32 l = 0;

		for(l=0;l<j;l++){
			UA_ReferenceDescription_copy((UA_ReferenceDescription*)element->payload, &response->results[i].references[l]);
			element = element->next;
		}
		UA_list_destroy(&referencesToReturn,(UA_list_PayloadVisitor)UA_ReferenceDescription_delete);
	}
return retval;
}
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
