/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Siemens AG (Authors: Tin Raic, Thomas Zeschg)
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)

#include "securitypolicy_common.h"
#include <openssl/rand.h>

#define UA_SHA256_LENGTH 32 /* 256 bit */
#define UA_SECURITYPOLICY_ECCNISTP256_ASYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_ECCNISTP256_ASYM_SIGNATURE_LENGTH 64
#define UA_SECURITYPOLICY_ECCNISTP256_SYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_ECCNISTP256_SYM_ENCRYPTION_KEY_LENGTH 16
#define UA_SECURITYPOLICY_ECCNISTP256_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCNISTP256_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCNISTP256_NONCE_LENGTH_BYTES 64

typedef struct {
    EVP_PKEY *localPrivateKey;
    EVP_PKEY *csrLocalPrivateKey;
    UA_ByteString localCertThumbprint;
    UA_ApplicationType applicationType;
} Policy_Context_EccNistP256;

typedef struct Channel_Context_EccNistP256 {
    EVP_PKEY *    localEphemeralKeyPair;
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    UA_ByteString remoteCertificate;
    X509 *remoteCertificateX509; /* X509 */
} Channel_Context_EccNistP256;

static UA_StatusCode
UA_Policy_EccNistP256_New_Context(UA_SecurityPolicy *securityPolicy,
                                  const UA_ByteString localPrivateKey,
                                  const UA_ApplicationType applicationType,
                                  const UA_Logger *logger) {
    Policy_Context_EccNistP256 *context = (Policy_Context_EccNistP256 *)
        UA_malloc(sizeof(Policy_Context_EccNistP256));
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
UA_Policy_EccNistP256_Clear_Context(UA_SecurityPolicy *policy) {
    if(!policy)
        return;

    UA_ByteString_clear(&policy->localCertificate);

    /* Delete all allocated members in the context */
    Policy_Context_EccNistP256 *pc =
        (Policy_Context_EccNistP256 *)policy->policyContext;
    if(!pc)
        return;

    EVP_PKEY_free(pc->localPrivateKey);
    EVP_PKEY_free(pc->csrLocalPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);
}

static UA_StatusCode
createSigningRequest_sp_eccnistp256(UA_SecurityPolicy *securityPolicy,
                                    const UA_String *subjectName,
                                    const UA_ByteString *nonce,
                                    const UA_KeyValueMap *params,
                                    UA_ByteString *csr,
                                    UA_ByteString *newPrivateKey) {
    if(!securityPolicy || !csr)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
     if(!securityPolicy->policyContext)
         return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccNistP256 *pc =
        (Policy_Context_EccNistP256*)securityPolicy->policyContext;
    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey,
                                           &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName,
                                           nonce, csr, newPrivateKey);
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_EccNistP256(UA_SecurityPolicy *securityPolicy,
                                              const UA_ByteString newCertificate,
                                              const UA_ByteString newPrivateKey) {
    if(!securityPolicy || !securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_EccNistP256 *pc =
        (Policy_Context_EccNistP256 *)securityPolicy->policyContext;

    /* Set the certificate */
    UA_ByteString_clear(&securityPolicy->localCertificate);
    UA_StatusCode retval = UA_OpenSSL_LoadLocalCertificate(
        &newCertificate, &securityPolicy->localCertificate,
        EVP_PKEY_EC);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the new private key */
    EVP_PKEY_free(pc->localPrivateKey);
    pc->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&newPrivateKey);
    if(!pc->localPrivateKey) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Update the thumbprint */
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
        UA_Policy_EccNistP256_Clear_Context(securityPolicy);
    return retval;
}

static UA_StatusCode
EccNistP256_New_Context(const UA_SecurityPolicy *securityPolicy,
                                         const UA_ByteString *remoteCertificate,
                                         void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccNistP256 *newContext = (Channel_Context_EccNistP256 *)
        UA_calloc(1, sizeof(Channel_Context_EccNistP256));
    if(!newContext)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval =
        UA_copyCertificate(&newContext->remoteCertificate, remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newContext);
        return retval;
    }

    /* Decode to X509 */
    newContext->remoteCertificateX509 =
        UA_OpenSSL_LoadCertificate(&newContext->remoteCertificate, EVP_PKEY_EC);
    if(newContext->remoteCertificateX509 == NULL) {
        UA_ByteString_clear (&newContext->remoteCertificate);
        UA_free (newContext);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

    /* Return the new channel context */
    *channelContext = newContext;

    return UA_STATUSCODE_GOOD;
}

static void
EccNistP256_Delete_Context(const UA_SecurityPolicy *policy,
                           void *channelContext) {
    if(!channelContext)
        return;
    Channel_Context_EccNistP256 *cc =
        (Channel_Context_EccNistP256 *)channelContext;
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
UA_compareCertificateThumbprint_EccNistP256(const UA_SecurityPolicy *policy,
                                            const UA_ByteString *certificateThumbprint) {
    if(policy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Policy_Context_EccNistP256 *pc =
        (Policy_Context_EccNistP256 *)policy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_makeCertificateThumbprint_EccNistP256(const UA_SecurityPolicy *securityPolicy,
                                         const UA_ByteString *certificate,
                                         UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

static size_t
UA_Asym_EccNistP256_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    /* According to the standard, the server and the client must agree on the
     * security mode and security policy. Therefore, it can be assumed that
     * the remote asymmetric cryptographic tokens have the same sizes as the
     * local ones. */
    return UA_SECURITYPOLICY_ECCNISTP256_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsySig_EccNistP256_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP256_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsymEn_EccNistP256_getRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                                  const void *channelContext) {
    return 1;
}

static size_t
UA_AsymEn_EccNistP256_getRemoteBlockSize(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    return 1;
}

static size_t
UA_AsymEn_EccNistP256_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    /* According to the standard, the server and the client must agree on the
     * security mode and security policy. Therefore, it can be assumed that the
     * remote asymmetric cryptographic tokens have the same sizes as the local
     * ones.
     *
     * No ECC encryption -> key length set to 1 to avoid division or
     * multiplication with 0 */
    return 1;
}

static UA_StatusCode
UA_Sym_EccNistP256_generateNonce(const UA_SecurityPolicy *policy,
                                 void *channelContext, UA_ByteString *out) {
    Policy_Context_EccNistP256* pctx = (Policy_Context_EccNistP256*)policy->policyContext;
    if(!pctx)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    /* Detect if we want to create an ephemeral key or just cryptographic random
     * data */
    if(out->data[0] == 'e' && out->data[1] == 'p' && out->data[2] == 'h') {
        Channel_Context_EccNistP256 *cctx = (Channel_Context_EccNistP256*)channelContext;
        return UA_OpenSSL_ECC_NISTP256_GenerateKey(&cctx->localEphemeralKeyPair, out);
    }

    UA_Int32 rc = RAND_bytes(out->data, (int)out->length);
    return (rc == 1) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADUNEXPECTEDERROR;
}

static size_t
UA_SymEn_EccNistP256_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                       const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_ECCNISTP256_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymSig_EccNistP256_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_ECCNISTP256_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_Sym_EccNistP256_generateKey(const UA_SecurityPolicy *policy,
                               void *channelContext, const UA_ByteString *secret,
                               const UA_ByteString *seed, UA_ByteString *out) {
    Policy_Context_EccNistP256* pctx = (Policy_Context_EccNistP256 *)policy->policyContext;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    if(!pctx || !cc)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_OpenSSL_ECC_DeriveKeys(EC_curve_nist2nid("P-256"), "SHA256",
                                     pctx->applicationType, cc->localEphemeralKeyPair,
                                     secret, seed, out);
}

static UA_StatusCode
EccNistP256_setLocalSymSigningKey(const UA_SecurityPolicy *policy,
                                                   void *channelContext,
                                                   const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
EccNistP256_setLocalSymEncryptingKey(const UA_SecurityPolicy *policy,
                                     void *channelContext,
                                     const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
EccNistP256_setLocalSymIv(const UA_SecurityPolicy *policy,
                          void *channelContext,
                          const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static size_t
UA_SymEn_EccNistP256_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_ECCNISTP256_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_EccNistP256_getBlockSize(const UA_SecurityPolicy *policy,
                                  const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP256_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
UA_SymSig_EccNistP256_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_ECCNISTP256_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
EccNistP256_setRemoteSymSigningKey(const UA_SecurityPolicy *policy,
                                   void *channelContext,
                                   const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
EccNistP256_setRemoteSymEncryptingKey(const UA_SecurityPolicy *policy,
                                      void *channelContext,
                                      const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
EccNistP256_setRemoteSymIv(const UA_SecurityPolicy *policy,
                           void *channelContext, const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

static UA_StatusCode
UA_AsymSig_EccNistP256_sign(const UA_SecurityPolicy *policy,
                            void *channelContext, const UA_ByteString *message,
                            UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Policy_Context_EccNistP256 *pc =
        (Policy_Context_EccNistP256 *)policy->policyContext;
    return UA_Openssl_ECDSA_SHA256_Sign(message, pc->localPrivateKey, signature);
}


static UA_StatusCode
UA_AsymSig_EccNistP256_verify(const UA_SecurityPolicy *policy, void *channelContext,
                              const UA_ByteString *message,
                              const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    return UA_Openssl_ECDSA_SHA256_Verify(message, cc->remoteCertificateX509, signature);
}

static UA_StatusCode
UA_Asym_EccNistP256_Dummy(const UA_SecurityPolicy *policy,
                          void *channelContext, UA_ByteString *data) {
    return UA_STATUSCODE_GOOD; /* Do nothing and return true */
}

static size_t
UA_SymSig_EccNistP256_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                             const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymSig_EccNistP256_verify(const UA_SecurityPolicy *policy, void *channelContext,
                             const UA_ByteString *message, const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Verify(message, &cc->remoteSymSigningKey, signature);
}

static UA_StatusCode
UA_SymSig_EccNistP256_sign(const UA_SecurityPolicy *policy,
                           void *channelContext, const UA_ByteString *message,
                           UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Sign(message, &cc->localSymSigningKey, signature);
}

static size_t
UA_SymSig_EccNistP256_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymEn_EccNistP256_decrypt(const UA_SecurityPolicy *policy,
                             void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    return UA_OpenSSL_AES_128_CBC_Decrypt(&cc->remoteSymIv, &cc->remoteSymEncryptingKey,
                                          data);
}

static UA_StatusCode
UA_SymEn_EccNistP256_encrypt(const UA_SecurityPolicy *policy,
                             void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP256 *cc = (Channel_Context_EccNistP256 *)channelContext;
    return UA_OpenSSL_AES_128_CBC_Encrypt(&cc->localSymIv, &cc->localSymEncryptingKey,
                                          data);
}

static UA_StatusCode
EccNistP256_compareCertificate(const UA_SecurityPolicy *policy,
                               const void *channelContext,
                               const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const Channel_Context_EccNistP256 *cc =
        (const Channel_Context_EccNistP256 *)channelContext;
    return UA_OpenSSL_X509_compare(certificate, cc->remoteCertificateX509);
}

static size_t
UA_AsymEn_EccNistP256_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                        const void *channelContext) {
    /* No ECC encryption -> key length set to 1 to avoid division or
     * multiplication with 0 */
    return 1;
}

UA_StatusCode
UA_SecurityPolicy_EccNistP256(UA_SecurityPolicy *sp,
                              const UA_ApplicationType applicationType,
                              const UA_ByteString localCertificate,
                              const UA_ByteString localPrivateKey,
                              const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256\0");
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(ECCNISTP256APPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 10;
    sp->policyType = UA_SECURITYPOLICYTYPE_ECC;

    /* Asymmetric Signature
     * TODO: The Softing SDK works when we send an empty string. It does not
     * work with either of the following:
     * asySigAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmldsig-more#ecdsa-sha256\0");
     * asySigAlgorithm->uri = UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2473\0"); */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING_NULL;
    asymSig->verify = UA_AsymSig_EccNistP256_verify;
    asymSig->sign = UA_AsymSig_EccNistP256_sign;
    asymSig->getLocalSignatureSize = UA_AsySig_EccNistP256_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = UA_Asym_EccNistP256_getRemoteSignatureSize;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2720\0");
    asymEnc->encrypt = UA_Asym_EccNistP256_Dummy;
    asymEnc->decrypt = UA_Asym_EccNistP256_Dummy;
    asymEnc->getLocalKeyLength = UA_AsymEn_EccNistP256_getLocalKeyLength;
    asymEnc->getRemoteKeyLength = UA_AsymEn_EccNistP256_getRemoteKeyLength;
    asymEnc->getRemoteBlockSize = UA_AsymEn_EccNistP256_getRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = UA_AsymEn_EccNistP256_getRemotePlainTextBlockSize;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256\0");
    symSig->verify = UA_SymSig_EccNistP256_verify;
    symSig->sign = UA_SymSig_EccNistP256_sign;
    symSig->getLocalSignatureSize = UA_SymSig_EccNistP256_getLocalSignatureSize;
    symSig->getRemoteSignatureSize = UA_SymSig_EccNistP256_getRemoteSignatureSize;
    symSig->getLocalKeyLength = UA_SymSig_EccNistP256_getLocalKeyLength;
    symSig->getRemoteKeyLength = UA_SymSig_EccNistP256_getRemoteKeyLength;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes128-cbc\0");
    symEnc->encrypt = UA_SymEn_EccNistP256_encrypt;
    symEnc->decrypt = UA_SymEn_EccNistP256_decrypt;
    symEnc->getLocalKeyLength = UA_SymEn_EccNistP256_getLocalKeyLength;
    symEnc->getRemoteKeyLength = UA_SymEn_EccNistP256_getRemoteKeyLength;
    symEnc->getRemoteBlockSize = UA_SymEn_EccNistP256_getBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_SymEn_EccNistP256_getBlockSize;

    /* Use the same signature algorithm as the asymmetric component for
     * certificate signing (see standard) */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = EccNistP256_New_Context;
    sp->deleteChannelContext = EccNistP256_Delete_Context;
    sp->setLocalSymEncryptingKey = EccNistP256_setLocalSymEncryptingKey;
    sp->setLocalSymSigningKey = EccNistP256_setLocalSymSigningKey;
    sp->setLocalSymIv = EccNistP256_setLocalSymIv;
    sp->setRemoteSymEncryptingKey = EccNistP256_setRemoteSymEncryptingKey;
    sp->setRemoteSymSigningKey = EccNistP256_setRemoteSymSigningKey;
    sp->setRemoteSymIv = EccNistP256_setRemoteSymIv;
    sp->compareCertificate = EccNistP256_compareCertificate;
    sp->generateKey = UA_Sym_EccNistP256_generateKey;
    sp->generateNonce = UA_Sym_EccNistP256_generateNonce;
    sp->nonceLength = UA_SECURITYPOLICY_ECCNISTP256_NONCE_LENGTH_BYTES;
    sp->makeCertThumbprint = UA_makeCertificateThumbprint_EccNistP256;
    sp->compareCertThumbprint = UA_compareCertificateThumbprint_EccNistP256;
    sp->updateCertificate = updateCertificateAndPrivateKey_sp_EccNistP256;
    sp->createSigningRequest = createSigningRequest_sp_eccnistp256;
    sp->clear = UA_Policy_EccNistP256_Clear_Context;

    /* Parse the certificate */
    UA_Openssl_Init();
    UA_StatusCode res =
        UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate,
                                        EVP_PKEY_EC);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = UA_Policy_EccNistP256_New_Context(sp, localPrivateKey, applicationType, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

#endif
