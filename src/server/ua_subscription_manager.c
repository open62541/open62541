#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_nodestore.h"
#include "ua_subscription_manager.h"

void SubscriptionManager_init(UA_Session *session) {
    UA_SubscriptionManager *manager = &(session->subscriptionManager);

    /* FIXME: These init values are empirical. Maybe they should be part
     *        of the server config? */
    manager->GlobalPublishingInterval = (UA_Int32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    manager->GlobalLifeTimeCount = (UA_UInt32_BoundedValue) { .maxValue = 15000, .minValue = 0, .currentValue=0 };
    manager->GlobalKeepAliveCount = (UA_UInt32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    manager->GlobalNotificationsPerPublish = (UA_Int32_BoundedValue)  { .maxValue = 1000, .minValue = 1, .currentValue=0 };
    manager->GlobalSamplingInterval = (UA_UInt32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    manager->GlobalQueueSize = (UA_UInt32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    LIST_INIT(&manager->ServerSubscriptions);
    manager->LastSessionID = (UA_UInt32) UA_DateTime_now();
}

void SubscriptionManager_deleteMembers(UA_Session *session) {
    // UA_SubscriptionManager *manager = &(session->subscriptionManager);
    // todo: delete all subscriptions and monitored items
}

void SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *newSubscription) {
    LIST_INSERT_HEAD(&manager->ServerSubscriptions, newSubscription, listEntry);
}

UA_Subscription *SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager,
                                                         UA_Int32 SubscriptionID) {
    UA_Subscription *sub;
    LIST_FOREACH(sub, &manager->ServerSubscriptions, listEntry) {
        if(sub->SubscriptionID == SubscriptionID)
            break;
    }
    return sub;
}

UA_Int32 SubscriptionManager_deleteMonitoredItem(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID,
                                                 UA_UInt32 MonitoredItemID) {
    UA_Subscription *sub = SubscriptionManager_getSubscriptionByID(manager, SubscriptionID);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
        if (mon->ItemId == MonitoredItemID) {
            // FIXME!! don't we need to remove the list entry?
            MonitoredItem_delete(mon);
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
}

UA_Int32 SubscriptionManager_deleteSubscription(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID) {
    UA_Subscription *sub = SubscriptionManager_getSubscriptionByID(manager, SubscriptionID);    
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    // Delete registered subscriptions
    UA_MonitoredItem *mon;
    while((mon = LIST_FIRST(&sub->MonitoredItems)))
        MonitoredItem_delete(mon);
    
    // Delete queued notification messages
    UA_unpublishedNotification *notify;
    while((notify = LIST_FIRST(&sub->unpublishedNotifications))) {
       LIST_REMOVE(notify, listEntry);
       UA_free(notify);
    }
    
    LIST_REMOVE(sub, listEntry);
    UA_free(sub);
    return UA_STATUSCODE_GOOD;
} 
