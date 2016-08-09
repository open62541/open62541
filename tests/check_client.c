#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "ua_types.h"
#include "ua_server.h"
#include "ua_client.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#include "check.h"

UA_Server *server;
UA_Boolean *running;
UA_ServerNetworkLayer nl;
pthread_t server_thread;

static void * serverloop(void *_) {
    while(*running)
        UA_Server_run_iterate(server, true);
    return NULL;
}

static void setup(void) {
    running = UA_Boolean_new();
    *running = true;
    UA_ServerConfig config = UA_ServerConfig_standard;
    nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    pthread_create(&server_thread, NULL, serverloop, NULL);
}

static void teardown(void) {
    *running = false;
    pthread_join(server_thread, NULL);
    UA_Server_run_shutdown(server);
    UA_Boolean_delete(running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
}

START_TEST(Client_connect) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:16664");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_connect);
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
