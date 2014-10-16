#include "ua_services.h"
#include "ua_namespace_0.h"
#include "ua_statuscodes.h"

void Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                                  const UA_CreateMonitoredItemsRequest *request,
                                  UA_CreateMonitoredItemsResponse *response) {
    if(request->itemsToCreateSize <= 0)
        return;

    //mock up
    response->resultsSize = request->itemsToCreateSize;
    UA_Array_new((void**)&response->results, response->resultsSize, &UA_[UA_MONITOREDITEMCREATERESULT]);
    for(int i = 0;request->itemsToCreateSize > 0 && i < request->itemsToCreateSize;i++) {
        //FIXME: search the object in the namespace
        if(request->itemsToCreate[i].itemToMonitor.nodeId.identifier.numeric == 2253) {  // server
            response->results[i].statusCode       = UA_STATUSCODE_GOOD;
            response->results[i].monitoredItemId  = 1;
            response->results[i].revisedSamplingInterval = 4294967295;
            response->results[i].revisedQueueSize = 0;
            continue;
        } else {
            // response->results[i]->statusCode = UA_STATUSCODE_BAD_NODEIDUNKNOWN;
            response->results[i].statusCode = -1;
        }
    }
}
