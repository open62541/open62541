#include "ua_services.h"
#include "ua_statuscodes.h"

UA_Int32 Service_Browse(SL_Channel *channel, const UA_BrowseRequest *request, UA_BrowseResponse *response) {
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE(UA_NodeId_printf("BrowseService - view=", &request->view.viewId));
	UA_Int32 i = 0;
	for (i=0;request->nodesToBrowseSize > 0 && i<request->nodesToBrowseSize;i++) {
		UA_NodeId_printf("BrowseService - nodesToBrowse=", &request->nodesToBrowse[i].nodeId);
	}
	return retval;
}

UA_Int32 Service_TranslateBrowsePathsToNodeIds(SL_Channel *channel, const UA_TranslateBrowsePathsToNodeIdsRequest *request, UA_TranslateBrowsePathsToNodeIdsResponse *response)
{
	UA_Int32 retval = UA_SUCCESS;

	DBG_VERBOSE(printf("TranslateBrowsePathsToNodeIdsService - %i path(s)", request->browsePathsSize));
	//Allocate space for a correct answer
	UA_Array_new((void**)&response->results, request->browsePathsSize, UA_BROWSEPATHRESULT);

	response->resultsSize = request->browsePathsSize;

	for(UA_Int32 i = 0;i<request->browsePathsSize;i++){
		UA_BrowsePathResult_init(&response->results[i]);
		//FIXME: implement
		response->results[i].statusCode = UA_STATUSCODE_BADQUERYTOOCOMPLEX;
	}

	return retval;
}
