/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2018, 2022 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016-2017 (c) Florian Palm
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2017 (c) Mattias Bornhager
 *    Copyright 2017 (c) Henrik Norrman
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 *    Copyright 2017-2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

static void
setPublishingEnabled(UA_Subscription *sub, UA_Boolean publishingEnabled) {
    if(sub->publishingEnabled == publishingEnabled)
        return;

    sub->publishingEnabled = publishingEnabled;

#ifdef UA_ENABLE_DIAGNOSTICS
    if(publishingEnabled)
        sub->enableCount++;
    else
        sub->disableCount++;
#endif
}

static void
setSubscriptionSettings(UA_Server *server, UA_Subscription *subscription,
                        UA_Double requestedPublishingInterval,
                        UA_UInt32 requestedLifetimeCount,
                        UA_UInt32 requestedMaxKeepAliveCount,
                        UA_UInt32 maxNotificationsPerPublish,
                        UA_Byte priority) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* re-parameterize the subscription */
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.publishingIntervalLimits,
                               requestedPublishingInterval,
                               subscription->publishingInterval);
    /* check for nan*/
    if(requestedPublishingInterval != requestedPublishingInterval)
        subscription->publishingInterval = server->config.publishingIntervalLimits.min;
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.keepAliveCountLimits,
                               requestedMaxKeepAliveCount, subscription->maxKeepAliveCount);
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.lifeTimeCountLimits,
                               requestedLifetimeCount, subscription->lifeTimeCount);
    if(subscription->lifeTimeCount < 3 * subscription->maxKeepAliveCount)
        subscription->lifeTimeCount = 3 * subscription->maxKeepAliveCount;
    subscription->notificationsPerPublish = maxNotificationsPerPublish;
    if(maxNotificationsPerPublish == 0 ||
       maxNotificationsPerPublish > server->config.maxNotificationsPerPublish)
        subscription->notificationsPerPublish = server->config.maxNotificationsPerPublish;
    subscription->priority = priority;
}

void
Service_CreateSubscription(UA_Server *server, UA_Session *session,
                           const UA_CreateSubscriptionRequest *request,
                           UA_CreateSubscriptionResponse *response) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Check limits for the number of subscriptions */
    if(((server->config.maxSubscriptions != 0) &&
        (server->subscriptionsSize >= server->config.maxSubscriptions)) ||
       ((server->config.maxSubscriptionsPerSession != 0) &&
        (session->subscriptionsSize >= server->config.maxSubscriptionsPerSession))) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYSUBSCRIPTIONS;
        return;
    }

    /* Create the subscription */
    UA_Subscription *sub= UA_Subscription_new();
    if(!sub) {
        UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                             "Processing CreateSubscriptionRequest failed");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* Set the subscription parameters */
    setSubscriptionSettings(server, sub, request->requestedPublishingInterval,
                            request->requestedLifetimeCount,
                            request->requestedMaxKeepAliveCount,
                            request->maxNotificationsPerPublish, request->priority);
    setPublishingEnabled(sub, request->publishingEnabled);
    sub->currentKeepAliveCount = sub->maxKeepAliveCount; /* set settings first */

    /* Assign the SubscriptionId */
    sub->subscriptionId = ++server->lastSubscriptionId;

    /* Register the cyclic callback */
    UA_StatusCode retval = Subscription_registerPublishCallback(server, sub);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_SESSION(&server->config.logger, sub->session,
                             "Subscription %" PRIu32 " | "
                             "Could not register publish callback with error code %s",
                             sub->subscriptionId, UA_StatusCode_name(retval));
        response->responseHeader.serviceResult = retval;
        UA_Subscription_delete(server, sub);
        return;
    }

    /* Register the subscription in the server */
    LIST_INSERT_HEAD(&server->subscriptions, sub, serverListEntry);
    server->subscriptionsSize++;

    /* Update the server statistics */
    server->serverDiagnosticsSummary.currentSubscriptionCount++;
    server->serverDiagnosticsSummary.cumulatedSubscriptionCount++;

    /* Attach the Subscription to the session */
    UA_Session_attachSubscription(session, sub);

    /* Prepare the response */
    response->subscriptionId = sub->subscriptionId;
    response->revisedPublishingInterval = sub->publishingInterval;
    response->revisedLifetimeCount = sub->lifeTimeCount;
    response->revisedMaxKeepAliveCount = sub->maxKeepAliveCount;

#ifdef UA_ENABLE_DIAGNOSTICS
    createSubscriptionObject(server, session, sub);
#endif

    UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, sub,
                             "Subscription created (Publishing interval %.2fms, "
                             "max %lu notifications per publish)",
                             sub->publishingInterval,
                             (long unsigned)sub->notificationsPerPublish);}

void
Service_ModifySubscription(UA_Server *server, UA_Session *session,
                           const UA_ModifySubscriptionRequest *request,
                           UA_ModifySubscriptionResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing ModifySubscriptionRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_Subscription *sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Store the old publishing interval */
    UA_Double oldPublishingInterval = sub->publishingInterval;
    UA_Byte oldPriority = sub->priority;

    /* Change the Subscription settings */
    setSubscriptionSettings(server, sub, request->requestedPublishingInterval,
                            request->requestedLifetimeCount,
                            request->requestedMaxKeepAliveCount,
                            request->maxNotificationsPerPublish, request->priority);

    /* Reset the subscription lifetime */
    sub->currentLifetimeCount = 0;

    /* Change the repeated callback to the new interval. This cannot fail as the
     * CallbackId must exist. */
    if(sub->publishCallbackId > 0 &&
       sub->publishingInterval != oldPublishingInterval)
        changeRepeatedCallbackInterval(server, sub->publishCallbackId,
                                       sub->publishingInterval);

    /* If the priority has changed, re-enter the subscription to the
     * priority-ordered queue in the session. */
    if(oldPriority != sub->priority) {
        UA_Session_detachSubscription(server, session, sub, false);
        UA_Session_attachSubscription(session, sub);
    }

    /* Set the response */
    response->revisedPublishingInterval = sub->publishingInterval;
    response->revisedLifetimeCount = sub->lifeTimeCount;
    response->revisedMaxKeepAliveCount = sub->maxKeepAliveCount;

    /* Update the diagnostics statistics */
#ifdef UA_ENABLE_DIAGNOSTICS
    sub->modifyCount++;
#endif
}

static void
Operation_SetPublishingMode(UA_Server *server, UA_Session *session,
                            const UA_Boolean *publishingEnabled,
                            const UA_UInt32 *subscriptionId,
                            UA_StatusCode *result) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, *subscriptionId);
    if(!sub) {
        *result = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    sub->currentLifetimeCount = 0; /* Reset the subscription lifetime */
    setPublishingEnabled(sub, *publishingEnabled); /* Set the publishing mode */
}

void
Service_SetPublishingMode(UA_Server *server, UA_Session *session,
                          const UA_SetPublishingModeRequest *request,
                          UA_SetPublishingModeResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing SetPublishingModeRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_Boolean publishingEnabled = request->publishingEnabled; /* request is const */
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation)Operation_SetPublishingMode,
                                           &publishingEnabled,
                                           &request->subscriptionIdsSize,
                                           &UA_TYPES[UA_TYPES_UINT32],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_STATUSCODE]);
}

UA_StatusCode
Service_Publish(UA_Server *server, UA_Session *session,
                const UA_PublishRequest *request, UA_UInt32 requestId) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing PublishRequest with RequestId %u", requestId);
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Return an error if the session has no subscription */
    if(TAILQ_EMPTY(&session->subscriptions)) {
        sendServiceFault(session->header.channel, requestId,
                         request->requestHeader.requestHandle,
                         UA_STATUSCODE_BADNOSUBSCRIPTION);
        return UA_STATUSCODE_BADNOSUBSCRIPTION;
    }

    /* Handle too many subscriptions to free resources before trying to allocate
     * resources for the new publish request. If the limit has been reached the
     * oldest publish request shall be responded */
    if((server->config.maxPublishReqPerSession != 0) &&
       (session->responseQueueSize >= server->config.maxPublishReqPerSession)) {
        if(!UA_Session_reachedPublishReqLimit(server, session)) {
            sendServiceFault(session->header.channel, requestId,
                             request->requestHeader.requestHandle,
                             UA_STATUSCODE_BADINTERNALERROR);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    /* Allocate the response to store it in the retransmission queue */
    UA_PublishResponseEntry *entry = (UA_PublishResponseEntry *)
        UA_malloc(sizeof(UA_PublishResponseEntry));
    if(!entry) {
        sendServiceFault(session->header.channel, requestId,
                         request->requestHeader.requestHandle,
                         UA_STATUSCODE_BADOUTOFMEMORY);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Prepare the response */
    entry->requestId = requestId;
    UA_PublishResponse *response = &entry->response;
    UA_PublishResponse_init(response);
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;

    /* Allocate the results array to acknowledge the acknowledge */
    if(request->subscriptionAcknowledgementsSize > 0) {
        response->results = (UA_StatusCode *)
            UA_Array_new(request->subscriptionAcknowledgementsSize,
                         &UA_TYPES[UA_TYPES_STATUSCODE]);
        if(!response->results) {
            UA_free(entry);
            sendServiceFault(session->header.channel, requestId,
                             request->requestHeader.requestHandle,
                             UA_STATUSCODE_BADOUTOFMEMORY);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        response->resultsSize = request->subscriptionAcknowledgementsSize;
    }

    /* <--- A good StatusCode is returned from here on ---> */

    /* Delete Acknowledged Subscription Messages */
    for(size_t i = 0; i < request->subscriptionAcknowledgementsSize; ++i) {
        UA_SubscriptionAcknowledgement *ack = &request->subscriptionAcknowledgements[i];
        UA_Subscription *sub = UA_Session_getSubscriptionById(session, ack->subscriptionId);
        if(!sub) {
            response->results[i] = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
            UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                                 "Cannot process acknowledgements subscription %u" PRIu32,
                                 ack->subscriptionId);
            continue;
        }
        /* Remove the acked transmission from the retransmission queue */
        response->results[i] =
            UA_Subscription_removeRetransmissionMessage(sub, ack->sequenceNumber);
    }

    /* Queue the publish response. It will be dequeued in a repeated publish
     * callback. This can also be triggered right now for a late
     * subscription. */
    UA_Session_queuePublishReq(session, entry, false);
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Queued a publication message");

    /* If there are late subscriptions, the new publish request is used to
     * answer them immediately. Late subscriptions with higher priority are
     * considered earlier. However, a single subscription that generates many
     * notifications must not "starve" other late subscriptions. Hence we move
     * it to the end of the queue for the subscriptions of that priority. */
    UA_Subscription *late = NULL;
    TAILQ_FOREACH(late, &session->subscriptions, sessionListEntry) {
        if(late->state != UA_SUBSCRIPTIONSTATE_LATE)
            continue;

        UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, late,
                                  "Send PublishResponse on a late subscription");
        UA_Subscription_publish(server, late);

        /* The subscription was detached from the session during publish? */
        if(!late->session)
            break;

        /* Find the first element with smaller priority. If there is none,
         * insert at the end. */
        UA_Subscription *after = TAILQ_NEXT(late, sessionListEntry);
        while(after && after->priority >= late->priority)
            after = TAILQ_NEXT(after, sessionListEntry);

        TAILQ_REMOVE(&session->subscriptions, late, sessionListEntry);

        if(after)
            TAILQ_INSERT_BEFORE(after, late, sessionListEntry);
        else
            TAILQ_INSERT_TAIL(&session->subscriptions, late, sessionListEntry);

        break;
    }

    return UA_STATUSCODE_GOOD;
}

static void
Operation_DeleteSubscription(UA_Server *server, UA_Session *session, void *_,
                             const UA_UInt32 *subscriptionId, UA_StatusCode *result) {
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, *subscriptionId);
    if(!sub) {
        *result = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                             "Deleting Subscription with Id %" PRIu32
                             " failed with error code %s",
                             *subscriptionId, UA_StatusCode_name(*result));
        return;
    }

    UA_Subscription_delete(server, sub);
    *result = UA_STATUSCODE_GOOD;
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Subscription %" PRIu32 " | Subscription deleted",
                         *subscriptionId);
}

void
Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                            const UA_DeleteSubscriptionsRequest *request,
                            UA_DeleteSubscriptionsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing DeleteSubscriptionsRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_DeleteSubscription, NULL,
                  &request->subscriptionIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

void
Service_Republish(UA_Server *server, UA_Session *session,
                  const UA_RepublishRequest *request,
                  UA_RepublishResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing RepublishRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Get the subscription */
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    sub->currentLifetimeCount = 0;

    /* Update the subscription statistics */
#ifdef UA_ENABLE_DIAGNOSTICS
    sub->republishRequestCount++;
#endif

    /* Find the notification in the retransmission queue  */
    UA_NotificationMessageEntry *entry;
    TAILQ_FOREACH(entry, &sub->retransmissionQueue, listEntry) {
        if(entry->message.sequenceNumber == request->retransmitSequenceNumber)
            break;
    }
    if(!entry) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADMESSAGENOTAVAILABLE;
        return;
    }

    response->responseHeader.serviceResult =
        UA_NotificationMessage_copy(&entry->message, &response->notificationMessage);

    /* Update the subscription statistics for the case where we return a message */
#ifdef UA_ENABLE_DIAGNOSTICS
    sub->republishMessageCount++;
#endif
}

static UA_StatusCode
setTransferredSequenceNumbers(const UA_Subscription *sub, UA_TransferResult *result) {
    /* Allocate memory */
    result->availableSequenceNumbers = (UA_UInt32*)
        UA_Array_new(sub->retransmissionQueueSize, &UA_TYPES[UA_TYPES_UINT32]);
    if(!result->availableSequenceNumbers)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    result->availableSequenceNumbersSize = sub->retransmissionQueueSize;

    /* Copy over the sequence numbers */
    UA_NotificationMessageEntry *entry;
    size_t i = 0;
    TAILQ_FOREACH(entry, &sub->retransmissionQueue, listEntry) {
        result->availableSequenceNumbers[i] = entry->message.sequenceNumber;
        i++;
    }

    UA_assert(i == result->availableSequenceNumbersSize);

    return UA_STATUSCODE_GOOD;
}

static void
Operation_TransferSubscription(UA_Server *server, UA_Session *session,
                               const UA_Boolean *sendInitialValues,
                               const UA_UInt32 *subscriptionId,
                               UA_TransferResult *result) {
    /* Get the subscription. This requires a server-wide lookup instead of the
     * usual session-wide lookup. */
    UA_Subscription *sub = UA_Server_getSubscriptionById(server, *subscriptionId);
    if(!sub) {
        result->statusCode = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Update the diagnostics statistics */
#ifdef UA_ENABLE_DIAGNOSTICS
    sub->transferRequestCount++;
#endif

    /* Is this the same session? Return the sequence numbers and do nothing else. */
    UA_Session *oldSession = sub->session;
    if(oldSession == session) {
        result->statusCode = setTransferredSequenceNumbers(sub, result);
#ifdef UA_ENABLE_DIAGNOSTICS
        sub->transferredToSameClientCount++;
#endif
        return;
    }

    /* Check with AccessControl if the transfer is allowed */
    if(!server->config.accessControl.allowTransferSubscription ||
       !server->config.accessControl.
       allowTransferSubscription(server, &server->config.accessControl,
                                 oldSession ? &oldSession->sessionId : NULL,
                                 oldSession ? oldSession->sessionHandle : NULL,
                                 &session->sessionId, session->sessionHandle)) {
        result->statusCode = UA_STATUSCODE_BADUSERACCESSDENIED;
        return;
    }

    /* Check limits for the number of subscriptions for this Session */
    if((server->config.maxSubscriptionsPerSession != 0) &&
       (session->subscriptionsSize >= server->config.maxSubscriptionsPerSession)) {
        result->statusCode = UA_STATUSCODE_BADTOOMANYSUBSCRIPTIONS;
        return;
    }

    /* Allocate memory for the new subscription */
    UA_Subscription *newSub = (UA_Subscription*)UA_malloc(sizeof(UA_Subscription));
    if(!newSub) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* Set the available sequence numbers */
    result->statusCode = setTransferredSequenceNumbers(sub, result);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_free(newSub);
        return;
    }

    /* Create an identical copy of the Subscription struct. The original
     * subscription remains in place until a StatusChange notification has been
     * sent. The elements for lists and queues are moved over manually to ensure
     * that all backpointers are set correctly. */
    memcpy(newSub, sub, sizeof(UA_Subscription));

    /* Register cyclic publish callback */
    result->statusCode = Subscription_registerPublishCallback(server, newSub);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_Array_delete(result->availableSequenceNumbers,
                        sub->retransmissionQueueSize, &UA_TYPES[UA_TYPES_UINT32]);
        result->availableSequenceNumbers = NULL;
        result->availableSequenceNumbersSize = 0;
        UA_free(newSub);
        return;
    }

    /* <-- The point of no return --> */

    /* Move over the MonitoredItems and adjust the backpointers */
    LIST_INIT(&newSub->monitoredItems);
    UA_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &sub->monitoredItems, listEntry, mon_tmp) {
        LIST_REMOVE(mon, listEntry);
        mon->subscription = newSub;
        LIST_INSERT_HEAD(&newSub->monitoredItems, mon, listEntry);
    }
    sub->monitoredItemsSize = 0;

    /* Move over the notification queue */
    TAILQ_INIT(&newSub->notificationQueue);
    UA_Notification *nn;
    TAILQ_FOREACH(nn, &sub->notificationQueue, globalEntry) {
        TAILQ_REMOVE(&sub->notificationQueue, nn, globalEntry);
        TAILQ_INSERT_TAIL(&newSub->notificationQueue, nn, globalEntry);
    }
    sub->notificationQueueSize = 0;
    sub->dataChangeNotifications = 0;
    sub->eventNotifications = 0;

    TAILQ_INIT(&newSub->retransmissionQueue);
    UA_NotificationMessageEntry *nme, *nme_tmp;
    TAILQ_FOREACH_SAFE(nme, &sub->retransmissionQueue, listEntry, nme_tmp) {
        TAILQ_REMOVE(&sub->retransmissionQueue, nme, listEntry);
        TAILQ_INSERT_TAIL(&newSub->retransmissionQueue, nme, listEntry);
        if(oldSession)
            oldSession->totalRetransmissionQueueSize -= 1;
        sub->retransmissionQueueSize -= 1;
    }
    UA_assert(sub->retransmissionQueueSize == 0);
    sub->retransmissionQueueSize = 0;

    /* Add to the server */
    UA_assert(newSub->subscriptionId == sub->subscriptionId);
    LIST_INSERT_HEAD(&server->subscriptions, newSub, serverListEntry);
    server->subscriptionsSize++;

    /* Attach to the session */
    UA_Session_attachSubscription(session, newSub);

    UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, newSub, "Transferred to this Session");

    /* Set StatusChange in the original subscription and force publish. This
     * also removes the Subscription, even if there was no PublishResponse
     * queued to send a StatusChangeNotification. */
    sub->statusChange = UA_STATUSCODE_GOODSUBSCRIPTIONTRANSFERRED;
    UA_Subscription_publish(server, sub);
    UA_assert(sub->publishCallbackId == 0);

    /* Create notifications with the current values */
    if(*sendInitialValues) {
        LIST_FOREACH(mon, &newSub->monitoredItems, listEntry) {

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
            UA_MonitoredItem_createDataChangeNotification(server, newSub, mon,
                                                          &mon->lastValue);
        }
    }

    /* Do not update the statistics for the number of Subscriptions here. The
     * fact that we duplicate the subscription and move over the content is just
     * an implementtion detail.
     * server->serverDiagnosticsSummary.currentSubscriptionCount++;
     * server->serverDiagnosticsSummary.cumulatedSubscriptionCount++;
     *
     * Update the diagnostics statistics: */
#ifdef UA_ENABLE_DIAGNOSTICS
    if(oldSession &&
       UA_order(&oldSession->clientDescription,
                &session->clientDescription,
                &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]) == UA_ORDER_EQ)
        sub->transferredToSameClientCount++;
    else
        sub->transferredToAltClientCount++;
#endif

    /* Immediately try to publish on the new Subscription. This might put it
     * into the "late subscription" mode. */
    UA_Subscription_publish(server, newSub);
}

void Service_TransferSubscriptions(UA_Server *server, UA_Session *session,
                                   const UA_TransferSubscriptionsRequest *request,
                                   UA_TransferSubscriptionsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing TransferSubscriptionsRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_TransferSubscription,
                  &request->sendInitialValues,
                  &request->subscriptionIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_TRANSFERRESULT]);
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
