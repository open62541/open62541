#include "ua_subscription.h"
#include "ua_server_internal.h"
#include "ua_nodestore.h"

/****************/
/* Subscription */
/****************/

UA_Subscription *UA_Subscription_new(UA_Int32 SubscriptionID) {
    UA_Subscription *new = UA_malloc(sizeof(UA_Subscription));
    if(!new)
        return UA_NULL;
    new->SubscriptionID = SubscriptionID;
    new->LastPublished  = 0;
    new->SequenceNumber = 0;
    memset(&new->timedUpdateJobGuid, 0, sizeof(UA_Guid));
    new->timedUpdateJob          = UA_NULL;
    new->timedUpdateIsRegistered = UA_FALSE;
    LIST_INIT(&new->MonitoredItems);
    LIST_INIT(&new->unpublishedNotifications);
    return new;
}

void UA_Subscription_deleteMembers(UA_Server *server, UA_Subscription *subscription) {
    UA_unpublishedNotification *not;
    UA_MonitoredItem *mon;
    
    // Just in case any parallel process attempts to acces this subscription
    // while we are deleting it... make it vanish.
    subscription->SubscriptionID = 0;
    
    // Delete monitored Items
    while(subscription->MonitoredItems.lh_first != NULL) {
        mon = subscription->MonitoredItems.lh_first;
        LIST_REMOVE(mon, listEntry);
        MonitoredItem_delete(mon);
    }
    
    // Delete unpublished Notifications
    while(subscription->unpublishedNotifications.lh_first !=  NULL) {
        not = subscription->unpublishedNotifications.lh_first;
        LIST_REMOVE(not, listEntry);
        Subscription_deleteUnpublishedNotification(not->notification->sequenceNumber, subscription);
    }
    
    // Unhook/Unregister any timed work assiociated with this subscription
    if (subscription->timedUpdateJob != UA_NULL)
        Subscription_unregisterUpdateJob(server, subscription);
        UA_free(subscription->timedUpdateJob);
    
    return;
}

UA_UInt32 Subscription_queuedNotifications(UA_Subscription *subscription) {
    if (!subscription)
        return 0;

    UA_UInt32 j = 0;
    UA_unpublishedNotification *i;
    LIST_FOREACH(i, &subscription->unpublishedNotifications, listEntry)
        j++;
    return j;
}

void Subscription_generateKeepAlive(UA_Subscription *subscription) {
    if(subscription->KeepAliveCount.currentValue > subscription->KeepAliveCount.minValue &&
       subscription->KeepAliveCount.currentValue <= subscription->KeepAliveCount.maxValue)
        return;

    UA_unpublishedNotification *msg = UA_malloc(sizeof(UA_unpublishedNotification));
    if(!msg)
        return;
    msg->notification = UA_NULL;
    msg->notification = UA_malloc(sizeof(UA_NotificationMessage));
    msg->notification->notificationData = UA_NULL;
    // KeepAlive uses next message, but does not increment counter
    msg->notification->sequenceNumber = subscription->SequenceNumber + 1;
    msg->notification->publishTime    = UA_DateTime_now();
    msg->notification->notificationDataSize = 0;
    LIST_INSERT_HEAD(&subscription->unpublishedNotifications, msg, listEntry);
    subscription->KeepAliveCount.currentValue = subscription->KeepAliveCount.maxValue;
}

void Subscription_updateNotifications(UA_Subscription *subscription) {
    UA_MonitoredItem *mon;
    //MonitoredItem_queuedValue *queuedValue;
    UA_unpublishedNotification *msg = NULL;
    UA_UInt32 monItemsChangeT = 0, monItemsStatusT = 0, monItemsEventT = 0;
    UA_DataChangeNotification *changeNotification;
    size_t notificationOffset;
    
    if(!subscription || subscription->LastPublished + subscription->PublishingInterval > UA_DateTime_now())
        return;
    
    // Make sure there is data to be published and establish which message types
    // will need to be generated
    LIST_FOREACH(mon, &subscription->MonitoredItems, listEntry) {
        // Check if this MonitoredItems Queue holds data and how much data is held in total
        if(!TAILQ_FIRST(&mon->queue))
            continue;
        if((mon->MonitoredItemType & MONITOREDITEM_TYPE_CHANGENOTIFY) != 0)
            monItemsChangeT+=mon->QueueSize.currentValue;
	    else if((mon->MonitoredItemType & MONITOREDITEM_TYPE_STATUSNOTIFY) != 0)
            monItemsStatusT+=mon->QueueSize.currentValue;
	    else if ((mon->MonitoredItemType & MONITOREDITEM_TYPE_EVENTNOTIFY)  != 0)
            monItemsEventT+=mon->QueueSize.currentValue;
    }
    
    // FIXME: This is hardcoded to 100 because it is not covered by the spec but we need to protect the server!
    if (Subscription_queuedNotifications(subscription) >= 10) {
        // Remove last entry
        LIST_FOREACH(msg, &subscription->unpublishedNotifications, listEntry)
            LIST_REMOVE(msg, listEntry);
        UA_free(msg);
    }
    
    if (monItemsChangeT == 0 && monItemsEventT == 0 && monItemsStatusT == 0) {
        // Decrement KeepAlive
        subscription->KeepAliveCount.currentValue--;
        // +- Generate KeepAlive msg if counter overruns
        Subscription_generateKeepAlive(subscription);
        return;
    }
    
    msg = (UA_unpublishedNotification *) UA_malloc(sizeof(UA_unpublishedNotification));
    msg->notification = UA_malloc(sizeof(UA_NotificationMessage));
    INITPOINTER(msg->notification->notificationData);
    msg->notification->sequenceNumber = subscription->SequenceNumber++;
    msg->notification->publishTime    = UA_DateTime_now();
    
    // NotificationData is an array of Change, Status and Event messages, each containing the appropriate
    // list of Queued values from all monitoredItems of that type
    msg->notification->notificationDataSize = ISNOTZERO(monItemsChangeT);
    // + ISNOTZERO(monItemsEventT) + ISNOTZERO(monItemsStatusT);
    msg->notification->notificationData = UA_malloc(sizeof(UA_ExtensionObject) *
                                                    msg->notification->notificationDataSize);
    
    for(int notmsgn=0; notmsgn < msg->notification->notificationDataSize; notmsgn++) {
        // Set the notification message type and encoding for each of 
        //   the three possible NotificationData Types
        msg->notification->notificationData[notmsgn].encoding = 1; // Encoding is always binary
        msg->notification->notificationData[notmsgn].typeId = UA_NODEID_NUMERIC(0, 811);
      
        if(notmsgn == 0) {
            // Construct a DataChangeNotification
            changeNotification = UA_malloc(sizeof(UA_DataChangeNotification));
	
            // Create one DataChangeNotification for each queue item held in each monitoredItems queue:
            changeNotification->monitoredItems =
                UA_malloc(sizeof(UA_MonitoredItemNotification) * monItemsChangeT);
	
            // Scan all monitoredItems in this subscription and have their queue transformed into an Array of
            // the propper NotificationMessageType (Status, Change, Event)
            monItemsChangeT = 0;
            LIST_FOREACH(mon, &subscription->MonitoredItems, listEntry) {
                if(mon->MonitoredItemType != MONITOREDITEM_TYPE_CHANGENOTIFY || !TAILQ_FIRST(&mon->queue))
                    continue;
                // Note: Monitored Items might not return a queuedValue if there is a problem encoding it.
                monItemsChangeT += MonitoredItem_QueueToDataChangeNotifications(&(changeNotification->monitoredItems[monItemsChangeT]), mon);
                MonitoredItem_ClearQueue(mon);
            }

            changeNotification->monitoredItemsSize  = monItemsChangeT;
            changeNotification->diagnosticInfosSize = 0;
            changeNotification->diagnosticInfos     = UA_NULL;
        
            msg->notification->notificationData[notmsgn].body.length =
                UA_calcSizeBinary(changeNotification, &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION]);
            msg->notification->notificationData[notmsgn].body.data   =
                UA_calloc(0, (msg->notification->notificationData[notmsgn]).body.length);
        
            notificationOffset = 0;
            UA_encodeBinary(changeNotification, &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION],
                            &(msg->notification->notificationData[notmsgn].body), &notificationOffset);
	
            // FIXME: Not properly freed!
            for(unsigned int i=0; i<monItemsChangeT; i++) {
                UA_MonitoredItemNotification *thisNotification = &(changeNotification->monitoredItems[i]);
                UA_DataValue_deleteMembers(&(thisNotification->value));
            }
            UA_free(changeNotification->monitoredItems);
            UA_free(changeNotification);
        } else if (notmsgn == 1) {
            // FIXME: Constructing a StatusChangeNotification is not implemented
        } else if (notmsgn == 2) {
            // FIXME: Constructing a EventListNotification is not implemented
        }
    }
    LIST_INSERT_HEAD(&subscription->unpublishedNotifications, msg, listEntry);
}

UA_UInt32 *Subscription_getAvailableSequenceNumbers(UA_Subscription *sub) {
    UA_UInt32 *seqArray = UA_malloc(sizeof(UA_UInt32) * Subscription_queuedNotifications(sub));
    if(!seqArray)
        return UA_NULL;
  
    int i = 0;
    UA_unpublishedNotification *not;
    LIST_FOREACH(not, &sub->unpublishedNotifications, listEntry) {
        seqArray[i] = not->notification->sequenceNumber;
        i++;
    }
    return seqArray;
}

void Subscription_copyTopNotificationMessage(UA_NotificationMessage *dst, UA_Subscription *sub) {
    UA_NotificationMessage *latest;
    
    if(!dst)
        return;
    
    if (Subscription_queuedNotifications(sub) == 0) {
      dst->notificationDataSize = 0;
      dst->publishTime = UA_DateTime_now();
      dst->sequenceNumber = 0;
      return;
    }
    
    latest = sub->unpublishedNotifications.lh_first->notification;
    dst->notificationDataSize = latest->notificationDataSize;
    dst->publishTime = latest->publishTime;
    dst->sequenceNumber = latest->sequenceNumber;
    
    if (latest->notificationDataSize == 0) return;
    
    dst->notificationData = (UA_ExtensionObject *) UA_malloc(sizeof(UA_ExtensionObject));
    dst->notificationData->encoding = latest->notificationData->encoding;
    dst->notificationData->typeId   = latest->notificationData->typeId;
    UA_ByteString_copy((UA_String *) &(latest->notificationData->body),
                       (UA_String *) &(dst->notificationData->body));
}

UA_UInt32 Subscription_deleteUnpublishedNotification(UA_UInt32 seqNo, UA_Subscription *sub) {
    UA_unpublishedNotification *not, *nxtnot = UA_NULL;
    UA_UInt32 deletedItems = 0;
  
    for(not=sub->unpublishedNotifications.lh_first; not != NULL; not=nxtnot) {
        nxtnot = not->listEntry.le_next;
        if(not->notification->sequenceNumber != seqNo)
            continue;
        LIST_REMOVE(not, listEntry);
        if(not->notification != NULL) {
            if (not->notification->notificationData != NULL) {
                if (not->notification->notificationData->body.data != NULL)
                    UA_free(not->notification->notificationData->body.data);
                UA_free(not->notification->notificationData);
            }
            UA_free(not->notification);
        }
        
        UA_free(not);
        deletedItems++;
    }
    return deletedItems;
}


static void Subscription_timedUpdateNotificationsJob(UA_Server *server, void *data) {
    // Timed-Worker/Job Version of updateNotifications
    UA_Subscription *sub = (UA_Subscription *) data;
    UA_MonitoredItem *mon;
    
    if (!data || !server)
        return;
    
    // This is set by the Subscription_delete function to detere us from fiddling with
    // this subscription if it is being deleted (not technically thread save, but better
    // then nothing at all)
    if (sub->SubscriptionID == 0)
        return;
    
    // FIXME: This should be done by the event system
    LIST_FOREACH(mon, &sub->MonitoredItems, listEntry)
        MonitoredItem_QueuePushDataValue(server, mon);
    
    Subscription_updateNotifications(sub);
    return;
}


UA_StatusCode Subscription_createdUpdateJob(UA_Server *server, UA_Guid jobId, UA_Subscription *sub) {
    if (server == UA_NULL || sub == UA_NULL)
        return UA_STATUSCODE_BADSERVERINDEXINVALID;
        
    UA_Job *theWork;
    theWork = (UA_Job *) malloc(sizeof(UA_Job));
    if (!theWork)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    
   /*
    *   UA_Job theWork = {.type = UA_JOBTYPE_METHODCALL, 
    *                     .job.methodCall = { .method = Subscription_timedUpdateNotificationsJob, .data = sub } };
    */
   *theWork = (UA_Job) {  .type = UA_JOBTYPE_METHODCALL,
                          .job.methodCall = {.method = Subscription_timedUpdateNotificationsJob, .data = sub} };
   
   sub->timedUpdateJobGuid = jobId;
   sub->timedUpdateJob     = theWork;
   
   return UA_STATUSCODE_GOOD;
}

UA_StatusCode Subscription_registerUpdateJob(UA_Server *server, UA_Subscription *sub) {
    UA_Int32 retval = UA_STATUSCODE_GOOD;
    if (server == UA_NULL || sub == UA_NULL)
        return UA_STATUSCODE_BADSERVERINDEXINVALID;
    
    if (sub->PublishingInterval <= 5 ) 
        return UA_STATUSCODE_BADNOTSUPPORTED;
    
    // Practically enough, the client sends a uint32 in ms, which we store as datetime, which here is required in as uint32 in ms as the interval
#ifdef _MSC_VER
    retval |= UA_Server_addRepeatedJob(server, *(sub->timedUpdateJob), (UA_UInt32) (sub->PublishingInterval), &(sub->timedUpdateJobGuid));
#else
    retval |= UA_Server_addRepeatedJob(server, (UA_Job)*(sub->timedUpdateJob), (UA_UInt32)(sub->PublishingInterval), &(sub->timedUpdateJobGuid));
#endif
    if (!retval) {
        sub->timedUpdateIsRegistered = UA_TRUE;
    }
    return retval;
}

UA_StatusCode Subscription_unregisterUpdateJob(UA_Server *server, UA_Subscription *sub) {
    UA_Int32 retval = UA_STATUSCODE_GOOD;
    if (server == UA_NULL || sub == UA_NULL)
        return UA_STATUSCODE_BADSERVERINDEXINVALID;
    
    retval |= UA_Server_removeRepeatedJob(server, sub->timedUpdateJobGuid);
    sub->timedUpdateIsRegistered = UA_FALSE;
    return retval;
}


/*****************/
/* MonitoredItem */
/*****************/

UA_MonitoredItem *UA_MonitoredItem_new() {
    UA_MonitoredItem *new = (UA_MonitoredItem *) UA_malloc(sizeof(UA_MonitoredItem));
    new->QueueSize   = (UA_UInt32_BoundedValue) { .minValue = 0, .maxValue = 0, .currentValue = 0};
    new->LastSampled = 0;
    // FIXME: This is currently hardcoded;
    new->MonitoredItemType = MONITOREDITEM_TYPE_CHANGENOTIFY;
    TAILQ_INIT(&new->queue);
    UA_NodeId_init(&new->monitoredNodeId);
    INITPOINTER(new->LastSampledValue.data );
    return new;
}

void MonitoredItem_delete(UA_MonitoredItem *monitoredItem) {
    // Delete Queued Data
    MonitoredItem_ClearQueue(monitoredItem);
    // Remove from subscription list
    LIST_REMOVE(monitoredItem, listEntry);
    // Release comparison sample
    if(monitoredItem->LastSampledValue.data != NULL) { 
      UA_free(monitoredItem->LastSampledValue.data);
    }
    
    UA_NodeId_deleteMembers(&(monitoredItem->monitoredNodeId));
    UA_free(monitoredItem);
}

int MonitoredItem_QueueToDataChangeNotifications(UA_MonitoredItemNotification *dst,
                                                 UA_MonitoredItem *monitoredItem) {
    int queueSize = 0;
    MonitoredItem_queuedValue *queueItem;
  
    // Count instead of relying on the items currentValue
    TAILQ_FOREACH(queueItem, &monitoredItem->queue, listEntry) {
        dst[queueSize].clientHandle = monitoredItem->ClientHandle;
        dst[queueSize].value.hasServerPicoseconds = UA_FALSE;
        dst[queueSize].value.hasServerTimestamp   = UA_FALSE;
        dst[queueSize].value.serverTimestamp      = UA_FALSE;
        dst[queueSize].value.hasSourcePicoseconds = UA_FALSE;
        dst[queueSize].value.hasSourceTimestamp   = UA_FALSE;
        dst[queueSize].value.hasValue             = UA_TRUE;
        dst[queueSize].value.status = UA_STATUSCODE_GOOD;
    
        UA_Variant_copy(&(queueItem->value), &(dst[queueSize].value.value));
    
        // Do not create variants with no type -> will make calcSizeBinary() segfault.
        if(dst[queueSize].value.value.type)
            queueSize++;
    }
    return queueSize;
}

void MonitoredItem_ClearQueue(UA_MonitoredItem *monitoredItem) {
    MonitoredItem_queuedValue *val;
    while((val = TAILQ_FIRST(&monitoredItem->queue))) {
        TAILQ_REMOVE(&monitoredItem->queue, val, listEntry);
        UA_Variant_deleteMembers(&val->value);
        UA_free(val);
    }
    monitoredItem->QueueSize.currentValue = 0;
}

UA_Boolean MonitoredItem_CopyMonitoredValueToVariant(UA_UInt32 AttributeID, const UA_Node *src,
                                                     UA_Variant *dst) {
    UA_Boolean samplingError = UA_TRUE; 
    UA_DataValue sourceDataValue;
    UA_DataValue_init(&sourceDataValue);
    const UA_VariableNode *srcAsVariableNode = (const UA_VariableNode *) src;
  
    // FIXME: Not all AttributeIDs can be monitored yet
    switch(AttributeID) {
    case UA_ATTRIBUTEID_NODEID:
        UA_Variant_setScalarCopy(dst, (const UA_NodeId *) &(src->nodeId), &UA_TYPES[UA_TYPES_NODEID]);
        samplingError = UA_FALSE;
        break;
    case UA_ATTRIBUTEID_NODECLASS:
        UA_Variant_setScalarCopy(dst, (const UA_Int32 *) &(src->nodeClass), &UA_TYPES[UA_TYPES_INT32]);
        samplingError = UA_FALSE;
        break;
    case UA_ATTRIBUTEID_BROWSENAME:
        UA_Variant_setScalarCopy(dst, (const UA_String *) &(src->browseName), &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        samplingError = UA_FALSE;
        break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
        UA_Variant_setScalarCopy(dst, (const UA_String *) &(src->displayName), &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        samplingError = UA_FALSE;
        break;
    case UA_ATTRIBUTEID_DESCRIPTION:
        UA_Variant_setScalarCopy(dst, (const UA_String *) &(src->displayName), &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        samplingError = UA_FALSE;
        break;
    case UA_ATTRIBUTEID_WRITEMASK:
        UA_Variant_setScalarCopy(dst, (const UA_String *) &(src->writeMask), &UA_TYPES[UA_TYPES_UINT32]);
        samplingError = UA_FALSE;
        break;
    case UA_ATTRIBUTEID_USERWRITEMASK:
        UA_Variant_setScalarCopy(dst, (const UA_String *) &(src->writeMask), &UA_TYPES[UA_TYPES_UINT32]);
        samplingError = UA_FALSE;
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
        if (src->nodeClass == UA_NODECLASS_VARIABLE) {
            if ( srcAsVariableNode->valueSource == UA_VALUESOURCE_VARIANT) {
                UA_Variant_copy( (const UA_Variant *) &((const UA_VariableNode *) src)->value, dst);
                samplingError = UA_FALSE;
            }
            else if (srcAsVariableNode->valueSource == UA_VALUESOURCE_DATASOURCE) {
                // todo: handle numeric ranges
                if(srcAsVariableNode->value.dataSource.read(((const UA_VariableNode *) src)->value.dataSource.handle,
                                                            (UA_Boolean) UA_TRUE, UA_NULL, &sourceDataValue)
                   == UA_STATUSCODE_GOOD) {
                    UA_Variant_copy( (const UA_Variant *) &(sourceDataValue.value), dst);
                    if (sourceDataValue.value.data != NULL) {
                        UA_deleteMembers(sourceDataValue.value.data, sourceDataValue.value.type);
                        UA_free(sourceDataValue.value.data);
                        sourceDataValue.value.data = NULL;
                    }
                    UA_DataValue_deleteMembers(&sourceDataValue);
                    samplingError = UA_FALSE;
                }
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
    MonitoredItem_queuedValue *newvalue = NULL, *queueItem = NULL;
    UA_Boolean samplingError = UA_TRUE; 
    UA_ByteString newValueAsByteString = { .length=0, .data=NULL };
    size_t encodingOffset = 0;
  
    if(!monitoredItem || monitoredItem->LastSampled + monitoredItem->SamplingInterval > UA_DateTime_now())
        return;
  
    // FIXME: Actively suppress non change value based monitoring. There should be
    // another function to handle status and events.
    if (monitoredItem->MonitoredItemType != MONITOREDITEM_TYPE_CHANGENOTIFY)
        return;

    newvalue = UA_malloc(sizeof(MonitoredItem_queuedValue));
    if(!newvalue)
        return;

    newvalue->listEntry.tqe_next = UA_NULL;
    newvalue->listEntry.tqe_prev = UA_NULL;
    UA_Variant_init(&newvalue->value);
  
    // Verify that the *Node being monitored is still valid
    // Looking up the in the nodestore is only necessary if we suspect that it is changed during writes
    // e.g. in multithreaded applications
    const UA_Node *target = UA_NodeStore_get(server->nodestore, &monitoredItem->monitoredNodeId);
    if(!target) {
        UA_free(newvalue);
        return;
    }
  
    samplingError = MonitoredItem_CopyMonitoredValueToVariant(monitoredItem->AttributeID, target,
                                                              &newvalue->value);
    UA_NodeStore_release(target);
    if(samplingError != UA_FALSE || !newvalue->value.type) {
        UA_Variant_deleteMembers(&newvalue->value);
        UA_free(newvalue);
        return;
    }
  
    if(monitoredItem->QueueSize.currentValue >= monitoredItem->QueueSize.maxValue) {
        if(monitoredItem->DiscardOldest == UA_TRUE) {
            queueItem = TAILQ_LAST(&monitoredItem->queue, QueueOfQueueDataValues);
            TAILQ_REMOVE(&monitoredItem->queue, queueItem, listEntry);
            UA_free(queueItem);
            monitoredItem->QueueSize.currentValue--;
        } else {
            // We cannot remove the oldest value and theres no queue space left. We're done here.
            UA_free(newvalue);
            return;
        }
    }
  
    // encode the data to find if its different to the previous
    newValueAsByteString.length = UA_calcSizeBinary(&newvalue->value, &UA_TYPES[UA_TYPES_VARIANT]);
    newValueAsByteString.data   = UA_malloc(newValueAsByteString.length);
    UA_encodeBinary(&newvalue->value, &UA_TYPES[UA_TYPES_VARIANT], &newValueAsByteString, &encodingOffset);
  
    if(!monitoredItem->LastSampledValue.data) { 
        monitoredItem->LastSampledValue = newValueAsByteString;
        TAILQ_INSERT_HEAD(&monitoredItem->queue, newvalue, listEntry);
        monitoredItem->QueueSize.currentValue++;
        monitoredItem->LastSampled = UA_DateTime_now();
    } else {
        if (UA_String_equal(&newValueAsByteString, &monitoredItem->LastSampledValue) == UA_TRUE) {
            UA_Variant_deleteMembers(&newvalue->value);
            UA_free(newvalue);
            UA_String_deleteMembers(&newValueAsByteString);
            return;
        } else {
            UA_ByteString_deleteMembers(&monitoredItem->LastSampledValue);
            monitoredItem->LastSampledValue = newValueAsByteString;
            TAILQ_INSERT_HEAD(&monitoredItem->queue, newvalue, listEntry);
            monitoredItem->QueueSize.currentValue++;
            monitoredItem->LastSampled = UA_DateTime_now();
        }
    }
    return;
}
