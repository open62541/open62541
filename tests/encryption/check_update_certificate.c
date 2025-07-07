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

START_TEST(update_certificate) {
    UA_ByteString newCertificate = UA_BYTESTRING_NULL;
    UA_ByteString newPrivateKey = UA_BYTESTRING_NULL;

    generateCertificate(&newCertificate, &newPrivateKey);

    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId certTypRsaSha256 = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);

    UA_StatusCode retval =
            UA_Server_updateCertificate(server, defaultApplicationGroup, certTypRsaSha256,
                               newCertificate, &newPrivateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&newCertificate);
    UA_ByteString_clear(&newPrivateKey);
}
END_TEST

START_TEST(update_certificate_wrongKey) {
    UA_ByteString newCertificate = UA_BYTESTRING_NULL;
    UA_ByteString newPrivateKey = UA_BYTESTRING_NULL;

    generateCertificate(&newCertificate, &newPrivateKey);

    UA_ByteString wrongPrivateKey;
    wrongPrivateKey.length = KEY_DER_LENGTH;
    wrongPrivateKey.data = KEY_DER_DATA;

    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId certTypRsaSha256 = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);

    UA_StatusCode retval =
            UA_Server_updateCertificate(server, defaultApplicationGroup, certTypRsaSha256,
                                        newCertificate, &wrongPrivateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTSUPPORTED);

    UA_ByteString_clear(&newCertificate);
    UA_ByteString_clear(&newPrivateKey);
}
END_TEST

START_TEST(update_certificate_noKey) {
    UA_ByteString newCertificate;
    newCertificate.length = CERT_DER_LENGTH;
    newCertificate.data = CERT_DER_DATA;

    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId certTypRsaSha256 = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);

    UA_StatusCode retval =
            UA_Server_updateCertificate(server, defaultApplicationGroup, certTypRsaSha256,
                                        newCertificate, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

static Suite* testSuite_create_certificate(void) {
    Suite *s = suite_create("Update Certificate");
    TCase *tc_cert = tcase_create("Update Certificate");
    tcase_add_checked_fixture(tc_cert, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert, update_certificate);
    tcase_add_test(tc_cert, update_certificate_wrongKey);
    tcase_add_test(tc_cert, update_certificate_noKey);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_cert);

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
    TCase *tc_cert_filestore = tcase_create("Update Certificate Filestore");
    tcase_add_checked_fixture(tc_cert_filestore, setup2, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert_filestore, update_certificate);
    tcase_add_test(tc_cert_filestore, update_certificate_wrongKey);
    tcase_add_test(tc_cert_filestore, update_certificate_noKey);
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
