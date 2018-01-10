/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_client_highlevel.h"
#include "ua_client_internal.h"
#include "ua_util.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

UA_StatusCode
UA_Client_Subscriptions_new(UA_Client *client, UA_SubscriptionSettings settings,
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
    if(retval != UA_STATUSCODE_GOOD) {
        UA_CreateSubscriptionResponse_deleteMembers(&response);
        return retval;
    }

    UA_Client_Subscription *newSub = (UA_Client_Subscription *)UA_malloc(sizeof(UA_Client_Subscription));
    if(!newSub) {
        UA_CreateSubscriptionResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    LIST_INIT(&newSub->monitoredItems);
    newSub->sequenceNumber = 0;
    newSub->lastActivity = UA_DateTime_nowMonotonic();
    newSub->lifeTime = response.revisedLifetimeCount;
    newSub->keepAliveCount = response.revisedMaxKeepAliveCount;
    newSub->publishingInterval = response.revisedPublishingInterval;
    newSub->subscriptionID = response.subscriptionId;
    newSub->notificationsPerPublish = request.maxNotificationsPerPublish;
    newSub->priority = request.priority;
    LIST_INSERT_HEAD(&client->subscriptions, newSub, listEntry);

    if(newSubscriptionId)
        *newSubscriptionId = newSub->subscriptionID;

    UA_CreateSubscriptionResponse_deleteMembers(&response);
    return UA_STATUSCODE_GOOD;
}

static UA_Client_Subscription *
findSubscription(const UA_Client *client, UA_UInt32 subscriptionId) {
    UA_Client_Subscription *sub = NULL;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if(sub->subscriptionID == subscriptionId)
            break;
    }
    return sub;
}

UA_StatusCode
UA_Client_Subscriptions_remove(UA_Client *client, UA_UInt32 subscriptionId) {
    UA_Client_Subscription *sub = findSubscription(client, subscriptionId);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Client_MonitoredItem *mon, *tmpmon;
    LIST_FOREACH_SAFE(mon, &sub->monitoredItems, listEntry, tmpmon) {
        retval = UA_Client_Subscriptions_removeMonitoredItem(client, sub->subscriptionID,
                                                             mon->monitoredItemId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* remove the subscription remotely */
    UA_DeleteSubscriptionsRequest request;
    UA_DeleteSubscriptionsRequest_init(&request);
    request.subscriptionIdsSize = 1;
    request.subscriptionIds = &sub->subscriptionID;
    UA_DeleteSubscriptionsResponse response = UA_Client_Service_deleteSubscriptions(client, request);
    retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize > 0)
        retval = response.results[0];
    UA_DeleteSubscriptionsResponse_deleteMembers(&response);

    if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Could not remove subscription %u with error code %s",
                    sub->subscriptionID, UA_StatusCode_name(retval));
        return retval;
    }

    UA_Client_Subscriptions_forceDelete(client, sub);
    return UA_STATUSCODE_GOOD;
}

void
UA_Client_Subscriptions_forceDelete(UA_Client *client,
                                    UA_Client_Subscription *sub) {
    UA_Client_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &sub->monitoredItems, listEntry, mon_tmp) {
        UA_NodeId_deleteMembers(&mon->monitoredNodeId);
        LIST_REMOVE(mon, listEntry);
        UA_free(mon);
    }
    LIST_REMOVE(sub, listEntry);
    UA_free(sub);
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
        newMon->attributeID = items[i].itemToMonitor.attributeId;
        newMon->clientHandle = items[i].requestedParameters.clientHandle;
        newMon->samplingInterval = result->revisedSamplingInterval;
        newMon->queueSize = result->revisedQueueSize;
        newMon->discardOldest = items[i].requestedParameters.discardOldest;
        newMon->monitoredItemId = response.results[i].monitoredItemId;
        newMon->isEventMonitoredItem = isEventMonitoredItem;
        /* eventHandler is at the same position in the union */
        newMon->handler.dataChangeHandler = (UA_MonitoredItemHandlingFunction)(uintptr_t)hfs[i];
        newMon->handlerContext = hfContexts[i];

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
                                         UA_NodeId nodeId, UA_UInt32 attributeID,
                                         UA_MonitoredItemHandlingFunction hf, void *hfContext,
                                         UA_UInt32 *newMonitoredItemId, UA_Double samplingInterval) {
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = nodeId;
    item.itemToMonitor.attributeId = attributeID;
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
                                          const UA_NodeId nodeId, UA_UInt32 attributeID,
                                          const UA_SimpleAttributeOperand *selectClauses,
                                          size_t selectClausesSize,
                                          const UA_ContentFilterElement *whereClauses,
                                          size_t whereClausesSize,
                                          const UA_MonitoredEventHandlingFunction hf,
                                          void *hfContext, UA_UInt32 *newMonitoredItemId) {
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = nodeId;
    item.itemToMonitor.attributeId = attributeID;
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
    request.subscriptionId = sub->subscriptionID;
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
processNotificationMessage(UA_Client *client, UA_Client_Subscription *sub,
                           UA_ExtensionObject *msg) {
    if(msg->encoding != UA_EXTENSIONOBJECT_DECODED)
        return;

    /* Handle DataChangeNotification */
    if(msg->content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION]) {
        UA_DataChangeNotification *dataChangeNotification =
            (UA_DataChangeNotification *)msg->content.decoded.data;
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
                             mitemNot->clientHandle, sub->subscriptionID);
                return;
            }

            if(mon->isEventMonitoredItem) {
                UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                             "MonitoredItem is configured for Events. But received a "
                             "DataChangeNotification.");
                return;
            }

            mon->handler.dataChangeHandler(mon->monitoredItemId, &mitemNot->value, mon->handlerContext);
        }
        return;
    }

    /* Handle EventNotification */
    if(msg->content.decoded.type == &UA_TYPES[UA_TYPES_EVENTNOTIFICATIONLIST]) {
        UA_EventNotificationList *eventNotificationList =
            (UA_EventNotificationList *)msg->content.decoded.data;
        for (size_t j = 0; j < eventNotificationList->eventsSize; ++j) {
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
                             eventFieldList->clientHandle, sub->subscriptionID);
                return;
            }

            if(!mon->isEventMonitoredItem) {
                UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                             "MonitoredItem is configured for DataChanges. But received a "
                             "EventNotification.");
                return;
            }

            mon->handler.eventHandler(mon->monitoredItemId, eventFieldList->eventFieldsSize,
                                      eventFieldList->eventFields, mon->handlerContext);
        }
    }
}

static void
processPublishResponse(UA_Client *client, UA_PublishRequest *request,
                       UA_PublishResponse *response) {
    UA_NotificationMessage *msg = &response->notificationMessage;

    /* Handle outstanding requests. TODO: If the server cannot handle so many
     * publish requests, reduce the maximum number. */
    client->currentlyOutStandingPublishRequests--;

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
                           "message on subscription %u", sub->subscriptionID);
            break;
        }   
        tmpAck->subAck.sequenceNumber = msg->sequenceNumber;
        tmpAck->subAck.subscriptionId = sub->subscriptionID;
        LIST_INSERT_HEAD(&client->pendingNotificationsAcks, tmpAck, listEntry);
        break;
    } 

 cleanup:
    UA_PublishRequest_deleteMembers(request);
    /* Fill up the outstanding publish requests */
    UA_Client_Subscriptions_backgroundPublish(client);
}

static void
processPublishResponseAsync(UA_Client *client, void *userdata, UA_UInt32 requestId,
                            void *response, const UA_DataType *responseType) {
    UA_PublishRequest *req = (UA_PublishRequest*)userdata;
    UA_PublishResponse *res = (UA_PublishResponse*)response;
    processPublishResponse(client, req, res);
    UA_PublishRequest_delete(req);
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
        UA_Client_Subscriptions_forceDelete(client, sub); /* force local removal */
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
        if(((UA_DateTime)(sub->publishingInterval * sub->keepAliveCount + client->config.timeout) * 
             UA_DATETIME_MSEC + sub->lastActivity) < UA_DateTime_nowMonotonic()) {
            UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Inactivity for Subscription %d. Closing the connection.",
                         sub->subscriptionID);
            UA_Client_close(client);
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
    }

    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
