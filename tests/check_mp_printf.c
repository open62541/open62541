/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <check.h>
#include "mp_printf.h"

START_TEST(printHex) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%.2X %X", 33, 282);
    ck_assert_int_eq(outlen, 6);
    ck_assert(strcmp(out, "21 11A") == 0);
} END_TEST

/* Custom format specifier for UA_NodeId */
START_TEST(printNodeId) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%N", UA_NODEID_NUMERIC(1,123));
    ck_assert_int_eq(outlen, 10);
    ck_assert(strcmp(out, "ns=1;i=123") == 0);
} END_TEST

/* Custom format specifier for UA_String */
START_TEST(printString) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%S", UA_STRING("open62541"));
    ck_assert_int_eq(outlen, 9);
    ck_assert(strcmp(out, "open62541") == 0);
} END_TEST

/* Signed decimal */
START_TEST(printDecimal) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%d", 42);
    ck_assert_int_eq(outlen, 2);
    ck_assert_str_eq(out, "42");
} END_TEST

START_TEST(printDecimalNegative) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%d", -42);
    ck_assert_int_eq(outlen, 3);
    ck_assert_str_eq(out, "-42");
} END_TEST

START_TEST(printDecimalZero) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%d", 0);
    ck_assert_int_eq(outlen, 1);
    ck_assert_str_eq(out, "0");
} END_TEST

START_TEST(printDecI) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%i", 99);
    ck_assert_int_eq(outlen, 2);
    ck_assert_str_eq(out, "99");
} END_TEST

/* Unsigned decimal */
START_TEST(printUnsigned) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%u", 12345);
    ck_assert_int_eq(outlen, 5);
    ck_assert_str_eq(out, "12345");
} END_TEST

START_TEST(printUnsignedZero) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%u", 0);
    ck_assert_int_eq(outlen, 1);
    ck_assert_str_eq(out, "0");
} END_TEST

/* Lowercase hex */
START_TEST(printHexLower) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%x", 255);
    ck_assert_int_eq(outlen, 2);
    ck_assert_str_eq(out, "ff");
} END_TEST

/* Octal */
START_TEST(printOctal) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%o", 8);
    ck_assert_int_eq(outlen, 2);
    ck_assert_str_eq(out, "10");
} END_TEST

/* Binary (non-standard extension) */
START_TEST(printBinary) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%b", 10);
    ck_assert_int_eq(outlen, 4);
    ck_assert_str_eq(out, "1010");
} END_TEST

/* Floating point */
START_TEST(printFloat) {
    char out[64];
    int outlen = mp_snprintf(out, 64, "%f", 3.14);
    ck_assert_int_gt(outlen, 0);
    /* Should contain digits */
    ck_assert(strstr(out, "3.14") != NULL);
} END_TEST

/* Character */
START_TEST(printChar) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%c", 'A');
    ck_assert_int_eq(outlen, 1);
    ck_assert_str_eq(out, "A");
} END_TEST

/* C string */
START_TEST(printCString) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%s", "hello");
    ck_assert_int_eq(outlen, 5);
    ck_assert_str_eq(out, "hello");
} END_TEST

START_TEST(printCStringNull) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%s", (char*)NULL);
    ck_assert_int_gt(outlen, 0);
    ck_assert_str_eq(out, "(null)");
} END_TEST

/* Pointer */
START_TEST(printPointerNull) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%p", (void*)NULL);
    ck_assert_int_gt(outlen, 0);
    ck_assert_str_eq(out, "(nil)");
} END_TEST

START_TEST(printPointerNonNull) {
    char out[64];
    int dummy = 42;
    int outlen = mp_snprintf(out, 64, "%p", (void*)&dummy);
    ck_assert_int_gt(outlen, 0);
    /* Should start with "0x" */
    ck_assert(strncmp(out, "0x", 2) == 0);
} END_TEST

/* Percent literal */
START_TEST(printPercent) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "100%%");
    ck_assert_int_eq(outlen, 4);
    ck_assert_str_eq(out, "100%");
} END_TEST

/* Width and padding */
START_TEST(printWidthRight) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%10d", 42);
    ck_assert_int_eq(outlen, 10);
    ck_assert_str_eq(out, "        42");
} END_TEST

START_TEST(printWidthLeft) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%-10d", 42);
    ck_assert_int_eq(outlen, 10);
    ck_assert_str_eq(out, "42        ");
} END_TEST

START_TEST(printZeroPad) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%010d", 42);
    ck_assert_int_eq(outlen, 10);
    ck_assert_str_eq(out, "0000000042");
} END_TEST

START_TEST(printPlusSign) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%+d", 42);
    ck_assert_int_eq(outlen, 3);
    ck_assert_str_eq(out, "+42");
} END_TEST

START_TEST(printSpaceSign) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "% d", 42);
    ck_assert_int_eq(outlen, 3);
    ck_assert_str_eq(out, " 42");
} END_TEST

START_TEST(printHashHex) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%#x", 255);
    ck_assert_int_gt(outlen, 0);
    ck_assert(strncmp(out, "0x", 2) == 0);
} END_TEST

START_TEST(printHashOctal) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%#o", 8);
    ck_assert_int_gt(outlen, 0);
    ck_assert(out[0] == '0');
} END_TEST

/* Precision with strings */
START_TEST(printStringPrecision) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%.3s", "hello");
    ck_assert_int_eq(outlen, 3);
    ck_assert_str_eq(out, "hel");
} END_TEST

/* Long length modifier */
START_TEST(printLong) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%ld", (long)42);
    ck_assert_int_eq(outlen, 2);
    ck_assert_str_eq(out, "42");
} END_TEST

START_TEST(printLongLong) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%lld", (long long)42);
    ck_assert_int_eq(outlen, 2);
    ck_assert_str_eq(out, "42");
} END_TEST

/* Size_t */
START_TEST(printSizeT) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%zu", (size_t)42);
    ck_assert_int_eq(outlen, 2);
    ck_assert_str_eq(out, "42");
} END_TEST

/* Buffer overflow protection */
START_TEST(printBufferTruncation) {
    char out[8];
    int outlen = mp_snprintf(out, 8, "hello world, this is a long string");
    /* Output should be truncated but reported length is the full length */
    ck_assert_int_gt(outlen, 7);
    /* Truncated: at most 7 chars (buf size - 1) + null terminator */
    ck_assert_uint_le(strlen(out), 7);
} END_TEST

/* UA_QualifiedName custom specifier */
START_TEST(printQualifiedName) {
    char out[64];
    UA_QualifiedName qn = UA_QUALIFIEDNAME(1, "TestName");
    int outlen = mp_snprintf(out, 64, "%Q", qn);
    ck_assert_int_gt(outlen, 0);
    /* Should contain the name */
    ck_assert(strstr(out, "TestName") != NULL);
} END_TEST

/* Width with * */
START_TEST(printWidthStar) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%*d", 10, 42);
    ck_assert_int_eq(outlen, 10);
} END_TEST

/* Precision with * */
START_TEST(printPrecisionStar) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%.*s", 3, "hello");
    ck_assert_int_eq(outlen, 3);
    ck_assert_str_eq(out, "hel");
} END_TEST

/* Character with width */
START_TEST(printCharWidth) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%5c", 'A');
    ck_assert_int_eq(outlen, 5);
    ck_assert(out[4] == 'A');
} END_TEST

/* Short modifier */
START_TEST(printShort) {
    char out[32];
    int outlen = mp_snprintf(out, 32, "%hd", (short)42);
    ck_assert_int_eq(outlen, 2);
    ck_assert_str_eq(out, "42");
} END_TEST

static Suite *testSuite_mp_printf(void) {
    TCase *tc_print = tcase_create("mp_printf");
    tcase_add_test(tc_print, printHex);
    tcase_add_test(tc_print, printNodeId);
    tcase_add_test(tc_print, printString);
    tcase_add_test(tc_print, printDecimal);
    tcase_add_test(tc_print, printDecimalNegative);
    tcase_add_test(tc_print, printDecimalZero);
    tcase_add_test(tc_print, printDecI);
    tcase_add_test(tc_print, printUnsigned);
    tcase_add_test(tc_print, printUnsignedZero);
    tcase_add_test(tc_print, printHexLower);
    tcase_add_test(tc_print, printOctal);
    tcase_add_test(tc_print, printBinary);
    tcase_add_test(tc_print, printFloat);
    tcase_add_test(tc_print, printChar);
    tcase_add_test(tc_print, printCString);
    tcase_add_test(tc_print, printCStringNull);
    tcase_add_test(tc_print, printPointerNull);
    tcase_add_test(tc_print, printPointerNonNull);
    tcase_add_test(tc_print, printPercent);
    tcase_add_test(tc_print, printWidthRight);
    tcase_add_test(tc_print, printWidthLeft);
    tcase_add_test(tc_print, printZeroPad);
    tcase_add_test(tc_print, printPlusSign);
    tcase_add_test(tc_print, printSpaceSign);
    tcase_add_test(tc_print, printHashHex);
    tcase_add_test(tc_print, printHashOctal);
    tcase_add_test(tc_print, printStringPrecision);
    tcase_add_test(tc_print, printLong);
    tcase_add_test(tc_print, printLongLong);
    tcase_add_test(tc_print, printSizeT);
    tcase_add_test(tc_print, printBufferTruncation);
    tcase_add_test(tc_print, printQualifiedName);
    tcase_add_test(tc_print, printWidthStar);
    tcase_add_test(tc_print, printPrecisionStar);
    tcase_add_test(tc_print, printCharWidth);
    tcase_add_test(tc_print, printShort);

    Suite *s = suite_create("Test custom printf handling");
    suite_add_tcase(s, tc_print);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_mp_printf();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
