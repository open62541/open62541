#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

#define UA_BOUNDEDVALUE_SETWBOUNDS(BOUNDS, SRC, DST) { \
        if(SRC > BOUNDS.max) DST = BOUNDS.max;         \
        else if(SRC < BOUNDS.min) DST = BOUNDS.min;    \
        else DST = SRC;                                \
    }

static void
setSubscriptionSettings(UA_Server *server, UA_Subscription *subscription,
                        UA_Double requestedPublishingInterval,
                        UA_UInt32 requestedLifetimeCount,
                        UA_UInt32 requestedMaxKeepAliveCount,
                        UA_UInt32 maxNotificationsPerPublish, UA_Byte priority) {
    Subscription_unregisterPublishJob(server, subscription);
    subscription->publishingInterval = requestedPublishingInterval;
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
    Subscription_registerPublishJob(server, subscription);
}

void Service_CreateSubscription(UA_Server *server, UA_Session *session,
                                const UA_CreateSubscriptionRequest *request,
                                UA_CreateSubscriptionResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing CreateSubscriptionRequest");
    response->subscriptionId = UA_Session_getUniqueSubscriptionID(session);
    UA_Subscription *newSubscription = UA_Subscription_new(session, response->subscriptionId);
    if(!newSubscription) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    UA_Session_addSubscription(session, newSubscription);
    newSubscription->publishingEnabled = request->publishingEnabled;
    setSubscriptionSettings(server, newSubscription, request->requestedPublishingInterval,
                            request->requestedLifetimeCount, request->requestedMaxKeepAliveCount,
                            request->maxNotificationsPerPublish, request->priority);
    /* immediately send the first response */
    newSubscription->currentKeepAliveCount = newSubscription->maxKeepAliveCount;
    response->revisedPublishingInterval = newSubscription->publishingInterval;
    response->revisedLifetimeCount = newSubscription->lifeTimeCount;
    response->revisedMaxKeepAliveCount = newSubscription->maxKeepAliveCount;
}

void Service_ModifySubscription(UA_Server *server, UA_Session *session,
                                const UA_ModifySubscriptionRequest *request,
                                UA_ModifySubscriptionResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing ModifySubscriptionRequest");
    UA_Subscription *sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    setSubscriptionSettings(server, sub, request->requestedPublishingInterval,
                            request->requestedLifetimeCount, request->requestedMaxKeepAliveCount,
                            request->maxNotificationsPerPublish, request->priority);
    sub->currentLifetimeCount = 0; /* Reset the subscription lifetime */
    response->revisedPublishingInterval = sub->publishingInterval;
    response->revisedLifetimeCount = sub->lifeTimeCount;
    response->revisedMaxKeepAliveCount = sub->maxKeepAliveCount;
    return;
}

void Service_SetPublishingMode(UA_Server *server, UA_Session *session,
                               const UA_SetPublishingModeRequest *request,
                               UA_SetPublishingModeResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing SetPublishingModeRequest");
    if(request->subscriptionIdsSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    size_t size = request->subscriptionIdsSize;
    response->results = UA_Array_new(size, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    response->resultsSize = size;
    for(size_t i = 0; i < size; i++) {
        UA_Subscription *sub = UA_Session_getSubscriptionByID(session, request->subscriptionIds[i]);
        if(!sub) {
            response->results[i] = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
            continue;
        }
        sub->publishingEnabled = request->publishingEnabled;
        sub->currentLifetimeCount = 0; /* Reset the subscription lifetime */
    }
}

static void
setMonitoredItemSettings(UA_Server *server, UA_MonitoredItem *mon,
                         UA_MonitoringMode monitoringMode, UA_UInt32 clientHandle,
                         UA_Double samplingInterval, UA_UInt32 queueSize,
                         UA_Boolean discardOldest) {
    MonitoredItem_unregisterSampleJob(server, mon);
    mon->monitoringMode = monitoringMode;
    mon->clientHandle = clientHandle;
    mon->samplingInterval = samplingInterval;
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.samplingIntervalLimits,
        samplingInterval, mon->samplingInterval);
    /* Check for nan */
    if(samplingInterval != samplingInterval)
        mon->samplingInterval = server->config.samplingIntervalLimits.min;
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.queueSizeLimits,
                               queueSize, mon->maxQueueSize);
    mon->discardOldest = discardOldest;
    MonitoredItem_registerSampleJob(server, mon);
}

static const UA_String binaryEncoding = {sizeof("DefaultBinary")-1, (UA_Byte*)"DefaultBinary"};
static void
Service_CreateMonitoredItems_single(UA_Server *server, UA_Session *session, UA_Subscription *sub,
                                    const UA_TimestampsToReturn timestampsToReturn,
                                    const UA_MonitoredItemCreateRequest *request,
                                    UA_MonitoredItemCreateResult *result) {
    /* Check if the target exists */
    const UA_Node *target = UA_NodeStore_get(server->nodestore, &request->itemToMonitor.nodeId);
    if(!target) {
        result->statusCode = UA_STATUSCODE_BADNODEIDINVALID;
        return;
    }
    // TODO: Check if the target node type has the requested attribute

    /* Check if the encoding is supported */
    if(request->itemToMonitor.dataEncoding.name.length > 0 &&
       !UA_String_equal(&binaryEncoding, &request->itemToMonitor.dataEncoding.name)) {
        result->statusCode = UA_STATUSCODE_BADDATAENCODINGUNSUPPORTED;
        return;
    }

    /* Create the monitoreditem */
    UA_MonitoredItem *newMon = UA_MonitoredItem_new();
    if(!newMon) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    UA_StatusCode retval = UA_NodeId_copy(&target->nodeId, &newMon->monitoredNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        result->statusCode = retval;
        MonitoredItem_delete(server, newMon);
        return;
    }
    newMon->subscription = sub;
    newMon->attributeID = request->itemToMonitor.attributeId;
    newMon->itemId = ++(sub->lastMonitoredItemId);
    newMon->timestampsToReturn = timestampsToReturn;
    setMonitoredItemSettings(server, newMon, request->monitoringMode,
                             request->requestedParameters.clientHandle,
                             request->requestedParameters.samplingInterval,
                             request->requestedParameters.queueSize,
                             request->requestedParameters.discardOldest);
    LIST_INSERT_HEAD(&sub->MonitoredItems, newMon, listEntry);

    /* Prepare the response */
    UA_String_copy(&request->itemToMonitor.indexRange, &newMon->indexRange);
    result->revisedSamplingInterval = newMon->samplingInterval;
    result->revisedQueueSize = newMon->maxQueueSize;
    result->monitoredItemId = newMon->itemId;
}

void
Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_CreateMonitoredItemsRequest *request,
                             UA_CreateMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing CreateMonitoredItemsRequest");
    UA_Subscription *sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    sub->currentLifetimeCount = 0;
    if(request->itemsToCreateSize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_Array_new(request->itemsToCreateSize,
                                     &UA_TYPES[UA_TYPES_MONITOREDITEMCREATERESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->itemsToCreateSize;

    for(size_t i = 0; i < request->itemsToCreateSize; i++)
        Service_CreateMonitoredItems_single(server, session, sub, request->timestampsToReturn,
                                            &request->itemsToCreate[i], &response->results[i]);
}

static void
Service_ModifyMonitoredItems_single(UA_Server *server, UA_Session *session, UA_Subscription *sub,
                                    const UA_MonitoredItemModifyRequest *request,
                                    UA_MonitoredItemModifyResult *result) {
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(sub, request->monitoredItemId);
    if(!mon) {
        result->statusCode = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        return;
    }

    setMonitoredItemSettings(server, mon, mon->monitoringMode,
                             request->requestedParameters.clientHandle,
                             request->requestedParameters.samplingInterval,
                             request->requestedParameters.queueSize,
                             request->requestedParameters.discardOldest);
    result->revisedSamplingInterval = mon->samplingInterval;
    result->revisedQueueSize = mon->maxQueueSize;
}

void Service_ModifyMonitoredItems(UA_Server *server, UA_Session *session,
                                  const UA_ModifyMonitoredItemsRequest *request,
                                  UA_ModifyMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing ModifyMonitoredItemsRequest");
    UA_Subscription *sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    sub->currentLifetimeCount = 0;
    if(request->itemsToModifySize <= 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    response->results = UA_Array_new(request->itemsToModifySize,
                                     &UA_TYPES[UA_TYPES_MONITOREDITEMMODIFYRESULT]);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->itemsToModifySize;

    for(size_t i = 0; i < request->itemsToModifySize; i++)
        Service_ModifyMonitoredItems_single(server, session, sub, &request->itemsToModify[i],
                                            &response->results[i]);

}

void
Service_Publish(UA_Server *server, UA_Session *session,
                const UA_PublishRequest *request, UA_UInt32 requestId) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing PublishRequest");
    /* Return an error if the session has no subscription */
    if(LIST_EMPTY(&session->serverSubscriptions)) {
        UA_PublishResponse response;
        UA_PublishResponse_init(&response);
        response.responseHeader.requestHandle = request->requestHeader.requestHandle;
        response.responseHeader.serviceResult = UA_STATUSCODE_BADNOSUBSCRIPTION;
        UA_SecureChannel_sendBinaryMessage(session->channel, requestId, &response,
                                           &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
        return;
    }

    // todo error handling for malloc
    UA_PublishResponseEntry *entry = UA_malloc(sizeof(UA_PublishResponseEntry));
    entry->requestId = requestId;
    UA_PublishResponse *response = &entry->response;
    UA_PublishResponse_init(response);
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;

    /* Delete Acknowledged Subscription Messages */
    response->results = UA_malloc(request->subscriptionAcknowledgementsSize * sizeof(UA_StatusCode));
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->subscriptionAcknowledgementsSize;
    for(size_t i = 0; i < request->subscriptionAcknowledgementsSize; i++) {
        UA_SubscriptionAcknowledgement *ack = &request->subscriptionAcknowledgements[i];
        UA_Subscription *sub = UA_Session_getSubscriptionByID(session, ack->subscriptionId);
        if(!sub) {
            response->results[i] = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
            UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Cannot process acknowledgements subscription %u", ack->subscriptionId);
            continue;
        }

        response->results[i] = UA_STATUSCODE_BADSEQUENCENUMBERUNKNOWN;
        UA_NotificationMessageEntry *pre, *pre_tmp;
        LIST_FOREACH_SAFE(pre, &sub->retransmissionQueue, listEntry, pre_tmp) {
            if(pre->message.sequenceNumber == ack->sequenceNumber) {
                LIST_REMOVE(pre, listEntry);
                response->results[i] = UA_STATUSCODE_GOOD;
                UA_NotificationMessage_deleteMembers(&pre->message);
                UA_free(pre);
                break;
            }
        }
    }

    /* Queue the publish response */
    SIMPLEQ_INSERT_TAIL(&session->responseQueue, entry, listEntry);
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Queued a publication message",
                 session->authenticationToken.identifier.numeric);

    /* Answer immediately to a late subscription */
    UA_Subscription *immediate;
    LIST_FOREACH(immediate, &session->serverSubscriptions, listEntry) {
        if(immediate->state == UA_SUBSCRIPTIONSTATE_LATE) {
            UA_LOG_DEBUG_SESSION(server->config.logger, session, "Response on a late subscription",
                                 session->authenticationToken.identifier.numeric);
            UA_Subscription_publishCallback(server, immediate);
            return;
        }
    }
}

void Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                                 const UA_DeleteSubscriptionsRequest *request,
                                 UA_DeleteSubscriptionsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing DeleteSubscriptionsRequest");
    response->results = UA_malloc(sizeof(UA_StatusCode) * request->subscriptionIdsSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->subscriptionIdsSize;

    for(size_t i = 0; i < request->subscriptionIdsSize; i++)
        response->results[i] = UA_Session_deleteSubscription(server, session, request->subscriptionIds[i]);
}

void Service_DeleteMonitoredItems(UA_Server *server, UA_Session *session,
                                  const UA_DeleteMonitoredItemsRequest *request,
                                  UA_DeleteMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing DeleteMonitoredItemsRequest");
    UA_Subscription *sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    sub->currentLifetimeCount = 0;
    response->results = UA_malloc(sizeof(UA_StatusCode) * request->monitoredItemIdsSize);
    if(!response->results) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    response->resultsSize = request->monitoredItemIdsSize;

    for(size_t i = 0; i < request->monitoredItemIdsSize; i++)
        response->results[i] = UA_Subscription_deleteMonitoredItem(server, sub, request->monitoredItemIds[i]);
}

void Service_Republish(UA_Server *server, UA_Session *session, const UA_RepublishRequest *request,
                       UA_RepublishResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing RepublishRequest");
    /* get the subscription */
    UA_Subscription *sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if (!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    sub->currentLifetimeCount = 0;

    /* Find the notification in the retransmission queue  */
    UA_NotificationMessageEntry *entry;
    LIST_FOREACH(entry, &sub->retransmissionQueue, listEntry) {
        if(entry->message.sequenceNumber == request->retransmitSequenceNumber)
            break;
    }
    if(entry)
        response->responseHeader.serviceResult =
            UA_NotificationMessage_copy(&entry->message, &response->notificationMessage);
    else
      response->responseHeader.serviceResult = UA_STATUSCODE_BADMESSAGENOTAVAILABLE;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
