/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2020 (c) Wind River Systems, Inc.
 *    Copyright 2020 (c) basysKom GmbH
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 *    Copyright 2026 (c) Andreas Hellenbrand
 *
 * The current RSA SecurityPolicies of OPC UA Part 7, implemented once:
 *
 *   Basic256Sha256, Aes128_Sha256_RsaOaep, Aes256_Sha256_RsaPss
 *
 * They differ only in which OpenSSL primitive each slot calls and in a handful
 * of lengths - there is no behavioural branch anywhere - so each policy is a
 * UA_RsaParams row and a constructor that hands that row to rsaPolicy().
 * The channel context lifecycle, the symmetric key setters, the certificate
 * update and the CSR were already shared through the *_generic helpers in
 * securitypolicy_common.c; this module adds the crypto slots to that.
 *
 * The deprecated Basic128Rsa15 and Basic256 stay hand-written: they need SHA-1
 * variants that nothing here would otherwise pull in, and keeping them out
 * leaves this module describing only what Part 7 still endorses.
 *
 * The policy context is the shared openssl_PolicyContext; the parameter row
 * travels in its rsaParams field.
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

/**************/
/* Parameters */
/**************/

/* What distinguishes the RSA SecurityPolicies from each other */
typedef struct {
    UA_String policyUri;
    UA_UInt32 certificateTypeId;   /* numeric identifier in namespace 0 */
    UA_Byte securityLevel;
    size_t nonceLength;

    /* Asymmetric signature, used for the message signatures */
    UA_String asymSigUri;
    UA_StatusCode (*asymSign)(const UA_ByteString *message, EVP_PKEY *privateKey,
                              UA_ByteString *signature);
    UA_StatusCode (*asymVerify)(const UA_ByteString *message, X509 *certificate,
                                const UA_ByteString *signature);

    /* Certificate signature. Aes256_Sha256_RsaPss signs messages with PSS but
     * certificates with PKCS#1 v1.5, so it cannot reuse the algorithm above.
     * A zero-length certSigUri means "the same as the asymmetric one". */
    UA_String certSigUri;
    UA_StatusCode (*certSign)(const UA_ByteString *message, EVP_PKEY *privateKey,
                              UA_ByteString *signature);
    UA_StatusCode (*certVerify)(const UA_ByteString *message, X509 *certificate,
                                const UA_ByteString *signature);

    /* Asymmetric encryption */
    UA_String asymEncUri;
    UA_StatusCode (*asymEncrypt)(UA_ByteString *data, size_t paddingSize,
                                 X509 *publicX509);
    UA_StatusCode (*asymDecrypt)(UA_ByteString *data, EVP_PKEY *privateKey);
    size_t rsaPaddingLength;       /* 42 for OAEP-SHA1, 66 for OAEP-SHA2 */

    /* Symmetric signature (HMAC) */
    UA_String symSigUri;
    UA_StatusCode (*symSign)(const UA_ByteString *message, const UA_ByteString *key,
                             UA_ByteString *signature);
    UA_StatusCode (*symVerify)(const UA_ByteString *message, const UA_ByteString *key,
                               const UA_ByteString *signature);
    size_t symSignatureLength;
    size_t symSigningKeyLength;

    /* Symmetric encryption (AES-CBC) */
    UA_String symEncUri;
    UA_StatusCode (*symEncrypt)(const UA_ByteString *iv, const UA_ByteString *key,
                                UA_ByteString *data);
    UA_StatusCode (*symDecrypt)(const UA_ByteString *iv, const UA_ByteString *key,
                                UA_ByteString *data);
    size_t symEncryptionKeyLength;
    size_t symEncryptionBlockSize;
} UA_RsaParams;

#define UA_SHA256_LENGTH 32    /* 256 bit */
#define UA_AES_CBC_BLOCK_SIZE 16
#define UA_RSA_OAEP_SHA1_PADDING_LEN 42
#define UA_RSA_OAEP_SHA2_PADDING_LEN 66

/* http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256 */
static const UA_RsaParams basic256Sha256Params = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256"),
    UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE, 20, 32,
    /* Asymmetric signature: RSA PKCS#1 v1.5 with SHA-256 */
    UA_STRING_STATIC("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256"),
    UA_Openssl_RSA_PKCS1_V15_SHA256_Sign, UA_OpenSSL_RSA_PKCS1_V15_SHA256_Verify,
    /* Certificate signature: same as the asymmetric one */
    UA_STRING_STATIC(""), NULL, NULL,
    /* Asymmetric encryption: RSA OAEP with SHA-1 */
    UA_STRING_STATIC("http://www.w3.org/2001/04/xmlenc#rsa-oaep"),
    UA_Openssl_RSA_OAEP_Encrypt, UA_Openssl_RSA_Oaep_Decrypt,
    UA_RSA_OAEP_SHA1_PADDING_LEN,
    /* Symmetric signature: HMAC-SHA-256 */
    UA_STRING_STATIC("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256"),
    UA_OpenSSL_HMAC_SHA256_Sign, UA_OpenSSL_HMAC_SHA256_Verify,
    UA_SHA256_LENGTH, 32,
    /* Symmetric encryption: AES-256-CBC */
    UA_STRING_STATIC("http://www.w3.org/2001/04/xmlenc#aes256-cbc"),
    UA_OpenSSL_AES_256_CBC_Encrypt, UA_OpenSSL_AES_256_CBC_Decrypt,
    32, UA_AES_CBC_BLOCK_SIZE
};

/* http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep */
static const UA_RsaParams aes128Sha256RsaOaepParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep"),
    UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE, 10, 32,
    /* Asymmetric signature: RSA PKCS#1 v1.5 with SHA-256 */
    UA_STRING_STATIC("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256"),
    UA_Openssl_RSA_PKCS1_V15_SHA256_Sign, UA_OpenSSL_RSA_PKCS1_V15_SHA256_Verify,
    /* Certificate signature: same as the asymmetric one */
    UA_STRING_STATIC(""), NULL, NULL,
    /* Asymmetric encryption: RSA OAEP with SHA-1 */
    UA_STRING_STATIC("http://www.w3.org/2001/04/xmlenc#rsa-oaep"),
    UA_Openssl_RSA_OAEP_Encrypt, UA_Openssl_RSA_Oaep_Decrypt,
    UA_RSA_OAEP_SHA1_PADDING_LEN,
    /* Symmetric signature: HMAC-SHA-256 */
    UA_STRING_STATIC("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256"),
    UA_OpenSSL_HMAC_SHA256_Sign, UA_OpenSSL_HMAC_SHA256_Verify,
    UA_SHA256_LENGTH, 32,
    /* Symmetric encryption: AES-128-CBC */
    UA_STRING_STATIC("http://www.w3.org/2001/04/xmlenc#aes128-cbc"),
    UA_OpenSSL_AES_128_CBC_Encrypt, UA_OpenSSL_AES_128_CBC_Decrypt,
    16, UA_AES_CBC_BLOCK_SIZE
};

/* http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss */
static const UA_RsaParams aes256Sha256RsaPssParams = {
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss"),
    UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE, 30, 32,
    /* Asymmetric signature: RSA-PSS with SHA-256 */
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/rsa-pss-sha2-256"),
    UA_Openssl_RSA_PSS_SHA256_Sign, UA_OpenSSL_RSA_PSS_SHA256_Verify,
    /* Certificate signature: PKCS#1 v1.5, not PSS */
    UA_STRING_STATIC("http://www.w3.org/2001/04/xmldsig-more#rsa-sha256"),
    UA_Openssl_RSA_PKCS1_V15_SHA256_Sign, UA_OpenSSL_RSA_PKCS1_V15_SHA256_Verify,
    /* Asymmetric encryption: RSA OAEP with SHA-256 */
    UA_STRING_STATIC("http://opcfoundation.org/UA/security/rsa-oaep-sha2-256"),
    UA_Openssl_RSA_OAEP_SHA2_Encrypt, UA_Openssl_RSA_Oaep_Sha2_Decrypt,
    UA_RSA_OAEP_SHA2_PADDING_LEN,
    /* Symmetric signature: HMAC-SHA-256 */
    UA_STRING_STATIC("http://www.w3.org/2000/09/xmldsig#hmac-sha2-256"),
    UA_OpenSSL_HMAC_SHA256_Sign, UA_OpenSSL_HMAC_SHA256_Verify,
    UA_SHA256_LENGTH, 32,
    /* Symmetric encryption: AES-256-CBC */
    UA_STRING_STATIC("http://www.w3.org/2001/04/xmlenc#aes256-cbc"),
    UA_OpenSSL_AES_256_CBC_Encrypt, UA_OpenSSL_AES_256_CBC_Decrypt,
    32, UA_AES_CBC_BLOCK_SIZE
};

/* The parameter row of a policy */
static const UA_RsaParams *
rsa_params(const UA_SecurityPolicy *policy) {
    if(!policy || !policy->policyContext)
        return NULL;
    return (const UA_RsaParams *)
        ((const openssl_PolicyContext *)policy->policyContext)->rsaParams;
}

/*******************/
/* Channel Context */
/*******************/

static UA_StatusCode
rsa_newChannelContext(const UA_SecurityPolicy *securityPolicy,
                      const UA_ByteString *remoteCertificate,
                      void **channelContext) {
    if(securityPolicy == NULL || remoteCertificate == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    openssl_ChannelContext *context = (openssl_ChannelContext *)
        UA_calloc(1, sizeof(openssl_ChannelContext));
    if(context == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

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
        return UA_STATUSCODE_BADCERTIFICATECHAININCOMPLETE;
    }

    *channelContext = context;

    return UA_STATUSCODE_GOOD;
}

static void
rsa_deleteChannelContext(const UA_SecurityPolicy *policy, void *channelContext) {
    if(!channelContext)
        return;

    openssl_ChannelContext *cc = (openssl_ChannelContext *)channelContext;
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

/*************************/
/* Asymmetric Signature  */
/*************************/

static UA_StatusCode
rsa_asymSign(const UA_SecurityPolicy *policy, void *channelContext,
             const UA_ByteString *message, UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_PolicyContext *pc =
        (const openssl_PolicyContext *)policy->policyContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!pc || !params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->asymSign(message, pc->localPrivateKey, signature);
}

static UA_StatusCode
rsa_asymVerify(const UA_SecurityPolicy *policy, void *channelContext,
               const UA_ByteString *message, const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext *)channelContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->asymVerify(message, cc->remoteCertificateX509, signature);
}

/* The certificate signature of a policy whose certSign/certVerify differ from
 * its message signature (Aes256_Sha256_RsaPss). */

static UA_StatusCode
rsa_certSign(const UA_SecurityPolicy *policy, void *channelContext,
             const UA_ByteString *message, UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    const openssl_PolicyContext *pc =
        (const openssl_PolicyContext *)policy->policyContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!pc || !params || !params->certSign)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->certSign(message, pc->localPrivateKey, signature);
}

static UA_StatusCode
rsa_certVerify(const UA_SecurityPolicy *policy, void *channelContext,
               const UA_ByteString *message, const UA_ByteString *signature) {
    if(message == NULL || signature == NULL || channelContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext *)channelContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!params || !params->certVerify)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->certVerify(message, cc->remoteCertificateX509, signature);
}

/* Both signature slots are RSA over the local/remote key, so their sizes come
 * from the key length and are shared by the message and certificate slots. */

static size_t
rsa_getLocalSignatureSize(const UA_SecurityPolicy *policy,
                          const void *channelContext) {
    if(channelContext == NULL || policy == NULL || policy->policyContext == NULL)
        return 0;
    const openssl_PolicyContext *pc =
        (const openssl_PolicyContext *)policy->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t)keyLen;
}

static size_t
rsa_getRemoteSignatureSize(const UA_SecurityPolicy *policy,
                           const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen;
}

/**************************/
/* Asymmetric Encryption  */
/**************************/

static UA_StatusCode
rsa_asymEncrypt(const UA_SecurityPolicy *policy, void *channelContext,
                UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext *)channelContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->asymEncrypt(data, params->rsaPaddingLength,
                               cc->remoteCertificateX509);
}

static UA_StatusCode
rsa_asymDecrypt(const UA_SecurityPolicy *policy, void *channelContext,
                UA_ByteString *data) {
    if(data == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const openssl_PolicyContext *pc =
        (const openssl_PolicyContext *)policy->policyContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!pc || !params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->asymDecrypt(data, pc->localPrivateKey);
}

static size_t
rsa_asymGetLocalKeyLength(const UA_SecurityPolicy *policy,
                          const void *channelContext) {
    if(channelContext == NULL || policy == NULL || policy->policyContext == NULL)
        return 0;
    const openssl_PolicyContext *pc =
        (const openssl_PolicyContext *)policy->policyContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Private_GetKeyLength(pc->localPrivateKey, &keyLen);
    return (size_t)keyLen * 8;
}

static size_t
rsa_asymGetRemoteKeyLength(const UA_SecurityPolicy *policy,
                           const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen * 8;
}

static size_t
rsa_asymGetRemoteBlockSize(const UA_SecurityPolicy *policy,
                           const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *)channelContext;
    UA_Int32 keyLen = 0;
    UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    return (size_t)keyLen;
}

static size_t
rsa_asymGetRemotePlainTextBlockSize(const UA_SecurityPolicy *policy,
                                    const void *channelContext) {
    if(channelContext == NULL)
        return 0;
    const openssl_ChannelContext *cc =
        (const openssl_ChannelContext *)channelContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!params)
        return 0;
    UA_Int32 keyLen = 0;
    UA_StatusCode retval =
        UA_Openssl_RSA_Public_GetKeyLength(cc->remoteCertificateX509, &keyLen);
    if(retval != UA_STATUSCODE_GOOD ||
       keyLen <= (UA_Int32)params->rsaPaddingLength)
        return 0;
    return (size_t)keyLen - params->rsaPaddingLength;
}

/*************************/
/* Symmetric Signature   */
/*************************/

static UA_StatusCode
rsa_symSign(const UA_SecurityPolicy *policy, void *channelContext,
            const UA_ByteString *message, UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext *)channelContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->symSign(message, &cc->localSymSigningKey, signature);
}

static UA_StatusCode
rsa_symVerify(const UA_SecurityPolicy *policy, void *channelContext,
              const UA_ByteString *message, const UA_ByteString *signature) {
    if(channelContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext *)channelContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->symVerify(message, &cc->remoteSymSigningKey, signature);
}

static size_t
rsa_symGetSignatureSize(const UA_SecurityPolicy *policy,
                        const void *channelContext) {
    const UA_RsaParams *params = rsa_params(policy);
    return params ? params->symSignatureLength : 0;
}

static size_t
rsa_symGetSigningKeyLength(const UA_SecurityPolicy *policy,
                           const void *channelContext) {
    const UA_RsaParams *params = rsa_params(policy);
    return params ? params->symSigningKeyLength : 0;
}

/*************************/
/* Symmetric Encryption  */
/*************************/

static UA_StatusCode
rsa_symEncrypt(const UA_SecurityPolicy *policy, void *channelContext,
               UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext *)channelContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->symEncrypt(&cc->localSymIv, &cc->localSymEncryptingKey, data);
}

static UA_StatusCode
rsa_symDecrypt(const UA_SecurityPolicy *policy, void *channelContext,
               UA_ByteString *data) {
    if(channelContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    openssl_ChannelContext *cc = (openssl_ChannelContext *)channelContext;
    const UA_RsaParams *params = rsa_params(policy);
    if(!params)
        return UA_STATUSCODE_BADINTERNALERROR;
    return params->symDecrypt(&cc->remoteSymIv, &cc->remoteSymEncryptingKey, data);
}

static size_t
rsa_symGetEncryptionKeyLength(const UA_SecurityPolicy *policy,
                              const void *channelContext) {
    const UA_RsaParams *params = rsa_params(policy);
    return params ? params->symEncryptionKeyLength : 0;
}

static size_t
rsa_symGetBlockSize(const UA_SecurityPolicy *policy,
                    const void *channelContext) {
    const UA_RsaParams *params = rsa_params(policy);
    return params ? params->symEncryptionBlockSize : 0;
}

/**************************/
/* Nonce and Derivation   */
/**************************/

static UA_StatusCode
rsa_generateNonce(const UA_SecurityPolicy *policy,
                  void *channelContext, UA_ByteString *out) {
    UA_Int32 rc = RAND_bytes(out->data, (int)out->length);
    if(rc != 1)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
rsa_generateKey(const UA_SecurityPolicy *policy, void *channelContext,
                const UA_ByteString *secret, const UA_ByteString *seed,
                UA_ByteString *out) {
    return UA_Openssl_Random_Key_PSHA256_Derive(secret, seed, out);
}

static UA_StatusCode
rsa_makeCertificateThumbprint(const UA_SecurityPolicy *policy,
                              const UA_ByteString *certificate,
                              UA_ByteString *thumbprint) {
    return UA_Openssl_X509_GetCertificateThumbprint(certificate, thumbprint, false);
}

static UA_StatusCode
rsa_createSigningRequest(UA_SecurityPolicy *securityPolicy,
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
        (openssl_PolicyContext *)securityPolicy->policyContext;
    return UA_OpenSSL_CreateSigningRequest(pc->localPrivateKey, &pc->csrLocalPrivateKey,
                                           securityPolicy, subjectName, nonce,
                                           csr, newPrivateKey);
}

/***************/
/* Constructor */
/***************/

static UA_StatusCode
rsaPolicy(UA_SecurityPolicy *sp, const UA_RsaParams *params,
          const UA_ByteString localCertificate,
          const UA_ByteString localPrivateKey,
          const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_SecurityPolicy));
    sp->logger = logger;
    sp->policyUri = params->policyUri;
    sp->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    sp->certificateTypeId = UA_NODEID_NUMERIC(0, params->certificateTypeId);
    sp->securityLevel = params->securityLevel;
    sp->policyType = UA_SECURITYPOLICYTYPE_RSA;

    /* Asymmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *asymSig = &sp->asymSignatureAlgorithm;
    asymSig->uri = params->asymSigUri;
    asymSig->verify = rsa_asymVerify;
    asymSig->sign = rsa_asymSign;
    asymSig->getLocalSignatureSize = rsa_getLocalSignatureSize;
    asymSig->getRemoteSignatureSize = rsa_getRemoteSignatureSize;

    /* Asymmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *asymEnc = &sp->asymEncryptionAlgorithm;
    asymEnc->uri = params->asymEncUri;
    asymEnc->encrypt = rsa_asymEncrypt;
    asymEnc->decrypt = rsa_asymDecrypt;
    asymEnc->getLocalKeyLength = rsa_asymGetLocalKeyLength;
    asymEnc->getRemoteKeyLength = rsa_asymGetRemoteKeyLength;
    asymEnc->getRemoteBlockSize = rsa_asymGetRemoteBlockSize;
    asymEnc->getRemotePlainTextBlockSize = rsa_asymGetRemotePlainTextBlockSize;

    /* Symmetric Signature */
    UA_SecurityPolicySignatureAlgorithm *symSig = &sp->symSignatureAlgorithm;
    symSig->uri = params->symSigUri;
    symSig->verify = rsa_symVerify;
    symSig->sign = rsa_symSign;
    symSig->getLocalSignatureSize = rsa_symGetSignatureSize;
    symSig->getRemoteSignatureSize = rsa_symGetSignatureSize;
    symSig->getLocalKeyLength = rsa_symGetSigningKeyLength;
    symSig->getRemoteKeyLength = rsa_symGetSigningKeyLength;

    /* Symmetric Encryption */
    UA_SecurityPolicyEncryptionAlgorithm *symEnc = &sp->symEncryptionAlgorithm;
    symEnc->uri = params->symEncUri;
    symEnc->encrypt = rsa_symEncrypt;
    symEnc->decrypt = rsa_symDecrypt;
    symEnc->getLocalKeyLength = rsa_symGetEncryptionKeyLength;
    symEnc->getRemoteKeyLength = rsa_symGetEncryptionKeyLength;
    symEnc->getRemoteBlockSize = rsa_symGetBlockSize;
    symEnc->getRemotePlainTextBlockSize = rsa_symGetBlockSize;

    /* Certificate signature. Unless the policy names its own, it is the same
     * algorithm as the asymmetric component (see standard). */
    if(params->certSigUri.length == 0) {
        sp->certSignatureAlgorithm = sp->asymSignatureAlgorithm;
    } else {
        UA_SecurityPolicySignatureAlgorithm *certSig = &sp->certSignatureAlgorithm;
        certSig->uri = params->certSigUri;
        certSig->verify = rsa_certVerify;
        certSig->sign = rsa_certSign;
        certSig->getLocalSignatureSize = rsa_getLocalSignatureSize;
        certSig->getRemoteSignatureSize = rsa_getRemoteSignatureSize;
    }

    /* Direct Method Pointers */
    sp->newChannelContext = rsa_newChannelContext;
    sp->deleteChannelContext = rsa_deleteChannelContext;
    sp->setLocalSymEncryptingKey = UA_OpenSSL_setLocalSymEncryptingKey_generic;
    sp->setLocalSymSigningKey = UA_OpenSSL_setLocalSymSigningKey_generic;
    sp->setLocalSymIv = UA_OpenSSL_setLocalSymIv_generic;
    sp->setRemoteSymEncryptingKey = UA_OpenSSL_setRemoteSymEncryptingKey_generic;
    sp->setRemoteSymSigningKey = UA_OpenSSL_setRemoteSymSigningKey_generic;
    sp->setRemoteSymIv = UA_OpenSSL_setRemoteSymIv_generic;
    sp->compareCertificate = UA_OpenSSL_compareCertificate_generic;
    sp->generateKey = rsa_generateKey;
    sp->generateNonce = rsa_generateNonce;
    sp->nonceLength = params->nonceLength;
    sp->makeCertThumbprint = rsa_makeCertificateThumbprint;
    sp->compareCertThumbprint = UA_OpenSSL_SecurityPolicy_compareCertThumbprint_generic;
    sp->updateCertificate = UA_OpenSSL_SecurityPolicy_updateCertificate_generic;
    sp->createSigningRequest = rsa_createSigningRequest;
    sp->clear = UA_OpenSSL_Policy_clearContext_generic;

    /* Parse the certificate */
    UA_Openssl_Init();
    UA_StatusCode res =
        UA_OpenSSL_LoadLocalCertificate(&localCertificate, &sp->localCertificate,
                                        EVP_PKEY_RSA);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Create the policy context */
    res = UA_OpenSSL_Policy_newContext_generic(sp, localPrivateKey, EVP_PKEY_RSA,
                                               logger);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&sp->localCertificate);
        return res;
    }

    /* Every callback above reads its parameters from here */
    ((openssl_PolicyContext *)sp->policyContext)->rsaParams = params;

    return UA_STATUSCODE_GOOD;
}

/***********************/
/* Public Entry Points */
/***********************/

UA_StatusCode
UA_SecurityPolicy_Basic256Sha256(UA_SecurityPolicy *sp,
                                 const UA_ByteString localCertificate,
                                 const UA_ByteString localPrivateKey,
                                 const UA_Logger *logger) {
    return rsaPolicy(sp, &basic256Sha256Params, localCertificate,
                     localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_Aes128Sha256RsaOaep(UA_SecurityPolicy *sp,
                                      const UA_ByteString localCertificate,
                                      const UA_ByteString localPrivateKey,
                                      const UA_Logger *logger) {
    return rsaPolicy(sp, &aes128Sha256RsaOaepParams, localCertificate,
                     localPrivateKey, logger);
}

UA_StatusCode
UA_SecurityPolicy_Aes256Sha256RsaPss(UA_SecurityPolicy *sp,
                                     const UA_ByteString localCertificate,
                                     const UA_ByteString localPrivateKey,
                                     const UA_Logger *logger) {
    return rsaPolicy(sp, &aes256Sha256RsaPssParams, localCertificate,
                     localPrivateKey, logger);
}

#endif
