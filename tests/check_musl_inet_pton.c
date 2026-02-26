/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <check.h>
#include "musl_inet_pton.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* IPv4 tests */

START_TEST(ipv4Loopback) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "127.0.0.1", addr);
    ck_assert_int_eq(ret, 1);
    ck_assert_uint_eq(addr[0], 127);
    ck_assert_uint_eq(addr[1], 0);
    ck_assert_uint_eq(addr[2], 0);
    ck_assert_uint_eq(addr[3], 1);
} END_TEST

START_TEST(ipv4AllZeros) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "0.0.0.0", addr);
    ck_assert_int_eq(ret, 1);
    ck_assert_uint_eq(addr[0], 0);
    ck_assert_uint_eq(addr[1], 0);
    ck_assert_uint_eq(addr[2], 0);
    ck_assert_uint_eq(addr[3], 0);
} END_TEST

START_TEST(ipv4AllOnes) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "255.255.255.255", addr);
    ck_assert_int_eq(ret, 1);
    ck_assert_uint_eq(addr[0], 255);
    ck_assert_uint_eq(addr[1], 255);
    ck_assert_uint_eq(addr[2], 255);
    ck_assert_uint_eq(addr[3], 255);
} END_TEST

START_TEST(ipv4Typical) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "192.168.1.100", addr);
    ck_assert_int_eq(ret, 1);
    ck_assert_uint_eq(addr[0], 192);
    ck_assert_uint_eq(addr[1], 168);
    ck_assert_uint_eq(addr[2], 1);
    ck_assert_uint_eq(addr[3], 100);
} END_TEST

START_TEST(ipv4OctetTooLarge) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "256.0.0.0", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

START_TEST(ipv4TooFewOctets) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "1.2.3", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

START_TEST(ipv4TooManyOctets) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "1.2.3.4.5", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

START_TEST(ipv4LeadingZeros) {
    /* Leading zeros are rejected by musl (octal ambiguity) */
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "01.01.01.01", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

START_TEST(ipv4EmptyString) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

START_TEST(ipv4JustDots) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "...", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

START_TEST(ipv4TrailingDot) {
    unsigned char addr[4];
    int ret = musl_inet_pton(AF_INET, "1.2.3.4.", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

/* IPv6 tests */

START_TEST(ipv6Loopback) {
    unsigned char addr[16];
    int ret = musl_inet_pton(AF_INET6, "::1", addr);
    ck_assert_int_eq(ret, 1);
    /* ::1 = 15 zero bytes + 0x01 */
    for(int i = 0; i < 15; i++)
        ck_assert_uint_eq(addr[i], 0);
    ck_assert_uint_eq(addr[15], 1);
} END_TEST

START_TEST(ipv6AllZeros) {
    unsigned char addr[16];
    int ret = musl_inet_pton(AF_INET6, "::", addr);
    ck_assert_int_eq(ret, 1);
    for(int i = 0; i < 16; i++)
        ck_assert_uint_eq(addr[i], 0);
} END_TEST

START_TEST(ipv6Full) {
    unsigned char addr[16];
    int ret = musl_inet_pton(AF_INET6, "2001:0db8:85a3:0000:0000:8a2e:0370:7334", addr);
    ck_assert_int_eq(ret, 1);
    ck_assert_uint_eq(addr[0], 0x20);
    ck_assert_uint_eq(addr[1], 0x01);
    ck_assert_uint_eq(addr[2], 0x0d);
    ck_assert_uint_eq(addr[3], 0xb8);
} END_TEST

START_TEST(ipv6Compressed) {
    unsigned char addr[16];
    int ret = musl_inet_pton(AF_INET6, "fe80::1", addr);
    ck_assert_int_eq(ret, 1);
    ck_assert_uint_eq(addr[0], 0xfe);
    ck_assert_uint_eq(addr[1], 0x80);
    ck_assert_uint_eq(addr[15], 1);
} END_TEST

START_TEST(ipv6EmbeddedV4) {
    /* ::ffff:192.0.2.1 */
    unsigned char addr[16];
    int ret = musl_inet_pton(AF_INET6, "::ffff:192.0.2.1", addr);
    ck_assert_int_eq(ret, 1);
    /* Last 4 bytes should be the IPv4 address */
    ck_assert_uint_eq(addr[12], 192);
    ck_assert_uint_eq(addr[13], 0);
    ck_assert_uint_eq(addr[14], 2);
    ck_assert_uint_eq(addr[15], 1);
} END_TEST

START_TEST(ipv6MultipleDoubleColon) {
    /* Multiple :: is invalid */
    unsigned char addr[16];
    int ret = musl_inet_pton(AF_INET6, "fe80::1::2", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

START_TEST(ipv6TooManyGroups) {
    unsigned char addr[16];
    int ret = musl_inet_pton(AF_INET6, "1:2:3:4:5:6:7:8:9", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

START_TEST(ipv6EmptyString) {
    unsigned char addr[16];
    int ret = musl_inet_pton(AF_INET6, "", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

/* Bad address family */
START_TEST(badAddressFamily) {
    unsigned char addr[16];
    errno = 0;
    int ret = musl_inet_pton(999, "127.0.0.1", addr);
    ck_assert_int_eq(ret, -1);
    ck_assert_int_eq(errno, EAFNOSUPPORT);
} END_TEST

START_TEST(ipv6SingleColon) {
    /* A single leading colon (not ::) is invalid */
    unsigned char addr[16];
    int ret = musl_inet_pton(AF_INET6, ":1", addr);
    ck_assert_int_eq(ret, 0);
} END_TEST

static Suite *testSuite_inet_pton(void) {
    TCase *tc_v4 = tcase_create("inet_pton_ipv4");
    tcase_add_test(tc_v4, ipv4Loopback);
    tcase_add_test(tc_v4, ipv4AllZeros);
    tcase_add_test(tc_v4, ipv4AllOnes);
    tcase_add_test(tc_v4, ipv4Typical);
    tcase_add_test(tc_v4, ipv4OctetTooLarge);
    tcase_add_test(tc_v4, ipv4TooFewOctets);
    tcase_add_test(tc_v4, ipv4TooManyOctets);
    tcase_add_test(tc_v4, ipv4LeadingZeros);
    tcase_add_test(tc_v4, ipv4EmptyString);
    tcase_add_test(tc_v4, ipv4JustDots);
    tcase_add_test(tc_v4, ipv4TrailingDot);

    TCase *tc_v6 = tcase_create("inet_pton_ipv6");
    tcase_add_test(tc_v6, ipv6Loopback);
    tcase_add_test(tc_v6, ipv6AllZeros);
    tcase_add_test(tc_v6, ipv6Full);
    tcase_add_test(tc_v6, ipv6Compressed);
    tcase_add_test(tc_v6, ipv6EmbeddedV4);
    tcase_add_test(tc_v6, ipv6MultipleDoubleColon);
    tcase_add_test(tc_v6, ipv6TooManyGroups);
    tcase_add_test(tc_v6, ipv6EmptyString);
    tcase_add_test(tc_v6, ipv6SingleColon);

    TCase *tc_misc = tcase_create("inet_pton_misc");
    tcase_add_test(tc_misc, badAddressFamily);

    Suite *s = suite_create("Test musl_inet_pton address parsing");
    suite_add_tcase(s, tc_v4);
    suite_add_tcase(s, tc_v6);
    suite_add_tcase(s, tc_misc);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_inet_pton();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
