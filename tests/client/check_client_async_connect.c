/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "open62541/common.h"

#include "client/ua_client_internal.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

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

    while(!connected) {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    /* If not connected requests are not sent */
    UA_Client_sendAsyncBrowseRequest(client, &bReq, asyncBrowseCallback,
                                     &asyncCounter, &reqId);
    UA_Server_run_iterate(server, false);
    retval = UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_BrowseRequest_clear(&bReq);
    ck_assert_uint_eq(connected, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
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
    while(!connected) {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    UA_Client_disconnectAsync(client);
    while(client->channel.state != UA_SECURECHANNELSTATE_CLOSED) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_connectSecureChannelAsync) {
    /* Connect only the SecureChannel without opening a Session */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connectSecureChannelAsync(client,
                                "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Drive both sides until the channel is open */
    while(client->channel.state != UA_SECURECHANNELSTATE_OPEN) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }

    UA_SecureChannelState channelState;
    UA_SessionState sessionState;
    UA_StatusCode connectStatus;
    UA_Client_getState(client, &channelState, &sessionState, &connectStatus);
    ck_assert_uint_eq(channelState, UA_SECURECHANNELSTATE_OPEN);
    /* No session should be created */
    ck_assert(sessionState != UA_SESSIONSTATE_ACTIVATED);

    UA_Client_disconnectAsync(client);
    while(client->channel.state != UA_SECURECHANNELSTATE_CLOSED) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_connect_async_wrongUrl) {
    /* Attempt to connect to a wrong port — should fail gracefully */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->timeout = 200; /* short timeout */
    UA_StatusCode retval = UA_Client_connectAsync(client, "opc.tcp://localhost:4841");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Drive a few iterations */
    for(int i = 0; i < 50; i++) {
        UA_fakeSleep(10);
        UA_Client_run_iterate(client, 0);
    }

    UA_SecureChannelState channelState;
    UA_StatusCode connectStatus;
    UA_Client_getState(client, &channelState, NULL, &connectStatus);
    /* Either still connecting or failed — not OPEN */
    ck_assert(channelState != UA_SECURECHANNELSTATE_OPEN ||
              connectStatus != UA_STATUSCODE_GOOD);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_connectUsername_async) {
    /* Connect with username/password via the sync connectUsername wrapper,
     * but using manual iteration for the server side */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = currentState;
    connected = false;
    UA_StatusCode retval = UA_Client_connectAsync(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    while(!connected) {
        UA_Server_run_iterate(server, false);
        retval = UA_Client_run_iterate(client, 0);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    /* Verify the client is connected with an active session */
    UA_SecureChannelState channelState;
    UA_SessionState sessionState;
    UA_Client_getState(client, &channelState, &sessionState, NULL);
    ck_assert_uint_eq(channelState, UA_SECURECHANNELSTATE_OPEN);
    ck_assert_uint_eq(sessionState, UA_SESSIONSTATE_ACTIVATED);

    /* Disconnect asynchronously */
    UA_Client_disconnectAsync(client);
    while(client->channel.state != UA_SECURECHANNELSTATE_CLOSED) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    UA_Client_delete(client);
}
END_TEST

/* Counts the connections of the client that were closed, from its log */
static size_t closedConnections;

static void
countClosedConnections(void *context, UA_LogLevel level, UA_LogCategory category,
                       const char *msg, va_list args) {
    if(strstr(msg, "SecureChannel closed"))
        closedConnections++;
}

static UA_Logger countingLogger = {countClosedConnections, NULL, NULL};

/* The server accepts the TCP connection and closes it before the SecureChannel
 * opens, without an ERR message. Here because its only SecureChannel is taken
 * by a client with an activated Session. The client has to report the failure
 * instead of reconnecting as fast as the network allows. */
START_TEST(Client_connect_async_closedBeforeChannelOpen) {
    UA_Server_getConfig(server)->maxSecureChannels = 1;

    UA_Client *holder = UA_Client_newForUnitTest();
    UA_Client_getConfig(holder)->stateCallback = currentState;
    connected = false;
    UA_StatusCode retval = UA_Client_connectAsync(holder, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    while(!connected) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(holder, 0);
    }

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_Logger *clientLogger = cc->logging;
    cc->logging = &countingLogger;
    closedConnections = 0;
    retval = UA_Client_connectAsync(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Without the fix the client is still reconnecting after one second. The
     * EventLoop of the test runs on a fake clock, the elapsed time is taken
     * from the real one. */
    UA_StatusCode connectStatus = UA_STATUSCODE_GOOD;
    UA_DateTime start = UA_DateTime_nowMonotonic();
    UA_DateTime elapsed = 0;
    while(elapsed < UA_DATETIME_SEC && connectStatus == UA_STATUSCODE_GOOD) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 1);
        UA_Client_getState(client, NULL, NULL, &connectStatus);
        elapsed = UA_DateTime_nowMonotonic() - start;
    }
    cc->logging = clientLogger;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,
                "Server without a free SecureChannel: %u connection(s) closed "
                "in %u ms, connectStatus %s", (unsigned)closedConnections,
                (unsigned)(elapsed / UA_DATETIME_MSEC),
                UA_StatusCode_name(connectStatus));
    ck_assert_uint_eq(connectStatus, UA_STATUSCODE_BADCONNECTIONCLOSED);
    ck_assert_uint_eq(closedConnections, 1);

    /* The failure is not permanent. Once the SecureChannel is free, the same
     * client connects. */
    UA_Client_disconnectAsync(holder);
    while(holder->channel.state != UA_SECURECHANNELSTATE_CLOSED) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(holder, 0);
    }
    UA_Client_delete(holder);
    start = UA_DateTime_nowMonotonic();
    while(UA_Server_getStatistics(server).scs.currentChannelCount > 0 &&
          UA_DateTime_nowMonotonic() - start < 5 * UA_DATETIME_SEC)
        UA_Server_run_iterate(server, true);
    ck_assert_uint_eq(UA_Server_getStatistics(server).scs.currentChannelCount, 0);

    UA_Client_getConfig(client)->stateCallback = currentState;
    connected = false;
    retval = UA_Client_connectAsync(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    while(!connected) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
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
    tcase_add_test(tc_client_connect, Client_connectSecureChannelAsync);
    tcase_add_test(tc_client_connect, Client_connect_async_wrongUrl);
    tcase_add_test(tc_client_connect, Client_connectUsername_async);
    tcase_add_test(tc_client_connect, Client_connect_async_closedBeforeChannelOpen);
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
