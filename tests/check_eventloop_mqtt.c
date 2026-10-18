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

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
#include <open62541/plugin/create_certificate.h>
#include <mbedtls/version.h>
#include <mbedtls/ssl.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <psa/crypto.h>
#endif

#ifdef _WIN32
#define newTestEventLoop UA_EventLoop_new_WIN32
#define newTestTCP UA_ConnectionManager_new_WIN32_TCP
#else
#define newTestEventLoop UA_EventLoop_new_POSIX
#define newTestTCP UA_ConnectionManager_new_POSIX_TCP
#endif

unsigned int messageCount = 0;
static unsigned int establishedCount = 0;

UA_String broker_hostname = {0, NULL};
UA_UInt16 broker_port = 0;

static char* get_mqtt_broker_address(void) {
    char* broker = getenv("OPEN62541_TEST_MQTT_BROKER");
    if (!broker) broker = "opc.mqtt://127.0.0.1:1883";
    return broker;
}

static void setup(void) {
    messageCount = 0;
    establishedCount = 0;
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
    UA_StatusCode res = TestConnectionManager_createConnection(cm, application, context, callback, &connectionId);
    if(res == UA_STATUSCODE_GOOD)
        res = TestConnectionManager_inject(cm, connectionId, UA_CONNECTIONSTATE_OPENING, NULL, NULL);
    return res;
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
    UA_DelayedCallback closer;
} OfflineMQTT;

static OfflineMQTT
setupOfflineMQTT(const TestConnectionManager_CallbackOverloads *overloads,
                 UA_Boolean fakeClock) {
    OfflineMQTT test = {0};
    test.tcp = TestConnectionManager_new("tcp", overloads);
    ck_assert_ptr_ne(test.tcp, NULL);
    test.mqtt = UA_ConnectionManager_new_MQTT(UA_STRING("mqttCM"));
    ck_assert_ptr_ne(test.mqtt, NULL);
    test.loop = newTestEventLoop(UA_Log_Stdout);
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
    ck_assert(!test->closer.callback);
    ck_assert_uint_eq(test->loop->free(test->loop), UA_STATUSCODE_GOOD);
}

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void *application, void **connectionContext,
                   UA_ConnectionState status,
                   const UA_KeyValueMap *params,
                   UA_ByteString msg) {
    uintptr_t *id = *(uintptr_t**)connectionContext;
    if(status == UA_CONNECTIONSTATE_ESTABLISHED && !msg.length)
        establishedCount++;
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
    UA_ConnectionManager *cm = newTestTCP(UA_STRING("tcpCM"));
    UA_ConnectionManager *mcm = UA_ConnectionManager_new_MQTT(UA_STRING("mqttCM"));
    UA_EventLoop *el = newTestEventLoop(UA_Log_Stdout);
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

    /* Wait for the broker to acknowledge both topic connections. */
    for(unsigned i = 0; establishedCount < 2 && i < 100; i++)
        el->run(el, 100);
    ck_assert_uint_eq(establishedCount, 2);

    /* Send with the subscribed connection succeeds */
    UA_ByteString msg;
    ck_assert_uint_eq(mcm->allocNetworkBuffer(mcm, subscribeConnectionId, &msg, 13), UA_STATUSCODE_GOOD);
    memcpy(msg.data, "open62541-msg", 13);
    res = mcm->sendWithConnection(mcm, subscribeConnectionId,
                                  &UA_KEYVALUEMAP_NULL, &msg);
    ck_assert(res == UA_STATUSCODE_GOOD);

    /* Send with the publish connection */
    ck_assert_uint_eq(mcm->allocNetworkBuffer(mcm, publishConnectionId, &msg, 13), UA_STATUSCODE_GOOD);
    memcpy(msg.data, "open62541-msg", 13);
    res = mcm->sendWithConnection(mcm, publishConnectionId,
                                  &UA_KEYVALUEMAP_NULL, &msg);
    ck_assert(res == UA_STATUSCODE_GOOD);

    /* Receive the message */
    ck_assert_uint_eq(messageCount, 0);
    for(unsigned i = 0; messageCount < 2 && i < 100; i++)
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
    ck_assert_uint_eq(txCount, 1); /* CONNECT only, before CONNACK */
    UA_Byte ack[] = {0x20, 2, 0, 0};
    UA_ByteString connack = {sizeof(ack), ack};
    ck_assert_uint_eq(TestConnectionManager_inject(test.tcp, 100, UA_CONNECTIONSTATE_ESTABLISHED, NULL, &connack), UA_STATUSCODE_GOOD);
    UA_fakeSleep(751);
    test.loop->run(test.loop, 1);
    const UA_ByteString *sent = TestConnectionManager_getLastSent(test.tcp);
    ck_assert_uint_eq(sent->length, 2);
    ck_assert_uint_eq(sent->data[0], 0xc0); /* PINGREQ */

    teardownOfflineMQTT(&test);
} END_TEST

typedef struct {
    uintptr_t id;
    unsigned closed;
    unsigned established;
    unsigned received;
    UA_Boolean stopOnMessage;
} TopicState;

static void
trackTopic(UA_ConnectionManager *cm, uintptr_t id, void *application, void **context,
           UA_ConnectionState state, const UA_KeyValueMap *params, UA_ByteString msg) {
    TopicState *topic = (TopicState*)application;
    topic->id = id;
    if(state == UA_CONNECTIONSTATE_ESTABLISHED && !msg.length)
        topic->established++;
    if(state == UA_CONNECTIONSTATE_CLOSING)
        topic->closed++;
    if(msg.length > 0) {
        topic->received++;
        if(topic->stopOnMessage)
            cm->eventSource.stop(&cm->eventSource);
    }
}

static void
injectPacket(OfflineMQTT *test, UA_Byte *data, size_t length) {
    UA_ByteString packet = {length, data};
    ck_assert_uint_eq(TestConnectionManager_inject(test->tcp, 100,
        UA_CONNECTIONSTATE_ESTABLISHED, NULL, &packet), UA_STATUSCODE_GOOD);
}

static UA_KeyValueMap
authenticatedParameters(void) {
    UA_String address = UA_STRING("broker.example");
    UA_String name = UA_STRING("topic");
    UA_String clientId = UA_STRING("test-client");
    UA_String username = UA_STRING("test-user");
    UA_String password = UA_STRING("test-pass");
    UA_KeyValuePair kvp[5];
    const char *keys[] = {"address", "topic", "client-id", "username", "password"};
    UA_String *values[] = {&address, &name, &clientId, &username, &password};
    for(size_t i = 0; i < 5; i++) {
        kvp[i].key = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)keys[i]);
        UA_Variant_setScalar(&kvp[i].value, values[i], &UA_TYPES[UA_TYPES_STRING]);
    }
    UA_KeyValueMap source = {5, kvp}, params = UA_KEYVALUEMAP_NULL;
    ck_assert_uint_eq(UA_KeyValueMap_copy(&source, &params), UA_STATUSCODE_GOOD);
    return params;
}

static void
openAuthenticatedTopic(OfflineMQTT *test, TopicState *topic) {
    UA_KeyValueMap params = authenticatedParameters();
    ck_assert_uint_eq(test->mqtt->openConnection(test->mqtt, &params, topic, NULL, trackTopic), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_clear(&params);
}

START_TEST(credentialsAndAcknowledgements) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);
    TopicState topic = {0};
    openAuthenticatedTopic(&test, &topic);
    const UA_ByteString *sent = TestConnectionManager_getLastSent(test.tcp);
    const UA_Byte expected[] = {0x10, 45, 0, 4, 'M', 'Q', 'T', 'T', 4, 0xc2, 1, 0x90,
        0, 11, 't','e','s','t','-','c','l','i','e','n','t',
        0, 9, 't','e','s','t','-','u','s','e','r', 0, 9, 't','e','s','t','-','p','a','s','s'};
    ck_assert_uint_eq(sent->length, sizeof(expected));
    ck_assert_mem_eq(sent->data, expected, sizeof(expected));
    ck_assert_uint_eq(topic.established, 0);
    UA_Byte connack[] = {0x20, 2, 0, 0};
    injectPacket(&test, connack, sizeof(connack));
    sent = TestConnectionManager_getLastSent(test.tcp);
    ck_assert_uint_eq(sent->data[0], 0x82);
    ck_assert_uint_eq(topic.established, 0);
    UA_Byte suback[] = {0x90, 3, sent->data[2], sent->data[3], 0};
    injectPacket(&test, suback, sizeof(suback));
    ck_assert_uint_eq(topic.established, 1);
    teardownOfflineMQTT(&test);
    ck_assert_uint_eq(topic.closed, 1);
} END_TEST

START_TEST(rejectedAcknowledgements) {
    for(unsigned rejectedSubscribe = 0; rejectedSubscribe < 2; rejectedSubscribe++) {
        OfflineMQTT test = setupOfflineMQTT(NULL, false);
        TopicState topic = {0};
        openAuthenticatedTopic(&test, &topic);
        UA_Byte connack[] = {0x20, 2, 0, rejectedSubscribe ? 0 : 4};
        injectPacket(&test, connack, sizeof(connack));
        if(rejectedSubscribe) {
            const UA_ByteString *sent = TestConnectionManager_getLastSent(test.tcp);
            UA_Byte suback[] = {0x90, 3, sent->data[2], sent->data[3], 0x80};
            injectPacket(&test, suback, sizeof(suback));
        }
        test.loop->run(test.loop, 1);
        ck_assert_uint_eq(topic.established, 0);
        ck_assert_uint_eq(topic.closed, 1);
        teardownOfflineMQTT(&test);
    }
} END_TEST

START_TEST(publisherWaitsForConnack) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);
    UA_KeyValueMap params = authenticatedParameters();
    UA_Boolean subscribe = false;
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "subscribe"), &subscribe, &UA_TYPES[UA_TYPES_BOOLEAN]), UA_STATUSCODE_GOOD);
    TopicState topic = {0};
    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &params, &topic, NULL, trackTopic), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(topic.established, 0);
    UA_ByteString message;
    ck_assert_uint_eq(test.mqtt->allocNetworkBuffer(test.mqtt, topic.id, &message, 1), UA_STATUSCODE_GOOD);
    message.data[0] = 'x';
    ck_assert_uint_eq(test.mqtt->sendWithConnection(test.mqtt, topic.id, &UA_KEYVALUEMAP_NULL, &message), UA_STATUSCODE_BADCONNECTIONREJECTED);
    UA_Byte connack[] = {0x20, 2, 0, 0};
    injectPacket(&test, connack, sizeof(connack));
    ck_assert_uint_eq(topic.established, 1);
    UA_KeyValueMap_clear(&params);
    teardownOfflineMQTT(&test);
    ck_assert_uint_eq(topic.closed, 1);
} END_TEST

START_TEST(credentialsSeparateBrokerConnections) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);
    UA_KeyValueMap params = authenticatedParameters();
    TopicState topics[3] = {{0}, {0}, {0}};
    for(unsigned i = 0; i < 2; i++)
        ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &params, &topics[i], NULL, trackTopic), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 101, NULL, NULL), UA_STATUSCODE_BADNOTFOUND);
    const char *keys[] = {"client-id", "username", "password"};
    UA_String different = UA_STRING("different");
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, (char*)(uintptr_t)keys[_i]), &different, &UA_TYPES[UA_TYPES_STRING]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &params, &topics[2], NULL, trackTopic), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 101, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_clear(&params);
    teardownOfflineMQTT(&test);
    for(unsigned i = 0; i < 3; i++)
        ck_assert_uint_eq(topics[i].closed, 1);
} END_TEST

START_TEST(invalidSecurityParametersAreRejected) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);
    UA_KeyValueMap params = authenticatedParameters();
    UA_Boolean enabled = true, disabled = false;
    UA_UInt16 wrongType = 1;
    UA_Byte raw[] = {'a', 0, 'b'};
    UA_String embeddedZero = {sizeof(raw), raw};
    UA_ByteString certificate = UA_BYTESTRING("invalid certificate");
    UA_StatusCode expected = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(_i == 0) {
        UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "username"), &wrongType, &UA_TYPES[UA_TYPES_UINT16]);
        expected = UA_STATUSCODE_BADCONNECTIONREJECTED;
    } else if(_i == 1) {
        UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "useSSL"), &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
        UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "use-tls"), &disabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    } else if(_i == 2) {
        UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "password"), &embeddedZero, &UA_TYPES[UA_TYPES_STRING]);
    } else if(_i == 3) {
        UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "certificate"), &certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
        expected = UA_STATUSCODE_BADNOTSUPPORTED;
    } else {
        UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "use-tls"), &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
        if(_i == 5)
            UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "ca-certificate"), &certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
        expected = UA_STATUSCODE_BADNOTCONNECTED;
#else
        expected = UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    }
    TopicState topic = {0};
    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &params, &topic, NULL, trackTopic), expected);
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 100, NULL, NULL), UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(topic.established, 0);
    UA_KeyValueMap_clear(&params);
    teardownOfflineMQTT(&test);
} END_TEST

#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
typedef struct {
    UA_ByteString input;
    UA_ByteString output;
    size_t offset;
} TLSBroker;

static void
appendTLSData(UA_ByteString *target, const UA_Byte *data, size_t size) {
    UA_Byte *buffer = (UA_Byte*)UA_realloc(target->data, target->length + size);
    ck_assert_ptr_ne(buffer, NULL);
    memcpy(buffer + target->length, data, size);
    target->data = buffer;
    target->length += size;
}

static UA_StatusCode
captureTLSClient(UA_ConnectionManager *cm, uintptr_t id, const UA_KeyValueMap *params, UA_ByteString *buf) {
    TLSBroker *broker = (TLSBroker*)TestConnectionManager_getContext(cm);
    appendTLSData(&broker->input, buf->data, buf->length);
    UA_ByteString_clear(buf);
    return UA_STATUSCODE_GOOD;
}

static int
sendTLSBroker(void *context, const unsigned char *data, size_t size) {
    TLSBroker *broker = (TLSBroker*)context;
    appendTLSData(&broker->output, data, size);
    return (int)size;
}

static int
receiveTLSBroker(void *context, unsigned char *data, size_t size) {
    TLSBroker *broker = (TLSBroker*)context;
    size_t available = broker->input.length - broker->offset;
    if(!available)
        return MBEDTLS_ERR_SSL_WANT_READ;
    if(size > available)
        size = available;
    memcpy(data, broker->input.data + broker->offset, size);
    broker->offset += size;
    return (int)size;
}

static void
deliverTLSBroker(OfflineMQTT *test, TLSBroker *broker) {
    if(!broker->output.length)
        return;
    UA_ByteString output = broker->output;
    broker->output = UA_BYTESTRING_NULL;
    injectPacket(test, output.data, output.length);
    UA_ByteString_clear(&output);
}

#if MBEDTLS_VERSION_NUMBER < 0x04000000
static int
tlsTestRandom(void *context, unsigned char *data, size_t size) {
    return psa_generate_random(data, size) == PSA_SUCCESS ? 0 : MBEDTLS_ERR_SSL_INTERNAL_ERROR;
}
#endif

START_TEST(tlsCertificateVerification) {
    UA_String subject = UA_STRING("CN=broker.example");
    UA_String alternateName = UA_STRING("DNS:broker.example");
    UA_ByteString key = UA_BYTESTRING_NULL, certificate = UA_BYTESTRING_NULL;
    UA_UInt16 bits = 2048;
    UA_KeyValuePair option;
    option.key = UA_QUALIFIEDNAME(0, "key-size-bits");
    UA_Variant_setScalar(&option.value, &bits, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap certOptions = {1, &option};
    ck_assert_uint_eq(UA_CreateCertificate(UA_Log_Stdout, &subject, 1, &alternateName, 1,
                                         UA_CERTIFICATEFORMAT_DER, &certOptions, &key, &certificate), UA_STATUSCODE_GOOD);
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_x509_crt cert;
    mbedtls_pk_context privateKey;
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&config);
    mbedtls_x509_crt_init(&cert);
    mbedtls_pk_init(&privateKey);
    ck_assert_int_eq(mbedtls_x509_crt_parse(&cert, certificate.data, certificate.length), 0);
#if MBEDTLS_VERSION_NUMBER < 0x04000000
    ck_assert_int_eq(mbedtls_pk_parse_key(&privateKey, key.data, key.length, NULL, 0, tlsTestRandom, NULL), 0);
#else
    ck_assert_int_eq(mbedtls_pk_parse_key(&privateKey, key.data, key.length, NULL, 0), 0);
#endif
    ck_assert_int_eq(mbedtls_ssl_config_defaults(&config, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT), 0);
#if MBEDTLS_VERSION_NUMBER < 0x04000000
    mbedtls_ssl_conf_rng(&config, tlsTestRandom, NULL);
#endif
    ck_assert_int_eq(mbedtls_ssl_conf_own_cert(&config, &cert, &privateKey), 0);
    ck_assert_int_eq(mbedtls_ssl_setup(&ssl, &config), 0);
    TLSBroker broker = {0};
    mbedtls_ssl_set_bio(&ssl, &broker, sendTLSBroker, receiveTLSBroker, NULL);
    TestConnectionManager_CallbackOverloads overloads = {NULL, captureTLSClient, NULL};
    OfflineMQTT test = setupOfflineMQTT(&overloads, false);
    TestConnectionManager_setContext(test.tcp, &broker);
    UA_KeyValueMap params = authenticatedParameters();
    UA_Boolean enabled = true;
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "use-tls"), &enabled, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_ByteString otherKey = UA_BYTESTRING_NULL, otherCert = UA_BYTESTRING_NULL;
    if(_i == 2) {
        ck_assert_uint_eq(UA_CreateCertificate(UA_Log_Stdout, &subject, 1, &alternateName, 1,
                                             UA_CERTIFICATEFORMAT_DER, &certOptions, &otherKey, &otherCert), UA_STATUSCODE_GOOD);
    }
    UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "ca-certificate"), _i == 2 ? &otherCert : &certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
    if(_i == 1) {
        UA_String wrongHostname = UA_STRING("wrong.example");
        UA_KeyValueMap_setScalar(&params, UA_QUALIFIEDNAME(0, "address"), &wrongHostname, &UA_TYPES[UA_TYPES_STRING]);
    }
    TopicState topic = {0};
    ck_assert_uint_eq(test.mqtt->openConnection(test.mqtt, &params, &topic, NULL, trackTopic), UA_STATUSCODE_GOOD);
    int ret = MBEDTLS_ERR_SSL_WANT_READ;
    for(unsigned i = 0; i < 100; i++) {
        if(TestConnectionManager_getCounters(test.tcp, 100, NULL, NULL) != UA_STATUSCODE_GOOD)
            break;
        ret = mbedtls_ssl_handshake(&ssl);
        ck_assert(ret == 0 || ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);
        deliverTLSBroker(&test, &broker);
        if(ret == 0)
            break;
    }
    ck_assert_uint_eq(topic.established, 0);
    if(_i == 0) {
        ck_assert_int_eq(ret, 0);
        unsigned char plain[128];
        ret = mbedtls_ssl_read(&ssl, plain, sizeof(plain));
        ck_assert_int_eq(ret, 47);
        ck_assert_uint_eq(plain[0], 0x10); /* CONNECT */
        ck_assert_mem_eq(plain + 14, "test-client", 11);
        ck_assert_mem_eq(plain + 27, "test-user", 9);
        ck_assert_mem_eq(plain + 38, "test-pass", 9);
        unsigned char connack[] = {0x20, 2, 0, 0};
        ck_assert_int_eq(mbedtls_ssl_write(&ssl, connack, sizeof(connack)), sizeof(connack));
        deliverTLSBroker(&test, &broker);
        ret = mbedtls_ssl_read(&ssl, plain, sizeof(plain));
        ck_assert_int_eq(ret, 12);
        ck_assert_uint_eq(plain[0], 0x82); /* SUBSCRIBE */
        ck_assert_uint_eq(topic.established, 0);
        unsigned char reply[] = {0x90, 3, plain[2], plain[3], 0, 0x30, 8, 0, 5, 't', 'o', 'p', 'i', 'c', 'x'};
        ck_assert_int_eq(mbedtls_ssl_write(&ssl, reply, sizeof(reply)), sizeof(reply));
        deliverTLSBroker(&test, &broker);
        ck_assert_uint_eq(topic.established, 1);
        ck_assert_uint_eq(topic.received, 1);
    } else {
        ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 100, NULL, NULL), UA_STATUSCODE_BADNOTFOUND);
    }
    teardownOfflineMQTT(&test);
    ck_assert_uint_eq(topic.closed, 1);
    UA_KeyValueMap_clear(&params);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&config);
    mbedtls_x509_crt_free(&cert);
    mbedtls_pk_free(&privateKey);
    UA_ByteString_clear(&key);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&otherKey);
    UA_ByteString_clear(&otherCert);
    UA_ByteString_clear(&broker.input);
    UA_ByteString_clear(&broker.output);
} END_TEST
#endif


static void
openTrackedTopic(OfflineMQTT *test, TopicState *topic) {
    UA_String address = UA_STRING("broker.example");
    UA_String name = UA_STRING("topic");
    UA_KeyValuePair kvp[2];
    kvp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&kvp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    kvp[1].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&kvp[1].value, &name, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap params = {2, kvp};
    ck_assert_uint_eq(test->mqtt->openConnection(test->mqtt, &params, topic, NULL, trackTopic), UA_STATUSCODE_GOOD);
}

static void
closeTopicDelayed(void *application, void *context) {
    OfflineMQTT *test = (OfflineMQTT*)application;
    TopicState *topic = (TopicState*)context;
    test->closer.callback = NULL;
    ck_assert_uint_eq(test->mqtt->closeConnection(test->mqtt, topic->id), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(test->mqtt->closeConnection(test->mqtt, topic->id), UA_STATUSCODE_GOOD);
}

START_TEST(pendingTopicRemovalDuringBrokerClose) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);
    TopicState topics[2] = {{0}, {0}};
    openTrackedTopic(&test, &topics[0]);
    openTrackedTopic(&test, &topics[1]);
    if(_i == 0)
        closeTopicDelayed(&test, &topics[0]);
    else if(_i == 1) {
        test.closer.callback = closeTopicDelayed;
        test.closer.application = &test;
        test.closer.context = &topics[0];
        test.loop->addDelayedCallback(test.loop, &test.closer);
    }
    ck_assert_uint_eq(TestConnectionManager_inject(test.tcp, 100, UA_CONNECTIONSTATE_CLOSING, NULL, NULL), UA_STATUSCODE_GOOD);
    if(_i == 2)
        closeTopicDelayed(&test, &topics[0]);
    for(unsigned i = 0; i < 3; i++)
        test.loop->run(test.loop, 1);
    ck_assert_uint_eq(topics[0].closed, 1);
    ck_assert_uint_eq(topics[1].closed, 1);
    teardownOfflineMQTT(&test);
    ck_assert_uint_eq(topics[0].closed, 1);
    ck_assert_uint_eq(topics[1].closed, 1);
} END_TEST

START_TEST(reconnectBeforeBrokerRemoval) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);
    TopicState oldTopic = {0}, newTopic = {0};
    openTrackedTopic(&test, &oldTopic);
    ck_assert_uint_eq(TestConnectionManager_inject(test.tcp, 100, UA_CONNECTIONSTATE_CLOSING, NULL, NULL), UA_STATUSCODE_GOOD);
    openTrackedTopic(&test, &newTopic);
    ck_assert_uint_eq(TestConnectionManager_getCounters(test.tcp, 101, NULL, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(oldTopic.id, newTopic.id);
    test.loop->run(test.loop, 1);
    ck_assert_uint_eq(oldTopic.closed, 1);
    ck_assert_uint_eq(newTopic.closed, 0);
    closeTopicDelayed(&test, &newTopic);
    teardownOfflineMQTT(&test);
    ck_assert_uint_eq(oldTopic.closed, 1);
    ck_assert_uint_eq(newTopic.closed, 1);
} END_TEST

START_TEST(shutdownWithPendingTopicRemoval) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);
    TopicState topic = {0};
    openTrackedTopic(&test, &topic);
    closeTopicDelayed(&test, &topic);
    teardownOfflineMQTT(&test);
    ck_assert_uint_eq(topic.closed, 1);
} END_TEST

START_TEST(shutdownFromReceiveCallback) {
    OfflineMQTT test = setupOfflineMQTT(NULL, false);
    TopicState topic = {0};
    topic.stopOnMessage = true;
    openTrackedTopic(&test, &topic);
    UA_Byte connack[] = {0x20, 2, 0, 0};
    injectPacket(&test, connack, sizeof(connack));
    const UA_ByteString *sent = TestConnectionManager_getLastSent(test.tcp);
    ck_assert_uint_ge(sent->length, 4);
    ck_assert_uint_eq(sent->data[0], 0x82);
    UA_Byte data[] = {0x90, 3, sent->data[2], sent->data[3], 0,
                      0x30, 8, 0, 5, 't', 'o', 'p', 'i', 'c', 'x'};
    UA_ByteString packet = {sizeof(data), data};
    ck_assert_uint_eq(TestConnectionManager_inject(test.tcp, 100, UA_CONNECTIONSTATE_ESTABLISHED, NULL, &packet), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(topic.received, 1);
    teardownOfflineMQTT(&test);
    ck_assert_uint_eq(topic.closed, 1);
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test MQTT TCP EventLoop");
    TCase *tcBroker = tcase_create("broker integration");
    tcase_add_checked_fixture(tcBroker, setup, teardown);
    tcase_add_loop_test(tcBroker, connectSubscribePublish, 0, 2);
    suite_add_tcase(s, tcBroker);
    TCase *tcOffline = tcase_create("offline broker reuse");
    tcase_add_test(tcOffline, distinctBrokersUseDistinctTcpConnections);
    tcase_add_test(tcOffline, distinctBrokerOptionsUseDistinctTcpConnections);
    tcase_add_test(tcOffline, subscriptionSendFailureClosesConnection);
    tcase_add_test(tcOffline, keepAliveTimerUsesMilliseconds);
    suite_add_tcase(s, tcOffline);
    TCase *tcShutdown = tcase_create("offline shutdown");
    tcase_add_loop_test(tcShutdown, pendingTopicRemovalDuringBrokerClose, 0, 3);
    tcase_add_test(tcShutdown, reconnectBeforeBrokerRemoval);
    tcase_add_test(tcShutdown, shutdownWithPendingTopicRemoval);
    tcase_add_test(tcShutdown, shutdownFromReceiveCallback);
    suite_add_tcase(s, tcShutdown);
    TCase *tcAuthentication = tcase_create("offline authentication");
    tcase_add_test(tcAuthentication, credentialsAndAcknowledgements);
    tcase_add_test(tcAuthentication, rejectedAcknowledgements);
    tcase_add_test(tcAuthentication, publisherWaitsForConnack);
    tcase_add_loop_test(tcAuthentication, credentialsSeparateBrokerConnections, 0, 3);
    tcase_add_loop_test(tcAuthentication, invalidSecurityParametersAreRejected, 0, 6);
#ifdef UA_ENABLE_ENCRYPTION_MBEDTLS
    tcase_add_loop_test(tcAuthentication, tlsCertificateVerification, 0, 3);
#endif
    suite_add_tcase(s, tcAuthentication);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
