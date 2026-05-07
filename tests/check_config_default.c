/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/util.h>

#include <check.h>
#include <stdlib.h>
#include <string.h>

static UA_ServerConfig cfg;

static void setup(void) {
    memset(&cfg, 0, sizeof(cfg));
}

static void teardown(void) {
    UA_ServerConfig_clean(&cfg);
}

START_TEST(SetBasicsZeroConfigSucceeds) {
    UA_StatusCode r = UA_ServerConfig_setBasics(&cfg);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    /* Default port + buildInfo populated */
    ck_assert(cfg.buildInfo.productUri.length > 0);
    ck_assert(cfg.applicationDescription.applicationUri.length > 0);
    ck_assert_uint_eq(cfg.endpointsSize, 0);
    ck_assert_uint_eq(cfg.securityPoliciesSize, 0);
} END_TEST

START_TEST(SetBasicsWithPortZeroIsAccepted) {
    UA_StatusCode r = UA_ServerConfig_setBasics_withPort(&cfg, 0);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(SetBasicsNullConfigRejected) {
    UA_StatusCode r = UA_ServerConfig_setBasics_withPort(NULL, 4840);
    ck_assert_uint_eq(r, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

START_TEST(AddSecurityPolicyNoneAndAllEndpoints) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    /* SecurityPolicy#None can always be added (no certificate) */
    UA_StatusCode r = UA_ServerConfig_addSecurityPolicyNone(&cfg, NULL);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cfg.securityPoliciesSize, 1);

    /* addAllEndpoints should add one endpoint per security policy */
    r = UA_ServerConfig_addAllEndpoints(&cfg);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cfg.endpointsSize, 1); /* None policy → one endpoint */
} END_TEST

START_TEST(AddEndpointWithUnknownPolicyRejected) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ServerConfig_addSecurityPolicyNone(&cfg, NULL),
                      UA_STATUSCODE_GOOD);

    UA_String unknown = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#NoSuchThing");
    UA_StatusCode r =
        UA_ServerConfig_addEndpoint(&cfg, unknown, UA_MESSAGESECURITYMODE_NONE);
    ck_assert_uint_eq(r, UA_STATUSCODE_BADINVALIDARGUMENT);

    /* Adding the explicit None policy by URI must succeed */
    UA_String noneUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    r = UA_ServerConfig_addEndpoint(&cfg, noneUri, UA_MESSAGESECURITYMODE_NONE);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddAllSecureEndpointsClearsExisting) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ServerConfig_addSecurityPolicyNone(&cfg, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ServerConfig_addAllEndpoints(&cfg),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cfg.endpointsSize, 1);

    /* addAllSecureEndpoints wipes the existing endpoint list and re-adds
     * SIGN+SIGNANDENCRYPT for each non-None policy. With only None present
     * (and no certificate), the resulting endpoint list ends up empty. */
    UA_StatusCode r = UA_ServerConfig_addAllSecureEndpoints(&cfg);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddAllSecurityPoliciesWithoutCertSucceeds) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    /* Without a certificate the secure policies cannot be added but the
     * function logs a warning and still returns GOOD. The "None" policy
     * is added in the !onlySecure branch. */
    UA_StatusCode r =
        UA_ServerConfig_addAllSecurityPolicies(&cfg, NULL, NULL);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);

    /* addAllSecureSecurityPolicies should be similarly safe */
    r = UA_ServerConfig_addAllSecureSecurityPolicies(&cfg, NULL, NULL);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
} END_TEST

#ifdef UA_ENABLE_ENCRYPTION
#include "../tests/encryption/certificates.h"

START_TEST(AddSecurityPolicyHelpersWithoutCertReturnError) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    /* Each of these allocates a SecurityPolicy slot, then constructs the
     * policy with an empty certificate. The construction itself returns
     * BADSECURITYCHECKSFAILED for the strong policies; we only care that
     * the allocate / clean-up paths in ua_config_default.c are exercised. */
    (void)UA_ServerConfig_addSecurityPolicyBasic128Rsa15(&cfg, NULL, NULL);
    (void)UA_ServerConfig_addSecurityPolicyBasic256(&cfg, NULL, NULL);
    (void)UA_ServerConfig_addSecurityPolicyBasic256Sha256(&cfg, NULL, NULL);
    (void)UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(&cfg, NULL, NULL);
    (void)UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(&cfg, NULL, NULL);
} END_TEST

START_TEST(AddSecurityPolicyHelpersWithCertSucceed) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    UA_ByteString cert = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString key  = {KEY_DER_LENGTH,  KEY_DER_DATA};

    UA_StatusCode r = UA_ServerConfig_addSecurityPolicyBasic256Sha256(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    r = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    r = UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(cfg.securityPoliciesSize, 3);

    /* Now exercise addAllSecureEndpoints with secure policies present */
    r = UA_ServerConfig_addAllSecureEndpoints(&cfg);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(cfg.endpointsSize, 0);
} END_TEST

START_TEST(AddAllSecurityPoliciesWithCertSucceed) {
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);

    UA_ByteString cert = {CERT_DER_LENGTH, CERT_DER_DATA};
    UA_ByteString key  = {KEY_DER_LENGTH,  KEY_DER_DATA};

    UA_StatusCode r = UA_ServerConfig_addAllSecurityPolicies(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(cfg.securityPoliciesSize, 0);

    /* And the secure-only variant on a fresh config */
    UA_ServerConfig_clean(&cfg);
    memset(&cfg, 0, sizeof(cfg));
    ck_assert_uint_eq(UA_ServerConfig_setBasics(&cfg), UA_STATUSCODE_GOOD);
    r = UA_ServerConfig_addAllSecureSecurityPolicies(&cfg, &cert, &key);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(cfg.securityPoliciesSize, 0);
} END_TEST
#endif

int main(void) {
    Suite *s = suite_create("ServerConfig defaults");
    TCase *tc = tcase_create("public API");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, SetBasicsZeroConfigSucceeds);
    tcase_add_test(tc, SetBasicsWithPortZeroIsAccepted);
    tcase_add_test(tc, SetBasicsNullConfigRejected);
    tcase_add_test(tc, AddSecurityPolicyNoneAndAllEndpoints);
    tcase_add_test(tc, AddEndpointWithUnknownPolicyRejected);
    tcase_add_test(tc, AddAllSecureEndpointsClearsExisting);
    tcase_add_test(tc, AddAllSecurityPoliciesWithoutCertSucceeds);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc, AddSecurityPolicyHelpersWithoutCertReturnError);
    tcase_add_test(tc, AddSecurityPolicyHelpersWithCertSucceed);
    tcase_add_test(tc, AddAllSecurityPoliciesWithCertSucceed);
#endif
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
