/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/create_certificate.h>

#include "ua_server_internal.h"

#include <check.h>
#include "test_helpers.h"
#include "certificates.h"

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
#include "mp_printf.h"
#define TEST_PATH_MAX 256
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

UA_Server *server;

static void setup(void) {
    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    server = UA_Server_newForUnitTestWithSecurityPolicies(4840, &certificate, &privateKey,
                                                          NULL, 0, NULL, 0, NULL, 0);
    ck_assert(server != NULL);
}

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
static void setup2(void) {
    /* Load certificate and private key */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    char storePathDir[TEST_PATH_MAX];
    getcwd(storePathDir, TEST_PATH_MAX - 4);
    mp_snprintf(storePathDir, TEST_PATH_MAX, "%s/pki", storePathDir);

    const UA_String storePath = UA_STRING(storePathDir);
    server =
        UA_Server_newForUnitTestWithSecurityPolicies_Filestore(4840, &certificate,
                                                               &privateKey, storePath);

    /* Reset the trust list for each test case.
     * This is necessary so that all of the old certificates
     * from previous test cases are deleted from the PKI file store */
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    server->config.secureChannelPKI.setTrustList(&server->config.secureChannelPKI, &trustList);
    UA_TrustListDataType_clear(&trustList);

    ck_assert(server != NULL);
}
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

static void teardown(void) {
    UA_Server_delete(server);
}

static void generateCertificate(UA_ByteString *certificate, UA_ByteString *privKey) {
    UA_String subject[3] = {UA_STRING_STATIC("C=DE"),
                            UA_STRING_STATIC("O=SampleOrganization"),
                            UA_STRING_STATIC("CN=Open62541Server@localhost")};
    UA_UInt32 lenSubject = 3;
    UA_String subjectAltName[2]= {
        UA_STRING_STATIC("DNS:localhost"),
        UA_STRING_STATIC("URI:urn:open62541.unconfigured.application")
    };
    UA_UInt32 lenSubjectAltName = 2;
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_UInt16 expiresIn = 14;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
                             (void *)&expiresIn, &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 keyLength = 2048;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
                             (void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);
    UA_CreateCertificate(UA_Log_Stdout, subject, lenSubject, subjectAltName,
                         lenSubjectAltName, UA_CERTIFICATEFORMAT_DER, kvm,
                         privKey, certificate);

    UA_KeyValueMap_delete(kvm);
}

START_TEST(add_ca_certificate_trustlist) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ByteString trustedCertificates[2] = {0};
    UA_ByteString trustedCrls[2] = {0};

    trustedCertificates[0].length = ROOT_CERT_DER_LENGTH;
    trustedCertificates[0].data = ROOT_CERT_DER_DATA;

    trustedCrls[0].length = ROOT_EMPTY_CRL_PEM_LENGTH;
    trustedCrls[0].data = ROOT_EMPTY_CRL_PEM_DATA;

    trustedCertificates[1].length = INTERMEDIATE_CERT_DER_LENGTH;
    trustedCertificates[1].data = INTERMEDIATE_CERT_DER_DATA;

    trustedCrls[1].length = INTERMEDIATE_EMPTY_CRL_PEM_LENGTH;
    trustedCrls[1].data = INTERMEDIATE_EMPTY_CRL_PEM_DATA;


    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);

    UA_StatusCode retval =
            UA_Server_addCertificates(server, defaultApplicationGroup, trustedCertificates, 2,
                                      trustedCrls, 2, true, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    retval = config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &trustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustList.trustedCertificatesSize, 2);
    ck_assert_uint_eq(trustList.issuerCertificatesSize, 0);
    ck_assert_uint_eq(trustList.trustedCrlsSize, 2);
    ck_assert_uint_eq(trustList.issuerCrlsSize, 0);

    UA_TrustListDataType_clear(&trustList);
}
END_TEST

START_TEST(add_ca_certificate_issuerlist) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ByteString issuerCertificates[2] = {0};
    UA_ByteString issuerCrls[2] = {0};

    issuerCertificates[0].length = ROOT_CERT_DER_LENGTH;
    issuerCertificates[0].data = ROOT_CERT_DER_DATA;

    issuerCrls[0].length = ROOT_EMPTY_CRL_PEM_LENGTH;
    issuerCrls[0].data = ROOT_EMPTY_CRL_PEM_DATA;

    issuerCertificates[1].length = INTERMEDIATE_CERT_DER_LENGTH;
    issuerCertificates[1].data = INTERMEDIATE_CERT_DER_DATA;

    issuerCrls[1].length = INTERMEDIATE_EMPTY_CRL_PEM_LENGTH;
    issuerCrls[1].data = INTERMEDIATE_EMPTY_CRL_PEM_DATA;


    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);

    UA_StatusCode retval =
            UA_Server_addCertificates(server, defaultApplicationGroup, issuerCertificates, 2,
                                      issuerCrls, 2, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    retval = config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &trustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustList.trustedCertificatesSize, 0);
    ck_assert_uint_eq(trustList.issuerCertificatesSize, 2);
    ck_assert_uint_eq(trustList.trustedCrlsSize, 0);
    ck_assert_uint_eq(trustList.issuerCrlsSize, 2);

    UA_TrustListDataType_clear(&trustList);
}
END_TEST

START_TEST(remove_certificate_trustlist) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ByteString trustedCertificates[2] = {0};
    UA_ByteString trustedCrls[2] = {0};

    trustedCertificates[0].length = ROOT_CERT_DER_LENGTH;
    trustedCertificates[0].data = ROOT_CERT_DER_DATA;

    trustedCrls[0].length = ROOT_EMPTY_CRL_PEM_LENGTH;
    trustedCrls[0].data = ROOT_EMPTY_CRL_PEM_DATA;

    trustedCertificates[1].length = INTERMEDIATE_CERT_DER_LENGTH;
    trustedCertificates[1].data = INTERMEDIATE_CERT_DER_DATA;

    trustedCrls[1].length = INTERMEDIATE_EMPTY_CRL_PEM_LENGTH;
    trustedCrls[1].data = INTERMEDIATE_EMPTY_CRL_PEM_DATA;


    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);

    UA_StatusCode retval =
            UA_Server_addCertificates(server, defaultApplicationGroup, trustedCertificates, 2,
                                      trustedCrls, 2, true, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_removeCertificates(server, defaultApplicationGroup,
                                          trustedCertificates, 2, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    retval = config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &trustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustList.trustedCertificatesSize, 0);
    ck_assert_uint_eq(trustList.issuerCertificatesSize, 0);
    ck_assert_uint_eq(trustList.trustedCrlsSize, 0);
    ck_assert_uint_eq(trustList.issuerCrlsSize, 0);

    UA_TrustListDataType_clear(&trustList);
}
END_TEST

START_TEST(remove_certificate_issuerlist) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ByteString issuerCertificates[2] = {0};
    UA_ByteString issuerCrls[2] = {0};

    issuerCertificates[0].length = ROOT_CERT_DER_LENGTH;
    issuerCertificates[0].data = ROOT_CERT_DER_DATA;

    issuerCrls[0].length = ROOT_EMPTY_CRL_PEM_LENGTH;
    issuerCrls[0].data = ROOT_EMPTY_CRL_PEM_DATA;

    issuerCertificates[1].length = INTERMEDIATE_CERT_DER_LENGTH;
    issuerCertificates[1].data = INTERMEDIATE_CERT_DER_DATA;

    issuerCrls[1].length = INTERMEDIATE_EMPTY_CRL_PEM_LENGTH;
    issuerCrls[1].data = INTERMEDIATE_EMPTY_CRL_PEM_DATA;


    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);

    UA_StatusCode retval =
            UA_Server_addCertificates(server, defaultApplicationGroup, issuerCertificates, 2,
                                      issuerCrls, 2, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_removeCertificates(server, defaultApplicationGroup,
                                          issuerCertificates, 2, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    retval = config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &trustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustList.trustedCertificatesSize, 0);
    ck_assert_uint_eq(trustList.issuerCertificatesSize, 0);
    ck_assert_uint_eq(trustList.trustedCrlsSize, 0);
    ck_assert_uint_eq(trustList.issuerCrlsSize, 0);

    UA_TrustListDataType_clear(&trustList);
}
END_TEST

START_TEST(add_application_certificate_trustlist) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_ByteString trustedCertificates[1] = {0};

    trustedCertificates[0].length = APPLICATION_CERT_DER_LENGTH;
    trustedCertificates[0].data = APPLICATION_CERT_DER_DATA;

    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);

    UA_StatusCode retval =
            UA_Server_addCertificates(server, defaultApplicationGroup, trustedCertificates, 1,
                                      NULL, 0, true, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    retval = config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &trustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustList.trustedCertificatesSize, 1);
    ck_assert_uint_eq(trustList.issuerCertificatesSize, 0);
    ck_assert_uint_eq(trustList.trustedCrlsSize, 0);
    ck_assert_uint_eq(trustList.issuerCrlsSize, 0);

    UA_TrustListDataType_clear(&trustList);
}
END_TEST

static Suite* testSuite_create_certificate(void) {
    Suite *s = suite_create("Update Trustlist");
    TCase *tc_cert = tcase_create("Update Trustlist");
    tcase_add_checked_fixture(tc_cert, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert, add_ca_certificate_trustlist);
    tcase_add_test(tc_cert, add_ca_certificate_issuerlist);
    tcase_add_test(tc_cert, remove_certificate_trustlist);
    tcase_add_test(tc_cert, remove_certificate_issuerlist);
    tcase_add_test(tc_cert, add_application_certificate_trustlist);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_cert);

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
    TCase *tc_cert_filestore = tcase_create("Update Certificate Filestore");
    tcase_add_checked_fixture(tc_cert_filestore, setup2, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert_filestore, add_ca_certificate_trustlist);
    tcase_add_test(tc_cert_filestore, add_ca_certificate_issuerlist);
    tcase_add_test(tc_cert_filestore, remove_certificate_trustlist);
    tcase_add_test(tc_cert_filestore, remove_certificate_issuerlist);
    tcase_add_test(tc_cert_filestore, add_application_certificate_trustlist);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_cert_filestore);
#endif /* defined(__linux__) || defined(UA_ARCHITECTURE_WIN32) */

    return s;
}

int main(void) {
    Suite *s = testSuite_create_certificate();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
