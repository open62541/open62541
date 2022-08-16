/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Holger Zipper, ifak
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>
#include "securitypolicy_mbedtls_common.h"

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/md.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/version.h>
#include <mbedtls/x509_crt.h>

/* Orinigal Notes:
 * mbedTLS' AES allows in-place encryption and decryption. Sow we don't have to
 * allocate temp buffers.
 * https://tls.mbed.org/discussions/generic/in-place-decryption-with-aes256-same-input-output-buffer
 */

#define UA_SHA256_LENGTH 32
#define UA_AES128CTR_SIGNING_KEY_LENGTH 32
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4
#define UA_AES128CTR_MESSAGENONCE_LENGTH 8
#define UA_AES128CTR_ENCRYPTION_BLOCK_SIZE 16
#define UA_AES128CTR_PLAIN_TEXT_BLOCK_SIZE 16
/* counter block=keynonce(4Byte)+Messagenonce(8Byte)+counter(4Byte) see Part14
 * 7.2.2.2.3.2 for details */
#define UA_AES128CTR_COUNTERBLOCK_SIZE 16

typedef struct {
    const UA_PubSubSecurityPolicy *securityPolicy;
    mbedtls_ctr_drbg_context drbgContext;
    mbedtls_entropy_context entropyContext;
    mbedtls_md_context_t sha256MdContext;
} PUBSUB_AES128CTR_PolicyContext;

typedef struct {
    PUBSUB_AES128CTR_PolicyContext *policyContext;
    UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH];
    UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH];
    UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH];
    UA_Byte messageNonce[UA_AES128CTR_MESSAGENONCE_LENGTH];
} PUBSUB_AES128CTR_ChannelContext;

/*******************/
/* SymmetricModule */
/*******************/

/* Signature and verify all using HMAC-SHA2-256, nothing to change */
static UA_StatusCode
verify_sp_pubsub_aes128ctr(PUBSUB_AES128CTR_ChannelContext *cc,
                           const UA_ByteString *message,
                           const UA_ByteString *signature) {
    if(cc == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute MAC */
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    PUBSUB_AES128CTR_PolicyContext *pc =
        (PUBSUB_AES128CTR_PolicyContext *)cc->policyContext;

    unsigned char mac[UA_SHA256_LENGTH];
    UA_ByteString signingKey =
        {UA_AES128CTR_SIGNING_KEY_LENGTH, cc->signingKey};
    mbedtls_hmac(&pc->sha256MdContext, &signingKey, message, mac);

    /* Compare with Signature */
    if(!UA_constantTimeEqual(signature->data, mac, UA_SHA256_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sign_sp_pubsub_aes128ctr(PUBSUB_AES128CTR_ChannelContext *cc,
                         const UA_ByteString *message, UA_ByteString *signature) {
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString signingKey =
        {UA_AES128CTR_SIGNING_KEY_LENGTH, cc->signingKey};
    mbedtls_hmac(&cc->policyContext->sha256MdContext, &signingKey, message,
                 signature->data);
    return UA_STATUSCODE_GOOD;
}

static size_t
getSignatureSize_sp_pubsub_aes128ctr(const void *channelContext) {
    return UA_SHA256_LENGTH;
}

static size_t
getSigningKeyLength_sp_pubsub_aes128ctr(const void *const channelContext) {
    return UA_AES128CTR_SIGNING_KEY_LENGTH;
}

static size_t
getEncryptionKeyLength_sp_pubsub_aes128ctr(const void *channelContext) {
    return UA_AES128CTR_KEY_LENGTH;
}

static size_t
getEncryptionBlockSize_sp_pubsub_aes128ctr(const void *channelContext) {
    return UA_AES128CTR_ENCRYPTION_BLOCK_SIZE;
}

static size_t
getPlainTextBlockSize_sp_pubsub_aes128ctr(const void *channelContext) {
    return UA_AES128CTR_PLAIN_TEXT_BLOCK_SIZE;
}

static UA_StatusCode
encrypt_sp_pubsub_aes128ctr(const PUBSUB_AES128CTR_ChannelContext *cc,
                            UA_ByteString *data) {
    if(cc == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* CTR mode does not need padding */

    /* Decode the header to Extract the message nonce */

    /* Keylength in bits */
    unsigned int keylength = (unsigned int)(UA_AES128CTR_KEY_LENGTH * 8);
    mbedtls_aes_context aesContext;
    int mbedErr = mbedtls_aes_setkey_enc(&aesContext, cc->encryptingKey, keylength);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Prepare the counterBlock required for encryption/decryption */
    UA_Byte counterBlockCopy[UA_AES128CTR_ENCRYPTION_BLOCK_SIZE];
    memcpy(counterBlockCopy, cc->keyNonce, UA_AES128CTR_KEYNONCE_LENGTH);
    memcpy(counterBlockCopy + UA_AES128CTR_KEYNONCE_LENGTH,
           cc->messageNonce, UA_AES128CTR_MESSAGENONCE_LENGTH);
    memset(counterBlockCopy + UA_AES128CTR_KEYNONCE_LENGTH +
           UA_AES128CTR_MESSAGENONCE_LENGTH, 0, 4);

    size_t counterblockoffset = 0;
    UA_Byte aesBuffer[UA_AES128CTR_ENCRYPTION_BLOCK_SIZE];
    mbedErr = mbedtls_aes_crypt_ctr(&aesContext, data->length, &counterblockoffset,
                                    counterBlockCopy, aesBuffer, data->data, data->data);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

/* a decryption function is exactly the same as an encryption one, since they all do XOR
 * operations*/
static UA_StatusCode
decrypt_sp_pubsub_aes128ctr(const PUBSUB_AES128CTR_ChannelContext *cc,
                            UA_ByteString *data) {
    return encrypt_sp_pubsub_aes128ctr(cc, data);
}

/*Tested, meeting  Profile*/
static UA_StatusCode
generateKey_sp_pubsub_aes128ctr(void *policyContext, const UA_ByteString *secret,
                                const UA_ByteString *seed, UA_ByteString *out) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

/* This nonce does not need to be a cryptographically random number, it can be
 * pseudo-random */
static UA_StatusCode
generateNonce_sp_pubsub_aes128ctr(void *policyContext, UA_ByteString *out) {
    if(policyContext == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES128CTR_PolicyContext *pc = (PUBSUB_AES128CTR_PolicyContext *)policyContext;
    int mbedErr = mbedtls_ctr_drbg_random(&pc->drbgContext, out->data, out->length);
    if(mbedErr)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

/*****************/
/* ChannelModule */
/*****************/

static void
channelContext_deleteContext_sp_pubsub_aes128ctr(PUBSUB_AES128CTR_ChannelContext *cc) {
    UA_free(cc);
}

static UA_StatusCode
channelContext_newContext_sp_pubsub_aes128ctr(void *policyContext,
                                              const UA_ByteString *signingKey,
                                              const UA_ByteString *encryptingKey,
                                              const UA_ByteString *keyNonce,
                                              void **wgContext) {
    if((signingKey && signingKey->length != UA_AES128CTR_SIGNING_KEY_LENGTH) ||
       (encryptingKey && encryptingKey->length != UA_AES128CTR_KEY_LENGTH) ||
       (keyNonce && keyNonce->length != UA_AES128CTR_KEYNONCE_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Allocate the channel context */
    PUBSUB_AES128CTR_ChannelContext *cc = (PUBSUB_AES128CTR_ChannelContext *)
        UA_calloc(1, sizeof(PUBSUB_AES128CTR_ChannelContext));
    if(cc == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Initialize the channel context */
    cc->policyContext = (PUBSUB_AES128CTR_PolicyContext *)policyContext;
    if(signingKey)
        memcpy(cc->signingKey, signingKey->data, signingKey->length);
    if(encryptingKey)
        memcpy(cc->encryptingKey, encryptingKey->data, encryptingKey->length);
    if(keyNonce)
        memcpy(cc->keyNonce, keyNonce->data, keyNonce->length);
    *wgContext = cc;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setKeys_sp_pubsub_aes128ctr(PUBSUB_AES128CTR_ChannelContext *cc,
                                           const UA_ByteString *signingKey,
                                           const UA_ByteString *encryptingKey,
                                           const UA_ByteString *keyNonce) {
    if(!cc)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!signingKey || signingKey->length != UA_AES128CTR_SIGNING_KEY_LENGTH ||
       !encryptingKey || encryptingKey->length != UA_AES128CTR_KEY_LENGTH ||
       !keyNonce || keyNonce->length != UA_AES128CTR_KEYNONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    memcpy(cc->signingKey, signingKey->data, signingKey->length);
    memcpy(cc->encryptingKey, encryptingKey->data, encryptingKey->length);
    memcpy(cc->keyNonce, keyNonce->data, keyNonce->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
channelContext_setMessageNonce_sp_pubsub_aes128ctr(PUBSUB_AES128CTR_ChannelContext *cc,
                                                   const UA_ByteString *nonce) {
    if(nonce->length != UA_AES128CTR_MESSAGENONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    memcpy(cc->messageNonce, nonce->data, nonce->length);
    return UA_STATUSCODE_GOOD;
}

static void
deleteMembers_sp_pubsub_aes128ctr(UA_PubSubSecurityPolicy *securityPolicy) {
    if(securityPolicy == NULL)
        return;

    if(securityPolicy->policyContext == NULL)
        return;

    /* delete all allocated members in the context */
    PUBSUB_AES128CTR_PolicyContext *pc =
        (PUBSUB_AES128CTR_PolicyContext *)securityPolicy->policyContext;

    mbedtls_ctr_drbg_free(&pc->drbgContext);
    mbedtls_entropy_free(&pc->entropyContext);
    mbedtls_md_free(&pc->sha256MdContext);
    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for sp_PUBSUB_AES128CTR");
    UA_free(pc);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
policyContext_newContext_sp_pubsub_aes128ctr(UA_PubSubSecurityPolicy *securityPolicy) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES128CTR_PolicyContext *pc = (PUBSUB_AES128CTR_PolicyContext *)
        UA_calloc(1, sizeof(PUBSUB_AES128CTR_PolicyContext));
    securityPolicy->policyContext = (void *)pc;
    if(!pc) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Initialize the PolicyContext */
    memset(pc, 0, sizeof(PUBSUB_AES128CTR_PolicyContext));
    mbedtls_ctr_drbg_init(&pc->drbgContext);
    mbedtls_entropy_init(&pc->entropyContext);
    mbedtls_md_init(&pc->sha256MdContext);
    pc->securityPolicy = securityPolicy;

    /* Initialized the message digest */
    const mbedtls_md_info_t *const mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    int mbedErr = mbedtls_md_setup(&pc->sha256MdContext, mdInfo, MBEDTLS_MD_SHA256);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    mbedErr = mbedtls_entropy_self_test(0);

    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    /* Seed the RNG */
    char *personalization = "open62541-drbg";
    mbedErr = mbedtls_ctr_drbg_seed(&pc->drbgContext, mbedtls_entropy_func,
                                    &pc->entropyContext,
                                    (const unsigned char *)personalization, 14);
    if(mbedErr) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    return retval;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext");
    if(securityPolicy->policyContext != NULL)
        deleteMembers_sp_pubsub_aes128ctr(securityPolicy);
    return retval;
}

UA_StatusCode
UA_PubSubSecurityPolicy_Aes128Ctr(UA_PubSubSecurityPolicy *policy,
                                  const UA_Logger *logger) {
    memset(policy, 0, sizeof(UA_PubSubSecurityPolicy));
    policy->logger = logger;

    policy->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes128-CTR");

    UA_SecurityPolicySymmetricModule *symmetricModule = &policy->symmetricModule;

    /* SymmetricModule */
    symmetricModule->generateKey = generateKey_sp_pubsub_aes128ctr;
    symmetricModule->generateNonce = generateNonce_sp_pubsub_aes128ctr;

    UA_SecurityPolicySignatureAlgorithm *signatureAlgorithm =
        &symmetricModule->cryptoModule.signatureAlgorithm;
    signatureAlgorithm->uri = UA_STRING("http://www.w3.org/2001/04/xmlenc#sha256");
    signatureAlgorithm->verify =
        (UA_StatusCode(*)(void *, const UA_ByteString *,
                          const UA_ByteString *))verify_sp_pubsub_aes128ctr;
    signatureAlgorithm->sign =
        (UA_StatusCode(*)(void *, const UA_ByteString *, UA_ByteString *))sign_sp_pubsub_aes128ctr;
    signatureAlgorithm->getLocalSignatureSize = getSignatureSize_sp_pubsub_aes128ctr;
    signatureAlgorithm->getRemoteSignatureSize = getSignatureSize_sp_pubsub_aes128ctr;
    signatureAlgorithm->getLocalKeyLength =
        (size_t(*)(const void *))getSigningKeyLength_sp_pubsub_aes128ctr;
    signatureAlgorithm->getRemoteKeyLength =
        (size_t(*)(const void *))getSigningKeyLength_sp_pubsub_aes128ctr;

    UA_SecurityPolicyEncryptionAlgorithm *encryptionAlgorithm =
        &symmetricModule->cryptoModule.encryptionAlgorithm;
    encryptionAlgorithm->uri =
        UA_STRING("https://tools.ietf.org/html/rfc3686"); /* Temp solution */
    encryptionAlgorithm->encrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))encrypt_sp_pubsub_aes128ctr;
    encryptionAlgorithm->decrypt =
        (UA_StatusCode(*)(void *, UA_ByteString *))decrypt_sp_pubsub_aes128ctr;
    encryptionAlgorithm->getLocalKeyLength =
        getEncryptionKeyLength_sp_pubsub_aes128ctr;
    encryptionAlgorithm->getRemoteKeyLength =
        getEncryptionKeyLength_sp_pubsub_aes128ctr;
    encryptionAlgorithm->getRemoteBlockSize =
        (size_t(*)(const void *))getEncryptionBlockSize_sp_pubsub_aes128ctr;
    encryptionAlgorithm->getRemotePlainTextBlockSize =
        (size_t(*)(const void *))getPlainTextBlockSize_sp_pubsub_aes128ctr;
    symmetricModule->secureChannelNonceLength = UA_AES128CTR_SIGNING_KEY_LENGTH +
        UA_AES128CTR_KEY_LENGTH + UA_AES128CTR_KEYNONCE_LENGTH;

    /* ChannelModule */
    policy->newContext = channelContext_newContext_sp_pubsub_aes128ctr;
    policy->deleteContext = (void (*)(void *))
        channelContext_deleteContext_sp_pubsub_aes128ctr;

    policy->setSecurityKeys = (UA_StatusCode(*)(void *, const UA_ByteString *,
                                                const UA_ByteString *,
                                                const UA_ByteString *))
            channelContext_setKeys_sp_pubsub_aes128ctr;
    policy->setMessageNonce = (UA_StatusCode(*)(void *, const UA_ByteString *))
        channelContext_setMessageNonce_sp_pubsub_aes128ctr;
    policy->clear = deleteMembers_sp_pubsub_aes128ctr;
    policy->policyContext = NULL;

    /* Initialize the policyContext */
    return policyContext_newContext_sp_pubsub_aes128ctr(policy);
}

#endif
