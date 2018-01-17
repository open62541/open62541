/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_client_highlevel.h"
#include "ua_client_internal.h"
#include "ua_util.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

UA_StatusCode
UA_Client_Subscription_create(UA_Client *client,
                              const UA_SubscriptionParameters *requestedParameters,
                              void *subscriptionContext,
                              UA_StatusChangeNotificationCallback statusChangeCallback,
                              UA_DeleteSubscriptionCallback deleteCallback,
                              UA_UInt32 *newSubscriptionId) {
    /* Prepare the request */
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);
    request.requestedPublishingInterval = requestedParameters->publishingInterval;
    request.requestedLifetimeCount = requestedParameters->lifetimeCount;
    request.requestedMaxKeepAliveCount = requestedParameters->maxKeepAliveCount;
    request.maxNotificationsPerPublish = requestedParameters->maxNotificationsPerPublish;
    request.publishingEnabled = requestedParameters->publishingEnabled;
    request.priority = requestedParameters->priority;

    /* Allocate the internal representation */
    UA_Client_Subscription *newSub = (UA_Client_Subscription*)
        UA_malloc(sizeof(UA_Client_Subscription));
    if(!newSub)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Send the request as a synchronous service call */
    UA_CreateSubscriptionResponse response = UA_Client_Service_createSubscription(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newSub);
        UA_CreateSubscriptionResponse_deleteMembers(&response);
        return retval;
    }

    /* Prepare the internal representation */
    newSub->context = subscriptionContext;
    newSub->subscriptionId = response.subscriptionId;
    newSub->parameters.publishingInterval = request.publishingEnabled;
    newSub->parameters.lifetimeCount = response.revisedLifetimeCount;
    newSub->parameters.maxKeepAliveCount = response.revisedMaxKeepAliveCount;
    newSub->parameters.publishingInterval = response.revisedPublishingInterval;
    newSub->parameters.maxNotificationsPerPublish = request.maxNotificationsPerPublish;
    newSub->parameters.priority = request.priority;
    newSub->sequenceNumber = 0;
    newSub->lastActivity = UA_DateTime_nowMonotonic();
    newSub->statusChangeCallback = statusChangeCallback;
    newSub->deleteCallback = deleteCallback;
    LIST_INIT(&newSub->monitoredItems);
    LIST_INSERT_HEAD(&client->subscriptions, newSub, listEntry);

    /* Finish */
    if(newSubscriptionId)
        *newSubscriptionId = newSub->subscriptionId;
    UA_CreateSubscriptionResponse_deleteMembers(&response);
    return UA_STATUSCODE_GOOD;
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

UA_StatusCode UA_EXPORT
UA_Client_Subscription_getParameters(UA_Client *client, UA_UInt32 subscriptionId,
                                   UA_SubscriptionParameters *parameters) {
    UA_Client_Subscription *sub = findSubscription(client, subscriptionId);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    *parameters = sub->parameters;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_Subscription_setParameters(UA_Client *client, UA_UInt32 subscriptionId,
                                     const UA_SubscriptionParameters *parameters) {
    UA_Client_Subscription *sub = findSubscription(client, subscriptionId);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    /* Set the Publishing Mode */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(parameters->publishingEnabled != sub->parameters.publishingEnabled) {
        UA_SetPublishingModeRequest request;
        UA_SetPublishingModeRequest_init(&request);
        request.publishingEnabled = parameters->publishingEnabled;
        request.subscriptionIds = &sub->subscriptionId;
        request.subscriptionIdsSize = 1;
        UA_SetPublishingModeResponse response =
            UA_Client_Service_setPublishingMode(client, request);
        retval = response.responseHeader.serviceResult;
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        if(response.resultsSize != 1) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }
        retval = response.results[0];

    cleanup:
        UA_SetPublishingModeResponse_deleteMembers(&response);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Have other parameters changed? */
    UA_ModifySubscriptionRequest request;
    UA_ModifySubscriptionResponse response;
    if(parameters->publishingInterval != sub->parameters.publishingInterval)
        goto sendmodify;
    if(parameters->lifetimeCount != sub->parameters.lifetimeCount)
        goto sendmodify;
    if(parameters->maxKeepAliveCount != sub->parameters.maxKeepAliveCount)
        goto sendmodify;
    if(parameters->maxNotificationsPerPublish != sub->parameters.maxNotificationsPerPublish)
        goto sendmodify;
    if(parameters->priority != sub->parameters.priority)
        goto sendmodify;
    
    return UA_STATUSCODE_GOOD;

 sendmodify:
    /* Send parameter changes */
    UA_ModifySubscriptionRequest_init(&request);
    request.subscriptionId = sub->subscriptionId;
    request.requestedPublishingInterval = parameters->publishingInterval;
    request.requestedLifetimeCount = parameters->lifetimeCount;
    request.requestedMaxKeepAliveCount = parameters->maxKeepAliveCount;
    request.maxNotificationsPerPublish = parameters->maxNotificationsPerPublish;
    request.priority = parameters->priority;
    response = UA_Client_Service_modifySubscription(client, request);

    /* Set the internal parameters */
    retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        sub->parameters.publishingInterval = response.revisedPublishingInterval;
        sub->parameters.lifetimeCount = response.revisedLifetimeCount;
        sub->parameters.maxKeepAliveCount = response.revisedMaxKeepAliveCount;
    }
    UA_ModifySubscriptionResponse_deleteMembers(&response);
    return UA_STATUSCODE_GOOD;
}

static void
UA_Client_Subscription_deleteInternal(UA_Client *client,
                                      UA_Client_Subscription *sub) {
    /* Remove the MonitoredItems */
    UA_Client_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &sub->monitoredItems, listEntry, mon_tmp) {
        UA_NodeId_deleteMembers(&mon->monitoredNodeId);
        LIST_REMOVE(mon, listEntry);
        UA_free(mon);
    }

    /* Call the delete callback */
    if(sub->deleteCallback)
        sub->deleteCallback(client, sub->subscriptionId, sub->context);

    /* Remove */
    LIST_REMOVE(sub, listEntry);
    UA_free(sub);
}

UA_StatusCode
UA_Client_Subscription_delete(UA_Client *client, UA_UInt32 subscriptionId) {
    UA_Client_Subscription *sub = findSubscription(client, subscriptionId);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    /* remove the subscription from the list    */
    /* will be reinserted after if error occurs */
    LIST_REMOVE(sub, listEntry);

    /* remove the subscription remotely */
    UA_DeleteSubscriptionsRequest request;
    UA_DeleteSubscriptionsRequest_init(&request);
    request.subscriptionIdsSize = 1;
    request.subscriptionIds = &sub->subscriptionId;
    UA_DeleteSubscriptionsResponse response = UA_Client_Service_deleteSubscriptions(client, request);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize > 0)
        retval = response.results[0];
    UA_DeleteSubscriptionsResponse_deleteMembers(&response);

    if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID) {
        /* error occurs re-insert the subscription in the list */
        LIST_INSERT_HEAD(&client->subscriptions, sub, listEntry);
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Could not remove subscription %u with error code %s",
                    sub->subscriptionId, UA_StatusCode_name(retval));
        return retval;
    }

    UA_Client_Subscription_deleteInternal(client, sub);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addMonitoredItems(UA_Client *client, const UA_UInt32 subscriptionId,
                  UA_MonitoredItemCreateRequest *items, size_t itemsSize,
                  void **hfs, void **hfContexts, UA_StatusCode *itemResults,
                  UA_UInt32 *newMonitoredItemIds, UA_Boolean isEventMonitoredItem) {
    UA_Client_Subscription *sub = findSubscription(client, subscriptionId);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    UA_CreateMonitoredItemsResponse response;
    UA_CreateMonitoredItemsResponse_init(&response);

    /* Create the handlers */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Client_MonitoredItem **mis = (UA_Client_MonitoredItem**)
        UA_alloca(sizeof(void*) * itemsSize);
    memset(mis, 0, sizeof(void*) * itemsSize);
    for(size_t i = 0; i < itemsSize; i++) {
        mis[i] = (UA_Client_MonitoredItem*)UA_malloc(sizeof(UA_Client_MonitoredItem));
        if(!mis[i]) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
    }

    /* Set the clientHandle */
    for(size_t i = 0; i < itemsSize; i++)
        items[i].requestedParameters.clientHandle = ++(client->monitoredItemHandles);

    /* Initialize the request */
    request.subscriptionId = subscriptionId;
    request.itemsToCreate = items;
    request.itemsToCreateSize = itemsSize;

    /* Send the request */
    response = UA_Client_Service_createMonitoredItems(client, request);

    /* Remove for _deleteMembers */
    request.itemsToCreate = NULL;
    request.itemsToCreateSize = 0;

    retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    if(response.resultsSize != itemsSize) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    for(size_t i = 0; i < itemsSize; i++) {
        UA_MonitoredItemCreateResult *result = &response.results[i];
        UA_Client_MonitoredItem *newMon = mis[i];

        itemResults[i] = result->statusCode;
        if(result->statusCode != UA_STATUSCODE_GOOD) {
            newMonitoredItemIds[i] = 0;
            UA_free(newMon);
            continue;
        }

        /* Set the internal representation */
        newMon->monitoringMode = UA_MONITORINGMODE_REPORTING;
        UA_NodeId_copy(&items[i].itemToMonitor.nodeId, &newMon->monitoredNodeId);
        newMon->attributeId = items[i].itemToMonitor.attributeId;
        newMon->clientHandle = items[i].requestedParameters.clientHandle;
        newMon->samplingInterval = result->revisedSamplingInterval;
        newMon->queueSize = result->revisedQueueSize;
        newMon->discardOldest = items[i].requestedParameters.discardOldest;
        newMon->monitoredItemId = response.results[i].monitoredItemId;
        newMon->isEventMonitoredItem = isEventMonitoredItem;
        /* eventHandler is at the same position in the union */
        newMon->handler.dataChangeHandler = (UA_MonitoredItemHandlingFunction)(uintptr_t)hfs[i];
        newMon->context = hfContexts[i];

        LIST_INSERT_HEAD(&sub->monitoredItems, newMon, listEntry);
        newMonitoredItemIds[i] = newMon->monitoredItemId;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Created a monitored item with client handle %u",
                     client->monitoredItemHandles);
    }

 cleanup:
    if(retval != UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < itemsSize; i++)
            UA_free(mis[i]);
    }
    UA_CreateMonitoredItemsRequest_deleteMembers(&request);
    UA_CreateMonitoredItemsResponse_deleteMembers(&response);
    return retval;
}


UA_StatusCode
UA_Client_Subscriptions_addMonitoredItems(UA_Client *client, const UA_UInt32 subscriptionId,
                                          UA_MonitoredItemCreateRequest *items, size_t itemsSize, 
                                          UA_MonitoredItemHandlingFunction *hfs,
                                          void **hfContexts, UA_StatusCode *itemResults,
                                          UA_UInt32 *newMonitoredItemIds) {
    return addMonitoredItems(client, subscriptionId, items, itemsSize, (void**)hfs,
                             hfContexts, itemResults, newMonitoredItemIds, false);
}

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                         UA_NodeId nodeId, UA_UInt32 attributeId,
                                         UA_MonitoredItemHandlingFunction hf, void *hfContext,
                                         UA_UInt32 *newMonitoredItemId, UA_Double samplingInterval) {
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = nodeId;
    item.itemToMonitor.attributeId = attributeId;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    item.requestedParameters.samplingInterval = samplingInterval;
    item.requestedParameters.discardOldest = true;
    item.requestedParameters.queueSize = 1;

    UA_StatusCode retval_item = UA_STATUSCODE_GOOD;
    UA_StatusCode retval = addMonitoredItems(client, subscriptionId, &item, 1,
                                             (void**)(uintptr_t)&hf, &hfContext,
                                             &retval_item, newMonitoredItemId, false);
    return retval | retval_item;
}


UA_StatusCode
UA_Client_Subscriptions_addMonitoredEvents(UA_Client *client, const UA_UInt32 subscriptionId,
                                           UA_MonitoredItemCreateRequest *items, size_t itemsSize, 
                                           UA_MonitoredEventHandlingFunction *hfs,
                                           void **hfContexts, UA_StatusCode *itemResults,
                                           UA_UInt32 *newMonitoredItemIds) {
    return addMonitoredItems(client, subscriptionId, items, itemsSize, (void**)hfs,
                             hfContexts, itemResults, newMonitoredItemIds, true);
}

UA_StatusCode
UA_Client_Subscriptions_addMonitoredEvent(UA_Client *client, UA_UInt32 subscriptionId,
                                          const UA_NodeId nodeId, UA_UInt32 attributeId,
                                          const UA_SimpleAttributeOperand *selectClauses,
                                          size_t selectClausesSize,
                                          const UA_ContentFilterElement *whereClauses,
                                          size_t whereClausesSize,
                                          const UA_MonitoredEventHandlingFunction hf,
                                          void *hfContext, UA_UInt32 *newMonitoredItemId) {
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = nodeId;
    item.itemToMonitor.attributeId = attributeId;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    item.requestedParameters.samplingInterval = 0;
    item.requestedParameters.discardOldest = false;

    UA_EventFilter *evFilter = UA_EventFilter_new();
    if(!evFilter)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_EventFilter_init(evFilter);
    evFilter->selectClausesSize = selectClausesSize;
    evFilter->selectClauses = (UA_SimpleAttributeOperand*)(uintptr_t)selectClauses;
    evFilter->whereClause.elementsSize = whereClausesSize;
    evFilter->whereClause.elements = (UA_ContentFilterElement*)(uintptr_t)whereClauses;

    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
    item.requestedParameters.filter.content.decoded.data = evFilter;
    UA_StatusCode retval_item = UA_STATUSCODE_GOOD;
    UA_StatusCode retval = addMonitoredItems(client, subscriptionId, &item, 1,
                                             (void**)(uintptr_t)&hf, &hfContext,
                                             &retval_item, newMonitoredItemId, true);
    UA_free(evFilter);
    return retval | retval_item;
}

UA_StatusCode
UA_Client_Subscriptions_removeMonitoredItems(UA_Client *client, UA_UInt32 subscriptionId,
                                            UA_UInt32 *monitoredItemId, size_t itemsSize,
                                            UA_StatusCode *itemResults) {
    UA_Client_Subscription *sub = findSubscription(client, subscriptionId);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    /* remove the monitoreditem remotely */
    UA_DeleteMonitoredItemsRequest request;
    UA_DeleteMonitoredItemsRequest_init(&request);
    request.subscriptionId = sub->subscriptionId;
    request.monitoredItemIdsSize = itemsSize;
    request.monitoredItemIds = monitoredItemId;
    UA_DeleteMonitoredItemsResponse response = UA_Client_Service_deleteMonitoredItems(client, request);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    if(response.resultsSize != itemsSize) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    for(size_t i = 0; i < itemsSize; i++) {
        itemResults[i] = response.results[i];
        if(response.results[i] != UA_STATUSCODE_GOOD &&
           response.results[i] != UA_STATUSCODE_BADMONITOREDITEMIDINVALID) {
            UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Could not remove monitoreditem %u with error code %s",
                        monitoredItemId[i], UA_StatusCode_name(response.results[i]));
            continue;
        }
        UA_Client_MonitoredItem *mon;
        LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
            if(mon->monitoredItemId == monitoredItemId[i]) {
                LIST_REMOVE(mon, listEntry);
                UA_NodeId_deleteMembers(&mon->monitoredNodeId);
                UA_free(mon);
                break;
            }
        }
    }

 cleanup:
    /* Remove for _deleteMembers */
    request.monitoredItemIdsSize = 0;
    request.monitoredItemIds = NULL;

    UA_DeleteMonitoredItemsRequest_deleteMembers(&request);
    UA_DeleteMonitoredItemsResponse_deleteMembers(&response);

    return retval;
}

UA_StatusCode
UA_Client_Subscriptions_removeMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                            UA_UInt32 monitoredItemId) {
    UA_StatusCode retval_item = UA_STATUSCODE_GOOD;
    UA_StatusCode retval = UA_Client_Subscriptions_removeMonitoredItems(client, subscriptionId,
                                                                        &monitoredItemId, 1,
                                                                        &retval_item);
    return retval | retval_item;
}

/* Assume the request is already initialized */
static UA_StatusCode
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
        UA_MonitoredItemNotification *mitemNot = &dataChangeNotification->monitoredItems[j];

        /* Find the MonitoredItem */
        UA_Client_MonitoredItem *mon;
        LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
            if(mon->clientHandle == mitemNot->clientHandle)
                break;
        }

        if(!mon) {
            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Could not process a notification with clienthandle %u on subscription %u",
                         mitemNot->clientHandle, sub->subscriptionId);
            continue;
        }

        if(mon->isEventMonitoredItem) {
            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "MonitoredItem is configured for Events. But received a "
                         "DataChangeNotification.");
            continue;
        }

        mon->handler.dataChangeHandler(mon->monitoredItemId, &mitemNot->value, mon->context);
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
            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Could not process a notification with clienthandle %u on subscription %u",
                         eventFieldList->clientHandle, sub->subscriptionId);
            continue;
        }

        if(!mon->isEventMonitoredItem) {
            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "MonitoredItem is configured for DataChanges. But received a "
                         "EventNotification.");
            continue;
        }

        mon->handler.eventHandler(mon->monitoredItemId, eventFieldList->eventFieldsSize,
                                  eventFieldList->eventFields, mon->context);
    }
}

static void
processNotificationMessage(UA_Client *client, UA_Client_Subscription *sub,
                           UA_ExtensionObject *msg) {
    if(msg->encoding != UA_EXTENSIONOBJECT_DECODED)
        return;

    if(msg->content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION]) {
        processDataChangeNotification(client, sub, (UA_DataChangeNotification *)
                                      msg->content.decoded.data);
    } else if(msg->content.decoded.type == &UA_TYPES[UA_TYPES_EVENTNOTIFICATIONLIST]) {
        processEventNotification(client, sub, (UA_EventNotificationList *)
                                 msg->content.decoded.data);
    } else if(msg->content.decoded.type == &UA_TYPES[UA_TYPES_STATUSCHANGENOTIFICATION]) {
        if(sub->statusChangeCallback) {
            sub->statusChangeCallback(client, sub->subscriptionId, sub->context,
                                      (UA_StatusChangeNotification*)msg->content.decoded.data);
        } else {
            UA_LOG_WARNING(client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "Dropped a StatusChangeNotification since no callback is registered");
        }
    }
}

static void
processPublishResponse(UA_Client *client, UA_PublishRequest *request,
                       UA_PublishResponse *response) {
    UA_NotificationMessage *msg = &response->notificationMessage;

    client->currentlyOutStandingPublishRequests--;

    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADTOOMANYPUBLISHREQUESTS){
        if(client->config.outStandingPublishRequests > 1) {
            client->config.outStandingPublishRequests--;
            UA_LOG_WARNING(client->config.logger, UA_LOGCATEGORY_CLIENT,
                          "Too many publishrequest, reduce outStandingPublishRequests to %d",
                           client->config.outStandingPublishRequests);
        } else {
            UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Too many publishrequest when outStandingPublishRequests = 1");
            UA_Client_close(client); /* TODO: Close the subscription but not the session */
        }
        goto cleanup;
    }

    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADNOSUBSCRIPTION){
       if(LIST_FIRST(&client->subscriptions)) {
            UA_Client_close(client);
            UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "PublishRequest error : No subscription");
        }
        goto cleanup;
    }

    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADSESSIONIDINVALID){
        UA_Client_close(client);
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Received BadSessionIdInvalid");
        goto cleanup;
    }

    UA_Client_Subscription *sub = findSubscription(client, response->subscriptionId);
    if(!sub)
        goto cleanup;
    sub->lastActivity = UA_DateTime_nowMonotonic();

    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Detect missing message - OPC Unified Architecture, Part 4 5.13.1.1 e) */
    if((sub->sequenceNumber != msg->sequenceNumber) &&
        (UA_Client_Subscriptions_nextSequenceNumber(sub->sequenceNumber) != msg->sequenceNumber)) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Invalid subscritpion sequenceNumber");
        UA_Client_close(client);
        goto cleanup;
    }
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
            UA_LOG_WARNING(client->config.logger, UA_LOGCATEGORY_CLIENT,
                           "Not enough memory to store the acknowledgement for a publish "
                           "message on subscription %u", sub->subscriptionId);
            break;
        }   
        tmpAck->subAck.sequenceNumber = msg->sequenceNumber;
        tmpAck->subAck.subscriptionId = sub->subscriptionId;
        LIST_INSERT_HEAD(&client->pendingNotificationsAcks, tmpAck, listEntry);
        break;
    } 

 cleanup:
    UA_PublishRequest_deleteMembers(request);
}

static void
processPublishResponseAsync(UA_Client *client, void *userdata, UA_UInt32 requestId,
                            void *response, const UA_DataType *responseType) {
    UA_PublishRequest *req = (UA_PublishRequest*)userdata;
    UA_PublishResponse *res = (UA_PublishResponse*)response;
    processPublishResponse(client, req, res);
    UA_PublishRequest_delete(req);
    /* Fill up the outstanding publish requests */
    UA_Client_Subscriptions_backgroundPublish(client);
}

UA_StatusCode
UA_Client_Subscriptions_manuallySendPublishRequest(UA_Client *client) {
    if(client->state < UA_CLIENTSTATE_SESSION)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime maxDate = now + (UA_DateTime)(client->config.timeout * UA_DATETIME_MSEC);

    UA_Boolean moreNotifications = true;
    while(moreNotifications) {
        UA_PublishRequest request;
        UA_PublishRequest_init(&request);
        retval = UA_Client_preparePublishRequest(client, &request);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        /* Manually increase the number of sent publish requests. Otherwise we
         * send out one too many when we process async responses when we wait
         * for the correct publish response. The
         * currentlyOutStandingPublishRequests will be reduced during processing
         * of the response. */
        client->currentlyOutStandingPublishRequests++;

        UA_PublishResponse response = UA_Client_Service_publish(client, request);
        processPublishResponse(client, &request, &response);
        
        now = UA_DateTime_nowMonotonic();
        if(now > maxDate) {
            moreNotifications = UA_FALSE;
            retval = UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
        } else {
            moreNotifications = response.moreNotifications;
        }
        
        UA_PublishResponse_deleteMembers(&response);
        UA_PublishRequest_deleteMembers(&request);
    }
    
    if (client->state < UA_CLIENTSTATE_SESSION)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    return retval;
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

        UA_StatusCode retval = UA_Client_preparePublishRequest(client, request);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_PublishRequest_delete(request);
            return retval;
        }
    
        UA_UInt32 requestId;
        client->currentlyOutStandingPublishRequests++;
        retval = __UA_Client_AsyncService(client, request, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                                          processPublishResponseAsync,
                                          &UA_TYPES[UA_TYPES_PUBLISHRESPONSE],
                                          (void*)request, &requestId);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_PublishRequest_delete(request);
            return retval;
        }
    }

    /* Check subscriptions inactivity */
    UA_Client_Subscription *sub;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        UA_DateTime maxSilence = (UA_DateTime)((sub->parameters.publishingInterval * sub->parameters.maxKeepAliveCount) +
                                               client->config.timeout) * UA_DATETIME_MSEC;
        if(maxSilence + sub->lastActivity < UA_DateTime_nowMonotonic()) {
            UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Inactivity for Subscription %d. Closing the connection.",
                         sub->subscriptionId);
            UA_Client_close(client);
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
    }

    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
