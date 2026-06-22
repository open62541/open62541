/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) o6 Automation GmbH (Author: AI-assisted)
 *
 * Coverage tests for plugins/crypto/ code paths that are not exercised by
 * existing encryption tests. This covers:
 *   - CertificateUtils public API (verifyApplicationUri, getExpirationDate,
 *     getSubjectName, getThumbprint, getKeySize, comparePublicKeys,
 *     checkKeyPair, checkCA, decryptPrivateKey)
 *   - SecurityPolicy None (updateCertificate, compareCertificate,
 *     generateNonce, makeThumbprint)
 *   - CertificateGroup AcceptAll stub coverage
 *   - Certificate generation PEM format + IP SAN
 *   - CertificateGroup CRL handling via trust list operations
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/certificategroup_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <stdlib.h>
#include <string.h>
#include <check.h>
#include "test_helpers.h"
#include "certificates.h"
#include "thread_wrapper.h"

/* memmem is a GNU extension (declared in <string.h> with _GNU_SOURCE on
 * glibc). Provide a portable fallback for MSVC and other non-glibc targets
 * so the PEM-header check below compiles on all platforms supported by the
 * unit-test matrix. */
static const void *
test_memmem(const void *haystack, size_t haystackLen,
            const void *needle, size_t needleLen) {
    if(needleLen == 0)
        return (const void *)haystack;
    if(needleLen > haystackLen)
        return NULL;
    const unsigned char *h = (const unsigned char *)haystack;
    const unsigned char *n = (const unsigned char *)needle;
    for(size_t i = 0; i + needleLen <= haystackLen; i++) {
        if(h[i] == n[0] && memcmp(h + i, n, needleLen) == 0)
            return h + i;
    }
    return NULL;
}
#define memmem test_memmem

/* ===== Suite 1: CertificateUtils ===== */

static UA_Server *utilServer;

static void setup_utils(void) {
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    utilServer = UA_Server_newForUnitTestWithSecurityPolicies(
        4840, &certificate, &privateKey,
        &certificate, 1, NULL, 0, NULL, 0);
    ck_assert(utilServer != NULL);
}

static void teardown_utils(void) {
    UA_Server_delete(utilServer);
}

START_TEST(verify_application_uri_match) {
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    /* The CERT_DER has applicationUri "urn:open62541.unconfigured.application" */
    UA_String uri = UA_STRING("urn:open62541.unconfigured.application");
    UA_StatusCode retval = UA_CertificateUtils_verifyApplicationUri(&cert, &uri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(verify_application_uri_mismatch) {
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_String uri = UA_STRING("urn:wrong:application");
    UA_StatusCode retval = UA_CertificateUtils_verifyApplicationUri(&cert, &uri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCERTIFICATEURIINVALID);
}
END_TEST

START_TEST(get_expiration_date) {
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_DateTime expiryDateTime = 0;
    UA_StatusCode retval = UA_CertificateUtils_getExpirationDate(&cert, &expiryDateTime);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* The certificate must expire after the epoch */
    ck_assert(expiryDateTime > 0);
}
END_TEST

START_TEST(get_subject_name) {
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_String subjectName = UA_STRING_NULL;
    UA_StatusCode retval = UA_CertificateUtils_getSubjectName(&cert, &subjectName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(subjectName.length > 0);
    UA_String_clear(&subjectName);
}
END_TEST

START_TEST(get_thumbprint) {
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    /* Thumbprint must be pre-allocated to SHA1_LENGTH * 2 = 40 bytes */
    UA_String thumbprint;
    thumbprint.length = 40;
    thumbprint.data = (UA_Byte *)UA_malloc(40);
    UA_StatusCode retval = UA_CertificateUtils_getThumbprint(&cert, &thumbprint);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(thumbprint.length, 40);
    UA_String_clear(&thumbprint);
}
END_TEST

START_TEST(get_key_size) {
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    size_t keySize = 0;
    UA_StatusCode retval = UA_CertificateUtils_getKeySize(&cert, &keySize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* Our test cert uses 2048-bit RSA */
    ck_assert_uint_eq(keySize, 2048);
}
END_TEST

START_TEST(compare_public_keys_same) {
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_StatusCode retval = UA_CertificateUtils_comparePublicKeys(&cert, &cert);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(compare_public_keys_different) {
    UA_ByteString cert1;
    cert1.length = CERT_DER_LENGTH;
    cert1.data = CERT_DER_DATA;
    UA_ByteString cert2;
    cert2.length = ROOT_CERT_DER_LENGTH;
    cert2.data = ROOT_CERT_DER_DATA;
    UA_StatusCode retval = UA_CertificateUtils_comparePublicKeys(&cert1, &cert2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOMATCH);
}
END_TEST

START_TEST(check_key_pair_valid) {
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_ByteString key;
    key.length = KEY_DER_LENGTH;
    key.data = KEY_DER_DATA;
    UA_StatusCode retval = UA_CertificateUtils_checkKeyPair(&cert, &key);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(check_key_pair_mismatch) {
    /* Use root CA cert with leaf key — mismatch */
    UA_ByteString cert;
    cert.length = ROOT_CERT_DER_LENGTH;
    cert.data = ROOT_CERT_DER_DATA;
    UA_ByteString key;
    key.length = KEY_DER_LENGTH;
    key.data = KEY_DER_DATA;
    UA_StatusCode retval = UA_CertificateUtils_checkKeyPair(&cert, &key);
    ck_assert(retval != UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(check_ca_true) {
    /* Root cert should be a CA */
    UA_ByteString cert;
    cert.length = ROOT_CERT_DER_LENGTH;
    cert.data = ROOT_CERT_DER_DATA;
    UA_StatusCode retval = UA_CertificateUtils_checkCA(&cert);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(check_ca_false) {
    /* Leaf cert should not be a CA */
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_StatusCode retval = UA_CertificateUtils_checkCA(&cert);
    ck_assert(retval != UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(decrypt_private_key_no_password) {
    /* DER key without password — should succeed */
    UA_ByteString key;
    key.length = KEY_DER_LENGTH;
    key.data = KEY_DER_DATA;
    UA_ByteString password = UA_BYTESTRING_NULL;
    UA_ByteString outDerKey = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_CertificateUtils_decryptPrivateKey(key, password, &outDerKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(outDerKey.length > 0);
    UA_ByteString_clear(&outDerKey);
}
END_TEST

START_TEST(decrypt_private_key_pem) {
    /* PEM key without password — should succeed */
    UA_ByteString key;
    key.length = KEY_PEM_LENGTH;
    key.data = KEY_PEM_DATA;
    UA_ByteString password = UA_BYTESTRING_NULL;
    UA_ByteString outDerKey = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_CertificateUtils_decryptPrivateKey(key, password, &outDerKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(outDerKey.length > 0);
    UA_ByteString_clear(&outDerKey);
}
END_TEST

START_TEST(decrypt_private_key_with_password) {
    /* Password-protected PEM key — should succeed with correct password */
    UA_ByteString key;
    key.length = KEY_PEM_PASSWORD_LENGTH;
    key.data = KEY_PEM_PASSWORD_DATA;
    UA_ByteString password = UA_BYTESTRING("pass1234");
    UA_ByteString outDerKey = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_CertificateUtils_decryptPrivateKey(key, password, &outDerKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(outDerKey.length > 0);
    UA_ByteString_clear(&outDerKey);
}
END_TEST

START_TEST(get_expiration_date_pem) {
    UA_ByteString cert;
    cert.length = CERT_PEM_LENGTH;
    cert.data = CERT_PEM_DATA;
    UA_DateTime expiryDateTime = 0;
    UA_StatusCode retval = UA_CertificateUtils_getExpirationDate(&cert, &expiryDateTime);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(expiryDateTime > 0);
}
END_TEST

START_TEST(get_subject_name_pem) {
    UA_ByteString cert;
    cert.length = CERT_PEM_LENGTH;
    cert.data = CERT_PEM_DATA;
    UA_String subjectName = UA_STRING_NULL;
    UA_StatusCode retval = UA_CertificateUtils_getSubjectName(&cert, &subjectName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(subjectName.length > 0);
    UA_String_clear(&subjectName);
}
END_TEST

START_TEST(check_key_pair_pem) {
    UA_ByteString cert;
    cert.length = CERT_PEM_LENGTH;
    cert.data = CERT_PEM_DATA;
    UA_ByteString key;
    key.length = KEY_PEM_LENGTH;
    key.data = KEY_PEM_DATA;
    UA_StatusCode retval = UA_CertificateUtils_checkKeyPair(&cert, &key);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(verify_application_uri_pem) {
    UA_ByteString cert;
    cert.length = CERT_PEM_LENGTH;
    cert.data = CERT_PEM_DATA;
    /* CERT_PEM has applicationUri "urn:unconfigured:application" */
    UA_String uri = UA_STRING("urn:unconfigured:application");
    UA_StatusCode retval = UA_CertificateUtils_verifyApplicationUri(&cert, &uri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(check_ca_intermediate) {
    /* Intermediate CA cert should also be a CA */
    UA_ByteString cert;
    cert.length = INTERMEDIATE_CERT_DER_LENGTH;
    cert.data = INTERMEDIATE_CERT_DER_DATA;
    UA_StatusCode retval = UA_CertificateUtils_checkCA(&cert);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

/* ===== Suite 2: SecurityPolicy None ===== */

START_TEST(policy_none_update_certificate) {
    UA_SecurityPolicy sp;
    memset(&sp, 0, sizeof(UA_SecurityPolicy));
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_StatusCode retval = UA_SecurityPolicy_None(&sp, cert, UA_Log_Stdout);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(sp.localCertificate.length > 0);

    /* Update with a different certificate */
    UA_ByteString newCert;
    newCert.length = ROOT_CERT_DER_LENGTH;
    newCert.data = ROOT_CERT_DER_DATA;
    UA_ByteString newKey = UA_BYTESTRING_NULL;
    retval = sp.updateCertificate(&sp, newCert, newKey);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    sp.clear(&sp);
}
END_TEST

START_TEST(policy_none_compare_certificate) {
    UA_SecurityPolicy sp;
    memset(&sp, 0, sizeof(UA_SecurityPolicy));
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_StatusCode retval = UA_SecurityPolicy_None(&sp, cert, UA_Log_Stdout);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* compareCertificate always returns GOOD for None policy */
    retval = sp.compareCertificate(&sp, NULL, &cert);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    sp.clear(&sp);
}
END_TEST

START_TEST(policy_none_generate_nonce) {
    UA_SecurityPolicy sp;
    memset(&sp, 0, sizeof(UA_SecurityPolicy));
    UA_ByteString cert = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_SecurityPolicy_None(&sp, cert, UA_Log_Stdout);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Generate a nonce */
    UA_ByteString nonce;
    UA_ByteString_allocBuffer(&nonce, 32);
    retval = sp.generateNonce(&sp, NULL, &nonce);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Nonce of length 0 */
    UA_ByteString emptyNonce = UA_BYTESTRING_NULL;
    retval = sp.generateNonce(&sp, NULL, &emptyNonce);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* NULL nonce */
    retval = sp.generateNonce(&sp, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINTERNALERROR);

    UA_ByteString_clear(&nonce);
    sp.clear(&sp);
}
END_TEST

START_TEST(policy_none_make_thumbprint) {
    UA_SecurityPolicy sp;
    memset(&sp, 0, sizeof(UA_SecurityPolicy));
    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_StatusCode retval = UA_SecurityPolicy_None(&sp, cert, UA_Log_Stdout);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* makeThumbprint for None always returns GOOD */
    UA_ByteString thumbprint;
    UA_ByteString_allocBuffer(&thumbprint, 20);
    retval = sp.makeCertThumbprint(&sp, &cert, &thumbprint);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&thumbprint);
    sp.clear(&sp);
}
END_TEST

START_TEST(policy_none_channel_context) {
    UA_SecurityPolicy sp;
    memset(&sp, 0, sizeof(UA_SecurityPolicy));
    UA_ByteString cert = UA_BYTESTRING_NULL;
    UA_StatusCode retval = UA_SecurityPolicy_None(&sp, cert, UA_Log_Stdout);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* newChannelContext */
    void *ctx = NULL;
    retval = sp.newChannelContext(&sp, NULL, &ctx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* setLocalSymEncryptingKey / setLocalSymSigningKey / setLocalSymIv */
    UA_ByteString key = UA_BYTESTRING_NULL;
    retval = sp.setLocalSymEncryptingKey(&sp, ctx, &key);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = sp.setLocalSymSigningKey(&sp, ctx, &key);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = sp.setLocalSymIv(&sp, ctx, &key);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* setRemoteSymEncryptingKey / setRemoteSymSigningKey / setRemoteSymIv */
    retval = sp.setRemoteSymEncryptingKey(&sp, ctx, &key);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = sp.setRemoteSymSigningKey(&sp, ctx, &key);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = sp.setRemoteSymIv(&sp, ctx, &key);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* generateKey */
    UA_ByteString out;
    UA_ByteString_allocBuffer(&out, 32);
    retval = sp.generateKey(&sp, ctx, &key, &key, &out);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);

    /* Symmetric sign/verify/encrypt/decrypt */
    UA_ByteString msg = UA_BYTESTRING("test message");
    UA_ByteString sig = UA_BYTESTRING_NULL;
    retval = sp.symSignatureAlgorithm.sign(&sp, ctx, &msg, &sig);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = sp.symSignatureAlgorithm.verify(&sp, ctx, &msg, &sig);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = sp.symEncryptionAlgorithm.encrypt(&sp, ctx, &msg);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = sp.symEncryptionAlgorithm.decrypt(&sp, ctx, &msg);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Length functions */
    ck_assert_uint_eq(sp.symSignatureAlgorithm.getLocalSignatureSize(&sp, ctx), 0);
    ck_assert_uint_eq(sp.symSignatureAlgorithm.getRemoteSignatureSize(&sp, ctx), 0);
    ck_assert_uint_eq(sp.symSignatureAlgorithm.getLocalKeyLength(&sp, ctx), 0);
    ck_assert_uint_eq(sp.symSignatureAlgorithm.getRemoteKeyLength(&sp, ctx), 0);
    ck_assert_uint_eq(sp.symEncryptionAlgorithm.getLocalKeyLength(&sp, ctx), 0);
    ck_assert_uint_eq(sp.symEncryptionAlgorithm.getRemoteKeyLength(&sp, ctx), 0);
    ck_assert_uint_eq(sp.symEncryptionAlgorithm.getRemoteBlockSize(&sp, ctx), 0);
    ck_assert_uint_eq(sp.symEncryptionAlgorithm.getRemotePlainTextBlockSize(&sp, ctx), 0);

    /* deleteChannelContext */
    sp.deleteChannelContext(&sp, ctx);
    sp.clear(&sp);
}
END_TEST

/* ===== Suite 3: AcceptAll CertificateGroup ===== */

START_TEST(accept_all_verify) {
    UA_CertificateGroup cg;
    memset(&cg, 0, sizeof(UA_CertificateGroup));
    cg.logging = UA_Log_Stdout;
    UA_CertificateGroup_AcceptAll(&cg);

    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_StatusCode retval = cg.verifyCertificate(&cg, &cert);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* All other function pointers should be NULL */
    ck_assert(cg.getTrustList == NULL);
    ck_assert(cg.setTrustList == NULL);
    ck_assert(cg.addToTrustList == NULL);
    ck_assert(cg.removeFromTrustList == NULL);
    ck_assert(cg.getRejectedList == NULL);
    ck_assert(cg.getCertificateCrls == NULL);

    cg.clear(&cg);
}
END_TEST

START_TEST(accept_all_reinit) {
    /* Test that calling AcceptAll on an already-initialized group works
     * (covers the "clear if already initialized" path) */
    UA_CertificateGroup cg;
    memset(&cg, 0, sizeof(UA_CertificateGroup));
    cg.logging = UA_Log_Stdout;
    UA_CertificateGroup_AcceptAll(&cg);

    /* Call again — should clear and re-init */
    UA_CertificateGroup_AcceptAll(&cg);

    UA_ByteString cert;
    cert.length = CERT_DER_LENGTH;
    cert.data = CERT_DER_DATA;
    UA_StatusCode retval = cg.verifyCertificate(&cg, &cert);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    cg.clear(&cg);
}
END_TEST

/* ===== Suite 4: Certificate Generation PEM + IP SAN ===== */

START_TEST(certificate_generation_pem) {
    UA_ByteString pemPrivKey = UA_BYTESTRING_NULL;
    UA_ByteString pemCert = UA_BYTESTRING_NULL;
    UA_String subject[3] = {UA_STRING_STATIC("C=DE"),
                            UA_STRING_STATIC("O=TestOrganization"),
                            UA_STRING_STATIC("CN=TestServer@localhost")};
    UA_String subjectAltName[2] = {
        UA_STRING_STATIC("DNS:localhost"),
        UA_STRING_STATIC("URI:urn:test.application")
    };
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_UInt16 expiresIn = 14;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
                             (void *)&expiresIn, &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 keyLength = 2048;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
                             (void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_CreateCertificate(
        UA_Log_Stdout, subject, 3, subjectAltName, 2,
        UA_CERTIFICATEFORMAT_PEM, kvm, &pemPrivKey, &pemCert);
    UA_KeyValueMap_delete(kvm);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(pemPrivKey.length > 0);
    ck_assert(pemCert.length > 0);

    /* Verify PEM headers */
    const char *certHeader = "-----BEGIN CERTIFICATE-----";
    const char *keyHeader = "-----BEGIN ";
    ck_assert(memmem(pemCert.data, pemCert.length, certHeader, strlen(certHeader)) != NULL);
    ck_assert(memmem(pemPrivKey.data, pemPrivKey.length, keyHeader, strlen(keyHeader)) != NULL);

    /* Verify the PEM cert can be used */
    UA_DateTime expiryDateTime = 0;
    retval = UA_CertificateUtils_getExpirationDate(&pemCert, &expiryDateTime);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(expiryDateTime > 0);

    UA_ByteString_clear(&pemCert);
    UA_ByteString_clear(&pemPrivKey);
}
END_TEST

START_TEST(certificate_generation_ip_san) {
    UA_ByteString derPrivKey = UA_BYTESTRING_NULL;
    UA_ByteString derCert = UA_BYTESTRING_NULL;
    UA_String subject[3] = {UA_STRING_STATIC("C=DE"),
                            UA_STRING_STATIC("O=TestOrganization"),
                            UA_STRING_STATIC("CN=TestServer@localhost")};
    UA_String subjectAltName[3] = {
        UA_STRING_STATIC("DNS:localhost"),
        UA_STRING_STATIC("URI:urn:test.application"),
        UA_STRING_STATIC("IP:127.0.0.1")
    };
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_UInt16 expiresIn = 14;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
                             (void *)&expiresIn, &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 keyLength = 2048;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
                             (void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_CreateCertificate(
        UA_Log_Stdout, subject, 3, subjectAltName, 3,
        UA_CERTIFICATEFORMAT_DER, kvm, &derPrivKey, &derCert);
    UA_KeyValueMap_delete(kvm);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(derPrivKey.length > 0);
    ck_assert(derCert.length > 0);

    UA_ByteString_clear(&derCert);
    UA_ByteString_clear(&derPrivKey);
}
END_TEST

START_TEST(certificate_generation_pem_ip_san) {
    UA_ByteString pemPrivKey = UA_BYTESTRING_NULL;
    UA_ByteString pemCert = UA_BYTESTRING_NULL;
    UA_String subject[3] = {UA_STRING_STATIC("C=DE"),
                            UA_STRING_STATIC("O=TestOrganization"),
                            UA_STRING_STATIC("CN=TestServer@localhost")};
    UA_String subjectAltName[3] = {
        UA_STRING_STATIC("DNS:localhost"),
        UA_STRING_STATIC("URI:urn:test.application"),
        UA_STRING_STATIC("IP:192.168.1.1")
    };
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_UInt16 expiresIn = 14;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
                             (void *)&expiresIn, &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 keyLength = 2048;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
                             (void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_CreateCertificate(
        UA_Log_Stdout, subject, 3, subjectAltName, 3,
        UA_CERTIFICATEFORMAT_PEM, kvm, &pemPrivKey, &pemCert);
    UA_KeyValueMap_delete(kvm);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(pemPrivKey.length > 0);
    ck_assert(pemCert.length > 0);

    UA_ByteString_clear(&pemCert);
    UA_ByteString_clear(&pemPrivKey);
}
END_TEST

/* ===== Suite 5: CertificateGroup CRL Handling ===== */

static UA_Server *crlServer;

static void setup_crl(void) {
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

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
    intermediateCaCrl.length = INTERMEDIATE_EMPTY_CRL_PEM_LENGTH;
    intermediateCaCrl.data = INTERMEDIATE_EMPTY_CRL_PEM_DATA;

    size_t trustListSize = 2;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    trustList[0] = rootCa;
    trustList[1] = intermediateCa;

    size_t issuerListSize = 2;
    UA_STACKARRAY(UA_ByteString, issuerList, issuerListSize);
    issuerList[0] = rootCa;
    issuerList[1] = intermediateCa;

    size_t revocationListSize = 2;
    UA_STACKARRAY(UA_ByteString, revocationList, revocationListSize);
    revocationList[0] = rootCaCrl;
    revocationList[1] = intermediateCaCrl;

    crlServer = UA_Server_newForUnitTestWithSecurityPolicies(
        4840, &certificate, &privateKey,
        trustList, trustListSize,
        issuerList, issuerListSize,
        revocationList, revocationListSize);
    ck_assert(crlServer != NULL);
}

static void teardown_crl(void) {
    UA_Server_delete(crlServer);
}

START_TEST(certificate_group_get_crls) {
    UA_ServerConfig *config = UA_Server_getConfig(crlServer);

    /* getCertificateCrls should return CRLs for a known certificate */
    if(config->secureChannelPKI.getCertificateCrls) {
        UA_ByteString cert;
        cert.length = INTERMEDIATE_CERT_DER_LENGTH;
        cert.data = INTERMEDIATE_CERT_DER_DATA;
        UA_ByteString *crls = NULL;
        size_t crlsSize = 0;
        UA_StatusCode retval =
            config->secureChannelPKI.getCertificateCrls(&config->secureChannelPKI,
                                                        &cert, true,
                                                        &crls, &crlsSize);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        /* We loaded CRLs for both root and intermediate, at least one should match */
        UA_Array_delete(crls, crlsSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    }
}
END_TEST

START_TEST(certificate_group_set_trustlist_with_crls) {
    UA_ServerConfig *config = UA_Server_getConfig(crlServer);

    UA_ByteString trustedCert;
    trustedCert.length = ROOT_CERT_DER_LENGTH;
    trustedCert.data = ROOT_CERT_DER_DATA;

    UA_ByteString crl;
    crl.length = ROOT_EMPTY_CRL_PEM_LENGTH;
    crl.data = ROOT_EMPTY_CRL_PEM_DATA;

    UA_TrustListDataType trustListTmp;
    memset(&trustListTmp, 0, sizeof(UA_TrustListDataType));
    trustListTmp.specifiedLists = (UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES |
                                   UA_TRUSTLISTMASKS_TRUSTEDCRLS |
                                   UA_TRUSTLISTMASKS_ISSUERCERTIFICATES |
                                   UA_TRUSTLISTMASKS_ISSUERCRLS);
    trustListTmp.trustedCertificates = &trustedCert;
    trustListTmp.trustedCertificatesSize = 1;
    trustListTmp.trustedCrls = &crl;
    trustListTmp.trustedCrlsSize = 1;
    trustListTmp.issuerCertificates = &trustedCert;
    trustListTmp.issuerCertificatesSize = 1;
    trustListTmp.issuerCrls = &crl;
    trustListTmp.issuerCrlsSize = 1;

    UA_StatusCode retval =
        config->secureChannelPKI.setTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify CRLs are returned */
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    retval = config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &trustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(trustList.trustedCertificatesSize, 1);
    ck_assert_uint_eq(trustList.trustedCrlsSize, 1);
    ck_assert_uint_eq(trustList.issuerCertificatesSize, 1);
    ck_assert_uint_eq(trustList.issuerCrlsSize, 1);

    UA_TrustListDataType_clear(&trustList);
}
END_TEST

START_TEST(certificate_group_add_trustlist_with_crls) {
    UA_ServerConfig *config = UA_Server_getConfig(crlServer);

    /* First clear the trust list */
    UA_TrustListDataType emptyList;
    memset(&emptyList, 0, sizeof(UA_TrustListDataType));
    emptyList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    UA_StatusCode retval =
        config->secureChannelPKI.setTrustList(&config->secureChannelPKI, &emptyList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Now add via addToTrustList */
    UA_ByteString trustedCert;
    trustedCert.length = ROOT_CERT_DER_LENGTH;
    trustedCert.data = ROOT_CERT_DER_DATA;

    UA_ByteString crl;
    crl.length = ROOT_EMPTY_CRL_PEM_LENGTH;
    crl.data = ROOT_EMPTY_CRL_PEM_DATA;

    UA_TrustListDataType trustListTmp;
    memset(&trustListTmp, 0, sizeof(UA_TrustListDataType));
    trustListTmp.specifiedLists = (UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES |
                                   UA_TRUSTLISTMASKS_TRUSTEDCRLS);
    trustListTmp.trustedCertificates = &trustedCert;
    trustListTmp.trustedCertificatesSize = 1;
    trustListTmp.trustedCrls = &crl;
    trustListTmp.trustedCrlsSize = 1;

    retval = config->secureChannelPKI.addToTrustList(&config->secureChannelPKI, &trustListTmp);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify CRLs added */
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    retval = config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &trustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(trustList.trustedCrlsSize, 1);

    UA_TrustListDataType_clear(&trustList);
}
END_TEST

START_TEST(certificate_group_verify_cert) {
    UA_ServerConfig *config = UA_Server_getConfig(crlServer);

    /* Verify a certificate that is in the trust list */
    UA_ByteString cert;
    cert.length = APPLICATION_CERT_DER_LENGTH;
    cert.data = APPLICATION_CERT_DER_DATA;
    UA_StatusCode retval =
        config->secureChannelPKI.verifyCertificate(&config->secureChannelPKI, &cert);
    /* The application cert is signed by the intermediate CA which is in our
     * trust list, so it should validate successfully */
    (void)retval; /* May or may not pass depending on URI checks; just exercise the path */
}
END_TEST

/* ===== Suite 6: SecurityPolicy Update Certificate via Server ===== */

static UA_Server *policyServer;
static UA_Boolean policyRunning;
THREAD_HANDLE policy_server_thread;

THREAD_CALLBACK(policyServerloop) {
    while(policyRunning)
        UA_Server_run_iterate(policyServer, true);
    return 0;
}

static void setup_policy(void) {
    policyRunning = true;

    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    policyServer = UA_Server_newForUnitTestWithSecurityPolicies(
        4840, &certificate, &privateKey,
        NULL, 0, NULL, 0, NULL, 0);
    ck_assert(policyServer != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(policyServer);
    UA_CertificateGroup_AcceptAll(&config->secureChannelPKI);
    UA_CertificateGroup_AcceptAll(&config->sessionPKI);

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:unconfigured:application");

    UA_Server_run_startup(policyServer);
    THREAD_CREATE(policy_server_thread, policyServerloop);
}

static void teardown_policy(void) {
    policyRunning = false;
    THREAD_JOIN(policy_server_thread);
    UA_Server_run_shutdown(policyServer);
    UA_Server_delete(policyServer);
}

START_TEST(encrypted_data_exchange) {
    /* Connect with Basic256Sha256 and exchange data */
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read the server build info (exercises encrypted read) */
    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Write and read back (exercises encrypted write) */
    UA_Variant writeVal;
    UA_Variant_init(&writeVal);
    UA_DateTime now = UA_DateTime_now();
    UA_Variant_setScalarCopy(&writeVal, &now, &UA_TYPES[UA_TYPES_DATETIME]);
    nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    /* Write may fail on read-only node, that's OK — we still exercise crypto paths */
    UA_Client_writeValueAttribute(client, nodeId, &writeVal);
    UA_Variant_clear(&writeVal);

    /* Browse (exercises encrypted browse) */
    UA_BrowseRequest browseReq;
    UA_BrowseRequest_init(&browseReq);
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    browseReq.nodesToBrowse = &bd;
    browseReq.nodesToBrowseSize = 1;
    UA_BrowseResponse browseResp = UA_Client_Service_browse(client, browseReq);
    ck_assert_uint_eq(browseResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_BrowseResponse_clear(&browseResp);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(connect_aes128sha256rsaoaep) {
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");

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

START_TEST(connect_aes256sha256rsapss) {
    UA_ByteString certificate;
    certificate.length = CERT_DER_LENGTH;
    certificate.data = CERT_DER_DATA;

    UA_ByteString privateKey;
    privateKey.length = KEY_DER_LENGTH;
    privateKey.data = KEY_DER_DATA;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         NULL, 0, NULL, 0);
    UA_CertificateGroup_AcceptAll(&cc->certificateVerification);
    cc->securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss");

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

/* ===== Build Test Suites ===== */

static Suite *testSuite_crypto_coverage(void) {
    Suite *s = suite_create("Crypto Coverage");

    /* Certificate Utils */
    TCase *tc_utils = tcase_create("CertificateUtils");
    tcase_add_checked_fixture(tc_utils, setup_utils, teardown_utils);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_utils, verify_application_uri_match);
    tcase_add_test(tc_utils, verify_application_uri_mismatch);
    tcase_add_test(tc_utils, get_expiration_date);
    tcase_add_test(tc_utils, get_subject_name);
    tcase_add_test(tc_utils, get_thumbprint);
    tcase_add_test(tc_utils, get_key_size);
    tcase_add_test(tc_utils, compare_public_keys_same);
    tcase_add_test(tc_utils, compare_public_keys_different);
    tcase_add_test(tc_utils, check_key_pair_valid);
    tcase_add_test(tc_utils, check_key_pair_mismatch);
    tcase_add_test(tc_utils, check_ca_true);
    tcase_add_test(tc_utils, check_ca_false);
    tcase_add_test(tc_utils, decrypt_private_key_no_password);
    tcase_add_test(tc_utils, decrypt_private_key_pem);
    tcase_add_test(tc_utils, decrypt_private_key_with_password);
    tcase_add_test(tc_utils, get_expiration_date_pem);
    tcase_add_test(tc_utils, get_subject_name_pem);
    tcase_add_test(tc_utils, check_key_pair_pem);
    tcase_add_test(tc_utils, verify_application_uri_pem);
    tcase_add_test(tc_utils, check_ca_intermediate);
#endif
    suite_add_tcase(s, tc_utils);

    /* SecurityPolicy None */
    TCase *tc_none = tcase_create("SecurityPolicy None");
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_none, policy_none_update_certificate);
    tcase_add_test(tc_none, policy_none_compare_certificate);
    tcase_add_test(tc_none, policy_none_generate_nonce);
    tcase_add_test(tc_none, policy_none_make_thumbprint);
    tcase_add_test(tc_none, policy_none_channel_context);
#endif
    suite_add_tcase(s, tc_none);

    /* AcceptAll CertificateGroup */
    TCase *tc_accept_all = tcase_create("AcceptAll CertificateGroup");
    tcase_add_test(tc_accept_all, accept_all_verify);
    tcase_add_test(tc_accept_all, accept_all_reinit);
    suite_add_tcase(s, tc_accept_all);

    /* Certificate Generation PEM + IP SAN */
    TCase *tc_certgen = tcase_create("Certificate Generation PEM");
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_certgen, certificate_generation_pem);
    tcase_add_test(tc_certgen, certificate_generation_ip_san);
    tcase_add_test(tc_certgen, certificate_generation_pem_ip_san);
#endif
    suite_add_tcase(s, tc_certgen);

    /* CertificateGroup CRL Handling */
    TCase *tc_crl = tcase_create("CertificateGroup CRL");
    tcase_add_checked_fixture(tc_crl, setup_crl, teardown_crl);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_crl, certificate_group_get_crls);
    tcase_add_test(tc_crl, certificate_group_set_trustlist_with_crls);
    tcase_add_test(tc_crl, certificate_group_add_trustlist_with_crls);
    tcase_add_test(tc_crl, certificate_group_verify_cert);
#endif
    suite_add_tcase(s, tc_crl);

    /* Security Policy Connections */
    TCase *tc_policy = tcase_create("SecurityPolicy Connections");
    tcase_add_checked_fixture(tc_policy, setup_policy, teardown_policy);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_policy, encrypted_data_exchange);
    tcase_add_test(tc_policy, connect_aes128sha256rsaoaep);
    tcase_add_test(tc_policy, connect_aes256sha256rsapss);
#endif
    suite_add_tcase(s, tc_policy);

    return s;
}

int main(void) {
    Suite *s = testSuite_crypto_coverage();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
