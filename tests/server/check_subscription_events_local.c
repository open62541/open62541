/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>

#include "test_helpers.h"

static UA_Server *server;
static UA_NodeId eventType;
static unsigned callbackCount = 0;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    UA_Server_run_startup(server);

    /* Add new Event Type */
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

    UA_Server_run_startup(server);
}

static void
teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
eventCallback(UA_Server *server, UA_UInt32 monitoredItemId,
              void *monitoredItemContext, const UA_KeyValueMap eventFields) {
    callbackCount++;
}

static void
createEvent(void) {
    UA_UInt16 severity = 100;
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_StatusCode res = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType,
                                              severity, message, NULL, NULL, NULL);
}

/* Ensure events are received with proper values */
START_TEST(generateEvents) {
    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClauses = (UA_SimpleAttributeOperand *)
            UA_Array_new(4, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ef.selectClausesSize = 4;

    /* BaseEventType i=2041 is used as the default in _parse */
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[0], UA_STRING("/Severity"));
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[1], UA_STRING("/Message"));
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[2], UA_STRING("/EventType"));
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[3], UA_STRING("/SourceNode"));

    UA_MonitoredItemCreateResult res =
        UA_Server_createEventMonitoredItem(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
                                           ef, NULL, eventCallback);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&ef);

    createEvent();

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);

    createEvent();
    createEvent();

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 3);
} END_TEST

/* ==== UA_Server_createEventMonitoredItemEx input-validation guards ==== */

START_TEST(Server_createEventMonitoredItemEx_wrongAttribute) {
    /* src/server/ua_services_monitoreditem.c:701-706:
     *   if(item.itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER) {
     *     result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
     *     return result;
     *   }
     * The createEventMonitoredItemEx creator requires the
     * EventNotifier attribute. Using anything else (e.g. VALUE) is
     * a mis-use and is rejected. */
    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClauses = (UA_SimpleAttributeOperand *)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ef.selectClausesSize = 1;
    UA_SimpleAttributeOperand_parse(&ef.selectClauses[0], UA_STRING("/Severity"));

    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_VALUE; /* wrong */
    request.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
    request.requestedParameters.filter.content.decoded.data = &ef;
    request.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_MonitoredItemCreateResult res =
        UA_Server_createEventMonitoredItemEx(server, request, NULL, eventCallback);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(res.monitoredItemId, 0);

    UA_Array_delete(ef.selectClauses, 1, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
} END_TEST

START_TEST(Server_createEventMonitoredItemEx_wrongFilterType) {
    /* src/server/ua_services_monitoreditem.c:708-716:
     *   if((filter->encoding != DECODED && ... != DECODED_NODELETE) ||
     *      filter->content.decoded.type != &UA_TYPES[EVENTFILTER]) {
     *     result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
     *   }
     * A non-EventFilter filter (e.g. an AFilter of a different
     * type, or a NONE-encoded ExtensionObject) is rejected. */
    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    request.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_ENCODED_NOBODY; /* not decoded */
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_MonitoredItemCreateResult res =
        UA_Server_createEventMonitoredItemEx(server, request, NULL, eventCallback);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(res.monitoredItemId, 0);
} END_TEST

START_TEST(Server_createEventMonitoredItemEx_noSelectClauses) {
    /* src/server/ua_services_monitoreditem.c:719-724:
     *   if(ef->selectClausesSize == 0) {
     *     result.statusCode = UA_STATUSCODE_BADINTERNALERROR;
     *   }
     * A valid EventFilter with zero select clauses is rejected. */
    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClausesSize = 0;
    ef.selectClauses = NULL;

    UA_MonitoredItemCreateRequest request;
    UA_MonitoredItemCreateRequest_init(&request);
    request.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    request.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    request.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
    request.requestedParameters.filter.content.decoded.data = &ef;
    request.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    request.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_MonitoredItemCreateResult res =
        UA_Server_createEventMonitoredItemEx(server, request, NULL, eventCallback);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(res.monitoredItemId, 0);
} END_TEST

static Suite *testSuite_event(void) {
    Suite *s = suite_create("Server Local Subscription Events");
    TCase *tc_server = tcase_create("Server Local Subscription Events");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, generateEvents);
    tcase_add_test(tc_server, Server_createEventMonitoredItemEx_wrongAttribute);
    tcase_add_test(tc_server, Server_createEventMonitoredItemEx_wrongFilterType);
    tcase_add_test(tc_server, Server_createEventMonitoredItemEx_noSelectClauses);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(void) {
    Suite *s = testSuite_event();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
