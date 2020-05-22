/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
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

static UA_StatusCode
setSubscriptionSettings(UA_Server *server, UA_Subscription *subscription,
                        UA_Double requestedPublishingInterval,
                        UA_UInt32 requestedLifetimeCount,
                        UA_UInt32 requestedMaxKeepAliveCount,
                        UA_UInt32 maxNotificationsPerPublish, UA_Byte priority) {
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* deregister the callback if required */
    Subscription_unregisterPublishCallback(server, subscription);

    /* re-parameterize the subscription */
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.publishingIntervalLimits,
                               requestedPublishingInterval, subscription->publishingInterval);
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

    UA_StatusCode retval = Subscription_registerPublishCallback(server, subscription);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_SESSION(&server->config.logger, subscription->session,
                             "Subscription %" PRIu32 " | Could not register publish callback with error code %s",
                             subscription->subscriptionId, UA_StatusCode_name(retval));
    }
    return retval;
}

void
Service_CreateSubscription(UA_Server *server, UA_Session *session,
                           const UA_CreateSubscriptionRequest *request,
                           UA_CreateSubscriptionResponse *response) {
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* Check limits for the number of subscriptions */
    if(((server->config.maxSubscriptions != 0) &&
        (server->numSubscriptions >= server->config.maxSubscriptions)) ||
       ((server->config.maxSubscriptionsPerSession != 0) &&
        (session->numSubscriptions >= server->config.maxSubscriptionsPerSession))) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYSUBSCRIPTIONS;
        return;
    }

    /* Create the subscription */
    UA_Subscription *newSubscription = UA_Subscription_new(session, response->subscriptionId);
    if(!newSubscription) {
        UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                             "Processing CreateSubscriptionRequest failed");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    UA_Session_addSubscription(server, session, newSubscription); /* Also assigns the subscription id */

    /* Set the subscription parameters */
    newSubscription->publishingEnabled = request->publishingEnabled;
    UA_StatusCode retval = setSubscriptionSettings(server, newSubscription, request->requestedPublishingInterval,
                                                   request->requestedLifetimeCount, request->requestedMaxKeepAliveCount,
                                                   request->maxNotificationsPerPublish, request->priority);

    if(retval != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = retval;
        return;
    }

    newSubscription->currentKeepAliveCount = newSubscription->maxKeepAliveCount; /* set settings first */

    /* Prepare the response */
    response->subscriptionId = newSubscription->subscriptionId;
    response->revisedPublishingInterval = newSubscription->publishingInterval;
    response->revisedLifetimeCount = newSubscription->lifeTimeCount;
    response->revisedMaxKeepAliveCount = newSubscription->maxKeepAliveCount;

    UA_LOG_INFO_SESSION(&server->config.logger, session, "Subscription %" PRIu32 " | "
                        "Created the Subscription with a publishing interval of %.2f ms",
                        response->subscriptionId, newSubscription->publishingInterval);
}

void
Service_ModifySubscription(UA_Server *server, UA_Session *session,
                           const UA_ModifySubscriptionRequest *request,
                           UA_ModifySubscriptionResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing ModifySubscriptionRequest");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    UA_Subscription *sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    UA_StatusCode retval = setSubscriptionSettings(server, sub, request->requestedPublishingInterval,
                                                   request->requestedLifetimeCount, request->requestedMaxKeepAliveCount,
                                                   request->maxNotificationsPerPublish, request->priority);

    if(retval != UA_STATUSCODE_GOOD) {
        response->responseHeader.serviceResult = retval;
        return;
    }

    sub->currentLifetimeCount = 0; /* Reset the subscription lifetime */
    response->revisedPublishingInterval = sub->publishingInterval;
    response->revisedLifetimeCount = sub->lifeTimeCount;
    response->revisedMaxKeepAliveCount = sub->maxKeepAliveCount;
}

static void
Operation_SetPublishingMode(UA_Server *server, UA_Session *session,
                            const UA_Boolean *publishingEnabled, const UA_UInt32 *subscriptionId,
                            UA_StatusCode *result) {
    UA_LOCK_ASSERT(server->serviceMutex, 1);
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, *subscriptionId);
    if(!sub) {
        *result = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    sub->currentLifetimeCount = 0; /* Reset the subscription lifetime */
    sub->publishingEnabled = *publishingEnabled; /* Set the publishing mode */
}

void
Service_SetPublishingMode(UA_Server *server, UA_Session *session,
                          const UA_SetPublishingModeRequest *request,
                          UA_SetPublishingModeResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing SetPublishingModeRequest");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    UA_Boolean publishingEnabled = request->publishingEnabled; /* request is const */
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_SetPublishingMode,
                                           &publishingEnabled,
                                           &request->subscriptionIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

void
Service_Publish(UA_Server *server, UA_Session *session,
                const UA_PublishRequest *request, UA_UInt32 requestId) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing PublishRequest");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* Return an error if the session has no subscription */
    if(LIST_EMPTY(&session->serverSubscriptions)) {
        sendServiceFault(session->header.channel, requestId, request->requestHeader.requestHandle,
                         &UA_TYPES[UA_TYPES_PUBLISHRESPONSE], UA_STATUSCODE_BADNOSUBSCRIPTION);
        return;
    }

    /* Handle too many subscriptions to free resources before trying to allocate
     * resources for the new publish request. If the limit has been reached the
     * oldest publish request shall be responded */
    if((server->config.maxPublishReqPerSession != 0) &&
       (session->numPublishReq >= server->config.maxPublishReqPerSession)) {
        if(!UA_Subscription_reachedPublishReqLimit(server, session)) {
            sendServiceFault(session->header.channel, requestId, request->requestHeader.requestHandle,
                             &UA_TYPES[UA_TYPES_PUBLISHRESPONSE], UA_STATUSCODE_BADINTERNALERROR);
            return;
        }
    }

    /* Allocate the response to store it in the retransmission queue */
    UA_PublishResponseEntry *entry = (UA_PublishResponseEntry *)
        UA_malloc(sizeof(UA_PublishResponseEntry));
    if(!entry) {
        sendServiceFault(session->header.channel, requestId, request->requestHeader.requestHandle,
                         &UA_TYPES[UA_TYPES_PUBLISHRESPONSE], UA_STATUSCODE_BADOUTOFMEMORY);
        return;
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
            sendServiceFault(session->header.channel, requestId, request->requestHeader.requestHandle,
                             &UA_TYPES[UA_TYPES_PUBLISHRESPONSE], UA_STATUSCODE_BADOUTOFMEMORY);
            return;
        }
        response->resultsSize = request->subscriptionAcknowledgementsSize;
    }

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
        response->results[i] = UA_Subscription_removeRetransmissionMessage(sub, ack->sequenceNumber);
    }

    /* Queue the publish response. It will be dequeued in a repeated publish
     * callback. This can also be triggered right now for a late
     * subscription. */
    UA_Session_queuePublishReq(session, entry, false);
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Queued a publication message");

    /* If there are late subscriptions, the new publish request is used to
     * answer them immediately. However, a single subscription that generates
     * many notifications must not "starve" other late subscriptions. Therefore
     * we keep track of the last subscription that got preferential treatment.
     * We start searching for late subscriptions **after** the last one. */

    UA_Subscription *immediate = NULL;
    if(session->lastSeenSubscriptionId > 0) {
        LIST_FOREACH(immediate, &session->serverSubscriptions, listEntry) {
            if(immediate->subscriptionId == session->lastSeenSubscriptionId) {
                immediate = LIST_NEXT(immediate, listEntry);
                break;
            }
        }
    }

    /* If no entry was found, start at the beginning and don't restart  */
    UA_Boolean found = false;
    if(!immediate)
        immediate = LIST_FIRST(&session->serverSubscriptions);
    else
        found = true;

 repeat:
    while(immediate) {
        if(immediate->state == UA_SUBSCRIPTIONSTATE_LATE) {
            session->lastSeenSubscriptionId = immediate->subscriptionId;
            UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                                 "Subscription %" PRIu32 " | Response on a late subscription",
                                 immediate->subscriptionId);
            UA_Subscription_publish(server, immediate);
            return;
        }
        immediate = LIST_NEXT(immediate, listEntry);
    }

    /* Restart at the beginning of the list */
    if(found) {
        immediate = LIST_FIRST(&session->serverSubscriptions);
        found = false;
        goto repeat;
    }

    /* No late subscription this time */
    session->lastSeenSubscriptionId = 0;
}

static void
Operation_DeleteSubscription(UA_Server *server, UA_Session *session, void *_,
                             const UA_UInt32 *subscriptionId, UA_StatusCode *result) {
    *result = UA_Session_deleteSubscription(server, session, *subscriptionId);
    if(*result == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                             "Subscription %" PRIu32 " | Subscription deleted",
                             *subscriptionId);
    } else {
        UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                             "Deleting Subscription with Id %" PRIu32 " failed with error code %s",
                             *subscriptionId, UA_StatusCode_name(*result));
    }
}

void
Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                            const UA_DeleteSubscriptionsRequest *request,
                            UA_DeleteSubscriptionsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing DeleteSubscriptionsRequest");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_DeleteSubscription, NULL,
                  &request->subscriptionIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);

    /* The session has at least one subscription */
    if(LIST_FIRST(&session->serverSubscriptions))
        return;

    /* Send remaining publish responses if the last subscription was removed */
    UA_Subscription_answerPublishRequestsNoSubscription(server, session);
}

void
Service_Republish(UA_Server *server, UA_Session *session,
                  const UA_RepublishRequest *request,
                  UA_RepublishResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing RepublishRequest");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* Get the subscription */
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    sub->currentLifetimeCount = 0;

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
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
