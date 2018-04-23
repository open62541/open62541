/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "ua_types.h"
#include "ua_server.h"
#include "ua_client.h"
#include "client/ua_client_internal.h"
#include "ua_client_highlevel_async.h"
#include "ua_config_default.h"
#include "ua_network_tcp.h"
#include "check.h"
#include "testing_clock.h"
#include "testing_networklayers.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_ServerConfig *config;
UA_Boolean running;
UA_ServerNetworkLayer nl;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void
onConnect (UA_Client *Client, void *connected, UA_UInt32 requestId,
           void *response) {
    if (UA_Client_getState (Client) == UA_CLIENTSTATE_SESSION)
        *(UA_Boolean *)connected = true;
}

static void setup(void) {
    running = true;
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
    /* Waiting server is up */
    UA_comboSleep(1000);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

static void
asyncBrowseCallback(UA_Client *Client, void *userdata,
                  UA_UInt32 requestId, UA_BrowseResponse *response) {
    UA_UInt16 *asyncCounter = (UA_UInt16*)userdata;
    (*asyncCounter)++;
}

START_TEST(Client_connect_async){
    UA_StatusCode retval;
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_Boolean connected = false;
    UA_Client_connect_async(client, "opc.tcp://localhost:4840", onConnect,
                            &connected);
    /*Windows needs time to response*/
    UA_sleep_ms(100);
    UA_UInt32 reqId = 0;
    UA_UInt16 asyncCounter = 0;
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init (&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new ();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */
    /* Connected gets updated when client is connected */

    do{
        if(connected) {
            /* If not connected requests are not sent */
            UA_Client_sendAsyncBrowseRequest (client, &bReq, asyncBrowseCallback,
                                              &asyncCounter, &reqId);
        }
        /* Manual clock for unit tests */
        UA_comboSleep(20);
        retval = UA_Client_run_iterate(client, 0);
        /*fix infinite loop, but why is server occasionally shut down in Appveyor?!*/
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
            break;
    } while(reqId < 10);

    UA_BrowseRequest_deleteMembers(&bReq);
    ck_assert_uint_eq(connected, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* With default setting the client uses 4 requests to connect */
    ck_assert_uint_eq(asyncCounter, 10-4);
    UA_Client_disconnect(client);
    UA_Client_delete (client);
}
END_TEST

START_TEST(Client_no_connection) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);

    UA_Boolean connected = false;
    UA_StatusCode retval = UA_Client_connect_async(client, "opc.tcp://localhost:4840", onConnect,
            &connected);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_recv = client->connection.recv;
    client->connection.recv = UA_Client_recvTesting;
    //simulating unconnected server
    UA_Client_recvTesting_result = UA_STATUSCODE_BADCONNECTIONCLOSED;
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONNECTIONCLOSED);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_without_run_iterate) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    UA_Boolean connected = false;
    UA_Client_connect_async(client, "opc.tcp://localhost:4840", onConnect,
                            &connected);
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client_connect = tcase_create("Client Connect Async");
    tcase_add_checked_fixture(tc_client_connect, setup, teardown);
    tcase_add_test(tc_client_connect, Client_connect_async);
    tcase_add_test(tc_client_connect, Client_no_connection);
    tcase_add_test(tc_client_connect, Client_without_run_iterate);
    suite_add_tcase(s,tc_client_connect);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
