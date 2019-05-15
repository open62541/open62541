/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2015-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) Sten Grüner
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2016-2017 (c) Florian Palm
 *    Copyright 2017 (c) Frank Meerkötter
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>

#include "ua_client_internal.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

/*****************/
/* Subscriptions */
/*****************/

UA_CreateSubscriptionResponse UA_EXPORT
UA_Client_Subscriptions_create(UA_Client *client,
                               const UA_CreateSubscriptionRequest request,
                               void *subscriptionContext,
                               UA_Client_StatusChangeNotificationCallback statusChangeCallback,
                               UA_Client_DeleteSubscriptionCallback deleteCallback) {
    UA_CreateSubscriptionResponse response;
    UA_CreateSubscriptionResponse_init(&response);
    
    /* Allocate the internal representation */
    UA_Client_Subscription *newSub = (UA_Client_Subscription*)
        UA_malloc(sizeof(UA_Client_Subscription));
    if(!newSub) {
        response.responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return response;
    }

    /* Send the request as a synchronous service call */
    __UA_Client_Service(client,
                        &request, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE]);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_free(newSub);
        return response;
    }

    /* Prepare the internal representation */
    newSub->context = subscriptionContext;
    newSub->subscriptionId = response.subscriptionId;
    newSub->sequenceNumber = 0;
    newSub->lastActivity = UA_DateTime_nowMonotonic();
    newSub->statusChangeCallback = statusChangeCallback;
    newSub->deleteCallback = deleteCallback;
    newSub->publishingInterval = response.revisedPublishingInterval;
    newSub->maxKeepAliveCount = response.revisedMaxKeepAliveCount;
    LIST_INIT(&newSub->monitoredItems);
    LIST_INSERT_HEAD(&client->subscriptions, newSub, listEntry);

    return response;
}

static UA_Client_Subscription *
findSubscription(const UA_Client *client, UA_UInt32 subscriptionId) {
    UA_Client_Subscription *sub = NULL;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if(sub->subscriptionId == subscriptionId)
            break;
    }
    return sub;
}

UA_ModifySubscriptionResponse UA_EXPORT
UA_Client_Subscriptions_modify(UA_Client *client, const UA_ModifySubscriptionRequest request) {
    UA_ModifySubscriptionResponse response;
    UA_ModifySubscriptionResponse_init(&response);

    /* Find the internal representation */
    UA_Client_Subscription *sub = findSubscription(client, request.subscriptionId);
    if(!sub) {
        response.responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return response;
    }
    
    /* Call the service */
    __UA_Client_Service(client,
                        &request, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE]);

    /* Adjust the internal representation */
    sub->publishingInterval = response.revisedPublishingInterval;
    sub->maxKeepAliveCount = response.revisedMaxKeepAliveCount;
    return response;
}

static void
UA_Client_Subscription_deleteInternal(UA_Client *client, UA_Client_Subscription *sub) {
    /* Remove the MonitoredItems */
    UA_Client_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &sub->monitoredItems, listEntry, mon_tmp)
        UA_Client_MonitoredItem_remove(client, sub, mon);

    /* Call the delete callback */
    if(sub->deleteCallback)
        sub->deleteCallback(client, sub->subscriptionId, sub->context);

    /* Remove */
    LIST_REMOVE(sub, listEntry);
    UA_free(sub);
}

UA_DeleteSubscriptionsResponse UA_EXPORT
UA_Client_Subscriptions_delete(UA_Client *client, const UA_DeleteSubscriptionsRequest request) {
    UA_STACKARRAY(UA_Client_Subscription*, subs, request.subscriptionIdsSize);
    memset(subs, 0, sizeof(void*) * request.subscriptionIdsSize);

    /* temporary remove the subscriptions from the list */
    for(size_t i = 0; i < request.subscriptionIdsSize; i++) {
        subs[i] = findSubscription(client, request.subscriptionIds[i]);
        if (subs[i])
            LIST_REMOVE(subs[i], listEntry);
    }

    /* Send the request */
    UA_DeleteSubscriptionsResponse response;
    __UA_Client_Service(client,
                        &request, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE]);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        goto cleanup;

    if(request.subscriptionIdsSize != response.resultsSize) {
        response.responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Loop over the removed subscriptions and remove internally */
    for(size_t i = 0; i < request.subscriptionIdsSize; i++) {
        if(response.results[i] != UA_STATUSCODE_GOOD &&
           response.results[i] != UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID) {
            /* Something was wrong, reinsert the subscription in the list */
            if (subs[i])
                LIST_INSERT_HEAD(&client->subscriptions, subs[i], listEntry);
            continue;
        }

        if(!subs[i]) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "No internal representation of subscription %u",
                        request.subscriptionIds[i]);
            continue;
        }
        
        LIST_INSERT_HEAD(&client->subscriptions, subs[i], listEntry);
        UA_Client_Subscription_deleteInternal(client, subs[i]);
    }

    return response;

cleanup:
    for(size_t i = 0; i < request.subscriptionIdsSize; i++) {
        if (subs[i]) {
            LIST_INSERT_HEAD(&client->subscriptions, subs[i], listEntry);
        }
    }
    return response;
}

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_deleteSingle(UA_Client *client, UA_UInt32 subscriptionId) {
    UA_DeleteSubscriptionsRequest request;
    UA_DeleteSubscriptionsRequest_init(&request);
    request.subscriptionIds = &subscriptionId;
    request.subscriptionIdsSize = 1;
    
    UA_DeleteSubscriptionsResponse response =
        UA_Client_Subscriptions_delete(client, request);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DeleteSubscriptionsResponse_deleteMembers(&response);
        return retval;
    }

    if(response.resultsSize != 1) {
        UA_DeleteSubscriptionsResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    retval = response.results[0];
    UA_DeleteSubscriptionsResponse_deleteMembers(&response);
    return retval;
}

/******************/
/* MonitoredItems */
/******************/

void
UA_Client_MonitoredItem_remove(UA_Client *client, UA_Client_Subscription *sub,
                               UA_Client_MonitoredItem *mon) {
    // NOLINTNEXTLINE
    LIST_REMOVE(mon, listEntry);
    if(mon->deleteCallback)
        mon->deleteCallback(client, sub->subscriptionId, sub->context,
                            mon->monitoredItemId, mon->context);
    UA_free(mon);
}

static void
__UA_Client_MonitoredItems_create(UA_Client *client,
                                  const UA_CreateMonitoredItemsRequest *request,
                                  void **contexts, void **handlingCallbacks,
                                  UA_Client_DeleteMonitoredItemCallback *deleteCallbacks,
                                  UA_CreateMonitoredItemsResponse *response) {
    UA_CreateMonitoredItemsResponse_init(response);

    if (!request->itemsToCreateSize) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* Fix clang warning */
    size_t itemsToCreateSize = request->itemsToCreateSize;
    UA_Client_Subscription *sub = NULL;
    
    /* Allocate the memory for internal representations */
    UA_STACKARRAY(UA_Client_MonitoredItem*, mis, itemsToCreateSize);
    memset(mis, 0, sizeof(void*) * itemsToCreateSize);
    for(size_t i = 0; i < itemsToCreateSize; i++) {
        mis[i] = (UA_Client_MonitoredItem*)UA_malloc(sizeof(UA_Client_MonitoredItem));
        if(!mis[i]) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
    }

    /* Get the subscription */
    sub = findSubscription(client, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        goto cleanup;
    }

    /* Set the clientHandle */
    for(size_t i = 0; i < itemsToCreateSize; i++)
        request->itemsToCreate[i].requestedParameters.clientHandle = ++(client->monitoredItemHandles);

    /* Call the service */
    __UA_Client_Service(client, request, &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST],
                        response, &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE]);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        goto cleanup;

    if(response->resultsSize != itemsToCreateSize) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Add internally */
    for(size_t i = 0; i < itemsToCreateSize; i++) {
        if(response->results[i].statusCode != UA_STATUSCODE_GOOD) {
            if (deleteCallbacks[i])
                deleteCallbacks[i](client, sub->subscriptionId, sub->context, 0, contexts[i]);
            UA_free(mis[i]);
            mis[i] = NULL;
            continue;
        }
            
        UA_Client_MonitoredItem *newMon = mis[i];
        newMon->clientHandle = request->itemsToCreate[i].requestedParameters.clientHandle;
        newMon->monitoredItemId = response->results[i].monitoredItemId;
        newMon->context = contexts[i];
        newMon->deleteCallback = deleteCallbacks[i];
        newMon->handler.dataChangeCallback =
            (UA_Client_DataChangeNotificationCallback)(uintptr_t)handlingCallbacks[i];
        newMon->isEventMonitoredItem =
            (request->itemsToCreate[i].itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER);
        LIST_INSERT_HEAD(&sub->monitoredItems, newMon, listEntry);

        UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Subscription %u | Added a MonitoredItem with handle %u",
                     sub->subscriptionId, newMon->clientHandle);
    }

    return;

 cleanup:
    for(size_t i = 0; i < itemsToCreateSize; i++) {
        if (deleteCallbacks[i]) {
            if (sub)
                deleteCallbacks[i](client, sub->subscriptionId, sub->context, 0, contexts[i]);
            else
                deleteCallbacks[i](client, 0, NULL, 0, contexts[i]);
        }
        if(mis[i])
            UA_free(mis[i]);
    }
}

UA_CreateMonitoredItemsResponse UA_EXPORT
UA_Client_MonitoredItems_createDataChanges(UA_Client *client,
            const UA_CreateMonitoredItemsRequest request, void **contexts,
            UA_Client_DataChangeNotificationCallback *callbacks,
            UA_Client_DeleteMonitoredItemCallback *deleteCallbacks) {
    UA_CreateMonitoredItemsResponse response;
    __UA_Client_MonitoredItems_create(client, &request, contexts,
                (void**)(uintptr_t)callbacks, deleteCallbacks, &response);
    return response;
}

UA_MonitoredItemCreateResult UA_EXPORT
UA_Client_MonitoredItems_createDataChange(UA_Client *client, UA_UInt32 subscriptionId,
          UA_TimestampsToReturn timestampsToReturn, const UA_MonitoredItemCreateRequest item,
          void *context, UA_Client_DataChangeNotificationCallback callback,
          UA_Client_DeleteMonitoredItemCallback deleteCallback) {
    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.timestampsToReturn = timestampsToReturn;
    request.itemsToCreate = (UA_MonitoredItemCreateRequest*)(uintptr_t)&item;
    request.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse response = 
       UA_Client_MonitoredItems_createDataChanges(client, request, &context,
                                                   &callback, &deleteCallback);
    UA_MonitoredItemCreateResult result;
    UA_MonitoredItemCreateResult_init(&result);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        result.statusCode = response.responseHeader.serviceResult;

    if(result.statusCode == UA_STATUSCODE_GOOD &&
       response.resultsSize != 1)
        result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
    
    if(result.statusCode == UA_STATUSCODE_GOOD)
       UA_MonitoredItemCreateResult_copy(&response.results[0] , &result);
    UA_CreateMonitoredItemsResponse_deleteMembers(&response);
    return result;
}

UA_CreateMonitoredItemsResponse UA_EXPORT
UA_Client_MonitoredItems_createEvents(UA_Client *client,
            const UA_CreateMonitoredItemsRequest request, void **contexts,
            UA_Client_EventNotificationCallback *callback,
            UA_Client_DeleteMonitoredItemCallback *deleteCallback) {
    UA_CreateMonitoredItemsResponse response;
    __UA_Client_MonitoredItems_create(client, &request, contexts,
                (void**)(uintptr_t)callback, deleteCallback, &response);
    return response;
}

UA_MonitoredItemCreateResult UA_EXPORT
UA_Client_MonitoredItems_createEvent(UA_Client *client, UA_UInt32 subscriptionId,
          UA_TimestampsToReturn timestampsToReturn, const UA_MonitoredItemCreateRequest item,
          void *context, UA_Client_EventNotificationCallback callback,
          UA_Client_DeleteMonitoredItemCallback deleteCallback) {
    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.timestampsToReturn = timestampsToReturn;
    request.itemsToCreate = (UA_MonitoredItemCreateRequest*)(uintptr_t)&item;
    request.itemsToCreateSize = 1;
    UA_CreateMonitoredItemsResponse response = 
       UA_Client_MonitoredItems_createEvents(client, request, &context,
                                             &callback, &deleteCallback);
    UA_MonitoredItemCreateResult result;
    UA_MonitoredItemCreateResult_copy(response.results , &result);
    UA_CreateMonitoredItemsResponse_deleteMembers(&response);
    return result;
}

UA_DeleteMonitoredItemsResponse UA_EXPORT
UA_Client_MonitoredItems_delete(UA_Client *client, const UA_DeleteMonitoredItemsRequest request) {
    /* Send the request */
    UA_DeleteMonitoredItemsResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE]);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return response;

    UA_Client_Subscription *sub = findSubscription(client, request.subscriptionId);
    if(!sub) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "No internal representation of subscription %u",
                    request.subscriptionId);
        return response;
    }

    /* Loop over deleted MonitoredItems */
    for(size_t i = 0; i < response.resultsSize; i++) {
        if(response.results[i] != UA_STATUSCODE_GOOD &&
           response.results[i] != UA_STATUSCODE_BADMONITOREDITEMIDINVALID) {
            continue;
        }

#ifndef __clang_analyzer__
        /* Delete the internal representation */
        UA_Client_MonitoredItem *mon;
        LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
            // NOLINTNEXTLINE
            if (mon->monitoredItemId == request.monitoredItemIds[i]) {
                UA_Client_MonitoredItem_remove(client, sub, mon);
                break;
            }
        }
#endif
    }

    return response;
}

UA_StatusCode UA_EXPORT
UA_Client_MonitoredItems_deleteSingle(UA_Client *client, UA_UInt32 subscriptionId, UA_UInt32 monitoredItemId) {
    UA_DeleteMonitoredItemsRequest request;
    UA_DeleteMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.monitoredItemIds = &monitoredItemId;
    request.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse response =
        UA_Client_MonitoredItems_delete(client, request);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DeleteMonitoredItemsResponse_deleteMembers(&response);
        return retval;
    }

    if(response.resultsSize != 1) {
        UA_DeleteMonitoredItemsResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    retval = response.results[0];
    UA_DeleteMonitoredItemsResponse_deleteMembers(&response);
    return retval;
}

/*************************************/
/* Async Processing of Notifications */
/*************************************/

/* Assume the request is already initialized */
UA_StatusCode
UA_Client_preparePublishRequest(UA_Client *client, UA_PublishRequest *request) {
    /* Count acks */
    UA_Client_NotificationsAckNumber *ack;
    LIST_FOREACH(ack, &client->pendingNotificationsAcks, listEntry)
        ++request->subscriptionAcknowledgementsSize;

    /* Create the array. Returns a sentinel pointer if the length is zero. */
    request->subscriptionAcknowledgements = (UA_SubscriptionAcknowledgement*)
        UA_Array_new(request->subscriptionAcknowledgementsSize,
                     &UA_TYPES[UA_TYPES_SUBSCRIPTIONACKNOWLEDGEMENT]);
    if(!request->subscriptionAcknowledgements) {
        request->subscriptionAcknowledgementsSize = 0;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    size_t i = 0;
    UA_Client_NotificationsAckNumber *ack_tmp;
    LIST_FOREACH_SAFE(ack, &client->pendingNotificationsAcks, listEntry, ack_tmp) {
        request->subscriptionAcknowledgements[i].sequenceNumber = ack->subAck.sequenceNumber;
        request->subscriptionAcknowledgements[i].subscriptionId = ack->subAck.subscriptionId;
        ++i;
        LIST_REMOVE(ack, listEntry);
        UA_free(ack);
    }
    return UA_STATUSCODE_GOOD;
}

/* According to OPC Unified Architecture, Part 4 5.13.1.1 i) */
/* The value 0 is never used for the sequence number         */
static UA_UInt32
UA_Client_Subscriptions_nextSequenceNumber(UA_UInt32 sequenceNumber) {
    UA_UInt32 nextSequenceNumber = sequenceNumber + 1;
    if(nextSequenceNumber == 0)
        nextSequenceNumber = 1;
    return nextSequenceNumber;
}

static void
processDataChangeNotification(UA_Client *client, UA_Client_Subscription *sub,
                              UA_DataChangeNotification *dataChangeNotification) {
    for(size_t j = 0; j < dataChangeNotification->monitoredItemsSize; ++j) {
        UA_MonitoredItemNotification *min = &dataChangeNotification->monitoredItems[j];

        /* Find the MonitoredItem */
        UA_Client_MonitoredItem *mon;
        LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
            if(mon->clientHandle == min->clientHandle)
                break;
        }

        if(!mon) {
            UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Could not process a notification with clienthandle %u on subscription %u",
                         min->clientHandle, sub->subscriptionId);
            continue;
        }

        if(mon->isEventMonitoredItem) {
            UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "MonitoredItem is configured for Events. But received a "
                         "DataChangeNotification.");
            continue;
        }

        mon->handler.dataChangeCallback(client, sub->subscriptionId, sub->context,
                                        mon->monitoredItemId, mon->context,
                                        &min->value);
    }
}

static void
processEventNotification(UA_Client *client, UA_Client_Subscription *sub,
                         UA_EventNotificationList *eventNotificationList) {
    for(size_t j = 0; j < eventNotificationList->eventsSize; ++j) {
        UA_EventFieldList *eventFieldList = &eventNotificationList->events[j];

        /* Find the MonitoredItem */
        UA_Client_MonitoredItem *mon;
        LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
            if(mon->clientHandle == eventFieldList->clientHandle)
                break;
        }

        if(!mon) {
            UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Could not process a notification with clienthandle %u on subscription %u",
                         eventFieldList->clientHandle, sub->subscriptionId);
            continue;
        }

        if(!mon->isEventMonitoredItem) {
            UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "MonitoredItem is configured for DataChanges. But received a "
                         "EventNotification.");
            continue;
        }

        mon->handler.eventCallback(client, sub->subscriptionId, sub->context,
                                   mon->monitoredItemId, mon->context,
                                   eventFieldList->eventFieldsSize,
                                   eventFieldList->eventFields);
    }
}

static void
processNotificationMessage(UA_Client *client, UA_Client_Subscription *sub,
                           UA_ExtensionObject *msg) {
    if(msg->encoding != UA_EXTENSIONOBJECT_DECODED)
        return;

    /* Handle DataChangeNotification */
    if(msg->content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION]) {
        UA_DataChangeNotification *dataChangeNotification =
            (UA_DataChangeNotification *)msg->content.decoded.data;
        processDataChangeNotification(client, sub, dataChangeNotification);
        return;
    }

    /* Handle EventNotification */
    if(msg->content.decoded.type == &UA_TYPES[UA_TYPES_EVENTNOTIFICATIONLIST]) {
        UA_EventNotificationList *eventNotificationList =
            (UA_EventNotificationList *)msg->content.decoded.data;
        processEventNotification(client, sub, eventNotificationList);
        return;
    }

    /* Handle StatusChangeNotification */
    if(msg->content.decoded.type == &UA_TYPES[UA_TYPES_STATUSCHANGENOTIFICATION]) {
        if(sub->statusChangeCallback) {
            sub->statusChangeCallback(client, sub->subscriptionId, sub->context,
                                      (UA_StatusChangeNotification*)msg->content.decoded.data);
        } else {
            UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "Dropped a StatusChangeNotification since no callback is registered");
        }
        return;
    }

    UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                   "Unknown notification message type");
}

void
UA_Client_Subscriptions_processPublishResponse(UA_Client *client, UA_PublishRequest *request,
                                               UA_PublishResponse *response) {
    UA_NotificationMessage *msg = &response->notificationMessage;

    client->currentlyOutStandingPublishRequests--;

    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADTOOMANYPUBLISHREQUESTS) {
        if(client->config.outStandingPublishRequests > 1) {
            client->config.outStandingPublishRequests--;
            UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                          "Too many publishrequest, reduce outStandingPublishRequests to %d",
                           client->config.outStandingPublishRequests);
        } else {
            UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Too many publishrequest when outStandingPublishRequests = 1");
            UA_Client_Subscriptions_deleteSingle(client, response->subscriptionId);
        }
        return;
    }

    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADSHUTDOWN)
        return;

    if(!LIST_FIRST(&client->subscriptions)) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOSUBSCRIPTION;
        return;
    }

    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADSESSIONCLOSED) {
        if(client->state >= UA_CLIENTSTATE_SESSION) {
            UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "Received Publish Response with code %s",
                            UA_StatusCode_name(response->responseHeader.serviceResult));
            UA_Client_Subscription* sub = findSubscription(client, response->subscriptionId);
            if (sub != NULL)
              UA_Client_Subscription_deleteInternal(client, sub);
        }
        return;
    }

    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADSESSIONIDINVALID) {
        UA_Client_disconnect(client); /* TODO: This should be handled before the process callback */
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Received BadSessionIdInvalid");
        return;
    }

    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Received Publish Response with code %s",
                       UA_StatusCode_name(response->responseHeader.serviceResult));
        return;
    }

    UA_Client_Subscription *sub = findSubscription(client, response->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Received Publish Response for a non-existant subscription");
        return;
    }

    sub->lastActivity = UA_DateTime_nowMonotonic();

    /* Detect missing message - OPC Unified Architecture, Part 4 5.13.1.1 e) */
    if(UA_Client_Subscriptions_nextSequenceNumber(sub->sequenceNumber) != msg->sequenceNumber) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Invalid subscription sequence number: expected %u but got %u",
                     UA_Client_Subscriptions_nextSequenceNumber(sub->sequenceNumber),
                     msg->sequenceNumber);
        /* This is an error. But we do not abort the connection. Some server
         * SDKs misbehave from time to time and send out-of-order sequence
         * numbers. (Probably some multi-threading synchronization issue.) */
        /* UA_Client_disconnect(client);
           return; */
    }
    /* According to f), a keep-alive message contains no notifications and has the sequence number
     * of the next NotificationMessage that is to be sent => More than one consecutive keep-alive
     * message or a NotificationMessage following a keep-alive message will share the same sequence
     * number. */
    if (msg->notificationDataSize)
        sub->sequenceNumber = msg->sequenceNumber;

    /* Process the notification messages */
    for(size_t k = 0; k < msg->notificationDataSize; ++k)
        processNotificationMessage(client, sub, &msg->notificationData[k]);

    /* Add to the list of pending acks */
    for(size_t i = 0; i < response->availableSequenceNumbersSize; i++) {
        if(response->availableSequenceNumbers[i] != msg->sequenceNumber)
            continue;
        UA_Client_NotificationsAckNumber *tmpAck = (UA_Client_NotificationsAckNumber*)
            UA_malloc(sizeof(UA_Client_NotificationsAckNumber));
        if(!tmpAck) {
            UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "Not enough memory to store the acknowledgement for a publish "
                           "message on subscription %u", sub->subscriptionId);
            break;
        }   
        tmpAck->subAck.sequenceNumber = msg->sequenceNumber;
        tmpAck->subAck.subscriptionId = sub->subscriptionId;
        LIST_INSERT_HEAD(&client->pendingNotificationsAcks, tmpAck, listEntry);
        break;
    } 
}

static void
processPublishResponseAsync(UA_Client *client, void *userdata, UA_UInt32 requestId,
                            void *response) {
    UA_PublishRequest *req = (UA_PublishRequest*)userdata;
    UA_PublishResponse *res = (UA_PublishResponse*)response;

    /* Process the response */
    UA_Client_Subscriptions_processPublishResponse(client, req, res);

    /* Delete the cached request */
    UA_PublishRequest_delete(req);

    /* Fill up the outstanding publish requests */
    UA_Client_Subscriptions_backgroundPublish(client);
}

void
UA_Client_Subscriptions_clean(UA_Client *client) {
    UA_Client_NotificationsAckNumber *n, *tmp;
    LIST_FOREACH_SAFE(n, &client->pendingNotificationsAcks, listEntry, tmp) {
        LIST_REMOVE(n, listEntry);
        UA_free(n);
    }

    UA_Client_Subscription *sub, *tmps;
    LIST_FOREACH_SAFE(sub, &client->subscriptions, listEntry, tmps)
        UA_Client_Subscription_deleteInternal(client, sub); /* force local removal */

    client->monitoredItemHandles = 0;
}

void
UA_Client_Subscriptions_backgroundPublishInactivityCheck(UA_Client *client) {
    if(client->state < UA_CLIENTSTATE_SESSION)
        return;

    /* Is the lack of responses the client's fault? */
    if(client->currentlyOutStandingPublishRequests == 0)
        return;

    UA_Client_Subscription *sub;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        UA_DateTime maxSilence = (UA_DateTime)
            ((sub->publishingInterval * sub->maxKeepAliveCount) +
             client->config.timeout) * UA_DATETIME_MSEC;
        if(maxSilence + sub->lastActivity < UA_DateTime_nowMonotonic()) {
            /* Reset activity */
            sub->lastActivity = UA_DateTime_nowMonotonic();

            if(client->config.subscriptionInactivityCallback)
                client->config.subscriptionInactivityCallback(client, sub->subscriptionId,
                                                              sub->context);
            UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Inactivity for Subscription %u.", sub->subscriptionId);
        }
    }
}

UA_StatusCode
UA_Client_Subscriptions_backgroundPublish(UA_Client *client) {
    if(client->state < UA_CLIENTSTATE_SESSION)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    /* The session must have at least one subscription */
    if(!LIST_FIRST(&client->subscriptions))
        return UA_STATUSCODE_GOOD;

    while(client->currentlyOutStandingPublishRequests < client->config.outStandingPublishRequests) {
        UA_PublishRequest *request = UA_PublishRequest_new();
        if (!request)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        request->requestHeader.timeoutHint=60000;
        UA_StatusCode retval = UA_Client_preparePublishRequest(client, request);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_PublishRequest_delete(request);
            return retval;
        }
    
        UA_UInt32 requestId;
        client->currentlyOutStandingPublishRequests++;

        /* Disable the timeout, it is treat in UA_Client_Subscriptions_backgroundPublishInactivityCheck */
        retval = __UA_Client_AsyncServiceEx(client, request, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                                            processPublishResponseAsync,
                                            &UA_TYPES[UA_TYPES_PUBLISHRESPONSE],
                                            (void*)request, &requestId, 0);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_PublishRequest_delete(request);
            return retval;
        }
    }

    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
