/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/* Regression tests for the HMAC verify helpers:
 *
 * 1. A tampered signature must still be rejected after switching from
 *    UA_ByteString_equal() to the constant-time UA_constantTimeEqual().
 * 2. A signature whose length does not match the expected digest size must
 *    be rejected before the fixed-length comparison, not read past its end.
 *
 * These call the plugin-internal Verify() helpers directly so a wrong-length
 * or corrupted signature can be constructed and fed in deliberately.
 */

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>

#include <string.h>

#include "check.h"

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#include "securitypolicy_openssl_common.h" /* plugins/crypto/openssl, see CMakeLists.txt */
#endif

static const UA_Byte messageBytes[] = "HMAC verify regression test message";
static const UA_ByteString message = {sizeof(messageBytes) - 1, (UA_Byte *)(uintptr_t)messageBytes};

static const UA_Byte keyBytes[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};
static const UA_ByteString key = {sizeof(keyBytes), (UA_Byte *)(uintptr_t)keyBytes};

typedef UA_StatusCode
(*SignFn)(const UA_ByteString *message, const UA_ByteString *key, UA_ByteString *signature);
typedef UA_StatusCode
(*VerifyFn)(const UA_ByteString *message, const UA_ByteString *key, const UA_ByteString *signature);

/* Sign, then verify with: (a) the untouched signature, (b) a same-length
 * signature with one flipped bit, (c) a truncated signature. */
static void
check_verify_helper(SignFn sign, VerifyFn verify, size_t macLen) {
    UA_Byte sigBuf[64];
    ck_assert_uint_le(macLen, sizeof(sigBuf));
    UA_ByteString signature = {sizeof(sigBuf), sigBuf};

    UA_StatusCode rv = sign(&message, &key, &signature);
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(signature.length, macLen);

    /* (a) correct signature must verify */
    rv = verify(&message, &key, &signature);
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);

    /* (b) same-length, corrupted signature must be rejected -- guards the
     * constant-time compare (fix 1): swapping to UA_constantTimeEqual must
     * not accidentally start accepting mismatched MACs. */
    UA_ByteString corrupted;
    UA_ByteString_copy(&signature, &corrupted);
    corrupted.data[corrupted.length - 1] ^= 0xFF;
    rv = verify(&message, &key, &corrupted);
    ck_assert_uint_ne(rv, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&corrupted);

    /* (c) shorter-than-expected signature must be rejected safely -- guards
     * the length check (fix 2): without it, a fixed-length constant-time
     * compare would read past the end of this shorter buffer. Run under
     * AddressSanitizer/Valgrind to catch a regression here. */
    UA_ByteString shortSig;
    shortSig.length = macLen > 4 ? 4 : macLen / 2;
    shortSig.data = (UA_Byte *)UA_malloc(shortSig.length);
    memcpy(shortSig.data, signature.data, shortSig.length);
    rv = verify(&message, &key, &shortSig);
    ck_assert_uint_ne(rv, UA_STATUSCODE_GOOD);
    UA_free(shortSig.data);
}

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
START_TEST(opensslHmacSha1VerifyRejectsTamperedAndShortSignatures) {
    check_verify_helper(UA_OpenSSL_HMAC_SHA1_Sign, UA_OpenSSL_HMAC_SHA1_Verify, 20);
}
END_TEST

START_TEST(opensslHmacSha256VerifyRejectsTamperedAndShortSignatures) {
    check_verify_helper(UA_OpenSSL_HMAC_SHA256_Sign, UA_OpenSSL_HMAC_SHA256_Verify, 32);
}
END_TEST
#endif /* defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL) */

static Suite *
testSuite_hmacVerifyConstantTime(void) {
    Suite *s = suite_create("HMAC verify: constant-time compare + length guard");
    TCase *tc = tcase_create("basic");

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
    tcase_add_test(tc, opensslHmacSha1VerifyRejectsTamperedAndShortSignatures);
    tcase_add_test(tc, opensslHmacSha256VerifyRejectsTamperedAndShortSignatures);
#endif

    suite_add_tcase(s, tc);
    return s;
}

int
main(void) {
    Suite *s = testSuite_hmacVerifyConstantTime();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
