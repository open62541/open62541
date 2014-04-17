#include "ua_services.h"

UA_Int32 Service_Browse(SL_Channel *channel, const UA_BrowseRequest *request, UA_BrowseResponse *response) {
	UA_Int32 retval = UA_SUCCESS;
	DBG_VERBOSE(UA_NodeId_printf("BrowseService - view=",&(request->view.viewId)));
	UA_Int32 i = 0;
	for (i=0;request->nodesToBrowseSize > 0 && i<request->nodesToBrowseSize;i++) {
		UA_NodeId_printf("BrowseService - nodesToBrowse=", &(request->nodesToBrowse[i]->nodeId));
	}
	return retval;
}
