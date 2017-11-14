/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_subscription.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

UA_Subscription *
UA_Subscription_new(UA_Session *session, UA_UInt32 subscriptionID) {
    /* Allocate the memory */
    UA_Subscription *newItem =
        (UA_Subscription*)UA_calloc(1, sizeof(UA_Subscription));
    if(!newItem)
        return NULL;

    /* Remaining members are covered by calloc zeroing out the memory */
    newItem->session = session;
    newItem->subscriptionID = subscriptionID;
    newItem->numMonitoredItems = 0;
    newItem->state = UA_SUBSCRIPTIONSTATE_NORMAL; /* The first publish response is sent immediately */
    TAILQ_INIT(&newItem->retransmissionQueue);
    return newItem;
}

void
UA_Subscription_deleteMembers(UA_Subscription *subscription, UA_Server *server) {
    Subscription_unregisterPublishCallback(server, subscription);

    /* Delete monitored Items */
    UA_MonitoredItem *mon, *tmp_mon;
    LIST_FOREACH_SAFE(mon, &subscription->monitoredItems,
                      listEntry, tmp_mon) {
        LIST_REMOVE(mon, listEntry);
        MonitoredItem_delete(server, mon);
    }

    /* Delete Retransmission Queue */
    UA_NotificationMessageEntry *nme, *nme_tmp;
    TAILQ_FOREACH_SAFE(nme, &subscription->retransmissionQueue,
                       listEntry, nme_tmp) {
        TAILQ_REMOVE(&subscription->retransmissionQueue, nme, listEntry);
        UA_NotificationMessage_deleteMembers(&nme->message);
        UA_free(nme);
    }
    subscription->retransmissionQueueSize = 0;
}

UA_MonitoredItem *
UA_Subscription_getMonitoredItem(UA_Subscription *sub,
                                 UA_UInt32 monitoredItemID) {
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
        if(mon->itemId == monitoredItemID)
            break;
    }
    return mon;
}

UA_StatusCode
UA_Subscription_deleteMonitoredItem(UA_Server *server, UA_Subscription *sub,
                                    UA_UInt32 monitoredItemID) {
    /* Find the MonitoredItem */
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
        if(mon->itemId == monitoredItemID)
            break;
    }
    if(!mon)
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;

    /* Remove the MonitoredItem */
    LIST_REMOVE(mon, listEntry);
    MonitoredItem_delete(server, mon);
    sub->numMonitoredItems--;
    return UA_STATUSCODE_GOOD;
}

void
UA_Subscription_addMonitoredItem(UA_Subscription *sub, UA_MonitoredItem *newMon) {
    sub->numMonitoredItems++;
    LIST_INSERT_HEAD(&sub->monitoredItems, newMon, listEntry);
}

UA_UInt32
UA_Subscription_getNumMonitoredItems(UA_Subscription *sub) {
    return sub->numMonitoredItems;
}

static size_t
countQueuedNotifications(UA_Subscription *sub,
                         UA_Boolean *moreNotifications) {
    if(!sub->publishingEnabled)
        return 0;

    size_t notifications = 0;
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
        MonitoredItem_queuedValue *qv;
        TAILQ_FOREACH(qv, &mon->queue, listEntry) {
            if(notifications >= sub->notificationsPerPublish) {
                *moreNotifications = true;
                break;
            }
            ++notifications;
        }
    }
    return notifications;
}

static void
UA_Subscription_addRetransmissionMessage(UA_Server *server, UA_Subscription *sub,
                                         UA_NotificationMessageEntry *entry) {
    /* Release the oldest entry if there is not enough space */
    if(server->config.maxRetransmissionQueueSize > 0 &&
       sub->retransmissionQueueSize >= server->config.maxRetransmissionQueueSize) {
        UA_NotificationMessageEntry *lastentry =
            TAILQ_LAST(&sub->retransmissionQueue, ListOfNotificationMessages);
        TAILQ_REMOVE(&sub->retransmissionQueue, lastentry, listEntry);
        --sub->retransmissionQueueSize;
        UA_NotificationMessage_deleteMembers(&lastentry->message);
        UA_free(lastentry);
    }

    /* Add entry */
    TAILQ_INSERT_HEAD(&sub->retransmissionQueue, entry, listEntry);
    ++sub->retransmissionQueueSize;
}

UA_StatusCode
UA_Subscription_removeRetransmissionMessage(UA_Subscription *sub,
                                            UA_UInt32 sequenceNumber) {
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
    UA_NotificationMessage_deleteMembers(&entry->message);
    UA_free(entry);
    return UA_STATUSCODE_GOOD;
}

static UA_MonitoredItem *
selectFirstMonToIterate(UA_Subscription *sub) {
    UA_MonitoredItem *mon = LIST_FIRST(&sub->monitoredItems);
    if(sub->lastSendMonitoredItemId > 0) {
        while(mon) {
            if(mon->itemId == sub->lastSendMonitoredItemId)
                break;
            mon = LIST_NEXT(mon, listEntry);
        }
        if(!mon)
            mon = LIST_FIRST(&sub->monitoredItems);
    }
    return mon;
}

/* Iterate over the monitoreditems of the subscription, starting at mon, and
 * move notifications into the response. */
static void
moveNotificationsFromMonitoredItems(UA_Subscription *sub, UA_MonitoredItem *mon,
                                    UA_MonitoredItemNotification *mins, size_t minsSize,
                                    size_t *pos) {
    MonitoredItem_queuedValue *qv, *qv_tmp;
    while(mon) {
        sub->lastSendMonitoredItemId = mon->itemId;
        TAILQ_FOREACH_SAFE(qv, &mon->queue, listEntry, qv_tmp) {
            if(*pos >= minsSize)
                return;
            UA_MonitoredItemNotification *min = &mins[*pos];
            min->clientHandle = qv->clientHandle;
            min->value = qv->value;
            TAILQ_REMOVE(&mon->queue, qv, listEntry);
            UA_free(qv);
            --mon->currentQueueSize;
            ++(*pos);
        }
        mon = LIST_NEXT(mon, listEntry);
    }
}

static UA_StatusCode
prepareNotificationMessage(UA_Subscription *sub,
                           UA_NotificationMessage *message,
                           size_t notifications) {
    /* Array of ExtensionObject to hold different kinds of notifications
     * (currently only DataChangeNotifications) */
    message->notificationData = UA_ExtensionObject_new();
    if(!message->notificationData)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    message->notificationDataSize = 1;

    /* Allocate Notification */
    UA_DataChangeNotification *dcn = UA_DataChangeNotification_new();
    if(!dcn) {
        UA_NotificationMessage_deleteMembers(message);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_ExtensionObject *data = message->notificationData;
    data->encoding = UA_EXTENSIONOBJECT_DECODED;
    data->content.decoded.data = dcn;
    data->content.decoded.type = &UA_TYPES[UA_TYPES_DATACHANGENOTIFICATION];

    /* Allocate array of notifications */
    dcn->monitoredItems = (UA_MonitoredItemNotification *)
        UA_Array_new(notifications,
                     &UA_TYPES[UA_TYPES_MONITOREDITEMNOTIFICATION]);
    if(!dcn->monitoredItems) {
        UA_NotificationMessage_deleteMembers(message);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    dcn->monitoredItemsSize = notifications;

    /* Move notifications into the response .. the point of no return */

    /* Select the first monitoredItem or the first monitoreditem after the last
     * that was processed. */
    UA_MonitoredItem *mon = selectFirstMonToIterate(sub);

    /* Move notifications into the response */
    size_t l = 0;
    moveNotificationsFromMonitoredItems(sub, mon, dcn->monitoredItems, notifications, &l);
    if(l < notifications) {
        /* Not done. We skipped MonitoredItems. Restart at the beginning. */
        moveNotificationsFromMonitoredItems(sub, LIST_FIRST(&sub->monitoredItems),
                                            dcn->monitoredItems, notifications, &l);
    }

    return UA_STATUSCODE_GOOD;
}

void
UA_Subscription_publishCallback(UA_Server *server, UA_Subscription *sub) {
    UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                         "Subscription %u | Publish Callback",
                         sub->subscriptionID);

    /* Count the available notifications */
    UA_Boolean moreNotifications = false;
    size_t notifications = countQueuedNotifications(sub, &moreNotifications);

    /* Return if nothing to do */
    if(notifications == 0) {
        ++sub->currentKeepAliveCount;
        if(sub->currentKeepAliveCount < sub->maxKeepAliveCount)
            return;
        UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                             "Subscription %u | Sending a KeepAlive",
                             sub->subscriptionID);
    }

    /* Check if the securechannel is valid */
    UA_SecureChannel *channel = sub->session->channel;
    if(!channel)
        return;

    /* Dequeue a response */
    UA_PublishResponseEntry *pre = UA_Session_getPublishReq(sub->session);

    /* Cannot publish without a response */
    if(!pre) {
        UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                             "Subscription %u | Cannot send a publish "
                             "response since the publish queue is empty",
                             sub->subscriptionID);
        if(sub->state != UA_SUBSCRIPTIONSTATE_LATE) {
            sub->state = UA_SUBSCRIPTIONSTATE_LATE;
        } else {
            ++sub->currentLifetimeCount;
            if(sub->currentLifetimeCount > sub->lifeTimeCount) {
                UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                                     "Subscription %u | End of lifetime "
                                     "for subscription", sub->subscriptionID);
                UA_Session_deleteSubscription(server, sub->session,
                                              sub->subscriptionID);
            }
        }
        return;
    }

    UA_PublishResponse *response = &pre->response;
    UA_NotificationMessage *message = &response->notificationMessage;
    UA_NotificationMessageEntry *retransmission = NULL;
    if(notifications > 0) {
        /* Allocate the retransmission entry */
        retransmission = (UA_NotificationMessageEntry*)
            UA_malloc(sizeof(UA_NotificationMessageEntry));
        if(!retransmission) {
            UA_LOG_WARNING_SESSION(server->config.logger, sub->session,
                                   "Subscription %u | Could not allocate memory "
                                   "for retransmission", sub->subscriptionID);
            return;
        }

        /* Prepare the response */
        UA_StatusCode retval =
            prepareNotificationMessage(sub, message, notifications);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_SESSION(server->config.logger, sub->session,
                                   "Subscription %u | Could not prepare the "
                                   "notification message", sub->subscriptionID);
            UA_free(retransmission);
            return;
        }
    }

    /* <-- The point of no return --> */

    /* Remove the response from the response queue */
    UA_Session_removePublishReq(sub->session, pre);

    /* Set up the response */
    response->responseHeader.timestamp = UA_DateTime_now();
    response->subscriptionId = sub->subscriptionID;
    response->moreNotifications = moreNotifications;
    message->publishTime = response->responseHeader.timestamp;
    if(notifications == 0) {
        /* Send sequence number for the next notification */
        message->sequenceNumber = sub->sequenceNumber + 1;
    } else {
        /* Increase the sequence number */
        message->sequenceNumber = ++sub->sequenceNumber;

        /* Put the notification message into the retransmission queue. This
         * needs to be done here, so that the message itself is included in the
         * available sequence numbers for acknowledgement. */
        retransmission->message = response->notificationMessage;
        UA_Subscription_addRetransmissionMessage(server, sub, retransmission);
    }

    /* Get the available sequence numbers from the retransmission queue */
    size_t available = sub->retransmissionQueueSize;
    if(available > 0) {
        response->availableSequenceNumbers =
            (UA_UInt32*)UA_alloca(available * sizeof(UA_UInt32));
        response->availableSequenceNumbersSize = available;
        size_t i = 0;
        UA_NotificationMessageEntry *nme;
        TAILQ_FOREACH(nme, &sub->retransmissionQueue, listEntry) {
            response->availableSequenceNumbers[i] = nme->message.sequenceNumber;
            ++i;
        }
    }

    /* Send the response */
    UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                         "Subscription %u | Sending out a publish response "
                         "with %u notifications", sub->subscriptionID,
                         (UA_UInt32)notifications);
    UA_SecureChannel_sendSymmetricMessage(sub->session->channel, pre->requestId,
                                          UA_MESSAGETYPE_MSG, response,
                                          &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    /* Reset subscription state to normal. */
    sub->state = UA_SUBSCRIPTIONSTATE_NORMAL;
    sub->currentKeepAliveCount = 0;
    sub->currentLifetimeCount = 0;

    /* Free the response */
    UA_Array_delete(response->results, response->resultsSize,
                    &UA_TYPES[UA_TYPES_UINT32]);
    UA_free(pre); /* no need for UA_PublishResponse_deleteMembers */

    if(!moreNotifications) {
        /* All notifications were sent. The next time, just start at the first
         * monitoreditem. */
        sub->lastSendMonitoredItemId = 0;
    } else {
        /* Repeat sending responses right away if there are more notifications
         * to send */
        UA_Subscription_publishCallback(server, sub);
    }
}

UA_Boolean
UA_Subscription_reachedPublishReqLimit(UA_Server *server,  UA_Session *session) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Reached number of publish request limit");


    /* Dequeue a response */
    UA_PublishResponseEntry *pre = UA_Session_getPublishReq(session);

    /* Cannot publish without a response */
    if(!pre) {
        UA_LOG_FATAL_SESSION(server->config.logger, session, "No publish requests available");
        return false;
    }

    UA_PublishResponse *response = &pre->response;
    UA_NotificationMessage *message = &response->notificationMessage;

    /* <-- The point of no return --> */

    /* Remove the response from the response queue */
    UA_Session_removePublishReq(session, pre);

    /* Set up the response. Note that this response has no related subscription id */
    response->responseHeader.timestamp = UA_DateTime_now();
    response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYPUBLISHREQUESTS;
    response->subscriptionId = 0;
    response->moreNotifications = false;
    message->publishTime = response->responseHeader.timestamp;
    message->sequenceNumber = 0;
    response->availableSequenceNumbersSize = 0;

    /* Send the response */
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Sending out a publish response triggered by too many publish requests");
    UA_SecureChannel_sendSymmetricMessage(session->channel, pre->requestId,
                                          UA_MESSAGETYPE_MSG, response,
                                          &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);

    /* Free the response */
    UA_Array_delete(response->results, response->resultsSize,
                    &UA_TYPES[UA_TYPES_UINT32]);
    UA_free(pre); /* no need for UA_PublishResponse_deleteMembers */

    return true;
}

UA_StatusCode
Subscription_registerPublishCallback(UA_Server *server, UA_Subscription *sub) {
    UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                         "Subscription %u | Register subscription "
                         "publishing callback", sub->subscriptionID);

    if(sub->publishCallbackIsRegistered)
        return UA_STATUSCODE_GOOD;

    UA_StatusCode retval =
        UA_Server_addRepeatedCallback(server,
                  (UA_ServerCallback)UA_Subscription_publishCallback,
                  sub, (UA_UInt32)sub->publishingInterval,
                  &sub->publishCallbackId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    sub->publishCallbackIsRegistered = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
Subscription_unregisterPublishCallback(UA_Server *server, UA_Subscription *sub) {
    UA_LOG_DEBUG_SESSION(server->config.logger, sub->session,
                         "Subscription %u | Unregister subscription "
                         "publishing callback", sub->subscriptionID);

    if(!sub->publishCallbackIsRegistered)
        return UA_STATUSCODE_GOOD;

    UA_StatusCode retval =
        UA_Server_removeRepeatedCallback(server, sub->publishCallbackId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    sub->publishCallbackIsRegistered = false;
    return UA_STATUSCODE_GOOD;
}

/* When the session has publish requests stored but the last subscription is
 * deleted... Send out empty responses */
void
UA_Subscription_answerPublishRequestsNoSubscription(UA_Server *server,
                                                    UA_Session *session) {
    /* No session or there are remaining subscriptions */
    if(!session || LIST_FIRST(&session->serverSubscriptions))
        return;

    /* Send a response for every queued request */
    UA_PublishResponseEntry *pre;
    while((pre = UA_Session_getPublishReq(session))) {
        UA_Session_removePublishReq(session, pre);
        UA_PublishResponse *response = &pre->response;
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOSUBSCRIPTION;
        response->responseHeader.timestamp = UA_DateTime_now();
        UA_SecureChannel_sendSymmetricMessage(session->channel, pre->requestId,
                                              UA_MESSAGETYPE_MSG, response,
                                              &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
        UA_PublishResponse_deleteMembers(response);
        UA_free(pre);
    }
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
