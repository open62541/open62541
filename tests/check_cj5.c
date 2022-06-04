/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <check.h>
#include "cj5.h"

#include <math.h>
#include <float.h>

/* vs2008 does not have INFINITY and NAN defined */
#ifndef INFINITY
# define INFINITY ((double)(DBL_MAX+DBL_MAX))
#endif
#ifndef NAN
# define NAN ((double)(INFINITY-INFINITY))
#endif

#if defined(_MSC_VER)
# pragma warning(disable: 4056)
# pragma warning(disable: 4756)
#endif

START_TEST(parseObject) {
    const char *json = "{'a':1.0, 'b':2, 'c':'abcde'}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);
    double d = 0.0;
    cj5_error_code err = cj5_get_float(&r, 2, &d);
    ck_assert(err == CJ5_ERROR_NONE);
    ck_assert(d == 1.0);

    ck_assert_uint_eq(tokens[6].size, 5);
} END_TEST

START_TEST(parseUTF8) {
    const char *json = "{'a':\"Lindestra\\u00dfe\"}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);
    char buf[32];
    cj5_error_code err = cj5_get_str(&r, 2, buf, NULL);
    ck_assert(err == CJ5_ERROR_NONE);

    ck_assert_uint_eq(tokens[2].size, 16);
} END_TEST

START_TEST(parseNestedObject) {
    const char *json = "{'a':{}, 'b':{'c':3}, 'd':true, 'e':false, 'f':null, 'g':[]}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);

    /* Test the cj5_find method for value lookup in objects */
    unsigned int idx = 0;
    cj5_error_code err = cj5_find(&r, &idx, "b");
    ck_assert_int_eq(err, CJ5_ERROR_NONE);
    ck_assert_uint_eq(idx, 4);

    err = cj5_find(&r, &idx, "c");
    ck_assert(err == CJ5_ERROR_NONE);
    ck_assert(idx == 6);

    int64_t val = 0;
    cj5_get_int(&r, idx, &val);
    ck_assert(val == 3);

    idx = 4;
    err = cj5_find(&r, &idx, "d");
    ck_assert(err == CJ5_ERROR_NOTFOUND);

    idx = 0;
    err = cj5_find(&r, &idx, "d");
    ck_assert_int_eq(err, CJ5_ERROR_NONE);

    bool bval = 0;
    cj5_get_bool(&r, idx, &bval);
    ck_assert(bval == true);

    idx = 0;
    err = cj5_find(&r, &idx, "e");
    ck_assert_int_eq(err, CJ5_ERROR_NONE);

    cj5_get_bool(&r, idx, &bval);
    ck_assert(bval == false);

    idx = 0;
    err = cj5_find(&r, &idx, "f");
    ck_assert_int_eq(err, CJ5_ERROR_NONE);
    ck_assert(tokens[idx].type == CJ5_TOKEN_NULL);
} END_TEST

START_TEST(parseObjectUnquoted) {
    const char *json = "{'a':1, b:true}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);
} END_TEST

START_TEST(parseObjectWrongBracket) {
    const char *json = "{'a':1, 'b':2]";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_INVALID);
} END_TEST

START_TEST(parseObjectWrongBracket2) {
    const char *json = "{{'a':1, 'b':2]}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_INVALID);
} END_TEST

START_TEST(parseObjectIncomplete) {
    const char *json = "{'a':1, 'b':2";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_INCOMPLETE);
} END_TEST

START_TEST(parseObjectNoRoot) {
    const char *json = "'a':1, 'b':2";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);
} END_TEST

START_TEST(parseObjectNoRootUnquoted) {
    const char *json = "a:1, 'b':2";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);
} END_TEST

START_TEST(parseObjectCloseNoRoot) {
    const char *json = "'a':1, 'b':2}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_INVALID);
} END_TEST

START_TEST(parseArray) {
    const char *json = "[1.23456,2,3,null]";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);

    double val = 0;
    cj5_get_float(&r, 1, &val);
    ck_assert(fabs(val - 1.23456) < 0.00001);
} END_TEST

START_TEST(parseValue) {
    const char *json = "null";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);
} END_TEST

START_TEST(parseInf) {
    const char *json = "Infinity";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);

    double val = 0;
    cj5_get_float(&r, 0, &val);
    ck_assert_msg(val == INFINITY, "val: %f", val);
} END_TEST

START_TEST(parseNegInf) {
    const char *json = "-Infinity";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);

    double val = 0;
    cj5_get_float(&r, 0, &val);
    ck_assert_msg(val == -INFINITY, "val: %f", val);
} END_TEST

static Suite *testSuite_builtin_json(void) {
    TCase *tc_parse= tcase_create("cj5_parse");
    tcase_add_test(tc_parse, parseObject);
    tcase_add_test(tc_parse, parseUTF8);
    tcase_add_test(tc_parse, parseNestedObject);
    tcase_add_test(tc_parse, parseObjectUnquoted);
    tcase_add_test(tc_parse, parseObjectWrongBracket);
    tcase_add_test(tc_parse, parseObjectWrongBracket2);
    tcase_add_test(tc_parse, parseObjectIncomplete);
    tcase_add_test(tc_parse, parseObjectNoRoot);
    tcase_add_test(tc_parse, parseObjectNoRootUnquoted);
    tcase_add_test(tc_parse, parseObjectCloseNoRoot);
    tcase_add_test(tc_parse, parseArray);
    tcase_add_test(tc_parse, parseValue);
    tcase_add_test(tc_parse, parseInf);
    tcase_add_test(tc_parse, parseNegInf);

    Suite *s = suite_create("Test JSON decoding with the cj5 library");
    suite_add_tcase(s, tc_parse);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_builtin_json();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

