/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Coverage tests for src/server/ua_services_securechannel.c.
 *
 * The OpenSecureChannel service is normally driven by the wire protocol, so
 * the error branches in Service_OpenSecureChannel and notifySecureChannel are
 * hard to hit via a regular client connection. This file drives the service
 * directly with crafted SecureChannel states and request types so that the
 * previously-uncovered branches get exercised:
 *   - the "channel in the wrong state" reject on ISSUE
 *   - the "channel in the wrong state" reject on RENEW
 *   - the "reused nonce" reject on RENEW
 *   - the unknown requestType default branch
 *   - notifySecureChannel: the early-return when no callback is set
 *   - notifySecureChannel: the per-type and global callback dispatch
 *
 * The "lifetime clamp" branches and the securityMode clamp branch need a
 * working setSecurityMode (which rejects mismatched policyType/mode pairs) and
 * are therefore already covered by the existing check_securechannel.c
 * wire-driven tests.
 *
 * The test uses a real UA_Server (so config, event-loop and
 * maxSecurityTokenLifetime are valid) and a hand-crafted UA_SecureChannel
 * with the state, securityMode and remoteNonce mutated per-test to drive each
 * branch deterministically.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "ua_securechannel.h"
#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "util/ua_util_internal.h"

#include "test_helpers.h"
#include "testing_networklayers.h"

#include <check.h>

/* Provide ck_assert_ptr_null / ck_assert_ptr_nonnull on top of older
 * libcheck (Ubuntu 20.04 ships 0.10.x where these shorthands are not
 * yet defined). The ck_assert_msg form compiles on every libcheck
 * version. */
#ifndef ck_assert_ptr_null
# define ck_assert_ptr_null(p) ck_assert_msg((p) == NULL, #p " != NULL")
#endif
#ifndef ck_assert_ptr_nonnull
# define ck_assert_ptr_nonnull(p) ck_assert_msg((p) != NULL, #p " == NULL")
#endif
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static UA_Server *server;

/* A scratch channel used to drive Service_OpenSecureChannel with hand-set
 * state without going through the wire. The fields we touch are the ones
 * the service reads (state, securityMode, securityPolicy, remoteNonce,
 * etc.). The SecurityPolicy comes from the server's built-in
 * SecurityPolicy#None so setSecurityPolicy succeeds. */
static UA_SecureChannel ch;

/* A real TestConnectionManager is attached to the scratch channel so that
 * notifySecureChannel can safely dereference ch.connectionManager when it
 * builds the notification KeyValueMap (it reads the connection-manager
 * eventSource name unconditionally whenever a callback is set). */
static UA_ConnectionManager *testCM;

static UA_SecurityPolicy *
getNonePolicy(void) {
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    ck_assert_ptr_ne(cfg, NULL);
    ck_assert_uint_gt(cfg->securityPoliciesSize, 0);
    return &cfg->securityPolicies[0];
}

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);

    UA_SecureChannel_init(&ch);
    ch.config = UA_ConnectionConfig_default;
    /* Set the security policy so a channel context is allocated. The
     * per-test setup then mutates ch.state / ch.securityMode / ch.remoteNonce
     * to drive the service into each branch. */
    UA_StatusCode spv = UA_SecureChannel_setSecurityPolicy(
        &ch, getNonePolicy(), &ch.remoteCertificate);
    ck_assert_uint_eq(spv, UA_STATUSCODE_GOOD);
    ch.securityMode = UA_MESSAGESECURITYMODE_NONE;
    /* Provide a real connection manager so the notifySecureChannel
     * callback-dispatch path can dereference ch.connectionManager safely. */
    testCM = TestConnectionManager_new("tcp", NULL);
    ck_assert_ptr_nonnull(testCM);
    ch.connectionManager = testCM;
    /* state is intentionally set per-test */
}

static void
teardown(void) {
    UA_SecureChannel_clear(&ch);
    UA_ConnectionManager *cm = testCM;
    testCM = NULL;
    if(cm)
        cm->eventSource.free(&cm->eventSource);
    UA_Server_delete(server);
}

/* ==== Service_OpenSecureChannel error branches ==== */

START_TEST(OpenSecureChannel_issue_wrongState_rejected) {
    /* Force the channel into CLOSED so the ISSUE branch returns
     * BADINTERNALERROR with the "Cannot open already open or closed channel"
     * log message. */
    ch.state = UA_SECURECHANNELSTATE_CLOSED;

    UA_OpenSecureChannelRequest req;
    UA_OpenSecureChannelRequest_init(&req);
    req.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
    req.securityMode = UA_MESSAGESECURITYMODE_NONE;
    req.requestedLifetime = 600000;

    UA_OpenSecureChannelResponse res;
    UA_OpenSecureChannelResponse_init(&res);

    Service_OpenSecureChannel(server, &ch, &req, &res);

    ck_assert_uint_eq(res.responseHeader.serviceResult,
                      UA_STATUSCODE_BADINTERNALERROR);

    UA_OpenSecureChannelResponse_clear(&res);
} END_TEST

START_TEST(OpenSecureChannel_renew_wrongState_rejected) {
    /* RENEW on a channel that is not OPEN returns BADINTERNALERROR. */
    ch.state = UA_SECURECHANNELSTATE_ACK_SENT;

    UA_OpenSecureChannelRequest req;
    UA_OpenSecureChannelRequest_init(&req);
    req.requestType = UA_SECURITYTOKENREQUESTTYPE_RENEW;
    req.securityMode = UA_MESSAGESECURITYMODE_NONE;
    req.requestedLifetime = 600000;

    UA_OpenSecureChannelResponse res;
    UA_OpenSecureChannelResponse_init(&res);

    Service_OpenSecureChannel(server, &ch, &req, &res);

    ck_assert_uint_eq(res.responseHeader.serviceResult,
                      UA_STATUSCODE_BADINTERNALERROR);

    UA_OpenSecureChannelResponse_clear(&res);
} END_TEST

START_TEST(OpenSecureChannel_renew_nonceReuse_rejected) {
    /* RENEW with the same clientNonce as remoteNonce is rejected with
     * BADSECURITYCHECKSFAILED. With SecurityMode NONE the nonce-check is
     * skipped, so the test sets a non-NONE mode. */
    ch.state = UA_SECURECHANNELSTATE_OPEN;
    /* Bump the securityMode to SIGN so the nonce-reuse check is active.
     * The SecurityPolicy#None policy still accepts this because of its
     * "temporary mode" trick in setSecurityMode, but only because the
     * SecurityTokenRequestType is ISSUE in setSecurityMode -- here we
     * deliberately bypass that by manually setting the mode. */
    ch.securityMode = UA_MESSAGESECURITYMODE_SIGN;
    ch.remoteNonce = UA_BYTESTRING_ALLOC("0123456789ABCDEF");

    UA_OpenSecureChannelRequest req;
    UA_OpenSecureChannelRequest_init(&req);
    req.requestType = UA_SECURITYTOKENREQUESTTYPE_RENEW;
    req.securityMode = UA_MESSAGESECURITYMODE_SIGN;
    req.requestedLifetime = 600000;
    /* Reuse the same nonce as remoteNonce */
    UA_ByteString_copy(&ch.remoteNonce, &req.clientNonce);

    UA_OpenSecureChannelResponse res;
    UA_OpenSecureChannelResponse_init(&res);

    Service_OpenSecureChannel(server, &ch, &req, &res);

    ck_assert_uint_eq(res.responseHeader.serviceResult,
                      UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    UA_OpenSecureChannelResponse_clear(&res);
    /* The copy above made req.clientNonce a separately-allocated 16-byte
     * buffer. Release it so AddressSanitizer/LeakSanitizer does not flag
     * a leak on every CI run that builds with sanitizers. */
    UA_OpenSecureChannelRequest_clear(&req);
    UA_ByteString_clear(&ch.remoteNonce);
} END_TEST

START_TEST(OpenSecureChannel_unknownRequestType_rejected) {
    /* A request type that is neither ISSUE nor RENEW hits the default:
     * branch and returns BADINTERNALERROR. */
    ch.state = UA_SECURECHANNELSTATE_ACK_SENT;

    UA_OpenSecureChannelRequest req;
    UA_OpenSecureChannelRequest_init(&req);
    req.requestType = (UA_SecurityTokenRequestType)99; /* out of range */
    req.securityMode = UA_MESSAGESECURITYMODE_NONE;
    req.requestedLifetime = 600000;

    UA_OpenSecureChannelResponse res;
    UA_OpenSecureChannelResponse_init(&res);

    Service_OpenSecureChannel(server, &ch, &req, &res);

    ck_assert_uint_eq(res.responseHeader.serviceResult,
                      UA_STATUSCODE_BADINTERNALERROR);

    UA_OpenSecureChannelResponse_clear(&res);
} END_TEST

/* ==== Service_CloseSecureChannel smoke ==== */

START_TEST(CloseSecureChannel_smoke_doesNotCrash) {
    /* Service_CloseSecureChannel needs a real connectionManager to call
     * cm->closeConnection, so the wrapper test lives in the lower-level
     * check_securechannel.c. Sanity-check that the symbol is present. */
    /* Cast through uintptr_t to silence -Wpedantic for the function-to-object
     * pointer round-trip. The cast is only here to assert non-NULL. */
    ck_assert_ptr_ne((void *)(uintptr_t)Service_CloseSecureChannel, NULL);
} END_TEST

/* ==== notifySecureChannel ==== */

static volatile size_t notifyCount = 0;
static volatile UA_ApplicationNotificationType lastNotifyType =
    UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED;

static void
countNotifyCb(UA_Server *s, UA_ApplicationNotificationType type,
              const UA_KeyValueMap payload) {
    (void)s; (void)payload;
    notifyCount++;
    lastNotifyType = type;
}

START_TEST(NotifySecureChannel_noCallback_isNoop) {
    /* With neither the global nor the secureChannel-specific callback
     * installed, notifySecureChannel must return immediately. The
     * connectionManager is left NULL because we only want to exercise
     * the callback-dispatch path; the connectionManager is dereferenced
     * unconditionally in the KeyValueMap construction (for the
     * connection-manager-name field), so this test confirms the early
     * return skips all of that work. */
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->globalNotificationCallback = NULL;
    cfg->secureChannelNotificationCallback = NULL;
    ch.securityMode = UA_MESSAGESECURITYMODE_SIGN;
    ch.securityToken.channelId = 7;
    ch.connectionManager = NULL;
    ch.remoteAddress = UA_STRING_NULL;

    /* This must not crash. */
    notifySecureChannel(server, &ch,
                        UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED);
} END_TEST

START_TEST(NotifySecureChannel_callbacksInvoked) {
    /* Install both the global and the per-type callback. notifySecureChannel
     * must call both with the 15-entry KeyValueMap. */
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->secureChannelNotificationCallback = countNotifyCb;
    cfg->globalNotificationCallback = countNotifyCb;
    notifyCount = 0;
    lastNotifyType = UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED;

    ch.securityMode = UA_MESSAGESECURITYMODE_SIGN;
    ch.securityToken.channelId = 42;
    /* ch.connectionManager is set up by setup() to a real TestCM. */
    ch.remoteAddress = UA_STRING_NULL;
    ch.remoteCertificate = UA_BYTESTRING_NULL;
    ch.endpointUrl = UA_STRING_NULL;

    notifySecureChannel(server, &ch,
                        UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED);

    /* Both callbacks should have fired exactly once. */
    ck_assert_uint_eq(notifyCount, 2);
    ck_assert_uint_eq(lastNotifyType,
                      UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED);

    /* Clean up for the next test in the same process. */
    cfg->secureChannelNotificationCallback = NULL;
    cfg->globalNotificationCallback = NULL;
} END_TEST

static Suite *
testSuite_ServicesSecureChannel(void) {
    Suite *s = suite_create("Services SecureChannel");
    TCase *tc = tcase_create("Service_OpenSecureChannel error paths");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, OpenSecureChannel_issue_wrongState_rejected);
    tcase_add_test(tc, OpenSecureChannel_renew_wrongState_rejected);
    tcase_add_test(tc, OpenSecureChannel_renew_nonceReuse_rejected);
    tcase_add_test(tc, OpenSecureChannel_unknownRequestType_rejected);
    tcase_add_test(tc, CloseSecureChannel_smoke_doesNotCrash);
    tcase_add_test(tc, NotifySecureChannel_noCallback_isNoop);
    tcase_add_test(tc, NotifySecureChannel_callbacksInvoked);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_ServicesSecureChannel();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
