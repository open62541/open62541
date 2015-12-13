#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_subscription_manager.h"
#include "ua_subscription.h"
#include "ua_statuscodes.h"
#include "ua_util.h"
#include "ua_nodestore.h"

#define UA_BOUNDEDVALUE_SETWBOUNDS(BOUNDS, SRC, DST) { \
    if(SRC > BOUNDS.maxValue) DST = BOUNDS.maxValue; \
    else if(SRC < BOUNDS.minValue) DST = BOUNDS.minValue; \
    else DST = SRC; \
    }

void Service_CreateSubscription(UA_Server *server, UA_Session *session,
                                const UA_CreateSubscriptionRequest *request,
                                UA_CreateSubscriptionResponse *response) {
    response->subscriptionId = SubscriptionManager_getUniqueUIntID(&session->subscriptionManager);
    UA_Subscription *newSubscription = UA_Subscription_new(response->subscriptionId);
    if(!newSubscription) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    
    /* set the publishing interval */
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.globalPublishingInterval,
                               request->requestedPublishingInterval, response->revisedPublishingInterval);
    newSubscription->publishingInterval = (UA_DateTime)response->revisedPublishingInterval;
    
    /* set the subscription lifetime (deleted when no publish requests arrive within this time) */
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.globalLifeTimeCount,
                               request->requestedLifetimeCount, response->revisedLifetimeCount);
    newSubscription->lifeTime = (UA_UInt32_BoundedValue)  {
        .minValue=session->subscriptionManager.globalLifeTimeCount.minValue,
        .maxValue=session->subscriptionManager.globalLifeTimeCount.maxValue,
        .currentValue=response->revisedLifetimeCount};
    
    /* set the keepalive count. the server sends an empty notification when
       nothin has happened for n publishing intervals */
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.globalKeepAliveCount,
                               request->requestedMaxKeepAliveCount, response->revisedMaxKeepAliveCount);
    newSubscription->keepAliveCount = (UA_Int32_BoundedValue)  {
        .minValue=session->subscriptionManager.globalKeepAliveCount.minValue,
        .maxValue=session->subscriptionManager.globalKeepAliveCount.maxValue,
        .currentValue=response->revisedMaxKeepAliveCount};
    
    newSubscription->notificationsPerPublish = request->maxNotificationsPerPublish;
    newSubscription->publishingMode          = request->publishingEnabled;
    newSubscription->priority                = request->priority;
    
    /* add the update job */
    UA_Guid jobId = SubscriptionManager_getUniqueGUID(&session->subscriptionManager);
    Subscription_createdUpdateJob(server, jobId, newSubscription);
    Subscription_registerUpdateJob(server, newSubscription);
    SubscriptionManager_addSubscription(&session->subscriptionManager, newSubscription);    
}

static void createMonitoredItems(UA_Server *server, UA_Session *session, UA_Subscription *sub,
                                 const UA_MonitoredItemCreateRequest *request,
                                 UA_MonitoredItemCreateResult *result) {
    const UA_Node *target = UA_NodeStore_get(server->nodestore, &request->itemToMonitor.nodeId);
    if(!target) {
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return;
    }

    UA_MonitoredItem *newMon = UA_MonitoredItem_new();
    if(!newMon) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    UA_StatusCode retval = UA_NodeId_copy(&target->nodeId, &newMon->monitoredNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        MonitoredItem_delete(newMon);
        return;
    }

    newMon->itemId = ++(session->subscriptionManager.lastSessionID);
    result->monitoredItemId = newMon->itemId;
    newMon->clientHandle = request->requestedParameters.clientHandle;

    /* set the sampling interval */
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.globalSamplingInterval,
                               request->requestedParameters.samplingInterval,
                               result->revisedSamplingInterval);
    newMon->samplingInterval = (UA_UInt32)result->revisedSamplingInterval;

    /* set the queue size */
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.globalQueueSize,
                               request->requestedParameters.queueSize,
                               result->revisedQueueSize);
    newMon->queueSize = (UA_UInt32_BoundedValue) {
        .maxValue=(result->revisedQueueSize) + 1,
        .minValue=0, .currentValue=0 };

    newMon->attributeID = request->itemToMonitor.attributeId;
    newMon->monitoredItemType = MONITOREDITEM_TYPE_CHANGENOTIFY;
    newMon->discardOldest = request->requestedParameters.discardOldest;
    LIST_INSERT_HEAD(&sub->MonitoredItems, newMon, listEntry);

    // todo: add a job that samples the value (for fixed intervals)
    // todo: add a pointer to the monitoreditem to the variable, so that events get propagated
}

void Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                                  const UA_CreateMonitoredItemsRequest *request,
                                  UA_CreateMonitoredItemsResponse *response) {
    UA_Subscription  *sub = SubscriptionManager_getSubscriptionByID(&session->subscriptionManager,
                                                                    request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }
    
    if(request->itemsToCreateSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_Array_new(request->itemsToCreateSize, &UA_TYPES[UA_TYPES_MONITOREDITEMCREATERESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->itemsToCreateSize;

    for(size_t i = 0; i < request->itemsToCreateSize; i++)
        createMonitoredItems(server, session, sub, &request->itemsToCreate[i], &response->results[i]);
}

void
Service_Publish(UA_Server *server, UA_Session *session,
                const UA_PublishRequest *request, UA_UInt32 requestId) {
    UA_SubscriptionManager *manager= &session->subscriptionManager;
    if(!manager)
        return;

    UA_PublishResponse response;
    UA_PublishResponse_init(&response);
    response.responseHeader.requestHandle = request->requestHeader.requestHandle;
    response.responseHeader.timestamp = UA_DateTime_now();
    
    // Delete Acknowledged Subscription Messages
    response.resultsSize = request->subscriptionAcknowledgementsSize;
    response.results = UA_calloc(response.resultsSize, sizeof(UA_StatusCode));
    for(size_t i = 0; i < request->subscriptionAcknowledgementsSize; i++) {
        response.results[i] = UA_STATUSCODE_GOOD;
        UA_UInt32 sid = request->subscriptionAcknowledgements[i].subscriptionId;
        UA_Subscription *sub = SubscriptionManager_getSubscriptionByID(manager, sid);
        if(!sub) {
            response.results[i] = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
            continue;
        }
        UA_UInt32 sn = request->subscriptionAcknowledgements[i].sequenceNumber;
        if(Subscription_deleteUnpublishedNotification(sn, false, sub) == 0)
            response.results[i] = UA_STATUSCODE_BADSEQUENCENUMBERINVALID;
    }
    
    UA_Boolean have_response = UA_FALSE;

    // See if any new data is available
    UA_Subscription *sub;
    LIST_FOREACH(sub, &manager->serverSubscriptions, listEntry) {
        if(sub->timedUpdateIsRegistered == UA_FALSE) {
            // FIXME: We are forcing a value update for monitored items. This should be done by the event system.
            // NOTE:  There is a clone of this functionality in the Subscription_timedUpdateNotificationsJob
            UA_MonitoredItem *mon;
            LIST_FOREACH(mon, &sub->MonitoredItems, listEntry)
                MonitoredItem_QueuePushDataValue(server, mon);
            
            // FIXME: We are forcing notification updates for the subscription. This
            // should be done by a timed work item.
            Subscription_updateNotifications(sub);
        }
        
        if(sub->unpublishedNotificationsSize == 0)
            continue;
        
        // This subscription has notifications in its queue (top NotificationMessage exists in the queue). 
        // Due to republish, we need to check if there are any unplublished notifications first ()
        UA_unpublishedNotification *notification = NULL;
        LIST_FOREACH(notification, &sub->unpublishedNotifications, listEntry) {
            if (notification->publishedOnce == UA_FALSE)
                break;
        }
        if (notification == NULL)
            continue;
    
        // We found an unpublished notification message in this subscription, which we will now publish.
        response.subscriptionId = sub->subscriptionID;
        Subscription_copyNotificationMessage(&response.notificationMessage, notification);
        // Mark this notification as published
        notification->publishedOnce = UA_TRUE;
        if(notification->notification.sequenceNumber > sub->sequenceNumber) {
            // If this is a keepalive message, its seqNo is the next seqNo to be used for an actual msg.
            response.availableSequenceNumbersSize = 0;
            // .. and must be deleted
            Subscription_deleteUnpublishedNotification(sub->sequenceNumber + 1, false, sub);
        } else {
            response.availableSequenceNumbersSize = sub->unpublishedNotificationsSize;
            response.availableSequenceNumbers = Subscription_getAvailableSequenceNumbers(sub);
        }	  
        have_response = UA_TRUE;
    }
    
    if(!have_response) {
        // FIXME: At this point, we would return nothing and "queue" the publish
        // request, but currently we need to return something to the client. If no
        // subscriptions have notifications, force one to generate a keepalive so we
        // don't return an empty message
        sub = LIST_FIRST(&manager->serverSubscriptions);
        if(sub) {
            response.subscriptionId = sub->subscriptionID;
            sub->keepAliveCount.currentValue=sub->keepAliveCount.minValue;
            Subscription_generateKeepAlive(sub);
            Subscription_copyNotificationMessage(&response.notificationMessage,
                                                 LIST_FIRST(&sub->unpublishedNotifications));
            Subscription_deleteUnpublishedNotification(sub->sequenceNumber + 1, false, sub);
        }
    }
    
    UA_SecureChannel *channel = session->channel;
    if(channel)
        UA_SecureChannel_sendBinaryMessage(channel, requestId, &response,
                                           &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
    UA_PublishResponse_deleteMembers(&response);
}

void Service_ModifySubscription(UA_Server *server, UA_Session *session,
                                 const UA_ModifySubscriptionRequest *request,
                                 UA_ModifySubscriptionResponse *response) {
    UA_Subscription *sub = SubscriptionManager_getSubscriptionByID(&session->subscriptionManager,
                                                                   request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }
    
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.globalPublishingInterval,
                               request->requestedPublishingInterval, response->revisedPublishingInterval);
    sub->publishingInterval = (UA_DateTime)response->revisedPublishingInterval;
    
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.globalLifeTimeCount,
                               request->requestedLifetimeCount, response->revisedLifetimeCount);
    sub->lifeTime = (UA_UInt32_BoundedValue)  {
        .minValue=session->subscriptionManager.globalLifeTimeCount.minValue,
        .maxValue=session->subscriptionManager.globalLifeTimeCount.maxValue,
        .currentValue=response->revisedLifetimeCount};
        
    UA_BOUNDEDVALUE_SETWBOUNDS(session->subscriptionManager.globalKeepAliveCount,
                                request->requestedMaxKeepAliveCount, response->revisedMaxKeepAliveCount);
    sub->keepAliveCount = (UA_Int32_BoundedValue)  {
        .minValue=session->subscriptionManager.globalKeepAliveCount.minValue,
        .maxValue=session->subscriptionManager.globalKeepAliveCount.maxValue,
        .currentValue=response->revisedMaxKeepAliveCount};
        
    sub->notificationsPerPublish = request->maxNotificationsPerPublish;
    sub->priority                = request->priority;
    
    Subscription_registerUpdateJob(server, sub);
    return;
}

void Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                                 const UA_DeleteSubscriptionsRequest *request,
                                 UA_DeleteSubscriptionsResponse *response) {
    response->results = UA_malloc(sizeof(UA_StatusCode) * request->subscriptionIdsSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->subscriptionIdsSize;

    for(size_t i = 0; i < request->subscriptionIdsSize; i++)
        response->results[i] =
            SubscriptionManager_deleteSubscription(server, &session->subscriptionManager,
                                                   request->subscriptionIds[i]);
} 

void Service_DeleteMonitoredItems(UA_Server *server, UA_Session *session,
                                  const UA_DeleteMonitoredItemsRequest *request,
                                  UA_DeleteMonitoredItemsResponse *response) {
    UA_SubscriptionManager *manager = &session->subscriptionManager;
    UA_Subscription *sub = SubscriptionManager_getSubscriptionByID(manager, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }
    
    response->results = UA_malloc(sizeof(UA_StatusCode) * request->monitoredItemIdsSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->monitoredItemIdsSize;

    for(size_t i = 0; i < request->monitoredItemIdsSize; i++)
        response->results[i] =
            SubscriptionManager_deleteMonitoredItem(manager, sub->subscriptionID,
                                                    request->monitoredItemIds[i]);
}

void Service_Republish(UA_Server *server, UA_Session *session,
                                const UA_RepublishRequest *request,
                                UA_RepublishResponse *response) {
    UA_SubscriptionManager *manager = &session->subscriptionManager;
    UA_Subscription *sub = SubscriptionManager_getSubscriptionByID(manager, request->subscriptionId);
    if (!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }
    
    // Find the notification in question
    UA_unpublishedNotification *notification;
    LIST_FOREACH(notification, &sub->unpublishedNotifications, listEntry) {
      if (notification->notification.sequenceNumber == request->retransmitSequenceNumber)
	break;
    }
    if (!notification) {
      response->responseHeader.serviceResult = UA_STATUSCODE_BADSEQUENCENUMBERINVALID;
      return;
    }
    
    // FIXME: By spec, this notification has to be in the "retransmit queue", i.e. publishedOnce must be
    //        true. If this is not tested, the client just gets what he asks for... hence this part is
    //        commented:
    /* Check if the notification is in the published queue
    if (notification->publishedOnce == UA_FALSE) {
      response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
      return;
    }
    */
    // Retransmit 
    Subscription_copyNotificationMessage(&response->notificationMessage, notification);
    // Mark this notification as published
    notification->publishedOnce = UA_TRUE;
    
    return;
}
