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

#ifdef UA_ARCHITECTURE_WIN32
# define UA_TEST_EVENTLOOP_NEW UA_EventLoop_new_WIN32
# define UA_TEST_TCP_MANAGER_NEW UA_ConnectionManager_new_WIN32_TCP
#else
# define UA_TEST_EVENTLOOP_NEW UA_EventLoop_new_POSIX
# define UA_TEST_TCP_MANAGER_NEW UA_ConnectionManager_new_POSIX_TCP
#endif

static UA_EventLoop *el;
static UA_ConnectionManager *cm;
static unsigned connCount;
static char *testMsg = "open62541";
static uintptr_t clientId;
static UA_Boolean received;
static UA_UInt16 listenPort;

static void setupEL(void) {
#if defined(UA_ARCHITECTURE_LWIP)
    el = UA_EventLoop_new_LWIP(UA_Log_Stdout, NULL);
    cm = UA_ConnectionManager_new_LWIP_TCP(UA_STRING("tcpCM"));
    el->registerEventSource(el, &cm->eventSource);
#elif defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32)
    el = UA_TEST_EVENTLOOP_NEW(UA_Log_Stdout);
    cm = UA_TEST_TCP_MANAGER_NEW(UA_STRING("tcpCM"));
    /* Set up the TCP EventLoop parameters */
    UA_UInt32 maxSockets = 2; /* Max number of server sockets (default: 0 -> unbounded) */
    UA_KeyValueMap_setScalar(&cm->eventSource.params, UA_QUALIFIEDNAME(0, "max-connections"),
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
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Closing connection %u", (unsigned)connectionId);
    } else {
        if(msg.length == 0) {
            UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                         "Opening connection %u", (unsigned)connectionId);
        } else {
            UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                         "Received a message of length %u", (unsigned)msg.length);
        }
    }

    const UA_UInt16 *boundPort = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "listen-port"), &UA_TYPES[UA_TYPES_UINT16]);
    if(boundPort)
        listenPort = *boundPort;

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

    UA_UInt16 port = 0;
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
    port = 0;
    /* Depending on IPv4/IPv6 availability, the first open may consume one or two
     * socket slots. Keep opening on new ports until the configured limit is hit. */
    retval = cm->openConnection(cm, &paramsMap, NULL, NULL, connectionCallback);

    if(retval == UA_STATUSCODE_GOOD) {
        port = 0;
        retval = cm->openConnection(cm, &paramsMap, NULL, NULL, connectionCallback);
    }

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

START_TEST(listenTCPAddressArrayUsesPerElementLength) {
    setupEL();
    el->start(el);

    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_Boolean validate = true;
    char tooLong[600];
    char shortAddressBacking[600] = "127.0.0.1";
    memset(tooLong, 'A', sizeof(tooLong));
    UA_String addresses[2] = {
        {sizeof(tooLong), (UA_Byte*)tooLong},
        {strlen(shortAddressBacking), (UA_Byte*)shortAddressBacking}
    };

    UA_KeyValuePair params[4];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[2].key = UA_QUALIFIEDNAME(0, "validate");
    UA_Variant_setScalar(&params[2].value, &validate, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[3].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setArray(&params[3].value, addresses, 2,
                        &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap paramsMap = {4, params};

    UA_StatusCode retval =
        cm->openConnection(cm, &paramsMap, NULL, NULL, connectionCallback);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED)
        el->run(el, 1);
    el->free(el);
    el = NULL;
} END_TEST

START_TEST(connectTCP) {
    setupEL();
    el->start(el);

    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_String host = UA_STRING("127.0.0.1");
    UA_Boolean reuseaddr = true;

    UA_KeyValuePair params[4];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[2].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[2].value, &host, &UA_TYPES[UA_TYPES_STRING]);
    params[3].key = UA_QUALIFIEDNAME(0, "reuse");
    UA_Variant_setScalar(&params[3].value, &reuseaddr, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_KeyValueMap paramsMap;
    paramsMap.map = params;
    paramsMap.mapSize = 4;

    connCount = 0;

    cm->openConnection(cm, &paramsMap, NULL, NULL, connectionCallback);

    size_t listenSockets = connCount;
    port = listenPort;
    ck_assert_uint_ne(port, 0);

#if !defined(UA_ARCHITECTURE_LWIP)
    /* Set up the TCP EventLoop parameters */
    UA_UInt32 maxSockets = (UA_UInt32)(listenSockets + 1); /* Max number of server sockets (default: 0 -> unbounded) */
    UA_KeyValueMap_setScalar(&cm->eventSource.params, UA_QUALIFIEDNAME(0, "max-connections"),
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
    for(size_t i = 0; i < 10; i++) {
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

/* The connection manager keeps a static send buffer (defaulting to the
 * recv-bufsize). allocNetworkBuffer reuses it instead of allocating per send,
 * and falls back to a fresh allocation for messages larger than the buffer. */
START_TEST(staticSendBuffer) {
    setupEL();

    /* Configure a small static send buffer */
    UA_UInt32 sendBufSize = 16;
    UA_KeyValueMap_setScalar(&cm->eventSource.params, UA_QUALIFIEDNAME(0, "send-bufsize"),
                             &sendBufSize, &UA_TYPES[UA_TYPES_UINT32]);

    el->start(el);
    /* Run once so the ConnectionManager eventSource starts its static buffers */
    el->run(el, 1);

    /* The resolved rx default was written back into the CM params (so the
     * SecureChannel logic can cap to the static-buffer size); the explicitly
     * configured send-bufsize is untouched. The rx default is
     * architecture-dependent and must match the default passed to
     * UA_EventLoopCommon_allocStaticBuffer in each eventloop porting
     * (64 KiB on POSIX/Win32, 8 KiB on lwip and Zephyr). */
#if defined(UA_ARCHITECTURE_LWIP) || defined(UA_ARCHITECTURE_ZEPHYR)
    UA_UInt32 expectedRxBufSize = 1u << 13;
#else
    UA_UInt32 expectedRxBufSize = 1u << 16;
#endif
    const UA_UInt32 *rxParam = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(&cm->eventSource.params,
                                 UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_ne(rxParam, NULL);
    ck_assert_uint_eq(*rxParam, expectedRxBufSize);
    const UA_UInt32 *txParam = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(&cm->eventSource.params,
                                 UA_QUALIFIEDNAME(0, "send-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_ne(txParam, NULL);
    ck_assert_uint_eq(*txParam, sendBufSize);

    /* A message that fits is served from the static send buffer */
    UA_ByteString a = UA_BYTESTRING_NULL;
    UA_StatusCode retval = cm->allocNetworkBuffer(cm, 0, &a, sendBufSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(a.length, sendBufSize);
    UA_Byte *reused = a.data;
    cm->freeNetworkBuffer(cm, 0, &a);

    /* The next fitting allocation reuses the same static buffer (no per-send
     * allocation) */
    UA_ByteString b = UA_BYTESTRING_NULL;
    retval = cm->allocNetworkBuffer(cm, 0, &b, sendBufSize);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(b.data, reused);
    cm->freeNetworkBuffer(cm, 0, &b);

    /* A message larger than the static buffer falls back to a fresh allocation
     * instead of failing with BadOutOfMemory */
    UA_ByteString big = UA_BYTESTRING_NULL;
    retval = cm->allocNetworkBuffer(cm, 0, &big, sendBufSize * 64);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(big.length, sendBufSize * 64);
    cm->freeNetworkBuffer(cm, 0, &big);

    /* Stop the EventLoop */
    int iteration = 0;
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED && iteration < 10) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        iteration++;
    }
    ck_assert(el->state == UA_EVENTLOOPSTATE_STOPPED);
    el->free(el);
    el = NULL;
} END_TEST

#if defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)
typedef struct {
    UA_UInt16 port;
    uintptr_t id;
    UA_Boolean closed;
} ReuseListener;

static void
reuseListenerCallback(UA_ConnectionManager *manager, uintptr_t id, void *application,
                      void **context, UA_ConnectionState state,
                      const UA_KeyValueMap *params, UA_ByteString msg) {
    (void)manager; (void)context; (void)msg;
    ReuseListener *listener = (ReuseListener*)application;
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        listener->closed = true;
        return;
    }
    const UA_UInt16 *port = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "listen-port"), &UA_TYPES[UA_TYPES_UINT16]);
    if(port) {
        listener->port = *port;
        listener->id = id;
    }
}

START_TEST(listenTCPReuseRejectsDuplicateAndAllowsRestart) {
    setupEL();
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    UA_UInt16 port = 0;
    UA_Boolean enabled = true;
    UA_String address = UA_STRING("127.0.0.1");
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "port"), &port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "address"), &address, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "listen"), &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "reuse"), &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ReuseListener first = {0}, second = {0};
    ck_assert_uint_eq(cm->openConnection(cm, &params, &first, NULL, reuseListenerCallback), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(first.port, 0);
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "port"), &first.port, &UA_TYPES[UA_TYPES_UINT16]);
    ck_assert_uint_ne(cm->openConnection(cm, &params, &second, NULL, reuseListenerCallback), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cm->closeConnection(cm, first.id), UA_STATUSCODE_GOOD);
    for(unsigned i = 0; i < 100 && !first.closed; ++i)
        el->run(el, 1);
    ck_assert(first.closed);
    ck_assert_uint_eq(cm->openConnection(cm, &params, &second, NULL, reuseListenerCallback), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_clear(&params);
    el->stop(el);
    for(unsigned i = 0; i < 100 && el->state != UA_EVENTLOOPSTATE_STOPPED; ++i)
        el->run(el, 1);
    ck_assert_uint_eq(el->state, UA_EVENTLOOPSTATE_STOPPED);
    ck_assert_uint_eq(el->free(el), UA_STATUSCODE_GOOD);
    el = NULL;
} END_TEST
#endif

int main(void) {
    Suite *s  = suite_create("Test TCP EventLoop");
    TCase *tc = tcase_create("test cases");
#if defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)
    tcase_add_test(tc, listenTCPReuseRejectsDuplicateAndAllowsRestart);
#endif
    tcase_add_test(tc, listenTCP);
    tcase_add_test(tc, listenTCPAddressArrayUsesPerElementLength);
    tcase_add_test(tc, connectTCP);
    tcase_add_test(tc, staticSendBuffer);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
