/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2015-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2015 (c) Joakim L. Gilje
 *    Copyright 2016-2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2017 (c) Mattias Bornhager
 *    Copyright 2018 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Martin Lang)
 *    Copyright 2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"
#include "itoa.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

#define UA_MAX_RETRANSMISSIONQUEUESIZE 256

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
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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
            UA_NotificationMessage_clear(message);
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
    UA_Notification *notification, *notification_tmp;
    TAILQ_FOREACH_SAFE(notification, &sub->notificationQueue,
                       globalEntry, notification_tmp) {
        if(totalNotifications >= maxNotifications)
            break;

        /* Move the content to the response */
        switch(notification->mon->itemToMonitor.attributeId) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        case UA_ATTRIBUTEID_EVENTNOTIFIER:
            UA_assert(enl != NULL); /* Have at least one event notification */
            enl->events[enlPos] = notification->data.event;
            UA_EventFieldList_init(&notification->data.event);
            enlPos++;
            break;
#endif
        default:
            UA_assert(dcn != NULL); /* Have at least one change notification */
            dcn->monitoredItems[dcnPos] = notification->data.dataChange;
            UA_DataValue_init(&notification->data.dataChange.value);
            dcnPos++;
            break;
        }

        /* If there are Notifications *before this one* in the MonitoredItem-
         * local queue, remove all of them. These are earlier Notifications that
         * are non-reporting. And we don't want them to show up after the
         * current Notification has been sent out. */
        UA_Notification *prev;
        while((prev = TAILQ_PREV(notification, NotificationQueue, localEntry))) {
            UA_Notification_delete(prev);
        }

        /* Delete the notification, remove from the queues and decrease the counters */
        UA_Notification_delete(notification);

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
    /* Cannot send out the StatusChange because no response is queued.
     * Delete the Subscription without sending the StatusChange, if the statusChange is Bad*/
    if(!pre) {
        UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                                  "Cannot send the StatusChange notification because no response is queued.");
        if(UA_StatusCode_isBad(sub->statusChange)) {
            UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub, "Removing the subscription.");
            UA_Subscription_delete(server, sub);
        }
        return;
    }

    UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                              "Sending out a StatusChange "
                              "notification and removing the subscription");

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
    UA_LOCK(&server->serviceMutex);
    sub->delayedCallbackRegistered = false;

    UA_Notification *n, *n_tmp;
    TAILQ_FOREACH_SAFE(n, &sub->notificationQueue, globalEntry, n_tmp) {
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
            UA_UNLOCK(&server->serviceMutex);
            localMon->callback.
                eventCallback(server, mon->monitoredItemId, localMon->context,
                              localMon->eventFields);
            UA_LOCK(&server->serviceMutex);
            break;
#endif
        default:
            getNodeContext(server, mon->itemToMonitor.nodeId, &nodeContext);
            UA_UNLOCK(&server->serviceMutex);
            localMon->callback.
                dataChangeCallback(server, mon->monitoredItemId, localMon->context,
                                   &mon->itemToMonitor.nodeId, nodeContext,
                                   mon->itemToMonitor.attributeId,
                                   &n->data.dataChange.value);
            UA_LOCK(&server->serviceMutex);
            break;
        }

        /* If there are Notifications *before this one* in the MonitoredItem-
         * local queue, remove all of them. These are earlier Notifications that
         * are non-reporting. And we don't want them to show up after the
         * current Notification has been sent out. */
        UA_Notification *prev;
        while((prev = TAILQ_PREV(n, NotificationQueue, localEntry))) {
            UA_Notification_delete(prev);
        }

        /* Delete the notification, remove from the queues and decrease the counters */
        UA_Notification_delete(n);
    }

    UA_UNLOCK(&server->serviceMutex);
}

static void
delayedPublishNotifications(UA_Server *server, UA_Subscription *sub) {
    UA_LOCK(&server->serviceMutex);
    sub->delayedCallbackRegistered = false;
    UA_Subscription_publish(server, sub);
    UA_UNLOCK(&server->serviceMutex);
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
     * only call monitoredItem_sampleCallback in the regular publish
     * callback. */
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
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
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
    UA_LOCK(&server->serviceMutex);
    UA_assert(sub);

    UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub,
                              "Sample and Publish Callback");

    /* Sample the MonitoredItems with sampling interval <0 (which implies
     * sampling in the same interval as the subscription) */
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->samplingMonitoredItems, sampling.subscriptionSampling) {
        monitoredItem_sampleCallback(server, mon);
    }

    /* Publish the queued notifications */
    UA_Subscription_publish(server, sub);

    UA_UNLOCK(&server->serviceMutex);
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

#endif /* UA_ENABLE_SUBSCRIPTIONS */
