/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) open62541 contributors */

#include <open62541/transport_generated.h>
#include <open62541/types.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "ua_securechannel.h"

#include "test_helpers.h"

#include <stdlib.h>
#include <string.h>
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

/* These tests exercise the helper/utility surface of the SecureChannel that
 * does not need a network connection manager. They run against a freshly
 * initialised UA_SecureChannel whose SecurityPolicy is the server's built-in
 * SecurityPolicy#None (so setSecurityPolicy succeeds without encryption). */

static UA_Server *server;
static UA_SecureChannel ch;

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
    UA_StatusCode spv = UA_SecureChannel_setSecurityPolicy(
        &ch, getNonePolicy(), &ch.remoteCertificate);
    ck_assert_uint_eq(spv, UA_STATUSCODE_GOOD);
    ch.securityMode = UA_MESSAGESECURITYMODE_NONE;
}

static void
teardown(void) {
    UA_SecureChannel_clear(&ch);
    UA_Server_delete(server);
}

/* ==== UA_SecureChannel_setSecurityMode reject paths ==== */

START_TEST(setSecurityMode_invalidMode_rejected) {
    /* UA_MESSAGESECURITYMODE_INVALID is below the valid range and must be
     * rejected before the SecurityPolicy is even consulted. */
    UA_StatusCode res = UA_SecureChannel_setSecurityMode(
        &ch, UA_MESSAGESECURITYMODE_INVALID);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADSECURITYMODEREJECTED);
    ck_assert_uint_eq(ch.securityMode, UA_MESSAGESECURITYMODE_NONE);
} END_TEST

START_TEST(setSecurityMode_outOfRange_rejected) {
    /* (UA_MessageSecurityMode)99 is above the valid enum range and must be
     * rejected by the same upper-bound check. */
    UA_StatusCode res = UA_SecureChannel_setSecurityMode(
        &ch, (UA_MessageSecurityMode)99);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADSECURITYMODEREJECTED);
    ck_assert_uint_eq(ch.securityMode, UA_MESSAGESECURITYMODE_NONE);
} END_TEST

START_TEST(setSecurityMode_noPolicy_rejected) {
    /* A freshly initialised channel has no SecurityPolicy. The mode setter
     * must reject with BADSECURITYMODEREJECTED and leave the mode unchanged. */
    UA_SecureChannel ch2;
    UA_SecureChannel_init(&ch2);
    UA_StatusCode res = UA_SecureChannel_setSecurityMode(
        &ch2, UA_MESSAGESECURITYMODE_NONE);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADSECURITYMODEREJECTED);
    /* Tear down (no policy was set so clear is just a reset). */
    UA_SecureChannel_clear(&ch2);
} END_TEST

START_TEST(setSecurityMode_nonePolicyWithSign_rejected) {
    /* The #None SecurityPolicy only accepts UA_MESSAGESECURITYMODE_NONE.
     * Setting any other mode must fail. */
    UA_StatusCode res = UA_SecureChannel_setSecurityMode(
        &ch, UA_MESSAGESECURITYMODE_SIGN);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADSECURITYMODEREJECTED);
    ck_assert_uint_eq(ch.securityMode, UA_MESSAGESECURITYMODE_NONE);
} END_TEST

START_TEST(setSecurityMode_noneMode_accepted) {
    /* The positive case: NONE mode is accepted when the policy is #None. */
    UA_StatusCode res = UA_SecureChannel_setSecurityMode(
        &ch, UA_MESSAGESECURITYMODE_NONE);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ch.securityMode, UA_MESSAGESECURITYMODE_NONE);
} END_TEST

/* ==== UA_SecureChannel_setSecurityPolicy reject paths ==== */

START_TEST(setSecurityPolicy_alreadyConfigured_rejected) {
    /* src/ua_securechannel.c:39-42:
     *   UA_CHECK_ERROR(!channel->securityPolicy,
     *                  return UA_STATUSCODE_BADINTERNALERROR, ...);
     * The setup() helper has already called setSecurityPolicy once.
     * A second call must fail with BADINTERNALERROR. None of the
     * existing tests exercises this guard. */
    UA_StatusCode res = UA_SecureChannel_setSecurityPolicy(
        &ch, getNonePolicy(), &ch.remoteCertificate);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
    /* The channel's policy must remain the original (untouched) */
    ck_assert_ptr_nonnull((void*)ch.securityPolicy);
} END_TEST

/* ==== UA_SecureChannel_processHELACK ==== */

START_TEST(processHELACK_happyPath_returnsGood) {
    /* Both sides offer 65535-byte buffers and unlimited message size. The
     * lowest common values are used; the function must return GOOD. */
    UA_TcpAcknowledgeMessage remote;
    memset(&remote, 0, sizeof(remote));
    remote.protocolVersion = 0;
    remote.receiveBufferSize = 65535;
    remote.sendBufferSize = 65535;
    remote.maxMessageSize = 0;       /* "unlimited" */
    remote.maxChunkCount = 0;         /* "unlimited" */

    UA_StatusCode res = UA_SecureChannel_processHELACK(&ch, &remote);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* Remote and local have the same size, so they should be unchanged. */
    ck_assert_uint_eq(ch.config.recvBufferSize, 65535);
    ck_assert_uint_eq(ch.config.sendBufferSize, 65535);
} END_TEST

START_TEST(processHELACK_remoteReceiveCapped_capsSendBuffer) {
    /* The remote receive-buffer (16384) is smaller than our send-buffer
     * (65535), so our send-buffer is capped to the remote's value. */
    UA_TcpAcknowledgeMessage remote;
    memset(&remote, 0, sizeof(remote));
    remote.protocolVersion = 0;
    remote.receiveBufferSize = 16384;
    remote.sendBufferSize = 65535;
    remote.maxMessageSize = 0;
    remote.maxChunkCount = 0;

    UA_StatusCode res = UA_SecureChannel_processHELACK(&ch, &remote);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ch.config.sendBufferSize, 16384);
} END_TEST

START_TEST(processHELACK_remoteSendCapped_capsRecvBuffer) {
    /* Symmetric: the remote send-buffer (16384) is smaller than our
     * recv-buffer, so our recv-buffer is capped to the remote's value. */
    UA_TcpAcknowledgeMessage remote;
    memset(&remote, 0, sizeof(remote));
    remote.protocolVersion = 0;
    remote.receiveBufferSize = 65535;
    remote.sendBufferSize = 16384;
    remote.maxMessageSize = 0;
    remote.maxChunkCount = 0;

    UA_StatusCode res = UA_SecureChannel_processHELACK(&ch, &remote);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ch.config.recvBufferSize, 16384);
} END_TEST

START_TEST(processHELACK_lowProtocolVersion_capsVersion) {
    /* The local protocol version must be capped to the lower of the two. */
    ch.config.protocolVersion = 0;
    UA_TcpAcknowledgeMessage remote;
    memset(&remote, 0, sizeof(remote));
    remote.protocolVersion = 0; /* local is also 0 - check that it stays */
    remote.receiveBufferSize = 65535;
    remote.sendBufferSize = 65535;
    remote.maxMessageSize = 0;
    remote.maxChunkCount = 0;

    UA_StatusCode res = UA_SecureChannel_processHELACK(&ch, &remote);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ch.config.protocolVersion, 0);
} END_TEST

START_TEST(processHELACK_bufferTooSmall_returnsBad) {
    /* The protocol mandates at least 8192-byte buffers. A remote offering
     * only 1024-byte buffers must be rejected with BADINTERNALERROR. */
    UA_TcpAcknowledgeMessage remote;
    memset(&remote, 0, sizeof(remote));
    remote.protocolVersion = 0;
    remote.receiveBufferSize = 1024;
    remote.sendBufferSize = 65535;
    remote.maxMessageSize = 0;
    remote.maxChunkCount = 0;

    UA_StatusCode res = UA_SecureChannel_processHELACK(&ch, &remote);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

START_TEST(processHELACK_maxMessageSizeTooSmall_returnsBad) {
    /* Even if the buffers are large, a maxMessageSize limit below 8192 must
     * cause rejection (the third branch in the 8192-byte check). */
    UA_TcpAcknowledgeMessage remote;
    memset(&remote, 0, sizeof(remote));
    remote.protocolVersion = 0;
    remote.receiveBufferSize = 65535;
    remote.sendBufferSize = 65535;
    remote.maxMessageSize = 4096;   /* below 8192 floor */
    remote.maxChunkCount = 0;

    UA_StatusCode res = UA_SecureChannel_processHELACK(&ch, &remote);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

/* ==== UA_SecureChannel_deleteBuffered ==== */

START_TEST(deleteBuffered_noBuffer_isNoop) {
    /* Fresh channel has no unprocessed buffer and no chunks. deleteBuffered
     * must be a no-op and not crash. */
    UA_SecureChannel_deleteBuffered(&ch);
    /* No asserts needed - the test passes if the call returns. */
} END_TEST

START_TEST(deleteBuffered_unprocessedCopied_clears) {
    /* A copied unprocessed buffer must be cleared. Use a heap-allocated
     * buffer so the inner free() is well-defined. */
    UA_Byte initData[4] = {1, 2, 3, 4};
    UA_ByteString tmp = {4, (UA_Byte*)UA_malloc(4)};
    memcpy(tmp.data, initData, 4);
    ch.unprocessed = tmp;
    ch.unprocessedCopied = true;
    UA_SecureChannel_deleteBuffered(&ch);
    ck_assert_msg(ch.unprocessed.data == NULL, "expected unprocessed.data to be NULL");
    ck_assert_uint_eq(ch.unprocessed.length, 0);
    /* unprocessedCopied is left as-is by the implementation; reset for
     * the next test. */
    ch.unprocessedCopied = false;
} END_TEST

START_TEST(deleteBuffered_unprocessedNotCopied_isNoop) {
    /* When the buffer was not copied, deleteBuffered must leave the
     * pointer alone. Use a heap-allocated buffer to keep teardown safe. */
    UA_Byte initData[4] = {1, 2, 3, 4};
    UA_Byte *data = (UA_Byte*)UA_malloc(4);
    memcpy(data, initData, 4);
    ch.unprocessed.data = data;
    ch.unprocessed.length = 4;
    ch.unprocessedCopied = false;
    UA_SecureChannel_deleteBuffered(&ch);
    ck_assert_ptr_eq((void*)ch.unprocessed.data, (void*)data);
    ck_assert_uint_eq(ch.unprocessed.length, 4);
    /* Clean up ourselves since teardown won't free this (not copied). */
    UA_free(data);
} END_TEST

/* ==== UA_SecureChannel_loadBuffer ==== */

START_TEST(loadBuffer_emptyChannel_usesBuffer) {
    /* When no unprocessed data exists, the new buffer is adopted as-is and
     * marked as not-copied. Use a heap buffer so teardown is safe. */
    UA_Byte *data = (UA_Byte*)UA_malloc(8);
    UA_ByteString buf = {8, data};
    UA_StatusCode res = UA_SecureChannel_loadBuffer(&ch, buf);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq((void*)ch.unprocessed.data, (void*)data);
    ck_assert_uint_eq(ch.unprocessed.length, 8);
    ck_assert(!ch.unprocessedCopied);
    /* Clean up ourselves - teardown won't free this (not copied). */
    UA_free(data);
    ch.unprocessed.data = NULL;
    ch.unprocessed.length = 0;
} END_TEST

START_TEST(loadBuffer_append_reallocates) {
    /* Set up the channel with a heap-allocated copied buffer, then
     * loadBuffer must realloc the existing buffer and append. */
    UA_Byte initData[4] = {1, 2, 3, 4};
    UA_Byte extraData[4] = {5, 6, 7, 8};
    UA_Byte *initial = (UA_Byte*)UA_malloc(4);
    memcpy(initial, initData, 4);
    ch.unprocessed.data = initial;
    ch.unprocessed.length = 4;
    ch.unprocessedCopied = true;

    UA_Byte *extra = (UA_Byte*)UA_malloc(4);
    memcpy(extra, extraData, 4);
    UA_ByteString buf2 = {4, extra};

    UA_StatusCode res = UA_SecureChannel_loadBuffer(&ch, buf2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ch.unprocessed.length, 8);
    ck_assert(ch.unprocessedCopied);
    ck_assert_msg(ch.unprocessed.data[0] == 1 &&
                  ch.unprocessed.data[3] == 4 &&
                  ch.unprocessed.data[4] == 5 &&
                  ch.unprocessed.data[7] == 8,
                  "Appended buffer contents wrong");
    /* teardown's deleteBuffered will free this since unprocessedCopied=true */
    UA_free(extra); /* the buf2 data is referenced but not adopted */
} END_TEST

START_TEST(loadBuffer_emptyBuffer_keepsExisting) {
    /* Existing heap-allocated copied buffer + an empty input. The empty
     * loadBuffer is a no-op realloc that keeps the existing data. */
    UA_Byte initData[4] = {1, 2, 3, 4};
    UA_Byte *initial = (UA_Byte*)UA_malloc(4);
    memcpy(initial, initData, 4);
    ch.unprocessed.data = initial;
    ch.unprocessed.length = 4;
    ch.unprocessedCopied = true;

    UA_ByteString bufEmpty = {0, NULL};
    UA_StatusCode res = UA_SecureChannel_loadBuffer(&ch, bufEmpty);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ch.unprocessed.length, 4);
    /* teardown's deleteBuffered will free this */
} END_TEST

/* ==== UA_SecureChannel_isConnected ==== */

START_TEST(isConnected_closedState_returnsFalse) {
    ch.state = UA_SECURECHANNELSTATE_CLOSED;
    ck_assert(!UA_SecureChannel_isConnected(&ch));
} END_TEST

START_TEST(isConnected_connectedState_returnsTrue) {
    /* OPEN is the canonical "connected" state. */
    ch.state = UA_SECURECHANNELSTATE_OPEN;
    ck_assert(UA_SecureChannel_isConnected(&ch));
} END_TEST

START_TEST(isConnected_closingState_returnsFalse) {
    /* CLOSING is past the upper bound of the connected range. */
    ch.state = UA_SECURECHANNELSTATE_CLOSING;
    ck_assert(!UA_SecureChannel_isConnected(&ch));
} END_TEST

/* ==== UA_SecureChannel_clear on a freshly-init'd channel ==== */

START_TEST(clear_emptyChannel_doesNotCrash) {
    /* The clear function asserts no sessions are attached. Calling it on
     * a freshly-initialised channel must not crash. */
    UA_SecureChannel ch2;
    UA_SecureChannel_init(&ch2);
    UA_SecureChannel_clear(&ch2);
    /* If we get here, the test passes. */
} END_TEST

START_TEST(shutdown_notConnected_isNoop) {
    /* Shutdown on a CLOSED channel must be a no-op (the early-return path
     * guarded by !UA_SecureChannel_isConnected). */
    ch.state = UA_SECURECHANNELSTATE_CLOSED;
    UA_SecureChannel_shutdown(&ch, UA_SHUTDOWNREASON_CLOSE);
    /* State must stay CLOSED. */
    ck_assert_uint_eq(ch.state, UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

static Suite *
testSuite(void) {
    Suite *s = suite_create("SecureChannel unit");

    TCase *tc_mode = tcase_create("setSecurityMode reject paths");
    tcase_add_checked_fixture(tc_mode, setup, teardown);
    tcase_add_test(tc_mode, setSecurityMode_invalidMode_rejected);
    tcase_add_test(tc_mode, setSecurityMode_outOfRange_rejected);
    tcase_add_test(tc_mode, setSecurityMode_noPolicy_rejected);
    tcase_add_test(tc_mode, setSecurityMode_nonePolicyWithSign_rejected);
    tcase_add_test(tc_mode, setSecurityMode_noneMode_accepted);
    tcase_add_test(tc_mode, setSecurityPolicy_alreadyConfigured_rejected);
    suite_add_tcase(s, tc_mode);

    TCase *tc_hel = tcase_create("processHELACK");
    tcase_add_checked_fixture(tc_hel, setup, teardown);
    tcase_add_test(tc_hel, processHELACK_happyPath_returnsGood);
    tcase_add_test(tc_hel, processHELACK_remoteReceiveCapped_capsSendBuffer);
    tcase_add_test(tc_hel, processHELACK_remoteSendCapped_capsRecvBuffer);
    tcase_add_test(tc_hel, processHELACK_lowProtocolVersion_capsVersion);
    tcase_add_test(tc_hel, processHELACK_bufferTooSmall_returnsBad);
    tcase_add_test(tc_hel, processHELACK_maxMessageSizeTooSmall_returnsBad);
    suite_add_tcase(s, tc_hel);

    TCase *tc_buf = tcase_create("deleteBuffered/loadBuffer");
    tcase_add_checked_fixture(tc_buf, setup, teardown);
    tcase_add_test(tc_buf, deleteBuffered_noBuffer_isNoop);
    tcase_add_test(tc_buf, deleteBuffered_unprocessedCopied_clears);
    tcase_add_test(tc_buf, deleteBuffered_unprocessedNotCopied_isNoop);
    tcase_add_test(tc_buf, loadBuffer_emptyChannel_usesBuffer);
    tcase_add_test(tc_buf, loadBuffer_append_reallocates);
    tcase_add_test(tc_buf, loadBuffer_emptyBuffer_keepsExisting);
    suite_add_tcase(s, tc_buf);

    TCase *tc_state = tcase_create("isConnected / clear");
    tcase_add_checked_fixture(tc_state, setup, teardown);
    tcase_add_test(tc_state, isConnected_closedState_returnsFalse);
    tcase_add_test(tc_state, isConnected_connectedState_returnsTrue);
    tcase_add_test(tc_state, isConnected_closingState_returnsFalse);
    tcase_add_test(tc_state, clear_emptyChannel_doesNotCrash);
    tcase_add_test(tc_state, shutdown_notConnected_isNoop);
    suite_add_tcase(s, tc_state);

    return s;
}

int main(void) {
    Suite *s = testSuite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
