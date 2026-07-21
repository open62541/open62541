/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>

#include <check.h>
#include <stdlib.h>

typedef struct {
    uintptr_t id;
    UA_Boolean established;
    size_t messages;
} ConnectionContext;

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void *application, void **connectionContext,
                   UA_ConnectionState state, const UA_KeyValueMap *params,
                   UA_ByteString msg) {
    (void)cm;
    (void)application;
    (void)params;
    ConnectionContext *ctx = (ConnectionContext*)*connectionContext;
    ctx->id = connectionId;
    if(state == UA_CONNECTIONSTATE_ESTABLISHED) {
        ctx->established = true;
        if(msg.length > 0)
            ++ctx->messages;
    }
}

static void
runUntil(UA_EventLoop *el, UA_Boolean (*predicate)(void *), void *context) {
    for(size_t i = 0; i < 200 && !predicate(context); ++i)
        el->run(el, 50);
    ck_assert(predicate(context));
}

typedef struct {
    ConnectionContext *subscriber;
    ConnectionContext *publisher;
} EstablishedContext;

static UA_Boolean
bothEstablished(void *context) {
    EstablishedContext *ctx = (EstablishedContext*)context;
    return ctx->subscriber->established && ctx->publisher->established;
}

static UA_Boolean
receivedTwoMessages(void *context) {
    return ((ConnectionContext*)context)->messages == 2;
}

static UA_Boolean
eventLoopStopped(void *context) {
    return ((UA_EventLoop*)context)->state == UA_EVENTLOOPSTATE_STOPPED;
}

START_TEST(connectSubscribePublish) {
    const char *portString = getenv("OPEN62541_TEST_MQTT_PORT");
    ck_assert_ptr_nonnull(portString);
    UA_UInt16 port = (UA_UInt16)strtoul(portString, NULL, 10);

    UA_ConnectionManager *mqtt =
        UA_ConnectionManager_new_LWS_MQTT(UA_STRING("mqttLwsCM"));
    ck_assert_ptr_nonnull(mqtt);
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(el);
    ck_assert_uint_eq(el->registerEventSource(el, &mqtt->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_String address = UA_STRING("127.0.0.1");
    UA_String topic = UA_STRING("open62541/test");
    UA_Boolean subscribe = true;
    UA_KeyValuePair pairs[4];
    pairs[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&pairs[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    pairs[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&pairs[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    pairs[2].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&pairs[2].value, &topic, &UA_TYPES[UA_TYPES_STRING]);
    pairs[3].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&pairs[3].value, &subscribe, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap params = {4, pairs};

    ConnectionContext subscriber = {0};
    ck_assert_uint_eq(mqtt->openConnection(mqtt, &params, NULL, &subscriber,
                                           connectionCallback),
                      UA_STATUSCODE_GOOD);
    subscribe = false;
    ConnectionContext publisher = {0};
    ck_assert_uint_eq(mqtt->openConnection(mqtt, &params, NULL, &publisher,
                                           connectionCallback),
                      UA_STATUSCODE_GOOD);

    EstablishedContext established = {&subscriber, &publisher};
    runUntil(el, bothEstablished, &established);

    for(size_t i = 0; i < 2; ++i) {
        UA_ByteString msg = UA_BYTESTRING_NULL;
        ck_assert_uint_eq(mqtt->allocNetworkBuffer(mqtt, publisher.id, &msg, 14),
                          UA_STATUSCODE_GOOD);
        memcpy(msg.data, "open62541-msg", 14);
        ck_assert_uint_eq(mqtt->sendWithConnection(mqtt, publisher.id,
                                                   &UA_KEYVALUEMAP_NULL, &msg),
                          UA_STATUSCODE_GOOD);
    }
    runUntil(el, receivedTwoMessages, &subscriber);

    el->stop(el);
    runUntil(el, eventLoopStopped, el);
    el->free(el);
}
END_TEST

int
main(void) {
    Suite *suite = suite_create("LWS MQTT ConnectionManager");
    TCase *testCase = tcase_create("integration");
    tcase_add_test(testCase, connectSubscribePublish);
    suite_add_tcase(suite, testCase);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
