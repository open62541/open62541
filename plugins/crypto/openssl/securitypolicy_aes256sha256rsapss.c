/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include "securitypolicy_common.h"

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#define UA_SHA256_LENGTH 32 /* 256 bit */
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_RSAPADDING_LEN 66
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_KEY_LENGTH 32
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_MINASYMKEYLENGTH 256
#define UA_SECURITYPOLICY_AES256SHA256RSAPSS_MAXASYMKEYLENGTH 512

static UA_StatusCode
UA_Policy_Aes256Sha256RsaPss_New_Context(UA_SecurityPolicy *securityPolicy,
                                         const UA_ByteString localPrivateKey,
                                         const UA_Logger *logger) {
    openssl_PolicyContext *context =
        (openssl_PolicyContext *)UA_malloc(
            sizeof(openssl_PolicyContext));
    if(context == NULL)
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

    securityPolicy->policyContext = context;

    return UA_STATUSCODE_GOOD;
}

static void
UA_Policy_Aes256Sha256RsaPss_Clear_Context(UA_SecurityPolicy *policy) {
    if(policy == NULL)
        return;

    UA_ByteString_clear(&policy->localCertificate);

    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    if(pc == NULL)
        return;

    EVP_PKEY_free(pc->localPrivateKey);
    EVP_PKEY_free(pc->csrLocalPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);
}

static UA_StatusCode
createSigningRequest_sp_aes128sha256rsapss(UA_SecurityPolicy *securityPolicy,
                                           const UA_String *subjectName,
                                           const UA_ByteString *nonce,
                                           const UA_KeyValueMap *params,
                                           UA_ByteString *csr,
                                           UA_ByteString *newPrivateKey) {
    if(!securityPolicy || !csr)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
     if(!securityPolicy->policyContext)
         return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext*)securityPolicy->policyContext;
    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey, &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName,
                                           nonce, csr, newPrivateKey);
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_aes128sha256rsapss(UA_SecurityPolicy *securityPolicy,
                                                     const UA_ByteString newCertificate,
                                                     const UA_ByteString newPrivateKey) {
    if(!securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)securityPolicy->policyContext;

    UA_Boolean isLocalKey = false;
    if(newPrivateKey.length <= 0) {
        if(UA_CertificateUtils_comparePublicKeys(&newCertificate,
                                                 &securityPolicy->localCertificate) == 0)
            isLocalKey = true;
    }

    UA_ByteString_clear(&securityPolicy->localCertificate);
    UA_ByteString_clear(&pc->localCertThumbprint);

    UA_StatusCode retval =
        UA_OpenSSL_LoadLocalCertificate(&newCertificate,
                                        &securityPolicy->localCertificate);
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

    retval = UA_Openssl_X509_GetCertificateThumbprint(&securityPolicy->localCertificate,
                                                      &pc->localCertThumbprint, true);
    if(retval != UA_STATUSCODE_GOOD)
        goto error;

    return UA_STATUSCODE_GOOD;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not update certificate and private key");
    if(securityPolicy->policyContext)
        UA_Policy_Aes256Sha256RsaPss_Clear_Context(securityPolicy);
    return retval;
}

static UA_StatusCode
Aes256Sha256RsaPss_New_Context(const UA_SecurityPolicy *securityPolicy,
                               const UA_ByteString *remoteCertificate,
                               void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    openssl_ChannelContext *context =
        (openssl_ChannelContext *)UA_malloc(
            sizeof(openssl_ChannelContext));
    if(context == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_ByteString_init(&context->localSymSigningKey);
    UA_ByteString_init(&context->localSymEncryptingKey);
    UA_ByteString_init(&context->localSymIv);
    UA_ByteString_init(&context->remoteSymSigningKey);
    UA_ByteString_init(&context->remoteSymEncryptingKey);
    UA_ByteString_init(&context->remoteSymIv);

    UA_StatusCode retval =
        UA_copyCertificate(&context->remoteCertificate, remoteCertificate);
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

    UA_LOG_INFO(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                "The Aes256Sha256RsaPss security policy channel with openssl is created");

    return UA_STATUSCODE_GOOD;
}

static void
Aes256Sha256RsaPss_Delete_Context(const UA_SecurityPolicy *policy,
                                  void *channelContext) {
    if(channelContext == NULL)
        return;
    openssl_ChannelContext *cc =
        (openssl_ChannelContext *)channelContext;
    X509_free(cc->remoteCertificateX509);
    UA_ByteString_clear(&cc->remoteCertificate);
    UA_ByteString_clear(&cc->localSymSigningKey);
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    UA_ByteString_clear(&cc->localSymIv);
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    UA_ByteString_clear(&cc->remoteSymIv);
    UA_LOG_INFO(policy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                "The Aes256Sha256RsaPss security policy channel with openssl is deleted.");
    UA_free(cc);
}

static UA_StatusCode
UA_AsySig_Aes256Sha256RsaPss_Verify(const UA_SecurityPolicy *policy, void *channelContext,
                                    const UA_ByteString *message,
                                    const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc =
        (openssl_ChannelContext *)channelContext;
    return UA_OpenSSL_RSA_PSS_SHA256_Verify(message, cc->remoteCertificateX509, signature);
}

static UA_StatusCode
UA_compareCertificateThumbprint_Aes256Sha256RsaPss(const UA_SecurityPolicy *securityPolicy,
                                                   const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_makeCertificateThumbprint_Aes256Sha256RsaPss(const UA_SecurityPolicy *securityPolicy,
                                                const UA_ByteString *certificate,
                                                UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

static UA_StatusCode
UA_Asym_Aes256Sha256RsaPss_Decrypt(const UA_SecurityPolicy *policy,
                                   void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    return UA_Openssl_RSA_Oaep_Sha2_Decrypt(data, pc->localPrivateKey);
}

static size_t
UA_Asym_Aes256Sha256RsaPss_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                                  const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen;
}

static size_t
UA_AsySig_Aes256Sha256RsaPss_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                                   const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t)keyLen;
}

static size_t
UA_AsymEn_Aes256Sha256RsaPss_getRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                                         const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen - UA_SECURITYPOLICY_AES256SHA256RSAPSS_RSAPADDING_LEN;
}

static size_t
UA_AsymEn_Aes256Sha256RsaPss_getRemoteBlockSize(const UA_SecurityPolicy *policy,
                                                const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen;
}

static size_t
UA_AsymEn_Aes256Sha256RsaPss_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                                const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen * 8;
}

static UA_StatusCode
UA_Sym_Aes256Sha256RsaPss_generateNonce(const UA_SecurityPolicy *policy,
                                        void *channelContext, UA_ByteString *out) {
    UA_Int32 rc = RAND_bytes(out->data, (int)out->length);
    if(rc != 1)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static size_t
UA_SymEn_Aes256Sha256RsaPss_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymSig_Aes256Sha256RsaPss_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_Sym_Aes256Sha256RsaPss_generateKey(const UA_SecurityPolicy *policy,
                                      void *channelContext, const UA_ByteString *secret,
                                      const UA_ByteString *seed, UA_ByteString *out) {
    return UA_Openssl_Random_Key_PSHA256_Derive(secret, seed, out);
}

static UA_StatusCode
UA_CertSig_Aes256Sha256RsaPss_Verify(const UA_SecurityPolicy *policy, void *channelContext,
                                     const UA_ByteString *message,
                                     const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc =
        (openssl_ChannelContext *)channelContext;
    return UA_OpenSSL_RSA_PKCS1_V15_SHA256_Verify(message, cc->remoteCertificateX509,
                                                  signature);
}

static UA_StatusCode
UA_CertSig_Aes256Sha256RsaPss_sign(const UA_SecurityPolicy *policy,
                                   void *channelContext, const UA_ByteString *message,
                                   UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    return UA_Openssl_RSA_PKCS1_V15_SHA256_Sign(message, pc->localPrivateKey, signature);
}

static size_t
UA_CertSig_Aes256Sha256RsaPss_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                                     const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen;
}

static size_t
UA_CertSig_Aes256Sha256RsaPss_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                                    const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t)keyLen;
}

static size_t
UA_SymEn_Aes256Sha256RsaPss_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_Aes256Sha256RsaPss_getBlockSize(const UA_SecurityPolicy *policy,
                                         const void *channelContext) {
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
UA_SymSig_Aes256Sha256RsaPss_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                                const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_AsySig_Aes256Sha256RsaPss_sign(const UA_SecurityPolicy *policy,
                                  void *channelContext, const UA_ByteString *message,
                                  UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    return UA_Openssl_RSA_PSS_SHA256_Sign(message, pc->localPrivateKey, signature);
}

static UA_StatusCode
UA_AsymEn_Aes256Sha256RsaPss_encrypt(const UA_SecurityPolicy *policy,
                                     void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc =
        (openssl_ChannelContext *)channelContext;
    return UA_Openssl_RSA_OAEP_SHA2_Encrypt(data,
                             UA_SECURITYPOLICY_AES256SHA256RSAPSS_RSAPADDING_LEN,
                                            cc->remoteCertificateX509);
}

static size_t
UA_SymSig_Aes256Sha256RsaPss_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                                    const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymSig_Aes256Sha256RsaPss_verify(const UA_SecurityPolicy *policy, void *channelContext,
                                    const UA_ByteString *message,
                                    const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc =
        (openssl_ChannelContext *)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Verify(message, &cc->remoteSymSigningKey, signature);
}

static UA_StatusCode
UA_SymSig_Aes256Sha256RsaPss_sign(const UA_SecurityPolicy *policy,
                                  void *channelContext, const UA_ByteString *message,
                                  UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc =
        (openssl_ChannelContext *)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Sign(message, &cc->localSymSigningKey, signature);
}

static size_t
UA_SymSig_Aes256Sha256RsaPss_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                                   const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymEn_Aes256Sha256RsaPss_decrypt(const UA_SecurityPolicy *policy,
                                    void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc =
        (openssl_ChannelContext *)channelContext;
    return UA_OpenSSL_AES_256_CBC_Decrypt(&cc->remoteSymIv, &cc->remoteSymEncryptingKey,
                                          data);
}

static UA_StatusCode
UA_SymEn_Aes256Sha256RsaPss_encrypt(const UA_SecurityPolicy *policy,
                                    void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc =
        (openssl_ChannelContext *)channelContext;
    return UA_OpenSSL_AES_256_CBC_Encrypt(&cc->localSymIv, &cc->localSymEncryptingKey,
                                          data);
}

static size_t
UA_AsymEn_Aes256Sha256RsaPss_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t)keyLen * 8;
}

UA_StatusCode
UA_SecurityPolicy_Aes256Sha256RsaPss(UA_SecurityPolicy *sp,
                                      const UA_ByteString localCertificate,
                                      const UA_ByteString localPrivateKey,
                                      const UA_Logger *logger) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                "The Aes256Sha256RsaPss security policy with openssl is added.");

    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss\0");
    sp->certificateGroupId = UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(RSASHA256APPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 30;
    sp->policyType = UA_SECURITYPOLICYTYPE_RSA;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING("http://opcfoundation.org/UA/security/rsa-pss-sha2-256\0");
    asymSig->verify = UA_AsySig_Aes256Sha256RsaPss_Verify;
    asymSig->sign = UA_AsySig_Aes256Sha256RsaPss_sign;
    asymSig->getLocalSignatureSize = UA_AsySig_Aes256Sha256RsaPss_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = UA_Asym_Aes256Sha256RsaPss_getRemoteSignatureSize;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("http://opcfoundation.org/UA/security/rsa-oaep-sha2-256\0");
    asymEnc->encrypt = UA_AsymEn_Aes256Sha256RsaPss_encrypt;
    asymEnc->decrypt = UA_Asym_Aes256Sha256RsaPss_Decrypt;
    asymEnc->getLocalKeyLength = UA_AsymEn_Aes256Sha256RsaPss_getLocalKeyLength;
    asymEnc->getRemoteKeyLength = UA_AsymEn_Aes256Sha256RsaPss_getRemoteKeyLength;
    asymEnc->getRemoteBlockSize = UA_AsymEn_Aes256Sha256RsaPss_getRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = UA_AsymEn_Aes256Sha256RsaPss_getRemotePlainTextBlockSize;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256\0");
    symSig->verify = UA_SymSig_Aes256Sha256RsaPss_verify;
    symSig->sign = UA_SymSig_Aes256Sha256RsaPss_sign;
    symSig->getLocalSignatureSize = UA_SymSig_Aes256Sha256RsaPss_getLocalSignatureSize;
    symSig->getRemoteSignatureSize = UA_SymSig_Aes256Sha256RsaPss_getRemoteSignatureSize;
    symSig->getLocalKeyLength = UA_SymSig_Aes256Sha256RsaPss_getLocalKeyLength;
    symSig->getRemoteKeyLength = UA_SymSig_Aes256Sha256RsaPss_getRemoteKeyLength;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc\0");
    symEnc->encrypt = UA_SymEn_Aes256Sha256RsaPss_encrypt;
    symEnc->decrypt = UA_SymEn_Aes256Sha256RsaPss_decrypt;
    symEnc->getLocalKeyLength = UA_SymEn_Aes256Sha256RsaPss_getLocalKeyLength;
    symEnc->getRemoteKeyLength = UA_SymEn_Aes256Sha256RsaPss_getRemoteKeyLength;
    symEnc->getRemoteBlockSize = UA_SymEn_Aes256Sha256RsaPss_getBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_SymEn_Aes256Sha256RsaPss_getBlockSize;

    /* Certificate Signing */
    UA_SecurityPolicySignatureAlgorithm *certSig = &sp->certSignatureAlgorithm;
    certSig->uri = UA_STRING("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256\0");
    certSig->verify = UA_CertSig_Aes256Sha256RsaPss_Verify;
    certSig->sign = UA_CertSig_Aes256Sha256RsaPss_sign;
    certSig->getLocalSignatureSize = UA_CertSig_Aes256Sha256RsaPss_getLocalSignatureSize;
    certSig->getRemoteSignatureSize = UA_CertSig_Aes256Sha256RsaPss_getRemoteSignatureSize;

    /* Direct Method Pointers */
    sp->newChannelContext = Aes256Sha256RsaPss_New_Context;
    sp->deleteChannelContext = Aes256Sha256RsaPss_Delete_Context;
    sp->setLocalSymEncryptingKey = UA_OpenSSL_setLocalSymEncryptingKey_generic;
    sp->setLocalSymSigningKey = UA_OpenSSL_setLocalSymSigningKey_generic;
    sp->setLocalSymIv = UA_OpenSSL_setLocalSymIv_generic;
    sp->setRemoteSymEncryptingKey = UA_OpenSSL_setRemoteSymEncryptingKey_generic;
    sp->setRemoteSymSigningKey = UA_OpenSSL_setRemoteSymSigningKey_generic;
    sp->setRemoteSymIv = UA_OpenSSL_setRemoteSymIv_generic;
    sp->compareCertificate = UA_OpenSSL_compareCertificate_generic;
    sp->generateKey = UA_Sym_Aes256Sha256RsaPss_generateKey;
    sp->generateNonce = UA_Sym_Aes256Sha256RsaPss_generateNonce;
    sp->nonceLength = 32;
    sp->makeCertThumbprint = UA_makeCertificateThumbprint_Aes256Sha256RsaPss;
    sp->compareCertThumbprint = UA_compareCertificateThumbprint_Aes256Sha256RsaPss;
    sp->updateCertificate = updateCertificateAndPrivateKey_sp_aes128sha256rsapss;
    sp->createSigningRequest = createSigningRequest_sp_aes128sha256rsapss;
    sp->clear = UA_Policy_Aes256Sha256RsaPss_Clear_Context;

    /* Parse the certificate */
    UA_Openssl_Init();
    UA_StatusCode res =
        UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = UA_Policy_Aes256Sha256RsaPss_New_Context(sp, localPrivateKey, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

#endif
