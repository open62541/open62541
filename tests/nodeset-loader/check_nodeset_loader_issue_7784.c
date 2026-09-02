/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodesetloader.h>

#include <check.h>
#include <stdlib.h>

#include "testing_clock.h"
#include "test_helpers.h"

UA_Server *server = NULL;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void
teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/*
 * Repro for issue 7784:
 * TypeA has a member of custom TypeB, but TypeA appears before TypeB in the
 * nodeset XML.
 */
START_TEST(Server_7784_wrong_order_simple) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_LOADER_TEST_DIR "issue_7784_minimal.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));

    size_t nsIndex = 0;
    retVal = UA_Server_getNamespaceByName(server,
        UA_STRING("http://open62541.org/test/issue7784/"), &nsIndex);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);

    /* TypeB: simple struct - must be found (ns=<nsIndex>;i=3002) */
    UA_NodeId typeBId = UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 3002);
    const UA_DataType *typeB = UA_Server_findDataType(server, &typeBId);
    ck_assert_msg(typeB != NULL, "TypeB (ns=X;i=3002) should be found");

    /* TypeA: struct with TypeB member, defined before TypeB in XML (ns=<nsIndex>;i=3001) */
    UA_NodeId typeAId = UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 3001);
    const UA_DataType *typeA = UA_Server_findDataType(server, &typeAId);
    ck_assert_msg(typeA != NULL, "TypeA (ns=X;i=3001) should be found");
}
END_TEST

START_TEST(Server_7784_wrong_order_complex) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));

    retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "AutoID/Opc.Ua.AutoID.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));

    size_t nsIndex = 0;
    retVal = UA_Server_getNamespaceByName(server,
        UA_STRING("http://opcfoundation.org/UA/AutoID/"), &nsIndex);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);

    UA_NodeId dataTypeId = UA_NODEID_NUMERIC(nsIndex, 3020);
    const UA_DataType *dt1 = UA_Server_findDataType(server, &dataTypeId);
    ck_assert_msg(dt1 != NULL,
                  "Expected AutoID datatype ns=X;i=3020 to be present");

    dataTypeId = UA_NODEID_NUMERIC(nsIndex, 3024);
    const UA_DataType *dt = UA_Server_findDataType(server, &dataTypeId);
    ck_assert_msg(dt != NULL,
                  "Expected AutoID datatype ns=X;i=3024 to be present");
}
END_TEST

static Suite *
testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Loader");
    TCase *tc_server = tcase_create("Issue 7784");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_7784_wrong_order_simple);
    tcase_add_test(tc_server, Server_7784_wrong_order_complex);
    suite_add_tcase(s, tc_server);
    return s;
}

int
main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
