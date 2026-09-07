/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "nodeset_loader_test.h"

#include <check.h>

static UA_Server *server;
static UA_UInt16 namespaceIndex;

static void
setup(void) {
    server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_StatusCode res = UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = loadNodesetFile(server, OPEN62541_TESTNODESET_DIR "datatype_edge_cases.xml", NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    size_t index = 0;
    res = UA_Server_getNamespaceByName(
        server, UA_STRING("http://open62541.org/test/datatype-edge-cases/"), &index);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_le(index, UA_UINT16_MAX);
    namespaceIndex = (UA_UInt16)index;
}

static void
teardown(void) {
    UA_Server_delete(server);
}

static const UA_DataType *
findType(UA_UInt32 numericId) {
    UA_NodeId id = UA_NODEID_NUMERIC(namespaceIndex, numericId);
    return UA_Server_findDataType(server, &id);
}

START_TEST(resolveEncodingDirections) {
    const UA_DataType *forward = findType(8001);
    ck_assert_ptr_nonnull(forward);
    UA_NodeId forwardBinary = UA_NODEID_NUMERIC(namespaceIndex, 8002);
    ck_assert(UA_NodeId_equal(&forward->binaryEncodingId, &forwardBinary));

    const UA_DataType *inverse = findType(8101);
    ck_assert_ptr_nonnull(inverse);
    UA_NodeId inverseBinary = UA_NODEID_NUMERIC(namespaceIndex, 8102);
    ck_assert(UA_NodeId_equal(&inverse->binaryEncodingId, &inverseBinary));
    UA_NodeId inverseXml = UA_NODEID_NUMERIC(namespaceIndex, 8103);
    ck_assert(UA_NodeId_equal(&inverse->xmlEncodingId, &inverseXml));
}
END_TEST

START_TEST(importEmptyStructures) {
    const UA_DataType *emptyBase = findType(6001);
    ck_assert_ptr_nonnull(emptyBase);
    ck_assert_uint_eq(emptyBase->typeKind, UA_DATATYPEKIND_STRUCTURE);
    ck_assert_uint_eq(emptyBase->membersSize, 0);

    const UA_DataType *emptyDerived = findType(6002);
    ck_assert_ptr_nonnull(emptyDerived);
    ck_assert_uint_eq(emptyDerived->typeKind, UA_DATATYPEKIND_STRUCTURE);
    ck_assert_uint_eq(emptyDerived->membersSize, 0);

    const UA_DataType *nonEmpty = findType(6003);
    ck_assert_ptr_nonnull(nonEmpty);
    ck_assert_uint_eq(nonEmpty->typeKind, UA_DATATYPEKIND_STRUCTURE);
    ck_assert_uint_eq(nonEmpty->membersSize, 1);
    ck_assert_ptr_eq(nonEmpty->members[0].memberType, &UA_TYPES[UA_TYPES_UINT32]);
}
END_TEST

START_TEST(resolveOpaqueSubtypeAncestors) {
    const UA_DataType *picture = findType(3001);
    ck_assert_ptr_nonnull(picture);
    ck_assert_uint_eq(picture->typeKind, UA_DATATYPEKIND_BYTESTRING);

    const UA_DataType *abstractByteString = findType(3002);
    ck_assert_ptr_nonnull(abstractByteString);
    ck_assert_uint_eq(abstractByteString->typeKind, UA_DATATYPEKIND_BYTESTRING);

    const UA_DataType *concreteByteString = findType(3003);
    ck_assert_ptr_nonnull(concreteByteString);
    ck_assert_uint_eq(concreteByteString->typeKind, UA_DATATYPEKIND_BYTESTRING);

    const UA_DataType *customVariant = findType(3004);
    ck_assert_ptr_nonnull(customVariant);
    ck_assert_uint_eq(customVariant->typeKind, UA_DATATYPEKIND_VARIANT);
}
END_TEST

START_TEST(importPolymorphicAndRecursiveFields) {
    const UA_DataType *polymorphic = findType(7001);
    ck_assert_ptr_nonnull(polymorphic);
    ck_assert_uint_eq(polymorphic->membersSize, 2);
    ck_assert_ptr_eq(polymorphic->members[0].memberType,
                     &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert_ptr_eq(polymorphic->members[1].memberType, &UA_TYPES[UA_TYPES_STRING]);

    const UA_DataType *recursive = findType(9001);
    ck_assert_ptr_nonnull(recursive);
    ck_assert_uint_eq(recursive->membersSize, 1);
    ck_assert(recursive->members[0].isArray);
    ck_assert_ptr_eq(recursive->members[0].memberType, recursive);
}
END_TEST

static void
assertUnionValue(UA_UInt32 nodeId, UA_UInt32 expectedSelection,
                 const void *expectedValue, const UA_DataType *expectedType) {
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode res =
        UA_Server_readValue(server, UA_NODEID_NUMERIC(namespaceIndex, nodeId), &value);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const UA_DataType *unionType = findType(9101);
    ck_assert_ptr_nonnull(unionType);
    ck_assert_ptr_eq(value.type, unionType);
    ck_assert(UA_Variant_isScalar(&value));

    UA_UInt32 selection = *(const UA_UInt32 *)value.data;
    ck_assert_uint_eq(selection, expectedSelection);
    ck_assert_uint_le(selection, unionType->membersSize);
    const UA_DataTypeMember *member = &unionType->members[selection - 1];
    ck_assert_ptr_eq(member->memberType, expectedType);
    ck_assert(UA_order((const UA_Byte *)value.data + member->padding, expectedValue,
                       expectedType) == UA_ORDER_EQ);
    UA_Variant_clear(&value);
}

START_TEST(readImportedUnionValues) {
    const UA_UInt32 x = 70000;
    assertUnionValue(9201, 1, &x, &UA_TYPES[UA_TYPES_UINT32]);

    const UA_Int32 y = -1000;
    assertUnionValue(9202, 2, &y, &UA_TYPES[UA_TYPES_INT32]);
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("NodeSet loader DataTypes");
    TCase *testCase = tcase_create("DataType edge cases");
    tcase_add_unchecked_fixture(testCase, setup, teardown);
    tcase_add_test(testCase, resolveEncodingDirections);
    tcase_add_test(testCase, importEmptyStructures);
    tcase_add_test(testCase, resolveOpaqueSubtypeAncestors);
    tcase_add_test(testCase, importPolymorphicAndRecursiveFields);
    tcase_add_test(testCase, readImportedUnionValues);
    suite_add_tcase(suite, testCase);
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
