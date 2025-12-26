/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include "securitypolicy_common.h"

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
    mbedtls_pk_context csrLocalPrivateKey;
} Aes256Sha256RsaPss_PolicyContext;

typedef struct {
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;

    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    mbedtls_x509_crt remoteCertificate;
} Aes256Sha256RsaPss_ChannelContext;

/* VERIFY AsymmetricSignatureAlgorithm_RSA-PKCS15-SHA2-256 */
static UA_StatusCode
asym_verify_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                               void *channelContext,
                               const UA_ByteString *message,
                               const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;

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
    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)
        policy->policyContext;
    int mbedErr =
        mbedtls_rsa_pkcs1_verify(rsaContext, mbedtls_ctr_drbg_random, &pc->drbgContext,
                                 MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256,
                                 UA_SHA256_LENGTH, hash, signature->data);
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
asym_sign_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                             void *channelContext, const UA_ByteString *message,
                             UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned char hash[UA_SHA256_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    // TODO check return status
    mbedtls_sha256_ret(message->data, message->length, hash, 0);
#else
    mbedtls_sha256(message->data, message->length, hash, 0);
#endif

    Aes256Sha256RsaPss_PolicyContext *pc =
        (Aes256Sha256RsaPss_PolicyContext *)policy->policyContext;
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(pc->localPrivateKey);
    mbedtls_rsa_set_padding(rsaContext, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);

#if MBEDTLS_VERSION_NUMBER < 0x03000000
    int mbedErr =
        mbedtls_rsa_pkcs1_sign(rsaContext, mbedtls_ctr_drbg_random, &pc->drbgContext,
                               MBEDTLS_RSA_PRIVATE, MBEDTLS_MD_SHA256,
                               UA_SHA256_LENGTH, hash, signature->data);
#else
    int mbedErr = mbedtls_rsa_pkcs1_sign(rsaContext, mbedtls_ctr_drbg_random,
                                         &pc->drbgContext, MBEDTLS_MD_SHA256,
                                         UA_SHA256_LENGTH, hash, signature->data);
#endif
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

static size_t
asym_getLocalSignatureSize_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const Aes256Sha256RsaPss_PolicyContext *pc =
        (const Aes256Sha256RsaPss_PolicyContext *)policy->policyContext;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(pc->localPrivateKey)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(pc->localPrivateKey));
#endif
}

static size_t
asym_getRemoteSignatureSize_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const Aes256Sha256RsaPss_ChannelContext *cc =
        (const Aes256Sha256RsaPss_ChannelContext*)channelContext;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->remoteCertificate.pk)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
}

static size_t
asym_getRemoteBlockSize_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const Aes256Sha256RsaPss_ChannelContext *cc =
        (const Aes256Sha256RsaPss_ChannelContext*)channelContext;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->remoteCertificate.pk)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
}

static size_t
asym_getRemotePlainTextBlockSize_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                                    const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const Aes256Sha256RsaPss_ChannelContext *cc =
        (const Aes256Sha256RsaPss_ChannelContext*)channelContext;
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
asym_encrypt_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)
        policy->policyContext;
    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
    const size_t plainTextBlockSize =
        asym_getRemotePlainTextBlockSize_aes256sha256rsapss(policy, cc);
    mbedtls_rsa_context *remoteRsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    mbedtls_rsa_set_padding(remoteRsaContext, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    return mbedtls_encrypt_rsaOaep(remoteRsaContext, &pc->drbgContext,
                                   data, plainTextBlockSize);
}

/* AsymmetricEncryptionAlgorithm_RSA-OAEP-SHA2 */
static UA_StatusCode
asym_decrypt_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_PolicyContext *pc =
        (Aes256Sha256RsaPss_PolicyContext *)policy->policyContext;
    return mbedtls_decrypt_rsaOaep(&pc->localPrivateKey, &pc->drbgContext,
                                   data, MBEDTLS_MD_SHA256);
}

static size_t
asym_getLocalEncryptionKeyLength_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                                    const void *channelContext) {
    const Aes256Sha256RsaPss_PolicyContext *pc =
        (const Aes256Sha256RsaPss_PolicyContext *)policy->policyContext;
    return mbedtls_pk_get_len(&pc->localPrivateKey) * 8;
}

static size_t
asym_getRemoteEncryptionKeyLength_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                                     const void *channelContext) {
    const Aes256Sha256RsaPss_ChannelContext *cc =
        (const Aes256Sha256RsaPss_ChannelContext*)channelContext;
    return mbedtls_pk_get_len(&cc->remoteCertificate.pk) * 8;
}

static UA_StatusCode
makeThumbprint_aes256sha256rsapss(const UA_SecurityPolicy *securityPolicy,
                                  const UA_ByteString *certificate,
                                  UA_ByteString *thumbprint) {
    if(securityPolicy == NULL || certificate == NULL || thumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return mbedtls_thumbprint_sha1(certificate, thumbprint);
}

static UA_StatusCode
compareCertificateThumbprint_aes256sha256rsapss(const UA_SecurityPolicy *securityPolicy,
                                                const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_PolicyContext *pc =
        (Aes256Sha256RsaPss_PolicyContext *)securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_verify_aes256sha256rsapss(const UA_SecurityPolicy *policy, void *channelContext,
                              const UA_ByteString *message,
                              const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)
        policy->policyContext;

    /* Compute MAC */
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    unsigned char mac[UA_SHA256_LENGTH];
    if(mbedtls_hmac(&pc->sha256MdContext, &cc->remoteSymSigningKey, message, mac) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Compare with Signature */
    if(!UA_constantTimeEqual(signature->data, mac, UA_SHA256_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                            void *channelContext, const UA_ByteString *message,
                            UA_ByteString *signature) {
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)
        policy->policyContext;
    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
    if(mbedtls_hmac(&pc->sha256MdContext, &cc->localSymSigningKey,
                    message, signature->data) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static size_t
sym_getSignatureSize_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static size_t
sym_getSigningKeyLength_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    return UA_AES256SHA256RSAPSS_SYM_SIGNING_KEY_LENGTH;
}

static size_t
sym_getEncryptionKeyLength_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_KEY_LENGTH;
}

static size_t
sym_getEncryptionBlockSize_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
sym_getPlainTextBlockSize_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                             const void *channelContext) {
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_PLAIN_TEXT_BLOCK_SIZE;
}

static UA_StatusCode
sym_encrypt_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                               void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;

    if(cc->localSymIv.length != UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t plainTextBlockSize =
        UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_PLAIN_TEXT_BLOCK_SIZE;
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
sym_decrypt_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                               void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t encryptionBlockSize =
        UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE;

    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;

    if(cc->remoteSymIv.length != encryptionBlockSize)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(data->length % encryptionBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned int keylength = (unsigned int)(cc->remoteSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_dec(&aesContext,
                                         cc->remoteSymEncryptingKey.data, keylength);
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
sym_generateKey_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                   void *channelContext, const UA_ByteString *secret,
                                   const UA_ByteString *seed, UA_ByteString *out) {
    if(secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)
        policy->policyContext;
    return mbedtls_generateKey(&pc->sha256MdContext, secret, seed, out);
}

static UA_StatusCode
sym_generateNonce_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                     void *channelContext, UA_ByteString *out) {
    if(out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_PolicyContext *pc =
        (Aes256Sha256RsaPss_PolicyContext *)policy->policyContext;
    int mbedErr = mbedtls_ctr_drbg_random(&pc->drbgContext, out->data, out->length);
    if(mbedErr)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
asym_cert_verify_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                    void *channelContext,
                                    const UA_ByteString *message,
                                    const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned char hash[UA_SHA256_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    // TODO check return status
    mbedtls_sha256_ret(message->data, message->length, hash, 0);
#else
    mbedtls_sha256(message->data, message->length, hash, 0);
#endif

    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;

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
asym_cert_sign_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                  void *channelContext, const UA_ByteString *message,
                                  UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned char hash[UA_SHA256_LENGTH];
#if MBEDTLS_VERSION_NUMBER >= 0x02070000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    // TODO check return status
    mbedtls_sha256_ret(message->data, message->length, hash, 0);
#else
    mbedtls_sha256(message->data, message->length, hash, 0);
#endif

    Aes256Sha256RsaPss_PolicyContext *pc = (Aes256Sha256RsaPss_PolicyContext *)
        policy->policyContext;

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
asym_cert_getLocalSignatureSize_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                                   const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const Aes256Sha256RsaPss_PolicyContext *pc =
        (const Aes256Sha256RsaPss_PolicyContext *) policy->policyContext;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(pc->localPrivateKey)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(pc->localPrivateKey));
#endif
}

static size_t
asym_cert_getRemoteSignatureSize_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                                    const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const Aes256Sha256RsaPss_ChannelContext *cc =
        (const Aes256Sha256RsaPss_ChannelContext*)channelContext;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->remoteCertificate.pk)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
}

/* Assumes that the certificate has been verified externally */
static UA_StatusCode
parseRemoteCertificate_aes256sha256rsapss(Aes256Sha256RsaPss_ChannelContext *cc,
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
deleteContext_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                 void *channelContext) {
    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
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
newContext_aes256sha256rsapss(const UA_SecurityPolicy *securityPolicy,
                              const UA_ByteString *remoteCertificate,
                              void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the channel context */
    *channelContext = UA_malloc(sizeof(Aes256Sha256RsaPss_ChannelContext));
    if(*channelContext == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext *)*channelContext;

    /* Initialize the channel context */
    UA_ByteString_init(&cc->localSymSigningKey);
    UA_ByteString_init(&cc->localSymEncryptingKey);
    UA_ByteString_init(&cc->localSymIv);
    UA_ByteString_init(&cc->remoteSymSigningKey);
    UA_ByteString_init(&cc->remoteSymEncryptingKey);
    UA_ByteString_init(&cc->remoteSymIv);
    mbedtls_x509_crt_init(&cc->remoteCertificate);

    // TODO: this can be optimized so that we dont allocate memory before
    // parsing the certificate
    UA_StatusCode retval =
        parseRemoteCertificate_aes256sha256rsapss(cc, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        deleteContext_aes256sha256rsapss(securityPolicy, cc);
        *channelContext = NULL;
    }
    return retval;
}

static UA_StatusCode
setLocalSymEncryptingKey_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                            void *channelContext,
                                            const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
setLocalSymSigningKey_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                         void *channelContext,
                                         const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}


static UA_StatusCode
setLocalSymIv_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                 void *channelContext, const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static UA_StatusCode
setRemoteSymEncryptingKey_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                             void *channelContext,
                                             const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
setRemoteSymSigningKey_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                          void *channelContext,
                                          const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
setRemoteSymIv_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                  void *channelContext,
                                  const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_ChannelContext *cc =
        (Aes256Sha256RsaPss_ChannelContext*)channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

static UA_StatusCode
compareCertificate_aes256sha256rsapss(const UA_SecurityPolicy *policy,
                                      const void *channelContext,
                                      const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int mbedErr = mbedtls_x509_crt_parse(&cert, certificate->data, certificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    const Aes256Sha256RsaPss_ChannelContext *cc =
        (const Aes256Sha256RsaPss_ChannelContext*)channelContext;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(cert.raw.len != cc->remoteCertificate.raw.len ||
       memcmp(cert.raw.p, cc->remoteCertificate.raw.p, cert.raw.len) != 0)
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    mbedtls_x509_crt_free(&cert);
    return retval;
}

static void
clear_aes256sha256rsapss(UA_SecurityPolicy *securityPolicy) {
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
    mbedtls_pk_free(&pc->csrLocalPrivateKey);
    mbedtls_md_free(&pc->sha256MdContext);
    UA_ByteString_clear(&pc->localCertThumbprint);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for aes256sha256rsapss");

    UA_free(pc);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
updateCertificateAndPrivateKey_aes256sha256rsapss(UA_SecurityPolicy *securityPolicy,
                                                  const UA_ByteString newCertificate,
                                                  const UA_ByteString newPrivateKey) {
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Aes256Sha256RsaPss_PolicyContext *pc =
        (Aes256Sha256RsaPss_PolicyContext*)securityPolicy->policyContext;

    UA_Boolean isLocalKey = false;
    if(newPrivateKey.length <= 0) {
        if(UA_CertificateUtils_comparePublicKeys(&newCertificate, &securityPolicy->localCertificate) == 0)
            isLocalKey = true;
    }

    UA_ByteString_clear(&securityPolicy->localCertificate);

    UA_StatusCode retval =
        UA_mbedTLS_LoadLocalCertificate(&newCertificate,
                                        &securityPolicy->localCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the new private key */
    if(newPrivateKey.length > 0) {
        mbedtls_pk_free(&pc->localPrivateKey);
        mbedtls_pk_init(&pc->localPrivateKey);
        if(UA_mbedTLS_LoadPrivateKey(&newPrivateKey, &pc->localPrivateKey, &pc->entropyContext)) {
            retval = UA_STATUSCODE_BADNOTSUPPORTED;
            goto error;
        }
    } else {
        if(!isLocalKey) {
            mbedtls_pk_free(&pc->localPrivateKey);
            pc->localPrivateKey = pc->csrLocalPrivateKey;
            mbedtls_pk_init(&pc->csrLocalPrivateKey);
        }
    }

    retval = makeThumbprint_aes256sha256rsapss(securityPolicy,
                                               &securityPolicy->localCertificate,
                                               &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return retval;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    if(securityPolicy->policyContext != NULL)
        clear_aes256sha256rsapss(securityPolicy);
    return retval;
}

static UA_StatusCode
createSigningRequest_aes256sha256rsapss(UA_SecurityPolicy *securityPolicy,
                                        const UA_String *subjectName,
                                        const UA_ByteString *nonce,
                                        const UA_KeyValueMap *params,
                                        UA_ByteString *csr,
                                        UA_ByteString *newPrivateKey) {
    if(securityPolicy == NULL || csr == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Aes256Sha256RsaPss_PolicyContext *pc =
            (Aes256Sha256RsaPss_PolicyContext *)securityPolicy->policyContext;
    return mbedtls_createSigningRequest(&pc->localPrivateKey, &pc->csrLocalPrivateKey,
                                        &pc->entropyContext, &pc->drbgContext,
                                        securityPolicy, subjectName, nonce,
                                        csr, newPrivateKey);
}

static UA_StatusCode
policyContext_newContext_aes256sha256rsapss(UA_SecurityPolicy *securityPolicy,
                                            const UA_ByteString localPrivateKey) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(localPrivateKey.length == 0) {
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
    retval = makeThumbprint_aes256sha256rsapss(securityPolicy,
                                               &securityPolicy->localCertificate,
                                               &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext: %s", UA_StatusCode_name(retval));
    if(securityPolicy->policyContext != NULL)
        clear_aes256sha256rsapss(securityPolicy);
    return retval;
}

UA_StatusCode
UA_SecurityPolicy_Aes256Sha256RsaPss(UA_SecurityPolicy *sp,
                                     const UA_ByteString localCertificate,
                                     const UA_ByteString localPrivateKey,
                                     const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss");
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(RSASHA256APPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 30;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING("http://opcfoundation.org/UA/security/rsa-pss-sha2-256");
    asymSig->verify = asym_verify_aes256sha256rsapss;
    asymSig->sign = asym_sign_aes256sha256rsapss;
    asymSig->getLocalSignatureSize = asym_getLocalSignatureSize_aes256sha256rsapss;
    asymSig->getRemoteSignatureSize = asym_getRemoteSignatureSize_aes256sha256rsapss;
    asymSig->getLocalKeyLength = NULL;
    asymSig->getRemoteKeyLength = NULL;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("http://opcfoundation.org/UA/security/rsa-oaep-sha2-256");
    asymEnc->encrypt = asym_encrypt_aes256sha256rsapss;
    asymEnc->decrypt = asym_decrypt_aes256sha256rsapss;
    asymEnc->getLocalKeyLength = asym_getLocalEncryptionKeyLength_aes256sha256rsapss;
    asymEnc->getRemoteKeyLength = asym_getRemoteEncryptionKeyLength_aes256sha256rsapss;
    asymEnc->getRemoteBlockSize = asym_getRemoteBlockSize_aes256sha256rsapss;
    asymEnc->getRemotePlainTextBlockSize = asym_getRemotePlainTextBlockSize_aes256sha256rsapss;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256");
    symSig->verify = sym_verify_aes256sha256rsapss;
    symSig->sign = sym_sign_aes256sha256rsapss;
    symSig->getLocalSignatureSize = sym_getSignatureSize_aes256sha256rsapss;
    symSig->getRemoteSignatureSize = sym_getSignatureSize_aes256sha256rsapss;
    symSig->getLocalKeyLength = sym_getSigningKeyLength_aes256sha256rsapss;
    symSig->getRemoteKeyLength = sym_getSigningKeyLength_aes256sha256rsapss;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc\0");
    symEnc->encrypt = sym_encrypt_aes256sha256rsapss;
    symEnc->decrypt = sym_decrypt_aes256sha256rsapss;
    symEnc->getLocalKeyLength = sym_getEncryptionKeyLength_aes256sha256rsapss;
    symEnc->getRemoteKeyLength = sym_getEncryptionKeyLength_aes256sha256rsapss;
    symEnc->getRemoteBlockSize = sym_getEncryptionBlockSize_aes256sha256rsapss;
    symEnc->getRemotePlainTextBlockSize = sym_getPlainTextBlockSize_aes256sha256rsapss;

    /* Certificate Signing */
    UA_SecurityPolicySignatureAlgorithm *certSig = &sp->certSignatureAlgorithm;
    certSig->uri = UA_STRING("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256");
    certSig->verify = asym_cert_verify_aes256sha256rsapss;
    certSig->sign = asym_cert_sign_aes256sha256rsapss;
    certSig->getLocalSignatureSize = asym_cert_getLocalSignatureSize_aes256sha256rsapss;
    certSig->getRemoteSignatureSize = asym_cert_getRemoteSignatureSize_aes256sha256rsapss;
    certSig->getLocalKeyLength = NULL;
    certSig->getRemoteKeyLength = NULL;

    /* Direct Method Pointers */
    sp->newChannelContext = newContext_aes256sha256rsapss;
    sp->deleteChannelContext = deleteContext_aes256sha256rsapss;
    sp->setLocalSymEncryptingKey = setLocalSymEncryptingKey_aes256sha256rsapss;
    sp->setLocalSymSigningKey = setLocalSymSigningKey_aes256sha256rsapss;
    sp->setLocalSymIv = setLocalSymIv_aes256sha256rsapss;
    sp->setRemoteSymEncryptingKey = setRemoteSymEncryptingKey_aes256sha256rsapss;
    sp->setRemoteSymSigningKey = setRemoteSymSigningKey_aes256sha256rsapss;
    sp->setRemoteSymIv = setRemoteSymIv_aes256sha256rsapss;
    sp->compareCertificate = compareCertificate_aes256sha256rsapss;
    sp->generateKey = sym_generateKey_aes256sha256rsapss;
    sp->generateNonce = sym_generateNonce_aes256sha256rsapss;
    sp->nonceLength = 32;
    sp->makeCertThumbprint = makeThumbprint_aes256sha256rsapss;
    sp->compareCertThumbprint = compareCertificateThumbprint_aes256sha256rsapss;
    sp->updateCertificate = updateCertificateAndPrivateKey_aes256sha256rsapss;
    sp->createSigningRequest = createSigningRequest_aes256sha256rsapss;
    sp->clear = clear_aes256sha256rsapss;

    UA_StatusCode res = UA_mbedTLS_LoadLocalCertificate(&localCertificate,
                                                        &sp->localCertificate);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = policyContext_newContext_aes256sha256rsapss(sp, localPrivateKey);
    if(res != UA_STATUSCODE_GOOD)
        clear_aes256sha256rsapss(sp);

    return res;
}

#endif
