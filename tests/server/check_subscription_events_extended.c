/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "server/ua_subscription.h"

#include <check.h>

#include "testing_clock.h"
#include "thread_wrapper.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

static UA_Server *server;
static size_t serverIterations;
static UA_Boolean running;
static THREAD_HANDLE server_thread;
static MUTEX_HANDLE serverMutex;

UA_Client *client;

static UA_UInt32 subscriptionId;
static UA_UInt32 monitoredItemId;
static UA_NodeId eventType;
static size_t nSelectClauses = 4;
static UA_Boolean notificationReceived;
static UA_SimpleAttributeOperand *selectClauses;
static UA_SimpleAttributeOperand *selectClausesTest;
static UA_StatusCode *retvals;
static UA_ContentFilterResult *contentFilterResult;
static UA_ContentFilter *conFilter;
static UA_NodeId *baseEventTypeIdSecondElement;
static UA_NodeId *baseEventTypeIdThirdElement;
static UA_ContentFilter *whereClauses;
static UA_ContentFilterElement *elements;
static const size_t nWhereClauses = 1;
UA_Double publishingInterval = 500.0;

static void
addNewEventType(void) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "SimpleEventType");
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "The simple event type we created");

    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_PROGRESSEVENTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(0, "SimpleEventType"),
                                attr, NULL, &eventType);
    UA_LocalizedText_clear(&attr.displayName);
    UA_LocalizedText_clear(&attr.description);
}

static void
setupSelectClauses(void) {
    /* Check for severity (set manually), message (set manually), eventType
     * (automatic) and sourceNode (automatic) */
    selectClauses = (UA_SimpleAttributeOperand *)
        UA_Array_new(nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses)
        return;

    for(size_t i = 0; i < nSelectClauses; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
        selectClauses[i].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        selectClauses[i].browsePathSize = 1;
        selectClauses[i].attributeId = UA_ATTRIBUTEID_VALUE;
        selectClauses[i].browsePath = (UA_QualifiedName *)
            UA_Array_new(selectClauses[i].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        if(!selectClauses[i].browsePathSize) {
            UA_Array_delete(selectClauses, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        }
    }

    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");
    selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");
    selectClauses[2].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "EventType");
    selectClauses[3].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "SourceNode");
}


static void setupWhereClauses(void){
    whereClauses = (UA_ContentFilter*)
        UA_Array_new(nWhereClauses,&UA_TYPES[UA_TYPES_CONTENTFILTER]);
    whereClauses->elementsSize= 1;
    whereClauses->elements  = (UA_ContentFilterElement*)
        UA_Array_new(whereClauses->elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
    for(size_t i =0; i<whereClauses->elementsSize; ++i) {
        UA_ContentFilterElement_init(&whereClauses->elements[i]);
    }
    whereClauses->elements[0].filterOperator = UA_FILTEROPERATOR_OFTYPE;
    whereClauses->elements[0].filterOperandsSize = 1;
    whereClauses->elements[0].filterOperands = (UA_ExtensionObject*)
        UA_Array_new(whereClauses->elements[0].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if (!whereClauses->elements[0].filterOperands){
        UA_ContentFilter_clear(whereClauses);
    }
    whereClauses->elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND];
    whereClauses->elements[0].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_AttributeOperand *pOperand;
    pOperand = UA_AttributeOperand_new();
    UA_AttributeOperand_init(pOperand);
    UA_NodeId *baseEventTypeId;
    baseEventTypeId = UA_NodeId_new();
    UA_NodeId_init(baseEventTypeId);
    *baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);  // filtern nach BaseEventType
    pOperand->nodeId = *baseEventTypeId;
    pOperand->attributeId = UA_ATTRIBUTEID_VALUE;
    whereClauses->elements[0].filterOperands[0].content.decoded.data = pOperand;
    UA_NodeId_delete(baseEventTypeId);
}

static void
handler_events_simple(UA_Client *lclient, UA_UInt32 subId, void *subContext,
                      UA_UInt32 monId, void *monContext,
                      size_t nEventFields, UA_Variant *eventFields) {
    UA_Boolean foundSeverity = UA_FALSE;
    UA_Boolean foundMessage = UA_FALSE;
    UA_Boolean foundType = UA_FALSE;
    UA_Boolean foundSource = UA_FALSE;
    ck_assert_uint_eq(*(UA_UInt32 *) monContext, monitoredItemId);
    ck_assert_uint_eq(nEventFields, nSelectClauses);
    // check all event fields
    for(size_t i = 0; i < nEventFields; i++) {
        // find out which attribute of the event is being looked at
        if(UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_UINT16])) {
            // Severity
            ck_assert_uint_eq(*((UA_UInt16 *) (eventFields[i].data)), 1000);
            foundSeverity = UA_TRUE;
        } else if(UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            // Message
            UA_LocalizedText comp = UA_LOCALIZEDTEXT("en-US", "Generated Event");
            ck_assert(UA_String_equal(&((UA_LocalizedText *) eventFields[i].data)->locale, &comp.locale));
            ck_assert(UA_String_equal(&((UA_LocalizedText *) eventFields[i].data)->text, &comp.text));
            foundMessage = UA_TRUE;
        } else if(UA_Variant_hasScalarType(&eventFields[i], &UA_TYPES[UA_TYPES_NODEID])) {
            // either SourceNode or EventType
            UA_NodeId serverId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
            if(UA_NodeId_equal((UA_NodeId *) eventFields[i].data, &eventType)) {
                // EventType
                foundType = UA_TRUE;
            } else if(UA_NodeId_equal((UA_NodeId *) eventFields[i].data, &serverId)) {
                // SourceNode
                foundSource = UA_TRUE;
            } else {
                ck_assert_msg(UA_FALSE, "NodeId doesn't match");
            }
        } else {
            ck_assert_msg(UA_FALSE, "Field doesn't match");
        }
    }
    ck_assert_uint_eq(foundMessage, UA_TRUE);
    ck_assert_uint_eq(foundSeverity, UA_TRUE);
    ck_assert_uint_eq(foundType, UA_TRUE);
    ck_assert_uint_eq(foundSource, UA_TRUE);
    notificationReceived = true;
}

// create a subscription and add a monitored item to it
static void
setupSubscription(void) {
    // Create subscription
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    subscriptionId = response.subscriptionId;
}

static void
removeSubscription(void) {
    UA_DeleteSubscriptionsRequest deleteSubscriptionsRequest;
    UA_DeleteSubscriptionsRequest_init(&deleteSubscriptionsRequest);
    UA_UInt32 removeId = subscriptionId;
    deleteSubscriptionsRequest.subscriptionIdsSize = 1;
    deleteSubscriptionsRequest.subscriptionIds = &removeId;

    UA_DeleteSubscriptionsResponse deleteSubscriptionsResponse;
    UA_DeleteSubscriptionsResponse_init(&deleteSubscriptionsResponse);
    UA_LOCK(server->serviceMutex);
    Service_DeleteSubscriptions(server, &server->adminSession, &deleteSubscriptionsRequest,
                                &deleteSubscriptionsResponse);
    UA_UNLOCK(server->serviceMutex);
    UA_DeleteSubscriptionsResponse_clear(&deleteSubscriptionsResponse);
}

static void serverMutexLock(void) {
    if (!(MUTEX_LOCK(serverMutex))) {
        fprintf(stderr, "Mutex cannot be locked.\n");
        exit(1);
    }
}

static void serverMutexUnlock(void) {
    if (!(MUTEX_UNLOCK(serverMutex))) {
        fprintf(stderr, "Mutex cannot be unlocked.\n");
        exit(1);
    }
}

THREAD_CALLBACK(serverloop) {
    while (running) {
        serverMutexLock();
        UA_Server_run_iterate(server, false);
        serverIterations++;
        serverMutexUnlock();
    }
    return 0;
}

static void
sleepUntilAnswer(UA_Double sleepMs) {
    UA_fakeSleep((UA_UInt32)sleepMs);
    serverMutexLock();
    size_t oldIterations = serverIterations;
    size_t newIterations;
    serverMutexUnlock();
    while(true) {
        serverMutexLock();
        newIterations = serverIterations;
        serverMutexUnlock();
        if(oldIterations != newIterations)
            return;
        UA_realSleep(1);
    }
}

static void
setup(void) {
    if (!MUTEX_INIT(serverMutex)) {
        fprintf(stderr, "Server mutex was not created correctly.");
        exit(1);
    }
    running = true;

    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->maxPublishReqPerSession = 5;
    UA_Server_run_startup(server);

    addNewEventType();
    setupSelectClauses();
    setupWhereClauses();
    THREAD_CREATE(server_thread, serverloop);

    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Client can not connect to opc.tcp://localhost:4840. %s",
                UA_StatusCode_name(retval));
        exit(1);
    }
    setupSubscription();

    sleepUntilAnswer(publishingInterval + 100);
}

static void
teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    removeSubscription();
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_Array_delete(selectClauses, nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    if (!MUTEX_DESTROY(serverMutex)) {
        fprintf(stderr, "Server mutex was not destroyed correctly.");
        exit(1);
    }
}

static UA_StatusCode
triggerEventLocked(const UA_NodeId eventNodeId, const UA_NodeId origin,
                   UA_ByteString *outEventId, const UA_Boolean deleteEventNode) {
    serverMutexLock();
    UA_StatusCode retval = UA_Server_triggerEvent(server, eventNodeId, origin,
                                                  outEventId, deleteEventNode);
    serverMutexUnlock();
    return retval;
}

static UA_StatusCode
eventSetup(UA_NodeId *eventNodeId) {
    UA_StatusCode retval;
    serverMutexLock();
    retval = UA_Server_createEvent(server, eventType, eventNodeId);
    serverMutexUnlock();
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    // add a severity to the event
    UA_Variant value;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = *eventNodeId;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    rpe.targetName = UA_QUALIFIEDNAME(0, "Severity");
    serverMutexLock();
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    serverMutexUnlock();
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    // number with no special meaning
    UA_UInt16 eventSeverity = 1000;
    UA_Variant_setScalar(&value, &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    serverMutexLock();
    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    serverMutexUnlock();
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_BrowsePathResult_clear(&bpr);

    //add a message to the event
    rpe.targetName = UA_QUALIFIEDNAME(0, "Message");
    serverMutexLock();
    bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    serverMutexUnlock();
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    serverMutexLock();
    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    serverMutexUnlock();
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_BrowsePathResult_clear(&bpr);

    return retval;
}

static UA_MonitoredItemCreateResult
addMonitoredItem(UA_Client_EventNotificationCallback handler, bool setFilter, bool discardOldest) {
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, 2253); // Root->Objects->Server
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = selectClauses;
    filter.selectClausesSize = nSelectClauses;
    filter.whereClause = *whereClauses;

    if (setFilter) {
        item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
        item.requestedParameters.filter.content.decoded.data = &filter;
        item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
    }

    item.requestedParameters.queueSize = 1;
    item.requestedParameters.discardOldest = discardOldest;

    return UA_Client_MonitoredItems_createEvent(client, subscriptionId,
                                                UA_TIMESTAMPSTORETURN_BOTH, item,
                                                &monitoredItemId, handler, NULL);
}


/* Create event with empty filter */

START_TEST(generateEventEmptyFilter) {
        UA_NodeId eventNodeId;
        UA_StatusCode retval = eventSetup(&eventNodeId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        // add a monitored item
        UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, false, true);
        ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    } END_TEST


/* Ensure events are received with proper values */
START_TEST(generateEvents) {
        UA_NodeId eventNodeId;
        UA_StatusCode retval = eventSetup(&eventNodeId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        // add a monitored item
        UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, true, true);
        ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
        monitoredItemId = createResult.monitoredItemId;
        // trigger the event
        retval = triggerEventLocked(eventNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), NULL, UA_TRUE);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        // let the client fetch the event and check if the correct values were received
        notificationReceived = false;
        sleepUntilAnswer(publishingInterval + 100);
        retval = UA_Client_run_iterate(client, 0);
        sleepUntilAnswer(publishingInterval + 100);
        retval = UA_Client_run_iterate(client, 0);


        // delete the monitoredItem
        UA_DeleteMonitoredItemsRequest deleteRequest;
        UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
        deleteRequest.subscriptionId = subscriptionId;
        deleteRequest.monitoredItemIds = &monitoredItemId;
        deleteRequest.monitoredItemIdsSize = 1;

        UA_DeleteMonitoredItemsResponse deleteResponse =
            UA_Client_MonitoredItems_delete(client, deleteRequest);

        sleepUntilAnswer(publishingInterval + 100);
        ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(deleteResponse.resultsSize, 1);
        ck_assert_uint_eq(*(deleteResponse.results), UA_STATUSCODE_GOOD);

        UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
    } END_TEST

static bool hasBaseModelChangeEventType(void) {

    UA_QualifiedName readBrowsename;
    UA_QualifiedName_init(&readBrowsename);
    UA_StatusCode retval = UA_Server_readBrowseName(server, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEMODELCHANGEEVENTTYPE), &readBrowsename);
    UA_QualifiedName_clear(&readBrowsename);
    return !(retval == UA_STATUSCODE_BADNODEIDUNKNOWN);
}

START_TEST(createAbstractEvent) {
        if (!hasBaseModelChangeEventType())
            return;

        UA_NodeId eventNodeId = UA_NODEID_NULL;
        UA_NodeId abstractEventType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEMODELCHANGEEVENTTYPE);
        serverMutexLock();
        UA_StatusCode retval = UA_Server_createEvent(server, abstractEventType, &eventNodeId);
        serverMutexUnlock();
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(createAbstractEventWithParent) {
        if (!hasBaseModelChangeEventType())
            return;
        UA_NodeId eventNodeId = UA_NODEID_NULL;
        UA_NodeId abstractEventType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEMODELCHANGEEVENTTYPE);
        serverMutexLock();
        // createEvent does not use a parent, so we are instead using addObjectNode
        UA_StatusCode retval = UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                       UA_QUALIFIEDNAME(0, "Abstract Event"), abstractEventType,
                                                       UA_ObjectAttributes_default, NULL, &eventNodeId);
        serverMutexUnlock();
        ck_assert_uint_eq(retval, UA_STATUSCODE_BADTYPEDEFINITIONINVALID);
    } END_TEST

START_TEST(createNonAbstractEventWithParent) {
        UA_NodeId eventNodeId = UA_NODEID_NULL;
        serverMutexLock();
        // Our SimpleEventType is not abstract
        UA_StatusCode retval = UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                       UA_QUALIFIEDNAME(0, "Non-Abstract Event"), eventType,
                                                       UA_ObjectAttributes_default, NULL, &eventNodeId);
        serverMutexUnlock();
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } END_TEST



START_TEST(multipleMonitoredItemsOneNode) {
        UA_UInt32 monitoredItemIdAr[3];

        // set up monitored items
        UA_MonitoredItemCreateRequest item;
        UA_MonitoredItemCreateRequest_init(&item);
        item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER); // Root->Objects->Server
        item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
        item.monitoringMode = UA_MONITORINGMODE_REPORTING;

        UA_EventFilter filter;
        UA_EventFilter_init(&filter);
        filter.selectClauses = selectClauses;
        filter.selectClausesSize = nSelectClauses;

        item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
        item.requestedParameters.filter.content.decoded.data = &filter;
        item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
        item.requestedParameters.queueSize = 1;
        item.requestedParameters.discardOldest = true;

        for(size_t i = 0; i < 3; i++) {
            UA_MonitoredItemCreateResult result =
                UA_Client_MonitoredItems_createEvent(client, subscriptionId, UA_TIMESTAMPSTORETURN_BOTH,
                                                     item, NULL, handler_events_simple, NULL);
            ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
            monitoredItemIdAr[i] = result.monitoredItemId;
        }

        // delete the three monitored items after another
        UA_DeleteMonitoredItemsRequest deleteRequest;
        UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
        deleteRequest.subscriptionId = subscriptionId;
        deleteRequest.monitoredItemIdsSize = 1;
        UA_DeleteMonitoredItemsResponse deleteResponse;

        for(size_t i = 0; i < 3; i++) {
            deleteRequest.monitoredItemIds = &monitoredItemIdAr[i];
            deleteResponse = UA_Client_MonitoredItems_delete(client, deleteRequest);

            ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(deleteResponse.resultsSize, 1);
            ck_assert_uint_eq(*(deleteResponse.results), UA_STATUSCODE_GOOD);
            UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
        }
    } END_TEST



START_TEST(evaluateWhereClause) {
        /* Everything is on the stack, so no memory cleaning required.*/
        UA_NodeId eventNodeId;
        UA_StatusCode retval = eventSetup(&eventNodeId);
        UA_ContentFilter contentFilter;
        UA_ContentFilter_init(&contentFilter);
        /* Empty Filter */
        UA_LOCK(server->serviceMutex);
        retval = UA_Server_evaluateWhereClauseContentFilter(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_ContentFilterElement contentFilterElement;
        UA_ContentFilterElement_init(&contentFilterElement);
        contentFilter.elements = &contentFilterElement;
        contentFilter.elementsSize = 1;

        /* Illegal filter operators */
        contentFilterElement.filterOperator = UA_FILTEROPERATOR_RELATEDTO;
        UA_LOCK(server->serviceMutex);
        retval = UA_Server_evaluateWhereClauseContentFilter(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(retval, UA_STATUSCODE_BADEVENTFILTERINVALID);
        contentFilterElement.filterOperator = UA_FILTEROPERATOR_INVIEW;
        UA_LOCK(server->serviceMutex);
        retval = UA_Server_evaluateWhereClauseContentFilter(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(retval, UA_STATUSCODE_BADEVENTFILTERINVALID);

        /* No operand provided */
        contentFilterElement.filterOperator = UA_FILTEROPERATOR_OFTYPE;
        UA_LOCK(server->serviceMutex);
        retval = UA_Server_evaluateWhereClauseContentFilter(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(retval, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);

        UA_ExtensionObject filterOperandExObj;
        UA_ExtensionObject_init(&filterOperandExObj);
        contentFilterElement.filterOperandsSize = 1;
        contentFilterElement.filterOperands = &filterOperandExObj;
        filterOperandExObj.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        UA_LiteralOperand literalOperand;
        UA_LiteralOperand_init(&literalOperand);
        filterOperandExObj.content.decoded.data = &literalOperand;

        /* Same type*/
        UA_Variant_setScalar(&literalOperand.value, &eventType, &UA_TYPES[UA_TYPES_NODEID]);
        UA_LOCK(server->serviceMutex);
        retval = UA_Server_evaluateWhereClauseContentFilter(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        /* Base type*/
        UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        UA_Variant_setScalar(&literalOperand.value, &nodeId, &UA_TYPES[UA_TYPES_NODEID]);
        UA_LOCK(server->serviceMutex);
        retval = UA_Server_evaluateWhereClauseContentFilter(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        /* Other type*/
        nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEMODELCHANGEEVENTTYPE);
        UA_LOCK(server->serviceMutex);
        retval = UA_Server_evaluateWhereClauseContentFilter(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
    }
END_TEST

static UA_ContentFilter *setupWhereClausesComplex(void){
    conFilter = (UA_ContentFilter*)
        UA_Array_new(nWhereClauses,&UA_TYPES[UA_TYPES_CONTENTFILTER]);
    for(size_t i =0; i<nWhereClauses; ++i) {
        UA_ContentFilter_init(&conFilter[i]);
    }
    conFilter[0].elementsSize = 3;
    conFilter[0].elements  = (UA_ContentFilterElement*)
        UA_Array_new(
            conFilter->elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
    for(size_t i =0; i< conFilter[0].elementsSize; ++i) {
        UA_ContentFilterElement_init(&conFilter[0].elements[i]);
    }
    conFilter[0].elements[0].filterOperator = UA_FILTEROPERATOR_OR; // set the first Operator
    conFilter[0].elements[1].filterOperator = UA_FILTEROPERATOR_OFTYPE; // set the second Operator
    conFilter[0].elements[2].filterOperator = UA_FILTEROPERATOR_OFTYPE; // set the third Operator
    conFilter[0].elements[0].filterOperandsSize = 2;            // set Operands size of first Operator
    conFilter[0].elements[1].filterOperandsSize = 1;            // set Operands size of second Operator
    conFilter[0].elements[2].filterOperandsSize = 1;            // set Operands size of third Operator
    for(size_t i =0; i< conFilter[0].elementsSize; ++i) {  // Set Operands Arrays
        conFilter[0].elements[i].filterOperands = (UA_ExtensionObject*)
            UA_Array_new(
                conFilter[0].elements[i].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
        if (!conFilter[0].elements[i].filterOperands){
            UA_ContentFilter_clear(conFilter);
            return NULL;
        }
        for(size_t n =0; n< conFilter[0].elements[i].filterOperandsSize; ++n) {
            UA_ExtensionObject_init(&conFilter[0].elements[i].filterOperands[n]);
        }
    }

    // Second Element
    conFilter[0].elements[1].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND];
    conFilter[0].elements[1].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_AttributeOperand *pOperandSecondElement;
    pOperandSecondElement = UA_AttributeOperand_new();
    UA_AttributeOperand_init(pOperandSecondElement);
    baseEventTypeIdSecondElement = UA_NodeId_new();
    UA_NodeId_init(baseEventTypeIdSecondElement);
    *baseEventTypeIdSecondElement = UA_NODEID_NUMERIC(0, UA_NS0ID_SYSTEMEVENTTYPE);      // filtern nach SYSTEMEVENTTYPE
    pOperandSecondElement->nodeId = *baseEventTypeIdSecondElement;
    pOperandSecondElement->attributeId = UA_ATTRIBUTEID_VALUE;
    conFilter[0].elements[1].filterOperands[0].content.decoded.data = pOperandSecondElement;

    // Third Element
    conFilter[0].elements[2].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND];
    conFilter[0].elements[2].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    UA_AttributeOperand *pOperandThirdElement;
    pOperandThirdElement = UA_AttributeOperand_new();
    UA_AttributeOperand_init(pOperandThirdElement);
    baseEventTypeIdThirdElement = UA_NodeId_new();
    UA_NodeId_init(baseEventTypeIdThirdElement);
    *baseEventTypeIdThirdElement = UA_NODEID_NUMERIC(0, UA_NS0ID_PROGRESSEVENTTYPE);  // filtern nach ProgressEventType
    // *baseEventTypeIdThirdElement = UA_NODEID_NUMERIC(0, UA_NS0ID_AUDITEVENTTYPE);  // filtern nach AuditCertificateEventType
    pOperandThirdElement->nodeId = *baseEventTypeIdThirdElement;
    pOperandThirdElement->attributeId = UA_ATTRIBUTEID_VALUE;
    conFilter[0].elements[2].filterOperands[0].content.decoded.data = pOperandThirdElement;

    //First Element
    UA_ElementOperand *elementOperand;
    elementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(elementOperand);
    elementOperand->index = 1;
    UA_ElementOperand *secondElementOperand;
    secondElementOperand = UA_ElementOperand_new();
    UA_ElementOperand_init(secondElementOperand);
    secondElementOperand->index = 2;
    conFilter[0].elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter[0].elements[0].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
    conFilter[0].elements[0].filterOperands[0].content.decoded.data = elementOperand;
    conFilter[0].elements[0].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    conFilter[0].elements[0].filterOperands[1].encoding = UA_EXTENSIONOBJECT_DECODED;
    conFilter[0].elements[0].filterOperands[1].content.decoded.data = secondElementOperand;
    return conFilter;
}



START_TEST(WhereClauseValidation){
        UA_ContentFilter *contentFilter = setupWhereClausesComplex();  // ContentFilter Setup  (SYSTEMEVENTTYPE) OR (ProgressEventType)
        UA_NodeId eventNodeId;
        UA_StatusCode retval = eventSetup(&eventNodeId);    // Event Setup (ProgressEventType)
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        /* init contentFilterResult */
        UA_ContentFilterResult *contentFilterRes = (UA_ContentFilterResult*)
            UA_Array_new(nWhereClauses,&UA_TYPES[UA_TYPES_CONTENTFILTERRESULT]);
        contentFilterRes[0].elementResultsSize = contentFilter->elementsSize;
        contentFilterRes[0].elementResults  = (UA_ContentFilterElementResult *)
            UA_Array_new(contentFilterRes[0].elementResultsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENTRESULT] );
        for(size_t i =0; i<contentFilterRes[0].elementResultsSize; ++i) {
            UA_ContentFilterElementResult_init(&contentFilterRes[0].elementResults[i]);
        }
        UA_LOCK(server->serviceMutex);
        UA_Server_WhereClauseValidation(server,&eventNodeId,contentFilter ,contentFilterRes,0);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterRes->elementResults[0].statusCode, UA_STATUSCODE_GOOD);
        UA_ContentFilterResult_delete(contentFilterRes);
    }
END_TEST



START_TEST(initialWhereClauseValidation) {
        /* Everything is on the stack, so no memory cleaning required.*/
        UA_NodeId eventNodeId;
        UA_ContentFilter contentFilter;
        contentFilter.elementsSize = 0;
        elements  = (UA_ContentFilterElement*)
            UA_Array_new(contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
        contentFilter.elements = elements;
        /* Empty Filter */
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults->statusCode, UA_STATUSCODE_GOOD);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* clear */
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_clear(&contentFilter.elements[i]);
        }
        contentFilter.elementsSize = 1;
        elements  = (UA_ContentFilterElement*)
            UA_Array_new(contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
        contentFilter.elements = elements;
        /* Illegal filter operators */
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_RELATEDTO;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_INVIEW;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);

        /*************************** UA_FILTEROPERATOR_OR ***************************/
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_OR;
        /* No operand provided */
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands size */
        contentFilter.elements[0].filterOperandsSize = 1;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands type */
        contentFilter.elements[0].filterOperandsSize = 2;
        contentFilter.elements[0].filterOperands = (UA_ExtensionObject*)
            UA_Array_new(contentFilter.elements[0].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        contentFilter.elements[0].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        // Illegal filter operands INDEXRANGE
        /* clear */
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_clear(&contentFilter.elements[i]);
        }
        UA_Array_delete(elements,contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
        /* init */
        contentFilter.elementsSize = 3;
        elements = (UA_ContentFilterElement*)
            UA_Array_new(contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
        contentFilter.elements  = elements;
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_init(&contentFilter.elements[i]);
        }
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_OR; // set the first Operator
        contentFilter.elements[1].filterOperator = UA_FILTEROPERATOR_OFTYPE; // set the second Operator
        contentFilter.elements[2].filterOperator = UA_FILTEROPERATOR_OFTYPE; // set the third Operator
        contentFilter.elements[0].filterOperandsSize = 2;            // set Operands size of first Operator
        contentFilter.elements[1].filterOperandsSize = 1;            // set Operands size of second Operator
        contentFilter.elements[2].filterOperandsSize = 1;            // set Operands size of third Operator
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {  // Set Operands Arrays
            contentFilter.elements[i].filterOperands = (UA_ExtensionObject*)
                UA_Array_new(contentFilter.elements[i].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
            for(size_t n =0; n<contentFilter.elements[i].filterOperandsSize; ++n) {
                UA_ExtensionObject_init(&contentFilter.elements[i].filterOperands[n]);
            }
        }
        // Second Element
        contentFilter.elements[1].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND];
        contentFilter.elements[1].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
        UA_AttributeOperand *pOperandSecondElement;
        pOperandSecondElement = UA_AttributeOperand_new();
        UA_AttributeOperand_init(pOperandSecondElement);
        baseEventTypeIdSecondElement = UA_NodeId_new();
        UA_NodeId_init(baseEventTypeIdSecondElement);
        *baseEventTypeIdSecondElement = UA_NODEID_NUMERIC(0, UA_NS0ID_AUDITEVENTTYPE);      // filtern nach AuditEventType
        pOperandSecondElement->nodeId = *baseEventTypeIdSecondElement;
        pOperandSecondElement->attributeId = UA_ATTRIBUTEID_VALUE;
        contentFilter.elements[1].filterOperands[0].content.decoded.data = pOperandSecondElement;
        // Third Element
        contentFilter.elements[2].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND];
        contentFilter.elements[2].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
        UA_AttributeOperand *pOperandThirdElement;
        pOperandThirdElement = UA_AttributeOperand_new();
        UA_AttributeOperand_init(pOperandThirdElement);
        baseEventTypeIdThirdElement = UA_NodeId_new();
        UA_NodeId_init(baseEventTypeIdThirdElement);
        *baseEventTypeIdThirdElement = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);  // filtern nach BaseEventType
        pOperandThirdElement->nodeId = *baseEventTypeIdThirdElement;
        pOperandThirdElement->attributeId = UA_ATTRIBUTEID_VALUE;
        contentFilter.elements[2].filterOperands[0].content.decoded.data = pOperandThirdElement;
        //First Element
        UA_ElementOperand *elementOperand;
        elementOperand = UA_ElementOperand_new();
        elementOperand->index = 1;
        UA_ElementOperand *secondElementOperand;
        secondElementOperand = UA_ElementOperand_new();
        secondElementOperand->index = 3;      // invalid index
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
        contentFilter.elements[0].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
        contentFilter.elements[0].filterOperands[0].content.decoded.data = elementOperand;
        contentFilter.elements[0].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
        contentFilter.elements[0].filterOperands[1].encoding = UA_EXTENSIONOBJECT_DECODED;
        contentFilter.elements[0].filterOperands[1].content.decoded.data = secondElementOperand;
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
        contentFilter.elements[0].filterOperands[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADINDEXRANGEINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        /*clear */
        UA_Array_delete(elements,contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
        /*************************** UA_FILTEROPERATOR_OFTYPE **************************/
        /* init */
        contentFilter.elementsSize = 1;
        elements  = (UA_ContentFilterElement*)
            UA_Array_new(contentFilter.elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT] );
        contentFilter.elements = elements;
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_init(&contentFilter.elements[i]);
        }
        contentFilter.elements[0].filterOperandsSize = 0;
        // No operand provided
        contentFilter.elements[0].filterOperator = UA_FILTEROPERATOR_OFTYPE;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands size */
        contentFilter.elements[0].filterOperandsSize = 2;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands type */
        contentFilter.elements[0].filterOperandsSize = 1;
        contentFilter.elements[0].filterOperands = (UA_ExtensionObject*)
            UA_Array_new(contentFilter.elements[0].filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands attributeId */
        contentFilter.elements[0].filterOperands[0].content.decoded.type = &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND];
        contentFilter.elements[0].filterOperands[0].encoding = UA_EXTENSIONOBJECT_DECODED;
        UA_AttributeOperand *pOperand;
        pOperand = UA_AttributeOperand_new();
        UA_AttributeOperand_init(pOperand);
        pOperand->attributeId = UA_NODEIDTYPE_NUMERIC;
        UA_NodeId *baseEventTypeId;
        baseEventTypeId = UA_NodeId_new();
        UA_NodeId_init(baseEventTypeId);
        *baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);  // filtern nach BaseEventType
        pOperand->nodeId = *baseEventTypeId;
        contentFilter.elements[0].filterOperands[0].content.decoded.data = pOperand;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADATTRIBUTEIDINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Illegal filter operands EventTypeId */
        pOperand->attributeId = UA_ATTRIBUTEID_VALUE;
        *baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NODEIDTYPE_NUMERIC);
        pOperand->nodeId = *baseEventTypeId;
        contentFilter.elements[0].filterOperands[0].content.decoded.data = pOperand;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_BADNODEIDINVALID);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* Filter operands EventTypeId is a subtype of BaseEventType */
        *baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        pOperand->nodeId = *baseEventTypeId;
        contentFilter.elements[0].filterOperands[0].content.decoded.data = pOperand;
        UA_LOCK(server->serviceMutex);
        contentFilterResult = UA_Server_initialWhereClauseValidation(server, &eventNodeId, &contentFilter);
        UA_UNLOCK(server->serviceMutex);
        ck_assert_uint_eq(contentFilterResult->elementResults[0].statusCode, UA_STATUSCODE_GOOD);
        UA_ContentFilterResult_delete(contentFilterResult);
        /* clear */
        for(size_t i =0; i<contentFilter.elementsSize; ++i) {
            UA_ContentFilterElement_clear(&contentFilter.elements[i]);
        }
        UA_NodeId_delete(baseEventTypeIdSecondElement);
        UA_NodeId_delete(baseEventTypeIdThirdElement);
        UA_NodeId_delete(baseEventTypeId);
    }
END_TEST


START_TEST(validateSelectClause) {
/* Everything is on the stack, so no memory cleaning required.*/
        UA_StatusCode *retval;
        UA_EventFilter eventFilter;
        UA_EventFilter_init(&eventFilter);
        retval = UA_Server_initialSelectClauseValidation(server, &eventFilter);
        ck_assert_uint_eq(*retval, UA_STATUSCODE_BADSTRUCTUREMISSING);
        UA_StatusCode_delete(retval);
        selectClausesTest = (UA_SimpleAttributeOperand *)
            UA_Array_new(7, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
        eventFilter.selectClausesSize = 0;
        eventFilter.selectClauses = selectClausesTest;
        retval = UA_Server_initialSelectClauseValidation(server, &eventFilter);
        ck_assert_uint_eq(*retval, UA_STATUSCODE_GOOD);
        UA_StatusCode_delete(retval);
        eventFilter.selectClausesSize = 7;
/*Initialization*/
        for(int i = 0; i < 7; i++){
            selectClausesTest[i].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
            selectClausesTest[i].browsePathSize = 1;
            selectClausesTest[i].browsePath = (UA_QualifiedName*)
                UA_Array_new(eventFilter.selectClauses[i].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
            selectClausesTest[i].browsePath[0] = UA_QUALIFIEDNAME(0, "Test");
            selectClausesTest[i].attributeId = UA_ATTRIBUTEID_VALUE;
        }
/*typeDefinitionId not subtype of BaseEventType*/
        selectClausesTest[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_NUMBER);
/*attributeId not valid*/
        selectClausesTest[1].attributeId = 28;
/*browsePath contains null*/
        selectClausesTest[2].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "");
/*indexRange not valid*/
        selectClausesTest[3].indexRange = UA_STRING("test");
/*attributeId not value when indexRange is set*/
        selectClausesTest[4].attributeId = UA_ATTRIBUTEID_DATATYPE;
        selectClausesTest[4].indexRange = UA_STRING("1");
/*attributeId not value (should return UA_STATUSCODE_GOOD)*/
        selectClausesTest[5].attributeId = UA_ATTRIBUTEID_DATATYPE;
        eventFilter.selectClauses = selectClausesTest;
        retvals = UA_Server_initialSelectClauseValidation(server, &eventFilter);
        ck_assert_uint_eq(retvals[0], UA_STATUSCODE_BADTYPEDEFINITIONINVALID);
        ck_assert_uint_eq(retvals[1], UA_STATUSCODE_BADATTRIBUTEIDINVALID);
        ck_assert_uint_eq(retvals[2], UA_STATUSCODE_BADBROWSENAMEINVALID);
        ck_assert_uint_eq(retvals[3], UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(retvals[4], UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(retvals[5], UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(retvals[6], UA_STATUSCODE_GOOD);
        // UA_Array_delete(retvals,7, &UA_TYPES[UA_TYPES_STATUSCODE]);
    }
END_TEST


#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */

/* Assumes subscriptions work fine with data change because of other unit test */
static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Server Subscription Events");
    TCase *tc_server = tcase_create("Server Subscription Events");
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, generateEventEmptyFilter);
    tcase_add_test(tc_server, generateEvents);
    tcase_add_test(tc_server, createAbstractEvent);
    tcase_add_test(tc_server, createAbstractEventWithParent);
    tcase_add_test(tc_server, createNonAbstractEventWithParent);
    tcase_add_test(tc_server, multipleMonitoredItemsOneNode);
    tcase_add_test(tc_server, evaluateWhereClause);
    tcase_add_test(tc_server, initialWhereClauseValidation);
    tcase_add_test(tc_server, validateSelectClause);
    tcase_add_test(tc_server, WhereClauseValidation);

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
    suite_add_tcase(s, tc_server);

    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}