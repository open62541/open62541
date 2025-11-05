/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2015-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015 (c) Joakim L. Gilje
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Florian Palm
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Mattias Bornhager
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2017-2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2018 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Martin Lang)
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 *    Copyright 2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 *    Copyright 2020-2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"
#include "itoa.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

#define UA_MAX_RETRANSMISSIONQUEUESIZE 256

/****************/
/* Notification */
/****************/

static void UA_Notification_dequeueMon(UA_Notification *n);
static void UA_Notification_enqueueSub(UA_Notification *n);
static void UA_Notification_dequeueSub(UA_Notification *n);

UA_Notification *
UA_Notification_new(void) {
    UA_Notification *n = (UA_Notification*)UA_calloc(1, sizeof(UA_Notification));
    if(n) {
        /* Set the sentinel for a notification that is not enqueued a
         * subscription */
        TAILQ_NEXT(n, subEntry) = UA_SUBSCRIPTION_QUEUE_SENTINEL;
    }
    return n;
}

/* Dequeue and delete the notification */
static void
UA_Notification_delete(UA_Notification *n) {
    UA_assert(n != UA_SUBSCRIPTION_QUEUE_SENTINEL);
    UA_assert(n->mon);
    UA_Notification_dequeueMon(n);
    UA_Notification_dequeueSub(n);
    switch(n->mon->itemToMonitor.attributeId) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
        UA_EventFieldList_clear(&n->data.event);
        break;
#endif
    default:
        UA_MonitoredItemNotification_clear(&n->data.dataChange);
        break;
    }
    UA_free(n);
}

/* Add to the MonitoredItem queue, update all counters and then handle overflow */
static void
UA_Notification_enqueueMon(UA_Server *server, UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;
    UA_assert(mon);

    /* Add to the MonitoredItem */
    TAILQ_INSERT_TAIL(&mon->queue, n, monEntry);
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

    UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, mon->subscription,
                              "MonitoredItem %" PRIi32 " | "
                              "Notification enqueued (Queue size %lu / %lu)",
                              mon->monitoredItemId,
                              (long unsigned)mon->queueSize,
                              (long unsigned)mon->parameters.queueSize);
}

static void
UA_Notification_enqueueSub(UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;
    UA_assert(mon);

    UA_Subscription *sub = mon->subscription;
    UA_assert(sub);

    if(TAILQ_NEXT(n, subEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL)
        return;

    /* Add to the subscription if reporting is enabled */
    TAILQ_INSERT_TAIL(&sub->notificationQueue, n, subEntry);
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
    UA_assert(sub); /* A MonitoredItem is always attached to a subscription. Can
                     * be a local MonitoredItem that gets published immediately
                     * with a callback. */

    /* If reporting or (sampled+triggered), enqueue into the Subscription first
     * and then into the MonitoredItem. UA_MonitoredItem_ensureQueueSpace
     * (called within UA_Notification_enqueueMon) assumes the notification is
     * already in the Subscription's publishing queue. */
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    if(mon->monitoringMode == UA_MONITORINGMODE_REPORTING ||
       (mon->monitoringMode == UA_MONITORINGMODE_SAMPLING &&
        mon->triggeredUntil > nowMonotonic)) {
        UA_Notification_enqueueSub(n);
        mon->triggeredUntil = UA_INT64_MIN;
        UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                                  "Notification enqueued (Queue size %lu)",
                                  (long unsigned)sub->notificationQueueSize);
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
        triggeredMon->triggeredUntil = nowMonotonic +
            (UA_DateTime)(sub->publishingInterval * (UA_Double)UA_DATETIME_MSEC);

        UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                                  "MonitoredItem %u triggers MonitoredItem %u",
                                  mon->monitoredItemId, triggeredMon->monitoredItemId);
    }

    /* If we just enqueued a notification into the local adminSubscription, then
     * register a delayed callback for "local publishing". */
    if(sub == server->adminSubscription && !sub->delayedCallbackRegistered) {
        sub->delayedCallbackRegistered = true;
        sub->delayedMoreNotifications.callback =
            (UA_Callback)UA_Subscription_localPublish;
        sub->delayedMoreNotifications.application = server;
        sub->delayedMoreNotifications.context = sub;

        el = server->config.eventLoop;
        el->addDelayedCallback(el, &sub->delayedMoreNotifications);
    }
}

/* Remove from the MonitoredItem queue. This only happens if the Notification is
 * deleted right after. */
static void
UA_Notification_dequeueMon(UA_Notification *n) {
    UA_MonitoredItem *mon = n->mon;
    UA_assert(mon);

    /* Remove from the MonitoredItem queue */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    if(n->isOverflowEvent)
        --mon->eventOverflows;
#endif

    TAILQ_REMOVE(&mon->queue, n, monEntry);
    --mon->queueSize;

    /* Test for consistency */
    UA_assert(mon->queueSize >= mon->eventOverflows);
    UA_assert(mon->eventOverflows <= mon->queueSize - mon->eventOverflows + 1);
}

void
UA_Notification_dequeueSub(UA_Notification *n) {
    if(TAILQ_NEXT(n, subEntry) == UA_SUBSCRIPTION_QUEUE_SENTINEL)
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

    TAILQ_REMOVE(&sub->notificationQueue, n, subEntry);
    --sub->notificationQueueSize;

    /* Reset the sentinel */
    TAILQ_NEXT(n, subEntry) = UA_SUBSCRIPTION_QUEUE_SENTINEL;
}

/****************/
/* Subscription */
/****************/

UA_Subscription *
UA_Subscription_new(void) {
    /* Allocate the memory */
    UA_Subscription *newSub = (UA_Subscription*)UA_calloc(1, sizeof(UA_Subscription));
    if(!newSub)
        return NULL;

    /* The first publish response is sent immediately */
    newSub->state = UA_SUBSCRIPTIONSTATE_STOPPED;

    /* Even if the first publish response is a keepalive the sequence number is 1.
     * This can happen by a subscription without a monitored item (see CTT test scripts). */
    newSub->nextSequenceNumber = 1;

    TAILQ_INIT(&newSub->retransmissionQueue);
    TAILQ_INIT(&newSub->notificationQueue);
    return newSub;
}

static void
delayedFreeSubscription(void *app, void *context) {
    UA_free(context);
}

void
UA_Subscription_delete(UA_Server *server, UA_Subscription *sub) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_EventLoop *el = server->config.eventLoop;

    /* Unregister the publish callback and possible delayed callback */
    Subscription_setState(server, sub, UA_SUBSCRIPTIONSTATE_REMOVING);

    /* Remove delayed callbacks for processing remaining notifications */
    if(sub->delayedCallbackRegistered) {
        el->removeDelayedCallback(el, &sub->delayedMoreNotifications);
        sub->delayedCallbackRegistered = false;
    }

    /* Remove the diagnostics object for the subscription */
#ifdef UA_ENABLE_DIAGNOSTICS
    if(!UA_NodeId_isNull(&sub->ns0Id))
        deleteNode(server, sub->ns0Id, true);
    UA_NodeId_clear(&sub->ns0Id);
#endif

    UA_LOG_INFO_SUBSCRIPTION(server->config.logging, sub, "Subscription deleted");

    /* Detach from the session if necessary */
    if(sub->session)
        UA_Session_detachSubscription(server, sub->session, sub, true);

    /* Remove from the server if not previously registered */
    if(sub->serverListEntry.le_prev) {
        LIST_REMOVE(sub, serverListEntry);
        UA_assert(server->subscriptionsSize > 0);
        server->subscriptionsSize--;
        server->serverDiagnosticsSummary.currentSubscriptionCount--;
    }

    /* Delete monitored Items */
    UA_assert(server->monitoredItemsSize >= sub->monitoredItemsSize);
    UA_MonitoredItem *mon, *tmp_mon;
    LIST_FOREACH_SAFE(mon, &sub->monitoredItems, listEntry, tmp_mon) {
        UA_MonitoredItem_delete(server, mon);
    }
    UA_assert(sub->monitoredItemsSize == 0);

    /* Delete Retransmission Queue */
    UA_NotificationMessageEntry *nme, *nme_tmp;
    TAILQ_FOREACH_SAFE(nme, &sub->retransmissionQueue, listEntry, nme_tmp) {
        TAILQ_REMOVE(&sub->retransmissionQueue, nme, listEntry);
        UA_NotificationMessage_clear(&nme->message);
        UA_free(nme);
        if(sub->session)
            --sub->session->totalRetransmissionQueueSize;
        --sub->retransmissionQueueSize;
    }
    UA_assert(sub->retransmissionQueueSize == 0);

    /* Pointers to the subscription may still exist upwards in the call stack.
     * Add a delayed callback to remove the Subscription when the current jobs
     * have completed. */
    sub->delayedFreePointers.callback = delayedFreeSubscription;
    sub->delayedFreePointers.application = NULL;
    sub->delayedFreePointers.context = sub;
    el->addDelayedCallback(el, &sub->delayedFreePointers);
}

void
Subscription_resetLifetime(UA_Subscription *sub) {
    sub->currentLifetimeCount = 0;
}

UA_MonitoredItem *
UA_Subscription_getMonitoredItem(UA_Subscription *sub, UA_UInt32 monitoredItemId) {
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
        if(mon->monitoredItemId == monitoredItemId)
            break;
    }
    return mon;
}

static void
removeOldestRetransmissionMessageFromSub(UA_Subscription *sub) {
    UA_NotificationMessageEntry *oldestEntry =
        TAILQ_LAST(&sub->retransmissionQueue, NotificationMessageQueue);
    TAILQ_REMOVE(&sub->retransmissionQueue, oldestEntry, listEntry);
    UA_NotificationMessage_clear(&oldestEntry->message);
    UA_free(oldestEntry);
    --sub->retransmissionQueueSize;
    if(sub->session)
        --sub->session->totalRetransmissionQueueSize;

#ifdef UA_ENABLE_DIAGNOSTICS
    sub->discardedMessageCount++;
#endif
}

static void
removeOldestRetransmissionMessageFromSession(UA_Session *session) {
    UA_NotificationMessageEntry *oldestEntry = NULL;
    UA_Subscription *oldestSub = NULL;
    UA_Subscription *sub;
    TAILQ_FOREACH(sub, &session->subscriptions, sessionListEntry) {
        UA_NotificationMessageEntry *first =
            TAILQ_LAST(&sub->retransmissionQueue, NotificationMessageQueue);
        if(!first)
            continue;
        if(!oldestEntry || oldestEntry->message.publishTime > first->message.publishTime) {
            oldestEntry = first;
            oldestSub = sub;
        }
    }
    UA_assert(oldestEntry);
    UA_assert(oldestSub);

    removeOldestRetransmissionMessageFromSub(oldestSub);
}

static void
UA_Subscription_addRetransmissionMessage(UA_Server *server, UA_Subscription *sub,
                                         UA_NotificationMessageEntry *entry) {
    /* Release the oldest entry if there is not enough space */
    UA_Session *session = sub->session;
    if(sub->retransmissionQueueSize >= UA_MAX_RETRANSMISSIONQUEUESIZE) {
        UA_LOG_WARNING_SUBSCRIPTION(server->config.logging, sub,
                                    "Subscription retransmission queue overflow");
        removeOldestRetransmissionMessageFromSub(sub);
    } else if(session && server->config.maxRetransmissionQueueSize > 0 &&
              session->totalRetransmissionQueueSize >=
              server->config.maxRetransmissionQueueSize) {
        UA_LOG_WARNING_SUBSCRIPTION(server->config.logging, sub,
                                    "Session-wide retransmission queue overflow");
        removeOldestRetransmissionMessageFromSession(sub->session);
    }

    /* Add entry */
    TAILQ_INSERT_TAIL(&sub->retransmissionQueue, entry, listEntry);
    ++sub->retransmissionQueueSize;
    if(session)
        ++session->totalRetransmissionQueueSize;
}

UA_StatusCode
UA_Subscription_removeRetransmissionMessage(UA_Subscription *sub, UA_UInt32 sequenceNumber) {
    /* Find the retransmission message */
    UA_NotificationMessageEntry *entry;
    TAILQ_FOREACH(entry, &sub->retransmissionQueue, listEntry) {
        if(entry->message.sequenceNumber == sequenceNumber)
            break;
    }
    if(!entry)
        return UA_STATUSCODE_BADSEQUENCENUMBERUNKNOWN;

    /* Remove the retransmission message */
    TAILQ_REMOVE(&sub->retransmissionQueue, entry, listEntry);
    --sub->retransmissionQueueSize;
    UA_NotificationMessage_clear(&entry->message);
    UA_free(entry);

    if(sub->session)
        --sub->session->totalRetransmissionQueueSize;

    return UA_STATUSCODE_GOOD;
}

/* The output counters are only set when the preparation is successful */
static UA_StatusCode
prepareNotificationMessage(UA_Server *server, UA_Subscription *sub,
                           UA_NotificationMessage *message,
                           size_t maxNotifications) {
    UA_assert(maxNotifications > 0);

    /* Allocate an ExtensionObject for Event- and DataChange-Notifications. Also
     * there can be StatusChange-Notifications. The standard says in Part 4,
     * 7.2.1:
     *
     * If a Subscription contains MonitoredItems for events and data, this array
     * should have not more than 2 elements. */
    message->notificationData = (UA_ExtensionObject*)
        UA_Array_new(2, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if(!message->notificationData)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    message->notificationDataSize = 2;

    /* Pre-allocate DataChangeNotifications */
    size_t notificationDataIdx = 0;
    size_t dcnPos = 0; /* How many DataChangeNotifications? */
    UA_DataChangeNotification *dcn = NULL;
    if(sub->dataChangeNotifications > 0) {
        dcn = UA_DataChangeNotification_new();
        if(!dcn) {
            UA_NotificationMessage_clear(message);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        UA_ExtensionObject_setValue(message->notificationData, dcn,
                                    &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION]);
        size_t dcnSize = sub->dataChangeNotifications;
        if(dcnSize > maxNotifications)
            dcnSize = maxNotifications;
        dcn->monitoredItems = (UA_MonitoredItemNotification*)
            UA_Array_new(dcnSize, &UA_TYPES[UA_TYPES_MONITOREDITEMNOTIFICATION]);
        if(!dcn->monitoredItems) {
            UA_NotificationMessage_clear(message); /* Also frees the dcn */
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dcn->monitoredItemsSize = dcnSize;
        notificationDataIdx++;
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    size_t enlPos = 0; /* How many EventNotifications? */
    UA_EventNotificationList *enl = NULL;
    if(sub->eventNotifications > 0) {
        enl = UA_EventNotificationList_new();
        if(!enl) {
            UA_NotificationMessage_clear(message);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        UA_ExtensionObject_setValue(&message->notificationData[notificationDataIdx],
                                    enl, &UA_TYPES[UA_TYPES_EVENTNOTIFICATIONLIST]);
        size_t enlSize = sub->eventNotifications;
        if(enlSize > maxNotifications)
            enlSize = maxNotifications;
        enl->events = (UA_EventFieldList*)
            UA_Array_new(enlSize, &UA_TYPES[UA_TYPES_EVENTFIELDLIST]);
        if(!enl->events) {
            UA_NotificationMessage_clear(message);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        enl->eventsSize = enlSize;
        notificationDataIdx++;
    }
#endif

    UA_assert(notificationDataIdx > 0);
    message->notificationDataSize = notificationDataIdx;

    /* <-- The point of no return --> */

    /* How many notifications were moved to the response overall? */
    size_t totalNotifications = 0;
    UA_Notification *n, *n_tmp;
    TAILQ_FOREACH_SAFE(n, &sub->notificationQueue, subEntry, n_tmp) {
        if(totalNotifications >= maxNotifications)
            break;

        /* Move the content to the response */
        switch(n->mon->itemToMonitor.attributeId) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        case UA_ATTRIBUTEID_EVENTNOTIFIER:
            UA_assert(enl != NULL); /* Have at least one event notification */
            enl->events[enlPos] = n->data.event;
            UA_EventFieldList_init(&n->data.event);
            enlPos++;
            break;
#endif
        default:
            UA_assert(dcn != NULL); /* Have at least one change notification */
            dcn->monitoredItems[dcnPos] = n->data.dataChange;
            UA_DataValue_init(&n->data.dataChange.value);
            dcnPos++;
            break;
        }

        /* If there are Notifications *before this one* in the MonitoredItem-
         * local queue, remove all of them. These are earlier Notifications that
         * are non-reporting. And we don't want them to show up after the
         * current Notification has been sent out. */
        UA_Notification *prev;
        while((prev = TAILQ_PREV(n, NotificationQueue, monEntry))) {
            UA_Notification_delete(prev);

            /* Help the Clang scan-analyzer */
            UA_assert(prev != TAILQ_PREV(n, NotificationQueue, monEntry));
        }

        /* Delete the notification, remove from the queues and decrease the counters */
        UA_Notification_delete(n);

        totalNotifications++;
    }

    /* Set sizes */
    if(dcn) {
        dcn->monitoredItemsSize = dcnPos;
        if(dcnPos == 0) {
            UA_free(dcn->monitoredItems);
            dcn->monitoredItems = NULL;
        }
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    if(enl) {
        enl->eventsSize = enlPos;
        if(enlPos == 0) {
            UA_free(enl->events);
            enl->events = NULL;
        }
    }
#endif

    return UA_STATUSCODE_GOOD;
}

/* According to OPC Unified Architecture, Part 4 5.13.1.1 i) The value 0 is
 * never used for the sequence number */
static UA_UInt32
UA_Subscription_nextSequenceNumber(UA_UInt32 sequenceNumber) {
    UA_UInt32 nextSequenceNumber = sequenceNumber + 1;
    if(nextSequenceNumber == 0)
        nextSequenceNumber = 1;
    return nextSequenceNumber;
}

static void
sendStatusChangeDelete(UA_Server *server, UA_Subscription *sub,
                       UA_PublishResponseEntry *pre) {
    /* Cannot send out the StatusChange because no response is queued. Delete
     * the Subscription without sending the StatusChange. */
    if(!pre) {
        UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                                  "Cannot send the StatusChange notification because "
                                  "no response is queued.");
        if(UA_StatusCode_isBad(sub->statusChange)) {
            UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                                      "Removing the subscription.");
            UA_Subscription_delete(server, sub);
        }
        return;
    }

    UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                              "Sending out a StatusChange notification and "
                              "removing the subscription");

    UA_EventLoop *el = server->config.eventLoop;

    /* Populate the response */
    UA_PublishResponse *response = &pre->response;

    UA_StatusChangeNotification scn;
    UA_StatusChangeNotification_init(&scn);
    scn.status = sub->statusChange;

    UA_ExtensionObject notificationData;
    UA_ExtensionObject_setValue(&notificationData, &scn,
                                &UA_TYPES[UA_TYPES_STATUSCHANGENOTIFICATION]);

    response->notificationMessage.notificationData = &notificationData;
    response->notificationMessage.notificationDataSize = 1;
    response->subscriptionId = sub->subscriptionId;
    response->notificationMessage.publishTime = el->dateTime_now(el);
    response->notificationMessage.sequenceNumber = sub->nextSequenceNumber;

    /* Send the response */
    UA_assert(sub->session); /* Otherwise pre is NULL */
    UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                              "Sending out a publish response");
    sendResponse(server, sub->session->channel, pre->requestId,
                 (UA_Response *)response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    /* Clean up */
    response->notificationMessage.notificationData = NULL;
    response->notificationMessage.notificationDataSize = 0;
    UA_PublishResponse_clear(&pre->response);
    UA_free(pre);

    /* Delete the subscription */
    UA_Subscription_delete(server, sub);
}

/* The local adminSubscription forwards notifications to a registered callback
 * method. This is done async from a delayed callback registered in the
 * EventLoop. */
void
UA_Subscription_localPublish(UA_Server *server, UA_Subscription *sub) {
    lockServer(server);
    sub->delayedCallbackRegistered = false;

    UA_Notification *n, *n_tmp;
    TAILQ_FOREACH_SAFE(n, &sub->notificationQueue, subEntry, n_tmp) {
        UA_MonitoredItem *mon = n->mon;
        UA_LocalMonitoredItem *localMon = (UA_LocalMonitoredItem*)mon;

        /* Move the content to the response */
        void *nodeContext = NULL;
        switch(mon->itemToMonitor.attributeId) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        case UA_ATTRIBUTEID_EVENTNOTIFIER:
            /* Set the fields in the key-value map */
            UA_assert(n->data.event.eventFieldsSize == localMon->eventFields.mapSize);
            for(size_t i = 0; i < localMon->eventFields.mapSize; i++) {
                localMon->eventFields.map[i].value = n->data.event.eventFields[i];
            }

            /* Call the callback */
            localMon->callback.
                eventCallback(server, mon->monitoredItemId, localMon->context,
                              localMon->eventFields);
            break;
#endif
        default:
            getNodeContext(server, mon->itemToMonitor.nodeId, &nodeContext);
            localMon->callback.
                dataChangeCallback(server, mon->monitoredItemId, localMon->context,
                                   &mon->itemToMonitor.nodeId, nodeContext,
                                   mon->itemToMonitor.attributeId,
                                   &n->data.dataChange.value);
            break;
        }

        /* If there are Notifications *before this one* in the MonitoredItem-
         * local queue, remove all of them. These are earlier Notifications that
         * are non-reporting. And we don't want them to show up after the
         * current Notification has been sent out. */
        UA_Notification *prev;
        while((prev = TAILQ_PREV(n, NotificationQueue, monEntry))) {
            UA_Notification_delete(prev);

            /* Help the Clang scan-analyzer */
            UA_assert(prev != TAILQ_PREV(n, NotificationQueue, monEntry));
        }

        /* Delete the notification, remove from the queues and decrease the counters */
        UA_Notification_delete(n);
    }

    unlockServer(server);
}

static void
delayedPublishNotifications(UA_Server *server, UA_Subscription *sub) {
    lockServer(server);
    sub->delayedCallbackRegistered = false;
    UA_Subscription_publish(server, sub);
    unlockServer(server);
}

/* Try to publish now. Enqueue a "next publish" as a delayed callback if not
 * done. */
void
UA_Subscription_publish(UA_Server *server, UA_Subscription *sub) {
    UA_EventLoop *el = server->config.eventLoop;

    /* Get a response */
    UA_PublishResponseEntry *pre = NULL;
    if(sub->session) {
        UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
        do {
            /* Dequeue the oldest response */
            pre = UA_Session_dequeuePublishReq(sub->session);
            if(!pre)
                break;

            /* Check if the TimeoutHint is still valid. Otherwise return with a bad
             * statuscode and continue. */
            if(pre->maxTime < nowMonotonic) {
                UA_LOG_DEBUG_SESSION(server->config.logging, sub->session,
                                     "Publish request %u has timed out", pre->requestId);
                pre->response.responseHeader.serviceResult = UA_STATUSCODE_BADTIMEOUT;
                sendResponse(server, sub->session->channel, pre->requestId,
                             (UA_Response *)&pre->response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
                UA_PublishResponse_clear(&pre->response);
                UA_free(pre);
                pre = NULL;
            }
        } while(!pre);
    }

    /* Update the LifetimeCounter */
    if(pre) {
        Subscription_resetLifetime(sub);
    } else {
        UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                                  "The publish queue is empty");
        ++sub->currentLifetimeCount;
        if(sub->currentLifetimeCount > sub->lifeTimeCount) {
            UA_LOG_WARNING_SUBSCRIPTION(server->config.logging, sub,
                                        "End of subscription lifetime");
            /* Set the StatusChange to delete the subscription. */
            sub->statusChange = UA_STATUSCODE_BADTIMEOUT;
        }
    }

    /* Send a StatusChange notification if possible and delete the
     * Subscription */
    if(sub->statusChange != UA_STATUSCODE_GOOD) {
        sendStatusChangeDelete(server, sub, pre);
        return;
    }

    /* Dsiabled subscriptions do not send notifications */
    UA_UInt32 notifications = (sub->state == UA_SUBSCRIPTIONSTATE_ENABLED) ?
        sub->notificationQueueSize : 0;

    /* Limit the number of notifications to the configured maximum */
    if(notifications > sub->notificationsPerPublish)
        notifications = sub->notificationsPerPublish;

    /* Return if no notifications and no keepalive */
    if(notifications == 0) {
        ++sub->currentKeepAliveCount;
        if(sub->currentKeepAliveCount < sub->maxKeepAliveCount) {
            if(pre)
                UA_Session_queuePublishReq(sub->session, pre, true); /* Re-enqueue */
            return;
        }
        UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub, "Sending a KeepAlive");
    }

    /* We want to send a response, but cannot. Either because there is no queued
     * response or because the Subscription is detached from a Session or because
     * the SecureChannel for the Session is closed. */
    if(!pre || !sub->session || !sub->session->channel) {
        UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                                  "Want to send a publish response but cannot. "
                                  "The subscription is late.");
        sub->late = true;
        if(pre)
            UA_Session_queuePublishReq(sub->session, pre, true); /* Re-enqueue */
        return;
    }

    UA_assert(pre);
    UA_assert(sub->session); /* Otherwise pre is NULL */

    /* Prepare the response */
    UA_PublishResponse *response = &pre->response;
    UA_NotificationMessage *message = &response->notificationMessage;
    UA_NotificationMessageEntry *retransmission = NULL;
#ifdef UA_ENABLE_DIAGNOSTICS
    size_t priorDataChangeNotifications = sub->dataChangeNotifications;
    size_t priorEventNotifications = sub->eventNotifications;
#endif
    if(notifications > 0) {
        if(server->config.enableRetransmissionQueue) {
            /* Allocate the retransmission entry */
            retransmission = (UA_NotificationMessageEntry*)
                UA_malloc(sizeof(UA_NotificationMessageEntry));
            if(!retransmission) {
                UA_LOG_WARNING_SUBSCRIPTION(server->config.logging, sub,
                                            "Could not allocate memory for retransmission. "
                                            "The subscription is late.");
                sub->late = true;
                UA_Session_queuePublishReq(sub->session, pre, true); /* Re-enqueue */
                return;
            }
        }

        /* Prepare the response */
        UA_StatusCode retval =
            prepareNotificationMessage(server, sub, message, notifications);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SUBSCRIPTION(server->config.logging, sub,
                                        "Could not prepare the notification message. "
                                        "The subscription is late.");
            /* If the retransmission queue is enabled a retransmission message is allocated */
            if(retransmission)
                UA_free(retransmission);
            sub->late = true;
            UA_Session_queuePublishReq(sub->session, pre, true); /* Re-enqueue */
            return;
        }
    }

    /* <-- The point of no return --> */

    /* Set up the response */
    response->subscriptionId = sub->subscriptionId;
    response->moreNotifications = (sub->notificationQueueSize > 0);
    message->publishTime = el->dateTime_now(el);

    /* Set sequence number to message. Started at 1 which is given during
     * creating a new subscription. The 1 is required for initial publish
     * response with or without an monitored item. */
    message->sequenceNumber = sub->nextSequenceNumber;

    if(notifications > 0) {
        /* If the retransmission queue is enabled a retransmission message is
         * allocated */
        if(retransmission) {
            /* Put the notification message into the retransmission queue. This
             * needs to be done here, so that the message itself is included in
             * the available sequence numbers for acknowledgement. */
            retransmission->message = response->notificationMessage;
            UA_Subscription_addRetransmissionMessage(server, sub, retransmission);
        }
        /* Only if a notification was created, the sequence number must be
         * increased. For a keepalive the sequence number can be reused. */
        sub->nextSequenceNumber =
            UA_Subscription_nextSequenceNumber(sub->nextSequenceNumber);
    }

    /* Get the available sequence numbers from the retransmission queue */
    UA_assert(sub->retransmissionQueueSize <= UA_MAX_RETRANSMISSIONQUEUESIZE);
    UA_UInt32 seqNumbers[UA_MAX_RETRANSMISSIONQUEUESIZE];
    response->availableSequenceNumbers = seqNumbers;
    response->availableSequenceNumbersSize = sub->retransmissionQueueSize;
    size_t i = 0;
    UA_NotificationMessageEntry *nme;
    TAILQ_FOREACH(nme, &sub->retransmissionQueue, listEntry) {
        response->availableSequenceNumbers[i] = nme->message.sequenceNumber;
        ++i;
    }
    UA_assert(i == sub->retransmissionQueueSize);

    /* Send the response */
    UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                              "Sending out a publish response with %" PRIu32
                              " notifications", notifications);
    sendResponse(server, sub->session->channel, pre->requestId,
                 (UA_Response*)response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    /* Reset the Subscription state to NORMAL. But only if all notifications
     * have been sent out. Otherwise keep the Subscription in the LATE state. So
     * we immediately answer incoming Publish requests.
     *
     * (We also check that session->responseQueueSize > 0 in Service_Publish. To
     * avoid answering Publish requests out of order. As we additionally may have
     * scheduled a publish callback as a delayed callback. */
    if(sub->notificationQueueSize == 0)
        sub->late = false;

    /* Reset the KeepAlive after publishing */
    sub->currentKeepAliveCount = 0;

    /* Free the response */
    if(retransmission) {
        /* NotificationMessage was moved into retransmission queue */
        UA_NotificationMessage_init(&response->notificationMessage);
    }
    response->availableSequenceNumbers = NULL;
    response->availableSequenceNumbersSize = 0;
    UA_PublishResponse_clear(&pre->response);
    UA_free(pre);

    /* Update the diagnostics statistics */
#ifdef UA_ENABLE_DIAGNOSTICS
    sub->publishRequestCount++;

    UA_UInt32 sentDCN = (UA_UInt32)
        (priorDataChangeNotifications - sub->dataChangeNotifications);
    UA_UInt32 sentEN = (UA_UInt32)(priorEventNotifications - sub->eventNotifications);
    sub->dataChangeNotificationsCount += sentDCN;
    sub->eventNotificationsCount += sentEN;
    sub->notificationsCount += (sentDCN + sentEN);
#endif

    /* Repeat sending notifications if there are more notifications to send. But
     * only call UA_MonitoredItem_sample in the regular publish callback. */
    UA_Boolean done = (sub->notificationQueueSize == 0);
    if(!done && !sub->delayedCallbackRegistered) {
        sub->delayedCallbackRegistered = true;

        sub->delayedMoreNotifications.callback = (UA_Callback)delayedPublishNotifications;
        sub->delayedMoreNotifications.application = server;
        sub->delayedMoreNotifications.context = sub;

        el = server->config.eventLoop;
        el->addDelayedCallback(el, &sub->delayedMoreNotifications);
    }
}

void
UA_Subscription_resendData(UA_Server *server, UA_Subscription *sub) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_assert(server);
    UA_assert(sub);

    /* Part 4, §6.7: If this Method is called, subsequent Publish responses
     * shall contain the current values of all data MonitoredItems in the
     * Subscription where the MonitoringMode is set to Reporting. If a value is
     * queued for a data MonitoredItem, the next value in the queue is sent in
     * the Publish response. If no value is queued for a data MonitoredItem, the
     * last value sent is repeated in the Publish response. */
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
        /* Create only DataChange notifications */
        if(mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER)
            continue;

        /* Only if the mode is monitoring */
        if(mon->monitoringMode != UA_MONITORINGMODE_REPORTING)
            continue;

        /* If a value is queued for a data MonitoredItem, the next value in
         * the queue is sent in the Publish response. */
        if(mon->queueSize > 0)
            continue;

        /* Create a notification with the last sampled value */
        UA_MonitoredItem_createDataChangeNotification(server, mon, &mon->lastValue);
    }
}

void
UA_Session_ensurePublishQueueSpace(UA_Server* server, UA_Session* session) {
    if(server->config.maxPublishReqPerSession == 0)
        return;

    while(session->responseQueueSize >= server->config.maxPublishReqPerSession) {
        /* Dequeue a response */
        UA_PublishResponseEntry *pre = UA_Session_dequeuePublishReq(session);
        UA_assert(pre != NULL); /* There must be a pre as session->responseQueueSize > 0 */

        UA_LOG_DEBUG_SESSION(server->config.logging, session,
                             "Sending out a publish response triggered by too many publish requests");

        /* Send the response. This response has no related subscription id */
        UA_PublishResponse *response = &pre->response;
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYPUBLISHREQUESTS;
        sendResponse(server, session->channel, pre->requestId,
                     (UA_Response *)response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

        /* Free the response */
        UA_PublishResponse_clear(response);
        UA_free(pre);
    }
}

static void
sampleAndPublishCallback(UA_Server *server, UA_Subscription *sub) {
    UA_assert(sub);

    lockServer(server);

    UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                              "Sample and Publish Callback");

    /* Sample the MonitoredItems with sampling interval <0 (which implies
     * sampling in the same interval as the subscription) */
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->samplingMonitoredItems, sampling.subscriptionSampling) {
        UA_MonitoredItem_sample(server, mon);
    }

    /* Publish the queued notifications */
    UA_Subscription_publish(server, sub);

    unlockServer(server);
}

UA_StatusCode
Subscription_setState(UA_Server *server, UA_Subscription *sub,
                      UA_SubscriptionState state) {
    if(state <= UA_SUBSCRIPTIONSTATE_REMOVING) {
        if(sub->publishCallbackId != 0) {
            removeCallback(server, sub->publishCallbackId);
            sub->publishCallbackId = 0;
#ifdef UA_ENABLE_DIAGNOSTICS
            sub->disableCount++;
#endif
        }
    } else if(sub->publishCallbackId == 0) {
        UA_StatusCode res =
            addRepeatedCallback(server, (UA_ServerCallback)sampleAndPublishCallback,
                                sub, sub->publishingInterval, &sub->publishCallbackId);
        if(res != UA_STATUSCODE_GOOD) {
            sub->state = UA_SUBSCRIPTIONSTATE_STOPPED;
            return res;
        }

        /* Send (at least a) keepalive after the next publish interval */
        sub->currentKeepAliveCount = sub->maxKeepAliveCount;

#ifdef UA_ENABLE_DIAGNOSTICS
        sub->enableCount++;
#endif
    }

    sub->state = state;
    return UA_STATUSCODE_GOOD;
}

/****************/
/* Notification */
/****************/

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

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
        UA_Notification *before = TAILQ_PREV(indicator, NotificationQueue, monEntry);
        if(before && before->isOverflowEvent)
            return UA_STATUSCODE_GOOD;
    }

    /* A Notification is inserted into the queue which includes only the
     * NodeId of the OverflowEventType. */

    /* Get the EventFilter */
    if(mon->parameters.filter.content.decoded.type != &UA_TYPES[UA_TYPES_EVENTFILTER])
        return UA_STATUSCODE_BADINTERNALERROR;
    const UA_EventFilter *ef = (const UA_EventFilter*)
        mon->parameters.filter.content.decoded.data;
    if(ef->selectClausesSize == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Initialize the notification */
    UA_Notification *n = UA_Notification_new();
    if(!n)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    n->isOverflowEvent = true;
    n->mon = mon;
    n->data.event.clientHandle = mon->parameters.clientHandle;

    /* The session is needed to evaluate the select-clause. But used only for
     * limited reads on the source node. So we can use the admin-session here if
     * the subscription is detached. */
    UA_Session *session = (sub->session) ? sub->session : &server->adminSession;

    /* Set up the context for the filter evaluation */
    static UA_String sourceName = UA_STRING_STATIC("Internal/EventQueueOverflow");
    UA_KeyValuePair fields[1];
    fields[0].key = (UA_QualifiedName){1, UA_STRING_STATIC("/SourceName")};
    UA_Variant_setScalar(&fields[0].value, &sourceName, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap fieldMap = {1, fields};

    UA_FilterEvalContext ctx;
    UA_FilterEvalContext_init(&ctx);
    ctx.server = server;
    ctx.session = session;
    ctx.filter = *ef;
    ctx.ed.sourceNode = UA_NS0ID(SERVER);
    ctx.ed.eventType = UA_NS0ID(EVENTQUEUEOVERFLOWEVENTTYPE);
    ctx.ed.severity = 201; /* TODO: Can this be configured? */
    ctx.ed.message = UA_LOCALIZEDTEXT(NULL, NULL);
    ctx.ed.eventFields = &fieldMap;

    /* Evaluate the select clause to populate the notification */
    UA_StatusCode res = evaluateSelectClause(&ctx, &n->data.event);
    UA_FilterEvalContext_reset(&ctx);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(n);
        return res;
    }

    /* Insert before the removed notification. This is either first in the
     * queue (if the oldest notification was removed) or before the new event
     * that remains the last element of the queue.
     *
     * Ensure that the following is consistent with UA_Notification_enqueueMon
     * and UA_Notification_enqueueSub! */
    TAILQ_INSERT_BEFORE(indicator, n, monEntry);
    ++mon->eventOverflows;
    ++mon->queueSize;

    /* Test for consistency */
    UA_assert(mon->queueSize >= mon->eventOverflows);
    UA_assert(mon->eventOverflows <= mon->queueSize - mon->eventOverflows + 1);

    if(TAILQ_NEXT(indicator, subEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL) {
        /* Insert just before the indicator */
        TAILQ_INSERT_BEFORE(indicator, n, subEntry);
    } else {
        /* The indicator was not reporting or not added yet. */
        if(!mon->parameters.discardOldest) {
            /* Add last to the per-Subscription queue */
            TAILQ_INSERT_TAIL(&mon->subscription->notificationQueue,
                              n, subEntry);
        } else {
            /* Find the oldest reported element. Add before that. */
            while(indicator) {
                indicator = TAILQ_PREV(indicator, NotificationQueue, monEntry);
                if(!indicator) {
                    TAILQ_INSERT_TAIL(&mon->subscription->notificationQueue,
                                      n, subEntry);
                    break;
                }
                if(TAILQ_NEXT(indicator, subEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL) {
                    TAILQ_INSERT_BEFORE(indicator, n, subEntry);
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

/*****************/
/* MonitoredItem */
/*****************/

void
UA_MonitoredItem_init(UA_MonitoredItem *mon) {
    memset(mon, 0, sizeof(UA_MonitoredItem));
    TAILQ_INIT(&mon->queue);
    mon->triggeredUntil = UA_INT64_MIN;
}

static UA_StatusCode
addMonitoredItemBackpointer(UA_Server *server, UA_Session *session,
                            UA_Node *node, void *data) {
    UA_MonitoredItem *mon = (UA_MonitoredItem*)data;
    UA_assert(mon != (UA_MonitoredItem*)~0);
    mon->sampling.nodeListNext = node->head.monitoredItems;
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
        node->head.monitoredItems = remove->sampling.nodeListNext;
        return UA_STATUSCODE_GOOD;
    }

    UA_MonitoredItem *prev = node->head.monitoredItems;
    UA_MonitoredItem *entry = prev->sampling.nodeListNext;
    for(; entry != NULL; prev = entry, entry = entry->sampling.nodeListNext) {
        if(entry == remove) {
            prev->sampling.nodeListNext = entry->sampling.nodeListNext;
            break;
        }
    }

    return UA_STATUSCODE_GOOD;
}

void
UA_MonitoredItem_register(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    if(mon->registered)
        return;
    mon->registered = true;

    /* Register in Subscription and Server */
    UA_Subscription *sub = mon->subscription;
    mon->monitoredItemId = ++sub->lastMonitoredItemId;
    mon->subscription = sub;
    LIST_INSERT_HEAD(&sub->monitoredItems, mon, listEntry);
    sub->monitoredItemsSize++;
    server->monitoredItemsSize++;

    /* Register the MonitoredItem in userland */
    if(server->config.monitoredItemRegisterCallback) {
        UA_Session *session = sub->session;
        void *targetContext = NULL;
        getNodeContext(server, mon->itemToMonitor.nodeId, &targetContext);
        server->config.monitoredItemRegisterCallback(server,
                                                     session ? &session->sessionId : NULL,
                                                     session ? session->context : NULL,
                                                     &mon->itemToMonitor.nodeId,
                                                     targetContext,
                                                     mon->itemToMonitor.attributeId, false);
    }
}

static void
UA_Server_unregisterMonitoredItem(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    if(!mon->registered)
        return;
    mon->registered = false;

    UA_Subscription *sub = mon->subscription;
    UA_LOG_INFO_SUBSCRIPTION(server->config.logging, sub,
                             "MonitoredItem %" PRIi32 " | Deleting the MonitoredItem",
                             mon->monitoredItemId);

    /* Deregister MonitoredItem in userland */
    if(server->config.monitoredItemRegisterCallback) {
        UA_Session *session = sub->session;
        void *targetContext = NULL;
        getNodeContext(server, mon->itemToMonitor.nodeId, &targetContext);
        server->config.monitoredItemRegisterCallback(server,
                                                     session ? &session->sessionId : NULL,
                                                     session ? session->context : NULL,
                                                     &mon->itemToMonitor.nodeId,
                                                     targetContext,
                                                     mon->itemToMonitor.attributeId, true);
    }

    /* Deregister in Subscription and server */
    sub->monitoredItemsSize--;
    LIST_REMOVE(mon, listEntry);
    server->monitoredItemsSize--;
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
        TAILQ_FOREACH_SAFE(notification, &mon->queue, monEntry, notification_tmp) {
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
        TAILQ_FOREACH(notification, &mon->queue, monEntry) {
            UA_Notification_dequeueSub(notification);
            UA_Notification_enqueueSub(notification);
        }
    } else /* mon->monitoringMode == UA_MONITORINGMODE_SAMPLING */ {
        /* Make all notifications non-reporting */
        TAILQ_FOREACH(notification, &mon->queue, monEntry)
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
        UA_MonitoredItem_sample(server, mon);

    return UA_STATUSCODE_GOOD;
}

static void
delayedFreeMonitoredItem(void *app, void *context) {
    UA_free(context);
}

void
UA_MonitoredItem_delete(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Remove the sampling callback */
    UA_MonitoredItem_unregisterSampling(server, mon);

    /* Deregister in Server and Subscription */
    if(mon->registered)
        UA_Server_unregisterMonitoredItem(server, mon);

    /* Cancel outstanding async reads. The status code avoids the sample being
     * processed. Call _processReady to ensure that the callbacks have been
     * triggered. */
    if(mon->outstandingAsyncReads > 0)
        async_cancel(server, mon, UA_STATUSCODE_BADREQUESTCANCELLEDBYREQUEST, true);
    UA_assert(mon->outstandingAsyncReads == 0);

    /* Remove the TriggeringLinks */
    if(mon->triggeringLinksSize > 0) {
        UA_free(mon->triggeringLinks);
        mon->triggeringLinks = NULL;
        mon->triggeringLinksSize = 0;
    }

    /* Remove the queued notifications attached to the subscription */
    UA_Notification *notification, *notification_tmp;
    TAILQ_FOREACH_SAFE(notification, &mon->queue, monEntry, notification_tmp) {
        UA_Notification_delete(notification);
    }

    /* Remove the settings */
    UA_ReadValueId_clear(&mon->itemToMonitor);
    UA_MonitoringParameters_clear(&mon->parameters);

    /* Remove the last samples */
    UA_DataValue_clear(&mon->lastValue);

    /* If this is a local MonitoredItem, clean up additional values */
    if(mon->subscription == server->adminSubscription) {
        UA_LocalMonitoredItem *lm = (UA_LocalMonitoredItem*)mon;
        for(size_t i = 0; i < lm->eventFields.mapSize; i++)
            UA_Variant_init(&lm->eventFields.map[i].value);
        UA_KeyValueMap_clear(&lm->eventFields);
    }

    /* Add a delayed callback to remove the MonitoredItem when the current jobs
     * have completed. This is needed to allow that a local MonitoredItem can
     * remove itself in the callback. */
    mon->delayedFreePointers.callback = delayedFreeMonitoredItem;
    mon->delayedFreePointers.application = NULL;
    mon->delayedFreePointers.context = mon;
    UA_EventLoop *el = server->config.eventLoop;
    el->addDelayedCallback(el, &mon->delayedFreePointers);
}

void
UA_MonitoredItem_ensureQueueSpace(UA_Server *server, UA_MonitoredItem *mon) {
    /* There can be only one EventOverflow more than normal entries. Because
     * EventOverflows are never adjacent. */
    UA_assert(mon->queueSize >= mon->eventOverflows);
    UA_assert(mon->eventOverflows <= mon->queueSize - mon->eventOverflows + 1);

    /* MonitoredItems are always attached to a Subscription */
    UA_Subscription *sub = mon->subscription;
    UA_assert(sub);

    /* Nothing to do */
    if(mon->queueSize - mon->eventOverflows <= mon->parameters.queueSize)
        return;

    /* Help clang-analyzer */
#if defined(UA_DEBUG) && defined(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
    UA_Notification *last_del = NULL;
    (void)last_del;
#endif

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
#if defined(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
            /* Skip overflow events */
            while(del->isOverflowEvent) {
                del = TAILQ_NEXT(del, monEntry);
                UA_assert(del != last_del);
            }
#endif
        } else {
            /* Remove the second newest (to keep the up-to-date notification).
             * The last entry is not an OverflowEvent -- we just added it. */
            del = TAILQ_LAST(&mon->queue, NotificationQueue);
            del = TAILQ_PREV(del, NotificationQueue, monEntry);
#if defined(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
            /* skip overflow events */
            while(del->isOverflowEvent) {
                del = TAILQ_PREV(del, NotificationQueue, monEntry);
                UA_assert(del != last_del);
            }
#endif
        }

        UA_assert(del); /* There must be one entry that can be deleted */

        /* Only create OverflowEvents (and set InfoBits) if the notification
         * that is removed is reported */
        if(TAILQ_NEXT(del, subEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL)
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
        if(TAILQ_NEXT(del, subEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL) {
            UA_Notification *after_del = TAILQ_NEXT(del, monEntry);
            UA_assert(after_del); /* There must be one remaining element after del */
            if(TAILQ_NEXT(after_del, subEntry) != UA_SUBSCRIPTION_QUEUE_SENTINEL) {
                TAILQ_REMOVE(&sub->notificationQueue, after_del, subEntry);
                TAILQ_INSERT_AFTER(&sub->notificationQueue, del, after_del, subEntry);
            }
        }

        remove--;

        /* Delete the notification and remove it from the queues */
        UA_Notification_delete(del);

        /* Update the subscription diagnostics statistics */
#ifdef UA_ENABLE_DIAGNOSTICS
        sub->monitoringQueueOverflowCount++;
#endif

        /* Help scan-analyzer */
#if defined(UA_DEBUG) && defined(UA_ENABLE_SUBSCRIPTIONS_EVENTS)
        last_del = del;
#endif
        UA_assert(del != TAILQ_FIRST(&mon->queue));
        UA_assert(del != TAILQ_LAST(&mon->queue, NotificationQueue));
        UA_assert(del != TAILQ_PREV(TAILQ_LAST(&mon->queue, NotificationQueue),
                                    NotificationQueue, monEntry));
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

static void
UA_MonitoredItem_lockAndSample(UA_Server *server, UA_MonitoredItem *mon) {
    lockServer(server);
    UA_MonitoredItem_sample(server, mon);
    unlockServer(server);
}

UA_StatusCode
UA_MonitoredItem_registerSampling(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Sampling is already registered */
    if(mon->samplingType != UA_MONITOREDITEMSAMPLINGTYPE_NONE)
        return UA_STATUSCODE_GOOD;

    /* The subscription is attached to a session at this point */
    UA_Subscription *sub = mon->subscription;
    if(!sub->session)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER ||
       mon->parameters.samplingInterval == 0.0) {
        /* Add to the linked list in the node */
        res = editNode(server, sub->session, &mon->itemToMonitor.nodeId, 0,
                       UA_REFERENCETYPESET_NONE, UA_BROWSEDIRECTION_INVALID,
                       addMonitoredItemBackpointer, mon);
        if(res == UA_STATUSCODE_GOOD)
            mon->samplingType = UA_MONITOREDITEMSAMPLINGTYPE_EVENT;
    } else if(mon->parameters.samplingInterval == sub->publishingInterval) {
        /* Add to the subscription for sampling before every publish */
        LIST_INSERT_HEAD(&sub->samplingMonitoredItems, mon,
                         sampling.subscriptionSampling);
        mon->samplingType = UA_MONITOREDITEMSAMPLINGTYPE_PUBLISH;
    } else {
        /* DataChange MonitoredItems with a positive sampling interval have a
         * repeated callback. Other MonitoredItems are attached to the Node in a
         * linked list of backpointers. */
        res = addRepeatedCallback(server,
                                  (UA_ServerCallback)UA_MonitoredItem_lockAndSample,
                                  mon, mon->parameters.samplingInterval,
                                  &mon->sampling.callbackId);
        if(res == UA_STATUSCODE_GOOD)
            mon->samplingType = UA_MONITOREDITEMSAMPLINGTYPE_CYCLIC;
    }

    return res;
}

void
UA_MonitoredItem_unregisterSampling(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    switch(mon->samplingType) {
    case UA_MONITOREDITEMSAMPLINGTYPE_CYCLIC:
        /* Remove repeated callback */
        removeCallback(server, mon->sampling.callbackId);
        break;

    case UA_MONITOREDITEMSAMPLINGTYPE_EVENT: {
        /* Removing is always done with the AdminSession. So it also works when
         * the Subscription has been detached from its Session. */
        editNode(server, &server->adminSession, &mon->itemToMonitor.nodeId, 0,
                 UA_REFERENCETYPESET_NONE, UA_BROWSEDIRECTION_INVALID,
                 removeMonitoredItemBackPointer, mon);
        break;
    }

    case UA_MONITOREDITEMSAMPLINGTYPE_PUBLISH:
        /* Added to the subscription */
        LIST_REMOVE(mon, sampling.subscriptionSampling);
        break;

    case UA_MONITOREDITEMSAMPLINGTYPE_NONE:
    default:
        /* Sampling is not registered */
        break;
    }

    mon->samplingType = UA_MONITOREDITEMSAMPLINGTYPE_NONE;
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
