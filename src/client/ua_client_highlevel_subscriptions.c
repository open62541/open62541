#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

#include "ua_client_highlevel.h"
#include "ua_client_internal.h"
#include "ua_util.h"
#include "ua_types_generated_encoding_binary.h"

const UA_SubscriptionSettings UA_SubscriptionSettings_standard = {
    .requestedPublishingInterval = 500.0,
    .requestedLifetimeCount = 10000,
    .requestedMaxKeepAliveCount = 1,
    .maxNotificationsPerPublish = 10,
    .publishingEnabled = true,
    .priority = 0
};

UA_StatusCode UA_Client_Subscriptions_new(UA_Client *client, UA_SubscriptionSettings settings,
                                          UA_UInt32 *newSubscriptionId) {
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);
    request.requestedPublishingInterval = settings.requestedPublishingInterval;
    request.requestedLifetimeCount = settings.requestedLifetimeCount;
    request.requestedMaxKeepAliveCount = settings.requestedMaxKeepAliveCount;
    request.maxNotificationsPerPublish = settings.maxNotificationsPerPublish;
    request.publishingEnabled = settings.publishingEnabled;
    request.priority = settings.priority;
    
    UA_CreateSubscriptionResponse response = UA_Client_Service_createSubscription(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        UA_Client_Subscription *newSub = UA_malloc(sizeof(UA_Client_Subscription));
        LIST_INIT(&newSub->MonitoredItems);
        newSub->LifeTime = response.revisedLifetimeCount;
        newSub->KeepAliveCount = response.revisedMaxKeepAliveCount;
        newSub->PublishingInterval = response.revisedPublishingInterval;
        newSub->SubscriptionID = response.subscriptionId;
        newSub->NotificationsPerPublish = request.maxNotificationsPerPublish;
        newSub->Priority = request.priority;
        if(newSubscriptionId)
            *newSubscriptionId = newSub->SubscriptionID;
        LIST_INSERT_HEAD(&client->subscriptions, newSub, listEntry);
    }
    
    UA_CreateSubscriptionResponse_deleteMembers(&response);
    return retval;
}

UA_StatusCode UA_Client_Subscriptions_remove(UA_Client *client, UA_UInt32 subscriptionId) {
    UA_Client_Subscription *sub;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if(sub->SubscriptionID == subscriptionId)
            break;
    }
    
    // Problem? We do not have this subscription registeres. Maybe the server should
    // be consulted at this point?
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    UA_DeleteSubscriptionsRequest request;
    UA_DeleteSubscriptionsRequest_init(&request);
    request.subscriptionIdsSize = 1;
    request.subscriptionIds = (UA_UInt32 *) UA_malloc(sizeof(UA_UInt32));
    *request.subscriptionIds = sub->SubscriptionID;
    
    UA_Client_MonitoredItem *mon, *tmpmon;
    LIST_FOREACH_SAFE(mon, &sub->MonitoredItems, listEntry, tmpmon) {
        retval |= UA_Client_Subscriptions_removeMonitoredItem(client, sub->SubscriptionID,
                                                              mon->MonitoredItemId);
    }
    if(retval != UA_STATUSCODE_GOOD) {
	    UA_DeleteSubscriptionsRequest_deleteMembers(&request);
        return retval;
    }
    
    UA_DeleteSubscriptionsResponse response = UA_Client_Service_deleteSubscriptions(client, request);
    if(response.resultsSize > 0)
        retval = response.results[0];
    else
        retval = response.responseHeader.serviceResult;
    
    if(retval == UA_STATUSCODE_GOOD) {
        LIST_REMOVE(sub, listEntry);
        UA_free(sub);
    }
    UA_DeleteSubscriptionsRequest_deleteMembers(&request);
    UA_DeleteSubscriptionsResponse_deleteMembers(&response);
    return retval;
}

UA_StatusCode
UA_Client_Subscriptions_addMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                         UA_NodeId nodeId, UA_UInt32 attributeID,
                                         UA_MonitoredItemHandlingFunction handlingFunction,
                                         void *handlingContext, UA_UInt32 *newMonitoredItemId) {
    UA_Client_Subscription *sub;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if(sub->SubscriptionID == subscriptionId)
            break;
    }
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    /* Send the request */
    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = nodeId;
    item.itemToMonitor.attributeId = attributeID;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    item.requestedParameters.clientHandle = ++(client->monitoredItemHandles);
    item.requestedParameters.samplingInterval = sub->PublishingInterval;
    item.requestedParameters.discardOldest = true;
    item.requestedParameters.queueSize = 1;
    request.itemsToCreate = &item;
    request.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse response = UA_Client_Service_createMonitoredItems(client, request);
    
    // slight misuse of retval here to check if the deletion was successfull.
    UA_StatusCode retval;
    if(response.resultsSize == 0)
        retval = response.responseHeader.serviceResult;
    else
        retval = response.results[0].statusCode;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_CreateMonitoredItemsResponse_deleteMembers(&response);
        return retval;
    }

    /* Create the handler */
    UA_Client_MonitoredItem *newMon = UA_malloc(sizeof(UA_Client_MonitoredItem));
    newMon->MonitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_NodeId_copy(&nodeId, &newMon->monitoredNodeId); 
    newMon->AttributeID = attributeID;
    newMon->ClientHandle = client->monitoredItemHandles;
    newMon->SamplingInterval = sub->PublishingInterval;
    newMon->QueueSize = 1;
    newMon->DiscardOldest = true;
    newMon->handler = handlingFunction;
    newMon->handlerContext = handlingContext;
    newMon->MonitoredItemId = response.results[0].monitoredItemId;
    LIST_INSERT_HEAD(&sub->MonitoredItems, newMon, listEntry);
    *newMonitoredItemId = newMon->MonitoredItemId;

    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Created a monitored item with client handle %u", client->monitoredItemHandles);
    
    UA_CreateMonitoredItemsResponse_deleteMembers(&response);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_Subscriptions_removeMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                            UA_UInt32 monitoredItemId) {
    UA_Client_Subscription *sub;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if(sub->SubscriptionID == subscriptionId)
            break;
    }
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    UA_Client_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
        if(mon->MonitoredItemId == monitoredItemId)
            break;
    }
    if(!mon)
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
    
    UA_DeleteMonitoredItemsRequest request;
    UA_DeleteMonitoredItemsRequest_init(&request);
    request.subscriptionId = sub->SubscriptionID;
    request.monitoredItemIdsSize = 1;
    request.monitoredItemIds = (UA_UInt32 *) UA_malloc(sizeof(UA_UInt32));
    request.monitoredItemIds[0] = mon->MonitoredItemId;
    
    UA_DeleteMonitoredItemsResponse response = UA_Client_Service_deleteMonitoredItems(client, request);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(response.resultsSize > 1)
        retval = response.results[0];
    else
        retval = response.responseHeader.serviceResult;
    
    if(retval == UA_STATUSCODE_GOOD) {
        LIST_REMOVE(mon, listEntry);
        UA_NodeId_deleteMembers(&mon->monitoredNodeId);
        UA_free(mon);
    }
    
    UA_DeleteMonitoredItemsRequest_deleteMembers(&request);
    UA_DeleteMonitoredItemsResponse_deleteMembers(&response);
    return retval;
}

static void
UA_Client_processPublishResponse(UA_Client *client, UA_PublishResponse *response) {
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;

    /* Find the subscription */
    UA_Client_Subscription *sub;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if(sub->SubscriptionID == response->subscriptionId)
            break;
    }
    if(!sub)
        return;

    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Processing a publish response on subscription %u with %u notifications",
                 sub->SubscriptionID, response->notificationMessage.notificationDataSize);

    /* Check if the server has acknowledged any of our ACKS */
    // TODO: The acks should be attached to the subscription
    UA_Client_NotificationsAckNumber *ack, *tmpAck;
    size_t i = 0;
    LIST_FOREACH_SAFE(ack, &client->pendingNotificationsAcks, listEntry, tmpAck) {
        if(response->results[i] == UA_STATUSCODE_GOOD ||
           response->results[i] == UA_STATUSCODE_BADSEQUENCENUMBERINVALID) {
            LIST_REMOVE(ack, listEntry);
            UA_free(ack);
        }
        i++;
    }
    
    /* Process the notification messages */
    UA_NotificationMessage *msg = &response->notificationMessage;
    for(size_t k = 0; k < msg->notificationDataSize; k++) {
        if(msg->notificationData[k].encoding != UA_EXTENSIONOBJECT_DECODED)
            continue;
        
        /* Currently only dataChangeNotifications are supported */
        if(msg->notificationData[k].content.decoded.type != &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION])
            continue;
        
        UA_DataChangeNotification *dataChangeNotification = msg->notificationData[k].content.decoded.data;
        for(size_t j = 0; j < dataChangeNotification->monitoredItemsSize; j++) {
            UA_MonitoredItemNotification *mitemNot = &dataChangeNotification->monitoredItems[j];
            UA_Client_MonitoredItem *mon;
            LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
                if(mon->ClientHandle == mitemNot->clientHandle) {
                    mon->handler(mon->MonitoredItemId, &mitemNot->value, mon->handlerContext);
                    break;
                }
            }
            if(!mon)
                UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                             "Could not process a notification with clienthandle %u on subscription %u",
                             mitemNot->clientHandle, sub->SubscriptionID);
        }
    }
    
    /* Add to the list of pending acks */
    tmpAck = UA_malloc(sizeof(UA_Client_NotificationsAckNumber));
    tmpAck->subAck.sequenceNumber = msg->sequenceNumber;
    tmpAck->subAck.subscriptionId = sub->SubscriptionID;
    LIST_INSERT_HEAD(&client->pendingNotificationsAcks, tmpAck, listEntry);
}

UA_StatusCode UA_Client_Subscriptions_manuallySendPublishRequest(UA_Client *client) {
    if (client->state == UA_CLIENTSTATE_ERRORED)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    UA_Boolean moreNotifications = true;
    while(moreNotifications == true) {
        UA_PublishRequest request;
        UA_PublishRequest_init(&request);
        request.subscriptionAcknowledgementsSize = 0;

        UA_Client_NotificationsAckNumber *ack;
        LIST_FOREACH(ack, &client->pendingNotificationsAcks, listEntry)
            request.subscriptionAcknowledgementsSize++;
        if(request.subscriptionAcknowledgementsSize > 0) {
            request.subscriptionAcknowledgements =
                UA_malloc(sizeof(UA_SubscriptionAcknowledgement) * request.subscriptionAcknowledgementsSize);
            if(!request.subscriptionAcknowledgements)
                return UA_STATUSCODE_GOOD;
        }
        
        int index = 0 ;
        LIST_FOREACH(ack, &client->pendingNotificationsAcks, listEntry) {
            request.subscriptionAcknowledgements[index].sequenceNumber = ack->subAck.sequenceNumber;
            request.subscriptionAcknowledgements[index].subscriptionId = ack->subAck.subscriptionId;
            index++;
        }
        
        UA_PublishResponse response = UA_Client_Service_publish(client, request);
        UA_Client_processPublishResponse(client, &response);
        moreNotifications = response.moreNotifications;
        
        UA_PublishResponse_deleteMembers(&response);
        UA_PublishRequest_deleteMembers(&request);
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
