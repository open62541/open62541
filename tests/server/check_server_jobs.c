/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "test_helpers.h"
#include "server/ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

#include "testing_clock.h"

UA_Server *server = NULL;
UA_Boolean *executed;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
dummyCallback(UA_Server *serverPtr, void *data) {
    *executed = true;
}

START_TEST(Server_addRemoveRepeatedCallback) {
    executed = UA_Boolean_new();

    /* The callback is added to the main queue only upon the next run_iterate */
    UA_UInt64 id;
    UA_Server_addRepeatedCallback(server, dummyCallback, NULL, 10, &id);

    /* Wait until the callback has surely timed out */
    UA_fakeSleep(15);
    UA_Server_run_iterate(server, false);

    /* Wait a bit longer until the workers have picked up the dispatched callback */
    UA_realSleep(100);
    ck_assert_uint_eq(*executed, true);

    UA_Server_removeRepeatedCallback(server, id);
    UA_Boolean_delete(executed);
}
END_TEST

UA_UInt64 *cbId;

static void
removeItselfCallback(UA_Server *serverPtr, void *data) {
    UA_Server_removeRepeatedCallback(serverPtr, *cbId);
}

START_TEST(Server_repeatedCallbackRemoveItself) {
    cbId = UA_UInt64_new();
    UA_Server_addRepeatedCallback(server, removeItselfCallback, NULL, 10, cbId);

    UA_fakeSleep(15);
    UA_Server_run_iterate(server, false);

    UA_UInt64_delete(cbId);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Callbacks");
    TCase *tc_server = tcase_create("Server Repeated Callbacks");
    tcase_add_checked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_addRemoveRepeatedCallback);
    tcase_add_test(tc_server, Server_repeatedCallbackRemoveItself);
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
