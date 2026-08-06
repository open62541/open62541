/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. */

#include <winsock2.h>

#include <open62541/plugin/eventloop.h>
#include <open62541/types.h>
#include <open62541/types_generated.h>

#include <check.h>
#include <stdlib.h>
#include <string.h>

#define SCALE_CONNECTIONS 1100u
#define UDP_BURST_MESSAGES 256u

static UA_EventLoop *el;
static UA_ConnectionManager *cm;
static char serverTag;
static char clientTag;
static UA_UInt16 serverPort;
static size_t clientEstablished;
static size_t serverAccepted;
static uintptr_t clientId;
static size_t udpMessagesReceived;
static size_t clientClosing;

static void
stopAndFreeEventLoop(void) {
    el->stop(el);
    for(size_t i = 0;
        i < 2000 && el->state != UA_EVENTLOOPSTATE_STOPPED; i++)
        ck_assert_uint_eq(el->run(el, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->state, UA_EVENTLOOPSTATE_STOPPED);
    ck_assert_uint_eq(el->free(el), UA_STATUSCODE_GOOD);
    el = NULL;
    cm = NULL;
}

static UA_StatusCode
openTcpListener(UA_ConnectionManager_connectionCallback callback) {
    UA_Boolean listen = true;
    UA_UInt16 port = 0;
    UA_String address = UA_STRING("127.0.0.1");
    UA_KeyValuePair pairs[3];
    pairs[0].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&pairs[0].value, &listen,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    pairs[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&pairs[1].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    pairs[2].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&pairs[2].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap params = {3, pairs};
    return cm->openConnection(cm, &params, &serverTag, NULL, callback);
}

static UA_StatusCode
openTcpClient(UA_ConnectionManager_connectionCallback callback) {
    UA_UInt16 port = serverPort;
    UA_String address = UA_STRING("127.0.0.1");
    UA_KeyValuePair pairs[2];
    pairs[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&pairs[0].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    pairs[1].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&pairs[1].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap params = {2, pairs};
    return cm->openConnection(cm, &params, &clientTag, NULL, callback);
}

static void
tcpScaleCallback(UA_ConnectionManager *manager, uintptr_t connectionId,
                 void *application, void **connectionContext,
                 UA_ConnectionState state, const UA_KeyValueMap *params,
                 UA_ByteString message) {
    (void)manager;
    (void)connectionContext;
    (void)message;
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        if(application == &clientTag)
            clientClosing++;
        return;
    }
    if(state != UA_CONNECTIONSTATE_ESTABLISHED)
        return;

    if(application == &serverTag) {
        const UA_UInt16 *listenPort = (const UA_UInt16*)
            UA_KeyValueMap_getScalar(
                params, UA_QUALIFIEDNAME(0, "listen-port"),
                &UA_TYPES[UA_TYPES_UINT16]);
        if(listenPort)
            serverPort = *listenPort;
        else
            serverAccepted++;
    } else if(application == &clientTag) {
        clientId = connectionId;
        clientEstablished++;
    }
}

static void
configureSmallBuffers(void) {
    UA_UInt32 bufferSize = 1024;
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(
            &cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "recv-bufsize"),
            &bufferSize, &UA_TYPES[UA_TYPES_UINT32]),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(
            &cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "send-bufsize"),
            &bufferSize, &UA_TYPES[UA_TYPES_UINT32]),
        UA_STATUSCODE_GOOD);
}

static UA_UInt16
reserveTcpPort(void) {
    SOCKET probe = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ck_assert_int_ne(probe, INVALID_SOCKET);
    SOCKADDR_IN address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ck_assert_int_eq(
        bind(probe, (const struct sockaddr*)&address, sizeof(address)), 0);
    int addressLength = sizeof(address);
    ck_assert_int_eq(
        getsockname(probe, (struct sockaddr*)&address, &addressLength), 0);
    UA_UInt16 port = ntohs(address.sin_port);
    closesocket(probe);
    return port;
}

START_TEST(tcpRefusedConnectCompletion) {
    clientEstablished = 0;
    clientClosing = 0;
    el = UA_EventLoop_new_WIN32(NULL);
    cm = UA_ConnectionManager_new_WIN32_TCP(
        UA_STRING("iocp refused connect"));
    ck_assert_ptr_ne(el, NULL);
    ck_assert_ptr_ne(cm, NULL);
    configureSmallBuffers();
    ck_assert_uint_eq(
        el->registerEventSource(el, &cm->eventSource),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    serverPort = reserveTcpPort();
    (void)openTcpClient(tcpScaleCallback);
    for(size_t i = 0; i < 500 && clientClosing == 0; i++)
        ck_assert_uint_eq(el->run(el, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientEstablished, 0);
    ck_assert_uint_eq(clientClosing, 1);
    stopAndFreeEventLoop();
}
END_TEST

START_TEST(tcpMoreThanFdSetSize) {
    serverPort = 0;
    clientEstablished = 0;
    serverAccepted = 0;

    el = UA_EventLoop_new_WIN32(NULL);
    cm = UA_ConnectionManager_new_WIN32_TCP(UA_STRING("iocp tcp scale"));
    ck_assert_ptr_ne(el, NULL);
    ck_assert_ptr_ne(cm, NULL);
    configureSmallBuffers();
    ck_assert_uint_eq(
        el->registerEventSource(el, &cm->eventSource),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openTcpListener(tcpScaleCallback), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(serverPort, 0);

    for(size_t i = 0; i < SCALE_CONNECTIONS; i++) {
        ck_assert_uint_eq(openTcpClient(tcpScaleCallback), UA_STATUSCODE_GOOD);
        if((i + 1) % 32 == 0 || i + 1 == SCALE_CONNECTIONS) {
            size_t target = i + 1;
            for(size_t iteration = 0;
                iteration < 1000 &&
                (clientEstablished < target || serverAccepted < target);
                iteration++)
                ck_assert_uint_eq(el->run(el, 10), UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(clientEstablished, target);
            ck_assert_uint_eq(serverAccepted, target);
        }
    }

    ck_assert_uint_eq(clientEstablished, SCALE_CONNECTIONS);
    ck_assert_uint_eq(serverAccepted, SCALE_CONNECTIONS);
    stopAndFreeEventLoop();
}
END_TEST

START_TEST(tcpImmediateSendFailure) {
    serverPort = 0;
    clientEstablished = 0;
    serverAccepted = 0;
    clientClosing = 0;
    clientId = 0;

    el = UA_EventLoop_new_WIN32(NULL);
    cm = UA_ConnectionManager_new_WIN32_TCP(
        UA_STRING("iocp tcp send fault"));
    ck_assert_ptr_ne(el, NULL);
    ck_assert_ptr_ne(cm, NULL);
    configureSmallBuffers();
    ck_assert_uint_eq(
        el->registerEventSource(el, &cm->eventSource),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openTcpListener(tcpScaleCallback), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openTcpClient(tcpScaleCallback), UA_STATUSCODE_GOOD);
    for(size_t i = 0;
        i < 500 && (clientEstablished < 1 || serverAccepted < 1); i++)
        ck_assert_uint_eq(el->run(el, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientEstablished, 1);
    ck_assert_uint_eq(serverAccepted, 1);

    /* Test-only handle invalidation forces WSASend to fail immediately while
     * the connection still owns a pending WSARecv. */
    ck_assert_int_eq(closesocket((SOCKET)clientId), 0);
    UA_ByteString message = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(
        cm->allocNetworkBuffer(cm, clientId, &message, 32),
        UA_STATUSCODE_GOOD);
    UA_StatusCode result = cm->sendWithConnection(
        cm, clientId, &UA_KEYVALUEMAP_NULL, &message);
    ck_assert_uint_ne(result, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(message.data, NULL);

    for(size_t i = 0; i < 500 && clientClosing == 0; i++)
        ck_assert_uint_eq(el->run(el, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientClosing, 1);
    stopAndFreeEventLoop();
}
END_TEST

START_TEST(tcpHardBackpressure) {
    serverPort = 0;
    clientEstablished = 0;
    serverAccepted = 0;
    clientId = 0;

    el = UA_EventLoop_new_WIN32(NULL);
    cm = UA_ConnectionManager_new_WIN32_TCP(
        UA_STRING("iocp tcp backpressure"));
    ck_assert_ptr_ne(el, NULL);
    ck_assert_ptr_ne(cm, NULL);
    configureSmallBuffers();

    UA_UInt32 low = 256;
    UA_UInt32 high = 512;
    UA_UInt32 hard = 1024;
    UA_UInt32 global = 4096;
    UA_UInt32 messageLimit = 4;
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(&cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "send-queue-low-watermark"),
            &low, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(&cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "send-queue-high-watermark"),
            &high, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(&cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "send-queue-hard-limit"),
            &hard, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(&cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "send-queue-global-limit"),
            &global, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(&cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "send-queue-message-limit"),
            &messageLimit, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(
        el->registerEventSource(el, &cm->eventSource),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openTcpListener(tcpScaleCallback), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openTcpClient(tcpScaleCallback), UA_STATUSCODE_GOOD);
    for(size_t i = 0;
        i < 1000 && (clientEstablished < 1 || serverAccepted < 1); i++)
        ck_assert_uint_eq(el->run(el, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientEstablished, 1);
    ck_assert_uint_eq(serverAccepted, 1);

    UA_ByteString first = UA_BYTESTRING_NULL;
    UA_ByteString second = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(
        cm->allocNetworkBuffer(cm, clientId, &first, 600),
        UA_STATUSCODE_GOOD);
    memset(first.data, 0x11, first.length);
    ck_assert_uint_eq(
        cm->sendWithConnection(
            cm, clientId, &UA_KEYVALUEMAP_NULL, &first),
        UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(
        cm->allocNetworkBuffer(cm, clientId, &second, 600),
        UA_STATUSCODE_GOOD);
    memset(second.data, 0x22, second.length);
    ck_assert_uint_eq(
        cm->sendWithConnection(
            cm, clientId, &UA_KEYVALUEMAP_NULL, &second),
        UA_STATUSCODE_BADCONNECTIONCLOSED);
    ck_assert_ptr_eq(second.data, NULL);

    stopAndFreeEventLoop();
}
END_TEST

static void
udpCallback(UA_ConnectionManager *manager, uintptr_t connectionId,
            void *application, void **connectionContext,
            UA_ConnectionState state, const UA_KeyValueMap *params,
            UA_ByteString message) {
    (void)manager;
    (void)connectionContext;
    (void)params;
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        if(application == &clientTag)
            clientClosing++;
        return;
    }
    if(state == UA_CONNECTIONSTATE_ESTABLISHED &&
       application == &clientTag && !message.data)
        clientId = connectionId;
    if(message.data) {
        ck_assert_uint_eq(message.length, 16);
        udpMessagesReceived++;
    }
}

static UA_UInt16
reserveUdpPort(void) {
    SOCKET probe = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ck_assert_int_ne(probe, INVALID_SOCKET);

    SOCKADDR_IN address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;
    ck_assert_int_eq(
        bind(probe, (const struct sockaddr*)&address, sizeof(address)), 0);

    int addressLength = sizeof(address);
    ck_assert_int_eq(
        getsockname(probe, (struct sockaddr*)&address, &addressLength), 0);
    UA_UInt16 port = ntohs(address.sin_port);
    closesocket(probe);
    return port;
}

static UA_StatusCode
openUdp(UA_UInt16 port, UA_Boolean listen, void *application) {
    UA_String address = UA_STRING("127.0.0.1");
    UA_KeyValuePair pairs[3];
    pairs[0].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&pairs[0].value, &listen,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    pairs[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&pairs[1].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    pairs[2].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&pairs[2].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap params = {3, pairs};
    return cm->openConnection(
        cm, &params, application, NULL, udpCallback);
}

START_TEST(udpOverlappedBurst) {
    udpMessagesReceived = 0;
    clientId = 0;

    el = UA_EventLoop_new_WIN32(NULL);
    cm = UA_ConnectionManager_new_WIN32_UDP(UA_STRING("iocp udp burst"));
    ck_assert_ptr_ne(el, NULL);
    ck_assert_ptr_ne(cm, NULL);
    configureSmallBuffers();

    UA_UInt32 receiveDepth = 8;
    UA_UInt32 messageLimit = UDP_BURST_MESSAGES + 16;
    UA_UInt32 hardLimit = 1024 * 1024;
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(&cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "udp-receive-depth"),
            &receiveDepth, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(&cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "send-queue-message-limit"),
            &messageLimit, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_KeyValueMap_setScalar(&cm->eventSource.params,
            UA_QUALIFIEDNAME(0, "send-queue-hard-limit"),
            &hardLimit, &UA_TYPES[UA_TYPES_UINT32]), UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(
        el->registerEventSource(el, &cm->eventSource),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_UInt16 port = reserveUdpPort();
    ck_assert_uint_eq(openUdp(port, true, &serverTag), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openUdp(port, false, &clientTag), UA_STATUSCODE_GOOD);

    ck_assert_uint_ne(clientId, 0);
    for(size_t i = 0; i < UDP_BURST_MESSAGES; i++) {
        UA_ByteString message = UA_BYTESTRING_NULL;
        ck_assert_uint_eq(
            cm->allocNetworkBuffer(cm, clientId, &message, 16),
            UA_STATUSCODE_GOOD);
        memset(message.data, (int)(i & 0xffu), message.length);
        ck_assert_uint_eq(
            cm->sendWithConnection(
                cm, clientId, &UA_KEYVALUEMAP_NULL, &message),
            UA_STATUSCODE_GOOD);
        ck_assert_ptr_eq(message.data, NULL);
    }

    for(size_t i = 0;
        i < 2000 && udpMessagesReceived < UDP_BURST_MESSAGES; i++)
        ck_assert_uint_eq(el->run(el, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(udpMessagesReceived, UDP_BURST_MESSAGES);
    stopAndFreeEventLoop();
}
END_TEST

START_TEST(udpImmediateSendFailure) {
    clientId = 0;
    clientClosing = 0;
    el = UA_EventLoop_new_WIN32(NULL);
    cm = UA_ConnectionManager_new_WIN32_UDP(
        UA_STRING("iocp udp send fault"));
    ck_assert_ptr_ne(el, NULL);
    ck_assert_ptr_ne(cm, NULL);
    configureSmallBuffers();
    ck_assert_uint_eq(
        el->registerEventSource(el, &cm->eventSource),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_UInt16 port = reserveUdpPort();
    ck_assert_uint_eq(openUdp(port, false, &clientTag), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(clientId, 0);

    /* Test-only handle invalidation forces WSASendTo to fail immediately. */
    ck_assert_int_eq(closesocket((SOCKET)clientId), 0);
    UA_ByteString message = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(
        cm->allocNetworkBuffer(cm, clientId, &message, 32),
        UA_STATUSCODE_GOOD);
    UA_StatusCode result = cm->sendWithConnection(
        cm, clientId, &UA_KEYVALUEMAP_NULL, &message);
    ck_assert_uint_ne(result, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(message.data, NULL);

    for(size_t i = 0; i < 100 && clientClosing == 0; i++)
        ck_assert_uint_eq(el->run(el, 10), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientClosing, 1);
    stopAndFreeEventLoop();
}
END_TEST

int
main(void) {
    Suite *suite = suite_create("Win32 IOCP EventLoop");
    TCase *testCase = tcase_create("IOCP");
    tcase_set_timeout(testCase, 180);
    tcase_add_test(testCase, tcpRefusedConnectCompletion);
    tcase_add_test(testCase, tcpImmediateSendFailure);
    tcase_add_test(testCase, tcpMoreThanFdSetSize);
    tcase_add_test(testCase, tcpHardBackpressure);
    tcase_add_test(testCase, udpOverlappedBurst);
    tcase_add_test(testCase, udpImmediateSendFailure);
    suite_add_tcase(suite, testCase);

    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}