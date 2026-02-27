/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
*    Copyright 2021 (c) Fraunhofer IOSB (Author: Andreas Ebner)
*/

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "server/ua_services.h"
#include "server/ua_server_internal.h"
#include "client/ua_client_internal.h"

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include <check.h>
#include <stdlib.h>
#include <stdio.h>

#include "testing_clock.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

static UA_Server *server;
static UA_Boolean running;
static size_t serverIterations;
static THREAD_HANDLE server_thread;
UA_NodeId EventType_A_Layer_1, EventType_B_Layer_1, EventType_C_Layer_2, EventType_D_Layer_3;

UA_Client *client;
static UA_UInt32 subscriptionId;
static UA_UInt32 monitoredItemId;

UA_Double publishingInterval = 500.0;
static UA_SimpleAttributeOperand *selectClauses;
static UA_Boolean notificationReceived;
static UA_UInt32 defaultSlectClauseSize = 4;
static UA_NodeId eventType;

static UA_EventFilterParserOptions options = {&UA_Log_Stdout_};

static void
addEventType(char* name, UA_NodeId parentNodeId, UA_NodeId requestedId, UA_NodeId* newEventType) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.description = UA_LOCALIZEDTEXT("en-US", name);
    UA_StatusCode retval = UA_Server_addObjectTypeNode(server, requestedId,
                                                       parentNodeId,
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                       UA_QUALIFIEDNAME(0, name),
                                                       attr, NULL, newEventType);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

/* Event Structure below
         EventType
             - EventType_A_Layer_1
             - EventType_B_Layer_1
                 - EventType_C_Layer_2
                     - EventType_D_Layer_3
*/
static void addEventTypes(void){
    addEventType("EventType_A_Layer_1",
                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                  UA_NODEID_NUMERIC(1, 5000),
                  &EventType_A_Layer_1);
    addEventType("EventType_B_Layer_1",
                 UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE),
                 UA_NODEID_NUMERIC(1, 5001),
                 &EventType_B_Layer_1);
    addEventType("EventType_C_Layer_2",
                 EventType_B_Layer_1,
                 UA_NODEID_NUMERIC(1, 5002),
                 &EventType_C_Layer_2);
    addEventType("EventType_D_Layer_3",
                 EventType_C_Layer_2,
                 UA_NODEID_NUMERIC(1, 5003),
                 &EventType_D_Layer_3);
}

static void
handler_events_simple(UA_Client *lclient, UA_UInt32 subId, void *subContext,
                      UA_UInt32 monId, void *monContext,
                      const UA_KeyValueMap eventFields) {
    UA_Boolean foundSeverity = false;
    UA_Boolean foundMessage = false;
    UA_Boolean foundType = false;
    UA_Boolean foundSource = false;
    ck_assert_uint_eq(*(UA_UInt32*)monContext, monitoredItemId);

    /* Check all event fields */
    for(size_t i = 0; i < eventFields.mapSize; i++) {
        /*  find out which attribute of the event is being looked at */
        const UA_Variant *value = &eventFields.map[i].value;
        if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_UINT16])) {
            /*  Severity */
            ck_assert_uint_eq(*(UA_UInt16*)value->data, 100);
            foundSeverity = true;
        } else if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT])) {
            /*  Message */
            UA_LocalizedText comp = UA_LOCALIZEDTEXT("en-US", "Generated Event");
            ck_assert(UA_String_equal(&((UA_LocalizedText *) value->data)->locale, &comp.locale));
            ck_assert(UA_String_equal(&((UA_LocalizedText *) value->data)->text, &comp.text));
            foundMessage = true;
        } else if(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_NODEID])) {
            /*  either SourceNode or EventType */
            UA_NodeId serverId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
            if(UA_NodeId_equal((UA_NodeId*)value->data, &eventType)) {
                foundType = true; /*  EventType */
            } else if(UA_NodeId_equal((UA_NodeId*)value->data, &serverId)) {
                foundSource = true; /*  SourceNode */
            } else {
                ck_assert_msg(false, "NodeId doesn't match");
            }
        }
    }
    ck_assert_uint_eq(foundMessage, true);
    ck_assert_uint_eq(foundSeverity, true);
    ck_assert_uint_eq(foundType, true);
    ck_assert_uint_eq(foundSource, true);
    notificationReceived = true;
}

static UA_Boolean severityExpected = false;

static void
handlerEventsWithModification(UA_Client *lclient, UA_UInt32 subId, void *subContext,
                                 UA_UInt32 monId, void *monContext,
                                 const UA_KeyValueMap eventFields) {

    ck_assert_uint_eq(*(UA_UInt32*)monContext, monitoredItemId);

    if (severityExpected)
        ck_assert_uint_eq(eventFields.mapSize, 2);
    else
        ck_assert_uint_eq(eventFields.mapSize, 1);

    const UA_QualifiedName messageName = UA_QUALIFIEDNAME(0, "/Message");
    const UA_QualifiedName severityName = UA_QUALIFIEDNAME(0, "/Severity");
    const UA_LocalizedText expectedMessage = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    const uint16_t expectedSeverity = 100;

    ck_assert(UA_QualifiedName_equal(&eventFields.map[0].key, &messageName));
    ck_assert(UA_Variant_hasScalarType(&eventFields.map[0].value, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]));
    ck_assert(UA_LocalizedText_equal((UA_LocalizedText *)eventFields.map[0].value.data, &expectedMessage));

    if (severityExpected) {
        ck_assert(UA_QualifiedName_equal(&eventFields.map[1].key, &severityName));
        ck_assert(UA_Variant_hasScalarType(&eventFields.map[1].value, &UA_TYPES[UA_TYPES_UINT16]));
        ck_assert_uint_eq(*(UA_UInt16 *)eventFields.map[1].value.data, expectedSeverity);
    }

    notificationReceived = true;
}

UA_Boolean monitoredItemModificationCallbackCalled = false;
UA_Boolean asyncMonitoredItemModificationErrorExpected = false;

static void
modifyMonitoredItemsCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                             UA_ModifyMonitoredItemsResponse *response) {
    ck_assert_uint_eq(response->resultsSize, 1);
    ck_assert_uint_eq(response->results->statusCode,
                      asyncMonitoredItemModificationErrorExpected ?
                          UA_STATUSCODE_BADEVENTFILTERINVALID : UA_STATUSCODE_GOOD);

    monitoredItemModificationCallbackCalled = true;
}

THREAD_CALLBACK(serverloop) {
    while (running) {
        UA_Server_run_iterate(server, true);
        serverIterations++;
    }
    return 0;
}

static void runServer(void) {
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

static void pauseServer(void) {
    running = false;
    THREAD_JOIN(server_thread);
}

static void
sleepUntilAnswer(UA_Double sleepMs) {
    pauseServer();
    UA_fakeSleep((UA_UInt32)sleepMs);
    UA_Server_run_iterate(server, true);
    runServer();
}

static void setup(void){
    /* Setup Server */
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    config->maxPublishReqPerSession = 5;
    UA_Server_run_startup(server);
    addEventTypes();
    THREAD_CREATE(server_thread, serverloop);

    /* Setup Client */
    client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        fprintf(stderr, "Client can not connect to opc.tcp://localhost:4840. %s",
                UA_StatusCode_name(retval));
        exit(1);
    }
    /*  Create subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    subscriptionId = response.subscriptionId;
    sleepUntilAnswer(publishingInterval + 100);
}

static void teardown(void) {
    /* Delete Server */
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);

    /* Delete Client */
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

static UA_MonitoredItemCreateResult
addMonitoredItem(UA_Client_EventNotificationCallback handler, UA_EventFilter *filter, bool discardOldest) {
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, 2253); /*  Root->Objects->Server */
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;

    if (filter) {
        item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
        item.requestedParameters.filter.content.decoded.data = filter;
        item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
    }

    item.requestedParameters.queueSize = 1;
    item.requestedParameters.discardOldest = discardOldest;

    return UA_Client_MonitoredItems_createEvent(client, subscriptionId,
                                                UA_TIMESTAMPSTORETURN_BOTH, item,
                                                &monitoredItemId, handler, NULL);
}

static UA_ModifyMonitoredItemsResponse
modifyMonitoredItem(UA_EventFilter *filter, bool discardOldest) {
    UA_ModifyMonitoredItemsRequest modifyRequest;
    UA_ModifyMonitoredItemsRequest_init(&modifyRequest);
    modifyRequest.subscriptionId = subscriptionId;
    modifyRequest.itemsToModifySize = 1;
    modifyRequest.itemsToModify = UA_MonitoredItemModifyRequest_new();
    modifyRequest.itemsToModify->monitoredItemId = monitoredItemId;
    modifyRequest.itemsToModify->requestedParameters.queueSize = 1;
    modifyRequest.itemsToModify->requestedParameters.discardOldest = true;
    UA_ExtensionObject_setValueNoDelete(&modifyRequest.itemsToModify->requestedParameters.filter,
                                        filter, &UA_TYPES[UA_TYPES_EVENTFILTER]);

    const UA_ModifyMonitoredItemsResponse response = UA_Client_MonitoredItems_modify(client, modifyRequest);
    UA_ModifyMonitoredItemsRequest_clear(&modifyRequest);
    return response;
}

static void
modifyMonitoredItemAsync(UA_EventFilter *filter, bool discardOldest) {
    UA_ModifyMonitoredItemsRequest modifyRequest;
    UA_ModifyMonitoredItemsRequest_init(&modifyRequest);
    modifyRequest.subscriptionId = subscriptionId;
    modifyRequest.itemsToModifySize = 1;
    modifyRequest.itemsToModify = UA_MonitoredItemModifyRequest_new();
    modifyRequest.itemsToModify->monitoredItemId = monitoredItemId;
    modifyRequest.itemsToModify->requestedParameters.queueSize = 1;
    modifyRequest.itemsToModify->requestedParameters.discardOldest = true;
    UA_ExtensionObject_setValueNoDelete(&modifyRequest.itemsToModify->requestedParameters.filter,
                                        filter, &UA_TYPES[UA_TYPES_EVENTFILTER]);

    const UA_StatusCode ret = UA_Client_MonitoredItems_modify_async(client, modifyRequest,
                                                                    modifyMonitoredItemsCallback,
                                                                    NULL, NULL);

    UA_ModifyMonitoredItemsRequest_clear(&modifyRequest);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    monitoredItemModificationCallbackCalled = false;
    sleepUntilAnswer(publishingInterval + 100);
    UA_StatusCode retval = UA_Client_run_iterate(client, 0);
    sleepUntilAnswer(publishingInterval + 100);
    retval |= UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(monitoredItemModificationCallbackCalled);
}

static void
createTestEvent(void) {
    UA_UInt16 severity = 100;
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_StatusCode res = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType,
                                              severity, message, NULL, NULL, NULL);
}

static void deleteMonitoredItems(void){
    /*  delete the monitoredItem */
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
}

static void
checkForEvent(UA_MonitoredItemCreateResult *createResult, UA_Boolean expect){
    /*  let the client fetch the event and check if the correct values were received */
    notificationReceived = false;
    sleepUntilAnswer(publishingInterval + 100);
    UA_StatusCode retval = UA_Client_run_iterate(client, 0);
    sleepUntilAnswer(publishingInterval + 100);
    retval |= UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, expect);
    ck_assert_uint_eq(createResult->revisedQueueSize, 1);
}

START_TEST(selectFilterValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    char *query = "SELECT /FOOBAR";
    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    ck_assert_int_eq(createResult.filterResult.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert(createResult.filterResult.content.decoded.type == &UA_TYPES[UA_TYPES_EVENTFILTERRESULT]);
    UA_EventFilterResult *eventFilterResult = (UA_EventFilterResult *)createResult.filterResult.content.decoded.data;
    ck_assert_uint_eq(eventFilterResult->selectClauseResultsSize, 1);
    ck_assert_uint_eq(eventFilterResult->selectClauseResults[0], UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_MonitoredItemCreateResult_clear(&createResult);

    UA_QualifiedName_clear(&filter.selectClauses->browsePath[0]);
    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    ck_assert_int_eq(createResult.filterResult.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert(createResult.filterResult.content.decoded.type == &UA_TYPES[UA_TYPES_EVENTFILTERRESULT]);
    eventFilterResult = (UA_EventFilterResult *)createResult.filterResult.content.decoded.data;
    ck_assert_uint_eq(eventFilterResult->selectClauseResultsSize, 1);
    ck_assert_uint_eq(eventFilterResult->selectClauseResults[0], UA_STATUSCODE_BADBROWSENAMEINVALID);
    UA_MonitoredItemCreateResult_clear(&createResult);
    UA_EventFilter_clear(&filter);
} END_TEST

/* Test Case "not-Operator" Description:
 Phase 1:
  Action -> Fire default "EventType_A_Layer_1" Event
  Event-Source: Server-Object
  Filters: Select(Severity, Message, EventType, SourceNode) Where (!true)
  Expect: No Notification
Phase 2:
  Action -> Fire default "EventType_A_Layer_1" Event
  Event-Source: Server-Object
  Filters: Select(Severity, Message, EventType, SourceNode) Where (!false)
  Expect: Get Notification */
START_TEST(notOperatorValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    /* Phase 1 */
    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode, /EventId "
        "WHERE !TRUE && /EventId == TRUE";
    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* setup event */
    eventType = EventType_A_Layer_1;

    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /* trigger the event */
    createTestEvent();
    checkForEvent(&createResult, false);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);

    /* Phase 2 */
    query =
        "SELECT /Severity, /Message, /EventType, /SourceNode, /EventId "
        "WHERE !FALSE";
    res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /* trigger the event */
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

/* Test Case "ofType-Operator" Description:
 Phase 1:
  Action -> Fire default "EventType_A_Layer_1" Event
  Event-Source: Server-Object
  Filters: Select(Severity, Message, EventType, SourceNode) Where (ofType EventType_B_Layer_1)
  Expect: No Notification
Phase 2:
  Action -> Fire default "EventType_B_Layer_1" Event
  Event-Source: Server-Object
  Filters: Select(Severity, Message, EventType, SourceNode) Where (ofType EventType_B_Layer_1)
  Expect: Get Notification
*/
START_TEST(ofTypeOperatorValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE OFTYPE ns=1;i=5001";
    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /*  trigger the event */
    eventType = EventType_A_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, false);

    /* trigger the event */
    eventType = EventType_B_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(ofTypeOperatorValidation_failure) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE OFTYPE ns=0;i=0";
    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* setup event */
    eventType = EventType_A_Layer_1;

    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    ck_assert_int_eq(createResult.filterResult.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert(createResult.filterResult.content.decoded.type == &UA_TYPES[UA_TYPES_EVENTFILTERRESULT]);
    UA_EventFilterResult *eventFilterResult = (UA_EventFilterResult *)createResult.filterResult.content.decoded.data;
    ck_assert_uint_eq(eventFilterResult->whereClauseResult.elementResultsSize, 1);
    ck_assert_uint_eq(eventFilterResult->whereClauseResult.elementResults[0].statusCode, UA_STATUSCODE_BADFILTEROPERANDINVALID);
    UA_MonitoredItemCreateResult_clear(&createResult);
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(orTypeOperatorValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE TRUE == TRUE OR TRUE == FALSE";
    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /* trigger the event */
    eventType = EventType_B_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(andTypeOperatorValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE TRUE == TRUE AND TRUE == FALSE";
    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /* trigger the event */
    eventType = EventType_B_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, false);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(bitwiseOperatorValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE (1 & 3) == 1";
    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /* trigger the event */
    eventType = EventType_B_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(equalOperatorValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    // Equality success
    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE 62541 == 62541";
    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateResult createResult =
        addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    eventType = EventType_A_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);

    // Equality failure
    query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE 62542 == 62541";
    res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    createTestEvent();
    checkForEvent(&createResult, false);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);

    // types wich need implicit cast
    query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE INT32 62541 == INT64 62541";
    res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);

    // check equal with nodeid
    query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE NODEID i=14123 == NODEID i=14123";
    res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(orderedCompareOperatorValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE 100 < 1000";

    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    eventType = EventType_A_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(betweenOperatorValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE 40 BETWEEN [10, 100]";

    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    eventType = EventType_A_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(inListOperatorValidation) {
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    char *query =
        "SELECT /Severity, /Message, /EventType, /SourceNode "
        "WHERE 40 INLIST [10, 100, 40]";

    UA_StatusCode res = UA_EventFilter_parse(&filter, UA_STRING(query), &options);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    eventType = EventType_A_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(modifySelectFilterValidation) {
    /* setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClausesSize = 1;
    filter.selectClauses = UA_SimpleAttributeOperand_new();
    filter.selectClauses->typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    filter.selectClauses->browsePathSize = 1;
    filter.selectClauses->browsePath = UA_QualifiedName_new();
    filter.selectClauses->attributeId = UA_ATTRIBUTEID_VALUE;

    /* Set up a valid monitored item */
    filter.selectClauses->browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(createResult.filterResult.encoding, UA_EXTENSIONOBJECT_ENCODED_NOBODY);
    monitoredItemId = createResult.monitoredItemId;
    UA_QualifiedName_clear(&filter.selectClauses->browsePath[0]);

    /* Attempt to update the monitored item's event filter with a different valid filter */
    filter.selectClauses->browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Time");
    UA_ModifyMonitoredItemsResponse modifyResult = modifyMonitoredItem(&filter, true);
    ck_assert_uint_eq(modifyResult.resultsSize, 1);
    ck_assert_uint_eq(modifyResult.results->statusCode, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(modifyResult.results->filterResult.encoding, UA_EXTENSIONOBJECT_ENCODED_NOBODY);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResult);
    UA_QualifiedName_clear(&filter.selectClauses->browsePath[0]);

    /* Attempt to update the monitored item's event filter with an invalid event field */
    filter.selectClauses->browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "IDONOTEXIST");
    modifyResult = modifyMonitoredItem(&filter, true);
    ck_assert_uint_eq(modifyResult.resultsSize, 1);
    ck_assert_uint_eq(modifyResult.results->statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);

    /* Expect an EventFilterResult with a bad status code for the first select clause result */
    ck_assert_int_eq(modifyResult.results->filterResult.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert(modifyResult.results->filterResult.content.decoded.type == &UA_TYPES[UA_TYPES_EVENTFILTERRESULT]);
    UA_EventFilterResult *eventFilterResult = (UA_EventFilterResult *)modifyResult.results->filterResult.content.decoded.data;
    ck_assert_uint_eq(eventFilterResult->selectClauseResultsSize, 1);
    ck_assert_uint_eq(eventFilterResult->selectClauseResults[0], UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResult);
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(modifySelectFilterSync) {
    /* setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    /* Prepare two select clauses, the first check only uses the first one */
    filter.selectClausesSize = 1;
    filter.selectClauses = (UA_SimpleAttributeOperand *)UA_Array_new(2, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    filter.selectClauses[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    filter.selectClauses[0].browsePathSize = 1;
    filter.selectClauses[0].browsePath = UA_QualifiedName_new();
    filter.selectClauses[0].attributeId = UA_ATTRIBUTEID_VALUE;
    filter.selectClauses[1].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    filter.selectClauses[1].browsePathSize = 1;
    filter.selectClauses[1].browsePath = UA_QualifiedName_new();
    filter.selectClauses[1].attributeId = UA_ATTRIBUTEID_VALUE;

    /* Set up a valid monitored item */
    filter.selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handlerEventsWithModification, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(createResult.filterResult.encoding, UA_EXTENSIONOBJECT_ENCODED_NOBODY);
    monitoredItemId = createResult.monitoredItemId;

    UA_Client_Subscription *sub = findSubscriptionById(client, subscriptionId);
    ck_assert(sub != NULL);
    UA_Client_MonitoredItem *mon = findMonitoredItemById(sub, monitoredItemId);
    ck_assert(mon != NULL);
    UA_UInt32 oldClientHandle = mon->clientHandle;

    severityExpected = false;
    createTestEvent();
    checkForEvent(&createResult, true);

    /* Attempt to update the monitored item's event filter with a second select clause */
    filter.selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");
    filter.selectClausesSize = 2;
    UA_ModifyMonitoredItemsResponse modifyResult = modifyMonitoredItem(&filter, true);
    ck_assert_uint_eq(modifyResult.resultsSize, 1);
    ck_assert_uint_eq(modifyResult.results->statusCode, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(modifyResult.results->filterResult.encoding, UA_EXTENSIONOBJECT_ENCODED_NOBODY);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResult);

    /* Make sure the client handle was updated */
    ck_assert_uint_ne(oldClientHandle, mon->clientHandle);
    oldClientHandle = mon->clientHandle;

    /* Check if the event fields map in the client's local monitored item was updated with the new filter */
    severityExpected = true;
    createTestEvent();
    checkForEvent(&createResult, true);

    /* Attempt to update with an invalid event filter */
    filter.selectClausesSize = 0;
    modifyResult = modifyMonitoredItem(&filter, true);
    ck_assert_uint_eq(modifyResult.resultsSize, 1);
    ck_assert_uint_eq(modifyResult.results->statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    UA_ModifyMonitoredItemsResponse_clear(&modifyResult);

    /* Check if the client handle is unchanged and the notifications are still processed */
    ck_assert_uint_eq(oldClientHandle, mon->clientHandle);
    severityExpected = true;
    createTestEvent();
    checkForEvent(&createResult, true);

    /* Free event filter members */
    filter.selectClausesSize = 2;
    UA_EventFilter_clear(&filter);
    UA_MonitoredItemCreateResult_clear(&createResult);

    deleteMonitoredItems();
} END_TEST

START_TEST(modifySelectFilterAsync) {
    /* setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);

    /* Prepare two select clauses, the first check only uses the first one */
    filter.selectClausesSize = 1;
    filter.selectClauses = (UA_SimpleAttributeOperand *)UA_Array_new(2, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    filter.selectClauses[0].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    filter.selectClauses[0].browsePathSize = 1;
    filter.selectClauses[0].browsePath = UA_QualifiedName_new();
    filter.selectClauses[0].attributeId = UA_ATTRIBUTEID_VALUE;
    filter.selectClauses[1].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    filter.selectClauses[1].browsePathSize = 1;
    filter.selectClauses[1].browsePath = UA_QualifiedName_new();
    filter.selectClauses[1].attributeId = UA_ATTRIBUTEID_VALUE;

    /* Set up a valid monitored item */
    filter.selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handlerEventsWithModification, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(createResult.filterResult.encoding, UA_EXTENSIONOBJECT_ENCODED_NOBODY);
    monitoredItemId = createResult.monitoredItemId;

    UA_Client_Subscription *sub = findSubscriptionById(client, subscriptionId);
    ck_assert(sub != NULL);
    UA_Client_MonitoredItem *mon = findMonitoredItemById(sub, monitoredItemId);
    ck_assert(mon != NULL);
    UA_UInt32 oldClientHandle = mon->clientHandle;

    severityExpected = false;
    createTestEvent();
    checkForEvent(&createResult, true);

    /* Attempt to update the monitored item's event filter with a second select clause */
    filter.selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");
    filter.selectClausesSize = 2;
    asyncMonitoredItemModificationErrorExpected = false;
    modifyMonitoredItemAsync(&filter, true);

    /* Make sure the client handle was updated */
    ck_assert_uint_ne(oldClientHandle, mon->clientHandle);
    oldClientHandle = mon->clientHandle;

    /* Check if the event fields map in the client's local monitored item was updated with the new filter */
    severityExpected = true;
    createTestEvent();
    checkForEvent(&createResult, true);

    /* Attempt to update with an invalid event filter */
    filter.selectClausesSize = 0;
    asyncMonitoredItemModificationErrorExpected = true;
    modifyMonitoredItemAsync(&filter, true);

    /* Check if the client handle is unchanged and the notifications are still processed */
    ck_assert_uint_eq(oldClientHandle, mon->clientHandle);
    severityExpected = true;
    createTestEvent();
    checkForEvent(&createResult, true);

    /* Free event filter members */
    filter.selectClausesSize = 2;
    UA_EventFilter_clear(&filter);
    UA_MonitoredItemCreateResult_clear(&createResult);

    deleteMonitoredItems();
} END_TEST

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Server Subscription Event Filters");
    TCase *tc_server = tcase_create("Basic Event Filters");
    tcase_add_checked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, selectFilterValidation);
    tcase_add_test(tc_server, notOperatorValidation);
    tcase_add_test(tc_server, ofTypeOperatorValidation);
    tcase_add_test(tc_server, ofTypeOperatorValidation_failure);
    tcase_add_test(tc_server, orTypeOperatorValidation);
    tcase_add_test(tc_server, andTypeOperatorValidation);
    tcase_add_test(tc_server, bitwiseOperatorValidation);
    tcase_add_test(tc_server, equalOperatorValidation);
    tcase_add_test(tc_server, orderedCompareOperatorValidation);
    tcase_add_test(tc_server, betweenOperatorValidation);
    tcase_add_test(tc_server, inListOperatorValidation);
    tcase_add_test(tc_server, modifySelectFilterValidation);
    tcase_add_test(tc_server, modifySelectFilterSync);
    tcase_add_test(tc_server, modifySelectFilterAsync);
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
