#ifndef UA_SUBSCRIPTION_MANAGER_H_
#define UA_SUBSCRIPTION_MANAGER_H_

#include "ua_server.h"
#include "ua_types.h"
#include "queue.h"
#include "ua_nodestore.h"
#include "ua_subscription.h"

typedef struct UA_SubscriptionManager {
    UA_Int32_BoundedValue globalPublishingInterval;
    UA_UInt32_BoundedValue globalLifeTimeCount;
    UA_UInt32_BoundedValue globalKeepAliveCount;
    UA_Int32_BoundedValue globalNotificationsPerPublish;
    UA_UInt32_BoundedValue globalSamplingInterval;
    UA_UInt32_BoundedValue globalQueueSize;
    UA_Int32 lastSessionID;
    UA_Guid lastJobGuid;
    LIST_HEAD(UA_ListOfUASubscriptions, UA_Subscription) serverSubscriptions;
} UA_SubscriptionManager;

void SubscriptionManager_init(UA_Session *session);
void SubscriptionManager_deleteMembers(UA_Session *session, UA_Server *server);
void SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *subscription);
UA_Subscription *SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager,
                                                         UA_Int32 subscriptionID);
UA_Int32 SubscriptionManager_deleteSubscription(UA_Server *server, UA_SubscriptionManager *manager, UA_Int32 subscriptionID);
UA_Int32 SubscriptionManager_deleteMonitoredItem(UA_SubscriptionManager *manager, UA_Int32 subscriptionID,
                                                 UA_UInt32 monitoredItemID);

UA_UInt32 SubscriptionManager_getUniqueUIntID(UA_SubscriptionManager *manager);
UA_Guid SubscriptionManager_getUniqueGUID(UA_SubscriptionManager *manager);
#endif /* UA_SUBSCRIPTION_MANAGER_H_ */
