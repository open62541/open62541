/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UA_CLIENT_SUBSCRIPTIONS_H_
#define UA_CLIENT_SUBSCRIPTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_client.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS

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
 * ``UA_Client_runAsync``. */

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

UA_CreateSubscriptionResponse UA_EXPORT
UA_Client_Subscriptions_create(UA_Client *client,
                               const UA_CreateSubscriptionRequest request,
                               void *subscriptionContext,
                               UA_Client_StatusChangeNotificationCallback statusChangeCallback,
                               UA_Client_DeleteSubscriptionCallback deleteCallback);

UA_ModifySubscriptionResponse UA_EXPORT
UA_Client_Subscriptions_modify(UA_Client *client, const UA_ModifySubscriptionRequest request);

UA_DeleteSubscriptionsResponse UA_EXPORT
UA_Client_Subscriptions_delete(UA_Client *client,
                               const UA_DeleteSubscriptionsRequest request);

/* Delete a single subscription */
UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_deleteSingle(UA_Client *client, UA_UInt32 subscriptionId);

static UA_INLINE UA_SetPublishingModeResponse
UA_Client_Subscriptions_setPublishingMode(UA_Client *client,
                                          const UA_SetPublishingModeRequest request) {
    UA_SetPublishingModeResponse response;
    __UA_Client_Service(client, &request,
                        &UA_TYPES[UA_TYPES_SETPUBLISHINGMODEREQUEST], &response,
                        &UA_TYPES[UA_TYPES_SETPUBLISHINGMODERESPONSE]);
    return response;
}

/**
 * MonitoredItems
 * --------------
 *
 * MonitoredItems for Events indicate the ``EventNotifier`` attribute. This
 * indicates to the server not to monitor changes of the attribute, but to
 * forward Event notifications from that node.
 *
 * During the creation of a MonitoredItem, the server may return changed
 * adjusted parameters. Use ``UA_Client_MonitoredItem_getParameters`` to get the
 * current parameters. */

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
UA_CreateMonitoredItemsResponse UA_EXPORT
UA_Client_MonitoredItems_createDataChanges(UA_Client *client,
            const UA_CreateMonitoredItemsRequest request, void **contexts,
            UA_Client_DataChangeNotificationCallback *callbacks,
            UA_Client_DeleteMonitoredItemCallback *deleteCallbacks);

UA_MonitoredItemCreateResult UA_EXPORT
UA_Client_MonitoredItems_createDataChange(UA_Client *client, UA_UInt32 subscriptionId,
          UA_TimestampsToReturn timestampsToReturn, const UA_MonitoredItemCreateRequest item,
          void *context, UA_Client_DataChangeNotificationCallback callback,
          UA_Client_DeleteMonitoredItemCallback deleteCallback);

/* Monitor the EventNotifier attribute only */
UA_CreateMonitoredItemsResponse UA_EXPORT
UA_Client_MonitoredItems_createEvents(UA_Client *client,
            const UA_CreateMonitoredItemsRequest request, void **contexts,
            UA_Client_EventNotificationCallback *callbacks,
            UA_Client_DeleteMonitoredItemCallback *deleteCallback);

UA_MonitoredItemCreateResult UA_EXPORT
UA_Client_MonitoredItems_createEvent(UA_Client *client, UA_UInt32 subscriptionId,
          UA_TimestampsToReturn timestampsToReturn, const UA_MonitoredItemCreateRequest item,
          void *context, UA_Client_EventNotificationCallback callback,
          UA_Client_DeleteMonitoredItemCallback deleteCallback);

UA_DeleteMonitoredItemsResponse UA_EXPORT
UA_Client_MonitoredItems_delete(UA_Client *client, const UA_DeleteMonitoredItemsRequest);

UA_StatusCode UA_EXPORT
UA_Client_MonitoredItems_deleteSingle(UA_Client *client, UA_UInt32 subscriptionId, UA_UInt32 monitoredItemId);

/**
 * The following service calls go directly to the server. The MonitoredItem settings are
 * not stored in the client. */

static UA_INLINE UA_ModifyMonitoredItemsResponse
UA_Client_MonitoredItems_modify(UA_Client *client,
                                const UA_ModifyMonitoredItemsRequest request) {
    UA_ModifyMonitoredItemsResponse response;
    __UA_Client_Service(client,
                        &request, &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSRESPONSE]);
    return response;
}

static UA_INLINE UA_SetMonitoringModeResponse
UA_Client_MonitoredItems_setMonitoringMode(UA_Client *client,
                                           const UA_SetMonitoringModeRequest request) {
    UA_SetMonitoringModeResponse response;
    __UA_Client_Service(client,
                        &request, &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST],
                        &response, &UA_TYPES[UA_TYPES_SETMONITORINGMODERESPONSE]);
    return response;
}

static UA_INLINE UA_SetTriggeringResponse
UA_Client_MonitoredItems_setTriggering(UA_Client *client,
                                       const UA_SetTriggeringRequest request) {
    UA_SetTriggeringResponse response;
    __UA_Client_Service(client,
                        &request, &UA_TYPES[UA_TYPES_SETTRIGGERINGREQUEST],
                        &response, &UA_TYPES[UA_TYPES_SETTRIGGERINGRESPONSE]);
    return response;
}

/**
 * Deprecated API
 * --------------
 * The following API is kept for backwards compatibility. It will be removed in
 * future releases. */

typedef struct {
    UA_Double requestedPublishingInterval;
    UA_UInt32 requestedLifetimeCount;
    UA_UInt32 requestedMaxKeepAliveCount;
    UA_UInt32 maxNotificationsPerPublish;
    UA_Boolean publishingEnabled;
    UA_Byte priority;
} UA_SubscriptionSettings;

extern const UA_EXPORT UA_SubscriptionSettings UA_SubscriptionSettings_default;

UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_new(UA_Client *client, UA_SubscriptionSettings settings,
                            UA_UInt32 *newSubscriptionId);

UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_remove(UA_Client *client, UA_UInt32 subscriptionId);

/* Send a publish request and wait until a response to the request is processed.
 * Note that other publish responses may be processed in the background until
 * then. */
UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_manuallySendPublishRequest(UA_Client *client);

/* For monitoring DataChanges */
typedef void (*UA_MonitoredItemHandlingFunction)(UA_Client *client, UA_UInt32 monId,
                                                 UA_DataValue *value, void *context);

UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredItems(UA_Client *client, const UA_UInt32 subscriptionId,
                                          UA_MonitoredItemCreateRequest *items, size_t itemsSize,
                                          UA_MonitoredItemHandlingFunction *hfs,
                                          void **hfContexts, UA_StatusCode *itemResults,
                                          UA_UInt32 *newMonitoredItemIds);

UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                         UA_NodeId nodeId, UA_UInt32 attributeID,
                                         UA_MonitoredItemHandlingFunction hf,
                                         void *hfContext,
                                         UA_UInt32 *newMonitoredItemId,
                                         UA_Double samplingInterval);

/* Monitored Events have different payloads from DataChanges. So they use a
 * different callback method signature. */
typedef void (*UA_MonitoredEventHandlingFunction)(UA_Client *client,
                                                  const UA_UInt32 monId,
                                                  const size_t nEventFields,
                                                  const UA_Variant *eventFields,
                                                  void *context);

UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredEvents(UA_Client *client, const UA_UInt32 subscriptionId,
                                           UA_MonitoredItemCreateRequest *items, size_t itemsSize,
                                           UA_MonitoredEventHandlingFunction *hfs,
                                           void **hfContexts, UA_StatusCode *itemResults,
                                           UA_UInt32 *newMonitoredItemIds);

/* TODO for 0.4: attribute is fix for events. */
UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredEvent(UA_Client *client, UA_UInt32 subscriptionId,
                                          const UA_NodeId nodeId, UA_UInt32 attributeID,
                                          const UA_SimpleAttributeOperand *selectClauses,
                                          size_t selectClausesSize,
                                          const UA_ContentFilterElement *whereClauses,
                                          size_t whereClausesSize,
                                          const UA_MonitoredEventHandlingFunction hf,
                                          void *hfContext, UA_UInt32 *newMonitoredItemId);

UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_removeMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                            UA_UInt32 monitoredItemId);

UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_removeMonitoredItems(UA_Client *client, UA_UInt32 subscriptionId,
                                             UA_UInt32 *monitoredItemIds, size_t itemsSize,
                                             UA_StatusCode *itemResults);

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CLIENT_SUBSCRIPTIONS_H_ */
