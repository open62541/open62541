/* This Source Code Form i subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#ifndef UA_CLIENT_SUBSCRIPTIONS_H_
#define UA_CLIENT_SUBSCRIPTIONS_H_

#include <open62541/client.h>
#include <open62541/client_highlevel_async.h>

_UA_BEGIN_DECLS

/**
 * .. _client-subscriptions:
 *
 * Subscriptions
 * -------------
 *
 * Subscriptions in OPC UA are asynchronous. That is, the client sends several
 * PublishRequests to the server. The server returns PublishResponses with
 * notifications. But only when a notification has been generated. The client
 * does not wait for the responses and continues normal operations.
 *
 * Note the difference between Subscriptions and MonitoredItems. Subscriptions
 * are used to report back notifications. MonitoredItems are used to generate
 * notifications. Every MonitoredItem is attached to exactly one Subscription.
 * And a Subscription can contain many MonitoredItems.
 *
 * The client automatically processes PublishResponses (with a callback) in the
 * background and keeps enough PublishRequests in transit. The PublishResponses
 * may be recieved during a synchronous service call or in
 * ``UA_Client_run_iterate``. See more about
 * :ref:`asynchronicity<client-async-services>`.
 */

/* Callbacks defined for Subscriptions */
typedef void (*UA_Client_DeleteSubscriptionCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext);

typedef void (*UA_Client_StatusChangeNotificationCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext,
     UA_StatusChangeNotification *notification);

/* The data value and the variant inside is owned by the library and should not be changed or freed! */
typedef struct {
    UA_UInt32 monitoredItemId;
    void *context;
    UA_DataValue *value;
} UA_DataItemsNotification;

typedef void (*UA_Client_DataItemsNotificationCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext, size_t numItems,
     UA_DataItemsNotification *monitoredItems);

/* The variant inside is owned by the library and should not be changed or freed! */
typedef struct {
    UA_UInt32 monitoredItemId;
    void *context;
    size_t eventFieldsSize;
    UA_Variant *eventFields;
} UA_EventsNotification;

typedef void(*UA_Client_EventsNotificationCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext, size_t numItems,
     UA_EventsNotification *eventItems);

/* Provides default values for a new subscription.
 *
 * RequestedPublishingInterval:  500.0 [ms]
 * RequestedLifetimeCount: 10000
 * RequestedMaxKeepAliveCount: 10
 * MaxNotificationsPerPublish: 0 (unlimited)
 * PublishingEnabled: true
 * Priority: 0 */
static UA_INLINE UA_CreateSubscriptionRequest
UA_CreateSubscriptionRequest_default(void) {
    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);

    request.requestedPublishingInterval = 500.0;
    request.requestedLifetimeCount = 10000;
    request.requestedMaxKeepAliveCount = 10;
    request.maxNotificationsPerPublish = 0;
    request.publishingEnabled = true;
    request.priority = 0;
    return request;
}

UA_CreateSubscriptionResponse UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_create(UA_Client *client,
    const UA_CreateSubscriptionRequest request,
    void *subscriptionContext,
    UA_Client_StatusChangeNotificationCallback statusChangeCallback,
    UA_Client_DeleteSubscriptionCallback deleteCallback);

UA_CreateSubscriptionResponse UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_createEx(UA_Client *client,
    const UA_CreateSubscriptionRequest request,
    void *subscriptionContext,
    UA_Client_StatusChangeNotificationCallback statusChangeCallback,
    UA_Client_DeleteSubscriptionCallback deleteCallback,
    UA_Client_DataItemsNotificationCallback dataChangeCallback,
    UA_Client_EventsNotificationCallback eventNotificationCallback);

typedef void
(*UA_ClientAsyncCreateSubscriptionCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_CreateSubscriptionResponse *response);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_create_async(UA_Client *client,
    const UA_CreateSubscriptionRequest request,
    void *subscriptionContext,
    UA_Client_StatusChangeNotificationCallback statusChangeCallback,
    UA_Client_DeleteSubscriptionCallback deleteCallback,
    UA_ClientAsyncCreateSubscriptionCallback createCallback,
    void *userdata, UA_UInt32 *requestId);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_createEx_async(UA_Client *client,
    const UA_CreateSubscriptionRequest request,
    void *subscriptionContext,
    UA_Client_StatusChangeNotificationCallback statusChangeCallback,
    UA_Client_DeleteSubscriptionCallback deleteCallback,
    UA_Client_DataItemsNotificationCallback dataChangeCallback,
    UA_Client_EventsNotificationCallback eventNotificationCallback,
    UA_ClientAsyncCreateSubscriptionCallback createCallback,
    void *userdata, UA_UInt32 *requestId);

UA_ModifySubscriptionResponse UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_modify(UA_Client *client,
    const UA_ModifySubscriptionRequest request);

typedef void
(*UA_ClientAsyncModifySubscriptionCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_ModifySubscriptionResponse *response);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_modify_async(UA_Client *client,
    const UA_ModifySubscriptionRequest request,
    UA_ClientAsyncModifySubscriptionCallback callback,
    void *userdata, UA_UInt32 *requestId);

UA_DeleteSubscriptionsResponse UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_delete(UA_Client *client,
    const UA_DeleteSubscriptionsRequest request);

typedef void
(*UA_ClientAsyncDeleteSubscriptionsCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_DeleteSubscriptionsResponse *response);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_delete_async(UA_Client *client,
    const UA_DeleteSubscriptionsRequest request,
    UA_ClientAsyncDeleteSubscriptionsCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Delete a single subscription */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_deleteSingle(UA_Client *client,
                                     UA_UInt32 subscriptionId);

/* Retrieve or change the user supplied subscription contexts */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_getContext(UA_Client *client,
                                   UA_UInt32 subscriptionId,
                                   void **subContext);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_setContext(UA_Client *client,
                                   UA_UInt32 subscriptionId,
                                   void *subContext);

UA_SetPublishingModeResponse UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_setPublishingMode(UA_Client *client,
    const UA_SetPublishingModeRequest request);

/**
 * MonitoredItems
 * ~~~~~~~~~~~~~~
 *
 * MonitoredItems for Events indicate the ``EventNotifier`` attribute. This
 * indicates to the server not to monitor changes of the attribute, but to
 * forward Event notifications from that node.
 *
 * During the creation of a MonitoredItem, the server may return changed
 * adjusted parameters. Check the returned ``UA_CreateMonitoredItemsResponse``
 * to get the current parameters. */

/* Provides default values for a new monitored item. */
static UA_INLINE UA_MonitoredItemCreateRequest
UA_MonitoredItemCreateRequest_default(UA_NodeId nodeId) {
    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = nodeId;
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE;
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;
    request.requestedParameters.samplingInterval = 250;
    request.requestedParameters.discardOldest = true;
    request.requestedParameters.queueSize = 1;
    return request;
}

/**
 * The clientHandle parameter cannot be set by the user, any value will be
 * replaced by the client before sending the request to the server. */

/* Callback for the deletion of a MonitoredItem */
typedef void (*UA_Client_DeleteMonitoredItemCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext,
     UA_UInt32 monId, void *monContext);

/* Callback for DataChange notifications */
typedef void (*UA_Client_DataChangeNotificationCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext,
     UA_UInt32 monId, void *monContext,
     UA_DataValue *value);

/* Callback for Event notifications */
typedef void (*UA_Client_EventNotificationCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext,
     UA_UInt32 monId, void *monContext,
     size_t nEventFields, UA_Variant *eventFields);

/* Don't use to monitor the EventNotifier attribute */
UA_CreateMonitoredItemsResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createDataChanges(UA_Client *client,
    const UA_CreateMonitoredItemsRequest request, void **contexts,
    UA_Client_DataChangeNotificationCallback *callbacks,
    UA_Client_DeleteMonitoredItemCallback *deleteCallbacks);

typedef void
(*UA_ClientAsyncCreateMonitoredItemsCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_CreateMonitoredItemsResponse *response);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createDataChanges_async(UA_Client *client,
    const UA_CreateMonitoredItemsRequest request, void **contexts,
    UA_Client_DataChangeNotificationCallback *callbacks,
    UA_Client_DeleteMonitoredItemCallback *deleteCallbacks,
    UA_ClientAsyncCreateMonitoredItemsCallback createCallback,
    void *userdata, UA_UInt32 *requestId);

UA_MonitoredItemCreateResult UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createDataChange(UA_Client *client,
    UA_UInt32 subscriptionId,
    UA_TimestampsToReturn timestampsToReturn,
    const UA_MonitoredItemCreateRequest item,
    void *context, UA_Client_DataChangeNotificationCallback callback,
    UA_Client_DeleteMonitoredItemCallback deleteCallback);

/* Monitor the EventNotifier attribute only */
UA_CreateMonitoredItemsResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createEvents(UA_Client *client,
    const UA_CreateMonitoredItemsRequest request, void **contexts,
    UA_Client_EventNotificationCallback *callback,
    UA_Client_DeleteMonitoredItemCallback *deleteCallback);

/* Monitor the EventNotifier attribute only */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createEvents_async(UA_Client *client,
    const UA_CreateMonitoredItemsRequest request, void **contexts,
    UA_Client_EventNotificationCallback *callbacks,
    UA_Client_DeleteMonitoredItemCallback *deleteCallbacks,
    UA_ClientAsyncCreateMonitoredItemsCallback createCallback,
    void *userdata, UA_UInt32 *requestId);

UA_MonitoredItemCreateResult UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createEvent(UA_Client *client,
    UA_UInt32 subscriptionId,
    UA_TimestampsToReturn timestampsToReturn,
    const UA_MonitoredItemCreateRequest item,
    void *context, UA_Client_EventNotificationCallback callback,
    UA_Client_DeleteMonitoredItemCallback deleteCallback);

UA_DeleteMonitoredItemsResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_delete(UA_Client *client,
    const UA_DeleteMonitoredItemsRequest);

typedef void
(*UA_ClientAsyncDeleteMonitoredItemsCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_DeleteMonitoredItemsResponse *response);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_delete_async(UA_Client *client,
    const UA_DeleteMonitoredItemsRequest request,
    UA_ClientAsyncDeleteMonitoredItemsCallback callback,
    void *userdata, UA_UInt32 *requestId);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_deleteSingle(UA_Client *client,
    UA_UInt32 subscriptionId, UA_UInt32 monitoredItemId);

/**
 * The "ClientHandle" is part of the MonitoredItem configuration. The handle is
 * set internally and not exposed to the user. */

UA_ModifyMonitoredItemsResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_modify(UA_Client *client,
    const UA_ModifyMonitoredItemsRequest request);

typedef void
(*UA_ClientAsyncModifyMonitoredItemsCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_DeleteMonitoredItemsResponse *response);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_modify_async(UA_Client *client,
    const UA_ModifyMonitoredItemsRequest request,
    UA_ClientAsyncModifyMonitoredItemsCallback callback,
    void *userdata, UA_UInt32 *requestId);

UA_SetMonitoringModeResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_setMonitoringMode(
    UA_Client *client, const UA_SetMonitoringModeRequest request);

typedef void
(*UA_ClientAsyncSetMonitoringModeCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_SetMonitoringModeResponse *response);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_setMonitoringMode_async(
    UA_Client *client, const UA_SetMonitoringModeRequest request,
    UA_ClientAsyncSetMonitoringModeCallback callback,
    void *userdata, UA_UInt32 *requestId);

UA_SetTriggeringResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_setTriggering(
    UA_Client *client, const UA_SetTriggeringRequest request);

typedef void
(*UA_ClientAsyncSetTriggeringCallback)(
    UA_Client *client, void *userdata, UA_UInt32 requestId,
    UA_SetTriggeringResponse *response);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_setTriggering_async(
    UA_Client *client, const UA_SetTriggeringRequest request,
    UA_ClientAsyncSetTriggeringCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Retrieve or change the user supplied MonitoredItem context */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItem_getContext(UA_Client *client,
    UA_UInt32 subscriptionId, UA_UInt32 monitoredItemId,
    void **monContext);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItem_setContext(UA_Client *client,
    UA_UInt32 subscriptionId, UA_UInt32 monitoredItemId,
    void *monContext);

_UA_END_DECLS

#endif /* UA_CLIENT_SUBSCRIPTIONS_H_ */
