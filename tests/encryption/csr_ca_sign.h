/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Bastian Engel)
 */

/* The GDS tests use CSR_CA_CERT_DER as the CA of the GDS. The functions issue
 * certificates with CSR_CA_KEY_DER. Defines CAN_SIGN_CSR if the crypto backend
 * can issue certificates. The client of the tests accepts all server
 * certificates, but checks the ApplicationUri in the SubjectAltName. So the
 * certificates keep the SubjectAltName. */

#ifndef CSR_CA_SIGN_H_
#define CSR_CA_SIGN_H_

#include <open62541/types.h>

#include <check.h>
#include "certificates_csr_ca.h"

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#define CAN_SIGN_CSR
#elif defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
#include <mbedtls/version.h>
#if MBEDTLS_VERSION_NUMBER >= 0x03050000
#include "crypto/mbedtls/securitypolicy_mbedtls_compat.h"
#define CAN_SIGN_CSR
#endif
#endif

#ifdef CAN_SIGN_CSR

static UA_UInt32 csrCaSerialNumber = 1;

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

/* Issue a certificate for the subject key with the subject name and the
 * extensions */
static UA_INLINE void
issueCertificate(EVP_PKEY *subjectKey, X509_NAME *subjectName,
                 STACK_OF(X509_EXTENSION) *exts, UA_ByteString *certificate) {
    const unsigned char *p = CSR_CA_CERT_DER_DATA;
    X509 *issuer = d2i_X509(NULL, &p, CSR_CA_CERT_DER_LENGTH);
    ck_assert_ptr_nonnull(issuer);
    p = CSR_CA_KEY_DER_DATA;
    EVP_PKEY *issuerKey = d2i_AutoPrivateKey(NULL, &p, CSR_CA_KEY_DER_LENGTH);
    ck_assert_ptr_nonnull(issuerKey);

    X509 *crt = X509_new();
    ck_assert_ptr_nonnull(crt);
    ck_assert_int_eq(X509_set_version(crt, 2), 1);
    ck_assert_int_eq(ASN1_INTEGER_set(X509_get_serialNumber(crt),
                                      (long)csrCaSerialNumber++), 1);
    ck_assert_int_eq(X509_set_issuer_name(crt, X509_get_subject_name(issuer)), 1);
    ck_assert_int_eq(X509_set_subject_name(crt, subjectName), 1);
    ck_assert_ptr_nonnull(X509_gmtime_adj(X509_getm_notBefore(crt), 0));
    ck_assert_ptr_nonnull(X509_gmtime_adj(X509_getm_notAfter(crt), 60 * 60 * 24));
    ck_assert_int_eq(X509_set_pubkey(crt, subjectKey), 1);
    for(int i = 0; i < sk_X509_EXTENSION_num(exts); i++)
        ck_assert_int_eq(X509_add_ext(crt, sk_X509_EXTENSION_value(exts, i), -1), 1);

    ck_assert_int_gt(X509_sign(crt, issuerKey, EVP_sha256()), 0);
    int len = i2d_X509(crt, NULL);
    ck_assert_int_gt(len, 0);
    UA_StatusCode res = UA_ByteString_allocBuffer(certificate, (size_t)len);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    unsigned char *out = certificate->data;
    ck_assert_int_eq(i2d_X509(crt, &out), len);

    X509_free(crt);
    EVP_PKEY_free(issuerKey);
    X509_free(issuer);
}

/* Issue a certificate for the public key, the subject and the extensions of a
 * CSR */
static UA_INLINE void
signCsr(const UA_ByteString *csr, UA_ByteString *certificate) {
    const unsigned char *p = csr->data;
    X509_REQ *req = d2i_X509_REQ(NULL, &p, (long)csr->length);
    ck_assert_ptr_nonnull(req);
    EVP_PKEY *subjectKey = X509_REQ_get_pubkey(req);
    ck_assert_ptr_nonnull(subjectKey);
    STACK_OF(X509_EXTENSION) *exts = X509_REQ_get_extensions(req);
    issueCertificate(subjectKey, X509_REQ_get_subject_name(req), exts, certificate);
    sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
    EVP_PKEY_free(subjectKey);
    X509_REQ_free(req);
}

/* Issue a new certificate for the public key, the subject and the
 * SubjectAltName of a certificate. No CSR is needed for the key of the
 * certificate. */
static UA_INLINE void
renewCertificate(const UA_ByteString *oldCertificate, UA_ByteString *certificate) {
    const unsigned char *p = oldCertificate->data;
    X509 *old = d2i_X509(NULL, &p, (long)oldCertificate->length);
    ck_assert_ptr_nonnull(old);
    EVP_PKEY *subjectKey = X509_get_pubkey(old);
    ck_assert_ptr_nonnull(subjectKey);
    STACK_OF(X509_EXTENSION) *exts = sk_X509_EXTENSION_new_null();
    ck_assert_ptr_nonnull(exts);
    int pos = X509_get_ext_by_NID(old, NID_subject_alt_name, -1);
    if(pos >= 0)
        ck_assert_int_gt(sk_X509_EXTENSION_push(exts, X509_get_ext(old, pos)), 0);
    issueCertificate(subjectKey, X509_get_subject_name(old), exts, certificate);
    sk_X509_EXTENSION_free(exts); /* The extension belongs to the certificate */
    EVP_PKEY_free(subjectKey);
    X509_free(old);
}

#else /* mbedTLS */

#define MAX_SAN_ENTRIES 8

/* Issue a certificate for the subject key with the subject name and the
 * SubjectAltName */
static UA_INLINE void
issueCertificate(mbedtls_pk_context *subjectKey, const mbedtls_x509_name *subject,
                 const mbedtls_x509_sequence *subjectAltNames,
                 UA_ByteString *certificate) {
    mbedtls_x509_crt issuer;
    mbedtls_pk_context issuerKey;
    mbedtls_x509write_cert crt;
    mbedtls_x509_crt_init(&issuer);
    mbedtls_pk_init(&issuerKey);
    mbedtls_x509write_crt_init(&crt);

    ck_assert_int_eq(mbedtls_x509_crt_parse_der(&issuer, CSR_CA_CERT_DER_DATA,
                                                CSR_CA_CERT_DER_LENGTH), 0);
    ck_assert_int_eq(UA_mbedTLS_compat_parsePrivateKey(&issuerKey, CSR_CA_KEY_DER_DATA,
                                                       CSR_CA_KEY_DER_LENGTH, NULL, 0), 0);

    mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&crt, subjectKey);
    mbedtls_x509write_crt_set_issuer_key(&crt, &issuerKey);
    char name[512];
    ck_assert_int_gt(mbedtls_x509_dn_gets(name, sizeof(name), subject), 0);
    ck_assert_int_eq(mbedtls_x509write_crt_set_subject_name(&crt, name), 0);
    ck_assert_int_gt(mbedtls_x509_dn_gets(name, sizeof(name), &issuer.subject), 0);
    ck_assert_int_eq(mbedtls_x509write_crt_set_issuer_name(&crt, name), 0);
    unsigned char serial[4] = {
        (unsigned char)(csrCaSerialNumber >> 24), (unsigned char)(csrCaSerialNumber >> 16),
        (unsigned char)(csrCaSerialNumber >> 8), (unsigned char)csrCaSerialNumber};
    csrCaSerialNumber++;
    ck_assert_int_eq(mbedtls_x509write_crt_set_serial_raw(&crt, serial,
                                                          sizeof(serial)), 0);
    ck_assert_int_eq(mbedtls_x509write_crt_set_validity(&crt, "20240101000000",
                                                        "20991231235959"), 0);

    mbedtls_x509_san_list san[MAX_SAN_ENTRIES];
    size_t sanSize = 0;
    for(const mbedtls_x509_sequence *cur = subjectAltNames;
        cur && cur->buf.p; cur = cur->next) {
        ck_assert_uint_lt(sanSize, MAX_SAN_ENTRIES);
        ck_assert_int_eq(mbedtls_x509_parse_subject_alt_name(&cur->buf,
                                                             &san[sanSize].node), 0);
        san[sanSize].next = NULL;
        if(sanSize > 0)
            san[sanSize - 1].next = &san[sanSize];
        sanSize++;
    }
    if(sanSize > 0)
        ck_assert_int_eq(mbedtls_x509write_crt_set_subject_alternative_name(&crt, san), 0);

    unsigned char buf[4096];
    int len = UA_mbedTLS_compat_writeCertificateDer(&crt, buf, sizeof(buf));
    ck_assert_int_gt(len, 0);
    UA_StatusCode res = UA_ByteString_allocBuffer(certificate, (size_t)len);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    memcpy(certificate->data, buf + sizeof(buf) - (size_t)len, (size_t)len);

    for(size_t i = 0; i < sanSize; i++)
        mbedtls_x509_free_subject_alt_name(&san[i].node);
    mbedtls_x509write_crt_free(&crt);
    mbedtls_pk_free(&issuerKey);
    mbedtls_x509_crt_free(&issuer);
}

/* Issue a certificate for the public key, the subject and the SubjectAltName
 * of a CSR */
static UA_INLINE void
signCsr(const UA_ByteString *csr, UA_ByteString *certificate) {
    mbedtls_x509_csr req;
    mbedtls_x509_csr_init(&req);
    ck_assert_int_eq(mbedtls_x509_csr_parse_der(&req, csr->data, csr->length), 0);
    issueCertificate(&req.pk, &req.subject, &req.subject_alt_names, certificate);
    mbedtls_x509_csr_free(&req);
}

/* Issue a new certificate for the public key, the subject and the
 * SubjectAltName of a certificate. No CSR is needed for the key of the
 * certificate. */
static UA_INLINE void
renewCertificate(const UA_ByteString *oldCertificate, UA_ByteString *certificate) {
    mbedtls_x509_crt old;
    mbedtls_x509_crt_init(&old);
    ck_assert_int_eq(mbedtls_x509_crt_parse_der(&old, oldCertificate->data,
                                                oldCertificate->length), 0);
    issueCertificate(&old.pk, &old.subject, &old.subject_alt_names, certificate);
    mbedtls_x509_crt_free(&old);
}

#endif /* mbedTLS */

#endif /* CAN_SIGN_CSR */

#endif /* CSR_CA_SIGN_H_ */
