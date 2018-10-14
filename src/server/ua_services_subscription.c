/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2016-2017 (c) Florian Palm
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2017 (c) Mattias Bornhager
 *    Copyright 2017 (c) Henrik Norrman
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 */

#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

#define UA_BOUNDEDVALUE_SETWBOUNDS(BOUNDS, SRC, DST) { \
        if(SRC > BOUNDS.max) DST = BOUNDS.max;         \
        else if(SRC < BOUNDS.min) DST = BOUNDS.min;    \
        else DST = SRC;                                \
    }

static UA_StatusCode
setSubscriptionSettings(UA_Server *server, UA_Subscription *subscription,
                        UA_Double requestedPublishingInterval,
                        UA_UInt32 requestedLifetimeCount,
                        UA_UInt32 requestedMaxKeepAliveCount,
                        UA_UInt32 maxNotificationsPerPublish, UA_Byte priority) {
    /* deregister the callback if required */
    UA_StatusCode retval = Subscription_unregisterPublishCallback(server, subscription);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_SESSION(server->config.logger, subscription->session,
                             "Subscription %u | Could not unregister publish callback with error code %s",
                             subscription->subscriptionId, UA_StatusCode_name(retval));
        return retval;
    }

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
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_SESSION(server->config.logger, subscription->session,
                             "Subscription %u | Could not register publish callback with error code %s",
                             subscription->subscriptionId, UA_StatusCode_name(retval));
        return retval;
    }
    return UA_STATUSCODE_GOOD;
}

void
Service_CreateSubscription(UA_Server *server, UA_Session *session,
                           const UA_CreateSubscriptionRequest *request,
                           UA_CreateSubscriptionResponse *response) {
    /* Check limits for the number of subscriptions */
    if((server->config.maxSubscriptionsPerSession != 0) &&
       (session->numSubscriptions >= server->config.maxSubscriptionsPerSession)) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYSUBSCRIPTIONS;
        return;
    }

    /* Create the subscription */
    UA_Subscription *newSubscription = UA_Subscription_new(session, response->subscriptionId);
    if(!newSubscription) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session,
                             "Processing CreateSubscriptionRequest failed");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    UA_Session_addSubscription(session, newSubscription); /* Also assigns the subscription id */

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

    UA_LOG_INFO_SESSION(server->config.logger, session, "Subscription %u | "
                        "Created the Subscription with a publishing interval of %f ms",
                        response->subscriptionId, newSubscription->publishingInterval);
}

void
Service_ModifySubscription(UA_Server *server, UA_Session *session,
                           const UA_ModifySubscriptionRequest *request,
                           UA_ModifySubscriptionResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing ModifySubscriptionRequest");

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
Operation_SetPublishingMode(UA_Server *Server, UA_Session *session,
                            UA_Boolean *publishingEnabled, UA_UInt32 *subscriptionId,
                            UA_StatusCode *result) {
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
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing SetPublishingModeRequest");
    UA_Boolean publishingEnabled = request->publishingEnabled; /* request is const */
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_SetPublishingMode,
                                           &publishingEnabled,
                                           &request->subscriptionIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

static UA_StatusCode
setMonitoredItemSettings(UA_Server *server, UA_MonitoredItem *mon,
                         UA_MonitoringMode monitoringMode,
                         const UA_MonitoringParameters *params,
                         // This parameter is optional and used only if mon->lastValue is not set yet.
                         // Then numeric type will be detected from this value. Set null as defaut.
                         const UA_DataType* dataType) {

    /* Filter */
    if(params->filter.encoding != UA_EXTENSIONOBJECT_DECODED) {
        UA_DataChangeFilter_init(&(mon->filter));
        mon->filter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    } else if(params->filter.content.decoded.type != &UA_TYPES[UA_TYPES_DATACHANGEFILTER]) {
        return UA_STATUSCODE_BADMONITOREDITEMFILTERINVALID;
    } else {
        UA_DataChangeFilter *filter = (UA_DataChangeFilter *)params->filter.content.decoded.data;
        // TODO implement EURange to support UA_DEADBANDTYPE_PERCENT
        if (filter->deadbandType == UA_DEADBANDTYPE_PERCENT) {
            return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;
        }
        if (UA_Variant_isEmpty(&mon->lastValue)) {
            if (!dataType || !isDataTypeNumeric(dataType))
                return UA_STATUSCODE_BADFILTERNOTALLOWED;
        } else
        if (!isDataTypeNumeric(mon->lastValue.type)) {
            return UA_STATUSCODE_BADFILTERNOTALLOWED;
        }
        UA_DataChangeFilter_copy(filter, &(mon->filter));
    }

    MonitoredItem_unregisterSampleCallback(server, mon);
    mon->monitoringMode = monitoringMode;

    /* ClientHandle */
    mon->clientHandle = params->clientHandle;

    /* SamplingInterval */
    UA_Double samplingInterval = params->samplingInterval;
    if(mon->attributeId == UA_ATTRIBUTEID_VALUE) {
        const UA_VariableNode *vn = (const UA_VariableNode *)
            UA_Nodestore_get(server, &mon->monitoredNodeId);
        if(vn) {
            if(vn->nodeClass == UA_NODECLASS_VARIABLE &&
               samplingInterval < vn->minimumSamplingInterval)
                samplingInterval = vn->minimumSamplingInterval;
            UA_Nodestore_release(server, (const UA_Node *)vn);
        }
    } else if(mon->attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
        /* TODO: events should not need a samplinginterval */
        samplingInterval = 10000.0f; // 10 seconds to reduce the load
    }
    mon->samplingInterval = samplingInterval;
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.samplingIntervalLimits,
                               samplingInterval, mon->samplingInterval);
    if(samplingInterval != samplingInterval) /* Check for nan */
        mon->samplingInterval = server->config.samplingIntervalLimits.min;


    /* QueueSize */
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.queueSizeLimits,
                               params->queueSize, mon->maxQueueSize);

    /* DiscardOldest */
    mon->discardOldest = params->discardOldest;

    /* Register sample callback if reporting is enabled */
    if(monitoringMode == UA_MONITORINGMODE_REPORTING)
        return MonitoredItem_registerSampleCallback(server, mon);

    return UA_STATUSCODE_GOOD;
}

static const UA_String binaryEncoding = {sizeof("Default Binary") - 1, (UA_Byte *)"Default Binary"};

/* Thread-local variables to pass additional arguments into the operation */
struct createMonContext {
    UA_Subscription *sub;
    UA_TimestampsToReturn timestampsToReturn;
};

static void
Operation_CreateMonitoredItem(UA_Server *server, UA_Session *session, struct createMonContext *cmc,
                              const UA_MonitoredItemCreateRequest *request,
                              UA_MonitoredItemCreateResult *result) {
    /* Check available capacity */
    if(server->config.maxMonitoredItemsPerSubscription != 0 &&
       cmc->sub->monitoredItemsSize >= server->config.maxMonitoredItemsPerSubscription) {
        result->statusCode = UA_STATUSCODE_BADTOOMANYMONITOREDITEMS;
        return;
    }

    /* Make an example read to get errors in the itemToMonitor. Allow return
     * codes "good" and "uncertain", as well as a list of statuscodes that might
     * be repaired inside the data source. */
    UA_DataValue v = UA_Server_readWithSession(server, session, &request->itemToMonitor,
                                               cmc->timestampsToReturn);
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

    /* Check if the encoding is supported */
    if(request->itemToMonitor.dataEncoding.name.length > 0 &&
       (!UA_String_equal(&binaryEncoding, &request->itemToMonitor.dataEncoding.name) ||
        request->itemToMonitor.dataEncoding.namespaceIndex != 0)) {
        result->statusCode = UA_STATUSCODE_BADDATAENCODINGUNSUPPORTED;
        UA_DataValue_deleteMembers(&v);
        return;
    }

    /* Check if the encoding is set for a value */
    if(request->itemToMonitor.attributeId != UA_ATTRIBUTEID_VALUE &&
       request->itemToMonitor.dataEncoding.name.length > 0) {
        result->statusCode = UA_STATUSCODE_BADDATAENCODINGINVALID;
        UA_DataValue_deleteMembers(&v);
        return;
    }

    /* Create the monitoreditem */
    UA_MonitoredItem *newMon = UA_MonitoredItem_new(UA_MONITOREDITEMTYPE_CHANGENOTIFY);
    if(!newMon) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_DataValue_deleteMembers(&v);
        return;
    }
    UA_StatusCode retval = UA_NodeId_copy(&request->itemToMonitor.nodeId,
                                          &newMon->monitoredNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        result->statusCode = retval;
        MonitoredItem_delete(server, newMon);
        UA_DataValue_deleteMembers(&v);
        return;
    }
    newMon->subscription = cmc->sub;
    newMon->attributeId = request->itemToMonitor.attributeId;
    UA_String_copy(&request->itemToMonitor.indexRange, &newMon->indexRange);
    newMon->monitoredItemId = ++cmc->sub->lastMonitoredItemId;
    newMon->timestampsToReturn = cmc->timestampsToReturn;
    retval = setMonitoredItemSettings(server, newMon, request->monitoringMode,
                             &request->requestedParameters, v.value.type);
    UA_DataValue_deleteMembers(&v);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(server->config.logger, cmc->sub->session,
                            "Subscription %u | Could not create a MonitoredItem "
                            "with StatusCode %s", cmc->sub->subscriptionId,
                            UA_StatusCode_name(retval));
        result->statusCode = retval;
        MonitoredItem_delete(server, newMon);
        --cmc->sub->lastMonitoredItemId;
        return;
    }

    UA_Subscription_addMonitoredItem(cmc->sub, newMon);
    UA_LOG_INFO_SESSION(server->config.logger, cmc->sub->session,
                        "Subscription %u | MonitoredItem %i | "
                        "Created the MonitoredItem", cmc->sub->subscriptionId,
                        newMon->monitoredItemId);

    /* Create the first sample */
    if(request->monitoringMode == UA_MONITORINGMODE_REPORTING)
        UA_MonitoredItem_SampleCallback(server, newMon);

    /* Prepare the response */
    result->revisedSamplingInterval = newMon->samplingInterval;
    result->revisedQueueSize = newMon->maxQueueSize;
    result->monitoredItemId = newMon->monitoredItemId;
}

void
Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_CreateMonitoredItemsRequest *request,
                             UA_CreateMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing CreateMonitoredItemsRequest");

    if(server->config.maxMonitoredItemsPerCall != 0 &&
       request->itemsToCreateSize > server->config.maxMonitoredItemsPerCall) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    /* Check if the timestampstoreturn is valid */
    struct createMonContext cmc;
    cmc.timestampsToReturn = request->timestampsToReturn;
    if(cmc.timestampsToReturn > UA_TIMESTAMPSTORETURN_NEITHER) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
        return;
    }

    /* Find the subscription */
    cmc.sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!cmc.sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    cmc.sub->currentLifetimeCount = 0;

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session, (UA_ServiceOperation)Operation_CreateMonitoredItem, &cmc,
                                           &request->itemsToCreateSize, &UA_TYPES[UA_TYPES_MONITOREDITEMCREATEREQUEST],
                                           &response->resultsSize, &UA_TYPES[UA_TYPES_MONITOREDITEMCREATERESULT]);
}

static void
Operation_ModifyMonitoredItem(UA_Server *server, UA_Session *session, UA_Subscription *sub,
                              const UA_MonitoredItemModifyRequest *request,
                              UA_MonitoredItemModifyResult *result) {
    /* Get the MonitoredItem */
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(sub, request->monitoredItemId);
    if(!mon) {
        result->statusCode = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        return;
    }
    UA_StatusCode retval;
    retval = setMonitoredItemSettings(server, mon, mon->monitoringMode, &request->requestedParameters, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        result->statusCode = retval;
        return;
    }

    result->revisedSamplingInterval = mon->samplingInterval;
    result->revisedQueueSize = mon->maxQueueSize;

    /* Remove some notifications if the queue is now too small */
    MonitoredItem_ensureQueueSpace(mon);
}

void
Service_ModifyMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_ModifyMonitoredItemsRequest *request,
                             UA_ModifyMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing ModifyMonitoredItemsRequest");

    if(server->config.maxMonitoredItemsPerCall != 0 &&
       request->itemsToModifySize > server->config.maxMonitoredItemsPerCall) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    /* Check if the timestampstoreturn is valid */
    if(request->timestampsToReturn > UA_TIMESTAMPSTORETURN_NEITHER) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
        return;
    }

    /* Get the subscription */
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    sub->currentLifetimeCount = 0; /* Reset the subscription lifetime */

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_ModifyMonitoredItem, sub,
                  &request->itemsToModifySize, &UA_TYPES[UA_TYPES_MONITOREDITEMMODIFYREQUEST],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_MONITOREDITEMMODIFYRESULT]);
}

struct setMonitoringContext {
    UA_Subscription *sub;
    UA_MonitoringMode monitoringMode;
};

static void
Operation_SetMonitoringMode(UA_Server *server, UA_Session *session,
                            struct setMonitoringContext *smc,
                            UA_UInt32 *monitoredItemId, UA_StatusCode *result) {
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(smc->sub, *monitoredItemId);
    if(!mon) {
        *result = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        return;
    }

    if(mon->monitoredItemType != UA_MONITOREDITEMTYPE_CHANGENOTIFY) {
        *result = UA_STATUSCODE_BADNOTIMPLEMENTED;
        return;
    }

    /* Check if the MonitoringMode is valid or not */
    if(smc->monitoringMode > UA_MONITORINGMODE_REPORTING) {
        *result = UA_STATUSCODE_BADMONITORINGMODEINVALID;
        return;
    }

    if(mon->monitoringMode == smc->monitoringMode)
        return;

    mon->monitoringMode = smc->monitoringMode;
    if(mon->monitoringMode == UA_MONITORINGMODE_REPORTING) {
        *result = MonitoredItem_registerSampleCallback(server, mon);
    } else {
        MonitoredItem_unregisterSampleCallback(server, mon);

        // TODO correctly implement SAMPLING
        /*  Setting the mode to DISABLED or SAMPLING causes all queued Notifications to be deleted */
        UA_Notification *notification, *notification_tmp;
        TAILQ_FOREACH_SAFE(notification, &mon->queue, listEntry, notification_tmp) {
            TAILQ_REMOVE(&mon->queue, notification, listEntry);
            TAILQ_REMOVE(&smc->sub->notificationQueue, notification, globalEntry);
            --smc->sub->notificationQueueSize;

            UA_DataValue_deleteMembers(&notification->data.value);
            UA_free(notification);
        }
        mon->queueSize = 0;

        /* Initialize lastSampledValue */
        UA_ByteString_deleteMembers(&mon->lastSampledValue);
        UA_Variant_deleteMembers(&mon->lastValue);
    }
}

void
Service_SetMonitoringMode(UA_Server *server, UA_Session *session,
                          const UA_SetMonitoringModeRequest *request,
                          UA_SetMonitoringModeResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing SetMonitoringMode");

    if(server->config.maxMonitoredItemsPerCall != 0 &&
       request->monitoredItemIdsSize > server->config.maxMonitoredItemsPerCall) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    /* Get the subscription */
    struct setMonitoringContext smc;
    smc.sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!smc.sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    smc.sub->currentLifetimeCount = 0; /* Reset the subscription lifetime */

    smc.monitoringMode = request->monitoringMode;
    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_SetMonitoringMode, &smc,
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
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing PublishRequest");

    /* Return an error if the session has no subscription */
    if(LIST_EMPTY(&session->serverSubscriptions)) {
        subscriptionSendError(session->header.channel, request->requestHeader.requestHandle,
                              requestId, UA_STATUSCODE_BADNOSUBSCRIPTION);
        return;
    }

    /* Handle too many subscriptions to free resources before trying to allocate
     * resources for the new publish request. If the limit has been reached the
     * oldest publish request shall be responded */
    if((server->config.maxPublishReqPerSession != 0) &&
       (session->numPublishReq >= server->config.maxPublishReqPerSession)) {
        if(!UA_Subscription_reachedPublishReqLimit(server, session)) {
            subscriptionSendError(session->header.channel, requestId,
                                  request->requestHeader.requestHandle,
                                  UA_STATUSCODE_BADINTERNALERROR);
            return;
        }
    }

    /* Allocate the response to store it in the retransmission queue */
    UA_PublishResponseEntry *entry = (UA_PublishResponseEntry *)
        UA_malloc(sizeof(UA_PublishResponseEntry));
    if(!entry) {
        subscriptionSendError(session->header.channel, requestId,
                              request->requestHeader.requestHandle,
                              UA_STATUSCODE_BADOUTOFMEMORY);
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
            subscriptionSendError(session->header.channel, requestId,
                                  request->requestHeader.requestHandle,
                                  UA_STATUSCODE_BADOUTOFMEMORY);
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
            UA_LOG_DEBUG_SESSION(server->config.logger, session,
                                 "Cannot process acknowledgements subscription %u",
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
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Queued a publication message");

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
            UA_LOG_DEBUG_SESSION(server->config.logger, session,
                                 "Subscription %u | Response on a late subscription",
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
                             UA_UInt32 *subscriptionId, UA_StatusCode *result) {
    *result = UA_Session_deleteSubscription(server, session, *subscriptionId);
    if(*result == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_SESSION(server->config.logger, session,
                             "Subscription %u | Subscription deleted",
                             *subscriptionId);
    } else {
        UA_LOG_DEBUG_SESSION(server->config.logger, session,
                             "Deleting Subscription with Id %u failed with error code %s",
                             *subscriptionId, UA_StatusCode_name(*result));
    }
}

void
Service_DeleteSubscriptions(UA_Server *server, UA_Session *session,
                            const UA_DeleteSubscriptionsRequest *request,
                            UA_DeleteSubscriptionsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing DeleteSubscriptionsRequest");

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

static void
Operation_DeleteMonitoredItem(UA_Server *server, UA_Session *session, UA_Subscription *sub,
                              UA_UInt32 *monitoredItemId, UA_StatusCode *result) {
    *result = UA_Subscription_deleteMonitoredItem(server, sub, *monitoredItemId);
}

void
Service_DeleteMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_DeleteMonitoredItemsRequest *request,
                             UA_DeleteMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing DeleteMonitoredItemsRequest");

    if(server->config.maxMonitoredItemsPerCall != 0 &&
       request->monitoredItemIdsSize > server->config.maxMonitoredItemsPerCall) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    /* Get the subscription */
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the subscription lifetime */
    sub->currentLifetimeCount = 0;

    response->responseHeader.serviceResult =
        UA_Server_processServiceOperations(server, session,
                  (UA_ServiceOperation)Operation_DeleteMonitoredItem, sub,
                  &request->monitoredItemIdsSize, &UA_TYPES[UA_TYPES_UINT32],
                  &response->resultsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
}

void
Service_Republish(UA_Server *server, UA_Session *session, const UA_RepublishRequest *request,
                  UA_RepublishResponse *response) {
    UA_LOG_DEBUG_SESSION(server->config.logger, session, "Processing RepublishRequest");

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
