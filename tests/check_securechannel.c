/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>

#include "ua_securechannel.h"
#include "ua_securitypolicy_none.h"

#include "check.h"
#include "ua_types_generated_handling.h"
#include "ua_log_stdout.h"

#define UA_BYTESTRING_STATIC(s) {sizeof(s)-1, (UA_Byte*)s}

UA_SecureChannel testChannel;
UA_ByteString dummyCertificate = UA_BYTESTRING_STATIC("DUMMY CERTIFICATE DUMMY CERTIFICATE DUMMY CERTIFICATE");
UA_SecurityPolicy dummyPolicy;

/*
static void
setup(void) {
    UA_SecureChannel_init(&testChannel, &dummyPolicy, &dummyCertificate);
}

static void
teardown(void) {
    UA_SecureChannel_deleteMembersCleanup(&testChannel);
}*/

static void
setup_dummyPolicy(void) {
    UA_SecurityPolicy_Dummy(&dummyPolicy);
}

static void
teardown_dummyPolicy(void) {
    UA_SecurityPolicy_Dummy_deleteMembers(&dummyPolicy);
}

START_TEST(SecureChannel_initAndDelete)
    {
        UA_SecurityPolicy_None(&dummyPolicy, dummyCertificate, UA_Log_Stdout);
        UA_StatusCode retval;

        UA_SecureChannel channel;
        retval = UA_SecureChannel_init(&channel, &dummyPolicy, &dummyCertificate);

        ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Expected StatusCode to be good");
        ck_assert_msg(channel.state == UA_SECURECHANNELSTATE_FRESH, "Expected state to be fresh");
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

/*
START_TEST(SecureChannel_generateNewKeys)
    {

    }
END_TEST*/

static Suite *
testSuite_SecureChannel(void) {
    Suite *s = suite_create("SecureChannel");

    TCase *tc_initAndDelete = tcase_create("Initialize and delete Securechannel");
    tcase_add_test(tc_initAndDelete, SecureChannel_initAndDelete);
    tcase_add_test(tc_initAndDelete, SecureChannel_initAndDelete_invalidParameters);
    suite_add_tcase(s, tc_initAndDelete);
    return s;
}

int
main(void) {
    Suite *s = testSuite_SecureChannel();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
