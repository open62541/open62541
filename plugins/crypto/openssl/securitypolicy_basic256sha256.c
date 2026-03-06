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

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#define UA_SHA256_LENGTH 32    /* 256 bit */
#define UA_SECURITYPOLICY_BASIC256SHA256_RSAPADDING_LEN 42
#define UA_SECURITYPOLICY_BASIC256SHA256_SYM_SIGNING_KEY_LENGTH 32
#define UA_SECURITYPOLICY_BASIC256SHA256_SYM_ENCRYPTION_KEY_LENGTH 32
#define UA_SECURITYPOLICY_BASIC256SHA256_SYM_ENCRYPTION_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC256SHA256_SYM_PLAIN_TEXT_BLOCK_SIZE 16
#define UA_SECURITYPOLICY_BASIC256SHA256_MINASYMKEYLENGTH 256
#define UA_SECURITYPOLICY_BASIC256SHA256_MAXASYMKEYLENGTH 512

static UA_StatusCode
createSigningRequest_sp_basic256sha256(UA_SecurityPolicy *securityPolicy,
                                       const UA_String *subjectName,
                                       const UA_ByteString *nonce,
                                       const UA_KeyValueMap *params,
                                       UA_ByteString *csr,
                                       UA_ByteString *newPrivateKey) {
    if(securityPolicy == NULL || csr == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(securityPolicy->policyContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
            (openssl_PolicyContext *) securityPolicy->policyContext;
    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey, &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName, nonce,
                                           csr, newPrivateKey);
}

static UA_StatusCode
New_Context(const UA_SecurityPolicy * securityPolicy,
            const UA_ByteString * remoteCertificate,
            void ** channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    openssl_ChannelContext *context = (openssl_ChannelContext *)
        UA_malloc(sizeof(openssl_ChannelContext));
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
        UA_OpenSSL_LoadCertificate(&context->remoteCertificate, EVP_PKEY_RSA);
    if(context->remoteCertificateX509 == NULL) {
        UA_ByteString_clear(&context->remoteCertificate);
        UA_free(context);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    *channelContext = context;

    return UA_STATUSCODE_GOOD;
}

static void
Delete_Context(const UA_SecurityPolicy *policy,
               void *channelContext) {
    if(!channelContext)
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
    UA_free(cc);
}

static UA_StatusCode
UA_AsySig_Basic256Sha256_Verify(const UA_SecurityPolicy *policy, void *channelContext,
                                const UA_ByteString *message,
                                const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext * cc =
        (openssl_ChannelContext *) channelContext;
    return UA_OpenSSL_RSA_PKCS1_V15_SHA256_Verify(message, cc->remoteCertificateX509,
                                                  signature);
}

static UA_StatusCode
UA_makeCertificateThumbprint(const UA_SecurityPolicy *policy,
                             const UA_ByteString *certificate,
                             UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

static UA_StatusCode
UA_Asym_Basic256Sha256_Decrypt(const UA_SecurityPolicy *policy,
                               void *channelContext, UA_ByteString *data) {
    if(data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    return UA_Openssl_RSA_Oaep_Decrypt(data, pc->localPrivateKey);
}

static size_t
UA_Asym_Basic256Sha256_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                              const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_ChannelContext * cc =
        (const openssl_ChannelContext *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen;
}

static size_t
UA_AsySig_Basic256Sha256_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t) keyLen;
}

static size_t
UA_AsymEn_Basic256Sha256_getRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                                     const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen - UA_SECURITYPOLICY_BASIC256SHA256_RSAPADDING_LEN;
}

static size_t
UA_AsymEn_Basic256Sha256_getRemoteBlockSize(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_ChannelContext * cc =
        (const openssl_ChannelContext *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen;
}

static size_t
UA_AsymEn_Basic256Sha256_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *) channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t) keyLen * 8;
}

static UA_StatusCode
UA_Sym_Basic256Sha256_generateNonce(const UA_SecurityPolicy *policy,
                                    void *channelContext, UA_ByteString *out) {
    UA_Int32 rc = RAND_bytes(out->data, (int) out->length);
    if(rc != 1)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static size_t
UA_SymEn_Basic256Sha256_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                          const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_BASIC256SHA256_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymSig_Basic256Sha256_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_BASIC256SHA256_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_Sym_Basic256Sha256_generateKey(const UA_SecurityPolicy *policy,
                                  void *channelContext, const UA_ByteString *secret,
                                  const UA_ByteString *seed, UA_ByteString *out) {
    return UA_Openssl_Random_Key_PSHA256_Derive(secret, seed, out);
}


static size_t
UA_SymEn_Basic256Sha256_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_BASIC256SHA256_SYM_ENCRYPTION_KEY_LENGTH;
}

static size_t
UA_SymEn_Basic256Sha256_getBlockSize(const UA_SecurityPolicy *policy,
                                     const void *channelContext) {
    return UA_SECURITYPOLICY_BASIC256SHA256_SYM_ENCRYPTION_BLOCK_SIZE;
}

static size_t
UA_SymSig_Basic256Sha256_getRemoteKeyLength(const UA_SecurityPolicy *policy,
                                            const void *channelContext) {
    /* 32 bytes 256 bits */
    return UA_SECURITYPOLICY_BASIC256SHA256_SYM_SIGNING_KEY_LENGTH;
}

static UA_StatusCode
UA_AsySig_Basic256Sha256_sign(const UA_SecurityPolicy *policy,
                              void *channelContext, const UA_ByteString *message,
                              UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    return UA_Openssl_RSA_PKCS1_V15_SHA256_Sign(message, pc->localPrivateKey, signature);
}

static UA_StatusCode
UA_AsymEn_Basic256Sha256_encrypt(const UA_SecurityPolicy *policy,
                                 void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext * cc =
        (openssl_ChannelContext *) channelContext;
    return UA_Openssl_RSA_OAEP_Encrypt(data,
                                       UA_SECURITYPOLICY_BASIC256SHA256_RSAPADDING_LEN,
                                       cc->remoteCertificateX509);
}

static size_t
UA_SymSig_Basic256Sha256_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                                                const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymSig_Basic256Sha256_verify(const UA_SecurityPolicy *policy, void *channelContext,
                                const UA_ByteString *message,
                                const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext*)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Verify(message, &cc->remoteSymSigningKey, signature);
}

static UA_StatusCode
UA_SymSig_Basic256Sha256_sign(const UA_SecurityPolicy *policy,
                              void *channelContext, const UA_ByteString *message,
                              UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext*)channelContext;
    return UA_OpenSSL_HMAC_SHA256_Sign(message, &cc->localSymSigningKey, signature);
}

static size_t
UA_SymSig_Basic256Sha256_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                                               const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static UA_StatusCode
UA_SymEn_Basic256Sha256_decrypt(const UA_SecurityPolicy *policy,
                                void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext*)channelContext;
    return UA_OpenSSL_AES_256_CBC_Decrypt(&cc->remoteSymIv,
                                          &cc->remoteSymEncryptingKey, data);
}

static UA_StatusCode
UA_SymEn_Basic256Sha256_encrypt(const UA_SecurityPolicy *policy,
                                void *channelContext, UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc =
        (openssl_ChannelContext *) channelContext;
    return UA_OpenSSL_AES_256_CBC_Encrypt(&cc->localSymIv,
                                          &cc->localSymEncryptingKey, data);
}

static size_t
UA_AsymEn_Basic256Sha256_getLocalKeyLength(const UA_SecurityPolicy *policy,
                                           const void *channelContext) {
    if(channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_PolicyContext *pc =
        (openssl_PolicyContext *)policy->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t) keyLen * 8;
}

UA_StatusCode
UA_SecurityPolicy_Basic256Sha256(UA_SecurityPolicy *sp,
                                 const UA_ByteString localCertificate,
                                 const UA_ByteString localPrivateKey,
                                 const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256\0");
    sp->certificateGroupId = UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NS0ID(RSASHA256APPLICATIONCERTIFICATETYPE);
    sp->securityLevel = 20;
    sp->policyType = UA_SECURITYPOLICYTYPE_RSA;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = UA_STRING("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256\0");
    asymSig->verify = UA_AsySig_Basic256Sha256_Verify;
    asymSig->sign = UA_AsySig_Basic256Sha256_sign;
    asymSig->getLocalSignatureSize = UA_AsySig_Basic256Sha256_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = UA_Asym_Basic256Sha256_getRemoteSignatureSize;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#rsa-oaep\0");
    asymEnc->encrypt = UA_AsymEn_Basic256Sha256_encrypt;
    asymEnc->decrypt = UA_Asym_Basic256Sha256_Decrypt;
    asymEnc->getLocalKeyLength = UA_AsymEn_Basic256Sha256_getLocalKeyLength;
    asymEnc->getRemoteKeyLength = UA_AsymEn_Basic256Sha256_getRemoteKeyLength;
    asymEnc->getRemoteBlockSize = UA_AsymEn_Basic256Sha256_getRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = UA_AsymEn_Basic256Sha256_getRemotePlainTextBlockSize;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = UA_STRING("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256\0");
    symSig->verify = UA_SymSig_Basic256Sha256_verify;
    symSig->sign = UA_SymSig_Basic256Sha256_sign;
    symSig->getLocalSignatureSize = UA_SymSig_Basic256Sha256_getLocalSignatureSize;
    symSig->getRemoteSignatureSize = UA_SymSig_Basic256Sha256_getRemoteSignatureSize;
    symSig->getLocalKeyLength = UA_SymSig_Basic256Sha256_getLocalKeyLength;
    symSig->getRemoteKeyLength = UA_SymSig_Basic256Sha256_getRemoteKeyLength;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#aes256-cbc\0");
    symEnc->encrypt = UA_SymEn_Basic256Sha256_encrypt;
    symEnc->decrypt = UA_SymEn_Basic256Sha256_decrypt;
    symEnc->getLocalKeyLength = UA_SymEn_Basic256Sha256_getLocalKeyLength;
    symEnc->getRemoteKeyLength = UA_SymEn_Basic256Sha256_getRemoteKeyLength;
    symEnc->getRemoteBlockSize = UA_SymEn_Basic256Sha256_getBlockSize;
    symEnc->getRemotePlainTextBlockSize = UA_SymEn_Basic256Sha256_getBlockSize;

    /* Use the same signature algorithm as the asymmetric component for
     * certificate signing (see standard) */
    sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;

    /* Direct Method Pointers */
    sp->newChannelContext = New_Context;
    sp->deleteChannelContext = Delete_Context;
    sp->setLocalSymEncryptingKey = UA_OpenSSL_setLocalSymEncryptingKey_generic;
    sp->setLocalSymSigningKey = UA_OpenSSL_setLocalSymSigningKey_generic;
    sp->setLocalSymIv = UA_OpenSSL_setLocalSymIv_generic;
    sp->setRemoteSymEncryptingKey = UA_OpenSSL_setRemoteSymEncryptingKey_generic;
    sp->setRemoteSymSigningKey = UA_OpenSSL_setRemoteSymSigningKey_generic;
    sp->setRemoteSymIv = UA_OpenSSL_setRemoteSymIv_generic;
    sp->compareCertificate = UA_OpenSSL_compareCertificate_generic;
    sp->generateKey = UA_Sym_Basic256Sha256_generateKey;
    sp->generateNonce = UA_Sym_Basic256Sha256_generateNonce;
    sp->nonceLength = 32;
    sp->makeCertThumbprint = UA_makeCertificateThumbprint;
    sp->compareCertThumbprint = UA_OpenSSL_SecurityPolicy_compareCertThumbprint_generic;
    sp->updateCertificate = UA_OpenSSL_SecurityPolicy_updateCertificate_generic;
    sp->createSigningRequest = createSigningRequest_sp_basic256sha256;
    sp->clear = UA_OpenSSL_Policy_clearContext_generic;

    /* Parse the certificate */
    UA_Openssl_Init();
    UA_StatusCode res =
        UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate, EVP_PKEY_RSA);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = UA_OpenSSL_Policy_newContext_generic(sp, localPrivateKey, EVP_PKEY_RSA, logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

#endif
