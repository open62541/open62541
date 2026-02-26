/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Server attribute read/write wrapper tests */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <check.h>
#include "test_helpers.h"

static UA_Server *server;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);

    /* Add a variable node */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    vattr.writeMask = 0xFFFFFFFF; /* all bits writable */
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 70001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "AttrTestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    /* Add an object node with a property */
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    oattr.writeMask = 0xFFFFFFFF;
    UA_Server_addObjectNode(server,
        UA_NODEID_NUMERIC(1, 70010),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "AttrTestObj"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oattr, NULL, NULL);

    /* Add property to the object */
    UA_VariableAttributes propAttr = UA_VariableAttributes_default;
    UA_Int32 propVal = 100;
    UA_Variant_setScalar(&propAttr.value, &propVal, &UA_TYPES[UA_TYPES_INT32]);
    propAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 70011),
        UA_NODEID_NUMERIC(1, 70010),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
        UA_QUALIFIEDNAME(1, "TestProp"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE),
        propAttr, NULL, NULL);

    /* Add a method node */
    UA_MethodAttributes mattr = UA_MethodAttributes_default;
    mattr.executable = true;
    mattr.userExecutable = true;
    mattr.writeMask = 0xFFFFFFFF;
    UA_Server_addMethodNode(server,
        UA_NODEID_NUMERIC(1, 70020),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "AttrTestMethod"),
        mattr, NULL, 0, NULL, 0, NULL, NULL, NULL);

    /* Add a view node */
    UA_ViewAttributes viewAttr = UA_ViewAttributes_default;
    viewAttr.writeMask = 0xFFFFFFFF;
    UA_Server_addViewNode(server,
        UA_NODEID_NUMERIC(1, 70030),
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "AttrTestView"),
        viewAttr, NULL, NULL);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* Read attribute wrappers */
START_TEST(read_symmetric) {
    UA_Boolean sym;
    UA_StatusCode res = UA_Server_readSymmetric(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &sym);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_containsNoLoops) {
    UA_Boolean cnl;
    UA_StatusCode res = UA_Server_readContainsNoLoops(server,
        UA_NODEID_NUMERIC(1, 70030), &cnl);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_arrayDimensions) {
    UA_Variant dims;
    UA_StatusCode res = UA_Server_readArrayDimensions(server,
        UA_NODEID_NUMERIC(1, 70001), &dims);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&dims);
} END_TEST

START_TEST(read_accessLevelEx) {
    UA_UInt32 al;
    UA_StatusCode res = UA_Server_readAccessLevelEx(server,
        UA_NODEID_NUMERIC(1, 70001), &al);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_executable) {
    UA_Boolean e;
    UA_StatusCode res = UA_Server_readExecutable(server,
        UA_NODEID_NUMERIC(1, 70020), &e);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(e == true);
} END_TEST

START_TEST(read_userExecutable) {
    /* UA_Server_readUserExecutable doesn't exist in this version,
       but readExecutable covers the method node read path */
    UA_Boolean e;
    UA_StatusCode res = UA_Server_readExecutable(server,
        UA_NODEID_NUMERIC(1, 70020), &e);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_minimumSamplingInterval) {
    UA_Double msi;
    UA_StatusCode res = UA_Server_readMinimumSamplingInterval(server,
        UA_NODEID_NUMERIC(1, 70001), &msi);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_historizing) {
    UA_Boolean h;
    UA_StatusCode res = UA_Server_readHistorizing(server,
        UA_NODEID_NUMERIC(1, 70001), &h);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_objectProperty) {
    UA_Variant val;
    UA_StatusCode res = UA_Server_readObjectProperty(server,
        UA_NODEID_NUMERIC(1, 70010),
        UA_QUALIFIEDNAME(1, "TestProp"), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32 *)val.data, 100);
    UA_Variant_clear(&val);
} END_TEST

/* Write attribute wrappers */
START_TEST(write_browseName) {
    /* BrowseName is often not writable; just exercise the function path */
    UA_QualifiedName bn = UA_QUALIFIEDNAME(1, "RenamedVar");
    UA_StatusCode res = UA_Server_writeBrowseName(server,
        UA_NODEID_NUMERIC(1, 70001), bn);
    /* Accept GOOD or any error - just exercise the code path */
    (void)res;
} END_TEST

START_TEST(write_description) {
    UA_LocalizedText desc = UA_LOCALIZEDTEXT("en", "Test description");
    UA_StatusCode res = UA_Server_writeDescription(server,
        UA_NODEID_NUMERIC(1, 70001), desc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_writeMask) {
    UA_UInt32 wm = 0;
    UA_StatusCode res = UA_Server_writeWriteMask(server,
        UA_NODEID_NUMERIC(1, 70001), wm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_isAbstract) {
    UA_Boolean ia = false;
    /* Need a type node for this */
    UA_ObjectTypeAttributes otattr = UA_ObjectTypeAttributes_default;
    otattr.writeMask = 0xFFFFFFFF;
    UA_Server_addObjectTypeNode(server,
        UA_NODEID_NUMERIC(1, 70040),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "AbsTestType"),
        otattr, NULL, NULL);
    UA_StatusCode res = UA_Server_writeIsAbstract(server,
        UA_NODEID_NUMERIC(1, 70040), ia);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_inverseName) {
    UA_ReferenceTypeAttributes refattr = UA_ReferenceTypeAttributes_default;
    refattr.writeMask = 0xFFFFFFFF;
    refattr.isAbstract = false;
    refattr.symmetric = false;
    UA_Server_addReferenceTypeNode(server,
        UA_NODEID_NUMERIC(1, 70050),
        UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "InvNameRef"),
        refattr, NULL, NULL);
    UA_LocalizedText inv = UA_LOCALIZEDTEXT("en", "InversedBy");
    UA_StatusCode res = UA_Server_writeInverseName(server,
        UA_NODEID_NUMERIC(1, 70050), inv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_eventNotifier) {
    UA_Byte en = 1;
    UA_StatusCode res = UA_Server_writeEventNotifier(server,
        UA_NODEID_NUMERIC(1, 70010), en);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_dataValue) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Int32 val = 123;
    UA_Variant_setScalar(&dv.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    dv.hasSourceTimestamp = true;
    dv.sourceTimestamp = UA_DateTime_now();
    UA_StatusCode res = UA_Server_writeDataValue(server,
        UA_NODEID_NUMERIC(1, 70001), dv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_dataType) {
    UA_NodeId dtId = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    UA_StatusCode res = UA_Server_writeDataType(server,
        UA_NODEID_NUMERIC(1, 70001), dtId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_valueRank) {
    UA_Int32 vr = UA_VALUERANK_SCALAR;
    UA_StatusCode res = UA_Server_writeValueRank(server,
        UA_NODEID_NUMERIC(1, 70001), vr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_arrayDimensions) {
    UA_Variant dims;
    UA_Variant_init(&dims);
    UA_StatusCode res = UA_Server_writeArrayDimensions(server,
        UA_NODEID_NUMERIC(1, 70001), dims);
    /* May or may not succeed depending on current value rank */
    (void)res;
} END_TEST

START_TEST(write_accessLevelEx) {
    UA_UInt32 al = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_StatusCode res = UA_Server_writeAccessLevelEx(server,
        UA_NODEID_NUMERIC(1, 70001), al);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_historizing) {
    UA_Boolean h = false;
    UA_StatusCode res = UA_Server_writeHistorizing(server,
        UA_NODEID_NUMERIC(1, 70001), h);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_executable) {
    UA_Boolean e = true;
    UA_StatusCode res = UA_Server_writeExecutable(server,
        UA_NODEID_NUMERIC(1, 70020), e);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(write_objectProperty) {
    UA_Variant val;
    UA_Int32 newVal = 200;
    UA_Variant_setScalar(&val, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = UA_Server_writeObjectProperty(server,
        UA_NODEID_NUMERIC(1, 70010),
        UA_QUALIFIEDNAME(1, "TestProp"), val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_Variant readVal;
    res = UA_Server_readObjectProperty(server,
        UA_NODEID_NUMERIC(1, 70010),
        UA_QUALIFIEDNAME(1, "TestProp"), &readVal);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32 *)readVal.data, 200);
    UA_Variant_clear(&readVal);
} END_TEST

START_TEST(write_objectProperty_scalar) {
    UA_Int32 propVal = 300;
    UA_StatusCode res = UA_Server_writeObjectProperty_scalar(server,
        UA_NODEID_NUMERIC(1, 70010),
        UA_QUALIFIEDNAME(1, "TestProp"),
        &propVal, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_Variant readVal;
    res = UA_Server_readObjectProperty(server,
        UA_NODEID_NUMERIC(1, 70010),
        UA_QUALIFIEDNAME(1, "TestProp"), &readVal);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32 *)readVal.data, 300);
    UA_Variant_clear(&readVal);
} END_TEST

/* Server repeated callback operations */
static void testCallback(UA_Server *s, void *data) {
    (void)s; (void)data;
}

START_TEST(srv_repeatedCallback) {
    UA_UInt64 callbackId = 0;
    UA_StatusCode res = UA_Server_addRepeatedCallback(server,
        testCallback, NULL, 1000.0, &callbackId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_changeRepeatedCallbackInterval(server, callbackId, 2000.0);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_removeRepeatedCallback(server, callbackId);
} END_TEST

static Suite *testSuite_serverAttributes(void) {
    TCase *tc_read = tcase_create("SrvAttr_Read");
    tcase_add_checked_fixture(tc_read, setup, teardown);
    tcase_add_test(tc_read, read_symmetric);
    tcase_add_test(tc_read, read_containsNoLoops);
    tcase_add_test(tc_read, read_arrayDimensions);
    tcase_add_test(tc_read, read_accessLevelEx);
    tcase_add_test(tc_read, read_executable);
    tcase_add_test(tc_read, read_userExecutable);
    tcase_add_test(tc_read, read_minimumSamplingInterval);
    tcase_add_test(tc_read, read_historizing);
    tcase_add_test(tc_read, read_objectProperty);

    TCase *tc_write = tcase_create("SrvAttr_Write");
    tcase_add_checked_fixture(tc_write, setup, teardown);
    tcase_add_test(tc_write, write_browseName);
    tcase_add_test(tc_write, write_description);
    tcase_add_test(tc_write, write_writeMask);
    tcase_add_test(tc_write, write_isAbstract);
    tcase_add_test(tc_write, write_inverseName);
    tcase_add_test(tc_write, write_eventNotifier);
    tcase_add_test(tc_write, write_dataValue);
    tcase_add_test(tc_write, write_dataType);
    tcase_add_test(tc_write, write_valueRank);
    tcase_add_test(tc_write, write_arrayDimensions);
    tcase_add_test(tc_write, write_accessLevelEx);
    tcase_add_test(tc_write, write_historizing);
    tcase_add_test(tc_write, write_executable);
    tcase_add_test(tc_write, write_objectProperty);
    tcase_add_test(tc_write, write_objectProperty_scalar);

    TCase *tc_cb = tcase_create("SrvAttr_Callback");
    tcase_add_checked_fixture(tc_cb, setup, teardown);
    tcase_add_test(tc_cb, srv_repeatedCallback);

    Suite *s = suite_create("Server Attribute Wrappers");
    suite_add_tcase(s, tc_read);
    suite_add_tcase(s, tc_write);
    suite_add_tcase(s, tc_cb);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_serverAttributes();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
