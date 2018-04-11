/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2015-2018 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) Sten Grüner
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2016-2017 (c) Florian Palm
 *    Copyright 2017 (c) Frank Meerkötter
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_client_highlevel.h"
#include "ua_client_internal.h"
#include "ua_util.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

const UA_SubscriptionSettings UA_SubscriptionSettings_default = {
    500.0, /* .requestedPublishingInterval */
    10000, /* .requestedLifetimeCount */
    1, /* .requestedMaxKeepAliveCount */
    0, /* .maxNotificationsPerPublish */
    true, /* .publishingEnabled */
    0 /* .priority */
};

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
    
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && newSubscriptionId)
        *newSubscriptionId = response.subscriptionId;
    
    UA_CreateSubscriptionResponse_deleteMembers(&response);
    return retval;
}

UA_StatusCode
UA_Client_Subscriptions_remove(UA_Client *client, UA_UInt32 subscriptionId) {
    UA_DeleteSubscriptionsRequest request;
    UA_DeleteSubscriptionsRequest_init(&request);
    request.subscriptionIdsSize = 1;
    request.subscriptionIds = &subscriptionId;

    UA_DeleteSubscriptionsResponse response =
        UA_Client_Subscriptions_delete(client, request);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(response.resultsSize != 1)
            retval = UA_STATUSCODE_BADINTERNALERROR;
    }

    if(retval == UA_STATUSCODE_GOOD)
        retval = response.results[0];

    UA_DeleteSubscriptionsResponse_deleteMembers(&response);
    return retval;
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

        UA_PublishResponse response;
        __UA_Client_Service(client,
                            &request, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                            &response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
        UA_Client_Subscriptions_processPublishResponse(client, &request, &response);
        UA_PublishRequest_deleteMembers(&request);
        
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

    if(client->state < UA_CLIENTSTATE_SESSION)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    return retval;
}

/* Callbacks for the MonitoredItems. The callbacks for the deprecated API are
 * wrapped. The wrapper is cleaned up upon destruction. */

typedef struct {
    UA_MonitoredItemHandlingFunction origCallback;
    void *context;
} dataChangeCallbackWrapper;

static void
dataChangeCallback(UA_Client *client, UA_UInt32 subId, void *subContext,
                   UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    dataChangeCallbackWrapper *wrapper = (dataChangeCallbackWrapper*)monContext;
    wrapper->origCallback(client, monId, value, wrapper->context);
}

typedef struct {
    UA_MonitoredEventHandlingFunction origCallback;
    void *context;
} eventCallbackWrapper;

static void
eventCallback(UA_Client *client, UA_UInt32 subId, void *subContext,
              UA_UInt32 monId, void *monContext, size_t nEventFields,
              UA_Variant *eventFields) {
    eventCallbackWrapper *wrapper = (eventCallbackWrapper*)monContext;
    wrapper->origCallback(client, monId, nEventFields, eventFields, wrapper->context);
}

static void
deleteMonitoredItemCallback(UA_Client *client, UA_UInt32 subId, void *subContext,
                            UA_UInt32 monId, void *monContext) {
    UA_free(monContext);
}

static UA_StatusCode
addMonitoredItems(UA_Client *client, const UA_UInt32 subscriptionId,
                  UA_MonitoredItemCreateRequest *items, size_t itemsSize,
                  UA_MonitoredItemHandlingFunction *hfs, void **hfContexts,
                  UA_StatusCode *itemResults, UA_UInt32 *newMonitoredItemIds) {
    /* Create array of wrappers and callbacks */
    UA_STACKARRAY(dataChangeCallbackWrapper*, wrappers, itemsSize);
    UA_STACKARRAY(UA_Client_DeleteMonitoredItemCallback, deleteCbs, itemsSize);
    UA_STACKARRAY(UA_Client_DataChangeNotificationCallback, wrapperCbs, itemsSize);

    for(size_t i = 0; i < itemsSize; i++) {
        wrappers[i] = (dataChangeCallbackWrapper*)UA_malloc(sizeof(dataChangeCallbackWrapper));
        if(!wrappers[i]) {
            for(size_t j = 0; j < i; j++)
                UA_free(wrappers[j]);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        wrappers[i]->origCallback = (UA_MonitoredItemHandlingFunction)(uintptr_t)hfs[i];
        wrappers[i]->context = hfContexts[i];

        deleteCbs[i] = deleteMonitoredItemCallback;
        wrapperCbs[i] = dataChangeCallback;
    }

    /* Prepare the request */
    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.itemsToCreateSize = itemsSize;
    request.itemsToCreate = items;

    /* Process and return */
    UA_CreateMonitoredItemsResponse response =
        UA_Client_MonitoredItems_createDataChanges(client, request, (void**)wrappers,
                                                   wrapperCbs, deleteCbs);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize != itemsSize)
        retval = UA_STATUSCODE_BADINTERNALERROR;

    if(retval == UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < itemsSize; i++) {
            itemResults[i] = response.results[i].statusCode;
            newMonitoredItemIds[i] = response.results[i].monitoredItemId;
        }
    }

    UA_CreateMonitoredItemsResponse_deleteMembers(&response);
    return retval;
}

UA_StatusCode
UA_Client_Subscriptions_addMonitoredItems(UA_Client *client, const UA_UInt32 subscriptionId,
                                          UA_MonitoredItemCreateRequest *items, size_t itemsSize,
                                          UA_MonitoredItemHandlingFunction *hfs,
                                          void **hfContexts, UA_StatusCode *itemResults,
                                          UA_UInt32 *newMonitoredItemIds) {
    return addMonitoredItems(client, subscriptionId, items, itemsSize, hfs, hfContexts, itemResults,
                             newMonitoredItemIds);
}

UA_StatusCode
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
    UA_StatusCode retval =
        addMonitoredItems(client, subscriptionId, &item, 1,
                          (UA_MonitoredItemHandlingFunction*)(uintptr_t)&hf,
                          &hfContext, &retval_item, newMonitoredItemId);
    return retval | retval_item;
}

static UA_StatusCode
addMonitoredEvents(UA_Client *client, const UA_UInt32 subscriptionId,
                   UA_MonitoredItemCreateRequest *items, size_t itemsSize,
                   UA_MonitoredEventHandlingFunction *hfs,
                   void **hfContexts, UA_StatusCode *itemResults,
                   UA_UInt32 *newMonitoredItemIds) {
    /* Create array of wrappers and callbacks */
    UA_STACKARRAY(eventCallbackWrapper*, wrappers, itemsSize);
    UA_STACKARRAY(UA_Client_DeleteMonitoredItemCallback, deleteCbs, itemsSize);
    UA_STACKARRAY(UA_Client_EventNotificationCallback, wrapperCbs, itemsSize);

    for(size_t i = 0; i < itemsSize; i++) {
        wrappers[i] = (eventCallbackWrapper*)UA_malloc(sizeof(eventCallbackWrapper));
        if(!wrappers[i]) {
            for(size_t j = 0; j < i; j++)
                UA_free(wrappers[j]);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        wrappers[i]->origCallback = (UA_MonitoredEventHandlingFunction)(uintptr_t)hfs[i];
        wrappers[i]->context = hfContexts[i];

        deleteCbs[i] = deleteMonitoredItemCallback;
        wrapperCbs[i] = eventCallback;
    }

    /* Prepare the request */
    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.itemsToCreateSize = itemsSize;
    request.itemsToCreate = items;

    /* Process and return */
    UA_CreateMonitoredItemsResponse response =
        UA_Client_MonitoredItems_createEvents(client, request, (void**)wrappers,
                                              wrapperCbs, deleteCbs);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD && response.resultsSize != itemsSize)
        retval = UA_STATUSCODE_BADINTERNALERROR;

    if(retval == UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < itemsSize; i++)
            itemResults[i] = response.results[i].statusCode;
    }

    UA_CreateMonitoredItemsResponse_deleteMembers(&response);
    return retval;
}

UA_StatusCode
UA_Client_Subscriptions_addMonitoredEvents(UA_Client *client, const UA_UInt32 subscriptionId,
                                           UA_MonitoredItemCreateRequest *items, size_t itemsSize,
                                           UA_MonitoredEventHandlingFunction *hfs,
                                           void **hfContexts, UA_StatusCode *itemResults,
                                           UA_UInt32 *newMonitoredItemIds) {
    return addMonitoredEvents(client, subscriptionId, items, itemsSize, hfs,
                              hfContexts, itemResults, newMonitoredItemIds);
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
    UA_StatusCode retval = addMonitoredEvents(client, subscriptionId, &item, 1,
                                             (UA_MonitoredEventHandlingFunction*)(uintptr_t)&hf,
                                              &hfContext, &retval_item, newMonitoredItemId);
    UA_free(evFilter);
    return retval | retval_item;
}

static UA_StatusCode
removeMonitoredItems(UA_Client *client, UA_UInt32 subscriptionId,
                     UA_UInt32 *monitoredItemIds, size_t itemsSize,
                     UA_StatusCode *itemResults) {
    UA_DeleteMonitoredItemsRequest request;
    UA_DeleteMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.monitoredItemIdsSize = itemsSize;
    request.monitoredItemIds = monitoredItemIds;

    UA_DeleteMonitoredItemsResponse response = UA_Client_MonitoredItems_delete(client, request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        if(response.resultsSize != itemsSize) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
        } else {
            for(size_t i = 0; i < itemsSize; i++)
                itemResults[i] = response.results[i];
        }
    }
    UA_DeleteMonitoredItemsResponse_deleteMembers(&response);
    return retval;
}

UA_StatusCode
UA_Client_Subscriptions_removeMonitoredItems(UA_Client *client, UA_UInt32 subscriptionId,
                                             UA_UInt32 *monitoredItemIds, size_t itemsSize,
                                             UA_StatusCode *itemResults) {
    return removeMonitoredItems(client, subscriptionId, monitoredItemIds, itemsSize, itemResults);
}

UA_StatusCode
UA_Client_Subscriptions_removeMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                            UA_UInt32 monitoredItemId) {
    UA_StatusCode retval_item = UA_STATUSCODE_GOOD;
    UA_StatusCode retval = removeMonitoredItems(client, subscriptionId, &monitoredItemId, 1, &retval_item);
    return retval | retval_item;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
