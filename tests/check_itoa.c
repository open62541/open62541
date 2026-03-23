/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <check.h>
#include "itoa.h"

#include <stdlib.h>
#include <string.h>

START_TEST(unsignedZeroBase10) {
    char buf[32];
    UA_UInt16 len = itoaUnsigned(0, buf, 10);
    ck_assert_uint_eq(len, 1);
    ck_assert_str_eq(buf, "0");
} END_TEST

START_TEST(unsignedOneBase10) {
    char buf[32];
    UA_UInt16 len = itoaUnsigned(1, buf, 10);
    ck_assert_uint_eq(len, 1);
    ck_assert_str_eq(buf, "1");
} END_TEST

START_TEST(unsignedLargeBase10) {
    char buf[32];
    UA_UInt16 len = itoaUnsigned(123456789, buf, 10);
    ck_assert_uint_eq(len, 9);
    ck_assert_str_eq(buf, "123456789");
} END_TEST

START_TEST(unsignedMaxBase10) {
    char buf[32];
    UA_UInt16 len = itoaUnsigned(UA_UINT64_MAX, buf, 10);
    ck_assert_str_eq(buf, "18446744073709551615");
    ck_assert_uint_eq(len, 20);
} END_TEST

START_TEST(unsignedBase16) {
    char buf[32];
    UA_UInt16 len = itoaUnsigned(255, buf, 16);
    ck_assert_str_eq(buf, "FF");
    ck_assert_uint_eq(len, 2);
} END_TEST

START_TEST(unsignedBase16Large) {
    char buf[32];
    UA_UInt16 len = itoaUnsigned(0xDEADBEEF, buf, 16);
    ck_assert_str_eq(buf, "DEADBEEF");
    ck_assert_uint_eq(len, 8);
} END_TEST

START_TEST(unsignedBase16Zero) {
    char buf[32];
    UA_UInt16 len = itoaUnsigned(0, buf, 16);
    ck_assert_str_eq(buf, "0");
    ck_assert_uint_eq(len, 1);
} END_TEST

START_TEST(unsignedBase2) {
    char buf[128];
    UA_UInt16 len = itoaUnsigned(10, buf, 2);
    ck_assert_str_eq(buf, "1010");
    ck_assert_uint_eq(len, 4);
} END_TEST

START_TEST(unsignedBase2Zero) {
    char buf[128];
    UA_UInt16 len = itoaUnsigned(0, buf, 2);
    ck_assert_str_eq(buf, "0");
    ck_assert_uint_eq(len, 1);
} END_TEST

START_TEST(unsignedBase2Large) {
    char buf[128];
    UA_UInt16 len = itoaUnsigned(255, buf, 2);
    ck_assert_str_eq(buf, "11111111");
    ck_assert_uint_eq(len, 8);
} END_TEST

START_TEST(unsignedBase8) {
    char buf[32];
    UA_UInt16 len = itoaUnsigned(8, buf, 8);
    ck_assert_str_eq(buf, "10");
    ck_assert_uint_eq(len, 2);
} END_TEST

START_TEST(unsignedBase8Value) {
    char buf[32];
    UA_UInt16 len = itoaUnsigned(0777, buf, 8);
    ck_assert_str_eq(buf, "777");
    ck_assert_uint_eq(len, 3);
} END_TEST

START_TEST(signedZero) {
    char buf[32];
    UA_UInt16 len = itoaSigned(0, buf);
    ck_assert_str_eq(buf, "0");
    ck_assert_uint_eq(len, 1);
} END_TEST

START_TEST(signedPositive) {
    char buf[32];
    UA_UInt16 len = itoaSigned(42, buf);
    ck_assert_str_eq(buf, "42");
    ck_assert_uint_eq(len, 2);
} END_TEST

START_TEST(signedNegative) {
    char buf[32];
    UA_UInt16 len = itoaSigned(-42, buf);
    ck_assert_str_eq(buf, "-42");
    ck_assert_uint_eq(len, 3);
} END_TEST

START_TEST(signedMax) {
    char buf[32];
    UA_UInt16 len = itoaSigned(UA_INT64_MAX, buf);
    ck_assert_str_eq(buf, "9223372036854775807");
    ck_assert_uint_eq(len, 19);
} END_TEST

START_TEST(signedMin) {
    /* UA_INT64_MIN is specially handled in the code */
    char buf[32];
    UA_UInt16 len = itoaSigned(UA_INT64_MIN, buf);
    ck_assert_str_eq(buf, "-9223372036854775808");
    ck_assert_uint_eq(len, 20);
} END_TEST

START_TEST(signedNegOne) {
    char buf[32];
    UA_UInt16 len = itoaSigned(-1, buf);
    ck_assert_str_eq(buf, "-1");
    ck_assert_uint_eq(len, 2);
} END_TEST

START_TEST(signedOne) {
    char buf[32];
    UA_UInt16 len = itoaSigned(1, buf);
    ck_assert_str_eq(buf, "1");
    ck_assert_uint_eq(len, 1);
} END_TEST

static Suite *testSuite_itoa(void) {
    TCase *tc_unsigned = tcase_create("itoaUnsigned");
    tcase_add_test(tc_unsigned, unsignedZeroBase10);
    tcase_add_test(tc_unsigned, unsignedOneBase10);
    tcase_add_test(tc_unsigned, unsignedLargeBase10);
    tcase_add_test(tc_unsigned, unsignedMaxBase10);
    tcase_add_test(tc_unsigned, unsignedBase16);
    tcase_add_test(tc_unsigned, unsignedBase16Large);
    tcase_add_test(tc_unsigned, unsignedBase16Zero);
    tcase_add_test(tc_unsigned, unsignedBase2);
    tcase_add_test(tc_unsigned, unsignedBase2Zero);
    tcase_add_test(tc_unsigned, unsignedBase2Large);
    tcase_add_test(tc_unsigned, unsignedBase8);
    tcase_add_test(tc_unsigned, unsignedBase8Value);

    TCase *tc_signed = tcase_create("itoaSigned");
    tcase_add_test(tc_signed, signedZero);
    tcase_add_test(tc_signed, signedPositive);
    tcase_add_test(tc_signed, signedNegative);
    tcase_add_test(tc_signed, signedMax);
    tcase_add_test(tc_signed, signedMin);
    tcase_add_test(tc_signed, signedNegOne);
    tcase_add_test(tc_signed, signedOne);

    Suite *s = suite_create("Test integer-to-ASCII conversion");
    suite_add_tcase(s, tc_unsigned);
    suite_add_tcase(s, tc_signed);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_itoa();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
