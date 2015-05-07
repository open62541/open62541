#ifdef ENABLESUBSCRIPTIONS
#include "ua_types.h"
#include "ua_server_internal.h"
#include "ua_subscription_manager.h"

#include <stdio.h> // Remove later, debugging only

void SubscriptionManager_init(UA_Session *session) {
    UA_SubscriptionManager *manager = &(session->subscriptionManager);

    /* FIXME: These init values are empirical. Maybe they should be part
     *        of the server config? */
    manager->GlobalPublishingInterval      = (UA_Int32_BoundedValue)  { .maxValue = 100,   .minValue = 0, .currentValue=0 };
    manager->GlobalLifeTimeCount           = (UA_UInt32_BoundedValue) { .maxValue = 15000, .minValue = 0, .currentValue=0 };
    manager->GlobalKeepAliveCount          = (UA_UInt32_BoundedValue) { .maxValue = 100,   .minValue = 0, .currentValue=0 };
    manager->GlobalNotificationsPerPublish = (UA_Int32_BoundedValue)  { .maxValue = 1000,  .minValue = 1, .currentValue=0 };
    manager->GlobalSamplingInterval        = (UA_UInt32_BoundedValue) { .maxValue = 100,   .minValue = 0, .currentValue=0 };
    manager->GlobalQueueSize               = (UA_UInt32_BoundedValue) { .maxValue = 100,   .minValue = 0, .currentValue=0 };
    
    manager->ServerSubscriptions = (UA_ListOfUASubscriptions *) malloc (sizeof(UA_ListOfUASubscriptions));
    LIST_INIT(manager->ServerSubscriptions);
    
    manager->LastSessionID = (UA_UInt32) UA_DateTime_now();
    return;
}

UA_Subscription *UA_Subscription_new(UA_Int32 SubscriptionID) {
    UA_Subscription *new = (UA_Subscription *) malloc(sizeof(UA_Subscription));
    
    new->SubscriptionID = SubscriptionID;
    new->LastPublished  = 0;
    new->SequenceNumber = 0;
    new->MonitoredItems = (UA_ListOfUAMonitoredItems *) malloc (sizeof(UA_ListOfUAMonitoredItems));
    LIST_INIT(new->MonitoredItems);
    new->unpublishedNotifications = (UA_ListOfUnpublishedNotifications *) malloc(sizeof(UA_ListOfUnpublishedNotifications));
    LIST_INIT(new->unpublishedNotifications);
    return new;
}

UA_MonitoredItem *UA_MonitoredItem_new() {
    UA_MonitoredItem *new = (UA_MonitoredItem *) malloc(sizeof(UA_MonitoredItem));
    new->queue       = (UA_ListOfQueuedDataValues *) malloc (sizeof(UA_ListOfQueuedDataValues));
    new->QueueSize   = (UA_UInt32_BoundedValue) { .minValue = 0, .maxValue = 0, .currentValue = 0};
    new->LastSampled = 0;
    
    LIST_INIT(new->queue);
    
    return new;
}

void SubscriptionManager_addSubscription(UA_SubscriptionManager *manager, UA_Subscription *newSubscription) {    
    LIST_INSERT_HEAD(manager->ServerSubscriptions, newSubscription, listEntry);

    return;
}

UA_Subscription *SubscriptionManager_getSubscriptionByID(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID) {
    UA_Subscription *retsub, *sub;
    retsub = (UA_Subscription *) NULL;
    
    for (sub = (manager->ServerSubscriptions)->lh_first; sub != NULL; sub = sub->listEntry.le_next) {
        if (sub->SubscriptionID == SubscriptionID) {
            retsub = sub;
            break;
        }
    }
    return retsub;
}

UA_Int32 SubscriptionManager_deleteMonitoredItem(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID, UA_UInt32 MonitoredItemID) {
    UA_Subscription *sub;
    UA_MonitoredItem *mon;
    
    if (manager == NULL) return UA_STATUSCODE_BADINTERNALERROR;
    
    sub = SubscriptionManager_getSubscriptionByID(manager, SubscriptionID);
    if (sub == NULL) return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    for(mon=(sub->MonitoredItems)->lh_first; mon != NULL; mon=(mon->listEntry).le_next) {
        if (mon->ItemId == MonitoredItemID) {
            MonitoredItem_delete(mon);
            return UA_STATUSCODE_GOOD;
        }
    }
    
    return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
}

void MonitoredItem_delete(UA_MonitoredItem *monitoredItem) {
    if (monitoredItem == NULL) return;
    
    // Delete Queued Data
    MonitoredItem_ClearQueue(monitoredItem);
    // Remove from subscription list
    LIST_REMOVE(monitoredItem, listEntry);
    UA_free(monitoredItem);
    
    return;
}

UA_Int32 SubscriptionManager_deleteSubscription(UA_SubscriptionManager *manager, UA_Int32 SubscriptionID) {
    UA_Subscription  *sub;
    UA_MonitoredItem *mon;
    UA_unpublishedNotification *notify;
    
    sub = SubscriptionManager_getSubscriptionByID(manager, SubscriptionID);    
    if (sub != NULL) {
        // Delete registered subscriptions
        while (sub->MonitoredItems->lh_first != NULL)  {
           mon = sub->MonitoredItems->lh_first;
           // Delete Sampled data
           MonitoredItem_delete(mon);
        }
    }
    else {
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    }
    
    // Delete queued notification messages
    notify = sub->unpublishedNotifications->lh_first;
    while (sub->unpublishedNotifications->lh_first != NULL)  {
       notify = sub->unpublishedNotifications->lh_first;
       LIST_REMOVE(notify, listEntry);
       UA_free(notify);
    }
    
    LIST_REMOVE(sub, listEntry);
    UA_free(sub);
    
    return UA_STATUSCODE_GOOD;
} 

int Subscription_queuedNotifications(UA_Subscription *subscription) {
    int j = 0;
    if (subscription == NULL) return 0;
    
    for(UA_unpublishedNotification *i = subscription->unpublishedNotifications->lh_first; i != NULL; i=(i->listEntry).le_next) j++;
    
    return j;
}
    
void Subscription_updateNotifications(UA_Subscription *subscription) {
    UA_MonitoredItem *mon;
    //MonitoredItem_queuedValue *queuedValue;
    UA_unpublishedNotification *msg = NULL;
    UA_Int32 monItemsWithData = 0;
    
    if (subscription == NULL) return;
    if ((subscription->LastPublished + subscription->PublishingInterval) > UA_DateTime_now()) return;
       
    // Check if any MonitoredItem Queues hold data
    for(mon=subscription->MonitoredItems->lh_first; mon!= NULL; mon=mon->listEntry.le_next) {
        if (mon->queue->lh_first != NULL) {
            monItemsWithData++;
        }
    }
    
    // FIXME: This is hardcoded to 100 because it is not covered by the spec but we need to protect the server!
    if (Subscription_queuedNotifications(subscription) >= 10) {
        // Remove last entry
        for(msg = subscription->unpublishedNotifications->lh_first; (msg->listEntry).le_next != NULL; msg=(msg->listEntry).le_next);
        LIST_REMOVE(msg, listEntry);
        UA_free(msg);
    }
    
    if (monItemsWithData == 0) {
        // Decrement KeepAlive
        subscription->KeepAliveCount.currentValue--;
        // +- Generate KeepAlive msg if counter overruns
        if (subscription->KeepAliveCount.currentValue < subscription->KeepAliveCount.minValue) {
            msg = (UA_unpublishedNotification *) malloc(sizeof(UA_unpublishedNotification));
            msg->notification.sequenceNumber = (subscription->SequenceNumber)++;
            msg->notification.publishTime    = UA_DateTime_now();
            msg->notification.notificationDataSize = monItemsWithData;
            msg->notification.sequenceNumber = subscription->SequenceNumber++;
            msg->notification.notificationDataSize = 0;
            
            LIST_INSERT_HEAD(subscription->unpublishedNotifications, msg, listEntry);
            subscription->KeepAliveCount.currentValue = subscription->KeepAliveCount.maxValue;
        }
        
        return;
    }
    
    // One or more MonitoredItems hold data -> create a new NotificationMessage
    printf("UpdateNotification: creating NotificationMessage for %i items holding data\n", msg->notification.notificationDataSize);
    
    // +- Create Array of NotificationData
    // +- Clear Queue
    
    // Fill the NotificationMessage with NotificationData
    for(mon=subscription->MonitoredItems->lh_first; mon!= NULL; mon=mon->listEntry.le_next) {
        if (mon->queue->lh_first != NULL) {
            printf("UpdateNotification: monitored %i\n", mon->ItemId);
            if (msg == NULL) {
                
            }
        }
    }
    return;
}

void MonitoredItem_ClearQueue(UA_MonitoredItem *monitoredItem) {
    if (monitoredItem == NULL) return;
    while(monitoredItem->queue->lh_first != NULL) {
        LIST_REMOVE(monitoredItem->queue->lh_first, listEntry);
    }
    
    return;
}

void MonitoredItem_QueuePushDataValue(UA_MonitoredItem *monitoredItem) {
    MonitoredItem_queuedValue *newvalue, *queueItem;
    
    if (monitoredItem == NULL) return;
    if( (monitoredItem->LastSampled + monitoredItem->SamplingInterval) > UA_DateTime_now()) return;
    
    newvalue = (MonitoredItem_queuedValue *) malloc(sizeof(MonitoredItem_queuedValue));
    
    // Create new Value
    switch(monitoredItem->AttributeID) {
        UA_ATTRIBUTEID_NODEID:
            UA_Variant_setScalarCopy(&(newvalue->value), (const UA_NodeId *) &((const UA_Node *) monitoredItem->monitoredNode)->nodeId, &UA_TYPES[UA_TYPES_NODEID]);
        break;
        UA_ATTRIBUTEID_NODECLASS:
            UA_Variant_setScalarCopy(&(newvalue->value), (const UA_Int32 *) &((const UA_Node *) monitoredItem->monitoredNode)->nodeClass, &UA_TYPES[UA_TYPES_INT32]);
        break;
        UA_ATTRIBUTEID_BROWSENAME:
            UA_Variant_setScalarCopy(&(newvalue->value), (const UA_String *) &((const UA_Node *) monitoredItem->monitoredNode)->browseName, &UA_TYPES[UA_TYPES_STRING]);
        break;
        UA_ATTRIBUTEID_DISPLAYNAME:
            UA_Variant_setScalarCopy(&(newvalue->value), (const UA_String *) &((const UA_Node *) monitoredItem->monitoredNode)->displayName, &UA_TYPES[UA_TYPES_STRING]);
        break;
        UA_ATTRIBUTEID_DESCRIPTION:
            UA_Variant_setScalarCopy(&(newvalue->value), (const UA_String *) &((const UA_Node *) monitoredItem->monitoredNode)->description, &UA_TYPES[UA_TYPES_STRING]);
        break;
        UA_ATTRIBUTEID_WRITEMASK:
        break;
        UA_ATTRIBUTEID_USERWRITEMASK:
        break;
        UA_ATTRIBUTEID_ISABSTRACT:
        break;
        UA_ATTRIBUTEID_SYMMETRIC:
        break;
        UA_ATTRIBUTEID_INVERSENAME:
        break;
        UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        break;
        UA_ATTRIBUTEID_EVENTNOTIFIER:
        break;
        UA_ATTRIBUTEID_VALUE: 
            if (((const UA_Node *) monitoredItem->monitoredNode)->nodeClass == UA_NODECLASS_VARIABLE) {
                UA_Variant_copy( (const UA_Variant *) &((const UA_VariableNode *) monitoredItem->monitoredNode)->value, &(newvalue->value));
            }
        break;
        UA_ATTRIBUTEID_DATATYPE:
        break;
        UA_ATTRIBUTEID_VALUERANK:
        break;
        UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        break;
        UA_ATTRIBUTEID_ACCESSLEVEL:
        break;
        UA_ATTRIBUTEID_USERACCESSLEVEL:
        break;
        UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        break;
        UA_ATTRIBUTEID_HISTORIZING:
        break;
        UA_ATTRIBUTEID_EXECUTABLE:
        break;
        UA_ATTRIBUTEID_USEREXECUTABLE:
        break;
        default:
        break;
    }
    
    printf("MonitoredItem AddValue: %i,%i\n", (monitoredItem->QueueSize).currentValue, (monitoredItem->QueueSize).maxValue);
    if ((monitoredItem->QueueSize).currentValue >= (monitoredItem->QueueSize).maxValue) {
        if (monitoredItem->DiscardOldest == UA_TRUE && monitoredItem->queue->lh_first != NULL ) {
            for(queueItem = monitoredItem->queue->lh_first; queueItem->listEntry.le_next != NULL; queueItem = queueItem->listEntry.le_next) {}

            LIST_REMOVE(queueItem, listEntry);
            UA_free(queueItem);
            (monitoredItem->QueueSize).currentValue--;
        }
        else {
            UA_free(newvalue);
        }
    }
    if ((monitoredItem->QueueSize).currentValue < (monitoredItem->QueueSize).maxValue) {
        LIST_INSERT_HEAD(monitoredItem->queue, newvalue, listEntry);
        (monitoredItem->QueueSize).currentValue++;
    }
    return;
}

#endif //#ifdef ENABLESUBSCRIPTIONS
