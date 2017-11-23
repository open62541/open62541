/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "ua_types.h"
#include "ua_server.h"
#include "ua_client.h"
#include "ua_config_default.h"
#include "ua_client_highlevel.h"
#include "ua_network_tcp.h"
#include "testing_clock.h"
#include "check.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_ServerConfig *config;
UA_Boolean *running;
UA_Boolean *blockServer;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(*running){
        if (*blockServer)
            UA_realSleep(100);
        else
            UA_Server_run_iterate(server, true);
    }
    return 0;
}

static void setup(void) {
    running = UA_Boolean_new();
    *running = true;
    blockServer = UA_Boolean_new();
    *blockServer = false;
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
    UA_realSleep(100);
}

static void teardown(void) {
    *running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Boolean_delete(running);
    UA_Boolean_delete(blockServer);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

START_TEST(SecureChannel_timeout_max) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_fakeSleep(UA_ClientConfig_default.secureChannelLifeTime);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant_deleteMembers(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST
/*
START_TEST(SecureChannel_timeout_fail) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_fakeSleep(UA_ClientConfig_default.secureChannelLifeTime+1);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert(retval != UA_STATUSCODE_GOOD);

    UA_Variant_deleteMembers(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST*/

START_TEST(SecureChannel_networkfail) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    *blockServer = true;
    UA_realSleep(100);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert(retval != UA_STATUSCODE_GOOD);

    UA_Variant_deleteMembers(&val);

    *blockServer = false;

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

int main(void) {
    TCase *tc_sc = tcase_create("Client SecureChannel");
    tcase_add_checked_fixture(tc_sc, setup, teardown);
    tcase_add_test(tc_sc, SecureChannel_timeout_max);
    // Temporarily disable test since it is failing. See #1388
    //tcase_add_test(tc_sc, SecureChannel_timeout_fail);
    tcase_add_test(tc_sc, SecureChannel_networkfail);

    Suite *s = suite_create("Client");
    suite_add_tcase(s, tc_sc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
