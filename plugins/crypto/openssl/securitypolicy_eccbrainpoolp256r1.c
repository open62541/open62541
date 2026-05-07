/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)

#include "securitypolicy_common.h"
#include <openssl/rand.h>

#define UA_SHA256_LENGTH 32 /* 256 bit */
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_ASYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_ASYM_SIGNATURE_LENGTH 64
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_KEY_LENGTH 16
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCBRAINPOOLP256R1_NONCE_LENGTH_BYTES 64

typedef struct {
    EVP_PKEY *localPrivateKey;
    EVP_PKEY *csrLocalPrivateKey;
    UA_ByteString localCertThumbprint;
    UA_ApplicationType applicationType;
} Policy_Context_EccBrainpoolP256r1;

typedef struct Channel_Context_EccBrainpoolP256r1 {
    EVP_PKEY *    localEphemeralKeyPair;
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    UA_ByteString remoteCertificate;
    X509 *remoteCertificateX509; /* X509 */
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

    context->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&localPrivateKey);
    if(!context->localPrivateKey) {
        UA_free(context);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    context->csrLocalPrivateKey = NULL;

    UA_StatusCode retval = UA_Openssl_X509_GetCertificateThumbprint(
        &securityPolicy->localCertificate, &context->localCertThumbprint, true);
    if(retval != UA_STATUSCODE_GOOD) {
        EVP_PKEY_free(context->localPrivateKey);
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

    EVP_PKEY_free(pc->localPrivateKey);
    EVP_PKEY_free(pc->csrLocalPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);
}

static UA_StatusCode
createSigningRequest_sp_eccbrainpoolp256r1(UA_SecurityPolicy *securityPolicy,
                                           const UA_String *subjectName,
                                           const UA_ByteString *nonce,
                                           const UA_KeyValueMap *params,
                                           UA_ByteString *csr,
                                           UA_ByteString *newPrivateKey) {
    if(!securityPolicy || !csr)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(!securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccBrainpoolP256r1 *pc =
        (Policy_Context_EccBrainpoolP256r1*)securityPolicy->policyContext;
    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey,
                                           &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName,
                                           nonce, csr, newPrivateKey);
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_EccBrainpoolP256r1(UA_SecurityPolicy *securityPolicy,
                                                     const UA_ByteString newCertificate,
                                                     const UA_ByteString newPrivateKey) {
    if(!securityPolicy || !securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_EccBrainpoolP256r1 *pc =
        (Policy_Context_EccBrainpoolP256r1 *)securityPolicy->policyContext;

    UA_ByteString_clear(&securityPolicy->localCertificate);
    UA_StatusCode retval = UA_OpenSSL_LoadLocalCertificate(
        &newCertificate, &securityPolicy->localCertificate,
        EVP_PKEY_EC);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    EVP_PKEY_free(pc->localPrivateKey);
    pc->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&newPrivateKey);
    if(!pc->localPrivateKey) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    UA_ByteString_clear(&pc->localCertThumbprint);
    retval = UA_Openssl_X509_GetCertificateThumbprint(&securityPolicy->localCertificate,
                                                      &pc->localCertThumbprint, true);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    if(securityPolicy->policyContext)
        UA_Policy_EccBrainpoolP256r1_Clear_Context(securityPolicy);
    return retval;
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
        UA_copyCertificate(&newContext->remoteCertificate, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newContext);
        return retval;
    }

    newContext->remoteCertificateX509 =
        UA_OpenSSL_LoadCertificate(&newContext->remoteCertificate, EVP_PKEY_EC);
    if(newContext->remoteCertificateX509 == NULL) {
        UA_ByteString_clear (&newContext->remoteCertificate);
        UA_free (newContext);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

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
    X509_free(cc->remoteCertificateX509);
    UA_ByteString_clear(&cc->remoteCertificate);
    UA_ByteString_clear(&cc->localSymSigningKey);
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    UA_ByteString_clear(&cc->localSymIv);
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    UA_ByteString_clear(&cc->remoteSymIv);
    EVP_PKEY_free(cc->localEphemeralKeyPair);
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
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
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
    return 1;
}

static UA_StatusCode
UA_Sym_EccBrainpoolP256r1_generateNonce(const UA_SecurityPolicy *policy,
                                        void *channelContext, UA_ByteString *out) {
    Policy_Context_EccBrainpoolP256r1 *pctx =
        (Policy_Context_EccBrainpoolP256r1*)policy->policyContext;
    if(!pctx)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    if(out->data[0] == 'e' && out->data[1] == 'p' && out->data[2] == 'h') {
        Channel_Context_EccBrainpoolP256r1 *cctx =
            (Channel_Context_EccBrainpoolP256r1*)channelContext;
        return UA_OpenSSL_ECC_BRAINPOOLP256R1_GenerateKey(&cctx->localEphemeralKeyPair, out);
    }

    UA_Int32 rc = RAND_bytes(out->data, (int)out->length);
    return (rc == 1) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADUNEXPECTEDERROR;
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
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    if(!pctx || !cc)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_OpenSSL_ECC_DeriveKeys(OBJ_txt2nid("brainpoolP256r1"), "SHA256",
                                     pctx->applicationType, cc->localEphemeralKeyPair,
                                     secret, seed, out);
}

static UA_StatusCode
EccBrainpoolP256r1_setLocalSymSigningKey(const UA_SecurityPolicy *policy,
                                         void *channelContext,
                                         const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
EccBrainpoolP256r1_setLocalSymEncryptingKey(const UA_SecurityPolicy *policy,
                                            void *channelContext,
                                            const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
EccBrainpoolP256r1_setLocalSymIv(const UA_SecurityPolicy *policy,
                                 void *channelContext,
                                 const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
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
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
EccBrainpoolP256r1_setRemoteSymEncryptingKey(const UA_SecurityPolicy *policy,
                                             void *channelContext,
                                             const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
EccBrainpoolP256r1_setRemoteSymIv(const UA_SecurityPolicy *policy,
                                  void *channelContext, const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
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
    return UA_Openssl_ECDSA_SHA256_Sign(message, pc->localPrivateKey, signature);
}

static UA_StatusCode
UA_AsymSig_EccBrainpoolP256r1_verify(const UA_SecurityPolicy *policy,
                                     void *channelContext,
                                     const UA_ByteString *message,
                                     const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_Openssl_ECDSA_SHA256_Verify(message, cc->remoteCertificateX509, signature);
}

static UA_StatusCode
UA_Asym_EccBrainpoolP256r1_Dummy(const UA_SecurityPolicy *policy,
                                 void *channelContext, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD;
}

static size_t
UA_SymSig_EccBrainpoolP256r1_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                                     const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymSig_EccBrainpoolP256r1_verify(const UA_SecurityPolicy *policy,
                                    void *channelContext,
                                    const UA_ByteString *message,
                                    const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Verify(message, &cc->remoteSymSigningKey, signature);
}

static UA_StatusCode
UA_SymSig_EccBrainpoolP256r1_sign(const UA_SecurityPolicy *policy,
                                  void *channelContext, const UA_ByteString *message,
                                  UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Sign(message, &cc->localSymSigningKey, signature);
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
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_OpenSSL_AES_128_CBC_Decrypt(&cc->remoteSymIv, &cc->remoteSymEncryptingKey,
                                          data);
}

static UA_StatusCode
UA_SymEn_EccBrainpoolP256r1_encrypt(const UA_SecurityPolicy *policy,
                                    void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccBrainpoolP256r1 *cc =
        (Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_OpenSSL_AES_128_CBC_Encrypt(&cc->localSymIv, &cc->localSymEncryptingKey,
                                          data);
}

static UA_StatusCode
EccBrainpoolP256r1_compareCertificate(const UA_SecurityPolicy *policy,
                                      const void *channelContext,
                                      const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const Channel_Context_EccBrainpoolP256r1 *cc =
        (const Channel_Context_EccBrainpoolP256r1 *)channelContext;
    return UA_OpenSSL_X509_compare(certificate, cc->remoteCertificateX509);
}

static size_t
UA_AsymEn_EccBrainpoolP256r1_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                                const void *channelContext) {
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
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1\0");
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(ECCBRAINPOOLP256R1APPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 10;
    sp->policyType = UA_SECURITYPOLICYTYPE_ECC;

    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING_NULL;
    asymSig->verify = UA_AsymSig_EccBrainpoolP256r1_verify;
    asymSig->sign = UA_AsymSig_EccBrainpoolP256r1_sign;
    asymSig->getLocalSignatureSize = UA_AsySig_EccBrainpoolP256r1_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = UA_Asym_EccBrainpoolP256r1_getRemoteSignatureSize;

    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2720\0");
    asymEnc->encrypt = UA_Asym_EccBrainpoolP256r1_Dummy;
    asymEnc->decrypt = UA_Asym_EccBrainpoolP256r1_Dummy;
    asymEnc->getLocalKeyLength = UA_AsymEn_EccBrainpoolP256r1_getLocalKeyLength;
    asymEnc->getRemoteKeyLength = UA_AsymEn_EccBrainpoolP256r1_getRemoteKeyLength;
    asymEnc->getRemoteBlockSize = UA_AsymEn_EccBrainpoolP256r1_getRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = UA_AsymEn_EccBrainpoolP256r1_getRemotePlainTextBlockSize;

    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256\0");
    symSig->verify = UA_SymSig_EccBrainpoolP256r1_verify;
    symSig->sign = UA_SymSig_EccBrainpoolP256r1_sign;
    symSig->getLocalSignatureSize = UA_SymSig_EccBrainpoolP256r1_getLocalSignatureSize;
    symSig->getRemoteSignatureSize = UA_SymSig_EccBrainpoolP256r1_getRemoteSignatureSize;
    symSig->getLocalKeyLength = UA_SymSig_EccBrainpoolP256r1_getLocalKeyLength;
    symSig->getRemoteKeyLength = UA_SymSig_EccBrainpoolP256r1_getRemoteKeyLength;

    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes128-cbc\0");
    symEnc->encrypt = UA_SymEn_EccBrainpoolP256r1_encrypt;
    symEnc->decrypt = UA_SymEn_EccBrainpoolP256r1_decrypt;
    symEnc->getLocalKeyLength = UA_SymEn_EccBrainpoolP256r1_getLocalKeyLength;
    symEnc->getRemoteKeyLength = UA_SymEn_EccBrainpoolP256r1_getRemoteKeyLength;
    symEnc->getRemoteBlockSize = UA_SymEn_EccBrainpoolP256r1_getBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_SymEn_EccBrainpoolP256r1_getBlockSize;

    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

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
    sp->createSigningRequest = createSigningRequest_sp_eccbrainpoolp256r1;
    sp->clear = UA_Policy_EccBrainpoolP256r1_Clear_Context;

    UA_Openssl_Init();
    UA_StatusCode res =
        UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate,
                                        EVP_PKEY_EC);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = UA_Policy_EccBrainpoolP256r1_New_Context(sp, localPrivateKey,
                                                    applicationType, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

#endif
