/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/create_certificate.h>

#include <check.h>
#include "test_helpers.h"

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>
#endif

UA_Server *server;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(certificate_generation) {
    UA_ByteString derPrivKey = UA_BYTESTRING_NULL;
    UA_ByteString derCert = UA_BYTESTRING_NULL;
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
    UA_StatusCode status = UA_CreateCertificate(
        UA_Log_Stdout, subject, lenSubject, subjectAltName, lenSubjectAltName,
        UA_CERTIFICATEFORMAT_DER, kvm, &derPrivKey, &derCert);
    UA_KeyValueMap_delete(kvm);
    ck_assert(status == UA_STATUSCODE_GOOD);
    ck_assert(derPrivKey.length > 0);
    ck_assert(derCert.length > 0);

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
    mbedtls_x509_crt parsed;
    mbedtls_x509_crt_init(&parsed);
    ck_assert_int_eq(mbedtls_x509_crt_parse_der(&parsed, derCert.data,
                                                derCert.length), 0);
    ck_assert_uint_eq(parsed.serial.len, 16);
    ck_assert(!mbedtls_x509_crt_has_ext_type(&parsed,
                                             MBEDTLS_X509_EXT_BASIC_CONSTRAINTS) ||
              !mbedtls_x509_crt_get_ca_istrue(&parsed));
    ck_assert_int_ne(mbedtls_x509_crt_check_key_usage(
                         &parsed, MBEDTLS_X509_KU_KEY_CERT_SIGN), 0);
    ck_assert_int_ne(mbedtls_x509_crt_check_key_usage(
                         &parsed, MBEDTLS_X509_KU_CRL_SIGN), 0);
    mbedtls_x509_crt_free(&parsed);
#endif

    UA_ServerConfig *config = UA_Server_getConfig(server);
    status = UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, &derCert, &derPrivKey,
                                                            NULL, 0, NULL, 0, NULL, 0);
    config->tcpReuseAddr = true;
    ck_assert(status == UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&derCert);
    UA_ByteString_clear(&derPrivKey);
}
END_TEST

START_TEST(certificate_generation_rejects_malformed_names) {
    UA_String validSubject = UA_STRING_STATIC("CN=localhost");
    UA_String validSubjectAltName = UA_STRING_STATIC("DNS:localhost");
    UA_String malformed = {1, NULL};
    UA_Byte keySentinelData = 0x11;
    UA_Byte certSentinelData = 0x22;
    UA_ByteString privateKey = {1, &keySentinelData};
    UA_ByteString certificate = {1, &certSentinelData};

    UA_StatusCode status = UA_CreateCertificate(
        UA_Log_Stdout, &malformed, 1, &validSubjectAltName, 1,
        UA_CERTIFICATEFORMAT_DER, NULL, &privateKey, &certificate);
    ck_assert_uint_eq(status, UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_ptr_eq(privateKey.data, &keySentinelData);
    ck_assert_ptr_eq(certificate.data, &certSentinelData);

    status = UA_CreateCertificate(
        UA_Log_Stdout, &validSubject, 1, &malformed, 1,
        UA_CERTIFICATEFORMAT_DER, NULL, &privateKey, &certificate);
    ck_assert_uint_eq(status, UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_ptr_eq(privateKey.data, &keySentinelData);
    ck_assert_ptr_eq(certificate.data, &certSentinelData);
}
END_TEST

START_TEST(certificate_utils_outputs_are_transactional) {
    UA_String subject[2] = {UA_STRING_STATIC("C=DE"),
                            UA_STRING_STATIC("O=open62541")};
    UA_String subjectAltName = UA_STRING_STATIC("DNS:localhost");
    UA_KeyValueMap *params = UA_KeyValueMap_new();
    ck_assert_ptr_ne(params, NULL);
    UA_UInt16 keyLength = 2048;
    UA_KeyValueMap_setScalar(params, UA_QUALIFIEDNAME(0, "key-size-bits"),
                             &keyLength, &UA_TYPES[UA_TYPES_UINT16]);

    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_StatusCode status = UA_CreateCertificate(
        UA_Log_Stdout, subject, 2, &subjectAltName, 1,
        UA_CERTIFICATEFORMAT_DER, params, &privateKey, &certificate);
    UA_KeyValueMap_delete(params);
    ck_assert_uint_eq(status, UA_STATUSCODE_GOOD);

    UA_Byte sentinelData[] = "unchanged";
    UA_String output = {sizeof(sentinelData) - 1, sentinelData};
    status = UA_CertificateUtils_getCertCommonName(&certificate, &output);
    ck_assert_uint_eq(status, UA_STATUSCODE_BADNOTFOUND);
    ck_assert_ptr_eq(output.data, sentinelData);
    ck_assert_uint_eq(output.length, sizeof(sentinelData) - 1);

    UA_ByteString malformedCertificate = UA_BYTESTRING("not-a-certificate");
    status = UA_CertificateUtils_getSubjectName(&malformedCertificate, &output);
    ck_assert_uint_ne(status, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(output.data, sentinelData);
    ck_assert_uint_eq(output.length, sizeof(sentinelData) - 1);

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
}
END_TEST

static Suite* testSuite_create_certificate(void) {
    Suite *s = suite_create("Create Certificate");
    TCase *tc_cert = tcase_create("Certificate Create");
    tcase_add_checked_fixture(tc_cert, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert, certificate_generation);
    tcase_add_test(tc_cert, certificate_generation_rejects_malformed_names);
    tcase_add_test(tc_cert, certificate_utils_outputs_are_transactional);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_cert);
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
