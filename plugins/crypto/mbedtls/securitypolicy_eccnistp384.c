/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS) && MBEDTLS_VERSION_NUMBER >= 0x03000000

#include "securitypolicy_common.h"

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/sha512.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>

#define UA_SHA384_LENGTH 48 /* 384 bit */
#define UA_SECURITYPOLICY_ECCNISTP384_ASYM_SIGNING_KEY_LENGTH 48
#define UA_SECURITYPOLICY_ECCNISTP384_ASYM_SIGNATURE_LENGTH 96
#define UA_SECURITYPOLICY_ECCNISTP384_SYM_SIGNING_KEY_LENGTH 48
#define UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_KEY_LENGTH 32
#define UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCNISTP384_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCNISTP384_NONCE_LENGTH_BYTES 96

typedef struct {
    mbedtls_pk_context localPrivateKey;
    mbedtls_pk_context csrLocalPrivateKey;
    mbedtls_ctr_drbg_context drbgContext;
    mbedtls_entropy_context entropyContext;
    UA_ByteString localCertThumbprint;
    UA_ApplicationType applicationType;
} Policy_Context_EccNistP384;

typedef struct Channel_Context_EccNistP384 {
    mbedtls_pk_context localEphemeralKeyPair;
    UA_Boolean ephemeralKeyInitialized;
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    UA_ByteString remoteCertificate;
    mbedtls_x509_crt remoteCertificateX509;
} Channel_Context_EccNistP384;

static UA_StatusCode
UA_Policy_EccNistP384_New_Context(UA_SecurityPolicy *securityPolicy,
                                  const UA_ByteString localPrivateKey,
                                  const UA_ApplicationType applicationType,
                                  const UA_Logger *logger) {
    Policy_Context_EccNistP384 *context = (Policy_Context_EccNistP384 *)
        UA_malloc(sizeof(Policy_Context_EccNistP384));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    mbedtls_pk_init(&context->localPrivateKey);
    mbedtls_pk_init(&context->csrLocalPrivateKey);
    mbedtls_entropy_init(&context->entropyContext);
    mbedtls_ctr_drbg_init(&context->drbgContext);

    int mbedErr = mbedtls_ctr_drbg_seed(&context->drbgContext,
                                         mbedtls_entropy_func,
                                         &context->entropyContext, NULL, 0);
    if(mbedErr) {
        UA_free(context);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    mbedErr = UA_mbedTLS_LoadPrivateKey(&localPrivateKey, &context->localPrivateKey,
                                        &context->entropyContext);
    if(mbedErr) {
        mbedtls_ctr_drbg_free(&context->drbgContext);
        mbedtls_entropy_free(&context->entropyContext);
        UA_free(context);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval =
        mbedtls_thumbprint_sha1(&securityPolicy->localCertificate,
                                &context->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD) {
        mbedtls_pk_free(&context->localPrivateKey);
        mbedtls_ctr_drbg_free(&context->drbgContext);
        mbedtls_entropy_free(&context->entropyContext);
        UA_free(context);
        return retval;
    }

    context->applicationType = applicationType;
    securityPolicy->policyContext = context;
    return UA_STATUSCODE_GOOD;
}

static void
UA_Policy_EccNistP384_Clear_Context(UA_SecurityPolicy *policy) {
    if(!policy || !policy->policyContext)
        return;

    UA_ByteString_clear(&policy->localCertificate);

    Policy_Context_EccNistP384 *pc =
        (Policy_Context_EccNistP384 *)policy->policyContext;
    if(!pc)
        return;

    mbedtls_pk_free(&pc->localPrivateKey);
    mbedtls_pk_free(&pc->csrLocalPrivateKey);
    mbedtls_ctr_drbg_free(&pc->drbgContext);
    mbedtls_entropy_free(&pc->entropyContext);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);
}

static UA_StatusCode
createSigningRequest_sp_eccnistp384(UA_SecurityPolicy *securityPolicy,
                                    const UA_String *subjectName,
                                    const UA_ByteString *nonce,
                                    const UA_KeyValueMap *params,
                                    UA_ByteString *csr,
                                    UA_ByteString *newPrivateKey) {
    if(!securityPolicy || !csr)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccNistP384 *pc =
        (Policy_Context_EccNistP384*)securityPolicy->policyContext;
    return mbedtls_createSigningRequest(&pc->localPrivateKey,
                                        &pc->csrLocalPrivateKey,
                                        &pc->entropyContext,
                                        &pc->drbgContext,
                                        securityPolicy, subjectName,
                                        nonce, csr, newPrivateKey);
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_EccNistP384(UA_SecurityPolicy *securityPolicy,
                                              const UA_ByteString newCertificate,
                                              const UA_ByteString newPrivateKey) {
    if(!securityPolicy || !securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_EccNistP384 *pc =
        (Policy_Context_EccNistP384 *)securityPolicy->policyContext;

    UA_ByteString_clear(&securityPolicy->localCertificate);
    UA_StatusCode retval = UA_mbedTLS_LoadLocalCertificate(
        &newCertificate, &securityPolicy->localCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    mbedtls_pk_free(&pc->localPrivateKey);
    mbedtls_pk_init(&pc->localPrivateKey);
    int mbedErr = UA_mbedTLS_LoadPrivateKey(&newPrivateKey, &pc->localPrivateKey,
                                            &pc->entropyContext);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    UA_ByteString_clear(&pc->localCertThumbprint);
    retval = mbedtls_thumbprint_sha1(&securityPolicy->localCertificate,
                                     &pc->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    if(securityPolicy->policyContext)
        UA_Policy_EccNistP384_Clear_Context(securityPolicy);
    return retval;
}

static UA_StatusCode
EccNistP384_New_Context(const UA_SecurityPolicy *securityPolicy,
                        const UA_ByteString *remoteCertificate,
                        void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccNistP384 *newContext = (Channel_Context_EccNistP384 *)
        UA_calloc(1, sizeof(Channel_Context_EccNistP384));
    if(!newContext)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval =
        UA_ByteString_copy(remoteCertificate, &newContext->remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newContext);
        return retval;
    }

    mbedtls_x509_crt_init(&newContext->remoteCertificateX509);
    retval = UA_mbedTLS_LoadCertificate(&newContext->remoteCertificate,
                                        &newContext->remoteCertificateX509);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&newContext->remoteCertificate);
        UA_free(newContext);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

    mbedtls_pk_init(&newContext->localEphemeralKeyPair);
    newContext->ephemeralKeyInitialized = UA_FALSE;

    *channelContext = newContext;
    return UA_STATUSCODE_GOOD;
}

static void
EccNistP384_Delete_Context(const UA_SecurityPolicy *policy,
                           void *channelContext) {
    if(!channelContext)
        return;
    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    mbedtls_x509_crt_free(&cc->remoteCertificateX509);
    UA_ByteString_clear(&cc->remoteCertificate);
    UA_ByteString_clear(&cc->localSymSigningKey);
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    UA_ByteString_clear(&cc->localSymIv);
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    UA_ByteString_clear(&cc->remoteSymIv);
    if(cc->ephemeralKeyInitialized)
        mbedtls_pk_free(&cc->localEphemeralKeyPair);
    UA_free(cc);
}

static UA_StatusCode
UA_compareCertificateThumbprint_EccNistP384(const UA_SecurityPolicy *policy,
                                            const UA_ByteString *certificateThumbprint) {
    if(policy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Policy_Context_EccNistP384 *pc =
        (Policy_Context_EccNistP384 *)policy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_makeCertificateThumbprint_EccNistP384(const UA_SecurityPolicy *securityPolicy,
                                         const UA_ByteString *certificate,
                                         UA_ByteString *thumbprint) {
    return mbedtls_thumbprint_sha1(certificate, thumbprint);
}

static size_t
UA_Asym_EccNistP384_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP384_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsySig_EccNistP384_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP384_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsymEn_EccNistP384_getRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                                  const void *channelContext) {
    return 1;
}

static size_t
UA_AsymEn_EccNistP384_getRemoteBlockSize(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    return 1;
}

static size_t
UA_AsymEn_EccNistP384_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    return 1;
}

static UA_StatusCode
UA_Sym_EccNistP384_generateNonce(const UA_SecurityPolicy *policy,
                                 void *channelContext, UA_ByteString *out) {
    Policy_Context_EccNistP384 *pctx =
        (Policy_Context_EccNistP384*)policy->policyContext;
    if(!pctx)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    if(out->data[0] == 'e' && out->data[1] == 'p' && out->data[2] == 'h') {
        Channel_Context_EccNistP384 *cctx = (Channel_Context_EccNistP384*)channelContext;
        if(cctx->ephemeralKeyInitialized)
            mbedtls_pk_free(&cctx->localEphemeralKeyPair);
        UA_StatusCode res = UA_mbedTLS_ECC_NISTP384_GenerateKey(
            &cctx->localEphemeralKeyPair, &pctx->drbgContext, out);
        if(res == UA_STATUSCODE_GOOD)
            cctx->ephemeralKeyInitialized = UA_TRUE;
        return res;
    }

    int rc = mbedtls_ctr_drbg_random(&pctx->drbgContext, out->data, out->length);
    return (rc == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADUNEXPECTEDERROR;
}

static size_t
UA_SymEn_EccNistP384_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                       const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymSig_EccNistP384_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_Sym_EccNistP384_generateKey(const UA_SecurityPolicy *policy,
                               void *channelContext, const UA_ByteString *secret,
                               const UA_ByteString *seed, UA_ByteString *out) {
    Policy_Context_EccNistP384 *pctx =
        (Policy_Context_EccNistP384 *)policy->policyContext;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    if(!pctx || !cc)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_mbedTLS_ECC_DeriveKeys(MBEDTLS_ECP_DP_SECP384R1, MBEDTLS_MD_SHA384,
                                     pctx->applicationType,
                                     &cc->localEphemeralKeyPair,
                                     &pctx->drbgContext,
                                     secret, seed, out);
}

static UA_StatusCode
EccNistP384_setLocalSymSigningKey(const UA_SecurityPolicy *policy,
                                  void *channelContext,
                                  const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
EccNistP384_setLocalSymEncryptingKey(const UA_SecurityPolicy *policy,
                                     void *channelContext,
                                     const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
EccNistP384_setLocalSymIv(const UA_SecurityPolicy *policy,
                          void *channelContext,
                          const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static size_t
UA_SymEn_EccNistP384_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_EccNistP384_getBlockSize(const UA_SecurityPolicy *policy,
                                  const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
UA_SymSig_EccNistP384_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
EccNistP384_setRemoteSymSigningKey(const UA_SecurityPolicy *policy,
                                   void *channelContext,
                                   const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
EccNistP384_setRemoteSymEncryptingKey(const UA_SecurityPolicy *policy,
                                      void *channelContext,
                                      const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
EccNistP384_setRemoteSymIv(const UA_SecurityPolicy *policy,
                           void *channelContext, const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

static UA_StatusCode
UA_AsymSig_EccNistP384_sign(const UA_SecurityPolicy *policy,
                            void *channelContext, const UA_ByteString *message,
                            UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccNistP384 *pc =
        (Policy_Context_EccNistP384 *)policy->policyContext;
    return UA_mbedTLS_ECDSA_SHA384_Sign(message, &pc->localPrivateKey,
                                        &pc->drbgContext, signature);
}

static UA_StatusCode
UA_AsymSig_EccNistP384_verify(const UA_SecurityPolicy *policy, void *channelContext,
                              const UA_ByteString *message,
                              const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    return UA_mbedTLS_ECDSA_SHA384_Verify(message, &cc->remoteCertificateX509, signature);
}

static UA_StatusCode
UA_Asym_EccNistP384_Dummy(const UA_SecurityPolicy *policy,
                          void *channelContext, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static size_t
UA_SymSig_EccNistP384_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                             const void *channelContext) {
    return UA_SHA384_LENGTH;
}

static UA_StatusCode
UA_SymSig_EccNistP384_verify(const UA_SecurityPolicy *policy, void *channelContext,
                             const UA_ByteString *message, const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    return UA_mbedTLS_HMAC_SHA384_Verify(message, &cc->remoteSymSigningKey, signature);
}

static UA_StatusCode
UA_SymSig_EccNistP384_sign(const UA_SecurityPolicy *policy,
                           void *channelContext, const UA_ByteString *message,
                           UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;
    return UA_mbedTLS_HMAC_SHA384_Sign(message, &cc->localSymSigningKey, signature);
}

static size_t
UA_SymSig_EccNistP384_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    return UA_SHA384_LENGTH;
}

static UA_StatusCode
UA_SymEn_EccNistP384_decrypt(const UA_SecurityPolicy *policy,
                             void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;

    if(cc->remoteSymIv.length != UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_BLOCK_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(data->length % UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_BLOCK_SIZE != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned int keylength = (unsigned int)(cc->remoteSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_dec(&aesContext, cc->remoteSymEncryptingKey.data,
                                         keylength);
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
UA_SymEn_EccNistP384_encrypt(const UA_SecurityPolicy *policy,
                             void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc = (Channel_Context_EccNistP384 *)channelContext;

    if(cc->localSymIv.length != UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_BLOCK_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t plainTextBlockSize = UA_SECURITYPOLICY_ECCNISTP384_SYM_PLAIN_TEXT_BLOCK_SIZE;
    if(data->length % plainTextBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    unsigned int keylength = (unsigned int)(cc->localSymEncryptingKey.length * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_enc(&aesContext, cc->localSymEncryptingKey.data,
                                         keylength);
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
EccNistP384_compareCertificate(const UA_SecurityPolicy *policy,
                               const void *channelContext,
                               const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const Channel_Context_EccNistP384 *cc =
        (const Channel_Context_EccNistP384 *)channelContext;

    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    UA_StatusCode retval = UA_mbedTLS_LoadCertificate(certificate, &cert);
    if(retval != UA_STATUSCODE_GOOD) {
        mbedtls_x509_crt_free(&cert);
        return retval;
    }

    if(cert.raw.len != cc->remoteCertificateX509.raw.len ||
       memcmp(cert.raw.p, cc->remoteCertificateX509.raw.p, cert.raw.len) != 0) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    mbedtls_x509_crt_free(&cert);
    return retval;
}

static size_t
UA_AsymEn_EccNistP384_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    return 1;
}

UA_StatusCode
UA_SecurityPolicy_EccNistP384(UA_SecurityPolicy *sp,
                              const UA_ApplicationType applicationType,
                              const UA_ByteString localCertificate,
                              const UA_ByteString localPrivateKey,
                              const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384\0");
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(ECCNISTP384APPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 15;
    sp->policyType = UA_SECURITYPOLICYTYPE_ECC;

    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING_NULL;
    asymSig->verify = UA_AsymSig_EccNistP384_verify;
    asymSig->sign = UA_AsymSig_EccNistP384_sign;
    asymSig->getLocalSignatureSize = UA_AsySig_EccNistP384_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = UA_Asym_EccNistP384_getRemoteSignatureSize;

    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2720\0");
    asymEnc->encrypt = UA_Asym_EccNistP384_Dummy;
    asymEnc->decrypt = UA_Asym_EccNistP384_Dummy;
    asymEnc->getLocalKeyLength = UA_AsymEn_EccNistP384_getLocalKeyLength;
    asymEnc->getRemoteKeyLength = UA_AsymEn_EccNistP384_getRemoteKeyLength;
    asymEnc->getRemoteBlockSize = UA_AsymEn_EccNistP384_getRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = UA_AsymEn_EccNistP384_getRemotePlainTextBlockSize;

    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-384\0");
    symSig->verify = UA_SymSig_EccNistP384_verify;
    symSig->sign = UA_SymSig_EccNistP384_sign;
    symSig->getLocalSignatureSize = UA_SymSig_EccNistP384_getLocalSignatureSize;
    symSig->getRemoteSignatureSize = UA_SymSig_EccNistP384_getRemoteSignatureSize;
    symSig->getLocalKeyLength = UA_SymSig_EccNistP384_getLocalKeyLength;
    symSig->getRemoteKeyLength = UA_SymSig_EccNistP384_getRemoteKeyLength;

    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc\0");
    symEnc->encrypt = UA_SymEn_EccNistP384_encrypt;
    symEnc->decrypt = UA_SymEn_EccNistP384_decrypt;
    symEnc->getLocalKeyLength = UA_SymEn_EccNistP384_getLocalKeyLength;
    symEnc->getRemoteKeyLength = UA_SymEn_EccNistP384_getRemoteKeyLength;
    symEnc->getRemoteBlockSize = UA_SymEn_EccNistP384_getBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_SymEn_EccNistP384_getBlockSize;

    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    sp->newChannelContext = EccNistP384_New_Context;
    sp->deleteChannelContext = EccNistP384_Delete_Context;
    sp->setLocalSymEncryptingKey = EccNistP384_setLocalSymEncryptingKey;
    sp->setLocalSymSigningKey = EccNistP384_setLocalSymSigningKey;
    sp->setLocalSymIv = EccNistP384_setLocalSymIv;
    sp->setRemoteSymEncryptingKey = EccNistP384_setRemoteSymEncryptingKey;
    sp->setRemoteSymSigningKey = EccNistP384_setRemoteSymSigningKey;
    sp->setRemoteSymIv = EccNistP384_setRemoteSymIv;
    sp->compareCertificate = EccNistP384_compareCertificate;
    sp->generateKey = UA_Sym_EccNistP384_generateKey;
    sp->generateNonce = UA_Sym_EccNistP384_generateNonce;
    sp->nonceLength = UA_SECURITYPOLICY_ECCNISTP384_NONCE_LENGTH_BYTES;
    sp->makeCertThumbprint = UA_makeCertificateThumbprint_EccNistP384;
    sp->compareCertThumbprint = UA_compareCertificateThumbprint_EccNistP384;
    sp->updateCertificate = updateCertificateAndPrivateKey_sp_EccNistP384;
    sp->createSigningRequest = createSigningRequest_sp_eccnistp384;
    sp->clear = UA_Policy_EccNistP384_Clear_Context;

    UA_StatusCode res =
        UA_mbedTLS_LoadLocalCertificate(&localCertificate, &sp->localCertificate);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = UA_Policy_EccNistP384_New_Context(sp, localPrivateKey, applicationType, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

#endif
