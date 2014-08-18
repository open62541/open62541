#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_namespace.h"
#include "ua_services_internal.h"
#include "ua_stack_session.h"
#define CHECKED_ACTION(ACTION, CLEAN_UP, GOTO) do {	\
	status |= ACTION; \
	if(status != UA_SUCCESS) { \
		CLEAN_UP; \
		goto GOTO; \
	} } while(0) \

static UA_AddNodesResult addSingleNode(Application *app, UA_AddNodesItem *item) {
	UA_AddNodesResult result;
	UA_AddNodesResult_init(&result);

	Namespace *parent_ns = UA_indexedList_findValue(app->namespaces, item->parentNodeId.nodeId.ns);
	// TODO: search for namespaceUris and not only ns-ids.
	if(parent_ns == UA_NULL) {
		result.statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID;
		return result;
	}

	Namespace *ns = UA_NULL;
	UA_Boolean nodeid_isnull = UA_NodeId_isNull(&item->requestedNewNodeId.nodeId);

	if(nodeid_isnull) ns = parent_ns;
	else ns = UA_indexedList_findValue(app->namespaces, item->requestedNewNodeId.nodeId.ns);

	if(ns == UA_NULL || item->requestedNewNodeId.nodeId.ns == 0) {
		result.statusCode = UA_STATUSCODE_BADNODEIDREJECTED;
		return result;
	}

	UA_Int32 status = UA_SUCCESS;
	const UA_Node *parent;
	CHECKED_ACTION(Namespace_get(parent_ns, &item->parentNodeId.nodeId, &parent),
				   result.statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID, ret);

	/* if(!nodeid_isnull && Namespace_contains(ns, &item->requestedNewNodeId.nodeId)) { */
	/* 	result.statusCode = UA_STATUSCODE_BADNODEIDEXISTS; */
	/* 	goto ret; */
	/* } */

	/**
	   TODO:

	   1) Check for the remaining conditions 
	   Bad_ReferenceTypeIdInvalid  See Table 166 for the description of this result code.
	   Bad_ReferenceNotAllowed  The reference could not be created because it violates constraints imposed by the data model.
	   Bad_NodeClassInvalid  See Table 166 for the description of this result code.
	   Bad_BrowseNameInvalid  See Table 166 for the description of this result code.
	   Bad_BrowseNameDuplicated  The browse name is not unique among nodes that share the same relationship with the parent.
	   Bad_NodeAttributesInvalid  The node Attributes are not valid for the node class.
	   Bad_TypeDefinitionInvalid  See Table 166 for the description of this result code.
	   Bad_UserAccessDenied  See Table 165 for the description of this result code

	   2) Parse the UA_Node from the ExtensionObject
	   3) Create a new entry in the namespace
	   4) Add the reference to the parent.
	 */

 ret:
	Namespace_releaseManagedNode(parent);
	return result;
}

UA_Int32 Service_AddNodes(UA_Session *session, const UA_AddNodesRequest *request, UA_AddNodesResponse *response) {

	Application *application;
	if(session == UA_NULL)
		return UA_ERROR;	// TODO: Return error message
	UA_Session_getApplicationPointer(session,&application);
	if(application == UA_NULL)
		return UA_ERROR;

	UA_Int32 nodestoaddsize = request->nodesToAddSize;
	if(nodestoaddsize <= 0) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
		response->resultsSize = 0;
		return UA_SUCCESS;
	}

	response->resultsSize = nodestoaddsize;
	UA_alloc((void **)&response->results, sizeof(void *) * nodestoaddsize);
	for(int i = 0; i < nodestoaddsize; i++) {
		DBG_VERBOSE(UA_QualifiedName_printf("service_addnodes - name=", &(request->nodesToAdd[i].browseName)));
		response->results[i] = addSingleNode(application, &request->nodesToAdd[i]);
	}
	response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
	response->diagnosticInfosSize = -1;
	return UA_SUCCESS;
	
}

static UA_Int32 AddSingleReference(UA_Node *node, UA_ReferenceNode *reference) {
	// TODO: Check if reference already exists
	UA_Int32 retval;
	UA_Int32 count = node->referencesSize;
	UA_ReferenceNode *old_refs = node->references;
	UA_ReferenceNode *new_refs;
	if(count < 0) count = 0; 

	retval = UA_alloc((void **)&new_refs, sizeof(UA_ReferenceNode)*(count+1));
	if(retval != UA_SUCCESS)
		return UA_ERROR;
	UA_memcpy(new_refs, old_refs, sizeof(UA_ReferenceNode)*count);
	retval |= UA_ReferenceNode_copy(reference, &new_refs[count]);

	if(retval != UA_SUCCESS) {
		UA_free(new_refs);
		return retval;
	}
	
	node->references = new_refs;
	node->referencesSize = count+1;
	UA_free(old_refs);
	return retval;
}

UA_Int32 AddReference(UA_Node *node, UA_ReferenceNode *reference, Namespace *targetns) {
	UA_Int32 retval = AddSingleReference(node, reference);
	UA_Node *targetnode;
	UA_ReferenceNode inversereference;
	if(retval != UA_SUCCESS || targetns == UA_NULL)
		return retval;

	// Do a copy every time?
	if(Namespace_get(targetns, &reference->targetId.nodeId, (const UA_Node**)&targetnode) != UA_SUCCESS)
		return UA_ERROR;

	inversereference.referenceTypeId = reference->referenceTypeId;
	inversereference.isInverse = !reference->isInverse;
	inversereference.targetId.nodeId = node->nodeId;
	inversereference.targetId.namespaceUri = UA_STRING_NULL;
	inversereference.targetId.serverIndex = 0;
	retval = AddSingleReference(targetnode, &inversereference);
	Namespace_releaseManagedNode(targetnode);

	return retval;
}

UA_Int32 Service_AddReferences(UA_Session *session, const UA_AddReferencesRequest *request, UA_AddReferencesResponse *response) {
	return UA_ERROR;
}
