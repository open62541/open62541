/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <check.h>
#include "cj5.h"

START_TEST(parseObject) {
    const char *json = "{'a':1.0, 'b':2}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);
    double d = 0.0;
    cj5_error_code err = cj5_get_float(json, &tokens[2], &d);
    ck_assert(err == CJ5_ERROR_NONE);
    ck_assert(d == 1.0);
} END_TEST

START_TEST(parseUTF8) {
    const char *json = "{'a':\"Lindestra\\u00dfe\"}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);
    char buf[32];
    cj5_error_code err = cj5_get_str(json, &tokens[2], buf, NULL);
    ck_assert(err == CJ5_ERROR_NONE);
} END_TEST

START_TEST(parseNestedObject) {
    const char *json = "{'a':{}, 'b':{'c':3}}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_NONE);
} END_TEST

START_TEST(parseObjectUnquoted) {
    const char *json = "{'a':1, b:2}";
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

START_TEST(parseObjectCloseNoRoot) {
    const char *json = "'a':1, 'b':2}";
    cj5_token tokens[32];
    cj5_result r = cj5_parse(json, (unsigned int)strlen(json), tokens, 32);
    ck_assert(r.error == CJ5_ERROR_INVALID);
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
    tcase_add_test(tc_parse, parseObjectCloseNoRoot);

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

