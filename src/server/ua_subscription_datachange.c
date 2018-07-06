/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"
#include "ua_types_encoding_binary.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

#define UA_VALUENCODING_MAXSTACK 512

void UA_MonitoredItem_init(UA_MonitoredItem *mon, UA_Subscription *sub) {
    memset(mon, 0, sizeof(UA_MonitoredItem));
    mon->subscription = sub;
    TAILQ_INIT(&mon->queue);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
static UA_StatusCode
removeMonitoredItemFromNodeCallback(UA_Server *server, UA_Session *session,
                                    UA_Node *node, void *data) {
    /* data is the monitoredItemID */
    /* catch edge case that it's the first element */
    if (data == ((UA_ObjectNode *) node)->monitoredItemQueue) {
        ((UA_ObjectNode *)node)->monitoredItemQueue = ((UA_MonitoredItem *)data)->next;
        return UA_STATUSCODE_GOOD;
    }

    /* SLIST_FOREACH */
    for (UA_MonitoredItem *entry = ((UA_ObjectNode *) node)->monitoredItemQueue->next;
         entry != NULL; entry=entry->next) {
        if (entry == (UA_MonitoredItem *)data) {
            /* SLIST_REMOVE */
            UA_MonitoredItem *iter = ((UA_ObjectNode *) node)->monitoredItemQueue;
            for (; iter->next != entry; iter=iter->next) {}
            iter->next = entry->next;
            UA_free(entry);
            break;
        }
    }
    return UA_STATUSCODE_GOOD;
}
#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

void UA_MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    if(monitoredItem->monitoredItemType == UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        /* Remove the sampling callback */
        UA_MonitoredItem_unregisterSampleCallback(server, monitoredItem);
    } else if (monitoredItem->monitoredItemType != UA_MONITOREDITEMTYPE_EVENTNOTIFY) {
        /* TODO: Access val data.event */
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "MonitoredItemTypes other than ChangeNotify or EventNotify are not supported yet");
    }

    /* Remove the queued notifications if attached to a subscription */
    if(monitoredItem->subscription) {
        UA_Subscription *sub = monitoredItem->subscription;
        UA_Notification *notification, *notification_tmp;
        TAILQ_FOREACH_SAFE(notification, &monitoredItem->queue,
                           listEntry, notification_tmp) {
            /* Remove the item from the queues and free the memory */
            UA_Notification_delete(sub, monitoredItem, notification);
        }
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    if(monitoredItem->monitoredItemType == UA_MONITOREDITEMTYPE_EVENTNOTIFY) {
        /* Remove the monitored item from the node queue */
        UA_Server_editNode(server, NULL, &monitoredItem->monitoredNodeId,
                           removeMonitoredItemFromNodeCallback, monitoredItem);
        /* Delete the event filter */
        UA_EventFilter_deleteMembers(&monitoredItem->filter.eventFilter);
    }
#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

    /* Remove the monitored item */
    if(monitoredItem->listEntry.le_prev != NULL)
        LIST_REMOVE(monitoredItem, listEntry);
    UA_String_deleteMembers(&monitoredItem->indexRange);
    UA_ByteString_deleteMembers(&monitoredItem->lastSampledValue);
    UA_Variant_deleteMembers(&monitoredItem->lastValue);
    UA_NodeId_deleteMembers(&monitoredItem->monitoredNodeId);
    UA_Server_delayedFree(server, monitoredItem);
}

UA_StatusCode
MonitoredItem_ensureQueueSpace(UA_Server *server, UA_MonitoredItem *mon) {
    if(mon->queueSize <= mon->maxQueueSize)
        return UA_STATUSCODE_GOOD;

    /* Remove notifications until the queue size is reached */
    UA_Subscription *sub = mon->subscription;
    while(mon->queueSize > mon->maxQueueSize) {
        UA_assert(mon->queueSize >= 2); /* At least two Notifications in the queue */

        /* Make sure that the MonitoredItem does not lose its place in the
         * global queue when notifications are removed. Otherwise the
         * MonitoredItem can "starve" itself by putting new notifications always
         * at the end of the global queue and removing the old ones.
         *
         * - If the oldest notification is removed, put the second oldest
         *   notification right behind it.
         * - If the newest notification is removed, put the new notification
         *   right behind it. */

        UA_Notification *del; /* The notification that will be deleted */
        UA_Notification *after_del; /* The notification to keep and move after del */
        if(mon->discardOldest) {
            /* Remove the oldest */
            del = TAILQ_FIRST(&mon->queue);
            after_del = TAILQ_NEXT(del, listEntry);
        } else {
            /* Remove the second newest (to keep the up-to-date notification) */
            after_del = TAILQ_LAST(&mon->queue, NotificationQueue);
            del = TAILQ_PREV(after_del, NotificationQueue, listEntry);
        }

        /* Move after_del right after del in the global queue */
        TAILQ_REMOVE(&sub->notificationQueue, after_del, globalEntry);
        TAILQ_INSERT_AFTER(&sub->notificationQueue, del, after_del, globalEntry);

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        /* Create an overflow notification */
        /* if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_EVENTNOTIFY) { */
        /*     /\* EventFilterResult currently isn't being used */
        /*     UA_EventFilterResult_deleteMembers(&del->data.event->result); *\/ */
        /*     UA_EventFieldList_deleteMembers(&del->data.event.fields); */

        /*     /\* cause an overflowEvent *\/ */
        /*     /\* an overflowEvent does not care about event filters and as such */
        /*      * will not be "triggered" correctly. Instead, a notification will */
        /*      * be inserted into the queue which includes only the nodeId of the */
        /*      * overflowEventType. It is up to the client to check for possible */
        /*      * overflows. *\/ */
        /*     UA_Notification *overflowNotification = (UA_Notification *) UA_malloc(sizeof(UA_Notification)); */
        /*     if(!overflowNotification) */
        /*         return UA_STATUSCODE_BADOUTOFMEMORY; */

        /*     UA_EventFieldList_init(&overflowNotification->data.event.fields); */
        /*     overflowNotification->data.event.fields.eventFields = UA_Variant_new(); */
        /*     if(!overflowNotification->data.event.fields.eventFields) { */
        /*         UA_EventFieldList_deleteMembers(&overflowNotification->data.event.fields); */
        /*         UA_free(overflowNotification); */
        /*         return UA_STATUSCODE_BADOUTOFMEMORY; */
        /*     } */

        /*     UA_Variant_init(overflowNotification->data.event.fields.eventFields); */
        /*     overflowNotification->data.event.fields.eventFieldsSize = 1; */
        /*     UA_NodeId overflowId = UA_NODEID_NUMERIC(0, UA_NS0ID_SIMPLEOVERFLOWEVENTTYPE); */
        /*     UA_Variant_setScalarCopy(overflowNotification->data.event.fields.eventFields, */
        /*                              &overflowId, &UA_TYPES[UA_TYPES_NODEID]); */
        /*     overflowNotification->mon = mon; */
        /*     if(mon->discardOldest) { */
        /*         TAILQ_INSERT_HEAD(&mon->queue, overflowNotification, listEntry); */
        /*         TAILQ_INSERT_HEAD(&mon->subscription->notificationQueue, overflowNotification, globalEntry); */
        /*     } else { */
        /*         TAILQ_INSERT_TAIL(&mon->queue, overflowNotification, listEntry); */
        /*         TAILQ_INSERT_TAIL(&mon->subscription->notificationQueue, overflowNotification, globalEntry); */
        /*     } */
        /*     ++mon->queueSize; */
        /*     ++sub->notificationQueueSize; */
        /*     ++sub->eventNotifications; */
        /* } */
#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

        /* Delete the notification. This also removes the notification from the
         * linked lists. */
        UA_Notification_delete(sub, mon, del);
    }

    if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        /* Get the element that carries the infobits */
        UA_Notification *notification = NULL;
        if(mon->discardOldest)
            notification = TAILQ_FIRST(&mon->queue);
        else
            notification = TAILQ_LAST(&mon->queue, NotificationQueue);
        UA_assert(notification);

        if(mon->maxQueueSize > 1) {
            /* Add the infobits either to the newest or the new last entry */
            notification->data.value.hasStatus = true;
            notification->data.value.status |= (UA_STATUSCODE_INFOTYPE_DATAVALUE |
                                                UA_STATUSCODE_INFOBITS_OVERFLOW);
        } else {
            /* If the queue size is reduced to one, remove the infobits */
            notification->data.value.status &= ~(UA_StatusCode)(UA_STATUSCODE_INFOTYPE_DATAVALUE |
                                                                UA_STATUSCODE_INFOBITS_OVERFLOW);
        }
    }

    /* TODO: Infobits for Events? */
    return UA_STATUSCODE_GOOD;
}

#define ABS_SUBTRACT_TYPE_INDEPENDENT(a,b) ((a)>(b)?(a)-(b):(b)-(a))

static UA_INLINE UA_Boolean
outOfDeadBand(const void *data1, const void *data2, const size_t index,
              const UA_DataType *type, const UA_Double deadbandValue) {
    if(type == &UA_TYPES[UA_TYPES_SBYTE]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_SByte*)data1)[index],
                                         ((const UA_SByte*)data2)[index]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_BYTE]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Byte*)data1)[index],
                                         ((const UA_Byte*)data2)[index]) <= deadbandValue)
                return false;
    } else if(type == &UA_TYPES[UA_TYPES_INT16]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Int16*)data1)[index],
                                          ((const UA_Int16*)data2)[index]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_UINT16]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_UInt16*)data1)[index],
                                          ((const UA_UInt16*)data2)[index]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_INT32]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Int32*)data1)[index],
                                         ((const UA_Int32*)data2)[index]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_UINT32]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_UInt32*)data1)[index],
                                         ((const UA_UInt32*)data2)[index]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_INT64]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Int64*)data1)[index],
                                         ((const UA_Int64*)data2)[index]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_UINT64]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_UInt64*)data1)[index],
                                         ((const UA_UInt64*)data2)[index]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_FLOAT]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Float*)data1)[index],
                                         ((const UA_Float*)data2)[index]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_DOUBLE]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Double*)data1)[index],
                                         ((const UA_Double*)data2)[index]) <= deadbandValue)
            return false;
    }
    return true;
}

static UA_INLINE UA_Boolean
updateNeededForFilteredValue(const UA_Variant *value, const UA_Variant *oldValue,
                             const UA_Double deadbandValue) {
    if(value->arrayLength != oldValue->arrayLength)
        return true;

    if(value->type != oldValue->type)
        return true;

    if (UA_Variant_isScalar(value)) {
        return outOfDeadBand(value->data, oldValue->data, 0, value->type, deadbandValue);
    } else {
        for (size_t i = 0; i < value->arrayLength; ++i) {
            if (outOfDeadBand(value->data, oldValue->data, i, value->type, deadbandValue))
                return true;
        }
    }
    return false;
}


/* When a change is detected, encoding contains the heap-allocated binary encoded value */
static UA_Boolean
detectValueChangeWithFilter(UA_Server *server, UA_MonitoredItem *mon, UA_DataValue *value,
                            UA_ByteString *encoding) {
    UA_Session *session = &adminSession;
    UA_UInt32 subscriptionId = 0;
    UA_Subscription *sub = mon->subscription;
    if(sub) {
        session = sub->session;
        subscriptionId = sub->subscriptionId;
    }

    if(isDataTypeNumeric(value->value.type) &&
       (mon->filter.dataChangeFilter.trigger == UA_DATACHANGETRIGGER_STATUSVALUE ||
        mon->filter.dataChangeFilter.trigger == UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP)) {
        if(mon->filter.dataChangeFilter.deadbandType == UA_DEADBANDTYPE_ABSOLUTE) {
            if(!updateNeededForFilteredValue(&value->value, &mon->lastValue,
                                             mon->filter.dataChangeFilter.deadbandValue))
                return false;
        }
        /* else if (mon->filter.deadbandType == UA_DEADBANDTYPE_PERCENT) {
            // TODO where do this EURange come from ?
            UA_Double deadbandValue = fabs(mon->filter.deadbandValue * (EURange.high-EURange.low));
            if (!updateNeededForFilteredValue(value->value, mon->lastValue, deadbandValue))
                return false;
        }*/
    }

    /* Stack-allocate some memory for the value encoding. We might heap-allocate
     * more memory if needed. This is just enough for scalars and small
     * structures. */
    UA_STACKARRAY(UA_Byte, stackValueEncoding, UA_VALUENCODING_MAXSTACK);
    UA_ByteString valueEncoding;
    valueEncoding.data = stackValueEncoding;
    valueEncoding.length = UA_VALUENCODING_MAXSTACK;

    /* Encode the value */
    UA_Byte *bufPos = valueEncoding.data;
    const UA_Byte *bufEnd = &valueEncoding.data[valueEncoding.length];
    UA_StatusCode retval = UA_encodeBinary(value, &UA_TYPES[UA_TYPES_DATAVALUE],
                                           &bufPos, &bufEnd, NULL, NULL);
    if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
        size_t binsize = UA_calcSizeBinary(value, &UA_TYPES[UA_TYPES_DATAVALUE]);
        if(binsize == 0)
            return false;
        retval = UA_ByteString_allocBuffer(&valueEncoding, binsize);
        if(retval == UA_STATUSCODE_GOOD) {
            bufPos = valueEncoding.data;
            bufEnd = &valueEncoding.data[valueEncoding.length];
            retval = UA_encodeBinary(value, &UA_TYPES[UA_TYPES_DATAVALUE],
                                     &bufPos, &bufEnd, NULL, NULL);
        }
    }

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(server->config.logger, session,
                               "Subscription %u | MonitoredItem %i | "
                               "Could not encode the value the MonitoredItem with status %s",
                               subscriptionId, mon->monitoredItemId, UA_StatusCode_name(retval));
        return false;
    }

    /* Has the value changed? */
    valueEncoding.length = (uintptr_t)bufPos - (uintptr_t)valueEncoding.data;
    UA_Boolean changed = (!mon->lastSampledValue.data ||
                          !UA_String_equal(&valueEncoding, &mon->lastSampledValue));

    /* No change */
    if(!changed) {
        if(valueEncoding.data != stackValueEncoding)
            UA_ByteString_deleteMembers(&valueEncoding);
        return false;
    }

    /* Change detected. Copy encoding on the heap if necessary. */
    if(valueEncoding.data == stackValueEncoding) {
        retval = UA_ByteString_copy(&valueEncoding, encoding);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SESSION(server->config.logger, session,
                                   "Subscription %u | MonitoredItem %i | "
                                   "Detected change, but could not allocate memory for the notification"
                                   "with status %s", subscriptionId, mon->monitoredItemId,
                                   UA_StatusCode_name(retval));
            return false;
        }
        return true;
    }

    *encoding = valueEncoding;
    return true;
}

/* Has this sample changed from the last one? The method may allocate additional
 * space for the encoding buffer. Detect the change in encoding->data. */
static UA_Boolean
detectValueChange(UA_Server *server, UA_MonitoredItem *mon,
                  UA_DataValue value, UA_ByteString *encoding) {
    /* Apply Filter */
    if(mon->filter.dataChangeFilter.trigger == UA_DATACHANGETRIGGER_STATUS)
        value.hasValue = false;

    value.hasServerTimestamp = false;
    value.hasServerPicoseconds = false;
    if(mon->filter.dataChangeFilter.trigger < UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP) {
        value.hasSourceTimestamp = false;
        value.hasSourcePicoseconds = false;
    }

    /* Detect the value change */
    return detectValueChangeWithFilter(server, mon, &value, encoding);
}

/* Returns whether the sample was stored in the MonitoredItem */
static UA_Boolean
sampleCallbackWithValue(UA_Server *server, UA_MonitoredItem *monitoredItem,
                        UA_DataValue *value) {
    UA_assert(monitoredItem->monitoredItemType == UA_MONITOREDITEMTYPE_CHANGENOTIFY);
    UA_Subscription *sub = monitoredItem->subscription;

    /* Contains heap-allocated binary encoding of the value if a change was detected */
    UA_ByteString binaryEncoding = UA_BYTESTRING_NULL;

    /* Has the value changed? Allocates memory in binaryEncoding if necessary.
     * value is edited internally so we make a shallow copy. */
    UA_Boolean changed = detectValueChange(server, monitoredItem, *value, &binaryEncoding);
    if(!changed)
        return false;

    UA_Boolean storedValue = false;
    if(sub) {
        /* Allocate a new notification */
        UA_Notification *newNotification = (UA_Notification *)UA_malloc(sizeof(UA_Notification));
        if(!newNotification) {
            UA_LOG_WARNING_SESSION(server->config.logger, sub->session,
                                   "Subscription %u | MonitoredItem %i | "
                                   "Item for the publishing queue could not be allocated",
                                   sub->subscriptionId, monitoredItem->monitoredItemId);
            UA_ByteString_deleteMembers(&binaryEncoding);
            return false;
        }

        /* <-- Point of no return --> */

        newNotification->mon = monitoredItem;
        newNotification->data.value = *value; /* Move the value to the notification */
        storedValue = true;

        /* Enqueue the new notification */
        UA_Notification_enqueue(server, sub, monitoredItem, newNotification);
    } else {
        /* Call the local callback if not attached to a subscription */
        UA_LocalMonitoredItem *localMon = (UA_LocalMonitoredItem*) monitoredItem;
        void *nodeContext = NULL;
        UA_Server_getNodeContext(server, monitoredItem->monitoredNodeId, &nodeContext);
        localMon->callback.dataChangeCallback(server, monitoredItem->monitoredItemId,
                                              localMon->context,
                                              &monitoredItem->monitoredNodeId,
                                              nodeContext, monitoredItem->attributeId,
                                              value);
    }

    // If someone called UA_Server_deleteMonitoredItem in the user callback,
    // then the monitored item will be deleted soon. So, there is no need to
    // add the lastValue or lastSampledValue to it.
    //
    // If we do so, we will leak
    // the memory of that values, because UA_Server_deleteMonitoredItem
    // already deleted all members and scheduled the monitored item pointer
    // for later delete. In the later delete the monitored item will be deleted
    // and not the members.
    //
    // Also in the later delete, all type information is lost and a deleteMember
    // is not possible.
    //
    // We do detect if the monitored item is already defunct.
    if (!monitoredItem->sampleCallbackIsRegistered) {
        UA_ByteString_deleteMembers(&binaryEncoding);
        return storedValue;
    }

    /* Store the encoding for comparison */
    UA_ByteString_deleteMembers(&monitoredItem->lastSampledValue);
    monitoredItem->lastSampledValue = binaryEncoding;
    UA_Variant_deleteMembers(&monitoredItem->lastValue);
    UA_Variant_copy(&value->value, &monitoredItem->lastValue);

    return storedValue;
}

void
UA_MonitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    UA_Session *session = &adminSession;
    UA_UInt32 subscriptionId = 0;
    UA_Subscription *sub = monitoredItem->subscription;
    if(sub) {
        session = sub->session;
        subscriptionId = sub->subscriptionId;
    }

    if(monitoredItem->monitoredItemType != UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session, "Subscription %u | "
                             "MonitoredItem %i | Not a data change notification",
                             subscriptionId, monitoredItem->monitoredItemId);
        return;
    }

    /* Sample the value */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = monitoredItem->monitoredNodeId;
    rvid.attributeId = monitoredItem->attributeId;
    rvid.indexRange = monitoredItem->indexRange;
    UA_DataValue value = UA_Server_readWithSession(server, session, &rvid, monitoredItem->timestampsToReturn);

    /* Operate on the sample */
    UA_Boolean storedValue = sampleCallbackWithValue(server, monitoredItem, &value);

    /* Delete the sample if it was not stored in the MonitoredItem  */
    if(!storedValue)
        UA_DataValue_deleteMembers(&value);
}

UA_StatusCode
UA_MonitoredItem_registerSampleCallback(UA_Server *server, UA_MonitoredItem *mon) {
    if(mon->sampleCallbackIsRegistered)
        return UA_STATUSCODE_GOOD;

    /* Only DataChange MonitoredItems have a callback with a sampling interval */
    if(mon->monitoredItemType != UA_MONITOREDITEMTYPE_CHANGENOTIFY)
        return UA_STATUSCODE_GOOD;

    UA_StatusCode retval =
        UA_Server_addRepeatedCallback(server, (UA_ServerCallback)UA_MonitoredItem_sampleCallback,
                                      mon, (UA_UInt32)mon->samplingInterval, &mon->sampleCallbackId);
    if(retval == UA_STATUSCODE_GOOD)
        mon->sampleCallbackIsRegistered = true;
    return retval;
}

UA_StatusCode
UA_MonitoredItem_unregisterSampleCallback(UA_Server *server, UA_MonitoredItem *mon) {
    if(!mon->sampleCallbackIsRegistered)
        return UA_STATUSCODE_GOOD;
    mon->sampleCallbackIsRegistered = false;
    return UA_Server_removeRepeatedCallback(server, mon->sampleCallbackId);
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
