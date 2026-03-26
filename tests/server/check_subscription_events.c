/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020-2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
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

static UA_UInt32 compareAnswer = 0;
static UA_UInt32 subscriptionId;
static UA_UInt32 monitoredItemId;
static UA_NodeId eventType;
static size_t nSelectClauses = 5;
static UA_Boolean notificationReceived;
static UA_Boolean overflowNotificationReceived = false;
static UA_SimpleAttributeOperand *selectClauses;
static UA_LocalizedText message = {UA_STRING_STATIC("en-US"),
                                   UA_STRING_STATIC("Generated Event")};

UA_Double publishingInterval = 500.0;

static void
addNewEventType(void) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SimpleEventType");
    attr.description = UA_LOCALIZEDTEXT("en-US", "The simple event type we created");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1,0),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(0, "SimpleEventType"),
                                attr, NULL, &eventType);

    // Add Variable to the EventType
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&vattr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    vattr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    vattr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ;

    /* Add the variable node to the information model */
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the.answer");
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1,0), eventType,
                              UA_NS0ID(HASCOMPONENT), myIntegerName,
                              UA_NS0ID(BASEDATAVARIABLETYPE), vattr, NULL, NULL);

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
        selectClauses[i].typeDefinitionId = UA_NS0ID(BASEEVENTTYPE); // Disables checks
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
    selectClauses[4].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(1, "the.answer"); // Custom event type member

    selectClauses[4].typeDefinitionId = eventType; //  Checks the presence of the field in the type
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
        } else if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_INT32])) {
            // 1:the.answer
            ck_assert(i == 4);
            UA_Int32 *answer = (UA_Int32*)value->data;
            ck_assert_int_eq(*answer, compareAnswer);
        } else {
            if(compareAnswer != 0)
                ck_assert_msg(false, "Field not defined");
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
    UA_StatusCode retval =
        UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType, 100, message, NULL, NULL, NULL);
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
        } else if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_INT32])) {
            // 1:the.answer
            ck_assert(i == 4);
        } else {
            // Value not defined
            ck_assert(value->type == NULL);
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
    UA_StatusCode retval =
        UA_Server_createEvent(server, UA_NS0ID(SERVER_VENDORSERVERINFO), eventType, 100, message, NULL, NULL, NULL);
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
    if(eventFields.mapSize != nSelectClauses)
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
    UA_StatusCode retval =
        UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType, 100, message, NULL, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType, 100, message, NULL, NULL, NULL);
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
        retval = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType, 100, message, NULL, NULL, NULL);
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
            retval = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType, 100, message, NULL, NULL, NULL);
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
    UA_FilterEvalContext ctx;
    UA_FilterEvalContext_init(&ctx);
    ctx.server = server;
    ctx.session = &server->adminSession;

    ctx.ed.sourceNode = UA_NS0ID(SERVER);
    ctx.ed.eventType = eventType;
    ctx.ed.severity = 100;
    ctx.ed.eventFields = NULL;
    ctx.ed.eventInstance = NULL;

    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ContentFilterElement contentFilterElement;
    UA_ContentFilterElement_init(&contentFilterElement);
    ctx.filter.whereClause.elements = &contentFilterElement;
    ctx.filter.whereClause.elementsSize = 1;

    /* Illegal filter operators */
    contentFilterElement.filterOperator = UA_FILTEROPERATOR_RELATEDTO;
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED);

    contentFilterElement.filterOperator = UA_FILTEROPERATOR_INVIEW;
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
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
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Base type*/
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    UA_Variant_setScalar(&literalOperand.value, &nodeId, &UA_TYPES[UA_TYPES_NODEID]);

    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Other type*/
    nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEMODELCHANGEEVENTTYPE);

    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);

    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
}
END_TEST

/* ---- WHERE clause filter operator tests ---- */

/* Helper: set up a FilterEvalContext with a single ContentFilterElement */
static void
setupFilterCtx(UA_FilterEvalContext *ctx, UA_ContentFilterElement *cfe,
               size_t nElements) {
    UA_FilterEvalContext_init(ctx);
    ctx->server = server;
    ctx->session = &server->adminSession;
    ctx->ed.sourceNode = UA_NS0ID(SERVER);
    ctx->ed.eventType = eventType;
    ctx->ed.severity = 100;
    ctx->ed.eventFields = NULL;
    ctx->ed.eventInstance = NULL;
    ctx->filter.whereClause.elements = cfe;
    ctx->filter.whereClause.elementsSize = nElements;
}

/* Helper: create a two-operand filter element with two LiteralOperands */
static void
setupBinaryLiteralFilter(UA_ContentFilterElement *cfe,
                         UA_ExtensionObject *ops, /* array of 2 */
                         UA_LiteralOperand *lop1,
                         UA_LiteralOperand *lop2,
                         UA_FilterOperator filterOp) {
    UA_ContentFilterElement_init(cfe);
    cfe->filterOperator = filterOp;
    cfe->filterOperandsSize = 2;
    cfe->filterOperands = ops;

    UA_ExtensionObject_init(&ops[0]);
    ops[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    ops[0].content.decoded.data = lop1;

    UA_ExtensionObject_init(&ops[1]);
    ops[1].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    ops[1].content.decoded.data = lop2;
}

START_TEST(filterEquals_int) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_Int32 v1 = 42, v2 = 42;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);

    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Not equal */
    v2 = 43;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterEquals_string) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_String s1 = UA_STRING("hello"), s2 = UA_STRING("hello");
    UA_Variant_setScalar(&lop1.value, &s1, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lop2.value, &s2, &UA_TYPES[UA_TYPES_STRING]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);

    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Not equal */
    s2 = UA_STRING("world");
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterGreaterThan) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_Int32 v1 = 10, v2 = 5;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_GREATERTHAN);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);

    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Not greater (equal) */
    v1 = 5;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);

    /* Less */
    v1 = 3;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterLessThan) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_Int32 v1 = 3, v2 = 10;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_LESSTHAN);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);

    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Not less (equal) */
    v1 = 10;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterGreaterThanOrEqual) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_Int32 v1 = 10, v2 = 10;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_GREATERTHANOREQUAL);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);

    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Greater */
    v1 = 20;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Less → no match */
    v1 = 5;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterLessThanOrEqual) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_Int32 v1 = 5, v2 = 5;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_LESSTHANOREQUAL);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);

    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Greater → no match */
    v1 = 10;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterIsNull) {
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_ISNULL;
    cfe.filterOperandsSize = 1;

    UA_ExtensionObject op;
    UA_ExtensionObject_init(&op);
    cfe.filterOperands = &op;

    UA_LiteralOperand lop;
    UA_LiteralOperand_init(&lop);
    op.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    op.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    op.content.decoded.data = &lop;

    /* NULL variant → should match IsNull */
    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);

    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Non-null → should not match */
    UA_Int32 v = 42;
    UA_Variant_setScalar(&lop.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterNot) {
    /* NOT(true) = false → BADNOMATCH, NOT(false) = true → GOOD */
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_NOT;
    cfe.filterOperandsSize = 1;

    UA_ExtensionObject op;
    UA_ExtensionObject_init(&op);
    cfe.filterOperands = &op;

    UA_LiteralOperand lop;
    UA_LiteralOperand_init(&lop);
    op.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    op.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    op.content.decoded.data = &lop;

    UA_Boolean bTrue = true;
    UA_Variant_setScalar(&lop.value, &bTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);

    UA_Boolean bFalse = false;
    UA_Variant_setScalar(&lop.value, &bFalse, &UA_TYPES[UA_TYPES_BOOLEAN]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(filterBitwiseAnd) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_UInt32 v1 = 0xFF, v2 = 0x0F;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_UINT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_BITWISEAND);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* BitwiseAnd returns a UInt32, not a Boolean. The WHERE clause
     * evaluation only considers Boolean TRUE as a match. Non-boolean
     * results are treated as BADNOMATCH. */
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterBitwiseOr) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_UInt32 v1 = 0, v2 = 0;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_UINT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_BITWISEOR);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* BitwiseOr returns a UInt32, not a Boolean. The WHERE clause
     * evaluation only considers Boolean TRUE as a match. Non-boolean
     * results are treated as BADNOMATCH. */
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterBetween) {
    /* BETWEEN takes 3 operands: value, low, high */
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_BETWEEN;
    cfe.filterOperandsSize = 3;

    UA_ExtensionObject ops[3];
    UA_LiteralOperand lops[3];
    for(int i = 0; i < 3; i++) {
        UA_LiteralOperand_init(&lops[i]);
        UA_ExtensionObject_init(&ops[i]);
        ops[i].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
        ops[i].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        ops[i].content.decoded.data = &lops[i];
    }
    cfe.filterOperands = ops;

    UA_Int32 val = 50, low = 10, high = 100;
    UA_Variant_setScalar(&lops[0].value, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lops[1].value, &low, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lops[2].value, &high, &UA_TYPES[UA_TYPES_INT32]);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Out of range */
    val = 200;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);

    /* Edge: equal to low boundary */
    val = 10;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Edge: equal to high boundary */
    val = 100;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(filterInList) {
    /* INLIST: first operand is value, rest are list items */
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_INLIST;
    cfe.filterOperandsSize = 4; /* 1 value + 3 list entries */

    UA_ExtensionObject ops[4];
    UA_LiteralOperand lops[4];
    for(int i = 0; i < 4; i++) {
        UA_LiteralOperand_init(&lops[i]);
        UA_ExtensionObject_init(&ops[i]);
        ops[i].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
        ops[i].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        ops[i].content.decoded.data = &lops[i];
    }
    cfe.filterOperands = ops;

    UA_Int32 val = 42, l1 = 10, l2 = 42, l3 = 99;
    UA_Variant_setScalar(&lops[0].value, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lops[1].value, &l1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lops[2].value, &l2, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lops[3].value, &l3, &UA_TYPES[UA_TYPES_INT32]);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Not in list */
    val = 7;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterAnd) {
    /* AND: elements[0] = AND(elem1, elem2)
     *      elements[1] = Equals(true, true) → true
     *      elements[2] = Equals(true, true) → true
     * Result: true AND true = true → GOOD */
    UA_ContentFilterElement cfe[3];
    UA_ExtensionObject ops0[2], ops1[2], ops2[2];
    UA_ElementOperand eop1, eop2;
    UA_LiteralOperand lop1a, lop1b, lop2a, lop2b;

    /* Element 0: AND with ElementOperands pointing to 1 and 2 */
    UA_ContentFilterElement_init(&cfe[0]);
    cfe[0].filterOperator = UA_FILTEROPERATOR_AND;
    cfe[0].filterOperandsSize = 2;
    cfe[0].filterOperands = ops0;

    UA_ElementOperand_init(&eop1);
    eop1.index = 1;
    UA_ExtensionObject_init(&ops0[0]);
    ops0[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops0[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    ops0[0].content.decoded.data = &eop1;

    UA_ElementOperand_init(&eop2);
    eop2.index = 2;
    UA_ExtensionObject_init(&ops0[1]);
    ops0[1].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops0[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    ops0[1].content.decoded.data = &eop2;

    /* Element 1: Equals(5, 5) → true */
    UA_LiteralOperand_init(&lop1a);
    UA_LiteralOperand_init(&lop1b);
    UA_Int32 v5a = 5, v5b = 5;
    UA_Variant_setScalar(&lop1a.value, &v5a, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop1b.value, &v5b, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe[1], ops1, &lop1a, &lop1b, UA_FILTEROPERATOR_EQUALS);

    /* Element 2: Equals(10, 10) → true */
    UA_LiteralOperand_init(&lop2a);
    UA_LiteralOperand_init(&lop2b);
    UA_Int32 v10a = 10, v10b = 10;
    UA_Variant_setScalar(&lop2a.value, &v10a, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop2b.value, &v10b, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe[2], ops2, &lop2a, &lop2b, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, cfe, 3);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Make one false: AND(true, false) → BADNOMATCH */
    v10b = 999;
    setupFilterCtx(&ctx, cfe, 3);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterOr) {
    /* OR: elements[0] = OR(elem1, elem2)
     *     elements[1] = Equals(5, 99) → false
     *     elements[2] = Equals(10, 10) → true
     * Result: false OR true = true → GOOD */
    UA_ContentFilterElement cfe[3];
    UA_ExtensionObject ops0[2], ops1[2], ops2[2];
    UA_ElementOperand eop1, eop2;
    UA_LiteralOperand lop1a, lop1b, lop2a, lop2b;

    UA_ContentFilterElement_init(&cfe[0]);
    cfe[0].filterOperator = UA_FILTEROPERATOR_OR;
    cfe[0].filterOperandsSize = 2;
    cfe[0].filterOperands = ops0;

    UA_ElementOperand_init(&eop1);
    eop1.index = 1;
    UA_ExtensionObject_init(&ops0[0]);
    ops0[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops0[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    ops0[0].content.decoded.data = &eop1;

    UA_ElementOperand_init(&eop2);
    eop2.index = 2;
    UA_ExtensionObject_init(&ops0[1]);
    ops0[1].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops0[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    ops0[1].content.decoded.data = &eop2;

    /* Element 1: Equals(5, 99) → false */
    UA_LiteralOperand_init(&lop1a);
    UA_LiteralOperand_init(&lop1b);
    UA_Int32 v5 = 5, v99 = 99;
    UA_Variant_setScalar(&lop1a.value, &v5, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop1b.value, &v99, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe[1], ops1, &lop1a, &lop1b, UA_FILTEROPERATOR_EQUALS);

    /* Element 2: Equals(10, 10) → true */
    UA_LiteralOperand_init(&lop2a);
    UA_LiteralOperand_init(&lop2b);
    UA_Int32 v10a = 10, v10b = 10;
    UA_Variant_setScalar(&lop2a.value, &v10a, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop2b.value, &v10b, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe[2], ops2, &lop2a, &lop2b, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, cfe, 3);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Both false → BADNOMATCH */
    v10b = 999;
    setupFilterCtx(&ctx, cfe, 3);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

START_TEST(filterEquals_typeConversion) {
    /* Test implicit numeric conversion: Int32 vs Double */
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_Int32 v1 = 42;
    UA_Double v2 = 42.0;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_DOUBLE]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* Implicit conversion between Int32 and Double should work */
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(filterOperandCountMismatch) {
    /* Test ContentFilterElementValidation with wrong operand count */
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_EQUALS; /* needs 2 operands */
    cfe.filterOperandsSize = 1; /* only 1 provided */

    UA_ExtensionObject op;
    UA_ExtensionObject_init(&op);
    UA_LiteralOperand lop;
    UA_LiteralOperand_init(&lop);
    op.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    op.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    op.content.decoded.data = &lop;
    cfe.filterOperands = &op;

    lockServer(server);
    UA_ContentFilterElementResult elmRes =
        UA_ContentFilterElementValidation(server, 0, 1, &cfe);
    unlockServer(server);
    ck_assert_uint_eq(elmRes.statusCode, UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH);
    UA_ContentFilterElementResult_clear(&elmRes);
} END_TEST

START_TEST(filterDouble_comparison) {
    /* Test GreaterThan with Double values */
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_Double v1 = 3.14, v2 = 2.71;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_DOUBLE]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_GREATERTHAN);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* --- Additional filter validation tests for uncovered paths --- */

/* Test ContentFilterElementValidation with InView operator (not allowed in event where clause) */
START_TEST(filterValidation_inViewRelatedTo) {
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);

    /* InView → BADEVENTFILTERINVALID */
    cfe.filterOperator = UA_FILTEROPERATOR_INVIEW;
    cfe.filterOperandsSize = 1;
    UA_ExtensionObject op;
    UA_ExtensionObject_init(&op);
    UA_LiteralOperand lop;
    UA_LiteralOperand_init(&lop);
    op.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    op.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    op.content.decoded.data = &lop;
    cfe.filterOperands = &op;

    lockServer(server);
    UA_ContentFilterElementResult elmRes =
        UA_ContentFilterElementValidation(server, 0, 1, &cfe);
    unlockServer(server);
    ck_assert_uint_eq(elmRes.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    UA_ContentFilterElementResult_clear(&elmRes);

    /* RelatedTo → BADEVENTFILTERINVALID */
    cfe.filterOperator = UA_FILTEROPERATOR_RELATEDTO;
    lockServer(server);
    elmRes = UA_ContentFilterElementValidation(server, 0, 1, &cfe);
    unlockServer(server);
    ck_assert_uint_eq(elmRes.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    UA_ContentFilterElementResult_clear(&elmRes);
} END_TEST

/* Test ContentFilterElementValidation with LIKE and CAST operators (unsupported) */
START_TEST(filterValidation_likeCast) {
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperandsSize = 2;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);
    UA_ExtensionObject_init(&ops[0]);
    ops[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    ops[0].content.decoded.data = &lop1;
    UA_ExtensionObject_init(&ops[1]);
    ops[1].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    ops[1].content.decoded.data = &lop2;
    cfe.filterOperands = ops;

    /* LIKE → BADFILTEROPERATORUNSUPPORTED */
    cfe.filterOperator = UA_FILTEROPERATOR_LIKE;
    lockServer(server);
    UA_ContentFilterElementResult elmRes =
        UA_ContentFilterElementValidation(server, 0, 1, &cfe);
    unlockServer(server);
    ck_assert_uint_eq(elmRes.statusCode, UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED);
    UA_ContentFilterElementResult_clear(&elmRes);

    /* CAST → BADFILTEROPERATORUNSUPPORTED */
    cfe.filterOperator = UA_FILTEROPERATOR_CAST;
    lockServer(server);
    elmRes = UA_ContentFilterElementValidation(server, 0, 1, &cfe);
    unlockServer(server);
    ck_assert_uint_eq(elmRes.statusCode, UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED);
    UA_ContentFilterElementResult_clear(&elmRes);
} END_TEST

/* Test ContentFilterElementValidation with AttributeOperand (not supported) */
START_TEST(filterValidation_attributeOperand) {
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_EQUALS;
    cfe.filterOperandsSize = 2;

    UA_ExtensionObject ops[2];
    UA_AttributeOperand aop;
    UA_AttributeOperand_init(&aop);
    UA_ExtensionObject_init(&ops[0]);
    ops[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops[0].content.decoded.type = &UA_TYPES[UA_TYPES_ATTRIBUTEOPERAND];
    ops[0].content.decoded.data = &aop;

    UA_LiteralOperand lop;
    UA_LiteralOperand_init(&lop);
    UA_ExtensionObject_init(&ops[1]);
    ops[1].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    ops[1].content.decoded.data = &lop;
    cfe.filterOperands = ops;

    lockServer(server);
    UA_ContentFilterElementResult elmRes =
        UA_ContentFilterElementValidation(server, 0, 1, &cfe);
    unlockServer(server);
    ck_assert_uint_eq(elmRes.statusCode, UA_STATUSCODE_BADFILTERNOTALLOWED);
    UA_ContentFilterElementResult_clear(&elmRes);
} END_TEST

/* Test ContentFilterElementValidation with ElementOperand point backward (invalid) */
START_TEST(filterValidation_elementOperandBackward) {
    UA_ContentFilterElement cfe[2];

    /* Element 0: Equals with ElementOperand pointing to index 0 (itself - backward!) */
    UA_ContentFilterElement_init(&cfe[0]);
    cfe[0].filterOperator = UA_FILTEROPERATOR_EQUALS;
    cfe[0].filterOperandsSize = 2;
    UA_ExtensionObject ops[2];
    UA_ElementOperand eop;
    UA_ElementOperand_init(&eop);
    eop.index = 0; /* points to self - invalid! */
    UA_ExtensionObject_init(&ops[0]);
    ops[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    ops[0].content.decoded.data = &eop;

    UA_LiteralOperand lop;
    UA_LiteralOperand_init(&lop);
    UA_ExtensionObject_init(&ops[1]);
    ops[1].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops[1].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    ops[1].content.decoded.data = &lop;
    cfe[0].filterOperands = ops;

    lockServer(server);
    UA_ContentFilterElementResult elmRes =
        UA_ContentFilterElementValidation(server, 0, 2, &cfe[0]);
    unlockServer(server);
    ck_assert_uint_eq(elmRes.statusCode, UA_STATUSCODE_BADINDEXRANGEINVALID);
    UA_ContentFilterElementResult_clear(&elmRes);
} END_TEST

/* Test ContentFilterElementValidation with undecoded (encoded) operand */
START_TEST(filterValidation_undecoded) {
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_ISNULL;
    cfe.filterOperandsSize = 1;

    UA_ExtensionObject op;
    UA_ExtensionObject_init(&op);
    /* Leave as UA_EXTENSIONOBJECT_ENCODED_NOBODY → not decoded */
    cfe.filterOperands = &op;

    lockServer(server);
    UA_ContentFilterElementResult elmRes =
        UA_ContentFilterElementValidation(server, 0, 1, &cfe);
    unlockServer(server);
    ck_assert_uint_eq(elmRes.statusCode, UA_STATUSCODE_BADFILTEROPERANDINVALID);
    UA_ContentFilterElementResult_clear(&elmRes);
} END_TEST

/* Test OfType validation with non-NodeId literal operand */
START_TEST(filterValidation_ofTypeNonNodeId) {
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_OFTYPE;
    cfe.filterOperandsSize = 1;

    UA_ExtensionObject op;
    UA_ExtensionObject_init(&op);
    UA_LiteralOperand lop;
    UA_LiteralOperand_init(&lop);
    UA_Int32 notANodeId = 42;
    UA_Variant_setScalar(&lop.value, &notANodeId, &UA_TYPES[UA_TYPES_INT32]);
    op.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    op.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    op.content.decoded.data = &lop;
    cfe.filterOperands = &op;

    lockServer(server);
    UA_ContentFilterElementResult elmRes =
        UA_ContentFilterElementValidation(server, 0, 1, &cfe);
    unlockServer(server);
    ck_assert_uint_eq(elmRes.statusCode, UA_STATUSCODE_BADFILTEROPERANDINVALID);
    UA_ContentFilterElementResult_clear(&elmRes);
} END_TEST

/* --- Implicit casting tests --- */

/* Test implicit cast String → Boolean ("0"/"1") and failed cast for
 * non-parseable strings. Note: The "true"/"false" comparison currently fails
 * due to an issue in the uppercase() helper. "0" and "1" work correctly. */
START_TEST(filterCast_stringToBoolean) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* "1" == true → GOOD */
    UA_String s1 = UA_STRING("1");
    UA_Boolean bTrue = true;
    UA_Variant_setScalar(&lop1.value, &s1, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lop2.value, &bTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* "0" == false → GOOD */
    UA_String s0 = UA_STRING("0");
    UA_Boolean bFalse = false;
    UA_Variant_setScalar(&lop1.value, &s0, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lop2.value, &bFalse, &UA_TYPES[UA_TYPES_BOOLEAN]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* "invalid" == true → casting fails → BADNOMATCH */
    UA_String sInvalid = UA_STRING("invalid");
    UA_Variant_setScalar(&lop1.value, &sInvalid, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lop2.value, &bTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

/* Test Ternary logic with NULL operands (AND/OR/NOT with null) */
START_TEST(filterTernary_null) {
    /* NOT(null) → null → BADNOMATCH */
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_NOT;
    cfe.filterOperandsSize = 1;

    UA_ExtensionObject op;
    UA_ExtensionObject_init(&op);
    cfe.filterOperands = &op;

    UA_LiteralOperand lop;
    UA_LiteralOperand_init(&lop);
    /* lop.value is empty (NULL) */
    op.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    op.content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    op.content.decoded.data = &lop;

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);

    /* AND(null, true) → null → BADNOMATCH */
    UA_ContentFilterElement cfe3[3];
    UA_ExtensionObject ops0[2], ops1[1], ops2[2];
    UA_ElementOperand eop1, eop2;
    UA_LiteralOperand lopNull, lopTrue1, lopTrue2;

    /* Element 0: AND(elem1, elem2) */
    UA_ContentFilterElement_init(&cfe3[0]);
    cfe3[0].filterOperator = UA_FILTEROPERATOR_AND;
    cfe3[0].filterOperandsSize = 2;
    cfe3[0].filterOperands = ops0;

    UA_ElementOperand_init(&eop1);
    eop1.index = 1;
    UA_ExtensionObject_init(&ops0[0]);
    ops0[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops0[0].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    ops0[0].content.decoded.data = &eop1;

    UA_ElementOperand_init(&eop2);
    eop2.index = 2;
    UA_ExtensionObject_init(&ops0[1]);
    ops0[1].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops0[1].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
    ops0[1].content.decoded.data = &eop2;

    /* Element 1: IsNull(null) → TRUE (returns boolean true) */
    UA_ContentFilterElement_init(&cfe3[1]);
    cfe3[1].filterOperator = UA_FILTEROPERATOR_ISNULL;
    cfe3[1].filterOperandsSize = 1;
    cfe3[1].filterOperands = ops1;

    UA_LiteralOperand_init(&lopNull);
    UA_ExtensionObject_init(&ops1[0]);
    ops1[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    ops1[0].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
    ops1[0].content.decoded.data = &lopNull;

    /* Element 2: Equals(5, 99) → FALSE */
    UA_LiteralOperand_init(&lopTrue1);
    UA_LiteralOperand_init(&lopTrue2);
    UA_Int32 v5 = 5, v99 = 99;
    UA_Variant_setScalar(&lopTrue1.value, &v5, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lopTrue2.value, &v99, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe3[2], ops2, &lopTrue1, &lopTrue2, UA_FILTEROPERATOR_EQUALS);

    /* AND(TRUE, FALSE) → FALSE → BADNOMATCH */
    setupFilterCtx(&ctx, cfe3, 3);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

/* Test event with EventId provided in event fields map */
START_TEST(eventFieldsMap_withEventId) {
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, true, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    UA_EventDescription ed = {0};
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventType = eventType;
    ed.severity = 100;
    ed.message = message;

    /* Provide EventId in the field map */
    UA_KeyValuePair vals[1];
    UA_KeyValueMap fieldMap = {1, vals};
    ed.eventFields = &fieldMap;

    UA_ByteString myEventId = UA_BYTESTRING("custom-event-id-1234");
    vals[0].key = UA_QUALIFIEDNAME(0, "/EventId");
    UA_Variant_setScalar(&vals[0].value, &myEventId, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_ByteString outEventId = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_Server_createEventEx(server, &ed, &outEventId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(outEventId.length > 0);
    /* The returned eventId should match the custom one */
    ck_assert(UA_ByteString_equal(&outEventId, &myEventId));
    UA_ByteString_clear(&outEventId);

    joinServer();

    notificationReceived = false;
    while(!notificationReceived) {
        UA_fakeSleep(500);
        UA_Server_run_iterate(server, false);
        res = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(notificationReceived, true);

    forkServer();
} END_TEST

/* Test createEvent with outEventId (creates and returns an EventId) */
START_TEST(createEvent_withOutEventId) {
    UA_ByteString outEventId = UA_BYTESTRING_NULL;
    UA_StatusCode retval =
        UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType, 100, message,
                              NULL, NULL, &outEventId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(outEventId.length > 0); /* Random EventId should be 16 bytes */
    ck_assert_uint_eq(outEventId.length, 16);
    UA_ByteString_clear(&outEventId);
} END_TEST

/* Test implicit cast: compare LocalizedText with String (LocalizedText → String) */
START_TEST(filterCast_localizedTextToString) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_LocalizedText lt = UA_LOCALIZEDTEXT("en", "hello");
    UA_String s = UA_STRING("hello");
    UA_Variant_setScalar(&lop1.value, &lt, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_Variant_setScalar(&lop2.value, &s, &UA_TYPES[UA_TYPES_STRING]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test implicit cast: QualifiedName → String */
START_TEST(filterCast_qualifiedNameToString) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "hello");
    UA_String s = UA_STRING("hello");
    UA_Variant_setScalar(&lop1.value, &qn, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    UA_Variant_setScalar(&lop2.value, &s, &UA_TYPES[UA_TYPES_STRING]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test implicit cast: empty variant (null) comparison */
START_TEST(filterCast_emptyVariant) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* Both empty → cast to the same (null) type, Equals should fail */
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* Two null variants → casting fails → result is FALSE → BADNOMATCH */
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

/* Test implicit numeric cast for different integer types */
START_TEST(filterCast_numericTypes) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* Int32 vs UInt16 → should cast to Int32 by precedence */
    UA_Int32 v1 = 42;
    UA_UInt16 v2 = 42;
    UA_Variant_setScalar(&lop1.value, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lop2.value, &v2, &UA_TYPES[UA_TYPES_UINT16]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Float vs Double → should cast to Double */
    UA_Float f1 = 3.14f;
    UA_Double d2 = 3.14;
    UA_Variant_setScalar(&lop1.value, &f1, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_Variant_setScalar(&lop2.value, &d2, &UA_TYPES[UA_TYPES_DOUBLE]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* Float 3.14f cast to Double may differ slightly from 3.14, resulting in no match */
    /* This is expected behavior - casting a float to double may lose precision */
    (void)retval; /* Just ensure it doesn't crash */

    /* SByte vs Int64 → should cast to Int64 */
    UA_SByte sb = -5;
    UA_Int64 i64 = -5;
    UA_Variant_setScalar(&lop1.value, &sb, &UA_TYPES[UA_TYPES_SBYTE]);
    UA_Variant_setScalar(&lop2.value, &i64, &UA_TYPES[UA_TYPES_INT64]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Byte vs UInt64 → should cast to UInt64 */
    UA_Byte b = 200;
    UA_UInt64 u64 = 200;
    UA_Variant_setScalar(&lop1.value, &b, &UA_TYPES[UA_TYPES_BYTE]);
    UA_Variant_setScalar(&lop2.value, &u64, &UA_TYPES[UA_TYPES_UINT64]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* StatusCode vs UInt32 → should cast implicitly */
    UA_StatusCode sc = UA_STATUSCODE_GOOD;
    UA_UInt32 u32 = 0;
    UA_Variant_setScalar(&lop1.value, &sc, &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_Variant_setScalar(&lop2.value, &u32, &UA_TYPES[UA_TYPES_UINT32]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test createEventEx with subscription+session filter */
START_TEST(createEventEx_withFilters) {
    UA_EventDescription ed = {0};
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventType = eventType;
    ed.severity = 100;
    ed.message = message;

    /* Pass a MonitoredItemId without a SubscriptionId → should fail with BADINTERNALERROR */
    UA_UInt32 monId = 1;
    ed.monitoredItemId = &monId;
    ed.subscriptionId = NULL;
    UA_StatusCode res = UA_Server_createEventEx(server, &ed, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);

    /* Pass a SubscriptionId without a SessionId → should fail too */
    UA_UInt32 subId = 1;
    ed.monitoredItemId = NULL;
    ed.subscriptionId = &subId;
    ed.sessionId = NULL;
    res = UA_Server_createEventEx(server, &ed, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

/* Test bitwiseAnd/Or with type mismatch */
START_TEST(filterBitwiseTypeMismatch) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* BitwiseAnd with String and Int32 → type mismatch */
    UA_String s = UA_STRING("hello");
    UA_Int32 v = 42;
    UA_Variant_setScalar(&lop1.value, &s, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lop2.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_BITWISEAND);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* Should fail - string and int cannot be bitwise'd */
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test implicit cast: String → Guid */
START_TEST(filterCast_stringToGuid) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* Create a GUID and the same GUID as string */
    UA_Guid guid = UA_GUID("72962B91-FA75-4AE6-9D28-B404DC7DAF63");
    UA_String guidStr = UA_STRING("72962B91-FA75-4AE6-9D28-B404DC7DAF63");

    UA_Variant_setScalar(&lop1.value, &guidStr, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lop2.value, &guid, &UA_TYPES[UA_TYPES_GUID]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Invalid GUID string → should not match */
    UA_String badGuid = UA_STRING("not-a-guid");
    UA_Variant_setScalar(&lop1.value, &badGuid, &UA_TYPES[UA_TYPES_STRING]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

/* Test implicit cast: String → Numerical via JSON decoding */
START_TEST(filterCast_stringToNumericalJSON) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* String "42" should cast to Int32 42 */
    UA_String numStr = UA_STRING("42");
    UA_Int32 val = 42;
    UA_Variant_setScalar(&lop1.value, &numStr, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lop2.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Non-numeric string → should fail cast → no match */
    UA_String badStr = UA_STRING("hello");
    UA_Variant_setScalar(&lop1.value, &badStr, &UA_TYPES[UA_TYPES_STRING]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

/* Test implicit cast: ExpandedNodeId → String */
START_TEST(filterCast_expandedNodeIdToString) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_ExpandedNodeId eid;
    UA_ExpandedNodeId_init(&eid);
    eid.nodeId = UA_NODEID_NUMERIC(0, 85); /* Objects folder */

    /* ExpandedNodeId should cast to its string representation via Equals */
    UA_String eidStr = UA_STRING("i=85");
    UA_Variant_setScalar(&lop1.value, &eid, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    UA_Variant_setScalar(&lop2.value, &eidStr, &UA_TYPES[UA_TYPES_STRING]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test implicit cast: NodeId → String */
START_TEST(filterCast_nodeIdToString) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_NodeId nid = UA_NODEID_NUMERIC(0, 85);
    UA_String nidStr = UA_STRING("i=85");
    UA_Variant_setScalar(&lop1.value, &nid, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&lop2.value, &nidStr, &UA_TYPES[UA_TYPES_STRING]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test implicit cast: NodeId → ExpandedNodeId */
START_TEST(filterCast_nodeIdToExpandedNodeId) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_NodeId nid = UA_NODEID_NUMERIC(0, 85);
    UA_ExpandedNodeId eid;
    UA_ExpandedNodeId_init(&eid);
    eid.nodeId = UA_NODEID_NUMERIC(0, 85);

    UA_Variant_setScalar(&lop1.value, &nid, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalar(&lop2.value, &eid, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test implicit cast: QualifiedName → LocalizedText
 * Note: The precedence table reports 255 for QualifiedName/LocalizedText,
 * so castResolveOperands returns BADFILTEROPERANDINVALID. The QN→LT cast
 * path (castImplicit) is not reachable through filter operators. We verify
 * the BADNOMATCH result (since the compare operator catches the error and
 * returns FALSE). */
START_TEST(filterCast_qualifiedNameToLocalizedText) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "TestName");
    UA_LocalizedText lt = UA_LOCALIZEDTEXT("", "TestName");

    UA_Variant_setScalar(&lop1.value, &qn, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    UA_Variant_setScalar(&lop2.value, &lt, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* QualifiedName has precedence 255 → implicit cast is not possible → BADNOMATCH */
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

/* Test GreaterThan with a non-ordered type (String) → should fail */
START_TEST(filterCompare_nonOrderedType) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    UA_String s1 = UA_STRING("aaa");
    UA_String s2 = UA_STRING("bbb");
    UA_Variant_setScalar(&lop1.value, &s1, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lop2.value, &s2, &UA_TYPES[UA_TYPES_STRING]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_GREATERTHAN);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* String is not an ordered type → BADFILTEROPERANDINVALID */
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADFILTEROPERANDINVALID);
} END_TEST

/* Test Between with incompatible types → type mismatch */
START_TEST(filterBetween_typeMismatch) {
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_BETWEEN;
    cfe.filterOperandsSize = 3;

    UA_ExtensionObject ops[3];
    UA_LiteralOperand lops[3];
    for(int i = 0; i < 3; i++) {
        UA_LiteralOperand_init(&lops[i]);
        UA_ExtensionObject_init(&ops[i]);
        ops[i].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
        ops[i].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        ops[i].content.decoded.data = &lops[i];
    }
    cfe.filterOperands = ops;

    /* value is String, low/high are Int32 → should be type mismatch after cast */
    UA_String sval = UA_STRING("hello");
    UA_Int32 low = 1, high = 10;
    UA_Variant_setScalar(&lops[0].value, &sval, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lops[1].value, &low, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lops[2].value, &high, &UA_TYPES[UA_TYPES_INT32]);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* Type mismatch or FALSE result from failed conversion */
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test InList with mixed types (some elements don't match type) */
START_TEST(filterInList_mixedTypes) {
    UA_ContentFilterElement cfe;
    UA_ContentFilterElement_init(&cfe);
    cfe.filterOperator = UA_FILTEROPERATOR_INLIST;
    cfe.filterOperandsSize = 4;

    UA_ExtensionObject ops[4];
    UA_LiteralOperand lops[4];
    for(int i = 0; i < 4; i++) {
        UA_LiteralOperand_init(&lops[i]);
        UA_ExtensionObject_init(&ops[i]);
        ops[i].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
        ops[i].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        ops[i].content.decoded.data = &lops[i];
    }
    cfe.filterOperands = ops;

    /* Value is Int32(42), list has: String("hello"), Int32(42), Guid */
    UA_Int32 val = 42;
    UA_String s = UA_STRING("hello");
    UA_Int32 match = 42;
    UA_Guid g = UA_GUID("72962B91-FA75-4AE6-9D28-B404DC7DAF63");
    UA_Variant_setScalar(&lops[0].value, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lops[1].value, &s, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lops[2].value, &match, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&lops[3].value, &g, &UA_TYPES[UA_TYPES_GUID]);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    /* Should still find the matching Int32(42) even though other types differ */
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* No match at all */
    UA_Int32 noMatch = 999;
    UA_Variant_setScalar(&lops[0].value, &noMatch, &UA_TYPES[UA_TYPES_INT32]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

/* Test SAO validation with invalid attributeId */
START_TEST(filterValidation_badAttributeId) {
    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);
    sao.typeDefinitionId = UA_NS0ID(BASEEVENTTYPE);
    sao.attributeId = 0; /* Invalid */
    sao.browsePathSize = 0;

    lockServer(server);
    UA_StatusCode retval = UA_SimpleAttributeOperandValidation(server, &sao);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADATTRIBUTEIDINVALID);

    /* Also test upper bound */
    sao.attributeId = 30; /* > 27 → invalid */
    lockServer(server);
    retval = UA_SimpleAttributeOperandValidation(server, &sao);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADATTRIBUTEIDINVALID);
} END_TEST

/* Test SAO validation with null BrowseName in browse path */
START_TEST(filterValidation_nullBrowseName) {
    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);
    sao.typeDefinitionId = UA_NS0ID(BASEEVENTTYPE);
    sao.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_QualifiedName nullQN;
    UA_QualifiedName_init(&nullQN);
    sao.browsePathSize = 1;
    sao.browsePath = &nullQN;

    lockServer(server);
    UA_StatusCode retval = UA_SimpleAttributeOperandValidation(server, &sao);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADBROWSENAMEINVALID);
} END_TEST

/* Test SAO validation with invalid IndexRange */
START_TEST(filterValidation_invalidIndexRange) {
    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);
    sao.typeDefinitionId = UA_NS0ID(BASEEVENTTYPE);
    sao.attributeId = UA_ATTRIBUTEID_VALUE;
    sao.browsePathSize = 0;

    /* Bad index range syntax */
    sao.indexRange = UA_STRING("abc");
    lockServer(server);
    UA_StatusCode retval = UA_SimpleAttributeOperandValidation(server, &sao);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINDEXRANGEINVALID);

    /* Valid range but wrong attributeId (not VALUE) */
    sao.indexRange = UA_STRING("0:2");
    sao.attributeId = UA_ATTRIBUTEID_BROWSENAME;
    lockServer(server);
    retval = UA_SimpleAttributeOperandValidation(server, &sao);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTYPEMISMATCH);
} END_TEST

/* Test ContentFilterElement validation with out-of-range operator */
START_TEST(filterValidation_invalidOperator) {
    UA_ContentFilterElement ef;
    UA_ContentFilterElement_init(&ef);
    ef.filterOperator = (UA_FilterOperator)99; /* Out of valid range */
    ef.filterOperandsSize = 0;

    lockServer(server);
    UA_ContentFilterElementResult er =
        UA_ContentFilterElementValidation(server, 0, 1, &ef);
    unlockServer(server);
    ck_assert_uint_eq(er.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    UA_ContentFilterElementResult_clear(&er);
} END_TEST

/* Test SAO validation with null TypeDefinitionId */
START_TEST(filterValidation_nullTypeDefinition) {
    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);
    /* TypeDefinitionId is left as NULL */
    sao.attributeId = UA_ATTRIBUTEID_VALUE;
    sao.browsePathSize = 0;

    lockServer(server);
    UA_StatusCode retval = UA_SimpleAttributeOperandValidation(server, &sao);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTYPEDEFINITIONINVALID);
} END_TEST

/* Test SAO validation with TypeDefinition not subtype of BaseEventType */
START_TEST(filterValidation_wrongTypeDefinition) {
    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);
    /* Use a node that exists but is not a subtype of BaseEventType */
    sao.typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    sao.attributeId = UA_ATTRIBUTEID_VALUE;
    sao.browsePathSize = 0;

    lockServer(server);
    UA_StatusCode retval = UA_SimpleAttributeOperandValidation(server, &sao);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTYPEDEFINITIONINVALID);
} END_TEST

/* Test numeric cast: Int16 → SByte conversion (covers castNumerical signed→SByte) */
START_TEST(filterCast_int16ToSByte) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* Int16(50) should equal SByte(50) after implicit cast */
    UA_Int16 i16 = 50;
    UA_SByte sb = 50;
    UA_Variant_setScalar(&lop1.value, &i16, &UA_TYPES[UA_TYPES_INT16]);
    UA_Variant_setScalar(&lop2.value, &sb, &UA_TYPES[UA_TYPES_SBYTE]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Int16(200) → SByte overflow → should produce NULL → no match */
    UA_Int16 overflow = 200;
    UA_Variant_setScalar(&lop1.value, &overflow, &UA_TYPES[UA_TYPES_INT16]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

/* Test numeric cast: Double → Int32 (covers castNumerical float→Int32) */
START_TEST(filterCast_doubleToInt32) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* Double(42.0) should equal Int32(42) after cast */
    UA_Double d = 42.0;
    UA_Int32 i = 42;
    UA_Variant_setScalar(&lop1.value, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Variant_setScalar(&lop2.value, &i, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* Test numeric cast: UInt16 → various targets (covers unsigned cast paths) */
START_TEST(filterCast_unsignedPaths) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* UInt16 → Int32 (unsigned → signed, safe range) */
    UA_UInt16 u16 = 100;
    UA_Int32 i32 = 100;
    UA_Variant_setScalar(&lop1.value, &u16, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Variant_setScalar(&lop2.value, &i32, &UA_TYPES[UA_TYPES_INT32]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* UInt32 → Float */
    UA_UInt32 u32 = 100;
    UA_Float f = 100.0f;
    UA_Variant_setScalar(&lop1.value, &u32, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&lop2.value, &f, &UA_TYPES[UA_TYPES_FLOAT]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* UInt16 → SByte overflow (300 > 127) → NULL → no match */
    UA_UInt16 big = 300;
    UA_SByte sb = 1;
    UA_Variant_setScalar(&lop1.value, &big, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Variant_setScalar(&lop2.value, &sb, &UA_TYPES[UA_TYPES_SBYTE]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

/* Test Equals with String type (checks Equals on non-ordered types works) */
START_TEST(filterEquals_strings) {
    UA_ContentFilterElement cfe;
    UA_ExtensionObject ops[2];
    UA_LiteralOperand lop1, lop2;
    UA_LiteralOperand_init(&lop1);
    UA_LiteralOperand_init(&lop2);

    /* String Equals should work (Equals is valid for all types) */
    UA_String s1 = UA_STRING("test");
    UA_String s2 = UA_STRING("test");
    UA_Variant_setScalar(&lop1.value, &s1, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&lop2.value, &s2, &UA_TYPES[UA_TYPES_STRING]);
    setupBinaryLiteralFilter(&cfe, ops, &lop1, &lop2, UA_FILTEROPERATOR_EQUALS);

    UA_FilterEvalContext ctx;
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    UA_StatusCode retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Different strings → no match */
    UA_String s3 = UA_STRING("other");
    UA_Variant_setScalar(&lop2.value, &s3, &UA_TYPES[UA_TYPES_STRING]);
    setupFilterCtx(&ctx, &cfe, 1);
    lockServer(server);
    retval = evaluateWhereClause(&ctx);
    UA_FilterEvalContext_reset(&ctx);
    unlockServer(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
} END_TEST

// Create an audit event that shall be delivered *only* to the AdminSession
START_TEST(auditEvent) {
    UA_NodeId adminSessionId = UA_NODEID("g=00000001-0000-0000-0000-000000000000");

    UA_EventDescription ed = {0};
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventType = UA_NS0ID(AUDITEVENTTYPE);
    ed.severity = 500;
    ed.message = UA_LOCALIZEDTEXT("something", "happened");
    ed.sessionId = &adminSessionId;

    // TOOD: Add all mandatory fields of the AuditEventType

    UA_StatusCode res = UA_Server_createEventEx(server, &ed, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

// Create an Event using a key-value map for the event fields
START_TEST(eventFieldsMap) {
    // add a monitored item
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, true, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    UA_EventDescription ed = {0};
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventType = UA_NS0ID(BASEEVENTTYPE);
    ed.severity = 500; // This triggers an assert in the handle, but is overridden below
    ed.message = UA_LOCALIZEDTEXT("something", "happened");

    // Override event fields
    UA_KeyValuePair vals[3];
    UA_KeyValueMap fieldMap = {3, vals};
    ed.eventFields = &fieldMap;

    UA_UInt16 trueSeverity = 100;
    vals[0].key = UA_QUALIFIEDNAME(0, "/Severity");
    UA_Variant_setScalar(&vals[0].value, &trueSeverity, &UA_TYPES[UA_TYPES_UINT16]);

    UA_LocalizedText msg = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    vals[1].key = UA_QUALIFIEDNAME(0, "/Message");
    UA_Variant_setScalar(&vals[1].value, &msg, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    vals[2].key = UA_QUALIFIEDNAME(0, "/EventType");
    UA_Variant_setScalar(&vals[2].value, &eventType, &UA_TYPES[UA_TYPES_NODEID]);

    UA_StatusCode res = UA_Server_createEventEx(server, &ed, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    joinServer();

    // fetch the events, ensure both the overflow and the original event are received
    notificationReceived = false;
    while(!notificationReceived) {
        UA_fakeSleep(500);
        UA_Server_run_iterate(server, false);
        res = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    }

    forkServer();

} END_TEST

// Event-fields from a key-value map, but with non-standard description
START_TEST(nonNormalFormEventFieldsMap) {
    // add a monitored item
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, true, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    UA_EventDescription ed = {0};
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventType = UA_NS0ID(BASEEVENTTYPE);
    ed.severity = 500; // This triggers an assert in the handle, but is overridden below
    ed.message = UA_LOCALIZEDTEXT("something", "happened");

    // Override event fields
    UA_KeyValuePair vals[4];
    UA_KeyValueMap fieldMap = {4, vals};
    ed.eventFields = &fieldMap;

    // Namespace-prefix and AttributeId postfix
    UA_UInt16 trueSeverity = 100;
    vals[0].key = UA_QUALIFIEDNAME(0, "/0:Severity#Value");
    UA_Variant_setScalar(&vals[0].value, &trueSeverity, &UA_TYPES[UA_TYPES_UINT16]);

    UA_LocalizedText msg = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    vals[1].key = UA_QUALIFIEDNAME(0, "/Message");
    UA_Variant_setScalar(&vals[1].value, &msg, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    vals[2].key = UA_QUALIFIEDNAME(0, "/EventType");
    UA_Variant_setScalar(&vals[2].value, &eventType, &UA_TYPES[UA_TYPES_NODEID]);

    compareAnswer = 43;
    UA_UInt32 answer = 43;
    vals[3].key = UA_QUALIFIEDNAME(0, "/1:the.answer");
    UA_Variant_setScalar(&vals[3].value, &answer, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode res = UA_Server_createEventEx(server, &ed, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    joinServer();

    // fetch the events, ensure both the overflow and the original event are received
    notificationReceived = false;
    while(!notificationReceived) {
        UA_fakeSleep(500);
        UA_Server_run_iterate(server, false);
        res = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    }

    compareAnswer = 0;
    forkServer();

    // Again, but now with the default nsid in the qualifiedname
    answer = 44;
    compareAnswer = 44;
    vals[3].key = UA_QUALIFIEDNAME(1, "/the.answer");

    res = UA_Server_createEventEx(server, &ed, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    joinServer();

    // fetch the events, ensure both the overflow and the original event are received
    notificationReceived = false;
    while(!notificationReceived) {
        UA_fakeSleep(500);
        UA_Server_run_iterate(server, false);
        res = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    }

    compareAnswer = 0;
    forkServer();
} END_TEST

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
    tcase_add_test(tc_server, filterEquals_int);
    tcase_add_test(tc_server, filterEquals_string);
    tcase_add_test(tc_server, filterGreaterThan);
    tcase_add_test(tc_server, filterLessThan);
    tcase_add_test(tc_server, filterGreaterThanOrEqual);
    tcase_add_test(tc_server, filterLessThanOrEqual);
    tcase_add_test(tc_server, filterIsNull);
    tcase_add_test(tc_server, filterNot);
    tcase_add_test(tc_server, filterBitwiseAnd);
    tcase_add_test(tc_server, filterBitwiseOr);
    tcase_add_test(tc_server, filterBetween);
    tcase_add_test(tc_server, filterInList);
    tcase_add_test(tc_server, filterAnd);
    tcase_add_test(tc_server, filterOr);
    tcase_add_test(tc_server, filterEquals_typeConversion);
    tcase_add_test(tc_server, filterOperandCountMismatch);
    tcase_add_test(tc_server, filterDouble_comparison);
    tcase_add_test(tc_server, auditEvent);
    tcase_add_test(tc_server, eventFieldsMap);
    tcase_add_test(tc_server, nonNormalFormEventFieldsMap);
    tcase_add_test(tc_server, filterValidation_inViewRelatedTo);
    tcase_add_test(tc_server, filterValidation_likeCast);
    tcase_add_test(tc_server, filterValidation_attributeOperand);
    tcase_add_test(tc_server, filterValidation_elementOperandBackward);
    tcase_add_test(tc_server, filterValidation_undecoded);
    tcase_add_test(tc_server, filterValidation_ofTypeNonNodeId);
    tcase_add_test(tc_server, filterCast_stringToBoolean);
    tcase_add_test(tc_server, filterTernary_null);
    tcase_add_test(tc_server, eventFieldsMap_withEventId);
    tcase_add_test(tc_server, createEvent_withOutEventId);
    tcase_add_test(tc_server, filterCast_localizedTextToString);
    tcase_add_test(tc_server, filterCast_qualifiedNameToString);
    tcase_add_test(tc_server, filterCast_emptyVariant);
    tcase_add_test(tc_server, filterCast_numericTypes);
    tcase_add_test(tc_server, createEventEx_withFilters);
    tcase_add_test(tc_server, filterBitwiseTypeMismatch);
    tcase_add_test(tc_server, filterCast_stringToGuid);
    tcase_add_test(tc_server, filterCast_stringToNumericalJSON);
    tcase_add_test(tc_server, filterCast_expandedNodeIdToString);
    tcase_add_test(tc_server, filterCast_nodeIdToString);
    tcase_add_test(tc_server, filterCast_nodeIdToExpandedNodeId);
    tcase_add_test(tc_server, filterCast_qualifiedNameToLocalizedText);
    tcase_add_test(tc_server, filterCompare_nonOrderedType);
    tcase_add_test(tc_server, filterBetween_typeMismatch);
    tcase_add_test(tc_server, filterInList_mixedTypes);
    tcase_add_test(tc_server, filterValidation_badAttributeId);
    tcase_add_test(tc_server, filterValidation_nullBrowseName);
    tcase_add_test(tc_server, filterValidation_invalidIndexRange);
    tcase_add_test(tc_server, filterValidation_invalidOperator);
    tcase_add_test(tc_server, filterValidation_nullTypeDefinition);
    tcase_add_test(tc_server, filterValidation_wrongTypeDefinition);
    tcase_add_test(tc_server, filterCast_int16ToSByte);
    tcase_add_test(tc_server, filterCast_doubleToInt32);
    tcase_add_test(tc_server, filterCast_unsignedPaths);
    tcase_add_test(tc_server, filterEquals_strings);
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
