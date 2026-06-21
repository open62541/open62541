/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Coverage tests for src/client/ua_client.c that exercise the public
 * UA_ClientConfig_* helpers which are otherwise only touched indirectly
 * by the integration tests. Each test uses only the public API.
 *
 * Focus:
 *   - UA_ClientConfig_setAuthenticationUsername
 *   - UA_ClientConfig_setDefault
 *   - UA_ClientConfig_delete on a non-NULL pointer
 *
 * Note: UA_ClientConfig_copy is intentionally NOT exercised here; the
 * implementation shallow-copies logging/eventLoop and requires the
 * caller to not free both source and destination. The integration
 * tests already cover copy indirectly through UA_Client_newWithConfig.
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "test_helpers.h"
#include "thread_wrapper.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(ClientConfig_setAuthenticationUsername_basic) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_ne(client, NULL);

    UA_StatusCode rv = UA_ClientConfig_setAuthenticationUsername(
        UA_Client_getConfig(client), "alice", "secret");
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);

    UA_ClientConfig *cfg = UA_Client_getConfig(client);
    ck_assert(cfg->userIdentityToken.encoding == UA_EXTENSIONOBJECT_DECODED);
    ck_assert(cfg->userIdentityToken.content.decoded.type ==
              &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]);

    UA_UserNameIdentityToken *token = (UA_UserNameIdentityToken *)
        cfg->userIdentityToken.content.decoded.data;
    ck_assert_ptr_ne(token, NULL);
    UA_String expectedName = UA_STRING("alice");
    UA_ByteString expectedPw = UA_BYTESTRING("secret");
    ck_assert(UA_String_equal(&token->userName, &expectedName));
    ck_assert(UA_ByteString_equal(&token->password, &expectedPw));

    UA_Client_delete(client);
} END_TEST

START_TEST(ClientConfig_setAuthenticationUsername_replaces) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cfg = UA_Client_getConfig(client);

    /* Setting twice must free the first token and install the second. */
    ck_assert_uint_eq(UA_ClientConfig_setAuthenticationUsername(
                          cfg, "first", "pw1"),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ClientConfig_setAuthenticationUsername(
                          cfg, "second", "pw2"),
                     UA_STATUSCODE_GOOD);

    UA_UserNameIdentityToken *token = (UA_UserNameIdentityToken *)
        cfg->userIdentityToken.content.decoded.data;
    UA_String expectedName = UA_STRING("second");
    UA_ByteString expectedPw = UA_BYTESTRING("pw2");
    ck_assert(UA_String_equal(&token->userName, &expectedName));
    ck_assert(UA_ByteString_equal(&token->password, &expectedPw));

    UA_Client_delete(client);
} END_TEST

START_TEST(ClientConfig_setDefault_populatesBasics) {
    UA_ClientConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    UA_StatusCode rv = UA_ClientConfig_setDefault(&cfg);
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);

    /* Default timeout is set. */
    ck_assert(cfg.timeout > 0);
    /* SecureChannel lifetime is set. */
    ck_assert(cfg.secureChannelLifeTime > 0);
    /* Logging is allocated. */
    ck_assert_ptr_ne(cfg.logging, NULL);
    /* EventLoop is allocated. */
    ck_assert_ptr_ne(cfg.eventLoop, NULL);
    ck_assert_uint_eq(cfg.externalEventLoop, false);
    /* ApplicationDescription has a non-empty applicationUri. */
    ck_assert(cfg.clientDescription.applicationUri.length > 0);

    UA_ClientConfig_clear(&cfg);
} END_TEST

START_TEST(ClientConfig_setDefault_preservesCustomValues) {
    /* setDefault must not clobber pre-populated non-zero fields. */
    UA_ClientConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.timeout = 99999;
    cfg.secureChannelLifeTime = 77777;

    UA_StatusCode rv = UA_ClientConfig_setDefault(&cfg);
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(cfg.timeout, 99999);
    ck_assert_uint_eq(cfg.secureChannelLifeTime, 77777);

    UA_ClientConfig_clear(&cfg);
} END_TEST

START_TEST(ClientConfig_delete_nonNull) {
    /* UA_ClientConfig_delete asserts on NULL (defined as UA_assert which
     * expands to assert(3) in debug builds), so we only exercise the
     * non-NULL path. The clear step is exercised separately. */
    UA_ClientConfig *cfg = (UA_ClientConfig *)UA_malloc(sizeof(UA_ClientConfig));
    ck_assert_ptr_ne(cfg, NULL);
    memset(cfg, 0, sizeof(UA_ClientConfig));
    ck_assert_uint_eq(UA_ClientConfig_setDefault(cfg), UA_STATUSCODE_GOOD);
    UA_ClientConfig_delete(cfg);
} END_TEST

START_TEST(ClientConfig_clear_idempotent) {
    /* clear on a fresh zeroed config must succeed without crashing. */
    UA_ClientConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    UA_ClientConfig_clear(&cfg);
    /* After clear, all pointers must be NULL. */
    ck_assert_ptr_eq(cfg.logging, NULL);
    ck_assert_ptr_eq(cfg.eventLoop, NULL);
} END_TEST

static Suite *testSuite_ClientConfigCoverage(void) {
    Suite *s = suite_create("Client Config Coverage");
    TCase *tc = tcase_create("ClientConfig");
    tcase_add_test(tc, ClientConfig_setAuthenticationUsername_basic);
    tcase_add_test(tc, ClientConfig_setAuthenticationUsername_replaces);
    tcase_add_test(tc, ClientConfig_setDefault_populatesBasics);
    tcase_add_test(tc, ClientConfig_setDefault_preservesCustomValues);
    tcase_add_test(tc, ClientConfig_delete_nonNull);
    tcase_add_test(tc, ClientConfig_clear_idempotent);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_ClientConfigCoverage();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_FORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
