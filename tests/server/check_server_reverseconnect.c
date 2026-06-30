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

static UA_UInt16 reverseListenPort = 4841;
static UA_UInt16 nextReverseListenPort = 4841;
static char reverseConnectUrl[64];

static int numServerCallbackCalled = 0;
static UA_SecureChannelState serverCallbackStates[64];
static UA_UInt64 reverseConnectHandle = 0;

static int numClientCallbackCalled = 0;
static UA_SecureChannelState clientCallbackStates[100];

#define REVERSE_RECONNECT_INTERVAL_TEST 10
#define REVERSECONNECT_MAX_ITERATIONS 1000

bool runServer = false;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while (runServer)
        UA_Server_run_iterate(server, false);
    return 0;
}

static void clientStateCallback(UA_Client *c,
                      UA_SecureChannelState channelState,
                      UA_SessionState sessionState,
                          UA_StatusCode connectStatus) {
    ck_assert(numClientCallbackCalled < (int)(sizeof(clientCallbackStates) / sizeof(UA_SecureChannelState)));
    clientCallbackStates[numClientCallbackCalled++] = channelState;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
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
    reverseListenPort = nextReverseListenPort++;
    snprintf(reverseConnectUrl, sizeof(reverseConnectUrl),
             "opc.tcp://127.0.0.1:%u", (unsigned)reverseListenPort);

    server = UA_Server_newForUnitTest();
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    UA_Array_delete(sc->serverUrls, sc->serverUrlsSize, &UA_TYPES[UA_TYPES_STRING]);
    sc->serverUrls = NULL;
    sc->serverUrlsSize = 0;
    ck_assert(server != NULL);
    sc->reverseReconnectInterval = REVERSE_RECONNECT_INTERVAL_TEST;

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
    ck_assert(numServerCallbackCalled <
              (int)(sizeof(serverCallbackStates) / sizeof(UA_SecureChannelState)));
    serverCallbackStates[numServerCallbackCalled++] = state;

    ck_assert_ptr_eq(server, s);
    ck_assert_ptr_eq(context, (void *)1234);
    ck_assert_uint_eq(handle, reverseConnectHandle);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                 "Reverse connect callback called with state %d", state);
}

static void
iterateClient(UA_UInt32 fakeSleep) {
    UA_Client_run_iterate(client, 0);
    UA_fakeSleep(fakeSleep);
}

static void
iterateClientServer(UA_UInt32 fakeSleep) {
    UA_Server_run_iterate(server, false);
    UA_Client_run_iterate(client, 0);
    UA_fakeSleep(fakeSleep);
}

static void
iterateClientUntilCallbacks(int callbacks, UA_UInt32 fakeSleep) {
    for(size_t i = 0; i < REVERSECONNECT_MAX_ITERATIONS &&
        numClientCallbackCalled < callbacks; ++i)
        iterateClient(fakeSleep);
}

static void
listenForReverseConnect(void) {
    const UA_String listenHost = UA_STRING("127.0.0.1");
    UA_StatusCode ret =
        UA_Client_startListeningForReverseConnect(client, &listenHost, 1,
                                                  reverseListenPort);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    iterateClientUntilCallbacks(numClientCallbackCalled + 1, 1);
    ck_assert_int_gt(numClientCallbackCalled, 0);
    ck_assert_int_eq(clientCallbackStates[numClientCallbackCalled - 1],
                     UA_SECURECHANNELSTATE_REVERSE_LISTENING);
}

static void
iterateClientServerUntilCallbacks(int serverCallbacks, int clientCallbacks,
                                  UA_UInt32 fakeSleep) {
    for(size_t i = 0; i < REVERSECONNECT_MAX_ITERATIONS &&
        (numServerCallbackCalled < serverCallbacks ||
         numClientCallbackCalled < clientCallbacks); ++i)
        iterateClientServer(fakeSleep);
}

static void
iterateServerUntilCallbacks(int serverCallbacks, UA_UInt32 fakeSleep) {
    for(size_t i = 0; i < REVERSECONNECT_MAX_ITERATIONS &&
        numServerCallbackCalled < serverCallbacks; ++i) {
        UA_Server_run_iterate(server, false);
        UA_fakeSleep(fakeSleep);
    }
}

static UA_Boolean
serverStatesContainSequence(size_t start, const UA_SecureChannelState *states,
                            size_t statesSize) {
    size_t match = 0;
    for(size_t i = start; i < (size_t)numServerCallbackCalled; ++i) {
        if(serverCallbackStates[i] == states[match]) {
            ++match;
            if(match == statesSize)
                return true;
        }
    }
    return false;
}

static void
iterateClientServerUntilServerSequence(size_t start,
                                       const UA_SecureChannelState *states,
                                       size_t statesSize,
                                       UA_UInt32 fakeSleep) {
    for(size_t i = 0; i < REVERSECONNECT_MAX_ITERATIONS &&
        !serverStatesContainSequence(start, states, statesSize); ++i)
        iterateClientServer(fakeSleep);
}

START_TEST(listenAndTeardown) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    UA_Server_delete(server);
    server = NULL;

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1,
                                                    reverseListenPort);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    iterateClientUntilCallbacks(1, 1000);

    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1,
                                                    reverseListenPort);
    ck_assert_uint_eq(ret, UA_STATUSCODE_BADINVALIDSTATE);

    UA_Client_disconnect(client);
    iterateClientUntilCallbacks(2, 1);

    ck_assert_int_ge(numClientCallbackCalled, 2);
    ck_assert_int_eq(clientCallbackStates[0], UA_SECURECHANNELSTATE_REVERSE_LISTENING);
    ck_assert(clientCallbackStates[1] == UA_SECURECHANNELSTATE_CLOSING ||
              clientCallbackStates[1] == UA_SECURECHANNELSTATE_CLOSED);

} END_TEST

START_TEST(noListenWhileConnected) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    runServer = true;
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_connect(client, "opc.tcp://127.0.0.1");

    ck_assert_int_ge(numClientCallbackCalled, 5);
    ck_assert_int_eq(clientCallbackStates[3], UA_SECURECHANNELSTATE_OPEN);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1,
                                                    reverseListenPort);
    ck_assert_uint_eq(ret, UA_STATUSCODE_BADINVALIDSTATE);

    UA_Client_disconnect(client);

    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1,
                                                    reverseListenPort);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(addBeforeStart) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    listenForReverseConnect();

    ret = UA_Server_addReverseConnect(server, UA_STRING(reverseConnectUrl),
                                      serverStateCallback, (void *)1234,
                                      &reverseConnectHandle);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    iterateClientServerUntilCallbacks(5, 5, 1);

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numServerCallbackCalled, 5);
    ck_assert_int_eq(serverCallbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(serverCallbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(serverCallbackStates[2], UA_SECURECHANNELSTATE_ACK_SENT);
    ck_assert_int_eq(serverCallbackStates[3], UA_SECURECHANNELSTATE_OPEN);
    ck_assert_int_eq(serverCallbackStates[4], UA_SECURECHANNELSTATE_CLOSED);

    ck_assert_int_ge(numClientCallbackCalled, 5);
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
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1,
                                                    reverseListenPort);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    iterateClientServer(1);

    ret = UA_Server_addReverseConnect(server, UA_STRING(reverseConnectUrl),
                                      serverStateCallback, (void *)1234,
                                      &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    iterateClientServerUntilCallbacks(4, 0, 1);

    UA_Server_removeReverseConnect(server, reverseConnectHandle);
    iterateServerUntilCallbacks(5, 1);

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

    listenForReverseConnect();

    ret = UA_Server_addReverseConnect(server, UA_STRING(reverseConnectUrl),
                                      serverStateCallback, (void *)1234,
                                      &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    const UA_SecureChannelState openSequence[] = {
        UA_SECURECHANNELSTATE_CONNECTING,
        UA_SECURECHANNELSTATE_RHE_SENT,
        UA_SECURECHANNELSTATE_ACK_SENT,
        UA_SECURECHANNELSTATE_OPEN
    };
    iterateClientServerUntilServerSequence(0, openSequence, 4, 1);
    ck_assert(serverStatesContainSequence(0, openSequence, 4));

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Stop listening and wait for reconnect");

    size_t reconnectStart = (size_t)numServerCallbackCalled;
    ret = UA_Client_disconnectAsync(client);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    const UA_SecureChannelState reconnectSequence[] = {
        UA_SECURECHANNELSTATE_CONNECTING
    };
    iterateClientServerUntilServerSequence(
        reconnectStart, reconnectSequence, 1,
        UA_Server_getConfig(server)->reverseReconnectInterval + 1);
    ck_assert(serverStatesContainSequence(reconnectStart, reconnectSequence, 1));

    listenForReverseConnect();

    UA_fakeSleep(UA_Server_getConfig(server)->reverseReconnectInterval + 1);

    size_t reopenStart = (size_t)numServerCallbackCalled;
    const UA_SecureChannelState reopenSequence[] = {
        UA_SECURECHANNELSTATE_RHE_SENT,
        UA_SECURECHANNELSTATE_ACK_SENT,
        UA_SECURECHANNELSTATE_OPEN
    };
    iterateClientServerUntilServerSequence(reopenStart, reopenSequence, 3, 1);
    ck_assert(serverStatesContainSequence(reopenStart, reopenSequence, 3));

    size_t removeStart = (size_t)numServerCallbackCalled;
    UA_Server_removeReverseConnect(server, reverseConnectHandle);

    const UA_SecureChannelState closeSequence[] = {
        UA_SECURECHANNELSTATE_CLOSED
    };
    iterateClientServerUntilServerSequence(removeStart, closeSequence, 1, 1);

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert(serverStatesContainSequence(removeStart, closeSequence, 1));
} END_TEST

START_TEST(removeOnShutdownWithConnection) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    listenForReverseConnect();

    ret = UA_Server_addReverseConnect(server, UA_STRING(reverseConnectUrl),
                                      serverStateCallback, (void *)1234,
                                      &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    iterateClientServerUntilCallbacks(5, 0, 1);

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

    ret = UA_Server_addReverseConnect(server, UA_STRING(reverseConnectUrl),
                                      serverStateCallback, (void *)1234,
                                      &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for(int i = 0; i < 20; ++i) {
        UA_Server_run_iterate(server, false);
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
