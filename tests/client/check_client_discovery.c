/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client_config_default.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"

#include <check.h>
#include <stdlib.h>

#include "thread_wrapper.h"
#include "test_helpers.h"

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
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Client_connect_badEndpointUrl) {
    UA_Client *client = UA_Client_newForUnitTest();

    /* Use the internal API to force a bad DiscoveryUrl */
    UA_String_clear(&client->config.endpointUrl);
    UA_String_clear(&client->discoveryUrl);
    client->config.endpointUrl = UA_STRING_ALLOC("opc.tcp://localhost:4840");
    client->discoveryUrl = UA_STRING_ALLOC("abc://xxx:4840");

    /* Open a Session when possible */
    client->config.noSession = false;

    lockClient(client);
    connectSync(client);
    unlockClient(client);
    ck_assert_uint_eq(client->connectStatus, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_getEndpoints) {
    UA_Client *client = UA_Client_newForUnitTest();

    size_t endpointCount = 0;
    UA_EndpointDescription *endpoints = NULL;
    UA_StatusCode retval = UA_Client_getEndpoints(client,
                                "opc.tcp://localhost:4840",
                                &endpointCount, &endpoints);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(endpointCount > 0);
    ck_assert_ptr_ne(endpoints, NULL);

    /* Verify each endpoint has a non-empty securityPolicyUri */
    for(size_t i = 0; i < endpointCount; i++) {
        ck_assert(endpoints[i].securityPolicyUri.length > 0);
    }

    UA_Array_delete(endpoints, endpointCount,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_findServers) {
    UA_Client *client = UA_Client_newForUnitTest();

    size_t serverCount = 0;
    UA_ApplicationDescription *servers = NULL;
    UA_StatusCode retval = UA_Client_findServers(client,
                                "opc.tcp://localhost:4840",
                                0, NULL, 0, NULL,
                                &serverCount, &servers);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(serverCount > 0);
    ck_assert_ptr_ne(servers, NULL);

    UA_Array_delete(servers, serverCount,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_getEndpoints_connected) {
    /* Connect first, then getEndpoints on same URL — should succeed */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t endpointCount = 0;
    UA_EndpointDescription *endpoints = NULL;
    retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
                                    &endpointCount, &endpoints);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(endpointCount > 0);

    UA_Array_delete(endpoints, endpointCount,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_findServers_connected) {
    /* Connect first, then findServers on same URL — should succeed */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t serverCount = 0;
    UA_ApplicationDescription *servers = NULL;
    retval = UA_Client_findServers(client, "opc.tcp://localhost:4840",
                                   0, NULL, 0, NULL,
                                   &serverCount, &servers);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(serverCount > 0);

    UA_Array_delete(servers, serverCount,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Discovery");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_connect_badEndpointUrl);
    tcase_add_test(tc_client, Client_getEndpoints);
    tcase_add_test(tc_client, Client_findServers);
    tcase_add_test(tc_client, Client_getEndpoints_connected);
    tcase_add_test(tc_client, Client_findServers_connected);
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
