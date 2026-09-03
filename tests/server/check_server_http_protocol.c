/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>

#include "testing_networklayers.h"
#include "ua_server_internal.h"
#include "ua_types_encoding_binary.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

static void *mockApplication;
static void *mockListenerContext;
static UA_ConnectionManager_connectionCallback mockCallback;
#define MOCK_LISTENERS_SIZE 2
static size_t mockListenersSize;
static void *mockListenerContexts[MOCK_LISTENERS_SIZE];
static uintptr_t mockListenerConnectionIds[MOCK_LISTENERS_SIZE];

static UA_UInt16 expectedHttpTimeout;
static UA_String expectedAcceptEncoding;
static size_t oversizedSendCount;
static UA_Boolean wideCarrierResponded;
static size_t httpServiceNotifications;

static void
captureHttpServiceNotification(UA_Server *server,
                               UA_ApplicationNotificationType type,
                               const UA_KeyValueMap payload) {
    (void)server;
    (void)type;
    const UA_UInt32 *requestId = (const UA_UInt32 *)UA_KeyValueMap_getScalar(
        &payload, UA_QUALIFIEDNAME(0, "request-id"),
        &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_nonnull(requestId);
    ck_assert_uint_eq(*requestId, 0);
    httpServiceNotifications++;
}

static UA_StatusCode
captureWideCarrierResponse(UA_ConnectionManager *cm, uintptr_t connectionId,
                           const UA_KeyValueMap *params,
                           UA_ByteString *buf) {
    (void)cm;
#if UINTPTR_MAX > UINT32_MAX
    ck_assert_uint_gt(connectionId, UINT32_MAX);
#else
    (void)connectionId;
#endif
    const UA_UInt16 *status = (const UA_UInt16 *)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "status-code"),
        &UA_TYPES[UA_TYPES_UINT16]);
    ck_assert_ptr_nonnull(status);
    ck_assert_uint_eq(*status, 200);
    ck_assert_ptr_nonnull(buf);
    ck_assert_uint_gt(buf->length, 0);
    UA_ByteString_clear(buf);
    wideCarrierResponded = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
rejectFirstOversizedResponse(UA_ConnectionManager *cm, uintptr_t connectionId,
                             const UA_KeyValueMap *params,
                             UA_ByteString *buf) {
    (void)cm;
    (void)connectionId;
    (void)params;
    oversizedSendCount++;
    if(oversizedSendCount == 1) {
        UA_ByteString_clear(buf);
        return UA_STATUSCODE_BADRESPONSETOOLARGE;
    }
    size_t offset = 0;
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    UA_StatusCode res = UA_decodeBinaryInternal(
        buf, &offset, &typeId, &UA_TYPES[UA_TYPES_NODEID], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(
        &typeId, &UA_TYPES[UA_TYPES_SERVICEFAULT].binaryEncodingId));
    UA_NodeId_clear(&typeId);
    UA_ServiceFault fault;
    UA_ServiceFault_init(&fault);
    res = UA_decodeBinaryInternal(buf, &offset, &fault,
                                  &UA_TYPES[UA_TYPES_SERVICEFAULT], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(offset, buf->length);
    ck_assert_uint_eq(fault.responseHeader.requestHandle, 321);
    ck_assert_uint_eq(fault.responseHeader.serviceResult,
                      UA_STATUSCODE_BADRESPONSETOOLARGE);
    UA_ServiceFault_clear(&fault);
    UA_ByteString_clear(buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
rejectAllOversizedResponses(UA_ConnectionManager *cm, uintptr_t connectionId,
                            const UA_KeyValueMap *params,
                            UA_ByteString *buf) {
    (void)cm;
    (void)connectionId;
    (void)params;
    oversizedSendCount++;
    UA_ByteString_clear(buf);
    return UA_STATUSCODE_BADRESPONSETOOLARGE;
}

static UA_StatusCode
captureHttpRequestMetadata(UA_ConnectionManager *cm, uintptr_t connectionId,
                           const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)cm;
    (void)connectionId;
    const UA_UInt16 *timeout = (const UA_UInt16 *)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "timeout"), &UA_TYPES[UA_TYPES_UINT16]);
    if(expectedHttpTimeout == 0)
        ck_assert_ptr_null(timeout);
    else {
        ck_assert_ptr_nonnull(timeout);
        ck_assert_uint_eq(*timeout, expectedHttpTimeout);
    }

    const UA_Variant *headers = UA_KeyValueMap_get(
        params, UA_QUALIFIEDNAME(0, "headers"));
    ck_assert_ptr_nonnull(headers);
    ck_assert(UA_Variant_hasArrayType(headers,
                                     &UA_TYPES[UA_TYPES_KEYVALUEPAIR]));
    UA_String acceptName = UA_STRING_STATIC("accept-encoding");
    const UA_KeyValuePair *array = (const UA_KeyValuePair *)headers->data;
    const UA_String *acceptEncoding = NULL;
    for(size_t i = 0; i < headers->arrayLength; i++) {
        if(UA_String_equal(&array[i].key.name, &acceptName)) {
            ck_assert(UA_Variant_hasScalarType(&array[i].value,
                                              &UA_TYPES[UA_TYPES_STRING]));
            acceptEncoding = (const UA_String *)array[i].value.data;
            break;
        }
    }
    ck_assert_ptr_nonnull(acceptEncoding);
    ck_assert(UA_String_equal(acceptEncoding, &expectedAcceptEncoding));
    UA_ByteString_clear(buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
mockHttpOpen(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
             void *application, void *context,
             UA_ConnectionManager_connectionCallback callback) {
    mockApplication = application;
    mockListenerContext = context;
    mockCallback = callback;
    uintptr_t connectionId = 0;
    UA_StatusCode res = TestConnectionManager_createConnection(
        cm, application, context, callback, &connectionId);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    if(mockListenersSize < MOCK_LISTENERS_SIZE) {
        mockListenerContexts[mockListenersSize] = context;
        mockListenerConnectionIds[mockListenersSize] = connectionId;
        mockListenersSize++;
    }

    const UA_UInt16 *configuredPort = (const UA_UInt16 *)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 listenPort = configuredPort && *configuredPort
                               ? *configuredPort : 4842;
    UA_KeyValuePair parameter = {0};
    parameter.key = UA_QUALIFIEDNAME(0, "listen-port");
    UA_Variant_setScalar(&parameter.value, &listenPort,
                         &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap callbackParams = {1, &parameter};
    return TestConnectionManager_inject(
        cm, connectionId, UA_CONNECTIONSTATE_OPENING, &callbackParams, NULL);
}

START_TEST(httpStartupWithoutProviderFails) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpAllowUnencrypted = true;
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.http://localhost:0/ua");

    /* Hide the default provider when libwebsockets is enabled. */
    const UA_String protocol = UA_STRING_STATIC("http");
    UA_ConnectionManager *provider =
        findConnectionManager(config->eventLoop, &protocol);
    UA_String providerProtocol = UA_STRING_NULL;
    if(provider) {
        const UA_String unavailable =
            UA_STRING_STATIC("http-unavailable");
        providerProtocol = provider->protocol;
        provider->protocol = unavailable;
    }

    ck_assert_uint_eq(UA_Server_run_startup(server),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    if(provider)
        provider->protocol = providerProtocol;
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(protocolManagerUsesGenericHttpProvider) {
    mockListenersSize = 0;
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpAllowUnencrypted = true;
    config->verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT;
    config->serviceNotificationCallback = captureHttpServiceNotification;
    httpServiceNotifications = 0;
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.http://localhost:0/ua");

    TestConnectionManager_CallbackOverloads overloads = {0};
    overloads.openConnection = mockHttpOpen;
    UA_ConnectionManager *mock = TestConnectionManager_new("http", &overloads);
    ck_assert_ptr_nonnull(mock);
    ck_assert_uint_eq(config->eventLoop->registerEventSource(
                          config->eventLoop, &mock->eventSource),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    const UA_String expected =
        UA_STRING_STATIC("opc.http://localhost:4842/ua");
    UA_Boolean found = false;
    for(size_t i = 0;
        i < config->applicationDescription.discoveryUrlsSize; i++) {
        if(UA_String_equal(&config->applicationDescription.discoveryUrls[i],
                           &expected))
            found = true;
    }
    ck_assert(found);

    /* Drive a complete service request through the generic provider API. */
    uintptr_t requestConnectionId = 0;
    ck_assert_uint_eq(TestConnectionManager_createConnection(
                          mock, mockApplication, mockListenerContext,
                          mockCallback, &requestConnectionId),
                      UA_STATUSCODE_GOOD);
    /* Fixed Part 6 Binary ExtensionObject vector. It is intentionally not
     * generated by the encoder under test. TypeId i=422 is FindServersRequest
     * and the embedded RequestHandle is 321. */
    static const UA_Byte requestBytes[] = {
        0x01,0x00,0xa6,0x01,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x41,
        0x01,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,
        0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
    };
    UA_ByteString requestBody = {
        sizeof(requestBytes), (UA_Byte *)(uintptr_t)requestBytes};

    UA_String method = UA_STRING("POST");
    /* Conventional HTTP clients append the discovery suffix. The response
     * still advertises the canonical /ua endpoint. */
    UA_String path = UA_STRING("/ua/discovery");
    UA_String contentType = UA_STRING("application/octet-stream");
    UA_KeyValuePair header = {UA_QUALIFIEDNAME(0, "content-type"), {0}};
    UA_Variant_setScalar(&header.value, &contentType,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValuePair requestParams[3] = {0};
    requestParams[0].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&requestParams[0].value, &method,
                         &UA_TYPES[UA_TYPES_STRING]);
    requestParams[1].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&requestParams[1].value, &path,
                         &UA_TYPES[UA_TYPES_STRING]);
    requestParams[2].key = UA_QUALIFIEDNAME(0, "headers");
    UA_Variant_setArray(&requestParams[2].value, &header, 1,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    UA_KeyValueMap requestMap = {3, requestParams};
#if UINTPTR_MAX > UINT32_MAX
    /* The accepted connection id is also the HTTP response token. Preserve
     * its full uintptr_t width through delayed service-response routing. */
    UA_StatusCode (*savedSendWithConnection)(UA_ConnectionManager *, uintptr_t,
                                             const UA_KeyValueMap *,
                                             UA_ByteString *) =
        mock->sendWithConnection;
    mock->sendWithConnection = captureWideCarrierResponse;
    wideCarrierResponded = false;
    void *wideContext = mockListenerContext;
    const uintptr_t wideConnectionId = (uintptr_t)UINT32_MAX + 1u;
    mockCallback(mock, wideConnectionId, mockApplication,
                 &wideContext, UA_CONNECTIONSTATE_ESTABLISHED,
                 &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    mockCallback(mock, wideConnectionId, mockApplication,
                 &wideContext, UA_CONNECTIONSTATE_ESTABLISHED, &requestMap,
                 requestBody);
    ck_assert(wideCarrierResponded);
    ck_assert_ptr_ne(wideContext, mockListenerContext);
    mockCallback(mock, wideConnectionId, mockApplication, &wideContext,
                 UA_CONNECTIONSTATE_CLOSING, NULL, UA_BYTESTRING_NULL);
    mock->sendWithConnection = savedSendWithConnection;
#endif
    ck_assert_uint_eq(TestConnectionManager_inject(
                          mock, requestConnectionId,
                          UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_inject(
                          mock, requestConnectionId,
                          UA_CONNECTIONSTATE_ESTABLISHED, &requestMap,
                          &requestBody),
                      UA_STATUSCODE_GOOD);

    const UA_ByteString *sent = TestConnectionManager_getLastSent(mock);
    ck_assert_ptr_nonnull(sent);
    ck_assert_uint_gt(sent->length, 0);
    size_t offset = 0;
    UA_NodeId responseTypeId;
    UA_NodeId_init(&responseTypeId);
    ck_assert_uint_eq(UA_decodeBinaryInternal(
                          sent, &offset, &responseTypeId,
                          &UA_TYPES[UA_TYPES_NODEID], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(
        &responseTypeId,
        &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE].binaryEncodingId));
    UA_NodeId_clear(&responseTypeId);
    UA_FindServersResponse response;
    UA_FindServersResponse_init(&response);
    ck_assert_uint_eq(UA_decodeBinaryInternal(
                          sent, &offset, &response,
                          &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(offset, sent->length);
    ck_assert_uint_eq(response.responseHeader.requestHandle, 321);
    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_FindServersResponse_clear(&response);
    ck_assert_uint_gt(httpServiceNotifications, 0);
    ck_assert_uint_eq(TestConnectionManager_inject(
                          mock, requestConnectionId,
                          UA_CONNECTIONSTATE_CLOSING, NULL, NULL),
                      UA_STATUSCODE_GOOD);

    /* A response rejected as oversized is retried as a ServiceFault through
     * the same carrier mapping. */
    uintptr_t oversizedConnectionId = 0;
    ck_assert_uint_eq(TestConnectionManager_createConnection(
                          mock, mockApplication, mockListenerContext,
                          mockCallback, &oversizedConnectionId),
                      UA_STATUSCODE_GOOD);
    UA_StatusCode (*originalSendWithConnection)(UA_ConnectionManager *, uintptr_t,
                                                const UA_KeyValueMap *,
                                                UA_ByteString *) =
        mock->sendWithConnection;
    mock->sendWithConnection = rejectFirstOversizedResponse;
    oversizedSendCount = 0;
    ck_assert_uint_eq(TestConnectionManager_inject(
                          mock, oversizedConnectionId,
                          UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_inject(
                          mock, oversizedConnectionId,
                          UA_CONNECTIONSTATE_ESTABLISHED, &requestMap,
                          &requestBody),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(oversizedSendCount, 2);
    mock->sendWithConnection = originalSendWithConnection;
    ck_assert_uint_eq(TestConnectionManager_inject(
                          mock, oversizedConnectionId,
                          UA_CONNECTIONSTATE_CLOSING, NULL, NULL),
                      UA_STATUSCODE_GOOD);

    /* If both the service response and its fallback fail, the exchange is
     * released and the accepted carrier is closed instead of remaining busy. */
    uintptr_t failedConnectionId = 0;
    ck_assert_uint_eq(TestConnectionManager_createConnection(
                          mock, mockApplication, mockListenerContext,
                          mockCallback, &failedConnectionId),
                      UA_STATUSCODE_GOOD);
    mock->sendWithConnection = rejectAllOversizedResponses;
    oversizedSendCount = 0;
    ck_assert_uint_eq(TestConnectionManager_inject(
                          mock, failedConnectionId,
                          UA_CONNECTIONSTATE_ESTABLISHED,
                          &UA_KEYVALUEMAP_NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(TestConnectionManager_inject(
                          mock, failedConnectionId,
                          UA_CONNECTIONSTATE_ESTABLISHED, &requestMap,
                          &requestBody),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(oversizedSendCount, 2);
    ck_assert_uint_eq(TestConnectionManager_getCounters(
                          mock, failedConnectionId, NULL, NULL),
                      UA_STATUSCODE_BADNOTFOUND);
    mock->sendWithConnection = originalSendWithConnection;

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

static UA_Boolean
hasDiscoveryUrl(const UA_ServerConfig *config, const UA_String *url) {
    for(size_t i = 0; i < config->applicationDescription.discoveryUrlsSize;
        i++) {
        if(UA_String_equal(&config->applicationDescription.discoveryUrls[i],
                           url))
            return true;
    }
    return false;
}

static UA_SecureChannel *
newRegisteredHttpChannel(UA_Server *server, UA_ConnectionManager *cm,
                         uintptr_t connectionId) {
    UA_SecureChannel *channel =
        (UA_SecureChannel *)UA_calloc(1, sizeof(*channel));
    ck_assert_ptr_nonnull(channel);
    UA_SecureChannel_init(channel);
    channel->transport = UA_SECURECHANNEL_TRANSPORT_HTTP;
    channel->encoding = UA_SECURECHANNEL_ENCODING_BINARY;
    channel->state = UA_SECURECHANNELSTATE_OPEN;
    channel->connectionManager = cm;
    channel->connectionId = connectionId;
    channel->securityPolicy = &server->config.securityPolicies[0];
    lockServer(server);
    ck_assert_uint_eq(registerSecureChannel(server, channel),
                      UA_STATUSCODE_GOOD);
    unlockServer(server);
    return channel;
}

START_TEST(closingHttpListenerCleansOnlyItsChannels) {
    mockListenersSize = 0;
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpAllowUnencrypted = true;
    config->httpCertificate = UA_BYTESTRING_ALLOC("certificate");
    config->httpPrivateKey = UA_BYTESTRING_ALLOC("private-key");
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 2;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.http://localhost:0/plain");
    config->serverUrls[1] = UA_STRING_ALLOC("opc.https://localhost:0/secure");

    TestConnectionManager_CallbackOverloads overloads = {0};
    overloads.openConnection = mockHttpOpen;
    UA_ConnectionManager *mock = TestConnectionManager_new("http", &overloads);
    ck_assert_ptr_nonnull(mock);
    ck_assert_uint_eq(config->eventLoop->registerEventSource(
                          config->eventLoop, &mock->eventSource),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(mockListenersSize, 2);
    const UA_String plainUrl =
        UA_STRING_STATIC("opc.http://localhost:4842/plain");
    const UA_String secureUrl =
        UA_STRING_STATIC("opc.https://localhost:4842/secure");
    ck_assert(hasDiscoveryUrl(config, &plainUrl));
    ck_assert(hasDiscoveryUrl(config, &secureUrl));

    UA_SecureChannel *plainChannel = newRegisteredHttpChannel(
        server, mock, mockListenerConnectionIds[0]);
    UA_SecureChannel *secureChannel = newRegisteredHttpChannel(
        server, mock, mockListenerConnectionIds[1]);
    UA_UInt32 plainChannelId = plainChannel->securityToken.channelId;
    UA_UInt32 secureChannelId = secureChannel->securityToken.channelId;

    ck_assert_uint_eq(TestConnectionManager_inject(
                          mock, mockListenerConnectionIds[0],
                          UA_CONNECTIONSTATE_CLOSING, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert(!hasDiscoveryUrl(config, &plainUrl));
    ck_assert(hasDiscoveryUrl(config, &secureUrl));

    UA_Boolean foundPlain = false;
    UA_Boolean foundSecure = false;
    lockServer(server);
    UA_SecureChannel *channel;
    TAILQ_FOREACH(channel, &server->channels, serverEntry) {
        foundPlain |= channel->securityToken.channelId == plainChannelId;
        foundSecure |= channel->securityToken.channelId == secureChannelId;
    }
    unlockServer(server);
    ck_assert(!foundPlain);
    ck_assert(foundSecure);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

static void
assertHttpRequestMetadata(UA_UInt32 timeoutHint,
                          UA_SecureChannelEncoding encoding,
                          UA_UInt16 expectedTimeout,
                          const char *acceptEncoding) {
    TestConnectionManager_CallbackOverloads overloads = {0};
    overloads.sendWithConnection = captureHttpRequestMetadata;
    UA_ConnectionManager *cm = TestConnectionManager_new("http", &overloads);
    ck_assert_ptr_nonnull(cm);

    UA_SecurityPolicy policy;
    memset(&policy, 0, sizeof(policy));
    policy.policyUri = UA_SECURITY_POLICY_NONE_URI;
    UA_SecureChannel channel;
    UA_SecureChannel_init(&channel);
    channel.transport = UA_SECURECHANNEL_TRANSPORT_HTTP;
    channel.encoding = encoding;
    channel.state = UA_SECURECHANNELSTATE_OPEN;
    channel.connectionManager = cm;
    channel.connectionId = 1;
    channel.securityPolicy = &policy;

    expectedHttpTimeout = expectedTimeout;
    expectedAcceptEncoding = UA_STRING((char *)(uintptr_t)acceptEncoding);
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.requestHeader.timeoutHint = timeoutHint;
    ck_assert_uint_eq(UA_SecureChannel_sendMSG(
                          &channel, 1, &request,
                          &UA_TYPES[UA_TYPES_READREQUEST]),
                      UA_STATUSCODE_GOOD);

    channel.securityPolicy = NULL;
    UA_SecureChannel_clear(&channel);
    ck_assert_uint_eq(cm->eventSource.free(&cm->eventSource),
                      UA_STATUSCODE_GOOD);
}

START_TEST(httpRequestTimeoutAndEncodingMetadata) {
    assertHttpRequestMetadata(0, UA_SECURECHANNEL_ENCODING_BINARY, 0,
                              "identity");
    assertHttpRequestMetadata(1, UA_SECURECHANNEL_ENCODING_BINARY, 1,
                              "identity");
    assertHttpRequestMetadata(999, UA_SECURECHANNEL_ENCODING_BINARY, 1,
                              "identity");
    assertHttpRequestMetadata(1000, UA_SECURECHANNEL_ENCODING_BINARY, 1,
                              "identity");
    assertHttpRequestMetadata(1001, UA_SECURECHANNEL_ENCODING_BINARY, 2,
                              "identity");
#ifdef UA_ENABLE_JSON_ENCODING
# ifdef UA_ENABLE_HTTP_COMPRESSION
    assertHttpRequestMetadata(1000, UA_SECURECHANNEL_ENCODING_JSON, 1, "gzip");
# else
    assertHttpRequestMetadata(1000, UA_SECURECHANNEL_ENCODING_JSON, 1,
                              "identity");
# endif
#endif
}
END_TEST

static UA_StatusCode
rejectCertificate(UA_CertificateGroup *group,
                  const UA_ByteString *certificate) {
    (void)group;
    (void)certificate;
    return UA_STATUSCODE_BADCERTIFICATEUNTRUSTED;
}

static UA_StatusCode
acceptTrustList(UA_CertificateGroup *group, const UA_TrustListDataType *trustList) {
    (void)group;
    (void)trustList;
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean uascCloseCalled;

static UA_StatusCode
recordUascClose(UA_ConnectionManager *cm, uintptr_t connectionId) {
    (void)cm;
    (void)connectionId;
    uascCloseCalled = true;
    return UA_STATUSCODE_GOOD;
}

static void
prepareRegisteredChannel(UA_Server *server, UA_SecureChannel *channel,
                         UA_Boolean direct, UA_ConnectionManager *cm) {
    UA_SecureChannel_init(channel);
    channel->securityPolicy = &server->config.securityPolicies[0];
    channel->securityMode = UA_MESSAGESECURITYMODE_NONE;
    channel->state = UA_SECURECHANNELSTATE_OPEN;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&channel->remoteCertificate, 1),
                      UA_STATUSCODE_GOOD);
    channel->remoteCertificate.data[0] = 0x42;
    if(direct)
        channel->transport = UA_SECURECHANNEL_TRANSPORT_HTTP;
    else {
        channel->connectionManager = cm;
        channel->connectionId = 1;
    }
    ck_assert_uint_eq(registerSecureChannel(server, channel),
                      UA_STATUSCODE_GOOD);
}

START_TEST(trustUpdateClosesOpenUascChannel) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    UA_ConnectionManager cm;
    memset(&cm, 0, sizeof(cm));
    cm.closeConnection = recordUascClose;
    UA_SecureChannel uasc;
    lockServer(server);
    prepareRegisteredChannel(server, &uasc, false, &cm);
    unlockServer(server);

    uascCloseCalled = false;
    config->secureChannelPKI.setTrustList = acceptTrustList;
    config->secureChannelPKI.verifyCertificate = rejectCertificate;
    UA_NodeId group = UA_NODEID_NUMERIC(
        0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    ck_assert_uint_eq(UA_Server_addCertificates(
                          server, group, NULL, 0, NULL, 0, true, false),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 10 && !uascCloseCalled; i++)
        config->eventLoop->run(config->eventLoop, 1);
    ck_assert(uascCloseCalled);
    ck_assert_uint_eq(uasc.state, UA_SECURECHANNELSTATE_CLOSING);

    lockServer(server);
    unregisterSecureChannel(server, &uasc);
    unlockServer(server);
    UA_SecureChannel_clear(&uasc);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(uascLimitDoesNotPurgeDirectChannel) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    server->config.maxSecureChannels = 1;
    server->secureChannelStatistics.currentChannelCount = 1;

    UA_SecureChannel direct;
    lockServer(server);
    prepareRegisteredChannel(server, &direct, true, NULL);

    UA_ConnectionManager cm;
    memset(&cm, 0, sizeof(cm));
    UA_ConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    UA_SecureChannel *created = NULL;
    UA_StatusCode res = createServerSecureChannel(
        server, &connectionConfig, &cm, 1, NULL, &created);

    /* Clean up an unexpected UACP channel before reporting the failure. */
    if(created) {
        unregisterSecureChannel(server, created);
        server->secureChannelStatistics.currentChannelCount--;
        UA_SecureChannel_clear(created);
        UA_free(created);
    }
    unregisterSecureChannel(server, &direct);
    unlockServer(server);

    ck_assert_uint_eq(res, UA_STATUSCODE_BADOUTOFMEMORY);
    ck_assert_uint_eq(direct.state, UA_SECURECHANNELSTATE_OPEN);
    UA_SecureChannel_clear(&direct);
    server->secureChannelStatistics.currentChannelCount = 0;
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(uascZeroLimitIsUnlimited) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    server->config.maxSecureChannels = 0;

    UA_ConnectionManager cm;
    memset(&cm, 0, sizeof(cm));
    UA_ConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    UA_SecureChannel *created = NULL;

    lockServer(server);
    UA_StatusCode res = createServerSecureChannel(
        server, &connectionConfig, &cm, 1, NULL, &created);
    UA_Boolean channelCreated = (created != NULL);
    if(created) {
        unregisterSecureChannel(server, created);
        server->secureChannelStatistics.currentChannelCount--;
        UA_SecureChannel_clear(created);
        UA_free(created);
    }
    unlockServer(server);

    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(channelCreated);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(secureChannelAttribute_maxMessageSizeRoundtrips) {
    /* The reserved "0:maxMessageSize" attribute key is the one piece of
     * server-interpreted behavior in the otherwise-generic SecureChannel
     * attribute map: writing it caches the value into
     * channel->maxMessageSizeOverride for the per-chunk hot path. */
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);

    UA_SecureChannel channel;
    lockServer(server);
    prepareRegisteredChannel(server, &channel, true, NULL);
    UA_UInt32 channelId = channel.securityToken.channelId;
    unlockServer(server);

    ck_assert_uint_eq(channel.maxMessageSizeOverride, 0);

    UA_UInt32 limit = 65536;
    UA_Variant value;
    UA_Variant_setScalar(&value, &limit, &UA_TYPES[UA_TYPES_UINT32]);
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "maxMessageSize");
    ck_assert_uint_eq(
        UA_Server_setSecureChannelAttribute(server, channelId, key, &value),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(channel.maxMessageSizeOverride, limit);

    UA_Variant readBack;
    ck_assert_uint_eq(
        UA_Server_getSecureChannelAttribute(server, channelId, key, &readBack),
        UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&readBack, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(*(UA_UInt32*)readBack.data, limit);

    UA_UInt32 scalarOut = 0;
    ck_assert_uint_eq(
        UA_Server_getSecureChannelAttribute_scalar(
            server, channelId, key, &UA_TYPES[UA_TYPES_UINT32], &scalarOut),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(scalarOut, limit);

    ck_assert_uint_eq(
        UA_Server_deleteSecureChannelAttribute(server, channelId, key),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(channel.maxMessageSizeOverride, 0);
    ck_assert_uint_eq(
        UA_Server_getSecureChannelAttribute(server, channelId, key, &readBack),
        UA_STATUSCODE_BADNOTFOUND);

    lockServer(server);
    unregisterSecureChannel(server, &channel);
    unlockServer(server);
    UA_SecureChannel_clear(&channel);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(secureChannelAttribute_arbitraryKeyIsGenericStorage) {
    /* Any other key behaves as plain application-defined storage. */
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);

    UA_SecureChannel channel;
    lockServer(server);
    prepareRegisteredChannel(server, &channel, true, NULL);
    UA_UInt32 channelId = channel.securityToken.channelId;
    unlockServer(server);

    UA_String tag = UA_STRING("example-tag");
    UA_Variant value;
    UA_Variant_setScalar(&value, &tag, &UA_TYPES[UA_TYPES_STRING]);
    UA_QualifiedName key = UA_QUALIFIEDNAME(1, "application-tag");
    ck_assert_uint_eq(
        UA_Server_setSecureChannelAttribute(server, channelId, key, &value),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(channel.maxMessageSizeOverride, 0);

    UA_Variant readBack;
    ck_assert_uint_eq(
        UA_Server_getSecureChannelAttributeCopy(server, channelId, key, &readBack),
        UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal((UA_String*)readBack.data, &tag));
    UA_Variant_clear(&readBack);

    lockServer(server);
    unregisterSecureChannel(server, &channel);
    unlockServer(server);
    UA_SecureChannel_clear(&channel);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(secureChannelAttribute_wrongTypeForMaxMessageSizeRejected) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);

    UA_SecureChannel channel;
    lockServer(server);
    prepareRegisteredChannel(server, &channel, true, NULL);
    UA_UInt32 channelId = channel.securityToken.channelId;
    unlockServer(server);

    UA_String notANumber = UA_STRING("nope");
    UA_Variant value;
    UA_Variant_setScalar(&value, &notANumber, &UA_TYPES[UA_TYPES_STRING]);
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "maxMessageSize");
    ck_assert_uint_eq(
        UA_Server_setSecureChannelAttribute(server, channelId, key, &value),
        UA_STATUSCODE_BADTYPEMISMATCH);
    ck_assert_uint_eq(channel.maxMessageSizeOverride, 0);

    lockServer(server);
    unregisterSecureChannel(server, &channel);
    unlockServer(server);
    UA_SecureChannel_clear(&channel);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(secureChannelAttribute_unknownChannelIdRejected) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);

    UA_UInt32 limit = 1024;
    UA_Variant value;
    UA_Variant_setScalar(&value, &limit, &UA_TYPES[UA_TYPES_UINT32]);
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "maxMessageSize");
    ck_assert_uint_eq(
        UA_Server_setSecureChannelAttribute(server, 424242, key, &value),
        UA_STATUSCODE_BADNOTFOUND);

    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(mixedTransportChannelIdsAreUnique) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);

    UA_ConnectionManager cm;
    memset(&cm, 0, sizeof(cm));
    cm.closeConnection = recordUascClose;

    UA_SecureChannel directA, uasc, directB, wrapped;
    UA_SecureChannel_init(&directA);
    UA_SecureChannel_init(&uasc);
    UA_SecureChannel_init(&directB);
    UA_SecureChannel_init(&wrapped);
    UA_SecureChannel *directChannels[] = {&directA, &directB, &wrapped};
    for(size_t i = 0; i < 3; i++) {
        directChannels[i]->transport = UA_SECURECHANNEL_TRANSPORT_HTTP;
        directChannels[i]->state = UA_SECURECHANNELSTATE_OPEN;
    }
    uasc.connectionManager = &cm;
    uasc.connectionId = 1;
    uasc.state = UA_SECURECHANNELSTATE_OPEN;

    lockServer(server);
    ck_assert_uint_eq(registerSecureChannel(server, &directA),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(registerSecureChannel(server, &uasc),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(registerSecureChannel(server, &directB),
                      UA_STATUSCODE_GOOD);
    /* Exercise wraparound: skip zero and the three active identifiers. */
    server->nextChannelId = 0;
    ck_assert_uint_eq(registerSecureChannel(server, &wrapped),
                      UA_STATUSCODE_GOOD);
    unlockServer(server);

    UA_UInt32 directAId = directA.securityToken.channelId;
    UA_UInt32 uascId = uasc.securityToken.channelId;
    UA_UInt32 directBId = directB.securityToken.channelId;
    UA_UInt32 wrappedId = wrapped.securityToken.channelId;
    ck_assert_uint_ne(directAId, 0);
    ck_assert_uint_ne(uascId, 0);
    ck_assert_uint_ne(directBId, 0);
    ck_assert_uint_ne(wrappedId, 0);
    ck_assert_uint_ne(directAId, uascId);
    ck_assert_uint_ne(directAId, directBId);
    ck_assert_uint_ne(directAId, wrappedId);
    ck_assert_uint_ne(uascId, directBId);
    ck_assert_uint_ne(uascId, wrappedId);
    ck_assert_uint_ne(directBId, wrappedId);

    uascCloseCalled = false;
    ck_assert_uint_eq(UA_Server_closeSecureChannel(
                          server, uascId, UA_SHUTDOWNREASON_CLOSE),
                      UA_STATUSCODE_GOOD);
    ck_assert(uascCloseCalled);
    ck_assert_uint_eq(uasc.state, UA_SECURECHANNELSTATE_CLOSING);
    ck_assert_uint_eq(directA.state, UA_SECURECHANNELSTATE_OPEN);
    ck_assert_uint_eq(directB.state, UA_SECURECHANNELSTATE_OPEN);

    lockServer(server);
    unregisterSecureChannel(server, &directA);
    unregisterSecureChannel(server, &uasc);
    unregisterSecureChannel(server, &directB);
    unregisterSecureChannel(server, &wrapped);
    unlockServer(server);
    UA_SecureChannel_clear(&directA);
    UA_SecureChannel_clear(&uasc);
    UA_SecureChannel_clear(&directB);
    UA_SecureChannel_clear(&wrapped);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
}
END_TEST

static void
noopConnectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                       void *application, void **connectionContext,
                       UA_ConnectionState state,
                       const UA_KeyValueMap *params, UA_ByteString msg) {
    (void)cm;
    (void)connectionId;
    (void)application;
    (void)connectionContext;
    (void)state;
    (void)params;
    (void)msg;
}

static UA_StatusCode
sendOnlyWhileTracked(UA_ConnectionManager *cm, uintptr_t connectionId,
                     const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)params;
    UA_StatusCode res =
        TestConnectionManager_getCounters(cm, connectionId, NULL, NULL);
    UA_ByteString_clear(buf);
    return res == UA_STATUSCODE_GOOD ? UA_STATUSCODE_BADCONNECTIONCLOSED : res;
}

START_TEST(closingHttpChannelDrainsUntilCarrierCloses) {
    TestConnectionManager_CallbackOverloads overloads = {0};
    overloads.sendWithConnection = sendOnlyWhileTracked;
    UA_ConnectionManager *cm = TestConnectionManager_new("http", &overloads);
    ck_assert_ptr_nonnull(cm);
    uintptr_t requestId = 0;
    ck_assert_uint_eq(TestConnectionManager_createConnection(
                          cm, NULL, NULL, noopConnectionCallback, &requestId),
                      UA_STATUSCODE_GOOD);

    UA_SecureChannel channel;
    UA_SecureChannel_init(&channel);
    channel.transport = UA_SECURECHANNEL_TRANSPORT_HTTP;
    channel.encoding = UA_SECURECHANNEL_ENCODING_BINARY;
    channel.state = UA_SECURECHANNELSTATE_CLOSING;
    channel.connectionManager = cm;

    UA_ServiceFault response;
    UA_ServiceFault_init(&response);
    response.responseHeader.requestHandle = 7;
    ck_assert_uint_eq(UA_SecureChannel_sendHttpResponse(
                          &channel, requestId, &response,
                          &UA_TYPES[UA_TYPES_SERVICEFAULT]),
                      UA_STATUSCODE_BADCONNECTIONCLOSED);

    ck_assert_uint_eq(TestConnectionManager_inject(
                          cm, requestId, UA_CONNECTIONSTATE_CLOSING,
                          NULL, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_SecureChannel_sendHttpResponse(
                          &channel, requestId, &response,
                          &UA_TYPES[UA_TYPES_SERVICEFAULT]),
                      UA_STATUSCODE_BADNOTFOUND);
    UA_SecureChannel_clear(&channel);
    ck_assert_uint_eq(cm->eventSource.free(&cm->eventSource),
                      UA_STATUSCODE_GOOD);
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("HTTP protocol manager");
    TCase *tc = tcase_create("provider and lifecycle");
    tcase_add_test(tc, httpStartupWithoutProviderFails);
    tcase_add_test(tc, protocolManagerUsesGenericHttpProvider);
    tcase_add_test(tc, closingHttpListenerCleansOnlyItsChannels);
    tcase_add_test(tc, trustUpdateClosesOpenUascChannel);
    tcase_add_test(tc, uascLimitDoesNotPurgeDirectChannel);
    tcase_add_test(tc, uascZeroLimitIsUnlimited);
    tcase_add_test(tc, secureChannelAttribute_maxMessageSizeRoundtrips);
    tcase_add_test(tc, secureChannelAttribute_arbitraryKeyIsGenericStorage);
    tcase_add_test(tc, secureChannelAttribute_wrongTypeForMaxMessageSizeRejected);
    tcase_add_test(tc, secureChannelAttribute_unknownChannelIdRejected);
    tcase_add_test(tc, mixedTransportChannelIdsAreUnique);
    tcase_add_test(tc, closingHttpChannelDrainsUntilCarrierCloses);
    tcase_add_test(tc, httpRequestTimeoutAndEncodingMetadata);
    /* Full namespace initialization under Valgrind exceeds Check's default
     * four-second per-test timeout. */
    tcase_set_timeout(tc, 30);
    suite_add_tcase(suite, tc);
    return suite;
}

int main(void) {
    SRunner *runner = srunner_create(testSuite());
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
