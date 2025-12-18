/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include "securitypolicy_common.h"

#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

#define UA_SHA1_LENGTH                                               20
#define UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN               11
#define UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_KEY_LENGTH    16
#define UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_BLOCK_SIZE    16
#define UA_SECURITYPOLICY_BASIC128RSA15_SYM_SIGNING_KEY_LENGTH       16
#define UA_SHA1_LENGTH                                               20

typedef struct {
    EVP_PKEY *localPrivateKey;
    EVP_PKEY *csrLocalPrivateKey;
    UA_ByteString localCertThumbprint;
} Policy_Context_Basic128Rsa15;

typedef struct {
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    UA_ByteString remoteCertificate;
    X509 *remoteCertificateX509;
} Channel_Context_Basic128Rsa15;

static UA_StatusCode
UA_Policy_Basic128Rsa15_New_Context(UA_SecurityPolicy * securityPolicy,
                                    const UA_ByteString localPrivateKey,
                                    const UA_Logger *   logger) {
    Policy_Context_Basic128Rsa15 * context = (Policy_Context_Basic128Rsa15 *)
                                    UA_malloc(sizeof(Policy_Context_Basic128Rsa15));
    if(context == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    context->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&localPrivateKey);
    if(!context->localPrivateKey) {
        UA_free(context);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    context->csrLocalPrivateKey = NULL;

    UA_StatusCode retval =
        UA_Openssl_X509_GetCertificateThumbprint(&securityPolicy->localCertificate,
                                                 &context->localCertThumbprint, true);
    if(retval != UA_STATUSCODE_GOOD) {
        EVP_PKEY_free(context->localPrivateKey);
        UA_free(context);
        return retval;
    }

    securityPolicy->policyContext = context;

    return UA_STATUSCODE_GOOD;
}

static void
UA_Policy_Basic128Rsa15_Clear_Context(UA_SecurityPolicy *policy) {
    if(policy == NULL) {
        return;
    }
    UA_ByteString_clear(&policy->localCertificate);

    Policy_Context_Basic128Rsa15 *ctx = (Policy_Context_Basic128Rsa15 *)
        policy->policyContext;
    if(ctx == NULL)
        return;

    /* delete all allocated members in the context */

    EVP_PKEY_free(ctx->localPrivateKey);
    EVP_PKEY_free(ctx->csrLocalPrivateKey);
    UA_ByteString_clear(&ctx->localCertThumbprint);
    UA_free(ctx);
}

static UA_StatusCode
updateCertificate_sp_basic128rsa15(UA_SecurityPolicy *securityPolicy,
                                   const UA_ByteString newCertificate,
                                   const UA_ByteString newPrivateKey) {
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_Basic128Rsa15 *pc =
        (Policy_Context_Basic128Rsa15 *)securityPolicy->policyContext;

    UA_Boolean isLocalKey = false;
    if(newPrivateKey.length <= 0) {
        if(UA_CertificateUtils_comparePublicKeys(&newCertificate, &securityPolicy->localCertificate) == 0)
            isLocalKey = true;
    }

    UA_ByteString_clear(&securityPolicy->localCertificate);

    UA_StatusCode retval = UA_OpenSSL_LoadLocalCertificate(
        &newCertificate, &securityPolicy->localCertificate);

    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the new private key */
    if(newPrivateKey.length > 0) {
        EVP_PKEY_free(pc->localPrivateKey);
        pc->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&newPrivateKey);
    } else {
        if(!isLocalKey) {
            EVP_PKEY_free(pc->localPrivateKey);
            pc->localPrivateKey = pc->csrLocalPrivateKey;
            pc->csrLocalPrivateKey = NULL;
        }
    }

    if(!pc->localPrivateKey) {
        retval = UA_STATUSCODE_BADNOTSUPPORTED;
        goto error;
    }

    UA_ByteString_clear(&pc->localCertThumbprint);

    retval = UA_Openssl_X509_GetCertificateThumbprint(&securityPolicy->localCertificate,
                                                      &pc->localCertThumbprint, true);
    if(retval != UA_STATUSCODE_GOOD) {
        goto error;
    }

    return retval;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    if(securityPolicy->policyContext != NULL)
        UA_Policy_Basic128Rsa15_Clear_Context(securityPolicy);
    return retval;
}

static UA_StatusCode
createSigningRequest_sp_basic128rsa15(UA_SecurityPolicy *securityPolicy,
                                      const UA_String *subjectName,
                                      const UA_ByteString *nonce,
                                      const UA_KeyValueMap *params,
                                      UA_ByteString *csr,
                                      UA_ByteString *newPrivateKey) {
    /* Check parameter */
    if(securityPolicy == NULL || csr == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_Basic128Rsa15 *pc =
            (Policy_Context_Basic128Rsa15 *) securityPolicy->policyContext;

    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey, &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName, nonce,
                                           csr, newPrivateKey);
}

/* create the channel context */

static UA_StatusCode
Basic128Rsa15_New_Context(const UA_SecurityPolicy *securityPolicy,
                          const UA_ByteString *remoteCertificate,
                          void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Basic128Rsa15 * context = (Channel_Context_Basic128Rsa15 *)
            UA_malloc(sizeof(Channel_Context_Basic128Rsa15));
    if(context == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_ByteString_init(&context->localSymSigningKey);
    UA_ByteString_init(&context->localSymEncryptingKey);
    UA_ByteString_init(&context->localSymIv);
    UA_ByteString_init(&context->remoteSymSigningKey);
    UA_ByteString_init(&context->remoteSymEncryptingKey);
    UA_ByteString_init(&context->remoteSymIv);

    UA_StatusCode retval = UA_copyCertificate(&context->remoteCertificate,
                                              remoteCertificate);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(context);
        return retval;
    }

    /* decode to X509 */
    context->remoteCertificateX509 =
        UA_OpenSSL_LoadCertificate(&context->remoteCertificate);
    if(context->remoteCertificateX509 == NULL) {
        UA_ByteString_clear(&context->remoteCertificate);
        UA_free(context);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

    *channelContext = context;

    UA_LOG_INFO(securityPolicy->logger,
                UA_LOGCATEGORY_SECURITYPOLICY,
                "The Basic128Rsa15 security policy channel with openssl is created.");

    return UA_STATUSCODE_GOOD;
}

static void
Basic128Rsa15_Delete_Context(const UA_SecurityPolicy *policy,
                             void *channelContext) {
    if(channelContext == NULL)
        return;

    Channel_Context_Basic128Rsa15 *cc =
        (Channel_Context_Basic128Rsa15 *)channelContext;
    X509_free(cc->remoteCertificateX509);
    UA_ByteString_clear(&cc->remoteCertificate);
    UA_ByteString_clear(&cc->localSymSigningKey);
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    UA_ByteString_clear(&cc->localSymIv);
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    UA_ByteString_clear(&cc->remoteSymIv);
    UA_LOG_INFO(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                "The Basic128Rsa15 security policy channel with openssl is deleted.");
    UA_free(cc);
}

static UA_StatusCode
Basic128Rsa15_setLocalSymSigningKey(const UA_SecurityPolicy *policy,
                                    void *channelContext,
                                    const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
Basic128Rsa15_setLocalSymEncryptingKey(const UA_SecurityPolicy *policy,
                                       void *channelContext,
                                       const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
Basic128Rsa15_setLocalSymIv(const UA_SecurityPolicy *policy,
                            void *channelContext,
                            const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static UA_StatusCode
Basic128Rsa15_setRemoteSymSigningKey(const UA_SecurityPolicy *policy,
                                     void *channelContext,
                                     const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
Basic128Rsa15_setRemoteSymEncryptingKey(const UA_SecurityPolicy *policy,
                                        void *channelContext,
                                        const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
Basic128Rsa15_setRemoteSymIv(const UA_SecurityPolicy *policy,
                             void *channelContext,
                             const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(iv, &cc->remoteSymIv);
}

static UA_StatusCode
Basic128Rsa15_compareCertificate(const UA_SecurityPolicy *policy,
                                 const void *channelContext,
                                 const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const Channel_Context_Basic128Rsa15 * cc =
        (const Channel_Context_Basic128Rsa15 *) channelContext;
    return UA_OpenSSL_X509_compare(certificate, cc->remoteCertificateX509);
}

static UA_StatusCode
UA_Asy_Basic128Rsa15_compareCertThumbprint(const UA_SecurityPolicy *securityPolicy,
                                           const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Policy_Context_Basic128Rsa15 *pc = (Policy_Context_Basic128Rsa15 *)
                                       securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint)) {
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Asy_Basic128Rsa15_makeCertificateThumbprint(const UA_SecurityPolicy *securityPolicy,
                                               const UA_ByteString *certificate,
                                               UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

static size_t
UA_AsySig_Basic128Rsa15_getRemoteSignatureSize (const UA_SecurityPolicy *policy,
                                                const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const Channel_Context_Basic128Rsa15 *cc =
        (const Channel_Context_Basic128Rsa15 *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen;
}

static size_t
UA_AsySig_Basic128Rsa15_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Policy_Context_Basic128Rsa15 *pc =
        (Policy_Context_Basic128Rsa15 *)policy->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t) keyLen;
}

static UA_StatusCode
UA_AsySig_Basic128Rsa15_Verify(const UA_SecurityPolicy *policy, void *channelContext,
                               const UA_ByteString *message,
                               const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    return UA_OpenSSL_RSA_PKCS1_V15_SHA1_Verify(message, cc->remoteCertificateX509,
                                                signature);
}

static UA_StatusCode
UA_AsySig_Basic128Rsa15_Sign(const UA_SecurityPolicy *policy, void *channelContext,
                             const UA_ByteString *message, UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const Policy_Context_Basic128Rsa15 *pc =
        (const Policy_Context_Basic128Rsa15*) policy->policyContext;
    return UA_Openssl_RSA_PKCS1_V15_SHA1_Sign(message, pc->localPrivateKey, signature);
}

static size_t
UA_AsymEn_Basic128Rsa15_getRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                                    const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const Channel_Context_Basic128Rsa15 *cc =
        (const Channel_Context_Basic128Rsa15 *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen - UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN;
}

static size_t
UA_AsymEn_Basic128Rsa15_getRemoteBlockSize(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const Channel_Context_Basic128Rsa15 *cc =
        (const Channel_Context_Basic128Rsa15 *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen;
}

static size_t
UA_AsymEn_Basic128Rsa15_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const Channel_Context_Basic128Rsa15 *cc =
        (const Channel_Context_Basic128Rsa15 *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen * 8;
}

static size_t
UA_AsymEn_Basic128Rsa15_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                          const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const Policy_Context_Basic128Rsa15 *pc =
        (const Policy_Context_Basic128Rsa15 *)policy->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t) keyLen * 8;
}

static UA_StatusCode
UA_AsymEn_Basic128Rsa15_Decrypt(const UA_SecurityPolicy *policy,
                                void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const Policy_Context_Basic128Rsa15 *pc =
        (const Policy_Context_Basic128Rsa15 *)policy->policyContext;
    return UA_Openssl_RSA_PKCS1_V15_Decrypt(data, pc->localPrivateKey);
}

static UA_StatusCode
UA_AsymEn_Basic128Rsa15_Encrypt(const UA_SecurityPolicy *policy,
                                void *channelContext, UA_ByteString *data) {
    if (channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    return UA_Openssl_RSA_PKCS1_V15_Encrypt(data,
                                            UA_SECURITYPOLICY_BASIC128RSA15_RSAPADDING_LEN,
                                            cc->remoteCertificateX509);
}

static UA_StatusCode
UA_Sym_Basic128Rsa15_generateNonce(const UA_SecurityPolicy *policy,
                                   void *channelContext, UA_ByteString *out) {
    UA_Int32 rc = RAND_bytes(out->data, (int) out->length);
    if(rc != 1)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Sym_Basic128Rsa15_generateKey(const UA_SecurityPolicy *policy,
                                 void *channelContext, const UA_ByteString *secret,
                                 const UA_ByteString *seed, UA_ByteString *out) {
    return UA_Openssl_Random_Key_PSHA1_Derive(secret, seed, out);
}

static size_t
UA_SymEn_Basic128Rsa15_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    /* 16 bytes 128 bits */
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_Basic128Rsa15_getBlockSize(const UA_SecurityPolicy *policy,
                                    const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
UA_SymEn_Basic128Rsa15_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                          const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_ENCRYPTION_KEY_LENGTH;
}

static UA_StatusCode
UA_SymEn_Basic128Rsa15_Encrypt(const UA_SecurityPolicy *policy,
                               void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    return UA_OpenSSL_AES_128_CBC_Encrypt(&cc->localSymIv,
                                          &cc->localSymEncryptingKey, data);
}

static UA_StatusCode
UA_SymEn_Basic128Rsa15_Decrypt(const UA_SecurityPolicy *policy,
                               void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15*)channelContext;
    return UA_OpenSSL_AES_128_CBC_Decrypt(&cc->remoteSymIv,
                                          &cc->remoteSymEncryptingKey, data);
}

static size_t
UA_SymSig_Basic128Rsa15_getKeyLength(const UA_SecurityPolicy *policy,
                                     const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC128RSA15_SYM_SIGNING_KEY_LENGTH;
}

static size_t
UA_SymSig_Basic128Rsa15_getSignatureSize(const UA_SecurityPolicy *policy,
                                    const void *channelContext) {
    return UA_SHA1_LENGTH;
}

static UA_StatusCode
UA_SymSig_Basic128Rsa15_Verify(const UA_SecurityPolicy *policy, void *channelContext,
                               const UA_ByteString *message,
                               const UA_ByteString *signature) {
    if (channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    return UA_OpenSSL_HMAC_SHA1_Verify(message, &cc->remoteSymSigningKey, signature);
}

static UA_StatusCode
UA_SymSig_Basic128Rsa15_Sign(const UA_SecurityPolicy *policy,
                          void *channelContext, const UA_ByteString *message,
                          UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_Basic128Rsa15 * cc = (Channel_Context_Basic128Rsa15 *) channelContext;
    return UA_OpenSSL_HMAC_SHA1_Sign(message, &cc->localSymSigningKey, signature);
}

UA_StatusCode
UA_SecurityPolicy_Basic128Rsa15(UA_SecurityPolicy *sp,
                                const UA_ByteString localCertificate,
                                const UA_ByteString localPrivateKey,
                                const UA_Logger *logger) {
    UA_LOG_WARNING(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                   "!! WARNING !! The Basic128Rsa15 SecurityPolicy is unsecure. "
                   "There are known attacks that break the encryption.");

    UA_LOG_INFO(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                "The Basic128Rsa15 security policy with openssl is added.");
    
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15\0");
    sp->certificateGroupId = UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(RSAMINAPPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 0;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#rsa-sha1\0");
    asymSig->verify = UA_AsySig_Basic128Rsa15_Verify;
    asymSig->sign = UA_AsySig_Basic128Rsa15_Sign;
    asymSig->getLocalSignatureSize = UA_AsySig_Basic128Rsa15_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = UA_AsySig_Basic128Rsa15_getRemoteSignatureSize;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#rsa-1_5\0");
    asymEnc->encrypt = UA_AsymEn_Basic128Rsa15_Encrypt;
    asymEnc->decrypt = UA_AsymEn_Basic128Rsa15_Decrypt;
    asymEnc->getLocalKeyLength = UA_AsymEn_Basic128Rsa15_getLocalKeyLength;
    asymEnc->getRemoteKeyLength = UA_AsymEn_Basic128Rsa15_getRemoteKeyLength;
    asymEnc->getRemoteBlockSize = UA_AsymEn_Basic128Rsa15_getRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = UA_AsymEn_Basic128Rsa15_getRemotePlainTextBlockSize;

    /* Symmetric signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha1\0");
    symSig->verify = UA_SymSig_Basic128Rsa15_Verify;
    symSig->sign = UA_SymSig_Basic128Rsa15_Sign;
    symSig->getLocalSignatureSize = UA_SymSig_Basic128Rsa15_getSignatureSize;
    symSig->getRemoteSignatureSize = UA_SymSig_Basic128Rsa15_getSignatureSize;
    symSig->getLocalKeyLength = UA_SymSig_Basic128Rsa15_getKeyLength;
    symSig->getRemoteKeyLength = UA_SymSig_Basic128Rsa15_getKeyLength;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes128-cbc\0");
    symEnc->encrypt = UA_SymEn_Basic128Rsa15_Encrypt;
    symEnc->decrypt = UA_SymEn_Basic128Rsa15_Decrypt;
    symEnc->getLocalKeyLength = UA_SymEn_Basic128Rsa15_getLocalKeyLength;
    symEnc->getRemoteKeyLength = UA_SymEn_Basic128Rsa15_getRemoteKeyLength;
    symEnc->getRemoteBlockSize = UA_SymEn_Basic128Rsa15_getBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_SymEn_Basic128Rsa15_getBlockSize;

    /* Use the same signature algorithm as the asymmetric component for
     * certificate signing (see standard) */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = Basic128Rsa15_New_Context;
    sp->deleteChannelContext = Basic128Rsa15_Delete_Context;
    sp->setLocalSymEncryptingKey = Basic128Rsa15_setLocalSymEncryptingKey;
    sp->setLocalSymSigningKey = Basic128Rsa15_setLocalSymSigningKey;
    sp->setLocalSymIv = Basic128Rsa15_setLocalSymIv;
    sp->setRemoteSymEncryptingKey = Basic128Rsa15_setRemoteSymEncryptingKey;
    sp->setRemoteSymSigningKey = Basic128Rsa15_setRemoteSymSigningKey;
    sp->setRemoteSymIv = Basic128Rsa15_setRemoteSymIv;
    sp->compareCertificate = Basic128Rsa15_compareCertificate;
    sp->generateKey = UA_Sym_Basic128Rsa15_generateKey;
    sp->generateNonce = UA_Sym_Basic128Rsa15_generateNonce;
    sp->nonceLength = 16;  /* 128 bits*/
    sp->makeCertThumbprint = UA_Asy_Basic128Rsa15_makeCertificateThumbprint;
    sp->compareCertThumbprint = UA_Asy_Basic128Rsa15_compareCertThumbprint;
    sp->updateCertificate = updateCertificate_sp_basic128rsa15;
    sp->createSigningRequest = createSigningRequest_sp_basic128rsa15;
    sp->clear = UA_Policy_Basic128Rsa15_Clear_Context;

    /* Parse the certificate */
    UA_Openssl_Init();
    UA_StatusCode res =
        UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = UA_Policy_Basic128Rsa15_New_Context(sp, localPrivateKey, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

#endif
