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
eventSetup(UA_NodeId *eventNodeId) {
    UA_Server_createEvent(server, eventType, eventNodeId);
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
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    // number with no special meaning
    UA_UInt16 eventSeverity = 1000;
    UA_Variant_setScalar(&value, &eventSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_clear(&bpr);

    //add a message to the event
    rpe.targetName = UA_QUALIFIEDNAME(0, "Message");
    bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_clear(&bpr);
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

    UA_NodeId eventNodeId;
    eventSetup(&eventNodeId);

    UA_Server_triggerEvent(server, eventNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), NULL, false);

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);

    UA_Server_triggerEvent(server, eventNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), NULL, false);
    UA_Server_triggerEvent(server, eventNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), NULL, false);

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 3);
} END_TEST

static Suite *testSuite_event(void) {
    Suite *s = suite_create("Server Local Subscription Events");
    TCase *tc_server = tcase_create("Server Local Subscription Events");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, generateEvents);
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
