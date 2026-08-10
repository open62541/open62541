/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "securitypolicy_mbedtls_compat.h"

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)

#include <mbedtls/md.h>
#include <mbedtls/psa_util.h>
#include <mbedtls/version.h>
#if MBEDTLS_VERSION_NUMBER < 0x04000000
#include <mbedtls/ecp.h>
#include <mbedtls/rsa.h>
#endif
#include <psa/crypto.h>

/* A small, explicit compatibility boundary is preferable to exposing private
 * mbedTLS certificate fields throughout the validation implementation. */
#ifndef MBEDTLS_PRIVATE
#define MBEDTLS_PRIVATE(x) x
#endif

int
UA_mbedTLS_compat_parsePrivateKey(mbedtls_pk_context *target,
                                  const unsigned char *key, size_t keyLength,
                                  const unsigned char *password,
                                  size_t passwordLength) {
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    return mbedtls_pk_parse_key(target, key, keyLength, password, passwordLength);
#else
    return mbedtls_pk_parse_key(target, key, keyLength, password, passwordLength,
                                mbedtls_psa_get_random,
                                MBEDTLS_PSA_RANDOM_STATE);
#endif
}

UA_Boolean
UA_mbedTLS_compat_isEccKeyPair(const mbedtls_pk_context *key) {
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    return key && PSA_KEY_TYPE_IS_ECC_KEY_PAIR(mbedtls_pk_get_key_type(key));
#else
    return key && mbedtls_pk_can_do(key, MBEDTLS_PK_ECKEY);
#endif
}

UA_Boolean
UA_mbedTLS_compat_isRsaKeyPair(const mbedtls_pk_context *key) {
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    return key && mbedtls_pk_get_key_type(key) == PSA_KEY_TYPE_RSA_KEY_PAIR;
#else
    return key && mbedtls_pk_can_do(key, MBEDTLS_PK_RSA);
#endif
}

UA_Boolean
UA_mbedTLS_compat_verifyCertificateSignature(const mbedtls_x509_crt *certificate,
                                              mbedtls_x509_crt *issuer) {
    size_t hashLength;
    unsigned char hash[MBEDTLS_MD_MAX_SIZE];
    mbedtls_md_type_t md = certificate->MBEDTLS_PRIVATE(sig_md);
#if !defined(MBEDTLS_USE_PSA_CRYPTO)
    const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(md);
    if(!mdInfo)
        return false;
    hashLength = mbedtls_md_get_size(mdInfo);
    if(mbedtls_md(mdInfo, certificate->tbs.p, certificate->tbs.len, hash) != 0)
        return false;
#else
    if(psa_hash_compute(mbedtls_md_psa_alg_from_type(md),
                        certificate->tbs.p, certificate->tbs.len,
                        hash, sizeof(hash), &hashLength) != PSA_SUCCESS)
        return false;
#endif

    const mbedtls_x509_buf *signature = &certificate->MBEDTLS_PRIVATE(sig);
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    return mbedtls_pk_verify_ext(certificate->MBEDTLS_PRIVATE(sig_pk),
                                 &issuer->pk, md, hash, hashLength,
                                 signature->p, signature->len) == 0;
#else
    return mbedtls_pk_verify_ext(certificate->MBEDTLS_PRIVATE(sig_pk),
                                 certificate->MBEDTLS_PRIVATE(sig_opts),
                                 &issuer->pk, md, hash, hashLength,
                                 signature->p, signature->len) == 0;
#endif
}

int
UA_mbedTLS_compat_writeCertificateDer(mbedtls_x509write_cert *certificate,
                                      unsigned char *buffer,
                                      size_t bufferSize) {
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    return mbedtls_x509write_crt_der(certificate, buffer, bufferSize);
#else
    return mbedtls_x509write_crt_der(certificate, buffer, bufferSize,
                                     mbedtls_psa_get_random,
                                     MBEDTLS_PSA_RANDOM_STATE);
#endif
}

int
UA_mbedTLS_compat_writeCertificatePem(mbedtls_x509write_cert *certificate,
                                      unsigned char *buffer,
                                      size_t bufferSize) {
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    return mbedtls_x509write_crt_pem(certificate, buffer, bufferSize);
#else
    return mbedtls_x509write_crt_pem(certificate, buffer, bufferSize,
                                     mbedtls_psa_get_random,
                                     MBEDTLS_PSA_RANDOM_STATE);
#endif
}

UA_StatusCode
UA_mbedTLS_compat_generateCsrKey(const mbedtls_pk_context *templateKey,
                                 mbedtls_pk_context *generatedKey) {
    if(!templateKey || !generatedKey)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    psa_key_type_t keyType = mbedtls_pk_get_key_type(templateKey);
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, keyType);
    psa_set_key_bits(&attributes, mbedtls_pk_get_bitlen(templateKey));
    psa_set_key_usage_flags(&attributes,
                            PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_SIGN_HASH);
    if(keyType == PSA_KEY_TYPE_RSA_KEY_PAIR)
        psa_set_key_algorithm(&attributes,
            PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256));
    else if(PSA_KEY_TYPE_IS_ECC_KEY_PAIR(keyType))
        psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    else {
        psa_reset_key_attributes(&attributes);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    mbedtls_svc_key_id_t generated = MBEDTLS_SVC_KEY_ID_INIT;
    psa_status_t status = psa_generate_key(&attributes, &generated);
    psa_reset_key_attributes(&attributes);
    if(status != PSA_SUCCESS)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    int err = mbedtls_pk_copy_from_psa(generated, generatedKey);
    (void)psa_destroy_key(generated);
    return err == 0 ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
#else
    if(mbedtls_pk_can_do(templateKey, MBEDTLS_PK_ECKEY)) {
        int err = mbedtls_pk_setup(
            generatedKey, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
        if(err != 0)
            return UA_STATUSCODE_BADINTERNALERROR;
        const mbedtls_ecp_keypair *templatePair = mbedtls_pk_ec(*templateKey);
        mbedtls_ecp_keypair *generatedPair = mbedtls_pk_ec(*generatedKey);
        err = mbedtls_ecp_gen_key(templatePair->MBEDTLS_PRIVATE(grp).id,
                                  generatedPair, mbedtls_psa_get_random,
                                  MBEDTLS_PSA_RANDOM_STATE);
        return err == 0 ? UA_STATUSCODE_GOOD :
            UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }
    if(!mbedtls_pk_can_do(templateKey, MBEDTLS_PK_RSA))
        return UA_STATUSCODE_BADNOTSUPPORTED;
    int err = mbedtls_pk_setup(
        generatedKey, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if(err != 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    err = mbedtls_rsa_gen_key(mbedtls_pk_rsa(*generatedKey),
                              mbedtls_psa_get_random,
                              MBEDTLS_PSA_RANDOM_STATE,
                              (unsigned int)mbedtls_pk_get_bitlen(templateKey),
                              65537);
    return err == 0 ? UA_STATUSCODE_GOOD :
        UA_STATUSCODE_BADSECURITYCHECKSFAILED;
#endif
}

UA_StatusCode
UA_mbedTLS_compat_configureCsrSigningKey(mbedtls_pk_context *key) {
    if(!key)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
#if MBEDTLS_VERSION_NUMBER < 0x04000000
    if(mbedtls_pk_can_do(key, MBEDTLS_PK_RSA)) {
        mbedtls_rsa_set_padding(mbedtls_pk_rsa(*key),
                                MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_NONE);
    }
#endif
    return UA_STATUSCODE_GOOD;
}

int
UA_mbedTLS_compat_writeCsrDer(mbedtls_x509write_csr *request,
                              unsigned char *buffer, size_t bufferSize) {
#if MBEDTLS_VERSION_NUMBER >= 0x04000000
    return mbedtls_x509write_csr_der(request, buffer, bufferSize);
#else
    return mbedtls_x509write_csr_der(request, buffer, bufferSize,
                                     mbedtls_psa_get_random,
                                     MBEDTLS_PSA_RANDOM_STATE);
#endif
}

#endif
