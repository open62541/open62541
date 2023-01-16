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

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"
#include "testing_clock.h"

static int max_stop_iteration_count = 100;

static UA_Server *server = NULL;

static int numCallbackCalled = 0;
static UA_SecureChannelState callbackStates[10];
static UA_UInt64 reverseConnectHandle = 0;
static UA_Boolean listening = false;
static UA_Boolean rheReceived = false;

UA_EventLoop *eventLoop = NULL;
UA_ConnectionManager *connectionManager = NULL;

static void listenCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                           void *application, void **connectionContext,
                           UA_ConnectionState state,
                           const UA_KeyValueMap *params,
                           UA_ByteString msg) {
    (void)cm;
    (void)connectionId;
    (void)application;
    (void)connectionContext;
    (void)params;
    (void)msg;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Listen callback was called with state %d and connection id %lu", state,
                (unsigned long)connectionId);

    if (msg.length) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Message: %ld %.*s",
                    (unsigned long)msg.length, (int) msg.length, (char *)msg.data);
        rheReceived = msg.length >= 3 && !strncmp((char *)msg.data, "RHE", 3);
    }

    listening = state == UA_CONNECTIONSTATE_ESTABLISHED;
}

static void setupListeningSocket(void) {
    connectionManager = UA_ConnectionManager_new_POSIX_TCP(UA_STRING("tcpCM"));

    eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    eventLoop->registerEventSource(eventLoop, &connectionManager->eventSource);
    eventLoop->start(eventLoop);

    UA_String listenHost = UA_STRING_STATIC("localhost");
    UA_UInt16 listenPort = 4841;
    UA_Boolean listen = true;

    UA_KeyValuePair params[3];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &listenPort, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[2].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setArray(&params[2].value, &listenHost, 1, &UA_TYPES[UA_TYPES_STRING]);

    UA_KeyValueMap paramsMap;
    paramsMap.map = params;
    paramsMap.mapSize = 3;

    UA_StatusCode res = connectionManager->openConnection(connectionManager, &paramsMap,
                                                          NULL, NULL, listenCallback);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 100; ++i) {
        eventLoop->run(eventLoop, 100);
        if (listening)
            break;
    }

    ck_assert_int_eq(listening, true);
}

static void stopListening(void) {
    if (eventLoop) {
        eventLoop->stop(eventLoop);

        for (int i = 0; i < max_stop_iteration_count && eventLoop->state != UA_EVENTLOOPSTATE_STOPPED; ++i) {
            UA_DateTime next = eventLoop->run(eventLoop, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        }

        eventLoop->free(eventLoop);
        eventLoop = NULL;
        connectionManager = NULL;
    }
}

static void setup(void) {
    for (int i = 0; i < numCallbackCalled; ++i)
        callbackStates[i] = UA_SECURECHANNELSTATE_FRESH;

    numCallbackCalled = 0;
    reverseConnectHandle = 0;
    listening = false;
    rheReceived = false;

    setupListeningSocket();

    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
}

static void teardown(void) {
    UA_Server_delete(server);
    stopListening();
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

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://localhost:4841"),
                                      stateCallback, (void *)1234, &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, true);
        eventLoop->run(eventLoop, 1);

        if (i == 10)
            UA_Server_removeReverseConnect(server, reverseConnectHandle);

        UA_fakeSleep(1);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numCallbackCalled, 3);
    ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(callbackStates[2], UA_SECURECHANNELSTATE_CLOSED);
    ck_assert(rheReceived);
} END_TEST

START_TEST(addAfterStart) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, true);
        eventLoop->run(eventLoop, 1);

        if (i == 10) {
            ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://localhost:4841"),
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

    ck_assert_int_eq(numCallbackCalled, 3);
    ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(callbackStates[2], UA_SECURECHANNELSTATE_CLOSED);
    ck_assert(rheReceived);
} END_TEST

START_TEST(checkReconnect) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://localhost:4841"),
                                      stateCallback, (void *)1234, &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, true);
        if (eventLoop)
            eventLoop->run(eventLoop, 1);

        if (i == 10) {
            ck_assert_int_eq(numCallbackCalled, 2);
            ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
            ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);

            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Stop listening and wait for reconnect");
            stopListening();

            UA_EventLoop *e = UA_Server_getConfig(server)->eventLoop;
            UA_DateTime next = e->run(e, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));

            ck_assert_int_eq(numCallbackCalled, 3);
            ck_assert_int_eq(callbackStates[2], UA_SECURECHANNELSTATE_CONNECTING);

            setupListeningSocket();

            ck_assert(listening);

            UA_fakeSleep(UA_Server_getConfig(server)->reverseReconnectInterval + 1000);

            next = e->run(e, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));

            if (eventLoop)
                eventLoop->run(eventLoop, 1);

            next = e->run(e, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));

            if (eventLoop)
                eventLoop->run(eventLoop, 1);

            next = e->run(e, 1);
            UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));

            ck_assert_int_eq(numCallbackCalled, 4);
            ck_assert_int_eq(callbackStates[3], UA_SECURECHANNELSTATE_RHE_SENT);
        }

        if (i == 50)
            UA_Server_removeReverseConnect(server, reverseConnectHandle);

        UA_fakeSleep(1);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numCallbackCalled, 5);
    ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(callbackStates[2], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(callbackStates[3], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(callbackStates[4], UA_SECURECHANNELSTATE_CLOSED);

    ck_assert(rheReceived);
} END_TEST

START_TEST(removeOnShutdownWithConnection) {
    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://localhost:4841"),
                                      stateCallback, (void *)1234, &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 100; ++i) {
        UA_Server_run_iterate(server, true);
        eventLoop->run(eventLoop, 1);

        UA_fakeSleep(1);
    }

    ret = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(numCallbackCalled, 3);
    ck_assert_int_eq(callbackStates[0], UA_SECURECHANNELSTATE_CONNECTING);
    ck_assert_int_eq(callbackStates[1], UA_SECURECHANNELSTATE_RHE_SENT);
    ck_assert_int_eq(callbackStates[2], UA_SECURECHANNELSTATE_CLOSED);
    ck_assert(rheReceived);
} END_TEST

START_TEST(removeOnShutdownWithoutConnection) {
    stopListening();

    UA_StatusCode ret = UA_STATUSCODE_BADINTERNALERROR;

    ret = UA_Server_addReverseConnect(server, UA_STRING("opc.tcp://localhost:4841"),
                                      stateCallback, (void *)1234, &reverseConnectHandle);

    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(reverseConnectHandle, 0);

    ret = UA_Server_run_startup(server);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);

    for (int i = 0; i < 100; ++i) {
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
