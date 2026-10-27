/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "testing_networklayers.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

static UA_Server *server;
static UA_ConnectionManager *mock, *hiddenProvider;
static UA_String hiddenProtocol;
static UA_KeyValueMap captured;
static unsigned openCalls, callbackFields;
static UA_StatusCode openResult;
static UA_UInt16 rejectedPort;
static uintptr_t listenerId;

static const void *
capturedScalar(const char *key, const UA_DataType *type) {
    const void *value = UA_KeyValueMap_getScalar(&captured,
        UA_QUALIFIEDNAME(0, (char *)(uintptr_t)key), type);
    ck_assert_ptr_nonnull(value);
    return value;
}

static UA_StatusCode
mockWebSocketOpen(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                  void *application, void *context,
                  UA_ConnectionManager_connectionCallback callback) {
    openCalls++;
    UA_KeyValueMap_clear(&captured);
    ck_assert_uint_eq(UA_KeyValueMap_copy(params, &captured), UA_STATUSCODE_GOOD);
    const UA_UInt16 *port = (const UA_UInt16 *)capturedScalar("port", &UA_TYPES[UA_TYPES_UINT16]);
    if(openResult != UA_STATUSCODE_GOOD)
        return openResult;
    if(rejectedPort && *port == rejectedPort)
        return UA_STATUSCODE_BADCONNECTIONREJECTED;

    ck_assert_uint_eq(TestConnectionManager_createConnection(
        cm, application, context, callback, &listenerId), UA_STATUSCODE_GOOD);
    UA_UInt16 actualPort = *port ? *port : 4862;
    UA_String address = UA_STRING("192.0.2.15");
    UA_KeyValuePair fields[2];
    size_t size = 0;
    if(callbackFields & 1u) {
        fields[size].key = UA_QUALIFIEDNAME(0, "listen-port");
        UA_Variant_setScalar(&fields[size++].value, &actualPort, &UA_TYPES[UA_TYPES_UINT16]);
    }
    if(callbackFields & 2u) {
        fields[size].key = UA_QUALIFIEDNAME(0, "listen-address");
        UA_Variant_setScalar(&fields[size++].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    }
    UA_KeyValueMap callbackParams = {size, fields};
    return TestConnectionManager_inject(cm, listenerId, UA_CONNECTIONSTATE_ESTABLISHED,
                                         &callbackParams, NULL);
}

static void
setServerUrl(const char *url) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_Array_delete(config->serverUrls, config->serverUrlsSize, &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls = (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC(url);
    ck_assert_ptr_nonnull(config->serverUrls[0].data);
}

static void
setup(void) {
    server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = false;
    config->webSocketEnabled = true;
    config->webSocketAllowUnencrypted = true;
    config->webSocketBufSize = 16384;
    config->webSocketMaxMsgSize = 131072;
    config->webSocketMaxChunks = 8;
    config->webSocketMaxQueueSize = 32768;
    /* Opaque test values: this suite checks forwarding to the provider. */
    config->webSocketCertificate = UA_BYTESTRING_ALLOC("provider certificate");
    config->webSocketPrivateKey = UA_BYTESTRING_ALLOC("provider private key");
    setServerUrl("opc.ws://localhost:0/ua");

    /* Keep these protocol-manager tests independent of installed providers. */
    UA_String protocol = UA_STRING("websocket");
    hiddenProvider = findConnectionManager(config->eventLoop, &protocol);
    if(hiddenProvider) {
        hiddenProtocol = hiddenProvider->protocol;
        hiddenProvider->protocol = UA_STRING("websocket-hidden");
    }
    openCalls = 0;
    callbackFields = 3;
    openResult = UA_STATUSCODE_GOOD;
    rejectedPort = 0;
    listenerId = 0;
    TestConnectionManager_CallbackOverloads overloads = {0};
    overloads.openConnection = mockWebSocketOpen;
    mock = TestConnectionManager_new("websocket", &overloads);
    ck_assert_ptr_nonnull(mock);
    ck_assert_uint_eq(config->eventLoop->registerEventSource(
        config->eventLoop, &mock->eventSource), UA_STATUSCODE_GOOD);
}

static void
teardown(void) {
    if(UA_Server_getLifecycleState(server) == UA_LIFECYCLESTATE_STARTED)
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    if(hiddenProvider)
        hiddenProvider->protocol = hiddenProtocol;
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_clear(&captured);
}

static UA_Boolean
hasDiscoveryUrl(const char *url) {
    UA_String expected = UA_STRING((char *)(uintptr_t)url);
    UA_ApplicationDescription *description = &UA_Server_getConfig(server)->applicationDescription;
    for(size_t i = 0; i < description->discoveryUrlsSize; i++) {
        if(UA_String_equal(&description->discoveryUrls[i], &expected))
            return true;
    }
    return false;
}

static const struct {
    const char *url, *path, *host, *advertised;
    UA_UInt16 port;
    UA_Boolean secure, password;
} listenerCases[] = {
    {"opc.ws://localhost:0/ua", "/ua", "localhost", "opc.ws://localhost:4862/ua", 0, false, false},
    {"opc.ws://:0/ua", "/ua", NULL, "opc.ws://192.0.2.15:4862/ua", 0, false, false},
    {"opc.ws://example.test", "/", "example.test", "opc.ws://example.test:4840", 4840, false, false},
    {"opc.wss://example.test/ua", "/ua", "example.test", "opc.wss://example.test:443/ua", 443, true, false},
    {"opc.wss://:0", "/", NULL, "opc.wss://192.0.2.15:4862", 0, true, true}
};

START_TEST(webSocketProviderReceivesConfiguration) {
    setServerUrl(listenerCases[_i].url);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    if(listenerCases[_i].password)
        config->webSocketPrivateKeyPassword = UA_STRING_ALLOC("provider password");
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openCalls, 1);
    ck_assert_uint_eq(*(const UA_UInt16 *)capturedScalar("port", &UA_TYPES[UA_TYPES_UINT16]),
                      listenerCases[_i].port);
    ck_assert(*(const UA_Boolean *)capturedScalar("listen", &UA_TYPES[UA_TYPES_BOOLEAN]));
    ck_assert_uint_eq(*(const UA_Boolean *)capturedScalar("useSSL", &UA_TYPES[UA_TYPES_BOOLEAN]),
                      listenerCases[_i].secure);
    ck_assert(*(const UA_Boolean *)capturedScalar("binary-only", &UA_TYPES[UA_TYPES_BOOLEAN]));
    UA_String expected = UA_STRING((char *)(uintptr_t)listenerCases[_i].path);
    ck_assert(UA_String_equal((const UA_String *)capturedScalar("path", &UA_TYPES[UA_TYPES_STRING]),
                              &expected));
    expected = UA_STRING("opcua+uacp");
    ck_assert(UA_String_equal((const UA_String *)capturedScalar("subprotocol", &UA_TYPES[UA_TYPES_STRING]),
                              &expected));
    const char *limits[] = {"recv-max-message-size", "send-max-message-size", "send-max-queue-size"};
    for(size_t i = 0; i < 3; i++)
        ck_assert_uint_eq(*(const UA_UInt32 *)capturedScalar(limits[i], &UA_TYPES[UA_TYPES_UINT32]),
                          i == 2 ? config->webSocketMaxQueueSize : config->webSocketBufSize);
    const UA_String *host = (const UA_String *)UA_KeyValueMap_getScalar(
        &captured, UA_QUALIFIEDNAME(0, "address"), &UA_TYPES[UA_TYPES_STRING]);
    if(listenerCases[_i].host) {
        ck_assert_ptr_nonnull(host);
        expected = UA_STRING((char *)(uintptr_t)listenerCases[_i].host);
        ck_assert(UA_String_equal(host, &expected));
    } else {
        ck_assert_ptr_null(host);
    }
    if(listenerCases[_i].secure) {
        ck_assert(UA_ByteString_equal((const UA_ByteString *)capturedScalar(
            "certificate", &UA_TYPES[UA_TYPES_BYTESTRING]), &config->webSocketCertificate));
        ck_assert(UA_ByteString_equal((const UA_ByteString *)capturedScalar(
            "private-key", &UA_TYPES[UA_TYPES_BYTESTRING]), &config->webSocketPrivateKey));
    } else {
        ck_assert_ptr_null(UA_KeyValueMap_get(&captured, UA_QUALIFIEDNAME(0, "certificate")));
        ck_assert_ptr_null(UA_KeyValueMap_get(&captured, UA_QUALIFIEDNAME(0, "private-key")));
    }
    if(listenerCases[_i].password) {
        ck_assert(UA_String_equal((const UA_String *)capturedScalar(
            "private-key-password", &UA_TYPES[UA_TYPES_STRING]), &config->webSocketPrivateKeyPassword));
    } else {
        ck_assert_ptr_null(UA_KeyValueMap_get(&captured, UA_QUALIFIEDNAME(0, "private-key-password")));
    }
    ck_assert(hasDiscoveryUrl(listenerCases[_i].advertised));
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_getCounters(mock, listenerId, NULL, NULL),
                      UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(webSocketRejectsInvalidConfiguration) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode expected = UA_STATUSCODE_BADCONFIGURATIONERROR;
    switch(_i) {
    case 0:
        setServerUrl("opc.tcp://localhost:4840");
        break;
    case 1:
        mock->protocol = UA_STRING("unavailable");
        break;
    case 2:
        config->webSocketAllowUnencrypted = false;
        break;
    case 3:
        config->webSocketEncryptionMode = UA_WEBSOCKET_ENCRYPTION_REQUIRED;
        break;
    case 4:
        setServerUrl("opc.wss://localhost:443");
        config->webSocketEncryptionMode = UA_WEBSOCKET_ENCRYPTION_DISABLED;
        break;
    case 5:
        setServerUrl("opc.wss://localhost:443");
        UA_ByteString_clear(&config->webSocketCertificate);
        break;
    case 6:
        setServerUrl("opc.wss://localhost:443");
        UA_ByteString_clear(&config->webSocketPrivateKey);
        break;
    case 7:
        openResult = UA_STATUSCODE_BADCONNECTIONREJECTED;
        expected = UA_STATUSCODE_BADINTERNALERROR;
        break;
    default:
        setServerUrl("opc.ws://localhost:70000/ua");
        expected = UA_STATUSCODE_BADINTERNALERROR;
        break;
    }
    ck_assert_uint_eq(UA_Server_run_startup(server), expected);
    ck_assert_uint_eq(openCalls, _i == 7 ? 1 : 0);
    ck_assert_uint_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
} END_TEST

START_TEST(webSocketIncompleteListenerMetadata) {
    setServerUrl("opc.ws://:0/ua");
    callbackFields = (unsigned)_i;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openCalls, 1);
    ck_assert_uint_eq(UA_Server_getConfig(server)->applicationDescription.discoveryUrlsSize, 0);
} END_TEST

START_TEST(webSocketFailedListenerIsNotAdvertised) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_String urls[3] = {
        UA_STRING("opc.ws://first.test:4862/ua"),
        UA_STRING("opc.tcp://ignored.test:4840"),
        UA_STRING("opc.ws://second.test:4863/other")
    };
    UA_Array_delete(config->serverUrls, config->serverUrlsSize, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(UA_Array_copy(urls, 3, (void **)&config->serverUrls,
                                    &UA_TYPES[UA_TYPES_STRING]), UA_STATUSCODE_GOOD);
    config->serverUrlsSize = 3;
    rejectedPort = 4863;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(openCalls, 2);
    ck_assert(hasDiscoveryUrl("opc.ws://first.test:4862/ua"));
    ck_assert(!hasDiscoveryUrl("opc.tcp://ignored.test:4840"));
    ck_assert(!hasDiscoveryUrl("opc.ws://second.test:4863/other"));
} END_TEST

int main(void) {
    Suite *suite = suite_create("WebSocket protocol provider");
    TCase *tc = tcase_create("provider configuration");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_loop_test(tc, webSocketProviderReceivesConfiguration, 0, 5);
    tcase_add_loop_test(tc, webSocketRejectsInvalidConfiguration, 0, 9);
    tcase_add_loop_test(tc, webSocketIncompleteListenerMetadata, 0, 3);
    tcase_add_test(tc, webSocketFailedListenerIsNotAdvertised);
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
