/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include "utf8.h"

#include <stdlib.h>
#include <string.h>

/* utf8_to_codepoint tests */

START_TEST(decodeAscii) {
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint((const unsigned char *)"A", 1, &codepoint);
    ck_assert_uint_eq(len, 1);
    ck_assert_uint_eq(codepoint, 0x41);
} END_TEST

START_TEST(decodeAsciiNull) {
    unsigned codepoint = 42;
    unsigned char null_byte = 0;
    unsigned len = utf8_to_codepoint(&null_byte, 1, &codepoint);
    ck_assert_uint_eq(len, 1);
    ck_assert_uint_eq(codepoint, 0);
} END_TEST

START_TEST(decodeAsciiMax) {
    unsigned codepoint = 0;
    unsigned char byte = 0x7F;
    unsigned len = utf8_to_codepoint(&byte, 1, &codepoint);
    ck_assert_uint_eq(len, 1);
    ck_assert_uint_eq(codepoint, 0x7F);
} END_TEST

START_TEST(decode2Byte) {
    /* U+00DF = ß = 0xC3 0x9F */
    unsigned char str[] = {0xC3, 0x9F};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 2, &codepoint);
    ck_assert_uint_eq(len, 2);
    ck_assert_uint_eq(codepoint, 0x00DF);
} END_TEST

START_TEST(decode2ByteMin) {
    /* U+0080 = 0xC2 0x80 (smallest 2-byte) */
    unsigned char str[] = {0xC2, 0x80};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 2, &codepoint);
    ck_assert_uint_eq(len, 2);
    ck_assert_uint_eq(codepoint, 0x0080);
} END_TEST

START_TEST(decode2ByteMax) {
    /* U+07FF = 0xDF 0xBF (largest 2-byte) */
    unsigned char str[] = {0xDF, 0xBF};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 2, &codepoint);
    ck_assert_uint_eq(len, 2);
    ck_assert_uint_eq(codepoint, 0x07FF);
} END_TEST

START_TEST(decode3Byte) {
    /* U+4E16 = 世 = 0xE4 0xB8 0x96 */
    unsigned char str[] = {0xE4, 0xB8, 0x96};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 3, &codepoint);
    ck_assert_uint_eq(len, 3);
    ck_assert_uint_eq(codepoint, 0x4E16);
} END_TEST

START_TEST(decode3ByteMin) {
    /* U+0800 = 0xE0 0xA0 0x80 (smallest 3-byte) */
    unsigned char str[] = {0xE0, 0xA0, 0x80};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 3, &codepoint);
    ck_assert_uint_eq(len, 3);
    ck_assert_uint_eq(codepoint, 0x0800);
} END_TEST

START_TEST(decode3ByteMax) {
    /* U+FFFF = 0xEF 0xBF 0xBF (largest 3-byte) */
    unsigned char str[] = {0xEF, 0xBF, 0xBF};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 3, &codepoint);
    ck_assert_uint_eq(len, 3);
    ck_assert_uint_eq(codepoint, 0xFFFF);
} END_TEST

START_TEST(decode4Byte) {
    /* U+1F600 = 😀 = 0xF0 0x9F 0x98 0x80 */
    unsigned char str[] = {0xF0, 0x9F, 0x98, 0x80};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 4, &codepoint);
    ck_assert_uint_eq(len, 4);
    ck_assert_uint_eq(codepoint, 0x1F600);
} END_TEST

START_TEST(decode4ByteMin) {
    /* U+10000 = 0xF0 0x90 0x80 0x80 (smallest 4-byte) */
    unsigned char str[] = {0xF0, 0x90, 0x80, 0x80};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 4, &codepoint);
    ck_assert_uint_eq(len, 4);
    ck_assert_uint_eq(codepoint, 0x10000);
} END_TEST

START_TEST(decode4ByteMax) {
    /* U+10FFFF = 0xF4 0x8F 0xBF 0xBF (largest valid Unicode) */
    unsigned char str[] = {0xF4, 0x8F, 0xBF, 0xBF};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 4, &codepoint);
    ck_assert_uint_eq(len, 4);
    ck_assert_uint_eq(codepoint, 0x10FFFF);
} END_TEST

START_TEST(decodeEmptyInput) {
    unsigned codepoint = 42;
    unsigned len = utf8_to_codepoint((const unsigned char *)"", 0, &codepoint);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(decodeContinuationAtStart) {
    /* 0x80 is a continuation byte, invalid at start */
    unsigned char str[] = {0x80};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 1, &codepoint);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(decodeContinuationC1) {
    /* 0xC1 is the max overlong 2-byte prefix → rejected */
    unsigned char str[] = {0xC1, 0x80};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 2, &codepoint);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(decodeOverlong2Byte) {
    /* 0xC0 0x80 encodes U+0000 as 2 bytes (overlong) */
    unsigned char str[] = {0xC0, 0x80};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 2, &codepoint);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(decodeTruncated2Byte) {
    /* 2-byte sequence with only 1 byte available */
    unsigned char str[] = {0xC3};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 1, &codepoint);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(decodeTruncated3Byte) {
    /* 3-byte sequence with only 2 bytes available */
    unsigned char str[] = {0xE4, 0xB8};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 2, &codepoint);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(decodeTruncated4Byte) {
    /* 4-byte sequence with only 3 bytes available */
    unsigned char str[] = {0xF0, 0x9F, 0x98};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 3, &codepoint);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(decodeInvalidByte) {
    /* 0xF5+ is invalid UTF-8 start byte */
    unsigned char str[] = {0xF5, 0x80, 0x80, 0x80};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 4, &codepoint);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(decodeBadContinuation) {
    /* Valid start byte but invalid continuation:
     * 0xC3 followed by 0x00 (not a continuation byte) */
    unsigned char str[] = {0xC3, 0x00};
    unsigned codepoint = 0;
    unsigned len = utf8_to_codepoint(str, 2, &codepoint);
    ck_assert_uint_eq(len, 0);
} END_TEST

/* utf8_from_codepoint tests (inline in header) */

START_TEST(encodeAscii) {
    unsigned char buf[4];
    unsigned len = utf8_from_codepoint(buf, 0x41);
    ck_assert_uint_eq(len, 1);
    ck_assert_uint_eq(buf[0], 'A');
} END_TEST

START_TEST(encode2ByteCP) {
    unsigned char buf[4];
    unsigned len = utf8_from_codepoint(buf, 0x00DF);
    ck_assert_uint_eq(len, 2);
    ck_assert_uint_eq(buf[0], 0xC3);
    ck_assert_uint_eq(buf[1], 0x9F);
} END_TEST

START_TEST(encode3ByteCP) {
    unsigned char buf[4];
    unsigned len = utf8_from_codepoint(buf, 0x4E16);
    ck_assert_uint_eq(len, 3);
    ck_assert_uint_eq(buf[0], 0xE4);
    ck_assert_uint_eq(buf[1], 0xB8);
    ck_assert_uint_eq(buf[2], 0x96);
} END_TEST

START_TEST(encode4ByteCP) {
    unsigned char buf[4];
    unsigned len = utf8_from_codepoint(buf, 0x1F600);
    ck_assert_uint_eq(len, 4);
    ck_assert_uint_eq(buf[0], 0xF0);
    ck_assert_uint_eq(buf[1], 0x9F);
    ck_assert_uint_eq(buf[2], 0x98);
    ck_assert_uint_eq(buf[3], 0x80);
} END_TEST

START_TEST(encodeOutOfRange) {
    unsigned char buf[4];
    unsigned len = utf8_from_codepoint(buf, 0x110000);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(encodeMaxValid) {
    unsigned char buf[4];
    unsigned len = utf8_from_codepoint(buf, 0x10FFFF);
    ck_assert_uint_eq(len, 4);
} END_TEST

/* utf8_length tests (inline in header) */

START_TEST(lengthAscii) {
    ck_assert_uint_eq(utf8_length(0x41), 1);
    ck_assert_uint_eq(utf8_length(0x7F), 1);
    ck_assert_uint_eq(utf8_length(0x00), 1);
} END_TEST

START_TEST(length2Byte) {
    ck_assert_uint_eq(utf8_length(0x80), 2);
    ck_assert_uint_eq(utf8_length(0x7FF), 2);
} END_TEST

START_TEST(length3Byte) {
    ck_assert_uint_eq(utf8_length(0x800), 3);
    ck_assert_uint_eq(utf8_length(0xFFFF), 3);
} END_TEST

START_TEST(length4Byte) {
    ck_assert_uint_eq(utf8_length(0x10000), 4);
    ck_assert_uint_eq(utf8_length(0x10FFFF), 4);
} END_TEST

START_TEST(lengthOutOfRange) {
    ck_assert_uint_eq(utf8_length(0x110000), 0);
} END_TEST

/* Roundtrip tests */

START_TEST(roundtripAll) {
    unsigned codepoints[] = {
        0x00, 0x41, 0x7F,           /* ASCII */
        0x80, 0xDF, 0x7FF,          /* 2-byte */
        0x800, 0x4E16, 0xFFFF,      /* 3-byte */
        0x10000, 0x1F600, 0x10FFFF  /* 4-byte */
    };
    for(size_t i = 0; i < sizeof(codepoints)/sizeof(codepoints[0]); i++) {
        unsigned char buf[4];
        unsigned enc_len = utf8_from_codepoint(buf, codepoints[i]);
        ck_assert_uint_gt(enc_len, 0);

        unsigned decoded = 0;
        unsigned dec_len = utf8_to_codepoint(buf, enc_len, &decoded);
        ck_assert_uint_eq(dec_len, enc_len);
        ck_assert_uint_eq(decoded, codepoints[i]);
    }
} END_TEST

static Suite *testSuite_utf8(void) {
    TCase *tc_decode = tcase_create("utf8_decode");
    tcase_add_test(tc_decode, decodeAscii);
    tcase_add_test(tc_decode, decodeAsciiNull);
    tcase_add_test(tc_decode, decodeAsciiMax);
    tcase_add_test(tc_decode, decode2Byte);
    tcase_add_test(tc_decode, decode2ByteMin);
    tcase_add_test(tc_decode, decode2ByteMax);
    tcase_add_test(tc_decode, decode3Byte);
    tcase_add_test(tc_decode, decode3ByteMin);
    tcase_add_test(tc_decode, decode3ByteMax);
    tcase_add_test(tc_decode, decode4Byte);
    tcase_add_test(tc_decode, decode4ByteMin);
    tcase_add_test(tc_decode, decode4ByteMax);
    tcase_add_test(tc_decode, decodeEmptyInput);
    tcase_add_test(tc_decode, decodeContinuationAtStart);
    tcase_add_test(tc_decode, decodeContinuationC1);
    tcase_add_test(tc_decode, decodeOverlong2Byte);
    tcase_add_test(tc_decode, decodeTruncated2Byte);
    tcase_add_test(tc_decode, decodeTruncated3Byte);
    tcase_add_test(tc_decode, decodeTruncated4Byte);
    tcase_add_test(tc_decode, decodeInvalidByte);
    tcase_add_test(tc_decode, decodeBadContinuation);

    TCase *tc_encode = tcase_create("utf8_encode");
    tcase_add_test(tc_encode, encodeAscii);
    tcase_add_test(tc_encode, encode2ByteCP);
    tcase_add_test(tc_encode, encode3ByteCP);
    tcase_add_test(tc_encode, encode4ByteCP);
    tcase_add_test(tc_encode, encodeOutOfRange);
    tcase_add_test(tc_encode, encodeMaxValid);

    TCase *tc_length = tcase_create("utf8_length");
    tcase_add_test(tc_length, lengthAscii);
    tcase_add_test(tc_length, length2Byte);
    tcase_add_test(tc_length, length3Byte);
    tcase_add_test(tc_length, length4Byte);
    tcase_add_test(tc_length, lengthOutOfRange);

    TCase *tc_roundtrip = tcase_create("utf8_roundtrip");
    tcase_add_test(tc_roundtrip, roundtripAll);

    Suite *s = suite_create("Test UTF-8 encoding/decoding");
    suite_add_tcase(s, tc_decode);
    suite_add_tcase(s, tc_encode);
    suite_add_tcase(s, tc_length);
    suite_add_tcase(s, tc_roundtrip);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_utf8();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
