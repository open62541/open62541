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
 */

#include "ua_server_internal.h"
#include "ua_services.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

#ifdef UA_ENABLE_DA

/* Translate a percentage deadband into an absolute deadband based on the
 * UARange property of the variable */
static UA_StatusCode
setAbsoluteFromPercentageDeadband(UA_Server *server, UA_Session *session,
                                  UA_MonitoredItem *mon, UA_DataChangeFilter *filter) {
    /* A valid deadband? */
    if(filter->deadbandValue < 0.0 || filter->deadbandValue > 100.0)
        return UA_STATUSCODE_BADDEADBANDFILTERINVALID;

    /* Browse for the percent range */
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "EURange");
    UA_BrowsePathResult bpr =
        browseSimplifiedBrowsePath(server, mon->monitoredNodeId, 1, &qn);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_BrowsePathResult_clear(&bpr);
        return UA_STATUSCODE_BADFILTERNOTALLOWED;
    }

    /* Read the range */
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = bpr.targets->targetId.nodeId;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue rangeVal = UA_Server_readWithSession(server, session, &rvi,
                                                      UA_TIMESTAMPSTORETURN_NEITHER);
    UA_BrowsePathResult_clear(&bpr);
    if(!UA_Variant_isScalar(&rangeVal.value) ||
       rangeVal.value.type != &UA_TYPES[UA_TYPES_RANGE]) {
        UA_DataValue_clear(&rangeVal);
        return UA_STATUSCODE_BADFILTERNOTALLOWED;
    }

    /* Compute the abs deadband */
    UA_Range* euRange = (UA_Range*)rangeVal.value.data;
    UA_Double absDeadband =
        (filter->deadbandValue/100.0) * (euRange->high - euRange->low);

    /* EURange invalid or NaN? */
    if(absDeadband < 0.0 || absDeadband != absDeadband) {
        UA_DataValue_clear(&rangeVal);
        return UA_STATUSCODE_BADFILTERNOTALLOWED;
    }

    mon->filter.dataChangeFilter.trigger = filter->trigger;
    mon->filter.dataChangeFilter.deadbandType = UA_DEADBANDTYPE_ABSOLUTE;
    mon->filter.dataChangeFilter.deadbandValue = absDeadband;
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_DA */

static UA_StatusCode
setMonitoredItemSettings(UA_Server *server, UA_Session *session, UA_MonitoredItem *mon,
                         UA_MonitoringMode monitoringMode,
                         const UA_MonitoringParameters *params,
                         const UA_DataType* dataType) {
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    if(mon->attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
        /* Event MonitoredItem */
#ifndef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        return UA_STATUSCODE_BADNOTSUPPORTED;
#else
        if(params->filter.encoding != UA_EXTENSIONOBJECT_DECODED &&
           params->filter.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE)
            return UA_STATUSCODE_BADEVENTFILTERINVALID;
        if(params->filter.content.decoded.type != &UA_TYPES[UA_TYPES_EVENTFILTER])
            return UA_STATUSCODE_BADEVENTFILTERINVALID;
        UA_EventFilter_clear(&mon->filter.eventFilter);
        retval = UA_EventFilter_copy((UA_EventFilter *)params->filter.content.decoded.data,
                                     &mon->filter.eventFilter);
#endif
    } else {
        /* DataChange MonitoredItem */
        if(params->filter.encoding != UA_EXTENSIONOBJECT_DECODED &&
           params->filter.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) {
            /* Default: Look for status and value */
            UA_DataChangeFilter_clear(&mon->filter.dataChangeFilter);
            mon->filter.dataChangeFilter.trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
        } else if(params->filter.content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGEFILTER]) {
            UA_DataChangeFilter *filter = (UA_DataChangeFilter *)params->filter.content.decoded.data;
            switch(filter->deadbandType) {
            case UA_DEADBANDTYPE_NONE:
                mon->filter.dataChangeFilter = *filter;
                break;
            case UA_DEADBANDTYPE_ABSOLUTE:
                if(!dataType || !UA_DataType_isNumeric(dataType))
                    return UA_STATUSCODE_BADFILTERNOTALLOWED;
                mon->filter.dataChangeFilter = *filter;
                break;
            case UA_DEADBANDTYPE_PERCENT:
#ifdef UA_ENABLE_DA
                if(!dataType || !UA_DataType_isNumeric(dataType))
                    return UA_STATUSCODE_BADFILTERNOTALLOWED;
                retval = setAbsoluteFromPercentageDeadband(server, session, mon, filter);
                break;
#else
                return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;
#endif
            default:
                return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;
            }
        } else {
            return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;
        }
    }

    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* <-- The point of no return --> */

    /* Unregister the callback */
    UA_MonitoredItem_unregisterSampleCallback(server, mon);

    /* Remove the old samples */
    UA_ByteString_clear(&mon->lastSampledValue);
    UA_Variant_clear(&mon->lastValue);

    /* ClientHandle */
    mon->clientHandle = params->clientHandle;

    /* SamplingInterval */
    UA_Double samplingInterval = params->samplingInterval;

    if(mon->attributeId == UA_ATTRIBUTEID_VALUE) {
        const UA_VariableNode *vn = (const UA_VariableNode *)
            UA_NODESTORE_GET(server, &mon->monitoredNodeId);
        if(vn) {
            if(vn->nodeClass == UA_NODECLASS_VARIABLE &&
               samplingInterval < vn->minimumSamplingInterval)
                samplingInterval = vn->minimumSamplingInterval;
            UA_NODESTORE_RELEASE(server, (const UA_Node *)vn);
        }
    }

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
    mon->monitoringMode = monitoringMode;
    if(monitoringMode == UA_MONITORINGMODE_SAMPLING ||
       monitoringMode == UA_MONITORINGMODE_REPORTING)
        return UA_MonitoredItem_registerSampleCallback(server, mon);

    return UA_STATUSCODE_GOOD;
}

static const UA_String binaryEncoding = {sizeof("Default Binary") - 1, (UA_Byte *)"Default Binary"};

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
static UA_StatusCode
UA_Server_addMonitoredItemToNodeEditNodeCallback(UA_Server *server, UA_Session *session,
                                                 UA_Node *node, void *data) {
    /* data is the MonitoredItem */
    /* SLIST_INSERT_HEAD */
    ((UA_MonitoredItem *)data)->next = ((UA_ObjectNode *)node)->monitoredItemQueue;
    ((UA_ObjectNode *)node)->monitoredItemQueue = (UA_MonitoredItem *)data;
    return UA_STATUSCODE_GOOD;
}
#endif

/* Thread-local variables to pass additional arguments into the operation */
struct createMonContext {
    UA_Subscription *sub;
    UA_TimestampsToReturn timestampsToReturn;

    /* If sub is NULL, use local callbacks */
    UA_Server_DataChangeNotificationCallback dataChangeCallback;
    void *context;
};

static void
Operation_CreateMonitoredItem(UA_Server *server, UA_Session *session, struct createMonContext *cmc,
                              const UA_MonitoredItemCreateRequest *request,
                              UA_MonitoredItemCreateResult *result) {
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* Check available capacity */
    if(cmc->sub &&
       (((server->config.maxMonitoredItems != 0) &&
         (server->numMonitoredItems >= server->config.maxMonitoredItems)) ||
        ((server->config.maxMonitoredItemsPerSubscription != 0) &&
         (cmc->sub->monitoredItemsSize >= server->config.maxMonitoredItemsPerSubscription)))) {
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
        UA_DataValue_clear(&v);
        return;
    }

    /* Check if the encoding is supported */
    if(request->itemToMonitor.dataEncoding.name.length > 0 &&
       (!UA_String_equal(&binaryEncoding, &request->itemToMonitor.dataEncoding.name) ||
        request->itemToMonitor.dataEncoding.namespaceIndex != 0)) {
        result->statusCode = UA_STATUSCODE_BADDATAENCODINGUNSUPPORTED;
        UA_DataValue_clear(&v);
        return;
    }

    /* Check if the encoding is set for a value */
    if(request->itemToMonitor.attributeId != UA_ATTRIBUTEID_VALUE &&
       request->itemToMonitor.dataEncoding.name.length > 0) {
        result->statusCode = UA_STATUSCODE_BADDATAENCODINGINVALID;
        UA_DataValue_clear(&v);
        return;
    }

    /* Allocate the MonitoredItem */
    size_t nmsize = sizeof(UA_MonitoredItem);
    if(!cmc->sub)
        nmsize = sizeof(UA_LocalMonitoredItem);
    UA_MonitoredItem *newMon = (UA_MonitoredItem*)UA_malloc(nmsize);
    if(!newMon) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        UA_DataValue_clear(&v);
        return;
    }

    /* Initialize the MonitoredItem */
    UA_MonitoredItem_init(newMon, cmc->sub);
    newMon->attributeId = request->itemToMonitor.attributeId;
    newMon->timestampsToReturn = cmc->timestampsToReturn;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_NodeId_copy(&request->itemToMonitor.nodeId, &newMon->monitoredNodeId);
    retval |= UA_String_copy(&request->itemToMonitor.indexRange, &newMon->indexRange);
    retval |= setMonitoredItemSettings(server, session, newMon, request->monitoringMode,
                                       &request->requestedParameters, v.value.type);
    UA_DataValue_clear(&v);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SESSION(&server->config.logger, session,
                            "Subscription %" PRIu32 " | Could not create a MonitoredItem "
                            "with StatusCode %s", cmc->sub ? cmc->sub->subscriptionId : 0,
                            UA_StatusCode_name(retval));
        result->statusCode = retval;
        UA_MonitoredItem_delete(server, newMon);
        return;
    }

    /* Add to the subscriptions or the local MonitoredItems */
    if(cmc->sub) {
        newMon->monitoredItemId = ++cmc->sub->lastMonitoredItemId;
        UA_Subscription_addMonitoredItem(server, cmc->sub, newMon);
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        if(newMon->attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
            /* Insert the monitored item into the node's queue */
            UA_Server_editNode(server, NULL, &newMon->monitoredNodeId,
                               UA_Server_addMonitoredItemToNodeEditNodeCallback, newMon);
        }
#endif
    } else {
        //TODO support events for local monitored items
        UA_LocalMonitoredItem *localMon = (UA_LocalMonitoredItem*)newMon;
        localMon->context = cmc->context;
        localMon->callback.dataChangeCallback = cmc->dataChangeCallback;
        newMon->monitoredItemId = ++server->lastLocalMonitoredItemId;
        LIST_INSERT_HEAD(&server->localMonitoredItems, newMon, listEntry);
    }

    /* Register MonitoredItem in userland */
    if(server->config.monitoredItemRegisterCallback) {
        void *targetContext = NULL;
        getNodeContext(server, request->itemToMonitor.nodeId, &targetContext);
        UA_UNLOCK(server->serviceMutex);
        server->config.monitoredItemRegisterCallback(server, &session->sessionId,
                                                     session->sessionHandle,
                                                     &request->itemToMonitor.nodeId,
                                                     targetContext, newMon->attributeId, false);
        UA_LOCK(server->serviceMutex);
        newMon->registered = true;
    }

    UA_LOG_INFO_SESSION(&server->config.logger, session,
                        "Subscription %" PRIu32 " | MonitoredItem %" PRIi32 " | "
                        "Created the MonitoredItem",
                        cmc->sub ? cmc->sub->subscriptionId : 0,
                        newMon->monitoredItemId);

    /* Create the first sample */
    if(request->monitoringMode == UA_MONITORINGMODE_REPORTING &&
       newMon->attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER)
        monitoredItem_sampleCallback(server, newMon);

    /* Prepare the response */
    result->revisedSamplingInterval = newMon->samplingInterval;
    result->revisedQueueSize = newMon->maxQueueSize;
    result->monitoredItemId = newMon->monitoredItemId;
}

void
Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_CreateMonitoredItemsRequest *request,
                             UA_CreateMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing CreateMonitoredItemsRequest");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

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

UA_MonitoredItemCreateResult
UA_Server_createDataChangeMonitoredItem(UA_Server *server,
                                        UA_TimestampsToReturn timestampsToReturn,
                                        const UA_MonitoredItemCreateRequest item,
                                        void *monitoredItemContext,
                                        UA_Server_DataChangeNotificationCallback callback) {
    struct createMonContext cmc;
    cmc.sub = NULL;
    cmc.context = monitoredItemContext;
    cmc.dataChangeCallback = callback;
    cmc.timestampsToReturn = timestampsToReturn;

    UA_MonitoredItemCreateResult result;
    UA_MonitoredItemCreateResult_init(&result);
    UA_LOCK(server->serviceMutex);
    Operation_CreateMonitoredItem(server, &server->adminSession, &cmc, &item, &result);
    UA_UNLOCK(server->serviceMutex);
    return result;
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

    /* Read the current value to test if filters are possible.
     * Can return an empty value (v.value.type == NULL). */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = mon->monitoredNodeId;
    rvid.attributeId = mon->attributeId;
    rvid.indexRange = mon->indexRange;
    UA_DataValue v = UA_Server_readWithSession(server, session, &rvid, mon->timestampsToReturn);
    UA_StatusCode retval = setMonitoredItemSettings(server, session, mon, mon->monitoringMode,
                                                    &request->requestedParameters,
                                                    v.value.type);
    UA_DataValue_clear(&v);
    if(retval != UA_STATUSCODE_GOOD) {
        result->statusCode = retval;
        return;
    }

    result->revisedSamplingInterval = mon->samplingInterval;
    result->revisedQueueSize = mon->maxQueueSize;

    /* Remove some notifications if the queue is now too small */
    UA_MonitoredItem_ensureQueueSpace(server, mon);
}

void
Service_ModifyMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_ModifyMonitoredItemsRequest *request,
                             UA_ModifyMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing ModifyMonitoredItemsRequest");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

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
                            const UA_UInt32 *monitoredItemId, UA_StatusCode *result) {
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(smc->sub, *monitoredItemId);
    if(!mon) {
        *result = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        return;
    }
    UA_Subscription *sub = mon->subscription;

    /* Check if the MonitoringMode is valid or not */
    if(smc->monitoringMode > UA_MONITORINGMODE_REPORTING) {
        *result = UA_STATUSCODE_BADMONITORINGMODEINVALID;
        return;
    }

    /* Nothing has changed */
    if(mon->monitoringMode == smc->monitoringMode)
        return;

    mon->monitoringMode = smc->monitoringMode;

    /* When reporting is enabled, put all notifications that were already
     * sampled into the global queue of the subscription. When sampling is
     * enabled, remove all notifications from the global queue. !!! This needs
     * to be the same operation as in UA_Notification_enqueue !!! */
    if(mon->monitoringMode == UA_MONITORINGMODE_REPORTING) {
        UA_Notification *notification;
        TAILQ_FOREACH(notification, &mon->queue, listEntry) {
            TAILQ_INSERT_TAIL(&sub->notificationQueue, notification, globalEntry);
            ++sub->notificationQueueSize;
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
            if(mon->attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
                ++sub->eventNotifications;
            } else
#endif
            {
                ++sub->dataChangeNotifications;
            }
        }
        /* Register the sampling callback with an interval */
        *result = UA_MonitoredItem_registerSampleCallback(server, mon);
    } else if(mon->monitoringMode == UA_MONITORINGMODE_SAMPLING) {
        UA_Notification *notification;
        TAILQ_FOREACH(notification, &mon->queue, listEntry) {
            TAILQ_REMOVE(&sub->notificationQueue, notification, globalEntry);
            TAILQ_NEXT(notification, globalEntry) = UA_SUBSCRIPTION_QUEUE_SENTINEL;
            --sub->notificationQueueSize;
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
            if(mon->attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
                --sub->eventNotifications;
            } else
#endif
            {
                --sub->dataChangeNotifications;
            }
        }
        /* Register the sampling callback with an interval */
        *result = UA_MonitoredItem_registerSampleCallback(server, mon);
    } else {
        /* UA_MONITORINGMODE_DISABLED */
        UA_MonitoredItem_unregisterSampleCallback(server, mon);

        /* Setting the mode to DISABLED or SAMPLING causes all queued Notifications to be deleted */
        UA_Notification *notification, *notification_tmp;
        TAILQ_FOREACH_SAFE(notification, &mon->queue, listEntry, notification_tmp) {
            UA_Notification_dequeue(server, notification);
            UA_Notification_delete(notification);
        }

        /* Initialize lastSampledValue */
        UA_ByteString_clear(&mon->lastSampledValue);
        UA_Variant_clear(&mon->lastValue);
    }
}

void
Service_SetMonitoringMode(UA_Server *server, UA_Session *session,
                          const UA_SetMonitoringModeRequest *request,
                          UA_SetMonitoringModeResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing SetMonitoringMode");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

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

static void
Operation_DeleteMonitoredItem(UA_Server *server, UA_Session *session, UA_Subscription *sub,
                              const UA_UInt32 *monitoredItemId, UA_StatusCode *result) {
    *result = UA_Subscription_deleteMonitoredItem(server, sub, *monitoredItemId);
}

void
Service_DeleteMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_DeleteMonitoredItemsRequest *request,
                             UA_DeleteMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing DeleteMonitoredItemsRequest");
    UA_LOCK_ASSERT(server->serviceMutex, 1);

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

UA_StatusCode
UA_Server_deleteMonitoredItem(UA_Server *server, UA_UInt32 monitoredItemId) {
    UA_LOCK(server->serviceMutex);
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &server->localMonitoredItems, listEntry) {
        if(mon->monitoredItemId != monitoredItemId)
            continue;
        LIST_REMOVE(mon, listEntry);
        UA_MonitoredItem_delete(server, mon);
        UA_UNLOCK(server->serviceMutex);
        return UA_STATUSCODE_GOOD;
    }
    UA_UNLOCK(server->serviceMutex);
    return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
