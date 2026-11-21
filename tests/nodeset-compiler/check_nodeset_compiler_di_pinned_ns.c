/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "check.h"
#include "namespace_tests_di_generated.h"
#include "test_helpers.h"
#include "types_tests_di_generated.h"

/* The tests-di nodeset is generated with NAMESPACE_MAP (see CMakeLists.txt),
 * which bakes the namespace index into the const type array.
 * This test only covers that configuration. */
#ifndef UA_TYPES_TESTS_DI_IS_CONST
#error "tests-di must be generated with NAMESPACE_MAP to cover the const DataType array"
#endif

#define DI_NAMESPACE_URI "http://opcfoundation.org/UA/DI/"

/* Namespace index pinned via NAMESPACE_MAP. NS0 is 0 and the server
 * application URI takes 1, so the first namespace added is 2. */
#define DI_PINNED_NS_INDEX 2

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
    server = NULL;
}

/* DI is the first namespace added, matching the baked datatype indexes. */
START_TEST(Server_pinnedNamespaceMatches) {
    UA_StatusCode retval = namespace_tests_di_generated(server, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t nsIndex = 0;
    retval = UA_Server_getNamespaceByName(server, UA_STRING(DI_NAMESPACE_URI), &nsIndex);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nsIndex, DI_PINNED_NS_INDEX);

    /* The const array is not patched at load time. Its typeIds already carry
     * the runtime namespace index and are directly usable for lookups. */
    ck_assert_uint_gt(UA_TYPES_TESTS_DI_COUNT, 0);
    for(size_t i = 0; i < UA_TYPES_TESTS_DI_COUNT; i++) {
        const UA_DataType *dt = &UA_TYPES_TESTS_DI[i];
        ck_assert_uint_eq(dt->typeId.namespaceIndex, DI_PINNED_NS_INDEX);
        ck_assert_ptr_ne(UA_Server_findDataType(server, &dt->typeId), NULL);
    }
}
END_TEST

/* Copy mode adjusts the pinned input when DI is loaded at another index. */
START_TEST(Server_copyPinnedNamespaceRemapped) {
    ck_assert_uint_eq(UA_Server_addNamespace(server, "urn:open62541.tests:not-di"),
                      DI_PINNED_NS_INDEX);
    ck_assert_uint_eq(namespace_tests_di_generated(server, true), UA_STATUSCODE_GOOD);
    size_t nsIndex;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(server, UA_STRING(DI_NAMESPACE_URI),
                                                   &nsIndex), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(nsIndex, DI_PINNED_NS_INDEX);
    for(size_t i = 0; i < UA_TYPES_TESTS_DI_COUNT; i++) {
        const UA_DataType *original = &UA_TYPES_TESTS_DI[i];
        UA_NodeId id = original->typeId;
        id.namespaceIndex = (UA_UInt16)nsIndex;
        const UA_DataType *loaded = UA_Server_findDataType(server, &id);
        ck_assert_ptr_ne(loaded, NULL);
        ck_assert_ptr_ne(loaded, original);
        ck_assert_uint_eq(original->typeId.namespaceIndex, DI_PINNED_NS_INDEX);
        ck_assert_uint_eq(loaded->typeId.namespaceIndex, nsIndex);
    }
} END_TEST

static Suite* testSuite_pinnedNamespace(void) {
    Suite *s = suite_create("Nodeset Compiler Pinned Namespace");
    TCase *tc = tcase_create("const DataType array");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, Server_pinnedNamespaceMatches);
    tcase_add_test(tc, Server_copyPinnedNamespaceRemapped);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_pinnedNamespace();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
