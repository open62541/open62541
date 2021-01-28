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

static void
UA_Client_MonitoredItem_remove(UA_Client *client, UA_Client_Subscription *sub,
                               UA_Client_MonitoredItem *mon);

static void
__Subscriptions_create_handler(UA_Client *client, void *data, UA_UInt32 requestId, void *r) {
    UA_Client_Subscription *newSub = NULL;
    UA_CreateSubscriptionResponse *response = (UA_CreateSubscriptionResponse *)r;
    CustomCallback *cc = (CustomCallback *)data;
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        if(cc->clientData)
            UA_free(cc->clientData);
        goto cleanup;
    }

    /* Prepare the internal representation */
    newSub = (UA_Client_Subscription *)cc->clientData;
    newSub->subscriptionId = response->subscriptionId;
    newSub->sequenceNumber = 0;
    newSub->lastActivity = UA_DateTime_nowMonotonic();
    newSub->publishingInterval = response->revisedPublishingInterval;
    newSub->maxKeepAliveCount = response->revisedMaxKeepAliveCount;
    LIST_INIT(&newSub->monitoredItems);
    LIST_INSERT_HEAD(&client->subscriptions, newSub, listEntry);

cleanup:
    if(cc->isAsync) {
        if(cc->userCallback)
            cc->userCallback(client, cc->userData, requestId, response);
        UA_free(cc);
    }
}

UA_CreateSubscriptionResponse
UA_Client_Subscriptions_create(UA_Client *client,
                               const UA_CreateSubscriptionRequest request,
                               void *subscriptionContext,
                               UA_Client_StatusChangeNotificationCallback statusChangeCallback,
                               UA_Client_DeleteSubscriptionCallback deleteCallback) {
    UA_CreateSubscriptionResponse response;
    UA_CreateSubscriptionResponse_init(&response);

    CustomCallback cc;
    memset(&cc, 0, sizeof(CustomCallback));
#ifdef __clang_analyzer__
    cc.isAsync = false;
#endif

    UA_Client_Subscription *sub = (UA_Client_Subscription *)
        UA_malloc(sizeof(UA_Client_Subscription));
    if(!sub) {
        response.responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return response;
    }
    sub->context = subscriptionContext;
    sub->statusChangeCallback = statusChangeCallback;
    sub->deleteCallback = deleteCallback;
    cc.clientData = sub;

    /* Send the request as a synchronous service call */
    __UA_Client_Service(client,
                        &request, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE]);

    __Subscriptions_create_handler(client, &cc, 0, &response);

    return response;
}

UA_StatusCode
UA_Client_Subscriptions_create_async(UA_Client *client, const UA_CreateSubscriptionRequest request,
                                     void *subscriptionContext,
                                     UA_Client_StatusChangeNotificationCallback statusChangeCallback,
                                     UA_Client_DeleteSubscriptionCallback deleteCallback,
                                     UA_ClientAsyncServiceCallback createCallback, void *userdata,
                                     UA_UInt32 *requestId) {
    CustomCallback *cc = (CustomCallback *)UA_calloc(1, sizeof(CustomCallback));
    if(!cc)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    cc->isAsync = true;
    cc->userCallback = createCallback;
    cc->userData = userdata;

    UA_Client_Subscription *sub = (UA_Client_Subscription *)
        UA_malloc(sizeof(UA_Client_Subscription));
    if(!sub) {
        UA_free(cc);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    sub->context = subscriptionContext;
    sub->statusChangeCallback = statusChangeCallback;
    sub->deleteCallback = deleteCallback;
    cc->clientData = sub;

    /* Send the request as asynchronous service call */
    return __UA_Client_AsyncService(client, &request,
                                    &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST],
                                    __Subscriptions_create_handler,
                                    &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE],
                                    cc, requestId);
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

static void
__Subscriptions_modify_handler(UA_Client *client, void *data, UA_UInt32 requestId, void *r) {
    UA_ModifySubscriptionResponse *response = (UA_ModifySubscriptionResponse *)r;
    CustomCallback *cc = (CustomCallback *)data;
    UA_Client_Subscription *sub = (UA_Client_Subscription *)cc->clientData;

    sub->publishingInterval = response->revisedPublishingInterval;
    sub->maxKeepAliveCount = response->revisedMaxKeepAliveCount;

    if(cc->isAsync) {
        if(cc->userCallback)
            cc->userCallback(client, cc->userData, requestId, response);
        UA_free(cc);
    }
}

UA_ModifySubscriptionResponse
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

UA_StatusCode
UA_Client_Subscriptions_modify_async(UA_Client *client,
                                     const UA_ModifySubscriptionRequest request,
                                     UA_ClientAsyncServiceCallback callback,
                                     void *userdata, UA_UInt32 *requestId) {
    /* Find the internal representation */
    UA_Client_Subscription *sub = findSubscription(client, request.subscriptionId);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    CustomCallback *cc = (CustomCallback *)UA_calloc(1, sizeof(CustomCallback));
    if(!cc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    cc->isAsync = true;
    cc->clientData = sub;
    cc->userData = userdata;
    cc->userCallback = callback;

    return __UA_Client_AsyncService(client, &request,
                                    &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST],
                                    __Subscriptions_modify_handler,
                                    &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE],
                                    cc, requestId);
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

typedef struct {
    UA_DeleteSubscriptionsRequest *request;
    UA_Client_Subscription **subs;
} Subscriptions_DeleteData;

static void
__Subscriptions_DeleteData_free(Subscriptions_DeleteData *data) {
    if(!data)
        return;
    if(data->subs)
        UA_free(data->subs);
    if(data->request)
        UA_delete(data->request, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST]);
    UA_free(data);
}

static UA_INLINE void
__Subscriptions_delete_prepare(UA_Client *client, Subscriptions_DeleteData *data) {
    /* temporary remove the subscriptions from the list */
    for(size_t i = 0; i < data->request->subscriptionIdsSize; i++) {
        data->subs[i] = findSubscription(client, data->request->subscriptionIds[i]);
        if(data->subs[i])
            LIST_REMOVE(data->subs[i], listEntry);
    }
}

static void
__Subscriptions_delete_handler(UA_Client *client, void *data, UA_UInt32 requestId, void *r) {
    UA_DeleteSubscriptionsResponse *response = (UA_DeleteSubscriptionsResponse *)r;
    CustomCallback *cc = (CustomCallback *)data;
    Subscriptions_DeleteData *delData = (Subscriptions_DeleteData *)cc->clientData;
    UA_DeleteSubscriptionsRequest *request = delData->request;
    UA_Client_Subscription **subs = delData->subs;

    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        goto cleanup;

    if(request->subscriptionIdsSize != response->resultsSize) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Loop over the removed subscriptions and remove internally */
    for(size_t i = 0; i < request->subscriptionIdsSize; i++) {
        if(response->results[i] != UA_STATUSCODE_GOOD &&
           response->results[i] != UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID) {
            /* Something was wrong, reinsert the subscription in the list */
            if(subs[i])
                LIST_INSERT_HEAD(&client->subscriptions, subs[i], listEntry);
            continue;
        }

        if(!subs[i]) {
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "No internal representation of subscription %" PRIu32,
                        delData->request->subscriptionIds[i]);
            continue;
        }

        LIST_INSERT_HEAD(&client->subscriptions, subs[i], listEntry);
        UA_Client_Subscription_deleteInternal(client, subs[i]);
    }

cleanup:
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < request->subscriptionIdsSize; i++) {
            if(subs[i]) {
                LIST_INSERT_HEAD(&client->subscriptions, subs[i], listEntry);
            }
        }
    }

    if(cc->isAsync) {
        if(cc->userCallback)
            cc->userCallback(client, cc->userData, requestId, response);
        __Subscriptions_DeleteData_free(delData);
        UA_free(cc);
    }
}

UA_StatusCode
UA_Client_Subscriptions_delete_async(UA_Client *client,
                                     const UA_DeleteSubscriptionsRequest request,
                                     UA_ClientAsyncServiceCallback callback,
                                     void *userdata, UA_UInt32 *requestId) {
    CustomCallback *cc = (CustomCallback *)UA_calloc(1, sizeof(CustomCallback));
    if(!cc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    Subscriptions_DeleteData *data = (Subscriptions_DeleteData *)
        UA_calloc(1, sizeof(Subscriptions_DeleteData));
    if(!data) {
        UA_free(cc);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    cc->clientData = data;

    data->subs = (UA_Client_Subscription **)
        UA_calloc(request.subscriptionIdsSize, sizeof(UA_Client_Subscription *));
    if(!data->subs)
        goto cleanup;

    data->request = UA_DeleteSubscriptionsRequest_new();
    if(!data->request)
        goto cleanup;
    UA_DeleteSubscriptionsRequest_copy(&request, data->request);

    __Subscriptions_delete_prepare(client, data);
    cc->isAsync = true;
    cc->userCallback = callback;
    cc->userData = userdata;

    return __UA_Client_AsyncService(client, &request, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST],
                                    __Subscriptions_delete_handler,
                                    &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE],
                                    cc, requestId);
cleanup:
    __Subscriptions_DeleteData_free(data);
    UA_free(cc);
    return UA_STATUSCODE_BADOUTOFMEMORY;
}

UA_DeleteSubscriptionsResponse
UA_Client_Subscriptions_delete(UA_Client *client,
                               const UA_DeleteSubscriptionsRequest request) {
    UA_STACKARRAY(UA_Client_Subscription *, subs, request.subscriptionIdsSize);
    memset(subs, 0, sizeof(void *) * request.subscriptionIdsSize);

    CustomCallback cc;
    memset(&cc, 0, sizeof(CustomCallback));
#ifdef __clang_analyzer__
    cc.isAsync = false;
#endif

    Subscriptions_DeleteData data;
    cc.clientData = &data;
    data.request = (UA_DeleteSubscriptionsRequest *)(uintptr_t)&request;
    data.subs = subs;

    __Subscriptions_delete_prepare(client, &data);

    /* Send the request */
    UA_DeleteSubscriptionsResponse response;

    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE]);

    __Subscriptions_delete_handler(client, &cc, 0, &response);
    return response;
}

UA_StatusCode
UA_Client_Subscriptions_deleteSingle(UA_Client *client, UA_UInt32 subscriptionId) {
    UA_DeleteSubscriptionsRequest request;
    UA_DeleteSubscriptionsRequest_init(&request);
    request.subscriptionIds = &subscriptionId;
    request.subscriptionIdsSize = 1;
    
    UA_DeleteSubscriptionsResponse response =
        UA_Client_Subscriptions_delete(client, request);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DeleteSubscriptionsResponse_clear(&response);
        return retval;
    }

    if(response.resultsSize != 1) {
        UA_DeleteSubscriptionsResponse_clear(&response);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    retval = response.results[0];
    UA_DeleteSubscriptionsResponse_clear(&response);
    return retval;
}

/******************/
/* MonitoredItems */
/******************/

static void
UA_Client_MonitoredItem_remove(UA_Client *client, UA_Client_Subscription *sub,
                               UA_Client_MonitoredItem *mon) {
    // NOLINTNEXTLINE
    LIST_REMOVE(mon, listEntry);
    if(mon->deleteCallback)
        mon->deleteCallback(client, sub->subscriptionId, sub->context,
                            mon->monitoredItemId, mon->context);
    UA_free(mon);
}

typedef struct {
    UA_Client_Subscription *sub;

    UA_Client_MonitoredItem **mis;
    void **contexts;
    UA_Client_DeleteMonitoredItemCallback *deleteCallbacks;
    void **handlingCallbacks;

    UA_CreateMonitoredItemsRequest *request;
} MonitoredItems_CreateData;

static void
MonitoredItems_CreateData_deleteItems(UA_Client_MonitoredItem **mis,
                                      MonitoredItems_CreateData *data, UA_Client *client) {
    bool hasCallbacks = (data->deleteCallbacks && data->contexts);
    for(size_t i = 0; i < data->request->itemsToCreateSize; i++) {
        if(!mis[i])
            continue;
        UA_free(mis[i]);
        mis[i] = NULL;
        if(hasCallbacks && data->deleteCallbacks[i]) {
            if(data->sub)
                data->deleteCallbacks[i](client, data->sub->subscriptionId,
                                         data->sub->context, 0, data->contexts[i]);
            else
                data->deleteCallbacks[i](client, 0, NULL, 0, data->contexts[i]);
        }
    }
}

static void
__MonitoredItems_create_handler(UA_Client *client, CustomCallback *cc, UA_UInt32 requestId,
                                UA_CreateMonitoredItemsResponse *response) {
    MonitoredItems_CreateData *data = (MonitoredItems_CreateData *)cc->clientData;

    // introduce local pointers to the variables/parameters in the CreateData
    // to keep the code completely intact
    UA_CreateMonitoredItemsRequest *request = data->request;
    UA_Client_DeleteMonitoredItemCallback *deleteCallbacks = data->deleteCallbacks;
    UA_Client_Subscription *sub = data->sub;
    void **contexts = data->contexts;
    UA_Client_MonitoredItem **mis = data->mis;
    void **handlingCallbacks = data->handlingCallbacks;

    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;

    if(response->resultsSize != request->itemsToCreateSize) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* Add internally */
    for(size_t i = 0; i < request->itemsToCreateSize; i++) {
        if(response->results[i].statusCode != UA_STATUSCODE_GOOD) {
            if(deleteCallbacks[i])
                deleteCallbacks[i](client, sub->subscriptionId, sub->context, 0, contexts[i]);
            UA_free(mis[i]);
            mis[i] = NULL;
            continue;
        }

        UA_assert(mis[i] != NULL);
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
                    "Subscription %" PRIu32 " | Added a MonitoredItem with handle %" PRIu32,
                     sub->subscriptionId, newMon->clientHandle);
        mis[i] = NULL;
    }
}

static void
__MonitoredItems_create_async_handler(UA_Client *client, void *d,
                                      UA_UInt32 requestId, void *r) {
    UA_CreateMonitoredItemsResponse *response = (UA_CreateMonitoredItemsResponse *)r;
    CustomCallback *cc = (CustomCallback *)d;
    MonitoredItems_CreateData *data = (MonitoredItems_CreateData *)cc->clientData;

    __MonitoredItems_create_handler(client, cc, requestId, response);
    MonitoredItems_CreateData_deleteItems(data->mis, data, client);
    if(cc->userCallback)
        cc->userCallback(client, cc->userData, requestId, response);
    UA_free(cc);
    if(data->mis)
        UA_free(data->mis);
    if(data->request)
        UA_CreateMonitoredItemsRequest_delete(data->request);
    UA_free(data);
}

static UA_StatusCode
MonitoredItems_CreateData_prepare(UA_Client_MonitoredItem **mis,
                                  MonitoredItems_CreateData *data, UA_Client *client) {
    /* Allocate the memory for internal representations */
    for(size_t i = 0; i < data->request->itemsToCreateSize; i++) {
        mis[i] = (UA_Client_MonitoredItem *)UA_malloc(sizeof(UA_Client_MonitoredItem));
        if(!mis[i])
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Set the clientHandle */
    for(size_t i = 0; i < data->request->itemsToCreateSize; i++)
        data->request->itemsToCreate[i].requestedParameters.clientHandle =
            ++(client->monitoredItemHandles);

    return UA_STATUSCODE_GOOD;
}

static void
__UA_Client_MonitoredItems_create(UA_Client *client,
                                  const UA_CreateMonitoredItemsRequest *request,
                                  void **contexts, void **handlingCallbacks,
                                  UA_Client_DeleteMonitoredItemCallback *deleteCallbacks,
                                  UA_CreateMonitoredItemsResponse *response) {
    UA_CreateMonitoredItemsResponse_init(response);

    if(!request->itemsToCreateSize) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    /* Get the subscription */
    UA_Client_Subscription *sub = findSubscription(client, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    size_t itemsToCreateSize = request->itemsToCreateSize;
    UA_STACKARRAY(UA_Client_MonitoredItem *, mis, itemsToCreateSize);
    memset(mis, 0, sizeof(void *) * itemsToCreateSize);

    MonitoredItems_CreateData data;
    memset(&data, 0, sizeof(MonitoredItems_CreateData));
    data.request = (UA_CreateMonitoredItemsRequest *)(uintptr_t)request;
    data.contexts = contexts;
    data.handlingCallbacks = handlingCallbacks;
    data.deleteCallbacks = deleteCallbacks;
    data.mis = mis;
    data.sub = sub;

    CustomCallback cc;
    memset(&cc, 0, sizeof(CustomCallback));
#ifdef __clang_analyzer__
    cc.isAsync = false;
#endif
    cc.clientData = &data;

    UA_StatusCode retval = MonitoredItems_CreateData_prepare(mis, &data, client);
    if(retval != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = retval;
        MonitoredItems_CreateData_deleteItems(mis, &data, client);
        return;
    }

    /* Call the service */
    __UA_Client_Service(client, request, &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST],
                        response, &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE]);

    __MonitoredItems_create_handler(client, &cc, 0, response);
    MonitoredItems_CreateData_deleteItems(mis, &data, client);

    for(size_t i = 0; i < itemsToCreateSize; i++)
        UA_assert(mis[i] == NULL);
}

static UA_StatusCode
__UA_Client_MonitoredItems_createDataChanges_async(UA_Client *client,
                                                   const UA_CreateMonitoredItemsRequest request,
                                                   void **contexts, void **callbacks,
                                                   UA_Client_DeleteMonitoredItemCallback *deleteCallbacks,
                                                   UA_ClientAsyncServiceCallback createCallback,
                                                   void *userdata, UA_UInt32 *requestId) {
    UA_Client_Subscription *sub = findSubscription(client, request.subscriptionId);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    CustomCallback *cc = (CustomCallback *)UA_calloc(1, sizeof(CustomCallback));
    if(!cc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    MonitoredItems_CreateData *data = (MonitoredItems_CreateData *)
        UA_calloc(1, sizeof(MonitoredItems_CreateData));
    if(!data) {
        UA_free(cc);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    data->sub = sub;
    cc->userCallback = createCallback;
    cc->userData = userdata;
    cc->isAsync = true;
    cc->clientData = data;

    /* Create a big array that holds the monitored items and parameters */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    void **array = (void **)UA_calloc(4 * request.itemsToCreateSize, sizeof(void *));
    if(!array) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    data->mis = (UA_Client_MonitoredItem **)array;
    data->contexts = (void **)
        ((uintptr_t)array + (sizeof(void *) * request.itemsToCreateSize));
    memcpy(data->contexts, contexts, request.itemsToCreateSize * sizeof(void *));
    data->deleteCallbacks = (UA_Client_DeleteMonitoredItemCallback *)
        ((uintptr_t)array + (2 * request.itemsToCreateSize * sizeof(void *)));
    memcpy(data->deleteCallbacks, deleteCallbacks,
           request.itemsToCreateSize * sizeof(UA_Client_DeleteMonitoredItemCallback));
    data->handlingCallbacks = (void **)
        ((uintptr_t)array + (3 * request.itemsToCreateSize * sizeof(void *)));
    memcpy(data->handlingCallbacks, callbacks, request.itemsToCreateSize * sizeof(void *));

    data->request = UA_CreateMonitoredItemsRequest_new();
    if(!data->request) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    retval = UA_CreateMonitoredItemsRequest_copy(&request, data->request);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = MonitoredItems_CreateData_prepare(data->mis, data, client);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    return __UA_Client_AsyncService(client, data->request,
                                    &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST],
                                    __MonitoredItems_create_async_handler,
                                    &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE],
                                    cc, requestId);
cleanup:
    MonitoredItems_CreateData_deleteItems(data->mis, data, client);
    if(data->mis)
        UA_free(data->mis);
    if(data->request)
        UA_CreateMonitoredItemsRequest_delete(data->request);
    UA_free(data);
    UA_free(cc);
    return retval;
}

UA_CreateMonitoredItemsResponse
UA_Client_MonitoredItems_createDataChanges(UA_Client *client,
                                           const UA_CreateMonitoredItemsRequest request,
                                           void **contexts,
                                           UA_Client_DataChangeNotificationCallback *callbacks,
                                           UA_Client_DeleteMonitoredItemCallback *deleteCallbacks) {
    UA_CreateMonitoredItemsResponse response;
    __UA_Client_MonitoredItems_create(client, &request, contexts,
                                      (void**)(uintptr_t)callbacks, deleteCallbacks, &response);
    return response;
}

UA_StatusCode
UA_Client_MonitoredItems_createDataChanges_async(UA_Client *client,
                                                 const UA_CreateMonitoredItemsRequest request,
                                                 void **contexts,
                                                 UA_Client_DataChangeNotificationCallback *callbacks,
                                                 UA_Client_DeleteMonitoredItemCallback *deleteCallbacks,
                                                 UA_ClientAsyncServiceCallback createCallback,
                                                 void *userdata, UA_UInt32 *requestId) {
    return __UA_Client_MonitoredItems_createDataChanges_async(client, request, contexts,
                                                              (void **)(uintptr_t)callbacks,
                                                              deleteCallbacks, createCallback,
                                                              userdata, requestId);
}

UA_MonitoredItemCreateResult
UA_Client_MonitoredItems_createDataChange(UA_Client *client, UA_UInt32 subscriptionId,
                                          UA_TimestampsToReturn timestampsToReturn,
                                          const UA_MonitoredItemCreateRequest item, void *context,
                                          UA_Client_DataChangeNotificationCallback callback,
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
    UA_CreateMonitoredItemsResponse_clear(&response);
    return result;
}

UA_CreateMonitoredItemsResponse
UA_Client_MonitoredItems_createEvents(UA_Client *client, const UA_CreateMonitoredItemsRequest request,
                                      void **contexts, UA_Client_EventNotificationCallback *callback,
                                      UA_Client_DeleteMonitoredItemCallback *deleteCallback) {
    UA_CreateMonitoredItemsResponse response;
    __UA_Client_MonitoredItems_create(client, &request, contexts,
                (void**)(uintptr_t)callback, deleteCallback, &response);
    return response;
}

/* Monitor the EventNotifier attribute only */
UA_StatusCode
UA_Client_MonitoredItems_createEvents_async(UA_Client *client,
                                            const UA_CreateMonitoredItemsRequest request,
                                            void **contexts,
                                            UA_Client_EventNotificationCallback *callbacks,
                                            UA_Client_DeleteMonitoredItemCallback *deleteCallbacks,
                                            UA_ClientAsyncServiceCallback createCallback,
                                            void *userdata, UA_UInt32 *requestId) {
    return __UA_Client_MonitoredItems_createDataChanges_async(client, request, contexts,
                                                              (void **)(uintptr_t)callbacks,
                                                              deleteCallbacks, createCallback,
                                                              userdata, requestId);
}

UA_MonitoredItemCreateResult
UA_Client_MonitoredItems_createEvent(UA_Client *client, UA_UInt32 subscriptionId,
                                     UA_TimestampsToReturn timestampsToReturn,
                                     const UA_MonitoredItemCreateRequest item, void *context,
                                     UA_Client_EventNotificationCallback callback,
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
    UA_StatusCode retval = response.responseHeader.serviceResult;
    UA_MonitoredItemCreateResult result;
    UA_MonitoredItemCreateResult_init(&result);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_CreateMonitoredItemsResponse_clear(&response);
        result.statusCode = retval;
        return result;
    }
    UA_MonitoredItemCreateResult_copy(response.results , &result);
    UA_CreateMonitoredItemsResponse_clear(&response);
    return result;
}

static void
__MonitoredItems_delete_handler(UA_Client *client, void *d, UA_UInt32 requestId, void *r) {
    UA_Client_Subscription *sub = NULL;
    UA_DeleteMonitoredItemsResponse *response = (UA_DeleteMonitoredItemsResponse *)r;
    CustomCallback *cc = (CustomCallback *)d;
    UA_DeleteMonitoredItemsRequest *request =
        (UA_DeleteMonitoredItemsRequest *)cc->clientData;
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        goto cleanup;

    sub = findSubscription(client, request->subscriptionId);
    if(!sub) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "No internal representation of subscription %" PRIu32,
                    request->subscriptionId);
        goto cleanup;
    }

    /* Loop over deleted MonitoredItems */
    for(size_t i = 0; i < response->resultsSize; i++) {
        if(response->results[i] != UA_STATUSCODE_GOOD &&
           response->results[i] != UA_STATUSCODE_BADMONITOREDITEMIDINVALID) {
            continue;
        }

#ifndef __clang_analyzer__
        /* Delete the internal representation */
        UA_Client_MonitoredItem *mon;
        LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
            // NOLINTNEXTLINE
            if(mon->monitoredItemId == request->monitoredItemIds[i]) {
                UA_Client_MonitoredItem_remove(client, sub, mon);
                break;
            }
        }
#endif
    }
cleanup:
    if(cc->isAsync) {
        if(cc->userCallback)
            cc->userCallback(client, cc->userData, requestId, response);
        UA_DeleteMonitoredItemsRequest_delete(request);
        UA_free(cc);
    }
}

UA_DeleteMonitoredItemsResponse
UA_Client_MonitoredItems_delete(UA_Client *client,
                                const UA_DeleteMonitoredItemsRequest request) {
    /* Send the request */
    UA_DeleteMonitoredItemsResponse response;
    CustomCallback cc;
    memset(&cc, 0, sizeof(CustomCallback));
#ifdef __clang_analyzer__
    cc.isAsync = false;
#endif
    cc.clientData = (void *)(uintptr_t)&request;

    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE]);

    __MonitoredItems_delete_handler(client, &cc, 0, &response);
    return response;
}

UA_StatusCode
UA_Client_MonitoredItems_delete_async(UA_Client *client,
                                      const UA_DeleteMonitoredItemsRequest request,
                                      UA_ClientAsyncServiceCallback callback,
                                      void *userdata, UA_UInt32 *requestId) {
    /* Send the request */
    CustomCallback *cc = (CustomCallback *)UA_calloc(1, sizeof(CustomCallback));
    if(!cc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_DeleteMonitoredItemsRequest *req_copy = UA_DeleteMonitoredItemsRequest_new();
    if(!req_copy) {
        UA_free(cc);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_DeleteMonitoredItemsRequest_copy(&request, req_copy);
    cc->isAsync = true;
    cc->clientData = req_copy;
    cc->userCallback = callback;
    cc->userData = userdata;

    return __UA_Client_AsyncService(
        client, &request, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST],
        __MonitoredItems_delete_handler, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE],
        cc, requestId);
}

UA_StatusCode
UA_Client_MonitoredItems_deleteSingle(UA_Client *client, UA_UInt32 subscriptionId,
                                      UA_UInt32 monitoredItemId) {
    UA_DeleteMonitoredItemsRequest request;
    UA_DeleteMonitoredItemsRequest_init(&request);
    request.subscriptionId = subscriptionId;
    request.monitoredItemIds = &monitoredItemId;
    request.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse response =
        UA_Client_MonitoredItems_delete(client, request);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DeleteMonitoredItemsResponse_clear(&response);
        return retval;
    }

    if(response.resultsSize != 1) {
        UA_DeleteMonitoredItemsResponse_clear(&response);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    retval = response.results[0];
    UA_DeleteMonitoredItemsResponse_clear(&response);
    return retval;
}

UA_ModifyMonitoredItemsResponse
UA_Client_MonitoredItems_modify(UA_Client *client,
                                const UA_ModifyMonitoredItemsRequest request) {
    UA_ModifyMonitoredItemsResponse response;

    UA_Client_Subscription *sub = 0;
    LIST_FOREACH(sub, &client->subscriptions, listEntry) {
        if (sub->subscriptionId == request.subscriptionId)
            break;
    }

    if (!sub) {
        UA_ModifyMonitoredItemsResponse_init(&response);
        response.responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return response;
    }

    UA_ModifyMonitoredItemsRequest modifiedRequest;
    UA_ModifyMonitoredItemsRequest_copy(&request, &modifiedRequest);

    for (size_t i = 0; i < modifiedRequest.itemsToModifySize; ++i) {
        UA_Client_MonitoredItem *mon = 0;
        LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
            if(mon->monitoredItemId == modifiedRequest.itemsToModify[i].monitoredItemId) {
                modifiedRequest.itemsToModify[i].requestedParameters.clientHandle = mon->clientHandle;
                break;
            }
        }
    }

    __UA_Client_Service(client,
                        &modifiedRequest, &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSRESPONSE]);

    UA_ModifyMonitoredItemsRequest_clear(&modifiedRequest);
    return response;
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
                         "Could not process a notification with clienthandle %" PRIu32
                         " on subscription %" PRIu32, min->clientHandle, sub->subscriptionId);
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
                         "Could not process a notification with clienthandle %" PRIu32
                         " on subscription %" PRIu32, eventFieldList->clientHandle,
                         sub->subscriptionId);
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

static void
UA_Client_Subscriptions_processPublishResponse(UA_Client *client, UA_PublishRequest *request,
                                               UA_PublishResponse *response) {
    UA_NotificationMessage *msg = &response->notificationMessage;

    client->currentlyOutStandingPublishRequests--;

    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADTOOMANYPUBLISHREQUESTS) {
        if(client->config.outStandingPublishRequests > 1) {
            client->config.outStandingPublishRequests--;
            UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                          "Too many publishrequest, reduce outStandingPublishRequests to %" PRId16,
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
        if(client->sessionState != UA_SESSIONSTATE_ACTIVATED) {
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

    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADTIMEOUT) {
        if (client->config.inactivityCallback)
            client->config.inactivityCallback(client);
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Received Timeout for Publish Response");
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
                     "Invalid subscription sequence number: expected %" PRIu32 " but got %" PRIu32,
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
                           "message on subscription %" PRIu32, sub->subscriptionId);
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
    if(client->sessionState < UA_SESSIONSTATE_ACTIVATED)
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
                         "Inactivity for Subscription %" PRIu32 ".", sub->subscriptionId);
        }
    }
}

void
UA_Client_Subscriptions_backgroundPublish(UA_Client *client) {
    if(client->sessionState < UA_SESSIONSTATE_ACTIVATED)
        return;

    /* The session must have at least one subscription */
    if(!LIST_FIRST(&client->subscriptions))
        return;

    while(client->currentlyOutStandingPublishRequests < client->config.outStandingPublishRequests) {
        UA_PublishRequest *request = UA_PublishRequest_new();
        if(!request)
            return;

        request->requestHeader.timeoutHint=60000;
        UA_StatusCode retval = UA_Client_preparePublishRequest(client, request);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_PublishRequest_delete(request);
            return;
        }
    
        UA_UInt32 requestId;
        client->currentlyOutStandingPublishRequests++;

        /* Disable the timeout, it is treat in
         * UA_Client_Subscriptions_backgroundPublishInactivityCheck */
        retval = __UA_Client_AsyncServiceEx(client, request, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                                            processPublishResponseAsync,
                                            &UA_TYPES[UA_TYPES_PUBLISHRESPONSE],
                                            (void*)request, &requestId, 0);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_PublishRequest_delete(request);
            return;
        }
    }
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
