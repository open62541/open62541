/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
    /* deregister the callback if required */
    UA_StatusCode retval = Subscription_unregisterPublishCallback(server, subscription);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_DEBUG_SESSION(server->config.logger, subscription->session, "Subscription %u | "
                             "Could not unregister publish callback with error code %s",
                             subscription->subscriptionID, UA_StatusCode_name(retval));

    /* re-parameterize the subscription */
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

    retval = Subscription_registerPublishCallback(server, subscription);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_DEBUG_SESSION(server->config.logger, subscription->session, "Subscription %u | "
                             "Could not register publish callback with error code %s",
                             subscription->subscriptionID, UA_StatusCode_name(retval));
}

void
Service_CreateSubscription(UA_Server *server, UA_Session *session,
                           const UA_CreateSubscriptionRequest *request,
                           UA_CreateSubscriptionResponse *response) {
    /* Create the subscription */
    UA_Subscription *newSubscription = UA_Subscription_new(session, response->subscriptionId);
    if(!newSubscription) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session,
                             "Processing CreateSubscriptionRequest failed");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    newSubscription->subscriptionID = UA_Session_getUniqueSubscriptionID(session);
    UA_Session_addSubscription(session, newSubscription);

    /* Set the subscription parameters */
    newSubscription->publishingEnabled = request->publishingEnabled;
    setSubscriptionSettings(server, newSubscription, request->requestedPublishingInterval,
                            request->requestedLifetimeCount, request->requestedMaxKeepAliveCount,
                            request->maxNotificationsPerPublish, request->priority);
    newSubscription->currentKeepAliveCount = newSubscription->maxKeepAliveCount; /* set settings first */

    /* Prepare the response */
    response->subscriptionId = newSubscription->subscriptionID;
    response->revisedPublishingInterval = newSubscription->publishingInterval;
    response->revisedLifetimeCount = newSubscription->lifeTimeCount;
    response->revisedMaxKeepAliveCount = newSubscription->maxKeepAliveCount;

    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "CreateSubscriptionRequest: Created Subscription %u "
                         "with a publishing interval of %f ms", response->subscriptionId,
                         newSubscription->publishingInterval);
}

void
Service_ModifySubscription(UA_Server *server, UA_Session *session,
                           const UA_ModifySubscriptionRequest *request,
                           UA_ModifySubscriptionResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing ModifySubscriptionRequest");

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
}

static UA_THREAD_LOCAL UA_Boolean op_publishingEnabled;

static void
Operation_SetPublishingMode(UA_Server *Server, UA_Session *session,
                            UA_UInt32 *subscriptionId,
                            UA_StatusCode *result) {
    UA_Subscription *sub =
        UA_Session_getSubscriptionByID(session, *subscriptionId);
    if(!sub) {
        *result = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    sub->currentLifetimeCount = 0; 

    /* Set the publishing mode */
    sub->publishingEnabled = op_publishingEnabled;
}

void
Service_SetPublishingMode(UA_Server *server, UA_Session *session,
                          const UA_SetPublishingModeRequest *request,
                          UA_SetPublishingModeResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing SetPublishingModeRequest");

    op_publishingEnabled = request->publishingEnabled;
    response->responseHeader.serviceResult = 
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_SetPublishingMode,
                  &request->subscriptionIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

static void
setMonitoredItemSettings(UA_Server *server, UA_MonitoredItem *mon,
                         UA_MonitoringMode monitoringMode,
                         const UA_MonitoringParameters *params) {
    MonitoredItem_unregisterSampleCallback(server, mon);
    mon->monitoringMode = monitoringMode;

    /* ClientHandle */
    mon->clientHandle = params->clientHandle;

    /* SamplingInterval */
    UA_Double samplingInterval = params->samplingInterval;
    if(mon->attributeID == UA_ATTRIBUTEID_VALUE) {
        const UA_VariableNode *vn = (const UA_VariableNode*)
            UA_Nodestore_get(server, &mon->monitoredNodeId);
        if(vn) {
            if(vn->nodeClass == UA_NODECLASS_VARIABLE &&
               samplingInterval <  vn->minimumSamplingInterval)
                samplingInterval = vn->minimumSamplingInterval;
            UA_Nodestore_release(server, (const UA_Node*)vn);
        }
    } else if(mon->attributeID == UA_ATTRIBUTEID_EVENTNOTIFIER) {
        /* TODO: events should not need a samplinginterval */
        samplingInterval = 10000.0f; // 10 seconds to reduce the load
    }
    mon->samplingInterval = samplingInterval;
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.samplingIntervalLimits,
        samplingInterval, mon->samplingInterval);
    if(samplingInterval != samplingInterval) /* Check for nan */
        mon->samplingInterval = server->config.samplingIntervalLimits.min;

    /* Filter */
    if(params->filter.encoding != UA_EXTENSIONOBJECT_DECODED ||
       params->filter.content.decoded.type != &UA_TYPES[UA_TYPES_DATACHANGEFILTER]) {
        /* Default: Trigger only on the value and the statuscode */
        mon->trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    } else {
        UA_DataChangeFilter *filter = (UA_DataChangeFilter *)params->filter.content.decoded.data;
        mon->trigger = filter->trigger;
    }

    /* QueueSize */
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.queueSizeLimits,
                               params->queueSize, mon->maxQueueSize);

    /* DiscardOldest */
    mon->discardOldest = params->discardOldest;

    /* Register sample callback if reporting is enabled */
    if(monitoringMode == UA_MONITORINGMODE_REPORTING)
        MonitoredItem_registerSampleCallback(server, mon);
}

static const UA_String binaryEncoding = {sizeof("Default Binary")-1, (UA_Byte*)"Default Binary"};

/* Thread-local variables to pass additional arguments into the operation */
static UA_THREAD_LOCAL UA_Subscription *op_sub;
static UA_THREAD_LOCAL UA_TimestampsToReturn op_timestampsToReturn2;

static void
Operation_CreateMonitoredItem(UA_Server *server, UA_Session *session,
                              const UA_MonitoredItemCreateRequest *request,
                              UA_MonitoredItemCreateResult *result) {
    /* Make an example read to get errors in the itemToMonitor. Allow return
     * codes "good" and "uncertain", as well as a list of statuscodes that might
     * be repaired inside the data source. */
    UA_DataValue v = UA_Server_readWithSession(server, session, &request->itemToMonitor,
                                               op_timestampsToReturn2);
    if(v.hasStatus && (v.status >> 30) > 1 &&
       v.status != UA_STATUSCODE_BADRESOURCEUNAVAILABLE &&
       v.status != UA_STATUSCODE_BADCOMMUNICATIONERROR &&
       v.status != UA_STATUSCODE_BADWAITINGFORINITIALDATA &&
       v.status != UA_STATUSCODE_BADUSERACCESSDENIED &&
       v.status != UA_STATUSCODE_BADNOTREADABLE &&
       v.status != UA_STATUSCODE_BADINDEXRANGENODATA) {
        result->statusCode = v.status;
        UA_DataValue_deleteMembers(&v);
        return;
    }
    UA_DataValue_deleteMembers(&v);

    /* Check if the encoding is supported */
    if(request->itemToMonitor.dataEncoding.name.length > 0 &&
       (!UA_String_equal(&binaryEncoding, &request->itemToMonitor.dataEncoding.name) ||
       request->itemToMonitor.dataEncoding.namespaceIndex != 0)) {
        result->statusCode = UA_STATUSCODE_BADDATAENCODINGUNSUPPORTED;
        return;
    }

    /* Check if the encoding is set for a value */
    if(request->itemToMonitor.attributeId != UA_ATTRIBUTEID_VALUE &&
       request->itemToMonitor.dataEncoding.name.length > 0) {
        result->statusCode = UA_STATUSCODE_BADDATAENCODINGINVALID;
        return;
    }

    /* Create the monitoreditem */
    UA_MonitoredItem *newMon = UA_MonitoredItem_new();
    if(!newMon) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    UA_StatusCode retval = UA_NodeId_copy(&request->itemToMonitor.nodeId,
                                          &newMon->monitoredNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        result->statusCode = retval;
        MonitoredItem_delete(server, newMon);
        return;
    }
    newMon->subscription = op_sub;
    newMon->attributeID = request->itemToMonitor.attributeId;
    newMon->itemId = ++(op_sub->lastMonitoredItemId);
    newMon->timestampsToReturn = op_timestampsToReturn2;
    setMonitoredItemSettings(server, newMon, request->monitoringMode,
                             &request->requestedParameters);
    LIST_INSERT_HEAD(&op_sub->monitoredItems, newMon, listEntry);

    /* Create the first sample */
    if(request->monitoringMode == UA_MONITORINGMODE_REPORTING)
        UA_MoniteredItem_SampleCallback(server, newMon);

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
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing CreateMonitoredItemsRequest");

    /* Check if the timestampstoreturn is valid */
    op_timestampsToReturn2 = request->timestampsToReturn;
    if(op_timestampsToReturn2 > UA_TIMESTAMPSTORETURN_NEITHER) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
        return;
    }

    /* Find the subscription */
    op_sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if(!op_sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    op_sub->currentLifetimeCount = 0;

    response->responseHeader.serviceResult = 
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_CreateMonitoredItem,
                  &request->itemsToCreateSize, &UA_TYPES[UA_TYPES_MONITOREDITEMCREATEREQUEST],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_MONITOREDITEMCREATERESULT]);
}

static void
Operation_ModifyMonitoredItem(UA_Server *server, UA_Session *session,
                              const UA_MonitoredItemModifyRequest *request,
                              UA_MonitoredItemModifyResult *result) {
    /* Get the MonitoredItem */
    UA_MonitoredItem *mon =
        UA_Subscription_getMonitoredItem(op_sub, request->monitoredItemId);
    if(!mon) {
        result->statusCode = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        return;
    }

    setMonitoredItemSettings(server, mon, mon->monitoringMode,
                             &request->requestedParameters);
    result->revisedSamplingInterval = mon->samplingInterval;
    result->revisedQueueSize = mon->maxQueueSize;
}

void Service_ModifyMonitoredItems(UA_Server *server, UA_Session *session,
                                  const UA_ModifyMonitoredItemsRequest *request,
                                  UA_ModifyMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing ModifyMonitoredItemsRequest");

    /* Check if the timestampstoreturn is valid */
    if(request->timestampsToReturn > UA_TIMESTAMPSTORETURN_NEITHER) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
        return;
    }

    /* Get the subscription */
    op_sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if(!op_sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    op_sub->currentLifetimeCount = 0;

    response->responseHeader.serviceResult = 
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_ModifyMonitoredItem,
                  &request->itemsToModifySize, &UA_TYPES[UA_TYPES_MONITOREDITEMMODIFYREQUEST],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_MONITOREDITEMMODIFYRESULT]);
}

/* Get the additional argument into the operation */
static UA_THREAD_LOCAL UA_MonitoringMode op_monitoringMode;

static void
Operation_SetMonitoringMode(UA_Server *server, UA_Session *session,
                            UA_UInt32 *monitoredItemId,
                            UA_StatusCode *result) {
    UA_MonitoredItem *mon =
        UA_Subscription_getMonitoredItem(op_sub, *monitoredItemId);
    if(!mon) {
        *result = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        return;
    }

    if(mon->monitoringMode == op_monitoringMode)
        return;

    mon->monitoringMode = op_monitoringMode;
    if(mon->monitoringMode == UA_MONITORINGMODE_REPORTING)
        MonitoredItem_registerSampleCallback(server, mon);
    else
        MonitoredItem_unregisterSampleCallback(server, mon);
}

void Service_SetMonitoringMode(UA_Server *server, UA_Session *session,
                               const UA_SetMonitoringModeRequest *request,
                               UA_SetMonitoringModeResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing SetMonitoringMode");

    /* Get the subscription */
    op_sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if(!op_sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    op_sub->currentLifetimeCount = 0;

    op_monitoringMode = request->monitoringMode;
    response->responseHeader.serviceResult = 
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_SetMonitoringMode,
                  &request->monitoredItemIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

/* TODO: Unify with senderror in ua_server_binary.c */
static void
subscriptionSendError(UA_SecureChannel *channel, UA_UInt32 requestHandle,
                      UA_UInt32 requestId, UA_StatusCode error) {
    UA_PublishResponse err_response;
    UA_PublishResponse_init(&err_response);
    err_response.responseHeader.requestHandle = requestHandle;
    err_response.responseHeader.timestamp = UA_DateTime_now();
    err_response.responseHeader.serviceResult = error;
    UA_SecureChannel_sendSymmetricMessage(channel, requestId, UA_MESSAGETYPE_MSG,
                                          &err_response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
}

void
Service_Publish(UA_Server *server, UA_Session *session,
                const UA_PublishRequest *request, UA_UInt32 requestId) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing PublishRequest");

    /* Return an error if the session has no subscription */
    if(LIST_EMPTY(&session->serverSubscriptions)) {
        subscriptionSendError(session->channel, request->requestHeader.requestHandle,
                              requestId, UA_STATUSCODE_BADNOSUBSCRIPTION);
        return;
    }

    UA_PublishResponseEntry *entry =
        (UA_PublishResponseEntry*)UA_malloc(sizeof(UA_PublishResponseEntry));
    if(!entry) {
        subscriptionSendError(session->channel, requestId,
                              request->requestHeader.requestHandle,
                              UA_STATUSCODE_BADOUTOFMEMORY);
        return;
    }
    entry->requestId = requestId;

    /* Build the response */
    UA_PublishResponse *response = &entry->response;
    UA_PublishResponse_init(response);
    response->responseHeader.requestHandle = request->requestHeader.requestHandle;
    if(request->subscriptionAcknowledgementsSize > 0) {
        response->results = (UA_StatusCode*)
            UA_Array_new(request->subscriptionAcknowledgementsSize,
                         &UA_TYPES[UA_TYPES_STATUSCODE]);
        if(!response->results) {
            UA_free(entry);
            subscriptionSendError(session->channel, requestId,
                                  request->requestHeader.requestHandle,
                                  UA_STATUSCODE_BADOUTOFMEMORY);
            return;
        }
        response->resultsSize = request->subscriptionAcknowledgementsSize;
    }

    /* Delete Acknowledged Subscription Messages */
    for(size_t i = 0; i < request->subscriptionAcknowledgementsSize; ++i) {
        UA_SubscriptionAcknowledgement *ack = &request->subscriptionAcknowledgements[i];
        UA_Subscription *sub = UA_Session_getSubscriptionByID(session, ack->subscriptionId);
        if(!sub) {
            response->results[i] = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
            UA_LOG_DEBUG_SESSION(server->config.logger, session,
                                 "Cannot process acknowledgements subscription %u",
                                 ack->subscriptionId);
            continue;
        }
        /* Remove the acked transmission from the retransmission queue */
        response->results[i] =
            UA_Subscription_removeRetransmissionMessage(sub, ack->sequenceNumber);
    }

    /* Queue the publish response */
    SIMPLEQ_INSERT_TAIL(&session->responseQueue, entry, listEntry);
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Queued a publication message");

    /* Answer immediately to a late subscription */
    UA_Subscription *immediate;
    LIST_FOREACH(immediate, &session->serverSubscriptions, listEntry) {
        if(immediate->state == UA_SUBSCRIPTIONSTATE_LATE) {
            UA_LOG_DEBUG_SESSION(server->config.logger, session, "Subscription %u | "
                                 "Response on a late subscription", immediate->subscriptionID);
            UA_Subscription_publishCallback(server, immediate);
            break;
        }
    }
}

static void
Operation_DeleteSubscription(UA_Server *server, UA_Session *session,
                             UA_UInt32 *subscriptionId, UA_StatusCode *result) {
    *result = UA_Session_deleteSubscription(server, session, *subscriptionId);
    if(*result == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session,
                             "Subscription %u | Subscription deleted",
                             *subscriptionId);
    } else {
        UA_LOG_DEBUG_SESSION(server->config.logger, session,
                             "Deleting Subscription with Id %u failed with error "
                             "code %s", *subscriptionId, UA_StatusCode_name(*result));
    }
}

void
Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                            const UA_DeleteSubscriptionsRequest *request,
                            UA_DeleteSubscriptionsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing DeleteSubscriptionsRequest");

    response->responseHeader.serviceResult = 
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_DeleteSubscription,
                  &request->subscriptionIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);

    /* The session has at least one subscription */
    if(LIST_FIRST(&session->serverSubscriptions))
        return;

    /* Send remaining publish responses if the last subscription was removed */
    UA_Subscription_answerPublishRequestsNoSubscription(server, session);
}

static void
Operation_DeleteMonitoredItem(UA_Server *server, UA_Session *session,
                              UA_UInt32 *monitoredItemId,
                              UA_StatusCode *result) {
    *result = UA_Subscription_deleteMonitoredItem(server, op_sub, *monitoredItemId);
}

void Service_DeleteMonitoredItems(UA_Server *server, UA_Session *session,
                                  const UA_DeleteMonitoredItemsRequest *request,
                                  UA_DeleteMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing DeleteMonitoredItemsRequest");

    /* Get the subscription */
    op_sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if(!op_sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    op_sub->currentLifetimeCount = 0;

    response->responseHeader.serviceResult = 
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_DeleteMonitoredItem,
                  &request->monitoredItemIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

void Service_Republish(UA_Server *server, UA_Session *session,
                       const UA_RepublishRequest *request,
                       UA_RepublishResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session,
                         "Processing RepublishRequest");

    /* Get the subscription */
    UA_Subscription *sub = UA_Session_getSubscriptionByID(session, request->subscriptionId);
    if (!sub) {
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
