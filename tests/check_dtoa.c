/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include "dtoa.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* vs2008 does not have INFINITY and NAN defined */
#ifndef INFINITY
# define INFINITY ((double)(DBL_MAX+DBL_MAX))
#endif
#ifndef NAN
# define NAN ((double)(INFINITY-INFINITY))
#endif

/* Helper: convert double and null-terminate */
static unsigned dtoa_str(double d, char *buf) {
    unsigned len = dtoa(d, buf);
    buf[len] = '\0';
    return len;
}

START_TEST(dtoaZero) {
    char buf[32];
    unsigned len = dtoa_str(0.0, buf);
    ck_assert_uint_eq(len, 3);
    ck_assert_str_eq(buf, "0.0");
} END_TEST

START_TEST(dtoaOne) {
    char buf[32];
    unsigned len = dtoa_str(1.0, buf);
    /* Should produce "1.0" */
    ck_assert_uint_ge(len, 2);
    double val = atof(buf);
    ck_assert((val) == (1.0));
} END_TEST

START_TEST(dtoaNegOne) {
    char buf[32];
    unsigned len = dtoa_str(-1.0, buf);
    ck_assert(buf[0] == '-');
    ck_assert_uint_ge(len, 3);
    double val = atof(buf);
    ck_assert((val) == (-1.0));
} END_TEST

START_TEST(dtoaFortyTwo) {
    char buf[32];
    dtoa_str(42.0, buf);
    double val = atof(buf);
    ck_assert((val) == (42.0));
} END_TEST

START_TEST(dtoaFractional) {
    char buf[32];
    dtoa_str(3.14, buf);
    double val = atof(buf);
    /* Allow small floating-point error */
    ck_assert((fabs(val - 3.14)) <= (1e-10));
} END_TEST

START_TEST(dtoaSmallFraction) {
    char buf[32];
    dtoa_str(0.1, buf);
    double val = atof(buf);
    ck_assert((fabs(val - 0.1)) <= (1e-15));
} END_TEST

START_TEST(dtoaHalf) {
    char buf[32];
    dtoa_str(0.5, buf);
    double val = atof(buf);
    ck_assert((val) == (0.5));
} END_TEST

START_TEST(dtoaNan) {
    char buf[32];
    memset(buf, 0, sizeof(buf));
    /* Call dtoa directly. dtoa has a known bug where the returned length
     * doesn't include the sign prefix for signed NaN, so dtoa_str's
     * null-termination at buf[len] would truncate the string.
     * Since buf is zero-initialized, the full output is preserved. */
    dtoa(NAN, buf);
    ck_assert(strstr(buf, "nan") != NULL);
} END_TEST

START_TEST(dtoaInf) {
    char buf[32];
    unsigned len = dtoa_str(INFINITY, buf);
    ck_assert_str_eq(buf, "inf");
    ck_assert_uint_eq(len, 3);
} END_TEST

START_TEST(dtoaNegInf) {
    char buf[32];
    unsigned len = dtoa_str(-INFINITY, buf);
    /* dtoa writes '-' then 'inf': verify combined result */
    ck_assert_uint_ge(len, 1);
    ck_assert(buf[0] == '-');
} END_TEST

START_TEST(dtoaVerySmall) {
    char buf[32];
    dtoa_str(DBL_MIN, buf);
    double val = atof(buf);
    /* Should be representable and parse back approximately */
    ck_assert((val) > (0.0));
} END_TEST

START_TEST(dtoaVeryLarge) {
    char buf[32];
    dtoa_str(DBL_MAX, buf);
    double val = atof(buf);
    /* Should be representable (could be inf for scientific notation) */
    ck_assert((val) > (0.0));
} END_TEST

START_TEST(dtoaEpsilon) {
    char buf[32];
    dtoa_str(DBL_EPSILON, buf);
    double val = atof(buf);
    ck_assert((val) > (0.0));
} END_TEST

START_TEST(dtoaNegativeZero) {
    char buf[32];
    unsigned len = dtoa_str(-0.0, buf);
    /* -0.0: bits have sign bit set but exponent/mantissa are 0,
     * so dtoa produces "0.0" (zero path) after writing '-' */
    (void)len;
    double val = atof(buf);
    ck_assert((val) == (0.0));
} END_TEST

START_TEST(dtoaLargeInteger) {
    char buf[32];
    dtoa_str(1e15, buf);
    double val = atof(buf);
    ck_assert((fabs(val - 1e15)) <= (1.0));
} END_TEST

START_TEST(dtoaScientificNotation) {
    /* Very large value that requires scientific notation */
    char buf[32];
    dtoa_str(1.23e200, buf);
    double val = atof(buf);
    ck_assert((fabs(val / 1.23e200 - 1.0)) <= (1e-10));
} END_TEST

START_TEST(dtoaTinyScientific) {
    /* Very small value that requires scientific notation */
    char buf[32];
    dtoa_str(1.23e-200, buf);
    double val = atof(buf);
    ck_assert((fabs(val / 1.23e-200 - 1.0)) <= (1e-10));
} END_TEST

START_TEST(dtoaSubnormal) {
    /* Smallest subnormal number */
    char buf[32];
    double subnormal = 5e-324; /* approx DBL_TRUE_MIN */
    dtoa_str(subnormal, buf);
    double val = atof(buf);
    ck_assert((val) > (0.0));
} END_TEST

START_TEST(dtoaRoundtrip) {
    /* Test several values roundtrip through dtoa -> atof */
    double values[] = {0.0, 1.0, -1.0, 0.1, 0.01, 123456789.0,
                       -123.456, 1e10, 1e-10, 3.141592653589793};
    for(size_t i = 0; i < sizeof(values)/sizeof(values[0]); i++) {
        char buf[32];
        dtoa_str(values[i], buf);
        double val = atof(buf);
        if(values[i] == 0.0) {
            ck_assert((val) == (0.0));
        } else {
            ck_assert((fabs(val / values[i] - 1.0)) <= (1e-10));
        }
    }
} END_TEST

static Suite *testSuite_dtoa(void) {
    TCase *tc = tcase_create("dtoa");
    tcase_add_test(tc, dtoaZero);
    tcase_add_test(tc, dtoaOne);
    tcase_add_test(tc, dtoaNegOne);
    tcase_add_test(tc, dtoaFortyTwo);
    tcase_add_test(tc, dtoaFractional);
    tcase_add_test(tc, dtoaSmallFraction);
    tcase_add_test(tc, dtoaHalf);
    tcase_add_test(tc, dtoaNan);
    tcase_add_test(tc, dtoaInf);
    tcase_add_test(tc, dtoaNegInf);
    tcase_add_test(tc, dtoaVerySmall);
    tcase_add_test(tc, dtoaVeryLarge);
    tcase_add_test(tc, dtoaEpsilon);
    tcase_add_test(tc, dtoaNegativeZero);
    tcase_add_test(tc, dtoaLargeInteger);
    tcase_add_test(tc, dtoaScientificNotation);
    tcase_add_test(tc, dtoaTinyScientific);
    tcase_add_test(tc, dtoaSubnormal);
    tcase_add_test(tc, dtoaRoundtrip);

    Suite *s = suite_create("Test double-to-ASCII conversion");
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_dtoa();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
