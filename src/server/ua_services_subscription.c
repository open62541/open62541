#ifdef ENABLE_SUBSCRIPTIONS
#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_subscription_manager.h"
#include "ua_statuscodes.h"
#include "ua_util.h"
#include "ua_nodestore.h"

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
    response->subscriptionId = ++(session->subscriptionManager.LastSessionID);
    newSubscription = UA_Subscription_new(response->subscriptionId);
    
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.GlobalPublishingInterval, request->requestedPublishingInterval, response->revisedPublishingInterval);
    newSubscription->PublishingInterval = response->revisedPublishingInterval;
    
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.GlobalLifeTimeCount,  request->requestedLifetimeCount, response->revisedLifetimeCount);
    newSubscription->LifeTime = (UA_UInt32_BoundedValue)  { .minValue=session->subscriptionManager.GlobalLifeTimeCount.minValue, .maxValue=session->subscriptionManager.GlobalLifeTimeCount.maxValue, .currentValue=response->revisedLifetimeCount};
    
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.GlobalKeepAliveCount, request->requestedMaxKeepAliveCount, response->revisedMaxKeepAliveCount);
    newSubscription->KeepAliveCount = (UA_Int32_BoundedValue)  { .minValue=session->subscriptionManager.GlobalKeepAliveCount.minValue, .maxValue=session->subscriptionManager.GlobalKeepAliveCount.maxValue, .currentValue=response->revisedMaxKeepAliveCount};
    
    newSubscription->NotificationsPerPublish = request->maxNotificationsPerPublish;
    newSubscription->PublishingMode          = request->publishingEnabled;
    newSubscription->Priority                = request->priority;
    
    SubscriptionManager_addSubscription(&(session->subscriptionManager), newSubscription);    
    
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
    sub=SubscriptionManager_getSubscriptionByID(&(session->subscriptionManager), request->subscriptionId);
    if (session == NULL ) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;           
    else if ( session->channel == NULL || session->activated == UA_FALSE) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
    else if ( sub == NULL) response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    if ( response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) return 0;
    
    // Allocate Result Array
    if (request->itemsToCreateSize > 0) {
        createResults = (UA_MonitoredItemCreateResult *) UA_malloc(sizeof(UA_MonitoredItemCreateResult) * (request->itemsToCreateSize));
        if (createResults == NULL) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return 0;
        }
    }
    else return 0;
    
    response->resultsSize = request->itemsToCreateSize;
    response->results = createResults;
    
    for(int i=0; i<request->itemsToCreateSize; i++) {
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
            newMon->monitoredNode = UA_NodeStore_get(server->nodestore, (const UA_NodeId *) &(thisItemsRequest->itemToMonitor.nodeId));
            newMon->ItemId = ++(session->subscriptionManager.LastSessionID);
            thisItemsResult->monitoredItemId = newMon->ItemId;
            
            newMon->ClientHandle = thisItemsRequest->requestedParameters.clientHandle;
            
            UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.GlobalSamplingInterval , thisItemsRequest->requestedParameters.samplingInterval, thisItemsResult->revisedSamplingInterval);
            newMon->SamplingInterval = thisItemsResult->revisedSamplingInterval;
            
            UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.GlobalQueueSize, thisItemsRequest->requestedParameters.queueSize, thisItemsResult->revisedQueueSize);
            newMon->QueueSize = (UA_UInt32_BoundedValue) { .maxValue=(thisItemsResult->revisedQueueSize) + 1, .minValue=0, .currentValue=0 };
            newMon->AttributeID = thisItemsRequest->itemToMonitor.attributeId;
            newMon->MonitoredItemType = MONITOREDITEM_CHANGENOTIFY_T;

            newMon->DiscardOldest = thisItemsRequest->requestedParameters.discardOldest;
            
            LIST_INSERT_HEAD(sub->MonitoredItems, newMon, listEntry);
        }
    }
    
    return 0;
}

UA_Int32 Service_Publish(UA_Server *server, UA_Session *session,
                         const UA_PublishRequest *request,
                         UA_PublishResponse *response) {
    
    UA_Subscription  *sub;
    UA_MonitoredItem *mon;
    UA_SubscriptionManager *manager;
    
    // Verify Session and Subscription
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    response->diagnosticInfosSize = 0;
    response->diagnosticInfos     = 0;
    response->availableSequenceNumbersSize = 0;
    response->resultsSize = 0;
    response->subscriptionId = 0;
    response->moreNotifications = UA_FALSE;
    response->notificationMessage.notificationDataSize = 0;
    
    //printf("Validity: %lu, %lu\n", session->validTill - UA_DateTime_now(), session->timeout);
    
    if (session == NULL ) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;           
    else if ( session->channel == NULL || session->activated == UA_FALSE) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
    if ( response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) return 0;

    manager = &(session->subscriptionManager);    
    if (manager == NULL) return 0;
    
    // Delete Acknowledged Subscription Messages
    response->resultsSize = request->subscriptionAcknowledgementsSize;
    response->results     = (UA_StatusCode *) UA_malloc(sizeof(UA_StatusCode)*(response->resultsSize));
    for(int i=0; i<request->subscriptionAcknowledgementsSize;i++ ) {
      response->results[i] = UA_STATUSCODE_GOOD;
      sub = SubscriptionManager_getSubscriptionByID(&(session->subscriptionManager), request->subscriptionAcknowledgements[i].subscriptionId);
      if (sub == NULL) {
        response->results[i] = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        continue;
      }
      if(Subscription_deleteUnpublishedNotification(request->subscriptionAcknowledgements[i].sequenceNumber, sub) == 0) {
        response->results[i] = UA_STATUSCODE_BADSEQUENCENUMBERINVALID;
      }
    }
    
    // See if any new data is available
    for (sub=(manager->ServerSubscriptions)->lh_first; sub != NULL; sub = sub->listEntry.le_next) {
	
        // FIXME: We are forcing a value update for monitored items. This should be done by the event system.
        if (sub->MonitoredItems->lh_first != NULL) {  
	  for(mon=sub->MonitoredItems->lh_first; mon != NULL; mon=mon->listEntry.le_next) {
            MonitoredItem_QueuePushDataValue(mon);
	  }
	}
	
	// FIXME: We are forcing notification updates for the subscription. This should be done by a timed work item.
	Subscription_updateNotifications(sub);
        
        if (Subscription_queuedNotifications(sub) > 0) {
	  response->subscriptionId = sub->SubscriptionID;
	  
	  Subscription_copyTopNotificationMessage(&(response->notificationMessage), sub);
	  
	  if (sub->unpublishedNotifications->lh_first->notification->sequenceNumber > sub->SequenceNumber) {
	    // If this is a keepalive message, its seqNo is the next seqNo to be used for an actual msg.
	    response->availableSequenceNumbersSize = 0;
	    // .. and must be deleted
	    Subscription_deleteUnpublishedNotification(sub->SequenceNumber + 1, sub);
	  }
	  else {
	    response->availableSequenceNumbersSize = Subscription_queuedNotifications(sub);
	    response->availableSequenceNumbers = Subscription_getAvailableSequenceNumbers(sub);
	  }	  
	  
	  // FIXME: This should be in processMSG();
	  session->validTill = UA_DateTime_now() + session->timeout * 10000;
	  return 0;
	}
    }
    
    // FIXME: At this point, we would return nothing and "queue" the publish request, but currently we need to 
    //        return something to the client.
    //        If no subscriptions have notifications, force one to generate a keepalive so we don't return an 
    //        empty message
    sub = manager->ServerSubscriptions->lh_first;
    if ( sub != NULL) {
      response->subscriptionId = sub->SubscriptionID;
      sub->KeepAliveCount.currentValue=sub->KeepAliveCount.minValue;
      Subscription_generateKeepAlive(sub);
      Subscription_copyTopNotificationMessage(&(response->notificationMessage), sub);
      Subscription_deleteUnpublishedNotification(sub->SequenceNumber + 1, sub);
      response->availableSequenceNumbersSize = 0;
    }
    
    // FIXME: This should be in processMSG();
    session->validTill = UA_DateTime_now() + session->timeout * 10000;
    
    response->diagnosticInfosSize = 0;
    response->diagnosticInfos     = 0;
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
    
    retStat = (UA_UInt32 *) UA_malloc(sizeof(UA_UInt32) * request->subscriptionIdsSize);
    if (retStat==NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return -1;
    }
    
    for(int i=0; i<request->subscriptionIdsSize;i++) {
        retStat[i] = SubscriptionManager_deleteSubscription(&(session->subscriptionManager), request->subscriptionIds[i]);
    }
    response->resultsSize = request->subscriptionIdsSize;
    response->results     = retStat;
    return 0;
} 

UA_Int32 Service_DeleteMonitoredItems(UA_Server *server, UA_Session *session,
                                      const UA_DeleteMonitoredItemsRequest *request,
                                      UA_DeleteMonitoredItemsResponse *response) {
    UA_SubscriptionManager *manager;
    UA_Subscription *sub;
    UA_Int32 *resultCodes;
    
    // Verify Session
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    if (session == NULL ) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;           
    else if ( session->channel == NULL || session->activated == UA_FALSE) response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
    if ( response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) return 0;
    
    response->diagnosticInfosSize=0;
    response->resultsSize=0;
    
    manager = &(session->subscriptionManager);
    
    sub = SubscriptionManager_getSubscriptionByID(manager, request->subscriptionId);
    
    if (sub == NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return 0;
    }
    
    resultCodes = (UA_Int32 *) UA_malloc(sizeof(UA_UInt32) * request->monitoredItemIdsSize);
    if (resultCodes == NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return 0;
    }

    response->results = (UA_StatusCode *) resultCodes;
    response->resultsSize = request->monitoredItemIdsSize;
    for(int i=0; i < request->monitoredItemIdsSize; i++) {
        resultCodes[i] = SubscriptionManager_deleteMonitoredItem(manager, sub->SubscriptionID, (request->monitoredItemIds)[i]);
    }
    
    return 0;
}
#endif //#ifdef ENABLE_SUBSCRIPTIONS
