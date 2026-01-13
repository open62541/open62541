/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_file_based.h>

#include "../common.h"

#include <check.h>

#if defined(_MSC_VER)
# pragma warning(disable: 4146)
#endif

static char const * const file_name = "server_json_config.json5";

START_TEST(UA_new_server_from_json) {

    UA_ByteString json_config = loadFile(file_name);

    /* test setup failure */
    ck_assert(!UA_ByteString_equal(&json_config, &UA_BYTESTRING_NULL));

    UA_Server *server = UA_Server_newFromFile(json_config);

    ck_assert_ptr_ne(server, NULL);
    UA_ByteString_clear(&json_config);
    UA_Server_delete(server);
}
END_TEST

static Suite *testSuite_server_from_json(void) {
    Suite *s = suite_create("Load server from json config");
    TCase *tc_new = tcase_create("new");
    tcase_add_test(tc_new, UA_new_server_from_json);
    suite_add_tcase(s, tc_new);
    return s;
}

int main(void) {
    int      number_failed = 0;
    Suite   *s;
    SRunner *sr;
    s  = testSuite_server_from_json();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
