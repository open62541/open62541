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
#include "ua_client_highlevel_async.h"
#include "ua_config_default.h"
#include "ua_network_tcp.h"
#include "check.h"
#include "testing_clock.h"

#include "thread_wrapper.h"

UA_Server *server;
UA_ServerConfig *config;
UA_Boolean running;
UA_ServerNetworkLayer nl;
THREAD_HANDLE server_thread;

UA_Client *client;
UA_StatusCode retval;
UA_Boolean connected;
THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void
onConnect (UA_Client *Client, void *Connected, UA_UInt32 requestId,
           void *status) {
    if (UA_Client_getState (Client) == UA_CLIENTSTATE_SESSION)
        *(UA_Boolean *) Connected = true;
    UA_fakeSleep(10);
}

static void setup(void) {
    running = true;
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);

    UA_fakeSleep(500);
}

static void teardown(void) {
    //UA_fakeSleep(5000);
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
    UA_fakeSleep(10);
}

START_TEST(Client_connect_async){

    client = UA_Client_new (UA_ClientConfig_default);
    connected = false;
    UA_Client_connect_async (client, "opc.tcp://localhost:4840", onConnect,
                                     &connected);

    UA_UInt32 reqId = 0;
    UA_UInt16 asyncCounter = 0;

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init (&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new ();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC (0,
    UA_NS0ID_OBJECTSFOLDER); /* browse objects folder */
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */

    UA_DateTime startTime = UA_DateTime_nowMonotonic();
    /*connected gets updated when client is connected*/
    do {
        if (connected) {
            /*if not connected requests are not sent*/
            UA_Client_sendAsyncBrowseRequest (client, &bReq, asyncBrowseCallback,
                                              &asyncCounter, &reqId);
        }
        /*manual clock for unit tests*/
        UA_realSleep(20);
        if (UA_DateTime_nowMonotonic() - startTime > 2000 * UA_DATETIME_MSEC){
            break; /*sometimes test can stuck*/
        }

        /* depending on the number of requests sent UA_Client_run_iterate can be called multiple times
         * here 2 calls are needed so that even when run with valgrind (very slow) the result is still correct*/
        UA_Client_run_iterate(client, 0);
        retval = UA_Client_run_iterate(client, 0);
    }
    while (reqId < 10);

    UA_BrowseRequest_deleteMembers(&bReq);
    /*with default setting the client uses 4 requests to connect*/
    if (connected)
        ck_assert_uint_eq(asyncCounter, 10-4);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete (client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client_connect = tcase_create("Client Connect");
    tcase_add_checked_fixture(tc_client_connect, setup, teardown);
    tcase_add_test(tc_client_connect, Client_connect_async);
    suite_add_tcase(s,tc_client_connect);

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
