/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include "libc_time.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Helper: tm_year is years since 1900 in struct musl_tm */

START_TEST(epochZero) {
    /* 1970-01-01 00:00:00 UTC */
    struct musl_tm tm;
    int ret = musl_secs_to_tm(0, &tm);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(tm.tm_year, 70);   /* 1970 - 1900 */
    ck_assert_int_eq(tm.tm_mon, 0);     /* January */
    ck_assert_int_eq(tm.tm_mday, 1);
    ck_assert_int_eq(tm.tm_hour, 0);
    ck_assert_int_eq(tm.tm_min, 0);
    ck_assert_int_eq(tm.tm_sec, 0);
    ck_assert_int_eq(tm.tm_wday, 4);    /* Thursday */
} END_TEST

START_TEST(y2kTimestamp) {
    /* 2000-01-01 00:00:00 UTC = 946684800 */
    struct musl_tm tm;
    int ret = musl_secs_to_tm(946684800LL, &tm);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(tm.tm_year, 100);  /* 2000 - 1900 */
    ck_assert_int_eq(tm.tm_mon, 0);     /* January */
    ck_assert_int_eq(tm.tm_mday, 1);
    ck_assert_int_eq(tm.tm_wday, 6);    /* Saturday */
} END_TEST

START_TEST(leapDay2000) {
    /* 2000-02-29 00:00:00 UTC = 951782400 */
    struct musl_tm tm;
    int ret = musl_secs_to_tm(951782400LL, &tm);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(tm.tm_year, 100);
    ck_assert_int_eq(tm.tm_mon, 1);     /* February (0-indexed) */
    ck_assert_int_eq(tm.tm_mday, 29);
} END_TEST

START_TEST(leapDay2024) {
    /* 2024-02-29 12:00:00 UTC = 1709208000 */
    struct musl_tm tm;
    int ret = musl_secs_to_tm(1709208000LL, &tm);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(tm.tm_year, 124);  /* 2024 - 1900 */
    ck_assert_int_eq(tm.tm_mon, 1);     /* February */
    ck_assert_int_eq(tm.tm_mday, 29);
    ck_assert_int_eq(tm.tm_hour, 12);
} END_TEST

START_TEST(beforeEpoch) {
    /* 1969-12-31 23:59:59 UTC = -1 */
    struct musl_tm tm;
    int ret = musl_secs_to_tm(-1, &tm);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(tm.tm_year, 69);   /* 1969 - 1900 */
    ck_assert_int_eq(tm.tm_mon, 11);    /* December */
    ck_assert_int_eq(tm.tm_mday, 31);
    ck_assert_int_eq(tm.tm_hour, 23);
    ck_assert_int_eq(tm.tm_min, 59);
    ck_assert_int_eq(tm.tm_sec, 59);
} END_TEST

START_TEST(year1900) {
    /* 1900 is not a leap year (divisible by 100, not by 400) */
    /* 1900-03-01 00:00:00 UTC = approx -2203891200 */
    struct musl_tm tm;
    int ret = musl_secs_to_tm(-2203891200LL, &tm);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(tm.tm_year, 0);    /* 1900 - 1900 */
    ck_assert_int_eq(tm.tm_mon, 2);     /* March */
    ck_assert_int_eq(tm.tm_mday, 1);
} END_TEST

START_TEST(endOfDay) {
    /* 1970-01-01 23:59:59 = 86399 */
    struct musl_tm tm;
    int ret = musl_secs_to_tm(86399, &tm);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(tm.tm_hour, 23);
    ck_assert_int_eq(tm.tm_min, 59);
    ck_assert_int_eq(tm.tm_sec, 59);
} END_TEST

START_TEST(startOfNextDay) {
    /* 1970-01-02 00:00:00 = 86400 */
    struct musl_tm tm;
    int ret = musl_secs_to_tm(86400, &tm);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(tm.tm_mday, 2);
    ck_assert_int_eq(tm.tm_hour, 0);
} END_TEST

START_TEST(roundtripEpoch) {
    /* secs_to_tm -> tm_to_secs should roundtrip */
    struct musl_tm tm;
    musl_secs_to_tm(0, &tm);
    long long secs = musl_tm_to_secs(&tm);
    ck_assert_int_eq(secs, 0);
} END_TEST

START_TEST(roundtripVariousTimes) {
    long long test_values[] = {
        0, 1, -1, 86400, -86400,
        946684800LL,    /* 2000-01-01 */
        1709208000LL,   /* 2024-02-29 */
        -2203891200LL,  /* 1900-03-01 */
        1000000000LL,   /* 2001-09-09 */
        2000000000LL,   /* 2033-05-18 */
    };
    for(size_t i = 0; i < sizeof(test_values)/sizeof(test_values[0]); i++) {
        struct musl_tm tm;
        int ret = musl_secs_to_tm(test_values[i], &tm);
        ck_assert_int_eq(ret, 0);
        long long back = musl_tm_to_secs(&tm);
        ck_assert_int_eq(back, test_values[i]);
    }
} END_TEST

START_TEST(tmToSecsBasic) {
    /* 1970-01-01 00:00:00 */
    struct musl_tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 70;
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    long long secs = musl_tm_to_secs(&tm);
    ck_assert_int_eq(secs, 0);
} END_TEST

START_TEST(tmToSecsY2k) {
    /* 2000-01-01 00:00:00 = 946684800 */
    struct musl_tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 100;
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    long long secs = musl_tm_to_secs(&tm);
    ck_assert_int_eq(secs, 946684800LL);
} END_TEST

START_TEST(tmToSecsMonthNormalization) {
    /* Month >= 12 should be normalized */
    struct musl_tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 70;  /* 1970 */
    tm.tm_mon = 12;   /* Should wrap to Jan next year */
    tm.tm_mday = 1;
    long long secs = musl_tm_to_secs(&tm);

    /* 1971-01-01 00:00:00 = 365*86400 = 31536000 */
    struct musl_tm tm2;
    memset(&tm2, 0, sizeof(tm2));
    tm2.tm_year = 71;
    tm2.tm_mon = 0;
    tm2.tm_mday = 1;
    long long secs2 = musl_tm_to_secs(&tm2);
    ck_assert_int_eq(secs, secs2);
} END_TEST

START_TEST(tmToSecsNegativeMonth) {
    /* Negative month should also be normalized */
    struct musl_tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 71;   /* 1971 */
    tm.tm_mon = -1;    /* Should be Dec of previous year */
    tm.tm_mday = 1;
    long long secs = musl_tm_to_secs(&tm);

    struct musl_tm tm2;
    memset(&tm2, 0, sizeof(tm2));
    tm2.tm_year = 70;
    tm2.tm_mon = 11;   /* December 1970 */
    tm2.tm_mday = 1;
    long long secs2 = musl_tm_to_secs(&tm2);
    ck_assert_int_eq(secs, secs2);
} END_TEST

START_TEST(leapYearCheck) {
    /* 2000 is a leap year (divisible by 400) */
    struct musl_tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 100;  /* 2000 */
    tm.tm_mon = 1;     /* February */
    tm.tm_mday = 29;
    long long feb29 = musl_tm_to_secs(&tm);

    tm.tm_mday = 28;
    long long feb28 = musl_tm_to_secs(&tm);
    ck_assert_int_eq(feb29 - feb28, 86400);
} END_TEST

START_TEST(centuryNonLeap) {
    /* 1900 is NOT a leap year (divisible by 100, not 400) */
    /* 2100 is NOT a leap year */
    struct musl_tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 200;  /* 2100 */
    tm.tm_mon = 2;     /* March 1 */
    tm.tm_mday = 1;
    long long mar1 = musl_tm_to_secs(&tm);

    tm.tm_mon = 1;
    tm.tm_mday = 28;
    long long feb28 = musl_tm_to_secs(&tm);

    /* If 2100 were a leap year, difference would be 2*86400; it's not, so 1*86400 */
    ck_assert_int_eq(mar1 - feb28, 86400);
} END_TEST

static Suite *testSuite_libc_time(void) {
    TCase *tc_secs_to_tm = tcase_create("secs_to_tm");
    tcase_add_test(tc_secs_to_tm, epochZero);
    tcase_add_test(tc_secs_to_tm, y2kTimestamp);
    tcase_add_test(tc_secs_to_tm, leapDay2000);
    tcase_add_test(tc_secs_to_tm, leapDay2024);
    tcase_add_test(tc_secs_to_tm, beforeEpoch);
    tcase_add_test(tc_secs_to_tm, year1900);
    tcase_add_test(tc_secs_to_tm, endOfDay);
    tcase_add_test(tc_secs_to_tm, startOfNextDay);

    TCase *tc_tm_to_secs = tcase_create("tm_to_secs");
    tcase_add_test(tc_tm_to_secs, tmToSecsBasic);
    tcase_add_test(tc_tm_to_secs, tmToSecsY2k);
    tcase_add_test(tc_tm_to_secs, tmToSecsMonthNormalization);
    tcase_add_test(tc_tm_to_secs, tmToSecsNegativeMonth);
    tcase_add_test(tc_tm_to_secs, leapYearCheck);
    tcase_add_test(tc_tm_to_secs, centuryNonLeap);

    TCase *tc_roundtrip = tcase_create("roundtrip");
    tcase_add_test(tc_roundtrip, roundtripEpoch);
    tcase_add_test(tc_roundtrip, roundtripVariousTimes);

    Suite *s = suite_create("Test libc_time conversion functions");
    suite_add_tcase(s, tc_secs_to_tm);
    suite_add_tcase(s, tc_tm_to_secs);
    suite_add_tcase(s, tc_roundtrip);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_libc_time();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
