/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "testing_clock.h"
#include "tests/namespace_tests_di_generated.h"
#include "tests/namespace_tests_plc_generated.h"
#include "unistd.h"

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_addDiNodeset) {
    UA_StatusCode retval = namespace_tests_di_generated(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_addPlcNodeset) {
    UA_StatusCode retval = namespace_tests_plc_generated(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Compiler");
    TCase *tc_server = tcase_create("Server DI and PLCopen nodeset");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_addDiNodeset);
    tcase_add_test(tc_server, Server_addPlcNodeset);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
