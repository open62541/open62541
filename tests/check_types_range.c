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

START_TEST(copyStringMatrixRange) {
    UA_String values[] = {
        UA_STRING_STATIC("a0"), UA_STRING_STATIC("a1"), UA_STRING_STATIC("a2"),
        UA_STRING_STATIC("b0"), UA_STRING_STATIC("b1"), UA_STRING_STATIC("b2")
    };
    UA_UInt32 dimensions[] = {2, 3};
    UA_Variant src;
    UA_Variant_setArray(&src, values, 6, &UA_TYPES[UA_TYPES_STRING]);
    src.arrayDimensions = dimensions;
    src.arrayDimensionsSize = 2;
    UA_NumericRangeDimension rangeDimensions[] = {{0, 1}, {1, 2}};
    UA_NumericRange range = {2, rangeDimensions};

    UA_Variant dst;
    UA_Variant_init(&dst);
    UA_StatusCode res = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&dst, &UA_TYPES[UA_TYPES_STRING]));
    ck_assert_uint_eq(dst.arrayLength, 4);
    ck_assert_uint_eq(dst.arrayDimensionsSize, 2);
    ck_assert_uint_eq(dst.arrayDimensions[0], 2);
    ck_assert_uint_eq(dst.arrayDimensions[1], 2);
    const size_t sourceIndices[] = {1, 2, 4, 5};
    UA_String *copied = (UA_String*)dst.data;
    for(size_t i = 0; i < 4; i++) {
        ck_assert(UA_String_equal(&copied[i], &values[sourceIndices[i]]));
        ck_assert_ptr_ne(copied[i].data, values[sourceIndices[i]].data);
    }
    /* The copied strings own their storage independently of the source. */
    copied[0].data[0] = 'x';
    ck_assert_int_eq(values[1].data[0], 'a');
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(copyNestedVariantRange) {
    UA_Int32 first[] = {10, 11, 12};
    UA_Int32 second[] = {20, 21, 22};
    UA_Variant values[2];
    UA_Variant_setArray(&values[0], first, 3, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setArray(&values[1], second, (_i == 0) ? 3 : 1,
                        &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant src;
    UA_Variant_setArray(&src, values, 2, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_NumericRangeDimension dimensions[] = {{0, 1}, {1, 2}};
    UA_NumericRange range = {2, dimensions};

    UA_Variant dst;
    UA_Variant_init(&dst);
    UA_StatusCode res = UA_Variant_copyRange(&src, &dst, range);
    if(_i == 0) {
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert(UA_Variant_hasArrayType(&dst, &UA_TYPES[UA_TYPES_VARIANT]));
        ck_assert_uint_eq(dst.arrayLength, 2);
        UA_Variant *copied = (UA_Variant*)dst.data;
        for(size_t i = 0; i < 2; i++) {
            ck_assert(UA_Variant_hasArrayType(&copied[i], &UA_TYPES[UA_TYPES_INT32]));
            ck_assert_uint_eq(copied[i].arrayLength, 2);
            UA_Int32 *data = (UA_Int32*)copied[i].data;
            ck_assert_int_eq(data[0], (i == 0) ? 11 : 21);
            ck_assert_int_eq(data[1], (i == 0) ? 12 : 22);
            ck_assert_ptr_ne(data, values[i].data);
        }
        ((UA_Int32*)copied[0].data)[0] = 99;
        ck_assert_int_eq(first[1], 11);
    } else {
        /* The second inner range fails after the first one was allocated.
         * The entire partial result must be released. */
        ck_assert_uint_eq(res, UA_STATUSCODE_BADINDEXRANGENODATA);
        ck_assert_ptr_null(dst.type);
        ck_assert_ptr_null(dst.data);
        ck_assert_uint_eq(dst.arrayLength, 0);
    }
    ck_assert_int_eq(first[0], 10);
    ck_assert_int_eq(second[0], 20);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(copyScalarStringInvalidRange) {
    UA_String value = UA_STRING("abcd");
    UA_Variant src;
    UA_Variant_setScalar(&src, &value, &UA_TYPES[UA_TYPES_STRING]);
    UA_NumericRangeDimension dimensions[] = {{1, 2}, {0, 0}};
    UA_NumericRange range = {1, dimensions};
    if(_i == 0) {
        dimensions[0].min = 3;
        dimensions[0].max = 1;
    } else if(_i == 1) {
        dimensions[0].min = 4;
        dimensions[0].max = 5;
    } else {
        range.dimensionsSize = 2;
    }

    UA_Variant dst;
    UA_Variant_init(&dst);
    UA_StatusCode res = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(res, (_i == 0) ? UA_STATUSCODE_BADINDEXRANGEINVALID :
                      UA_STATUSCODE_BADINDEXRANGENODATA);
    ck_assert_ptr_null(dst.type);
    ck_assert_ptr_null(dst.data);
    UA_String expected = UA_STRING("abcd");
    ck_assert(UA_String_equal(&value, &expected));
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(copyRangeInvalidShape) {
    UA_Int32 value = 42;
    UA_UInt32 dimensions[] = {1, 1};
    UA_Variant src;
    UA_Variant_init(&src);
    if(_i == 1) {
        UA_Variant_setScalar(&src, &value, &UA_TYPES[UA_TYPES_INT32]);
    } else if(_i == 2) {
        UA_Variant_setArray(&src, &value, 1, &UA_TYPES[UA_TYPES_INT32]);
        src.arrayDimensions = dimensions;
        src.arrayDimensionsSize = 2;
    }
    UA_NumericRangeDimension dimension = {0, 0};
    UA_NumericRange range = {1, &dimension};
    const UA_StatusCode expected[] = {
        UA_STATUSCODE_BADINVALIDARGUMENT,
        UA_STATUSCODE_BADINDEXRANGENODATA,
        UA_STATUSCODE_BADINDEXRANGEINVALID
    };
    UA_Variant dst;
    UA_Variant_init(&dst);
    UA_StatusCode res = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(res, expected[_i]);
    ck_assert_ptr_null(dst.type);
    ck_assert_ptr_null(dst.data);
    ck_assert_int_eq(value, 42);
    UA_Variant_clear(&dst);
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test Variant Range Access");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, parseRange);
    tcase_add_test(tc, parseRangeMinEqualMax);
    tcase_add_test(tc, copySimpleArrayRange);
    tcase_add_test(tc, copyIntoStringArrayRange);
    tcase_add_test(tc, copyArrayRangeUpperBoundOutOfRange);
    tcase_add_test(tc, copyStringMatrixRange);
    tcase_add_loop_test(tc, copyNestedVariantRange, 0, 2);
    tcase_add_loop_test(tc, copyScalarStringInvalidRange, 0, 3);
    tcase_add_loop_test(tc, copyRangeInvalidShape, 0, 3);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
