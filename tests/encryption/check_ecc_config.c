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
#include <stdlib.h>

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

#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
/* The certificate must be on the curve of the policy. UA_OpenSSL_LoadCertificate
 * compares only the key type, so without the curve check of the ECC AEAD module
 * a P-256 certificate would pass as the certificate of a P-384 policy, which
 * then advertises an endpoint it cannot serve. */
static void
assertAddRejected(UA_StatusCode (*add)(UA_ServerConfig *, const UA_ByteString *,
                                       const UA_ByteString *),
                  UA_ByteString cert, UA_ByteString key) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    size_t prevSize = config->securityPoliciesSize;
    ck_assert_uint_eq(add(config, &cert, &key), UA_STATUSCODE_BADCERTIFICATEINVALID);
    ck_assert_uint_eq(config->securityPoliciesSize, prevSize);
    UA_Server_delete(server);
}

START_TEST(config_add_ecc_wrong_curve) {
    UA_ByteString cert256 = {CERT_P256_DER_LENGTH, CERT_P256_DER_DATA};
    UA_ByteString key256 = {KEY_P256_DER_LENGTH, KEY_P256_DER_DATA};
    UA_ByteString certBp384 = {CERT_BP384R1_DER_LENGTH, CERT_BP384R1_DER_DATA};
    UA_ByteString keyBp384 = {KEY_BP384R1_DER_LENGTH, KEY_BP384R1_DER_DATA};
    UA_ByteString certEd25519 = {CERT_ED25519_DER_LENGTH, CERT_ED25519_DER_DATA};
    UA_ByteString keyEd25519 = {KEY_ED25519_DER_LENGTH, KEY_ED25519_DER_DATA};

    /* Another curve of another size */
    assertAddRejected(UA_ServerConfig_addSecurityPolicyEccNistP384AesGcm,
                      cert256, key256);
    /* Another curve of the same size: a key-length check would let it pass */
    assertAddRejected(UA_ServerConfig_addSecurityPolicyEccNistP384ChaChaPoly,
                      certBp384, keyBp384);
    assertAddRejected(UA_ServerConfig_addSecurityPolicyEccBrainpoolP256r1AesGcm,
                      cert256, key256);
    /* An Edwards certificate of the other curve */
    assertAddRejected(UA_ServerConfig_addSecurityPolicyEccCurve448AesGcm,
                      certEd25519, keyEd25519);
    /* A Weierstrass certificate for an Edwards policy */
    assertAddRejected(UA_ServerConfig_addSecurityPolicyEccCurve25519ChaChaPoly,
                      cert256, key256);

    /* The matching curve still works, so the check does not refuse
     * everything */
    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ByteString cert384 = {CERT_P384_DER_LENGTH, CERT_P384_DER_DATA};
    UA_ByteString key384 = {KEY_P384_DER_LENGTH, KEY_P384_DER_DATA};
    ck_assert_uint_eq(UA_ServerConfig_addSecurityPolicyEccNistP384AesGcm(config, &cert384,
                                                                         &key384),
                      UA_STATUSCODE_GOOD);

    /* A certificate update is the other way to the same defect */
    UA_SecurityPolicy *sp = &config->securityPolicies[config->securityPoliciesSize - 1];
    ck_assert_uint_eq(sp->updateCertificate(sp, certBp384, keyBp384),
                      UA_STATUSCODE_BADCERTIFICATEINVALID);
    ck_assert_uint_eq(sp->updateCertificate(sp, cert384, key384), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

/* Every add-helper of the ECC AEAD policies registers its policy */
START_TEST(config_add_ecc_aead) {
    const struct {
        const char *uri;
        UA_StatusCode (*add)(UA_ServerConfig *, const UA_ByteString *,
                             const UA_ByteString *);
        UA_ByteString cert;
        UA_ByteString key;
    } rows[] = {
#define ROW(FRAG, FN, C) \
        {"http://opcfoundation.org/UA/SecurityPolicy#" FRAG, \
         UA_ServerConfig_addSecurityPolicy##FN, \
         {CERT_##C##_DER_LENGTH, CERT_##C##_DER_DATA}, \
         {KEY_##C##_DER_LENGTH, KEY_##C##_DER_DATA}}
        ROW("ECC_nistP256_AesGcm", EccNistP256AesGcm, P256),
        ROW("ECC_nistP256_ChaChaPoly", EccNistP256ChaChaPoly, P256),
        ROW("ECC_nistP384_AesGcm", EccNistP384AesGcm, P384),
        ROW("ECC_nistP384_ChaChaPoly", EccNistP384ChaChaPoly, P384),
        ROW("ECC_brainpoolP256r1_AesGcm", EccBrainpoolP256r1AesGcm, BP256R1),
        ROW("ECC_brainpoolP256r1_ChaChaPoly", EccBrainpoolP256r1ChaChaPoly, BP256R1),
        ROW("ECC_brainpoolP384r1_AesGcm", EccBrainpoolP384r1AesGcm, BP384R1),
        ROW("ECC_brainpoolP384r1_ChaChaPoly", EccBrainpoolP384r1ChaChaPoly, BP384R1),
        ROW("ECC_curve25519_AesGcm", EccCurve25519AesGcm, ED25519),
        ROW("ECC_curve25519_ChaChaPoly", EccCurve25519ChaChaPoly, ED25519),
        ROW("ECC_curve448_AesGcm", EccCurve448AesGcm, ED448),
        ROW("ECC_curve448_ChaChaPoly", EccCurve448ChaChaPoly, ED448)
#undef ROW
    };

    /* All of them in one configuration */
    UA_Server *server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    size_t baseSize = config->securityPoliciesSize;
    for(size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        ck_assert_uint_eq(rows[i].add(config, &rows[i].cert, &rows[i].key),
                          UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(config->securityPoliciesSize, baseSize + i + 1);
        UA_String uri = UA_STRING((char *)(uintptr_t)rows[i].uri);
        ck_assert(UA_String_equal(&config->securityPolicies[baseSize + i].policyUri,
                                  &uri));
    }
    UA_Server_delete(server);
}
END_TEST
#endif /* UA_ENABLE_ENCRYPTION_OPENSSL */

/* --- Test: the per-policy crypto constants are declared, not re-derived ---
 *
 * secretLength, certThumbprintLength and secureChannelEnhancements are stated
 * by each policy so that core code does not have to recover them by matching
 * substrings of the policy URI. This pins the value of every shipped ECC
 * policy; a new policy that forgets to set them shows up here as a zero.
 *
 * Note that secretLength is NOT nonceLength/2 in general: that holds for the
 * Weierstrass curves, where the nonce is the ephemeral public key X||Y, but
 * not for X25519/X448, where the nonce is the whole raw public key. */
typedef struct {
    const char *uri;
    UA_StatusCode (*add)(UA_ServerConfig *, const UA_ByteString *,
                         const UA_ByteString *);
    const UA_ByteString cert;
    const UA_ByteString key;
    size_t nonceLength;
    size_t secretLength;
    size_t certThumbprintLength;
    UA_Boolean enhanced;
    const char *symUri; /* the symmetric algorithm URI, NULL: not checked */
} EccPolicyConstants;

#define AES128_GCM "http://opcfoundation.org/UA/security/aes128-gcm"
#define AES256_GCM "http://opcfoundation.org/UA/security/aes256-gcm"
#define CHACHA20_POLY1305 "http://opcfoundation.org/UA/security/chacha20-poly1305"

START_TEST(ecc_policies_declare_their_crypto_constants) {
    const EccPolicyConstants rows[] = {
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256",
         UA_ServerConfig_addSecurityPolicyEccNistP256,
         {CERT_P256_DER_LENGTH, CERT_P256_DER_DATA},
         {KEY_P256_DER_LENGTH, KEY_P256_DER_DATA}, 64, 32, 32, false, NULL},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384",
         UA_ServerConfig_addSecurityPolicyEccNistP384,
         {CERT_P384_DER_LENGTH, CERT_P384_DER_DATA},
         {KEY_P384_DER_LENGTH, KEY_P384_DER_DATA}, 96, 48, 48, false, NULL},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1",
         UA_ServerConfig_addSecurityPolicyEccBrainpoolP256r1,
         {CERT_BP256R1_DER_LENGTH, CERT_BP256R1_DER_DATA},
         {KEY_BP256R1_DER_LENGTH, KEY_BP256R1_DER_DATA}, 64, 32, 32, false, NULL},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1",
         UA_ServerConfig_addSecurityPolicyEccBrainpoolP384r1,
         {CERT_BP384R1_DER_LENGTH, CERT_BP384R1_DER_DATA},
         {KEY_BP384R1_DER_LENGTH, KEY_BP384R1_DER_DATA}, 96, 48, 48, false, NULL},
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_AesGcm",
         UA_ServerConfig_addSecurityPolicyEccNistP256AesGcm,
         {CERT_P256_DER_LENGTH, CERT_P256_DER_DATA},
         {KEY_P256_DER_LENGTH, KEY_P256_DER_DATA}, 64, 32, 32, true,
         AES128_GCM},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256_ChaChaPoly",
         UA_ServerConfig_addSecurityPolicyEccNistP256ChaChaPoly,
         {CERT_P256_DER_LENGTH, CERT_P256_DER_DATA},
         {KEY_P256_DER_LENGTH, KEY_P256_DER_DATA}, 64, 32, 32, true,
         CHACHA20_POLY1305},
        /* X25519/X448: nonce == raw public key == the shared secret */
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519",
         UA_ServerConfig_addSecurityPolicyEccCurve25519,
         {CERT_ED25519_DER_LENGTH, CERT_ED25519_DER_DATA},
         {KEY_ED25519_DER_LENGTH, KEY_ED25519_DER_DATA}, 32, 32, 32, false, NULL},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448",
         UA_ServerConfig_addSecurityPolicyEccCurve448,
         {CERT_ED448_DER_LENGTH, CERT_ED448_DER_DATA},
         {KEY_ED448_DER_LENGTH, KEY_ED448_DER_DATA}, 56, 56, 48, false, NULL},
        /* The AEAD policies of Part 7. Every one is enhanced-security; the
         * 384-bit tier derives with SHA-384, hence the 48-byte thumbprint. */
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384_AesGcm",
         UA_ServerConfig_addSecurityPolicyEccNistP384AesGcm,
         {CERT_P384_DER_LENGTH, CERT_P384_DER_DATA},
         {KEY_P384_DER_LENGTH, KEY_P384_DER_DATA}, 96, 48, 48, true,
         AES256_GCM},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384_ChaChaPoly",
         UA_ServerConfig_addSecurityPolicyEccNistP384ChaChaPoly,
         {CERT_P384_DER_LENGTH, CERT_P384_DER_DATA},
         {KEY_P384_DER_LENGTH, KEY_P384_DER_DATA}, 96, 48, 48, true,
         CHACHA20_POLY1305},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1_AesGcm",
         UA_ServerConfig_addSecurityPolicyEccBrainpoolP256r1AesGcm,
         {CERT_BP256R1_DER_LENGTH, CERT_BP256R1_DER_DATA},
         {KEY_BP256R1_DER_LENGTH, KEY_BP256R1_DER_DATA}, 64, 32, 32, true,
         AES128_GCM},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1_ChaChaPoly",
         UA_ServerConfig_addSecurityPolicyEccBrainpoolP256r1ChaChaPoly,
         {CERT_BP256R1_DER_LENGTH, CERT_BP256R1_DER_DATA},
         {KEY_BP256R1_DER_LENGTH, KEY_BP256R1_DER_DATA}, 64, 32, 32, true,
         CHACHA20_POLY1305},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1_AesGcm",
         UA_ServerConfig_addSecurityPolicyEccBrainpoolP384r1AesGcm,
         {CERT_BP384R1_DER_LENGTH, CERT_BP384R1_DER_DATA},
         {KEY_BP384R1_DER_LENGTH, KEY_BP384R1_DER_DATA}, 96, 48, 48, true,
         AES256_GCM},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1_ChaChaPoly",
         UA_ServerConfig_addSecurityPolicyEccBrainpoolP384r1ChaChaPoly,
         {CERT_BP384R1_DER_LENGTH, CERT_BP384R1_DER_DATA},
         {KEY_BP384R1_DER_LENGTH, KEY_BP384R1_DER_DATA}, 96, 48, 48, true,
         CHACHA20_POLY1305},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519_AesGcm",
         UA_ServerConfig_addSecurityPolicyEccCurve25519AesGcm,
         {CERT_ED25519_DER_LENGTH, CERT_ED25519_DER_DATA},
         {KEY_ED25519_DER_LENGTH, KEY_ED25519_DER_DATA}, 32, 32, 32, true,
         AES128_GCM},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519_ChaChaPoly",
         UA_ServerConfig_addSecurityPolicyEccCurve25519ChaChaPoly,
         {CERT_ED25519_DER_LENGTH, CERT_ED25519_DER_DATA},
         {KEY_ED25519_DER_LENGTH, KEY_ED25519_DER_DATA}, 32, 32, 32, true,
         CHACHA20_POLY1305},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448_AesGcm",
         UA_ServerConfig_addSecurityPolicyEccCurve448AesGcm,
         {CERT_ED448_DER_LENGTH, CERT_ED448_DER_DATA},
         {KEY_ED448_DER_LENGTH, KEY_ED448_DER_DATA}, 56, 56, 48, true,
         AES256_GCM},
        {"http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448_ChaChaPoly",
         UA_ServerConfig_addSecurityPolicyEccCurve448ChaChaPoly,
         {CERT_ED448_DER_LENGTH, CERT_ED448_DER_DATA},
         {KEY_ED448_DER_LENGTH, KEY_ED448_DER_DATA}, 56, 56, 48, true,
         CHACHA20_POLY1305},
#endif
    };

    for(size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        const EccPolicyConstants *r = &rows[i];
        UA_Server *server = UA_Server_newForUnitTest();
        UA_ServerConfig *config = UA_Server_getConfig(server);
        ck_assert_uint_eq(r->add(config, &r->cert, &r->key), UA_STATUSCODE_GOOD);

        UA_String uri = UA_STRING((char *)(uintptr_t)r->uri);
        const UA_SecurityPolicy *sp = NULL;
        for(size_t j = 0; j < config->securityPoliciesSize; j++) {
            if(UA_String_equal(&config->securityPolicies[j].policyUri, &uri))
                sp = &config->securityPolicies[j];
        }
        ck_assert_msg(sp != NULL, "policy %s not registered", r->uri);

        ck_assert_uint_eq(sp->nonceLength, r->nonceLength);
        ck_assert_uint_eq(sp->secretLength, r->secretLength);
        ck_assert_uint_eq(sp->certThumbprintLength, r->certThumbprintLength);
        ck_assert_int_eq(sp->secureChannelEnhancements, r->enhanced);

        /* The AEAD cipher is both the symmetric signature and encryption */
        if(r->symUri) {
            UA_String symUri = UA_STRING((char *)(uintptr_t)r->symUri);
            ck_assert(UA_String_equal(&sp->symSignatureAlgorithm.uri, &symUri));
            ck_assert(UA_String_equal(&sp->symEncryptionAlgorithm.uri, &symUri));
        }

        ck_assert_int_eq(UA_SecurityPolicy_isEnhancedSecurity(sp), r->enhanced);
        if(r->enhanced)
            ck_assert_uint_eq(sp->policyType, UA_SECURITYPOLICYTYPE_ECC_AEAD);

        UA_Server_delete(server);
    }
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
#ifdef UA_ENABLE_ENCRYPTION_OPENSSL
    tcase_add_test(tc, config_add_ecc_wrong_curve);
    tcase_add_test(tc, config_add_ecc_aead);
#endif
    tcase_add_test(tc, ecc_policies_declare_their_crypto_constants);
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
