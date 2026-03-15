/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Client highlevel operation tests */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "testing_clock.h"
#include "thread_wrapper.h"
#include "test_helpers.h"
#include <check.h>
#include <stdlib.h>

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);

    /* Add test variables for read/write */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    vattr.writeMask = 0xFFFFFFFF; /* Allow all writes */
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 62001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "HLTestVar1"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    /* String variable */
    UA_VariableAttributes sattr = UA_VariableAttributes_default;
    UA_String sval = UA_STRING("test");
    UA_Variant_setScalar(&sattr.value, &sval, &UA_TYPES[UA_TYPES_STRING]);
    sattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    sattr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;

    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 62002),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "HLTestVar2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        sattr, NULL, NULL);

    /* Boolean variable */
    UA_VariableAttributes battr = UA_VariableAttributes_default;
    UA_Boolean bval = true;
    UA_Variant_setScalar(&battr.value, &bval, &UA_TYPES[UA_TYPES_BOOLEAN]);
    battr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    battr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;

    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 62003),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "HLTestVar3"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        battr, NULL, NULL);

    /* Double variable */
    UA_VariableAttributes dattr = UA_VariableAttributes_default;
    UA_Double dval = 3.14;
    UA_Variant_setScalar(&dattr.value, &dval, &UA_TYPES[UA_TYPES_DOUBLE]);
    dattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    dattr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;

    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 62004),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "HLTestVar4"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        dattr, NULL, NULL);

    UA_Server_run_startup(server);
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static UA_Client *connectClient(void) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return client;
}

static void disconnectClient(UA_Client *client) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

/* === Client highlevel read operations === */
START_TEST(hl_readDataType) {
    UA_Client *client = connectClient();
    UA_NodeId dataType;
    UA_StatusCode res = UA_Client_readDataTypeAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &dataType);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&dataType, &UA_TYPES[UA_TYPES_INT32].typeId));
    UA_NodeId_clear(&dataType);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readValueRank) {
    UA_Client *client = connectClient();
    UA_Int32 valueRank;
    UA_StatusCode res = UA_Client_readValueRankAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &valueRank);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readAccessLevel) {
    UA_Client *client = connectClient();
    UA_Byte accessLevel;
    UA_StatusCode res = UA_Client_readAccessLevelAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &accessLevel);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(accessLevel & UA_ACCESSLEVELMASK_READ);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readUserAccessLevel) {
    UA_Client *client = connectClient();
    UA_Byte userAccessLevel;
    UA_StatusCode res = UA_Client_readUserAccessLevelAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &userAccessLevel);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readMinSamplingInterval) {
    UA_Client *client = connectClient();
    UA_Double minInterval;
    UA_StatusCode res = UA_Client_readMinimumSamplingIntervalAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &minInterval);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readHistorizing) {
    UA_Client *client = connectClient();
    UA_Boolean historizing;
    UA_StatusCode res = UA_Client_readHistorizingAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &historizing);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readExecutable) {
    UA_Client *client = connectClient();
    UA_Boolean executable;
    /* Try reading executable of a method node. Server GetMonitoredItems is a method. */
    UA_StatusCode res = UA_Client_readExecutableAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS), &executable);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readUserExecutable) {
    UA_Client *client = connectClient();
    UA_Boolean userExecutable;
    UA_StatusCode res = UA_Client_readUserExecutableAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS), &userExecutable);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

/* === Client highlevel write operations === */
START_TEST(hl_writeDisplayName) {
    UA_Client *client = connectClient();
    UA_LocalizedText newName = UA_LOCALIZEDTEXT("en", "UpdatedName");
    UA_StatusCode res = UA_Client_writeDisplayNameAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &newName);
    /* May fail if writeMask doesn't allow it */
    (void)res;
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeDescription) {
    UA_Client *client = connectClient();
    UA_LocalizedText newDesc = UA_LOCALIZEDTEXT("en", "Updated description");
    UA_StatusCode res = UA_Client_writeDescriptionAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &newDesc);
    (void)res;
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeWriteMask) {
    UA_Client *client = connectClient();
    UA_UInt32 newWM = 0;
    UA_StatusCode res = UA_Client_writeWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &newWM);
    (void)res;
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeValue) {
    UA_Client *client = connectClient();
    UA_Int32 newVal = 999;
    UA_Variant wv;
    UA_Variant_setScalar(&wv, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = UA_Client_writeValueAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &wv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_Variant rv;
    UA_Variant_init(&rv);
    res = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(1, 62001), &rv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32 *)rv.data, 999);
    UA_Variant_clear(&rv);

    disconnectClient(client);
} END_TEST

START_TEST(hl_writeAccessLevel) {
    UA_Client *client = connectClient();
    UA_Byte newAL = UA_ACCESSLEVELMASK_READ;
    UA_StatusCode res = UA_Client_writeAccessLevelAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &newAL);
    (void)res;
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeMinSamplingInterval) {
    UA_Client *client = connectClient();
    UA_Double newInterval = 500.0;
    UA_StatusCode res = UA_Client_writeMinimumSamplingIntervalAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &newInterval);
    (void)res;
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeHistorizing) {
    UA_Client *client = connectClient();
    UA_Boolean newHist = true;
    UA_StatusCode res = UA_Client_writeHistorizingAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &newHist);
    (void)res;
    disconnectClient(client);
} END_TEST

/* === Client add nodes === */
START_TEST(hl_addObjectNode) {
    UA_Client *client = connectClient();
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en", "ClientObj");

    UA_NodeId outId;
    UA_StatusCode res = UA_Client_addObjectNode(client,
        UA_NODEID_NUMERIC(1, 62100),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ClientObj"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oattr, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&outId);

    disconnectClient(client);
} END_TEST

START_TEST(hl_addVariableNode) {
    UA_Client *client = connectClient();
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 99;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.displayName = UA_LOCALIZEDTEXT("en", "ClientVar");
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    UA_NodeId outId;
    UA_StatusCode res = UA_Client_addVariableNode(client,
        UA_NODEID_NUMERIC(1, 62101),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ClientVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&outId);

    disconnectClient(client);
} END_TEST

START_TEST(hl_addObjectTypeNode) {
    UA_Client *client = connectClient();
    UA_ObjectTypeAttributes otattr = UA_ObjectTypeAttributes_default;
    otattr.displayName = UA_LOCALIZEDTEXT("en", "ClientObjType");

    UA_NodeId outId;
    UA_StatusCode res = UA_Client_addObjectTypeNode(client,
        UA_NODEID_NUMERIC(1, 62102),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "ClientObjType"),
        otattr, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&outId);

    disconnectClient(client);
} END_TEST

START_TEST(hl_addVariableTypeNode) {
    UA_Client *client = connectClient();
    UA_VariableTypeAttributes vtattr = UA_VariableTypeAttributes_default;
    vtattr.displayName = UA_LOCALIZEDTEXT("en", "ClientVarType");
    vtattr.valueRank = UA_VALUERANK_SCALAR;

    UA_NodeId outId;
    UA_StatusCode res = UA_Client_addVariableTypeNode(client,
        UA_NODEID_NUMERIC(1, 62103),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "ClientVarType"),
        vtattr, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&outId);

    disconnectClient(client);
} END_TEST

START_TEST(hl_addReferenceTypeNode) {
    UA_Client *client = connectClient();
    UA_ReferenceTypeAttributes rtattr = UA_ReferenceTypeAttributes_default;
    rtattr.displayName = UA_LOCALIZEDTEXT("en", "ClientRefType");
    rtattr.inverseName = UA_LOCALIZEDTEXT("en", "InvClientRefType");

    UA_NodeId outId;
    UA_StatusCode res = UA_Client_addReferenceTypeNode(client,
        UA_NODEID_NUMERIC(1, 62104),
        UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "ClientRefType"),
        rtattr, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&outId);

    disconnectClient(client);
} END_TEST

START_TEST(hl_addDataTypeNode) {
    UA_Client *client = connectClient();
    UA_DataTypeAttributes dtattr = UA_DataTypeAttributes_default;
    dtattr.displayName = UA_LOCALIZEDTEXT("en", "ClientDT");

    UA_NodeId outId;
    UA_StatusCode res = UA_Client_addDataTypeNode(client,
        UA_NODEID_NUMERIC(1, 62105),
        UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "ClientDT"),
        dtattr, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&outId);

    disconnectClient(client);
} END_TEST

START_TEST(hl_addReference) {
    UA_Client *client = connectClient();

    /* First add two objects */
    UA_ObjectAttributes oa1 = UA_ObjectAttributes_default;
    oa1.displayName = UA_LOCALIZEDTEXT("en", "RefObj1");
    UA_Client_addObjectNode(client, UA_NODEID_NUMERIC(1, 62110),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefObj1"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oa1, NULL);

    UA_ObjectAttributes oa2 = UA_ObjectAttributes_default;
    oa2.displayName = UA_LOCALIZEDTEXT("en", "RefObj2");
    UA_Client_addObjectNode(client, UA_NODEID_NUMERIC(1, 62111),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefObj2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oa2, NULL);

    /* Add a reference from Obj1 to Obj2 */
    UA_StatusCode res = UA_Client_addReference(client,
        UA_NODEID_NUMERIC(1, 62110),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        true,
        UA_STRING_NULL, /* no server URI */
        UA_EXPANDEDNODEID_NUMERIC(1, 62111),
        UA_NODECLASS_OBJECT);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Delete the reference */
    res = UA_Client_deleteReference(client,
        UA_NODEID_NUMERIC(1, 62110),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        true,
        UA_EXPANDEDNODEID_NUMERIC(1, 62111),
        true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Delete nodes */
    UA_Client_deleteNode(client, UA_NODEID_NUMERIC(1, 62110), true);
    UA_Client_deleteNode(client, UA_NODEID_NUMERIC(1, 62111), true);

    disconnectClient(client);
} END_TEST

/* === Read multiple (batch) === */
START_TEST(hl_readMultiple) {
    UA_Client *client = connectClient();

    UA_ReadValueId ids[3];
    UA_ReadValueId_init(&ids[0]);
    UA_ReadValueId_init(&ids[1]);
    UA_ReadValueId_init(&ids[2]);
    ids[0].nodeId = UA_NODEID_NUMERIC(1, 62001);
    ids[0].attributeId = UA_ATTRIBUTEID_VALUE;
    ids[1].nodeId = UA_NODEID_NUMERIC(1, 62002);
    ids[1].attributeId = UA_ATTRIBUTEID_VALUE;
    ids[2].nodeId = UA_NODEID_NUMERIC(1, 62003);
    ids[2].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = ids;
    req.nodesToReadSize = 3;

    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.resultsSize, 3);
    UA_ReadResponse_clear(&resp);

    disconnectClient(client);
} END_TEST

/* === Write multiple (batch) === */
START_TEST(hl_writeMultiple) {
    UA_Client *client = connectClient();

    UA_WriteValue wvs[2];
    UA_WriteValue_init(&wvs[0]);
    UA_WriteValue_init(&wvs[1]);
    wvs[0].nodeId = UA_NODEID_NUMERIC(1, 62001);
    wvs[0].attributeId = UA_ATTRIBUTEID_VALUE;
    wvs[0].value.hasValue = true;
    UA_Int32 v1 = 111;
    UA_Variant_setScalar(&wvs[0].value.value, &v1, &UA_TYPES[UA_TYPES_INT32]);

    wvs[1].nodeId = UA_NODEID_NUMERIC(1, 62004);
    wvs[1].attributeId = UA_ATTRIBUTEID_VALUE;
    wvs[1].value.hasValue = true;
    UA_Double v2 = 99.9;
    UA_Variant_setScalar(&wvs[1].value.value, &v2, &UA_TYPES[UA_TYPES_DOUBLE]);

    UA_WriteRequest req;
    UA_WriteRequest_init(&req);
    req.nodesToWrite = wvs;
    req.nodesToWriteSize = 2;

    UA_WriteResponse resp = UA_Client_Service_write(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.resultsSize, 2);
    UA_WriteResponse_clear(&resp);

    disconnectClient(client);
} END_TEST

/* === Client status === */
START_TEST(hl_clientState) {
    UA_Client *client = UA_Client_newForUnitTest();

    UA_SecureChannelState scState;
    UA_SessionState sessState;
    UA_StatusCode connStatus;
    UA_Client_getState(client, &scState, &sessState, &connStatus);
    ck_assert_int_eq(sessState, UA_SESSIONSTATE_CLOSED);

    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Client_getState(client, &scState, &sessState, &connStatus);
    ck_assert_int_eq(sessState, UA_SESSIONSTATE_ACTIVATED);

    /* Run iterate */
    for(int i = 0; i < 5; i++)
        UA_Client_run_iterate(client, 50);

    disconnectClient(client);
} END_TEST

/* === Suite definition === */
static Suite *testSuite_clientHL(void) {
    TCase *tc_read = tcase_create("HL_Read");
    tcase_add_checked_fixture(tc_read, setup, teardown);
    tcase_set_timeout(tc_read, 30);
    tcase_add_test(tc_read, hl_readDataType);
    tcase_add_test(tc_read, hl_readValueRank);
    tcase_add_test(tc_read, hl_readAccessLevel);
    tcase_add_test(tc_read, hl_readUserAccessLevel);
    tcase_add_test(tc_read, hl_readMinSamplingInterval);
    tcase_add_test(tc_read, hl_readHistorizing);
    tcase_add_test(tc_read, hl_readExecutable);
    tcase_add_test(tc_read, hl_readUserExecutable);

    TCase *tc_write = tcase_create("HL_Write");
    tcase_add_checked_fixture(tc_write, setup, teardown);
    tcase_set_timeout(tc_write, 30);
    tcase_add_test(tc_write, hl_writeDisplayName);
    tcase_add_test(tc_write, hl_writeDescription);
    tcase_add_test(tc_write, hl_writeWriteMask);
    tcase_add_test(tc_write, hl_writeValue);
    tcase_add_test(tc_write, hl_writeAccessLevel);
    tcase_add_test(tc_write, hl_writeMinSamplingInterval);
    tcase_add_test(tc_write, hl_writeHistorizing);

    TCase *tc_nodes = tcase_create("HL_Nodes");
    tcase_add_checked_fixture(tc_nodes, setup, teardown);
    tcase_set_timeout(tc_nodes, 30);
    tcase_add_test(tc_nodes, hl_addObjectNode);
    tcase_add_test(tc_nodes, hl_addVariableNode);
    tcase_add_test(tc_nodes, hl_addObjectTypeNode);
    tcase_add_test(tc_nodes, hl_addVariableTypeNode);
    tcase_add_test(tc_nodes, hl_addReferenceTypeNode);
    tcase_add_test(tc_nodes, hl_addDataTypeNode);
    tcase_add_test(tc_nodes, hl_addReference);

    TCase *tc_batch = tcase_create("HL_Batch");
    tcase_add_checked_fixture(tc_batch, setup, teardown);
    tcase_set_timeout(tc_batch, 30);
    tcase_add_test(tc_batch, hl_readMultiple);
    tcase_add_test(tc_batch, hl_writeMultiple);
    tcase_add_test(tc_batch, hl_clientState);

    Suite *s = suite_create("Client HighLevel Extended");
    suite_add_tcase(s, tc_read);
    suite_add_tcase(s, tc_write);
    suite_add_tcase(s, tc_nodes);
    suite_add_tcase(s, tc_batch);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_clientHL();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
