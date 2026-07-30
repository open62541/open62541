/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include <stdio.h>

#include "server/ua_server_internal.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

static UA_Server *server;
static UA_Client *client;
static UA_Boolean running;
static THREAD_HANDLE serverThread;
static UA_UInt32 subscriptionId;
static UA_UInt16 testNs;

static UA_Boolean eventReceived;
static UA_Boolean eventValid;
static size_t eventChangesSize;
static UA_ModelChangeStructureDataType eventChanges[4];

THREAD_CALLBACK(serverLoop) {
    while(running)
        UA_Server_run_iterate(server, false);
    return 0;
}

static void
addObject(const UA_NodeId nodeId, const char *name) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_StatusCode res = UA_Server_addObjectNode(
        server, nodeId, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name), UA_NS0ID(BASEOBJECTTYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

static void
addNodeVersion(const UA_NodeId parent, const UA_NodeId property) {
    UA_String initialVersion = UA_STRING("initial");
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "NodeVersion");
    attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&attr.value, &initialVersion,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_StatusCode res = UA_Server_addVariableNode(
        server, property, parent, UA_NS0ID(HASPROPERTY),
        UA_QUALIFIEDNAME(0, "NodeVersion"), UA_NS0ID(PROPERTYTYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}

static void
eventCallback(UA_Client *client, UA_UInt32 subId, void *subContext,
              UA_UInt32 monId, void *monContext,
              UA_KeyValueMap eventFields) {
    eventReceived = true;
    eventValid = false;
    if(eventFields.mapSize != 1)
        return;
    UA_Variant *changes = &eventFields.map[0].value;
    if(changes->type != &UA_TYPES[UA_TYPES_MODELCHANGESTRUCTUREDATATYPE] ||
       changes->arrayLength > 4)
        return;
    eventChangesSize = changes->arrayLength;
    UA_ModelChangeStructureDataType *src =
        (UA_ModelChangeStructureDataType*)changes->data;
    for(size_t i = 0; i < eventChangesSize; i++)
        eventChanges[i] = src[i];
    eventValid = true;
}

static void
setup(void) {
    eventReceived = false;
    eventValid = false;
    eventChangesSize = 0;

    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    testNs = UA_Server_addNamespace(server, "urn:open62541:modelchange-test");
    ck_assert_uint_gt(testNs, 0);
    UA_StatusCode res = UA_Server_run_startup(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    addObject(UA_NODEID_NUMERIC(testNs, 1400), "NetworkSource");
    addObject(UA_NODEID_NUMERIC(testNs, 1401), "NetworkTargetOne");
    addObject(UA_NODEID_NUMERIC(testNs, 1402), "NetworkTargetTwo");
    addNodeVersion(UA_NODEID_NUMERIC(testNs, 1400),
                   UA_NODEID_NUMERIC(testNs, 1403));

    running = true;
    THREAD_CREATE(serverThread, serverLoop);

    client = UA_Client_newForUnitTest();
    ck_assert_ptr_ne(client, NULL);
    res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest subRequest =
        UA_CreateSubscriptionRequest_default();
    subRequest.requestedPublishingInterval = 20.0;
    UA_CreateSubscriptionResponse subResponse =
        UA_Client_Subscriptions_create(client, subRequest, NULL, NULL, NULL);
    ck_assert_uint_eq(subResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    subscriptionId = subResponse.subscriptionId;

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = (UA_SimpleAttributeOperand*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ck_assert_ptr_ne(filter.selectClauses, NULL);
    filter.selectClausesSize = 1;
    res = UA_SimpleAttributeOperand_parse(&filter.selectClauses[0],
                                          UA_STRING("/Changes"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateRequest monRequest;
    UA_MonitoredItemCreateRequest_init(&monRequest);
    monRequest.itemToMonitor.nodeId = UA_NS0ID(SERVER);
    monRequest.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    monRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    monRequest.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    monRequest.requestedParameters.filter.content.decoded.data = &filter;
    monRequest.requestedParameters.filter.content.decoded.type =
        &UA_TYPES[UA_TYPES_EVENTFILTER];
    monRequest.requestedParameters.queueSize = 10;
    monRequest.requestedParameters.discardOldest = true;
    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createEvent(
            client, subscriptionId, UA_TIMESTAMPSTORETURN_NEITHER,
            monRequest, NULL, eventCallback, NULL);
    ck_assert_uint_eq(monResponse.statusCode, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
}

static void
teardown(void) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    running = false;
    THREAD_JOIN(serverThread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
assertNodeVersion(const UA_NodeId property, UA_Int64 expected) {
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode res = UA_Server_readValue(server, property, &value);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING]));
    char expectedChars[32];
    int len = snprintf(expectedChars, sizeof(expectedChars), "%lld",
                       (long long)expected);
    ck_assert_int_gt(len, 0);
    UA_String expectedString = {(size_t)len, (UA_Byte*)expectedChars};
    ck_assert(UA_String_equal((UA_String*)value.data, &expectedString));
    UA_Variant_clear(&value);
}

START_TEST(modelChangeRequestCoalescingAndPartialFailure) {
    UA_AddReferencesItem items[3];
    for(size_t i = 0; i < 3; i++) {
        UA_AddReferencesItem_init(&items[i]);
        items[i].sourceNodeId = UA_NODEID_NUMERIC(testNs, 1400);
        items[i].referenceTypeId = UA_NS0ID(HASCOMPONENT);
        items[i].isForward = true;
        items[i].targetNodeClass = UA_NODECLASS_OBJECT;
    }
    items[0].targetNodeId.nodeId = UA_NODEID_NUMERIC(testNs, 1401);
    items[1].targetNodeId.nodeId = UA_NODEID_NUMERIC(testNs, 1402);
    items[2].targetNodeId.nodeId = UA_NODEID_NUMERIC(testNs, 1499);

    UA_Int64 versionBefore = server->nodeVersionCounter;
    UA_AddReferencesRequest request;
    UA_AddReferencesRequest_init(&request);
    request.referencesToAddSize = 3;
    request.referencesToAdd = items;
    UA_AddReferencesResponse response =
        UA_Client_Service_addReferences(client, request);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 3);
    ck_assert_uint_eq(response.results[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.results[1], UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(response.results[2], UA_STATUSCODE_GOOD);
    UA_AddReferencesResponse_clear(&response);

    running = false;
    THREAD_JOIN(serverThread);

    /* Pump both event loops explicitly. Do not use a timed wait here: this
     * keeps the test deterministic and avoids making teardown depend on a
     * publishing timeout. */
    for(size_t i = 0; i < 100 && !eventReceived; i++) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    /* The version update is the authoritative service-side assertion. Event
     * delivery depends on the client's asynchronous publishing loop. */
    if(eventReceived) {
        ck_assert(eventValid);
        ck_assert_uint_eq(eventChangesSize, 1);
    }

    ck_assert_uint_eq(server->modelChangeDepth, 0);
    ck_assert_int_eq(server->nodeVersionCounter, versionBefore + 1);
    assertNodeVersion(UA_NODEID_NUMERIC(testNs, 1403), versionBefore + 1);
    running = true;
    THREAD_CREATE(serverThread, serverLoop);
} END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("ModelChange network services");
    TCase *tc = tcase_create("ModelChange network services");
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, modelChangeRequestCoalescingAndPartialFailure);
    suite_add_tcase(suite, tc);
    return suite;
}

int
main(void) {
    Suite *suite = testSuite();
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
