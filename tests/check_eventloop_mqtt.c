/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/types.h"
#include "open62541/types_generated.h"
#include "open62541/util.h"

#include "testing_clock.h"
#include "testing_networklayers.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <check.h>

unsigned int messageCount = 0;

UA_String broker_hostname = {0, NULL};
UA_UInt16 broker_port = 0;

static char* get_mqtt_broker_address(void) {
    char* broker = getenv("OPEN62541_TEST_MQTT_BROKER");
    if (!broker) broker = "opc.mqtt://127.0.0.1:1883";
    return broker;
}

static void setup(void) {
    UA_String broker = UA_STRING(get_mqtt_broker_address());
    if (UA_parseEndpointUrl(&broker, &broker_hostname, &broker_port, NULL) == UA_STATUSCODE_BADTCPENDPOINTURLINVALID) {
        printf("Failed to parse OPEN62541_TEST_MQTT_BROKER");
        exit(1);
    }
}

static void teardown(void) {}

static void
noopConnectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                       void *application, void **connectionContext,
                       UA_ConnectionState status,
                       const UA_KeyValueMap *params, UA_ByteString msg) {
    (void)cm;
    (void)connectionId;
    (void)application;
    (void)connectionContext;
    (void)status;
    (void)params;
    (void)msg;
}

static UA_StatusCode
openPendingConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                      void *application, void *context,
                      UA_ConnectionManager_connectionCallback callback) {
    (void)params;
    uintptr_t connectionId;
    return TestConnectionManager_createConnection(cm, application, context,
                                                  callback, &connectionId);
}

static UA_StatusCode
rejectSend(UA_ConnectionManager *cm, uintptr_t connectionId,
           const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)cm;
    (void)connectionId;
    (void)params;
    UA_ByteString_clear(buf);
    return UA_STATUSCODE_BADCONNECTIONCLOSED;
}

typedef struct {
    UA_ConnectionManager *tcp;
    UA_ConnectionManager *mqtt;
    UA_EventLoop *loop;
} OfflineMQTT;

static OfflineMQTT
setupOfflineMQTT(const TestConnectionManager_CallbackOverloads *overloads,
                 UA_Boolean fakeClock) {
    OfflineMQTT test;
    test.tcp = TestConnectionManager_new("tcp", overloads);
    ck_assert_ptr_ne(test.tcp, NULL);
    test.mqtt = UA_ConnectionManager_new_MQTT(UA_STRING("mqttCM"));
    ck_assert_ptr_ne(test.mqtt, NULL);
    test.loop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_ne(test.loop, NULL);
    if(fakeClock) {
        test.loop->dateTime_now = UA_DateTime_now_fake;
        test.loop->dateTime_nowMonotonic = UA_DateTime_now_fake;
    }
    ck_assert_uint_eq(test.loop->registerEventSource(test.loop,
                                                     &test.tcp->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(test.loop->registerEventSource(test.loop,
                                                     &test.mqtt->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(test.loop->start(test.loop), UA_STATUSCODE_GOOD);
    return test;
}

static void
teardownOfflineMQTT(OfflineMQTT *test) {
    test->loop->stop(test->loop);
    while(test->loop->state != UA_EVENTLOOPSTATE_STOPPED)
        test->loop->run(test->loop, 1);
    ck_assert_uint_eq(test->loop->free(test->loop), UA_STATUSCODE_GOOD);
}

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void *application, void **connectionContext,
                   UA_ConnectionState status,
                   const UA_KeyValueMap *params,
                   UA_ByteString msg) {
    uintptr_t *id = *(uintptr_t**)connectionContext;
    if(status == UA_CONNECTIONSTATE_CLOSING) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Closing connection %u", (unsigned)connectionId);
    } else if(msg.length > 0) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Received a message of length %u", (unsigned)msg.length);
        messageCount++;
    } else if(*id == 0) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                     "Opening connection %u", (unsigned)connectionId);
        *id = connectionId;
    }
}

START_TEST(connectSubscribePublish) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_POSIX_TCP(UA_STRING("tcpCM"));
    UA_ConnectionManager *mcm = UA_ConnectionManager_new_MQTT(UA_STRING("mqttCM"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    el->registerEventSource(el, &cm->eventSource);
    el->registerEventSource(el, &mcm->eventSource);
    el->start(el);

    UA_UInt16 port = broker_port;
    UA_String hostname = broker_hostname;
    UA_String topic = UA_STRING("mytopic");
    UA_Boolean subscribe = true;

    UA_KeyValuePair params[4];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[1].value, &hostname, &UA_TYPES[UA_TYPES_STRING]);
    params[2].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&params[2].value, &topic, &UA_TYPES[UA_TYPES_STRING]);
    params[3].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&params[3].value, &subscribe, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {4, params};

    uintptr_t subscribeConnectionId = 0;
    UA_StatusCode res = mcm->openConnection(mcm, &kvm, NULL,
                                            &subscribeConnectionId, connectionCallback);
    ck_assert(res == UA_STATUSCODE_GOOD);

    subscribe = false;
    uintptr_t publishConnectionId = 0;
    res = mcm->openConnection(mcm, &kvm, NULL,
                              &publishConnectionId, connectionCallback);
    ck_assert(res == UA_STATUSCODE_GOOD);

    /* Iterate to open the connection */
    el->run(el, 100);

    /* Send with the subscribed connection succeeds */
    UA_ByteString msg = UA_BYTESTRING_ALLOC("open62541-msg");
    res = mcm->sendWithConnection(mcm, subscribeConnectionId,
                                  &UA_KEYVALUEMAP_NULL, &msg);
    ck_assert(res == UA_STATUSCODE_GOOD);

    /* Send with the publish connection */
    msg = UA_BYTESTRING_ALLOC("open62541-msg");
    res = mcm->sendWithConnection(mcm, publishConnectionId,
                                  &UA_KEYVALUEMAP_NULL, &msg);
    ck_assert(res == UA_STATUSCODE_GOOD);

    /* Receive the message */
    ck_assert_uint_eq(messageCount, 0);
    while(messageCount < 2)
        el->run(el, 100);
    ck_assert_uint_eq(messageCount, 2);

    /* Stop the EventLoop */
    int max_stop_iteration_count = 10;
    int iteration = 0;
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED && iteration < max_stop_iteration_count) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
        iteration++;
    }
    ck_assert(el->state == UA_EVENTLOOPSTATE_STOPPED);
    el->free(el);
    el = NULL;
} END_TEST

START_TEST(distinctBrokersUseDistinctTcpConnections) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);

    UA_String address = UA_STRING("broker-a.example");
    UA_String topic = UA_STRING("topic-a");
    UA_UInt16 port = 1883;
    UA_Boolean subscribe = false;
    UA_KeyValuePair params[4];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[1].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    params[2].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&params[2].value, &topic,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[3].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&params[3].value, &subscribe,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {4, params};

    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &kvm, NULL, NULL,
                                                noopConnectionCallback),
                      UA_STATUSCODE_GOOD);
    address = UA_STRING("broker-b.example");
    topic = UA_STRING("topic-b");
    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &kvm, NULL, NULL,
                                                noopConnectionCallback),
                      UA_STATUSCODE_GOOD);

    /* Each broker address gets a distinct TCP connection id. */
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 100,
                                                        NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 101,
                                                        NULL, NULL),
                      UA_STATUSCODE_GOOD);

    teardownOfflineMQTT(&test);
} END_TEST

START_TEST(distinctBrokerOptionsUseDistinctTcpConnections) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);

    UA_String address = UA_STRING("broker.example");
    UA_String topic = UA_STRING("topic");
    UA_UInt16 keepAlive = 60;
    UA_KeyValuePair params[4];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&params[1].value, &topic,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap kvm = {2, params};

    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &kvm, NULL, NULL,
                                                noopConnectionCallback),
                      UA_STATUSCODE_GOOD);

    params[2].key = UA_QUALIFIEDNAME(0, "keep-alive");
    UA_Variant_setScalar(&params[2].value, &keepAlive,
                         &UA_TYPES[UA_TYPES_UINT16]);
    kvm.mapSize = 3;
    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &kvm, NULL, NULL,
                                                noopConnectionCallback),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 100,
                                                        NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 101,
                                                        NULL, NULL),
                      UA_STATUSCODE_GOOD);

    teardownOfflineMQTT(&test);
} END_TEST

START_TEST(subscriptionSendFailureClosesConnection) {
    TestConnectionManager_CallbackOverloads overloads = {
        openPendingConnection, rejectSend, NULL
    };
    OfflineMQTT test = setupOfflineMQTT(&overloads, false);

    UA_String address = UA_STRING("broker.example");
    UA_String topic = UA_STRING("topic");
    UA_Boolean subscribe = true;
    UA_KeyValuePair params[3];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&params[1].value, &topic,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[2].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&params[2].value, &subscribe,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {3, params};

    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &kvm, NULL, NULL,
                                                noopConnectionCallback),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_inject(
                          test.tcp, 100, UA_CONNECTIONSTATE_ESTABLISHED,
                          NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 100,
                                                        NULL, NULL),
                      UA_STATUSCODE_BADNOTFOUND);

    teardownOfflineMQTT(&test);
} END_TEST

START_TEST(keepAliveTimerUsesMilliseconds) {
    TestConnectionManager_CallbackOverloads overloads = {
        openPendingConnection, NULL, NULL
    };
    OfflineMQTT test = setupOfflineMQTT(&overloads, true);

    UA_String address = UA_STRING("broker.example");
    UA_String topic = UA_STRING("topic");
    UA_UInt16 keepAlive = 1;
    UA_Boolean subscribe = false;
    UA_KeyValuePair params[4];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&params[1].value, &topic,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[2].key = UA_QUALIFIEDNAME(0, "keep-alive");
    UA_Variant_setScalar(&params[2].value, &keepAlive,
                         &UA_TYPES[UA_TYPES_UINT16]);
    params[3].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&params[3].value, &subscribe,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {4, params};

    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &kvm, NULL, NULL,
                                                noopConnectionCallback),
                      UA_STATUSCODE_GOOD);

    UA_fakeSleep(751);
    test.loop->run(test.loop, 1);
    size_t txCount = 0;
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 100, NULL,
                                                        &txCount),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(txCount, 0);

    ck_assert_uint_eq(TestConnectionManager_inject(
                          test.tcp, 100, UA_CONNECTIONSTATE_ESTABLISHED,
                          NULL, NULL),
                      UA_STATUSCODE_GOOD);
    UA_fakeSleep(751);
    test.loop->run(test.loop, 1);
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 100, NULL,
                                                        &txCount),
                      UA_STATUSCODE_GOOD);
    ck_assert(txCount > 0);

    teardownOfflineMQTT(&test);
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test MQTT TCP EventLoop");
    TCase *tcBroker = tcase_create("broker integration");
    tcase_add_checked_fixture(tcBroker, setup, teardown);
    tcase_add_test(tcBroker, connectSubscribePublish);
    suite_add_tcase(s, tcBroker);
    TCase *tcOffline = tcase_create("offline broker reuse");
    tcase_add_test(tcOffline, distinctBrokersUseDistinctTcpConnections);
    tcase_add_test(tcOffline, distinctBrokerOptionsUseDistinctTcpConnections);
    tcase_add_test(tcOffline, subscriptionSendFailureClosesConnection);
    tcase_add_test(tcOffline, keepAliveTimerUsesMilliseconds);
    suite_add_tcase(s, tcOffline);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
