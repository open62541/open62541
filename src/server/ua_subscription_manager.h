#ifndef UA_SUBSCRIPTION_MANAGER_H_
#define UA_SUBSCRIPTION_MANAGER_H_

#include "ua_server.h"
#include "ua_types.h"
#include "queue.h"
#include "ua_nodestore.h"
#include "ua_subscription.h"

typedef struct UA_SubscriptionManager {
    UA_UInt32 lastSessionID;
    LIST_HEAD(UA_ListOfUASubscriptions, UA_Subscription) serverSubscriptions;
} UA_SubscriptionManager;

void SubscriptionManager_init(UA_Session *session);
void SubscriptionManager_deleteMembers(UA_Session *session, UA_Server *server);
void SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *subscription);
UA_Subscription *
SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager, UA_UInt32 subscriptionID);
UA_StatusCode
SubscriptionManager_deleteSubscription(UA_Server *server, UA_SubscriptionManager *manager,
                                       UA_UInt32 subscriptionID);
UA_StatusCode
SubscriptionManager_deleteMonitoredItem(UA_SubscriptionManager *manager, UA_UInt32 subscriptionID,
                                        UA_UInt32 monitoredItemID);

UA_UInt32 SubscriptionManager_getUniqueUIntID(UA_SubscriptionManager *manager);
#endif /* UA_SUBSCRIPTION_MANAGER_H_ */
