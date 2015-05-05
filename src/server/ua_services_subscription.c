#ifdef ENABLESUBSCRIPTIONS
#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_subscription_manager.h"
#include "ua_statuscodes.h"
#include "ua_util.h"
#include "ua_nodestore.h"


#include "ua_log.h" // Remove later, debugging only

#define UA_BOUNDEDVALUE_SETWBOUNDS(BOUNDS, SRC, DST) { \
    if (SRC > BOUNDS.maxValue) DST = BOUNDS.maxValue; \
    else if (SRC < BOUNDS.minValue) DST = BOUNDS.minValue; \
    else DST = SRC; \
    }

UA_Int32 Service_CreateSubscription(UA_Server *server, UA_Session *session,
                                     const UA_CreateSubscriptionRequest *request,
                                     UA_CreateSubscriptionResponse *response) {
    UA_Subscription *newSubscription;
    
    // Verify Session
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    if (session == NULL ) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;           
    else if ( session->channel == NULL || session->activated == UA_FALSE) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
    if ( response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) return 0;
    
    // Create Subscription and Response
    response->subscriptionId = ++(server->subscriptionManager.LastSessionID);
    newSubscription = UA_Subscription_new(response->subscriptionId);
    
    UA_BOUNDEDVALUE_SETWBOUNDS(server->subscriptionManager.GlobalPublishingInterval, request->requestedPublishingInterval, response->revisedPublishingInterval);
    newSubscription->PublishingInterval = response->revisedPublishingInterval;
    
    UA_BOUNDEDVALUE_SETWBOUNDS(server->subscriptionManager.GlobalLifeTimeCount,  request->requestedLifetimeCount, response->revisedLifetimeCount);
    newSubscription->LifeTime = (UA_UInt32_BoundedValue)  { .minValue=server->subscriptionManager.GlobalLifeTimeCount.minValue, .maxValue=server->subscriptionManager.GlobalLifeTimeCount.maxValue, .currentValue=response->revisedLifetimeCount};
    
    UA_BOUNDEDVALUE_SETWBOUNDS(server->subscriptionManager.GlobalKeepAliveCount, request->requestedMaxKeepAliveCount, response->revisedMaxKeepAliveCount);
    newSubscription->KeepAliveCount = (UA_UInt32_BoundedValue)  { .minValue=server->subscriptionManager.GlobalKeepAliveCount.minValue, .maxValue=server->subscriptionManager.GlobalKeepAliveCount.maxValue, .currentValue=response->revisedMaxKeepAliveCount};
    
    newSubscription->NotificationsPerPublish = request->maxNotificationsPerPublish;
    newSubscription->PublishingMode          = request->publishingEnabled;
    newSubscription->Priority                = request->priority;
    
    SubscriptionManager_addSubscription(&(server->subscriptionManager), newSubscription);    
    
    return (UA_Int32) 0;
}

UA_Int32 Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                                      const UA_CreateMonitoredItemsRequest *request,
                                      UA_CreateMonitoredItemsResponse *response) {
    UA_MonitoredItem *newMon;
    UA_Subscription  *sub;
    UA_MonitoredItemCreateResult *createResults, *thisItemsResult;
    UA_MonitoredItemCreateRequest *thisItemsRequest;
    
    // Verify Session and Subscription
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    sub=SubscriptionManager_getSubscriptionByID(&(server->subscriptionManager), request->subscriptionId);
    if (session == NULL ) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;           
    else if ( session->channel == NULL || session->activated == UA_FALSE) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
    else if ( sub == NULL) response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    if ( response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) return 0;
    
    // Allocate Result Array
    if (request->itemsToCreateSize > 0) {
        createResults = (UA_MonitoredItemCreateResult *) malloc(sizeof(UA_MonitoredItemCreateResult) * (request->itemsToCreateSize));
        if (createResults == NULL) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return 0;
        }
    }
    else return 0;
    
    response->resultsSize = request->itemsToCreateSize;
    response->results = createResults;
    
    for(int i=0;i<request->itemsToCreateSize;i++) {
        thisItemsRequest = &(request->itemsToCreate[i]);
        thisItemsResult  = &(createResults[i]);
        
        // FIXME: Completely ignoring filters for now!
        thisItemsResult->filterResult.typeId   = UA_NODEID_NULL;
        thisItemsResult->filterResult.encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED;
        thisItemsResult->filterResult.body     = UA_BYTESTRING_NULL;
        
        if (UA_NodeStore_get(server->nodestore, (const UA_NodeId *) &(thisItemsRequest->itemToMonitor.nodeId)) == UA_NULL) {
            thisItemsResult->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
            thisItemsResult->monitoredItemId = 0;
            thisItemsResult->revisedSamplingInterval = 0;
            thisItemsResult->revisedQueueSize = 0;
        }
        else {
            thisItemsResult->statusCode = UA_STATUSCODE_GOOD;
            
            newMon = UA_MonitoredItem_new();
            memcpy(&(newMon->ItemNodeId), &(thisItemsRequest->itemToMonitor.nodeId), sizeof(UA_NodeId));
            newMon->ItemId = ++(server->subscriptionManager.LastSessionID);
            thisItemsResult->monitoredItemId = newMon->ItemId;
            
            newMon->ClientHandle = thisItemsRequest->requestedParameters.clientHandle;
            
            UA_BOUNDEDVALUE_SETWBOUNDS(server->subscriptionManager.GlobalSamplingInterval , thisItemsRequest->requestedParameters.samplingInterval, thisItemsResult->revisedSamplingInterval);
            newMon->SamplingInterval = thisItemsResult->revisedSamplingInterval;
            
            UA_BOUNDEDVALUE_SETWBOUNDS(server->subscriptionManager.GlobalQueueSize, thisItemsRequest->requestedParameters.queueSize, thisItemsResult->revisedQueueSize);
            newMon->QueueSize = thisItemsResult->revisedQueueSize;
            
            LIST_INSERT_HEAD(sub->MonitoredItems, newMon, listEntry);
        }
    }
    
    return 0;
}

UA_Int32 Service_Publish(UA_Server *server, UA_Session *session,
                         const UA_PublishRequest *request,
                         UA_PublishResponse *response) {
    
    // Verify Session
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    if (session == NULL ) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;           
    else if ( session->channel == NULL || session->activated == UA_FALSE) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
    if ( response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) return 0;
    
    // FIXME
    response->responseHeader.serviceResult = UA_STATUSCODE_BADSERVICEUNSUPPORTED;
    return 0;
}

UA_Int32 Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                                     const UA_DeleteSubscriptionsRequest *request,
                                     UA_DeleteSubscriptionsResponse *response) {
    UA_StatusCode *retStat;
    
    // Verify Session
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    if (session == NULL ) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;           
    else if ( session->channel == NULL || session->activated == UA_FALSE) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
    if ( response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) return 0;
    
    retStat = (UA_StatusCode *) malloc(sizeof(UA_StatusCode) * request->subscriptionIdsSize);
    if (retStat==NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return -1;
    }
    
    for(int i=0; i<request->subscriptionIdsSize;i++) {
        retStat[i] = SubscriptionManager_deleteSubscription(&(server->subscriptionManager), request->subscriptionIds[i]);
    }
    response->resultsSize = request->subscriptionIdsSize;
    response->results     = retStat;
    return 0;
} 

#endif //#ifdef ENABLESUBSCRIPTIONS
