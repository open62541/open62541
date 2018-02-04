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

typedef struct {
    UA_Double publishingInterval;
    UA_UInt32 lifetimeCount;
    UA_UInt32 maxKeepAliveCount;
    UA_UInt32 maxNotificationsPerPublish;
    UA_Byte priority;
    UA_Boolean publishingEnabled;
} UA_SubscriptionParameters;

extern const UA_EXPORT UA_SubscriptionParameters UA_SubscriptionParameters_default;

/* Callbacks defined for Subscriptions */
typedef void (*UA_DeleteSubscriptionCallback)
    (UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext);

typedef void (*UA_StatusChangeNotificationCallback)
    (UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext,
     const UA_StatusChangeNotification *notification);

UA_StatusCode UA_EXPORT
UA_Client_Subscription_create(UA_Client *client,
                              const UA_SubscriptionParameters *requestedParameters,
                              void *subscriptionContext,
                              UA_StatusChangeNotificationCallback statusChangeCallback,
                              UA_DeleteSubscriptionCallback deleteCallback,
                              UA_UInt32 *newSubscriptionId);

UA_StatusCode UA_EXPORT
UA_Client_Subscription_getParameters(UA_Client *client, UA_UInt32 subscriptionId,
                                     UA_SubscriptionParameters *parameters);

/* Calls the ModifySubscription and/or the SetPublishingMode service, depending
 * on the parameter changes. */
UA_StatusCode UA_EXPORT
UA_Client_Subscription_setParameters(UA_Client *client, UA_UInt32 subscriptionId,
                                     const UA_SubscriptionParameters *parameters);

UA_StatusCode UA_EXPORT
UA_Client_Subscription_delete(UA_Client *client, UA_UInt32 subscriptionId);

/**
 * MonitoredItems
 * --------------
 */

typedef void (*UA_MonitoredItemHandlingFunction)(UA_UInt32 monId, UA_DataValue *value,
                                                 void *context);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredItems(UA_Client *client, const UA_UInt32 subscriptionId,
                                          UA_MonitoredItemCreateRequest *items, size_t itemsSize,
                                          UA_MonitoredItemHandlingFunction *hfs,
                                          void **hfContexts, UA_StatusCode *itemResults,
                                          UA_UInt32 *newMonitoredItemIds);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                         UA_NodeId nodeId, UA_UInt32 attributeId,
                                         UA_MonitoredItemHandlingFunction hf,
                                         void *hfContext,
                                         UA_UInt32 *newMonitoredItemId,
                                         UA_Double samplingInterval);

/* Monitored Events have different payloads from DataChanges. So they use a
 * different callback method signature. */
typedef void (*UA_MonitoredEventHandlingFunction)(const UA_UInt32 monId,
                                                  const size_t nEventFields,
                                                  const UA_Variant *eventFields,
                                                  void *context);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredEvents(UA_Client *client, const UA_UInt32 subscriptionId,
                                           UA_MonitoredItemCreateRequest *items, size_t itemsSize,
                                           UA_MonitoredEventHandlingFunction *hfs,
                                           void **hfContexts, UA_StatusCode *itemResults,
                                           UA_UInt32 *newMonitoredItemIds);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_addMonitoredEvent(UA_Client *client, UA_UInt32 subscriptionId,
                                          const UA_NodeId nodeId, UA_UInt32 attributeId,
                                          const UA_SimpleAttributeOperand *selectClauses,
                                          size_t selectClausesSize,
                                          const UA_ContentFilterElement *whereClauses,
                                          size_t whereClausesSize,
                                          const UA_MonitoredEventHandlingFunction hf,
                                          void *hfContext, UA_UInt32 *newMonitoredItemId);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_removeMonitoredItem(UA_Client *client, UA_UInt32 subscriptionId,
                                            UA_UInt32 monitoredItemId);

UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_removeMonitoredItems(UA_Client *client, UA_UInt32 subscriptionId,
                                             UA_UInt32 *monitoredItemId, size_t itemsSize,
                                             UA_StatusCode *itemResults);


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

UA_DEPRECATED static UA_INLINE UA_StatusCode
UA_Client_Subscriptions_new(UA_Client *client, UA_SubscriptionSettings settings,
                            UA_UInt32 *newSubscriptionId) {
    UA_SubscriptionParameters parameters;
    parameters.publishingInterval = settings.requestedPublishingInterval;
    parameters.lifetimeCount = settings.requestedLifetimeCount;
    parameters.maxKeepAliveCount = settings.requestedLifetimeCount;
    parameters.maxNotificationsPerPublish = settings.maxNotificationsPerPublish;
    parameters.publishingEnabled = settings.publishingEnabled;
    parameters.priority = settings.priority;
    return UA_Client_Subscription_create(client, &parameters,
                                         NULL, NULL, NULL, newSubscriptionId);
}

UA_DEPRECATED static UA_INLINE UA_StatusCode
UA_Client_Subscriptions_remove(UA_Client *client, UA_UInt32 subscriptionId) {
    return UA_Client_Subscription_delete(client, subscriptionId);
}

/* Send a publish request and wait until a response to the request is processed.
 * Note that other publish responses may be processed in the background until
 * then. */
UA_DEPRECATED UA_StatusCode UA_EXPORT
UA_Client_Subscriptions_manuallySendPublishRequest(UA_Client *client);

#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_CLIENT_SUBSCRIPTIONS_H_ */
