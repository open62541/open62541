/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/certificategroup_default.h>

#include <stdlib.h>

#include "certificates.h"
#include "check.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup1(void) {
    running = true;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    UA_ByteString rootCa;
    rootCa.length = ROOT_CERT_DER_LENGTH;
    rootCa.data = ROOT_CERT_DER_DATA;

    UA_ByteString rootCaCrl;
    rootCaCrl.length = ROOT_EMPTY_CRL_PEM_LENGTH;
    rootCaCrl.data = ROOT_EMPTY_CRL_PEM_DATA;

    UA_ByteString intermediateCa;
    intermediateCa.length = INTERMEDIATE_CERT_DER_LENGTH;
    intermediateCa.data = INTERMEDIATE_CERT_DER_DATA;

    UA_ByteString intermediateCaCrl;
    intermediateCaCrl.length = INTERMEDIATE_EMPTY_CRL_PEM_LENGTH;
    intermediateCaCrl.data = INTERMEDIATE_EMPTY_CRL_PEM_DATA;

    /* Load the trustlist */
    size_t trustListSize = 2;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    trustList[0] = intermediateCa;
    trustList[1] = rootCa;

    /* Load the issuerList */
    size_t issuerListSize = 2;
    UA_STACKARRAY(UA_ByteString, issuerList, issuerListSize);
    issuerList[0] = intermediateCa;
    issuerList[1] = rootCa;

    size_t revocationListSize = 2;
    UA_STACKARRAY(UA_ByteString, revocationList, revocationListSize);
    revocationList[0] = rootCaCrl;
    revocationList[1] = intermediateCaCrl;

    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &certificate, &privateKey,
                                                   trustList, trustListSize,
                                                   issuerList, issuerListSize,
                                                   revocationList, revocationListSize);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void setup2(void) {
    running = true;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    UA_ByteString rootCa;
    rootCa.length = ROOT_CERT_DER_LENGTH;
    rootCa.data = ROOT_CERT_DER_DATA;

    UA_ByteString rootCaCrl;
    rootCaCrl.length = ROOT_EMPTY_CRL_PEM_LENGTH;
    rootCaCrl.data = ROOT_EMPTY_CRL_PEM_DATA;

    UA_ByteString intermediateCa;
    intermediateCa.length = INTERMEDIATE_CERT_DER_LENGTH;
    intermediateCa.data = INTERMEDIATE_CERT_DER_DATA;

    UA_ByteString intermediateCaCrl;
    intermediateCaCrl.length = INTERMEDIATE_CRL_PEM_LENGTH;
    intermediateCaCrl.data = INTERMEDIATE_CRL_PEM_DATA;

    /* Load the trustlist */
    size_t trustListSize = 2;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    trustList[0] = intermediateCa;
    trustList[1] = rootCa;

    /* Load the issuerList */
    size_t issuerListSize = 2;
    UA_STACKARRAY(UA_ByteString, issuerList, issuerListSize);
    issuerList[0] = intermediateCa;
    issuerList[1] = rootCa;

    size_t revocationListSize = 2;
    UA_STACKARRAY(UA_ByteString, revocationList, revocationListSize);
    revocationList[0] = rootCaCrl;
    revocationList[1] = intermediateCaCrl;

    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &certificate, &privateKey,
                                                   trustList, trustListSize,
                                                   issuerList, issuerListSize,
                                                   revocationList, revocationListSize);

    /* Set the ApplicationUri used in the certificate */
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void setup3(void) {
    running = true;

    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    UA_ByteString rootCa;
    rootCa.length = ROOT_CERT_DER_LENGTH;
    rootCa.data = ROOT_CERT_DER_DATA;

    UA_ByteString rootCaCrl;
    rootCaCrl.length = ROOT_CRL_PEM_LENGTH;
    rootCaCrl.data = ROOT_CRL_PEM_DATA;

    UA_ByteString intermediateCa;
    intermediateCa.length = INTERMEDIATE_CERT_DER_LENGTH;
    intermediateCa.data = INTERMEDIATE_CERT_DER_DATA;

    UA_ByteString intermediateCaCrl;
    intermediateCaCrl.length = INTERMEDIATE_EMPTY_CRL_PEM_LENGTH;
    intermediateCaCrl.data = INTERMEDIATE_EMPTY_CRL_PEM_DATA;

    /* Load the trustlist */
    size_t trustListSize = 2;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    trustList[0] = intermediateCa;
    trustList[1] = rootCa;

    /* Load the issuerList */
    size_t issuerListSize = 2;
    UA_STACKARRAY(UA_ByteString, issuerList, issuerListSize);
    issuerList[0] = rootCa;
    issuerList[1] = intermediateCa;

    size_t revocationListSize = 2;
    UA_STACKARRAY(UA_ByteString, revocationList, revocationListSize);
    revocationList[0] = rootCaCrl;
    revocationList[1] = intermediateCaCrl;

    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &certificate, &privateKey,
                                                   trustList, trustListSize,
                                                   issuerList, issuerListSize,
                                                   revocationList, revocationListSize);

    /* Set the ApplicationUri used in the certificate */
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

START_TEST(encryption_connect_valid) {
/* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = APPLICATION_CERT_DER_LENGTH;
    certificate.data = APPLICATION_CERT_DER_DATA;
    ck_assert_uint_ne(certificate.length, 0);

    UA_ByteString privateKey;
    privateKey.length = APPLICATION_KEY_DER_LENGTH;
    privateKey.data = APPLICATION_KEY_DER_DATA;
    ck_assert_uint_ne(privateKey.length, 0);

    /* Load the trustlist */
    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Secure client initialization */
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    ck_assert(client != NULL);

    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(encryption_connect_revoked) {
    UA_ByteString certificate;
    certificate.length = APPLICATION_CERT_DER_LENGTH;
    certificate.data = APPLICATION_CERT_DER_DATA;
    ck_assert_uint_ne(certificate.length, 0);

    UA_ByteString privateKey;
    privateKey.length = APPLICATION_KEY_DER_LENGTH;
    privateKey.data = APPLICATION_KEY_DER_DATA;
    ck_assert_uint_ne(privateKey.length, 0);

    /* Load the trustlist */
    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    /* Secure client initialization */
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    ck_assert(client != NULL);

    /* Secure client connect */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_encryption(void) {
    Suite *s = suite_create("Certificate Revocation List");
    TCase *tc_encryption_valid = tcase_create("Certificate Revocation Valid");
    tcase_add_checked_fixture(tc_encryption_valid, setup1, teardown);
    TCase *tc_encryption_revoked = tcase_create("Certificate Revocation Revoked");
    tcase_add_checked_fixture(tc_encryption_revoked, setup2, teardown);
    TCase *tc_encryption_revoked2 = tcase_create("Certificate Revocation Revoked2");
    tcase_add_checked_fixture(tc_encryption_revoked2, setup3, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_encryption_valid, encryption_connect_valid);
    tcase_add_test(tc_encryption_revoked, encryption_connect_revoked);
    tcase_add_test(tc_encryption_revoked2, encryption_connect_revoked);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_encryption_valid);
    suite_add_tcase(s,tc_encryption_revoked);
    suite_add_tcase(s,tc_encryption_revoked2);
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
