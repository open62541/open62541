/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include "pcg_basic.h"

#include <stdlib.h>
#include <string.h>

START_TEST(deterministicSequence) {
    /* Same seed → same sequence */
    pcg32_random_t rng1, rng2;
    pcg32_srandom_r(&rng1, 42u, 54u);
    pcg32_srandom_r(&rng2, 42u, 54u);
    for(int i = 0; i < 100; i++) {
        uint32_t v1 = pcg32_random_r(&rng1);
        uint32_t v2 = pcg32_random_r(&rng2);
        ck_assert_uint_eq(v1, v2);
    }
} END_TEST

START_TEST(differentSeeds) {
    /* Different seeds → (almost certainly) different sequences */
    pcg32_random_t rng1, rng2;
    pcg32_srandom_r(&rng1, 42u, 54u);
    pcg32_srandom_r(&rng2, 123u, 456u);
    int differ = 0;
    for(int i = 0; i < 100; i++) {
        uint32_t v1 = pcg32_random_r(&rng1);
        uint32_t v2 = pcg32_random_r(&rng2);
        if(v1 != v2)
            differ++;
    }
    /* At least most values should differ */
    ck_assert_int_gt(differ, 90);
} END_TEST

START_TEST(differentSequences) {
    /* Same state, different sequence → different output */
    pcg32_random_t rng1, rng2;
    pcg32_srandom_r(&rng1, 42u, 1u);
    pcg32_srandom_r(&rng2, 42u, 2u);
    int differ = 0;
    for(int i = 0; i < 100; i++) {
        uint32_t v1 = pcg32_random_r(&rng1);
        uint32_t v2 = pcg32_random_r(&rng2);
        if(v1 != v2)
            differ++;
    }
    ck_assert_int_gt(differ, 80);
} END_TEST

START_TEST(zeroSeed) {
    /* Zero seed should not crash */
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, 0u, 0u);
    uint32_t val = pcg32_random_r(&rng);
    (void)val; /* Just ensure no crash */
} END_TEST

START_TEST(defaultInitializer) {
    /* PCG32_INITIALIZER should produce consistent output */
    pcg32_random_t rng = PCG32_INITIALIZER;
    uint32_t v1 = pcg32_random_r(&rng);
    uint32_t v2 = pcg32_random_r(&rng);
    /* Values should not be equal (extremely unlikely) */
    (void)v1;
    (void)v2;
    /* Just ensure it works without crashing */
} END_TEST

START_TEST(rangeCheck) {
    /* Generate many values and verify some basic statistical properties */
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, 12345u, 67890u);
    int has_high_bit = 0;
    int has_low_bit = 0;
    for(int i = 0; i < 1000; i++) {
        uint32_t val = pcg32_random_r(&rng);
        if(val & 0x80000000u)
            has_high_bit = 1;
        if(val & 1u)
            has_low_bit = 1;
    }
    /* Over 1000 samples, should exercise both high and low bits */
    ck_assert_int_eq(has_high_bit, 1);
    ck_assert_int_eq(has_low_bit, 1);
} END_TEST

START_TEST(incIsOdd) {
    /* The increment should always be odd after seeding */
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, 42u, 54u);
    ck_assert(rng.inc & 1u);

    pcg32_srandom_r(&rng, 0u, 0u);
    ck_assert(rng.inc & 1u);

    pcg32_srandom_r(&rng, 100u, 100u);
    ck_assert(rng.inc & 1u);
} END_TEST

static Suite *testSuite_pcg_basic(void) {
    TCase *tc = tcase_create("pcg32");
    tcase_add_test(tc, deterministicSequence);
    tcase_add_test(tc, differentSeeds);
    tcase_add_test(tc, differentSequences);
    tcase_add_test(tc, zeroSeed);
    tcase_add_test(tc, defaultInitializer);
    tcase_add_test(tc, rangeCheck);
    tcase_add_test(tc, incIsOdd);

    Suite *s = suite_create("Test PCG random number generator");
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_pcg_basic();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
