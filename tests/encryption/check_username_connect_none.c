/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <open62541/client_config_default.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/server_config_default.h>

#include "ua_server_internal.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include "certificates.h"
#include "thread_wrapper.h"
#include "test_helpers.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin usernamePasswords[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;

    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          NULL, 0, NULL, 0, NULL, 0);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateVerification_AcceptAll(&config->secureChannelPKI);
    UA_AccessControl_default(config, false, NULL, usernamePasswordsSize, usernamePasswords);

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(none_policy_connect) {
    server->config.allowNonePolicyPassword = true;
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(none_policy_connect_fail) {
    server->config.allowNonePolicyPassword = false;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");

    ck_assert_uint_eq(retval, UA_STATUSCODE_BADIDENTITYTOKENINVALID);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_encryption(void) {
    Suite *s = suite_create("NoneClientWithEncryptionServer");
    TCase *tc_encryption = tcase_create("Client without certificate and server with certificate");
    tcase_add_checked_fixture(tc_encryption, setup, teardown);
    tcase_add_test(tc_encryption, none_policy_connect);
    tcase_add_test(tc_encryption, none_policy_connect_fail);
    suite_add_tcase(s,tc_encryption);
    return s;
}

int main(void) {
    Suite *s = testSuite_encryption();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
