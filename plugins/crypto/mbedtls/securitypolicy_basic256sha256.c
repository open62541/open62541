/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2018 (c) Daniel Feist, Precitec GmbH & Co. KG
 *    Copyright 2018 (c) HMS Industrial Networks AB (Author: Jonas Green)
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
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
 * mbedTLS' AES allows in-place encryption and decryption. Sow we don't have to
 * allocate temp buffers.
 * https://tls.mbed.org/discussions/generic/in-place-decryption-with-aes256-same-input-output-buffer
 */

#define UA_SECURITYPOLICY_BASIC256SHA256_RSAPADDING_LEN 42
#define UA_SHA1_LENGTH 20
#define UA_SHA256_LENGTH 32
#define UA_BASIC256SHA256_SYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_BASIC256SHA256_SYM_KEY_LENGTH 32
#define UA_SECURITYPOLICY_BASIC256SHA256_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC256SHA256_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC256SHA256_MINASYMKEYLENGTH 256
#define UA_SECURITYPOLICY_BASIC256SHA256_MAXASYMKEYLENGTH 512

typedef struct {
    UA_ByteString localCertThumbprint;

    mbedtls_ctr_drbg_context drbgContext;
    mbedtls_entropy_context entropyContext;
    mbedtls_md_context_t sha256MdContext;
    mbedtls_pk_context localPrivateKey;
    mbedtls_pk_context csrLocalPrivateKey;
} Basic256Sha256_PolicyContext;

typedef struct {
    Basic256Sha256_PolicyContext *policyContext;

    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;

    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    mbedtls_x509_crt remoteCertificate;
} Basic256Sha256_ChannelContext;

/********************/
/* AsymmetricModule */
/********************/

/* VERIFY AsymmetricSignatureAlgorithm_RSA-PKCS15-SHA2-256 */
static UA_StatusCode
asym_verify_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
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
asym_sign_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
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

    Basic256Sha256_PolicyContext *pc = cc->policyContext;
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
asym_getLocalSignatureSize_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->policyContext->localPrivateKey)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->policyContext->localPrivateKey));
#endif
}

static size_t
asym_getRemoteSignatureSize_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    return mbedtls_pk_rsa(cc->remoteCertificate.pk)->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
}

static size_t
asym_getRemoteBlockSize_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_context *const rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    return rsaContext->len;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
}

static size_t
asym_getRemotePlainTextBlockSize_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc) {
    if(cc == NULL)
        return 0;
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_context *const rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    return rsaContext->len - UA_SECURITYPOLICY_BASIC256SHA256_RSAPADDING_LEN;
#else
    return mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk)) -
        UA_SECURITYPOLICY_BASIC256SHA256_RSAPADDING_LEN;
#endif
}


/* AsymmetricEncryptionAlgorithm_RSA-OAEP-SHA1 */
static UA_StatusCode
asym_encrypt_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                               UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const size_t plainTextBlockSize = asym_getRemotePlainTextBlockSize_sp_basic256sha256(cc);

    mbedtls_rsa_context *remoteRsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    mbedtls_rsa_set_padding(remoteRsaContext, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA1);

    return mbedtls_encrypt_rsaOaep(remoteRsaContext, &cc->policyContext->drbgContext,
                                   data, plainTextBlockSize);
}

/* AsymmetricEncryptionAlgorithm_RSA-OAEP-SHA1 */
static UA_StatusCode
asym_decrypt_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                               UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return mbedtls_decrypt_rsaOaep(&cc->policyContext->localPrivateKey,
                                   &cc->policyContext->drbgContext, data, MBEDTLS_MD_SHA1);
}

static size_t
asym_getLocalEncryptionKeyLength_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc) {
    return mbedtls_pk_get_len(&cc->policyContext->localPrivateKey) * 8;
}

static size_t
asym_getRemoteEncryptionKeyLength_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc) {
    return mbedtls_pk_get_len(&cc->remoteCertificate.pk) * 8;
}

static UA_StatusCode
asym_makeThumbprint_sp_basic256sha256(const UA_SecurityPolicy *securityPolicy,
                                      const UA_ByteString *certificate,
                                      UA_ByteString *thumbprint) {
    if(securityPolicy == NULL || certificate == NULL || thumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return mbedtls_thumbprint_sha1(certificate, thumbprint);
}

static UA_StatusCode
asymmetricModule_compareCertificateThumbprint_sp_basic256sha256(const UA_SecurityPolicy *securityPolicy,
                                                                const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Basic256Sha256_PolicyContext *pc = (Basic256Sha256_PolicyContext *)securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    return UA_STATUSCODE_GOOD;
}

/*******************/
/* SymmetricModule */
/*******************/

static UA_StatusCode
sym_verify_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                             const UA_ByteString *message,
                             const UA_ByteString *signature) {
    if(cc == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute MAC */
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    Basic256Sha256_PolicyContext *pc = cc->policyContext;
    unsigned char mac[UA_SHA256_LENGTH];
    if(mbedtls_hmac(&pc->sha256MdContext, &cc->remoteSymSigningKey, message, mac) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Compare with Signature */
    if(!UA_constantTimeEqual(signature->data, mac, UA_SHA256_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sym_sign_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc,
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
sym_getSignatureSize_sp_basic256sha256(const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static size_t
sym_getSigningKeyLength_sp_basic256sha256(const void *channelContext) {
    return UA_BASIC256SHA256_SYM_SIGNING_KEY_LENGTH;
}

static size_t
sym_getEncryptionKeyLength_sp_basic256sha256(const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC256SHA256_SYM_KEY_LENGTH;
}

static size_t
sym_getEncryptionBlockSize_sp_basic256sha256(const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC256SHA256_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
sym_getPlainTextBlockSize_sp_basic256sha256(const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC256SHA256_SYM_PLAIN_TEXT_BLOCK_SIZE;
}

static UA_StatusCode
sym_encrypt_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc,
                              UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(cc->localSymIv.length != UA_SECURITYPOLICY_BASIC256SHA256_SYM_ENCRYPTION_BLOCK_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t plainTextBlockSize = UA_SECURITYPOLICY_BASIC256SHA256_SYM_PLAIN_TEXT_BLOCK_SIZE;

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
sym_decrypt_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc,
                              UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t encryptionBlockSize = UA_SECURITYPOLICY_BASIC256SHA256_SYM_ENCRYPTION_BLOCK_SIZE;
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
sym_generateKey_sp_basic256sha256(void *policyContext, const UA_ByteString *secret,
                                  const UA_ByteString *seed, UA_ByteString *out) {
    if(secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Basic256Sha256_PolicyContext *pc = (Basic256Sha256_PolicyContext *)policyContext;
    return mbedtls_generateKey(&pc->sha256MdContext, secret, seed, out);
}

static UA_StatusCode
sym_generateNonce_sp_basic256sha256(void *policyContext, UA_ByteString *out) {
    if(out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Basic256Sha256_PolicyContext *pc = (Basic256Sha256_PolicyContext *)policyContext;
    int mbedErr = mbedtls_ctr_drbg_random(&pc->drbgContext, out->data, out->length);
    if(mbedErr)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

/*****************/
/* ChannelModule */
/*****************/

/* Assumes that the certificate has been verified externally */
static UA_StatusCode
parseRemoteCertificate_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                                         const UA_ByteString *remoteCertificate) {
    if(remoteCertificate == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Parse the certificate */
    int mbedErr = mbedtls_x509_crt_parse(&cc->remoteCertificate, remoteCertificate->data,
                                         remoteCertificate->length);
    if(mbedErr)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Check the key length */
#if MBEDTLS_VERSION_NUMBER >= 0x02060000 && MBEDTLS_VERSION_NUMBER < 0x03000000
    mbedtls_rsa_context *rsaContext = mbedtls_pk_rsa(cc->remoteCertificate.pk);
    size_t keylen = rsaContext->len;
#else
    size_t keylen = mbedtls_rsa_get_len(mbedtls_pk_rsa(cc->remoteCertificate.pk));
#endif
    if(keylen < UA_SECURITYPOLICY_BASIC256SHA256_MINASYMKEYLENGTH ||
       keylen > UA_SECURITYPOLICY_BASIC256SHA256_MAXASYMKEYLENGTH)
        return UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED;

    return UA_STATUSCODE_GOOD;
}

static void
channelContext_deleteContext_sp_basic256sha256(Basic256Sha256_ChannelContext *cc) {
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
channelContext_newContext_sp_basic256sha256(const UA_SecurityPolicy *securityPolicy,
                                            const UA_ByteString *remoteCertificate,
                                            void **pp_contextData) {
    if(securityPolicy == NULL || remoteCertificate == NULL || pp_contextData == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the channel context */
    *pp_contextData = UA_malloc(sizeof(Basic256Sha256_ChannelContext));
    if(*pp_contextData == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    Basic256Sha256_ChannelContext *cc = (Basic256Sha256_ChannelContext *)*pp_contextData;

    /* Initialize the channel context */
    cc->policyContext = (Basic256Sha256_PolicyContext *)securityPolicy->policyContext;

    UA_ByteString_init(&cc->localSymSigningKey);
    UA_ByteString_init(&cc->localSymEncryptingKey);
    UA_ByteString_init(&cc->localSymIv);

    UA_ByteString_init(&cc->remoteSymSigningKey);
    UA_ByteString_init(&cc->remoteSymEncryptingKey);
    UA_ByteString_init(&cc->remoteSymIv);

    mbedtls_x509_crt_init(&cc->remoteCertificate);

    // TODO: this can be optimized so that we dont allocate memory before parsing the certificate
    UA_StatusCode retval = parseRemoteCertificate_sp_basic256sha256(cc, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        channelContext_deleteContext_sp_basic256sha256(cc);
        *pp_contextData = NULL;
    }
    return retval;
}

static UA_StatusCode
channelContext_setLocalSymEncryptingKey_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                                                          const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
channelContext_setLocalSymSigningKey_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                                                       const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}


static UA_StatusCode
channelContext_setLocalSymIv_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                                               const UA_ByteString *iv) {
    if(iv == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static UA_StatusCode
channelContext_setRemoteSymEncryptingKey_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                                                           const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
channelContext_setRemoteSymSigningKey_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                                                        const UA_ByteString *key) {
    if(key == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
channelContext_setRemoteSymIv_sp_basic256sha256(Basic256Sha256_ChannelContext *cc,
                                                const UA_ByteString *iv) {
    if(iv == NULL || cc == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

static UA_StatusCode
channelContext_compareCertificate_sp_basic256sha256(const Basic256Sha256_ChannelContext *cc,
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
clear_sp_basic256sha256(UA_SecurityPolicy *securityPolicy) {
    if(securityPolicy == NULL)
        return;

    UA_ByteString_clear(&securityPolicy->localCertificate);

    if(securityPolicy->policyContext == NULL)
        return;

    /* delete all allocated members in the context */
    Basic256Sha256_PolicyContext *pc = (Basic256Sha256_PolicyContext *)
        securityPolicy->policyContext;

    mbedtls_ctr_drbg_free(&pc->drbgContext);
    mbedtls_entropy_free(&pc->entropyContext);
    mbedtls_pk_free(&pc->localPrivateKey);
    mbedtls_pk_free(&pc->csrLocalPrivateKey);
    mbedtls_md_free(&pc->sha256MdContext);
    UA_ByteString_clear(&pc->localCertThumbprint);

    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for sp_basic256sha256");

    UA_free(pc);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_basic256sha256(UA_SecurityPolicy *securityPolicy,
                                                 const UA_ByteString newCertificate,
                                                 const UA_ByteString newPrivateKey) {
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Basic256Sha256_PolicyContext *pc =
            (Basic256Sha256_PolicyContext *) securityPolicy->policyContext;

    UA_Boolean isLocalKey = false;
    if(newPrivateKey.length <= 0) {
        if(UA_CertificateUtils_comparePublicKeys(&newCertificate, &securityPolicy->localCertificate) == 0)
            isLocalKey = true;
    }

    UA_ByteString_clear(&securityPolicy->localCertificate);

    UA_StatusCode retval = UA_mbedTLS_LoadLocalCertificate(&newCertificate, &securityPolicy->localCertificate);

    if (retval != UA_STATUSCODE_GOOD)
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

    retval = asym_makeThumbprint_sp_basic256sha256(securityPolicy,
                                                   &securityPolicy->localCertificate,
                                                   &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return retval;

    error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    if(securityPolicy->policyContext != NULL)
        clear_sp_basic256sha256(securityPolicy);
    return retval;
}

static UA_StatusCode
createSigningRequest_sp_basic256sha256(UA_SecurityPolicy *securityPolicy,
                                            const UA_String *subjectName,
                                            const UA_ByteString *nonce,
                                            const UA_KeyValueMap *params,
                                            UA_ByteString *csr,
                                            UA_ByteString *newPrivateKey) {
    /* Check parameter */
    if (securityPolicy == NULL || csr == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Basic256Sha256_PolicyContext *pc =
            (Basic256Sha256_PolicyContext *) securityPolicy->policyContext;

    return mbedtls_createSigningRequest(&pc->localPrivateKey, &pc->csrLocalPrivateKey,
                                        &pc->entropyContext, &pc->drbgContext,
                                        securityPolicy, subjectName, nonce,
                                        csr, newPrivateKey);
}

static UA_StatusCode
policyContext_newContext_sp_basic256sha256(UA_SecurityPolicy *securityPolicy,
                                           const UA_ByteString localPrivateKey) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if (localPrivateKey.length == 0) {
        UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                     "Can not initialize security policy. Private key is empty.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    Basic256Sha256_PolicyContext *pc = (Basic256Sha256_PolicyContext *)
        UA_malloc(sizeof(Basic256Sha256_PolicyContext));
    securityPolicy->policyContext = (void *)pc;
    if(!pc) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Initialize the PolicyContext */
    memset(pc, 0, sizeof(Basic256Sha256_PolicyContext));
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
    retval = asym_makeThumbprint_sp_basic256sha256(securityPolicy,
                                                  &securityPolicy->localCertificate,
                                                  &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext: %s", UA_StatusCode_name(retval));
    if(securityPolicy->policyContext != NULL)
        clear_sp_basic256sha256(securityPolicy);
    return retval;
}

UA_StatusCode
UA_SecurityPolicy_Basic256Sha256(UA_SecurityPolicy *policy, const UA_ByteString localCertificate,
                                 const UA_ByteString localPrivateKey, const UA_Logger *logger) {
    memset(policy, 0, sizeof(UA_SecurityPolicy));
    policy->logger = logger;

    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    policy->certificateGroupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    policy->certificateTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
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
        UA_STRING("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256\0");
    asym_signatureAlgorithm->verify =
        (UA_StatusCode (*)(void *, const UA_ByteString *, const UA_ByteString *))asym_verify_sp_basic256sha256;
    asym_signatureAlgorithm->sign =
        (UA_StatusCode (*)(void *, const UA_ByteString *, UA_ByteString *))asym_sign_sp_basic256sha256;
    asym_signatureAlgorithm->getLocalSignatureSize =
        (size_t (*)(const void *))asym_getLocalSignatureSize_sp_basic256sha256;
    asym_signatureAlgorithm->getRemoteSignatureSize =
        (size_t (*)(const void *))asym_getRemoteSignatureSize_sp_basic256sha256;
    asym_signatureAlgorithm->getLocalKeyLength = NULL; // TODO: Write function
    asym_signatureAlgorithm->getRemoteKeyLength = NULL; // TODO: Write function

    UA_SecurityPolicyEncryptionAlgorithm *asym_encryptionAlgorithm =
        &asymmetricModule->cryptoModule.encryptionAlgorithm;
    asym_encryptionAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#rsa-oaep\0");
    asym_encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))asym_encrypt_sp_basic256sha256;
    asym_encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))
            asym_decrypt_sp_basic256sha256;
    asym_encryptionAlgorithm->getLocalKeyLength =
        (size_t (*)(const void *))asym_getLocalEncryptionKeyLength_sp_basic256sha256;
    asym_encryptionAlgorithm->getRemoteKeyLength =
        (size_t (*)(const void *))asym_getRemoteEncryptionKeyLength_sp_basic256sha256;
    asym_encryptionAlgorithm->getRemoteBlockSize =
        (size_t (*)(const void *))asym_getRemoteBlockSize_sp_basic256sha256;
    asym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t (*)(const void *))asym_getRemotePlainTextBlockSize_sp_basic256sha256;

    asymmetricModule->makeCertificateThumbprint = asym_makeThumbprint_sp_basic256sha256;
    asymmetricModule->compareCertificateThumbprint =
        asymmetricModule_compareCertificateThumbprint_sp_basic256sha256;

    /* SymmetricModule */
    symmetricModule->generateKey = sym_generateKey_sp_basic256sha256;
    symmetricModule->generateNonce = sym_generateNonce_sp_basic256sha256;

    UA_SecurityPolicySignatureAlgorithm *sym_signatureAlgorithm =
        &symmetricModule->cryptoModule.signatureAlgorithm;
    sym_signatureAlgorithm->uri =
        UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256\0");
    sym_signatureAlgorithm->verify =
        (UA_StatusCode (*)(void *, const UA_ByteString *, const UA_ByteString *))sym_verify_sp_basic256sha256;
    sym_signatureAlgorithm->sign =
        (UA_StatusCode (*)(void *, const UA_ByteString *, UA_ByteString *))sym_sign_sp_basic256sha256;
    sym_signatureAlgorithm->getLocalSignatureSize = sym_getSignatureSize_sp_basic256sha256;
    sym_signatureAlgorithm->getRemoteSignatureSize = sym_getSignatureSize_sp_basic256sha256;
    sym_signatureAlgorithm->getLocalKeyLength =
        (size_t (*)(const void *))sym_getSigningKeyLength_sp_basic256sha256;
    sym_signatureAlgorithm->getRemoteKeyLength =
        (size_t (*)(const void *))sym_getSigningKeyLength_sp_basic256sha256;

    UA_SecurityPolicyEncryptionAlgorithm *sym_encryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    sym_encryptionAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc");
    sym_encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))sym_encrypt_sp_basic256sha256;
    sym_encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))sym_decrypt_sp_basic256sha256;
    sym_encryptionAlgorithm->getLocalKeyLength = sym_getEncryptionKeyLength_sp_basic256sha256;
    sym_encryptionAlgorithm->getRemoteKeyLength = sym_getEncryptionKeyLength_sp_basic256sha256;
    sym_encryptionAlgorithm->getRemoteBlockSize =
        (size_t (*)(const void *))sym_getEncryptionBlockSize_sp_basic256sha256;
    sym_encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t (*)(const void *))sym_getPlainTextBlockSize_sp_basic256sha256;
    symmetricModule->secureChannelNonceLength = 32;

    // Use the same signature algorithm as the asymmetric component for certificate signing (see standard)
    policy->certificateSigningAlgorithm = policy->asymmetricModule.cryptoModule.signatureAlgorithm;

    /* ChannelModule */
    channelModule->newContext = channelContext_newContext_sp_basic256sha256;
    channelModule->deleteContext = (void (*)(void *))
        channelContext_deleteContext_sp_basic256sha256;

    channelModule->setLocalSymEncryptingKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setLocalSymEncryptingKey_sp_basic256sha256;
    channelModule->setLocalSymSigningKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setLocalSymSigningKey_sp_basic256sha256;
    channelModule->setLocalSymIv = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setLocalSymIv_sp_basic256sha256;

    channelModule->setRemoteSymEncryptingKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setRemoteSymEncryptingKey_sp_basic256sha256;
    channelModule->setRemoteSymSigningKey = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setRemoteSymSigningKey_sp_basic256sha256;
    channelModule->setRemoteSymIv = (UA_StatusCode (*)(void *, const UA_ByteString *))
        channelContext_setRemoteSymIv_sp_basic256sha256;

    channelModule->compareCertificate = (UA_StatusCode (*)(const void *, const UA_ByteString *))
        channelContext_compareCertificate_sp_basic256sha256;

    policy->updateCertificateAndPrivateKey = updateCertificateAndPrivateKey_sp_basic256sha256;
    policy->createSigningRequest = createSigningRequest_sp_basic256sha256;
    policy->clear = clear_sp_basic256sha256;

    UA_StatusCode res = policyContext_newContext_sp_basic256sha256(policy, localPrivateKey);
    if(res != UA_STATUSCODE_GOOD)
        clear_sp_basic256sha256(policy);

    return res;
}

#endif
