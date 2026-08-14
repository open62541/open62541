/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_types_encoding_binary.h"
#include "ua_server_internal.h"
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_subscriptions.h>
#include "client/ua_client_internal.h"
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    size_t callbacks;
    UA_StatusCode status;
} AsyncReadResult;

typedef struct {
    size_t notifications;
} DataChangeResult;

static void
asyncReadResultCallback(UA_Client *client, void *userdata,
                        UA_UInt32 requestId, UA_ReadResponse *response) {
    (void)client;
    (void)requestId;
    AsyncReadResult *result = (AsyncReadResult *)userdata;
    result->callbacks++;
    result->status = response->responseHeader.serviceResult;
}

static void
dataChangeResultCallback(UA_Client *client, UA_UInt32 subscriptionId,
                         void *subscriptionContext, UA_UInt32 monitoredItemId,
                         void *monitoredItemContext, UA_DataValue *value) {
    (void)client;
    (void)subscriptionId;
    (void)subscriptionContext;
    (void)monitoredItemId;
    (void)value;
    DataChangeResult *result = (DataChangeResult *)monitoredItemContext;
    result->notifications++;
}

static size_t
countPendingResponses(UA_Client *client, const UA_DataType *responseType) {
    size_t count = 0;
    lockClient(client);
    AsyncServiceCall *ac;
    LIST_FOREACH(ac, &client->asyncServiceCalls, pointers) {
        if(ac->responseType == responseType)
            count++;
    }
    unlockClient(client);
    return count;
}

static void
addSyntheticReadCall(UA_Client *client, UA_UInt32 requestId,
                     AsyncReadResult *result) {
    AsyncServiceCall *ac = (AsyncServiceCall *)UA_calloc(1, sizeof(*ac));
    ck_assert_ptr_nonnull(ac);
    ac->callback = (UA_ClientAsyncServiceCallback)asyncReadResultCallback;
    ac->userdata = result;
    ac->responseType = &UA_TYPES[UA_TYPES_READRESPONSE];
    ac->requestId = requestId;
    ac->timeout = 1000;
    lockClient(client);
    LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);
    unlockClient(client);
}

static void
injectHttpClientResponseStatus(UA_Client *client, UA_UInt32 requestId,
                               UA_UInt16 statusCode,
                               const UA_String *contentType,
                               UA_ByteString body,
                               UA_Boolean includeRequestStatus) {
    UA_Boolean complete = true;
    UA_StatusCode requestStatus = UA_STATUSCODE_GOOD;
    UA_KeyValuePair header = {UA_QUALIFIEDNAME(0, "content-type"), {0}};
    if(contentType)
        UA_Variant_setScalar(&header.value, (void *)(uintptr_t)contentType,
                             &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValuePair params[5] = {0};
    params[0].key = UA_QUALIFIEDNAME(0, "status-code");
    UA_Variant_setScalar(&params[0].value, &statusCode,
                         &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "headers");
    UA_Variant_setArray(&params[1].value, &header, contentType ? 1 : 0,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    params[2].key = UA_QUALIFIEDNAME(0, "handle");
    UA_Variant_setScalar(&params[2].value, &requestId,
                         &UA_TYPES[UA_TYPES_UINT32]);
    params[3].key = UA_QUALIFIEDNAME(0, "response-complete");
    UA_Variant_setScalar(&params[3].value, &complete,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    size_t paramsSize = 4;
    if(includeRequestStatus) {
        params[4].key = UA_QUALIFIEDNAME(0, "request-status");
        UA_Variant_setScalar(&params[4].value, &requestStatus,
                             &UA_TYPES[UA_TYPES_STATUSCODE]);
        paramsSize++;
    }
    UA_KeyValueMap map = {paramsSize, params};
    void *connectionContext = &client->channel;
    __Client_httpConnectionCallback(
        client->channel.connectionManager, client->channel.connectionId,
        client, &connectionContext, UA_CONNECTIONSTATE_ESTABLISHED, &map,
        body);
}

static void
injectHttpClientResponse(UA_Client *client, UA_UInt32 requestId,
                         const UA_String *contentType, UA_ByteString body,
                         UA_Boolean includeRequestStatus) {
    injectHttpClientResponseStatus(client, requestId, 200, contentType, body,
                                   includeRequestStatus);
}

static void
injectHttpClientResponseHeaders(UA_Client *client, UA_UInt32 requestId,
                                UA_KeyValuePair *headers, size_t headersSize,
                                UA_ByteString body) {
    UA_UInt16 statusCode = 200;
    UA_Boolean complete = true;
    UA_StatusCode requestStatus = UA_STATUSCODE_GOOD;
    UA_KeyValuePair params[5] = {0};
    params[0].key = UA_QUALIFIEDNAME(0, "status-code");
    UA_Variant_setScalar(&params[0].value, &statusCode,
                         &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "headers");
    UA_Variant_setArray(&params[1].value, headers, headersSize,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    params[2].key = UA_QUALIFIEDNAME(0, "handle");
    UA_Variant_setScalar(&params[2].value, &requestId,
                         &UA_TYPES[UA_TYPES_UINT32]);
    params[3].key = UA_QUALIFIEDNAME(0, "response-complete");
    UA_Variant_setScalar(&params[3].value, &complete,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[4].key = UA_QUALIFIEDNAME(0, "request-status");
    UA_Variant_setScalar(&params[4].value, &requestStatus,
                         &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_KeyValueMap map = {5, params};
    void *connectionContext = &client->channel;
    __Client_httpConnectionCallback(
        client->channel.connectionManager, client->channel.connectionId,
        client, &connectionContext, UA_CONNECTIONSTATE_ESTABLISHED, &map,
        body);
}

static UA_ByteString
encodeServiceResponse(const void *response, const UA_DataType *responseType) {
    size_t size = UA_calcSizeBinary(
        &responseType->binaryEncodingId, &UA_TYPES[UA_TYPES_NODEID], NULL) +
        UA_calcSizeBinary(response, responseType, NULL);
    UA_ByteString body;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&body, size),
                      UA_STATUSCODE_GOOD);
    UA_Byte *pos = body.data;
    const UA_Byte *end = &body.data[body.length];
    ck_assert_uint_eq(UA_encodeBinaryInternal(
                          &responseType->binaryEncodingId,
                          &UA_TYPES[UA_TYPES_NODEID],
                          &pos, &end, NULL, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_encodeBinaryInternal(
                          response, responseType, &pos, &end,
                          NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    return body;
}

static UA_UInt16
getAdvertisedPort(UA_ServerConfig *config, const char *prefix,
                  size_t prefixLength, UA_String *advertisedUrl) {
    for(size_t i = 0; i < config->applicationDescription.discoveryUrlsSize;
        i++) {
        const UA_String *url = &config->applicationDescription.discoveryUrls[i];
        if(url->length < prefixLength ||
           memcmp(url->data, prefix, prefixLength) != 0)
            continue;
        UA_String hostname = UA_STRING_NULL;
        UA_UInt16 port = 0;
        ck_assert_uint_eq(UA_parseEndpointUrl(url, &hostname, &port, NULL),
                          UA_STATUSCODE_GOOD);
        if(advertisedUrl)
            ck_assert_uint_eq(UA_String_copy(url, advertisedUrl),
                              UA_STATUSCODE_GOOD);
        return port;
    }
    return 0;
}

typedef enum {
    HTTP_CLIENT_SCENARIO_SECURITY,
    HTTP_CLIENT_SCENARIO_SERVICES,
    HTTP_CLIENT_SCENARIO_ENDPOINT,
    HTTP_CLIENT_SCENARIO_JSON
} HttpClientScenario;

static void
runClientScenario(HttpClientScenario scenario) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *serverConfig = UA_Server_getConfig(server);
    serverConfig->tcpEnabled = false;
    serverConfig->httpEnabled = true;
    serverConfig->httpAllowUnencrypted = true;
    serverConfig->httpListenAddress = UA_STRING_ALLOC("127.0.0.1");
    UA_Array_delete(serverConfig->serverUrls, serverConfig->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    serverConfig->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(serverConfig->serverUrls);
    serverConfig->serverUrlsSize = 1;
    serverConfig->serverUrls[0] =
        UA_STRING_ALLOC("opc.http://127.0.0.1:0/client");

    UA_ByteString largeValue = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&largeValue, 16 * 1024),
                      UA_STATUSCODE_GOOD);
    memset(largeValue.data, 0x5a, largeValue.length);
    UA_VariableAttributes largeAttributes = UA_VariableAttributes_default;
    largeAttributes.displayName =
        UA_LOCALIZEDTEXT_ALLOC("en-US", "Large HTTP value");
    ck_assert_uint_eq(UA_Variant_setScalarCopy(
                          &largeAttributes.value, &largeValue,
                          &UA_TYPES[UA_TYPES_BYTESTRING]),
                      UA_STATUSCODE_GOOD);
    const UA_NodeId largeValueNode = UA_NODEID_NUMERIC(1, 6001);
    ck_assert_uint_eq(UA_Server_addVariableNode(
                          server, largeValueNode,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                          UA_QUALIFIEDNAME(1, "LargeHttpValue"),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                          largeAttributes, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    UA_VariableAttributes_clear(&largeAttributes);
    UA_ByteString_clear(&largeValue);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    UA_String endpointUrl = UA_STRING_NULL;
    ck_assert_uint_ne(getAdvertisedPort(serverConfig, "opc.http://", 11,
                                        &endpointUrl), 0);
    char *endpoint = (char *)UA_malloc(endpointUrl.length + 1);
    ck_assert_ptr_nonnull(endpoint);
    memcpy(endpoint, endpointUrl.data, endpointUrl.length);
    endpoint[endpointUrl.length] = 0;

    UA_ClientConfig clientConfig;
    UA_Client *client = NULL;
    UA_Variant value;

    if(scenario == HTTP_CLIENT_SCENARIO_SECURITY) {
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, endpoint),
                      UA_STATUSCODE_BADSECURITYPOLICYREJECTED);
    UA_Client_delete(client);

    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    clientConfig.httpAllowUnencrypted = true;
    clientConfig.securityMode = UA_MESSAGESECURITYMODE_SIGN;
    clientConfig.securityPolicyUri = UA_STRING_ALLOC(
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, endpoint),
                      UA_STATUSCODE_BADSECURITYPOLICYREJECTED);
    UA_Client_delete(client);

    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    clientConfig.httpAllowUnencrypted = true;
    clientConfig.httpClientCertificate = UA_BYTESTRING_ALLOC("certificate");
    client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, endpoint),
                      UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_Client_delete(client);

    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    clientConfig.httpAllowUnencrypted = true;
    ck_assert_uint_eq(UA_ClientConfig_setAuthenticationUsername(
                          &clientConfig, "user", "password"),
                      UA_STATUSCODE_GOOD);
    client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, endpoint),
                      UA_STATUSCODE_BADIDENTITYTOKENREJECTED);
    UA_Client_delete(client);
    }

    if(scenario == HTTP_CLIENT_SCENARIO_SERVICES) {
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    clientConfig.httpAllowUnencrypted = true;
    clientConfig.httpMaxMsgSize = 32 * 1024;
    clientConfig.httpMaxDecompressedMsgSize = 32 * 1024;
    clientConfig.outStandingPublishRequests = 3;
    client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, endpoint), UA_STATUSCODE_GOOD);

    UA_MessageSecurityMode mode = UA_MESSAGESECURITYMODE_INVALID;
    UA_QualifiedName modeKey = UA_QUALIFIEDNAME(0, "securityMode");
    ck_assert_uint_eq(UA_Client_getConnectionAttribute_scalar(
                          client, modeKey,
                          &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE], &mode),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(mode, UA_MESSAGESECURITYMODE_NONE);

    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Client_readValueAttribute(
                          client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE),
                          &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_isScalar(&value));
    UA_Variant_clear(&value);

    /* The shared RequestId generator skips zero when its 32-bit counter wraps. */
    lockClient(client);
    client->requestId = UA_UINT32_MAX;
    unlockClient(client);
    UA_ReadRequest rolloverRequest;
    UA_ReadRequest_init(&rolloverRequest);
    rolloverRequest.requestHeader.timeoutHint = 1000;
    AsyncReadResult rolloverResult = {0};
    UA_UInt32 rolloverRequestId = 0;
    ck_assert_uint_eq(
        UA_Client_sendAsyncReadRequest(client, &rolloverRequest,
                                       asyncReadResultCallback, &rolloverResult,
                                       &rolloverRequestId),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolloverRequestId, 1);
    for(size_t i = 0; i < 100 && rolloverResult.callbacks == 0; i++)
        ck_assert_uint_eq(UA_Client_run_iterate(client, 10),
                          UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolloverResult.callbacks, 1);
    ck_assert_uint_eq(rolloverResult.status, UA_STATUSCODE_BADNOTHINGTODO);

    /* OPC UA Binary uses identity content coding. Its encoded response therefore
     * has to fit the ordinary HTTP message limit. */
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Client_readValueAttribute(
                          client, largeValueNode, &value),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value,
                                      &UA_TYPES[UA_TYPES_BYTESTRING]));
    ck_assert_uint_eq(((UA_ByteString *)value.data)->length, 16 * 1024);
    UA_Variant_clear(&value);

    UA_CreateSubscriptionRequest subscriptionRequest =
        UA_CreateSubscriptionRequest_default();
    subscriptionRequest.requestedPublishingInterval = 100.0;
    subscriptionRequest.requestedMaxKeepAliveCount = 100;
    subscriptionRequest.requestedLifetimeCount = 1000;
    UA_CreateSubscriptionResponse subscriptionResponse =
        UA_Client_Subscriptions_create(client, subscriptionRequest,
                                       NULL, NULL, NULL);
    ck_assert_uint_eq(subscriptionResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(subscriptionResponse.subscriptionId, 0);

    DataChangeResult dataChanges = {0};
    UA_MonitoredItemCreateRequest monitoredItemRequest =
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    monitoredItemRequest.requestedParameters.samplingInterval = 50.0;
    UA_MonitoredItemCreateResult monitoredItemResult =
        UA_Client_MonitoredItems_createDataChange(
            client, subscriptionResponse.subscriptionId,
            UA_TIMESTAMPSTORETURN_BOTH, monitoredItemRequest, &dataChanges,
            dataChangeResultCallback, NULL);
    ck_assert_uint_eq(monitoredItemResult.statusCode, UA_STATUSCODE_GOOD);
    UA_MonitoredItemCreateResult_clear(&monitoredItemResult);

    /* Publish is represented by concurrent, long-standing HTTP requests.
     * Establish all configured requests and verify that an ordinary service
     * can complete without waiting for them. */
    for(size_t i = 0; i < 100 &&
        countPendingResponses(client, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]) < 3;
        i++)
        ck_assert_uint_eq(UA_Client_run_iterate(client, 10),
                          UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        countPendingResponses(client, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]), 3);

    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Client_readValueAttribute(
                          client, UA_NODEID_NUMERIC(
                              0, UA_NS0ID_SERVER_SERVERSTATUS_STATE),
                          &value), UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
    ck_assert_uint_eq(
        countPendingResponses(client, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]), 3);

    /* One malformed request is a request-local failure. It must not close the
     * binding, Session or the three outstanding Publish requests. */
    UA_ReadRequest malformed;
    UA_ReadRequest_init(&malformed);
    malformed.requestHeader.timeoutHint = 1000;
    AsyncReadResult malformedResult = {0};
    UA_UInt32 malformedRequestId = 0;
    ck_assert_uint_eq(
        UA_Client_sendAsyncReadRequest(client, &malformed,
                                       asyncReadResultCallback,
                                       &malformedResult,
                                       &malformedRequestId),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(malformedRequestId, 0);
    for(size_t i = 0; i < 100 && malformedResult.callbacks == 0; i++)
        ck_assert_uint_eq(UA_Client_run_iterate(client, 10),
                          UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(malformedResult.callbacks, 1);
    ck_assert_uint_eq(malformedResult.status, UA_STATUSCODE_BADNOTHINGTODO);
    ck_assert_uint_eq(
        countPendingResponses(client, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]), 3);

    /* Malformed transport metadata and service payloads consume their
     * synthetic request exactly once and leave the Publish window untouched. */
    UA_ReadResponse truncatedResponse;
    UA_ReadResponse_init(&truncatedResponse);
    UA_ByteString malformedBody = encodeServiceResponse(
        &truncatedResponse, &UA_TYPES[UA_TYPES_READRESPONSE]);
    ck_assert_uint_gt(malformedBody.length, 1);
    malformedBody.length--;
    AsyncReadResult missingStatusResult = {0};
    addSyntheticReadCall(client, 0xf0000001, &missingStatusResult);
    injectHttpClientResponse(client, 0xf0000001,
                             &UA_HTTP_CONTENTTYPE_BINARY, malformedBody,
                             false);
    ck_assert_uint_eq(missingStatusResult.callbacks, 1);
    ck_assert_uint_eq(missingStatusResult.status,
                      UA_STATUSCODE_BADDECODINGERROR);
    injectHttpClientResponse(client, 0xf0000001,
                             &UA_HTTP_CONTENTTYPE_BINARY, malformedBody,
                             false);
    ck_assert_uint_eq(missingStatusResult.callbacks, 1);

    AsyncReadResult malformedBodyResult = {0};
    addSyntheticReadCall(client, 0xf0000002, &malformedBodyResult);
    injectHttpClientResponse(client, 0xf0000002,
                             &UA_HTTP_CONTENTTYPE_BINARY, malformedBody,
                             true);
    ck_assert_uint_eq(malformedBodyResult.callbacks, 1);
    ck_assert_uint_eq(malformedBodyResult.status,
                      UA_STATUSCODE_BADDECODINGERROR);
    UA_ByteString_clear(&malformedBody);

    UA_ReadResponse validReadResponse;
    UA_ReadResponse_init(&validReadResponse);
    UA_ByteString trailingBody = encodeServiceResponse(
        &validReadResponse, &UA_TYPES[UA_TYPES_READRESPONSE]);
    UA_Byte *trailingData = (UA_Byte *)UA_realloc(
        trailingBody.data, trailingBody.length + 1);
    ck_assert_ptr_nonnull(trailingData);
    trailingBody.data = trailingData;
    trailingBody.data[trailingBody.length++] = 0xff;
    AsyncReadResult trailingBodyResult = {0};
    addSyntheticReadCall(client, 0xf000000b, &trailingBodyResult);
    injectHttpClientResponse(client, 0xf000000b,
                             &UA_HTTP_CONTENTTYPE_BINARY, trailingBody, true);
    ck_assert_uint_eq(trailingBodyResult.callbacks, 1);
    ck_assert_uint_eq(trailingBodyResult.status,
                      UA_STATUSCODE_BADDECODINGERROR);
    UA_ByteString_clear(&trailingBody);
    UA_ReadResponse_clear(&validReadResponse);

    UA_String binaryType = UA_STRING_STATIC("application/octet-stream");
    UA_KeyValuePair duplicateTypeHeaders[2] = {0};
    duplicateTypeHeaders[0].key = UA_QUALIFIEDNAME(0, "content-type");
    duplicateTypeHeaders[1].key = UA_QUALIFIEDNAME(0, "Content-Type");
    UA_Variant_setScalar(&duplicateTypeHeaders[0].value, &binaryType,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&duplicateTypeHeaders[1].value, &binaryType,
                         &UA_TYPES[UA_TYPES_STRING]);
    AsyncReadResult duplicateTypeResult = {0};
    addSyntheticReadCall(client, 0xf000000c, &duplicateTypeResult);
    injectHttpClientResponseHeaders(client, 0xf000000c,
                                    duplicateTypeHeaders, 2,
                                    UA_BYTESTRING("body"));
    ck_assert_uint_eq(duplicateTypeResult.callbacks, 1);
    ck_assert_uint_eq(duplicateTypeResult.status,
                      UA_STATUSCODE_BADDECODINGERROR);

    AsyncReadResult missingTypeResult = {0};
    addSyntheticReadCall(client, 0xf0000006, &missingTypeResult);
    injectHttpClientResponse(client, 0xf0000006, NULL,
                             UA_BYTESTRING("body"), true);
    ck_assert_uint_eq(missingTypeResult.callbacks, 1);
    ck_assert_uint_eq(missingTypeResult.status,
                      UA_STATUSCODE_BADDECODINGERROR);

    AsyncReadResult wrongContentTypeResult = {0};
    addSyntheticReadCall(client, 0xf000000a, &wrongContentTypeResult);
    const UA_String jsonContentType = UA_STRING_STATIC("application/json");
    injectHttpClientResponse(client, 0xf000000a, &jsonContentType,
                             UA_BYTESTRING("body"), true);
    ck_assert_uint_eq(wrongContentTypeResult.callbacks, 1);
    ck_assert_uint_eq(wrongContentTypeResult.status,
                      UA_STATUSCODE_BADDECODINGERROR);

    AsyncReadResult emptyBodyResult = {0};
    addSyntheticReadCall(client, 0xf0000007, &emptyBodyResult);
    injectHttpClientResponse(client, 0xf0000007,
                             &UA_HTTP_CONTENTTYPE_BINARY,
                             UA_BYTESTRING_NULL, true);
    ck_assert_uint_eq(emptyBodyResult.callbacks, 1);
    ck_assert_uint_eq(emptyBodyResult.status,
                      UA_STATUSCODE_BADDECODINGERROR);

    AsyncReadResult tooLargeResult = {0};
    addSyntheticReadCall(client, 0xf0000008, &tooLargeResult);
    injectHttpClientResponseStatus(client, 0xf0000008, 413, NULL,
                                   UA_BYTESTRING_NULL, true);
    ck_assert_uint_eq(tooLargeResult.callbacks, 1);
    ck_assert_uint_eq(tooLargeResult.status,
                      UA_STATUSCODE_BADREQUESTTOOLARGE);

    AsyncReadResult serverErrorResult = {0};
    addSyntheticReadCall(client, 0xf0000009, &serverErrorResult);
    injectHttpClientResponseStatus(client, 0xf0000009, 500, NULL,
                                   UA_BYTESTRING_NULL, true);
    ck_assert_uint_eq(serverErrorResult.callbacks, 1);
    ck_assert_uint_eq(serverErrorResult.status,
                      UA_STATUSCODE_BADCOMMUNICATIONERROR);

    UA_WriteResponse wrongResponse;
    UA_WriteResponse_init(&wrongResponse);
    UA_ByteString wrongBody = encodeServiceResponse(
        &wrongResponse, &UA_TYPES[UA_TYPES_WRITERESPONSE]);
    AsyncReadResult wrongTypeResult = {0};
    addSyntheticReadCall(client, 0xf0000003, &wrongTypeResult);
    injectHttpClientResponse(client, 0xf0000003,
                             &UA_HTTP_CONTENTTYPE_BINARY, wrongBody, true);
    ck_assert_uint_eq(wrongTypeResult.callbacks, 1);
    ck_assert_uint_eq(wrongTypeResult.status,
                      UA_STATUSCODE_BADCOMMUNICATIONERROR);
    UA_ByteString_clear(&wrongBody);
    UA_WriteResponse_clear(&wrongResponse);

    UA_ServiceFault syntheticFault;
    UA_ServiceFault_init(&syntheticFault);
    syntheticFault.responseHeader.serviceResult =
        UA_STATUSCODE_BADSERVICEUNSUPPORTED;
    UA_ByteString faultBody = encodeServiceResponse(
        &syntheticFault, &UA_TYPES[UA_TYPES_SERVICEFAULT]);
    AsyncReadResult faultResult = {0};
    addSyntheticReadCall(client, 0xf0000004, &faultResult);
    injectHttpClientResponse(client, 0xf0000004,
                             &UA_HTTP_CONTENTTYPE_BINARY, faultBody, true);
    ck_assert_uint_eq(faultResult.callbacks, 1);
    ck_assert_uint_eq(faultResult.status,
                      UA_STATUSCODE_BADSERVICEUNSUPPORTED);
    UA_ByteString_clear(&faultBody);
    UA_ServiceFault_clear(&syntheticFault);
    ck_assert_uint_eq(
        countPendingResponses(client, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]), 3);

    /* Drop the shared HTTP binding while Publish requests are pending. The
     * client must reopen it, reactivate the same Session and replenish all
     * Publish requests without recreating the subscription. */
    UA_NodeId sessionTokenBeforeReconnect;
    UA_NodeId_init(&sessionTokenBeforeReconnect);
    uintptr_t bindingId = 0;
    UA_ConnectionManager *bindingManager = NULL;
    lockClient(client);
    ck_assert_uint_eq(UA_NodeId_copy(&client->authenticationToken,
                                     &sessionTokenBeforeReconnect),
                      UA_STATUSCODE_GOOD);
    bindingId = client->channel.connectionId;
    bindingManager = client->channel.connectionManager;
    unlockClient(client);
    ck_assert_ptr_nonnull(bindingManager);
    ck_assert_uint_eq(bindingManager->closeConnection(bindingManager,
                                                       bindingId),
                      UA_STATUSCODE_GOOD);

    UA_SecureChannelState channelState = UA_SECURECHANNELSTATE_CLOSED;
    UA_SessionState sessionState = UA_SESSIONSTATE_CLOSED;
    UA_StatusCode connectStatus = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < 400; i++) {
        ck_assert_uint_eq(UA_Client_run_iterate(client, 10),
                          UA_STATUSCODE_GOOD);
        UA_Client_getState(client, &channelState, &sessionState,
                           &connectStatus);
        if(channelState == UA_SECURECHANNELSTATE_OPEN &&
           sessionState == UA_SESSIONSTATE_ACTIVATED &&
           countPendingResponses(client,
                                 &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]) == 3)
            break;
    }
    ck_assert_uint_eq(connectStatus, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(channelState, UA_SECURECHANNELSTATE_OPEN);
    ck_assert_uint_eq(sessionState, UA_SESSIONSTATE_ACTIVATED);
    ck_assert_uint_eq(
        countPendingResponses(client, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]), 3);
    lockClient(client);
    ck_assert(UA_NodeId_equal(&client->authenticationToken,
                              &sessionTokenBeforeReconnect));
    unlockClient(client);
    UA_NodeId_clear(&sessionTokenBeforeReconnect);

    /* The initial monitored value must arrive through one of the replenished
     * long-poll requests. Afterwards the client immediately restores the
     * configured Publish window. */
    for(size_t i = 0; i < 200 && dataChanges.notifications == 0; i++)
        ck_assert_uint_eq(UA_Client_run_iterate(client, 10),
                          UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(dataChanges.notifications, 0);
    for(size_t i = 0; i < 100 &&
        countPendingResponses(client, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]) < 3;
        i++)
        ck_assert_uint_eq(UA_Client_run_iterate(client, 10),
                          UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        countPendingResponses(client, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]), 3);

    /* Explicit shutdown must cancel the outstanding long-poll requests rather
     * than waiting for their ten-minute Publish timeout. */
    ck_assert_uint_eq(UA_Client_disconnect(client), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        countPendingResponses(client, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]), 0);
    UA_CreateSubscriptionResponse_clear(&subscriptionResponse);
    size_t notificationsAfterDisconnect = dataChanges.notifications;
    UA_Client_delete(client);
    for(size_t i = 0; i < 10; i++)
        serverConfig->eventLoop->run(serverConfig->eventLoop, 1);
    ck_assert_uint_eq(dataChanges.notifications, notificationsAfterDisconnect);
    lockServer(server);
    ck_assert_uint_eq(server->sessionCount, 0);
    unlockServer(server);

    /* The same identity-encoded response is rejected by a smaller client wire
     * limit. The binding remains usable for subsequent requests. */
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    clientConfig.httpAllowUnencrypted = true;
    clientConfig.httpMaxMsgSize = 4096;
    clientConfig.httpMaxDecompressedMsgSize = 8192;
    client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, endpoint), UA_STATUSCODE_GOOD);
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Client_readValueAttribute(
                          client, largeValueNode, &value),
                      UA_STATUSCODE_BADRESPONSETOOLARGE);
    UA_Variant_clear(&value);
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Client_readValueAttribute(
                          client, UA_NODEID_NUMERIC(
                              0, UA_NS0ID_SERVER_SERVERSTATUS_STATE),
                          &value),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Client_disconnect(client), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
    }

    if(scenario == HTTP_CLIENT_SCENARIO_ENDPOINT) {
    /* An explicitly selected EndpointDescription takes precedence over the
     * initial URL. This exercises the same URL handoff used after discovery. */
    UA_EndpointDescription *availableEndpoints = NULL;
    size_t availableEndpointsSize = 0;
    lockServer(server);
    ck_assert_uint_eq(setCurrentEndpointsArray(
                          server, endpointUrl, NULL, 0, &availableEndpoints,
                          &availableEndpointsSize),
                      UA_STATUSCODE_GOOD);
    unlockServer(server);
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    clientConfig.httpAllowUnencrypted = true;
    const UA_String binaryProfile = UA_STRING_STATIC(
        "http://open62541.org/UA-Profile/Transport/http-uabinary");
    for(size_t i = 0; i < availableEndpointsSize; i++) {
        if(UA_String_equal(&availableEndpoints[i].transportProfileUri,
                           &binaryProfile)) {
            ck_assert_uint_eq(UA_EndpointDescription_copy(
                                  &availableEndpoints[i],
                                  &clientConfig.endpoint),
                              UA_STATUSCODE_GOOD);
            break;
        }
    }
    ck_assert_uint_gt(clientConfig.endpoint.endpointUrl.length, 0);
    UA_Array_delete(availableEndpoints, availableEndpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    char wrongEndpoint[256];
    int wrongLength = snprintf(wrongEndpoint, sizeof(wrongEndpoint),
                               "opc.http://127.0.0.1:%u/wrong-path",
                               (unsigned)getAdvertisedPort(
                                   serverConfig, "opc.http://", 11, NULL));
    ck_assert_int_gt(wrongLength, 0);
    ck_assert_int_lt(wrongLength, (int)sizeof(wrongEndpoint));
    client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, wrongEndpoint),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_disconnect(client), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
    }

#ifdef UA_ENABLE_JSON_ENCODING
    if(scenario == HTTP_CLIENT_SCENARIO_JSON) {
    char *jsonEndpoint = (char *)UA_malloc(endpointUrl.length + 6);
    ck_assert_ptr_nonnull(jsonEndpoint);
    memcpy(jsonEndpoint, endpointUrl.data, endpointUrl.length);
    memcpy(&jsonEndpoint[endpointUrl.length], "/json", 6);
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    clientConfig.httpAllowUnencrypted = true;
    client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, jsonEndpoint),
                      UA_STATUSCODE_GOOD);
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Client_readValueAttribute(
                          client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE),
                          &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_isScalar(&value));
    UA_Variant_clear(&value);
    AsyncReadResult malformedJsonResult = {0};
    addSyntheticReadCall(client, 0xf0000005, &malformedJsonResult);
    UA_ByteString malformedJson = UA_BYTESTRING("{");
    injectHttpClientResponse(client, 0xf0000005,
                             &UA_HTTP_CONTENTTYPE_JSON, malformedJson, true);
    ck_assert_uint_eq(malformedJsonResult.callbacks, 1);
    ck_assert_uint_eq(malformedJsonResult.status,
                      UA_STATUSCODE_BADDECODINGERROR);
    ck_assert_uint_eq(UA_Client_disconnect(client), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
    UA_free(jsonEndpoint);
    }
#else
    (void)scenario;
#endif

    UA_free(endpoint);
    UA_String_clear(&endpointUrl);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}

START_TEST(httpClientSecurityValidation) {
    runClientScenario(HTTP_CLIENT_SCENARIO_SECURITY);
}
END_TEST

START_TEST(httpClientServicesSubscriptionsAndReconnect) {
    runClientScenario(HTTP_CLIENT_SCENARIO_SERVICES);
}
END_TEST

START_TEST(httpClientEndpointSelection) {
    runClientScenario(HTTP_CLIENT_SCENARIO_ENDPOINT);
}
END_TEST

#ifdef UA_ENABLE_JSON_ENCODING
START_TEST(httpJsonClientServiceAndMalformedResponse) {
    runClientScenario(HTTP_CLIENT_SCENARIO_JSON);
}
END_TEST
#endif

int main(void) {
    Suite *suite = suite_create("OPC UA HTTP client");
    TCase *tc = tcase_create("transport and services");
    tcase_set_timeout(tc, 30);
    tcase_add_test(tc, httpClientSecurityValidation);
    tcase_add_test(tc, httpClientServicesSubscriptionsAndReconnect);
    tcase_add_test(tc, httpClientEndpointSelection);
#ifdef UA_ENABLE_JSON_ENCODING
    tcase_add_test(tc, httpJsonClientServiceAndMalformedResponse);
#endif
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
