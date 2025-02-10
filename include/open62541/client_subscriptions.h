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

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_create_async(UA_Client *client,
    const UA_CreateSubscriptionRequest request,
    void *subscriptionContext,
    UA_Client_StatusChangeNotificationCallback statusChangeCallback,
    UA_Client_DeleteSubscriptionCallback deleteCallback,
    UA_ClientAsyncServiceCallback callback,
    void *userdata, UA_UInt32 *requestId);

UA_ModifySubscriptionResponse UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_modify(UA_Client *client,
    const UA_ModifySubscriptionRequest request);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_modify_async(UA_Client *client,
    const UA_ModifySubscriptionRequest request,
    UA_ClientAsyncServiceCallback callback,
    void *userdata, UA_UInt32 *requestId);

UA_DeleteSubscriptionsResponse UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_delete(UA_Client *client,
    const UA_DeleteSubscriptionsRequest request);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_delete_async(UA_Client *client,
    const UA_DeleteSubscriptionsRequest request,
    UA_ClientAsyncServiceCallback callback,
    void *userdata, UA_UInt32 *requestId);

/* Delete a single subscription */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_deleteSingle(UA_Client *client, UA_UInt32 subscriptionId);

/* Retrieve or change the user supplied subscription contexts */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_getContext(UA_Client *client, UA_UInt32 subscriptionId, void **subContext);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_Subscriptions_setContext(UA_Client *client, UA_UInt32 subscriptionId, void *subContext);

static UA_INLINE UA_THREADSAFE UA_SetPublishingModeResponse
UA_Client_Subscriptions_setPublishingMode(UA_Client *client,
    const UA_SetPublishingModeRequest request) {
    UA_SetPublishingModeResponse response;
    __UA_Client_Service(client,
        &request, &UA_TYPES[UA_TYPES_SETPUBLISHINGMODEREQUEST],
        &response, &UA_TYPES[UA_TYPES_SETPUBLISHINGMODERESPONSE]);
    return response;
}

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
 * to get the current parameters.
 *
 * The clientHandle parameter cannot be set by the user, any value will be
 * replaced by the client before sending the request to the server. */

/* Default values for a new monitored item. */
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
 * Within the client, every MonitoredItem has a user-defined context pointer
 * attached during the initialization. The context pointer can be read and
 * modified with the following methods. */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItem_getContext(UA_Client *client, UA_UInt32 subscriptionId,
                                   UA_UInt32 monitoredItemId, void **monContext);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItem_setContext(UA_Client *client, UA_UInt32 subscriptionId,
                                   UA_UInt32 monitoredItemId, void *monContext);

/**
 * **Deletion callback**
 *
 * The next callback is used for the application to receive a trigger when the
 * MonitoredItem is deleted. */
typedef void (*UA_Client_DeleteMonitoredItemCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext,
     UA_UInt32 monId, void *monContext);

UA_DeleteMonitoredItemsResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_delete(UA_Client *client,
    const UA_DeleteMonitoredItemsRequest);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_delete_async(UA_Client *client,
    const UA_DeleteMonitoredItemsRequest request,
    UA_ClientAsyncServiceCallback callback,
    void *userdata, UA_UInt32 *requestId);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_deleteSingle(UA_Client *client,
    UA_UInt32 subscriptionId, UA_UInt32 monitoredItemId);

/* The clientHandle parameter will be filled automatically */
UA_ModifyMonitoredItemsResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_modify(UA_Client *client,
    const UA_ModifyMonitoredItemsRequest request);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_modify_async(UA_Client *client,
    const UA_ModifyMonitoredItemsRequest request,
    UA_ClientAsyncServiceCallback callback,
    void *userdata, UA_UInt32 *requestId);

/**
 * **DataChange MonitoredItems**
 *
 * The next definitions are for MonitoredItems that monitor the value of
 * attributes in the information model. Any attribute can be monitored with the
 * exception of the ``EventNotifier``attribute. The latter is used to define
 * Event-MonitoredItems where Events are emited by Objects in the information
 * model. */
typedef void (*UA_Client_DataChangeNotificationCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext,
     UA_UInt32 monId, void *monContext,
     UA_DataValue *value);

UA_CreateMonitoredItemsResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createDataChanges(UA_Client *client,
    const UA_CreateMonitoredItemsRequest request, void **contexts,
    UA_Client_DataChangeNotificationCallback *callbacks,
    UA_Client_DeleteMonitoredItemCallback *deleteCallbacks);

UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createDataChanges_async(UA_Client *client,
    const UA_CreateMonitoredItemsRequest request, void **contexts,
    UA_Client_DataChangeNotificationCallback *callbacks,
    UA_Client_DeleteMonitoredItemCallback *deleteCallbacks,
    UA_ClientAsyncServiceCallback createCallback,
    void *userdata, UA_UInt32 *requestId);

UA_MonitoredItemCreateResult UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createDataChange(UA_Client *client,
    UA_UInt32 subscriptionId,
    UA_TimestampsToReturn timestampsToReturn,
    const UA_MonitoredItemCreateRequest item,
    void *context, UA_Client_DataChangeNotificationCallback callback,
    UA_Client_DeleteMonitoredItemCallback deleteCallback);

/**
 * **Event MonitoredItems**
 *
 * Events are emotted by Objects in the information model. The Events themselves
 * are specified by an *EventType* in the hierarchy of ObjectTypes. Hence Events
 * instances can be though of as temporary Objects in the information model. The
 * MonitoredItems is configured with an *EventFilter*. The EventFilter describes
 * the elements (fields) to be transmitted for every Event.
 *
 * **Callback for Event notifications (Option 2)**
 * This callback transmits the list of Event fields to the application. */

typedef void (*UA_Client_EventNotificationCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext,
     UA_UInt32 monId, void *monContext,
     size_t nEventFields, UA_Variant *eventFields);

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
    UA_ClientAsyncServiceCallback createCallback,
    void *userdata, UA_UInt32 *requestId);

UA_MonitoredItemCreateResult UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createEvent(UA_Client *client,
    UA_UInt32 subscriptionId,
    UA_TimestampsToReturn timestampsToReturn,
    const UA_MonitoredItemCreateRequest item,
    void *context, UA_Client_EventNotificationCallback callback,
    UA_Client_DeleteMonitoredItemCallback deleteCallback);

/**
 * **Callback for Event notifications (Option 2)**
 *
 * The fields for an Event MonitoredItem are specified as a
 * SimpleAttributeOperand. This callback option forwards a key-value map which
 * maps from the human-readable SimpleAttributeOperand to the variant value. The
 * ordering of the fields is identical to the order in which they are defined in
 * the MonitoredItemCreateRequest for the configuration.
 *
 * See the section on :ref:`parse-sao` for details on the human-redable format
 * for SimpleAttributeOperands. */
typedef void (*UA_Client_NamedEventNotificationCallback)
    (UA_Client *client, UA_UInt32 subId, void *subContext,
     UA_UInt32 monId, void *monContext,
     const UA_KeyValueMap fields);

/* Monitor the EventNotifier attribute only */
UA_CreateMonitoredItemsResponse UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createEventMonitoredItems(
    UA_Client *client, const UA_CreateMonitoredItemsRequest request,
    void **contexts, UA_Client_NamedEventNotificationCallback *callbacks,
    UA_Client_DeleteMonitoredItemCallback *deleteCallbacks);

/* Monitor the EventNotifier attribute only */
UA_StatusCode UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createEventMonitoredItems_async(
    UA_Client *client, const UA_CreateMonitoredItemsRequest request,
    void **contexts, UA_Client_NamedEventNotificationCallback *callbacks,
    UA_Client_DeleteMonitoredItemCallback *deleteCallbacks,
    UA_ClientAsyncServiceCallback createCallback,
    void *userdata, UA_UInt32 *requestId);

UA_MonitoredItemCreateResult UA_EXPORT UA_THREADSAFE
UA_Client_MonitoredItems_createEventMonitoredItem(
    UA_Client *client, UA_UInt32 subscriptionId,
    UA_TimestampsToReturn timestampsToReturn,
    const UA_MonitoredItemCreateRequest item,
    void *context, UA_Client_NamedEventNotificationCallback callback,
    UA_Client_DeleteMonitoredItemCallback deleteCallback);

/**
 * The following service calls go directly to the server. The changes are not
 * stored in the client. */

static UA_INLINE UA_THREADSAFE UA_SetMonitoringModeResponse
UA_Client_MonitoredItems_setMonitoringMode(UA_Client *client,
    const UA_SetMonitoringModeRequest request) {
    UA_SetMonitoringModeResponse response;
    __UA_Client_Service(client,
        &request, &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST],
        &response, &UA_TYPES[UA_TYPES_SETMONITORINGMODERESPONSE]);
    return response;
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Client_MonitoredItems_setMonitoringMode_async(UA_Client *client,
    const UA_SetMonitoringModeRequest request,
    UA_ClientAsyncServiceCallback callback,
    void *userdata, UA_UInt32 *requestId) {
    return __UA_Client_AsyncService(client, &request,
        &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST], callback,
        &UA_TYPES[UA_TYPES_SETMONITORINGMODERESPONSE],
        userdata, requestId);
}

static UA_INLINE UA_THREADSAFE UA_SetTriggeringResponse
UA_Client_MonitoredItems_setTriggering(UA_Client *client,
    const UA_SetTriggeringRequest request) {
    UA_SetTriggeringResponse response;
    __UA_Client_Service(client,
        &request, &UA_TYPES[UA_TYPES_SETTRIGGERINGREQUEST],
        &response, &UA_TYPES[UA_TYPES_SETTRIGGERINGRESPONSE]);
    return response;
}

static UA_INLINE UA_THREADSAFE UA_StatusCode
UA_Client_MonitoredItems_setTriggering_async(UA_Client *client,
    const UA_SetTriggeringRequest request,
    UA_ClientAsyncServiceCallback callback,
    void *userdata, UA_UInt32 *requestId) {
    return __UA_Client_AsyncService(client, &request,
        &UA_TYPES[UA_TYPES_SETTRIGGERINGREQUEST], callback,
        &UA_TYPES[UA_TYPES_SETTRIGGERINGRESPONSE],
        userdata, requestId);
}

_UA_END_DECLS

#endif /* UA_CLIENT_SUBSCRIPTIONS_H_ */
