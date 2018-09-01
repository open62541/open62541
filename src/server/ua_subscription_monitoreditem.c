/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

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

UA_StatusCode
UA_MonitoredItem_ensureQueueSpace(UA_Server *server, UA_MonitoredItem *mon) {
    if(mon->queueSize - mon->eventOverflows <= mon->maxQueueSize)
        return UA_STATUSCODE_GOOD;

    /* Remove notifications until the queue size is reached */
    UA_Subscription *sub = mon->subscription;
    while(mon->queueSize - mon->eventOverflows > mon->maxQueueSize) {
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
         if(mon->monitoredItemType == UA_MONITOREDITEMTYPE_EVENTNOTIFY) {
             /* check if an overflowEvent is being deleted
              * TODO: make sure overflowEvents are never deleted */
             UA_NodeId overflowBaseId = UA_NODEID_NUMERIC(0, UA_NS0ID_EVENTQUEUEOVERFLOWEVENTTYPE);
             UA_NodeId overflowId = UA_NODEID_NUMERIC(0, UA_NS0ID_SIMPLEOVERFLOWEVENTTYPE);

             /* Check if an OverflowEvent is being deleted */
             if(del->data.event.fields.eventFieldsSize == 1 &&
                 del->data.event.fields.eventFields[0].type == &UA_TYPES[UA_TYPES_NODEID] &&
                 isNodeInTree(&server->config.nodestore,
                              (UA_NodeId*)del->data.event.fields.eventFields[0].data,
                              &overflowBaseId, &subtypeId, 1)) {
                 /* Don't do anything, since adding and removing an overflow will not change anything */
                 return UA_STATUSCODE_GOOD;
             }

             /* cause an overflowEvent */
             /* an overflowEvent does not care about event filters and as such
              * will not be "triggered" correctly. Instead, a notification will
              * be inserted into the queue which includes only the nodeId of the
              * overflowEventType. It is up to the client to check for possible
              * overflows. */
             UA_Notification *overflowNotification = (UA_Notification*)UA_malloc(sizeof(UA_Notification));
             if(!overflowNotification)
                 return UA_STATUSCODE_BADOUTOFMEMORY;

             UA_EventFieldList_init(&overflowNotification->data.event.fields);
             overflowNotification->data.event.fields.eventFields = UA_Variant_new();
             if(!overflowNotification->data.event.fields.eventFields) {
                 UA_EventFieldList_deleteMembers(&overflowNotification->data.event.fields);
                 UA_free(overflowNotification);
                 return UA_STATUSCODE_BADOUTOFMEMORY;
             }

             overflowNotification->data.event.fields.eventFieldsSize = 1;
             UA_StatusCode retval =
                 UA_Variant_setScalarCopy(overflowNotification->data.event.fields.eventFields,
                                          &overflowId, &UA_TYPES[UA_TYPES_NODEID]);
             if (retval != UA_STATUSCODE_GOOD) {
                 UA_EventFieldList_deleteMembers(&overflowNotification->data.event.fields);
                 UA_free(overflowNotification);
                 return retval;
             }

             overflowNotification->mon = mon;
             if(mon->discardOldest) {
                 TAILQ_INSERT_HEAD(&mon->queue, overflowNotification, listEntry);
                 TAILQ_INSERT_HEAD(&mon->subscription->notificationQueue,
                                   overflowNotification, globalEntry);
             } else {
                 TAILQ_INSERT_TAIL(&mon->queue, overflowNotification, listEntry);
                 TAILQ_INSERT_TAIL(&mon->subscription->notificationQueue,
                                   overflowNotification, globalEntry);
             }


             /* The amount of notifications in the subscription don't change. The specification
              * only states that the queue size in each MonitoredItem isn't affected by OverflowEvents.
              * Since they are reduced in Notification_delete the queues are increased here, so they
              * will remain the same in the end.
              */
             ++sub->notificationQueueSize;
             ++sub->eventNotifications;
         }
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
