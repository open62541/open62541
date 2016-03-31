#include "ua_subscription.h"
#include "ua_server_internal.h"
#include "ua_nodestore.h"

/****************/
/* Subscription */
/****************/

UA_Subscription *UA_Subscription_new(UA_UInt32 subscriptionID) {
    UA_Subscription *new = UA_malloc(sizeof(UA_Subscription));
    if(!new)
        return NULL;
    new->subscriptionID = subscriptionID;
    new->lastPublished  = 0;
    new->sequenceNumber = 1;
    memset(&new->timedUpdateJobGuid, 0, sizeof(UA_Guid));
    new->timedUpdateIsRegistered = false;
    LIST_INIT(&new->MonitoredItems);
    LIST_INIT(&new->unpublishedNotifications);
    new->unpublishedNotificationsSize = 0;
    return new;
}

void UA_Subscription_deleteMembers(UA_Subscription *subscription, UA_Server *server) {
    // Just in case any parallel process attempts to access this subscription
    // while we are deleting it... make it vanish.
    subscription->subscriptionID = 0;
    
    // Delete monitored Items
    UA_MonitoredItem *mon, *tmp_mon;
    LIST_FOREACH_SAFE(mon, &subscription->MonitoredItems, listEntry, tmp_mon) {
        LIST_REMOVE(mon, listEntry);
        MonitoredItem_delete(server, mon);
    }
    
    // Delete unpublished Notifications
    Subscription_deleteUnpublishedNotification(0, true, subscription);
    
    // Unhook/Unregister any timed work assiociated with this subscription
    if(subscription->timedUpdateIsRegistered) {
        Subscription_unregisterUpdateJob(server, subscription);
        subscription->timedUpdateIsRegistered = false;
    }
}

void Subscription_generateKeepAlive(UA_Subscription *subscription) {
    if(subscription->keepAliveCount.current > subscription->keepAliveCount.min &&
       subscription->keepAliveCount.current <= subscription->keepAliveCount.max)
        return;

    UA_unpublishedNotification *msg = UA_calloc(1,sizeof(UA_unpublishedNotification));
    if(!msg)
        return;
    msg->notification.notificationData = NULL;
    // KeepAlive uses next message, but does not increment counter
    msg->notification.sequenceNumber = subscription->sequenceNumber + 1;
    msg->notification.publishTime    = UA_DateTime_now();
    msg->notification.notificationDataSize = 0;
    LIST_INSERT_HEAD(&subscription->unpublishedNotifications, msg, listEntry);
    subscription->unpublishedNotificationsSize += 1;
    subscription->keepAliveCount.current = subscription->keepAliveCount.max;
}

void Subscription_updateNotifications(UA_Subscription *subscription) {
    UA_MonitoredItem *mon;
    //MonitoredItem_queuedValue *queuedValue;
    UA_unpublishedNotification *msg;
    UA_UInt32 monItemsChangeT = 0, monItemsStatusT = 0, monItemsEventT = 0;
    
    if(!subscription || subscription->lastPublished +
       (UA_UInt32)(subscription->publishingInterval * UA_MSEC_TO_DATETIME) > UA_DateTime_now())
        return;
    
    // Make sure there is data to be published and establish which message types
    // will need to be generated
    LIST_FOREACH(mon, &subscription->MonitoredItems, listEntry) {
        // Check if this MonitoredItems Queue holds data and how much data is held in total
        if(!TAILQ_FIRST(&mon->queue))
            continue;
        if((mon->monitoredItemType & MONITOREDITEM_TYPE_CHANGENOTIFY) != 0)
            monItemsChangeT+=mon->queueSize.current;
	    else if((mon->monitoredItemType & MONITOREDITEM_TYPE_STATUSNOTIFY) != 0)
            monItemsStatusT+=mon->queueSize.current;
	    else if((mon->monitoredItemType & MONITOREDITEM_TYPE_EVENTNOTIFY)  != 0)
            monItemsEventT+=mon->queueSize.current;
    }
    
    // FIXME: This is hardcoded to 100 because it is not covered by the spec but we need to protect the server!
    if(subscription->unpublishedNotificationsSize >= 10) {
        // Remove last entry
        Subscription_deleteUnpublishedNotification(0, true, subscription);
    }
    
    if(monItemsChangeT == 0 && monItemsEventT == 0 && monItemsStatusT == 0) {
        // Decrement KeepAlive
        subscription->keepAliveCount.current--;
        // +- Generate KeepAlive msg if counter overruns
        if (subscription->keepAliveCount.current < subscription->keepAliveCount.min)
          Subscription_generateKeepAlive(subscription);
        
        return;
    }
    
    msg = UA_calloc(1, sizeof(UA_unpublishedNotification));
    msg->notification.sequenceNumber = subscription->sequenceNumber++;
    msg->notification.publishTime = UA_DateTime_now();
    
    // NotificationData is an array of Change, Status and Event messages, each containing the appropriate
    // list of Queued values from all monitoredItems of that type
    msg->notification.notificationDataSize = !!monItemsChangeT; // 1 if the pointer is not null, else 0
    // + ISNOTZERO(monItemsEventT) + ISNOTZERO(monItemsStatusT);
    msg->notification.notificationData =
        UA_Array_new(msg->notification.notificationDataSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    
    for(size_t notmsgn = 0; notmsgn < msg->notification.notificationDataSize; notmsgn++) {
        // Set the notification message type and encoding for each of 
        //   the three possible NotificationData Types

        /* msg->notification->notificationData[notmsgn].encoding = 1; // Encoding is always binary */
        /* msg->notification->notificationData[notmsgn].typeId = UA_NODEID_NUMERIC(0, 811); */
      
        if(notmsgn == 0) {
            UA_DataChangeNotification *changeNotification = UA_DataChangeNotification_new();
            changeNotification->monitoredItems = UA_Array_new(monItemsChangeT, &UA_TYPES[UA_TYPES_MONITOREDITEMNOTIFICATION]);
	
            // Scan all monitoredItems in this subscription and have their queue transformed into an Array of
            // the propper NotificationMessageType (Status, Change, Event)
            monItemsChangeT = 0;
            LIST_FOREACH(mon, &subscription->MonitoredItems, listEntry) {
                if(mon->monitoredItemType != MONITOREDITEM_TYPE_CHANGENOTIFY || !TAILQ_FIRST(&mon->queue))
                    continue;
                // Note: Monitored Items might not return a queuedValue if there is a problem encoding it.
                monItemsChangeT += MonitoredItem_QueueToDataChangeNotifications(&changeNotification->monitoredItems[monItemsChangeT], mon);
                MonitoredItem_ClearQueue(mon);
            }
            changeNotification->monitoredItemsSize = monItemsChangeT;
            msg->notification.notificationData[notmsgn].encoding = UA_EXTENSIONOBJECT_DECODED;
            msg->notification.notificationData[notmsgn].content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION];
            msg->notification.notificationData[notmsgn].content.decoded.data = changeNotification;
        } else if(notmsgn == 1) {
            // FIXME: Constructing a StatusChangeNotification is not implemented
        } else if(notmsgn == 2) {
            // FIXME: Constructing a EventListNotification is not implemented
        }
    }
    LIST_INSERT_HEAD(&subscription->unpublishedNotifications, msg, listEntry);
    subscription->unpublishedNotificationsSize += 1;
}

UA_UInt32 *Subscription_getAvailableSequenceNumbers(UA_Subscription *sub) {
    UA_UInt32 *seqArray = UA_malloc(sizeof(UA_UInt32) * sub->unpublishedNotificationsSize);
    if(!seqArray)
        return NULL;
  
    int i = 0;
    UA_unpublishedNotification *not;
    LIST_FOREACH(not, &sub->unpublishedNotifications, listEntry) {
        seqArray[i] = not->notification.sequenceNumber;
        i++;
    }
    return seqArray;
}

void Subscription_copyNotificationMessage(UA_NotificationMessage *dst, UA_unpublishedNotification *src) {
    if(!dst)
        return;
    
    UA_NotificationMessage *latest = &src->notification;
    dst->notificationDataSize = latest->notificationDataSize;
    dst->publishTime = latest->publishTime;
    dst->sequenceNumber = latest->sequenceNumber;
    
    if(latest->notificationDataSize == 0)
        return;

    dst->notificationData = UA_ExtensionObject_new();
    UA_ExtensionObject_copy(latest->notificationData, dst->notificationData);
}

UA_UInt32 Subscription_deleteUnpublishedNotification(UA_UInt32 seqNo, UA_Boolean bDeleteAll, UA_Subscription *sub) {
    UA_UInt32 deletedItems = 0;
    UA_unpublishedNotification *not, *tmp;
    LIST_FOREACH_SAFE(not, &sub->unpublishedNotifications, listEntry, tmp) {
        if(!bDeleteAll && not->notification.sequenceNumber != seqNo)
            continue;
        LIST_REMOVE(not, listEntry);
        sub->unpublishedNotificationsSize -= 1;
        UA_NotificationMessage_deleteMembers(&not->notification);
        UA_free(not);
        deletedItems++;
    }
    return deletedItems;
}


static void Subscription_timedUpdateNotificationsJob(UA_Server *server, void *data) {
    // Timed-Worker/Job Version of updateNotifications
    UA_Subscription *sub = (UA_Subscription *) data;
    UA_MonitoredItem *mon;
    
    if(!data || !server)
        return;
    
    // This is set by the Subscription_delete function to detere us from fiddling with
    // this subscription if it is being deleted (not technically thread save, but better
    // then nothing at all)
    if(sub->subscriptionID == 0)
        return;
    
    // FIXME: This should be done by the event system
    LIST_FOREACH(mon, &sub->MonitoredItems, listEntry)
        MonitoredItem_QueuePushDataValue(server, mon);
    
    Subscription_updateNotifications(sub);
}

UA_StatusCode Subscription_registerUpdateJob(UA_Server *server, UA_Subscription *sub) {
    if(sub->publishingInterval <= 5 ) 
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_Job job = (UA_Job) {.type = UA_JOBTYPE_METHODCALL,
                           .job.methodCall = {.method = Subscription_timedUpdateNotificationsJob,
                                              .data = sub} };
    
    /* Practically enough, the client sends a uint32 in ms, which we store as
       datetime, which here is required in as uint32 in ms as the interval */
    UA_StatusCode retval = UA_Server_addRepeatedJob(server, job,
                                                    (UA_UInt32)sub->publishingInterval,
                                                    &sub->timedUpdateJobGuid);
    if(retval == UA_STATUSCODE_GOOD)
        sub->timedUpdateIsRegistered = true;
    return retval;
}

UA_StatusCode Subscription_unregisterUpdateJob(UA_Server *server, UA_Subscription *sub) {
    sub->timedUpdateIsRegistered = false;
    return UA_Server_removeRepeatedJob(server, sub->timedUpdateJobGuid);
}

/*****************/
/* MonitoredItem */
/*****************/

UA_MonitoredItem * UA_MonitoredItem_new() {
    UA_MonitoredItem *new = UA_malloc(sizeof(UA_MonitoredItem));
    new->queueSize = (UA_BoundedUInt32) { .min = 0, .max = 0, .current = 0};
    // FIXME: This is currently hardcoded;
    new->monitoredItemType = MONITOREDITEM_TYPE_CHANGENOTIFY;
    TAILQ_INIT(&new->queue);
    UA_NodeId_init(&new->monitoredNodeId);
    new->lastSampledValue = UA_BYTESTRING_NULL;
    memset(&new->sampleJobGuid, 0, sizeof(UA_Guid));
    new->sampleJobIsRegistered = false;
    return new;
}

void MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    MonitoredItem_unregisterUpdateJob(server, monitoredItem);
    MonitoredItem_ClearQueue(monitoredItem);
    LIST_REMOVE(monitoredItem, listEntry);
    UA_ByteString_deleteMembers(&monitoredItem->lastSampledValue);
    UA_NodeId_deleteMembers(&monitoredItem->monitoredNodeId);
    UA_free(monitoredItem);
}

UA_UInt32 MonitoredItem_QueueToDataChangeNotifications(UA_MonitoredItemNotification *dst,
                                                       UA_MonitoredItem *monitoredItem) {
    UA_UInt32 queueSize = 0;
    MonitoredItem_queuedValue *queueItem;
  
    // Count instead of relying on the items current
    TAILQ_FOREACH(queueItem, &monitoredItem->queue, listEntry) {
        dst[queueSize].clientHandle = monitoredItem->clientHandle;
        UA_DataValue_copy(&queueItem->value, &dst[queueSize].value);

        dst[queueSize].value.hasServerPicoseconds = false;
        dst[queueSize].value.hasServerTimestamp   = true;
        dst[queueSize].value.serverTimestamp      = UA_DateTime_now();
    
        // Do not create variants with no type -> will make calcSizeBinary() segfault.
        if(dst[queueSize].value.value.type)
            queueSize++;
    }
    return queueSize;
}

void MonitoredItem_ClearQueue(UA_MonitoredItem *monitoredItem) {
    MonitoredItem_queuedValue *val, *val_tmp;
    TAILQ_FOREACH_SAFE(val, &monitoredItem->queue, listEntry, val_tmp) {
        TAILQ_REMOVE(&monitoredItem->queue, val, listEntry);
        UA_DataValue_deleteMembers(&val->value);
        UA_free(val);
    }
    monitoredItem->queueSize.current = 0;
}

UA_Boolean MonitoredItem_CopyMonitoredValueToVariant(UA_UInt32 attributeID, const UA_Node *src,
                                                     UA_DataValue *dst) {
    UA_Boolean samplingError = true; 
    UA_DataValue sourceDataValue;
    UA_DataValue_init(&sourceDataValue);
  
    // FIXME: Not all attributeIDs can be monitored yet
    switch(attributeID) {
    case UA_ATTRIBUTEID_NODEID:
        UA_Variant_setScalarCopy(&dst->value, (const UA_NodeId*)&src->nodeId, &UA_TYPES[UA_TYPES_NODEID]);
        dst->hasValue = true;
        samplingError = false;
        break;
    case UA_ATTRIBUTEID_NODECLASS:
        UA_Variant_setScalarCopy(&dst->value, (const UA_Int32*)&src->nodeClass, &UA_TYPES[UA_TYPES_INT32]);
        dst->hasValue = true;
        samplingError = false;
        break;
    case UA_ATTRIBUTEID_BROWSENAME:
        UA_Variant_setScalarCopy(&dst->value, (const UA_String*)&src->browseName, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        dst->hasValue = true;
        samplingError = false;
        break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
        UA_Variant_setScalarCopy(&dst->value, (const UA_String*)&src->displayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        dst->hasValue = true;
        samplingError = false;
        break;
    case UA_ATTRIBUTEID_DESCRIPTION:
        UA_Variant_setScalarCopy(&dst->value, (const UA_String*)&src->displayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        dst->hasValue = true;
        samplingError = false;
        break;
    case UA_ATTRIBUTEID_WRITEMASK:
        UA_Variant_setScalarCopy(&dst->value, (const UA_String*)&src->writeMask, &UA_TYPES[UA_TYPES_UINT32]);
        dst->hasValue = true;
        samplingError = false;
        break;
    case UA_ATTRIBUTEID_USERWRITEMASK:
        UA_Variant_setScalarCopy(&dst->value, (const UA_String*)&src->writeMask, &UA_TYPES[UA_TYPES_UINT32]);
        dst->hasValue = true;
        samplingError = false;
        break;
    case UA_ATTRIBUTEID_ISABSTRACT:
        break;
    case UA_ATTRIBUTEID_SYMMETRIC:
        break;
    case UA_ATTRIBUTEID_INVERSENAME:
        break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
        break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        break;
    case UA_ATTRIBUTEID_VALUE: 
        if(src->nodeClass == UA_NODECLASS_VARIABLE) {
            const UA_VariableNode *vsrc = (const UA_VariableNode*)src;
            if(vsrc->valueSource == UA_VALUESOURCE_VARIANT) {
                if(vsrc->value.variant.callback.onRead)
                    vsrc->value.variant.callback.onRead(vsrc->value.variant.callback.handle, vsrc->nodeId,
                                                        &dst->value, NULL);
                UA_Variant_copy(&vsrc->value.variant.value, &dst->value);
                dst->hasValue = true;
                samplingError = false;
            } else {
                if(vsrc->valueSource != UA_VALUESOURCE_DATASOURCE || vsrc->value.dataSource.read == NULL)
                    break;
                if(vsrc->value.dataSource.read(vsrc->value.dataSource.handle, vsrc->nodeId, true,
                                               NULL, &sourceDataValue) != UA_STATUSCODE_GOOD)
                    break;
                UA_DataValue_copy(&sourceDataValue, dst);
                UA_DataValue_deleteMembers(&sourceDataValue);
                samplingError = false;
            }
        }
        break;
    case UA_ATTRIBUTEID_DATATYPE:
        break;
    case UA_ATTRIBUTEID_VALUERANK:
        break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
        break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
        break;
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
        break;
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
        break;
    case UA_ATTRIBUTEID_HISTORIZING:
        break;
    case UA_ATTRIBUTEID_EXECUTABLE:
        break;
    case UA_ATTRIBUTEID_USEREXECUTABLE:
        break;
    default:
        break;
    }
  
    return samplingError;
}

void MonitoredItem_QueuePushDataValue(UA_Server *server, UA_MonitoredItem *monitoredItem) {
  
    /* if(monitoredItem->lastSampled + (UA_MSEC_TO_DATETIME * monitoredItem->samplingInterval) > UA_DateTime_now()) */
    /*     return; */
  
    // FIXME: Actively suppress non change value based monitoring. There should be
    // another function to handle status and events.
    if(monitoredItem->monitoredItemType != MONITOREDITEM_TYPE_CHANGENOTIFY)
        return;

    /* Verify that the node being monitored is still valid */
    const UA_Node *target = UA_NodeStore_get(server->nodestore, &monitoredItem->monitoredNodeId);
    if(!target)
        return;

    MonitoredItem_queuedValue *newvalue = UA_malloc(sizeof(MonitoredItem_queuedValue));
    if(!newvalue)
        return;
    UA_DataValue_init(&newvalue->value);
  
    UA_Boolean samplingError = MonitoredItem_CopyMonitoredValueToVariant(monitoredItem->attributeID, target,
                                                                         &newvalue->value);

    if(samplingError || !newvalue->value.value.type) {
        UA_DataValue_deleteMembers(&newvalue->value);
        UA_free(newvalue);
        return;
    }

    /* encode the data */
    size_t binsize = UA_calcSizeBinary(&newvalue->value, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_ByteString newValueAsByteString;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&newValueAsByteString, binsize);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_DataValue_deleteMembers(&newvalue->value);
        UA_free(newvalue);
        return;
    }
    size_t encodingOffset = 0;
    retval = UA_encodeBinary(&newvalue->value, &UA_TYPES[UA_TYPES_DATAVALUE], &newValueAsByteString, &encodingOffset);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_deleteMembers(&newValueAsByteString);
        UA_DataValue_deleteMembers(&newvalue->value);
        UA_free(newvalue);
        return;
    }

    /* did the content change? */
    if(monitoredItem->lastSampledValue.data &&
       UA_String_equal(&newValueAsByteString, &monitoredItem->lastSampledValue)) {
        UA_DataValue_deleteMembers(&newvalue->value);
        UA_free(newvalue);
        UA_String_deleteMembers(&newValueAsByteString);
        return;
    }
  
    /* do we have space? */
    if(monitoredItem->queueSize.current >= monitoredItem->queueSize.max) {
        if(!monitoredItem->discardOldest) {
            // We cannot remove the oldest value and theres no queue space left. We're done here.
            UA_DataValue_deleteMembers(&newvalue->value);
            UA_free(newvalue);
            UA_String_deleteMembers(&newValueAsByteString);
            return;
        }
        MonitoredItem_queuedValue *queueItem = TAILQ_LAST(&monitoredItem->queue, QueueOfQueueDataValues);
        TAILQ_REMOVE(&monitoredItem->queue, queueItem, listEntry);
        UA_free(queueItem);
        monitoredItem->queueSize.current--;
    }
  
    /* add the sample */
    UA_ByteString_deleteMembers(&monitoredItem->lastSampledValue);
    monitoredItem->lastSampledValue = newValueAsByteString;
    TAILQ_INSERT_HEAD(&monitoredItem->queue, newvalue, listEntry);
    monitoredItem->queueSize.current++;
}

UA_StatusCode MonitoredItem_registerSampleJob(UA_Server *server, UA_MonitoredItem *mon) {
    if(mon->samplingInterval <= 5 ) 
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_Job job = (UA_Job) {.type = UA_JOBTYPE_METHODCALL,
                           .job.methodCall = {.method = (UA_ServerCallback)MonitoredItem_QueuePushDataValue,
                                              .data = mon} };
    
    UA_StatusCode retval = UA_Server_addRepeatedJob(server, job, (UA_UInt32)mon->samplingInterval,
                                                    &mon->sampleJobGuid);
    if(retval == UA_STATUSCODE_GOOD)
        mon->sampleJobIsRegistered = true;
    return retval;
}

UA_StatusCode MonitoredItem_unregisterUpdateJob(UA_Server *server, UA_MonitoredItem *mon) {
    mon->sampleJobIsRegistered = false;
    return UA_Server_removeRepeatedJob(server, mon->sampleJobGuid);
}
