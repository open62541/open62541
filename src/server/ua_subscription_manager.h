#ifndef UA_SUBSCRIPTION_MANAGER_H_
#define UA_SUBSCRIPTION_MANAGER_H_

#include "ua_server.h"
#include "ua_types.h"
#include "queue.h"
#include "ua_nodestore.h"
#include "ua_subscription.h"

typedef struct UA_SubscriptionManager_s {
    UA_Int32_BoundedValue    GlobalPublishingInterval;
    UA_UInt32_BoundedValue   GlobalLifeTimeCount;
    UA_UInt32_BoundedValue   GlobalKeepAliveCount;
    UA_Int32_BoundedValue    GlobalNotificationsPerPublish;
    UA_UInt32_BoundedValue   GlobalSamplingInterval;
    UA_UInt32_BoundedValue   GlobalQueueSize;
    UA_Int32                 LastSessionID;
    LIST_HEAD(UA_ListOfUASubscriptions, UA_Subscription_s) ServerSubscriptions;
} UA_SubscriptionManager;

void SubscriptionManager_init(UA_Session *session);
void SubscriptionManager_deleteMembers(UA_Session *session);
void SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *subscription);
UA_Subscription *SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager,
                                                         UA_Int32 SubscriptionID);
UA_Int32 SubscriptionManager_deleteSubscription(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID);
UA_Int32 SubscriptionManager_deleteMonitoredItem(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID,
                                                 UA_UInt32 MonitoredItemID);

#endif  // ifndef... define UA_SUBSCRIPTION_MANAGER_H_
