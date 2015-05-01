#ifdef ENABLESUBSCRIPTIONS
#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_subscription_manager.h"

void SubscriptionManager_init(UA_Server *server) {
    UA_SubscriptionManager *manager = &(server->subscriptionManager);

    manager->LastSessionID = (UA_UInt32) (server->random_seed + (UA_UInt32)UA_DateTime_now());
    /* FIXME: These init values are empirical. Maybe they should be part
     *        of the server config? */
    manager->GlobalPublishingInterval = (UA_Int32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    manager->GlobalLifeTimeCount      = (UA_UInt32_BoundedValue) { .maxValue = 15000, .minValue = 0, .currentValue=0 };
    manager->GlobalKeepAliveCount     = (UA_UInt32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    manager->GlobalNotificationsPerPublish = (UA_Int32_BoundedValue) { .maxValue = 1000, .minValue = 1, .currentValue=0 };
    
    LIST_INIT(manager->ServerSubscriptions);
    return;
}

UA_Subscription *UA_Subscription_new(UA_Int32 SubscriptionID) {
    UA_Subscription *new = (UA_Subscription *) malloc(sizeof(UA_Subscription));
    
    LIST_INIT(new->MonitoredItems);
    return new;
}

void SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *newSubscription) {
    LIST_INSERT_HEAD(manager->ServerSubscriptions, newSubscription, listEntry);
    
    return;
}

void SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID) {
    return;
}

void SubscriptionManager_deleteSubscription(UA_SubscriptionManager *manager, UA_Subscription *subscription) {
    return;
}

#endif //#ifdef ENABLESUBSCRIPTIONS
