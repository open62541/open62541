/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "ua_types.h"
#include "ua_server.h"
#include "ua_client.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#include "check.h"
#include "testing_clock.h"

UA_Server *server;
UA_ServerConfig *config;
UA_Boolean running;
UA_ServerNetworkLayer nl;
pthread_t server_thread;

static void * serverloop(void *_) {
    while(running)
        UA_Server_run_iterate(server, true);
    return NULL;
}

static void setup(void) {
    running = true;
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    pthread_create(&server_thread, NULL, serverloop, NULL);
}

static void teardown(void) {
    running = false;
    pthread_join(server_thread, NULL);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

static void
asyncReadCallback(UA_Client *client, void *userdata,
                  UA_UInt32 requestId, const UA_ReadResponse *response) {
    UA_UInt16 *asyncCounter = (UA_UInt16*)userdata;
    (*asyncCounter)++;
    UA_sleep(1000);
}

START_TEST(Client_read_async) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt16 asyncCounter = 0;

    UA_ReadRequest rr;
    UA_ReadRequest_init(&rr);

    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;
    rvid.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);

    rr.nodesToRead = &rvid;
    rr.nodesToReadSize = 1;

    /* 1st request */
    retval = __UA_Client_AsyncService(client, &rr, &UA_TYPES[UA_TYPES_READREQUEST],
                                      (UA_ClientAsyncServiceCallback)asyncReadCallback,
                                      &UA_TYPES[UA_TYPES_READRESPONSE], &asyncCounter, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* 2nd request */
    retval = __UA_Client_AsyncService(client, &rr, &UA_TYPES[UA_TYPES_READREQUEST],
                                      (UA_ClientAsyncServiceCallback)asyncReadCallback,
                                      &UA_TYPES[UA_TYPES_READRESPONSE], &asyncCounter, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Process async responses during 100ms */
    retval = UA_Client_runAsync(client, 1000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(asyncCounter, 2);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_read_async);
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
