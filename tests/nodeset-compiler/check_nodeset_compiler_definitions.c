/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer) */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "check.h"
#include "namespace_tests_definitions_generated.h"
#include "test_helpers.h"

START_TEST(loadDefinitionsNodeset) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
    ck_assert_uint_eq(namespace_tests_definitions_generated(server),
                      UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

int main(void) {
    Suite *suite = suite_create("NodeSet Definition datatypes");
    TCase *testCase = tcase_create("load");
    tcase_add_test(testCase, loadDefinitionsNodeset);
    suite_add_tcase(suite, testCase);

    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
