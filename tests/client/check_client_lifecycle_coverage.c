/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Client-lifecycle coverage tests for src/client/ua_client.c and
 * src/client/ua_client_connect.c that exercise the public UA_Client_*
 * paths which the existing tests/ do not reach:
 *
 *   - UA_Client_run_iterate with no EventLoop configured
 *     (src/client/ua_client.c:1081-1136, __UA_Client_startup line 1087)
 *
 *   - UA_Client_getSessionAuthenticationToken without an active session
 *     (src/client/ua_client_connect.c:2372-2390, the
 *      sessionState != CREATED && != ACTIVATED early return)
 *
 *   - UA_Client_activateCurrentSession without a prior connect
 *     (src/client/ua_client_connect.c:2354-2361 calls
 *      activateSessionSync -> activateSessionAsync line 954, which
 *      returns BADSESSIONCLOSED)
 *
 * All three tests are pure unit tests: no server required, fully
 * deterministic.
 *
 * The existing tests/client/check_client_*.c files only exercise
 * UA_Client_run_iterate on a client with a fully configured EventLoop
 * and only call UA_Client_getSessionAuthenticationToken /
 * UA_Client_activateCurrentSession after a successful connect, so the
 * negative branches are uncovered.
 */

/* On libcheck < 0.11 (ubuntu-20.04 ships 0.10), ck_assert_ptr_null /
 * ck_assert_ptr_nonnull are missing. Shim them to ck_assert_msg. */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
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

/* Build a client with no EventLoop configured. Only the logging plugin
 * is set, plus externalEventLoop=true so UA_ClientConfig_clear does not
 * try to free a NULL eventLoop. */
static UA_Client *
client_newWithoutEventLoop(void) {
    UA_ClientConfig cc;
    memset(&cc, 0, sizeof(UA_ClientConfig));
    cc.logging = UA_Log_Stdout_new(UA_LOGLEVEL_WARNING);
    ck_assert_ptr_nonnull(cc.logging);
    cc.externalEventLoop = true;

    UA_Client *client = UA_Client_newWithConfig(&cc);
    if(!client) {
        /* Config cleanup happens via UA_ClientConfig_clear on the
         * shallow-copied config inside the client. With the client
         * construction failed, free the logging plugin by hand. */
        if(cc.logging && cc.logging->clear)
            cc.logging->clear(cc.logging);
    }
    return client;
}

START_TEST(Client_run_iterate_noEventLoop_returnsInternalError) {
    /* src/client/ua_client.c:1087-1090:
     *   UA_CHECK_ERROR(el != NULL, return UA_STATUSCODE_BADINTERNALERROR, ...);
     * The early-return path is reached when UA_Client_run_iterate is
     * called on a client that has no EventLoop configured. */
    UA_Client *client = client_newWithoutEventLoop();
    ck_assert_ptr_nonnull(client);

    UA_StatusCode rv = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADINTERNALERROR);

    UA_Client_disconnect(client); /* must be safe on never-connected */
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_getSessionAuthenticationToken_noSession_returnsBadSessionClosed) {
    /* src/client/ua_client_connect.c:2377-2383:
     *   if(sessionState != CREATED && != ACTIVATED)
     *       return UA_STATUSCODE_BADSESSIONCLOSED;
     * A freshly created client has sessionState == CLOSED. The
     * negative branch is not exercised by any existing test. */
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(client);

    UA_SessionState ss;
    UA_SecureChannelState cs;
    UA_StatusCode conn;
    UA_Client_getState(client, &cs, &ss, &conn);
    ck_assert_uint_eq(ss, UA_SESSIONSTATE_CLOSED);

    UA_NodeId token = UA_NODEID_NULL;
    UA_ByteString nonce = UA_BYTESTRING_NULL;
    UA_StatusCode rv =
        UA_Client_getSessionAuthenticationToken(client, &token, &nonce);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADSESSIONCLOSED);

    /* The output args must not have been written to */
    ck_assert(UA_NodeId_isNull(&token));
    ck_assert_uint_eq(nonce.length, 0);
    ck_assert_ptr_null(nonce.data);

    UA_Client_delete(client);
} END_TEST

START_TEST(Client_activateCurrentSession_notConnected_returnsBadSessionClosed) {
    /* src/client/ua_client_connect.c:958-964 (activateSessionAsync):
     *   if(sessionState != CREATED && != ACTIVATED)
     *       return UA_STATUSCODE_BADSESSIONCLOSED;
     * UA_Client_activateCurrentSession (line 2354) calls
     * activateSessionSync -> activateSessionAsync. The early-return
     * path is not exercised by any existing test. */
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(client);

    UA_SessionState ss;
    UA_Client_getState(client, NULL, &ss, NULL);
    ck_assert_uint_eq(ss, UA_SESSIONSTATE_CLOSED);

    UA_StatusCode rv = UA_Client_activateCurrentSession(client);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADSESSIONCLOSED);

    /* Async variant covers the same dispatch */
    rv = UA_Client_activateCurrentSessionAsync(client);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADSESSIONCLOSED);

    UA_Client_delete(client);
} END_TEST

START_TEST(Client_disconnect_notConnected_isSafe) {
    /* src/client/ua_client_connect.c:2784-2790 (UA_Client_disconnect):
     *   if(sessionState == ACTIVATED) sendCloseSession(client);
     *   cleanupSession(client);
     *   disconnectSecureChannel(client, true);
     *   return UA_STATUSCODE_GOOD;
     * Disconnect on a freshly created client (sessionState == CLOSED)
     * is a no-op for the session branch but still drives
     * disconnectSecureChannel. The public tests only call disconnect
     * after a successful connect. */
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(client);

    UA_SessionState ss;
    UA_SecureChannelState cs;
    UA_Client_getState(client, &cs, &ss, NULL);
    ck_assert_uint_eq(ss, UA_SESSIONSTATE_CLOSED);

    /* Must not crash and must return GOOD */
    UA_StatusCode rv = UA_Client_disconnect(client);
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);

    /* Calling disconnect again is also safe (idempotent) */
    rv = UA_Client_disconnect(client);
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);

    UA_Client_delete(client);
} END_TEST

static Suite *testSuite(void) {
    TCase *tc = tcase_create("ClientLifecycle");
    tcase_add_test(tc, Client_run_iterate_noEventLoop_returnsInternalError);
    tcase_add_test(tc, Client_getSessionAuthenticationToken_noSession_returnsBadSessionClosed);
    tcase_add_test(tc, Client_activateCurrentSession_notConnected_returnsBadSessionClosed);
    tcase_add_test(tc, Client_disconnect_notConnected_isSafe);

    Suite *s = suite_create("Client Lifecycle Coverage");
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
