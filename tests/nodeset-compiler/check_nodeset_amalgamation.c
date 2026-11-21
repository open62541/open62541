/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Intentionally include the implementations in one translation unit. */
#include "types_amalgamation_a_generated.c"
#include "types_amalgamation_b_generated.c"
#include "namespace_amalgamation_a_generated.c"
#include "namespace_amalgamation_b_generated.c"
#include "types_tests_pinned_multi_generated.c"
#include "namespace_tests_pinned_multi_generated.c"
#include "namespace_tests_pinned_renamed_generated.c"

#include <check.h>
#include <stdlib.h>
#include <open62541/server_config_default.h>

START_TEST(combinedModules) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_ne(server, NULL);
    UA_Boolean copy = (UA_Boolean)_i;
    if(copy)
        UA_Server_addNamespace(server, "urn:unrelated");
    UA_UInt16 a = UA_Server_addNamespace(server, "urn:open62541:test:amalgamation-a");
    UA_UInt16 b = UA_Server_addNamespace(server, "urn:open62541:test:amalgamation-b");
    ck_assert_uint_eq(namespace_amalgamation_a_generated(server, copy), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_amalgamation_b_generated(server, copy), UA_STATUSCODE_GOOD);
    UA_Byte bytes[] = {0, 1, 2, 255};
    UA_UInt16 indexes[] = {a, b};
    for(size_t i = 0; i < 2; i++) {
        UA_NodeId id = UA_NODEID_NUMERIC(indexes[i], 1001);
        ck_assert_ptr_ne(UA_Server_findDataType(server, &id), NULL);
        id.identifierType = UA_NODEIDTYPE_BYTESTRING;
        id.identifier.byteString = (UA_ByteString){sizeof(bytes), bytes};
        UA_NodeClass nodeClass;
        ck_assert_uint_eq(UA_Server_readNodeClass(server, id, &nodeClass), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(nodeClass, UA_NODECLASS_OBJECT);
    }
    UA_Server_delete(server);
} END_TEST

START_TEST(sharedArrayLoaders) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_ne(server, NULL);
    UA_Server_addNamespace(server, "urn:open62541:test:pinned-a");
    UA_Server_addNamespace(server, "urn:open62541:test:pinned-b");
    UA_StatusCode res = _i ? namespace_tests_pinned_renamed_generated(server, false)
                          : namespace_tests_pinned_multi_generated(server, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId id = UA_NODEID_NUMERIC(2, 1001);
    ck_assert_ptr_eq(UA_Server_findDataType(server, &id),
                     &UA_TYPES_TESTS_PINNED_MULTI[UA_TYPES_TESTS_PINNED_MULTI_PINNEDA]);
    UA_Server_delete(server);
} END_TEST

int main(void) {
    Suite *suite = suite_create("Generated module amalgamation");
    TCase *tc = tcase_create("loading");
    tcase_add_loop_test(tc, combinedModules, 0, 2);
    tcase_add_loop_test(tc, sharedArrayLoaders, 0, 2);
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
