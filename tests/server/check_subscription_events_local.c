/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>

#include "server/ua_server_internal.h"
#include "test_helpers.h"

static UA_Server *server;
static UA_NodeId eventType;
static unsigned callbackCount = 0;
static UA_Boolean modelChangeReceived = false;
static UA_ModelChangeStructureDataType expectedChanges[4];
static size_t expectedChangesSize = 0;
static UA_ModelChangeStructureDataType actualChanges[4];
static size_t actualChangesSize = 0;
static UA_Boolean actualChangesValid = false;
static unsigned modelChangeCount = 0;

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
modelChangeCallback(UA_Server *server, UA_UInt32 monitoredItemId,
                    void *monitoredItemContext,
                    const UA_KeyValueMap eventFields) {
    actualChangesValid = false;
    if(eventFields.mapSize != 1)
        return;
    UA_Variant *changes = &eventFields.map[0].value;
    if(changes->type != &UA_TYPES[UA_TYPES_MODELCHANGESTRUCTUREDATATYPE] ||
       changes->arrayLength > 4)
        return;
    actualChangesSize = changes->arrayLength;
    UA_ModelChangeStructureDataType *change =
        (UA_ModelChangeStructureDataType*)changes->data;
    for(size_t i = 0; i < actualChangesSize; i++)
        actualChanges[i] = change[i];
    actualChangesValid = true;
    modelChangeCount++;
    modelChangeReceived = true;
}

static void
expectModelChanges(size_t changesSize, const UA_NodeId *affected,
                   const UA_Byte *verbs) {
    ck_assert_uint_le(changesSize, 4);
    expectedChangesSize = changesSize;
    for(size_t i = 0; i < changesSize; i++) {
        UA_ModelChangeStructureDataType_init(&expectedChanges[i]);
        expectedChanges[i].affected = affected[i];
        expectedChanges[i].verb = verbs[i];
    }
    modelChangeReceived = false;
    actualChangesValid = false;
    actualChangesSize = 0;
}

static void
receiveExpectedModelChanges(void) {
    UA_Server_run_iterate(server, false);
    ck_assert(modelChangeReceived);
    ck_assert(actualChangesValid);
    ck_assert_uint_eq(actualChangesSize, expectedChangesSize);
    for(size_t i = 0; i < expectedChangesSize; i++) {
        ck_assert(UA_NodeId_equal(&actualChanges[i].affected,
                                  &expectedChanges[i].affected));
        ck_assert_uint_eq(actualChanges[i].verb, expectedChanges[i].verb);
    }
}

static void
createTestEvent(void) {
    UA_UInt16 severity = 100;
    UA_LocalizedText message = UA_LOCALIZEDTEXT("en-US", "Generated Event");
    UA_StatusCode res = UA_Server_createEvent(server, UA_NS0ID(SERVER), eventType,
                                              severity, message, NULL, NULL, NULL);
}

static void
createModelChangeMonitoredItem(void) {
    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClauses = (UA_SimpleAttributeOperand*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ck_assert_ptr_ne(ef.selectClauses, NULL);
    ef.selectClausesSize = 1;
    UA_StatusCode res = UA_SimpleAttributeOperand_parse(&ef.selectClauses[0],
                                                        UA_STRING("/Changes"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_MonitoredItemCreateResult mon =
        UA_Server_createEventMonitoredItem(server, UA_NS0ID(SERVER), ef, NULL,
                                           modelChangeCallback);
    ck_assert_uint_eq(mon.statusCode, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&ef);
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
addObject(const UA_NodeId nodeId, const char *name) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_StatusCode res = UA_Server_addObjectNode(
        server, nodeId, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name), UA_NS0ID(BASEOBJECTTYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
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

    createTestEvent();

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 1);

    createTestEvent();
    createTestEvent();

    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(callbackCount, 3);
} END_TEST

START_TEST(modelChangeFromLocalApi) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_NodeId affected = UA_NODEID_NUMERIC(1, 1234);
    UA_StatusCode res = UA_Server_addObjectNode(
        server, affected, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, "VersionedObject"), UA_NS0ID(BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId target = UA_NODEID_NUMERIC(1, 1236);
    res = UA_Server_addObjectNode(
        server, target, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, "TargetObject"), UA_NS0ID(BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_String initialVersion = UA_STRING("initial");
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.displayName = UA_LOCALIZEDTEXT("", "NodeVersion");
    vAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    vAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&vAttr.value, &initialVersion,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_NodeId versionProperty = UA_NODEID_NUMERIC(1, 1235);
    res = UA_Server_addVariableNode(
        server, versionProperty, affected, UA_NS0ID(HASPROPERTY),
        UA_QUALIFIEDNAME(0, "NodeVersion"), UA_NS0ID(PROPERTYTYPE),
        vAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_EventFilter ef;
    UA_EventFilter_init(&ef);
    ef.selectClauses = (UA_SimpleAttributeOperand*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    ck_assert_ptr_ne(ef.selectClauses, NULL);
    ef.selectClausesSize = 1;
    res = UA_SimpleAttributeOperand_parse(&ef.selectClauses[0],
                                          UA_STRING("/Changes"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateResult mon =
        UA_Server_createEventMonitoredItem(server, UA_NS0ID(SERVER), ef, NULL,
                                           modelChangeCallback);
    ck_assert_uint_eq(mon.statusCode, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&ef);

    UA_Byte verb = UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEADDED;
    expectModelChanges(1, &affected, &verb);
    res = UA_Server_addReference(server, affected, UA_NS0ID(HASCOMPONENT),
                                 UA_EXPANDEDNODEID_NUMERIC(1, 1236), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant version;
    UA_Variant_init(&version);
    res = UA_Server_readValue(server, versionProperty, &version);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&version, &UA_TYPES[UA_TYPES_STRING]));
    UA_String expectedVersion = UA_STRING("2");
    ck_assert(UA_String_equal((UA_String*)version.data, &expectedVersion));
    UA_Variant_clear(&version);

    receiveExpectedModelChanges();
} END_TEST

START_TEST(modelChangeReferenceDeleteAndNodeDelete) {
    UA_NodeId first = UA_NODEID_NUMERIC(1, 1240);
    UA_NodeId second = UA_NODEID_NUMERIC(1, 1241);
    UA_NodeId deleted = UA_NODEID_NUMERIC(1, 1242);
    addObject(first, "FirstVersionedObject");
    addObject(second, "SecondVersionedObject");
    addObject(deleted, "DeletedVersionedObject");
    addNodeVersion(first, UA_NODEID_NUMERIC(1, 1250));
    addNodeVersion(second, UA_NODEID_NUMERIC(1, 1251));
    addNodeVersion(deleted, UA_NODEID_NUMERIC(1, 1252));
    createModelChangeMonitoredItem();

    UA_NodeId affected[2] = {first, second};
    UA_Byte added[2] = {
        UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEADDED,
        UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEADDED
    };
    expectModelChanges(2, affected, added);
    UA_StatusCode res = UA_Server_addReference(
        server, first, UA_NS0ID(HASCOMPONENT),
        UA_EXPANDEDNODEID_NUMERIC(1, 1241), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    receiveExpectedModelChanges();

    UA_Byte removed[2] = {
        UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEDELETED,
        UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEDELETED
    };
    expectModelChanges(2, affected, removed);
    res = UA_Server_deleteReference(
        server, first, UA_NS0ID(HASCOMPONENT), true,
        UA_EXPANDEDNODEID_NUMERIC(1, 1241), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    receiveExpectedModelChanges();

    UA_Byte nodeDeleted =
        UA_MODELCHANGESTRUCTUREVERBMASK_NODEDELETED |
        UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEDELETED;
    expectModelChanges(1, &deleted, &nodeDeleted);
    res = UA_Server_deleteNode(server, deleted, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    receiveExpectedModelChanges();
} END_TEST

START_TEST(modelChangeNodeAdded) {
    UA_NodeId type = UA_NODEID_NUMERIC(1, 1280);
    UA_ObjectTypeAttributes typeAttr = UA_ObjectTypeAttributes_default;
    UA_StatusCode res = UA_Server_addObjectTypeNode(
        server, type, UA_NS0ID(BASEOBJECTTYPE), UA_NS0ID(HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "VersionedObjectType"), typeAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId typeVersion = UA_NODEID_NUMERIC(1, 1281);
    addNodeVersion(type, typeVersion);
    res = UA_Server_addReference(
        server, typeVersion, UA_NS0ID(HASMODELLINGRULE),
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    createModelChangeMonitoredItem();

    UA_NodeId instance = UA_NODEID_NUMERIC(1, 1282);
    UA_NodeId affected[2] = {type, instance};
    UA_Byte verbs[2] = {
        UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEADDED,
        UA_MODELCHANGESTRUCTUREVERBMASK_NODEADDED |
        UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEADDED
    };
    expectModelChanges(2, affected, verbs);
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    res = UA_Server_addObjectNode(
        server, instance, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, "VersionedInstance"), type, attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    receiveExpectedModelChanges();
} END_TEST

START_TEST(modelChangeDataTypeAttributes) {
    UA_NodeId variable = UA_NODEID_NUMERIC(1, 1260);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", "VersionedVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.valueRank = UA_VALUERANK_ANY;
    UA_Variant_setArray(&attr.value, UA_EMPTY_ARRAY_SENTINEL, 0,
                        &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = UA_Server_addVariableNode(
        server, variable, UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
        UA_QUALIFIEDNAME(1, "VersionedVariable"), UA_NS0ID(BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    addNodeVersion(variable, UA_NODEID_NUMERIC(1, 1261));
    createModelChangeMonitoredItem();

    UA_Byte verb = UA_MODELCHANGESTRUCTUREVERBMASK_DATATYPECHANGED;
    expectModelChanges(1, &variable, &verb);
    res = UA_Server_writeDataType(server, variable, UA_NS0ID(NUMBER));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    receiveExpectedModelChanges();

    expectModelChanges(1, &variable, &verb);
    res = UA_Server_writeValueRank(server, variable,
                                   UA_VALUERANK_ONE_DIMENSION);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    receiveExpectedModelChanges();

    UA_UInt32 dimension = 0;
    UA_Variant dimensions;
    UA_Variant_setArray(&dimensions, &dimension, 1,
                        &UA_TYPES[UA_TYPES_UINT32]);
    expectModelChanges(1, &variable, &verb);
    res = UA_Server_writeArrayDimensions(server, variable, dimensions);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    receiveExpectedModelChanges();
} END_TEST

START_TEST(modelChangeFailureAndSuppression) {
    UA_NodeId first = UA_NODEID_NUMERIC(1, 1270);
    UA_NodeId second = UA_NODEID_NUMERIC(1, 1271);
    addObject(first, "FailureFirst");
    addObject(second, "FailureSecond");
    addNodeVersion(first, UA_NODEID_NUMERIC(1, 1272));
    createModelChangeMonitoredItem();

    UA_StatusCode res = UA_Server_addReference(
        server, first, UA_NS0ID(HASCOMPONENT),
        UA_EXPANDEDNODEID_NUMERIC(1, 1271), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    unsigned before = modelChangeCount;

    /* Adding the existing reference changes nothing. */
    res = UA_Server_addReference(
        server, first, UA_NS0ID(HASCOMPONENT),
        UA_EXPANDEDNODEID_NUMERIC(1, 1271), true);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(modelChangeCount, before);

    /* Internal cleanup uses the suppression counter. */
    lockServer(server);
    beginModelChange(server);
    res = deleteNode(server, first, true);
    endModelChange(server);
    unlockServer(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(modelChangeCount, before);
} END_TEST

START_TEST(modelChangeUmbrellaCoalescing) {
    UA_NodeId source = UA_NODEID_NUMERIC(1, 1290);
    UA_NodeId first = UA_NODEID_NUMERIC(1, 1291);
    UA_NodeId second = UA_NODEID_NUMERIC(1, 1292);
    UA_NodeId versionId = UA_NODEID_NUMERIC(1, 1293);
    addObject(source, "UmbrellaSource");
    addObject(first, "UmbrellaFirst");
    addObject(second, "UmbrellaSecond");
    addNodeVersion(source, versionId);
    createModelChangeMonitoredItem();

    UA_Int64 versionBefore = server->nodeVersionCounter;
    UA_Byte verb =
        UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEADDED |
        UA_MODELCHANGESTRUCTUREVERBMASK_REFERENCEDELETED;
    expectModelChanges(1, &source, &verb);

    lockServer(server);
    beginModelChange(server); /* Equivalent to the decoded-request umbrella. */
    unlockServer(server);
    UA_StatusCode res = UA_Server_addReference(
        server, source, UA_NS0ID(HASCOMPONENT),
        UA_EXPANDEDNODEID_NUMERIC(1, 1291), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_addReference(
        server, source, UA_NS0ID(HASCOMPONENT),
        UA_EXPANDEDNODEID_NUMERIC(1, 1292), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_deleteReference(
        server, source, UA_NS0ID(HASCOMPONENT), true,
        UA_EXPANDEDNODEID_NUMERIC(1, 1291), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    lockServer(server);
    endModelChange(server);
    ck_assert_uint_eq(server->modelChangeDepth, 0);
    unlockServer(server);

    receiveExpectedModelChanges();
    ck_assert_int_eq(server->nodeVersionCounter, versionBefore + 1);
} END_TEST

START_TEST(modelChangeStartupSuppressed) {
    ck_assert_uint_eq(server->modelChangeSuppressionDepth, 0);
    ck_assert_uint_eq(server->modelChangeDepth, 0);
    ck_assert_uint_eq(server->modelChanges.changesSize, 0);
} END_TEST

static Suite *testSuite_event(void) {
    Suite *s = suite_create("Server Local Subscription Events");
    TCase *tc_server = tcase_create("Server Local Subscription Events");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, generateEvents);
    tcase_add_test(tc_server, modelChangeFromLocalApi);
    tcase_add_test(tc_server, modelChangeReferenceDeleteAndNodeDelete);
    tcase_add_test(tc_server, modelChangeNodeAdded);
    tcase_add_test(tc_server, modelChangeDataTypeAttributes);
    tcase_add_test(tc_server, modelChangeFailureAndSuppression);
    tcase_add_test(tc_server, modelChangeUmbrellaCoalescing);
    tcase_add_test(tc_server, modelChangeStartupSuppressed);
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
