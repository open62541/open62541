/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *
 */

#include <../../plugins/include/open62541/client_config_default.h>

#include <check.h>
#include <limits.h>

#include "tests/namespace_tests_di_generated.h"
#include "tests/namespace_tests_plc_generated.h"

#include "test_helpers.h"
#include "thread_wrapper.h"
#include "client/ua_client_internal.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    size_t idx = LONG_MAX;
    UA_StatusCode retval = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/DI/"), &idx);
    if(retval != UA_STATUSCODE_GOOD) {
        retval = namespace_tests_di_generated(server);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
    retval = UA_Server_getNamespaceByName(server, UA_STRING("http://PLCopen.org/OpcUa/IEC61131-3/"), &idx);
    if(retval != UA_STATUSCODE_GOOD) {
        retval = namespace_tests_plc_generated(server);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Client_nsMapping){
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t max_stop_iteration_count = 100000;
    size_t iteration = 0;
    while(!client->haveNamespaces && iteration < max_stop_iteration_count) {
        UA_Client_run_iterate(client, 0);
        iteration++;
    }

    UA_UInt16 idx = UA_NamespaceMapping_local2Remote(client->channel.namespaceMapping, 2);
    ck_assert_uint_eq(idx, 2);

    idx = UA_NamespaceMapping_local2Remote(client->channel.namespaceMapping, 3);
    ck_assert_uint_eq(idx, 3);

    idx = UA_NamespaceMapping_remote2Local(client->channel.namespaceMapping, 2);
    ck_assert_uint_eq(idx, 2);

    idx = UA_NamespaceMapping_remote2Local(client->channel.namespaceMapping, 3);
    ck_assert_uint_eq(idx, 3);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Namespace Mapping");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_nsMapping);
    suite_add_tcase(s,tc_client);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
