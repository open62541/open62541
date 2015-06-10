#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_nodestore.h"
#include "ua_subscription_manager.h"

void SubscriptionManager_init(UA_Session *session) {
    UA_SubscriptionManager *manager = &(session->subscriptionManager);

    /* FIXME: These init values are empirical. Maybe they should be part
     *        of the server config? */
    manager->GlobalPublishingInterval = (UA_Int32_BoundedValue) { .maxValue = 10000, .minValue = 0, .currentValue=0 };
    manager->GlobalLifeTimeCount = (UA_UInt32_BoundedValue) { .maxValue = 15000, .minValue = 0, .currentValue=0 };
    manager->GlobalKeepAliveCount = (UA_UInt32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    manager->GlobalNotificationsPerPublish = (UA_Int32_BoundedValue)  { .maxValue = 1000, .minValue = 1, .currentValue=0 };
    manager->GlobalSamplingInterval = (UA_UInt32_BoundedValue) { .maxValue = 1000, .minValue = 5, .currentValue=0 };
    manager->GlobalQueueSize = (UA_UInt32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    LIST_INIT(&manager->ServerSubscriptions);
    manager->LastSessionID = (UA_UInt32) UA_DateTime_now();
    
    // Initialize a GUID with a 2^64 time dependant part, then fold the time in on itself to provide a more randomish
    // Counter
    unsigned long guidInitH = (UA_UInt64) UA_DateTime_now();
    manager->LastJobGuid.data1 = (UA_UInt16) (guidInitH >> 32);
    manager->LastJobGuid.data2 = (UA_UInt16) (guidInitH >> 16);
    manager->LastJobGuid.data3 = (UA_UInt16) (guidInitH);
    unsigned long guidInitL = (UA_UInt64) UA_DateTime_now();
    manager->LastJobGuid.data4[0] = (UA_Byte) guidInitL;
    manager->LastJobGuid.data4[1] = (UA_Byte) (guidInitL >> 8); 
    manager->LastJobGuid.data4[2] = (UA_Byte) (guidInitL >> 16);
    manager->LastJobGuid.data4[3] = (UA_Byte) (guidInitL >> 24);
    manager->LastJobGuid.data4[4] = (UA_Byte) (manager->LastJobGuid.data4[0]) ^ (guidInitL >> 32);
    manager->LastJobGuid.data4[5] = (UA_Byte) (manager->LastJobGuid.data4[0]) ^ (guidInitL >> 40);
    manager->LastJobGuid.data4[6] = (UA_Byte) (manager->LastJobGuid.data4[1]) ^ (guidInitL >> 48);
    manager->LastJobGuid.data4[7] = (UA_Byte) (manager->LastJobGuid.data4[0]) ^ (guidInitL >> 58);
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

UA_Int32 SubscriptionManager_deleteSubscription(UA_Server *server, UA_SubscriptionManager *manager, UA_Int32 SubscriptionID) {
    UA_Subscription *sub = SubscriptionManager_getSubscriptionByID(manager, SubscriptionID);    
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    UA_Subscription_deleteMembers(server, sub);
    
    LIST_REMOVE(sub, listEntry);
    UA_free(sub);
    return UA_STATUSCODE_GOOD;
} 

UA_UInt32 SubscriptionManager_getUniqueUIntID(UA_SubscriptionManager *manager) {
    UA_UInt32 id = ++(manager->LastSessionID);
    return id;
}

UA_Guid SubscriptionManager_getUniqueGUID(UA_SubscriptionManager *manager) {
    UA_Guid id;
    unsigned long *incremental;
    
    incremental = (unsigned long *) &manager->LastJobGuid.data4[0];
    incremental++;
    
    UA_Guid_copy(&(manager->LastJobGuid), &id);
    
    return id;
}