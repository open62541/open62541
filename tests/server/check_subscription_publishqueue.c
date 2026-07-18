/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Coverage tests for src/server/ua_subscription.c that exercise
 * branches which the existing tests/ do not reach.
 *
 * Focus:
 *   - UA_Session_ensurePublishQueueSpace
 *     (src/server/ua_subscription.c:979-1002)
 *
 * The default server config has maxPublishReqPerSession == 0, so
 * the early-return at line 981-982 is reachable without any queue
 * setup. The while-loop at 984-1001 requires a populated response
 * queue and is harder to drive deterministically; covered by
 * the existing check_client_subscriptions tests in a real run.
 *
 * This test focuses on the early-return which is never called
 * from any existing test.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_subscription.h"
#include "test_helpers.h"

#include <check.h>

/* On libcheck < 0.11 (ubuntu-20.04 ships 0.10), ck_assert_ptr_nonnull
 * and ck_assert_ptr_null are missing. Shim them to ck_assert_msg. */
#ifndef ck_assert_ptr_null
# define ck_assert_ptr_null(p) ck_assert_msg((p) == NULL, #p " is not NULL")
#endif
#ifndef ck_assert_ptr_nonnull
# define ck_assert_ptr_nonnull(p) ck_assert_msg((p) != NULL, #p " is NULL")
#endif
#include <stdlib.h>
#include <string.h>

static UA_Server *server;
static UA_Session *session;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
    UA_Server_run_startup(server);

    /* Create a minimal session */
    UA_CreateSessionRequest req;
    UA_CreateSessionRequest_init(&req);
    req.requestedSessionTimeout = UA_UINT32_MAX;
    lockServer(server);
    UA_StatusCode res = UA_Session_create(server, NULL, &req, &session);
    unlockServer(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_nonnull(session);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* ==== UA_Session_ensurePublishQueueSpace ==== */

START_TEST(Session_ensurePublishQueueSpace_zeroLimit_returnsImmediately) {
    /* src/server/ua_subscription.c:981-982:
     *   if(server->config.maxPublishReqPerSession == 0)
     *     return;
     * The default config has maxPublishReqPerSession == 0. The
     * function must be a safe no-op and not touch the session's
     * response queue. None of the existing tests calls
     * UA_Session_ensurePublishQueueSpace directly. */
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    ck_assert_uint_eq(cfg->maxPublishReqPerSession, 0);

    /* The session was just created with an empty response queue. */
    ck_assert_uint_eq(session->responseQueueSize, 0);

    lockServer(server);
    UA_Session_ensurePublishQueueSpace(server, session);
    unlockServer(server);

    /* After the call, the queue is still empty. */
    ck_assert_uint_eq(session->responseQueueSize, 0);
} END_TEST

static Suite *testSuite(void) {
    TCase *tc = tcase_create("SessionPublishQueue");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, Session_ensurePublishQueueSpace_zeroLimit_returnsImmediately);

    Suite *s = suite_create("Server Subscription PublishQueue");
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
