/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>
#include "tests/namespace_tests_di_generated.h"

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "thread_wrapper.h"

static UA_Server *server;
static UA_Client *client;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void
setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    // Add DI datatypes to server
    const UA_StatusCode resDi = namespace_tests_di_generated(server);
    ck_assert_uint_eq(resDi, UA_STATUSCODE_GOOD);
    size_t diIndex = 0;
    const UA_StatusCode resIdx = UA_Server_getNamespaceByName(server,
        UA_STRING("http://opcfoundation.org/UA/DI/"), &diIndex);
    ck_assert_uint_eq(resIdx, UA_STATUSCODE_GOOD);
    // Verify that n=resIdx;i=15889 (TransferResultDataDataType) exists on server
    UA_NodeId dataTypeId = UA_NODEID_NUMERIC(diIndex, 15889);
    const UA_DataType* dt = UA_Server_findDataType(server, &dataTypeId);
    ck_assert(NULL != dt);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
    client = UA_Client_newForUnitTest();
    ck_assert(NULL != client);
    const UA_StatusCode resConnect = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(resConnect, UA_STATUSCODE_GOOD);
}

static void
teardown(void) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(GetRemoteDatatypes_di) {
    // Fetch all remote datatypes
    UA_DataTypeArray* array = NULL;
    const UA_StatusCode resGet = UA_Client_getRemoteDataTypes(client, 0, NULL, &array);
    ck_assert_uint_eq(resGet, UA_STATUSCODE_GOOD);
    ck_assert(NULL != array);
    ck_assert(NULL != UA_Client_getConfig(client));
    UA_Client_getConfig(client)->customDataTypes = array;

    UA_UInt16 diIdx = 0;
    const UA_StatusCode resIdx = UA_Client_getNamespaceIndex(client,
        UA_STRING("http://opcfoundation.org/UA/DI/"), &diIdx);
    ck_assert_uint_eq(resIdx, UA_STATUSCODE_GOOD);

    // Client should now have ns=idx,
    const UA_NodeId dataTypeId = UA_NODEID_NUMERIC(diIdx, 15889);
    const UA_DataType* dt = UA_Client_findDataType(client, &dataTypeId);
    ck_assert(NULL != dt);
}
END_TEST

static Suite*
testSuite_GetRemoteDatatypes(void) {
    Suite *s = suite_create("GetRemoteDatatypes");
    TCase *tc = tcase_create("Basic");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, GetRemoteDatatypes_di);
    suite_add_tcase(s, tc);
    return s;
}

int
main(void) {
    Suite *s = testSuite_GetRemoteDatatypes();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}