/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include <stdlib.h>

#include "certificates.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;
static UA_StatusCode forcedVerifyStatus;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static UA_StatusCode
forceCertVerifyStatus(UA_CertificateGroup *certGroup,
                      const UA_ByteString *certificate) {
    (void)certGroup;
    (void)certificate;
    return forcedVerifyStatus;
}

static void
setup(void) {
    running = true;

    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    size_t trustListSize = 0;
    UA_ByteString *trustList = NULL;
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;
    size_t revocationListSize = 0;
    UA_ByteString *revocationList = NULL;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          trustList, trustListSize,
                                                          issuerList, issuerListSize,
                                                          revocationList, revocationListSize);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);
    config->secureChannelPKI.verifyCertificate = forceCertVerifyStatus;

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void
teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static UA_Client *
newSecureClient(void) {
    UA_ByteString certificate;
    certificate.length = APPLICATION_CERT_DER_LENGTH;
    certificate.data = APPLICATION_CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = APPLICATION_KEY_DER_LENGTH;
    privateKey.data = APPLICATION_KEY_DER_DATA;

    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);

    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:unconfigured:application");
    return client;
}

START_TEST(testOpenSecureChannelCertificateFailuresHidden) {
    static const UA_StatusCode certErrors[] = {
        UA_STATUSCODE_BADCERTIFICATEINVALID,
        UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE,
        UA_STATUSCODE_BADCERTIFICATEPOLICYCHECKFAILED,
        UA_STATUSCODE_BADCERTIFICATEUNTRUSTED,
        UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN,
        UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN,
        UA_STATUSCODE_BADCERTIFICATEREVOKED,
        UA_STATUSCODE_BADCERTIFICATEISSUERREVOKED
    };

    for(size_t i = 0; i < (sizeof(certErrors) / sizeof(certErrors[0])); i++) {
        forcedVerifyStatus = certErrors[i];

        UA_Client *client = newSecureClient();
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

        ck_assert_msg(retval == UA_STATUSCODE_BADSECURITYCHECKSFAILED,
                      "verifyCertificate returned %s, but client connect returned %s",
                      UA_StatusCode_name(certErrors[i]), UA_StatusCode_name(retval));

        UA_Client_delete(client);
    }
}
END_TEST

static Suite *
testSuite_create(void) {
    Suite *s = suite_create("Certificate Validation Client Response");
    TCase *tc = tcase_create("OpenSecureChannel Certificate Failures Hidden");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, testOpenSecureChannelCertificateFailuresHidden);
    suite_add_tcase(s, tc);
    return s;
}

int
main(void) {
    Suite *s = testSuite_create();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
