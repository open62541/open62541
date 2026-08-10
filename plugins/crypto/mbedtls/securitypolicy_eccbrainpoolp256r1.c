/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
#include <mbedtls/version.h>

#include "securitypolicy_common.h"

#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>

#define UA_SHA256_LENGTH 32 /* 256 bit */
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_ASYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_ASYM_SIGNATURE_LENGTH 64
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_KEY_LENGTH 16
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_NONCE_LENGTH_BYTES 64

typedef struct {
    UA_ByteString localCertThumbprint;
    mbedtls_pk_context localPrivateKey;
    mbedtls_pk_context csrLocalPrivateKey;
    UA_ApplicationType applicationType;
} Policy_Context_EccBrainpoolP256r1;

typedef struct Channel_Context_EccBrainpoolP256r1 {
    UA_mbedTLS_PsaKey localEphemeralKeyPair;
    UA_mbedTLS_PsaKey localSymSigningKey;
    UA_mbedTLS_PsaKey localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_mbedTLS_PsaKey remoteSymSigningKey;
    UA_mbedTLS_PsaKey remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    UA_ByteString remoteCertificate;
    mbedtls_x509_crt remoteCertificateX509;
} Channel_Context_EccBrainpoolP256r1;

static UA_StatusCode
UA_Policy_EccBrainpoolP256r1_New_Context(UA_SecurityPolicy *securityPolicy,
                                  const UA_ByteString localPrivateKey,
                                  const UA_ApplicationType applicationType,
                                  const UA_Logger *logger) {
    Policy_Context_EccBrainpoolP256r1 *context = (Policy_Context_EccBrainpoolP256r1 *)
        UA_malloc(sizeof(Policy_Context_EccBrainpoolP256r1));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    mbedtls_pk_init(&context->localPrivateKey);
    mbedtls_pk_init(&context->csrLocalPrivateKey);
    int mbedErr = UA_mbedTLS_LoadPrivateKey(&localPrivateKey,
                                            &context->localPrivateKey);
    if(mbedErr) {
        UA_free(context);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Verify the key is an EC signing key */
    if(!UA_mbedTLS_IsEccKeyPair(&context->localPrivateKey)) {
        mbedtls_pk_free(&context->localPrivateKey);
        UA_free(context);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval =
        UA_ByteString_allocBuffer(&context->localCertThumbprint, UA_SHA1_LENGTH);
    if(retval != UA_STATUSCODE_GOOD) {
        mbedtls_pk_free(&context->localPrivateKey);
        UA_free(context);
        return retval;
    }

    retval = UA_mbedTLS_thumbprintSha1(&securityPolicy->localCertificate,
                                     &context->localCertThumbprint);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&context->localCertThumbprint);
        mbedtls_pk_free(&context->localPrivateKey);
        UA_free(context);
        return retval;
    }

    context->applicationType = applicationType;
    securityPolicy->policyContext = context;
    return UA_STATUSCODE_GOOD;
}

static void
UA_Policy_EccBrainpoolP256r1_Clear_Context(UA_SecurityPolicy *policy) {
    if(!policy || !policy->policyContext)
        return;

    UA_ByteString_clear(&policy->localCertificate);

    Policy_Context_EccBrainpoolP256r1 *pc =
        (Policy_Context_EccBrainpoolP256r1 *)policy->policyContext;
    if(!pc)
        return;

    mbedtls_pk_free(&pc->localPrivateKey);
    mbedtls_pk_free(&pc->csrLocalPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_EccBrainpoolP256r1(UA_SecurityPolicy *securityPolicy,
                                              const UA_ByteString newCertificate,
                                              const UA_ByteString newPrivateKey) {
    return UA_mbedTLS_UpdateCertificateAndPrivateKey(
        securityPolicy, newCertificate, newPrivateKey);
}

static UA_StatusCode
EccBrainpoolP256r1_New_Context(const UA_SecurityPolicy *securityPolicy,
                        const UA_ByteString *remoteCertificate,
                        void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccBrainpoolP256r1 *newContext = (Channel_Context_EccBrainpoolP256r1 *)
        UA_calloc(1, sizeof(Channel_Context_EccBrainpoolP256r1));
    if(!newContext)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval =
        UA_ByteString_copy(remoteCertificate, &newContext->remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newContext);
        return retval;
    }

    /* Decode to X509 */
    mbedtls_x509_crt_init(&newContext->remoteCertificateX509);
    retval = UA_mbedTLS_LoadCertificate(&newContext->remoteCertificate,
                                        &newContext->remoteCertificateX509);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&newContext->remoteCertificate);
        UA_free(newContext);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

    UA_mbedTLS_PsaKey_init(&newContext->localEphemeralKeyPair);
    UA_mbedTLS_PsaKey_init(&newContext->localSymSigningKey);
    UA_mbedTLS_PsaKey_init(&newContext->localSymEncryptingKey);
    UA_mbedTLS_PsaKey_init(&newContext->remoteSymSigningKey);
    UA_mbedTLS_PsaKey_init(&newContext->remoteSymEncryptingKey);

    *channelContext = newContext;
    return UA_STATUSCODE_GOOD;
}

static void
EccBrainpoolP256r1_Delete_Context(const UA_SecurityPolicy *policy,
                           void *channelContext) {
    if(!channelContext)
        return;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    mbedtls_x509_crt_free(&cc->remoteCertificateX509);
    UA_ByteString_clear(&cc->remoteCertificate);
    UA_mbedTLS_PsaKey_clear(&cc->localSymSigningKey);
    UA_mbedTLS_PsaKey_clear(&cc->localSymEncryptingKey);
    UA_ByteString_clear(&cc->localSymIv);
    UA_mbedTLS_PsaKey_clear(&cc->remoteSymSigningKey);
    UA_mbedTLS_PsaKey_clear(&cc->remoteSymEncryptingKey);
    UA_ByteString_clear(&cc->remoteSymIv);
    UA_mbedTLS_PsaKey_clear(&cc->localEphemeralKeyPair);
    UA_free(cc);
}

static UA_StatusCode
UA_compareCertificateThumbprint_EccBrainpoolP256r1(const UA_SecurityPolicy *policy,
                                            const UA_ByteString *certificateThumbprint) {
    if(policy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Policy_Context_EccBrainpoolP256r1 *pc =
        (Policy_Context_EccBrainpoolP256r1 *)policy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_makeCertificateThumbprint_EccBrainpoolP256r1(const UA_SecurityPolicy *securityPolicy,
                                         const UA_ByteString *certificate,
                                         UA_ByteString *thumbprint) {
    return UA_mbedTLS_thumbprintSha1(certificate, thumbprint);
}

static size_t
UA_Asym_EccBrainpoolP256r1_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    return UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsySig_EccBrainpoolP256r1_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    return UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsymEn_EccBrainpoolP256r1_getRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                                  const void *channelContext) {
    return 1;
}

static size_t
UA_AsymEn_EccBrainpoolP256r1_getRemoteBlockSize(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    return 1;
}

static size_t
UA_AsymEn_EccBrainpoolP256r1_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    /* No ECC encryption -> key length set to 1 to avoid division or
     * multiplication with 0 */
    return 1;
}

static UA_StatusCode
UA_Sym_EccBrainpoolP256r1_generateNonce(const UA_SecurityPolicy *policy,
                                 void *channelContext, UA_ByteString *out) {
    Channel_Context_EccBrainpoolP256r1 *cctx =
        (Channel_Context_EccBrainpoolP256r1*)channelContext;
    return UA_mbedTLS_EccGenerateNonce(
        policy, cctx ? &cctx->localEphemeralKeyPair : NULL,
        PSA_ECC_FAMILY_BRAINPOOL_P_R1, 256, out);
}

static size_t
UA_SymEn_EccBrainpoolP256r1_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                       const void *channelContext) {
    return UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymSig_EccBrainpoolP256r1_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    return UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_Sym_EccBrainpoolP256r1_generateKey(const UA_SecurityPolicy *policy,
                               void *channelContext, const UA_ByteString *secret,
                               const UA_ByteString *seed, UA_ByteString *out) {
    Policy_Context_EccBrainpoolP256r1 *pctx =
        (Policy_Context_EccBrainpoolP256r1 *)policy->policyContext;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    if(!pctx || !cc)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_mbedTLS_PsaEccDerive(PSA_ALG_SHA_256, pctx->applicationType,
                                   &cc->localEphemeralKeyPair,
                                   secret, seed, out);
}

static UA_StatusCode
EccBrainpoolP256r1_setLocalSymSigningKey(const UA_SecurityPolicy *policy,
                                  void *channelContext,
                                  const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_mbedTLS_PsaKey_import(&cc->localSymSigningKey, PSA_KEY_TYPE_HMAC,
        PSA_KEY_USAGE_SIGN_MESSAGE, PSA_ALG_HMAC(PSA_ALG_SHA_256), key);
}

static UA_StatusCode
EccBrainpoolP256r1_setLocalSymEncryptingKey(const UA_SecurityPolicy *policy,
                                     void *channelContext,
                                     const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_mbedTLS_PsaKey_import(&cc->localSymEncryptingKey, PSA_KEY_TYPE_AES,
        PSA_KEY_USAGE_ENCRYPT, PSA_ALG_CBC_NO_PADDING, key);
}

static UA_StatusCode
EccBrainpoolP256r1_setLocalSymIv(const UA_SecurityPolicy *policy,
                          void *channelContext,
                          const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static size_t
UA_SymEn_EccBrainpoolP256r1_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    return UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_EccBrainpoolP256r1_getBlockSize(const UA_SecurityPolicy *policy,
                                  const void *channelContext) {
    return UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
UA_SymSig_EccBrainpoolP256r1_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    return UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
EccBrainpoolP256r1_setRemoteSymSigningKey(const UA_SecurityPolicy *policy,
                                   void *channelContext,
                                   const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_mbedTLS_PsaKey_import(&cc->remoteSymSigningKey, PSA_KEY_TYPE_HMAC,
        PSA_KEY_USAGE_VERIFY_MESSAGE, PSA_ALG_HMAC(PSA_ALG_SHA_256), key);
}

static UA_StatusCode
EccBrainpoolP256r1_setRemoteSymEncryptingKey(const UA_SecurityPolicy *policy,
                                      void *channelContext,
                                      const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_mbedTLS_PsaKey_import(&cc->remoteSymEncryptingKey, PSA_KEY_TYPE_AES,
        PSA_KEY_USAGE_DECRYPT, PSA_ALG_CBC_NO_PADDING, key);
}

static UA_StatusCode
EccBrainpoolP256r1_setRemoteSymIv(const UA_SecurityPolicy *policy,
                           void *channelContext, const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

static UA_StatusCode
UA_AsymSig_EccBrainpoolP256r1_sign(const UA_SecurityPolicy *policy,
                            void *channelContext, const UA_ByteString *message,
                            UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccBrainpoolP256r1 *pc =
        (Policy_Context_EccBrainpoolP256r1 *)policy->policyContext;
    return UA_mbedTLS_PsaAsymmetricSign(&pc->localPrivateKey,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256), PSA_ALG_SHA_256, message, signature);
}

static UA_StatusCode
UA_AsymSig_EccBrainpoolP256r1_verify(const UA_SecurityPolicy *policy, void *channelContext,
                              const UA_ByteString *message,
                              const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_mbedTLS_PsaAsymmetricVerify(&cc->remoteCertificateX509.pk,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256), PSA_ALG_SHA_256, message, signature);
}

static UA_StatusCode
UA_Asym_EccBrainpoolP256r1_Dummy(const UA_SecurityPolicy *policy,
                          void *channelContext, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD; /* Do nothing and return true */
}

static size_t
UA_SymSig_EccBrainpoolP256r1_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                             const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymSig_EccBrainpoolP256r1_verify(const UA_SecurityPolicy *policy, void *channelContext,
                             const UA_ByteString *message, const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_mbedTLS_PsaMacVerify(cc->remoteSymSigningKey.id,
        PSA_ALG_HMAC(PSA_ALG_SHA_256), message, signature);
}

static UA_StatusCode
UA_SymSig_EccBrainpoolP256r1_sign(const UA_SecurityPolicy *policy,
                           void *channelContext, const UA_ByteString *message,
                           UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    signature->length = UA_SHA256_LENGTH;
    return UA_mbedTLS_PsaMacCompute(cc->localSymSigningKey.id,
        PSA_ALG_HMAC(PSA_ALG_SHA_256), message, signature);
}

static size_t
UA_SymSig_EccBrainpoolP256r1_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymEn_EccBrainpoolP256r1_decrypt(const UA_SecurityPolicy *policy,
                             void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;

    if(cc->remoteSymIv.length != UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_BLOCK_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(data->length % UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_BLOCK_SIZE != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_mbedTLS_PsaCipher(cc->remoteSymEncryptingKey.id,
                                   PSA_ALG_CBC_NO_PADDING, false,
                                   &cc->remoteSymIv, data);
}

static UA_StatusCode
UA_SymEn_EccBrainpoolP256r1_encrypt(const UA_SecurityPolicy *policy,
                             void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc = (Channel_Context_EccBrainpoolP256r1 *)channelContext;

    if(cc->localSymIv.length != UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_BLOCK_SIZE)
        return UA_STATUSCODE_BADINTERNALERROR;

    size_t plainTextBlockSize = UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_PLAIN_TEXT_BLOCK_SIZE;
    if(data->length % plainTextBlockSize != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_mbedTLS_PsaCipher(cc->localSymEncryptingKey.id,
                                   PSA_ALG_CBC_NO_PADDING, true,
                                   &cc->localSymIv, data);
}

static UA_StatusCode
EccBrainpoolP256r1_compareCertificate(const UA_SecurityPolicy *policy,
                               const void *channelContext,
                               const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const Channel_Context_EccBrainpoolP256r1 *cc =
        (const Channel_Context_EccBrainpoolP256r1 *)channelContext;

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
UA_AsymEn_EccBrainpoolP256r1_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    /* No ECC encryption -> key length set to 1 to avoid division or
     * multiplication with 0 */
    return 1;
}

UA_StatusCode
UA_SecurityPolicy_EccBrainpoolP256r1(UA_SecurityPolicy *sp,
                              const UA_ApplicationType applicationType,
                              const UA_ByteString localCertificate,
                              const UA_ByteString localPrivateKey,
                              const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                   "SecurityPolicy ECC_brainpoolP256r1 is deprecated (OPC UA Part 7); "
                   "use ECC_brainpoolP256r1_AesGcm or ECC_brainpoolP256r1_ChaChaPoly instead");
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1\0");
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(ECCBRAINPOOLP256R1APPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 10;
    sp->policyType = UA_SECURITYPOLICYTYPE_ECC;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING_NULL;
    asymSig->verify = UA_AsymSig_EccBrainpoolP256r1_verify;
    asymSig->sign = UA_AsymSig_EccBrainpoolP256r1_sign;
    asymSig->getLocalSignatureSize = UA_AsySig_EccBrainpoolP256r1_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = UA_Asym_EccBrainpoolP256r1_getRemoteSignatureSize;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2720\0");
    asymEnc->encrypt = UA_Asym_EccBrainpoolP256r1_Dummy;
    asymEnc->decrypt = UA_Asym_EccBrainpoolP256r1_Dummy;
    asymEnc->getLocalKeyLength = UA_AsymEn_EccBrainpoolP256r1_getLocalKeyLength;
    asymEnc->getRemoteKeyLength = UA_AsymEn_EccBrainpoolP256r1_getRemoteKeyLength;
    asymEnc->getRemoteBlockSize = UA_AsymEn_EccBrainpoolP256r1_getRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = UA_AsymEn_EccBrainpoolP256r1_getRemotePlainTextBlockSize;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256\0");
    symSig->verify = UA_SymSig_EccBrainpoolP256r1_verify;
    symSig->sign = UA_SymSig_EccBrainpoolP256r1_sign;
    symSig->getLocalSignatureSize = UA_SymSig_EccBrainpoolP256r1_getLocalSignatureSize;
    symSig->getRemoteSignatureSize = UA_SymSig_EccBrainpoolP256r1_getRemoteSignatureSize;
    symSig->getLocalKeyLength = UA_SymSig_EccBrainpoolP256r1_getLocalKeyLength;
    symSig->getRemoteKeyLength = UA_SymSig_EccBrainpoolP256r1_getRemoteKeyLength;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes128-cbc\0");
    symEnc->encrypt = UA_SymEn_EccBrainpoolP256r1_encrypt;
    symEnc->decrypt = UA_SymEn_EccBrainpoolP256r1_decrypt;
    symEnc->getLocalKeyLength = UA_SymEn_EccBrainpoolP256r1_getLocalKeyLength;
    symEnc->getRemoteKeyLength = UA_SymEn_EccBrainpoolP256r1_getRemoteKeyLength;
    symEnc->getRemoteBlockSize = UA_SymEn_EccBrainpoolP256r1_getBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_SymEn_EccBrainpoolP256r1_getBlockSize;

    /* Use the same signature algorithm as the asymmetric component for
     * certificate signing (see standard) */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = EccBrainpoolP256r1_New_Context;
    sp->deleteChannelContext = EccBrainpoolP256r1_Delete_Context;
    sp->setLocalSymEncryptingKey = EccBrainpoolP256r1_setLocalSymEncryptingKey;
    sp->setLocalSymSigningKey = EccBrainpoolP256r1_setLocalSymSigningKey;
    sp->setLocalSymIv = EccBrainpoolP256r1_setLocalSymIv;
    sp->setRemoteSymEncryptingKey = EccBrainpoolP256r1_setRemoteSymEncryptingKey;
    sp->setRemoteSymSigningKey = EccBrainpoolP256r1_setRemoteSymSigningKey;
    sp->setRemoteSymIv = EccBrainpoolP256r1_setRemoteSymIv;
    sp->compareCertificate = EccBrainpoolP256r1_compareCertificate;
    sp->generateKey = UA_Sym_EccBrainpoolP256r1_generateKey;
    sp->generateNonce = UA_Sym_EccBrainpoolP256r1_generateNonce;
    sp->nonceLength = UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_NONCE_LENGTH_BYTES;
    sp->makeCertThumbprint = UA_makeCertificateThumbprint_EccBrainpoolP256r1;
    sp->compareCertThumbprint = UA_compareCertificateThumbprint_EccBrainpoolP256r1;
    sp->updateCertificate = updateCertificateAndPrivateKey_sp_EccBrainpoolP256r1;
    sp->createSigningRequest = UA_mbedTLS_createSigningRequest_generic;
    sp->clear = UA_Policy_EccBrainpoolP256r1_Clear_Context;

    /* Parse the certificate */
    UA_StatusCode res =
        UA_mbedTLS_LoadLocalCertificate(&localCertificate, &sp->localCertificate);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = UA_Policy_EccBrainpoolP256r1_New_Context(sp, localPrivateKey, applicationType, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_ENCRYPTION_MBEDTLS */
