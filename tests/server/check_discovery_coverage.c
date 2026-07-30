/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Coverage tests for src/server/ua_discovery.c.
 *
 * Focus: the early-return guards in UA_Server_registerDiscovery and
 * UA_Server_deregisterDiscovery when the discovery manager is not
 * in the STARTED state. The existing tests/encryption/check_* and
 * tests/discovery/ paths are either encryption-gated or only run
 * against a started server. This file tests the not-started path
 * which is reachable via public API and does not need encryption.
 *
 *   - UA_Server_register: line 433-438
 *     if(dm->sc.state != UA_LIFECYCLESTATE_STARTED)
 *       return UA_STATUSCODE_BADINTERNALERROR;
 *   - UA_Server_deregisterDiscovery: same guard (mirrors)
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

#ifndef ck_assert_ptr_null
# define ck_assert_ptr_null(p) ck_assert_msg((p) == NULL, #p " is not NULL")
#endif
#ifndef ck_assert_ptr_nonnull
# define ck_assert_ptr_nonnull(p) ck_assert_msg((p) != NULL, #p " is NULL")
#endif

static UA_Server *server;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
    /* Intentionally NOT calling UA_Server_run_startup -- the
     * discovery manager remains in the FRESH state. */
}

static void teardown(void) {
    /* Use a manual delete since run_startup was never called. */
    UA_Server_delete(server);
}

START_TEST(Server_registerDiscovery_serverNotStarted_rejected) {
    /* src/server/ua_discovery.c:433-438 (UA_Server_register guard):
     *   if(dm->sc.state != UA_LIFECYCLESTATE_STARTED) {
     *     ... return UA_STATUSCODE_BADINTERNALERROR;
     *   }
     * The server was created but never run_startup'd. The discovery
     * component is registered (UA_ENABLE_DISCOVERY is on) but its
     * state is FRESH. UA_Server_registerDiscovery must fail with
     * BADINTERNALERROR. The public cc is consumed and freed by
     * the failing call (UA_ClientConfig_clear at line 436-437). */
    UA_ClientConfig cc;
    memset(&cc, 0, sizeof(UA_ClientConfig));
    cc.logging = UA_Log_Stdout_new(UA_LOGLEVEL_ERROR);
    ck_assert_ptr_nonnull(cc.logging);
    UA_String discoveryUrl = UA_STRING("opc.tcp://localhost:4840");

    UA_StatusCode res = UA_Server_registerDiscovery(server, &cc, discoveryUrl,
                                                   UA_STRING_NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
    /* The public cc was already cleared by the failing call. */
} END_TEST

START_TEST(Server_deregisterDiscovery_serverNotStarted_rejected) {
    /* Same guard, mirror function. UA_Server_deregisterDiscovery
     * goes through UA_Server_register(server, cc, true, ...) which
     * checks the same state and returns BADINTERNALERROR. */
    UA_ClientConfig cc;
    memset(&cc, 0, sizeof(UA_ClientConfig));
    cc.logging = UA_Log_Stdout_new(UA_LOGLEVEL_ERROR);
    ck_assert_ptr_nonnull(cc.logging);
    UA_String discoveryUrl = UA_STRING("opc.tcp://localhost:4840");

    UA_StatusCode res = UA_Server_deregisterDiscovery(server, &cc, discoveryUrl);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

static Suite *testSuite(void) {
    TCase *tc = tcase_create("ServerDiscoveryNotStarted");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, Server_registerDiscovery_serverNotStarted_rejected);
    tcase_add_test(tc, Server_deregisterDiscovery_serverNotStarted_rejected);

    Suite *s = suite_create("Server Discovery NotStarted");
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int failed = 0;
    Suite *s = testSuite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
