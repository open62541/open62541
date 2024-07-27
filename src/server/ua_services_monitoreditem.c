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
 *    Copyright 2017-2023 (c) Thomas Stalder, Blue Time Concept SA
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
        return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;

    /* Browse for the percent range */
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "EURange");
    UA_BrowsePathResult bpr =
        browseSimplifiedBrowsePath(server, mon->itemToMonitor.nodeId, 1, &qn);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_BrowsePathResult_clear(&bpr);
        return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;
    }

    /* Read the range */
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = bpr.targets->targetId.nodeId;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue rangeVal = readWithSession(server, session, &rvi,
                                            UA_TIMESTAMPSTORETURN_NEITHER);
    UA_BrowsePathResult_clear(&bpr);
    if(!UA_Variant_isScalar(&rangeVal.value) ||
       rangeVal.value.type != &UA_TYPES[UA_TYPES_RANGE]) {
        UA_DataValue_clear(&rangeVal);
        return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;
    }

    /* Compute the abs deadband */
    UA_Range *euRange = (UA_Range*)rangeVal.value.data;
    UA_Double absDeadband = (filter->deadbandValue/100.0) * (euRange->high - euRange->low);

    UA_DataValue_clear(&rangeVal);

    /* EURange invalid or NaN? */
    if(absDeadband < 0.0 || absDeadband != absDeadband) {
        UA_DataValue_clear(&rangeVal);
        return UA_STATUSCODE_BADMONITOREDITEMFILTERUNSUPPORTED;
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

    /* Reset the lifetime counter of the Subscription */
    Subscription_resetLifetime(sub);

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

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
static UA_StatusCode
checkEventFilterParam(UA_Server *server, UA_Session *session,
                      const UA_MonitoredItem *mon,
                      UA_MonitoringParameters *params,
                      UA_ExtensionObject *filterResult) {
    UA_assert(mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER);

    /* Correct data type? */
    if(params->filter.encoding != UA_EXTENSIONOBJECT_DECODED &&
       params->filter.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE)
        return UA_STATUSCODE_BADEVENTFILTERINVALID;
    if(params->filter.content.decoded.type != &UA_TYPES[UA_TYPES_EVENTFILTER])
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    UA_EventFilter *eventFilter = (UA_EventFilter *)params->filter.content.decoded.data;

    /* Correct number of elements? */
    if(eventFilter->selectClausesSize == 0 ||
       eventFilter->selectClausesSize > UA_EVENTFILTER_MAXSELECT)
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    /* Allow empty where clauses --> select every event */
    if(eventFilter->whereClause.elementsSize > UA_EVENTFILTER_MAXELEMENTS)
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    /* Check where-clause */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    const UA_ContentFilter *cf = &eventFilter->whereClause;
    UA_ContentFilterElementResult whereRes[UA_EVENTFILTER_MAXELEMENTS];
    for(size_t i = 0; i < cf->elementsSize; ++i) {
        UA_ContentFilterElement *ef = &cf->elements[i];
        whereRes[i] = UA_ContentFilterElementValidation(server, i, cf->elementsSize, ef);
        if(whereRes[i].statusCode != UA_STATUSCODE_GOOD && res == UA_STATUSCODE_GOOD)
            res = whereRes[i].statusCode;
    }

    /* Check select clause */
    UA_StatusCode selectRes[UA_EVENTFILTER_MAXSELECT];
    for(size_t i = 0; i < eventFilter->selectClausesSize; i++) {
        const UA_SimpleAttributeOperand *sao = &eventFilter->selectClauses[i];
        selectRes[i] = UA_SimpleAttributeOperandValidation(server, sao);
        if(selectRes[i] != UA_STATUSCODE_GOOD && res == UA_STATUSCODE_GOOD)
            res = selectRes[i];
    }

    /* Filter bad, return details */
    if(res != UA_STATUSCODE_GOOD) {
        UA_EventFilterResult *efr = UA_EventFilterResult_new();
        if(!efr) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
        } else {
            UA_EventFilterResult tmp_efr;
            UA_EventFilterResult_init(&tmp_efr);
            tmp_efr.selectClauseResultsSize = eventFilter->selectClausesSize;
            tmp_efr.selectClauseResults = selectRes;
            tmp_efr.whereClauseResult.elementResultsSize = cf->elementsSize;
            tmp_efr.whereClauseResult.elementResults = whereRes;
            UA_EventFilterResult_copy(&tmp_efr, efr);
            UA_ExtensionObject_setValue(filterResult, efr,
                                        &UA_TYPES[UA_TYPES_EVENTFILTERRESULT]);
        }
    }

    for(size_t i = 0; i < cf->elementsSize; ++i)
        UA_ContentFilterElementResult_clear(&whereRes[i]);
    return res;
}
#endif

/* Verify and adjust the parameters of a MonitoredItem */
static UA_StatusCode
checkAdjustMonitoredItemParams(UA_Server *server, UA_Session *session,
                               const UA_MonitoredItem *mon,
                               const UA_DataType* valueType,
                               UA_MonitoringParameters *params,
                               UA_ExtensionObject *filterResult) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Check the filter */
    if(mon->itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
        /* Event MonitoredItems need a filter */
#ifndef UA_ENABLE_SUBSCRIPTIONS_EVENTS
        return UA_STATUSCODE_BADNOTSUPPORTED;
#else
        UA_StatusCode res = checkEventFilterParam(server, session, mon,
                                                  params, filterResult);
        if(res != UA_STATUSCODE_GOOD)
            return res;
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
            if(node->head.nodeClass == UA_NODECLASS_VARIABLE) {
                /* Take into account if the publishing interval is used for sampling */
                UA_Double samplingInterval = params->samplingInterval;
                if(samplingInterval < 0 && mon->subscription)
                    samplingInterval = mon->subscription->publishingInterval;
                /* Adjust if smaller than the allowed minimum for the variable */
                if(samplingInterval < vn->minimumSamplingInterval)
                    params->samplingInterval = vn->minimumSamplingInterval;
            }
            UA_NODESTORE_RELEASE(server, node);
        }
    }
        

    /* A negative number indicates that the sampling interval is the publishing
     * interval of the Subscription. Note that the sampling interval selected
     * here remains also when the Subscription's publish interval is adjusted
     * afterwards. */
    if(mon->subscription && params->samplingInterval < 0.0)
        params->samplingInterval = mon->subscription->publishingInterval;

    /* Adjust non-null sampling interval to lie within the configured limits */
    if(params->samplingInterval != 0.0) {
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

static const UA_String
binaryEncoding = {sizeof("Default Binary") - 1, (UA_Byte *)"Default Binary"};

/* Structure to pass additional arguments into the operation */
struct createMonContext {
    UA_Subscription *sub;
    UA_TimestampsToReturn timestampsToReturn;
    UA_LocalMonitoredItem *localMon; /* used if non-null */
};

static void
Operation_CreateMonitoredItem(UA_Server *server, UA_Session *session,
                              struct createMonContext *cmc,
                              const UA_MonitoredItemCreateRequest *request,
                              UA_MonitoredItemCreateResult *result) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Check available capacity */
    if(!cmc->localMon &&
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
    UA_DataValue v = readWithSession(server, session, &request->itemToMonitor,
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
            UA_LOG_INFO_SUBSCRIPTION(server->config.logging, cmc->sub,
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
    if(cmc->localMon) {
        newMon = &cmc->localMon->monitoredItem;
        cmc->localMon = NULL; /* clean up internally from now on */
    } else {
        newMon = (UA_MonitoredItem*)UA_malloc(sizeof(UA_MonitoredItem));
        if(!newMon) {
            result->statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
            return;
        }
    }

    /* Initialize the MonitoredItem */
    UA_MonitoredItem_init(newMon);
    newMon->subscription = cmc->sub;
    newMon->timestampsToReturn = cmc->timestampsToReturn;
    result->statusCode |= UA_ReadValueId_copy(&request->itemToMonitor,
                                              &newMon->itemToMonitor);
    result->statusCode |= UA_MonitoringParameters_copy(&request->requestedParameters,
                                                       &newMon->parameters);
    result->statusCode |= checkAdjustMonitoredItemParams(server, session, newMon,
                                                         valueType, &newMon->parameters,
                                                         &result->filterResult);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_SUBSCRIPTION(server->config.logging, cmc->sub,
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
    result->statusCode = UA_MonitoredItem_setMonitoringMode(server, newMon,
                                                            request->monitoringMode);
    if(result->statusCode != UA_STATUSCODE_GOOD) {
        UA_MonitoredItem_delete(server, newMon);
        return;
    }

    /* Prepare the response */
    result->revisedSamplingInterval = newMon->parameters.samplingInterval;
    result->revisedQueueSize = newMon->parameters.queueSize;
    result->monitoredItemId = newMon->monitoredItemId;

    UA_LOG_INFO_SUBSCRIPTION(server->config.logging, cmc->sub,
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
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
                         "Processing CreateMonitoredItemsRequest");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Check the upper bound for the number of items */
    if(server->config.maxMonitoredItemsPerCall != 0 &&
       request->itemsToCreateSize > server->config.maxMonitoredItemsPerCall) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTOOMANYOPERATIONS;
        return;
    }

    /* Check if the timestampstoreturn is valid */
    if(request->timestampsToReturn < UA_TIMESTAMPSTORETURN_SOURCE ||
       request->timestampsToReturn > UA_TIMESTAMPSTORETURN_NEITHER) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID;
        return;
    }

    /* Find the subscription */
    UA_Subscription *sub = UA_Session_getSubscriptionById(session, request->subscriptionId);
    if(!sub) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
        return;
    }

    /* Reset the lifetime counter of the Subscription */
    Subscription_resetLifetime(sub);

    /* Call the service */
    struct createMonContext cmc;
    cmc.timestampsToReturn = request->timestampsToReturn;
    cmc.sub = sub;
    cmc.localMon = NULL;

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
    UA_MonitoredItemCreateResult result;
    UA_MonitoredItemCreateResult_init(&result);

    /* Check that we don't use the DataChange callback for events */
    if(item.itemToMonitor.attributeId == UA_ATTRIBUTEID_EVENTNOTIFIER) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "DataChange-MonitoredItem cannot be created for the "
                     "EventNotifier attribute");
        result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }

    /* Pre-allocate the local MonitoredItem structure */
    UA_LocalMonitoredItem *localMon = (UA_LocalMonitoredItem*)
        UA_calloc(1, sizeof(UA_LocalMonitoredItem));
    if(!localMon) {
        result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return result;
    }
    localMon->context = monitoredItemContext;
    localMon->callback.dataChangeCallback = callback;

    /* Call the service */
    struct createMonContext cmc;
    cmc.sub = server->adminSubscription;
    cmc.localMon = localMon;
    cmc.timestampsToReturn = timestampsToReturn;

    UA_LOCK(&server->serviceMutex);
    Operation_CreateMonitoredItem(server, &server->adminSession, &cmc, &item, &result);
    UA_UNLOCK(&server->serviceMutex);

    /* If this failed, clean up the local MonitoredItem structure */
    if(result.statusCode != UA_STATUSCODE_GOOD && cmc.localMon)
        UA_free(localMon);

    return result;
}

UA_MonitoredItemCreateResult
UA_Server_createEventMonitoredItemEx(UA_Server *server,
                                     const UA_MonitoredItemCreateRequest item,
                                     void *monitoredItemContext,
                                     UA_Server_EventNotificationCallback callback) {
    UA_MonitoredItemCreateResult result;
    UA_MonitoredItemCreateResult_init(&result);

    /* Check that we don't use the DataChange callback for events */
    if(item.itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Event-MonitoredItem must monitor the EventNotifier attribute");
        result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }

    const UA_ExtensionObject *filter = &item.requestedParameters.filter;
    if((filter->encoding != UA_EXTENSIONOBJECT_DECODED &&
        filter->encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       filter->content.decoded.type != &UA_TYPES[UA_TYPES_EVENTFILTER]) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Filter is not of EventFilter data type");
        result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }

    UA_EventFilter *ef = (UA_EventFilter*)filter->content.decoded.data;
    if(ef->selectClausesSize == 0) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Event filter must define at least one select clause");
        result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }

    /* Pre-allocate the local MonitoredItem structure */
    UA_LocalMonitoredItem *localMon = (UA_LocalMonitoredItem*)
        UA_calloc(1, sizeof(UA_LocalMonitoredItem));
    if(!localMon) {
        result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return result;
    }
    localMon->context = monitoredItemContext;
    localMon->callback.eventCallback = callback;

    /* Create the string names for the event fields */
    localMon->eventFields.map = (UA_KeyValuePair*)
        UA_calloc(ef->selectClausesSize, sizeof(UA_KeyValuePair));
    if(!localMon->eventFields.map) {
        UA_free(localMon);
        result.statusCode = UA_STATUSCODE_BADOUTOFMEMORY;
        return result;
    }
    localMon->eventFields.mapSize = ef->selectClausesSize;

#ifdef UA_ENABLE_PARSING
    for(size_t i = 0; i < ef->selectClausesSize; i++) {
        result.statusCode |=
            UA_SimpleAttributeOperand_print(&ef->selectClauses[i],
                                            &localMon->eventFields.map[i].key.name);
    }
    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_clear(&localMon->eventFields);
        UA_free(localMon);
        return result;
    }
#endif

    /* Call the service */
    struct createMonContext cmc;
    cmc.sub = server->adminSubscription;
    cmc.localMon = localMon;
    cmc.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;

    UA_LOCK(&server->serviceMutex);
    Operation_CreateMonitoredItem(server, &server->adminSession, &cmc, &item, &result);
    UA_UNLOCK(&server->serviceMutex);

    /* If the service failed, clean up the local MonitoredItem structure */
    if(result.statusCode != UA_STATUSCODE_GOOD && cmc.localMon) {
        UA_KeyValueMap_clear(&localMon->eventFields);
        UA_free(localMon);
    }
    return result;
}

UA_MonitoredItemCreateResult
UA_Server_createEventMonitoredItem(UA_Server *server, const UA_NodeId nodeId,
                                   const UA_EventFilter filter, void *monitoredItemContext,
                                   UA_Server_EventNotificationCallback callback) {
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = nodeId;
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_ExtensionObject_setValue(&item.requestedParameters.filter,
                                (void*)(uintptr_t)&filter,
                                &UA_TYPES[UA_TYPES_EVENTFILTER]);
    return UA_Server_createEventMonitoredItemEx(server, item, monitoredItemContext, callback);
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
    UA_DataValue v = readWithSession(server, session, &mon->itemToMonitor,
                                     mon->timestampsToReturn);

    /* Verify and adjust the new parameters. This still leaves the original
     * MonitoredItem untouched. */
    result->statusCode =
        checkAdjustMonitoredItemParams(server, session, mon, v.value.type,
                                       &params, &result->filterResult);
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

    /* If the sampling interval is negative (the sampling callback is called
     * from within the publishing callback), return the publishing interval of
     * the Subscription. Note that we only use the cyclic callback of the
     * Subscription. So if the Subscription publishing interval is modified,
     * this also impacts this MonitoredItem. */
    if(result->revisedSamplingInterval < 0.0 && mon->subscription)
        result->revisedSamplingInterval = mon->subscription->publishingInterval;

    /* Remove some notifications if the queue is now too small */
    UA_MonitoredItem_ensureQueueSpace(server, mon);

    /* Remove the overflow bits if the queue has now a size of 1 */
    UA_MonitoredItem_removeOverflowInfoBits(mon);

    /* If the sampling interval is negative (the sampling callback is called
     * from within the publishing callback), return the publishing interval of
     * the Subscription. Note that we only use the cyclic callback of the
     * Subscription. So if the Subscription publishing interval is modified,
     * this also impacts this MonitoredItem. */
    if(result->revisedSamplingInterval < 0.0 && mon->subscription)
        result->revisedSamplingInterval = mon->subscription->publishingInterval;

    UA_LOG_INFO_SUBSCRIPTION(server->config.logging, sub,
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
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
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

    /* Reset the lifetime counter of the Subscription */
    Subscription_resetLifetime(sub);

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
    UA_LOG_DEBUG_SESSION(server->config.logging, session, "Processing SetMonitoringMode");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Check the max number if items */
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

    /* Reset the lifetime counter of the Subscription */
    Subscription_resetLifetime(sub);

    /* Call the service */
    struct setMonitoringContext smc;
    smc.sub = sub;
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
    UA_LOG_DEBUG_SESSION(server->config.logging, session,
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

    /* Reset the lifetime counter of the Subscription */
    Subscription_resetLifetime(sub);

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

    UA_Subscription *sub = server->adminSubscription;
    UA_MonitoredItem *mon;
    LIST_FOREACH(mon, &sub->monitoredItems, listEntry) {
        if(mon->monitoredItemId == monitoredItemId)
            break;
    }

    UA_StatusCode res = UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
    if(mon) {
        UA_MonitoredItem_delete(server, mon);
        res = UA_STATUSCODE_GOOD;
    }

    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
