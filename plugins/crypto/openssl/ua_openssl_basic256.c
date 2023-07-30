/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include "securitypolicy_openssl_common.h"

#include <openssl/x509.h>
#include <openssl/rand.h>

#define UA_SECURITYPOLICY_BASIC256SHA1_RSAPADDING_LEN                42
#define UA_SECURITYPOLICY_BASIC256_SYM_ENCRYPTION_KEY_LENGTH         32
#define UA_SECURITYPOLICY_BASIC256_SYM_ENCRYPTION_BLOCK_SIZE         16
#define UA_SECURITYPOLICY_BASIC256_SYM_SIGNING_KEY_LENGTH            24
#define UA_SHA1_LENGTH                                               20

static UA_StatusCode
UA_Policy_Basic256_New_Context (
	UA_SecurityPolicy* securityPolicy,
    const UA_Logger* logger
) {
    Policy_Context_openssl * context = (Policy_Context_openssl *)
                                    UA_malloc (sizeof (Policy_Context_openssl));
    if (context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    context->logger = logger;
    securityPolicy->policyContext = context;

    return UA_STATUSCODE_GOOD;
}

static void
UA_Policy_Basic256_Clear_Context (UA_SecurityPolicy *policy) {
    if(policy == NULL) {
        return;
    }

    Policy_Context_openssl *context = (Policy_Context_openssl*)policy->policyContext;
    if(context != NULL) {
        UA_free(context);

    }
    return;
}

/* create the channel context */

static UA_StatusCode
UA_ChannelModule_Basic256_New_Context(
	const UA_SecurityPolicy* securityPolicy,
	UA_PKIStore *pkiStore,
    const UA_ByteString* remoteCertificate,
    void ** channelContext
) {
    if (securityPolicy == NULL || remoteCertificate == NULL ||
        channelContext == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    Channel_Context_openssl * context = (Channel_Context_openssl *)
            UA_malloc (sizeof (Channel_Context_openssl));
    if (context == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    context->pkiStore = pkiStore;
    UA_StatusCode retval = UA_NodeId_copy(&securityPolicy->certificateTypeId, &context->certificateTypeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(context);
        return retval;
    }

    UA_ByteString_init(&context->localSymSigningKey);
    UA_ByteString_init(&context->localSymEncryptingKey);
    UA_ByteString_init(&context->localSymIv);
    UA_ByteString_init(&context->remoteSymSigningKey);
    UA_ByteString_init(&context->remoteSymEncryptingKey);
    UA_ByteString_init(&context->remoteSymIv);

    retval = UA_copyCertificate(&context->remoteCertificate, remoteCertificate);
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

    context->policyContext = (Policy_Context_openssl *)(securityPolicy->policyContext);
    *channelContext = context;

    UA_LOG_INFO (securityPolicy->logger,
                 UA_LOGCATEGORY_SECURITYPOLICY,
                 "The basic256 security policy channel with openssl is created.");

    return UA_STATUSCODE_GOOD;
}

/* delete the channel context */

static void
UA_ChannelModule_Basic256_Delete_Context (void * channelContext) {

    if (channelContext != NULL) {
        Channel_Context_openssl * cc = (Channel_Context_openssl *)
                                           channelContext;
        X509_free (cc->remoteCertificateX509);
        UA_ByteString_clear (&cc->remoteCertificate);
        UA_ByteString_clear (&cc->localSymSigningKey);
        UA_ByteString_clear (&cc->localSymEncryptingKey);
        UA_ByteString_clear (&cc->localSymIv);
        UA_ByteString_clear (&cc->remoteSymSigningKey);
        UA_ByteString_clear (&cc->remoteSymEncryptingKey);
        UA_ByteString_clear (&cc->remoteSymIv);
        UA_LOG_INFO (cc->policyContext->logger,
                 UA_LOGCATEGORY_SECURITYPOLICY,
                 "The basic256 security policy channel with openssl is deleted.");

        UA_free (cc);
    }
}

static UA_StatusCode
UA_ChannelModule_Basic256_setLocalSymSigningKey (void *                channelContext,
                                                 const UA_ByteString * key) {
    if (key == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    UA_ByteString_clear(&cc->localSymSigningKey);
    return UA_ByteString_copy(key, &cc->localSymSigningKey);
}

static UA_StatusCode
UA_ChannelModule_Basic256_setLocalSymEncryptingKey (void *                channelContext,
                                                    const UA_ByteString * key) {
    if (key == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    UA_ByteString_clear(&cc->localSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->localSymEncryptingKey);
}

static UA_StatusCode
UA_ChannelModule_Basic256_setLocalSymIv (void *                channelContext,
                                         const UA_ByteString * iv) {
    if (iv == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    UA_ByteString_clear(&cc->localSymIv);
    return UA_ByteString_copy(iv, &cc->localSymIv);
}

static UA_StatusCode
UA_ChannelModule_Basic256_setRemoteSymSigningKey (void *                channelContext,
                                                  const UA_ByteString * key) {
    if (key == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    UA_ByteString_clear(&cc->remoteSymSigningKey);
    return UA_ByteString_copy(key, &cc->remoteSymSigningKey);
}

static UA_StatusCode
UA_ChannelModule_Basic256_setRemoteSymEncryptingKey (void *                channelContext,
                                                     const UA_ByteString * key) {
    if (key == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    UA_ByteString_clear(&cc->remoteSymEncryptingKey);
    return UA_ByteString_copy(key, &cc->remoteSymEncryptingKey);
}

static UA_StatusCode
UA_ChannelModule_Basic256_setRemoteSymIv (void *                channelContext,
                                          const UA_ByteString * key) {
    if (key == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    UA_ByteString_clear(&cc->remoteSymIv);
    return UA_ByteString_copy(key, &cc->remoteSymIv);
}

static UA_StatusCode
UA_ChannelModule_Basic256_compareCertificate (const void *          channelContext,
                                              const UA_ByteString * certificate) {
    if(channelContext == NULL || certificate == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    const Channel_Context_openssl * cc =
                     (const Channel_Context_openssl *) channelContext;
    return UA_OpenSSL_X509_compare (certificate, cc->remoteCertificateX509);
}

static size_t
UA_AsySig_Basic256_getRemoteSignatureSize (const void *channelContext) {
    if (channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    const Channel_Context_openssl * cc = (const Channel_Context_openssl *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength (cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen;
}

static size_t
asySig_Basic256_getLocalSignatureSize(
	const Channel_Context_openssl* channelContext,
	EVP_PKEY* privateKey
) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(privateKey, &keyLen);
    return (size_t) keyLen;
}

static size_t
UA_AsySig_Basic256_getLocalSignatureSize(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    return channelContext_loadKeyThenGetSize(
    	(const Channel_Context_openssl*)channelContext,
		asySig_Basic256_getLocalSignatureSize
	);
}

static UA_StatusCode
UA_AsySig_Basic256_Verify (void *                    channelContext,
                           const UA_ByteString *     message,
                           const UA_ByteString *     signature) {
    if (message == NULL || signature == NULL || channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
        }

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    UA_StatusCode retval = UA_OpenSSL_RSA_PKCS1_V15_SHA1_Verify (message,
                            cc->remoteCertificateX509, signature);
    return retval;
}

static UA_StatusCode
asySig_Basic256_Sign(
	const Channel_Context_openssl* channelContext,
	const UA_ByteString* message,
    UA_ByteString* signature,
	EVP_PKEY* privateKey
) {
	if(channelContext == NULL || message == NULL || signature == NULL) {
		return UA_STATUSCODE_BADINTERNALERROR;
	}

	return UA_Openssl_RSA_PKCS1_V15_SHA1_Sign(message, privateKey, signature);
}

static UA_StatusCode
UA_AsySig_Basic256_Sign(void *channelContext, const UA_ByteString * message,
                              UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    return channelContext_loadKeyThenSign(
    	(const Channel_Context_openssl*) channelContext, message,
    	signature, asySig_Basic256_Sign
	);
}

static UA_StatusCode
UA_AsySig_Basic256_SignAuth(
	void *channelContext, const UA_ByteString *message, UA_ByteString *signature, UA_ByteString *privateKey
) {
	if(channelContext == NULL || message == NULL || signature == NULL || privateKey == NULL) {
	    return UA_STATUSCODE_BADINTERNALERROR;
	}
	return channelContext_parseKeyThenSign(
	    (const Channel_Context_openssl*)channelContext, message,
	    signature, privateKey, asySig_Basic256_Sign
	);
}

static size_t
UA_AsymEn_Basic256_getRemotePlainTextBlockSize (const void *channelContext) {
    if (channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    const Channel_Context_openssl * cc = (const Channel_Context_openssl *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength (cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen - UA_SECURITYPOLICY_BASIC256SHA1_RSAPADDING_LEN;
}

static size_t
UA_AsymEn_Basic256_getRemoteBlockSize (const void *channelContext) {
    if (channelContext == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    const Channel_Context_openssl * cc = (const Channel_Context_openssl *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength (cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen;
}

static size_t
UA_AsymEn_Basic256_getRemoteKeyLength (const void *channelContext) {
    if (channelContext == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    const Channel_Context_openssl * cc = (const Channel_Context_openssl *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength (cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen * 8;
}

static size_t
asymEn_Basic256_getLocalKeyLength(
	const Channel_Context_openssl* channelContext,
	EVP_PKEY* privateKey
) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(privateKey, &keyLen);
    return (size_t) keyLen * 8;
}

static size_t
UA_AsymEn_Basic256_getLocalKeyLength(const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    return channelContext_loadKeyThenGetSize(
    	(const Channel_Context_openssl*) channelContext,
		asymEn_Basic256_getLocalKeyLength
    );
}

static UA_StatusCode
asymEn_Basic256_Decrypt(
	const Channel_Context_openssl* channelContext,
	UA_ByteString* data,
	EVP_PKEY* privateKey
) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    return UA_Openssl_RSA_Oaep_Decrypt(data, privateKey);
}

static UA_StatusCode
UA_AsymEn_Basic256_Decrypt(void* channelContext, UA_ByteString* data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
   return channelContext_loadKeyThenDecrypt(
       (const Channel_Context_openssl*)channelContext, data, asymEn_Basic256_Decrypt
   );
}

static UA_StatusCode
UA_AsymEn_Basic256_Encrypt (void *                    channelContext,
                            UA_ByteString *           data) {
    if (channelContext == NULL || data == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    return UA_Openssl_RSA_OAEP_Encrypt (data, UA_SECURITYPOLICY_BASIC256SHA1_RSAPADDING_LEN,
                                        cc->remoteCertificateX509);
}

static UA_StatusCode
UA_Sym_Basic256_generateNonce(void *policyContext,
                              UA_ByteString *out) {
    UA_Int32 rc = RAND_bytes(out->data, (int) out->length);
    if (rc != 1) {
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Sym_Basic256_generateKey(void *policyContext, const UA_ByteString *secret,
                            const UA_ByteString *seed, UA_ByteString *out) {
    return UA_Openssl_Random_Key_PSHA1_Derive(secret, seed, out);
}

static size_t
UA_SymEn_Basic256_getLocalKeyLength (const void *              channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_BASIC256_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_Basic256_getBlockSize (const void *              channelContext) {
    return UA_SECURITYPOLICY_BASIC256_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
UA_SymEn_Basic256_getRemoteKeyLength (const void * channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_BASIC256_SYM_ENCRYPTION_KEY_LENGTH;
}

static UA_StatusCode
UA_SymEn_Basic256_Encrypt (void *                    channelContext,
                           UA_ByteString *           data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    return UA_OpenSSL_AES_256_CBC_Encrypt (&cc->localSymIv, &cc->localSymEncryptingKey, data);
}

static UA_StatusCode
UA_SymEn_Basic256_Decrypt (void *                    channelContext,
                           UA_ByteString *           data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    return UA_OpenSSL_AES_256_CBC_Decrypt (&cc->remoteSymIv, &cc->remoteSymEncryptingKey, data);
}

static size_t
UA_SymSig_Basic256_getKeyLength (const void *              channelContext) {
    return UA_SECURITYPOLICY_BASIC256_SYM_SIGNING_KEY_LENGTH;
}

static size_t
UA_SymSig_Basic256_getSignatureSize (const void *              channelContext) {
    return UA_SHA1_LENGTH;
}

static UA_StatusCode
UA_SymSig_Basic256_Verify (void *                    channelContext,
                           const UA_ByteString *     message,
                           const UA_ByteString *     signature) {
    if (channelContext == NULL ||
       message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    return UA_OpenSSL_HMAC_SHA1_Verify (message,
                                        &cc->remoteSymSigningKey,
                                        signature);
}

static UA_StatusCode
UA_SymSig_Basic256_Sign (void *                    channelContext,
                         const UA_ByteString *     message,
                         UA_ByteString *           signature) {
    if (channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    Channel_Context_openssl * cc = (Channel_Context_openssl *) channelContext;
    return UA_OpenSSL_HMAC_SHA1_Sign (message, &cc->localSymSigningKey, signature);
}

/* the main entry of Basic256 */

UA_StatusCode
UA_SecurityPolicy_Basic256 (UA_SecurityPolicy * policy,
                            const UA_Logger *   logger) {
    UA_SecurityPolicyAsymmetricModule * const asymmetricModule = &policy->asymmetricModule;
    UA_SecurityPolicySymmetricModule * const  symmetricModule = &policy->symmetricModule;
    UA_SecurityPolicyChannelModule * const    channelModule = &policy->channelModule;

    UA_LOG_INFO (logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "The basic256 security policy with openssl is added.");

    UA_Openssl_Init ();
    memset(policy, 0, sizeof(UA_SecurityPolicy));
    policy->logger = logger;
    policy->policyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256\0");
    policy->certificateTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE);

    /* set ChannelModule context  */
    channelModule->newContext = UA_ChannelModule_Basic256_New_Context;
    channelModule->deleteContext = UA_ChannelModule_Basic256_Delete_Context;

    channelModule->setLocalSymSigningKey = UA_ChannelModule_Basic256_setLocalSymSigningKey;
    channelModule->setLocalSymEncryptingKey = UA_ChannelModule_Basic256_setLocalSymEncryptingKey;
    channelModule->setLocalSymIv = UA_ChannelModule_Basic256_setLocalSymIv;
    channelModule->setRemoteSymSigningKey = UA_ChannelModule_Basic256_setRemoteSymSigningKey;
    channelModule->setRemoteSymEncryptingKey = UA_ChannelModule_Basic256_setRemoteSymEncryptingKey;
    channelModule->setRemoteSymIv = UA_ChannelModule_Basic256_setRemoteSymIv;
    channelModule->compareCertificate = UA_ChannelModule_Basic256_compareCertificate;

    /* asymmetricModule */

    asymmetricModule->compareCertificateThumbprint = UA_compareCertificateThumbprint;
    asymmetricModule->makeCertificateThumbprint = UA_makeCertificateThumbprint;

    /* AsymmetricModule - signature algorithm */

    UA_SecurityPolicySignatureAlgorithm * asySigAlgorithm =
        &asymmetricModule->cryptoModule.signatureAlgorithm;
    asySigAlgorithm->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#rsa-sha1\0");
    asySigAlgorithm->getRemoteSignatureSize = UA_AsySig_Basic256_getRemoteSignatureSize;
    asySigAlgorithm->getLocalSignatureSize = UA_AsySig_Basic256_getLocalSignatureSize;
    asySigAlgorithm->verify = UA_AsySig_Basic256_Verify;
    asySigAlgorithm->sign = UA_AsySig_Basic256_Sign;
    asySigAlgorithm->signAuth = UA_AsySig_Basic256_SignAuth;
    asySigAlgorithm->getLocalKeyLength = NULL;
    asySigAlgorithm->getRemoteKeyLength = NULL;

    /*  AsymmetricModule encryption algorithm */

    UA_SecurityPolicyEncryptionAlgorithm * asymEncryAlg =
        &asymmetricModule->cryptoModule.encryptionAlgorithm;
    asymEncryAlg->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#rsa-oaep\0");
    asymEncryAlg->getRemotePlainTextBlockSize = UA_AsymEn_Basic256_getRemotePlainTextBlockSize;
    asymEncryAlg->getRemoteBlockSize = UA_AsymEn_Basic256_getRemoteBlockSize;
    asymEncryAlg->getRemoteKeyLength = UA_AsymEn_Basic256_getRemoteKeyLength;
    asymEncryAlg->getLocalKeyLength = UA_AsymEn_Basic256_getLocalKeyLength;
    asymEncryAlg->decrypt = UA_AsymEn_Basic256_Decrypt;
    asymEncryAlg->encrypt = UA_AsymEn_Basic256_Encrypt;

    /* SymmetricModule */

    symmetricModule->secureChannelNonceLength = 32;
    symmetricModule->generateNonce = UA_Sym_Basic256_generateNonce;
    symmetricModule->generateKey = UA_Sym_Basic256_generateKey;

    /* Symmetric encryption Algorithm */

    UA_SecurityPolicyEncryptionAlgorithm * symEncryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    symEncryptionAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc\0");
    symEncryptionAlgorithm->getLocalKeyLength = UA_SymEn_Basic256_getLocalKeyLength;
    symEncryptionAlgorithm->getRemoteKeyLength = UA_SymEn_Basic256_getRemoteKeyLength;
    symEncryptionAlgorithm->getRemoteBlockSize = UA_SymEn_Basic256_getBlockSize;
    symEncryptionAlgorithm->getRemotePlainTextBlockSize = UA_SymEn_Basic256_getBlockSize;
    symEncryptionAlgorithm->decrypt = UA_SymEn_Basic256_Decrypt;
    symEncryptionAlgorithm->encrypt = UA_SymEn_Basic256_Encrypt;

    /* Symmetric signature Algorithm */

    UA_SecurityPolicySignatureAlgorithm * symSignatureAlgorithm =
        &symmetricModule->cryptoModule.signatureAlgorithm;
    symSignatureAlgorithm->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha1\0");
    symSignatureAlgorithm->getLocalKeyLength = UA_SymSig_Basic256_getKeyLength;
    symSignatureAlgorithm->getRemoteKeyLength = UA_SymSig_Basic256_getKeyLength;
    symSignatureAlgorithm->getRemoteSignatureSize = UA_SymSig_Basic256_getSignatureSize;
    symSignatureAlgorithm->getLocalSignatureSize = UA_SymSig_Basic256_getSignatureSize;
    symSignatureAlgorithm->verify = UA_SymSig_Basic256_Verify;
    symSignatureAlgorithm->sign = UA_SymSig_Basic256_Sign;

    UA_StatusCode retval = UA_Policy_Basic256_New_Context (policy, logger);
    if (retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    policy->getLocalCertificate = UA_OpenSSL_LoadLocalCertificate;
    policy->clear = UA_Policy_Basic256_Clear_Context;

    /* Use the same signature algorithm as the asymmetric component for
         * certificate signing (see standard) */
    policy->certificateSigningAlgorithm = policy->asymmetricModule.cryptoModule.signatureAlgorithm;

    return UA_STATUSCODE_GOOD;
}

#endif
