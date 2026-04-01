/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Holger Zipper, ifak
 *    Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include "securitypolicy_common.h"

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
#define UA_AES256CTR_SIGNING_KEY_LENGTH 32
#define UA_AES256CTR_KEY_LENGTH 32
#define UA_AES256CTR_KEYNONCE_LENGTH 4
#define UA_AES256CTR_MESSAGENONCE_LENGTH 8
#define UA_AES256CTR_ENCRYPTION_BLOCK_SIZE 16

typedef struct {
    mbedtls_ctr_drbg_context drbgContext;
    mbedtls_entropy_context entropyContext;
    mbedtls_md_context_t sha256MdContext;
} PUBSUB_AES256CTR_PolicyContext;

typedef struct {
    UA_Byte signingKey[UA_AES256CTR_SIGNING_KEY_LENGTH];
    UA_Byte encryptingKey[UA_AES256CTR_KEY_LENGTH];
    UA_Byte keyNonce[UA_AES256CTR_KEYNONCE_LENGTH];
    UA_Byte messageNonce[UA_AES256CTR_MESSAGENONCE_LENGTH];
} PUBSUB_AES256CTR_ChannelContext;

/* Signature and verify all using HMAC-SHA2-256, nothing to change */
static UA_StatusCode
verify_pubsub_aes256ctr(const UA_PubSubSecurityPolicy *policy,
                        void *gContext, const UA_ByteString *message,
                        const UA_ByteString *signature) {
    if(gContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES256CTR_ChannelContext *gc =
        (PUBSUB_AES256CTR_ChannelContext*)gContext;
    PUBSUB_AES256CTR_PolicyContext *pc =
        (PUBSUB_AES256CTR_PolicyContext *)policy->policyContext;

    /* Compute MAC */
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    unsigned char mac[UA_SHA256_LENGTH];
    UA_ByteString signingKey =
        {UA_AES256CTR_SIGNING_KEY_LENGTH, gc->signingKey};
    if(mbedtls_hmac(&pc->sha256MdContext, &signingKey, message, mac) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Compare with Signature */
    if(!UA_constantTimeEqual(signature->data, mac, UA_SHA256_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sign_pubsub_aes256ctr(const UA_PubSubSecurityPolicy *policy,
                      void *gContext, const UA_ByteString *message,
                      UA_ByteString *signature) {
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    PUBSUB_AES256CTR_ChannelContext *gc =
        (PUBSUB_AES256CTR_ChannelContext*)gContext;
    PUBSUB_AES256CTR_PolicyContext *pc =
        (PUBSUB_AES256CTR_PolicyContext *)policy->policyContext;
    UA_ByteString signingKey =
        {UA_AES256CTR_SIGNING_KEY_LENGTH, gc->signingKey};
    if(mbedtls_hmac(&pc->sha256MdContext, &signingKey,
                    message, signature->data) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static size_t
getSignatureSize_pubsub_aes256ctr(const UA_PubSubSecurityPolicy *policy,
                                  const void *gContext) {
    return UA_SHA256_LENGTH;
}

static size_t
getSignatureKeyLength_pubsub_aes256ctr(const UA_PubSubSecurityPolicy *policy,
                                       const void *gContext) {
    return UA_AES256CTR_SIGNING_KEY_LENGTH;
}

static size_t
getEncryptionKeyLength_pubsub_aes256ctr(const UA_PubSubSecurityPolicy *policy,
                                        const void *gContext) {
    return UA_AES256CTR_KEY_LENGTH;
}

static UA_StatusCode
encrypt_pubsub_aes256ctr(const UA_PubSubSecurityPolicy *policy, void *gContext,
                         UA_ByteString *data) {
    if(gContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES256CTR_ChannelContext *gc =
        (PUBSUB_AES256CTR_ChannelContext*)gContext;

    /* CTR mode does not need padding */

    /* Decode the header to Extract the message nonce */

    /* Keylength in bits */
    unsigned int keylength = (unsigned int)(UA_AES256CTR_KEY_LENGTH * 8);
    mbedtls_aes_context aesContext;
    int mbedErr =
        mbedtls_aes_setkey_enc(&aesContext, gc->encryptingKey, keylength);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Prepare the counterBlock required for encryption/decryption
     * Block counter starts at 1 according to part 14 (7.2.2.4.3.2)*/
    UA_Byte counterBlockCopy[UA_AES256CTR_ENCRYPTION_BLOCK_SIZE];
    UA_Byte counterInitialValue[4] = {0,0,0,1};
    memcpy(counterBlockCopy, gc->keyNonce, UA_AES256CTR_KEYNONCE_LENGTH);
    memcpy(counterBlockCopy + UA_AES256CTR_KEYNONCE_LENGTH,
           gc->messageNonce, UA_AES256CTR_MESSAGENONCE_LENGTH);
    memcpy(counterBlockCopy + UA_AES256CTR_KEYNONCE_LENGTH +
           UA_AES256CTR_MESSAGENONCE_LENGTH, &counterInitialValue, 4);

    size_t counterblockoffset = 0;
    UA_Byte aesBuffer[UA_AES256CTR_ENCRYPTION_BLOCK_SIZE];
    mbedErr = mbedtls_aes_crypt_ctr(&aesContext, data->length, &counterblockoffset,
                                    counterBlockCopy, aesBuffer, data->data, data->data);
    if(mbedErr)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

/* a decryption function is exactly the same as an encryption one, since they all do XOR
 * operations*/
static UA_StatusCode
decrypt_pubsub_aes256ctr(const UA_PubSubSecurityPolicy *policy, void *gContext,
                         UA_ByteString *data) {
    return encrypt_pubsub_aes256ctr(policy, gContext, data);
}

static UA_StatusCode
generateKey_pubsub_aes256ctr(UA_PubSubSecurityPolicy *policy,
                             void *gContext, const UA_ByteString *secret,
                             const UA_ByteString *seed, UA_ByteString *out) {
    if(policy == NULL || secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    PUBSUB_AES256CTR_PolicyContext *pc =
        (PUBSUB_AES256CTR_PolicyContext *)policy->policyContext;
    return mbedtls_generateKey(&pc->sha256MdContext, secret, seed, out);
}

/* This nonce does not to be  a
cryptographically random number, it can be pseudo-random */
static UA_StatusCode
generateNonce_pubsub_aes256ctr(UA_PubSubSecurityPolicy *policy,
                               void *gContext, UA_ByteString *out) {
    if(policy == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    PUBSUB_AES256CTR_PolicyContext *pc =
        (PUBSUB_AES256CTR_PolicyContext *)policy->policyContext;
    int mbedErr = mbedtls_ctr_drbg_random(&pc->drbgContext, out->data, out->length);
    if(mbedErr)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static void
deleteGroupContext_pubsub_aes256ctr(UA_PubSubSecurityPolicy *policy, void *gContext) {
    UA_free(gContext);
}

static UA_StatusCode
newGroupContext_pubsub_aes256ctr(UA_PubSubSecurityPolicy *policy,
                                 const UA_ByteString *signingKey,
                                 const UA_ByteString *encryptingKey,
                                 const UA_ByteString *keyNonce,
                                 void **gContext) {
    if((signingKey && signingKey->length != UA_AES256CTR_SIGNING_KEY_LENGTH) ||
       (encryptingKey && encryptingKey->length != UA_AES256CTR_KEY_LENGTH) ||
       (keyNonce && keyNonce->length != UA_AES256CTR_KEYNONCE_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Allocate the channel context */
    PUBSUB_AES256CTR_ChannelContext *cc = (PUBSUB_AES256CTR_ChannelContext *)
        UA_calloc(1, sizeof(PUBSUB_AES256CTR_ChannelContext));
    if(cc == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Initialize the channel context */
    if(signingKey)
        memcpy(cc->signingKey, signingKey->data, signingKey->length);
    if(encryptingKey)
        memcpy(cc->encryptingKey, encryptingKey->data, encryptingKey->length);
    if(keyNonce)
        memcpy(cc->keyNonce, keyNonce->data, keyNonce->length);
    *gContext = cc;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setKeys_pubsub_aes256ctr(UA_PubSubSecurityPolicy *policy, void *gContext,
                         const UA_ByteString *signingKey,
                         const UA_ByteString *encryptingKey,
                         const UA_ByteString *keyNonce) {
    if(!gContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!signingKey || signingKey->length != UA_AES256CTR_SIGNING_KEY_LENGTH ||
       !encryptingKey || encryptingKey->length != UA_AES256CTR_KEY_LENGTH ||
       !keyNonce || keyNonce->length != UA_AES256CTR_KEYNONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    PUBSUB_AES256CTR_ChannelContext *gc =
        (PUBSUB_AES256CTR_ChannelContext*)gContext;
    memcpy(gc->signingKey, signingKey->data, signingKey->length);
    memcpy(gc->encryptingKey, encryptingKey->data, encryptingKey->length);
    memcpy(gc->keyNonce, keyNonce->data, keyNonce->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setMessageNonce_pubsub_aes256ctr(UA_PubSubSecurityPolicy *policy,
                                 void *gContext, const UA_ByteString *nonce) {
    if(nonce->length != UA_AES256CTR_MESSAGENONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    PUBSUB_AES256CTR_ChannelContext *gc =
        (PUBSUB_AES256CTR_ChannelContext*)gContext;
    memcpy(gc->messageNonce, nonce->data, nonce->length);
    return UA_STATUSCODE_GOOD;
}

static void
clear_pubsub_aes256ctr(UA_PubSubSecurityPolicy *policy) {
    if(policy == NULL || policy->policyContext == NULL)
        return;
    PUBSUB_AES256CTR_PolicyContext *pc =
        (PUBSUB_AES256CTR_PolicyContext *)policy->policyContext;
    mbedtls_ctr_drbg_free(&pc->drbgContext);
    mbedtls_entropy_free(&pc->entropyContext);
    mbedtls_md_free(&pc->sha256MdContext);
    UA_free(pc);
    policy->policyContext = NULL;
}

static UA_StatusCode
setup_pubsub_aes256ctr(UA_PubSubSecurityPolicy *securityPolicy) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(securityPolicy == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES256CTR_PolicyContext *pc = (PUBSUB_AES256CTR_PolicyContext *)
        UA_calloc(1, sizeof(PUBSUB_AES256CTR_PolicyContext));
    securityPolicy->policyContext = (void *)pc;
    if(!pc) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto error;
    }

    /* Initialize the PolicyContext */
    memset(pc, 0, sizeof(PUBSUB_AES256CTR_PolicyContext));
    mbedtls_ctr_drbg_init(&pc->drbgContext);
    mbedtls_entropy_init(&pc->entropyContext);
    mbedtls_md_init(&pc->sha256MdContext);

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
        clear_pubsub_aes256ctr(securityPolicy);
    return retval;
}

UA_StatusCode
UA_PubSubSecurityPolicy_Aes256Ctr(UA_PubSubSecurityPolicy *sp,
                                  const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_PubSubSecurityPolicy));
    sp->logger = logger;
    sp->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR");

    /* Set the method pointers */
    sp->newGroupContext = newGroupContext_pubsub_aes256ctr;
    sp->deleteGroupContext = deleteGroupContext_pubsub_aes256ctr;
    sp->verify = verify_pubsub_aes256ctr;
    sp->sign = sign_pubsub_aes256ctr;
    sp->getSignatureSize = getSignatureSize_pubsub_aes256ctr;
    sp->getSignatureKeyLength = getSignatureKeyLength_pubsub_aes256ctr;
    sp->getEncryptionKeyLength = getEncryptionKeyLength_pubsub_aes256ctr;
    sp->encrypt = encrypt_pubsub_aes256ctr;
    sp->decrypt = decrypt_pubsub_aes256ctr;
    sp->setSecurityKeys = setKeys_pubsub_aes256ctr;
    sp->generateKey = generateKey_pubsub_aes256ctr;
    sp->generateNonce = generateNonce_pubsub_aes256ctr;
    sp->nonceLength = UA_AES256CTR_SIGNING_KEY_LENGTH +
        UA_AES256CTR_KEY_LENGTH + UA_AES256CTR_KEYNONCE_LENGTH;
    sp->setMessageNonce = setMessageNonce_pubsub_aes256ctr;
    sp->clear = clear_pubsub_aes256ctr;

    /* Initialize the policyContext */
    return setup_pubsub_aes256ctr(sp);
}

#endif
