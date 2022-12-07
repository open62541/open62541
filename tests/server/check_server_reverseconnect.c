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

#include "check.h"
#include "testing_clock.h"

static UA_Server *server = NULL;
static UA_Client *client = NULL;

static int numCallbackCalled = 0;
static UA_SecureChannelState callbackStates[10];
static UA_UInt64 reverseConnectHandle = 0;

static void setup(void) {
    for (int i = 0; i < numCallbackCalled; ++i)
        callbackStates[i] = UA_SECURECHANNELSTATE_FRESH;

    numCallbackCalled = 0;
    reverseConnectHandle = 0;

    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
}

static void teardown(void) {
    UA_Server_delete(server);
    UA_Client_delete(client);
}

static void stateCallback(UA_Server *s, UA_UInt64 handle,
                          UA_SecureChannelState state,
                          void *context) {
    callbackStates[numCallbackCalled++] = state;

    ck_assert_ptr_eq(server, s);
    ck_assert_ptr_eq(context, (void *)1234);
    ck_assert_uint_eq(handle, reverseConnectHandle);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                 "Reverse connect callback called with state %d", state);
}

START_TEST(addBeforeStart) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                      stateCallback, (void *)1234, &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 1);
        UA_fakeSleep(1000);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numCallbackCalled, 5);
    ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(callbackStates[2], UA_SECURECHANNELSTATE_ACK_SENT);
    ck_assert_int_eq(callbackStates[3], UA_SECURECHANNELSTATE_OPEN);
    ck_assert_int_eq(callbackStates[4], UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

START_TEST(addAfterStart) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 1);

        if (i == 10) {
            ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                              stateCallback, (void *)1234, &reverseConnectHandle);

            ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
            ck_assert_uint_ne(reverseConnectHandle, 0);
        }

        if (i == 20)
            UA_Server_removeReverseConnect(server, reverseConnectHandle);

        UA_fakeSleep(1);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numCallbackCalled, 5);
    ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(callbackStates[2], UA_SECURECHANNELSTATE_ACK_SENT);
    ck_assert_int_eq(callbackStates[3], UA_SECURECHANNELSTATE_OPEN);
    ck_assert_int_eq(callbackStates[4], UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

START_TEST(checkReconnect) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                      stateCallback, (void *)1234, &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 1);

        if (i == 50) {
            ck_assert_int_eq(numCallbackCalled, 4);
            ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
            ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
            ck_assert_int_eq(callbackStates[2], UA_SECURECHANNELSTATE_ACK_SENT);
            ck_assert_int_eq(callbackStates[3], UA_SECURECHANNELSTATE_OPEN);

            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Stop listening and wait for reconnect");

            ret = UA_Client_disconnectAsync(client);
            ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

            for (int j = 0; j < 5; ++j) {
                UA_Client_run_iterate(client, 1);
                UA_Server_run_iterate(server, true);
            }

            ck_assert_int_eq(numCallbackCalled, 6);
            ck_assert_int_eq(callbackStates[4], UA_SECURECHANNELSTATE_CLOSING);
            ck_assert_int_eq(callbackStates[5], UA_SECURECHANNELSTATE_CONNECTING);

            ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
            ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

            UA_fakeSleep(UA_Server_getConfig(server)->reverseReconnectInterval + 1000);

            UA_EventLoop *serverLoop = UA_Server_getConfig(server)->eventLoop;
            UA_DateTime next = serverLoop->run(serverLoop, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));

            for (int j = 0; j < 5; ++j) {
                UA_Client_run_iterate(client, 1);
                next = serverLoop->run(serverLoop, 1);
                UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
            }

            ck_assert_int_eq(numCallbackCalled, 9);
            ck_assert_int_eq(callbackStates[6], UA_SECURECHANNELSTATE_RHE_SENT);
            ck_assert_int_eq(callbackStates[7], UA_SECURECHANNELSTATE_ACK_SENT);
            ck_assert_int_eq(callbackStates[8], UA_SECURECHANNELSTATE_OPEN);

        }

        if (i == 80)
            UA_Server_removeReverseConnect(server, reverseConnectHandle);

        UA_fakeSleep(1000);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numCallbackCalled, 10);
    ck_assert_int_eq(callbackStates[9], UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

START_TEST(removeOnShutdownWithConnection) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                      stateCallback, (void *)1234, &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    const UA_String listenHost = UA_STRING("127.0.0.1");
    ret = UA_Client_startListeningForReverseConnect(client, &listenHost, 1, 4841);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 20; ++i) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 100);

        UA_fakeSleep(1000);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numCallbackCalled, 5);
    ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(callbackStates[2], UA_SECURECHANNELSTATE_ACK_SENT);
    ck_assert_int_eq(callbackStates[3], UA_SECURECHANNELSTATE_OPEN);
    ck_assert_int_eq(callbackStates[4], UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

START_TEST(removeOnShutdownWithoutConnection) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://127.0.0.1:4841"),
                                      stateCallback, (void *)1234, &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 20; ++i) {
        UA_Server_run_iterate(server, true);
        UA_fakeSleep(1);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numCallbackCalled, 2);
    ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_CLOSED);
} END_TEST

int main(void) {
    Suite *s = suite_create("server_reverseconnect");

    TCase *tc_call = tcase_create("basics");
    tcase_add_checked_fixture(tc_call, setup, teardown);
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
