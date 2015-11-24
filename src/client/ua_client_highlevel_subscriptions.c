#include "ua_client_highlevel.h"
#include "ua_client_internal.h"
#include "ua_util.h"
#include "ua_types_generated_encoding_binary.h"

const UA_SubscriptionSettings UA_SubscriptionSettings_standard = {
    .requestedPublishingInterval = 0.0,
    .requestedLifetimeCount = 100,
    .requestedMaxKeepAliveCount = 10,
    .maxNotificationsPerPublish = 10,
    .publishingEnabled = UA_TRUE,
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
                                         void *handlingFunction, UA_UInt32 *newMonitoredItemId) {
    UA_Client_Subscription *sub;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if(sub->SubscriptionID == subscriptionId)
            break;
    }
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
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
    item.requestedParameters.discardOldest = UA_TRUE;
    item.requestedParameters.queueSize = 1;
    request.itemsToCreate = &item;
    request.itemsToCreateSize = 1;
    // Filter can be left void for now, only changes are supported (UA_Expert does the same with changeItems)
    
    UA_CreateMonitoredItemsResponse response = UA_Client_Service_createMonitoredItems(client, request);
    
    UA_StatusCode retval;
    // slight misuse of retval here to check if the deletion was successfull.
    if(response.resultsSize == 0)
        retval = response.responseHeader.serviceResult;
    else
        retval = response.results[0].statusCode;
    
    if(retval == UA_STATUSCODE_GOOD) {
        UA_Client_MonitoredItem *newMon = UA_malloc(sizeof(UA_Client_MonitoredItem));
        newMon->MonitoringMode = UA_MONITORINGMODE_REPORTING;
        UA_NodeId_copy(&nodeId, &newMon->monitoredNodeId); 
        newMon->AttributeID = attributeID;
        newMon->ClientHandle = client->monitoredItemHandles;
        newMon->SamplingInterval = sub->PublishingInterval;
        newMon->QueueSize = 1;
        newMon->DiscardOldest = UA_TRUE;
        newMon->handler = handlingFunction;
        newMon->MonitoredItemId = response.results[0].monitoredItemId;
        LIST_INSERT_HEAD(&sub->MonitoredItems, newMon, listEntry);
        *newMonitoredItemId = newMon->MonitoredItemId;
    }
    
    UA_CreateMonitoredItemsResponse_deleteMembers(&response);
    return retval;
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

static UA_Boolean
UA_Client_processPublishRx(UA_Client *client, UA_PublishResponse response) {
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return UA_FALSE;
    
    // Check if the server has acknowledged any of our ACKS
    // Note that a list of serverside status codes may be send without valid publish data, i.e. 
    // during keepalives or no data availability
    UA_Client_NotificationsAckNumber *ack, *tmpAck;
    size_t i = 0;
    LIST_FOREACH_SAFE(ack, &client->pendingNotificationsAcks, listEntry, tmpAck) {
        if(response.results[i] == UA_STATUSCODE_GOOD ||
           response.results[i] == UA_STATUSCODE_BADSEQUENCENUMBERINVALID) {
            LIST_REMOVE(ack, listEntry);
            UA_free(ack);
        }
        i++;
    }
    
    if(response.subscriptionId == 0)
        return UA_FALSE;
    
    UA_Client_Subscription *sub;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if(sub->SubscriptionID == response.subscriptionId)
            break;
    }
    if(!sub)
        return UA_FALSE;
    
    UA_NotificationMessage msg = response.notificationMessage;
    UA_Client_MonitoredItem *mon;
    for(size_t k = 0; k < msg.notificationDataSize; k++) {
        if(msg.notificationData[k].encoding != UA_EXTENSIONOBJECT_DECODED)
            continue;
        
        if(msg.notificationData[k].content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION]) {
            // This is a dataChangeNotification
            UA_DataChangeNotification *dataChangeNotification = msg.notificationData[k].content.decoded.data;
            for(size_t i = 0; i < dataChangeNotification->monitoredItemsSize; i++) {
            UA_MonitoredItemNotification *mitemNot = &dataChangeNotification->monitoredItems[i];
                // find this client handle
                LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
                    if(mon->ClientHandle == mitemNot->clientHandle) {
                        mon->handler(mitemNot->clientHandle, &mitemNot->value);
                        break;
                    }
                }
            }
            continue;
        }

        /* if(msg.notificationData[k].typeId.namespaceIndex == 0 && */
        /*    msg.notificationData[k].typeId.identifier.numeric == 820 ) { */
        /*     //FIXME: This is a statusChangeNotification (not supported yet) */
        /*     continue; */
        /* } */

        /* if(msg.notificationData[k].typeId.namespaceIndex == 0 && */
        /*    msg.notificationData[k].typeId.identifier.numeric == 916 ) { */
        /*     //FIXME: This is an EventNotification */
        /*     continue; */
        /* } */
    }
    
    /* We processed this message, add it to the list of pending acks (but make
       sure it's not in the list first) */
    LIST_FOREACH(tmpAck, &client->pendingNotificationsAcks, listEntry) {
        if(tmpAck->subAck.sequenceNumber == msg.sequenceNumber &&
            tmpAck->subAck.subscriptionId == response.subscriptionId)
            break;
    }

    if(!tmpAck) {
        tmpAck = UA_malloc(sizeof(UA_Client_NotificationsAckNumber));
        tmpAck->subAck.sequenceNumber = msg.sequenceNumber;
        tmpAck->subAck.subscriptionId = sub->SubscriptionID;
        LIST_INSERT_HEAD(&client->pendingNotificationsAcks, tmpAck, listEntry);
    }
    
    return response.moreNotifications;
}

void UA_Client_Subscriptions_manuallySendPublishRequest(UA_Client *client) {
    UA_Boolean moreNotifications = UA_TRUE;
    do {
        UA_PublishRequest request;
        UA_PublishRequest_init(&request);
        request.subscriptionAcknowledgementsSize = 0;

        UA_Client_NotificationsAckNumber *ack;
        LIST_FOREACH(ack, &client->pendingNotificationsAcks, listEntry)
            request.subscriptionAcknowledgementsSize++;
        if(request.subscriptionAcknowledgementsSize > 0) {
            request.subscriptionAcknowledgements = UA_malloc(sizeof(UA_SubscriptionAcknowledgement) *
                                                             request.subscriptionAcknowledgementsSize);
            if(!request.subscriptionAcknowledgements)
                return;
        }
        
        int index = 0 ;
        LIST_FOREACH(ack, &client->pendingNotificationsAcks, listEntry) {
            request.subscriptionAcknowledgements[index].sequenceNumber = ack->subAck.sequenceNumber;
            request.subscriptionAcknowledgements[index].subscriptionId = ack->subAck.subscriptionId;
            index++;
        }
        
        UA_PublishResponse response = UA_Client_Service_publish(client, request);
        if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            moreNotifications = UA_Client_processPublishRx(client, response);
        else
            moreNotifications = UA_FALSE;
        
        UA_PublishResponse_deleteMembers(&response);
        UA_PublishRequest_deleteMembers(&request);
    } while(moreNotifications == UA_TRUE);
}
