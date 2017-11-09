/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <src_generated/ua_types_generated.h>

#include "testing_policy.h"
#include "ua_securechannel.h"

#include "check.h"

#define UA_BYTESTRING_STATIC(s) {sizeof(s)-1, (UA_Byte*)s}

UA_SecureChannel testChannel;
UA_ByteString dummyCertificate = UA_BYTESTRING_STATIC("DUMMY CERTIFICATE DUMMY CERTIFICATE DUMMY CERTIFICATE");
UA_SecurityPolicy dummyPolicy;


funcs_called fCalled;

static void
setup_secureChannel(void) {
    TestingPolicy(&dummyPolicy, dummyCertificate, &fCalled);
    UA_SecureChannel_init(&testChannel, &dummyPolicy, &dummyCertificate);
}

static void
teardown_secureChannel(void) {
    UA_SecureChannel_deleteMembersCleanup(&testChannel);
    dummyPolicy.deleteMembers(&dummyPolicy);
}

static void
setup_funcs_called(void) {
    memset(&fCalled, 0, sizeof(struct funcs_called));
}

static void
teardown_funcs_called(void) {
    memset(&fCalled, 0, sizeof(struct funcs_called));
}

/*
static void
setup_dummyPolicy(void) {
    TestingPolicy(&dummyPolicy, dummyCertificate, &fCalled);
}

static void
teardown_dummyPolicy(void) {
    dummyPolicy.deleteMembers(&dummyPolicy);
}*/

START_TEST(SecureChannel_initAndDelete)
    {
        TestingPolicy(&dummyPolicy, dummyCertificate, &fCalled);
        UA_StatusCode retval;

        UA_SecureChannel channel;
        retval = UA_SecureChannel_init(&channel, &dummyPolicy, &dummyCertificate);

        ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode to be good");
        ck_assert_msg(channel.state == UA_SECURECHANNELSTATE_FRESH, "Expected state to be fresh");
        ck_assert_msg(fCalled.newContext, "Expected newContext to have been called");
        ck_assert_msg(fCalled.makeCertificateThumbprint, "Expected makeCertificateThumbprint to have been called");
        ck_assert_msg(channel.securityPolicy == &dummyPolicy, "SecurityPolicy not set correctly");

        UA_SecureChannel_deleteMembersCleanup(&channel);
        ck_assert_msg(fCalled.deleteContext, "Expected deleteContext to have been called");
    }
END_TEST

START_TEST(SecureChannel_initAndDelete_invalidParameters)
    {
        UA_StatusCode retval = UA_SecureChannel_init(NULL, NULL, NULL);
        ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected init to fail");

        UA_SecureChannel channel;
        retval = UA_SecureChannel_init(&channel, &dummyPolicy, NULL);
        ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected init to fail");

        retval = UA_SecureChannel_init(&channel, NULL, &dummyCertificate);
        ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected init to fail");

        retval = UA_SecureChannel_init(NULL, &dummyPolicy, &dummyCertificate);
        ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected init to fail");

        UA_SecureChannel_deleteMembersCleanup(NULL);
    }
END_TEST


START_TEST(SecureChannel_generateNewKeys)
    {
        UA_StatusCode retval = UA_SecureChannel_generateNewKeys(&testChannel);

        ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected Statuscode to be good");
        ck_assert_msg(fCalled.generateKey, "Expected generateKey to have been called");
        ck_assert_msg(fCalled.setLocalSymEncryptingKey, "Expected setLocalSymEncryptingKey to have been called");
        ck_assert_msg(fCalled.setLocalSymSigningKey, "Expected setLocalSymSigningKey to have been called");
        ck_assert_msg(fCalled.setLocalSymIv, "Expected setLocalSymIv to have been called");
        ck_assert_msg(fCalled.setRemoteSymEncryptingKey, "Expected setRemoteSymEncryptingKey to have been called");
        ck_assert_msg(fCalled.setRemoteSymSigningKey, "Expected setRemoteSymSigningKey to have been called");
        ck_assert_msg(fCalled.setRemoteSymIv, "Expected setRemoteSymIv to have been called");

        retval = UA_SecureChannel_generateNewKeys(NULL);
        ck_assert_msg(retval != UA_STATUSCODE_GOOD, "Expected failure on NULL pointer");
    }
END_TEST

START_TEST(SecureChannel_revolveTokens)
    {
        // Fake that no token was issued by setting 0
        testChannel.nextSecurityToken.tokenId = 0;
        UA_StatusCode retval = UA_SecureChannel_revolveTokens(&testChannel);
        ck_assert_msg(retval == UA_STATUSCODE_BADSECURECHANNELTOKENUNKNOWN,
                      "Expected failure because tokenId 0 signifies that no token was issued");

        // Fake an issued token by setting an id
        testChannel.nextSecurityToken.tokenId = 10;
        retval = UA_SecureChannel_revolveTokens(&testChannel);
        ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected function to return GOOD");
        ck_assert_msg(fCalled.generateKey,
                      "Expected generateKey to be called because new keys need to be generated,"
                          "when switching to the next token.");

        UA_ChannelSecurityToken testToken;
        UA_ChannelSecurityToken_init(&testToken);

        ck_assert_msg(memcmp(&testChannel.nextSecurityToken, &testToken, sizeof(UA_ChannelSecurityToken)) == 0,
                     "Expected the next securityToken to be freshly initialized");
        ck_assert_msg(testChannel.securityToken.tokenId == 10, "Expected token to have been copied");
    }
END_TEST

static Suite *
testSuite_SecureChannel(void) {
    Suite *s = suite_create("SecureChannel");

    TCase *tc_initAndDelete = tcase_create("Initialize and delete Securechannel");
    tcase_add_checked_fixture(tc_initAndDelete, setup_funcs_called, teardown_funcs_called);
    tcase_add_test(tc_initAndDelete, SecureChannel_initAndDelete);
    tcase_add_test(tc_initAndDelete, SecureChannel_initAndDelete_invalidParameters);
    suite_add_tcase(s, tc_initAndDelete);

    TCase *tc_generateNewKeys = tcase_create("Test generateNewKeys function");
    tcase_add_checked_fixture(tc_generateNewKeys, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_generateNewKeys, setup_secureChannel, teardown_secureChannel);
    tcase_add_test(tc_generateNewKeys, SecureChannel_generateNewKeys);
    suite_add_tcase(s, tc_generateNewKeys);

    TCase *tc_revolveTokens = tcase_create("Test revolveTokens function");
    tcase_add_checked_fixture(tc_revolveTokens, setup_funcs_called, teardown_funcs_called);
    tcase_add_checked_fixture(tc_revolveTokens, setup_secureChannel, teardown_secureChannel);
    tcase_add_test(tc_revolveTokens, SecureChannel_revolveTokens);
    suite_add_tcase(s, tc_revolveTokens);

    return s;
}

int
main(void) {
    Suite *s = testSuite_SecureChannel();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_VERBOSE);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
