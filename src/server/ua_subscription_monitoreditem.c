/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 *    Copyright 2020-2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

/****************/
/* Notification */
/****************/

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

static const UA_NodeId eventQueueOverflowEventType =
    {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_EVENTQUEUEOVERFLOWEVENTTYPE}};

/* The specification states in Part 4 5.12.1.5 that an EventQueueOverflowEvent
 * "is generated when the first Event has to be discarded [...] without
 * discarding any other event". So only generate one for all deleted events. */
static UA_StatusCode
createEventOverflowNotification(UA_Server *server, UA_Subscription *sub,
                                UA_MonitoredItem *mon) {
    /* Avoid creating two adjacent overflow events */
    UA_Notification *indicator = NULL;
    if(mon->parameters.discardOldest) {
        indicator = TAILQ_FIRST(&mon->queue);
        UA_assert(indicator); /* must exist */
        if(indicator->isOverflowEvent)
            return UA_STATUSCODE_GOOD;
    } else {
        indicator = TAILQ_LAST(&mon->queue, NotificationQueue);
        UA_assert(indicator); /* must exist */
        /* Skip the last element. It is the recently added notification that
         * shall be kept. We know it is not an OverflowEvent. */
        UA_Notification *before = TAILQ_PREV(indicator, NotificationQueue, localEntry);
        if(before && before->isOverflowEvent)
            return UA_STATUSCODE_GOOD;
    }

    /* A Notification is inserted into the queue which includes only the
     * NodeId of the OverflowEventType. */

    /* Allocate the notification */
    UA_Notification *overflowNotification = UA_Notification_new();
    if(!overflowNotification)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set the notification fields */
    overflowNotification->isOverflowEvent = true;
    overflowNotification->mon = mon;
    overflowNotification->data.event.clientHandle = mon->parameters.clientHandle;
    overflowNotification->data.event.eventFields = UA_Variant_new();
    if(!overflowNotification->data.event.eventFields) {
        UA_free(overflowNotification);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    overflowNotification->data.event.eventFieldsSize = 1;
    UA_StatusCode retval =
        UA_Variant_setScalarCopy(overflowNotification->data.event.eventFields,
                                 &eventQueueOverflowEventType, &UA_TYPES[UA_TYPES_NODEID]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Notification_delete(overflowNotification);
        return retval;
    }

    /* Insert before the removed notification. This is either first in the
     * queue (if the oldest notification was removed) or before the new event
     * that remains the last element of the queue.
     *
     * Ensure that the following is consistent with UA_Notification_enqueueMon
     * and UA_Notification_enqueueSub! */
    TAILQ_INSERT_BEFORE(indicator, overflowNotification, localEntry);
    ++mon->eventOverflows;
    ++mon->queueSize;

    /* Test for consistency */
    UA_assert(mon->queueSize >= mon->eventOverflows);
    UA_assert(mon->eventOverflows <= mon->queueSize - mon->eventOverflows + 1);

    if(TAILQ_NEXT(indicator, globalEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL) {
        /* Insert just before the indicator */
        TAILQ_INSERT_BEFORE(indicator, overflowNotification, globalEntry);
    } else {
        /* The indicator was not reporting or not added yet. */
        if(!mon->parameters.discardOldest) {
            /* Add last to the per-Subscription queue */
            TAILQ_INSERT_TAIL(&mon->subscription->notificationQueue,
                              overflowNotification, globalEntry);
        } else {
            /* Find the oldest reported element. Add before that. */
            while(indicator) {
                indicator = TAILQ_PREV(indicator, NotificationQueue, localEntry);
                if(!indicator) {
                    TAILQ_INSERT_TAIL(&mon->subscription->notificationQueue,
                                      overflowNotification, globalEntry);
                    break;
                }
                if(TAILQ_NEXT(indicator, globalEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL) {
                    TAILQ_INSERT_BEFORE(indicator, overflowNotification, globalEntry);
                    break;
                }
            }
        }
    }

    ++sub->notificationQueueSize;
    ++sub->eventNotifications;

    /* Update the diagnostics statistics */
#ifdef UA_ENABLE_DIAGNOSTICS
    sub->eventQueueOverFlowCount++;
#endif

    return UA_STATUSCODE_GOOD;
}

#endif

/* Set the InfoBits that a datachange notification was removed */
static void
setOverflowInfoBits(UA_MonitoredItem *mon) {
    /* Only for queues with more than one element */
    if(mon->parameters.queueSize == 1)
        return;

    UA_Notification *indicator = NULL;
    if(mon->parameters.discardOldest) {
        indicator = TAILQ_FIRST(&mon->queue);
    } else {
        indicator = TAILQ_LAST(&mon->queue, NotificationQueue);
    }
    UA_assert(indicator); /* must exist */

    indicator->data.dataChange.value.hasStatus = true;
    indicator->data.dataChange.value.status |=
        (UA_STATUSCODE_INFOTYPE_DATAVALUE | UA_STATUSCODE_INFOBITS_OVERFLOW);
}

/* Remove the InfoBits when the queueSize was reduced to 1 */
void
UA_MonitoredItem_removeOverflowInfoBits(UA_MonitoredItem *mon) {
    /* Don't consider queue size > 1 and Event MonitoredItems */
    if(mon->parameters.queueSize > 1 ||
       mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER)
        return;

    /* Get the first notification */
    UA_Notification *n = TAILQ_FIRST(&mon->queue);
    if(!n)
        return;

    /* Assertion that at most one notification is in the queue */
    UA_assert(n == TAILQ_LAST(&mon->queue, NotificationQueue));

    /* Remve the Infobits */
    n->data.dataChange.value.status &= ~(UA_StatusCode)
        (UA_STATUSCODE_INFOTYPE_DATAVALUE | UA_STATUSCODE_INFOBITS_OVERFLOW);
}

UA_Notification *
UA_Notification_new(void) {
    UA_Notification *n = (UA_Notification*)UA_calloc(1, sizeof(UA_Notification));
    if(n) {
        /* Set the sentinel for a notification that is not enqueued */
        TAILQ_NEXT(n, globalEntry) = UA_SUBSCRIPTION_QUEUE_SENTINEL;
        TAILQ_NEXT(n, localEntry) = UA_SUBSCRIPTION_QUEUE_SENTINEL;
    }
    return n;
}

static void UA_Notification_dequeueMon(UA_Notification *n);
static void UA_Notification_enqueueSub(UA_Notification *n);
static void UA_Notification_dequeueSub(UA_Notification *n);

void
UA_Notification_delete(UA_Notification *n) {
    UA_assert(n != UA_SUBSCRIPTION_QUEUE_SENTINEL);
    if(n->mon) {
        UA_Notification_dequeueMon(n);
        UA_Notification_dequeueSub(n);
        switch(n->mon->itemToMonitor.attributeId) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        case UA_ATTRIBUTEID_EVENTNOTIFIER:
            UA_EventFieldList_clear(&n->data.event);
            UA_EventFilterResult_clear(&n->result);
            break;
#endif
        default:
            UA_MonitoredItemNotification_clear(&n->data.dataChange);
            break;
        }
    }
    UA_free(n);
}

/* Add to the MonitoredItem queue, update all counters and then handle overflow */
static void
UA_Notification_enqueueMon(UA_Server *server, UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;
    UA_assert(mon);
    UA_assert(TAILQ_NEXT(n, localEntry) == UA_SUBSCRIPTION_QUEUE_SENTINEL);

    /* Add to the MonitoredItem */
    TAILQ_INSERT_TAIL(&mon->queue, n, localEntry);
    ++mon->queueSize;

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    if(n->isOverflowEvent)
        ++mon->eventOverflows;
#endif

    /* Test for consistency */
    UA_assert(mon->queueSize >= mon->eventOverflows);
    UA_assert(mon->eventOverflows <= mon->queueSize - mon->eventOverflows + 1);

    /* Ensure enough space is available in the MonitoredItem. Do this only after
     * adding the new Notification. */
    UA_MonitoredItem_ensureQueueSpace(server, mon);

    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, mon->subscription,
                              "MonitoredItem %" PRIi32 " | "
                              "Notification enqueued (Queue size %lu / %lu)",
                              mon->monitoredItemId,
                              (long unsigned)mon->queueSize,
                              (long unsigned)mon->parameters.queueSize);
}

void
UA_Notification_enqueueSub(UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;
    UA_assert(mon);

    UA_Subscription *sub = mon->subscription;
    UA_assert(sub);

    if(TAILQ_NEXT(n, globalEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL)
        return;

    /* Add to the subscription if reporting is enabled */
    TAILQ_INSERT_TAIL(&sub->notificationQueue, n, globalEntry);
    ++sub->notificationQueueSize;

    switch(mon->itemToMonitor.attributeId) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        ++sub->eventNotifications;
        break;
#endif
    default:
        ++sub->dataChangeNotifications;
        break;
    }
}

void
UA_Notification_enqueueAndTrigger(UA_Server *server, UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;
    UA_Subscription *sub = mon->subscription;
    UA_assert(sub); /* This function is never called for local MonitoredItems */

    /* If reporting or (sampled+triggered), enqueue into the Subscription first
     * and then into the MonitoredItem. UA_MonitoredItem_ensureQueueSpace
     * (called within UA_Notification_enqueueMon) assumes the notification is
     * already in the Subscription's publishing queue. */
    if(mon->monitoringMode == UA_MONITORINGMODE_REPORTING ||
       (mon->monitoringMode == UA_MONITORINGMODE_SAMPLING &&
        mon->triggeredUntil > UA_DateTime_nowMonotonic())) {
        UA_Notification_enqueueSub(n);
        mon->triggeredUntil = UA_INT64_MIN;
        UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, mon->subscription,
                                  "Notification enqueued (Queue size %lu)",
                                  (long unsigned)mon->subscription->notificationQueueSize);
    }

    /* Insert into the MonitoredItem. This checks the queue size and
     * handles overflow. */
    UA_Notification_enqueueMon(server, n);

    for(size_t i = mon->triggeringLinksSize - 1; i < mon->triggeringLinksSize; i--) {
        /* Get the triggered MonitoredItem. Remove the link if the MI doesn't exist. */
        UA_MonitoredItem *triggeredMon =
            UA_Subscription_getMonitoredItem(sub, mon->triggeringLinks[i]);
        if(!triggeredMon) {
            UA_MonitoredItem_removeLink(sub, mon, mon->triggeringLinks[i]);
            continue;
        }

        /* Only sampling MonitoredItems receive a trigger. Reporting
         * MonitoredItems send out Notifications anyway and disabled
         * MonitoredItems don't create samples to send. */
        if(triggeredMon->monitoringMode != UA_MONITORINGMODE_SAMPLING)
            continue;

        /* Get the latest sampled Notification from the triggered MonitoredItem.
         * Enqueue for publication. */
        UA_Notification *n2 = TAILQ_LAST(&triggeredMon->queue, NotificationQueue);
        if(n2)
            UA_Notification_enqueueSub(n2);

        /* The next Notification within the publishing interval is going to be
         * published as well. (Falsely) assume that the publishing cycle has
         * started right now, so that we don't have to loop over MonitoredItems
         * to deactivate the triggering after the publishing cycle. */
        triggeredMon->triggeredUntil = UA_DateTime_nowMonotonic() +
            (UA_DateTime)(sub->publishingInterval * (UA_Double)UA_DATETIME_MSEC);

        UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                                  "MonitoredItem %u triggers MonitoredItem %u",
                                  mon->monitoredItemId, triggeredMon->monitoredItemId);
    }
}

/* Remove from the MonitoredItem queue and adjust all counters */
static void
UA_Notification_dequeueMon(UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;
    UA_assert(mon);

    if(TAILQ_NEXT(n, localEntry) == UA_SUBSCRIPTION_QUEUE_SENTINEL)
        return;

    /* Remove from the MonitoredItem queue */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    if(n->isOverflowEvent)
        --mon->eventOverflows;
#endif

    TAILQ_REMOVE(&mon->queue, n, localEntry);
    --mon->queueSize;

    /* Test for consistency */
    UA_assert(mon->queueSize >= mon->eventOverflows);
    UA_assert(mon->eventOverflows <= mon->queueSize - mon->eventOverflows + 1);

    /* Reset the sentintel */
    TAILQ_NEXT(n, localEntry) = UA_SUBSCRIPTION_QUEUE_SENTINEL;
}

void
UA_Notification_dequeueSub(UA_Notification *n) {
    if(TAILQ_NEXT(n, globalEntry) == UA_SUBSCRIPTION_QUEUE_SENTINEL)
        return;

    UA_MonitoredItem *mon = n->mon;
    UA_assert(mon);
    UA_Subscription *sub = mon->subscription;
    UA_assert(sub);

    switch(mon->itemToMonitor.attributeId) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        --sub->eventNotifications;
        break;
#endif
    default:
        --sub->dataChangeNotifications;
        break;
    }

    TAILQ_REMOVE(&sub->notificationQueue, n, globalEntry);
    --sub->notificationQueueSize;

    /* Reset the sentinel */
    TAILQ_NEXT(n, globalEntry) = UA_SUBSCRIPTION_QUEUE_SENTINEL;
}

/*****************/
/* MonitoredItem */
/*****************/

void
UA_MonitoredItem_init(UA_MonitoredItem *mon) {
    memset(mon, 0, sizeof(UA_MonitoredItem));
    mon->next = (UA_MonitoredItem*)~0; /* Not added to a node */
    TAILQ_INIT(&mon->queue);
    mon->triggeredUntil = UA_INT64_MIN;
}

static UA_StatusCode
addMonitoredItemBackpointer(UA_Server *server, UA_Session *session,
                            UA_Node *node, void *data) {
    UA_MonitoredItem *mon = (UA_MonitoredItem*)data;
    UA_assert(mon != (UA_MonitoredItem*)~0);
    mon->next = node->head.monitoredItems;
    node->head.monitoredItems = mon;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeMonitoredItemBackPointer(UA_Server *server, UA_Session *session,
                               UA_Node *node, void *data) {
    if(!node->head.monitoredItems)
        return UA_STATUSCODE_GOOD;

    /* Edge case that it's the first element */
    UA_MonitoredItem *remove = (UA_MonitoredItem*)data;
    if(node->head.monitoredItems == remove) {
        node->head.monitoredItems = remove->next;
        remove->next = (UA_MonitoredItem*)~0;
        return UA_STATUSCODE_GOOD;
    }

    UA_MonitoredItem *prev = node->head.monitoredItems;
    UA_MonitoredItem *entry = prev->next;
    for(; entry != NULL; prev = entry, entry = entry->next) {
        if(entry == remove) {
            prev->next = entry->next;
            remove->next = (UA_MonitoredItem*)~0;
            break;
        }
    }

    return UA_STATUSCODE_GOOD;
}

void
UA_Server_registerMonitoredItem(UA_Server *server, UA_MonitoredItem *mon) {
    if(mon->registered)
        return;

    /* Register in Subscription and Server */
    UA_Subscription *sub = mon->subscription;
    if(sub) {
        mon->monitoredItemId = ++sub->lastMonitoredItemId;
        mon->subscription = sub;
        sub->monitoredItemsSize++;
        LIST_INSERT_HEAD(&sub->monitoredItems, mon, listEntry);
    } else {
        mon->monitoredItemId = ++server->lastLocalMonitoredItemId;
        LIST_INSERT_HEAD(&server->localMonitoredItems, mon, listEntry);
    }
    server->monitoredItemsSize++;

    /* Register the MonitoredItem in userland */
    if(server->config.monitoredItemRegisterCallback) {
        UA_Session *session = &server->adminSession;
        if(sub)
            session = sub->session;

        void *targetContext = NULL;
        getNodeContext(server, mon->itemToMonitor.nodeId, &targetContext);
        UA_UNLOCK(&server->serviceMutex);
        server->config.monitoredItemRegisterCallback(server,
                                                     session ? &session->sessionId : NULL,
                                                     session ? session->sessionHandle : NULL,
                                                     &mon->itemToMonitor.nodeId,
                                                     targetContext,
                                                     mon->itemToMonitor.attributeId, false);
        UA_LOCK(&server->serviceMutex);
    }

    mon->registered = true;
}

static void
UA_Server_unregisterMonitoredItem(UA_Server *server, UA_MonitoredItem *mon) {
    if(!mon->registered)
        return;

    UA_Subscription *sub = mon->subscription;
    UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, sub,
                             "MonitoredItem %" PRIi32 " | Deleting the MonitoredItem",
                             mon->monitoredItemId);

    /* Deregister MonitoredItem in userland */
    if(server->config.monitoredItemRegisterCallback) {
        UA_Session *session = &server->adminSession;
        if(sub)
            session = sub->session;

        void *targetContext = NULL;
        getNodeContext(server, mon->itemToMonitor.nodeId, &targetContext);
        UA_UNLOCK(&server->serviceMutex);
        server->config.monitoredItemRegisterCallback(server,
                                                     session ? &session->sessionId : NULL,
                                                     session ? session->sessionHandle : NULL,
                                                     &mon->itemToMonitor.nodeId,
                                                     targetContext,
                                                     mon->itemToMonitor.attributeId, true);
        UA_LOCK(&server->serviceMutex);
    }

    /* Deregister in Subscription and server */
    if(sub)
        sub->monitoredItemsSize--;
    LIST_REMOVE(mon, listEntry); /* Also for LocalMonitoredItems */
    server->monitoredItemsSize--;

    mon->registered = false;
}

UA_StatusCode
UA_MonitoredItem_setMonitoringMode(UA_Server *server, UA_MonitoredItem *mon,
                                   UA_MonitoringMode monitoringMode) {
    /* Check if the MonitoringMode is valid or not */
    if(monitoringMode > UA_MONITORINGMODE_REPORTING)
        return UA_STATUSCODE_BADMONITORINGMODEINVALID;

    /* Set the MonitoringMode, store the old mode */
    UA_MonitoringMode oldMode = mon->monitoringMode;
    mon->monitoringMode = monitoringMode;

    UA_Notification *notification;
    /* Reporting is disabled. This causes all Notifications to be dequeued and
     * deleted. Also remove the last samples so that we immediately generate a
     * Notification when re-activated. */
    if(mon->monitoringMode == UA_MONITORINGMODE_DISABLED) {
        UA_Notification *notification_tmp;
        UA_MonitoredItem_unregisterSampling(server, mon);
        TAILQ_FOREACH_SAFE(notification, &mon->queue, localEntry, notification_tmp) {
            UA_Notification_delete(notification);
        }
        UA_DataValue_clear(&mon->lastValue);
        return UA_STATUSCODE_GOOD;
    }

    /* When reporting is enabled, put all notifications that were already
     * sampled into the global queue of the subscription. When sampling is
     * enabled, remove all notifications from the global queue. !!! This needs
     * to be the same operation as in UA_Notification_enqueue !!! */
    if(mon->monitoringMode == UA_MONITORINGMODE_REPORTING) {
        /* Make all notifications reporting. Re-enqueue to ensure they have the
         * right order if some notifications are already reported by a trigger
         * link. */
        TAILQ_FOREACH(notification, &mon->queue, localEntry) {
            UA_Notification_dequeueSub(notification);
            UA_Notification_enqueueSub(notification);
        }
    } else /* mon->monitoringMode == UA_MONITORINGMODE_SAMPLING */ {
        /* Make all notifications non-reporting */
        TAILQ_FOREACH(notification, &mon->queue, localEntry)
            UA_Notification_dequeueSub(notification);
    }

    /* Register the sampling callback with an interval. If registering the
     * sampling callback failed, set to disabled. But don't delete the current
     * notifications. */
    UA_StatusCode res = UA_MonitoredItem_registerSampling(server, mon);
    if(res != UA_STATUSCODE_GOOD) {
        mon->monitoringMode = UA_MONITORINGMODE_DISABLED;
        return res;
    }

    /* Manually create the first sample if the MonitoredItem was disabled, the
     * MonitoredItem is now sampling (or reporting) and it is not an
     * Event-MonitoredItem */
    if(oldMode == UA_MONITORINGMODE_DISABLED &&
       mon->monitoringMode > UA_MONITORINGMODE_DISABLED &&
       mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER)
        monitoredItem_sampleCallback(server, mon);

    return UA_STATUSCODE_GOOD;
}

void
UA_MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Remove the sampling callback */
    UA_MonitoredItem_unregisterSampling(server, mon);

    UA_assert(mon->next == (UA_MonitoredItem*)~0); /* Not attached to any node */

    /* Deregister in Server and Subscription */
    if(mon->registered)
        UA_Server_unregisterMonitoredItem(server, mon);

    /* Remove the TriggeringLinks */
    if(mon->triggeringLinksSize > 0) {
        UA_free(mon->triggeringLinks);
        mon->triggeringLinks = NULL;
        mon->triggeringLinksSize = 0;
    }

    /* Remove the queued notifications attached to the subscription */
    UA_Notification *notification, *notification_tmp;
    TAILQ_FOREACH_SAFE(notification, &mon->queue, localEntry, notification_tmp) {
        UA_Notification_delete(notification);
    }

    /* Remove the settings */
    UA_ReadValueId_clear(&mon->itemToMonitor);
    UA_MonitoringParameters_clear(&mon->parameters);

    /* Remove the last samples */
    UA_DataValue_clear(&mon->lastValue);

    /* Add a delayed callback to remove the MonitoredItem when the current jobs
     * have completed. This is needed to allow that a local MonitoredItem can
     * remove itself in the callback. */
    mon->delayedFreePointers.callback = NULL;
    mon->delayedFreePointers.application = server;
    mon->delayedFreePointers.context = NULL;
    server->config.eventLoop->
        addDelayedCallback(server->config.eventLoop, &mon->delayedFreePointers);
}

void
UA_MonitoredItem_ensureQueueSpace(UA_Server *server, UA_MonitoredItem *mon) {
    /* There can be only one EventOverflow more than normal entries. Because
     * EventOverflows are never adjacent. */
    UA_assert(mon->queueSize >= mon->eventOverflows);
    UA_assert(mon->eventOverflows <= mon->queueSize - mon->eventOverflows + 1);

    /* Always attached to a Subscription (no local MonitoredItem) */
    UA_Subscription *sub = mon->subscription;
    UA_assert(sub);

    /* Nothing to do */
    if(mon->queueSize - mon->eventOverflows <= mon->parameters.queueSize)
        return;

    /* Remove notifications until the required queue size is reached */
    UA_Boolean reporting = false;
    size_t remove = mon->queueSize - mon->eventOverflows - mon->parameters.queueSize;
    while(remove > 0) {
        /* The minimum queue size (without EventOverflows) is 1. At least two
         * notifications that are not EventOverflows are in the queue. */
        UA_assert(mon->queueSize - mon->eventOverflows >= 2);

        /* Select the next notification to delete. Skip over overflow events. */
        UA_Notification *del = NULL;
        if(mon->parameters.discardOldest) {
            /* Remove the oldest */
            del = TAILQ_FIRST(&mon->queue);
#if defined(UA_ENABLE_SUBSCRIPTIONS_EVENTS) && !defined(__clang_analyzer__)
            while(del->isOverflowEvent)
                del = TAILQ_NEXT(del, localEntry); /* skip overflow events */
#endif
        } else {
            /* Remove the second newest (to keep the up-to-date notification).
             * The last entry is not an OverflowEvent -- we just added it. */
            del = TAILQ_LAST(&mon->queue, NotificationQueue);
            del = TAILQ_PREV(del, NotificationQueue, localEntry);
#if defined(UA_ENABLE_SUBSCRIPTIONS_EVENTS) && !defined(__clang_analyzer__)
            while(del->isOverflowEvent)
                del = TAILQ_PREV(del, NotificationQueue, localEntry); /* skip overflow events */
#endif
        }

        UA_assert(del); /* There must have been one entry that can be deleted */

        /* Only create OverflowEvents (and set InfoBits) if the notification
         * that is removed is reported */
        if(TAILQ_NEXT(del, globalEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL)
            reporting = true;

        /* Move the entry after del in the per-MonitoredItem queue right after
         * del in the per-Subscription queue. So we don't starve MonitoredItems
         * with a high sampling interval in the Subscription queue by always
         * removing their first appearance in the per-Subscription queue.
         *
         * With MonitoringMode == SAMPLING, the Notifications are not (all) in
         * the per-Subscription queue. Don't reinsert in that case.
         *
         * For the reinsertion to work, first insert into the per-Subscription
         * queue. */
        if(TAILQ_NEXT(del, globalEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL) {
            UA_Notification *after_del = TAILQ_NEXT(del, localEntry);
            UA_assert(after_del); /* There must be one remaining element after del */
            if(TAILQ_NEXT(after_del, globalEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL) {
                TAILQ_REMOVE(&sub->notificationQueue, after_del, globalEntry);
                TAILQ_INSERT_AFTER(&sub->notificationQueue, del, after_del, globalEntry);
            }
        }

        remove--;

        /* Delete the notification and remove it from the queues */
        UA_Notification_delete(del);

        /* Update the subscription diagnostics statistics */
#ifdef UA_ENABLE_DIAGNOSTICS
        sub->monitoringQueueOverflowCount++;
#endif

        /* Assertions to help Clang's scan-analyzer */
        UA_assert(del != TAILQ_FIRST(&mon->queue));
        UA_assert(del != TAILQ_LAST(&mon->queue, NotificationQueue));
        UA_assert(del != TAILQ_PREV(TAILQ_LAST(&mon->queue, NotificationQueue),
                                    NotificationQueue, localEntry));
    }

        /* Leave an entry to indicate that notifications were removed */
    if(reporting) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        if(mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER)
            createEventOverflowNotification(server, sub, mon);
        else
#endif
            setOverflowInfoBits(mon);
    }
}

UA_StatusCode
UA_MonitoredItem_registerSampling(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if(mon->sampleCallbackIsRegistered)
        return UA_STATUSCODE_GOOD;

    UA_assert(mon->next == (UA_MonitoredItem*)~0); /* Not registered in a node */

    /* Only DataChange MonitoredItems with a positive sampling interval have a
     * repeated callback. Other MonitoredItems are attached to the Node in a
     * linked list of backpointers. */
    UA_StatusCode res;
    if(mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER ||
       mon->parameters.samplingInterval == 0.0) {
        UA_Subscription *sub = mon->subscription;
        UA_Session *session = &server->adminSession;
        if(sub)
            session = sub->session;
        res = UA_Server_editNode(server, session, &mon->itemToMonitor.nodeId,
                                 addMonitoredItemBackpointer, mon);
    } else {
        res = addRepeatedCallback(server,
                                  (UA_ServerCallback)UA_MonitoredItem_sampleCallback,
                                  mon, mon->parameters.samplingInterval,
                                  &mon->sampleCallbackId);
    }

    if(res == UA_STATUSCODE_GOOD)
        mon->sampleCallbackIsRegistered = true;
    return res;
}

void
UA_MonitoredItem_unregisterSampling(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if(!mon->sampleCallbackIsRegistered)
        return;

    mon->sampleCallbackIsRegistered = false;

    /* Check for mon->next and not the samplingInterval. Because that might
     * currently be changed. */
    if(mon->next != (UA_MonitoredItem*)~0) {
        /* Added to a node */
        UA_Subscription *sub = mon->subscription;
        UA_Session *session = &server->adminSession;
        if(sub)
            session = sub->session;
        UA_Server_editNode(server, session, &mon->itemToMonitor.nodeId,
                           removeMonitoredItemBackPointer, mon);
    } else {
        /* Registered with a repeated callback */
        removeCallback(server, mon->sampleCallbackId);
    }
}

UA_StatusCode
UA_MonitoredItem_removeLink(UA_Subscription *sub, UA_MonitoredItem *mon, UA_UInt32 linkId) {
    /* Find the index */
    size_t i = 0;
    for(; i < mon->triggeringLinksSize; i++) {
        if(mon->triggeringLinks[i] == linkId)
            break;
    }

    /* Not existing / already removed */
    if(i == mon->triggeringLinksSize)
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;

    /* Remove the link */
    mon->triggeringLinksSize--;
    if(mon->triggeringLinksSize == 0) {
        UA_free(mon->triggeringLinks);
        mon->triggeringLinks = NULL;
    } else {
        mon->triggeringLinks[i] = mon->triggeringLinks[mon->triggeringLinksSize];
        UA_UInt32 *tmpLinks = (UA_UInt32*)
            UA_realloc(mon->triggeringLinks, mon->triggeringLinksSize * sizeof(UA_UInt32));
        if(tmpLinks)
            mon->triggeringLinks = tmpLinks;
    }

    /* Does the target MonitoredItem exist? This is stupid, but the CTT wants us
     * to to this. We don't auto-remove links together with the target
     * MonitoredItem. Links to removed MonitoredItems are removed when the link
     * triggers and the target no longer exists. */
    UA_MonitoredItem *mon2 = UA_Subscription_getMonitoredItem(sub, linkId);
    if(!mon2)
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_MonitoredItem_addLink(UA_Subscription *sub, UA_MonitoredItem *mon, UA_UInt32 linkId) {
    /* Does the target MonitoredItem exist? */
    UA_MonitoredItem *mon2 = UA_Subscription_getMonitoredItem(sub, linkId);
    if(!mon2)
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;

    /* Does the link already exist? */
    for(size_t i = 0 ; i < mon->triggeringLinksSize; i++) {
        if(mon->triggeringLinks[i] == linkId)
            return UA_STATUSCODE_GOOD;
    }

    /* Allocate the memory */
    UA_UInt32 *tmpLinkIds = (UA_UInt32*)
        UA_realloc(mon->triggeringLinks, (mon->triggeringLinksSize + 1) * sizeof(UA_UInt32));
    if(!tmpLinkIds)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    mon->triggeringLinks = tmpLinkIds;

    /* Add the link */
    mon->triggeringLinks[mon->triggeringLinksSize] = linkId;
    mon->triggeringLinksSize++;
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
