#ifdef ENABLESUBSCRIPTIONS
#ifndef UA_SUBSCRIPTION_MANAGER_H_
#define UA_SUBSCRIPTION_MANAGER_H_

#include "ua_server.h"
#include "ua_types.h"
#include "queue.h"

typedef struct {
    UA_Int32 currentValue;
    UA_Int32 minValue;
    UA_Int32 maxValue;
} UA_Int32_BoundedValue;

typedef struct {
    UA_UInt32 currentValue;
    UA_UInt32 minValue;
    UA_UInt32 maxValue;
} UA_UInt32_BoundedValue;

typedef struct UA_MonitoredItem_s {
    UA_NodeId node;
    LIST_ENTRY(UA_MonitoredItem_s) listEntry;
} UA_MonitoredItem;


typedef struct UA_Subscription_s {
    UA_UInt32_BoundedValue LiveTime;
    UA_UInt32_BoundedValue KeepAliveCount;
    UA_Int32  PublishingInterval;
    UA_Int32  SubscriptionID;
    UA_Int32  NotificationsPerPublish;
    UA_Byte   PublishingMode;
    UA_UInt32 Priority;
    LIST_ENTRY(UA_Subscription_s) listEntry;
    LIST_HEAD(ListOfUAMonitoredItems, UA_MonitoredItem_s) *MonitoredItems;
} UA_Subscription;


typedef struct UA_SubscriptionManager_s {
    UA_Int32_BoundedValue  GlobalPublishingInterval;
    UA_UInt32_BoundedValue GlobalLifeTimeCount;
    UA_UInt32_BoundedValue GlobalKeepAliveCount;
    UA_Int32_BoundedValue  GlobalNotificationsPerPublish;
    UA_Int32 LastSessionID;
    LIST_HEAD(ListOfUASubscription, UA_Subscription_s) *ServerSubscriptions;
} UA_SubscriptionManager;

void SubscriptionManager_init(UA_Server *server);
UA_Subscription *UA_Subscription_new(UA_Int32 SubscriptionID);
void SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *subscription);
void SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID);
void SubscriptionManager_deleteSubscription(UA_SubscriptionManager *manager, UA_Subscription *subscription);

#endif  // ifndef... define UA_SUBSCRIPTION_MANAGER_H_
#endif  // ifdef EnableSubscriptions ...
