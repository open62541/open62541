/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "namespace_tests_pinned_multi_generated.h"
#include "namespace_tests_pinned_renamed_generated.h"
#include "test_helpers.h"

#include <check.h>

#ifndef UA_TYPES_TESTS_PINNED_MULTI_IS_CONST
#error "The namespace validation tests require a const datatype array"
#endif

#define NS_A "urn:open62541:test:pinned-a"
#define NS_B "urn:open62541:test:pinned-b"

static UA_Server *server;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
}

static void
teardown(void) {
    UA_Server_delete(server);
}

/* Exercise the same array with both the conventional output name and an
 * independently named nodeset using the explicit TYPES_ARRAY argument. */
static UA_StatusCode
loadNodeset(int renamed) {
    return renamed ? namespace_tests_pinned_renamed_generated(server) :
        namespace_tests_pinned_multi_generated(server);
}

START_TEST(matchingNamespaces) {
    ck_assert_uint_eq(loadNodeset(_i), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_TYPES_TESTS_PINNED_MULTI[
        UA_TYPES_TESTS_PINNED_MULTI_PINNEDA].typeId.identifier.numeric, 1001);
    ck_assert_uint_eq(UA_TYPES_TESTS_PINNED_MULTI[
        UA_TYPES_TESTS_PINNED_MULTI_PINNEDB].typeId.identifier.numeric, 2001);
    for(size_t i = 0; i < UA_TYPES_TESTS_PINNED_MULTI_COUNT; i++) {
        const UA_DataType *type = &UA_TYPES_TESTS_PINNED_MULTI[i];
        ck_assert_ptr_eq(UA_Server_findDataType(server, &type->typeId), type);
        UA_NodeClass nodeClass;
        ck_assert_uint_eq(UA_Server_readNodeClass(server, type->typeId, &nodeClass),
                          UA_STATUSCODE_GOOD);
        ck_assert_int_eq(nodeClass, UA_NODECLASS_DATATYPE);
    }
}
END_TEST

START_TEST(swappedNamespaces) {
    ck_assert_uint_eq(UA_Server_addNamespace(server, NS_B), 2);
    ck_assert_uint_eq(UA_Server_addNamespace(server, NS_A), 3);
    ck_assert_uint_eq(loadNodeset(_i), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_ptr_null(UA_Server_getConfig(server)->customDataTypes);
}
END_TEST

START_TEST(secondNamespaceMismatch) {
    ck_assert_uint_eq(UA_Server_addNamespace(server, NS_A), 2);
    ck_assert_uint_eq(UA_Server_addNamespace(server, "urn:unrelated"), 3);
    ck_assert_uint_eq(loadNodeset(_i), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_ptr_null(UA_Server_getConfig(server)->customDataTypes);
}
END_TEST

START_TEST(firstNamespaceMismatch) {
    ck_assert_uint_eq(UA_Server_addNamespace(server, "urn:unrelated"), 2);
    ck_assert_uint_eq(UA_Server_addNamespace(server, NS_B), 3);
    ck_assert_uint_eq(loadNodeset(_i), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_ptr_null(UA_Server_getConfig(server)->customDataTypes);
}
END_TEST

int
main(void) {
    Suite *suite = suite_create("Nodeset compiler namespace validation");
    TCase *tc = tcase_create("Pinned namespaces and explicit array names");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_loop_test(tc, matchingNamespaces, 0, 2);
    tcase_add_loop_test(tc, swappedNamespaces, 0, 2);
    tcase_add_loop_test(tc, secondNamespaceMismatch, 0, 2);
    tcase_add_loop_test(tc, firstNamespaceMismatch, 0, 2);
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
