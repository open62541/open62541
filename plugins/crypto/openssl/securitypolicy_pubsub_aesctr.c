/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)

#include "securitypolicy_common.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <limits.h>
#include <string.h>

/* OpenSSL 3 computes MACs with EVP_MAC. The HMAC_CTX API is deprecated there
 * but is the only option for OpenSSL 1.1 and LibreSSL. */
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && !defined(LIBRESSL_VERSION_NUMBER)
#include <openssl/core_names.h>
#include <openssl/params.h>
typedef EVP_MAC_CTX PubSubAesCtrMacCtx;
#else
#include <openssl/hmac.h>
typedef HMAC_CTX PubSubAesCtrMacCtx;
#endif

/* PubSub message security with AES-CTR and HMAC-SHA256 (OPC UA Part 14
 * 7.2.4.4.3). The key material of Table 155 is SigningKey | EncryptingKey |
 * KeyNonce. The counter block of Table 157 is KeyNonce[4] | MessageNonce[8] |
 * BlockCounter[4] with a big-endian block counter that starts at 1. */

#define UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH 32
#define UA_PUBSUB_AESCTR_KEYNONCE_LENGTH 4
#define UA_PUBSUB_AESCTR_MESSAGENONCE_LENGTH 8
#define UA_PUBSUB_AESCTR_BLOCK_SIZE 16

typedef struct {
    size_t encryptionKeyLength;
} PubSubAesCtrPolicyContext;

/* A group context is stateful by contract: setMessageNonce is followed by
 * encrypt or decrypt on the same context. Concurrent use of one group context
 * is therefore not supported with any backend. The cipher context holds the
 * AES key schedule, so a NetworkMessage only resets the IV. The MAC context
 * is re-keyed for every message: OpenSSL 3.0 does not restart a MAC
 * computation from a stored key, so the signing key is kept (and wiped on
 * release). */
typedef struct {
    UA_ByteString signingKey;
    PubSubAesCtrMacCtx *macCtx;
    EVP_CIPHER_CTX *cipherCtx;
    UA_Byte keyNonce[UA_PUBSUB_AESCTR_KEYNONCE_LENGTH];
    UA_Byte messageNonce[UA_PUBSUB_AESCTR_MESSAGENONCE_LENGTH];
} PubSubAesCtrChannelContext;

static size_t
encryptionKeyLength(const UA_PubSubSecurityPolicy *policy) {
    const PubSubAesCtrPolicyContext *pc =
        (const PubSubAesCtrPolicyContext*)policy->policyContext;
    if(!pc)
        return 0;
    return pc->encryptionKeyLength;
}

static UA_Boolean
validByteString(const UA_ByteString *value) {
    return value && (value->length == 0 || value->data);
}

static void
freeMacCtx(PubSubAesCtrMacCtx *ctx) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && !defined(LIBRESSL_VERSION_NUMBER)
    EVP_MAC_CTX_free(ctx);
#else
    HMAC_CTX_free(ctx);
#endif
}

static void
clearSigningKey(UA_ByteString *key) {
    if(key->data)
        OPENSSL_cleanse(key->data, key->length);
    UA_ByteString_clear(key);
}

/* Key the MAC context for one computation. Called for every message. */
static UA_StatusCode
keyMacCtx(PubSubAesCtrMacCtx *ctx, const UA_ByteString *key) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && !defined(LIBRESSL_VERSION_NUMBER)
    char digest[] = "SHA256";
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, digest, 0);
    params[1] = OSSL_PARAM_construct_end();
    if(EVP_MAC_init(ctx, key->data, key->length, params) != 1)
        return UA_STATUSCODE_BADINTERNALERROR;
#else
    if(HMAC_Init_ex(ctx, key->data, (int)key->length, EVP_sha256(), NULL) != 1)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

/* Copy the signing key and create a MAC context. The key is installed with a
 * test computation so that a bad key is rejected here and not per message. */
static UA_StatusCode
importSigningKey(const UA_ByteString *key, UA_ByteString *keyCopy,
                 PubSubAesCtrMacCtx **out) {
    UA_StatusCode res = UA_ByteString_copy(key, keyCopy);
    if(res != UA_STATUSCODE_GOOD)
        return res;
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && !defined(LIBRESSL_VERSION_NUMBER)
    EVP_MAC *mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    if(!mac) {
        clearSigningKey(keyCopy);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    PubSubAesCtrMacCtx *ctx = EVP_MAC_CTX_new(mac);
    EVP_MAC_free(mac);
#else
    PubSubAesCtrMacCtx *ctx = HMAC_CTX_new();
#endif
    if(!ctx) {
        clearSigningKey(keyCopy);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    res = keyMacCtx(ctx, keyCopy);
    if(res != UA_STATUSCODE_GOOD) {
        freeMacCtx(ctx);
        clearSigningKey(keyCopy);
        return res;
    }
    *out = ctx;
    return UA_STATUSCODE_GOOD;
}

/* The cipher context is the only copy of the AES key. EVP_CIPHER_CTX_free
 * clears the key schedule. The IV is set per message. */
static UA_StatusCode
importEncryptingKey(const UA_ByteString *key, size_t keyLength,
                    EVP_CIPHER_CTX **out) {
    const EVP_CIPHER *cipher = (keyLength == 32) ?
        EVP_aes_256_ctr() : EVP_aes_128_ctr();
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    if(EVP_EncryptInit_ex(ctx, cipher, NULL, key->data, NULL) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    *out = ctx;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
computeHmac(PubSubAesCtrChannelContext *cc, const UA_ByteString *message,
            UA_Byte *out) {
    if(!cc->macCtx)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    UA_StatusCode res = keyMacCtx(cc->macCtx, &cc->signingKey);
    if(res != UA_STATUSCODE_GOOD)
        return res;
#if OPENSSL_VERSION_NUMBER >= 0x30000000L && !defined(LIBRESSL_VERSION_NUMBER)
    size_t outLength = 0;
    if((message->length > 0 &&
        EVP_MAC_update(cc->macCtx, message->data, message->length) != 1) ||
       EVP_MAC_final(cc->macCtx, out, &outLength,
                     UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH) != 1 ||
       outLength != UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
#else
    unsigned int outLength = 0;
    if((message->length > 0 &&
        HMAC_Update(cc->macCtx, message->data, message->length) != 1) ||
       HMAC_Final(cc->macCtx, out, &outLength) != 1 ||
       outLength != UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
verify(const UA_PubSubSecurityPolicy *policy, void *gContext,
       const UA_ByteString *message, const UA_ByteString *signature) {
    (void)policy;
    if(!gContext || !message || !signature)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signature->length != UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    if(!validByteString(message) || !signature->data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Byte mac[UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH];
    UA_StatusCode res =
        computeHmac((PubSubAesCtrChannelContext*)gContext, message, mac);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_Boolean equal = UA_constantTimeEqual(mac, signature->data, sizeof(mac));
    OPENSSL_cleanse(mac, sizeof(mac));
    return equal ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADSECURITYCHECKSFAILED;
}

static UA_StatusCode
sign(const UA_PubSubSecurityPolicy *policy, void *gContext,
     const UA_ByteString *message, UA_ByteString *signature) {
    (void)policy;
    if(!gContext || !message || !signature)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(signature->length != UA_PUBSUB_AESCTR_SIGNING_KEY_LENGTH)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!validByteString(message) || !signature->data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    /* The signature is a view into the message buffer. Only the bytes are
     * written, the length stays untouched. */
    return computeHmac((PubSubAesCtrChannelContext*)gContext, message,
                       signature->data);
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

static void
buildCounterBlock(const PubSubAesCtrChannelContext *cc,
                  UA_Byte counter[UA_PUBSUB_AESCTR_BLOCK_SIZE]) {
    memcpy(counter, cc->keyNonce, UA_PUBSUB_AESCTR_KEYNONCE_LENGTH);
    memcpy(counter + UA_PUBSUB_AESCTR_KEYNONCE_LENGTH, cc->messageNonce,
           UA_PUBSUB_AESCTR_MESSAGENONCE_LENGTH);
    counter[12] = 0;
    counter[13] = 0;
    counter[14] = 0;
    counter[15] = 1;
}

/* CTR mode XORs the key stream. Encryption and decryption are the same
 * operation, so both directions use the encrypt context. */
static UA_StatusCode
pubSubCrypt(void *gContext, UA_ByteString *data) {
    if(!gContext || !data)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!validByteString(data))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    PubSubAesCtrChannelContext *cc = (PubSubAesCtrChannelContext*)gContext;
    if(!cc->cipherCtx)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    if(data->length > (size_t)INT_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(data->length == 0)
        return UA_STATUSCODE_GOOD;

    UA_Byte counter[UA_PUBSUB_AESCTR_BLOCK_SIZE];
    buildCounterBlock(cc, counter);
    /* Keep the key schedule, reset the IV and the block offset */
    if(EVP_EncryptInit_ex(cc->cipherCtx, NULL, NULL, NULL, counter) != 1)
        return UA_STATUSCODE_BADINTERNALERROR;
    /* In-place update. The CTR cipher has block size 1, so no final call */
    int outLength = 0;
    if(EVP_EncryptUpdate(cc->cipherCtx, data->data, &outLength,
                         data->data, (int)data->length) != 1 ||
       outLength < 0 || (size_t)outLength != data->length)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
pubSubEncrypt(const UA_PubSubSecurityPolicy *policy, void *gContext,
              UA_ByteString *data) {
    (void)policy;
    return pubSubCrypt(gContext, data);
}

static UA_StatusCode
pubSubDecrypt(const UA_PubSubSecurityPolicy *policy, void *gContext,
              UA_ByteString *data) {
    (void)policy;
    return pubSubCrypt(gContext, data);
}

static UA_StatusCode
generateKey(UA_PubSubSecurityPolicy *policy, void *gContext,
            const UA_ByteString *secret, const UA_ByteString *seed,
            UA_ByteString *out) {
    (void)gContext;
    if(!policy || !secret || !seed || !out)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!validByteString(secret) || !validByteString(seed) ||
       !validByteString(out))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_Openssl_Random_Key_PSHA256_Derive(secret, seed, out);
}

/* gContext may be NULL. The SKS generates key material without a group. */
static UA_StatusCode
generateNonce(UA_PubSubSecurityPolicy *policy, void *gContext,
              UA_ByteString *out) {
    (void)gContext;
    if(!policy || !out)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(!validByteString(out))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(out->length == 0)
        return UA_STATUSCODE_GOOD;
    if(out->length > (size_t)INT_MAX)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(RAND_bytes(out->data, (int)out->length) != 1)
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    return UA_STATUSCODE_GOOD;
}

static void
deleteGroupContext(UA_PubSubSecurityPolicy *policy, void *gContext) {
    (void)policy;
    PubSubAesCtrChannelContext *cc = (PubSubAesCtrChannelContext*)gContext;
    if(!cc)
        return;
    clearSigningKey(&cc->signingKey);
    freeMacCtx(cc->macCtx);
    EVP_CIPHER_CTX_free(cc->cipherCtx);
    OPENSSL_cleanse(cc, sizeof(*cc));
    UA_free(cc);
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

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(signingKey)
        res = importSigningKey(signingKey, &cc->signingKey, &cc->macCtx);
    if(res == UA_STATUSCODE_GOOD && encryptingKey)
        res = importEncryptingKey(encryptingKey, encryptionKeyLength(policy),
                                  &cc->cipherCtx);
    if(res != UA_STATUSCODE_GOOD) {
        deleteGroupContext(policy, cc);
        return res;
    }
    if(keyNonce)
        memcpy(cc->keyNonce, keyNonce->data, keyNonce->length);
    *gContext = cc;
    return UA_STATUSCODE_GOOD;
}

/* All-or-nothing: the new keys are prepared first. The installed keys are
 * left untouched if any step fails. */
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
    UA_ByteString newSigningKey = UA_BYTESTRING_NULL;
    PubSubAesCtrMacCtx *newMacCtx = NULL;
    EVP_CIPHER_CTX *newCipherCtx = NULL;
    UA_StatusCode res = importSigningKey(signingKey, &newSigningKey, &newMacCtx);
    if(res == UA_STATUSCODE_GOOD)
        res = importEncryptingKey(encryptingKey, encryptionKeyLength(policy),
                                  &newCipherCtx);
    if(res != UA_STATUSCODE_GOOD) {
        clearSigningKey(&newSigningKey);
        freeMacCtx(newMacCtx);
        EVP_CIPHER_CTX_free(newCipherCtx);
        return res;
    }
    clearSigningKey(&cc->signingKey);
    cc->signingKey = newSigningKey;
    freeMacCtx(cc->macCtx);
    cc->macCtx = newMacCtx;
    EVP_CIPHER_CTX_free(cc->cipherCtx);
    cc->cipherCtx = newCipherCtx;
    memcpy(cc->keyNonce, keyNonce->data, keyNonce->length);
    return UA_STATUSCODE_GOOD;
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

    /* Install the callbacks first. A policy whose constructor failed is still
     * cleared by UA_ServerConfig_clean. */
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

    PubSubAesCtrPolicyContext *pc =
        (PubSubAesCtrPolicyContext*)UA_malloc(sizeof(*pc));
    if(!pc)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    pc->encryptionKeyLength = keyLength;
    sp->policyContext = pc;
    UA_Openssl_Init();
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
