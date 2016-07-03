#include "ua_client_highlevel.h"
#include "ua_client_internal.h"
#include "ua_util.h"
#include "ua_types_generated_encoding_binary.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

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
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_Client_Subscription *newSub = UA_malloc(sizeof(UA_Client_Subscription));
    if(!newSub) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    LIST_INIT(&newSub->MonitoredItems);
    newSub->LifeTime = response.revisedLifetimeCount;
    newSub->KeepAliveCount = response.revisedMaxKeepAliveCount;
    newSub->PublishingInterval = response.revisedPublishingInterval;
    newSub->SubscriptionID = response.subscriptionId;
    newSub->NotificationsPerPublish = request.maxNotificationsPerPublish;
    newSub->Priority = request.priority;
    LIST_INSERT_HEAD(&client->subscriptions, newSub, listEntry);

    if(newSubscriptionId)
        *newSubscriptionId = newSub->SubscriptionID;

 cleanup:
    UA_CreateSubscriptionResponse_deleteMembers(&response);
    return retval;
}

/* remove the subscription remotely */
UA_StatusCode UA_Client_Subscriptions_remove(UA_Client *client, UA_UInt32 subscriptionId) {
    UA_Client_Subscription *sub;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if(sub->SubscriptionID == subscriptionId)
            break;
    }
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Client_MonitoredItem *mon, *tmpmon;
    LIST_FOREACH_SAFE(mon, &sub->MonitoredItems, listEntry, tmpmon) {
        retval = UA_Client_Subscriptions_removeMonitoredItem(client, sub->SubscriptionID,
                                                             mon->MonitoredItemId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* remove the subscription remotely */
    UA_DeleteSubscriptionsRequest request;
    UA_DeleteSubscriptionsRequest_init(&request);
    request.subscriptionIdsSize = 1;
    request.subscriptionIds = &sub->SubscriptionID;
    UA_DeleteSubscriptionsResponse response = UA_Client_Service_deleteSubscriptions(client, request);
    retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize > 0)
        retval = response.results[0];
    UA_DeleteSubscriptionsResponse_deleteMembers(&response);

    if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Could not remove subscription %u with statuscode 0x%08x",
                    sub->SubscriptionID, retval);
        return retval;
    }

    UA_Client_Subscriptions_forceDelete(client, sub);
    return UA_STATUSCODE_GOOD;
}

void UA_Client_Subscriptions_forceDelete(UA_Client *client, UA_Client_Subscription *sub) {
    UA_Client_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &sub->MonitoredItems, listEntry, mon_tmp) {
        UA_NodeId_deleteMembers(&mon->monitoredNodeId);
        LIST_REMOVE(mon, listEntry);
        UA_free(mon);
    }
    LIST_REMOVE(sub, listEntry);
    UA_free(sub);
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

    /* remove the monitoreditem remotely */
    UA_DeleteMonitoredItemsRequest request;
    UA_DeleteMonitoredItemsRequest_init(&request);
    request.subscriptionId = sub->SubscriptionID;
    request.monitoredItemIdsSize = 1;
    request.monitoredItemIds = &mon->MonitoredItemId;
    UA_DeleteMonitoredItemsResponse response = UA_Client_Service_deleteMonitoredItems(client, request);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize > 1)
        retval = response.results[0];
    UA_DeleteMonitoredItemsResponse_deleteMembers(&response);
    if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADMONITOREDITEMIDINVALID) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Could not remove monitoreditem %u with statuscode 0x%08x",
                    monitoredItemId, retval);
        return retval;
    }

    LIST_REMOVE(mon, listEntry);
    UA_NodeId_deleteMembers(&mon->monitoredNodeId);
    UA_free(mon);
    return UA_STATUSCODE_GOOD;
}

static void
UA_Client_processPublishResponse(UA_Client *client, UA_PublishRequest *request, UA_PublishResponse *response) {
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

    /* Check if the server has acknowledged any of the sent ACKs */
    for(size_t i = 0; i < response->resultsSize && i < request->subscriptionAcknowledgementsSize; i++) {
        /* remove also acks that are unknown to the server */
        if(response->results[i] != UA_STATUSCODE_GOOD &&
           response->results[i] != UA_STATUSCODE_BADSEQUENCENUMBERUNKNOWN)
            continue;

        /* Remove the ack from the list */
        UA_SubscriptionAcknowledgement *orig_ack = &request->subscriptionAcknowledgements[i];
        UA_Client_NotificationsAckNumber *ack;
        LIST_FOREACH(ack, &client->pendingNotificationsAcks, listEntry) {
            if(ack->subAck.subscriptionId == orig_ack->subscriptionId &&
               ack->subAck.sequenceNumber == orig_ack->sequenceNumber) {
                LIST_REMOVE(ack, listEntry);
                UA_free(ack);
                UA_assert(ack != LIST_FIRST(&client->pendingNotificationsAcks));
                break;
            }
        }
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
    UA_Client_NotificationsAckNumber *tmpAck = UA_malloc(sizeof(UA_Client_NotificationsAckNumber));
    tmpAck->subAck.sequenceNumber = msg->sequenceNumber;
    tmpAck->subAck.subscriptionId = sub->SubscriptionID;
    LIST_INSERT_HEAD(&client->pendingNotificationsAcks, tmpAck, listEntry);
}

UA_StatusCode UA_Client_Subscriptions_manuallySendPublishRequest(UA_Client *client) {
    if (client->state == UA_CLIENTSTATE_ERRORED)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    UA_Boolean moreNotifications = true;
    while(moreNotifications) {
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
        
        int i = 0;
        LIST_FOREACH(ack, &client->pendingNotificationsAcks, listEntry) {
            request.subscriptionAcknowledgements[i].sequenceNumber = ack->subAck.sequenceNumber;
            request.subscriptionAcknowledgements[i].subscriptionId = ack->subAck.subscriptionId;
            i++;
        }
        
        UA_PublishResponse response = UA_Client_Service_publish(client, request);
        UA_Client_processPublishResponse(client, &request, &response);
        moreNotifications = response.moreNotifications;
        
        UA_PublishResponse_deleteMembers(&response);
        UA_PublishRequest_deleteMembers(&request);
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
