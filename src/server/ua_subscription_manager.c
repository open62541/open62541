#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_nodestore.h"
#include "ua_subscription_manager.h"

void SubscriptionManager_init(UA_Session *session) {
    UA_SubscriptionManager *manager = &(session->subscriptionManager);

    /* FIXME: These init values are empirical. Maybe they should be part
     *        of the server config? */
    manager->GlobalPublishingInterval = (UA_Int32_BoundedValue) { .maxValue = 100,   .minValue = 0, .currentValue=0 };
    manager->GlobalLifeTimeCount           = (UA_UInt32_BoundedValue) { .maxValue = 15000, .minValue = 0, .currentValue=0 };
    manager->GlobalKeepAliveCount          = (UA_UInt32_BoundedValue) { .maxValue = 100,   .minValue = 0, .currentValue=0 };
    manager->GlobalNotificationsPerPublish = (UA_Int32_BoundedValue)  { .maxValue = 1000,  .minValue = 1, .currentValue=0 };
    manager->GlobalSamplingInterval        = (UA_UInt32_BoundedValue) { .maxValue = 100,   .minValue = 0, .currentValue=0 };
    manager->GlobalQueueSize               = (UA_UInt32_BoundedValue) { .maxValue = 100,   .minValue = 0, .currentValue=0 };
    
    manager->ServerSubscriptions = (UA_ListOfUASubscriptions *) UA_malloc (sizeof(UA_ListOfUASubscriptions));
    LIST_INIT(manager->ServerSubscriptions);
    manager->LastSessionID = (UA_UInt32) UA_DateTime_now();
}

void SubscriptionManager_deleteMembers(UA_Session *session) {
    UA_SubscriptionManager *manager = &(session->subscriptionManager);
    UA_free(manager->ServerSubscriptions);
}


void SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *newSubscription) {
    LIST_INSERT_HEAD(manager->ServerSubscriptions, newSubscription, listEntry);
}

UA_Subscription *SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager,
                                                         UA_Int32 SubscriptionID) {
    UA_Subscription *retsub, *sub;
    retsub = UA_NULL;
    for (sub = (manager->ServerSubscriptions)->lh_first; sub != NULL; sub = sub->listEntry.le_next) {
        if (sub->SubscriptionID == SubscriptionID) {
            retsub = sub;
            break;
        }
    }
    return retsub;
}

UA_Int32 SubscriptionManager_deleteMonitoredItem(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID,
                                                 UA_UInt32 MonitoredItemID) {
    UA_Subscription *sub;
    UA_MonitoredItem *mon;
    
    if (manager == NULL) return UA_STATUSCODE_BADINTERNALERROR;
    
    sub = SubscriptionManager_getSubscriptionByID(manager, SubscriptionID);
    if (sub == NULL) return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    for(mon=sub->MonitoredItems.lh_first; mon != NULL; mon=mon->listEntry.le_next) {
        if (mon->ItemId == MonitoredItemID) {
            MonitoredItem_delete(mon);
            return UA_STATUSCODE_GOOD;
        }
    }
    
    return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
}

UA_Int32 SubscriptionManager_deleteSubscription(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID) {
    UA_Subscription  *sub;
    UA_MonitoredItem *mon;
    UA_unpublishedNotification *notify;
    
    sub = SubscriptionManager_getSubscriptionByID(manager, SubscriptionID);    
    if (sub != NULL) {
        // Delete registered subscriptions
        while (sub->MonitoredItems.lh_first != NULL)  {
           mon = sub->MonitoredItems.lh_first;
           // Delete Sampled data
           MonitoredItem_delete(mon);
        }
    }
    else {
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    }
    
    // Delete queued notification messages
    notify = LIST_FIRST(&sub->unpublishedNotifications);
    while(sub->unpublishedNotifications.lh_first != NULL)  {
       notify = sub->unpublishedNotifications.lh_first;
       LIST_REMOVE(notify, listEntry);
       UA_free(notify);
    }
    
    LIST_REMOVE(sub, listEntry);
    UA_free(sub);
    return UA_STATUSCODE_GOOD;
} 
