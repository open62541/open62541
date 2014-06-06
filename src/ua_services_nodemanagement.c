#include "ua_services.h"
#include "ua_statuscodes.h"
#include "ua_namespace.h"

#define CHECKED_ACTION(ACTION, CLEAN_UP, GOTO) do {	\
	status |= ACTION; \
	if(status != UA_SUCCESS) { \
		CLEAN_UP; \
		goto GOTO; \
	} } while(0) \

static UA_AddNodesResult addSingleNode(Application *app, UA_AddNodesItem *item) {
	UA_AddNodesResult result;
	UA_AddNodesResult_init(&result);

	Namespace *parent_ns = UA_indexedList_findValue(app->namespaces, item->parentNodeId.nodeId.namespace);
	// TODO: search for namespaceUris and not only ns-ids.
	if(parent_ns == UA_NULL) {
		result.statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID;
		return result;
	}

	Namespace *ns = UA_NULL;
	UA_Boolean nodeid_isnull = UA_NodeId_isNull(&item->requestedNewNodeId.nodeId);

	if(nodeid_isnull) ns = parent_ns;
	else ns = UA_indexedList_findValue(app->namespaces, item->requestedNewNodeId.nodeId.namespace);

	if(ns == UA_NULL || item->requestedNewNodeId.nodeId.namespace == 0) {
		result.statusCode = UA_STATUSCODE_BADNODEIDREJECTED;
		return result;
	}

	UA_Int32 status = UA_SUCCESS;
	const UA_Node *parent;
	Namespace_Entry_Lock *parent_lock = UA_NULL;

	CHECKED_ACTION(Namespace_get(parent_ns, &item->parentNodeId.nodeId, &parent, &parent_lock),
				   result.statusCode = UA_STATUSCODE_BADPARENTNODEIDINVALID, ret);

	if(!nodeid_isnull && Namespace_contains(ns, &item->requestedNewNodeId.nodeId)) {
		result.statusCode = UA_STATUSCODE_BADNODEIDEXISTS;
		goto ret;
	}

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
	Namespace_Entry_Lock_release(parent_lock);
	return result;
}

UA_Int32 Service_AddNodes(SL_Channel *channel, const UA_AddNodesRequest *request, UA_AddNodesResponse *response) {
	if(channel->session == UA_NULL || channel->session->application == UA_NULL)
		return UA_ERROR;	// TODO: Return error message

	int nodestoaddsize = request->nodesToAddSize;
	if(nodestoaddsize <= 0) {
		response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
		response->resultsSize = 0;
		return UA_SUCCESS;
	}

	response->resultsSize = nodestoaddsize;
	UA_alloc((void **)&response->results, sizeof(void *) * nodestoaddsize);
	for(int i = 0; i < nodestoaddsize; i++) {
		DBG_VERBOSE(UA_QualifiedName_printf("service_addnodes - name=", &(request->nodesToAdd[i].browseName)));
		response->results[i] = addSingleNode(channel->session->application, &request->nodesToAdd[i]);
	}
	response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
	response->diagnosticInfosSize = -1;
	return UA_SUCCESS;
	
}
