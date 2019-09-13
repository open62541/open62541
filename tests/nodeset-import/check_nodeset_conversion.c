/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "conversion.h"

#include "check.h"

START_TEST(NullNodeId) {
 UA_NodeId id = extractNodeId(NULL);
 ck_assert(UA_NodeId_equal(&UA_NODEID_NULL, &id));
}
END_TEST

START_TEST(ExtractNumericIds) {
 UA_NodeId id = extractNodeId("i=10");
 UA_NodeId expectedId = UA_NODEID_NUMERIC(0,10);
 ck_assert(UA_NodeId_equal(&expectedId, &id));
 id = extractNodeId("ns=1456;i=10");
 expectedId = UA_NODEID_NUMERIC(1456,10);
 ck_assert(UA_NodeId_equal(&expectedId, &id));
 id = extractNodeId("abc");
 ck_assert(UA_NodeId_equal(&UA_NODEID_NULL, &id));
}
END_TEST

START_TEST(ExtractStringIds) {
 UA_NodeId id = extractNodeId("s=StringIdentifier");
 UA_NodeId expectedId = UA_NODEID_STRING(0, "StringIdentifier");
 ck_assert(UA_NodeId_equal(&expectedId, &id));
 id = extractNodeId("ns=1456;s=StringIdentifier");
 expectedId = UA_NODEID_STRING(1456,"StringIdentifier");
 ck_assert(UA_NodeId_equal(&expectedId, &id));
 id = extractNodeId("abc");
 ck_assert(UA_NodeId_equal(&UA_NODEID_NULL, &id));
}
END_TEST



static Suite *testSuite_Client(void) {
    Suite *s = suite_create("server nodeset conversion");
    TCase *tc_server = tcase_create("server nodeset conversion");
    tcase_add_test(tc_server, NullNodeId);
    tcase_add_test(tc_server, ExtractNumericIds);
    tcase_add_test(tc_server, ExtractStringIds);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(int argc, char*argv[]) {
    printf("%s", argv[0]);
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
