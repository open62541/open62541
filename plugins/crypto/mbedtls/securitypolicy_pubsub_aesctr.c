/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) Holger Zipper, ifak
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS

#include "securitypolicy_common.h"

#define UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH 32
#define UA_PUBSUB_AESCTR_KEYNONCE_LENGTH 4
#define UA_PUBSUB_AESCTR_MESSAGENONCE_LENGTH 8
#define UA_PUBSUB_AESCTR_BLOCK_SIZE 16

typedef struct {
    size_t encryptionKeyLength;
} PubSubAesCtrPolicyContext;

typedef struct {
    UA_mbedTLS_PsaKey signingKey;
    UA_mbedTLS_PsaKey encryptingKey;
    UA_Byte keyNonce[UA_PUBSUB_AESCTR_KEYNONCE_LENGTH];
    UA_Byte messageNonce[UA_PUBSUB_AESCTR_MESSAGENONCE_LENGTH];
} PubSubAesCtrChannelContext;

static size_t
encryptionKeyLength(const UA_PubSubSecurityPolicy *policy) {
    const PubSubAesCtrPolicyContext *pc =
        (const PubSubAesCtrPolicyContext*)policy->policyContext;
    return pc->encryptionKeyLength;
}

static UA_StatusCode
verify(const UA_PubSubSecurityPolicy *policy, void *gContext,
       const UA_ByteString *message, const UA_ByteString *signature) {
    (void)policy;
    if(!gContext || !message || !signature)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signature->length != UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    PubSubAesCtrChannelContext *cc = (PubSubAesCtrChannelContext*)gContext;
    return UA_mbedTLS_PsaMacVerify(cc->signingKey.id,
                                  PSA_ALG_HMAC(PSA_ALG_SHA_256),
                                  message, signature);
}

static UA_StatusCode
sign(const UA_PubSubSecurityPolicy *policy, void *gContext,
     const UA_ByteString *message, UA_ByteString *signature) {
    (void)policy;
    if(!gContext || !message || !signature)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signature->length != UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    PubSubAesCtrChannelContext *cc = (PubSubAesCtrChannelContext*)gContext;
    return UA_mbedTLS_PsaMacCompute(cc->signingKey.id,
                                   PSA_ALG_HMAC(PSA_ALG_SHA_256),
                                   message, signature);
}

static size_t
getSignatureSize(const UA_PubSubSecurityPolicy *policy, const void *gContext) {
    (void)policy;
    (void)gContext;
    return UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH;
}

static size_t
getSignatureKeyLength(const UA_PubSubSecurityPolicy *policy,
                      const void *gContext) {
    (void)policy;
    (void)gContext;
    return UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH;
}

static size_t
getEncryptionKeyLength(const UA_PubSubSecurityPolicy *policy,
                       const void *gContext) {
    (void)gContext;
    return encryptionKeyLength(policy);
}

static UA_StatusCode
pubSubCrypt(void *gContext, UA_ByteString *data, UA_Boolean encrypting) {
    if(!gContext || !data)
        return UA_STATUSCODE_BADINTERNALERROR;
    PubSubAesCtrChannelContext *cc = (PubSubAesCtrChannelContext*)gContext;
    UA_Byte counter[UA_PUBSUB_AESCTR_BLOCK_SIZE];
    memcpy(counter, cc->keyNonce, UA_PUBSUB_AESCTR_KEYNONCE_LENGTH);
    memcpy(counter + UA_PUBSUB_AESCTR_KEYNONCE_LENGTH, cc->messageNonce,
           UA_PUBSUB_AESCTR_MESSAGENONCE_LENGTH);
    counter[12] = 0;
    counter[13] = 0;
    counter[14] = 0;
    counter[15] = 1;
    UA_ByteString counterBlock = {sizeof(counter), counter};
    return UA_mbedTLS_PsaCipher(cc->encryptingKey.id, PSA_ALG_CTR, encrypting,
                                &counterBlock, data);
}

static UA_StatusCode
pubSubEncrypt(const UA_PubSubSecurityPolicy *policy, void *gContext,
              UA_ByteString *data) {
    (void)policy;
    return pubSubCrypt(gContext, data, true);
}

static UA_StatusCode
pubSubDecrypt(const UA_PubSubSecurityPolicy *policy, void *gContext,
              UA_ByteString *data) {
    (void)policy;
    return pubSubCrypt(gContext, data, false);
}

static UA_StatusCode
generateKey(UA_PubSubSecurityPolicy *policy, void *gContext,
            const UA_ByteString *secret, const UA_ByteString *seed,
            UA_ByteString *out) {
    (void)gContext;
    if(!policy || !secret || !seed || !out)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_mbedTLS_PsaPHash(PSA_ALG_SHA_256, secret, seed, out);
}

static UA_StatusCode
generateNonce(UA_PubSubSecurityPolicy *policy, void *gContext,
              UA_ByteString *out) {
    (void)gContext;
    if(!policy || !out)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_mbedTLS_PsaRandom(out);
}

static void
deleteGroupContext(UA_PubSubSecurityPolicy *policy, void *gContext) {
    (void)policy;
    PubSubAesCtrChannelContext *cc = (PubSubAesCtrChannelContext*)gContext;
    if(!cc)
        return;
    UA_mbedTLS_PsaKey_clear(&cc->signingKey);
    UA_mbedTLS_PsaKey_clear(&cc->encryptingKey);
    UA_free(cc);
}

static UA_Boolean
validByteString(const UA_ByteString *value) {
    return value && (value->length == 0 || value->data);
}

static UA_StatusCode
newGroupContext(UA_PubSubSecurityPolicy *policy,
                const UA_ByteString *signingKey,
                const UA_ByteString *encryptingKey,
                const UA_ByteString *keyNonce, void **gContext) {
    if(!policy || !gContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    if((signingKey && !validByteString(signingKey)) ||
       (encryptingKey && !validByteString(encryptingKey)) ||
       (keyNonce && !validByteString(keyNonce)))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if((signingKey && signingKey->length != UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH) ||
       (encryptingKey && encryptingKey->length != encryptionKeyLength(policy)) ||
       (keyNonce && keyNonce->length != UA_PUBSUB_AESCTR_KEYNONCE_LENGTH))
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    PubSubAesCtrChannelContext *cc =
        (PubSubAesCtrChannelContext*)UA_calloc(1, sizeof(*cc));
    if(!cc)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_mbedTLS_PsaKey_init(&cc->signingKey);
    UA_mbedTLS_PsaKey_init(&cc->encryptingKey);

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(signingKey)
        res = UA_mbedTLS_PsaKey_import(&cc->signingKey, PSA_KEY_TYPE_HMAC,
            PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE,
            PSA_ALG_HMAC(PSA_ALG_SHA_256), signingKey);
    if(res == UA_STATUSCODE_GOOD && encryptingKey)
        res = UA_mbedTLS_PsaKey_import(&cc->encryptingKey, PSA_KEY_TYPE_AES,
            PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT,
            PSA_ALG_CTR, encryptingKey);
    if(res != UA_STATUSCODE_GOOD) {
        deleteGroupContext(policy, cc);
        return res;
    }
    if(keyNonce)
        memcpy(cc->keyNonce, keyNonce->data, keyNonce->length);
    *gContext = cc;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setSecurityKeys(UA_PubSubSecurityPolicy *policy, void *gContext,
                const UA_ByteString *signingKey,
                const UA_ByteString *encryptingKey,
                const UA_ByteString *keyNonce) {
    if(!policy || !gContext || !signingKey || !encryptingKey || !keyNonce)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!validByteString(signingKey) || !validByteString(encryptingKey) ||
       !validByteString(keyNonce))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(signingKey->length != UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH ||
       encryptingKey->length != encryptionKeyLength(policy) ||
       keyNonce->length != UA_PUBSUB_AESCTR_KEYNONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    PubSubAesCtrChannelContext *cc = (PubSubAesCtrChannelContext*)gContext;
    UA_mbedTLS_PsaKey newSigningKey;
    UA_mbedTLS_PsaKey newEncryptingKey;
    UA_mbedTLS_PsaKey_init(&newSigningKey);
    UA_mbedTLS_PsaKey_init(&newEncryptingKey);
    UA_StatusCode res = UA_mbedTLS_PsaKey_import(&newSigningKey,
        PSA_KEY_TYPE_HMAC,
        PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE,
        PSA_ALG_HMAC(PSA_ALG_SHA_256), signingKey);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_mbedTLS_PsaKey_import(&newEncryptingKey, PSA_KEY_TYPE_AES,
            PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT,
            PSA_ALG_CTR, encryptingKey);
    if(res == UA_STATUSCODE_GOOD) {
        UA_mbedTLS_PsaKey_clear(&cc->signingKey);
        cc->signingKey = newSigningKey;
        UA_mbedTLS_PsaKey_init(&newSigningKey);
        UA_mbedTLS_PsaKey_clear(&cc->encryptingKey);
        cc->encryptingKey = newEncryptingKey;
        UA_mbedTLS_PsaKey_init(&newEncryptingKey);
        memcpy(cc->keyNonce, keyNonce->data, keyNonce->length);
    }
    UA_mbedTLS_PsaKey_clear(&newSigningKey);
    UA_mbedTLS_PsaKey_clear(&newEncryptingKey);
    return res;
}

static UA_StatusCode
setMessageNonce(UA_PubSubSecurityPolicy *policy, void *gContext,
                const UA_ByteString *nonce) {
    (void)policy;
    if(!gContext || !nonce)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!validByteString(nonce))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(nonce->length != UA_PUBSUB_AESCTR_MESSAGENONCE_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    PubSubAesCtrChannelContext *cc = (PubSubAesCtrChannelContext*)gContext;
    memcpy(cc->messageNonce, nonce->data, nonce->length);
    return UA_STATUSCODE_GOOD;
}

static void
clear(UA_PubSubSecurityPolicy *policy) {
    if(!policy)
        return;
    UA_free(policy->policyContext);
    policy->policyContext = NULL;
}

static UA_StatusCode
setup(UA_PubSubSecurityPolicy *sp, const UA_Logger *logger,
      UA_String policyUri, size_t keyLength) {
    if(!sp)
        return UA_STATUSCODE_BADINTERNALERROR;
    memset(sp, 0, sizeof(*sp));
    sp->logger = logger;
    sp->policyUri = policyUri;

    PubSubAesCtrPolicyContext *pc =
        (PubSubAesCtrPolicyContext*)UA_malloc(sizeof(*pc));
    if(!pc)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    pc->encryptionKeyLength = keyLength;
    sp->policyContext = pc;
    UA_StatusCode res = UA_mbedTLS_PSA_Init();
    if(res != UA_STATUSCODE_GOOD) {
        clear(sp);
        return res;
    }

    sp->newGroupContext = newGroupContext;
    sp->deleteGroupContext = deleteGroupContext;
    sp->verify = verify;
    sp->sign = sign;
    sp->getSignatureSize = getSignatureSize;
    sp->getSignatureKeyLength = getSignatureKeyLength;
    sp->getEncryptionKeyLength = getEncryptionKeyLength;
    sp->encrypt = pubSubEncrypt;
    sp->decrypt = pubSubDecrypt;
    sp->setSecurityKeys = setSecurityKeys;
    sp->generateKey = generateKey;
    sp->generateNonce = generateNonce;
    sp->keyMaterialLength = UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH + keyLength +
        UA_PUBSUB_AESCTR_KEYNONCE_LENGTH;
    sp->messageNonceLength = UA_PUBSUB_AESCTR_MESSAGENONCE_LENGTH;
    sp->setMessageNonce = setMessageNonce;
    sp->clear = clear;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubSecurityPolicy_Aes128Ctr(UA_PubSubSecurityPolicy *sp,
                                  const UA_Logger *logger) {
    return setup(sp, logger,
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes128-CTR"),
        16);
}

UA_StatusCode
UA_PubSubSecurityPolicy_Aes256Ctr(UA_PubSubSecurityPolicy *sp,
                                  const UA_Logger *logger) {
    return setup(sp, logger,
        UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#PubSub-Aes256-CTR"),
        32);
}

#endif
