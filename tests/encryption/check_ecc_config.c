/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* Test the UA_ServerConfig_addSecurityPolicyEcc*() public API functions.
 * These are the entry points users call to add individual ECC policies. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/certificategroup_default.h>

#include "test_helpers.h"
#include "certificates.h"
#include "check.h"

/* --- Test: add ECC_nistP256 policy via public API --- */
START_TEST(config_add_ecc_nistp256) {
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;

    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    size_t prevSize = config->securityPoliciesSize;
    UA_StatusCode retval =
        UA_ServerConfig_addSecurityPolicyEccNistP256(config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config->securityPoliciesSize, prevSize + 1);

    UA_Server_delete(server);
}
END_TEST

/* --- Test: add ECC_nistP384 policy via public API --- */
START_TEST(config_add_ecc_nistp384) {
    UA_ByteString certificate;
    certificate.length = CERT_P384_DER_LENGTH;
    certificate.data = CERT_P384_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_P384_DER_LENGTH;
    privateKey.data = KEY_P384_DER_DATA;

    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    size_t prevSize = config->securityPoliciesSize;
    UA_StatusCode retval =
        UA_ServerConfig_addSecurityPolicyEccNistP384(config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config->securityPoliciesSize, prevSize + 1);

    UA_Server_delete(server);
}
END_TEST

/* --- Test: add ECC_brainpoolP256r1 policy via public API --- */
START_TEST(config_add_ecc_bp256r1) {
    UA_ByteString certificate;
    certificate.length = CERT_BP256R1_DER_LENGTH;
    certificate.data = CERT_BP256R1_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_BP256R1_DER_LENGTH;
    privateKey.data = KEY_BP256R1_DER_DATA;

    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    size_t prevSize = config->securityPoliciesSize;
    UA_StatusCode retval =
        UA_ServerConfig_addSecurityPolicyEccBrainpoolP256r1(config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config->securityPoliciesSize, prevSize + 1);

    UA_Server_delete(server);
}
END_TEST

/* --- Test: add ECC_brainpoolP384r1 policy via public API --- */
START_TEST(config_add_ecc_bp384r1) {
    UA_ByteString certificate;
    certificate.length = CERT_BP384R1_DER_LENGTH;
    certificate.data = CERT_BP384R1_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_BP384R1_DER_LENGTH;
    privateKey.data = KEY_BP384R1_DER_DATA;

    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    size_t prevSize = config->securityPoliciesSize;
    UA_StatusCode retval =
        UA_ServerConfig_addSecurityPolicyEccBrainpoolP384r1(config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config->securityPoliciesSize, prevSize + 1);

    UA_Server_delete(server);
}
END_TEST

#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
/* --- Test: add ECC_curve25519 policy via public API --- */
START_TEST(config_add_ecc_curve25519) {
    UA_ByteString certificate;
    certificate.length = CERT_ED25519_DER_LENGTH;
    certificate.data = CERT_ED25519_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_ED25519_DER_LENGTH;
    privateKey.data = KEY_ED25519_DER_DATA;

    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    size_t prevSize = config->securityPoliciesSize;
    UA_StatusCode retval =
        UA_ServerConfig_addSecurityPolicyEccCurve25519(config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config->securityPoliciesSize, prevSize + 1);

    UA_Server_delete(server);
}
END_TEST

/* --- Test: add ECC_curve448 policy via public API --- */
START_TEST(config_add_ecc_curve448) {
    UA_ByteString certificate;
    certificate.length = CERT_ED448_DER_LENGTH;
    certificate.data = CERT_ED448_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_ED448_DER_LENGTH;
    privateKey.data = KEY_ED448_DER_DATA;

    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    size_t prevSize = config->securityPoliciesSize;
    UA_StatusCode retval =
        UA_ServerConfig_addSecurityPolicyEccCurve448(config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config->securityPoliciesSize, prevSize + 1);

    UA_Server_delete(server);
}
END_TEST
#endif /* UA_ENABLE_ENCRYPTION_OPENSSL */

/* --- Test: addSecurityPolicyEcc* with wrong cert type (RSA cert) --- */
START_TEST(config_add_ecc_wrong_cert) {
    /* Use an RSA certificate — should fail for ECC policies */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    size_t prevSize = config->securityPoliciesSize;
    UA_StatusCode retval =
        UA_ServerConfig_addSecurityPolicyEccNistP256(config, &certificate, &privateKey);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
    /* Policy count should not have increased */
    ck_assert_uint_eq(config->securityPoliciesSize, prevSize);

    UA_Server_delete(server);
}
END_TEST

/* --- Test: add multiple ECC policies to same config --- */
START_TEST(config_add_multiple_ecc_policies) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    size_t baseSize = config->securityPoliciesSize;

    /* Add P256 */
    UA_ByteString certP256;
    certP256.length = CERT_P256_DER_LENGTH;
    certP256.data = CERT_P256_DER_DATA;
    UA_ByteString keyP256;
    keyP256.length = KEY_P256_DER_LENGTH;
    keyP256.data = KEY_P256_DER_DATA;
    UA_StatusCode retval =
        UA_ServerConfig_addSecurityPolicyEccNistP256(config, &certP256, &keyP256);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add P384 */
    UA_ByteString certP384;
    certP384.length = CERT_P384_DER_LENGTH;
    certP384.data = CERT_P384_DER_DATA;
    UA_ByteString keyP384;
    keyP384.length = KEY_P384_DER_LENGTH;
    keyP384.data = KEY_P384_DER_DATA;
    retval =
        UA_ServerConfig_addSecurityPolicyEccNistP384(config, &certP384, &keyP384);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add BP256 */
    UA_ByteString certBP256;
    certBP256.length = CERT_BP256R1_DER_LENGTH;
    certBP256.data = CERT_BP256R1_DER_DATA;
    UA_ByteString keyBP256;
    keyBP256.length = KEY_BP256R1_DER_LENGTH;
    keyBP256.data = KEY_BP256R1_DER_DATA;
    retval =
        UA_ServerConfig_addSecurityPolicyEccBrainpoolP256r1(config, &certBP256, &keyBP256);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add BP384 */
    UA_ByteString certBP384;
    certBP384.length = CERT_BP384R1_DER_LENGTH;
    certBP384.data = CERT_BP384R1_DER_DATA;
    UA_ByteString keyBP384;
    keyBP384.length = KEY_BP384R1_DER_LENGTH;
    keyBP384.data = KEY_BP384R1_DER_DATA;
    retval =
        UA_ServerConfig_addSecurityPolicyEccBrainpoolP384r1(config, &certBP384, &keyBP384);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(config->securityPoliciesSize, baseSize + 4);

    UA_Server_delete(server);
}
END_TEST

static Suite* testSuite_ecc_config(void) {
    Suite *s = suite_create("ECC Config");
    TCase *tc = tcase_create("addSecurityPolicyEcc");
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc, config_add_ecc_nistp256);
    tcase_add_test(tc, config_add_ecc_nistp384);
    tcase_add_test(tc, config_add_ecc_bp256r1);
    tcase_add_test(tc, config_add_ecc_bp384r1);
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
    tcase_add_test(tc, config_add_ecc_curve25519);
    tcase_add_test(tc, config_add_ecc_curve448);
#endif
    tcase_add_test(tc, config_add_ecc_wrong_cert);
    tcase_add_test(tc, config_add_multiple_ecc_policies);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_ecc_config();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
