/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "open62541/common.h"

#include "client/ua_client_internal.h"

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean connected = false;

static void
currentState(UA_Client *client, UA_SecureChannelState channelState,
             UA_SessionState sessionState, UA_StatusCode recoveryStatus) {
    if(sessionState == UA_SESSIONSTATE_ACTIVATED)
        connected = true;
    else
        connected = false;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
asyncBrowseCallback(UA_Client *Client, void *userdata,
                  UA_UInt32 requestId, UA_BrowseResponse *response) {
    UA_UInt16 *asyncCounter = (UA_UInt16*)userdata;
    (*asyncCounter)++;
}

START_TEST(Client_connect_async) {
    UA_StatusCode retval;
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = currentState;
    connected = false;
    UA_Client_connectAsync(client, "opc.tcp://localhost:4840");
    UA_Server_run_iterate(server, false);

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

    do {
        if(connected) {
            /* If not connected requests are not sent */
            UA_Client_sendAsyncBrowseRequest(client, &bReq, asyncBrowseCallback,
                                             &asyncCounter, &reqId);
        }
        /* Give network a chance to process packet */
        UA_realSleep(100);
        /* Manual clock for unit tests */
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        /*fix infinite loop, but why is server occasionally shut down in Appveyor?!*/
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
            break;
    } while(reqId < 10);

    UA_BrowseRequest_clear(&bReq);
    ck_assert_uint_eq(connected, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* With default setting the client uses 7 iterations to connect */
    ck_assert_uint_eq(asyncCounter, 5);
    UA_Client_disconnectAsync(client);
    while(connected) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    UA_Client_delete (client);
}
END_TEST

UA_SecureChannelState abortState;
static void
abortSecureChannelConnect(UA_Client *client, UA_SecureChannelState channelState,
                          UA_SessionState sessionState, UA_StatusCode recoveryStatus) {
    if(channelState >= abortState)
        UA_Client_disconnectAsync(client);
}

/* Abort the connection by calling disconnect */
START_TEST(Client_connect_async_abort) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = abortSecureChannelConnect;

    for(int i = 0; i < UA_SECURECHANNELSTATE_CLOSING; i++) {
        abortState = (UA_SecureChannelState)i;
        UA_Client_connectAsync(client, "opc.tcp://localhost:4840");

        UA_SecureChannelState currentState;
        do {
            UA_Server_run_iterate(server, false);
            UA_Client_run_iterate(client, 5);
            UA_Client_getState(client, &currentState, NULL, NULL);
        } while(currentState != UA_SECURECHANNELSTATE_CLOSED);
    }

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_no_connection) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = currentState;
    connected = false;
    UA_StatusCode retval = UA_Client_connectAsync(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Wait for the initial socket */
    //UA_EventLoop *el = client->config.eventLoop;
    while(client->channel.state != UA_SECURECHANNELSTATE_OPEN) {
        //el->run(el, 0);
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 1);
    }

    /* Manually close the TCP connection */
    UA_ConnectionManager *cm = client->channel.connectionManager;
    cm->closeConnection(cm, client->channel.connectionId);

    do {
        UA_Server_run_iterate(server, false);
        retval |= UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    } while(client->channel.state != UA_SECURECHANNELSTATE_OPEN);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_without_run_iterate) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = currentState;
    connected = false;
    UA_Client_connectAsync(client, "opc.tcp://localhost:4840");
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_run_iterate) {
    UA_StatusCode retval;
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = currentState;
    connected = false;
    retval = UA_Client_connectAsync(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    while (!connected) {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_realSleep(100);
    }

    UA_Client_disconnectAsync(client);
    while(client->channel.state != UA_SECURECHANNELSTATE_CLOSED) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client_connect = tcase_create("Client Connect Async");
    tcase_add_checked_fixture(tc_client_connect, setup, teardown);
    tcase_add_test(tc_client_connect, Client_connect_async);
    tcase_add_test(tc_client_connect, Client_connect_async_abort);
    tcase_add_test(tc_client_connect, Client_no_connection);
    tcase_add_test(tc_client_connect, Client_without_run_iterate);
    tcase_add_test(tc_client_connect, Client_run_iterate);
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
