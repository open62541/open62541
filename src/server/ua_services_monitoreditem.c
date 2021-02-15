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
 *    Copyright 2020 (c) Kalycito Infotech Private Limited
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
                                  const UA_MonitoredItem *mon, UA_DataChangeFilter *filter) {
    /* A valid deadband? */
    if(filter->deadbandValue < 0.0 || filter->deadbandValue > 100.0)
        return UA_STATUSCODE_BADDEADBANDFILTERINVALID;

    /* Browse for the percent range */
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "EURange");
    UA_BrowsePathResult bpr =
        browseSimplifiedBrowsePath(server, mon->itemToMonitor.nodeId, 1, &qn);
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
    UA_Range *euRange = (UA_Range*)rangeVal.value.data;
    UA_Double absDeadband = (filter->deadbandValue/100.0) * (euRange->high - euRange->low);

    /* EURange invalid or NaN? */
    if(absDeadband < 0.0 || absDeadband != absDeadband) {
        UA_DataValue_clear(&rangeVal);
        return UA_STATUSCODE_BADFILTERNOTALLOWED;
    }

    /* Adjust the original filter */
    filter->deadbandType = UA_DEADBANDTYPE_ABSOLUTE;
    filter->deadbandValue = absDeadband;
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_DA */

void
Service_SetTriggering(UA_Server *server, UA_Session *session,
                      const UA_SetTriggeringRequest *request,
                      UA_SetTriggeringResponse *response) {
    /* Nothing to do? */
    if(request->linksToRemoveSize == 0 &&
       request->linksToAddSize == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADNOTHINGTODO;
        return;
    }

    /* Get the Subscription */
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }
    
    /* Get the MonitoredItem */
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(sub, request->triggeringItemId);
    if(!mon) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        return;
    }

    /* Allocate the results arrays */
    if(request->linksToRemoveSize > 0) {
        response->removeResults = (UA_StatusCode*)
            UA_Array_new(request->linksToRemoveSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
        if(!response->removeResults) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        response->removeResultsSize = request->linksToRemoveSize;
    }

    if(request->linksToAddSize> 0) {
        response->addResults = (UA_StatusCode*)
            UA_Array_new(request->linksToAddSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
        if(!response->addResults) {
            UA_Array_delete(response->removeResults,
                            request->linksToAddSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
            response->removeResults = NULL;
            response->removeResultsSize = 0;
            response->responseHeader.serviceResult = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
        response->addResultsSize = request->linksToAddSize;
    }

    /* Apply the changes */
    for(size_t i = 0; i < request->linksToRemoveSize; i++)
        response->removeResults[i] =
            UA_MonitoredItem_removeLink(sub, mon, request->linksToRemove[i]);

    for(size_t i = 0; i < request->linksToAddSize; i++)
        response->addResults[i] =
            UA_MonitoredItem_addLink(sub, mon, request->linksToAdd[i]);
}

/* Verify and adjust the parameters of a MonitoredItem */
static UA_StatusCode
checkAdjustMonitoredItemParams(UA_Server *server, UA_Session *session,
                               const UA_MonitoredItem *mon,
                               const UA_DataType* valueType,
                               UA_MonitoringParameters *params) {
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* Check the filter */
    if(mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
        /* Event MonitoredItems need a filter */
#ifndef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        return UA_STATUSCODE_BADNOTSUPPORTED;
#else
        if(params->filter.encoding != UA_EXTENSIONOBJECT_DECODED &&
           params->filter.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE)
            return UA_STATUSCODE_BADEVENTFILTERINVALID;
        if(params->filter.content.decoded.type != &UA_TYPES[UA_TYPES_EVENTFILTER])
            return UA_STATUSCODE_BADEVENTFILTERINVALID;
#endif
    } else {
        /* DataChange MonitoredItem. Can be "no filter" which defaults to
         * triggering on Status and Value. */
        if(params->filter.encoding != UA_EXTENSIONOBJECT_DECODED &&
           params->filter.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE &&
           params->filter.encoding != UA_EXTENSIONOBJECT_ENCODED_NOBODY)
            return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;

        /* If the filter ExtensionObject has a body, then it must be a
         * DataChangeFilter */
        if(params->filter.encoding != UA_EXTENSIONOBJECT_ENCODED_NOBODY &&
           params->filter.content.decoded.type != &UA_TYPES[UA_TYPES_DATACHANGEFILTER])
            return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;

        /* Check the deadband and adjust if necessary. */
        if(params->filter.content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGEFILTER]) {
            UA_DataChangeFilter *filter = (UA_DataChangeFilter *)
                params->filter.content.decoded.data;
            switch(filter->deadbandType) {
            case UA_DEADBANDTYPE_NONE:
                break;
            case UA_DEADBANDTYPE_ABSOLUTE:
                if(!valueType || !UA_DataType_isNumeric(valueType))
                    return UA_STATUSCODE_BADFILTERNOTALLOWED;
                break;
#ifdef UA_ENABLE_DA
            case UA_DEADBANDTYPE_PERCENT: {
                if(!valueType || !UA_DataType_isNumeric(valueType))
                    return UA_STATUSCODE_BADFILTERNOTALLOWED;
                /* If percentage deadband is supported, look up the range values
                 * and precompute as if it was an absolute deadband. */
                UA_StatusCode res =
                    setAbsoluteFromPercentageDeadband(server, session, mon, filter);
                if(res != UA_STATUSCODE_GOOD)
                    return res;
                break;
            }
#endif
            default:
                return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;
            }
        }
    }

    /* Read the minimum sampling interval for the variable */
    if(mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_VALUE) {
        const UA_Node *node = UA_NODESTORE_GET(server, &mon->itemToMonitor.nodeId);
        if(node) {
            const UA_VariableNode *vn = &node->variableNode;
            if(node->head.nodeClass == UA_NODECLASS_VARIABLE &&
               params->samplingInterval < vn->minimumSamplingInterval)
                params->samplingInterval = vn->minimumSamplingInterval;
            UA_NODESTORE_RELEASE(server, node);
        }
    }
        
    /* Adjust to sampling interval to lie within the limits and check for NaN */
    UA_BOUNDEDVALUE_SETWBOUNDS(server->config.samplingIntervalLimits,
                               params->samplingInterval, params->samplingInterval);
    if(mon->parameters.samplingInterval != mon->parameters.samplingInterval)
        params->samplingInterval = server->config.samplingIntervalLimits.min;

    /* Adjust the maximum queue size */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    if(mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
        /* 0 => Set to the configured maximum. Otherwise adjust with configured limits */
        if(params->queueSize == 0) {
            params->queueSize = server->config.queueSizeLimits.max;
        } else {
            UA_BOUNDEDVALUE_SETWBOUNDS(server->config.queueSizeLimits,
                                       params->queueSize, params->queueSize);
        }
    } else
#endif
    {
        /* 0 or 1 => queue-size 1. Otherwise adjust with configured limits */
        if(params->queueSize == 0)
            params->queueSize = 1;
        if(params->queueSize != 1)
            UA_BOUNDEDVALUE_SETWBOUNDS(server->config.queueSizeLimits,
                                       params->queueSize, params->queueSize);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setMonitoringMode(UA_Server *server, UA_MonitoredItem *mon,
                  UA_MonitoringMode monitoringMode);

static const UA_String
binaryEncoding = {sizeof("Default Binary") - 1, (UA_Byte *)"Default Binary"};

/* Structure to pass additional arguments into the operation */
struct createMonContext {
    UA_Subscription *sub;
    UA_TimestampsToReturn timestampsToReturn;

    /* If sub is NULL, use local callbacks */
    UA_Server_DataChangeNotificationCallback dataChangeCallback;
    void *context;
};

static void
Operation_CreateMonitoredItem(UA_Server *server, UA_Session *session,
                              struct createMonContext *cmc,
                              const UA_MonitoredItemCreateRequest *request,
                              UA_MonitoredItemCreateResult *result) {
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* Check available capacity */
    if(cmc->sub &&
       (((server->config.maxMonitoredItems != 0) &&
         (server->monitoredItemsSize >= server->config.maxMonitoredItems)) ||
        ((server->config.maxMonitoredItemsPerSubscription != 0) &&
         (cmc->sub->monitoredItemsSize >= server->config.maxMonitoredItemsPerSubscription)))) {
        result->statusCode = UA_STATUSCODE_BADTOOMANYMONITOREDITEMS;
        return;
    }

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

    /* Adding an Event MonitoredItem */
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    if(request->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
        /* TODO: Only remote clients can add Event-MonitoredItems at the moment */
        if(!cmc->sub) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Only remote clients can add Event-MonitoredItems");
            result->statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
            UA_DataValue_clear(&v);
            return;
        }

        /* If the 'SubscribeToEvents' bit of EventNotifier attribute is
         * zero, then the object cannot be subscribed to monitor events */
        if(!v.hasValue || !v.value.data) {
            result->statusCode = UA_STATUSCODE_BADINTERNALERROR;
            UA_DataValue_clear(&v);
            return;
        }
        UA_Byte eventNotifierValue = *((UA_Byte *)v.value.data);
        if((eventNotifierValue & 0x01) != 1) {
            result->statusCode = UA_STATUSCODE_BADNOTSUPPORTED;
            UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, cmc->sub,
                                     "Could not create a MonitoredItem as the "
                                     "'SubscribeToEvents' bit of the EventNotifier "
                                     "attribute is not set");
            UA_DataValue_clear(&v);
            return;
        }
    }
#endif

    const UA_DataType *valueType = v.value.type;
    UA_DataValue_clear(&v);

    /* Allocate the MonitoredItem */
    UA_MonitoredItem *newMon = NULL;
    if(cmc->sub) {
        newMon = (UA_MonitoredItem*)UA_malloc(sizeof(UA_MonitoredItem));
    } else {
        UA_LocalMonitoredItem *localMon = (UA_LocalMonitoredItem*)
            UA_malloc(sizeof(UA_LocalMonitoredItem));
        if(localMon) {
            /* Set special values only for the LocalMonitoredItem */
            localMon->context = cmc->context;
            localMon->callback.dataChangeCallback = cmc->dataChangeCallback;
        }
        newMon = &localMon->monitoredItem;
    }
    if(!newMon) {
        result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }

    /* Initialize the MonitoredItem */
    UA_MonitoredItem_init(newMon);
    newMon->subscription = cmc->sub; /* Can be NULL for local MonitoredItems */
    newMon->timestampsToReturn = cmc->timestampsToReturn;
    result->statusCode |= UA_ReadValueId_copy(&request->itemToMonitor,
                                              &newMon->itemToMonitor);
    result->statusCode |= UA_MonitoringParameters_copy(&request->requestedParameters,
                                                       &newMon->parameters);
    result->statusCode |= checkAdjustMonitoredItemParams(server, session, newMon,
                                                         valueType, &newMon->parameters);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, cmc->sub,
                                 "Could not create a MonitoredItem "
                                 "with StatusCode %s",
                                 UA_StatusCode_name(result->statusCode));
        UA_MonitoredItem_delete(server, newMon);
        return;
    }

    /* Register the Monitoreditem in the server and subscription. Add Events as
     * "listeners" to the monitored Node. */
    result->statusCode = UA_Server_registerMonitoredItem(server, newMon);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_MonitoredItem_delete(server, newMon);
        return;
    }

    /* Activate the MonitoredItem */
    result->statusCode |= setMonitoringMode(server, newMon, request->monitoringMode);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_MonitoredItem_delete(server, newMon);
        return;
    }

    UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, cmc->sub,
                             "MonitoredItem %" PRIi32 " | "
                             "Created the MonitoredItem "
                             "(Sampling Interval: %fms, Queue Size: %lu)",
                             newMon->monitoredItemId,
                             newMon->parameters.samplingInterval,
                             (unsigned long)newMon->queueSize);

    /* Create the first sample */
    if(request->monitoringMode > UA_MONITORINGMODE_DISABLED &&
       newMon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER)
        monitoredItem_sampleCallback(server, newMon);

    /* Prepare the response */
    result->revisedSamplingInterval = newMon->parameters.samplingInterval;
    result->revisedQueueSize = newMon->parameters.queueSize;
    result->monitoredItemId = newMon->monitoredItemId;
}

void
Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_CreateMonitoredItemsRequest *request,
                             UA_CreateMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing CreateMonitoredItemsRequest");
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

    /* Make local copy of the settings */
    UA_MonitoringParameters params;
    result->statusCode =
        UA_MonitoringParameters_copy(&request->requestedParameters, &params);
    if(result->statusCode != UA_STATUSCODE_GOOD)
        return;

    /* Read the current value to test if filters are possible.
     * Can return an empty value (v.value.type == NULL). */
    UA_DataValue v =
        UA_Server_readWithSession(server, session, &mon->itemToMonitor,
                                  mon->timestampsToReturn);

    /* Verify and adjust the new parameters. This still leaves the original
     * MonitoredItem untouched. */
    result->statusCode =
        checkAdjustMonitoredItemParams(server, session, mon,
                                       v.value.type, &params);
    UA_DataValue_clear(&v);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_MonitoringParameters_clear(&params);
        return;
    }

    /* Store the old sampling interval */
    UA_Double oldSamplingInterval = mon->parameters.samplingInterval;

    /* Move over the new settings */
    UA_MonitoringParameters_clear(&mon->parameters);
    mon->parameters = params;

    /* Re-register the callback if necessary */
    if(oldSamplingInterval != mon->parameters.samplingInterval) {
        UA_MonitoredItem_unregisterSampleCallback(server, mon);
        result->statusCode = setMonitoringMode(server, mon, mon->monitoringMode);
    }

    result->revisedSamplingInterval = mon->parameters.samplingInterval;
    result->revisedQueueSize = mon->parameters.queueSize;

    /* Remove some notifications if the queue is now too small */
    UA_MonitoredItem_ensureQueueSpace(server, mon);

    UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, sub,
                             "MonitoredItem %" PRIi32 " | "
                             "Modified the MonitoredItem "
                             "(Sampling Interval: %fms, Queue Size: %lu)",
                             mon->monitoredItemId,
                             mon->parameters.samplingInterval,
                             (unsigned long)mon->queueSize);
}

void
Service_ModifyMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_ModifyMonitoredItemsRequest *request,
                             UA_ModifyMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing ModifyMonitoredItemsRequest");
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

static UA_StatusCode
setMonitoringMode(UA_Server *server, UA_MonitoredItem *mon,
                  UA_MonitoringMode monitoringMode) {
    /* Check if the MonitoringMode is valid or not */
    if(monitoringMode > UA_MONITORINGMODE_REPORTING)
        return UA_STATUSCODE_BADMONITORINGMODEINVALID;

    /* Set the MonitoringMode */
    mon->monitoringMode = monitoringMode;

    UA_Notification *notification;
    /* Reporting is disabled. This causes all Notifications to be dequeued and
     * deleted. Also remove the last samples so that we immediately generate a
     * Notification when re-activated. */
    if(mon->monitoringMode == UA_MONITORINGMODE_DISABLED) {
        UA_Notification *notification_tmp;
        UA_MonitoredItem_unregisterSampleCallback(server, mon);
        TAILQ_FOREACH_SAFE(notification, &mon->queue, listEntry, notification_tmp)
            UA_Notification_delete(server, notification);
        UA_ByteString_clear(&mon->lastSampledValue);
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
        TAILQ_FOREACH(notification, &mon->queue, listEntry) {
            UA_Notification_dequeueSub(notification);
            UA_Notification_enqueueSub(notification);
        }
    } else /* mon->monitoringMode == UA_MONITORINGMODE_SAMPLING */ {
        /* Make all notifications non-reporting */
        TAILQ_FOREACH(notification, &mon->queue, listEntry)
            UA_Notification_dequeueSub(notification);
    }

    /* Register the sampling callback with an interval. If registering the
     * sampling callback failed, set to disabled. But don't delete the current
     * notifications. */
    UA_StatusCode res = UA_MonitoredItem_registerSampleCallback(server, mon);
    if(res != UA_STATUSCODE_GOOD)
        mon->monitoringMode = UA_MONITORINGMODE_DISABLED;
    return res;
}

static void
Operation_SetMonitoringMode(UA_Server *server, UA_Session *session,
                            struct setMonitoringContext *smc,
                            const UA_UInt32 *monitoredItemId, UA_StatusCode *result) {
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(smc->sub, *monitoredItemId);
    if(!mon) {
        *result = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        return;
    }
    *result = setMonitoringMode(server, mon, smc->monitoringMode);
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
    UA_LOCK_ASSERT(server->serviceMutex, 1);
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(sub, *monitoredItemId);
    if(!mon) {
        *result = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
        return;
    }
    UA_MonitoredItem_delete(server, mon);
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
    UA_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &server->localMonitoredItems, listEntry, mon_tmp) {
        if(mon->monitoredItemId != monitoredItemId)
            continue;
        UA_MonitoredItem_delete(server, mon);
        UA_UNLOCK(server->serviceMutex);
        return UA_STATUSCODE_GOOD;
    }
    UA_UNLOCK(server->serviceMutex);
    return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
