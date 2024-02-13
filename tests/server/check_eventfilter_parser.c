

#include "open62541/types_generated.h"
#include "open62541/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../common.h"
#include "check.h"


static UA_EventFilter filter;

START_TEST(Case_0) {
    char *inp = "SELECT\n"
                "PATH \"/Message\", PATH \"/0:Severity\", PATH \"/EventType\"\n"
                "WHERE\n"
                "OR($\"ref_1\", $\"ref_2\")\n"
                "FOR\n"
                "$\"ref_2\":= OFTYPE ns=1;i=5003\n"
                "$\"ref_1\":= OFTYPE i=3035";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter empty_filter;
    UA_EventFilter_init(&empty_filter);
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, &empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_1) {
    char *inp = "SELECT\n"
                "\n"
                "PATH \"/Message\",\n"
                "PATH \"/Severity\",\n"
                "PATH \"/EventType\"\n"
                "\n"
                "WHERE\n"
                "OFTYPE ns=1;i=5001";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter empty_filter;
    UA_EventFilter_init(&empty_filter);
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, &empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_2) {
    char *inp = "SELECT\n"
                "\n"
                "PATH \"/Message\", PATH \"/Severity\", PATH \"/EventType\"\n"
                "\n"
                "WHERE\n"
                "OR(OR(OR(OFTYPE ns=1;i=5002, $4), OR($5, OFTYPE i=3035)), OR($1,$2))\n"
                "\n"
                "FOR\n"
                "$1:= OFTYPE $7\n"
                "$2:= OFTYPE $8\n"
                "$4:= OFTYPE ns=1;i=5003\n"
                "$5:= OFTYPE ns=1;i=5004\n"
                "$7:= NODEID ns=1;i=5000\n"
                "$8:= ns=1;i=5001";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter empty_filter;
    UA_EventFilter_init(&empty_filter);
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, &empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_3) {
    char *inp = "SELECT\n"
                "\n"
                "PATH \"/Message\",\n"
                "PATH \"/Severity\",\n"
                "PATH \"/EventType\"\n"
                "\n"
                "WHERE\n"
                "AND((OFTYPE ns=1;i=5001), $1)\n"
                "\n"
                "FOR\n"
                "$1:=  AND($20, $30)\n"
                "$20:= 99 == Int64 99\n"
                "$30:= TYPEID i=5000 PATH \"/Severity\" > 99";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(filter, &case_);
    ck_assert_ptr_eq(filter, empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_4) {
    char *inp = "SELECT\n"
                "\n"
                "PATH \"/Message\",\n"
                "PATH \"/0:Severity\",\n"
                "PATH \"/EventType\"\n"
                "\n"
                "WHERE\n"
                "\n"
                "AND($4, TYPEID i=5000 PATH \"/Severity\" GREATERTHAN $\"ref\")\n"
                "\n"
                "FOR\n"
                "$\"ref\":= 99\n"
                "$4:= OFTYPE ns=1;i=5000";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter empty_filter;
    UA_EventFilter_init(&empty_filter);
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, &empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

static void setup(void) {
    UA_EventFilter_init(&filter);
}

static void teardown(void) {
    UA_EventFilter_clear(&filter);
}

int main(void) {
    Suite *s = suite_create("EventFilter Parser");

    TCase *tc_call = tcase_create("eventfilter parser - basics");
    tcase_add_checked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, Case_0);
    tcase_add_test(tc_call, Case_1);
    tcase_add_test(tc_call, Case_2);
    tcase_add_test(tc_call, Case_3);
    tcase_add_test(tc_call, Case_4);
    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_VERBOSE);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
