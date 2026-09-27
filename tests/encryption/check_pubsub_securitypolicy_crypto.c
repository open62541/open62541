/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2025 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types.h>

#include <check.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef UA_StatusCode (*PubSubPolicyInit)(UA_PubSubSecurityPolicy *policy,
                                          const UA_Logger *logger);

#define KEYNONCE_LENGTH 4
#define MESSAGENONCE_LENGTH 8

/* Exercise the symmetric crypto primitives of a PubSub SecurityPolicy
 * (AES-CTR). Covers sign / verify / encrypt / decrypt round trips, the key /
 * nonce setters, the size getters and a number of error branches. */
static void
exercisePubSubPolicy(PubSubPolicyInit init) {
    UA_PubSubSecurityPolicy policy;
    UA_StatusCode rv = init(&policy, UA_Log_Stdout);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    /* Create a group context with NULL keys (set later) */
    void *ctx = NULL;
    rv = policy.newGroupContext(&policy, NULL, NULL, NULL, &ctx);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(ctx, NULL);

    size_t signKeyLen = policy.getSignatureKeyLength(&policy, ctx);
    size_t encKeyLen = policy.getEncryptionKeyLength(&policy, ctx);
    ck_assert_uint_gt(signKeyLen, 0);
    ck_assert_uint_gt(encKeyLen, 0);

    UA_ByteString signKey;
    UA_ByteString encKey;
    UA_ByteString keyNonce;
    UA_ByteString_allocBuffer(&signKey, signKeyLen);
    UA_ByteString_allocBuffer(&encKey, encKeyLen);
    UA_ByteString_allocBuffer(&keyNonce, KEYNONCE_LENGTH);
    for(size_t i = 0; i < signKey.length; i++) signKey.data[i] = (UA_Byte)(i + 1);
    for(size_t i = 0; i < encKey.length; i++) encKey.data[i] = (UA_Byte)(i + 9);
    for(size_t i = 0; i < keyNonce.length; i++) keyNonce.data[i] = (UA_Byte)(i + 5);

    /* setSecurityKeys with wrong-length keys must fail */
    UA_ByteString shortKey = UA_BYTESTRING("short");
    rv = policy.setSecurityKeys(&policy, ctx, &shortKey, &encKey, &keyNonce);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    UA_ByteString malformedSignKey = {signKeyLen, NULL};
    UA_ByteString malformedEncKey = {encKeyLen, NULL};
    UA_ByteString malformedKeyNonce = {KEYNONCE_LENGTH, NULL};
    rv = policy.setSecurityKeys(&policy, ctx, &malformedSignKey, &encKey, &keyNonce);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADINVALIDARGUMENT);
    rv = policy.setSecurityKeys(&policy, ctx, &signKey, &malformedEncKey, &keyNonce);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADINVALIDARGUMENT);
    rv = policy.setSecurityKeys(&policy, ctx, &signKey, &encKey, &malformedKeyNonce);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADINVALIDARGUMENT);

    rv = policy.setSecurityKeys(&policy, ctx, &signKey, &encKey, &keyNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    /* Set the message nonce (used as part of the AES-CTR counter block) */
    UA_ByteString msgNonce;
    UA_ByteString_allocBuffer(&msgNonce, MESSAGENONCE_LENGTH);
    for(size_t i = 0; i < msgNonce.length; i++) msgNonce.data[i] = (UA_Byte)(i + 2);
    UA_ByteString malformedMsgNonce = {MESSAGENONCE_LENGTH, NULL};
    rv = policy.setMessageNonce(&policy, ctx, &malformedMsgNonce);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADINVALIDARGUMENT);
    rv = policy.setMessageNonce(&policy, ctx, &msgNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    /* ---- Signature round trip ---- */
    UA_ByteString msg = UA_BYTESTRING("PubSub network message payload bytes");
    size_t sigSize = policy.getSignatureSize(&policy, ctx);
    ck_assert_uint_gt(sigSize, 0);
    UA_ByteString sig;
    UA_ByteString_allocBuffer(&sig, sigSize);
    rv = policy.sign(&policy, ctx, &msg, &sig);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.verify(&policy, ctx, &msg, &sig);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    /* Corrupted signature must not verify */
    sig.data[0] = (UA_Byte)(sig.data[0] ^ 0xFF);
    rv = policy.verify(&policy, ctx, &msg, &sig);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    /* Wrong signature length is rejected */
    UA_ByteString badSig = UA_BYTESTRING("tooshort");
    rv = policy.verify(&policy, ctx, &msg, &badSig);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    rv = policy.sign(&policy, ctx, &msg, &badSig);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    /* NULL arguments are rejected */
    rv = policy.verify(&policy, ctx, NULL, &sig);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&sig);

    /* ---- Encryption round trip (CTR is a stream cipher) ---- */
    UA_ByteString data;
    UA_ByteString_allocBuffer(&data, 37);
    for(size_t i = 0; i < data.length; i++) data.data[i] = (UA_Byte)(i & 0xFF);
    UA_ByteString plain;
    UA_ByteString_copy(&data, &plain);
    rv = policy.encrypt(&policy, ctx, &data);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    /* The ciphertext should differ from the plaintext */
    ck_assert(memcmp(data.data, plain.data, plain.length) != 0);
    /* Reset the message nonce and decrypt to recover the plaintext */
    rv = policy.setMessageNonce(&policy, ctx, &msgNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.decrypt(&policy, ctx, &data);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(data.length, plain.length);
    ck_assert(memcmp(data.data, plain.data, plain.length) == 0);
    UA_ByteString_clear(&data);
    UA_ByteString_clear(&plain);

    /* ---- generateKey ---- */
    UA_ByteString secret = UA_BYTESTRING("0123456789abcdef0123456789abcdef");
    UA_ByteString seed = UA_BYTESTRING("fedcba9876543210fedcba9876543210");
    UA_ByteString derived;
    UA_ByteString_allocBuffer(&derived, signKeyLen + encKeyLen + KEYNONCE_LENGTH);
    rv = policy.generateKey(&policy, ctx, &secret, &seed, &derived);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&derived);

    /* ---- generateNonce ---- */
    if(policy.keyMaterialLength > 0) {
        UA_ByteString nonce;
        UA_ByteString_allocBuffer(&nonce, policy.keyMaterialLength);
        rv = policy.generateNonce(&policy, ctx, &nonce);
        ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
        UA_ByteString_clear(&nonce);
    }

    UA_ByteString_clear(&signKey);
    UA_ByteString_clear(&encKey);
    UA_ByteString_clear(&keyNonce);
    UA_ByteString_clear(&msgNonce);

    policy.deleteGroupContext(&policy, ctx);
    policy.clear(&policy);
}

START_TEST(pubsub_policy_aes128ctr) {
    exercisePubSubPolicy(UA_PubSubSecurityPolicy_Aes128Ctr);
} END_TEST

START_TEST(pubsub_policy_aes256ctr) {
    exercisePubSubPolicy(UA_PubSubSecurityPolicy_Aes256Ctr);
} END_TEST

/* A context created with keys passed directly to newGroupContext */
START_TEST(pubsub_policy_newGroupContext_withKeys) {
    UA_PubSubSecurityPolicy policy;
    UA_StatusCode rv = UA_PubSubSecurityPolicy_Aes256Ctr(&policy, UA_Log_Stdout);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    /* Determine the key lengths via a throwaway context */
    void *tmp = NULL;
    rv = policy.newGroupContext(&policy, NULL, NULL, NULL, &tmp);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    size_t signKeyLen = policy.getSignatureKeyLength(&policy, tmp);
    size_t encKeyLen = policy.getEncryptionKeyLength(&policy, tmp);
    policy.deleteGroupContext(&policy, tmp);

    UA_ByteString signKey;
    UA_ByteString encKey;
    UA_ByteString keyNonce;
    UA_ByteString_allocBuffer(&signKey, signKeyLen);
    UA_ByteString_allocBuffer(&encKey, encKeyLen);
    UA_ByteString_allocBuffer(&keyNonce, KEYNONCE_LENGTH);
    memset(signKey.data, 0x11, signKey.length);
    memset(encKey.data, 0x22, encKey.length);
    memset(keyNonce.data, 0x33, keyNonce.length);

    void *ctx = NULL;
    UA_ByteString malformedKeyNonce = {KEYNONCE_LENGTH, NULL};
    rv = policy.newGroupContext(&policy, &signKey, &encKey, &malformedKeyNonce, &ctx);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_ptr_eq(ctx, NULL);
    rv = policy.newGroupContext(&policy, &signKey, &encKey, &keyNonce, &ctx);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(ctx, NULL);

    policy.deleteGroupContext(&policy, ctx);
    UA_ByteString_clear(&signKey);
    UA_ByteString_clear(&encKey);
    UA_ByteString_clear(&keyNonce);
    policy.clear(&policy);
} END_TEST

/* Known-answer vectors. Keys and plaintext are taken from NIST SP 800-38A
 * F.5 (AES-CTR). The policy fixes the block counter of the counter block to 1
 * (OPC UA Part 14, 7.2.4.4.3.2, Table 157), so the counter block is
 * keyNonce | messageNonce | 00000001 with keyNonce f0f1f2f3 and messageNonce
 * f4f5f6f7f8f9fafb. The expected values were produced with OpenSSL 3.6:
 *
 *   printf <plaintext> | xxd -r -p | openssl enc -aes-128-ctr -K <key> \
 *       -iv f0f1f2f3f4f5f6f7f8f9fafb00000001 -nopad | xxd -p
 *   printf <plaintext> | xxd -r -p | openssl dgst -sha256 -mac HMAC \
 *       -macopt hexkey:<signing key>
 *
 * Every crypto backend has to reproduce them byte for byte. The vectors pin
 * the primitives (counter block, key stream, HMAC). The NetworkMessage
 * framing on top of them is pinned by check_pubsub_decryption. */

static const UA_Byte katPlaintext[64] = {
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11,
    0x73, 0x93, 0x17, 0x2a, 0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
    0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51, 0x30, 0xc8, 0x1c, 0x46,
    0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
    0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b,
    0xe6, 0x6c, 0x37, 0x10
};

static const UA_Byte katKeyNonce[KEYNONCE_LENGTH] = {0xf0, 0xf1, 0xf2, 0xf3};
static const UA_Byte katMessageNonce[MESSAGENONCE_LENGTH] = {
    0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb
};

static const UA_Byte katSigningKey[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

static const UA_Byte katHmacSha256[32] = {
    0x61, 0x5e, 0xa0, 0x48, 0x8c, 0xe0, 0xa5, 0xd8, 0xa6, 0x02, 0xf8, 0x85,
    0xce, 0x9f, 0x54, 0xb3, 0x55, 0xca, 0xeb, 0x11, 0x88, 0x92, 0xef, 0x3a,
    0x64, 0x5e, 0xcc, 0xda, 0x1c, 0x39, 0x19, 0x31
};

static const UA_Byte katAes128Key[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88,
    0x09, 0xcf, 0x4f, 0x3c
};

static const UA_Byte katAes128Ciphertext[64] = {
    0x28, 0x80, 0x28, 0xc7, 0x15, 0x99, 0xc5, 0xa8, 0xdd, 0x53, 0xc2, 0x67,
    0x1b, 0x86, 0xb8, 0x13, 0xab, 0x25, 0x39, 0x7a, 0xd2, 0x1f, 0x8b, 0x4b,
    0x94, 0x89, 0x2b, 0x65, 0xcf, 0x89, 0x1e, 0xdd, 0xd4, 0x7c, 0xfd, 0x8d,
    0x0e, 0xcd, 0x23, 0xa4, 0xeb, 0x8c, 0x05, 0x58, 0x45, 0x4a, 0x63, 0x44,
    0x11, 0x42, 0x07, 0x17, 0xb4, 0xd2, 0xcc, 0x75, 0xb7, 0x23, 0x99, 0xa9,
    0xc5, 0x89, 0x7f, 0x66
};

static const UA_Byte katAes256Key[32] = {
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0,
    0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

static const UA_Byte katAes256Ciphertext[64] = {
    0x7b, 0x7a, 0x7d, 0x83, 0x85, 0xf8, 0x81, 0xf3, 0x32, 0x33, 0xd9, 0xfb,
    0x04, 0x73, 0xd4, 0x2f, 0x70, 0xde, 0x90, 0x3e, 0xd0, 0xa9, 0x93, 0x8a,
    0x91, 0xf3, 0xb5, 0x29, 0x4d, 0x2a, 0x74, 0xd0, 0xdc, 0x4e, 0x5c, 0x9b,
    0x97, 0x24, 0xd8, 0x02, 0xfe, 0xab, 0x38, 0xe8, 0x73, 0x51, 0x29, 0x7e,
    0xf1, 0xf9, 0x40, 0x78, 0xb1, 0x04, 0x7a, 0x78, 0x61, 0x07, 0x47, 0xe6,
    0x8c, 0x0f, 0xa8, 0x76
};

static void
checkKnownAnswers(PubSubPolicyInit init, const UA_Byte *encKeyData,
                  size_t encKeyLength, const UA_Byte *expectedCiphertext) {
    UA_PubSubSecurityPolicy policy;
    UA_StatusCode rv = init(&policy, UA_Log_Stdout);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_ByteString signKey = {sizeof(katSigningKey), (UA_Byte*)(uintptr_t)katSigningKey};
    UA_ByteString encKey = {encKeyLength, (UA_Byte*)(uintptr_t)encKeyData};
    UA_ByteString keyNonce = {sizeof(katKeyNonce), (UA_Byte*)(uintptr_t)katKeyNonce};
    UA_ByteString msgNonce = {sizeof(katMessageNonce), (UA_Byte*)(uintptr_t)katMessageNonce};
    UA_ByteString plain = {sizeof(katPlaintext), (UA_Byte*)(uintptr_t)katPlaintext};

    void *ctx = NULL;
    rv = policy.newGroupContext(&policy, &signKey, &encKey, &keyNonce, &ctx);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(ctx, NULL);
    ck_assert_uint_eq(policy.getEncryptionKeyLength(&policy, ctx), encKeyLength);
    ck_assert_uint_eq(policy.getSignatureKeyLength(&policy, ctx), sizeof(katSigningKey));
    ck_assert_uint_eq(policy.keyMaterialLength,
                      sizeof(katSigningKey) + encKeyLength + sizeof(katKeyNonce));
    ck_assert_uint_eq(policy.messageNonceLength, sizeof(katMessageNonce));

    /* ---- Encrypt the full 64-byte buffer ---- */
    UA_Byte buf[sizeof(katPlaintext)];
    memcpy(buf, katPlaintext, sizeof(buf));
    UA_ByteString data = {sizeof(buf), buf};
    rv = policy.setMessageNonce(&policy, ctx, &msgNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.encrypt(&policy, ctx, &data);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, sizeof(buf));
    ck_assert(memcmp(buf, expectedCiphertext, sizeof(buf)) == 0);

    /* ---- Decrypt restores the plaintext ---- */
    rv = policy.setMessageNonce(&policy, ctx, &msgNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.decrypt(&policy, ctx, &data);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, sizeof(buf));
    ck_assert(memcmp(buf, katPlaintext, sizeof(buf)) == 0);

    /* ---- Partial last block: a 37-byte message is a prefix of the key
     * stream (the last block is used only in part) ---- */
    memcpy(buf, katPlaintext, 37);
    data.length = 37;
    rv = policy.setMessageNonce(&policy, ctx, &msgNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.encrypt(&policy, ctx, &data);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(data.length, 37);
    ck_assert(memcmp(buf, expectedCiphertext, 37) == 0);

    /* ---- HMAC-SHA256 signature ---- */
    UA_Byte sigBuf[sizeof(katHmacSha256)];
    UA_ByteString sig = {sizeof(sigBuf), sigBuf};
    ck_assert_uint_eq(policy.getSignatureSize(&policy, ctx), sizeof(sigBuf));
    rv = policy.sign(&policy, ctx, &plain, &sig);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sig.length, sizeof(sigBuf));
    ck_assert(memcmp(sigBuf, katHmacSha256, sizeof(sigBuf)) == 0);
    rv = policy.verify(&policy, ctx, &plain, &sig);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    /* A single flipped bit is a security check failure */
    sigBuf[sizeof(sigBuf) - 1] ^= 0x01;
    rv = policy.verify(&policy, ctx, &plain, &sig);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    /* ---- The same keys installed with setSecurityKeys give the same
     * ciphertext (second key path, used by the SKS rollover) ---- */
    void *ctx2 = NULL;
    rv = policy.newGroupContext(&policy, NULL, NULL, NULL, &ctx2);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.setSecurityKeys(&policy, ctx2, &signKey, &encKey, &keyNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.setMessageNonce(&policy, ctx2, &msgNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    memcpy(buf, katPlaintext, sizeof(buf));
    data.length = sizeof(buf);
    rv = policy.encrypt(&policy, ctx2, &data);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(memcmp(buf, expectedCiphertext, sizeof(buf)) == 0);
    memset(sigBuf, 0, sizeof(sigBuf));
    rv = policy.sign(&policy, ctx2, &plain, &sig);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert(memcmp(sigBuf, katHmacSha256, sizeof(sigBuf)) == 0);

    policy.deleteGroupContext(&policy, ctx2);
    policy.deleteGroupContext(&policy, ctx);
    policy.clear(&policy);
}

START_TEST(pubsub_policy_aes128ctr_known_answer) {
    checkKnownAnswers(UA_PubSubSecurityPolicy_Aes128Ctr, katAes128Key,
                      sizeof(katAes128Key), katAes128Ciphertext);
} END_TEST

START_TEST(pubsub_policy_aes256ctr_known_answer) {
    checkKnownAnswers(UA_PubSubSecurityPolicy_Aes256Ctr, katAes256Key,
                      sizeof(katAes256Key), katAes256Ciphertext);
} END_TEST

/* A group context without keys refuses every operation until keys are
 * installed. The SKS flow creates the context before the first key arrives, so
 * every backend has to report the same status for that window. */
static void
checkNoKeys(PubSubPolicyInit init, const UA_Byte *encKeyData, size_t encKeyLength) {
    UA_PubSubSecurityPolicy policy;
    UA_StatusCode rv = init(&policy, UA_Log_Stdout);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    void *ctx = NULL;
    rv = policy.newGroupContext(&policy, NULL, NULL, NULL, &ctx);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_ByteString msgNonce = {sizeof(katMessageNonce), (UA_Byte*)(uintptr_t)katMessageNonce};
    rv = policy.setMessageNonce(&policy, ctx, &msgNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_Byte buf[16];
    memset(buf, 0x5a, sizeof(buf));
    UA_ByteString data = {sizeof(buf), buf};
    rv = policy.encrypt(&policy, ctx, &data);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    rv = policy.decrypt(&policy, ctx, &data);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    UA_Byte sigBuf[sizeof(katHmacSha256)];
    memset(sigBuf, 0, sizeof(sigBuf));
    UA_ByteString sig = {sizeof(sigBuf), sigBuf};
    rv = policy.sign(&policy, ctx, &data, &sig);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADSECURITYCHECKSFAILED);
    rv = policy.verify(&policy, ctx, &data, &sig);
    ck_assert_uint_eq(rv, UA_STATUSCODE_BADSECURITYCHECKSFAILED);

    /* Installing keys unlocks the context */
    UA_ByteString signKey = {sizeof(katSigningKey), (UA_Byte*)(uintptr_t)katSigningKey};
    UA_ByteString encKey = {encKeyLength, (UA_Byte*)(uintptr_t)encKeyData};
    UA_ByteString keyNonce = {sizeof(katKeyNonce), (UA_Byte*)(uintptr_t)katKeyNonce};
    rv = policy.setSecurityKeys(&policy, ctx, &signKey, &encKey, &keyNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.encrypt(&policy, ctx, &data);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.sign(&policy, ctx, &data, &sig);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.verify(&policy, ctx, &data, &sig);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    policy.deleteGroupContext(&policy, ctx);
    policy.clear(&policy);
}

START_TEST(pubsub_policy_aes128ctr_no_keys) {
    checkNoKeys(UA_PubSubSecurityPolicy_Aes128Ctr, katAes128Key, sizeof(katAes128Key));
} END_TEST

START_TEST(pubsub_policy_aes256ctr_no_keys) {
    checkNoKeys(UA_PubSubSecurityPolicy_Aes256Ctr, katAes256Key, sizeof(katAes256Key));
} END_TEST

static Suite *
testSuite_PubSubSecurityPolicy(void) {
    Suite *s = suite_create("PubSub SecurityPolicy Crypto");
    TCase *tc = tcase_create("ctr roundtrips");
    tcase_add_test(tc, pubsub_policy_aes128ctr);
    tcase_add_test(tc, pubsub_policy_aes256ctr);
    tcase_add_test(tc, pubsub_policy_newGroupContext_withKeys);
    suite_add_tcase(s, tc);
    TCase *tcKat = tcase_create("known answers");
    tcase_add_test(tcKat, pubsub_policy_aes128ctr_known_answer);
    tcase_add_test(tcKat, pubsub_policy_aes256ctr_known_answer);
    tcase_add_test(tcKat, pubsub_policy_aes128ctr_no_keys);
    tcase_add_test(tcKat, pubsub_policy_aes256ctr_no_keys);
    suite_add_tcase(s, tcKat);
    return s;
}

int main(void) {
    Suite *s = testSuite_PubSubSecurityPolicy();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
