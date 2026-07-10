/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types.h>

#include <check.h>
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

    rv = policy.setSecurityKeys(&policy, ctx, &signKey, &encKey, &keyNonce);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    /* Set the message nonce (used as part of the AES-CTR counter block) */
    UA_ByteString msgNonce;
    UA_ByteString_allocBuffer(&msgNonce, MESSAGENONCE_LENGTH);
    for(size_t i = 0; i < msgNonce.length; i++) msgNonce.data[i] = (UA_Byte)(i + 2);
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
    if(policy.nonceLength > 0) {
        UA_ByteString nonce;
        UA_ByteString_allocBuffer(&nonce, policy.nonceLength);
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
    rv = policy.newGroupContext(&policy, &signKey, &encKey, &keyNonce, &ctx);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(ctx, NULL);

    policy.deleteGroupContext(&policy, ctx);
    UA_ByteString_clear(&signKey);
    UA_ByteString_clear(&encKey);
    UA_ByteString_clear(&keyNonce);
    policy.clear(&policy);
} END_TEST

static Suite *
testSuite_PubSubSecurityPolicy(void) {
    Suite *s = suite_create("PubSub SecurityPolicy Crypto");
    TCase *tc = tcase_create("ctr roundtrips");
    tcase_add_test(tc, pubsub_policy_aes128ctr);
    tcase_add_test(tc, pubsub_policy_aes256ctr);
    tcase_add_test(tc, pubsub_policy_newGroupContext_withKeys);
    suite_add_tcase(s, tc);
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
