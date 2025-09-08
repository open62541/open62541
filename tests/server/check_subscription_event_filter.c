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

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>

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
setupSelectClauses(void) {
    /* Check for severity (set manually), message (set manually), eventType
     * (automatic) and sourceNode (automatic) */
    selectClauses = (UA_SimpleAttributeOperand *)
        UA_Array_new(defaultSlectClauseSize, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ck_assert_ptr_ne(selectClauses, NULL);
    for(size_t i = 0; i < defaultSlectClauseSize; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
        selectClauses[i].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        selectClauses[i].browsePathSize = 1;
        selectClauses[i].attributeId = UA_ATTRIBUTEID_VALUE;
        selectClauses[i].browsePath = (UA_QualifiedName *)
            UA_Array_new(selectClauses[i].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        ck_assert_ptr_ne(selectClauses[i].browsePath, NULL);
    }
    selectClauses[0].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Severity");
    selectClauses[1].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "Message");
    selectClauses[2].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "EventType");
    selectClauses[3].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "SourceNode");
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
    ck_assert_uint_eq(eventFields.mapSize, defaultSlectClauseSize);

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

THREAD_CALLBACK(serverloop) {
    while (running) {
        UA_Server_run_iterate(server, true);
        serverIterations++;
    }
    return 0;
}

static void
sleepUntilAnswer(UA_Double sleepMs) {
    UA_fakeSleep((UA_UInt32)sleepMs);
    size_t oldIterations = serverIterations;
    size_t newIterations;
    while(true) {
        newIterations = serverIterations;
        if(oldIterations != newIterations)
            return;
        UA_realSleep(1);
    }
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
createTestEvent(void) {
    UA_UInt16 severity = 100;
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_StatusCode res = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType,
                                              severity, message, NULL, NULL);
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

/*  helper functions for the generation of the content-filter */
static void
setupContentFilter(UA_ContentFilter *contentFilter, size_t elements){
    UA_ContentFilter_init(contentFilter);
    contentFilter->elementsSize = elements;
    contentFilter->elements  = (UA_ContentFilterElement *)
        UA_Array_new(contentFilter->elementsSize, &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENT]);
    ck_assert_ptr_ne(contentFilter->elements, NULL);
    for(size_t i = 0; i < contentFilter->elementsSize; ++i) {
        UA_ContentFilterElement_init(&contentFilter->elements[i]);
    }
}

static void
setupOperandArrays(UA_ContentFilterElement *contentFilterElement){
    contentFilterElement->filterOperands = (UA_ExtensionObject*)
        UA_Array_new(contentFilterElement->filterOperandsSize, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert_ptr_ne(contentFilterElement->filterOperands, NULL);
    for(size_t n =0; n< contentFilterElement->filterOperandsSize; ++n) {
        UA_ExtensionObject_init(&contentFilterElement->filterOperands[n]);
    }
}

static void
setupNotFilter(UA_ContentFilterElement *element){
    element->filterOperator = UA_FILTEROPERATOR_NOT;
    element->filterOperandsSize = 1;
    setupOperandArrays(element);
}

static void
setupOfTypeFilter(UA_ContentFilterElement *element){
    element->filterOperator = UA_FILTEROPERATOR_OFTYPE;
    element->filterOperandsSize = 1;
    setupOperandArrays(element);
}

static void
setupEqualsFilter(UA_ContentFilterElement *element, UA_FilterOperator compareOperator){
    switch(compareOperator) {
    case UA_FILTEROPERATOR_EQUALS:
        element->filterOperator = UA_FILTEROPERATOR_EQUALS;
        break;
    case UA_FILTEROPERATOR_LESSTHAN:
        element->filterOperator = UA_FILTEROPERATOR_LESSTHAN;
        break;
    case UA_FILTEROPERATOR_GREATERTHAN:
        element->filterOperator = UA_FILTEROPERATOR_GREATERTHAN;
        break;
    default:
        element->filterOperator = UA_FILTEROPERATOR_EQUALS;
        break;
    }
    element->filterOperandsSize = 2;
    setupOperandArrays(element);
}

static void
setupBetweenFilter(UA_ContentFilterElement *element){
    element->filterOperator = UA_FILTEROPERATOR_BETWEEN;
    element->filterOperandsSize = 3;
    setupOperandArrays(element);
}

static void
setupInListFilter(UA_ContentFilterElement *element, UA_UInt16 elements){
    element->filterOperator = UA_FILTEROPERATOR_INLIST;
    element->filterOperandsSize = elements;
    setupOperandArrays(element);
}

/*static void
setupElementOperand(UA_ContentFilterElement *element, size_t count, UA_UInt32 *indexes){
    for(size_t i = 0; i < count; ++i) {
        element->filterOperands[i].content.decoded.type = &UA_TYPES[UA_TYPES_ELEMENTOPERAND];
        element->filterOperands[i].encoding = UA_EXTENSIONOBJECT_DECODED;
        UA_ElementOperand *firstElementOperand = UA_ElementOperand_new();
        UA_ElementOperand_init(firstElementOperand);
        firstElementOperand->index = indexes[i];
        element->filterOperands[i].content.decoded.data = firstElementOperand;
    }
}*/

static void
setupLiteralOperand(UA_ContentFilterElement *element, size_t count, UA_Variant *literals){
    for(size_t i = 0; i < count; ++i) {
        element->filterOperands[i].content.decoded.type = &UA_TYPES[UA_TYPES_LITERALOPERAND];
        element->filterOperands[i].encoding = UA_EXTENSIONOBJECT_DECODED;
        UA_LiteralOperand *literalOperand = UA_LiteralOperand_new();
        UA_LiteralOperand_init(literalOperand);
        literalOperand->value = literals[i];
        element->filterOperands[i].content.decoded.data = literalOperand;
    }
}

START_TEST(selectFilterValidation) {
    /* setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.whereClause.elementsSize = 0;
    filter.whereClause.elements = NULL;
    filter.selectClauses = UA_SimpleAttributeOperand_new();
    filter.selectClausesSize = 1;
    UA_SimpleAttributeOperand_init(filter.selectClauses);
    filter.selectClauses->typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    filter.selectClauses->browsePathSize = 1;
    filter.selectClauses->browsePath = (UA_QualifiedName*)
            UA_Array_new(filter.selectClauses->browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    filter.selectClauses->attributeId = UA_ATTRIBUTEID_VALUE;

    filter.selectClauses->browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "FOOBAR");
    UA_MonitoredItemCreateResult createResult;
    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    ck_assert_int_eq(createResult.filterResult.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert(createResult.filterResult.content.decoded.type == &UA_TYPES[UA_TYPES_EVENTFILTERRESULT]);
    UA_EventFilterResult *eventFilterResult = (UA_EventFilterResult *)createResult.filterResult.content.decoded.data;
    ck_assert_uint_eq(eventFilterResult->selectClauseResultsSize, 1);
    ck_assert_uint_eq(eventFilterResult->selectClauseResults[0], UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_QualifiedName_clear(&filter.selectClauses->browsePath[0]);
    UA_MonitoredItemCreateResult_clear(&createResult);

    filter.selectClauses->browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, "xx");
    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_BADEVENTFILTERINVALID);
    ck_assert_int_eq(createResult.filterResult.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert(createResult.filterResult.content.decoded.type == &UA_TYPES[UA_TYPES_EVENTFILTERRESULT]);
    eventFilterResult = (UA_EventFilterResult *)createResult.filterResult.content.decoded.data;
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
    /* setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    setupSelectClauses();
    filter.selectClauses = selectClauses;
    filter.selectClausesSize = defaultSlectClauseSize;
    setupContentFilter(&filter.whereClause, 1);
    setupNotFilter(&filter.whereClause.elements[0]);
    UA_Boolean condition = true;
    UA_Variant literalContent;
    UA_Variant_init(&literalContent);
    UA_Variant_setScalar(&literalContent, &condition, &UA_TYPES[UA_TYPES_BOOLEAN]);
    setupLiteralOperand(&filter.whereClause.elements[0], 1, &literalContent);

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
    UA_free(filter.whereClause.elements[0].filterOperands->content.decoded.data);

    condition = false;
    UA_Variant_setScalarCopy(&literalContent, &condition, &UA_TYPES[UA_TYPES_BOOLEAN]);
    setupLiteralOperand(&filter.whereClause.elements[0], 1, &literalContent);
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
    /* setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    setupSelectClauses();
    filter.selectClauses = selectClauses;
    filter.selectClausesSize = defaultSlectClauseSize;
    setupContentFilter(&filter.whereClause, 1);
    setupOfTypeFilter(&filter.whereClause.elements[0]);
    UA_Variant literalContent;
    UA_NodeId *nodeId = UA_NodeId_new();
    UA_NodeId_init(nodeId);
    *nodeId = EventType_B_Layer_1;
    UA_Variant_setScalar(&literalContent, nodeId, &UA_TYPES[UA_TYPES_NODEID]);
    setupLiteralOperand(&filter.whereClause.elements[0], 1, &literalContent);

    /* setup event */
    eventType = EventType_A_Layer_1;

    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /*  trigger the event */
    createTestEvent();

    /* trigger the event */
    eventType = EventType_B_Layer_1;
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(ofTypeOperatorValidation_failure) {
    /* setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    setupSelectClauses();
    filter.selectClauses = selectClauses;
    filter.selectClausesSize = defaultSlectClauseSize;
    setupContentFilter(&filter.whereClause, 1);
    setupOfTypeFilter(&filter.whereClause.elements[0]);

    UA_Variant literalContent;
    UA_NodeId *nodeId = UA_NodeId_new();
    UA_NodeId_init(nodeId);
    *nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE);
    UA_Variant_setScalar(&literalContent, nodeId, &UA_TYPES[UA_TYPES_NODEID]);
    setupLiteralOperand(&filter.whereClause.elements[0], 1, &literalContent);

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

} END_TEST

START_TEST(andTypeOperatorValidation) {

} END_TEST

START_TEST(equalOperatorValidation) {
    /*  setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    setupSelectClauses();
    filter.selectClauses = selectClauses;
    filter.selectClausesSize = defaultSlectClauseSize;
    setupContentFilter(&filter.whereClause, 1);
    setupEqualsFilter(&filter.whereClause.elements[0], UA_FILTEROPERATOR_EQUALS);
    /*  setup operands */
    UA_UInt32 left = 62541;
    UA_UInt32 right = 62541;
    UA_Variant literalContent[2];
    memset(literalContent, 0, sizeof(UA_Variant) * 2);
    UA_Variant_setScalar(&literalContent[0], &left, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&literalContent[1], &right, &UA_TYPES[UA_TYPES_UINT32]);
    setupLiteralOperand(&filter.whereClause.elements[0], 2, literalContent);

    /*  setup event */
    eventType = EventType_A_Layer_1;

    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /*  trigger the event */
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_free(filter.whereClause.elements->filterOperands[0].content.decoded.data);
    UA_free(filter.whereClause.elements->filterOperands[1].content.decoded.data);

    left = 62542;
    UA_Variant_setScalar(&literalContent[0], &left, &UA_TYPES[UA_TYPES_UINT32]);
    setupLiteralOperand(&filter.whereClause.elements[0], 2, literalContent);
    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /*  trigger the event */
    createTestEvent();
    checkForEvent(&createResult, false);
    deleteMonitoredItems();
    UA_free(filter.whereClause.elements->filterOperands[0].content.decoded.data);
    UA_free(filter.whereClause.elements->filterOperands[1].content.decoded.data);

    /* test types wich need implicit cast */
    UA_UInt64 left_big = 62541;
    UA_Variant_setScalar(&literalContent[0], &left_big, &UA_TYPES[UA_TYPES_UINT64]);
    setupLiteralOperand(&filter.whereClause.elements[0], 2, literalContent);
    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /*  trigger the event */
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_free(filter.whereClause.elements->filterOperands[0].content.decoded.data);
    UA_free(filter.whereClause.elements->filterOperands[1].content.decoded.data);

    /*  check equal with nodeid */
    UA_NodeId left_nodeid = UA_NODEID_NUMERIC(0, 14123);
    UA_NodeId right_nodeid = UA_NODEID_NUMERIC(0, 14123);
    memset(literalContent, 0, sizeof(UA_Variant) * 2);
    UA_Variant_setScalarCopy(&literalContent[0], &left_nodeid, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Variant_setScalarCopy(&literalContent[1], &right_nodeid, &UA_TYPES[UA_TYPES_NODEID]);
    setupLiteralOperand(&filter.whereClause.elements[0], 2, literalContent);
    /*  setup event */
    eventType = EventType_A_Layer_1;
    /*  add a monitored item (with filter) */
    createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /*  trigger the event */
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(orderedCompareOperatorValidation) {
    /*  setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    setupSelectClauses();
    filter.selectClauses = selectClauses;
    filter.selectClausesSize = defaultSlectClauseSize;
    setupContentFilter(&filter.whereClause, 1);
    setupEqualsFilter(&filter.whereClause.elements[0], UA_FILTEROPERATOR_LESSTHAN);
    /*  setup operands */
    UA_UInt32 left = 100;
    UA_UInt32 right = 1000;
    UA_Variant literalContent[2];
    memset(literalContent, 0, sizeof(UA_Variant) * 2);
    UA_Variant_setScalarCopy(&literalContent[0], &left, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalarCopy(&literalContent[1], &right, &UA_TYPES[UA_TYPES_UINT32]);
    setupLiteralOperand(&filter.whereClause.elements[0], 2, literalContent);
    /*  setup event */
    eventType = EventType_A_Layer_1;
    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /*  trigger the event */
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(betweenOperatorValidation) {
    /*  setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    setupSelectClauses();
    filter.selectClauses = selectClauses;
    filter.selectClausesSize = defaultSlectClauseSize;
    setupContentFilter(&filter.whereClause, 1);
    setupBetweenFilter(&filter.whereClause.elements[0]);
    /*  setup operands */
    UA_UInt32 range_element = 40;
    UA_UInt32 range_start = 10;
    UA_UInt32 range_stop = 100;
    UA_Variant literalContent[3];
    memset(literalContent, 0, sizeof(UA_Variant) * 3);
    UA_Variant_setScalarCopy(&literalContent[0], &range_element, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalarCopy(&literalContent[1], &range_start, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalarCopy(&literalContent[2], &range_stop, &UA_TYPES[UA_TYPES_UINT32]);
    setupLiteralOperand(&filter.whereClause.elements[0], 3, literalContent);
    /*  setup event */
    eventType = EventType_A_Layer_1;
    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /*  trigger the event */
    createTestEvent();
    checkForEvent(&createResult, true);
    deleteMonitoredItems();
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(inListOperatorValidation) {
    /*  setup event filter */
    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    setupSelectClauses();
    filter.selectClauses = selectClauses;
    filter.selectClausesSize = defaultSlectClauseSize;
    setupContentFilter(&filter.whereClause, 1);
    setupInListFilter(&filter.whereClause.elements[0], 4);
    /*  setup operands */
    UA_UInt32 target_element = 40;
    UA_UInt32 element_1 = 10;
    UA_UInt32 element_2 = 100;
    UA_UInt32 element_3 = 40;
    UA_Variant literalContent[4];
    memset(literalContent, 0, sizeof(UA_Variant) * 4);
    UA_Variant_setScalarCopy(&literalContent[0], &target_element, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalarCopy(&literalContent[1], &element_1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalarCopy(&literalContent[2], &element_2, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalarCopy(&literalContent[3], &element_3, &UA_TYPES[UA_TYPES_INT32]);
    setupLiteralOperand(&filter.whereClause.elements[0], 4, literalContent);
    /*  setup event */
    eventType = EventType_A_Layer_1;
    /*  add a monitored item (with filter) */
    UA_MonitoredItemCreateResult createResult = addMonitoredItem(handler_events_simple, &filter, true);
    ck_assert_uint_eq(createResult.statusCode, UA_STATUSCODE_GOOD);
    monitoredItemId = createResult.monitoredItemId;

    /*  trigger the event */
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
    tcase_add_test(tc_server, equalOperatorValidation);
    tcase_add_test(tc_server, orderedCompareOperatorValidation);
    tcase_add_test(tc_server, betweenOperatorValidation);
    tcase_add_test(tc_server, inListOperatorValidation);
    tcase_add_test(tc_server, modifySelectFilterValidation);
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
