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

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

#define UA_MAX_RETRANSMISSIONQUEUESIZE 256

UA_Subscription *
UA_Subscription_new(void) {
    /* Allocate the memory */
    UA_Subscription *newSub = (UA_Subscription*)UA_calloc(1, sizeof(UA_Subscription));
    if(!newSub)
        return NULL;

    /* The first publish response is sent immediately */
    newSub->state = UA_SUBSCRIPTIONSTATE_NORMAL;

    /* Even if the first publish response is a keepalive the sequence number is 1.
     * This can happen by a subscription without a monitored item (see CTT test scripts). */
    newSub->nextSequenceNumber = 1;

    TAILQ_INIT(&newSub->retransmissionQueue);
    TAILQ_INIT(&newSub->notificationQueue);
    return newSub;
}

void
UA_Subscription_delete(UA_Server *server, UA_Subscription *sub) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Unregister the publish callback */
    Subscription_unregisterPublishCallback(server, sub);

    /* Remove the diagnostics object for the subscription */
#ifdef UA_ENABLE_DIAGNOSTICS
    if(sub->session) {
        /* Use a browse path to find the node */
        char subIdStr[32];
        snprintf(subIdStr, 32, "%u", sub->subscriptionId);
        UA_BrowsePath bp;
        UA_BrowsePath_init(&bp);
        bp.startingNode = sub->session->sessionId;
        UA_RelativePathElement rpe[2];
        memset(rpe, 0, sizeof(UA_RelativePathElement) * 2);
        rpe[0].targetName = UA_QUALIFIEDNAME(0, "SubscriptionDiagnosticsArray");
        rpe[1].targetName = UA_QUALIFIEDNAME(0, subIdStr);
        bp.relativePath.elements = rpe;
        bp.relativePath.elementsSize = 2;
        UA_BrowsePathResult bpr = translateBrowsePathToNodeIds(server, &bp);

        /* Delete all nodes matching the browse path */
        for(size_t i = 0; i < bpr.targetsSize; i++) {
            if(bpr.targets[i].remainingPathIndex < UA_UINT32_MAX)
                continue;
            deleteNode(server, bpr.targets[i].targetId.nodeId, true);
        }
        UA_BrowsePathResult_clear(&bpr);
    }
#endif

    UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, sub, "Subscription deleted");

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

    /* Add a delayed callback to remove the Subscription when the current jobs
     * have completed. Pointers to the subscription may still exist upwards in
     * the call stack. */
    sub->delayedFreePointers.callback = NULL;
    sub->delayedFreePointers.application = server;
    sub->delayedFreePointers.context = NULL;
    server->config.eventLoop->
        addDelayedCallback(server->config.eventLoop, &sub->delayedFreePointers);
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
        UA_LOG_WARNING_SUBSCRIPTION(&server->config.logger, sub,
                                    "Subscription retransmission queue overflow");
        removeOldestRetransmissionMessageFromSub(sub);
    } else if(session && server->config.maxRetransmissionQueueSize > 0 &&
              session->totalRetransmissionQueueSize >=
              server->config.maxRetransmissionQueueSize) {
        UA_LOG_WARNING_SUBSCRIPTION(&server->config.logger, sub,
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
publishCallback(UA_Server *server, UA_Subscription *sub) {
    UA_LOCK(&server->serviceMutex);
    UA_Subscription_publish(server, sub);
    UA_UNLOCK(&server->serviceMutex);
}

static void
sendStatusChangeDelete(UA_Server *server, UA_Subscription *sub,
                       UA_PublishResponseEntry *pre) {
    /* Cannot send out the StatusChange because no response is queued.
     * Delete the Subscription without sending the StatusChange. */
    if(!pre) {
        UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                                  "Cannot send the StatusChange notification. "
                                  "Removing the subscription.");
        UA_Subscription_delete(server, sub);
        return;
    }

    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                              "Sending out a StatusChange "
                              "notification and removing the subscription");

    /* Populate the response */
    UA_PublishResponse *response = &pre->response;

    UA_StatusChangeNotification scn;
    UA_StatusChangeNotification_init(&scn);
    scn.status = sub->statusChange;

    UA_ExtensionObject notificationData;
    UA_ExtensionObject_setValue(&notificationData, &scn,
                                &UA_TYPES[UA_TYPES_STATUSCHANGENOTIFICATION]);

    response->responseHeader.timestamp = UA_DateTime_now();
    response->notificationMessage.notificationData = &notificationData;
    response->notificationMessage.notificationDataSize = 1;
    response->subscriptionId = sub->subscriptionId;
    response->notificationMessage.publishTime = response->responseHeader.timestamp;
    response->notificationMessage.sequenceNumber = sub->nextSequenceNumber;

    /* Send the response */
    UA_assert(sub->session); /* Otherwise pre is NULL */
    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                              "Sending out a publish response");
    sendResponse(server, sub->session, sub->session->header.channel, pre->requestId,
                 (UA_Response *)response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    /* Clean up */
    response->notificationMessage.notificationData = NULL;
    response->notificationMessage.notificationDataSize = 0;
    UA_PublishResponse_clear(&pre->response);
    UA_free(pre);

    /* Delete the subscription */
    UA_Subscription_delete(server, sub);
}

/* Called every time we set the subscription late (or it is still late) */
static void
UA_Subscription_isLate(UA_Subscription *sub) {
    sub->state = UA_SUBSCRIPTIONSTATE_LATE;
#ifdef UA_ENABLE_DIAGNOSTICS
    sub->latePublishRequestCount++;
#endif
}

void
UA_Subscription_publish(UA_Server *server, UA_Subscription *sub) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub, "Publish Callback");
    UA_assert(sub);

    /* Dequeue a response */
    UA_PublishResponseEntry *pre = NULL;
    if(sub->session)
        pre = UA_Session_dequeuePublishReq(sub->session);

    /* Update the LifetimeCounter */
    if(pre) {
        sub->currentLifetimeCount = 0;
    } else {
        UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                                  "The publish queue is empty");
        ++sub->currentLifetimeCount;
        if(sub->currentLifetimeCount > sub->lifeTimeCount) {
            UA_LOG_WARNING_SUBSCRIPTION(&server->config.logger, sub,
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

    /* Count the available notifications */
    UA_UInt32 notifications = (sub->publishingEnabled) ? sub->notificationQueueSize : 0;
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
        UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub, "Sending a KeepAlive");
    }

    /* We want to send a response, but cannot. Either because there is no queued
     * response or because the Subscription is detached from a Session or because
     * the SecureChannel for the Session is closed. */
    if(!pre || !sub->session || !sub->session->header.channel) {
        UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                                  "Want to send a publish response but cannot. "
                                  "The subscription is late.");
        UA_Subscription_isLate(sub);
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
                UA_LOG_WARNING_SUBSCRIPTION(&server->config.logger, sub,
                                            "Could not allocate memory for retransmission. "
                                            "The subscription is late.");

                UA_Subscription_isLate(sub);
                UA_Session_queuePublishReq(sub->session, pre, true); /* Re-enqueue */
                return;
            }
        }

        /* Prepare the response */
        UA_StatusCode retval =
            prepareNotificationMessage(server, sub, message, notifications);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SUBSCRIPTION(&server->config.logger, sub,
                                        "Could not prepare the notification message. "
                                        "The subscription is late.");
            /* If the retransmission queue is enabled a retransmission message is allocated */
            if(retransmission)
                UA_free(retransmission);
            UA_Subscription_isLate(sub);
            UA_Session_queuePublishReq(sub->session, pre, true); /* Re-enqueue */
            return;
        }
    }

    /* <-- The point of no return --> */

    /* Notifications remaining? */
    UA_Boolean moreNotifications = (sub->notificationQueueSize > 0);

    /* Set up the response */
    response->responseHeader.timestamp = UA_DateTime_now();
    response->subscriptionId = sub->subscriptionId;
    response->moreNotifications = moreNotifications;
    message->publishTime = response->responseHeader.timestamp;

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
    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                              "Sending out a publish response with %" PRIu32
                              " notifications", notifications);
    sendResponse(server, sub->session, sub->session->header.channel, pre->requestId,
                 (UA_Response*)response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    /* Reset subscription state to normal */
    sub->state = UA_SUBSCRIPTIONSTATE_NORMAL;
    sub->currentKeepAliveCount = 0;

    /* Free the response */
    if(retransmission)
        /* NotificationMessage was moved into retransmission queue */
        UA_NotificationMessage_init(&response->notificationMessage);
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

    /* Repeat sending responses if there are more notifications to send */
    if(moreNotifications)
        UA_Subscription_publish(server, sub);
}

UA_Boolean
UA_Session_reachedPublishReqLimit(UA_Server *server, UA_Session *session) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Reached number of publish request limit");

    /* Dequeue a response */
    UA_PublishResponseEntry *pre = UA_Session_dequeuePublishReq(session);

    /* Cannot publish without a response */
    if(!pre) {
        UA_LOG_FATAL_SESSION(&server->config.logger, session,
                             "No publish requests available");
        return false;
    }

    /* <-- The point of no return --> */

    UA_PublishResponse *response = &pre->response;
    UA_NotificationMessage *message = &response->notificationMessage;

    /* Set up the response. Note that this response has no related subscription id */
    response->responseHeader.timestamp = UA_DateTime_now();
    response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYPUBLISHREQUESTS;
    response->subscriptionId = 0;
    response->moreNotifications = false;
    message->publishTime = response->responseHeader.timestamp;
    message->sequenceNumber = 0;
    response->availableSequenceNumbersSize = 0;

    /* Send the response */
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Sending out a publish response triggered by too many publish requests");
    sendResponse(server, session, session->header.channel, pre->requestId,
                 (UA_Response*)response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    /* Free the response */
    UA_Array_delete(response->results, response->resultsSize, &UA_TYPES[UA_TYPES_UINT32]);
    UA_free(pre); /* no need for UA_PublishResponse_clear */

    return true;
}

UA_StatusCode
Subscription_registerPublishCallback(UA_Server *server, UA_Subscription *sub) {
    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                              "Register subscription publishing callback");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(sub->publishCallbackId > 0)
        return UA_STATUSCODE_GOOD;

    UA_StatusCode retval =
        addRepeatedCallback(server, (UA_ServerCallback)publishCallback,
                            sub, sub->publishingInterval, &sub->publishCallbackId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_assert(sub->publishCallbackId > 0);
    return UA_STATUSCODE_GOOD;
}

void
Subscription_unregisterPublishCallback(UA_Server *server, UA_Subscription *sub) {
    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                              "Unregister subscription publishing callback");

    if(sub->publishCallbackId == 0)
        return;

    removeCallback(server, sub->publishCallbackId);
    sub->publishCallbackId = 0;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
