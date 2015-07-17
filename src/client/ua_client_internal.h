#ifndef UA_CLIENT_INTERNAL_H_
#define UA_CLIENT_INTERNAL_H_

#include "queue.h"

#ifdef ENABLE_SUBSCRIPTIONS
typedef struct UA_Client_NotificationsAckNumber_s {
    UA_SubscriptionAcknowledgement subAck;
    LIST_ENTRY(UA_Client_NotificationsAckNumber_s) listEntry;
} UA_Client_NotificationsAckNumber;

typedef struct UA_Client_MonitoredItem_s {
    UA_UInt32          MonitoredItemId;
    UA_UInt32          MonitoringMode;
    UA_NodeId          monitoredNodeId;
    UA_UInt32          AttributeID;
    UA_UInt32          ClientHandle;
    UA_UInt32          SamplingInterval;
    UA_UInt32          QueueSize;
    UA_Boolean         DiscardOldest;
    void               (*handler)(UA_UInt32 handle, UA_DataValue *value);
    LIST_ENTRY(UA_Client_MonitoredItem_s)  listEntry;
} UA_Client_MonitoredItem;

typedef struct UA_Client_Subscription_s {
    UA_UInt32    LifeTime;
    UA_Int32     KeepAliveCount;
    UA_DateTime  PublishingInterval;
    UA_UInt32    SubscriptionID;
    UA_Int32     NotificationsPerPublish;
    UA_UInt32    Priority;
    LIST_ENTRY(UA_Client_Subscription_s) listEntry;
    LIST_HEAD(UA_ListOfClientMonitoredItems, UA_Client_MonitoredItem_s) MonitoredItems;
} UA_Client_Subscription;

UA_CreateSubscriptionResponse   UA_Client_createSubscription(UA_Client *client, UA_CreateSubscriptionRequest *request);
UA_ModifySubscriptionResponse   UA_Client_modifySubscription(UA_Client *client, UA_ModifySubscriptionRequest *request);
UA_DeleteSubscriptionsResponse  UA_Client_deleteSubscriptions(UA_Client *client, UA_DeleteSubscriptionsRequest *request);
UA_CreateMonitoredItemsResponse UA_Client_createMonitoredItems(UA_Client *client, UA_CreateMonitoredItemsRequest *request);
UA_DeleteMonitoredItemsResponse UA_Client_deleteMonitoredItems(UA_Client *client, UA_DeleteMonitoredItemsRequest *request);
UA_PublishResponse              UA_Client_publish(UA_Client *client, UA_PublishRequest *request);


UA_Boolean UA_Client_processPublishRx(UA_Client *client, UA_PublishResponse response);
#endif

#endif /* UA_CLIENT_INTERNAL_H_ */
