/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "open62541/util.h"
#include "check.h"

static UA_EventFilter filter;

START_TEST(Case_0) {
    char *inp = "SELECT /Message#Value, /Severity";
    UA_String case0 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case0);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(Case_1) {
    char *inp = "SELECT /Message, /Severity, /EventType "
                "WHERE OFTYPE ns=1;i=5001";
    UA_String case1 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case1);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId typeId = UA_NODEID_NUMERIC(1, 5001);
    UA_ContentFilterElement *elm = &filter.whereClause.elements[0];
    UA_LiteralOperand *op = (UA_LiteralOperand*)elm->filterOperands[0].content.decoded.data;
    ck_assert(UA_NodeId_equal(&typeId, (UA_NodeId*)op->value.data));
    UA_EventFilter_clear(&filter);
} END_TEST

START_TEST(Case_2) {
    char *inp = "SELECT /Message, /Severity, /EventType "
                "WHERE /Severity >= 1000";
    UA_String case2 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case2);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

/* Indirection */
START_TEST(Case_3) {
    char *inp = "SELECT /Message, /Severity, /EventType "
                "WHERE /Severity >= 1000";
    char *inp2 = "SELECT /Message, /Severity, /EventType "
                 "WHERE $test FOR $test := /Severity >= 1000";
    UA_EventFilter filter, filter2;
    UA_String case1 = UA_STRING(inp);
    UA_String case2 = UA_STRING(inp2);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case1);
    res |= UA_EventFilter_parse(&filter2, &case2);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_EventFilter_equal(&filter, &filter));
    UA_EventFilter_clear(&filter);
    UA_EventFilter_clear(&filter2);
} END_TEST

/* Nested Indirection */
START_TEST(Case_4) {
    char *inp = "SELECT /Message, /Severity, /EventType "
                "WHERE /Severity >= 1000";
    char *inp2 = "SELECT /Message, /Severity, /EventType "
                 "WHERE $test FOR $test := /Severity >= $value, $value := 1000";
    UA_EventFilter filter, filter2;
    UA_String case1 = UA_STRING(inp);
    UA_String case2 = UA_STRING(inp2);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case1);
    res |= UA_EventFilter_parse(&filter2, &case2);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_EventFilter_equal(&filter, &filter));
    UA_EventFilter_clear(&filter);
    UA_EventFilter_clear(&filter2);
} END_TEST

/* Parentheses */
START_TEST(Case_5) {
    char *inp = "SELECT /Severity "
                "WHERE true AND false AND true OR 123";
    char *inp2 = "SELECT /Severity "
                 "WHERE true AND (false AND (true OR 123))";
    UA_EventFilter filter, filter2;
    UA_String case1 = UA_STRING(inp);
    UA_String case2 = UA_STRING(inp2);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case1);
    res |= UA_EventFilter_parse(&filter2, &case2);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_EventFilter_equal(&filter, &filter));
    UA_EventFilter_clear(&filter);
    UA_EventFilter_clear(&filter2);
} END_TEST

/* Definition chains */
START_TEST(Case_6) {
    char *inp = "SELECT /Severity WHERE OFTYPE ns=1;i=5001";
    char *inp2 = "SELECT /Severity WHERE $test1 "
        "FOR $test1 := $test2, $test2 := OFTYPE ns=1;i=5001";
    UA_EventFilter filter, filter2;
    UA_String case1 = UA_STRING(inp);
    UA_String case2 = UA_STRING(inp2);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case1);
    res |= UA_EventFilter_parse(&filter2, &case2);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_EventFilter_equal(&filter, &filter));
    UA_EventFilter_clear(&filter);
    UA_EventFilter_clear(&filter2);
} END_TEST

/* Infinite recursion */
START_TEST(Case_7) {
    char *inp = "SELECT /Severity WHERE $test1 "
                "FOR $test1 := $test2, $test2 := $test1";
    UA_String case7 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case7);
    ck_assert_int_ne(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

/* Duplicate assignment */
START_TEST(Case_8) {
    char *inp = "SELECT /Severity WHERE $test1 "
        "FOR $test1 := 123, $test1 := 456";
    UA_String case8 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case8);
    ck_assert_int_ne(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

/* Missing definition */
START_TEST(Case_9) {
    char *inp = "SELECT /Severity WHERE $test1";
    UA_String case9 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case9);
    ck_assert_int_ne(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

/* Select clause reference */
START_TEST(Case_10) {
    char *inp = "SELECT $select1 FOR $select1 := /Severity";
    UA_String case10 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case10);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

/* Select clause missing reference */
START_TEST(Case_11) {
    char *inp = "SELECT $select1";
    UA_String case11 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case11);
    ck_assert_int_ne(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

/* JSON */
START_TEST(Case_12) {
    char *inp = "SELECT /Severity "
                "WHERE /Value == {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}";
    UA_String case1 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case1);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

/* Invalid token */
START_TEST(Case_13) {
    char *inp = "SELECT2 /Severity";
    UA_String case1 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case1);
    ck_assert_int_ne(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

/* Token does not match the grammar*/
START_TEST(Case_14) {
    char *inp = "SELECT WHERE";
    UA_String case1 = UA_STRING(inp);
    UA_StatusCode res = UA_EventFilter_parse(&filter, &case1);
    ck_assert_int_ne(res, UA_STATUSCODE_GOOD);
    UA_EventFilter_clear(&filter);
} END_TEST

int main(void) {
    Suite *s = suite_create("EventFilter Parser");
    TCase *tc_call = tcase_create("eventfilter parser - basics");
    tcase_add_test(tc_call, Case_0);
    tcase_add_test(tc_call, Case_1);
    tcase_add_test(tc_call, Case_2);
    tcase_add_test(tc_call, Case_3);
    tcase_add_test(tc_call, Case_4);
    tcase_add_test(tc_call, Case_5);
    tcase_add_test(tc_call, Case_6);
    tcase_add_test(tc_call, Case_7);
    tcase_add_test(tc_call, Case_8);
    tcase_add_test(tc_call, Case_9);
    tcase_add_test(tc_call, Case_10);
    tcase_add_test(tc_call, Case_11);
    tcase_add_test(tc_call, Case_12);
    tcase_add_test(tc_call, Case_13);
    tcase_add_test(tc_call, Case_14);
    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
