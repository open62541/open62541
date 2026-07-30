/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uintptr_t id;
    UA_Boolean established;
    UA_Boolean closed;
    size_t messages;
} ConnectionContext;

static UA_Boolean certificateGroupCleared;
static UA_Boolean acceptCertificate;
static size_t certificateVerifyCalls;

static UA_ByteString
loadFile(const char *path) {
    UA_ByteString out = UA_BYTESTRING_NULL;
    FILE *f = fopen(path, "rb");
    if(!f || fseek(f, 0, SEEK_END) || (out.length = (size_t)ftell(f)) == 0 ||
       fseek(f, 0, SEEK_SET) ||
       UA_ByteString_allocBuffer(&out, out.length) != UA_STATUSCODE_GOOD ||
       fread(out.data, 1, out.length, f) != out.length)
        UA_ByteString_clear(&out);
    if(f)
        fclose(f);
    return out;
}

static UA_StatusCode
verifyCertificate(UA_CertificateGroup *cg, const UA_ByteString *certificate) {
    (void)cg;
    ++certificateVerifyCalls;
    if(!certificate || certificate->length == 0)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    return acceptCertificate ? UA_STATUSCODE_GOOD :
        UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
}

static void
clearCertificateGroup(UA_CertificateGroup *cg) {
    (void)cg;
    certificateGroupCleared = true;
}

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
    if(state == UA_CONNECTIONSTATE_CLOSED)
        ctx->closed = true;
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

static UA_Boolean
connectionEstablished(void *context) {
    return ((ConnectionContext*)context)->established;
}

static UA_Boolean
connectionClosed(void *context) {
    return ((ConnectionContext*)context)->closed;
}

static void
runTlsConnection(const char *portEnvironment, const char *addressString,
                 UA_Boolean accept, UA_Boolean withClientCertificate,
                 UA_Boolean expectEstablished) {
    const char *portString = getenv(portEnvironment);
    ck_assert_ptr_nonnull(portString);
    UA_UInt16 port = (UA_UInt16)strtoul(portString, NULL, 10);

    UA_ConnectionManager *mqtt =
        UA_ConnectionManager_new_LWS_MQTT(UA_STRING("mqttTlsCM"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(mqtt);
    ck_assert_ptr_nonnull(el);
    UA_CertificateGroup certificateGroup = {0};
    certificateGroup.verifyCertificate = verifyCertificate;
    mqtt->certificateGroup = &certificateGroup;
    acceptCertificate = accept;
    certificateVerifyCalls = 0;
    ck_assert_uint_eq(el->registerEventSource(el, &mqtt->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_String address = {strlen(addressString),
                         (UA_Byte*)(uintptr_t)addressString};
    UA_String topic = UA_STRING("open62541/tls");
    UA_Boolean useSSL = true;
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_KeyValuePair pairs[6] = {0};
    pairs[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&pairs[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    pairs[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&pairs[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    pairs[2].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&pairs[2].value, &topic, &UA_TYPES[UA_TYPES_STRING]);
    pairs[3].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&pairs[3].value, &useSSL, &UA_TYPES[UA_TYPES_BOOLEAN]);
    size_t paramsSize = 4;
    if(withClientCertificate) {
        certificate = loadFile("server_cert.der");
        privateKey = loadFile("server_key.der");
        ck_assert_ptr_nonnull(certificate.data);
        ck_assert_ptr_nonnull(privateKey.data);
        pairs[4].key = UA_QUALIFIEDNAME(0, "certificate");
        UA_Variant_setScalar(&pairs[4].value, &certificate,
                             &UA_TYPES[UA_TYPES_BYTESTRING]);
        pairs[5].key = UA_QUALIFIEDNAME(0, "private-key");
        UA_Variant_setScalar(&pairs[5].value, &privateKey,
                             &UA_TYPES[UA_TYPES_BYTESTRING]);
        paramsSize = 6;
    }
    UA_KeyValueMap params = {paramsSize, pairs};
    ConnectionContext connection = {0};
    ck_assert_uint_eq(mqtt->openConnection(mqtt, &params, NULL, &connection,
                                           connectionCallback),
                      UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    if(expectEstablished)
        runUntil(el, connectionEstablished, &connection);
    else
        runUntil(el, connectionClosed, &connection);

    if(expectEstablished || !accept)
        ck_assert_uint_gt(certificateVerifyCalls, 0);
    ck_assert_uint_eq(connection.established, expectEstablished);
    el->stop(el);
    runUntil(el, eventLoopStopped, el);
    el->free(el);
}

START_TEST(connectSubscribePublish) {
    certificateGroupCleared = false;
    const char *portString = getenv("OPEN62541_TEST_MQTT_PORT");
    ck_assert_ptr_nonnull(portString);
    UA_UInt16 port = (UA_UInt16)strtoul(portString, NULL, 10);

    UA_ConnectionManager *mqtt =
        UA_ConnectionManager_new_LWS_MQTT(UA_STRING("mqttLwsCM"));
    ck_assert_ptr_nonnull(mqtt);
    mqtt->certificateGroup =
        (UA_CertificateGroup*)UA_calloc(1, sizeof(UA_CertificateGroup));
    ck_assert_ptr_nonnull(mqtt->certificateGroup);
    mqtt->certificateGroup->clear = clearCertificateGroup;
    mqtt->certificateGroupOwned = true;
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
    ck_assert(certificateGroupCleared);
}
END_TEST

START_TEST(connectTlsCertificateAccepted) {
    runTlsConnection("OPEN62541_TEST_MQTTS_PORT", "127.0.0.1",
                     true, false, true);
}
END_TEST

START_TEST(connectTlsCertificateRejected) {
    runTlsConnection("OPEN62541_TEST_MQTTS_PORT", "127.0.0.1",
                     false, false, false);
}
END_TEST

START_TEST(connectTlsHostnameMismatch) {
    runTlsConnection("OPEN62541_TEST_MQTTS_PORT", "localhost",
                     true, false, false);
}
END_TEST

START_TEST(connectMutualTls) {
    runTlsConnection("OPEN62541_TEST_MQTTS_MTLS_PORT", "127.0.0.1",
                     true, true, true);
}
END_TEST

START_TEST(connectMutualTlsMissingCertificate) {
    runTlsConnection("OPEN62541_TEST_MQTTS_MTLS_PORT", "127.0.0.1",
                     true, false, false);
}
END_TEST

int
main(void) {
    Suite *suite = suite_create("LWS MQTT ConnectionManager");
    TCase *testCase = tcase_create("integration");
    tcase_add_test(testCase, connectSubscribePublish);
    tcase_add_test(testCase, connectTlsCertificateAccepted);
    tcase_add_test(testCase, connectTlsCertificateRejected);
    tcase_add_test(testCase, connectTlsHostnameMismatch);
    tcase_add_test(testCase, connectMutualTls);
    tcase_add_test(testCase, connectMutualTlsMissingCertificate);
    suite_add_tcase(suite, testCase);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
