#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_nodestore.h"
#include "ua_subscription_manager.h"

void SubscriptionManager_init(UA_Session *session) {
    UA_SubscriptionManager *manager = &(session->subscriptionManager);

    /* FIXME: These init values are empirical. Maybe they should be part
     *        of the server config? */
    manager->globalPublishingInterval = (UA_Int32_BoundedValue) { .maxValue = 10000, .minValue = 0, .currentValue=0 };
    manager->globalLifeTimeCount = (UA_UInt32_BoundedValue) { .maxValue = 15000, .minValue = 0, .currentValue=0 };
    manager->globalKeepAliveCount = (UA_UInt32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    manager->globalNotificationsPerPublish = (UA_Int32_BoundedValue)  { .maxValue = 1000, .minValue = 1, .currentValue=0 };
    manager->globalSamplingInterval = (UA_UInt32_BoundedValue) { .maxValue = 1000, .minValue = 5, .currentValue=0 };
    manager->globalQueueSize = (UA_UInt32_BoundedValue) { .maxValue = 100, .minValue = 0, .currentValue=0 };
    LIST_INIT(&manager->serverSubscriptions);
    manager->lastSessionID = (UA_UInt32) UA_DateTime_now();
    
    // Initialize a GUID with a 2^64 time dependant part, then fold the time in on itself to provide a more randomish
    // Counter
    // NOTE: On a 32 bit plattform, assigning 64 bit (2 regs) is allowed by the compiler, but shifting though multiple
    //       regs is usually not. To support both 32 and 64bit, the struct splits the 64Bit timestamp into two parts.
    union {
        struct {
            UA_UInt32 ui32h;
            UA_UInt32 ui32l;
        };
        UA_UInt64 ui64;
    } guidInitH;
    guidInitH.ui64 = (UA_UInt64) UA_DateTime_now();
    manager->lastJobGuid.data1 = guidInitH.ui32h;
    manager->lastJobGuid.data2 = (UA_UInt16) (guidInitH.ui32l >> 16);
    manager->lastJobGuid.data3 = (UA_UInt16) (guidInitH.ui32l);
    union {
        struct {
            UA_UInt32 ui32h;
            UA_UInt32 ui32l;
        };
        UA_UInt64 ui64;
    } guidInitL;
    guidInitL.ui64 = (UA_UInt64) UA_DateTime_now();
    manager->lastJobGuid.data4[0] = (UA_Byte) guidInitL.ui32l;
    manager->lastJobGuid.data4[1] = (UA_Byte) (guidInitL.ui32l >> 8); 
    manager->lastJobGuid.data4[2] = (UA_Byte) (guidInitL.ui32l >> 16);
    manager->lastJobGuid.data4[3] = (UA_Byte) (guidInitL.ui32l >> 24);
    manager->lastJobGuid.data4[4] = (UA_Byte) (manager->lastJobGuid.data4[0]) ^ (guidInitL.ui32h);
    manager->lastJobGuid.data4[5] = (UA_Byte) (manager->lastJobGuid.data4[0]) ^ (guidInitL.ui32h >> 8);
    manager->lastJobGuid.data4[6] = (UA_Byte) (manager->lastJobGuid.data4[1]) ^ (guidInitL.ui32h >> 16);
    manager->lastJobGuid.data4[7] = (UA_Byte) (manager->lastJobGuid.data4[0]) ^ (guidInitL.ui32h >> 24);
}

void SubscriptionManager_deleteMembers(UA_Session *session, UA_Server *server) {
    UA_SubscriptionManager *manager = &(session->subscriptionManager);
    UA_Subscription *current;
    while((current = LIST_FIRST(&manager->serverSubscriptions))) {
        LIST_REMOVE(current, listEntry);
        UA_Subscription_deleteMembers(current, server);
        UA_free(current);
    }
}

void SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *newSubscription) {
    LIST_INSERT_HEAD(&manager->serverSubscriptions, newSubscription, listEntry);
}

UA_Subscription *SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager,
                                                         UA_Int32 subscriptionID) {
    UA_Subscription *sub;
    LIST_FOREACH(sub, &manager->serverSubscriptions, listEntry) {
        if(sub->subscriptionID == subscriptionID)
            break;
    }
    return sub;
}

UA_Int32 SubscriptionManager_deleteMonitoredItem(UA_SubscriptionManager *manager, UA_Int32 subscriptionID,
                                                 UA_UInt32 monitoredItemID) {
    UA_Subscription *sub = SubscriptionManager_getSubscriptionByID(manager, subscriptionID);
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->MonitoredItems, listEntry) {
        if (mon->itemId == monitoredItemID) {
            // FIXME!! don't we need to remove the list entry?
            MonitoredItem_delete(mon);
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
}

UA_Int32 SubscriptionManager_deleteSubscription(UA_Server *server, UA_SubscriptionManager *manager, UA_Int32 subscriptionID) {
    UA_Subscription *sub = SubscriptionManager_getSubscriptionByID(manager, subscriptionID);    
    if(!sub)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    LIST_REMOVE(sub, listEntry);
    UA_Subscription_deleteMembers(sub, server);
    UA_free(sub);
    return UA_STATUSCODE_GOOD;
} 

UA_UInt32 SubscriptionManager_getUniqueUIntID(UA_SubscriptionManager *manager) {
    UA_UInt32 id = ++(manager->lastSessionID);
    return id;
}

UA_Guid SubscriptionManager_getUniqueGUID(UA_SubscriptionManager *manager) {
    UA_Guid id;
    unsigned long *incremental = (unsigned long *) &manager->lastJobGuid.data4[0];
    incremental++;
    UA_Guid_copy(&(manager->lastJobGuid), &id);
    return id;
}
