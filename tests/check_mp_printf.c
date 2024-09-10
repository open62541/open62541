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

static Suite *testSuite_mp_printf(void) {
    TCase *tc_print = tcase_create("mp_printf");
    tcase_add_test(tc_print, printHex);
    tcase_add_test(tc_print, printNodeId);
    tcase_add_test(tc_print, printString);

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
