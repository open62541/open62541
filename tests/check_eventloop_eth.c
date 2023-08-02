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
static char *testMsg = "open62541";
static uintptr_t clientId;
static UA_Boolean received;

#define ETHERNET_INTERFACE "lo" /* use the loopback interface for testing */
#define MULTICAST_MAC_ADDRESS "00-00-00-00-00-00"

typedef struct TestContext {
    unsigned connCount;
} TestContext;

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void *application, void **connectionContext,
                   UA_ConnectionState status, const UA_KeyValueMap *params,
                   UA_ByteString msg) {
    TestContext *ctx = (TestContext*) *connectionContext;
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

    if(msg.length == 0 && status == UA_CONNECTIONSTATE_ESTABLISHED) {
        ctx->connCount++;
        clientId = connectionId;

        /* The remote-hostname is set during the first callback */
        if(params->mapSize> 0) {
            const void *hn =
                UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "remote-hostname"),
                                         &UA_TYPES[UA_TYPES_STRING]);
            ck_assert(hn != NULL);
        }
    }

    if(status == UA_CONNECTIONSTATE_CLOSING)
        ctx->connCount--;

    if(msg.length > 0) {
        UA_ByteString rcv = UA_BYTESTRING(testMsg);
        ck_assert(UA_String_equal(&msg, &rcv));
        received = true;
    }
}

START_TEST(listenETH) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_POSIX_Ethernet(UA_STRING("ethCM"));
    el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    el->registerEventSource(el, &cm->eventSource);
    el->start(el);

    TestContext testContext = {0};

    UA_String interface = UA_STRING(ETHERNET_INTERFACE);
    UA_String address = UA_STRING(MULTICAST_MAC_ADDRESS);
    UA_Boolean listen = true;

    UA_KeyValuePair params[3];
    params[0].key = UA_QUALIFIEDNAME(0, "interface");
    UA_Variant_setScalar(&params[0].value, &interface, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[1].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    params[2].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[2].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_KeyValueMap kvm = {3, params};
    UA_StatusCode res = cm->openConnection(cm, &kvm, NULL, &testContext, connectionCallback);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    ck_assert(testContext.connCount == 1);

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

    ck_assert_uint_eq(testContext.connCount, 0);
} END_TEST

START_TEST(connectETH) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_POSIX_Ethernet(UA_STRING("udpCM"));
    el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    el->registerEventSource(el, &cm->eventSource);
    el->start(el);

    UA_String interface = UA_STRING(ETHERNET_INTERFACE);
    UA_String address = UA_STRING(MULTICAST_MAC_ADDRESS);
    UA_Boolean listen = true;
    UA_UInt16 etherType = 0xb62c; /* OPC UA PubSub EtherType */

    UA_KeyValuePair params[4];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "interface");
    UA_Variant_setScalar(&params[1].value, &interface, &UA_TYPES[UA_TYPES_STRING]);
    params[2].key = UA_QUALIFIEDNAME(0, "ethertype");
    UA_Variant_setScalar(&params[2].value, &etherType, &UA_TYPES[UA_TYPES_UINT16]);
    params[3].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[3].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);

    TestContext testContext;
    testContext.connCount = 0;

    /* Don't use the address parameter for listening */
    UA_KeyValueMap kvm = {3, &params[1]};
    UA_StatusCode retval =
        cm->openConnection(cm, &kvm, NULL, &testContext, connectionCallback);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t listenSockets = testContext.connCount;

    /* Open a client connection. Don't use the listen parameter.*/
    kvm.map = params;
    clientId = 0;
    retval = cm->openConnection(cm, &kvm, NULL, &testContext, connectionCallback);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 2; i++) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    ck_assert(clientId != 0);
    ck_assert_uint_eq(testContext.connCount, listenSockets + 1);

    /* Send a message from the client */
    received = false;
    UA_ByteString snd;
    retval = cm->allocNetworkBuffer(cm, clientId, &snd, strlen(testMsg));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    memcpy(snd.data, testMsg, strlen(testMsg));
    retval = cm->sendWithConnection(cm, clientId, NULL, &snd);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    while(!received) {
        UA_DateTime next = el->run(el, 100);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    ck_assert(received);

    /* Close the connection */
    retval = cm->closeConnection(cm, clientId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(testContext.connCount, listenSockets + 1);
    for(size_t i = 0; i < 2; i++) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    ck_assert_uint_eq(testContext.connCount, listenSockets);

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
    Suite *s  = suite_create("Test ETH EventLoop");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, listenETH);
    tcase_add_test(tc, connectETH);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
