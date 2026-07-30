/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/types.h>

#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "certificates.h"

typedef UA_StatusCode (*PolicyInit)(UA_SecurityPolicy *policy,
                                    const UA_ByteString localCertificate,
                                    const UA_ByteString localPrivateKey,
                                    const UA_Logger *logger);

static UA_ByteString
makeCert(void) {
    UA_ByteString c;
    c.length = CERT_DER_LENGTH;
    c.data = CERT_DER_DATA;
    return c;
}

static UA_ByteString
makeKey(void) {
    UA_ByteString k;
    k.length = KEY_DER_LENGTH;
    k.data = KEY_DER_DATA;
    return k;
}

/* Exercise all crypto primitives of a single RSA SecurityPolicy using the
 * local certificate as the (self) remote certificate. This covers the sign /
 * verify / encrypt / decrypt callbacks plus the various size getters and the
 * thumbprint / nonce helpers - including a number of error branches. */
static void
exercisePolicy(PolicyInit init) {
    UA_ByteString cert = makeCert();
    UA_ByteString key = makeKey();

    UA_SecurityPolicy policy;
    UA_StatusCode rv = init(&policy, cert, key, UA_Log_Stdout);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    /* New channel context: remote certificate == local certificate */
    void *cc = NULL;
    rv = policy.newChannelContext(&policy, &cert, &cc);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(cc, NULL);

    /* newChannelContext with an invalid certificate must fail */
    void *badcc = NULL;
    UA_ByteString badCert = UA_BYTESTRING("not a certificate");
    rv = policy.newChannelContext(&policy, &badCert, &badcc);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    /* compareCertificate */
    rv = policy.compareCertificate(&policy, cc, &cert);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.compareCertificate(&policy, cc, &badCert);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    /* ---- Asymmetric signature round trip ---- */
    const UA_SecurityPolicySignatureAlgorithm *asymSig =
        &policy.asymSignatureAlgorithm;
    UA_ByteString msg = UA_BYTESTRING("The quick brown fox jumps over the lazy dog");

    size_t localSigSize = asymSig->getLocalSignatureSize(&policy, cc);
    ck_assert_uint_gt(localSigSize, 0);
    size_t remoteSigSize = asymSig->getRemoteSignatureSize(&policy, cc);
    ck_assert_uint_eq(localSigSize, remoteSigSize);

    UA_ByteString asigBuf;
    rv = UA_ByteString_allocBuffer(&asigBuf, localSigSize);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = asymSig->sign(&policy, cc, &msg, &asigBuf);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = asymSig->verify(&policy, cc, &msg, &asigBuf);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    /* Corrupted signature must not verify */
    asigBuf.data[0] = (UA_Byte)(asigBuf.data[0] ^ 0xFF);
    rv = asymSig->verify(&policy, cc, &msg, &asigBuf);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    /* NULL arguments are rejected */
    rv = asymSig->sign(&policy, cc, NULL, &asigBuf);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    rv = asymSig->verify(&policy, cc, &msg, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&asigBuf);

    /* ---- Asymmetric encryption round trip ---- */
    const UA_SecurityPolicyEncryptionAlgorithm *asymEnc =
        &policy.asymEncryptionAlgorithm;
    size_t ptBlock = asymEnc->getRemotePlainTextBlockSize(&policy, cc);
    size_t ctBlock = asymEnc->getRemoteBlockSize(&policy, cc);
    ck_assert_uint_gt(ptBlock, 0);
    ck_assert_uint_ge(ctBlock, ptBlock);
    if(asymEnc->getRemoteKeyLength)
        ck_assert_uint_gt(asymEnc->getRemoteKeyLength(&policy, cc), 0);

    /* Buffer holds one cipher block; only one plaintext block is filled */
    UA_ByteString edata;
    rv = UA_ByteString_allocBuffer(&edata, ctBlock);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    memset(edata.data, 0x42, ptBlock);
    edata.length = ptBlock;
    rv = asymEnc->encrypt(&policy, cc, &edata);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    /* After encryption the buffer holds one cipher block */
    edata.length = ctBlock;
    rv = asymEnc->decrypt(&policy, cc, &edata);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(edata.length, ptBlock);
    for(size_t i = 0; i < ptBlock; i++)
        ck_assert_uint_eq(edata.data[i], 0x42);
    UA_ByteString_clear(&edata);

    /* Decrypt of a non-block-aligned buffer must fail */
    UA_ByteString bad = UA_BYTESTRING("12345");
    rv = asymEnc->decrypt(&policy, cc, &bad);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    /* ---- Symmetric keys ---- */
    const UA_SecurityPolicySignatureAlgorithm *symSig =
        &policy.symSignatureAlgorithm;
    const UA_SecurityPolicyEncryptionAlgorithm *symEnc =
        &policy.symEncryptionAlgorithm;

    size_t symSignKeyLen = symSig->getLocalKeyLength(&policy, cc);
    size_t symEncKeyLen = symEnc->getLocalKeyLength(&policy, cc);
    size_t symBlock = symEnc->getRemoteBlockSize(&policy, cc);
    ck_assert_uint_gt(symSignKeyLen, 0);
    ck_assert_uint_gt(symEncKeyLen, 0);
    ck_assert_uint_gt(symBlock, 0);

    UA_ByteString signKey;
    UA_ByteString encKey;
    UA_ByteString iv;
    UA_ByteString_allocBuffer(&signKey, symSignKeyLen);
    UA_ByteString_allocBuffer(&encKey, symEncKeyLen);
    UA_ByteString_allocBuffer(&iv, symBlock);
    for(size_t i = 0; i < signKey.length; i++) signKey.data[i] = (UA_Byte)(i + 1);
    for(size_t i = 0; i < encKey.length; i++) encKey.data[i] = (UA_Byte)(i + 7);
    for(size_t i = 0; i < iv.length; i++) iv.data[i] = (UA_Byte)(i + 3);

    /* Use the same keys for local and remote so round trips succeed */
    ck_assert_int_eq(policy.setLocalSymSigningKey(&policy, cc, &signKey),
                     UA_STATUSCODE_GOOD);
    ck_assert_int_eq(policy.setRemoteSymSigningKey(&policy, cc, &signKey),
                     UA_STATUSCODE_GOOD);
    ck_assert_int_eq(policy.setLocalSymEncryptingKey(&policy, cc, &encKey),
                     UA_STATUSCODE_GOOD);
    ck_assert_int_eq(policy.setRemoteSymEncryptingKey(&policy, cc, &encKey),
                     UA_STATUSCODE_GOOD);
    ck_assert_int_eq(policy.setLocalSymIv(&policy, cc, &iv), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(policy.setRemoteSymIv(&policy, cc, &iv), UA_STATUSCODE_GOOD);

    /* ---- Symmetric signature round trip ---- */
    size_t symSigSize = symSig->getLocalSignatureSize(&policy, cc);
    ck_assert_uint_gt(symSigSize, 0);
    UA_ByteString ssig;
    UA_ByteString_allocBuffer(&ssig, symSigSize);
    rv = symSig->sign(&policy, cc, &msg, &ssig);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = symSig->verify(&policy, cc, &msg, &ssig);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ssig.data[0] = (UA_Byte)(ssig.data[0] ^ 0xFF);
    rv = symSig->verify(&policy, cc, &msg, &ssig);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&ssig);

    /* ---- Symmetric encryption round trip ---- */
    size_t symPt = symEnc->getRemotePlainTextBlockSize(&policy, cc);
    ck_assert_uint_gt(symPt, 0);
    UA_ByteString sdata;
    UA_ByteString_allocBuffer(&sdata, symPt * 2);
    for(size_t i = 0; i < sdata.length; i++) sdata.data[i] = (UA_Byte)(i & 0xFF);
    UA_ByteString plain;
    UA_ByteString_copy(&sdata, &plain);
    rv = symEnc->encrypt(&policy, cc, &sdata);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = symEnc->decrypt(&policy, cc, &sdata);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(sdata.length, plain.length);
    ck_assert(memcmp(sdata.data, plain.data, plain.length) == 0);
    UA_ByteString_clear(&sdata);
    UA_ByteString_clear(&plain);

    UA_ByteString_clear(&signKey);
    UA_ByteString_clear(&encKey);
    UA_ByteString_clear(&iv);

    /* ---- generateKey ---- */
    UA_ByteString secret = UA_BYTESTRING("0123456789abcdef0123456789abcdef");
    UA_ByteString seed = UA_BYTESTRING("fedcba9876543210fedcba9876543210");
    UA_ByteString derived;
    UA_ByteString_allocBuffer(&derived, 64);
    rv = policy.generateKey(&policy, cc, &secret, &seed, &derived);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&derived);

    /* ---- generateNonce ---- */
    if(policy.nonceLength > 0) {
        UA_ByteString nonce;
        UA_ByteString_allocBuffer(&nonce, policy.nonceLength);
        rv = policy.generateNonce(&policy, cc, &nonce);
        ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
        UA_ByteString_clear(&nonce);
    }

    /* ---- Thumbprint ---- */
    UA_ByteString thumb;
    UA_ByteString_allocBuffer(&thumb, 20);
    rv = policy.makeCertThumbprint(&policy, &cert, &thumb);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = policy.compareCertThumbprint(&policy, &thumb);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    /* A different thumbprint must not match */
    thumb.data[0] = (UA_Byte)(thumb.data[0] ^ 0xFF);
    rv = policy.compareCertThumbprint(&policy, &thumb);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&thumb);

    policy.deleteChannelContext(&policy, cc);
    policy.clear(&policy);
}

START_TEST(securitypolicy_basic128rsa15) {
    exercisePolicy(UA_SecurityPolicy_Basic128Rsa15);
} END_TEST

START_TEST(securitypolicy_basic256) {
    exercisePolicy(UA_SecurityPolicy_Basic256);
} END_TEST

START_TEST(securitypolicy_basic256sha256) {
    exercisePolicy(UA_SecurityPolicy_Basic256Sha256);
} END_TEST

START_TEST(securitypolicy_aes128sha256rsaoaep) {
    exercisePolicy(UA_SecurityPolicy_Aes128Sha256RsaOaep);
} END_TEST

START_TEST(securitypolicy_aes256sha256rsapss) {
    exercisePolicy(UA_SecurityPolicy_Aes256Sha256RsaPss);
} END_TEST

static Suite *
testSuite_SecurityPolicyCrypto(void) {
    Suite *s = suite_create("SecurityPolicy Crypto");
    TCase *tc = tcase_create("crypto roundtrips");
    tcase_add_test(tc, securitypolicy_basic128rsa15);
    tcase_add_test(tc, securitypolicy_basic256);
    tcase_add_test(tc, securitypolicy_basic256sha256);
    tcase_add_test(tc, securitypolicy_aes128sha256rsaoaep);
    tcase_add_test(tc, securitypolicy_aes256sha256rsapss);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_SecurityPolicyCrypto();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
