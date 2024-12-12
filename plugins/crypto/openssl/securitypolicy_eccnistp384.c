/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Siemens AG (Authors: Tin Raic, Thomas Zeschg)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)

#include "securitypolicy_common.h"
#include <openssl/rand.h>

#define UA_SHA384_LENGTH 48 /* 384 bit */
#define UA_SECURITYPOLICY_ECCNISTP384_ASYM_SIGNING_KEY_LENGTH 48
#define UA_SECURITYPOLICY_ECCNISTP384_ASYM_SIGNATURE_LENGTH 96
#define UA_SECURITYPOLICY_ECCNISTP384_SYM_SIGNING_KEY_LENGTH 48
#define UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_KEY_LENGTH 32
#define UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCNISTP384_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_ECCNISTP384_NONCE_LENGTH_BYTES 96

typedef struct {
    EVP_PKEY *localPrivateKey;
    EVP_PKEY *csrLocalPrivateKey;
    UA_ByteString localCertThumbprint;
    const UA_Logger *logger;
    UA_ApplicationType applicationType;
    struct _Channel_Context_EccNistP384 *channelContext;
} Policy_Context_EccNistP384;

typedef struct _Channel_Context_EccNistP384 {
    EVP_PKEY *    localEphemeralKeyPair;
    UA_ByteString localSymSigningKey;
    UA_ByteString localSymEncryptingKey;
    UA_ByteString localSymIv;
    UA_ByteString remoteSymSigningKey;
    UA_ByteString remoteSymEncryptingKey;
    UA_ByteString remoteSymIv;

    Policy_Context_EccNistP384 *policyContext;
    UA_ByteString remoteCertificate;
    X509 *remoteCertificateX509; /* X509 */
} Channel_Context_EccNistP384;

/* create the policy context */

static UA_StatusCode
UA_Policy_EccNistP384_New_Context(UA_SecurityPolicy *securityPolicy,
                                          const UA_ByteString localPrivateKey,
                                          const UA_ApplicationType applicationType,
                                          const UA_Logger *logger) {
    Policy_Context_EccNistP384 *context =
        (Policy_Context_EccNistP384 *)UA_malloc(
            sizeof(Policy_Context_EccNistP384));
    if(context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    context->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&localPrivateKey);
    if (!context->localPrivateKey) {
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
    context->logger = logger;
    securityPolicy->policyContext = context;

    return UA_STATUSCODE_GOOD;
}

/* clear the policy context */

static void
UA_Policy_EccNistP384_Clear_Context(UA_SecurityPolicy *policy) {
    if(policy == NULL)
        return;

    UA_ByteString_clear(&policy->localCertificate);

    /* delete all allocated members in the context */

    Policy_Context_EccNistP384 *pc =
        (Policy_Context_EccNistP384 *)policy->policyContext;
    if (pc == NULL) {
        return;
    }

    EVP_PKEY_free(pc->localPrivateKey);
    EVP_PKEY_free(pc->csrLocalPrivateKey);
    UA_ByteString_clear(&pc->localCertThumbprint);
    UA_free(pc);

    return;
}

static UA_StatusCode
createSigningRequest_sp_eccnistp384(UA_SecurityPolicy *securityPolicy,
                                           const UA_String *subjectName,
                                           const UA_ByteString *nonce,
                                           const UA_KeyValueMap *params,
                                           UA_ByteString *csr,
                                           UA_ByteString *newPrivateKey) {
    /* Check parameter */
    if(!securityPolicy || !csr)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

     if(!securityPolicy->policyContext)
         return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_EccNistP384 *pc =
        (Policy_Context_EccNistP384*)securityPolicy->policyContext;

    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey, &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName,
                                           nonce, csr, newPrivateKey);
}

static UA_StatusCode
updateCertificateAndPrivateKey_sp_eccnistp384(UA_SecurityPolicy *securityPolicy,
                                                     const UA_ByteString newCertificate,
                                                     const UA_ByteString newPrivateKey) {
    if(!securityPolicy)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(!securityPolicy->policyContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    Policy_Context_EccNistP384 *pc =
        (Policy_Context_EccNistP384 *)securityPolicy->policyContext;

    UA_ByteString_clear(&securityPolicy->localCertificate);
    UA_ByteString_clear(&pc->localCertThumbprint);

    UA_StatusCode retval =
        UA_OpenSSL_LoadLocalCertificate(&newCertificate,
                                        &securityPolicy->localCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the new private key */
    EVP_PKEY_free(pc->localPrivateKey);

    pc->localPrivateKey = UA_OpenSSL_LoadPrivateKey(&newPrivateKey);
    if(!pc->localPrivateKey) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
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
        UA_Policy_EccNistP384_Clear_Context(securityPolicy);
    return retval;
}

/* create the channel context */

static UA_StatusCode
UA_ChannelModule_EccNistP384_New_Context(const UA_SecurityPolicy *securityPolicy,
                                                 const UA_ByteString *remoteCertificate,
                                                 void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    Channel_Context_EccNistP384 *context =
        (Channel_Context_EccNistP384 *)UA_calloc(1,
            sizeof(Channel_Context_EccNistP384));
    if(context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

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
        (Policy_Context_EccNistP384 *)(securityPolicy->policyContext);

    /* This reference is needed in UA_Sym_EccNistP384_generateNonce to get
     * access to the localEphemeralKeyPair variable in the channelContext */
    context->policyContext->channelContext = context;

    *channelContext = context;

    UA_LOG_INFO(
        securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
        "The EccNistP384 security policy channel with openssl is created.");

    return UA_STATUSCODE_GOOD;
}

/* delete the channel context */

static void
UA_ChannelModule_EccNistP384_Delete_Context(void *channelContext) {
    if(channelContext != NULL) {
        Channel_Context_EccNistP384 *cc =
            (Channel_Context_EccNistP384 *)channelContext;
        X509_free(cc->remoteCertificateX509);
        UA_ByteString_clear(&cc->remoteCertificate);
        UA_ByteString_clear(&cc->localSymSigningKey);
        UA_ByteString_clear(&cc->localSymEncryptingKey);
        UA_ByteString_clear(&cc->localSymIv);
        UA_ByteString_clear(&cc->remoteSymSigningKey);
        UA_ByteString_clear(&cc->remoteSymEncryptingKey);
        UA_ByteString_clear(&cc->remoteSymIv);
        EVP_PKEY_free(cc->localEphemeralKeyPair);

        /* Remove reference */
        cc->policyContext->channelContext = NULL;

        UA_LOG_INFO(
            cc->policyContext->logger, UA_LOGCATEGORY_SECURITYPOLICY,
            "The EccNistP384 security policy channel with openssl is deleted.");
        UA_free(cc);
    }
}

/* Verifies the signature of the message using the provided keys in the context.
 * AsymmetricSignatureAlgorithm_RSA-PKCS15-SHA2-256
 */

static UA_StatusCode
UA_AsySig_EccNistP384_Verify(void *channelContext, const UA_ByteString *message,
                                     const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    UA_StatusCode retval = UA_Openssl_ECDSA_SHA384_Verify(
        message, cc->remoteCertificateX509, signature);

    return retval;
}

/* Compares the supplied certificate with the certificate
 * in the endpoint context
 */

static UA_StatusCode
UA_compareCertificateThumbprint_EccNistP384(const UA_SecurityPolicy *securityPolicy,
                                                    const UA_ByteString *certificateThumbprint) {
    if(securityPolicy == NULL || certificateThumbprint == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    Policy_Context_EccNistP384 *pc =
        (Policy_Context_EccNistP384 *)securityPolicy->policyContext;
    if(!UA_ByteString_equal(certificateThumbprint, &pc->localCertThumbprint))
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return UA_STATUSCODE_GOOD;
}

/* Generates a thumbprint for the specified certificate */

static UA_StatusCode
UA_makeCertificateThumbprint_EccNistP384(const UA_SecurityPolicy *securityPolicy,
                                                 const UA_ByteString *certificate,
                                                 UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

static size_t
UA_Asym_EccNistP384_getRemoteSignatureSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* According to the standard, the server and the client must agree on the
     * security mode and security policy. Therefore, it can be assumed that
     * the remote asymmetric cryptographic tokens have the same sizes as the
     * local ones. */
    return UA_SECURITYPOLICY_ECCNISTP384_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsySig_EccNistP384_getLocalSignatureSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

     return UA_SECURITYPOLICY_ECCNISTP384_ASYM_SIGNATURE_LENGTH;
}

static size_t
UA_AsymEn_EccNistP384_getRemotePlainTextBlockSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return 1;
}

static size_t
UA_AsymEn_EccNistP384_getRemoteBlockSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return 1;
}

static size_t
UA_AsymEn_EccNistP384_getRemoteKeyLength(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* According to the standard, the server and the client must agree on the
     * security mode and security policy. Therefore, it can be assumed that
     * the remote asymmetric cryptographic tokens have the same sizes as the
     * local ones.
     *
     * No ECC encryption -> key length set to 1 to avoid division or
     * multiplication with 0 */
    return 1;
}

static UA_StatusCode
UA_Sym_EccNistP384_generateNonce(void *policyContext,
                                         UA_ByteString *out) {
    Policy_Context_EccNistP384 *pctx =
        (Policy_Context_EccNistP384 *) policyContext;

    if(pctx == NULL)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    if(pctx == NULL) {
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }

    if (out->length == UA_SECURITYPOLICY_ECCNISTP384_NONCE_LENGTH_BYTES) {
        if (UA_OpenSSL_ECC_NISTP384_GenerateKey(&pctx->channelContext->localEphemeralKeyPair, out) != UA_STATUSCODE_GOOD) {
            return UA_STATUSCODE_BADUNEXPECTEDERROR;
        }
    } else {
        UA_Int32 rc = RAND_bytes(out->data, (int)out->length);
        if(rc != 1) {
            return UA_STATUSCODE_BADUNEXPECTEDERROR;
        }
    }

    return UA_STATUSCODE_GOOD;
}

static size_t
UA_SymEn_EccNistP384_getLocalKeyLength(const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymSig_EccNistP384_getLocalKeyLength(const void *channelContext) {
    /* 48 bytes 384 bits */
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_Sym_EccNistP384_generateKey(void *policyContext, const UA_ByteString *key1,
                                const UA_ByteString *key2, UA_ByteString *out) {
    Policy_Context_EccNistP384* pctx =
        (Policy_Context_EccNistP384 *) policyContext;

    return UA_OpenSSL_ECC_DeriveKeys(EC_curve_nist2nid("P-384"), "SHA384",
                                     pctx->applicationType,
                                     pctx->channelContext->localEphemeralKeyPair,
                                     key1, key2, out);
}

static UA_StatusCode
UA_ChannelModule_EccNistP384_setLocalSymSigningKey(void *channelContext,
                                                           const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
UA_ChannelM_EccNistP384_setLocalSymEncryptingKey(void *channelContext,
                                                         const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
UA_ChannelM_EccNistP384_setLocalSymIv(void *channelContext,
                                              const UA_ByteString *iv) {
    if(iv == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static size_t
UA_SymEn_EccNistP384_getRemoteKeyLength(const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_EccNistP384_getBlockSize(const void *channelContext) {
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
UA_SymSig_EccNistP384_getRemoteKeyLength(const void *channelContext) {
    /* 48 bytes 384 bits */
    return UA_SECURITYPOLICY_ECCNISTP384_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_ChannelM_EccNistP384_setRemoteSymSigningKey(void *channelContext,
                                                       const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
UA_ChannelM_EccNistP384_setRemoteSymEncryptingKey(void *channelContext,
                                                          const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
UA_ChannelM_EccNistP384_setRemoteSymIv(void *channelContext,
                                               const UA_ByteString *key) {
    if(key == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(key, &cc->remoteSymIv);
}

static UA_StatusCode
UA_AsySig_EccNistP384_sign(void *channelContext, const UA_ByteString *message,
                                   UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL ||
       signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    Policy_Context_EccNistP384 *pc = cc->policyContext;
    return UA_Openssl_ECDSA_SHA384_Sign(message, pc->localPrivateKey, signature);
}

static UA_StatusCode
UA_Asym_EccNistP384_Dummy(void *channelContext, UA_ByteString *data) {
    /* Do nothing and return true */
    return UA_STATUSCODE_GOOD;
}

static size_t
UA_SymSig_EccNistP384_getRemoteSignatureSize(const void *channelContext) {
    return UA_SHA384_LENGTH;
}

static UA_StatusCode
UA_SymSig_EccNistP384_verify(void *channelContext, const UA_ByteString *message,
                                     const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL ||
       signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    return UA_OpenSSL_HMAC_SHA384_Verify(message, &cc->remoteSymSigningKey, signature);
}

static UA_StatusCode
UA_SymSig_EccNistP384_sign(void *channelContext, const UA_ByteString *message,
                                   UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL ||
       signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    return UA_OpenSSL_HMAC_SHA384_Sign(message, &cc->localSymSigningKey, signature);
}

static size_t
UA_SymSig_EccNistP384_getLocalSignatureSize(const void *channelContext) {
    return UA_SHA384_LENGTH;
}

static UA_StatusCode
UA_SymEn_EccNistP384_decrypt(void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    return UA_OpenSSL_AES_256_CBC_Decrypt(&cc->remoteSymIv, &cc->remoteSymEncryptingKey,
                                          data);
}

static UA_StatusCode
UA_SymEn_EccNistP384_encrypt(void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    Channel_Context_EccNistP384 *cc =
        (Channel_Context_EccNistP384 *)channelContext;
    return UA_OpenSSL_AES_256_CBC_Encrypt(&cc->localSymIv, &cc->localSymEncryptingKey,
                                          data);
}

static UA_StatusCode
UA_ChannelM_EccNistP384_compareCertificate(const void *channelContext,
                                                   const UA_ByteString *certificate) {
    if(channelContext == NULL || certificate == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    const Channel_Context_EccNistP384 *cc =
        (const Channel_Context_EccNistP384 *)channelContext;
    return UA_OpenSSL_X509_compare(certificate, cc->remoteCertificateX509);
}

static size_t
UA_AsymEn_EccNistP384_getLocalKeyLength(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* No ECC encryption -> key length set to 1 to avoid division or
     * multiplication with 0 */
    return 1;
}

/* the main entry of EccNistP384 */

UA_StatusCode
UA_SecurityPolicy_EccNistP384(UA_SecurityPolicy *policy,
                                      UA_ApplicationType applicationType,
                                      const UA_ByteString localCertificate,
                                      const UA_ByteString localPrivateKey,
                                      const UA_Logger *logger) {

    UA_SecurityPolicyAsymmetricModule *const asymmetricModule = &policy->asymmetricModule;
    UA_SecurityPolicySymmetricModule *const symmetricModule = &policy->symmetricModule;
    UA_SecurityPolicyChannelModule *const channelModule = &policy->channelModule;
    UA_StatusCode retval;

    UA_LOG_INFO(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                "The EccNistP384 security policy with openssl is added.");

    UA_Openssl_Init();
    memset(policy, 0, sizeof(UA_SecurityPolicy));
    policy->logger = logger;
    policy->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384\0");
    policy->securityLevel = 30;

    /* set ChannelModule context  */

    channelModule->newContext = UA_ChannelModule_EccNistP384_New_Context;
    channelModule->deleteContext = UA_ChannelModule_EccNistP384_Delete_Context;
    channelModule->setLocalSymSigningKey =
        UA_ChannelModule_EccNistP384_setLocalSymSigningKey;
    channelModule->setLocalSymEncryptingKey =
        UA_ChannelM_EccNistP384_setLocalSymEncryptingKey;
    channelModule->setLocalSymIv = UA_ChannelM_EccNistP384_setLocalSymIv;
    channelModule->setRemoteSymSigningKey =
        UA_ChannelM_EccNistP384_setRemoteSymSigningKey;
    channelModule->setRemoteSymEncryptingKey =
        UA_ChannelM_EccNistP384_setRemoteSymEncryptingKey;
    channelModule->setRemoteSymIv = UA_ChannelM_EccNistP384_setRemoteSymIv;
    channelModule->compareCertificate =
        UA_ChannelM_EccNistP384_compareCertificate;

    /* Copy the certificate and add a NULL to the end */

    retval = UA_copyCertificate(&policy->localCertificate, &localCertificate);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* AsymmetricModule - signature algorithm */

    UA_SecurityPolicySignatureAlgorithm *asySigAlgorithm =
        &asymmetricModule->cryptoModule.signatureAlgorithm;
    asySigAlgorithm->uri =
        UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2461\0");
    asySigAlgorithm->verify = UA_AsySig_EccNistP384_Verify;
    asySigAlgorithm->getRemoteSignatureSize =
        UA_Asym_EccNistP384_getRemoteSignatureSize;
    asySigAlgorithm->getLocalSignatureSize =
        UA_AsySig_EccNistP384_getLocalSignatureSize;
    asySigAlgorithm->sign = UA_AsySig_EccNistP384_sign;
    asySigAlgorithm->getLocalKeyLength = NULL;
    asySigAlgorithm->getRemoteKeyLength = NULL;

    /*  AsymmetricModule encryption algorithm */

    UA_SecurityPolicyEncryptionAlgorithm *asymEncryAlg =
        &asymmetricModule->cryptoModule.encryptionAlgorithm;
    asymEncryAlg->uri = UA_STRING("https://profiles.opcfoundation.org/conformanceunit/2720\0");
    asymEncryAlg->getRemotePlainTextBlockSize =
        UA_AsymEn_EccNistP384_getRemotePlainTextBlockSize;
    asymEncryAlg->getRemoteBlockSize = UA_AsymEn_EccNistP384_getRemoteBlockSize;
    asymEncryAlg->getRemoteKeyLength = UA_AsymEn_EccNistP384_getRemoteKeyLength;
    asymEncryAlg->getLocalKeyLength = UA_AsymEn_EccNistP384_getLocalKeyLength;
    asymEncryAlg->encrypt = UA_Asym_EccNistP384_Dummy;
    asymEncryAlg->decrypt = UA_Asym_EccNistP384_Dummy;

    /* asymmetricModule */

    asymmetricModule->compareCertificateThumbprint =
        UA_compareCertificateThumbprint_EccNistP384;
    asymmetricModule->makeCertificateThumbprint =
        UA_makeCertificateThumbprint_EccNistP384;

    /* SymmetricModule */

    symmetricModule->secureChannelNonceLength = UA_SECURITYPOLICY_ECCNISTP384_NONCE_LENGTH_BYTES;
    symmetricModule->generateNonce = UA_Sym_EccNistP384_generateNonce;
    symmetricModule->generateKey = UA_Sym_EccNistP384_generateKey;

    /* Symmetric encryption Algorithm */

    UA_SecurityPolicyEncryptionAlgorithm *symEncryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    symEncryptionAlgorithm->uri =
        UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc\0");
    symEncryptionAlgorithm->getLocalKeyLength = UA_SymEn_EccNistP384_getLocalKeyLength;
    symEncryptionAlgorithm->getRemoteKeyLength = UA_SymEn_EccNistP384_getRemoteKeyLength;
    symEncryptionAlgorithm->getRemoteBlockSize = UA_SymEn_EccNistP384_getBlockSize;
    symEncryptionAlgorithm->getRemotePlainTextBlockSize = UA_SymEn_EccNistP384_getBlockSize;
    symEncryptionAlgorithm->decrypt = UA_SymEn_EccNistP384_decrypt;
    symEncryptionAlgorithm->encrypt = UA_SymEn_EccNistP384_encrypt;

    /* Symmetric signature Algorithm */

    UA_SecurityPolicySignatureAlgorithm *symSignatureAlgorithm =
        &symmetricModule->cryptoModule.signatureAlgorithm;
    symSignatureAlgorithm->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-384\0");
    symSignatureAlgorithm->getLocalKeyLength = UA_SymSig_EccNistP384_getLocalKeyLength;
    symSignatureAlgorithm->getRemoteKeyLength = UA_SymSig_EccNistP384_getRemoteKeyLength;
    symSignatureAlgorithm->getRemoteSignatureSize = UA_SymSig_EccNistP384_getRemoteSignatureSize;
    symSignatureAlgorithm->verify = UA_SymSig_EccNistP384_verify;
    symSignatureAlgorithm->sign = UA_SymSig_EccNistP384_sign;
    symSignatureAlgorithm->getLocalSignatureSize = UA_SymSig_EccNistP384_getLocalSignatureSize;

    retval = UA_Policy_EccNistP384_New_Context(policy, localPrivateKey, applicationType, logger);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&policy->localCertificate);
        return retval;
    }

    policy->createSigningRequest = createSigningRequest_sp_eccnistp384;
    policy->updateCertificateAndPrivateKey = updateCertificateAndPrivateKey_sp_eccnistp384;
    policy->clear = UA_Policy_EccNistP384_Clear_Context;

    /* Use the same signature algorithm as the asymmetric component for
       certificate signing (see standard) */

    policy->certificateSigningAlgorithm =
        policy->asymmetricModule.cryptoModule.signatureAlgorithm;

    return UA_STATUSCODE_GOOD;
}

#endif
