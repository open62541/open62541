/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/types.h"
#include "open62541/types_generated.h"

#include "testing_clock.h"
#include <time.h>
#include <stdlib.h>
#include <check.h>

static UA_EventLoop *el;
static UA_ConnectionManager *cm;
static unsigned connCount;
static char *testMsg = "open62541";
static uintptr_t clientId;
static UA_Boolean received;

static void setupEL(void) {
#if defined(UA_ARCHITECTURE_LWIP)
    el = UA_EventLoop_new_LWIP(UA_Log_Stdout, NULL);
    cm = UA_ConnectionManager_new_LWIP_TCP(UA_STRING("tcpCM"));
    el->registerEventSource(el, &cm->eventSource);
#elif defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32)
    el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    cm = UA_ConnectionManager_new_POSIX_TCP(UA_STRING("tcpCM"));
    /* Set up the TCP EventLoop parameters */
    UA_UInt32 maxSockets = 2; /* Max number of server sockets (default: 0 -> unbounded) */
    UA_KeyValueMap_setScalar(&cm->eventSource.params, UA_QUALIFIEDNAME(0, "max-sockets"),
                             (void *)&maxSockets, &UA_TYPES[UA_TYPES_UINT32]);
    el->registerEventSource(el, &cm->eventSource);
#else
#error Add other EventLoop implementations here
#endif
}

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void *application, void **connectionContext,
                   UA_ConnectionState status,
                   const UA_KeyValueMap *params,
                   UA_ByteString msg) {
    if(status == UA_CONNECTIONSTATE_CLOSING) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Closing connection %u", (unsigned)connectionId);
    } else {
        if(msg.length == 0) {
            UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Opening connection %u", (unsigned)connectionId);
        } else {
            UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Received a message of length %u", (unsigned)msg.length);
        }
    }

    if(*connectionContext != NULL)
        clientId = connectionId;
    if(msg.length == 0 && status == UA_CONNECTIONSTATE_ESTABLISHED)
        connCount++;

    if(status == UA_CONNECTIONSTATE_CLOSING)
        connCount--;

    if(msg.length > 0) {
        UA_ByteString rcv = UA_BYTESTRING(testMsg);
        ck_assert(UA_String_equal(&msg, &rcv));
        received = true;
    }
}

START_TEST(listenTCP) {
    setupEL();
    el->start(el);

    UA_UInt16 port = 4840;
    UA_Boolean listen = true;

    UA_KeyValuePair params[2];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_KeyValueMap paramsMap;
    paramsMap.map = params;
    paramsMap.mapSize = 2;

    ck_assert_uint_eq(connCount, 0);

    UA_StatusCode retval = cm->openConnection(cm, &paramsMap, NULL, NULL, connectionCallback);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(connCount > 0);

#if !defined(UA_ARCHITECTURE_LWIP)
    port = 4841;
    /* This should fail because the maximum number of sockets has been reached */
    retval = cm->openConnection(cm, &paramsMap, NULL, NULL, connectionCallback);

    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
#endif

    for(size_t i = 0; i < 10; i++) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }

    /* Stop the EventLoop */
    int max_stop_iteration_count = 1000;
    int iteration = 0;
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED &&
          iteration < max_stop_iteration_count) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        iteration++;
    }
    el->free(el);
    el = NULL;

    ck_assert_uint_eq(connCount, 0);
} END_TEST

START_TEST(connectTCP) {
    setupEL();
    el->start(el);

    UA_UInt16 port = 4840;
    UA_Boolean listen = true;
    UA_String host = UA_STRING("localhost");

    UA_KeyValuePair params[3];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[2].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[2].value, &host, &UA_TYPES[UA_TYPES_STRING]);

    UA_KeyValueMap paramsMap;
    paramsMap.map = params;
    paramsMap.mapSize = 3;

    connCount = 0;

    cm->openConnection(cm, &paramsMap, NULL, NULL, connectionCallback);

    size_t listenSockets = connCount;

#if !defined(UA_ARCHITECTURE_LWIP)
    /* Set up the TCP EventLoop parameters */
    UA_UInt32 maxSockets = listenSockets + 1; /* Max number of server sockets (default: 0 -> unbounded) */
    UA_KeyValueMap_setScalar(&cm->eventSource.params, UA_QUALIFIEDNAME(0, "max-sockets"),
                             (void *)&maxSockets, &UA_TYPES[UA_TYPES_UINT32]);
#endif

    /* Open a client connection */
    clientId = 0;
    listen = false;

    UA_StatusCode retval =
        cm->openConnection(cm, &paramsMap, NULL, (void*)0x01, connectionCallback);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 2; i++) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    ck_assert(clientId != 0);

    ck_assert_uint_eq(connCount, listenSockets + 2);

    /* Send a message from the client */
    received = false;
    UA_ByteString snd;
    retval = cm->allocNetworkBuffer(cm, clientId, &snd, strlen(testMsg));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    memcpy(snd.data, testMsg, strlen(testMsg));
    retval = cm->sendWithConnection(cm, clientId, NULL, &snd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 2; i++) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    ck_assert(received);

#if !defined(UA_ARCHITECTURE_LWIP)
    /* Open a second client connection.
     * This should fail because the maximum number of sockets has been reached */
    retval = cm->openConnection(cm, &paramsMap, NULL, (void*)0x01, connectionCallback);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 2; i++) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
#endif

    /* Close the connection */
    retval = cm->closeConnection(cm, clientId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(connCount, listenSockets + 2);
    for(size_t i = 0; i < 2; i++) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    ck_assert_uint_eq(connCount, listenSockets);

    /* Stop the EventLoop */
    int max_stop_iteration_count = 10;
    int iteration = 0;
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED &&
          iteration < max_stop_iteration_count) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        iteration++;
    }
    ck_assert(el->state == UA_EVENTLOOPSTATE_STOPPED);
    el->free(el);
    el = NULL;
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test TCP EventLoop");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, listenTCP);
    tcase_add_test(tc, connectTCP);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
