/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <check.h>
#include "base64.h"

#include <stdlib.h>
#include <string.h>

/* RFC 4648 test vectors */
START_TEST(encodeEmpty) {
    size_t out_len = 42;
    unsigned char *out = UA_base64((const unsigned char *)"", 0, &out_len);
    ck_assert_uint_eq(out_len, 0);
    ck_assert_ptr_eq(out, UA_EMPTY_ARRAY_SENTINEL);
} END_TEST

START_TEST(encodeF) {
    size_t out_len = 0;
    unsigned char *out = UA_base64((const unsigned char *)"f", 1, &out_len);
    ck_assert_uint_eq(out_len, 4);
    ck_assert(memcmp(out, "Zg==", 4) == 0);
    UA_free(out);
} END_TEST

START_TEST(encodeFo) {
    size_t out_len = 0;
    unsigned char *out = UA_base64((const unsigned char *)"fo", 2, &out_len);
    ck_assert_uint_eq(out_len, 4);
    ck_assert(memcmp(out, "Zm8=", 4) == 0);
    UA_free(out);
} END_TEST

START_TEST(encodeFoo) {
    size_t out_len = 0;
    unsigned char *out = UA_base64((const unsigned char *)"foo", 3, &out_len);
    ck_assert_uint_eq(out_len, 4);
    ck_assert(memcmp(out, "Zm9v", 4) == 0);
    UA_free(out);
} END_TEST

START_TEST(encodeFoob) {
    size_t out_len = 0;
    unsigned char *out = UA_base64((const unsigned char *)"foob", 4, &out_len);
    ck_assert_uint_eq(out_len, 8);
    ck_assert(memcmp(out, "Zm9vYg==", 8) == 0);
    UA_free(out);
} END_TEST

START_TEST(encodeFooba) {
    size_t out_len = 0;
    unsigned char *out = UA_base64((const unsigned char *)"fooba", 5, &out_len);
    ck_assert_uint_eq(out_len, 8);
    ck_assert(memcmp(out, "Zm9vYmE=", 8) == 0);
    UA_free(out);
} END_TEST

START_TEST(encodeFoobar) {
    size_t out_len = 0;
    unsigned char *out = UA_base64((const unsigned char *)"foobar", 6, &out_len);
    ck_assert_uint_eq(out_len, 8);
    ck_assert(memcmp(out, "Zm9vYmFy", 8) == 0);
    UA_free(out);
} END_TEST

START_TEST(encodeBuf) {
    /* Test UA_base64_buf with pre-allocated buffer */
    unsigned char buf[8];
    size_t len = UA_base64_buf((const unsigned char *)"foo", 3, buf);
    ck_assert_uint_eq(len, 4);
    ck_assert(memcmp(buf, "Zm9v", 4) == 0);
} END_TEST

START_TEST(encodeBufPadding1) {
    unsigned char buf[8];
    size_t len = UA_base64_buf((const unsigned char *)"fo", 2, buf);
    ck_assert_uint_eq(len, 4);
    ck_assert(memcmp(buf, "Zm8=", 4) == 0);
} END_TEST

START_TEST(encodeBufPadding2) {
    unsigned char buf[8];
    size_t len = UA_base64_buf((const unsigned char *)"f", 1, buf);
    ck_assert_uint_eq(len, 4);
    ck_assert(memcmp(buf, "Zg==", 4) == 0);
} END_TEST

START_TEST(encodeBinaryData) {
    /* Test encoding binary data with all byte values 0x00-0x02 */
    unsigned char data[] = {0x00, 0x01, 0x02};
    size_t out_len = 0;
    unsigned char *out = UA_base64(data, 3, &out_len);
    ck_assert_uint_eq(out_len, 4);
    ck_assert(memcmp(out, "AAEC", 4) == 0);
    UA_free(out);
} END_TEST

/* Decode tests */
START_TEST(decodeEmpty) {
    size_t out_len = 42;
    unsigned char *out = UA_unbase64((const unsigned char *)"", 0, &out_len);
    ck_assert_uint_eq(out_len, 0);
    ck_assert_ptr_eq(out, UA_EMPTY_ARRAY_SENTINEL);
} END_TEST

START_TEST(decodeF) {
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Zg==", 4, &out_len);
    ck_assert_ptr_ne(out, NULL);
    ck_assert_uint_eq(out_len, 1);
    ck_assert(out[0] == 'f');
    UA_free(out);
} END_TEST

START_TEST(decodeFo) {
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Zm8=", 4, &out_len);
    ck_assert_ptr_ne(out, NULL);
    ck_assert_uint_eq(out_len, 2);
    ck_assert(memcmp(out, "fo", 2) == 0);
    UA_free(out);
} END_TEST

START_TEST(decodeFoo) {
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Zm9v", 4, &out_len);
    ck_assert_ptr_ne(out, NULL);
    ck_assert_uint_eq(out_len, 3);
    ck_assert(memcmp(out, "foo", 3) == 0);
    UA_free(out);
} END_TEST

START_TEST(decodeFoobar) {
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Zm9vYmFy", 8, &out_len);
    ck_assert_ptr_ne(out, NULL);
    ck_assert_uint_eq(out_len, 6);
    ck_assert(memcmp(out, "foobar", 6) == 0);
    UA_free(out);
} END_TEST

START_TEST(decodeWithWhitespace) {
    /* RFC 2045 allows whitespace (tabs, newlines, spaces) */
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Zm9v\nYmFy", 9, &out_len);
    ck_assert_ptr_ne(out, NULL);
    ck_assert_uint_eq(out_len, 6);
    ck_assert(memcmp(out, "foobar", 6) == 0);
    UA_free(out);
} END_TEST

START_TEST(decodeWithTab) {
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Zm9v\tYmFy", 9, &out_len);
    ck_assert_ptr_ne(out, NULL);
    ck_assert_uint_eq(out_len, 6);
    ck_assert(memcmp(out, "foobar", 6) == 0);
    UA_free(out);
} END_TEST

START_TEST(decodeInvalidChar) {
    /* '!' is not a valid base64 character */
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Zm9!YmFy", 8, &out_len);
    ck_assert_ptr_eq(out, NULL);
} END_TEST

START_TEST(decodeNonAscii) {
    /* Byte with high bit set */
    unsigned char input[] = {'Z', 'm', 0x80, 'v'};
    size_t out_len = 0;
    unsigned char *out = UA_unbase64(input, 4, &out_len);
    ck_assert_ptr_eq(out, NULL);
} END_TEST

START_TEST(decodePaddingTooEarly) {
    /* Padding as first byte in a block */
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"=m9v", 4, &out_len);
    ck_assert_ptr_eq(out, NULL);
} END_TEST

START_TEST(decodePaddingSecond) {
    /* Padding as second byte in a block */
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Z===", 4, &out_len);
    ck_assert_ptr_eq(out, NULL);
} END_TEST

START_TEST(decodePaddingThree) {
    /* Three padding chars is invalid */
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Z===", 4, &out_len);
    ck_assert_ptr_eq(out, NULL);
} END_TEST

START_TEST(decodeNotMultipleOfFour) {
    /* Input not a multiple of 4 → error */
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Zm9vY", 5, &out_len);
    ck_assert_ptr_eq(out, NULL);
} END_TEST

START_TEST(decodePaddingNotTerminated) {
    /* Padding followed by non-padding within a block */
    size_t out_len = 0;
    unsigned char *out = UA_unbase64((const unsigned char *)"Zm=v", 4, &out_len);
    ck_assert_ptr_eq(out, NULL);
} END_TEST

/* Roundtrip tests */
START_TEST(roundtripEmpty) {
    size_t enc_len = 0;
    unsigned char *enc = UA_base64((const unsigned char *)"", 0, &enc_len);
    ck_assert_uint_eq(enc_len, 0);

    size_t dec_len = 0;
    unsigned char *dec = UA_unbase64(enc, enc_len, &dec_len);
    ck_assert_uint_eq(dec_len, 0);
} END_TEST

START_TEST(roundtripBinary) {
    /* Roundtrip with all bytes 0x00..0xFF */
    unsigned char data[256];
    for(int i = 0; i < 256; i++)
        data[i] = (unsigned char)i;

    size_t enc_len = 0;
    unsigned char *enc = UA_base64(data, 256, &enc_len);
    ck_assert_ptr_ne(enc, NULL);
    ck_assert_uint_gt(enc_len, 0);

    size_t dec_len = 0;
    unsigned char *dec = UA_unbase64(enc, enc_len, &dec_len);
    ck_assert_ptr_ne(dec, NULL);
    ck_assert_uint_eq(dec_len, 256);
    ck_assert(memcmp(data, dec, 256) == 0);

    UA_free(enc);
    UA_free(dec);
} END_TEST

START_TEST(roundtripVariousLengths) {
    /* Test roundtrip for lengths 1..20 */
    for(size_t len = 1; len <= 20; len++) {
        unsigned char data[20];
        for(size_t i = 0; i < len; i++)
            data[i] = (unsigned char)(i * 7 + 13);

        size_t enc_len = 0;
        unsigned char *enc = UA_base64(data, len, &enc_len);
        ck_assert_ptr_ne(enc, NULL);

        size_t dec_len = 0;
        unsigned char *dec = UA_unbase64(enc, enc_len, &dec_len);
        ck_assert_ptr_ne(dec, NULL);
        ck_assert_uint_eq(dec_len, len);
        ck_assert(memcmp(data, dec, len) == 0);

        UA_free(enc);
        UA_free(dec);
    }
} END_TEST

static Suite *testSuite_base64(void) {
    TCase *tc_encode = tcase_create("base64_encode");
    tcase_add_test(tc_encode, encodeEmpty);
    tcase_add_test(tc_encode, encodeF);
    tcase_add_test(tc_encode, encodeFo);
    tcase_add_test(tc_encode, encodeFoo);
    tcase_add_test(tc_encode, encodeFoob);
    tcase_add_test(tc_encode, encodeFooba);
    tcase_add_test(tc_encode, encodeFoobar);
    tcase_add_test(tc_encode, encodeBuf);
    tcase_add_test(tc_encode, encodeBufPadding1);
    tcase_add_test(tc_encode, encodeBufPadding2);
    tcase_add_test(tc_encode, encodeBinaryData);

    TCase *tc_decode = tcase_create("base64_decode");
    tcase_add_test(tc_decode, decodeEmpty);
    tcase_add_test(tc_decode, decodeF);
    tcase_add_test(tc_decode, decodeFo);
    tcase_add_test(tc_decode, decodeFoo);
    tcase_add_test(tc_decode, decodeFoobar);
    tcase_add_test(tc_decode, decodeWithWhitespace);
    tcase_add_test(tc_decode, decodeWithTab);
    tcase_add_test(tc_decode, decodeInvalidChar);
    tcase_add_test(tc_decode, decodeNonAscii);
    tcase_add_test(tc_decode, decodePaddingTooEarly);
    tcase_add_test(tc_decode, decodePaddingSecond);
    tcase_add_test(tc_decode, decodePaddingThree);
    tcase_add_test(tc_decode, decodeNotMultipleOfFour);
    tcase_add_test(tc_decode, decodePaddingNotTerminated);

    TCase *tc_roundtrip = tcase_create("base64_roundtrip");
    tcase_add_test(tc_roundtrip, roundtripEmpty);
    tcase_add_test(tc_roundtrip, roundtripBinary);
    tcase_add_test(tc_roundtrip, roundtripVariousLengths);

    Suite *s = suite_create("Test Base64 encoding/decoding");
    suite_add_tcase(s, tc_encode);
    suite_add_tcase(s, tc_decode);
    suite_add_tcase(s, tc_roundtrip);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_base64();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
