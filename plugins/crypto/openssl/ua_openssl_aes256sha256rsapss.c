/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2022 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include "securitypolicy_openssl_common.h"

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

typedef struct {
    EVP_PKEY *localPrivateKey;
    UA_ByteString localCertThumbprint;
    const UA_Logger *logger;
} Policy_Context_Aes256Sha256RsaPss;

typedef struct {
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    Policy_Context_Aes256Sha256RsaPss *policyContext;
    UA_ByteString remoteCertificate;
    X509 *remoteCertificateX509; /* X509 */
} Channel_Context_Aes256Sha256RsaPss;

/* create the policy context */

static UA_StatusCode
UA_Policy_Aes256Sha256RsaPss_New_Context(UA_SecurityPolicy *securityPolicy,
                                          const UA_ByteString localPrivateKey,
                                          const UA_Logger *logger) {
    Policy_Context_Aes256Sha256RsaPss *context =
        (Policy_Context_Aes256Sha256RsaPss *)UA_malloc(
            sizeof(Policy_Context_Aes256Sha256RsaPss));
    if(context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    context->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&localPrivateKey);
    if (!context->localPrivateKey) {
        UA_free(context);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval = UA_Openssl_X509_GetCertificateThumbprint(
        &securityPolicy->localCertificate, &context->localCertThumbprint, true);
    if(retval != UA_STATUSCODE_GOOD) {
        EVP_PKEY_free(context->localPrivateKey);
        UA_free(context);
        return retval;
    }

    context->logger = logger;
    securityPolicy->policyContext = context;

    return UA_STATUSCODE_GOOD;
}

/* clear the policy context */

static void
UA_Policy_Aes256Sha256RsaPss_Clear_Context(UA_SecurityPolicy *policy) {
    if(policy == NULL)
        return;

    UA_ByteString_clear(&policy->localCertificate);

    /* delete all allocated members in the context */

    Policy_Context_Aes256Sha256RsaPss *pc =
        (Policy_Context_Aes256Sha256RsaPss *)policy->policyContext;
    if (pc == NULL) {
        return;
    }

    EVP_PKEY_free(pc->localPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);

    return;
}

/* create the channel context */

static UA_StatusCode
UA_ChannelModule_Aes256Sha256RsaPss_New_Context(const UA_SecurityPolicy *securityPolicy,
                                                 const UA_ByteString *remoteCertificate,
                                                 void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    Channel_Context_Aes256Sha256RsaPss *context =
        (Channel_Context_Aes256Sha256RsaPss *)UA_malloc(
            sizeof(Channel_Context_Aes256Sha256RsaPss));
    if(context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

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
    context->remoteCertificateX509 = UA_OpenSSL_LoadCertificate(&context->remoteCertificate);
    if (context->remoteCertificateX509 == NULL) {
        UA_ByteString_clear (&context->remoteCertificate);
        UA_free (context);
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

    context->policyContext =
        (Policy_Context_Aes256Sha256RsaPss *)(securityPolicy->policyContext);

    *channelContext = context;

    UA_LOG_INFO(
        securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
        "The Aes256Sha256RsaPss security policy channel with openssl is created.");

    return UA_STATUSCODE_GOOD;
}

/* delete the channel context */

static void
UA_ChannelModule_Aes256Sha256RsaPss_Delete_Context(void *channelContext) {
    if(channelContext != NULL) {
        Channel_Context_Aes256Sha256RsaPss *cc =
            (Channel_Context_Aes256Sha256RsaPss *)channelContext;
        X509_free(cc->remoteCertificateX509);
        UA_ByteString_clear(&cc->remoteCertificate);
        UA_ByteString_clear(&cc->localSymSigningKey);
        UA_ByteString_clear(&cc->localSymEncryptingKey);
        UA_ByteString_clear(&cc->localSymIv);
        UA_ByteString_clear(&cc->remoteSymSigningKey);
        UA_ByteString_clear(&cc->remoteSymEncryptingKey);
        UA_ByteString_clear(&cc->remoteSymIv);

        UA_LOG_INFO(
            cc->policyContext->logger, UA_LOGCATEGORY_SECURITYPOLICY,
            "The Aes256Sha256RsaPss security policy channel with openssl is deleted.");
        UA_free(cc);
    }
}

/* Verifies the signature of the message using the provided keys in the context.
 * AsymmetricSignatureAlgorithm_RSA-PKCS15-SHA2-256
 */

static UA_StatusCode
UA_AsySig_Aes256Sha256RsaPss_Verify(void *channelContext, const UA_ByteString *message,
                                     const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_StatusCode retval = UA_OpenSSL_RSA_PSS_SHA256_Verify(
        message, cc->remoteCertificateX509, signature);

    return retval;
}

/* Compares the supplied certificate with the certificate
 * in the endpoint context
 */

static UA_StatusCode
UA_compareCertificateThumbprint_Aes256Sha256RsaPss(const UA_SecurityPolicy *securityPolicy,
                                                    const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    Policy_Context_Aes256Sha256RsaPss *pc =
        (Policy_Context_Aes256Sha256RsaPss *)securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

/* Generates a thumbprint for the specified certificate */

static UA_StatusCode
UA_makeCertificateThumbprint_Aes256Sha256RsaPss(const UA_SecurityPolicy *securityPolicy,
                                                 const UA_ByteString *certificate,
                                                 UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

static UA_StatusCode
UA_Asym_Aes256Sha256RsaPss_Decrypt(void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    Channel_Context_Aes256Sha256RsaPss *cc = (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_StatusCode ret = UA_Openssl_RSA_Oaep_Sha2_Decrypt(data, cc->policyContext->localPrivateKey);
    return ret;
}

static size_t
UA_Asym_Aes256Sha256RsaPss_getRemoteSignatureSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_Aes256Sha256RsaPss *cc = (const Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen;
}

static size_t
UA_AsySig_Aes256Sha256RsaPss_getLocalSignatureSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_Aes256Sha256RsaPss *cc = (const Channel_Context_Aes256Sha256RsaPss *)channelContext;
    Policy_Context_Aes256Sha256RsaPss *pc = cc->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t)keyLen;
}

static size_t
UA_AsymEn_Aes256Sha256RsaPss_getRemotePlainTextBlockSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_Aes256Sha256RsaPss *cc =
        (const Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen - UA_SECURITYPOLICY_AES256SHA256RSAPSS_RSAPADDING_LEN;
}

static size_t
UA_AsymEn_Aes256Sha256RsaPss_getRemoteBlockSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_Aes256Sha256RsaPss *cc =
        (const Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen;
}

static size_t
UA_AsymEn_Aes256Sha256RsaPss_getRemoteKeyLength(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_Aes256Sha256RsaPss *cc =
        (const Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen * 8;
}

static UA_StatusCode
UA_Sym_Aes256Sha256RsaPss_generateNonce(void *policyContext,
                                         UA_ByteString *out) {
    UA_Int32 rc = RAND_bytes(out->data, (int)out->length);
    if(rc != 1) {
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static size_t
UA_SymEn_Aes256Sha256RsaPss_getLocalKeyLength(const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymSig_Aes256Sha256RsaPss_getLocalKeyLength(const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_Sym_Aes256Sha256RsaPss_generateKey(void *policyContext,
                                       const UA_ByteString *secret,
                                       const UA_ByteString *seed, UA_ByteString *out) {
    return UA_Openssl_Random_Key_PSHA256_Derive(secret, seed, out);
}

static UA_StatusCode
UA_CertSig_Aes256Sha256RsaPss_Verify(void *channelContext, const UA_ByteString *message,
                                     const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_StatusCode retval = UA_OpenSSL_RSA_PKCS1_V15_SHA256_Verify(
        message, cc->remoteCertificateX509, signature);

    return retval;
}

static UA_StatusCode
UA_CertSig_Aes256Sha256RsaPss_sign(void *channelContext, const UA_ByteString *message,
                                   UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL ||
       signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc = (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    Policy_Context_Aes256Sha256RsaPss *pc = cc->policyContext;
    return UA_Openssl_RSA_PKCS1_V15_SHA256_Sign(message, pc->localPrivateKey, signature);
}

static size_t
UA_CertSig_Aes256Sha256RsaPss_getRemoteSignatureSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_Aes256Sha256RsaPss *cc = (const Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen;
}

static size_t
UA_CertSig_Aes256Sha256RsaPss_getLocalSignatureSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_Aes256Sha256RsaPss *cc = (const Channel_Context_Aes256Sha256RsaPss *)channelContext;
    Policy_Context_Aes256Sha256RsaPss *pc = cc->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t)keyLen;
}

static UA_StatusCode
UA_ChannelModule_Aes256Sha256RsaPss_setLocalSymSigningKey(void *channelContext,
                                                           const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
UA_ChannelM_Aes256Sha256RsaPss_setLocalSymEncryptingKey(void *channelContext,
                                                         const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
UA_ChannelM_Aes256Sha256RsaPss_setLocalSymIv(void *channelContext,
                                              const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static size_t
UA_SymEn_Aes256Sha256RsaPss_getRemoteKeyLength(const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_Aes256Sha256RsaPss_getBlockSize(const void *channelContext) {
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
UA_SymSig_Aes256Sha256RsaPss_getRemoteKeyLength(const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_AES256SHA256RSAPSS_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_ChannelM_Aes256Sha256RsaPss_setRemoteSymSigningKey(void *channelContext,
                                                       const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
UA_ChannelM_Aes256Sha256RsaPss_setRemoteSymEncryptingKey(void *channelContext,
                                                          const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
UA_ChannelM_Aes256Sha256RsaPss_setRemoteSymIv(void *channelContext,
                                               const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(key, &cc->remoteSymIv);
}

static UA_StatusCode
UA_AsySig_Aes256Sha256RsaPss_sign(void *channelContext, const UA_ByteString *message,
                                   UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL ||
       signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    Policy_Context_Aes256Sha256RsaPss *pc = cc->policyContext;
    return UA_Openssl_RSA_PSS_SHA256_Sign(message, pc->localPrivateKey, signature);
}

static UA_StatusCode
UA_AsymEn_Aes256Sha256RsaPss_encrypt(void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    return UA_Openssl_RSA_OAEP_SHA2_Encrypt(
        data, UA_SECURITYPOLICY_AES256SHA256RSAPSS_RSAPADDING_LEN,
        cc->remoteCertificateX509);
}

static size_t
UA_SymSig_Aes256Sha256RsaPss_getRemoteSignatureSize(const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymSig_Aes256Sha256RsaPss_verify(void *channelContext, const UA_ByteString *message,
                                     const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL ||
       signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Verify(message, &cc->remoteSymSigningKey, signature);
}

static UA_StatusCode
UA_SymSig_Aes256Sha256RsaPss_sign(void *channelContext, const UA_ByteString *message,
                                   UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL ||
       signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Sign(message, &cc->localSymSigningKey, signature);
}

static size_t
UA_SymSig_Aes256Sha256RsaPss_getLocalSignatureSize(const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymEn_Aes256Sha256RsaPss_decrypt(void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    return UA_OpenSSL_AES_256_CBC_Decrypt(&cc->remoteSymIv, &cc->remoteSymEncryptingKey,
                                          data);
}

static UA_StatusCode
UA_SymEn_Aes256Sha256RsaPss_encrypt(void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_Aes256Sha256RsaPss *cc =
        (Channel_Context_Aes256Sha256RsaPss *)channelContext;
    return UA_OpenSSL_AES_256_CBC_Encrypt(&cc->localSymIv, &cc->localSymEncryptingKey,
                                          data);
}

static UA_StatusCode
UA_ChannelM_Aes256Sha256RsaPss_compareCertificate(const void *channelContext,
                                                   const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_Aes256Sha256RsaPss *cc =
        (const Channel_Context_Aes256Sha256RsaPss *)channelContext;
    return UA_OpenSSL_X509_compare(certificate, cc->remoteCertificateX509);
}

static size_t
UA_AsymEn_Aes256Sha256RsaPss_getLocalKeyLength(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_Aes256Sha256RsaPss *cc =
        (const Channel_Context_Aes256Sha256RsaPss *)channelContext;
    Policy_Context_Aes256Sha256RsaPss *pc = cc->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t)keyLen * 8;
}

/* the main entry of Aes256Sha256RsaPss */

UA_StatusCode
UA_SecurityPolicy_Aes256Sha256RsaPss(UA_SecurityPolicy *policy,
                                      const UA_ByteString localCertificate,
                                      const UA_ByteString localPrivateKey,
                                      const UA_Logger *logger) {

    UA_SecurityPolicyAsymmetricModule *const asymmetricModule = &policy->asymmetricModule;
    UA_SecurityPolicySymmetricModule *const symmetricModule = &policy->symmetricModule;
    UA_SecurityPolicyChannelModule *const channelModule = &policy->channelModule;
    UA_StatusCode retval;

    UA_LOG_INFO(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                "The Aes256Sha256RsaPss security policy with openssl is added.");

    UA_Openssl_Init();
    memset(policy, 0, sizeof(UA_SecurityPolicy));
    policy->logger = logger;
    policy->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss\0");
    policy->securityLevel = 20;

    /* set ChannelModule context  */

    channelModule->newContext = UA_ChannelModule_Aes256Sha256RsaPss_New_Context;
    channelModule->deleteContext = UA_ChannelModule_Aes256Sha256RsaPss_Delete_Context;
    channelModule->setLocalSymSigningKey =
        UA_ChannelModule_Aes256Sha256RsaPss_setLocalSymSigningKey;
    channelModule->setLocalSymEncryptingKey =
        UA_ChannelM_Aes256Sha256RsaPss_setLocalSymEncryptingKey;
    channelModule->setLocalSymIv = UA_ChannelM_Aes256Sha256RsaPss_setLocalSymIv;
    channelModule->setRemoteSymSigningKey =
        UA_ChannelM_Aes256Sha256RsaPss_setRemoteSymSigningKey;
    channelModule->setRemoteSymEncryptingKey =
        UA_ChannelM_Aes256Sha256RsaPss_setRemoteSymEncryptingKey;
    channelModule->setRemoteSymIv = UA_ChannelM_Aes256Sha256RsaPss_setRemoteSymIv;
    channelModule->compareCertificate =
        UA_ChannelM_Aes256Sha256RsaPss_compareCertificate;

    /* Copy the certificate and add a NULL to the end */

    retval = UA_copyCertificate(&policy->localCertificate, &localCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* AsymmetricModule - signature algorithm */

    UA_SecurityPolicySignatureAlgorithm *asySigAlgorithm =
        &asymmetricModule->cryptoModule.signatureAlgorithm;
    asySigAlgorithm->uri =
        UA_STRING("http://opcfoundation.org/UA/security/rsa-pss-sha2-256\0");
    asySigAlgorithm->verify = UA_AsySig_Aes256Sha256RsaPss_Verify;
    asySigAlgorithm->getRemoteSignatureSize =
        UA_Asym_Aes256Sha256RsaPss_getRemoteSignatureSize;
    asySigAlgorithm->getLocalSignatureSize =
        UA_AsySig_Aes256Sha256RsaPss_getLocalSignatureSize;
    asySigAlgorithm->sign = UA_AsySig_Aes256Sha256RsaPss_sign;
    asySigAlgorithm->getLocalKeyLength = NULL;
    asySigAlgorithm->getRemoteKeyLength = NULL;

    /*  AsymmetricModule encryption algorithm */

    UA_SecurityPolicyEncryptionAlgorithm *asymEncryAlg =
        &asymmetricModule->cryptoModule.encryptionAlgorithm;
    asymEncryAlg->uri = UA_STRING("http://opcfoundation.org/UA/security/rsa-oaep-sha2-256\0");
    asymEncryAlg->decrypt = UA_Asym_Aes256Sha256RsaPss_Decrypt;
    asymEncryAlg->getRemotePlainTextBlockSize =
        UA_AsymEn_Aes256Sha256RsaPss_getRemotePlainTextBlockSize;
    asymEncryAlg->getRemoteBlockSize = UA_AsymEn_Aes256Sha256RsaPss_getRemoteBlockSize;
    asymEncryAlg->getRemoteKeyLength = UA_AsymEn_Aes256Sha256RsaPss_getRemoteKeyLength;
    asymEncryAlg->encrypt = UA_AsymEn_Aes256Sha256RsaPss_encrypt;
    asymEncryAlg->getLocalKeyLength = UA_AsymEn_Aes256Sha256RsaPss_getLocalKeyLength;

    /* asymmetricModule */

    asymmetricModule->compareCertificateThumbprint =
        UA_compareCertificateThumbprint_Aes256Sha256RsaPss;
    asymmetricModule->makeCertificateThumbprint =
        UA_makeCertificateThumbprint_Aes256Sha256RsaPss;

    /* SymmetricModule */

    symmetricModule->secureChannelNonceLength = 32;
    symmetricModule->generateNonce = UA_Sym_Aes256Sha256RsaPss_generateNonce;
    symmetricModule->generateKey = UA_Sym_Aes256Sha256RsaPss_generateKey;

    /* Symmetric encryption Algorithm */

    UA_SecurityPolicyEncryptionAlgorithm *symEncryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    symEncryptionAlgorithm->uri =
        UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc\0");
    symEncryptionAlgorithm->getLocalKeyLength = UA_SymEn_Aes256Sha256RsaPss_getLocalKeyLength;
    symEncryptionAlgorithm->getRemoteKeyLength = UA_SymEn_Aes256Sha256RsaPss_getRemoteKeyLength;
    symEncryptionAlgorithm->getRemoteBlockSize = UA_SymEn_Aes256Sha256RsaPss_getBlockSize;
    symEncryptionAlgorithm->getRemotePlainTextBlockSize = UA_SymEn_Aes256Sha256RsaPss_getBlockSize;
    symEncryptionAlgorithm->decrypt = UA_SymEn_Aes256Sha256RsaPss_decrypt;
    symEncryptionAlgorithm->encrypt = UA_SymEn_Aes256Sha256RsaPss_encrypt;

    /* Symmetric signature Algorithm */

    UA_SecurityPolicySignatureAlgorithm *symSignatureAlgorithm =
        &symmetricModule->cryptoModule.signatureAlgorithm;
    symSignatureAlgorithm->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256\0");
    symSignatureAlgorithm->getLocalKeyLength = UA_SymSig_Aes256Sha256RsaPss_getLocalKeyLength;
    symSignatureAlgorithm->getRemoteKeyLength = UA_SymSig_Aes256Sha256RsaPss_getRemoteKeyLength;
    symSignatureAlgorithm->getRemoteSignatureSize = UA_SymSig_Aes256Sha256RsaPss_getRemoteSignatureSize;
    symSignatureAlgorithm->verify = UA_SymSig_Aes256Sha256RsaPss_verify;
    symSignatureAlgorithm->sign = UA_SymSig_Aes256Sha256RsaPss_sign;
    symSignatureAlgorithm->getLocalSignatureSize = UA_SymSig_Aes256Sha256RsaPss_getLocalSignatureSize;

    retval = UA_Policy_Aes256Sha256RsaPss_New_Context(policy, localPrivateKey, logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&policy->localCertificate);
        return retval;
    }
    policy->clear = UA_Policy_Aes256Sha256RsaPss_Clear_Context;

    /* Certificate Signing Algorithm */
    policy->certificateSigningAlgorithm.uri =
        UA_STRING("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256\0");
    policy->certificateSigningAlgorithm.verify =
        (UA_StatusCode (*)(void *, const UA_ByteString *, const UA_ByteString *))UA_CertSig_Aes256Sha256RsaPss_Verify;
    policy->certificateSigningAlgorithm.sign =
        (UA_StatusCode (*)(void *, const UA_ByteString *, UA_ByteString *))UA_CertSig_Aes256Sha256RsaPss_sign;
    policy->certificateSigningAlgorithm.getLocalSignatureSize =
        (size_t (*)(const void *))UA_CertSig_Aes256Sha256RsaPss_getLocalSignatureSize;
    policy->certificateSigningAlgorithm.getRemoteSignatureSize =
        (size_t (*)(const void *))UA_CertSig_Aes256Sha256RsaPss_getRemoteSignatureSize;
    policy->certificateSigningAlgorithm.getLocalKeyLength = NULL; // TODO: Write function
    policy->certificateSigningAlgorithm.getRemoteKeyLength = NULL; // TODO: Write function

    return UA_STATUSCODE_GOOD;
}

#endif
