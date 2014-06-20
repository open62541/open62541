#include "ua_services.h"
#include "ua_statuscodes.h"

//#if 0
/* Activate once the infrastructure for pushing events is in place. */

UA_Int32 Service_CreateMonitoredItems(SL_Channel *channel, const UA_CreateMonitoredItemsRequest *request, UA_CreateMonitoredItemsResponse *response) {
	if (request->itemsToCreateSize > 0) {
		response->resultsSize = request->itemsToCreateSize;

		UA_Array_new((void**)&(response->results),response->resultsSize,&UA_.types[UA_MONITOREDITEMCREATERESULT]);
		for (int i=0;request->itemsToCreateSize > 0 && i < request->itemsToCreateSize;i++) {
			UA_NodeId_printf("CreateMonitoredItems - itemToCreate=",&(request->itemsToCreate[i].itemToMonitor.nodeId));
			//FIXME: search the object in the namespace

			if (request->itemsToCreate[i].itemToMonitor.nodeId.identifier.numeric == 2253) { // server

				response->results[i].statusCode = UA_STATUSCODE_GOOD;
				response->results[i].monitoredItemId = 1024;
				response->results[i].revisedSamplingInterval = 100000;
				response->results[i].revisedQueueSize = 1;
			} else {
				// response->results[i]->statusCode = UA_STATUSCODE_BAD_NODEIDUNKNOWN;

				response->results[i].statusCode = -1;
			}
		}
	}
	//mock up
	return UA_SUCCESS;
}

//#endif
