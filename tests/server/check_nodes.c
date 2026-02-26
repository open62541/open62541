/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "test_helpers.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

/* --- NodePointer tests --- */

START_TEST(nodePointerFromNumericNodeId) {
    UA_NodeId id = UA_NODEID_NUMERIC(0, 85); /* ObjectsFolder */
    UA_NodePointer np = UA_NodePointer_fromNodeId(&id);
    UA_NodeId back = UA_NodePointer_toNodeId(np);
    ck_assert(UA_NodeId_equal(&id, &back));
    ck_assert(UA_NodePointer_isLocal(np));
    UA_NodePointer_clear(&np);
} END_TEST

START_TEST(nodePointerFromStringNodeId) {
    UA_NodeId id = UA_NODEID_STRING_ALLOC(1, "test.node.pointer");
    UA_NodePointer np = UA_NodePointer_fromNodeId(&id);
    UA_NodeId back = UA_NodePointer_toNodeId(np);
    ck_assert(UA_NodeId_equal(&id, &back));
    ck_assert(UA_NodePointer_isLocal(np));
    UA_NodeId_clear(&id);
    /* np references the original id memory, we've cleared it */
} END_TEST

START_TEST(nodePointerCopyNumeric) {
    UA_NodeId id = UA_NODEID_NUMERIC(0, 100);
    UA_NodePointer np = UA_NodePointer_fromNodeId(&id);
    UA_NodePointer copy;
    UA_NodePointer_init(&copy);
    UA_StatusCode res = UA_NodePointer_copy(np, &copy);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId nid1 = UA_NodePointer_toNodeId(np);
    UA_NodeId nid2 = UA_NodePointer_toNodeId(copy);
    ck_assert(UA_NodeId_equal(&nid1, &nid2));

    UA_NodePointer_clear(&copy);
} END_TEST

START_TEST(nodePointerCopyString) {
    UA_NodeId id = UA_NODEID_STRING_ALLOC(2, "copytest");
    UA_NodePointer np = UA_NodePointer_fromNodeId(&id);
    UA_NodePointer copy;
    UA_NodePointer_init(&copy);
    UA_StatusCode res = UA_NodePointer_copy(np, &copy);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    ck_assert(UA_NodePointer_isLocal(copy));

    UA_NodeId nid2 = UA_NodePointer_toNodeId(copy);
    ck_assert(UA_NodeId_equal(&id, &nid2));

    UA_NodePointer_clear(&copy);
    UA_NodeId_clear(&id);
} END_TEST

START_TEST(nodePointerOrder) {
    UA_NodeId id1 = UA_NODEID_NUMERIC(0, 10);
    UA_NodeId id2 = UA_NODEID_NUMERIC(0, 20);
    UA_NodePointer np1 = UA_NodePointer_fromNodeId(&id1);
    UA_NodePointer np2 = UA_NodePointer_fromNodeId(&id2);

    UA_Order order = UA_NodePointer_order(np1, np2);
    ck_assert_int_eq(order, UA_ORDER_LESS);

    order = UA_NodePointer_order(np2, np1);
    ck_assert_int_eq(order, UA_ORDER_MORE);

    order = UA_NodePointer_order(np1, np1);
    ck_assert_int_eq(order, UA_ORDER_EQ);
} END_TEST

START_TEST(nodePointerOrderString) {
    UA_NodeId id1 = UA_NODEID_STRING_ALLOC(1, "aaa");
    UA_NodeId id2 = UA_NODEID_STRING_ALLOC(1, "bbb");
    UA_NodePointer np1 = UA_NodePointer_fromNodeId(&id1);
    UA_NodePointer np2 = UA_NodePointer_fromNodeId(&id2);

    UA_Order order = UA_NodePointer_order(np1, np2);
    ck_assert_int_eq(order, UA_ORDER_LESS);

    UA_NodeId_clear(&id1);
    UA_NodeId_clear(&id2);
} END_TEST

/* --- Node read operations via UA_Server --- */

START_TEST(readObjectNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);
} END_TEST

START_TEST(readVariableNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_VARIABLE);
} END_TEST

START_TEST(readObjectTypeNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECTTYPE);

    /* Read IsAbstract */
    UA_Boolean isAbstract;
    retval = UA_Server_readIsAbstract(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), &isAbstract);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(readDataTypeNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_DATATYPE);
} END_TEST

START_TEST(readReferenceTypeNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_REFERENCETYPE);

    /* Read InverseName */
    UA_LocalizedText inverseName;
    retval = UA_Server_readInverseName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &inverseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&inverseName);
} END_TEST

START_TEST(readViewNode) {
    /* The default namespace has Views folder but might not have actual views.
     * Just read the ViewsFolder node class. */
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);
} END_TEST

/* --- Node add/copy operations via server API --- */

START_TEST(addVariableNodeCheckAttributes) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Double val = 3.14;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "MyDouble");
    attr.description = UA_LOCALIZEDTEXT("en-US", "A double value");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr.historizing = false;

    UA_NodeId myId = UA_NODEID_STRING(1, "nodes.test.double");
    UA_StatusCode retval = UA_Server_addVariableNode(server, myId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "MyDouble"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back all attributes */
    UA_Variant readVal;
    retval = UA_Server_readValue(server, myId, &readVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&readVal, &UA_TYPES[UA_TYPES_DOUBLE]));
    ck_assert((*(UA_Double*)readVal.data) - 3.14 < 1e-10 &&
              (*(UA_Double*)readVal.data) - 3.14 > -1e-10);
    UA_Variant_clear(&readVal);

    UA_LocalizedText dn;
    retval = UA_Server_readDisplayName(server, myId, &dn);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_String expectedDN = UA_STRING("MyDouble");
    ck_assert(UA_String_equal(&dn.text, &expectedDN));
    UA_LocalizedText_clear(&dn);

    UA_Byte accessLevel;
    retval = UA_Server_readAccessLevel(server, myId, &accessLevel);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(accessLevel & UA_ACCESSLEVELMASK_READ);
    ck_assert(accessLevel & UA_ACCESSLEVELMASK_WRITE);

    UA_Boolean historizing;
    retval = UA_Server_readHistorizing(server, myId, &historizing);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(historizing == false);
} END_TEST

START_TEST(addObjectTypeNode) {
    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestObjectType");
    otAttr.isAbstract = false;

    UA_NodeId otId = UA_NODEID_STRING(1, "nodes.test.ot");
    UA_StatusCode retval = UA_Server_addObjectTypeNode(server, otId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestObjectType"),
        otAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read NodeClass */
    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, otId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECTTYPE);
} END_TEST

START_TEST(addVariableTypeNode) {
    UA_VariableTypeAttributes vtAttr = UA_VariableTypeAttributes_default;
    vtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVariableType");
    vtAttr.isAbstract = false;
    vtAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    vtAttr.valueRank = UA_VALUERANK_SCALAR;

    UA_NodeId vtId = UA_NODEID_STRING(1, "nodes.test.vt");
    UA_StatusCode retval = UA_Server_addVariableTypeNode(server, vtId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestVariableType"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vtAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, vtId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_VARIABLETYPE);
} END_TEST

START_TEST(addReferenceTypeNode) {
    UA_ReferenceTypeAttributes rtAttr = UA_ReferenceTypeAttributes_default;
    rtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestRefType");
    rtAttr.isAbstract = false;
    rtAttr.symmetric = false;
    rtAttr.inverseName = UA_LOCALIZEDTEXT("en-US", "IsTestedBy");

    UA_NodeId rtId = UA_NODEID_STRING(1, "nodes.test.rt");
    UA_StatusCode retval = UA_Server_addReferenceTypeNode(server, rtId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestRefType"),
        rtAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, rtId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_REFERENCETYPE);
} END_TEST

int main(void) {
    Suite *s = suite_create("nodes");

    TCase *tc_np = tcase_create("NodePointer");
    tcase_add_test(tc_np, nodePointerFromNumericNodeId);
    tcase_add_test(tc_np, nodePointerFromStringNodeId);
    tcase_add_test(tc_np, nodePointerCopyNumeric);
    tcase_add_test(tc_np, nodePointerCopyString);
    tcase_add_test(tc_np, nodePointerOrder);
    tcase_add_test(tc_np, nodePointerOrderString);
    suite_add_tcase(s, tc_np);

    TCase *tc_read = tcase_create("ReadNodes");
    tcase_add_checked_fixture(tc_read, setup, teardown);
    tcase_add_test(tc_read, readObjectNode);
    tcase_add_test(tc_read, readVariableNode);
    tcase_add_test(tc_read, readObjectTypeNode);
    tcase_add_test(tc_read, readDataTypeNode);
    tcase_add_test(tc_read, readReferenceTypeNode);
    tcase_add_test(tc_read, readViewNode);
    suite_add_tcase(s, tc_read);

    TCase *tc_add = tcase_create("AddNodes");
    tcase_add_checked_fixture(tc_add, setup, teardown);
    tcase_add_test(tc_add, addVariableNodeCheckAttributes);
    tcase_add_test(tc_add, addObjectTypeNode);
    tcase_add_test(tc_add, addVariableTypeNode);
    tcase_add_test(tc_add, addReferenceTypeNode);
    suite_add_tcase(s, tc_add);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
