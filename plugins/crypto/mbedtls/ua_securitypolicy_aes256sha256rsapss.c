/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include "securitypolicy_mbedtls_common.h"

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>

/* Notes:
 * mbedTLS' AES allows in-place encryption and decryption. So we don't have to
 * allocate temp buffers.
 * https://tls.mbed.org/discussions/generic/in-place-decryption-with-aes256-same-input-output-buffer
 */

#define UA_SHA1_LENGTH 20
#define UA_SHA256_LENGTH 32
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_RSAPADDING_LEN 66 /* UA_SHA256_LENGTH * 2 + 2 */
#define UA_AES256SHA256RSAPSS_SYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_KEY_LENGTH 32 /*16*/
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_MINASYMKEYLENGTH 256
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_MAXASYMKEYLENGTH 512

typedef struct {
    UA_ByteString localCertThumbprint;

    mbedtls_ctr_drbg_context drbgContext;
    mbedtls_entropy_context entropyContext;
    mbedtls_md_context_t sha256MdContext;
    mbedtls_pk_context localPrivateKey;
} Aes256Sha256RsaPss_PolicyContext;

typedef struct {
    Aes256Sha256RsaPss_PolicyContext *policyContext;

    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;

    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    mbedtls_x509_crt remoteCertificate;
} Aes256Sha256RsaPss_ChannelContext;

/********************/
/* AsymmetricModule */
/********************/

/* VERIFY AsymmetricSignatureAlgorithm_RSA-PKCS15-SHA2-256 */
static UA_StatusCode
asym_verify_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                   const UA_ByteString *message,
                                   const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned char hash[UA_SHA256_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    // TODO check return status
    mbedtls_sha256_ret(message->data, message->length, hash, 0);
#else
    mbedtls_sha256(message->data, message->length, hash, 0);
#endif

    /* Set the RSA settings */
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);

#if MBEDTLS_VERSION_NUMBER < 0x03000000
    Aes256Sha256RsaPss_PolicyContext *pc = cc->policyContext;
    int mbedErr = mbedtls_rsa_pkcs1_verify(rsaContext, mbedtls_ctr_drbg_random, &pc->drbgContext,
                                                MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256, UA_SHA256_LENGTH,
                                                hash, signature->data);
#else
    int mbedErr = mbedtls_rsa_pkcs1_verify(rsaContext, MBEDTLS_MD_SHA256,
                                         UA_SHA256_LENGTH, hash, signature->data);
#endif

    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

/* AsymmetricSignatureAlgorithm_RSA-PSS-SHA2-256 */
static UA_StatusCode
asym_sign_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                 const UA_ByteString *message,
                                 UA_ByteString *signature) {
    if(message == NULL || signature == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned char hash[UA_SHA256_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    // TODO check return status
    mbedtls_sha256_ret(message->data, message->length, hash, 0);
#else
    mbedtls_sha256(message->data, message->length, hash, 0);
#endif

    Aes256Sha256RsaPss_PolicyContext *pc = cc->policyContext;
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(pc->localPrivateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);

#if MBEDTLS_VERSION_NUMBER < 0x03000000
    int mbedErr =  mbedtls_rsa_pkcs1_sign(rsaContext, mbedtls_ctr_drbg_random, &pc->drbgContext,
                                          MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_SHA256, UA_SHA256_LENGTH,
                                          hash, signature->data);
#else
    int mbedErr =  mbedtls_rsa_pkcs1_sign(rsaContext, mbedtls_ctr_drbg_random, &pc->drbgContext,
                                          MBEDTLS_MD_SHA256, UA_SHA256_LENGTH,
                                          hash, signature->data);
#endif


    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getLocalSignatureSize_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->policyContext->localPrivateKey)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->policyContext->localPrivateKey));
#endif
}

static size_t
asym_getRemoteSignatureSize_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->remoteCertificate.pk)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
}

static size_t
asym_getRemoteBlockSize_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->remoteCertificate.pk)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
}

static size_t
asym_getRemotePlainTextBlockSize_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_context *const rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    return rsaContext->len - UA_SECURITYPOLICY_AES256SHA256RSAPSS_RSAPADDING_LEN;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk)) -
        UA_SECURITYPOLICY_AES256SHA256RSAPSS_RSAPADDING_LEN;
#endif
}


/* AsymmetricEncryptionAlgorithm_RSA-OAEP-SHA2 */
static UA_StatusCode
asym_encrypt_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                    UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const size_t plainTextBlockSize = asym_getRemotePlainTextBlockSize_sp_aes256sha256rsapss(cc);

    mbedtls_rsa_context *remoteRsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    mbedtls_rsa_set_padding(remoteRsaContext, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);

    return mbedtls_encrypt_rsaOaep(remoteRsaContext, &cc->policyContext->drbgContext,
                                   data, plainTextBlockSize);
}

/* AsymmetricEncryptionAlgorithm_RSA-OAEP-SHA2 */
static UA_StatusCode
asym_decrypt_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                    UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return mbedtls_decrypt_rsaOaep(&cc->policyContext->localPrivateKey,
                                   &cc->policyContext->drbgContext, data, MBEDTLS_MD_SHA256);
}

static size_t
asym_getLocalEncryptionKeyLength_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc) {
    return mbedtls_pk_get_len(&cc->policyContext->localPrivateKey) * 8;
}

static size_t
asym_getRemoteEncryptionKeyLength_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc) {
    return mbedtls_pk_get_len(&cc->remoteCertificate.pk) * 8;
}

static UA_StatusCode
asym_makeThumbprint_sp_aes256sha256rsapss(const UA_SecurityPolicy *securityPolicy,
                                           const UA_ByteString *certificate,
                                           UA_ByteString *thumbprint) {
    if(securityPolicy == NULL || certificate == NULL || thumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return mbedtls_thumbprint_sha1(certificate, thumbprint);
}

static UA_StatusCode
asymmetricModule_compareCertificateThumbprint_sp_aes256sha256rsapss(const UA_SecurityPolicy *securityPolicy,
                                                                     const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    return UA_STATUSCODE_GOOD;
}

/*******************/
/* SymmetricModule */
/*******************/

static UA_StatusCode
sym_verify_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                  const UA_ByteString *message,
                                  const UA_ByteString *signature) {
    if(cc == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute MAC */
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    Aes256Sha256RsaPss_PolicyContext *pc = cc->policyContext;
    unsigned char mac[UA_SHA256_LENGTH];
    if(mbedtls_hmac(&pc->sha256MdContext, &cc->remoteSymSigningKey, message, mac) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Compare with Signature */
    if(!UA_constantTimeEqual(signature->data, mac, UA_SHA256_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc,
                                const UA_ByteString *message,
                                UA_ByteString *signature) {
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(mbedtls_hmac(&cc->policyContext->sha256MdContext, &cc->localSymSigningKey,
                    message, signature->data) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    return UA_STATUSCODE_GOOD;
}

static size_t
sym_getSignatureSize_sp_aes256sha256rsapss(const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static size_t
sym_getSigningKeyLength_sp_aes256sha256rsapss(const void *channelContext) {
    return UA_AES256SHA256RSAPSS_SYM_SIGNING_KEY_LENGTH;
}

static size_t
sym_getEncryptionKeyLength_sp_aes256sha256rsapss(const void *channelContext) {
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_KEY_LENGTH;
}

static size_t
sym_getEncryptionBlockSize_sp_aes256sha256rsapss(const void *channelContext) {
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
sym_getPlainTextBlockSize_sp_aes256sha256rsapss(const void *channelContext) {
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_PLAIN_TEXT_BLOCK_SIZE;
}

static UA_StatusCode
sym_encrypt_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc,
                                   UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(cc->localSymIv.length != UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t plainTextBlockSize = UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_PLAIN_TEXT_BLOCK_SIZE;

    if(data->length % plainTextBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Keylength in bits */
    unsigned int keylength = (unsigned int)(cc->localSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_enc(&aesContext, cc->localSymEncryptingKey.data, keylength);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString ivCopy;
    UA_StatusCode retval = UA_ByteString_copy(&cc->localSymIv, &ivCopy);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    mbedErr = mbedtls_aes_crypt_cbc(&aesContext, MBEDTLS_AES_ENCRYPT, data->length,
                                    ivCopy.data, data->data, data->data);
    if(mbedErr)
        retval = UA_STATUSCODE_BADINTERNALERROR;
    UA_ByteString_clear(&ivCopy);
    return retval;
}

static UA_StatusCode
sym_decrypt_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc,
                                   UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t encryptionBlockSize = UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE;

    if(cc->remoteSymIv.length != encryptionBlockSize)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(data->length % encryptionBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned int keylength = (unsigned int)(cc->remoteSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_dec(&aesContext, cc->remoteSymEncryptingKey.data, keylength);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString ivCopy;
    UA_StatusCode retval = UA_ByteString_copy(&cc->remoteSymIv, &ivCopy);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    mbedErr = mbedtls_aes_crypt_cbc(&aesContext, MBEDTLS_AES_DECRYPT, data->length,
                                    ivCopy.data, data->data, data->data);
    if(mbedErr)
        retval = UA_STATUSCODE_BADINTERNALERROR;
    UA_ByteString_clear(&ivCopy);
    return retval;
}

static UA_StatusCode
sym_generateKey_sp_aes256sha256rsapss(void *policyContext, const UA_ByteString *secret,
                                       const UA_ByteString *seed, UA_ByteString *out) {
    if(secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)policyContext;
    return mbedtls_generateKey(&pc->sha256MdContext, secret, seed, out);
}

static UA_StatusCode
sym_generateNonce_sp_aes256sha256rsapss(void *policyContext, UA_ByteString *out) {
    if(out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_PolicyContext *pc =
        (Aes256Sha256RsaPss_PolicyContext *)policyContext;
    int mbedErr = mbedtls_ctr_drbg_random(&pc->drbgContext, out->data, out->length);
    if(mbedErr)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

/***********************************/
/* CertificateSigningAlgorithms    */
/***********************************/

static UA_StatusCode
asym_cert_verify_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                   const UA_ByteString *message,
                                   const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned char hash[UA_SHA256_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    // TODO check return status
    mbedtls_sha256_ret(message->data, message->length, hash, 0);
#else
    mbedtls_sha256(message->data, message->length, hash, 0);
#endif

    /* Set the RSA settings */
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_SHA256);

    /* For RSA keys, the default padding type is PKCS#1 v1.5 in mbedtls_pk_verify() */
    /* Alternatively, use more specific function mbedtls_rsa_rsassa_pkcs1_v15_verify(), i.e. */
    /* int mbedErr = mbedtls_rsa_rsassa_pkcs1_v15_verify(rsaContext, NULL, NULL,
                                                         MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256,
                                                         UA_SHA256_LENGTH, hash,
                                                         signature->data); */
    int mbedErr = mbedtls_pk_verify(&cc->remoteCertificate.pk,
                                    MBEDTLS_MD_SHA256, hash, UA_SHA256_LENGTH,
                                    signature->data, signature->length);

    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

/* AsymmetricSignatureAlgorithm_RSA-PKCS15-SHA2-256 */
static UA_StatusCode
asym_cert_sign_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                 const UA_ByteString *message,
                                 UA_ByteString *signature) {
    if(message == NULL || signature == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned char hash[UA_SHA256_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    // TODO check return status
    mbedtls_sha256_ret(message->data, message->length, hash, 0);
#else
    mbedtls_sha256(message->data, message->length, hash, 0);
#endif

    Aes256Sha256RsaPss_PolicyContext *pc = cc->policyContext;
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(pc->localPrivateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_SHA256);

    size_t sigLen = 0;

    /* For RSA keys, the default padding type is PKCS#1 v1.5 in mbedtls_pk_sign */
    /* Alternatively use more specific function mbedtls_rsa_rsassa_pkcs1_v15_sign() */
    int mbedErr = mbedtls_pk_sign(&pc->localPrivateKey,
                                  MBEDTLS_MD_SHA256, hash,
                                  UA_SHA256_LENGTH, signature->data,
#if MBEDTLS_VERSION_NUMBER >= 0x03000000
        signature->length,
#endif
                                  &sigLen, mbedtls_ctr_drbg_random,
                                  &pc->drbgContext);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_cert_getLocalSignatureSize_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->policyContext->localPrivateKey)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->policyContext->localPrivateKey));
#endif
}

static size_t
asym_cert_getRemoteSignatureSize_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->remoteCertificate.pk)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
}

/*****************/
/* ChannelModule */
/*****************/

/* Assumes that the certificate has been verified externally */
static UA_StatusCode
parseRemoteCertificate_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                              const UA_ByteString *remoteCertificate) {
    if(remoteCertificate == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Parse the certificate */
    int mbedErr = mbedtls_x509_crt_parse(&cc->remoteCertificate, remoteCertificate->data,
                                         remoteCertificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Check the key length */
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    size_t keylen = rsaContext->len;
#else
    size_t keylen = mbedtls_rsa_get_len(rsaContext);
#endif
    if(keylen < UA_SECURITYPOLICY_AES256SHA256RSAPSS_MINASYMKEYLENGTH ||
       keylen > UA_SECURITYPOLICY_AES256SHA256RSAPSS_MAXASYMKEYLENGTH)
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;
    return UA_STATUSCODE_GOOD;
}

static void
channelContext_deleteContext_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc) {
    UA_ByteString_clear(&cc->localSymSigningKey);
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    UA_ByteString_clear(&cc->localSymIv);

    UA_ByteString_clear(&cc->remoteSymSigningKey);
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    UA_ByteString_clear(&cc->remoteSymIv);

    mbedtls_x509_crt_free(&cc->remoteCertificate);

    UA_free(cc);
}

static UA_StatusCode
channelContext_newContext_sp_aes256sha256rsapss(const UA_SecurityPolicy *securityPolicy,
                                                 const UA_ByteString *remoteCertificate,
                                                 void **pp_contextData) {
    if(securityPolicy == NULL || remoteCertificate == NULL || pp_contextData == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the channel context */
    *pp_contextData = UA_malloc(sizeof(Aes256Sha256RsaPss_ChannelContext));
    if(*pp_contextData == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    Aes256Sha256RsaPss_ChannelContext *cc = (Aes256Sha256RsaPss_ChannelContext *)*pp_contextData;

    /* Initialize the channel context */
    cc->policyContext = (Aes256Sha256RsaPss_PolicyContext *)securityPolicy->policyContext;

    UA_ByteString_init(&cc->localSymSigningKey);
    UA_ByteString_init(&cc->localSymEncryptingKey);
    UA_ByteString_init(&cc->localSymIv);

    UA_ByteString_init(&cc->remoteSymSigningKey);
    UA_ByteString_init(&cc->remoteSymEncryptingKey);
    UA_ByteString_init(&cc->remoteSymIv);

    mbedtls_x509_crt_init(&cc->remoteCertificate);

    // TODO: this can be optimized so that we dont allocate memory before parsing the certificate
    UA_StatusCode retval = parseRemoteCertificate_sp_aes256sha256rsapss(cc, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        channelContext_deleteContext_sp_aes256sha256rsapss(cc);
        *pp_contextData = NULL;
    }
    return retval;
}

static UA_StatusCode
channelContext_setLocalSymEncryptingKey_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                                               const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
channelContext_setLocalSymSigningKey_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                                            const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}


static UA_StatusCode
channelContext_setLocalSymIv_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                                    const UA_ByteString *iv) {
    if(iv == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static UA_StatusCode
channelContext_setRemoteSymEncryptingKey_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                                                const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
channelContext_setRemoteSymSigningKey_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                                             const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
channelContext_setRemoteSymIv_sp_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
                                                     const UA_ByteString *iv) {
    if(iv == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

static UA_StatusCode
channelContext_compareCertificate_sp_aes256sha256rsapss(const Aes256Sha256RsaPss_ChannelContext *cc,
                                                         const UA_ByteString *certificate) {
    if(cc == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int mbedErr = mbedtls_x509_crt_parse(&cert, certificate->data, certificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(cert.raw.len != cc->remoteCertificate.raw.len ||
       memcmp(cert.raw.p, cc->remoteCertificate.raw.p, cert.raw.len) != 0)
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    mbedtls_x509_crt_free(&cert);
    return retval;
}

static void
clear_sp_aes256sha256rsapss(UA_SecurityPolicy *securityPolicy) {
    if(securityPolicy == NULL)
        return;

    UA_ByteString_clear(&securityPolicy->localCertificate);

    if(securityPolicy->policyContext == NULL)
        return;

    /* delete all allocated members in the context */
    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)
        securityPolicy->policyContext;

    mbedtls_ctr_drbg_free(&pc->drbgContext);
    mbedtls_entropy_free(&pc->entropyContext);
    mbedtls_pk_free(&pc->localPrivateKey);
    mbedtls_md_free(&pc->sha256MdContext);
    UA_ByteString_clear(&pc->localCertThumbprint);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for sp_aes256sha256rsapss");

    UA_free(pc);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_aes256sha256rsapss(UA_SecurityPolicy *securityPolicy,
                                                      const UA_ByteString newCertificate,
                                                      const UA_ByteString newPrivateKey) {
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Aes256Sha256RsaPss_PolicyContext *pc =
        (Aes256Sha256RsaPss_PolicyContext *) securityPolicy->policyContext;

    UA_ByteString_clear(&securityPolicy->localCertificate);

    UA_StatusCode retval = UA_ByteString_allocBuffer(&securityPolicy->localCertificate,
                                                     newCertificate.length + 1);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    memcpy(securityPolicy->localCertificate.data, newCertificate.data, newCertificate.length);
    securityPolicy->localCertificate.data[newCertificate.length] = '\0';
    securityPolicy->localCertificate.length--;

    /* Set the new private key */
    mbedtls_pk_free(&pc->localPrivateKey);
    mbedtls_pk_init(&pc->localPrivateKey);
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    int mbedErr = mbedtls_pk_parse_key(&pc->localPrivateKey, newPrivateKey.data,
                                       newPrivateKey.length, NULL, 0);
#else
    int mbedErr = mbedtls_pk_parse_key(&pc->localPrivateKey, newPrivateKey.data,
                                       newPrivateKey.length, NULL, 0, mbedtls_entropy_func, &pc->drbgContext);
#endif
    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    retval = asym_makeThumbprint_sp_aes256sha256rsapss(securityPolicy,
                                                        &securityPolicy->localCertificate,
                                                        &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return retval;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    if(securityPolicy->policyContext != NULL)
        clear_sp_aes256sha256rsapss(securityPolicy);
    return retval;
}

static UA_StatusCode
policyContext_newContext_sp_aes256sha256rsapss(UA_SecurityPolicy *securityPolicy,
                                                const UA_ByteString localPrivateKey) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if (localPrivateKey.length == 0) {
        UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Can not initialize security policy. Private key is empty.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)
        UA_malloc(sizeof(Aes256Sha256RsaPss_PolicyContext));
    securityPolicy->policyContext = (void *)pc;
    if(!pc) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Initialize the PolicyContext */
    memset(pc, 0, sizeof(Aes256Sha256RsaPss_PolicyContext));
    mbedtls_ctr_drbg_init(&pc->drbgContext);
    mbedtls_entropy_init(&pc->entropyContext);
    mbedtls_pk_init(&pc->localPrivateKey);
    mbedtls_md_init(&pc->sha256MdContext);

    /* Initialized the message digest */
    const mbedtls_md_info_t *const mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    int mbedErr = mbedtls_md_setup(&pc->sha256MdContext, mdInfo, MBEDTLS_MD_SHA256);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    mbedErr = mbedtls_entropy_self_test(0);

    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Seed the RNG */
    char *personalization = "open62541-drbg";
    mbedErr = mbedtls_ctr_drbg_seed(&pc->drbgContext, mbedtls_entropy_func,
                                    &pc->entropyContext,
                                    (const unsigned char *)personalization, 14);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Set the private key */
    mbedErr = UA_mbedTLS_LoadPrivateKey(&localPrivateKey, &pc->localPrivateKey, &pc->entropyContext);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Set the local certificate thumbprint */
    retval = UA_ByteString_allocBuffer(&pc->localCertThumbprint, UA_SHA1_LENGTH);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;
    retval = asym_makeThumbprint_sp_aes256sha256rsapss(securityPolicy,
                                                        &securityPolicy->localCertificate,
                                                        &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext: %s", UA_StatusCode_name(retval));
    if(securityPolicy->policyContext != NULL)
        clear_sp_aes256sha256rsapss(securityPolicy);
    return retval;
}

UA_StatusCode
UA_SecurityPolicy_Aes256Sha256RsaPss(UA_SecurityPolicy *policy, const UA_ByteString localCertificate,
                                      const UA_ByteString localPrivateKey, const UA_Logger *logger) {
    memset(policy, 0, sizeof(UA_SecurityPolicy));
    policy->logger = logger;

    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss");
    policy->securityLevel = 20;

    UA_SecurityPolicyAsymmetricModule *const asymmetricModule = &policy->asymmetricModule;
    UA_SecurityPolicySymmetricModule *const symmetricModule = &policy->symmetricModule;
    UA_SecurityPolicyChannelModule *const channelModule = &policy->channelModule;

    UA_StatusCode retval = UA_mbedTLS_LoadLocalCertificate(&localCertificate, &policy->localCertificate);

    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    /* AsymmetricModule */
    UA_SecurityPolicySignatureAlgorithm *asym_signatureAlgorithm =
        &asymmetricModule->cryptoModule.signatureAlgorithm;
    asym_signatureAlgorithm->uri =
        UA_STRING("http://opcfoundation.org/UA/security/rsa-pss-sha2-256\0");
    asym_signatureAlgorithm->verify =
        (UA_StatusCode (*)(void *, const UA_ByteString *, const UA_ByteString *))asym_verify_sp_aes256sha256rsapss;
    asym_signatureAlgorithm->sign =
        (UA_StatusCode (*)(void *, const UA_ByteString *, UA_ByteString *))asym_sign_sp_aes256sha256rsapss;
    asym_signatureAlgorithm->getLocalSignatureSize =
        (size_t (*)(const void *))asym_getLocalSignatureSize_sp_aes256sha256rsapss;
    asym_signatureAlgorithm->getRemoteSignatureSize =
        (size_t (*)(const void *))asym_getRemoteSignatureSize_sp_aes256sha256rsapss;
    asym_signatureAlgorithm->getLocalKeyLength = NULL; // TODO: Write function
    asym_signatureAlgorithm->getRemoteKeyLength = NULL; // TODO: Write function

    UA_SecurityPolicyEncryptionAlgorithm *asym_encryptionAlgorithm =
        &asymmetricModule->cryptoModule.encryptionAlgorithm;
    asym_encryptionAlgorithm->uri = UA_STRING("http://opcfoundation.org/UA/security/rsa-oaep-sha2-256\0");
    asym_encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))asym_encrypt_sp_aes256sha256rsapss;
    asym_encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *)) asym_decrypt_sp_aes256sha256rsapss;
    asym_encryptionAlgorithm->getLocalKeyLength =
        (size_t (*)(const void *))asym_getLocalEncryptionKeyLength_sp_aes256sha256rsapss;
    asym_encryptionAlgorithm->getRemoteKeyLength =
        (size_t (*)(const void *))asym_getRemoteEncryptionKeyLength_sp_aes256sha256rsapss;
    asym_encryptionAlgorithm->getRemoteBlockSize = (size_t (*)(const void *))asym_getRemoteBlockSize_sp_aes256sha256rsapss;
    asym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t (*)(const void *))asym_getRemotePlainTextBlockSize_sp_aes256sha256rsapss;

    asymmetricModule->makeCertificateThumbprint = asym_makeThumbprint_sp_aes256sha256rsapss;
    asymmetricModule->compareCertificateThumbprint =
        asymmetricModule_compareCertificateThumbprint_sp_aes256sha256rsapss;

    /* SymmetricModule */
    symmetricModule->generateKey = sym_generateKey_sp_aes256sha256rsapss;
    symmetricModule->generateNonce = sym_generateNonce_sp_aes256sha256rsapss;

    UA_SecurityPolicySignatureAlgorithm *sym_signatureAlgorithm =
        &symmetricModule->cryptoModule.signatureAlgorithm;
    sym_signatureAlgorithm->uri =
        UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256\0");
    sym_signatureAlgorithm->verify =
        (UA_StatusCode (*)(void *, const UA_ByteString *, const UA_ByteString *))sym_verify_sp_aes256sha256rsapss;
    sym_signatureAlgorithm->sign =
        (UA_StatusCode (*)(void *, const UA_ByteString *, UA_ByteString *))sym_sign_sp_aes256sha256rsapss;
    sym_signatureAlgorithm->getLocalSignatureSize = sym_getSignatureSize_sp_aes256sha256rsapss;
    sym_signatureAlgorithm->getRemoteSignatureSize = sym_getSignatureSize_sp_aes256sha256rsapss;
    sym_signatureAlgorithm->getLocalKeyLength =
        (size_t (*)(const void *))sym_getSigningKeyLength_sp_aes256sha256rsapss;
    sym_signatureAlgorithm->getRemoteKeyLength =
        (size_t (*)(const void *))sym_getSigningKeyLength_sp_aes256sha256rsapss;

    UA_SecurityPolicyEncryptionAlgorithm *sym_encryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    sym_encryptionAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc\0");
    sym_encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))sym_encrypt_sp_aes256sha256rsapss;
    sym_encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))sym_decrypt_sp_aes256sha256rsapss;
    sym_encryptionAlgorithm->getLocalKeyLength = sym_getEncryptionKeyLength_sp_aes256sha256rsapss;
    sym_encryptionAlgorithm->getRemoteKeyLength = sym_getEncryptionKeyLength_sp_aes256sha256rsapss;
    sym_encryptionAlgorithm->getRemoteBlockSize =
        (size_t (*)(const void *))sym_getEncryptionBlockSize_sp_aes256sha256rsapss;
    sym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t (*)(const void *))sym_getPlainTextBlockSize_sp_aes256sha256rsapss;
    symmetricModule->secureChannelNonceLength = 32;

    /* Certificate Signing Algorithm */
    policy->certificateSigningAlgorithm.uri =
        UA_STRING("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256\0");
    policy->certificateSigningAlgorithm.verify =
        (UA_StatusCode (*)(void *, const UA_ByteString *, const UA_ByteString *))asym_cert_verify_sp_aes256sha256rsapss;
    policy->certificateSigningAlgorithm.sign =
        (UA_StatusCode (*)(void *, const UA_ByteString *, UA_ByteString *))asym_cert_sign_sp_aes256sha256rsapss;
    policy->certificateSigningAlgorithm.getLocalSignatureSize =
        (size_t (*)(const void *))asym_cert_getLocalSignatureSize_sp_aes256sha256rsapss;
    policy->certificateSigningAlgorithm.getRemoteSignatureSize =
        (size_t (*)(const void *))asym_cert_getRemoteSignatureSize_sp_aes256sha256rsapss;
    policy->certificateSigningAlgorithm.getLocalKeyLength = NULL; // TODO: Write function
    policy->certificateSigningAlgorithm.getRemoteKeyLength = NULL; // TODO: Write function

    /* ChannelModule */
    channelModule->newContext = channelContext_newContext_sp_aes256sha256rsapss;
    channelModule->deleteContext = (void (*)(void *))
        channelContext_deleteContext_sp_aes256sha256rsapss;

    channelModule->setLocalSymEncryptingKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setLocalSymEncryptingKey_sp_aes256sha256rsapss;
    channelModule->setLocalSymSigningKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setLocalSymSigningKey_sp_aes256sha256rsapss;
    channelModule->setLocalSymIv = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setLocalSymIv_sp_aes256sha256rsapss;

    channelModule->setRemoteSymEncryptingKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setRemoteSymEncryptingKey_sp_aes256sha256rsapss;
    channelModule->setRemoteSymSigningKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setRemoteSymSigningKey_sp_aes256sha256rsapss;
    channelModule->setRemoteSymIv = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setRemoteSymIv_sp_aes256sha256rsapss;

    channelModule->compareCertificate = (UA_StatusCode (*)(const void *, const UA_ByteString *))
        channelContext_compareCertificate_sp_aes256sha256rsapss;

    policy->updateCertificateAndPrivateKey = updateCertificateAndPrivateKey_sp_aes256sha256rsapss;
    policy->clear = clear_sp_aes256sha256rsapss;

    UA_StatusCode res = policyContext_newContext_sp_aes256sha256rsapss(policy, localPrivateKey);
    if(res != UA_STATUSCODE_GOOD)
        clear_sp_aes256sha256rsapss(policy);

    return res;
}

#endif
