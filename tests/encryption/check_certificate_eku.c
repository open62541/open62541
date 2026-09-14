/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"
#include "certificate_eku.h"
#include "certificates.h"

#include <check.h>
#include <stdlib.h>

START_TEST(parse_extended_key_usage) {
    UA_ByteString clientOnly = {RSA_SERVER_WRONG_EKU_LENGTH,
                                RSA_SERVER_WRONG_EKU_PEM};
    UA_CertificateEku eku = UA_CERTIFICATEEKU_NONE;
    UA_StatusCode res =
        UA_CertificateUtils_getExtendedKeyUsage(&clientOnly, &eku);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eku, UA_CERTIFICATEEKU_CLIENTAUTH);

    UA_ByteString serverOnly = {RSA_CLIENT_WRONG_EKU_LENGTH,
                                RSA_CLIENT_WRONG_EKU_PEM};
    res = UA_CertificateUtils_getExtendedKeyUsage(&serverOnly, &eku);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eku, UA_CERTIFICATEEKU_SERVERAUTH);

    UA_ByteString missing = {APPLICATION_CERT_DER_LENGTH,
                             APPLICATION_CERT_DER_DATA};
    res = UA_CertificateUtils_getExtendedKeyUsage(&missing, &eku);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eku, UA_CERTIFICATEEKU_NONE);
}
END_TEST

START_TEST(check_extended_key_usage_profile) {
    UA_ByteString wrong = {RSA_SERVER_WRONG_EKU_LENGTH,
                           RSA_SERVER_WRONG_EKU_PEM};
    UA_StatusCode res = UA_CertificateUtils_checkExtendedKeyUsage(
        &wrong, UA_CERTIFICATEEKU_SERVERAUTH, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED);

    UA_ByteString missing = {APPLICATION_CERT_DER_LENGTH,
                             APPLICATION_CERT_DER_DATA};
    res = UA_CertificateUtils_checkExtendedKeyUsage(
        &missing, UA_CERTIFICATEEKU_SERVERAUTH, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED);
    res = UA_CertificateUtils_checkExtendedKeyUsage(
        &missing, UA_CERTIFICATEEKU_CLIENTAUTH, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(client_certificate_eku_rule) {
    UA_ClientConfig config;
    memset(&config, 0, sizeof(config));
    UA_SecurityPolicy policy;
    memset(&policy, 0, sizeof(policy));
    policy.policyUri = UA_STRING("urn:test:rsa");

    UA_ByteString wrong = {RSA_SERVER_WRONG_EKU_LENGTH,
                           RSA_SERVER_WRONG_EKU_PEM};
    UA_StatusCode res = verifyServerCertificateEku(&config, &policy, &wrong);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    config.certificateEkuRule = UA_RULEHANDLING_ABORT;
    res = verifyServerCertificateEku(&config, &policy, &wrong);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED);

    policy.policyUri = UA_SECURITY_POLICY_NONE_URI;
    res = verifyServerCertificateEku(&config, &policy, &wrong);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(server_certificate_eku_rule) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_ne(server, NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_SecurityPolicy policy;
    memset(&policy, 0, sizeof(policy));
    policy.policyUri = UA_STRING("urn:test:rsa");
    UA_ByteString wrong = {RSA_CLIENT_WRONG_EKU_LENGTH,
                           RSA_CLIENT_WRONG_EKU_PEM};

    UA_StatusCode res = validateCertificateEku(server, &policy, &wrong, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    config->certificateEkuRule = UA_RULEHANDLING_ABORT;
    res = validateCertificateEku(server, &policy, &wrong, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED);
    res = validateCertificateEku(server, &policy, &wrong, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED);

    UA_ByteString missing = {APPLICATION_CERT_DER_LENGTH,
                             APPLICATION_CERT_DER_DATA};
    res = validateCertificateEku(server, &policy, &missing, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED);
    res = validateCertificateEku(server, &policy, &missing, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_delete(server);
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("Certificate EKU");
    TCase *tc = tcase_create("certificate purpose validation");
    tcase_add_test(tc, parse_extended_key_usage);
    tcase_add_test(tc, check_extended_key_usage_profile);
    tcase_add_test(tc, client_certificate_eku_rule);
    tcase_add_test(tc, server_certificate_eku_rule);
    suite_add_tcase(suite, tc);
    return suite;
}

int main(void) {
    Suite *suite = testSuite();
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
