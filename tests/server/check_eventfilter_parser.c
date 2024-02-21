/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "open62541/types_generated.h"
#include "open62541/util.h"
#include <stdlib.h>
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
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
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
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
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
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
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
                "$20:= {\"Type\": 3,\"Body\": 99} == INT64 99\n"
                "$30:= TYPEID i=5000 PATH \"/Severity\" > 99";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
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
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_5) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "(TYPEID s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]),\n"
                "\n"
                "TYPEID ns=1;b=\"b3BlbjYyNTQxIQ==\" PATH \"/2:Severity/1:AnotherTest\",\n"
                "\n"
                "TYPEID i=5 PATH \"/1:AnotherAnotherTest\" ATTRIBUTE 23,\n"
                "\n"
                "PATH \"/2:FirstElement/ThirdElement\" ATTRIBUTE 15 INDEX [1],\n"
                "\n"
                "PATH \"/2:Severity\"\n"
                "\n"
                "WHERE\n"
                "\n"
                "\n"
                "OR(OR({\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}, $19 ),AND(OR(PATH \"/2:FirstElement/ThirdElement\" ATTRIBUTE 15 INDEX [1],$10), $\"another\"))\n"
                "\n"
                "FOR\n"
                "\n"
                "$2:= $19 LESSTHAN $19\n"
                "\n"
                "$\"another\":= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} INLIST [{\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}, TYPEID ns=1;b=\"b3BlbjYyNTQxIQ==\" PATH \"/1:AnotherAnotherTest\"]\n"
                "\n"
                "$19:= ISNULL {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}\n"
                "\n"
                "$10:= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}  BETWEEN [FLOAT 17, TYPEID ns=1234;g=09087e75-8e5e-499b-954f-f2a9603db28a PATH \"/1:AnotherAnotherTest ATTRIBUTE 23\"]";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_6) {
    char *inp = "SELECT\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "OR($6, $4 GREATEROREQUAL STRING \"tes tSt 3 ring\" )\n"
                "\n"
                "FOR\n"
                "\n"
                "$4:= ({\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}) > STRING \"tes tSt 3 ring\"\n"
                "\n"
                "$6:= TYPEID ns=3;s=\"15 23\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20] LESSTHAN $4";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_7) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "NOT TYPEID ns=2;i=2000 PATH \"/0:Severity\" ATTRIBUTE 15 INDEX [1,9]";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_8) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "$\"another\"\n"
                "\n"
                "FOR\n"
                "\n"
                "$\"another\":= ({\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}) GREATERTHAN STRING \"tes tSt 3 ring\"";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_9) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "{\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} GREATERTHAN TYPEID ns=1234;g=09087e75-8e5e-499b-954f-f2a9603db28a PATH \"/1:AnotherAnotherTest\" ATTRIBUTE 23";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_10) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "TYPEID ns=1234;g=09087e75-8e5e-499b-954f-f2a9603db28a PATH \"/1:AnotherAnotherTest\" ATTRIBUTE 23 GREATERTHAN {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_11) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "\n"
                "OR(OR( ISNULL TYPEID ns=2;i=2000 PATH \"/0:Severity\" ATTRIBUTE 15 INDEX [1,9], FLOAT 17 LESSOREQUAL TYPEID b=\"b3BlbjYyNTQxIQ==\" PATH \"/7:OnlyPathSpecified\" ATTRIBUTE 3 INDEX [1,4:7,90]), AND({\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} GREATERTHAN STRING \"tes tSt 3 ring\", ($6) GREATEROREQUAL STRING \"tes tSt 3 ring\" ))\n"
                "\n"
                "FOR\n"
                "\n"
                "$19:= NOT TYPEID ns=2;i=2000 PATH \"/0:Severity\" ATTRIBUTE 15 INDEX [1,9]\n"
                "\n"
                "$6:= (TYPEID ns=3;s=\"15 23\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]) LESSTHAN $19";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_12) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "OR($1, $2)\n"
                "\n"
                "FOR\n"
                "\n"
                "\n"
                "$2:= ( TYPEID ns=3;s=\"15 23\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]) & $19\n"
                "\n"
                "$\"another\":= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} | TYPEID ns=1234;g=09087e75-8e5e-499b-954f-f2a9603db28a PATH \"/1:AnotherAnotherTest\" ATTRIBUTE 23\n"
                "\n"
                "$19:= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} LESSOREQUAL $\"another\"\n"
                "\n"
                "$1:= AND($\"another\", $19)";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_13) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "OR($1, $2)\n"
                "\n"
                "FOR\n"
                "\n"
                "$2:= OR($19, {\"Type\": 6,\"Body\": 27} <=> $\"another\")\n"
                "\n"
                "$\"another\":= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} -> TYPEID ns=1234;g=09087e75-8e5e-499b-954f-f2a9603db28a PATH \"/1:AnotherAnotherTest\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "$19:= OFTYPE ns=10;s=\"Hel&,lo:Wor ld\"\n"
                "\n"
                "$1:= AND($\"another\", $19)";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_14) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "OR($1, $2)\n"
                "\n"
                "FOR\n"
                "\n"
                "\n"
                "$2:= $\"another\" LE $19\n"
                "\n"
                "$\"another\":= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} <= TYPEID ns=1234;g=09087e75-8e5e-499b-954f-f2a9603db28a PATH \"/1:AnotherAnotherTest\" ATTRIBUTE 23\n"
                "\n"
                "$19:= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} BETWEEN [FLOAT 17, TYPEID ns=12;i=12345 PATH \"/1:AnotherAnotherTest\"]\n"
                "\n"
                "$163:= TYPEID ns=1234;g=09087e75-8e5e-499b-954f-f2a9603db28a PATH \"/1:AnotherAnotherTest\" ATTRIBUTE 23\n"
                "\n"
                "$\"literal float\":= FLOAT 17\n"
                "\n"
                "$\"literal json\":= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}\n"
                "\n"
                "$\"element oper\":= $\"element operand\"\n"
                "\n"
                "$\"element operand\":= $163\n"
                "\n"
                "$\"element op\":= $\"element operand\"\n"
                "\n"
                "\n"
                "$1:= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} INLIST [\n"
                "                                                                {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]},\n"
                "                                                                TYPEID ns=1;b=\"b3BlbjYyNTQxIQ==\" PATH \"/1:AnotherAnotherTest\",\n"
                "                                                                ({\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]}),\n"
                "                                                                STRING \"tes tSt 3 ring\",\n"
                "                                                                {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]},\n"
                "                                                                $\"element op\",\n"
                "                                                                $\"element operand\",\n"
                "                                                                EXPNODEID \"svr=5;nsu=https://test.test.com;s=test\"\n"
                "                                                             ]";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_15) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "\n"
                "AND($19, OR( FLOAT 17 LESSOREQUAL TYPEID b=\"b3BlbjYyNTQxIQ==\" PATH \"/7:OnlyPathSpecified\" ATTRIBUTE 3 INDEX [1,4:7,90], ISNULL TYPEID ns=2;i=2000 PATH \"/0:Severity\" ATTRIBUTE 15 INDEX [1,9]))\n"
                "\n"
                "FOR\n"
                "\n"
                "$19:= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} GE TYPEID ns=1234;g=09087e75-8e5e-499b-954f-f2a9603db28a PATH \"/1:AnotherAnotherTest\" ATTRIBUTE 23\n";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_16) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "OR($1, $19)\n"
                "\n"
                "FOR\n"
                "\n"
                "\n"
                "$\"another\":= {\"Type\": 3,\"Body\": [1,2,1,5],\"Dimension\": [2,2]} >= TYPEID ns=1234;g=09087e75-8e5e-499b-954f-f2a9603db28a PATH \"/1:AnotherAnotherTest\" ATTRIBUTE 23\n"
                "\n"
                "$19:= OR($\"another\", $\"another\")\n"
                "\n"
                "$1:= AND($\"another\", $19)";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
    UA_ByteString_clear(&case_);
} END_TEST

START_TEST(Case_17) {
    char *inp = "SELECT\n"
                "\n"
                "\n"
                "TYPEID ns=134;s=\"1;;5 2;3\" PATH \"/1:Duration/15:SecondElement/7:ThirdElement\" ATTRIBUTE 23 INDEX [100,5:20]\n"
                "\n"
                "WHERE\n"
                "\n"
                "OR(OR($10, AND($8, OR($9, $19))),AND(OR( $6 ,EXPNODEID \"svr=5;nsu=https://test.test.com;s=test\"), AND($11, $7)))\n"
                "\n"
                "FOR\n"
                "\n"
                "\n"
                "$\"another\":= {\n"
                "                \"Type\": 3,\n"
                "                \"Body\": [1,2,1,5],\n"
                "                \"Dimension\": [2,2]\n"
                "             }\n"
                "             GREATERTHAN \"tes tSt 3 ring\"\n"
                "\n"
                "$19:= OFTYPE TYPEID ns=1;i=5001 PATH \".\" ATTRIBUTE 1\n"
                "\n"
                "$6:= GUID 09087e75-8e5e-499b-954f-f2a9603db28a LESSTHAN UINT16 30\n"
                "\n"
                "$7:= false | ns=2;i=2000\n"
                "\n"
                "$8:= SBYTE -125 <= TIME \"2023-06-06T09:55:50.730824Z\"\n"
                "\n"
                "$9:= EXPNODEID \"svr=5;nsu=ht.test.com;s=test\" >= STATUSCODE 15\n"
                "\n"
                "$11:= LOCALIZED \"Duration\" <=> 100.33\n"
                "\n"
                "$10:= QNAME \"<0:HasProperty>1:Durat ion\" == \"/37:Duration\"";

    UA_ByteString case_ = UA_String_fromChars(inp);
    UA_EventFilter *empty_filter = UA_EventFilter_new();
    UA_EventFilter_parse(&filter, &case_);
    ck_assert_ptr_ne(&filter, empty_filter);
    UA_EventFilter_delete(empty_filter);
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
    tcase_add_test(tc_call, Case_15);
    tcase_add_test(tc_call, Case_16);
    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_VERBOSE);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
