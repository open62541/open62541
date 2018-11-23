/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

/****************/
/* Notification */
/****************/

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

static const UA_NodeId overflowEventType =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_EVENTQUEUEOVERFLOWEVENTTYPE}};
static const UA_NodeId simpleOverflowEventType =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SIMPLEOVERFLOWEVENTTYPE}};

static UA_Boolean
UA_Notification_isOverflowEvent(UA_Server *server, UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;
    if(mon->monitoredItemType != UA_MONITOREDITEMTYPE_EVENTNOTIFY)
        return false;

    UA_EventFieldList *efl = &n->data.event.fields;
    if(efl->eventFieldsSize == 1 &&
       efl->eventFields[0].type == &UA_TYPES[UA_TYPES_NODEID] &&
       isNodeInTree(&server->config.nodestore,
                    (const UA_NodeId *)efl->eventFields[0].data,
                    &overflowEventType, &subtypeId, 1)) {
        return true;
    }

    return false;
}

static UA_Notification *
createEventOverflowNotification(UA_MonitoredItem *mon) {
    UA_Notification *overflowNotification = (UA_Notification *) UA_malloc(sizeof(UA_Notification));
    if(!overflowNotification)
        return NULL;

    overflowNotification->mon = mon;
    UA_EventFieldList_init(&overflowNotification->data.event.fields);
    overflowNotification->data.event.fields.eventFields = UA_Variant_new();
    if(!overflowNotification->data.event.fields.eventFields) {
        UA_EventFieldList_deleteMembers(&overflowNotification->data.event.fields);
        UA_free(overflowNotification);
        return NULL;
    }

    overflowNotification->data.event.fields.eventFieldsSize = 1;
    UA_StatusCode retval =
        UA_Variant_setScalarCopy(overflowNotification->data.event.fields.eventFields,
                                 &simpleOverflowEventType, &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_EventFieldList_deleteMembers(&overflowNotification->data.event.fields);
        UA_free(overflowNotification);
        return NULL;
    }

    return overflowNotification;
}

#endif

void
UA_Notification_enqueue(UA_Server *server, UA_Subscription *sub,
                        UA_MonitoredItem *mon, UA_Notification *n) {
    /* Add to the MonitoredItem */
    TAILQ_INSERT_TAIL(&mon->queue, n, listEntry);
    ++mon->queueSize;

    /* Add to the subscription */
    TAILQ_INSERT_TAIL(&sub->notificationQueue, n, globalEntry);
    ++sub->notificationQueueSize;

    if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        ++sub->dataChangeNotifications;
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    } else if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_EVENTNOTIFY) {
        ++sub->eventNotifications;
        if(UA_Notification_isOverflowEvent(server, n))
            ++mon->eventOverflows;
#endif
    } else if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_STATUSNOTIFY) {
        ++sub->statusChangeNotifications;
    }

    /* Ensure enough space is available in the MonitoredItem. Do this only after
     * adding the new Notification. */
    UA_MonitoredItem_ensureQueueSpace(server, mon);
}

void
UA_Notification_dequeue(UA_Server *server, UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;
    UA_Subscription *sub = mon->subscription;

    if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        --sub->dataChangeNotifications;
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    } else if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_EVENTNOTIFY) {
        --sub->eventNotifications;
        if(UA_Notification_isOverflowEvent(server, n))
            --mon->eventOverflows;
#endif
    } else if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_STATUSNOTIFY) {
        --sub->statusChangeNotifications;
    }

    TAILQ_REMOVE(&mon->queue, n, listEntry);
    --mon->queueSize;

    TAILQ_REMOVE(&sub->notificationQueue, n, globalEntry);
    --sub->notificationQueueSize;
}

void
UA_Notification_delete(UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;

    if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        UA_DataValue_deleteMembers(&n->data.value);
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    } else if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_EVENTNOTIFY) {
        UA_EventFieldList_deleteMembers(&n->data.event.fields);
        /* EventFilterResult currently isn't being used
         * UA_EventFilterResult_delete(notification->data.event->result); */
#endif
    } else if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_STATUSNOTIFY) {
        /* Nothing to do */
    }

    UA_free(n);
}

/*****************/
/* MonitoredItem */
/*****************/

void
UA_MonitoredItem_init(UA_MonitoredItem *mon, UA_Subscription *sub) {
    memset(mon, 0, sizeof(UA_MonitoredItem));
    mon->subscription = sub;
    TAILQ_INIT(&mon->queue);
}

void
UA_MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    if(monitoredItem->monitoredItemType == UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        /* Remove the sampling callback */
        UA_MonitoredItem_unregisterSampleCallback(server, monitoredItem);
    } else if (monitoredItem->monitoredItemType != UA_MONITOREDITEMTYPE_EVENTNOTIFY) {
        /* TODO: Access val data.event */
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "MonitoredItemTypes other than ChangeNotify or EventNotify "
                     "are not supported yet");
    }

    /* Remove the queued notifications if attached to a subscription (not a
     * local MonitoredItem) */
    if(monitoredItem->subscription) {
        UA_Notification *notification, *notification_tmp;
        TAILQ_FOREACH_SAFE(notification, &monitoredItem->queue,
                           listEntry, notification_tmp) {
            /* Remove the item from the queues and free the memory */
            UA_Notification_dequeue(server, notification);
            UA_Notification_delete(notification);
        }
    }

    /* if(monitoredItem->monitoredItemType == UA_MONITOREDITEMTYPE_CHANGENOTIFY)
     * -> UA_DataChangeFilter does not hold dynamic content we need to free */

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    if(monitoredItem->monitoredItemType == UA_MONITOREDITEMTYPE_EVENTNOTIFY) {
        /* Remove the monitored item from the node queue */
        UA_Server_editNode(server, NULL, &monitoredItem->monitoredNodeId,
                           UA_MonitoredItem_removeNodeEventCallback, monitoredItem);
        /* Delete the event filter */
        UA_EventFilter_deleteMembers(&monitoredItem->filter.eventFilter);
    }
#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

    /* Deregister MonitoredItem in userland */
    if(server->config.monitoredItemRegisterCallback && monitoredItem->registered) {
        /* Get the session context. Local MonitoredItems don't have a subscription. */
        UA_Session *session = NULL;
        if(monitoredItem->subscription)
            session = monitoredItem->subscription->session;
        if(!session)
            session = &server->adminSession;

        /* Get the node context */
        void *targetContext = NULL;
        UA_Server_getNodeContext(server, monitoredItem->monitoredNodeId, &targetContext);

        /* Deregister */
        server->config.monitoredItemRegisterCallback(server, &session->sessionId,
                                                     session->sessionHandle, &monitoredItem->monitoredNodeId,
                                                     targetContext, monitoredItem->attributeId, true);
    }

    /* Remove the monitored item */
    if(monitoredItem->listEntry.le_prev != NULL)
        LIST_REMOVE(monitoredItem, listEntry);
    UA_String_deleteMembers(&monitoredItem->indexRange);
    UA_ByteString_deleteMembers(&monitoredItem->lastSampledValue);
    UA_Variant_deleteMembers(&monitoredItem->lastValue);
    UA_NodeId_deleteMembers(&monitoredItem->monitoredNodeId);

    /* No actual callback, just remove the structure */
    monitoredItem->delayedFreePointers.callback = NULL;
    UA_WorkQueue_enqueueDelayed(&server->workQueue, &monitoredItem->delayedFreePointers);
}

#ifdef __clang_analyzer__
# define UA_CA_assert(clause) UA_assert(clause)
#else
# define UA_CA_assert(clause)
#endif

UA_StatusCode
UA_MonitoredItem_ensureQueueSpace(UA_Server *server, UA_MonitoredItem *mon) {
    if(mon->queueSize - mon->eventOverflows <= mon->maxQueueSize)
        return UA_STATUSCODE_GOOD;

    /* Remove notifications until the queue size is reached */
    UA_Subscription *sub = mon->subscription;
#ifdef __clang_analyzer__
    UA_Notification *last = NULL;
#endif

    while(mon->queueSize - mon->eventOverflows > mon->maxQueueSize) {
        /* At least two notifications that are not eventOverflows in the queue */
        UA_assert(mon->queueSize - mon->eventOverflows >= 2);

        /* Select the next notification to delete. Skip over overflow events. */
        UA_Notification *del;
        if(mon->discardOldest) {
            /* Remove the oldest */
            del = TAILQ_FIRST(&mon->queue);
            UA_CA_assert(del != last);
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
            while(UA_Notification_isOverflowEvent(server, del)) {
                del = TAILQ_NEXT(del, listEntry); /* skip overflow events */
                UA_CA_assert(del != last);
            }
#endif
        } else {
            /* Remove the second newest (to keep the up-to-date notification) */
            del = TAILQ_LAST(&mon->queue, NotificationQueue);
            del = TAILQ_PREV(del, NotificationQueue, listEntry);
            UA_CA_assert(del != last);
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
            while(UA_Notification_isOverflowEvent(server, del)) {
                del = TAILQ_PREV(del, NotificationQueue, listEntry); /* skip overflow events */
                UA_CA_assert(del != last);
            }
#endif
        }

        UA_assert(del && del->mon == mon);

        /* Move after_del right after del in the global queue. (It is already
         * right after del in the per-MonitoredItem queue.) This is required so
         * we don't starve MonitoredItems with a high sampling interval by
         * always removing their first appearance in the gloal queue for the
         * Subscription. */
        UA_Notification *after_del = TAILQ_NEXT(del, listEntry);
        UA_CA_assert(after_del != last);
        if(after_del) {
            TAILQ_REMOVE(&sub->notificationQueue, after_del, globalEntry);
            TAILQ_INSERT_AFTER(&sub->notificationQueue, del, after_del, globalEntry);
        }

#ifdef __clang_analyzer__
        last = del;
#endif

        /* Delete the notification */
        UA_Notification_dequeue(server, del);
        UA_Notification_delete(del);
    }

    /* Get the element where the overflow shall be announced (infobits or
     * overflowevent) */
    UA_Notification *indicator;
    if(mon->discardOldest)
        indicator = TAILQ_FIRST(&mon->queue);
    else
        indicator = TAILQ_LAST(&mon->queue, NotificationQueue);
    UA_assert(indicator);
    UA_CA_assert(indicator != last);

    /* Create an overflow notification */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    /* The specification states in Part 4 5.12.1.5 that an EventQueueOverflowEvent
     * "is generated when the first Event has to be discarded [...] without discarding
     * any other event". So only generate one for all deleted events. */
    if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_EVENTNOTIFY) {
        /* Avoid two redundant overflow events in a row */
        if(UA_Notification_isOverflowEvent(server, indicator)) {
            if(mon->discardOldest)
                return UA_STATUSCODE_GOOD;
            UA_Notification *prev = TAILQ_PREV(indicator, NotificationQueue, listEntry);
            UA_CA_assert(prev != last);
            if(prev && UA_Notification_isOverflowEvent(server, prev))
                return UA_STATUSCODE_GOOD;
        }

        /* A notification is inserted into the queue which includes only the
         * NodeId of the overflowEventType. It is up to the client to check for
         * possible overflows. */
        UA_Notification *overflowNotification = createEventOverflowNotification(mon);
        if(!overflowNotification)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        /* Insert before the "indicator notification". This is either first in
         * the queue (if the oldest notification was removed) or before the new
         * event that remains the last element of the queue. */
        TAILQ_INSERT_BEFORE(indicator, overflowNotification, listEntry);
        TAILQ_INSERT_BEFORE(indicator, overflowNotification, globalEntry);
        ++mon->eventOverflows;
        ++mon->queueSize;
        ++sub->notificationQueueSize;
        ++sub->eventNotifications;
    }
#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

    /* Set the infobits of a datachange notification */
    if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        /* Set the infobits */
        if(mon->maxQueueSize > 1) {
            /* Add the infobits either to the newest or the new last entry */
            indicator->data.value.hasStatus = true;
            indicator->data.value.status |= (UA_STATUSCODE_INFOTYPE_DATAVALUE |
                                             UA_STATUSCODE_INFOBITS_OVERFLOW);
        } else {
            /* If the queue size is reduced to one, remove the infobits */
            indicator->data.value.status &= ~(UA_StatusCode)(UA_STATUSCODE_INFOTYPE_DATAVALUE |
                                                             UA_STATUSCODE_INFOBITS_OVERFLOW);
        }
    }

    return UA_STATUSCODE_GOOD;
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
                                      mon, mon->samplingInterval, &mon->sampleCallbackId);
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
