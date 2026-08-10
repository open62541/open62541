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

#define UA_SHA256_LENGTH 32
#define UA_AES128CTR_SIGNING_KEY_LENGTH 32
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4
#define UA_AES128CTR_MESSAGENONCE_LENGTH 8
#define UA_AES128CTR_ENCRYPTION_BLOCK_SIZE 16

typedef struct {
    UA_Byte unused;
} PUBSUB_AES128CTR_PolicyContext;

typedef struct {
    UA_mbedTLS_PsaKey signingKey;
    UA_mbedTLS_PsaKey encryptingKey;
    UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH];
    UA_Byte messageNonce[UA_AES128CTR_MESSAGENONCE_LENGTH];
} PUBSUB_AES128CTR_ChannelContext;

/* Signature and verify all using HMAC-SHA2-256, nothing to change */
static UA_StatusCode
verify_pubsub_aes128ctr(const UA_PubSubSecurityPolicy *policy,
                        void *gContext, const UA_ByteString *message,
                        const UA_ByteString *signature) {
    if(gContext == NULL || message == NULL || signature == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute MAC */
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    PUBSUB_AES128CTR_ChannelContext *gc =
        (PUBSUB_AES128CTR_ChannelContext*)gContext;

    unsigned char mac[UA_SHA256_LENGTH];
    UA_ByteString computed = {UA_SHA256_LENGTH, mac};
    if(UA_mbedTLS_PsaMacCompute(gc->signingKey.id,
                               PSA_ALG_HMAC(PSA_ALG_SHA_256),
                               message, &computed) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Compare with Signature */
    if(!UA_constantTimeEqual(signature->data, mac, UA_SHA256_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sign_pubsub_aes128ctr(const UA_PubSubSecurityPolicy *policy,
                      void *gContext, const UA_ByteString *message,
                      UA_ByteString *signature) {
    if(signature->length != UA_SHA256_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES128CTR_ChannelContext *gc =
        (PUBSUB_AES128CTR_ChannelContext*)gContext;
    if(UA_mbedTLS_PsaMacCompute(gc->signingKey.id,
                               PSA_ALG_HMAC(PSA_ALG_SHA_256),
                               message, signature) != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    return UA_STATUSCODE_GOOD;
}

static size_t
getSignatureSize_pubsub_aes128ctr(const UA_PubSubSecurityPolicy *policy,
                                  const void *gContext) {
    return UA_SHA256_LENGTH;
}

static size_t
getSignatureKeyLength_pubsub_aes128ctr(const UA_PubSubSecurityPolicy *policy,
                                       const void *gContext) {
    return UA_AES128CTR_SIGNING_KEY_LENGTH;
}

static size_t
getEncryptionKeyLength_pubsub_aes128ctr(const UA_PubSubSecurityPolicy *policy,
                                        const void *gContext) {
    return UA_AES128CTR_KEY_LENGTH;
}

static UA_StatusCode
encrypt_pubsub_aes128ctr(const UA_PubSubSecurityPolicy *policy, void *gContext,
                         UA_ByteString *data) {
    if(gContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    PUBSUB_AES128CTR_ChannelContext *gc =
        (PUBSUB_AES128CTR_ChannelContext*)gContext;

    /* Prepare the counterBlock required for encryption/decryption 
     * Block counter starts at 1 according to part 14 (7.2.2.4.3.2)*/
    UA_Byte counterBlockCopy[UA_AES128CTR_ENCRYPTION_BLOCK_SIZE];
    UA_Byte counterInitialValue[4] = {0,0,0,1};
    memcpy(counterBlockCopy, gc->keyNonce, UA_AES128CTR_KEYNONCE_LENGTH);
    memcpy(counterBlockCopy + UA_AES128CTR_KEYNONCE_LENGTH,
           gc->messageNonce, UA_AES128CTR_MESSAGENONCE_LENGTH);
    memcpy(counterBlockCopy + UA_AES128CTR_KEYNONCE_LENGTH +
           UA_AES128CTR_MESSAGENONCE_LENGTH, &counterInitialValue, 4);

    UA_ByteString counterBlock = {sizeof(counterBlockCopy), counterBlockCopy};
    return UA_mbedTLS_PsaCipher(gc->encryptingKey.id, PSA_ALG_CTR, true,
                                &counterBlock, data);
}

/* a decryption function is exactly the same as an encryption one, since they all do XOR
 * operations*/
static UA_StatusCode
decrypt_pubsub_aes128ctr(const UA_PubSubSecurityPolicy *policy, void *gContext,
                         UA_ByteString *data) {
    if(gContext == NULL || data == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    PUBSUB_AES128CTR_ChannelContext *gc =
        (PUBSUB_AES128CTR_ChannelContext*)gContext;
    UA_Byte counterBlockCopy[UA_AES128CTR_ENCRYPTION_BLOCK_SIZE];
    UA_Byte counterInitialValue[4] = {0,0,0,1};
    memcpy(counterBlockCopy, gc->keyNonce, UA_AES128CTR_KEYNONCE_LENGTH);
    memcpy(counterBlockCopy + UA_AES128CTR_KEYNONCE_LENGTH,
           gc->messageNonce, UA_AES128CTR_MESSAGENONCE_LENGTH);
    memcpy(counterBlockCopy + UA_AES128CTR_KEYNONCE_LENGTH +
           UA_AES128CTR_MESSAGENONCE_LENGTH, &counterInitialValue, 4);
    UA_ByteString counterBlock = {sizeof(counterBlockCopy), counterBlockCopy};
    return UA_mbedTLS_PsaCipher(gc->encryptingKey.id, PSA_ALG_CTR, false,
                                &counterBlock, data);
}

static UA_StatusCode
generateKey_pubsub_aes128ctr(UA_PubSubSecurityPolicy *policy,
                             void *gContext, const UA_ByteString *secret,
                             const UA_ByteString *seed, UA_ByteString *out) {
    if(policy == NULL || secret == NULL || seed == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_mbedTLS_PsaPHash(PSA_ALG_SHA_256, secret, seed, out);
}

static UA_StatusCode
generateNonce_pubsub_aes128ctr(UA_PubSubSecurityPolicy *policy,
                               void *gContext, UA_ByteString *out) {
    if(policy == NULL || out == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_mbedTLS_PsaRandom(out);
}

static void
deleteContext_pubsub_aes128ctr(UA_PubSubSecurityPolicy *policy, void *gContext) {
    PUBSUB_AES128CTR_ChannelContext *gc =
        (PUBSUB_AES128CTR_ChannelContext*)gContext;
    if(gc) {
        UA_mbedTLS_PsaKey_clear(&gc->signingKey);
        UA_mbedTLS_PsaKey_clear(&gc->encryptingKey);
        UA_free(gc);
    }
}

static UA_StatusCode
newContext_pubsub_aes128ctr(UA_PubSubSecurityPolicy *policy,
                            const UA_ByteString *signingKey,
                            const UA_ByteString *encryptingKey,
                            const UA_ByteString *keyNonce,
                            void **gContext) {
    if((signingKey && signingKey->length != UA_AES128CTR_SIGNING_KEY_LENGTH) ||
       (encryptingKey && encryptingKey->length != UA_AES128CTR_KEY_LENGTH) ||
       (keyNonce && keyNonce->length != UA_AES128CTR_KEYNONCE_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Allocate the channel context */
    PUBSUB_AES128CTR_ChannelContext *gc = (PUBSUB_AES128CTR_ChannelContext *)
        UA_calloc(1, sizeof(PUBSUB_AES128CTR_ChannelContext));
    if(gc == NULL)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_mbedTLS_PsaKey_init(&gc->signingKey);
    UA_mbedTLS_PsaKey_init(&gc->encryptingKey);
    if(signingKey) {
        UA_StatusCode res = UA_mbedTLS_PsaKey_import(&gc->signingKey,
            PSA_KEY_TYPE_HMAC, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE,
            PSA_ALG_HMAC(PSA_ALG_SHA_256), signingKey);
        if(res != UA_STATUSCODE_GOOD) {
            deleteContext_pubsub_aes128ctr(policy, gc);
            return res;
        }
    }
    if(encryptingKey) {
        UA_StatusCode res = UA_mbedTLS_PsaKey_import(&gc->encryptingKey,
            PSA_KEY_TYPE_AES, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT,
            PSA_ALG_CTR, encryptingKey);
        if(res != UA_STATUSCODE_GOOD) {
            deleteContext_pubsub_aes128ctr(policy, gc);
            return res;
        }
    }
    if(keyNonce)
        memcpy(gc->keyNonce, keyNonce->data, keyNonce->length);
    *gContext = gc;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setKeys_pubsub_aes128ctr(UA_PubSubSecurityPolicy *policy, void *gContext,
                         const UA_ByteString *signingKey,
                         const UA_ByteString *encryptingKey,
                         const UA_ByteString *keyNonce) {
    if(!gContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!signingKey || signingKey->length != UA_AES128CTR_SIGNING_KEY_LENGTH ||
       !encryptingKey || encryptingKey->length != UA_AES128CTR_KEY_LENGTH ||
       !keyNonce || keyNonce->length != UA_AES128CTR_KEYNONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    PUBSUB_AES128CTR_ChannelContext *gc =
        (PUBSUB_AES128CTR_ChannelContext*)gContext;
    UA_StatusCode res = UA_mbedTLS_PsaKey_import(&gc->signingKey,
        PSA_KEY_TYPE_HMAC, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE,
        PSA_ALG_HMAC(PSA_ALG_SHA_256), signingKey);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_mbedTLS_PsaKey_import(&gc->encryptingKey,
        PSA_KEY_TYPE_AES, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT,
        PSA_ALG_CTR, encryptingKey);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    memcpy(gc->keyNonce, keyNonce->data, keyNonce->length);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setMessageNonce_pubsub_aes128ctr(UA_PubSubSecurityPolicy *policy, void *gContext,
                                 const UA_ByteString *nonce) {
    if(nonce->length != UA_AES128CTR_MESSAGENONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    PUBSUB_AES128CTR_ChannelContext *gc =
        (PUBSUB_AES128CTR_ChannelContext*)gContext;
    memcpy(gc->messageNonce, nonce->data, nonce->length);
    return UA_STATUSCODE_GOOD;
}

static void
clear_pubsub_aes128ctr(UA_PubSubSecurityPolicy *securityPolicy) {
    if(securityPolicy == NULL)
        return;
    if(securityPolicy->policyContext == NULL)
        return;
    UA_LOG_DEBUG(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Deleted members of EndpointContext for PUBSUB_AES128CTR");
    UA_free(securityPolicy->policyContext);
    securityPolicy->policyContext = NULL;
}

static UA_StatusCode
setup_pubsub_aes128ctr(UA_PubSubSecurityPolicy *securityPolicy) {
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

    if(UA_mbedTLS_PSA_Init() != UA_STATUSCODE_GOOD) {
        retval = UA_STATUSCODE_BADSECURITYCHECKSFAILED;
        goto error;
    }

    return retval;

error:
    UA_LOG_ERROR(securityPolicy->logger, UA_LOGCATEGORY_SECURITYPOLICY,
                 "Could not create securityContext");
    if(securityPolicy->policyContext != NULL)
        clear_pubsub_aes128ctr(securityPolicy);
    return retval;
}

UA_StatusCode
UA_PubSubSecurityPolicy_Aes128Ctr(UA_PubSubSecurityPolicy *sp,
                                  const UA_Logger *logger) {
    memset(sp, 0, sizeof(UA_PubSubSecurityPolicy));
    sp->logger = logger;
    sp->policyUri =
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes128-CTR");

    /* Set the method pointers */
    sp->newGroupContext = newContext_pubsub_aes128ctr;
    sp->deleteGroupContext = deleteContext_pubsub_aes128ctr;
    sp->verify = verify_pubsub_aes128ctr;
    sp->sign = sign_pubsub_aes128ctr;
    sp->getSignatureSize = getSignatureSize_pubsub_aes128ctr;
    sp->getSignatureKeyLength = getSignatureKeyLength_pubsub_aes128ctr;
    sp->getEncryptionKeyLength = getEncryptionKeyLength_pubsub_aes128ctr;
    sp->encrypt = encrypt_pubsub_aes128ctr;
    sp->decrypt = decrypt_pubsub_aes128ctr;
    sp->setSecurityKeys = setKeys_pubsub_aes128ctr;
    sp->generateKey = generateKey_pubsub_aes128ctr;
    sp->generateNonce = generateNonce_pubsub_aes128ctr;
    sp->nonceLength = UA_AES128CTR_SIGNING_KEY_LENGTH +
        UA_AES128CTR_KEY_LENGTH + UA_AES128CTR_KEYNONCE_LENGTH;
    sp->setMessageNonce = setMessageNonce_pubsub_aes128ctr;
    sp->clear = clear_pubsub_aes128ctr;

    /* Initialize the policyContext */
    return setup_pubsub_aes128ctr(sp);
}

#endif
