/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020-2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "server/ua_subscription.h"

#include <check.h>
#include <stdlib.h>
#include <stdio.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

UA_Client *client;

static UA_UInt32 subscriptionId;
static UA_UInt32 monitoredItemId;
static UA_NodeId eventType;
static size_t nSelectClauses = 4;
static UA_Boolean notificationReceived;
static UA_Boolean overflowNotificationReceived = false;
static UA_SimpleAttributeOperand *selectClauses;

UA_Double publishingInterval = 500.0;

static void
addNewEventType(void) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "SimpleEventType");
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "The simple event type we created");

    UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
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

static void
handler_events_simple(UA_Client *lclient, UA_UInt32 subId, void *subContext,
                      UA_UInt32 monId, void *monContext,
                      UA_KeyValueMap eventFields) {
    UA_Boolean foundSeverity = false;
    UA_Boolean foundMessage = false;
    UA_Boolean foundType = false;
    UA_Boolean foundSource = false;
    ck_assert_uint_eq(*(UA_UInt32 *) monContext, monitoredItemId);
    ck_assert_uint_eq(eventFields.mapSize, nSelectClauses);

    // check all event fields
    for(size_t i = 0; i < eventFields.mapSize; i++) {
        // find out which attribute of the event is being looked at
        UA_Variant *value = &eventFields.map[i].value;
        if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_UINT16])) {
            // Severity
            ck_assert_uint_eq(*(UA_UInt16*)value->data, 100);
            foundSeverity = true;
        } else if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            // Message
            UA_LocalizedText comp = UA_LOCALIZEDTEXT("en-US", "Generated Event");
            ck_assert(UA_String_equal(&((UA_LocalizedText *) value->data)->locale, &comp.locale));
            ck_assert(UA_String_equal(&((UA_LocalizedText *) value->data)->text, &comp.text));
            foundMessage = true;
        } else if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_NODEID])) {
            // either SourceNode or EventType
            UA_NodeId serverId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
            if(UA_NodeId_equal((UA_NodeId *) value->data, &eventType)) {
                // EventType
                foundType = true;
            } else if(UA_NodeId_equal((UA_NodeId *) value->data, &serverId)) {
                // SourceNode
                foundSource = true;
            } else {
                ck_assert_msg(false, "NodeId doesn't match");
            }
        } else {
            ck_assert_msg(false, "Field doesn't match");
        }
    }
    ck_assert_uint_eq(foundMessage, true);
    ck_assert_uint_eq(foundSeverity, true);
    ck_assert_uint_eq(foundType, true);
    ck_assert_uint_eq(foundSource, true);
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
    lockServer(server);
    Service_DeleteSubscriptions(server, &server->adminSession, &deleteSubscriptionsRequest,
                                &deleteSubscriptionsResponse);
    unlockServer(server);
    UA_DeleteSubscriptionsResponse_clear(&deleteSubscriptionsResponse);
}

THREAD_CALLBACK(serverloop) {
    while(running) {
        UA_Server_run_iterate(server, true);
    }
    return 0;
}

static void joinServer(void) {
    running = false;
    THREAD_JOIN(server_thread);
}

static void forkServer(void) {
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

static void
setup(void) {
    running = true;

    server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->maxPublishReqPerSession = 5;
    UA_Server_run_startup(server);

    addNewEventType();
    setupSelectClauses();
    THREAD_CREATE(server_thread, serverloop);

    client = UA_Client_newForUnitTest();

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Client can not connect to opc.tcp://localhost:4840. %s",
                UA_StatusCode_name(retval));
        exit(1);
    }
    setupSubscription();
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
}

static UA_StatusCode
createTestEvent(const UA_NodeId sourceNode) {
    UA_KeyValueMap eventFields = UA_KEYVALUEMAP_NULL;
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_KeyValueMap_setScalar(&eventFields, UA_QUALIFIEDNAME(0, "/Message"),
                             &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_StatusCode res =
        UA_Server_createEvent(server, eventType, sourceNode, 100, eventFields);
    UA_KeyValueMap_clear(&eventFields);
    return res;
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
    // add a monitored item
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, false, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
} END_TEST

/* Ensure events are received with proper values */
START_TEST(generateEvents) {
    // add a monitored item
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, true, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    // Create an event
    UA_StatusCode retval = createTestEvent(UA_NS0ID(SERVER));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    joinServer();

    // let the client fetch the event and check if the correct values were received
    notificationReceived = false;
    while(!notificationReceived) {
        UA_fakeSleep(500);
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(createResult.revisedQueueSize, 1);

    forkServer();

    // delete the monitoredItem
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subscriptionId;
    deleteRequest.monitoredItemIds = &monitoredItemId;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

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

static void
handler_events_propagate(UA_Client *lclient, UA_UInt32 subId, void *subContext,
                         UA_UInt32 monId, void *monContext,
                         const UA_KeyValueMap eventFields) {
    UA_Boolean foundSeverity = false;
    UA_Boolean foundMessage = false;
    UA_Boolean foundType = false;
    UA_Boolean foundSource = false;
    ck_assert_uint_eq(*(UA_UInt32 *) monContext, monitoredItemId);
    ck_assert_uint_eq(eventFields.mapSize, nSelectClauses);
    // check all event fields
    for(size_t i = 0; i < eventFields.mapSize; i++) {
        UA_Variant *value = &eventFields.map[i].value;
        // find out which attribute of the event is being looked at
        if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_UINT16])) {
            // Severity
            ck_assert_uint_eq(*(UA_UInt16 *)value->data, 100);
            foundSeverity = true;
        } else if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            // Message
            UA_LocalizedText comp = UA_LOCALIZEDTEXT("en-US", "Generated Event");
            ck_assert(UA_String_equal(&((UA_LocalizedText *) value->data)->locale, &comp.locale));
            ck_assert(UA_String_equal(&((UA_LocalizedText *) value->data)->text, &comp.text));
            foundMessage = true;
        } else if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_NODEID])) {
            // either SourceNode or EventType
            UA_NodeId serverNameSpaceId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_VENDORSERVERINFO);
            if(UA_NodeId_equal((UA_NodeId *) value->data, &eventType)) {
                // EventType
                foundType = true;
            } else if(UA_NodeId_equal((UA_NodeId *) value->data, &serverNameSpaceId)) {
                // SourceNode
                foundSource = true;
            } else {
                ck_assert_msg(false, "NodeId doesn't match");
            }
        } else {
            ck_assert_msg(false, "Field doesn't match");
        }
    }
    ck_assert_uint_eq(foundMessage, true);
    ck_assert_uint_eq(foundSeverity, true);
    ck_assert_uint_eq(foundType, true);
    ck_assert_uint_eq(foundSource, true);
    notificationReceived = true;
}

START_TEST(uppropagation) {
    //add a monitored item
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_propagate, true, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    // create the event on a child of server
    UA_StatusCode retval = createTestEvent(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_VENDORSERVERINFO));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    joinServer();

    // let the client fetch the event and check if the correct values were received
    notificationReceived = false;
    while(!notificationReceived) {
        UA_fakeSleep(500);
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(createResult.revisedQueueSize, 1);

    forkServer();

    // delete the monitoredItem
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subscriptionId;
    deleteRequest.monitoredItemIds = &monitoredItemId;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(*(deleteResponse.results), UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
} END_TEST

static void
handler_events_overflow(UA_Client *lclient, UA_UInt32 subId, void *subContext,
                        UA_UInt32 monId, void *monContext,
                        const UA_KeyValueMap eventFields) {
    ck_assert_uint_eq(*(UA_UInt32 *) monContext, monitoredItemId);
    if(eventFields.mapSize != 4)
        return;

    if(eventFields.map[2].value.type != &UA_TYPES[UA_TYPES_NODEID])
        return;

    UA_NodeId *eventType = (UA_NodeId*)eventFields.map[2].value.data;
    UA_NodeId comp = UA_NODEID_NUMERIC(0, UA_NS0ID_EVENTQUEUEOVERFLOWEVENTTYPE);
    if(UA_NodeId_equal(eventType, &comp)) {
        overflowNotificationReceived = true; /* Overflow was received */
    } else {
        handler_events_simple(lclient, subId, subContext, monId, monContext, eventFields);
    }
}

/* Ensures an eventQueueOverflowEvent is published when appropriate */
START_TEST(eventOverflow) {
    // add a monitored item
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_overflow, true, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    // create events
    UA_StatusCode retval = createTestEvent(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = createTestEvent(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    joinServer();

    // fetch the events, ensure both the overflow and the original event are received
    notificationReceived = false;
    overflowNotificationReceived = false;
    while(!notificationReceived) {
        UA_fakeSleep(500);
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(overflowNotificationReceived, true);
    ck_assert_uint_eq(createResult.revisedQueueSize, 1);

    forkServer();

    // delete the monitoredItem
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subscriptionId;
    deleteRequest.monitoredItemIds = &monitoredItemId;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(*(deleteResponse.results), UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
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

START_TEST(discardNewestOverflow) {
    // add a monitored item
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_overflow, true, false);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    // create a large amount of events, ensure the server doesnt crash because of it
    UA_StatusCode retval;
    for(size_t j = 0; j < 100; j++) {
        retval = createTestEvent(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // delete the monitoredItem
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subscriptionId;
    deleteRequest.monitoredItemIds = &monitoredItemId;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
            UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(*(deleteResponse.results), UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
} END_TEST

START_TEST(eventStressing) {
    // add a monitored item
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_overflow, true, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    // create a large amount of events, ensure the server doesnt crash because of it
    UA_StatusCode retval;
    for(size_t i = 0; i < 20; i++) {
        for(size_t j = 0; j < 30; j++) {
            retval = createTestEvent(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER));
            ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        }
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // delete the monitoredItem
    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subscriptionId;
    deleteRequest.monitoredItemIds = &monitoredItemId;
    deleteRequest.monitoredItemIdsSize = 1;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 1);
    ck_assert_uint_eq(*(deleteResponse.results), UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);
} END_TEST

START_TEST(evaluateFilterWhereClause) {
    //test empty filter
    UA_ContentFilter contentFilter;
    UA_ContentFilter_init(&contentFilter);

    UA_EventDescription ed;
    ed.eventType = eventType;
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.severity = 100;
    ed.otherEventFields = UA_KEYVALUEMAP_NULL;

    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(server, &server->adminSession,
                                               &contentFilter, &ed);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Illegal filter operators */
    UA_ContentFilterElement contentFilterElement;
    UA_ContentFilterElement_init(&contentFilterElement);
    contentFilter.elements = &contentFilterElement;
    contentFilter.elementsSize = 1;
    contentFilterElement.filterOperator = UA_FILTEROPERATOR_RELATEDTO;

    lockServer(server);
    retval = evaluateWhereClause(server, &server->adminSession,
                                 &contentFilter, &ed);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED);

    contentFilterElement.filterOperator = UA_FILTEROPERATOR_INVIEW;

    lockServer(server);
    retval = evaluateWhereClause(server, &server->adminSession,
                                 &contentFilter, &ed);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED);

    /* No operand provided */
    contentFilterElement.filterOperator = UA_FILTEROPERATOR_OFTYPE;
    lockServer(server);
    UA_ContentFilterElementResult elmRes =
        UA_ContentFilterElementValidation(server, 0, 1, &contentFilterElement);
    retval = elmRes.statusCode;
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
    UA_ContentFilterElementResult_clear(&elmRes);

    UA_ExtensionObject filterOperandExObj;
    UA_ExtensionObject_init(&filterOperandExObj);
    contentFilterElement.filterOperandsSize = 1;
    contentFilterElement.filterOperands = &filterOperandExObj;
    UA_LiteralOperand literalOperand;
    UA_LiteralOperand_init(&literalOperand);
    filterOperandExObj.content.decoded.data = &literalOperand;
    filterOperandExObj.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    filterOperandExObj.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;

    /* Same type*/
    UA_Variant_setScalar(&literalOperand.value, &eventType, &UA_TYPES[UA_TYPES_NODEID]);

    lockServer(server);
    retval = evaluateWhereClause(server, &server->adminSession,
                                 &contentFilter, &ed);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Base type*/
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    UA_Variant_setScalar(&literalOperand.value, &nodeId, &UA_TYPES[UA_TYPES_NODEID]);

    lockServer(server);
    retval = evaluateWhereClause(server, &server->adminSession,
                                 &contentFilter, &ed);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Other type*/
    nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEMODELCHANGEEVENTTYPE);

    lockServer(server);
    retval = evaluateWhereClause(server, &server->adminSession,
                                 &contentFilter, &ed);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
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
    tcase_add_test(tc_server, uppropagation);
    tcase_add_test(tc_server, eventOverflow);
    tcase_add_test(tc_server, multipleMonitoredItemsOneNode);
    tcase_add_test(tc_server, discardNewestOverflow);
    tcase_add_test(tc_server, eventStressing);
    tcase_add_test(tc_server, evaluateFilterWhereClause);
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
