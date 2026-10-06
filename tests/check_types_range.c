/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>

#include "ua_server_internal.h"

#include <stdlib.h>
#include <check.h>

START_TEST(parseRange) {
    UA_NumericRange range;
    UA_String str = UA_STRING("1:2,0:3,5");
    UA_StatusCode retval = UA_NumericRange_parse(&range, str);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensionsSize,3);
    ck_assert_uint_eq(range.dimensions[0].min,1);
    ck_assert_uint_eq(range.dimensions[0].max,2);
    ck_assert_uint_eq(range.dimensions[1].min,0);
    ck_assert_uint_eq(range.dimensions[1].max,3);
    ck_assert_uint_eq(range.dimensions[2].min,5);
    ck_assert_uint_eq(range.dimensions[2].max,5);
    UA_free(range.dimensions);
} END_TEST

START_TEST(parseRangeMinEqualMax) {
    UA_NumericRange range;
    UA_String str = UA_STRING("1:2,1");
    UA_StatusCode retval = UA_NumericRange_parse(&range, str);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensionsSize,2);
    ck_assert_uint_eq(range.dimensions[0].min,1);
    ck_assert_uint_eq(range.dimensions[0].max,2);
    ck_assert_uint_eq(range.dimensions[1].min,1);
    ck_assert_uint_eq(range.dimensions[1].max,1);
    UA_free(range.dimensions);
} END_TEST

/* An index that does not fit into 32 bits has a valid syntax. It must not wrap
 * around to a small index (Part 4, 7.27). */
START_TEST(parseRangeIndexOverflow) {
    UA_NumericRange range;
    UA_String str = UA_STRING("4294967297");
    UA_StatusCode retval = UA_NumericRange_parse(&range, str);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensions[0].min, UA_UINT32_MAX);
    ck_assert_uint_eq(range.dimensions[0].max, UA_UINT32_MAX);
    UA_free(range.dimensions);

    str = UA_STRING("1:99999999999");
    retval = UA_NumericRange_parse(&range, str);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensions[0].min, 1);
    ck_assert_uint_eq(range.dimensions[0].max, UA_UINT32_MAX);
    UA_free(range.dimensions);
} END_TEST

/* Beyond the lower bound there is no data, beyond the upper bound the result
 * is partial (Part 4, 7.27) */
START_TEST(copyArrayRangeIndexOverflow) {
    UA_Variant v, v2;
    UA_Variant_init(&v);
    UA_Variant_init(&v2);
    UA_UInt32 arr[5] = {1,2,3,4,5};
    UA_Variant_setArray(&v, arr, 5, &UA_TYPES[UA_TYPES_UINT32]);

    UA_NumericRange r;
    UA_StatusCode retval = UA_NumericRange_parse(&r, UA_STRING("4294967297"));
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Variant_copyRange(&v, &v2, r);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADINDEXRANGENODATA);
    UA_Variant_clear(&v2);
    UA_free(r.dimensions);

    retval = UA_NumericRange_parse(&r, UA_STRING("3:4294967297"));
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Variant_copyRange(&v, &v2, r);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v2.arrayLength, 2);
    ck_assert_uint_eq(((UA_UInt32*)v2.data)[0], 4);
    ck_assert_uint_eq(((UA_UInt32*)v2.data)[1], 5);
    UA_Variant_clear(&v2);
    UA_free(r.dimensions);
} END_TEST

/* No value can be indexed with more than 100 dimensions */
START_TEST(parseRangeTooManyDimensions) {
    char buf[2 * 101];
    for(size_t i = 0; i < 101; i++) {
        buf[2 * i] = '0';
        buf[2 * i + 1] = ',';
    }

    UA_NumericRange range;
    UA_String str = {2 * 100 - 1, (UA_Byte*)buf}; /* 100 dimensions */
    UA_StatusCode retval = UA_NumericRange_parse(&range, str);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensionsSize, 100);
    UA_free(range.dimensions);

    str.length = 2 * 101 - 1; /* 101 dimensions */
    retval = UA_NumericRange_parse(&range, str);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADINDEXRANGENODATA);
} END_TEST

START_TEST(copySimpleArrayRange) {
   UA_Variant v, v2;
   UA_Variant_init(&v);
   UA_Variant_init(&v2);
   UA_UInt32 arr[5] = {1,2,3,4,5};
   UA_Variant_setArray(&v, arr, 5, &UA_TYPES[UA_TYPES_UINT32]);

   UA_NumericRange r;
   UA_String sr = UA_STRING("1:3");
   UA_StatusCode retval = UA_NumericRange_parse(&r, sr);
   ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

   retval = UA_Variant_copyRange(&v, &v2, r);
   ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
   ck_assert_uint_eq(3, v2.arrayLength);
   ck_assert_uint_eq(2, *(UA_UInt32*)v2.data);

   UA_Variant_clear(&v2);
   UA_free(r.dimensions);
}
END_TEST

START_TEST(copyIntoStringArrayRange) {
    UA_Variant v, v2;
    UA_Variant_init(&v);
    UA_Variant_init(&v2);
    UA_String arr[2];
    arr[0] = UA_STRING("abcd");
    arr[1] = UA_STRING("wxyz");
    UA_Variant_setArray(&v, arr, 5, &UA_TYPES[UA_TYPES_STRING]);

    UA_NumericRange r;
    UA_String sr = UA_STRING("0:1,1:2");
    UA_StatusCode retval = UA_NumericRange_parse(&r, sr);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Variant_copyRange(&v, &v2, r);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(2, v2.arrayLength);

    UA_String s1 = UA_STRING("bc");
    UA_String s2 = UA_STRING("xy");
    UA_String *arr2 = (UA_String*)v2.data;
    ck_assert(UA_String_equal(&arr2[0], &s1));
    ck_assert(UA_String_equal(&arr2[1], &s2));

    UA_Variant_clear(&v2);
    UA_free(r.dimensions);
}
END_TEST

START_TEST(copyArrayRangeUpperBoundOutOfRange) {
    UA_Variant v, v2;
    UA_Variant_init(&v);
    UA_Variant_init(&v2);
    UA_UInt32 arr[5] = {1,2,3,4,5};
    UA_UInt32 arraySize = 5;
    UA_Variant_setArray(&v, arr, 5, &UA_TYPES[UA_TYPES_UINT32]);
    v.arrayDimensionsSize = 1;
    v.arrayDimensions = &arraySize;

    UA_NumericRange r;
    UA_String sr = UA_STRING("3:5");
    UA_StatusCode retval = UA_NumericRange_parse(&r, sr);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Variant_copyRange(&v, &v2, r);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(2, v2.arrayLength);
    ck_assert_uint_eq(4, *(UA_UInt32*)v2.data);
    ck_assert_uint_eq(5, *((UA_UInt32*)v2.data+1));
    ck_assert_uint_eq(1, v2.arrayDimensionsSize);
    ck_assert_uint_eq(2, v2.arrayDimensions[0]);

    UA_Variant_clear(&v2);
    UA_free(r.dimensions);
}
END_TEST

int main(void) {
    Suite *s  = suite_create("Test Variant Range Access");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, parseRange);
    tcase_add_test(tc, parseRangeMinEqualMax);
    tcase_add_test(tc, parseRangeIndexOverflow);
    tcase_add_test(tc, copyArrayRangeIndexOverflow);
    tcase_add_test(tc, parseRangeTooManyDimensions);
    tcase_add_test(tc, copySimpleArrayRange);
    tcase_add_test(tc, copyIntoStringArrayRange);
    tcase_add_test(tc, copyArrayRangeUpperBoundOutOfRange);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
