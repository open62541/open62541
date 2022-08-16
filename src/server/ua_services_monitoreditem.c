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
 *    Copyright 2021 (c) Uranik, Berisha
 *    Copyright 2021 (c) Ammar, Morshed
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Andreas Ebner)
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

    UA_DataValue_clear(&rangeVal);

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
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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
            return UA_STATUSCODE_BADFILTERNOTALLOWED;

        /* Check the deadband and adjust if necessary. */
        if(params->filter.content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGEFILTER]) {
            UA_DataChangeFilter *filter = (UA_DataChangeFilter *)
                params->filter.content.decoded.data;
            switch(filter->deadbandType) {
            case UA_DEADBANDTYPE_NONE:
                break;
            case UA_DEADBANDTYPE_ABSOLUTE:
                if(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_VALUE ||
                   !valueType || !UA_DataType_isNumeric(valueType))
                    return UA_STATUSCODE_BADFILTERNOTALLOWED;
                break;
#ifdef UA_ENABLE_DA
            case UA_DEADBANDTYPE_PERCENT: {
                if(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_VALUE ||
                   !valueType || !UA_DataType_isNumeric(valueType))
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

    /* Read the minimum sampling interval for the variable. The sampling
     * interval of the MonitoredItem must not be less than that. */
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

    /* Adjust to sampling interval to lie within the limits */
    if(params->samplingInterval <= 0.0) {
        /* A sampling interval of zero is possible and indicates that the
         * MonitoredItem is checked for every write operation */
        params->samplingInterval = 0.0;
    } else {
        UA_BOUNDEDVALUE_SETWBOUNDS(server->config.samplingIntervalLimits,
                                   params->samplingInterval, params->samplingInterval);
        /* Check for NaN */
        if(mon->parameters.samplingInterval != mon->parameters.samplingInterval)
            params->samplingInterval = server->config.samplingIntervalLimits.min;
    }

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

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
static UA_StatusCode
checkEventFilterParam(UA_Server *server, UA_Session *session,
                      const UA_MonitoredItem *mon,
                      UA_MonitoringParameters *params){
    if(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER)
        return UA_STATUSCODE_GOOD;

    UA_EventFilter *eventFilter =
        (UA_EventFilter *) params->filter.content.decoded.data;

    if(eventFilter == NULL)
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    //TODO make the maximum select clause size param an server-config parameter
    if(eventFilter->selectClausesSize > 128)
        return UA_STATUSCODE_BADCONFIGURATIONERROR;

    //check the where clause for logical consistency
    if(eventFilter->whereClause.elementsSize != 0) {
        UA_ContentFilterResult contentFilterResult;
        UA_Event_staticWhereClauseValidation(server, &eventFilter->whereClause,
                                             &contentFilterResult);
        for(size_t i = 0; i < contentFilterResult.elementResultsSize; ++i) {
            if(contentFilterResult.elementResults[i].statusCode != UA_STATUSCODE_GOOD){
                //ToDo currently we return the first non good status code, check if
                //we can use the detailed contentFilterResult later
                UA_StatusCode whereResult =
                    contentFilterResult.elementResults[i].statusCode;
                UA_ContentFilterResult_clear(&contentFilterResult);
                return whereResult;
            }
        }
        UA_ContentFilterResult_clear(&contentFilterResult);
    }
    //check the select clause for logical consistency
    UA_StatusCode selectClauseValidationResult[128];
    UA_Event_staticSelectClauseValidation(server,eventFilter,
                                          selectClauseValidationResult);
    for(size_t i = 0; i < eventFilter->selectClausesSize; ++i){
        //ToDo currently we return the first non good status code, check if
        //we can use the detailed status code list later
        if(selectClauseValidationResult[i] != UA_STATUSCODE_GOOD){
            return selectClauseValidationResult[i];
        }
    }

    return UA_STATUSCODE_GOOD;
}
#endif

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
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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

    /* Make an example read to check the itemToMonitor. The DataSource itself
     * could return a (temporary) error. This should still result in a valid
     * MonitoredItem. Only a few StatusCodes are considered unrecoverable and
     * lead to an abort:
     * - The Node does not exist
     * - The AttributeId does not match the NodeClass
     * - The Session does not have sufficient access rights
     * - The indicated encoding is not supported or not valid */
    UA_DataValue v = UA_Server_readWithSession(server, session, &request->itemToMonitor,
                                               cmc->timestampsToReturn);
    if(v.hasStatus &&
       (v.status == UA_STATUSCODE_BADNODEIDUNKNOWN ||
        v.status == UA_STATUSCODE_BADATTRIBUTEIDINVALID ||
        v.status == UA_STATUSCODE_BADDATAENCODINGUNSUPPORTED ||
        v.status == UA_STATUSCODE_BADDATAENCODINGINVALID ||
        v.status == UA_STATUSCODE_BADINDEXRANGEINVALID
        /* Part 4, 5.12.2 CreateMonitoredItems: When a user adds a monitored
         * item that the user is denied read access to, the add operation for
         * the item shall succeed and the bad status Bad_NotReadable or
         * Bad_UserAccessDenied shall be returned in the Publish response.
         * v.status == UA_STATUSCODE_BADNOTREADABLE
         * v.status == UA_STATUSCODE_BADUSERACCESSDENIED
         *
         * The IndexRange error can change depending on the value.
         * v.status == UA_STATUSCODE_BADINDEXRANGENODATA */
        )) {
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
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    result->statusCode |= checkEventFilterParam(server, session, newMon,
                                                         &newMon->parameters);
#endif
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, cmc->sub,
                                 "Could not create a MonitoredItem "
                                 "with StatusCode %s",
                                 UA_StatusCode_name(result->statusCode));
        UA_MonitoredItem_delete(server, newMon);
        return;
    }

    /* Initialize the value status so the first sample always passes the filter */
    newMon->lastValue.hasStatus = true;
    newMon->lastValue.status = ~(UA_StatusCode)0;

    /* Register the Monitoreditem in the server and subscription */
    UA_Server_registerMonitoredItem(server, newMon);

    /* Activate the MonitoredItem */
    result->statusCode |=
        UA_MonitoredItem_setMonitoringMode(server, newMon, request->monitoringMode);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_MonitoredItem_delete(server, newMon);
        return;
    }

    /* Prepare the response */
    result->revisedSamplingInterval = newMon->parameters.samplingInterval;
    result->revisedQueueSize = newMon->parameters.queueSize;
    result->monitoredItemId = newMon->monitoredItemId;

    UA_LOG_INFO_SUBSCRIPTION(&server->config.logger, cmc->sub,
                             "MonitoredItem %" PRIi32 " | "
                             "Created the MonitoredItem "
                             "(Sampling Interval: %.2fms, Queue Size: %lu)",
                             newMon->monitoredItemId,
                             newMon->parameters.samplingInterval,
                             (unsigned long)newMon->parameters.queueSize);
}

void
Service_CreateMonitoredItems(UA_Server *server, UA_Session *session,
                             const UA_CreateMonitoredItemsRequest *request,
                             UA_CreateMonitoredItemsResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session,
                         "Processing CreateMonitoredItemsRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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
        UA_Server_processServiceOperations(server, session,
                                           (UA_ServiceOperation)Operation_CreateMonitoredItem,
                                           &cmc, &request->itemsToCreateSize,
                                           &UA_TYPES[UA_TYPES_MONITOREDITEMCREATEREQUEST],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_MONITOREDITEMCREATERESULT]);
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
    UA_LOCK(&server->serviceMutex);
    Operation_CreateMonitoredItem(server, &server->adminSession, &cmc, &item, &result);
    UA_UNLOCK(&server->serviceMutex);
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
        UA_MonitoredItem_unregisterSampling(server, mon);
        result->statusCode =
            UA_MonitoredItem_setMonitoringMode(server, mon, mon->monitoringMode);
    }

    result->revisedSamplingInterval = mon->parameters.samplingInterval;
    result->revisedQueueSize = mon->parameters.queueSize;

    /* Remove some notifications if the queue is now too small */
    UA_MonitoredItem_ensureQueueSpace(server, mon);

    /* Remove the overflow bits if the queue has now a size of 1 */
    UA_MonitoredItem_removeOverflowInfoBits(mon);

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
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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
                                           (UA_ServiceOperation)Operation_ModifyMonitoredItem,
                                           sub, &request->itemsToModifySize,
                                           &UA_TYPES[UA_TYPES_MONITOREDITEMMODIFYREQUEST],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_MONITOREDITEMMODIFYRESULT]);
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
    *result = UA_MonitoredItem_setMonitoringMode(server, mon, smc->monitoringMode);
}

void
Service_SetMonitoringMode(UA_Server *server, UA_Session *session,
                          const UA_SetMonitoringModeRequest *request,
                          UA_SetMonitoringModeResponse *response) {
    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Processing SetMonitoringMode");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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
                                           (UA_ServiceOperation)Operation_SetMonitoringMode,
                                           &smc, &request->monitoredItemIdsSize,
                                           &UA_TYPES[UA_TYPES_UINT32],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_STATUSCODE]);
}

static void
Operation_DeleteMonitoredItem(UA_Server *server, UA_Session *session, UA_Subscription *sub,
                              const UA_UInt32 *monitoredItemId, UA_StatusCode *result) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
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
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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
                                           (UA_ServiceOperation)Operation_DeleteMonitoredItem,
                                           sub, &request->monitoredItemIdsSize,
                                           &UA_TYPES[UA_TYPES_UINT32],
                                           &response->resultsSize,
                                           &UA_TYPES[UA_TYPES_STATUSCODE]);
}

UA_StatusCode
UA_Server_deleteMonitoredItem(UA_Server *server, UA_UInt32 monitoredItemId) {
    UA_LOCK(&server->serviceMutex);
    UA_MonitoredItem *mon, *mon_tmp;
    LIST_FOREACH_SAFE(mon, &server->localMonitoredItems, listEntry, mon_tmp) {
        if(mon->monitoredItemId != monitoredItemId)
            continue;
        UA_MonitoredItem_delete(server, mon);
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_GOOD;
    }
    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
