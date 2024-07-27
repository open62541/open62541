/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2022 (c) basysKom GmbH <opensource@basyskom.com> (Author: Jannis Völker)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "test_helpers.h"
#include "check.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

static UA_Server *server = NULL;
static UA_Client *client = NULL;

static int numServerCallbackCalled = 0;
static UA_SecureChannelState serverCallbackStates[10];
static UA_UInt64 reverseConnectHandle = 0;

static int numClientCallbackCalled = 0;
static UA_SecureChannelState clientCallbackStates[100];

bool runServer = false;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while (runServer)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void clientStateCallback(UA_Client *c,
                      UA_SecureChannelState channelState,
                      UA_SessionState sessionState,
                          UA_StatusCode connectStatus) {
    ck_assert(numClientCallbackCalled < (int)(sizeof(clientCallbackStates) / sizeof(UA_SecureChannelState)));
    clientCallbackStates[numClientCallbackCalled++] = channelState;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Client state callback called with state %d", channelState);
}

static void setup(void) {
    for(int i = 0; i < numServerCallbackCalled; ++i)
        serverCallbackStates[i] = UA_SECURECHANNELSTATE_CLOSED;

    for(int i = 0; i < numClientCallbackCalled; ++i)
        clientCallbackStates[i] = UA_SECURECHANNELSTATE_CLOSED;

    numServerCallbackCalled = 0;
    reverseConnectHandle = 0;
    numClientCallbackCalled = 0;

    server = UA_Server_newForUnitTest();
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->reverseReconnectInterval = 15000;
    UA_Array_delete(sc->serverUrls, sc->serverUrlsSize, &UA_TYPES[UA_TYPES_STRING]);
    sc->serverUrls = NULL;
    sc->serverUrlsSize = 0;
    ck_assert(server != NULL);

    client = UA_Client_newForUnitTest();
    UA_Client_getConfig(client)->stateCallback = clientStateCallback;
}

static void teardown(void) {
    if(runServer) {
        runServer = false;
        THREAD_JOIN(server_thread);
    }

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_Client_delete(client);
}

static void serverStateCallback(UA_Server *s, UA_UInt64 handle,
                          UA_SecureChannelState state,
                          void *context) {
    serverCallbackStates[numServerCallbackCalled++] = state;

    ck_assert_ptr_eq(server, s);
    ck_assert_ptr_eq(context, (void *)1234);
    ck_assert_uint_eq(handle, reverseConnectHandle);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Reverse connect callback called with state %d", state);
}

START_TEST(listenAndTeardown) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    UA_Server_delete(server);
    server = NULL;

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for(int i = 0; i < 10; ++i) {
        UA_Client_run_iterate(client, 1);
        UA_fakeSleep(1000);
    }

    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_BADINVALIDSTATE);

    UA_Client_disconnect(client);

    ck_assert_int_ge(numClientCallbackCalled, 3);
    ck_assert_int_eq(clientCallbackStates[0], UA_SECURECHANNELSTATE_REVERSE_LISTENING);
    ck_assert_int_eq(clientCallbackStates[1], UA_SECURECHANNELSTATE_CLOSING);
    ck_assert_int_eq(clientCallbackStates[2], UA_SECURECHANNELSTATE_CLOSED);

} END_TEST

START_TEST(noListenWhileConnected) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    runServer = true;
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_connect(client, "opc.tcp://127.0.0.1");

    ck_assert_int_gt(numClientCallbackCalled, 5);
    ck_assert_int_eq(clientCallbackStates[3], UA_SECURECHANNELSTATE_OPEN);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_BADINVALIDSTATE);

    UA_Client_disconnect(client);

    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(addBeforeStart) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                      serverStateCallback, (void *)1234,
                                      &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for(int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 1);

        if(numServerCallbackCalled == 5 &&
           numClientCallbackCalled == 5)
            break;

        UA_fakeSleep(1000);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numServerCallbackCalled, 5);
    ck_assert_int_eq(serverCallbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(serverCallbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(serverCallbackStates[2], UA_SECURECHANNELSTATE_ACK_SENT);
    ck_assert_int_eq(serverCallbackStates[3], UA_SECURECHANNELSTATE_OPEN);
    ck_assert_int_eq(serverCallbackStates[4], UA_SECURECHANNELSTATE_CLOSED);

    ck_assert_int_gt(numClientCallbackCalled, 5);
    ck_assert_int_eq(clientCallbackStates[0], UA_SECURECHANNELSTATE_REVERSE_LISTENING);
    ck_assert_int_eq(clientCallbackStates[1], UA_SECURECHANNELSTATE_REVERSE_CONNECTED);
    ck_assert_int_eq(clientCallbackStates[2], UA_SECURECHANNELSTATE_HEL_SENT);
    ck_assert_int_eq(clientCallbackStates[3], UA_SECURECHANNELSTATE_OPN_SENT);
    ck_assert_int_eq(clientCallbackStates[4], UA_SECURECHANNELSTATE_OPEN);
} END_TEST

START_TEST(addAfterStart) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for(int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 1);

        if(i == 10) {
            ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                              serverStateCallback, (void *)1234,
                                              &reverseConnectHandle);

            ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
            ck_assert_uint_ne(reverseConnectHandle, 0);
        }

        if(i == 20)
            UA_Server_removeReverseConnect(server, reverseConnectHandle);

        if(numServerCallbackCalled == 5)
            break;

        UA_fakeSleep(1);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numServerCallbackCalled, 5);
    ck_assert_int_eq(serverCallbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(serverCallbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(serverCallbackStates[2], UA_SECURECHANNELSTATE_ACK_SENT);
    ck_assert_int_eq(serverCallbackStates[3], UA_SECURECHANNELSTATE_OPEN);
    ck_assert_int_eq(serverCallbackStates[4], UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

START_TEST(checkReconnect) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                      serverStateCallback, (void *)1234,
                                      &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for(int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 1);

        if(i == 50) {
            ck_assert_int_eq(numServerCallbackCalled, 4);
            ck_assert_int_eq(serverCallbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
            ck_assert_int_eq(serverCallbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
            ck_assert_int_eq(serverCallbackStates[2], UA_SECURECHANNELSTATE_ACK_SENT);
            ck_assert_int_eq(serverCallbackStates[3], UA_SECURECHANNELSTATE_OPEN);

            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Stop listening and wait for reconnect");

            ret = UA_Client_disconnectAsync(client);
            ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

            for(int j = 0; j < 5; ++j) {
                UA_Client_run_iterate(client, 1);
                UA_Server_run_iterate(server, true);
            }

            ck_assert_int_eq(numServerCallbackCalled, 6);
            ck_assert_int_eq(serverCallbackStates[4], UA_SECURECHANNELSTATE_CLOSING);
            ck_assert_int_eq(serverCallbackStates[5], UA_SECURECHANNELSTATE_CONNECTING);

            ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
            ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

            UA_fakeSleep(UA_Server_getConfig(server)->reverseReconnectInterval + 1000);

            for(int j = 0; j < 5; ++j) {
                UA_Server_run_iterate(server, true);
                UA_Client_run_iterate(client, 1);
            }

            ck_assert_int_eq(numServerCallbackCalled, 9);
            ck_assert_int_eq(serverCallbackStates[6], UA_SECURECHANNELSTATE_RHE_SENT);
            ck_assert_int_eq(serverCallbackStates[7], UA_SECURECHANNELSTATE_ACK_SENT);
            ck_assert_int_eq(serverCallbackStates[8], UA_SECURECHANNELSTATE_OPEN);
        }

        if(i == 80)
            UA_Server_removeReverseConnect(server, reverseConnectHandle);

        if(numServerCallbackCalled == 10)
            break;

        UA_fakeSleep(1000);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numServerCallbackCalled, 10);
    ck_assert_int_eq(serverCallbackStates[9], UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

START_TEST(removeOnShutdownWithConnection) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                      serverStateCallback, (void *)1234,
                                      &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for(int i = 0; i < 20; ++i) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 100);

        if(numServerCallbackCalled == 5)
            break;

        UA_fakeSleep(1000);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numServerCallbackCalled, 5);
    ck_assert_int_eq(serverCallbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(serverCallbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(serverCallbackStates[2], UA_SECURECHANNELSTATE_ACK_SENT);
    ck_assert_int_eq(serverCallbackStates[3], UA_SECURECHANNELSTATE_OPEN);
    ck_assert_int_eq(serverCallbackStates[4], UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

START_TEST(removeOnShutdownWithoutConnection) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                      serverStateCallback, (void *)1234,
                                      &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for(int i = 0; i < 20; ++i) {
        UA_Server_run_iterate(server, true);
        UA_fakeSleep(1);

        if(numServerCallbackCalled == 2)
            break;
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numServerCallbackCalled, 2);
    ck_assert_int_eq(serverCallbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(serverCallbackStates[1], UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

int main(void) {
    Suite *s = suite_create("server_reverseconnect");

    TCase *tc_call = tcase_create("basics");
    tcase_add_checked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, listenAndTeardown);
    tcase_add_test(tc_call, noListenWhileConnected);
    tcase_add_test(tc_call, addBeforeStart);
    tcase_add_test(tc_call, addAfterStart);
    tcase_add_test(tc_call, checkReconnect);
    tcase_add_test(tc_call, removeOnShutdownWithConnection);
    tcase_add_test(tc_call, removeOnShutdownWithoutConnection);

    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
