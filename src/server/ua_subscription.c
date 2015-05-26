#ifndef ENABLE_SUBSCRIPTIONS
#define ENABLE_SUBSCRIPTIONS
#endif

#ifdef ENABLE_SUBSCRIPTIONS

#include "ua_subscription.h"
#include "ua_server_internal.h"
#include "ua_nodestore.h"
/****************/
/* Subscription */
/****************/

UA_Subscription *UA_Subscription_new(UA_Int32 SubscriptionID) {
    UA_Subscription *new = (UA_Subscription *) UA_malloc(sizeof(UA_Subscription));
    
    new->SubscriptionID = SubscriptionID;
    new->LastPublished  = 0;
    new->SequenceNumber = 0;
    LIST_INIT(&new->MonitoredItems);
    LIST_INITENTRY(new, listEntry);
    LIST_INIT(&new->unpublishedNotifications);
    return new;
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
  UA_unpublishedNotification *msg = NULL;
  
  if (subscription->KeepAliveCount.currentValue <= subscription->KeepAliveCount.minValue || subscription->KeepAliveCount.currentValue > subscription->KeepAliveCount.maxValue) {
    msg = (UA_unpublishedNotification *) UA_malloc(sizeof(UA_unpublishedNotification));
    LIST_INITENTRY(msg, listEntry);
    INITPOINTER(msg->notification);
    
    msg->notification = (UA_NotificationMessage *) UA_malloc(sizeof(UA_NotificationMessage));
    INITPOINTER(msg->notification->notificationData);
    msg->notification->sequenceNumber = (subscription->SequenceNumber)+1; // KeepAlive uses next message, but does not increment counter
    msg->notification->publishTime    = UA_DateTime_now();
    msg->notification->notificationDataSize = 0;
    
    LIST_INSERT_HEAD(&subscription->unpublishedNotifications, msg, listEntry);
    subscription->KeepAliveCount.currentValue = subscription->KeepAliveCount.maxValue;
  }
  
  return;
}

void Subscription_updateNotifications(UA_Subscription *subscription) {
    UA_MonitoredItem *mon;
    //MonitoredItem_queuedValue *queuedValue;
    UA_unpublishedNotification *msg = NULL;
    UA_UInt32 monItemsChangeT = 0, monItemsStatusT = 0, monItemsEventT = 0;
    UA_DataChangeNotification *changeNotification;
    size_t notificationOffset;
    
    if(!subscription)
        return;
    if((subscription->LastPublished + subscription->PublishingInterval) > UA_DateTime_now())
        return;
    
    // Make sure there is data to be published and establish which message types
    // will need to be generated
    LIST_FOREACH(mon, &subscription->MonitoredItems, listEntry) {
        // Check if this MonitoredItems Queue holds data and how much data is held in total
        if (!mon->queue.lh_first)
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
    LIST_INITENTRY(msg, listEntry);
    msg->notification = UA_malloc(sizeof(UA_NotificationMessage));
    INITPOINTER(msg->notification->notificationData);
    msg->notification->sequenceNumber = subscription->SequenceNumber++;
    msg->notification->publishTime    = UA_DateTime_now();
    
    // NotificationData is an array of Change, Status and Event messages, each containing the appropriate
    // list of Queued values from all monitoredItems of that type
    msg->notification->notificationDataSize = ISNOTZERO(monItemsChangeT);// + ISNOTZERO(monItemsEventT) + ISNOTZERO(monItemsStatusT);
    msg->notification->notificationData = (UA_ExtensionObject *) UA_malloc(sizeof(UA_ExtensionObject) * msg->notification->notificationDataSize);
    
    for(int notmsgn=0; notmsgn < msg->notification->notificationDataSize; notmsgn++) {
      // Set the notification message type and encoding for each of 
      //   the three possible NotificationData Types
      (msg->notification->notificationData)[notmsgn].encoding = 1; // Encoding is always binary
      (msg->notification->notificationData)[notmsgn].typeId = UA_NODEID_NUMERIC(0, 811);
      
      if(notmsgn == 0) {
	// Construct a DataChangeNotification
	changeNotification = (UA_DataChangeNotification *) UA_malloc(sizeof(UA_DataChangeNotification));
	
	// Create one DataChangeNotification for each queue item held in each monitoredItems queue:
	changeNotification->monitoredItems      = (UA_MonitoredItemNotification *) UA_malloc(sizeof(UA_MonitoredItemNotification) * monItemsChangeT);
	
        // Scan all monitoredItems in this subscription and have their queue transformed into an Array of
        // the propper NotificationMessageType (Status, Change, Event)
	monItemsChangeT = 0;
	for(mon=subscription->MonitoredItems.lh_first; mon != NULL; mon=mon->listEntry.le_next) {
	  if (mon->MonitoredItemType != MONITOREDITEM_TYPE_CHANGENOTIFY || mon->queue.lh_first == NULL ) continue;
	  // Note: Monitored Items might not return a queuedValue if there is a problem encoding it.
          monItemsChangeT += MonitoredItem_QueueToDataChangeNotifications( &((changeNotification->monitoredItems)[monItemsChangeT]), mon);
          MonitoredItem_ClearQueue(mon);
	}
	changeNotification->monitoredItemsSize  = monItemsChangeT;
        changeNotification->diagnosticInfosSize = 0;
        changeNotification->diagnosticInfos     = NULL;
        
	(msg->notification->notificationData[notmsgn]).body.length = UA_calcSizeBinary(changeNotification, &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION]);
        (msg->notification->notificationData[notmsgn]).body.data   =  UA_malloc((msg->notification->notificationData[notmsgn]).body.length);
        
        notificationOffset = 0;
        UA_encodeBinary((const void *) changeNotification, &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION], &(msg->notification->notificationData[notmsgn].body), &notificationOffset);
	
	// FIXME: Not properly freed!
	for(unsigned int i=0; i<monItemsChangeT; i++) {
	  UA_MonitoredItemNotification *thisNotification = &(changeNotification->monitoredItems[i]);
	  UA_DataValue_deleteMembers(&(thisNotification->value));
	}
        UA_free(changeNotification->monitoredItems);
        UA_free(changeNotification);
      }
      else if (notmsgn == 1) {
	// FIXME: Constructing a StatusChangeNotification is not implemented
      }
      else if (notmsgn == 2) {
	// FIXME: Constructing a EventListNotification is not implemented
      }
    }
    LIST_INSERT_HEAD(&subscription->unpublishedNotifications, msg, listEntry);
}

UA_UInt32 *Subscription_getAvailableSequenceNumbers(UA_Subscription *sub) {
  UA_UInt32 *seqArray;
  int i;
  UA_unpublishedNotification *not;
  
  if(!sub)
      return NULL;
  
  seqArray = (UA_UInt32 *) UA_malloc(sizeof(UA_UInt32) * Subscription_queuedNotifications(sub));
  if (seqArray == NULL ) return NULL;
  
  i = 0;
  for(not = sub->unpublishedNotifications.lh_first; not != NULL; not=(not->listEntry).le_next) {
    seqArray[i] = not->notification->sequenceNumber;
    i++;
  }
  return seqArray;
}

void Subscription_copyTopNotificationMessage(UA_NotificationMessage *dst, UA_Subscription *sub) {
    UA_NotificationMessage *latest;
    
    if (dst == NULL) return;
    
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
    UA_unpublishedNotification *not;
    UA_UInt32 deletedItems = 0;
  
    for(not=sub->unpublishedNotifications.lh_first; not != NULL; not=not->listEntry.le_next) {
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


/*****************/
/* MonitoredItem */
/*****************/

UA_MonitoredItem *UA_MonitoredItem_new() {
    UA_MonitoredItem *new = (UA_MonitoredItem *) UA_malloc(sizeof(UA_MonitoredItem));
    new->QueueSize   = (UA_UInt32_BoundedValue) { .minValue = 0, .maxValue = 0, .currentValue = 0};
    new->LastSampled = 0;
    
    // FIXME: This is currently hardcoded;
    new->MonitoredItemType = MONITOREDITEM_TYPE_CHANGENOTIFY;
    
    LIST_INIT(&new->queue);
    LIST_INITENTRY(new, listEntry);
    INITPOINTER(new->monitoredNode);
    INITPOINTER(new->LastSampledValue.data );
    return new;
}

void MonitoredItem_delete(UA_MonitoredItem *monitoredItem) {
    if (monitoredItem == NULL) return;
    
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
  LIST_FOREACH(queueItem, &monitoredItem->queue, listEntry) {
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
    if (monitoredItem == NULL)
        return;
    while(monitoredItem->queue.lh_first) {
        val = monitoredItem->queue.lh_first;
        LIST_REMOVE(val, listEntry);
	if(val->value.data != NULL) {
	  UA_Variant_deleteMembers(&(val->value));
	  UA_free(val->value.data);
	}
	UA_free(val);
    }
    monitoredItem->QueueSize.currentValue = 0;
}

UA_Boolean MonitoredItem_CopyMonitoredValueToVariant(UA_UInt32 AttributeID, const UA_Node *src,
                                                     UA_Variant *dst) {
  UA_Boolean samplingError = UA_TRUE; 
  UA_DataValue sourceDataValue;
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
          if (srcAsVariableNode->value.dataSource.read(((const UA_VariableNode *) src)->value.dataSource.handle, (UA_Boolean) UA_TRUE, UA_NULL, &sourceDataValue) == UA_STATUSCODE_GOOD) {
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
  
  // Verify that the *Node being monitored is still valid
  // Looking up the in the nodestore is only necessary if we suspect that it is changed during writes
  // e.g. in multithreaded applications
#ifdef MONITOREDITEM_FORCE_NODEPOINTER_VERIFY
  const UA_Node *target;
  target = UA_NodeStore_get((const UA_NodeStore *) (server->nodestore), (const UA_NodeId *) &( monitoredItem->monitoredNodeId));
  if (target == NULL)
    return;
  if (target != monitoredItem->monitoredNode)
    monitoredItem->monitoredNode = target;
#endif
  
  newvalue = (MonitoredItem_queuedValue *) UA_malloc(sizeof(MonitoredItem_queuedValue));
  LIST_INITENTRY(newvalue,listEntry);
  newvalue->value.arrayLength         = 0;
  newvalue->value.arrayDimensionsSize = 0;
  newvalue->value.arrayDimensions     = NULL;
  newvalue->value.data                = NULL;
  newvalue->value.type                = NULL;
  
  samplingError = MonitoredItem_CopyMonitoredValueToVariant(monitoredItem->AttributeID, monitoredItem->monitoredNode, &(newvalue->value));
  
  if ((monitoredItem->QueueSize).currentValue >= (monitoredItem->QueueSize).maxValue) {
    if (newvalue->value.type != NULL && monitoredItem->DiscardOldest == UA_TRUE && monitoredItem->queue.lh_first != NULL ) {
          for(queueItem = monitoredItem->queue.lh_first; queueItem->listEntry.le_next != NULL; queueItem = queueItem->listEntry.le_next) {}

          LIST_REMOVE(queueItem, listEntry);
          UA_free(queueItem);
          (monitoredItem->QueueSize).currentValue--;
      }
      else {
          // We cannot remove the oldest value and theres no queue space left. We're done here.
          UA_free(newvalue);
          return;
      }
  }
  
  // Only add a value if we have sampled it correctly and it fits into the queue;
  if ( samplingError != UA_FALSE || newvalue->value.type == NULL || (monitoredItem->QueueSize).currentValue >= (monitoredItem->QueueSize).maxValue) {
    if (newvalue->value.data != NULL ) {
      UA_Variant_deleteMembers(&(newvalue->value));
      UA_free(&(newvalue->value));
    }
    UA_free(newvalue);
    return;
  }
  
  newValueAsByteString.length = UA_calcSizeBinary((const void *) &(newvalue->value), &UA_TYPES[UA_TYPES_VARIANT]);
  newValueAsByteString.data   = UA_malloc(newValueAsByteString.length);
  UA_encodeBinary((const void *) &(newvalue->value), &UA_TYPES[UA_TYPES_VARIANT], &(newValueAsByteString), &encodingOffset );
  
  if(monitoredItem->LastSampledValue.data == NULL) { 
    UA_ByteString_copy((UA_String *) &newValueAsByteString, (UA_String *) &(monitoredItem->LastSampledValue));
    LIST_INSERT_HEAD(&monitoredItem->queue, newvalue, listEntry);
    (monitoredItem->QueueSize).currentValue++;
    monitoredItem->LastSampled = UA_DateTime_now();
    UA_free(newValueAsByteString.data);
  }
  else {
    if (UA_String_equal((UA_String *) &newValueAsByteString, (UA_String *) &(monitoredItem->LastSampledValue)) == UA_TRUE) {
      if (newvalue->value.data != NULL ) {
	UA_Variant_deleteMembers(&(newvalue->value));
	UA_free(&(newvalue->value)); // Same addr as newvalue
      }
      else {
	UA_free(newvalue); // Same address as newvalue->value
      }
      UA_free(newValueAsByteString.data);
      return;
    }
    
    UA_ByteString_copy((UA_String *) &newValueAsByteString, (UA_String *) &(monitoredItem->LastSampledValue));
    LIST_INSERT_HEAD(&monitoredItem->queue, newvalue, listEntry);
    (monitoredItem->QueueSize).currentValue++;
    monitoredItem->LastSampled = UA_DateTime_now();
    UA_free(newValueAsByteString.data);
  }
  
  return;
}

#endif
