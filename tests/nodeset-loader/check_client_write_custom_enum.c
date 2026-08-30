/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/nodesetloader.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/util.h>

#include "open62541/client_highlevel.h"

#include <check.h>
#include <stdlib.h>
#include <unistd.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

static UA_Server *server = NULL;
static UA_Client *client = NULL;
static UA_Boolean running = false;
static THREAD_HANDLE server_thread;

#define DEVICE_HEALTH_NODE_ID 70001

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

    const UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
    const UA_UInt16 nsIndex = UA_Server_addNamespace(server,
        "http://opcfoundation.org/UA/DI/");
    ck_assert_uint_ne(0, nsIndex);

    const UA_NodeId dti = UA_NODEID_NUMERIC(nsIndex, 6244);
    const UA_DataType* deviceHealthEnumerationType = UA_Server_findDataType(server, &dti);
    ck_assert(NULL != deviceHealthEnumerationType);

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 health = 2;
    UA_Variant_setScalar(&attr.value, &health, deviceHealthEnumerationType);
    attr.dataType = dti;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "DeviceHealth");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    const UA_StatusCode addVarStatus = UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(nsIndex, DEVICE_HEALTH_NODE_ID),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(nsIndex, "DeviceHealth"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, addVarStatus);

    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Server_run_startup(server));
    THREAD_CREATE(server_thread, serverloop);

    client = UA_Client_newForUnitTest();
    ck_assert(client != NULL);

    const UA_StatusCode resConnect = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(resConnect, UA_STATUSCODE_GOOD);

    UA_DataTypeArray* array = NULL;
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Client_getRemoteDataTypes(client, 0, NULL,
        &array));
    ck_assert(NULL != array);
    ck_assert(NULL != UA_Client_getConfig(client));
    UA_Client_getConfig(client)->customDataTypes = array;
}

static void
teardown(void) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    client = NULL;

    running = false;
    THREAD_JOIN(server_thread);

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

START_TEST(Client_writeCustomEnum_emptyScaffold) {
    UA_UInt16 nsIdx = 0;
    ck_assert_uint_eq(UA_STATUSCODE_GOOD, UA_Client_addNamespace(client,
        UA_STRING("http://opcfoundation.org/UA/DI/"), &nsIdx));
    ck_assert_uint_ne(0, nsIdx);

    const UA_NodeId dti = UA_NODEID_NUMERIC(nsIdx, 6244);
    const UA_DataType* deviceHealthEnumerationType = UA_Client_findDataType(client, &dti);
    ck_assert(NULL != deviceHealthEnumerationType);


    UA_Int32 newValue = 3;
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &newValue, deviceHealthEnumerationType);

    const UA_StatusCode writeStatus = UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(nsIdx,DEVICE_HEALTH_NODE_ID), &value);
    ck_assert_uint_eq(writeStatus, UA_STATUSCODE_GOOD);
}
END_TEST

static Suite *
testSuite_Client(void) {
    Suite *s = suite_create("Client Write Custom Datatypes");
    TCase *tc_client = tcase_create("DeviceHealthEnumeration");
    tcase_add_unchecked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_writeCustomEnum_emptyScaffold);
    suite_add_tcase(s, tc_client);
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
