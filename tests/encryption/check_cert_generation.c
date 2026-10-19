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

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include <openssl/pem.h>
#include <openssl/x509v3.h>
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

START_TEST(certificate_without_application_uri_is_rejected) {
    UA_String subject = UA_STRING_STATIC("CN=localhost");
    UA_String subjectAltName = UA_STRING_STATIC("DNS:localhost");
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_ByteString certificate = UA_BYTESTRING_NULL;

    UA_StatusCode status = UA_CreateCertificate(
        UA_Log_Stdout, &subject, 1, &subjectAltName, 1,
        UA_CERTIFICATEFORMAT_DER, NULL, &privateKey, &certificate);
    ck_assert_uint_eq(status, UA_STATUSCODE_GOOD);

    UA_String emptyUri = UA_STRING_NULL;
    status = UA_CertificateUtils_verifyApplicationUri(&certificate, &emptyUri);
    ck_assert_uint_eq(status, UA_STATUSCODE_BADCERTIFICATEURIINVALID);

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
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

START_TEST(certificate_generation_rejects_missing_arguments) {
    UA_String subject = UA_STRING_STATIC("CN=localhost");
    UA_String altName = UA_STRING_STATIC("DNS:localhost");
    UA_Byte keySentinel = 0x11, certSentinel = 0x22;
    UA_ByteString key = {1, &keySentinel};
    UA_ByteString cert = {1, &certSentinel};
    UA_StatusCode status = UA_CreateCertificate(
        _i == 0 ? NULL : UA_Log_Stdout,
        _i == 1 ? NULL : &subject, _i == 2 ? 0 : 1,
        _i == 3 ? NULL : &altName, _i == 4 ? 0 : 1,
        _i == 5 ? (UA_CertificateFormat)99 : UA_CERTIFICATEFORMAT_DER,
        NULL, _i == 6 ? NULL : &key, _i == 7 ? NULL : &cert);
    ck_assert_uint_eq(status, UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_ptr_eq(key.data, &keySentinel);
    ck_assert_uint_eq(key.length, 1);
    ck_assert_ptr_eq(cert.data, &certSentinel);
    ck_assert_uint_eq(cert.length, 1);
    ck_assert_uint_eq(keySentinel, 0x11);
    ck_assert_uint_eq(certSentinel, 0x22);
}
END_TEST

static UA_KeyValueMap *
certificateParams(UA_Boolean ecc, const UA_String *curve) {
    UA_KeyValueMap *params = UA_KeyValueMap_new();
    ck_assert_ptr_ne(params, NULL);
    UA_UInt16 keyBits = 2048, expires = 7;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(
        params, UA_QUALIFIEDNAME(0, "key-size-bits"),
        &keyBits, &UA_TYPES[UA_TYPES_UINT16]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(
        params, UA_QUALIFIEDNAME(0, "expires-in-days"),
        &expires, &UA_TYPES[UA_TYPES_UINT16]), UA_STATUSCODE_GOOD);
    if(ecc) {
        UA_String keyType = UA_STRING_STATIC("eC");
        ck_assert_uint_eq(UA_KeyValueMap_setScalar(
            params, UA_QUALIFIEDNAME(0, "key-type"),
            &keyType, &UA_TYPES[UA_TYPES_STRING]), UA_STATUSCODE_GOOD);
    }
    if(curve)
        ck_assert_uint_eq(UA_KeyValueMap_setScalar(
            params, UA_QUALIFIEDNAME(0, "ecc-curve"),
            curve, &UA_TYPES[UA_TYPES_STRING]), UA_STATUSCODE_GOOD);
    return params;
}

static UA_KeyValueMap *
certificateValidationParams(void) {
    /* Avoid expensive RSA key generation when testing later input checks. */
#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS) || \
    (defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L)
    return certificateParams(true, NULL);
#else
    return certificateParams(false, NULL);
#endif
}

START_TEST(certificate_generation_rejects_invalid_subjects) {
    static const UA_String subjects[] = {
        UA_STRING_STATIC("missing-delimiter"),
        UA_STRING_STATIC("=localhost"),
        UA_STRING_STATIC("CN="),
        UA_STRING_STATIC("123456789012345=localhost"),
        UA_STRING_STATIC("unknown=localhost")
    };
    UA_String altName = UA_STRING_STATIC("DNS:localhost");
    UA_KeyValueMap *params = certificateValidationParams();
    UA_ByteString key = UA_BYTESTRING_NULL, cert = UA_BYTESTRING_NULL;
    UA_StatusCode status = UA_CreateCertificate(
        UA_Log_Stdout, &subjects[_i], 1, &altName, 1,
        UA_CERTIFICATEFORMAT_DER, params, &key, &cert);
    UA_KeyValueMap_delete(params);
    ck_assert_uint_eq(status, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_ptr_eq(key.data, NULL);
    ck_assert_uint_eq(key.length, 0);
    ck_assert_ptr_eq(cert.data, NULL);
    ck_assert_uint_eq(cert.length, 0);
}
END_TEST

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS) || \
    (defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L)

static void
checkGeneratedCertificate(UA_ByteString *cert, const UA_ByteString *key) {
    ck_assert_uint_gt(cert->length, 0);
    ck_assert_uint_gt(key->length, 0);
    ck_assert_uint_eq(UA_CertificateUtils_checkKeyPair(cert, key), UA_STATUSCODE_GOOD);
    UA_String uri = UA_STRING_STATIC("urn:test.generated");
    ck_assert_uint_eq(UA_CertificateUtils_verifyApplicationUri(cert, &uri),
                      UA_STATUSCODE_GOOD);
    UA_CertificateEku usage = UA_CERTIFICATEEKU_NONE;
    ck_assert_uint_eq(UA_CertificateUtils_getExtendedKeyUsage(cert, &usage),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(usage, UA_CERTIFICATEEKU_SERVERAUTH | UA_CERTIFICATEEKU_CLIENTAUTH);
}

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
static X509 *
parseGeneratedCertificate(const UA_ByteString *cert, UA_CertificateFormat format) {
    BIO *bio = BIO_new_mem_buf(cert->data, (int)cert->length);
    ck_assert_ptr_ne(bio, NULL);
    X509 *parsed = format == UA_CERTIFICATEFORMAT_PEM ?
        PEM_read_bio_X509(bio, NULL, NULL, NULL) : d2i_X509_bio(bio, NULL);
    BIO_free(bio);
    ck_assert_ptr_ne(parsed, NULL);
    return parsed;
}
#endif

START_TEST(certificate_generation_ecc_curves) {
    /* Include aliases, mixed case, an absent curve and an empty curve. */
    static const UA_String curves[] = {
        UA_STRING_STATIC("PrImE256v1"), UA_STRING_STATIC("NISTP256"),
        UA_STRING_STATIC("SECP384R1"), UA_STRING_STATIC("nistp384"),
        UA_STRING_STATIC("brainpoolP256r1"), UA_STRING_STATIC("BRAINPOOLP384R1"),
        UA_STRING_STATIC(""), UA_STRING_STATIC("")
    };
    static const unsigned curveIndexes[] = {0, 0, 1, 1, 2, 3, 0, 0};
    static const unsigned curveBits[] = {256, 384, 256, 384};
    unsigned caseIndex = (unsigned)_i / 2;
    unsigned curveIndex = curveIndexes[caseIndex];
    unsigned bits = curveBits[curveIndex];
    UA_CertificateFormat format = (_i % 2) ?
        UA_CERTIFICATEFORMAT_PEM : UA_CERTIFICATEFORMAT_DER;
    UA_KeyValueMap *params = certificateParams(true, caseIndex == 6 ? NULL :
                                              &curves[caseIndex]);
    UA_String subject = UA_STRING_STATIC("CN=localhost");
    UA_String altNames[] = {UA_STRING_STATIC("DNS:localhost"),
                           UA_STRING_STATIC("URI:urn:test.generated")};
    UA_ByteString key = UA_BYTESTRING_NULL, cert = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_CreateCertificate(
        UA_Log_Stdout, &subject, 1, altNames, 2, format, params, &key, &cert),
        UA_STATUSCODE_GOOD);
    UA_KeyValueMap_delete(params);
    checkGeneratedCertificate(&cert, &key);

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
    mbedtls_x509_crt parsed;
    mbedtls_x509_crt_init(&parsed);
    /* PEM parsing requires a trailing NUL, which is not part of UA_String. */
    UA_ByteString terminated = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&terminated, cert.length + 1),
                      UA_STATUSCODE_GOOD);
    memcpy(terminated.data, cert.data, cert.length);
    terminated.data[cert.length] = 0;
    ck_assert_int_eq(mbedtls_x509_crt_parse(&parsed, terminated.data,
        format == UA_CERTIFICATEFORMAT_PEM ? terminated.length : cert.length), 0);
    UA_ByteString_clear(&terminated);
    ck_assert_uint_eq(mbedtls_pk_get_bitlen(&parsed.pk), bits);
    ck_assert_int_eq(mbedtls_x509_crt_check_key_usage(
        &parsed, MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_KEY_AGREEMENT), 0);
    if(bits == 384)
        ck_assert_int_eq(MBEDTLS_OID_CMP(MBEDTLS_OID_ECDSA_SHA384, &parsed.sig_oid), 0);
    else
        ck_assert_int_eq(MBEDTLS_OID_CMP(MBEDTLS_OID_ECDSA_SHA256, &parsed.sig_oid), 0);

    /* Inspect the public curve OID without relying on backend key internals. */
    static const UA_ByteString curveOids[] = {
        UA_STRING_STATIC("\x2a\x86\x48\xce\x3d\x03\x01\x07"), /* prime256v1 */
        UA_STRING_STATIC("\x2b\x81\x04\x00\x22"),             /* secp384r1 */
        UA_STRING_STATIC("\x2b\x24\x03\x03\x02\x08\x01\x01\x07"), /* brainpoolP256r1 */
        UA_STRING_STATIC("\x2b\x24\x03\x03\x02\x08\x01\x01\x0b")  /* brainpoolP384r1 */
    };
    unsigned char *pos = parsed.pk_raw.p;
    const unsigned char *end = pos + parsed.pk_raw.len;
    size_t length;
    ck_assert_int_eq(mbedtls_asn1_get_tag(&pos, end, &length,
        MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE), 0);
    mbedtls_asn1_buf algorithm, curve;
    ck_assert_int_eq(mbedtls_asn1_get_alg(&pos, end, &algorithm, &curve), 0);
    ck_assert_int_eq(curve.tag, MBEDTLS_ASN1_OID);
    UA_ByteString curveOid = {curve.len, curve.p};
    ck_assert(UA_ByteString_equal(&curveOid, &curveOids[curveIndex]));
    uint32_t flags = 0;
    int verifyResult = mbedtls_x509_crt_verify(&parsed, &parsed, NULL,
                                               "localhost", &flags, NULL, NULL);
    ck_assert_msg(verifyResult == 0, "certificate verification failed: %d, flags: 0x%08x",
                  verifyResult, (unsigned)flags);
    ck_assert_uint_eq(flags, 0);
    mbedtls_x509_crt_free(&parsed);
#else
    static const char *curveNames[] = {
        "prime256v1", "secp384r1", "brainpoolP256r1", "brainpoolP384r1"
    };
    X509 *parsed = parseGeneratedCertificate(&cert, format);
    EVP_PKEY *publicKey = X509_get_pubkey(parsed);
    ck_assert_ptr_ne(publicKey, NULL);
    ck_assert_int_eq(EVP_PKEY_base_id(publicKey), EVP_PKEY_EC);
    ck_assert_int_eq(EVP_PKEY_bits(publicKey), (int)bits);
    char curveName[80];
    size_t nameLength;
    ck_assert_int_eq(EVP_PKEY_get_group_name(publicKey, curveName,
                                            sizeof(curveName), &nameLength), 1);
    ck_assert_str_eq(curveName, curveNames[curveIndex]);
    ck_assert_int_eq(X509_get_signature_nid(parsed), bits == 384 ?
                     NID_ecdsa_with_SHA384 : NID_ecdsa_with_SHA256);
    ck_assert_int_eq(X509_verify(parsed, publicKey), 1);
    EVP_PKEY_free(publicKey);
    X509_free(parsed);
#endif
    UA_ByteString_clear(&key);
    UA_ByteString_clear(&cert);
}
END_TEST

START_TEST(certificate_generation_unsupported_curve) {
    UA_String curve = UA_STRING_STATIC("not-a-curve");
    UA_KeyValueMap *params = certificateParams(true, &curve);
    UA_String subject = UA_STRING_STATIC("CN=localhost");
    UA_String altName = UA_STRING_STATIC("DNS:localhost");
    UA_ByteString key = UA_BYTESTRING_NULL, cert = UA_BYTESTRING_NULL;
    UA_StatusCode status = UA_CreateCertificate(
        UA_Log_Stdout, &subject, 1, &altName, 1,
        UA_CERTIFICATEFORMAT_DER, params, &key, &cert);
    UA_KeyValueMap_delete(params);
#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
    ck_assert_uint_eq(status, UA_STATUSCODE_BADINVALIDARGUMENT);
#else
    ck_assert_uint_eq(status, UA_STATUSCODE_BADINTERNALERROR);
#endif
    ck_assert_ptr_eq(key.data, NULL);
    ck_assert_uint_eq(key.length, 0);
    ck_assert_ptr_eq(cert.data, NULL);
    ck_assert_uint_eq(cert.length, 0);
}
END_TEST

START_TEST(certificate_generation_eddsa) {
    static const UA_String curves[] = {
        UA_STRING_STATIC("ED25519"), UA_STRING_STATIC("Ed448")
    };
    UA_KeyValueMap *params = certificateParams(true, &curves[_i / 2]);
    UA_CertificateFormat format = (_i % 2) ?
        UA_CERTIFICATEFORMAT_PEM : UA_CERTIFICATEFORMAT_DER;
    UA_String subject = UA_STRING_STATIC("CN=localhost");
    UA_String altName = UA_STRING_STATIC("URI:urn:test.generated");
    UA_ByteString key = UA_BYTESTRING_NULL, cert = UA_BYTESTRING_NULL;
    UA_StatusCode status = UA_CreateCertificate(
        UA_Log_Stdout, &subject, 1, &altName, 1, format, params, &key, &cert);
    UA_KeyValueMap_delete(params);
#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
    ck_assert_uint_eq(status, UA_STATUSCODE_BADNOTIMPLEMENTED);
    ck_assert_ptr_eq(key.data, NULL);
    ck_assert_uint_eq(key.length, 0);
    ck_assert_ptr_eq(cert.data, NULL);
    ck_assert_uint_eq(cert.length, 0);
#else
    ck_assert_uint_eq(status, UA_STATUSCODE_GOOD);
    checkGeneratedCertificate(&cert, &key);
    X509 *parsed = parseGeneratedCertificate(&cert, format);
    EVP_PKEY *publicKey = X509_get_pubkey(parsed);
    ck_assert_ptr_ne(publicKey, NULL);
    int expectedType = _i / 2 == 0 ? EVP_PKEY_ED25519 : EVP_PKEY_ED448;
    ck_assert_int_eq(EVP_PKEY_base_id(publicKey), expectedType);
    ck_assert_int_eq(X509_get_signature_nid(parsed), expectedType);
    ck_assert_int_eq(X509_verify(parsed, publicKey), 1);
    EVP_PKEY_free(publicKey);
    X509_free(parsed);
#endif
    UA_ByteString_clear(&key);
    UA_ByteString_clear(&cert);
}
END_TEST
#endif

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
START_TEST(certificate_generation_email_san) {
    UA_String subject = UA_STRING_STATIC("CN=localhost");
    UA_String altNames[] = {UA_STRING_STATIC("URI:urn:test.generated"),
                           UA_STRING_STATIC("RFC822:test@example.org")};
    UA_KeyValueMap *params = certificateValidationParams();
    UA_ByteString key = UA_BYTESTRING_NULL, cert = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_CreateCertificate(
        UA_Log_Stdout, &subject, 1, altNames, 2,
        UA_CERTIFICATEFORMAT_DER, params, &key, &cert), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_delete(params);
    checkGeneratedCertificate(&cert, &key);
    mbedtls_x509_crt parsed;
    mbedtls_x509_crt_init(&parsed);
    ck_assert_int_eq(mbedtls_x509_crt_parse_der(&parsed, cert.data, cert.length), 0);
    unsigned nameCount = 0, emailCount = 0;
    for(const mbedtls_x509_sequence *san = &parsed.subject_alt_names;
        san; san = san->next) {
        nameCount++;
        if(san->buf.tag != (MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_X509_SAN_RFC822_NAME))
            continue;
        UA_String value = {san->buf.len, san->buf.p};
        UA_String expected = UA_STRING_STATIC("test@example.org");
        ck_assert(UA_String_equal(&value, &expected));
        emailCount++;
    }
    ck_assert_uint_eq(nameCount, 2);
    ck_assert_uint_eq(emailCount, 1);
    mbedtls_x509_crt_free(&parsed);
    UA_ByteString_clear(&key);
    UA_ByteString_clear(&cert);
}
END_TEST
#endif

START_TEST(certificate_generation_rejects_invalid_san) {
    static const char *const invalidNames[] = {
        "missing-separator", "UNKNOWN:value", "IP:300.1.1.1", ""
    };
    UA_String subject = UA_STRING("CN=localhost");
    /* A valid entry first also exercises cleanup of the partially built list. */
    UA_String names[] = {UA_STRING("URI:urn:test.generated"),
                         UA_STRING((char*)(uintptr_t)invalidNames[_i])};
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    UA_UInt16 keyLength = 2048;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(
        &params, UA_QUALIFIEDNAME(0, "key-size-bits"), &keyLength,
        &UA_TYPES[UA_TYPES_UINT16]), UA_STATUSCODE_GOOD);
#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
    UA_String keyType = UA_STRING("EC");
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(
        &params, UA_QUALIFIEDNAME(0, "key-type"), &keyType,
        &UA_TYPES[UA_TYPES_STRING]), UA_STATUSCODE_GOOD);
#endif
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_StatusCode status = UA_CreateCertificate(
        UA_Log_Stdout, &subject, 1, names, 2, UA_CERTIFICATEFORMAT_DER,
        &params, &privateKey, &certificate);
    UA_KeyValueMap_clear(&params);
    ck_assert_uint_eq(status, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_ptr_null(privateKey.data);
    ck_assert_uint_eq(privateKey.length, 0);
    ck_assert_ptr_null(certificate.data);
    ck_assert_uint_eq(certificate.length, 0);
}
END_TEST

static Suite* testSuite_create_certificate(void) {
    Suite *s = suite_create("Create Certificate");
    TCase *tc_cert = tcase_create("Certificate Create");
    tcase_add_checked_fixture(tc_cert, setup, teardown);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc_cert, certificate_generation);
    tcase_add_test(tc_cert, certificate_without_application_uri_is_rejected);
    tcase_add_test(tc_cert, certificate_generation_rejects_malformed_names);
    tcase_add_test(tc_cert, certificate_utils_outputs_are_transactional);
#endif /* UA_ENABLE_ENCRYPTION */
    suite_add_tcase(s,tc_cert);

    TCase *tc_options = tcase_create("Certificate generation options");
    tcase_add_loop_test(tc_options, certificate_generation_rejects_missing_arguments, 0, 8);
    tcase_add_loop_test(tc_options, certificate_generation_rejects_invalid_subjects, 0, 5);
#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
    tcase_add_test(tc_options, certificate_generation_email_san);
#endif
#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS) || \
    (defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L)
    tcase_add_loop_test(tc_options, certificate_generation_ecc_curves, 0, 16);
    tcase_add_test(tc_options, certificate_generation_unsupported_curve);
    tcase_add_loop_test(tc_options, certificate_generation_eddsa, 0, 4);
#endif
    suite_add_tcase(s, tc_options);
    TCase *tc_san = tcase_create("Invalid subject alternative names");
    tcase_add_loop_test(tc_san, certificate_generation_rejects_invalid_san, 0, 4);
    suite_add_tcase(s, tc_san);
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
