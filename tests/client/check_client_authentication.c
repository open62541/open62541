/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/client_config_default.h>
#include <open62541/server_config_default.h>

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/server.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "certificates.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
    UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;

    /* Load server certificate and private key */
    UA_ByteString certificate;
    certificate.length = SERVER_CERT_DER_LENGTH;
    certificate.data = SERVER_CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = SERVER_KEY_DER_LENGTH;
    privateKey.data = SERVER_KEY_DER_DATA;

    /* Load client certificate for authentication */
    UA_ByteString certificate_client_auth;
    certificate_client_auth.length = CLIENT_CERT_AUTH_DER_LENGTH;
    certificate_client_auth.data = CLIENT_CERT_AUTH_DER_DATA;

    /* Add client certificate to the trust list */
    size_t trustListSize = 1;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    trustList[0] = certificate_client_auth;
    size_t issuerListSize = 0;
    UA_ByteString *issuerList = NULL;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          trustList, trustListSize,
                                                          issuerList, issuerListSize,
                                                          revocationList, revocationListSize);
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_CertificateVerification_AcceptAll(&config->secureChannelPKI);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Client_connect_certificate) {
    /* Load client certificate and private key for the SecureChannel */
    UA_ByteString certificate;
    certificate.length = CLIENT_CERT_DER_LENGTH;
    certificate.data = CLIENT_CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = CLIENT_KEY_DER_LENGTH;
    privateKey.data = CLIENT_KEY_DER_DATA;

    /* Load client certificate and private key for authentication */
    UA_ByteString certificateAuth;
    certificateAuth.length = CLIENT_CERT_AUTH_DER_LENGTH;
    certificateAuth.data = CLIENT_CERT_AUTH_DER_DATA;

    UA_ByteString privateKeyAuth;
    privateKeyAuth.length = CLIENT_KEY_AUTH_DER_LENGTH;
    privateKeyAuth.data = CLIENT_KEY_AUTH_DER_DATA;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);

    /* Set securityMode and securityPolicyUri */
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    cc->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");

    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    UA_CertificateVerification_AcceptAll(&cc->certificateVerification);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&cc->clientDescription.applicationUri);
    cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");

    UA_ClientConfig_setAuthenticationCert(cc, certificateAuth, privateKeyAuth);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_connect_invalid_certificate) {
        /* Load client certificate and private key for the SecureChannel */
        UA_ByteString certificate;
        certificate.length = CLIENT_CERT_DER_LENGTH;
        certificate.data = CLIENT_CERT_DER_DATA;

        UA_ByteString privateKey;
        privateKey.length = CLIENT_KEY_DER_LENGTH;
        privateKey.data = CLIENT_KEY_DER_DATA;

        /* Load client certificate and private key for authentication */
        UA_ByteString certificateAuth;
        certificateAuth.length = CLIENT_CERT_DER_LENGTH;
        certificateAuth.data = CLIENT_CERT_DER_DATA;

        UA_ByteString privateKeyAuth;
        privateKeyAuth.length = CLIENT_KEY_DER_LENGTH;
        privateKeyAuth.data = CLIENT_KEY_DER_DATA;

        UA_Client *client = UA_Client_newForUnitTest();
        UA_ClientConfig *cc = UA_Client_getConfig(client);

        /* Set securityMode and securityPolicyUri */
        cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
        cc->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");

        UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                             NULL, 0, NULL, 0);
        UA_CertificateVerification_AcceptAll(&cc->certificateVerification);

        /* Set the ApplicationUri used in the certificate */
        UA_String_clear(&cc->clientDescription.applicationUri);
        cc->clientDescription.applicationUri = UA_STRING_ALLOC("urn:open62541.server.application");

        UA_ClientConfig_setAuthenticationCert(cc, certificateAuth, privateKeyAuth);
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

        /* openssl v.3 returns a different exit code than other versions. */
        //ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATEUNTRUSTED);
        ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client with Authentication");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_connect_certificate);
    tcase_add_test(tc_client, Client_connect_invalid_certificate);
    suite_add_tcase(s,tc_client);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
