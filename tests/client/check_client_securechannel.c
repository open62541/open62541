/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <pthread.h>

#include "ua_types.h"
#include "ua_server.h"
#include "ua_client.h"
#include "ua_client_internal.h"
#include "ua_config_default.h"
#include "ua_client_highlevel.h"
#include "ua_network_tcp.h"
#include "testing_clock.h"
#include "check.h"

UA_Server *server;
UA_ServerConfig *config;
UA_Boolean *running;
UA_ServerNetworkLayer nl;
pthread_t server_thread;

static void
addVariable(size_t size) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32* array = (UA_Int32*)UA_malloc(size * sizeof(UA_Int32));
    memset(array, 0, size * sizeof(UA_Int32));
    UA_Variant_setArray(&attr.value, array, size, &UA_TYPES[UA_TYPES_INT32]);

    char name[] = "my.variable";
    attr.description = UA_LOCALIZEDTEXT("en-US", name);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, name);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);

    UA_free(array);
}

static void * serverloop(void *_) {
    while(*running)
        UA_Server_run_iterate(server, true);
    return NULL;
}

static void setup(void) {
    running = UA_Boolean_new();
    *running = true;
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    addVariable(16366);
    pthread_create(&server_thread, NULL, serverloop, NULL);
}

static void teardown(void) {
    *running = false;
    pthread_join(server_thread, NULL);
    UA_Server_run_shutdown(server);
    UA_Boolean_delete(running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

START_TEST(SecureChannel_timeout_max) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_sleep(UA_ClientConfig_default.secureChannelLifeTime);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant_deleteMembers(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(SecureChannel_timeout_fail) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_sleep(UA_ClientConfig_default.secureChannelLifeTime+1);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert(retval != UA_STATUSCODE_GOOD);

    UA_Variant_deleteMembers(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(SecureChannel_reconnect) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    client->state = UA_CLIENTSTATE_CONNECTED;

    retval = UA_Client_disconnect(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_delete(client);
}
END_TEST

int main(void) {
    TCase *tc_sc = tcase_create("Client SecureChannel");
    tcase_add_checked_fixture(tc_sc, setup, teardown);
    tcase_add_test(tc_sc, SecureChannel_timeout_max);
    tcase_add_test(tc_sc, SecureChannel_timeout_fail);
    tcase_add_test(tc_sc, SecureChannel_reconnect);

    Suite *s = suite_create("Client");
    suite_add_tcase(s, tc_sc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
