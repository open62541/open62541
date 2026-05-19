/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "certificates.h"
#include "check.h"

#include <string.h>

static UA_ByteString
rsaCertificate(void) {
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;
    return certificate;
}

static UA_ByteString
rsaPrivateKey(void) {
    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;
    return privateKey;
}

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
static UA_ByteString
eccP256Certificate(void) {
    UA_ByteString certificate;
    certificate.length = CERT_P256_DER_LENGTH;
    certificate.data = CERT_P256_DER_DATA;
    return certificate;
}

static UA_ByteString
eccP256PrivateKey(void) {
    UA_ByteString privateKey;
    privateKey.length = KEY_P256_DER_LENGTH;
    privateKey.data = KEY_P256_DER_DATA;
    return privateKey;
}
#endif

static void
assertPolicyUri(const UA_SecurityPolicy *policy, const char *expectedUri) {
    UA_String expected = UA_STRING((char*)(uintptr_t)expectedUri);
    ck_assert(UA_String_equal(&policy->policyUri, &expected));
}

static UA_StatusCode
serverPasswordCallbackGood(UA_ServerConfig *config, UA_ByteString *password) {
    (void)config;
    *password = UA_STRING_ALLOC("pass1234");
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
serverPasswordCallbackWrong(UA_ServerConfig *config, UA_ByteString *password) {
    (void)config;
    *password = UA_STRING_ALLOC("wrong-password");
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
serverPasswordCallbackFail(UA_ServerConfig *config, UA_ByteString *password) {
    (void)config;
    UA_ByteString_init(password);
    return UA_STATUSCODE_BADINTERNALERROR;
}

static void
passwordProtectedPemCertificateAndKey(UA_ByteString *certificate,
                                      UA_ByteString *privateKey) {
    certificate->length = CERT_PEM_LENGTH;
    certificate->data = CERT_PEM_DATA;
    privateKey->length = KEY_PEM_PASSWORD_LENGTH;
    privateKey->data = KEY_PEM_PASSWORD_DATA;
}

static UA_StatusCode
setDefaultWithSecurityPoliciesPasswordCallback(
    UA_ServerConfig *config,
    UA_StatusCode (*passwordCallback)(UA_ServerConfig*, UA_ByteString*)) {
    UA_ByteString certificate;
    UA_ByteString privateKey;

    passwordProtectedPemCertificateAndKey(&certificate, &privateKey);

    memset(config, 0, sizeof(*config));
    config->privateKeyPasswordCallback = passwordCallback;
    return UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840,
                                                          &certificate, &privateKey,
                                                          NULL, 0, NULL, 0,
                                                          NULL, 0);
}

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
static UA_StatusCode
setDefaultWithFilestorePasswordCallback(
    UA_ServerConfig *config,
    UA_StatusCode (*passwordCallback)(UA_ServerConfig*, UA_ByteString*),
    const char *storePathChars) {
    UA_ByteString certificate;
    UA_ByteString privateKey;
    UA_String storePath = UA_STRING((char*)(uintptr_t)storePathChars);

    passwordProtectedPemCertificateAndKey(&certificate, &privateKey);

    memset(config, 0, sizeof(*config));
    config->privateKeyPasswordCallback = passwordCallback;
    return UA_ServerConfig_setDefaultWithFilestore(config, 4840,
                                                   &certificate, &privateKey,
                                                   storePath);
}
#endif

START_TEST(addSecurityPolicies_individual) {
    UA_StatusCode retval;
    UA_ByteString certificate = rsaCertificate();
    UA_ByteString privateKey = rsaPrivateKey();
    UA_ServerConfig config;

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setBasics(&config);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_ServerConfig_addSecurityPolicyBasic256Sha256(&config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.securityPoliciesSize, 1);
    assertPolicyUri(&config.securityPolicies[0],
                    "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    retval = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(&config, &certificate,
                                                                   &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.securityPoliciesSize, 2);
    assertPolicyUri(&config.securityPolicies[1],
                    "http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");

    retval = UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(&config, &certificate,
                                                                  &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.securityPoliciesSize, 3);
    assertPolicyUri(&config.securityPolicies[2],
                    "http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss");

    UA_ServerConfig_clear(&config);
}
END_TEST

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
START_TEST(addSecurityPolicy_eccNistP256) {
    UA_StatusCode retval;
    UA_ByteString certificate = eccP256Certificate();
    UA_ByteString privateKey = eccP256PrivateKey();
    UA_ServerConfig config;

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setBasics(&config);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_ServerConfig_addSecurityPolicyEccNistP256(&config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.securityPoliciesSize, 1);
    assertPolicyUri(&config.securityPolicies[0],
                    "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256");

    UA_ServerConfig_clear(&config);
}
END_TEST
#endif

START_TEST(addEndpoint_unknownPolicy_returnsBadInvalidArgument) {
    UA_StatusCode retval;
    UA_ServerConfig config;
    UA_String unknownPolicy =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setMinimalCustomBuffer(&config, 4840, NULL, 4096, 8192);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.endpointsSize, 1);

    retval = UA_ServerConfig_addEndpoint(&config, unknownPolicy, UA_MESSAGESECURITYMODE_SIGN);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(config.endpointsSize, 1);

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setMinimalCustomBuffer_addsOnlyNone) {
    UA_StatusCode retval;
    UA_ServerConfig config;

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setMinimalCustomBuffer(&config, 4840, NULL, 4096, 8192);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(config.tcpBufSize, 8192);
    ck_assert_uint_eq(config.securityPoliciesSize, 1);
    ck_assert_uint_eq(config.endpointsSize, 1);
    ck_assert_uint_eq(config.endpoints[0].securityMode, UA_MESSAGESECURITYMODE_NONE);
    assertPolicyUri(&config.securityPolicies[0],
                    "http://opcfoundation.org/UA/SecurityPolicy#None");
    ck_assert(UA_String_equal(&config.endpoints[0].securityPolicyUri,
                              &config.securityPolicies[0].policyUri));

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(addAllSecureEndpoints_skipsLevel0Policies) {
    UA_StatusCode retval;
    UA_ByteString certificate = rsaCertificate();
    UA_ByteString privateKey = rsaPrivateKey();
    UA_ServerConfig config;

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setBasics(&config);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_ServerConfig_addSecurityPolicyNone(&config, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_ServerConfig_addSecurityPolicyBasic256Sha256(&config, &certificate, &privateKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_ServerConfig_addAllSecureEndpoints(&config);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.endpointsSize, 2);
    ck_assert_uint_ne(config.endpoints[0].securityMode, UA_MESSAGESECURITYMODE_NONE);
    ck_assert_uint_ne(config.endpoints[1].securityMode, UA_MESSAGESECURITYMODE_NONE);
    ck_assert(UA_String_equal(&config.endpoints[0].securityPolicyUri,
                              &config.securityPolicies[1].policyUri));
    ck_assert(UA_String_equal(&config.endpoints[1].securityPolicyUri,
                              &config.securityPolicies[1].policyUri));

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setDefaultWithSecureSecurityPolicies_excludesNone) {
    UA_StatusCode retval;
    UA_ByteString certificate = rsaCertificate();
    UA_ByteString privateKey = rsaPrivateKey();
    UA_ServerConfig config;

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setDefaultWithSecureSecurityPolicies(&config, 4840,
                                                                  &certificate, &privateKey,
                                                                  NULL, 0, NULL, 0,
                                                                  NULL, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(config.securityPoliciesSize, 0);
    ck_assert_uint_eq(config.endpointsSize, 2 * config.securityPoliciesSize);

    UA_String noneUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    for(size_t i = 0; i < config.securityPoliciesSize; ++i) {
        ck_assert(!UA_String_equal(&config.securityPolicies[i].policyUri, &noneUri));
    }
    for(size_t i = 0; i < config.endpointsSize; ++i) {
        ck_assert_uint_ne(config.endpoints[i].securityMode, UA_MESSAGESECURITYMODE_NONE);
        ck_assert(!UA_String_equal(&config.endpoints[i].securityPolicyUri, &noneUri));
    }

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setBasics_overridesServerUrls) {
    UA_StatusCode retval;
    UA_ServerConfig config;

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setBasics_withPort(&config, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.serverUrlsSize, 1);

    retval = UA_ServerConfig_setBasics_withPort(&config, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config.serverUrlsSize, 1);

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setDefaultWithSecureSecurityPolicies_withTrustLists) {
    UA_StatusCode retval;
    UA_ByteString certificate = rsaCertificate();
    UA_ByteString privateKey = rsaPrivateKey();
    UA_ServerConfig config;

    UA_ByteString rootCa;
    rootCa.length = ROOT_CERT_DER_LENGTH;
    rootCa.data = ROOT_CERT_DER_DATA;

    UA_ByteString intermediateCa;
    intermediateCa.length = INTERMEDIATE_CERT_DER_LENGTH;
    intermediateCa.data = INTERMEDIATE_CERT_DER_DATA;

    UA_ByteString rootCaCrl;
    rootCaCrl.length = ROOT_EMPTY_CRL_PEM_LENGTH;
    rootCaCrl.data = ROOT_EMPTY_CRL_PEM_DATA;

    UA_ByteString intermediateCaCrl;
    intermediateCaCrl.length = INTERMEDIATE_CRL_PEM_LENGTH;
    intermediateCaCrl.data = INTERMEDIATE_CRL_PEM_DATA;

    UA_ByteString trustList[2] = {intermediateCa, rootCa};
    UA_ByteString issuerList[2] = {intermediateCa, rootCa};
    UA_ByteString revocationList[2] = {rootCaCrl, intermediateCaCrl};

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setDefaultWithSecureSecurityPolicies(&config, 4840,
                                                                  &certificate, &privateKey,
                                                                  trustList, 2,
                                                                  issuerList, 2,
                                                                  revocationList, 2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(config.securityPoliciesSize, 0);
    ck_assert_uint_eq(config.endpointsSize, 2 * config.securityPoliciesSize);
    ck_assert(config.secureChannelPKI.clear != NULL);
    ck_assert(config.sessionPKI.clear != NULL);

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setDefaultWithSecurityPolicies_withTrustLists) {
    UA_StatusCode retval;
    UA_ByteString certificate = rsaCertificate();
    UA_ByteString privateKey = rsaPrivateKey();
    UA_ServerConfig config;

    UA_ByteString rootCa;
    rootCa.length = ROOT_CERT_DER_LENGTH;
    rootCa.data = ROOT_CERT_DER_DATA;

    UA_ByteString intermediateCa;
    intermediateCa.length = INTERMEDIATE_CERT_DER_LENGTH;
    intermediateCa.data = INTERMEDIATE_CERT_DER_DATA;

    UA_ByteString rootCaCrl;
    rootCaCrl.length = ROOT_EMPTY_CRL_PEM_LENGTH;
    rootCaCrl.data = ROOT_EMPTY_CRL_PEM_DATA;

    UA_ByteString intermediateCaCrl;
    intermediateCaCrl.length = INTERMEDIATE_CRL_PEM_LENGTH;
    intermediateCaCrl.data = INTERMEDIATE_CRL_PEM_DATA;

    UA_ByteString trustList[2] = {intermediateCa, rootCa};
    UA_ByteString issuerList[2] = {intermediateCa, rootCa};
    UA_ByteString revocationList[2] = {rootCaCrl, intermediateCaCrl};

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setDefaultWithSecurityPolicies(&config, 4840,
                                                            &certificate, &privateKey,
                                                            trustList, 2,
                                                            issuerList, 2,
                                                            revocationList, 2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(config.securityPoliciesSize, 0);
    ck_assert_uint_gt(config.endpointsSize, config.securityPoliciesSize);
    ck_assert(config.secureChannelPKI.clear != NULL);
    ck_assert(config.sessionPKI.clear != NULL);

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setDefaultWithSecurityPolicies_passwordCallback_success) {
    UA_StatusCode retval;
    UA_ServerConfig config;

    retval = setDefaultWithSecurityPoliciesPasswordCallback(&config,
                                                            serverPasswordCallbackGood);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(config.securityPoliciesSize, 0);
    ck_assert_uint_gt(config.endpointsSize, 0);

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setDefaultWithSecurityPolicies_passwordCallback_failure) {
    UA_StatusCode retval;
    UA_ServerConfig config;

    retval = setDefaultWithSecurityPoliciesPasswordCallback(&config,
                                                            serverPasswordCallbackFail);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINTERNALERROR);

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setDefaultWithSecurityPolicies_passwordCallback_wrongPassword) {
    UA_StatusCode retval;
    UA_ServerConfig config;

    retval = setDefaultWithSecurityPoliciesPasswordCallback(&config,
                                                            serverPasswordCallbackWrong);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

    UA_ServerConfig_clear(&config);
}
END_TEST

#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
START_TEST(setDefaultWithFilestore_requiresStorePath) {
    UA_StatusCode retval;
    UA_ServerConfig config;

    memset(&config, 0, sizeof(config));
    retval = UA_ServerConfig_setDefaultWithFilestore(&config, 4840, NULL, NULL,
                                                     UA_STRING_NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setDefaultWithFilestore_passwordCallback_failure) {
    UA_StatusCode retval;
    UA_ServerConfig config;

    retval = setDefaultWithFilestorePasswordCallback(
        &config, serverPasswordCallbackFail,
        "/tmp/open62541-server-config-default-filestore-fail");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINTERNALERROR);

    UA_ServerConfig_clear(&config);
}
END_TEST

START_TEST(setDefaultWithFilestore_passwordCallback_wrongPassword) {
    UA_StatusCode retval;
    UA_ServerConfig config;

    retval = setDefaultWithFilestorePasswordCallback(
        &config, serverPasswordCallbackWrong,
        "/tmp/open62541-server-config-default-filestore-wrong");
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);

    UA_ServerConfig_clear(&config);
}
END_TEST
#endif

static Suite*
testSuite_server_config_default(void) {
    Suite *suite = suite_create("Server config default");
    TCase *testCase = tcase_create("Coverage");

    tcase_add_test(testCase, addSecurityPolicies_individual);
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
    tcase_add_test(testCase, addSecurityPolicy_eccNistP256);
#endif
    tcase_add_test(testCase, addEndpoint_unknownPolicy_returnsBadInvalidArgument);
    tcase_add_test(testCase, setMinimalCustomBuffer_addsOnlyNone);
    tcase_add_test(testCase, addAllSecureEndpoints_skipsLevel0Policies);
    tcase_add_test(testCase, setDefaultWithSecureSecurityPolicies_excludesNone);
    tcase_add_test(testCase, setBasics_overridesServerUrls);
    tcase_add_test(testCase, setDefaultWithSecureSecurityPolicies_withTrustLists);
    tcase_add_test(testCase, setDefaultWithSecurityPolicies_withTrustLists);
    tcase_add_test(testCase, setDefaultWithSecurityPolicies_passwordCallback_success);
    tcase_add_test(testCase, setDefaultWithSecurityPolicies_passwordCallback_failure);
    tcase_add_test(testCase, setDefaultWithSecurityPolicies_passwordCallback_wrongPassword);
#if defined(__linux__) || defined(UA_ARCHITECTURE_WIN32)
    tcase_add_test(testCase, setDefaultWithFilestore_requiresStorePath);
    tcase_add_test(testCase, setDefaultWithFilestore_passwordCallback_failure);
    tcase_add_test(testCase, setDefaultWithFilestore_passwordCallback_wrongPassword);
#endif

    suite_add_tcase(suite, testCase);
    return suite;
}

int
main(void) {
    Suite *suite = testSuite_server_config_default();
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int numberFailed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (numberFailed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}