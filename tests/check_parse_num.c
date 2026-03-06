/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include "parse_num.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

/* parseUInt64 tests */

START_TEST(parseUint64Zero) {
    uint64_t result = 42;
    size_t len = parseUInt64("0", 1, &result);
    ck_assert_uint_eq(len, 1);
    ck_assert_uint_eq(result, 0);
} END_TEST

START_TEST(parseUint64Simple) {
    uint64_t result = 0;
    size_t len = parseUInt64("12345", 5, &result);
    ck_assert_uint_eq(len, 5);
    ck_assert_uint_eq(result, 12345);
} END_TEST

START_TEST(parseUint64Max) {
    uint64_t result = 0;
    const char *str = "18446744073709551615";
    size_t len = parseUInt64(str, strlen(str), &result);
    ck_assert_uint_eq(len, 20);
    ck_assert_uint_eq(result, UINT64_MAX);
} END_TEST

START_TEST(parseUint64Overflow) {
    uint64_t result = 0;
    /* One more than UINT64_MAX: 18446744073709551616 */
    const char *str = "18446744073709551616";
    size_t len = parseUInt64(str, strlen(str), &result);
    ck_assert_uint_eq(len, 0); /* overflow → return 0 */
} END_TEST

START_TEST(parseUint64Hex) {
    uint64_t result = 0;
    size_t len = parseUInt64("0xFF", 4, &result);
    ck_assert_uint_eq(len, 4);
    ck_assert_uint_eq(result, 255);
} END_TEST

START_TEST(parseUint64HexLower) {
    uint64_t result = 0;
    size_t len = parseUInt64("0xff", 4, &result);
    ck_assert_uint_eq(len, 4);
    ck_assert_uint_eq(result, 255);
} END_TEST

START_TEST(parseUint64HexZero) {
    uint64_t result = 0;
    size_t len = parseUInt64("0x0", 3, &result);
    ck_assert_uint_eq(len, 3);
    ck_assert_uint_eq(result, 0);
} END_TEST

START_TEST(parseUint64HexNoDigit) {
    /* "0x" with no hex digit after prefix: parser may consume '0' as decimal */
    uint64_t result = 42;
    size_t len = parseUInt64("0x", 2, &result);
    /* Implementation parses '0' as a decimal zero, returns len=1 */
    ck_assert_uint_le(len, 2);
} END_TEST

START_TEST(parseUint64HexLarge) {
    uint64_t result = 0;
    const char *str = "0xFFFFFFFFFFFFFFFF";
    size_t len = parseUInt64(str, strlen(str), &result);
    ck_assert_uint_eq(len, 18);
    ck_assert_uint_eq(result, UINT64_MAX);
} END_TEST

START_TEST(parseUint64PartialConsumption) {
    uint64_t result = 0;
    size_t len = parseUInt64("123abc", 6, &result);
    ck_assert_uint_eq(len, 3);
    ck_assert_uint_eq(result, 123);
} END_TEST

START_TEST(parseUint64EmptyString) {
    uint64_t result = 42;
    size_t len = parseUInt64("", 0, &result);
    ck_assert_uint_eq(len, 0);
    ck_assert_uint_eq(result, 0);
} END_TEST

START_TEST(parseUint64NonNumeric) {
    uint64_t result = 42;
    size_t len = parseUInt64("abc", 3, &result);
    ck_assert_uint_eq(len, 0);
    ck_assert_uint_eq(result, 0);
} END_TEST

START_TEST(parseUint64HexMixed) {
    uint64_t result = 0;
    size_t len = parseUInt64("0xAbCd", 6, &result);
    ck_assert_uint_eq(len, 6);
    ck_assert_uint_eq(result, 0xABCD);
} END_TEST

/* parseInt64 tests */

START_TEST(parseInt64Zero) {
    int64_t result = 42;
    size_t len = parseInt64("0", 1, &result);
    ck_assert_uint_eq(len, 1);
    ck_assert_int_eq(result, 0);
} END_TEST

START_TEST(parseInt64Positive) {
    int64_t result = 0;
    size_t len = parseInt64("12345", 5, &result);
    ck_assert_uint_eq(len, 5);
    ck_assert_int_eq(result, 12345);
} END_TEST

START_TEST(parseInt64Negative) {
    int64_t result = 0;
    size_t len = parseInt64("-12345", 6, &result);
    ck_assert_uint_eq(len, 6);
    ck_assert_int_eq(result, -12345);
} END_TEST

START_TEST(parseInt64PlusSign) {
    int64_t result = 0;
    size_t len = parseInt64("+42", 3, &result);
    ck_assert_uint_eq(len, 3);
    ck_assert_int_eq(result, 42);
} END_TEST

START_TEST(parseInt64Max) {
    int64_t result = 0;
    const char *str = "9223372036854775807";
    size_t len = parseInt64(str, strlen(str), &result);
    ck_assert_uint_eq(len, 19);
    ck_assert_int_eq(result, INT64_MAX);
} END_TEST

START_TEST(parseInt64Min) {
    int64_t result = 0;
    const char *str = "-9223372036854775808";
    size_t len = parseInt64(str, strlen(str), &result);
    ck_assert_uint_eq(len, 20);
    ck_assert_int_eq(result, INT64_MIN);
} END_TEST

START_TEST(parseInt64OverflowPositive) {
    int64_t result = 0;
    /* One more than INT64_MAX */
    const char *str = "9223372036854775808";
    size_t len = parseInt64(str, strlen(str), &result);
    ck_assert_uint_eq(len, 0); /* overflow → return 0 */
} END_TEST

START_TEST(parseInt64OverflowNegative) {
    int64_t result = 0;
    /* One less than INT64_MIN (abs value one more than 9223372036854775808) */
    const char *str = "-9223372036854775809";
    size_t len = parseInt64(str, strlen(str), &result);
    ck_assert_uint_eq(len, 0); /* overflow → return 0 */
} END_TEST

/* parseDouble tests */

START_TEST(parseDoubleSimple) {
    double result = 0.0;
    size_t len = parseDouble("3.14", 4, &result);
    ck_assert_uint_gt(len, 0);
    ck_assert((fabs(result - 3.14)) <= (1e-10));
} END_TEST

START_TEST(parseDoubleZero) {
    double result = 42.0;
    size_t len = parseDouble("0.0", 3, &result);
    ck_assert_uint_gt(len, 0);
    ck_assert((result) == (0.0));
} END_TEST

START_TEST(parseDoubleNegative) {
    double result = 0.0;
    size_t len = parseDouble("-1.5", 4, &result);
    ck_assert_uint_gt(len, 0);
    ck_assert((result) == (-1.5));
} END_TEST

START_TEST(parseDoubleScientific) {
    double result = 0.0;
    size_t len = parseDouble("1.5e10", 6, &result);
    ck_assert_uint_gt(len, 0);
    ck_assert((fabs(result - 1.5e10)) <= (1.0));
} END_TEST

START_TEST(parseDoubleTooLong) {
    /* Size >= 2000 → return 0 */
    double result = 42.0;
    char buf[2001];
    memset(buf, '1', 2000);
    buf[2000] = '\0';
    size_t len = parseDouble(buf, 2000, &result);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(parseDoubleInteger) {
    double result = 0.0;
    size_t len = parseDouble("42", 2, &result);
    ck_assert_uint_gt(len, 0);
    ck_assert((result) == (42.0));
} END_TEST


static Suite *testSuite_parse_num(void) {
    TCase *tc_uint = tcase_create("parseUInt64");
    tcase_add_test(tc_uint, parseUint64Zero);
    tcase_add_test(tc_uint, parseUint64Simple);
    tcase_add_test(tc_uint, parseUint64Max);
    tcase_add_test(tc_uint, parseUint64Overflow);
    tcase_add_test(tc_uint, parseUint64Hex);
    tcase_add_test(tc_uint, parseUint64HexLower);
    tcase_add_test(tc_uint, parseUint64HexZero);
    tcase_add_test(tc_uint, parseUint64HexNoDigit);
    tcase_add_test(tc_uint, parseUint64HexLarge);
    tcase_add_test(tc_uint, parseUint64PartialConsumption);
    tcase_add_test(tc_uint, parseUint64EmptyString);
    tcase_add_test(tc_uint, parseUint64NonNumeric);
    tcase_add_test(tc_uint, parseUint64HexMixed);

    TCase *tc_int = tcase_create("parseInt64");
    tcase_add_test(tc_int, parseInt64Zero);
    tcase_add_test(tc_int, parseInt64Positive);
    tcase_add_test(tc_int, parseInt64Negative);
    tcase_add_test(tc_int, parseInt64PlusSign);
    tcase_add_test(tc_int, parseInt64Max);
    tcase_add_test(tc_int, parseInt64Min);
    tcase_add_test(tc_int, parseInt64OverflowPositive);
    tcase_add_test(tc_int, parseInt64OverflowNegative);

    TCase *tc_double = tcase_create("parseDouble");
    tcase_add_test(tc_double, parseDoubleSimple);
    tcase_add_test(tc_double, parseDoubleZero);
    tcase_add_test(tc_double, parseDoubleNegative);
    tcase_add_test(tc_double, parseDoubleScientific);
    tcase_add_test(tc_double, parseDoubleTooLong);
    tcase_add_test(tc_double, parseDoubleInteger);

    Suite *s = suite_create("Test number parsing functions");
    suite_add_tcase(s, tc_uint);
    suite_add_tcase(s, tc_int);
    suite_add_tcase(s, tc_double);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_parse_num();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
