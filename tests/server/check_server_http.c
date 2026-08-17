/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_types_encoding_binary.h"
#include "ua_server_internal.h"
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#ifdef UA_ENABLE_ENCRYPTION
# include <open62541/plugin/certificategroup_default.h>
# include <open62541/plugin/securitypolicy_default.h>
#endif
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#ifdef UA_ENABLE_HTTP_COMPRESSION
# include "../../arch/posix/eventloop_posix_http_compression.h"
#endif

#include <check.h>
#include <stdio.h>

static uintptr_t clientConnectionId;
static UA_ByteString responseBody;
static UA_UInt16 responseStatus;
static UA_Boolean responseIsJson;
static UA_Boolean responseIsGzip;
static UA_Boolean responseComplete;


static void clientCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                           void *application, void **connectionContext,
                           UA_ConnectionState state,
                           const UA_KeyValueMap *params, UA_ByteString msg);

static UA_ConnectionManager *
getHttpConnectionManager(UA_ServerConfig *config) {
    const UA_String protocol = UA_STRING_STATIC("http");
    for(UA_EventSource *es = config->eventLoop->eventSources; es;
        es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager *)es;
        if(UA_String_equal(&cm->protocol, &protocol))
            return cm;
    }
    return NULL;
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

static void
openRawHttpClient(UA_ConnectionManager *http, UA_UInt16 port,
                  UA_Boolean useSSL, const UA_ByteString *caCertificate) {
    UA_String address = UA_STRING("127.0.0.1");
    UA_UInt16 timeout = 10;
    UA_KeyValuePair params[5] = {0};
    size_t paramsSize = 0;
#define ADD_OPEN_PARAM(NAME, VALUE, TYPE)                                     \
    do {                                                                       \
        params[paramsSize].key = UA_QUALIFIEDNAME(0, NAME);                    \
        UA_Variant_setScalar(&params[paramsSize++].value, VALUE,               \
                             &UA_TYPES[TYPE]);                                  \
    } while(0)
    ADD_OPEN_PARAM("address", &address, UA_TYPES_STRING);
    ADD_OPEN_PARAM("port", &port, UA_TYPES_UINT16);
    ADD_OPEN_PARAM("timeout", &timeout, UA_TYPES_UINT16);
    ADD_OPEN_PARAM("useSSL", &useSSL, UA_TYPES_BOOLEAN);
    if(caCertificate)
        ADD_OPEN_PARAM("ca-certificate", (void *)(uintptr_t)caCertificate,
                       UA_TYPES_BYTESTRING);
#undef ADD_OPEN_PARAM
    UA_KeyValueMap map = {paramsSize, params};
    clientConnectionId = 0;
    ck_assert_uint_eq(http->openConnection(http, &map, NULL, NULL,
                                           clientCallback),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(clientConnectionId, 0);
}

static const UA_String *
getResponseHeader(const UA_KeyValueMap *params, const char *name) {
    const UA_Variant *v =
        UA_KeyValueMap_get(params, UA_QUALIFIEDNAME(0, "headers"));
    if(!v || !UA_Variant_hasArrayType(v, &UA_TYPES[UA_TYPES_KEYVALUEPAIR]))
        return NULL;
    UA_String headerName = UA_STRING((char *)(uintptr_t)name);
    const UA_KeyValuePair *headers = (const UA_KeyValuePair *)v->data;
    for(size_t i = 0; i < v->arrayLength; i++) {
        if(UA_String_equal(&headers[i].key.name, &headerName) &&
           UA_Variant_hasScalarType(&headers[i].value,
                                    &UA_TYPES[UA_TYPES_STRING]))
            return (const UA_String *)headers[i].value.data;
    }
    return NULL;
}

static UA_ByteString loadFile(const char *path) {
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

static void clientCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                           void *application, void **connectionContext,
                           UA_ConnectionState state,
                           const UA_KeyValueMap *params, UA_ByteString msg) {
    (void)cm;
    (void)application;
    (void)connectionContext;
    if(state == UA_CONNECTIONSTATE_OPENING) {
        clientConnectionId = connectionId;
        return;
    }
    if(state != UA_CONNECTIONSTATE_ESTABLISHED)
        return;
    const UA_Boolean *complete =
        (const UA_Boolean *)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "response-complete"),
            &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(complete && *complete)
        responseComplete = true;
    const UA_UInt16 *status = (const UA_UInt16 *)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "status-code"), &UA_TYPES[UA_TYPES_UINT16]);
    if(status)
        responseStatus = *status;
    const UA_String *contentType = getResponseHeader(params, "content-type");
    const UA_String json = UA_STRING_STATIC("application/json");
    if(contentType && UA_String_equal(contentType, &json))
        responseIsJson = true;
    const UA_String *contentEncoding =
        getResponseHeader(params, "content-encoding");
    const UA_String gzip = UA_STRING_STATIC("gzip");
    if(contentEncoding && UA_String_equal_ignorecase(contentEncoding, &gzip))
        responseIsGzip = true;
    if(msg.length == 0)
        return;
    size_t oldLength = responseBody.length;
    UA_Byte *data =
        (UA_Byte *)UA_realloc(responseBody.data, oldLength + msg.length);
    ck_assert_ptr_nonnull(data);
    responseBody.data = data;
    memcpy(&responseBody.data[oldLength], msg.data, msg.length);
    responseBody.length += msg.length;
}

static UA_ByteString
encodeServiceRequest(const void *request, const UA_DataType *requestType) {
    size_t size = UA_calcSizeBinary(
        &requestType->binaryEncodingId, &UA_TYPES[UA_TYPES_NODEID], NULL) +
        UA_calcSizeBinary(request, requestType, NULL);
    UA_ByteString body;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&body, size),
                      UA_STATUSCODE_GOOD);
    UA_Byte *pos = body.data;
    const UA_Byte *end = &body.data[body.length];
    ck_assert_uint_eq(UA_encodeBinaryInternal(
                          &requestType->binaryEncodingId,
                          &UA_TYPES[UA_TYPES_NODEID],
                          &pos, &end, NULL, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_encodeBinaryInternal(
                          request, requestType, &pos, &end,
                          NULL, NULL, NULL), UA_STATUSCODE_GOOD);
    return body;
}

#ifdef UA_ENABLE_JSON_ENCODING
static UA_ByteString
encodeServiceRequestJson(const void *request, const UA_DataType *requestType) {
    UA_ExtensionObject envelope;
    UA_ExtensionObject_setValueNoDelete(
        &envelope, (void *)(uintptr_t)request, requestType);
    UA_EncodeJsonOptions options;
    memset(&options, 0, sizeof(options));
    UA_ByteString body = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(
        UA_encodeJson(&envelope, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &body,
                      &options),
        UA_STATUSCODE_GOOD);
    return body;
}
#endif

static void
sendHttpPost(UA_ConnectionManager *http, UA_EventLoop *eventLoop,
             const UA_String *path, UA_KeyValuePair *headers,
             size_t headersSize, UA_ByteString *body,
             UA_Boolean expectResponseBody);

#ifdef UA_ENABLE_HTTP_COMPRESSION
static void
sendCodedServiceRequest(UA_ConnectionManager *http, UA_EventLoop *eventLoop,
                        const UA_String *path, const void *request,
                        const UA_DataType *requestType, UA_Boolean json,
                        UA_HTTPContentEncoding requestEncoding,
                        const char *acceptEncoding, UA_UInt16 expectedStatus) {
    UA_ByteString body;
#ifdef UA_ENABLE_JSON_ENCODING
    if(json)
        body = encodeServiceRequestJson(request, requestType);
    else
#else
    ck_assert(!json);
#endif
        body = encodeServiceRequest(request, requestType);

    UA_ByteString compressed = UA_BYTESTRING_NULL;
    if(requestEncoding != UA_HTTP_CONTENT_ENCODING_IDENTITY) {
        ck_assert_uint_eq(UA_HTTP_compress(requestEncoding, &body, &compressed),
                          UA_STATUSCODE_GOOD);
        UA_ByteString_clear(&body);
        body = compressed;
    }

    UA_String contentType = json ? UA_STRING("application/json") :
                                   UA_STRING("application/octet-stream");
    UA_String contentCoding = UA_STRING((char *)(uintptr_t)
        UA_HTTP_contentEncodingName(requestEncoding));
    UA_String accept = UA_STRING((char *)(uintptr_t)acceptEncoding);
    UA_KeyValuePair headers[3] = {0};
    headers[0].key = UA_QUALIFIEDNAME(0, "content-type");
    UA_Variant_setScalar(&headers[0].value, &contentType,
                         &UA_TYPES[UA_TYPES_STRING]);
    headers[1].key = UA_QUALIFIEDNAME(0, "content-encoding");
    UA_Variant_setScalar(&headers[1].value, &contentCoding,
                         &UA_TYPES[UA_TYPES_STRING]);
    headers[2].key = UA_QUALIFIEDNAME(0, "accept-encoding");
    UA_Variant_setScalar(&headers[2].value, &accept,
                         &UA_TYPES[UA_TYPES_STRING]);
    sendHttpPost(http, eventLoop, path, headers, 3, &body, false);
    ck_assert_uint_eq(responseStatus, expectedStatus);
    if(expectedStatus == 200)
        ck_assert_uint_gt(responseBody.length, 0);
}
#endif

static void
startHttpPost(UA_ConnectionManager *http, const UA_String *path,
              UA_KeyValuePair *headers, size_t headersSize,
              UA_ByteString *body) {
    UA_ByteString_clear(&responseBody);
    responseStatus = 0;
    responseIsJson = false;
    responseIsGzip = false;
    responseComplete = false;
    UA_String method = UA_STRING("POST");
    UA_KeyValuePair sendParams[3] = {0};
    sendParams[0].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&sendParams[0].value, (void *)(uintptr_t)path,
                         &UA_TYPES[UA_TYPES_STRING]);
    sendParams[1].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&sendParams[1].value, &method,
                         &UA_TYPES[UA_TYPES_STRING]);
    sendParams[2].key = UA_QUALIFIEDNAME(0, "headers");
    UA_Variant_setArray(&sendParams[2].value, headers, headersSize,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    UA_KeyValueMap sendMap = {3, sendParams};
    ck_assert_uint_eq(http->sendWithConnection(http, clientConnectionId,
                                               &sendMap, body),
                      UA_STATUSCODE_GOOD);
}

static void
sendHttpPost(UA_ConnectionManager *http, UA_EventLoop *eventLoop,
             const UA_String *path, UA_KeyValuePair *headers,
             size_t headersSize, UA_ByteString *body,
             UA_Boolean expectResponseBody) {
    startHttpPost(http, path, headers, headersSize, body);
    for(size_t i = 0; i < 300 && !responseComplete; i++)
        eventLoop->run(eventLoop, 20);
    ck_assert(responseComplete);
    if(expectResponseBody && responseStatus == 200)
        ck_assert_uint_gt(responseBody.length, 0);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
static void
startBinaryServiceRequest(UA_ConnectionManager *http, const UA_String *path,
                          const void *request,
                          const UA_DataType *requestType) {
    UA_String contentType = UA_STRING("application/octet-stream");
    UA_KeyValuePair header = {UA_QUALIFIEDNAME(0, "content-type"), {0}};
    UA_Variant_setScalar(&header.value, &contentType,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_ByteString requestBody = encodeServiceRequest(request, requestType);
    startHttpPost(http, path, &header, 1, &requestBody);
}
#endif

static void
sendServiceRequestWithEncoding(UA_ConnectionManager *http,
                               UA_EventLoop *eventLoop,
                               const UA_String *path, const void *request,
                               const UA_DataType *requestType,
                               UA_Boolean json) {
    UA_String contentType = json ? UA_STRING("application/json; charset=utf-8")
                                 : UA_STRING("application/octet-stream");
    UA_KeyValuePair header = {UA_QUALIFIEDNAME(0, "content-type"), {0}};
    UA_Variant_setScalar(&header.value, &contentType,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_ByteString requestBody;
#ifdef UA_ENABLE_JSON_ENCODING
    if(json)
        requestBody = encodeServiceRequestJson(request, requestType);
    else
#else
    ck_assert(!json);
#endif
        requestBody = encodeServiceRequest(request, requestType);
    sendHttpPost(http, eventLoop, path, &header, 1, &requestBody, true);
    ck_assert_uint_eq(responseStatus, 200);
    ck_assert_uint_gt(responseBody.length, 0);
    ck_assert_uint_eq(responseIsJson, json);
}

static void
sendServiceRequest(UA_ConnectionManager *http, UA_EventLoop *eventLoop,
                   const UA_String *path, const void *request,
                   const UA_DataType *requestType) {
    sendServiceRequestWithEncoding(http, eventLoop, path, request, requestType,
                                   false);
}

#ifdef UA_ENABLE_ENCRYPTION
static void
sendSecureServiceRequest(UA_ConnectionManager *http, UA_EventLoop *eventLoop,
                         const UA_String *path, const UA_String *policyUri,
                         const void *request, const UA_DataType *requestType) {
    UA_String contentType = UA_STRING("application/octet-stream");
    UA_KeyValuePair headers[2] = {0};
    headers[0].key = UA_QUALIFIEDNAME(0, "content-type");
    UA_Variant_setScalar(&headers[0].value, &contentType,
                         &UA_TYPES[UA_TYPES_STRING]);
    headers[1].key = UA_QUALIFIEDNAME(0, "opcua-securitypolicy");
    UA_Variant_setScalar(&headers[1].value, (void*)(uintptr_t)policyUri,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_ByteString body = encodeServiceRequest(request, requestType);
    sendHttpPost(http, eventLoop, path, headers, 2, &body, true);
    ck_assert_uint_eq(responseStatus, 200);
    ck_assert_uint_gt(responseBody.length, 0);
}

static void
setLegacyClientSignature(UA_SecurityPolicy *policy, void *channelContext,
                         const UA_ByteString *serverCertificate,
                         const UA_ByteString *serverNonce,
                         UA_SignatureData *signature) {
    ck_assert_uint_eq(
        UA_String_copy(&policy->asymSignatureAlgorithm.uri,
                       &signature->algorithm),
        UA_STATUSCODE_GOOD);
    size_t signatureSize =
        policy->asymSignatureAlgorithm.getLocalSignatureSize(policy,
                                                              channelContext);
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&signature->signature,
                                                signatureSize),
                      UA_STATUSCODE_GOOD);
    UA_ByteString data = UA_BYTESTRING_NULL;
    ck_assert_uint_le(serverCertificate->length,
                      SIZE_MAX - serverNonce->length);
    ck_assert_uint_eq(UA_ByteString_allocBuffer(
                          &data, serverCertificate->length +
                                     serverNonce->length),
                      UA_STATUSCODE_GOOD);
    memcpy(data.data, serverCertificate->data, serverCertificate->length);
    memcpy(data.data + serverCertificate->length, serverNonce->data,
           serverNonce->length);
    ck_assert_uint_eq(policy->asymSignatureAlgorithm.sign(
                          policy, channelContext, &data,
                          &signature->signature),
                      UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&data);
}
#endif

static void
sendInvalidRequest(UA_ConnectionManager *http, UA_EventLoop *eventLoop,
                   const UA_String *path, const char *contentTypeValue,
                   const char *contentEncodingValue, const char *payload,
                   UA_UInt16 expectedStatus) {
    UA_String contentType = UA_STRING((char *)(uintptr_t)contentTypeValue);
    UA_String contentEncoding = UA_STRING_NULL;
    UA_KeyValuePair headers[2] = {0};
    headers[0].key = UA_QUALIFIEDNAME(0, "content-type");
    UA_Variant_setScalar(&headers[0].value, &contentType,
                         &UA_TYPES[UA_TYPES_STRING]);
    size_t headersSize = 1;
    if(contentEncodingValue) {
        contentEncoding =
            UA_STRING((char *)(uintptr_t)contentEncodingValue);
        headers[1].key = UA_QUALIFIEDNAME(0, "content-encoding");
        UA_Variant_setScalar(&headers[1].value, &contentEncoding,
                             &UA_TYPES[UA_TYPES_STRING]);
        headersSize++;
    }
    UA_ByteString body = UA_BYTESTRING_ALLOC((char *)(uintptr_t)payload);
    sendHttpPost(http, eventLoop, path, headers, headersSize, &body, false);
    ck_assert_uint_eq(responseStatus, expectedStatus);
    ck_assert_uint_eq(responseBody.length, 0);
}

#ifdef UA_ENABLE_JSON_ENCODING
static void
sendServiceRequestJson(UA_ConnectionManager *http, UA_EventLoop *eventLoop,
                       const UA_String *path, const void *request,
                       const UA_DataType *requestType) {
    sendServiceRequestWithEncoding(http, eventLoop, path, request, requestType,
                                   true);
}

static void
decodeJsonResponse(const UA_DataType *expectedType, void *response) {
    UA_DecodeJsonOptions options;
    memset(&options, 0, sizeof(options));
    UA_ExtensionObject envelope;
    ck_assert_uint_eq(
        UA_decodeJson(&responseBody, &envelope,
                      &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &options),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(envelope.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert_ptr_eq(envelope.content.decoded.type, expectedType);
    ck_assert_uint_eq(UA_copy(envelope.content.decoded.data, response,
                             expectedType),
                      UA_STATUSCODE_GOOD);
    UA_ExtensionObject_clear(&envelope);
}
#endif

static void
decodeResponseType(const UA_DataType *expectedType, size_t *offset) {
    UA_NodeId responseType;
    ck_assert_uint_eq(UA_decodeBinaryInternal(&responseBody, offset,
                                              &responseType,
                                              &UA_TYPES[UA_TYPES_NODEID], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&responseType, &expectedType->binaryEncodingId));
    UA_NodeId_clear(&responseType);
}

START_TEST(binaryServiceOverHttps) {
    clientConnectionId = 0;
    responseStatus = 0;
    responseBody = UA_BYTESTRING_NULL;

    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpCertificate = loadFile("server_cert.der");
    config->httpPrivateKey = loadFile("server_key.der");
    config->httpListenAddress = UA_STRING_ALLOC("");
    ck_assert_uint_gt(config->httpCertificate.length, 0);
    ck_assert_uint_gt(config->httpPrivateKey.length, 0);
    ck_assert_ptr_nonnull(config->httpListenAddress.data);
    UA_UsernamePasswordLogin login = {
        UA_STRING_STATIC("user"), UA_STRING_STATIC("password")};
    ck_assert_uint_eq(UA_AccessControl_default(
                          config, true, &UA_SECURITY_POLICY_NONE_URI,
                          1, &login),
                      UA_STATUSCODE_GOOD);
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.https://127.0.0.1:0/ua");
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_Boolean foundHttpDriver = false;
    UA_String httpDriverName = UA_STRING("opc-http");
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(UA_String_equal(&drv->name, &httpDriverName)) {
            foundHttpDriver = true;
            ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STARTED);
        }
    }
    ck_assert(foundHttpDriver);

    UA_UInt16 port = 0;
    UA_String advertisedUrl = UA_STRING_NULL;
    for(size_t i = 0; i < config->applicationDescription.discoveryUrlsSize;
        i++) {
        const UA_String *url = &config->applicationDescription.discoveryUrls[i];
        if(url->length >= 12 && memcmp(url->data, "opc.https://", 12) == 0) {
            UA_String hostname = UA_STRING_NULL;
            UA_String urlPath = UA_STRING_NULL;
            ck_assert_uint_eq(
                UA_parseEndpointUrl(url, &hostname, &port, &urlPath),
                UA_STATUSCODE_GOOD);
            const UA_String expectedHostname = UA_STRING_STATIC("127.0.0.1");
            const UA_String expectedPath = UA_STRING_STATIC("ua");
            ck_assert(UA_String_equal(&hostname, &expectedHostname));
            ck_assert(UA_String_equal(&urlPath, &expectedPath));
            ck_assert_uint_eq(UA_String_copy(url, &advertisedUrl),
                              UA_STATUSCODE_GOOD);
            break;
        }
    }
    ck_assert_uint_ne(port, 0);

    char *publicEndpoint = (char *)UA_malloc(advertisedUrl.length + 1);
    ck_assert_ptr_nonnull(publicEndpoint);
    memcpy(publicEndpoint, advertisedUrl.data, advertisedUrl.length);
    publicEndpoint[advertisedUrl.length] = 0;
    UA_ClientConfig publicConfig;
    memset(&publicConfig, 0, sizeof(publicConfig));
    publicConfig.eventLoop = config->eventLoop;
    publicConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&publicConfig),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ByteString_copy(&config->httpCertificate,
                                         &publicConfig.httpCaCertificate),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ClientConfig_setAuthenticationUsername(
                          &publicConfig, "user", "password"),
                      UA_STATUSCODE_GOOD);
    UA_Client *publicClient = UA_Client_newWithConfig(&publicConfig);
    ck_assert_ptr_nonnull(publicClient);
    ck_assert_uint_eq(UA_Client_connect(publicClient, publicEndpoint),
                      UA_STATUSCODE_GOOD);
    UA_MessageSecurityMode publicMode = UA_MESSAGESECURITYMODE_INVALID;
    UA_QualifiedName modeKey = UA_QUALIFIEDNAME(0, "securityMode");
    ck_assert_uint_eq(UA_Client_getConnectionAttribute_scalar(
                          publicClient, modeKey,
                          &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE],
                          &publicMode),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(publicMode,
                      UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    UA_Boolean foundRemoteAddress = false;
    lockServer(server);
    UA_SecureChannel *publicChannel;
    TAILQ_FOREACH(publicChannel, &server->channels, serverEntry) {
        if(publicChannel->transport != UA_SECURECHANNEL_TRANSPORT_HTTP ||
           !publicChannel->sessions)
            continue;
        ck_assert_uint_eq(publicChannel->securityMode,
                          UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
        foundRemoteAddress |= publicChannel->remoteAddress.length > 0;
    }
    unlockServer(server);
    ck_assert(foundRemoteAddress);
    UA_Variant publicValue;
    UA_Variant_init(&publicValue);
    ck_assert_uint_eq(UA_Client_readValueAttribute(
                          publicClient,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE),
                          &publicValue), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_isScalar(&publicValue));
    UA_Variant_clear(&publicValue);
    ck_assert_uint_eq(UA_Client_disconnect(publicClient), UA_STATUSCODE_GOOD);
    UA_Client_delete(publicClient);
    UA_free(publicEndpoint);

#ifdef UA_ENABLE_JSON_ENCODING
    /* Binary and JSON are separate endpoints so a logical SecureChannel is
     * pinned to one encoding for its entire lifetime. */
    UA_String jsonProfile = UA_STRING(
        "http://opcfoundation.org/UA-Profile/Transport/https-uajson");
    UA_EndpointDescription *jsonEndpoints = NULL;
    size_t jsonEndpointsSize = 0;
    lockServer(server);
    ck_assert_uint_eq(
        setCurrentEndpointsArray(server, advertisedUrl, &jsonProfile, 1,
                                 &jsonEndpoints, &jsonEndpointsSize),
        UA_STATUSCODE_GOOD);
    unlockServer(server);
    ck_assert_uint_gt(jsonEndpointsSize, 0);
    const UA_String jsonSuffix = UA_STRING_STATIC("/json");
    for(size_t i = 0; i < jsonEndpointsSize; i++) {
        ck_assert(UA_String_equal(&jsonEndpoints[i].transportProfileUri,
                                  &jsonProfile));
        ck_assert_uint_ge(jsonEndpoints[i].endpointUrl.length,
                          jsonSuffix.length);
        ck_assert_mem_eq(
            &jsonEndpoints[i].endpointUrl.data[
                jsonEndpoints[i].endpointUrl.length - jsonSuffix.length],
            jsonSuffix.data, jsonSuffix.length);
    }
    UA_Array_delete(jsonEndpoints, jsonEndpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
#endif

    UA_ConnectionManager *http = getHttpConnectionManager(config);
    ck_assert_ptr_nonnull(http);
    openRawHttpClient(http, port, true, &config->httpCertificate);

    UA_String path = UA_STRING("/ua");
#ifdef UA_ENABLE_JSON_ENCODING
    UA_String jsonPath = UA_STRING("/ua/json");
    sendInvalidRequest(http, config->eventLoop, &jsonPath, "application/json",
                       NULL, "{", 400);
    sendInvalidRequest(http, config->eventLoop, &path, "application/json",
                       NULL, "{}", 415);
    sendInvalidRequest(http, config->eventLoop, &jsonPath,
                       "application/octet-stream", NULL, "invalid", 415);
#endif
    sendInvalidRequest(http, config->eventLoop, &path,
                       "application/octet-stream", "gzip", "not-gzip",
#ifdef UA_ENABLE_HTTP_COMPRESSION
                       400);
#else
                       415);
#endif

    UA_FindServersRequest findRequest;
    UA_FindServersRequest_init(&findRequest);
    findRequest.requestHeader.timestamp = UA_DateTime_now();
    findRequest.requestHeader.requestHandle = 42;

#ifdef UA_ENABLE_HTTP_COMPRESSION
    /* OPC UA Binary permits only identity coding. JSON permits gzip, but not
     * the generic HTTP manager's deflate extension. */
    sendCodedServiceRequest(http, config->eventLoop, &path, &findRequest,
                            &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST], false,
                            UA_HTTP_CONTENT_ENCODING_GZIP, "identity", 415);
    sendCodedServiceRequest(http, config->eventLoop, &path, &findRequest,
                            &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST], false,
                            UA_HTTP_CONTENT_ENCODING_IDENTITY,
                            "gzip, identity;q=0", 406);
# ifdef UA_ENABLE_JSON_ENCODING
    sendCodedServiceRequest(http, config->eventLoop, &jsonPath, &findRequest,
                            &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST], true,
                            UA_HTTP_CONTENT_ENCODING_DEFLATE, "identity", 415);
    sendCodedServiceRequest(http, config->eventLoop, &jsonPath, &findRequest,
                            &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST], true,
                            UA_HTTP_CONTENT_ENCODING_GZIP,
                            "gzip, identity;q=0", 200);
    ck_assert(responseIsGzip);
# endif
#endif

    /* Header names are case-insensitive. Duplicate single-valued OPC UA
     * routing headers are rejected before dispatch. */
    UA_String mixedContentType = UA_STRING("application/octet-stream");
    UA_KeyValuePair mixedHeader = {
        UA_QUALIFIEDNAME(0, "CoNtEnT-TyPe"), {0}};
    UA_Variant_setScalar(&mixedHeader.value, &mixedContentType,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_ByteString mixedBody = encodeServiceRequest(
        &findRequest, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    sendHttpPost(http, config->eventLoop, &path, &mixedHeader, 1,
                 &mixedBody, true);
    ck_assert_uint_eq(responseStatus, 200);

    UA_String duplicateType = UA_STRING("application/octet-stream");
    UA_KeyValuePair duplicateHeaders[2] = {0};
    duplicateHeaders[0].key = UA_QUALIFIEDNAME(0, "content-type");
    duplicateHeaders[1].key = UA_QUALIFIEDNAME(0, "Content-Type");
    UA_Variant_setScalar(&duplicateHeaders[0].value, &duplicateType,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&duplicateHeaders[1].value, &duplicateType,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_ByteString duplicateBody = encodeServiceRequest(
        &findRequest, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    sendHttpPost(http, config->eventLoop, &path, duplicateHeaders, 2,
                 &duplicateBody, false);
    ck_assert_uint_eq(responseStatus, 400);

    UA_String nonePolicy = UA_SECURITY_POLICY_NONE_URI;
    UA_KeyValuePair duplicatePolicyHeaders[3] = {0};
    duplicatePolicyHeaders[0].key = UA_QUALIFIEDNAME(0, "content-type");
    UA_Variant_setScalar(&duplicatePolicyHeaders[0].value, &duplicateType,
                         &UA_TYPES[UA_TYPES_STRING]);
    duplicatePolicyHeaders[1].key =
        UA_QUALIFIEDNAME(0, "opcua-securitypolicy");
    duplicatePolicyHeaders[2].key =
        UA_QUALIFIEDNAME(0, "OPCUA-SecurityPolicy");
    UA_Variant_setScalar(&duplicatePolicyHeaders[1].value, &nonePolicy,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&duplicatePolicyHeaders[2].value, &nonePolicy,
                         &UA_TYPES[UA_TYPES_STRING]);
    duplicateBody = encodeServiceRequest(
        &findRequest, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    sendHttpPost(http, config->eventLoop, &path, duplicatePolicyHeaders, 3,
                 &duplicateBody, false);
    ck_assert_uint_eq(responseStatus, 400);

    /* One HTTP body contains exactly one encoded OPC UA service request. */
    UA_ByteString trailingBody = encodeServiceRequest(
        &findRequest, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    UA_Byte *trailingData = (UA_Byte*)UA_realloc(
        trailingBody.data, trailingBody.length + 1);
    ck_assert_ptr_nonnull(trailingData);
    trailingBody.data = trailingData;
    trailingBody.data[trailingBody.length++] = 0xff;
    UA_String binaryContentType = UA_STRING("application/octet-stream");
    UA_KeyValuePair binaryHeader = {
        UA_QUALIFIEDNAME(0, "content-type"), {0}};
    UA_Variant_setScalar(&binaryHeader.value, &binaryContentType,
                         &UA_TYPES[UA_TYPES_STRING]);
    sendHttpPost(http, config->eventLoop, &path, &binaryHeader, 1,
                 &trailingBody, false);
    ck_assert_uint_eq(responseStatus, 400);
    ck_assert_uint_eq(responseBody.length, 0);

    sendServiceRequest(http, config->eventLoop, &path, &findRequest,
                       &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);

    size_t offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE], &offset);
    UA_FindServersResponse response;
    UA_FindServersResponse_init(&response);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &response,
                                &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.responseHeader.requestHandle, 42);
    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(response.serversSize, 0);
    UA_Boolean foundAdvertisedUrl = false;
    for(size_t i = 0; i < response.serversSize; i++) {
        for(size_t j = 0; j < response.servers[i].discoveryUrlsSize; j++) {
            if(UA_String_equal(&response.servers[i].discoveryUrls[j],
                               &advertisedUrl))
                foundAdvertisedUrl = true;
        }
    }
    ck_assert(foundAdvertisedUrl);
    UA_FindServersResponse_clear(&response);

#ifdef UA_ENABLE_JSON_ENCODING
    sendServiceRequestJson(http, config->eventLoop, &jsonPath, &findRequest,
                           &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    UA_FindServersResponse_init(&response);
    decodeJsonResponse(&UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE], &response);
    ck_assert_uint_eq(response.responseHeader.requestHandle, 42);
    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_FindServersResponse_clear(&response);
#endif

    /* CreateSession, ActivateSession and Read use separate HTTP requests but
     * must resolve to one persistent logical SecureChannel. */
    UA_CreateSessionRequest createRequest;
    UA_CreateSessionRequest_init(&createRequest);
    createRequest.requestHeader.timestamp = UA_DateTime_now();
    createRequest.requestHeader.requestHandle = 43;
    createRequest.clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;
    createRequest.clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541:http-test-client");
    createRequest.clientDescription.productUri =
        UA_STRING_ALLOC("urn:open62541:http-test");
    createRequest.clientDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("en", "HTTP test client");
    ck_assert_uint_eq(UA_String_copy(&advertisedUrl,
                                     &createRequest.endpointUrl),
                      UA_STATUSCODE_GOOD);
    createRequest.sessionName = UA_STRING_ALLOC("HTTP test session");
    createRequest.requestedSessionTimeout = 60000.0;
    createRequest.maxResponseMessageSize = 0;
    sendServiceRequest(http, config->eventLoop, &path, &createRequest,
                       &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]);
    UA_CreateSessionRequest_clear(&createRequest);

    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], &offset);
    UA_CreateSessionResponse createResponse;
    UA_CreateSessionResponse_init(&createResponse);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &createResponse,
                                &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.responseHeader.requestHandle, 43);
    ck_assert_uint_eq(createResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.authenticationToken.identifierType,
                      UA_NODEIDTYPE_BYTESTRING);
    ck_assert_uint_ge(
        createResponse.authenticationToken.identifier.byteString.length, 32);
    UA_NodeId authenticationToken;
    UA_NodeId_init(&authenticationToken);
    ck_assert_uint_eq(UA_NodeId_copy(&createResponse.authenticationToken,
                                     &authenticationToken),
                      UA_STATUSCODE_GOOD);
    UA_String anonymousPolicyId = UA_STRING_NULL;
    UA_Boolean foundHttpsBinaryProfile = false;
#ifdef UA_ENABLE_JSON_ENCODING
    UA_Boolean foundHttpsJsonProfile = false;
#endif
    const UA_String httpsBinaryProfile = UA_STRING_STATIC(
        "http://opcfoundation.org/UA-Profile/Transport/https-uabinary");
#ifdef UA_ENABLE_JSON_ENCODING
    const UA_String httpsJsonProfile = UA_STRING_STATIC(
        "http://opcfoundation.org/UA-Profile/Transport/https-uajson");
#endif
    for(size_t i = 0; i < createResponse.serverEndpointsSize; i++) {
        UA_EndpointDescription *ed = &createResponse.serverEndpoints[i];
        if(UA_String_equal(&ed->transportProfileUri, &httpsBinaryProfile)) {
            ck_assert_uint_eq(ed->securityMode,
                              UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
            foundHttpsBinaryProfile = true;
        }
#ifdef UA_ENABLE_JSON_ENCODING
        if(UA_String_equal(&ed->transportProfileUri, &httpsJsonProfile)) {
            ck_assert_uint_eq(ed->securityMode,
                              UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
            foundHttpsJsonProfile = true;
        }
#endif
        for(size_t j = 0; j < ed->userIdentityTokensSize; j++) {
            UA_UserTokenPolicy *utp = &ed->userIdentityTokens[j];
            if(utp->tokenType == UA_USERTOKENTYPE_ANONYMOUS &&
               anonymousPolicyId.length == 0)
                ck_assert_uint_eq(UA_String_copy(&utp->policyId,
                                                 &anonymousPolicyId),
                                  UA_STATUSCODE_GOOD);
        }
    }
    ck_assert_uint_gt(anonymousPolicyId.length, 0);
    ck_assert(foundHttpsBinaryProfile);
#ifdef UA_ENABLE_JSON_ENCODING
    ck_assert(foundHttpsJsonProfile);
#endif
    UA_CreateSessionResponse_clear(&createResponse);

    UA_ActivateSessionRequest activateRequest;
    UA_ActivateSessionRequest_init(&activateRequest);
    activateRequest.requestHeader.timestamp = UA_DateTime_now();
    activateRequest.requestHeader.requestHandle = 44;
    ck_assert_uint_eq(UA_NodeId_copy(&authenticationToken,
                                     &activateRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    UA_AnonymousIdentityToken anonymousToken;
    UA_AnonymousIdentityToken_init(&anonymousToken);
    ck_assert_uint_eq(UA_String_copy(&anonymousPolicyId,
                                     &anonymousToken.policyId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_ExtensionObject_setValueCopy(
            &activateRequest.userIdentityToken, &anonymousToken,
            &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]),
        UA_STATUSCODE_GOOD);
    UA_AnonymousIdentityToken_clear(&anonymousToken);
#ifdef UA_ENABLE_JSON_ENCODING
    /* An AuthenticationToken cannot move from its Binary channel to the JSON
     * endpoint. The request gets a JSON ServiceFault and the Binary channel
     * remains usable for the subsequent activation. */
    sendServiceRequestJson(http, config->eventLoop, &jsonPath,
                           &activateRequest,
                           &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST]);
    UA_ServiceFault crossEncodingFault;
    UA_ServiceFault_init(&crossEncodingFault);
    decodeJsonResponse(&UA_TYPES[UA_TYPES_SERVICEFAULT], &crossEncodingFault);
    ck_assert_uint_eq(
        crossEncodingFault.responseHeader.serviceResult,
        UA_STATUSCODE_BADSESSIONIDINVALID);
    UA_ServiceFault_clear(&crossEncodingFault);
#endif
    sendServiceRequest(http, config->eventLoop, &path, &activateRequest,
                       &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST]);
    UA_ActivateSessionRequest_clear(&activateRequest);

    UA_ActivateSessionResponse activateResponse;
    UA_ActivateSessionResponse_init(&activateResponse);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], &offset);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &activateResponse,
                                &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(activateResponse.responseHeader.requestHandle, 44);
    ck_assert_uint_eq(activateResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_ActivateSessionResponse_clear(&activateResponse);

    /* Losing the HTTP carrier must not tear down the logical channel selected
     * by the AuthenticationToken. Reopen the transport and continue the same
     * Session below. */
    UA_UInt32 logicalChannelId = 0;
    lockServer(server);
    UA_Session *sessionBeforeReconnect =
        getSessionByToken(server, &authenticationToken);
    ck_assert_ptr_nonnull(sessionBeforeReconnect);
    ck_assert_ptr_nonnull(sessionBeforeReconnect->channel);
    ck_assert_uint_eq(sessionBeforeReconnect->channel->transport,
                      UA_SECURECHANNEL_TRANSPORT_HTTP);
    ck_assert_uint_eq(sessionBeforeReconnect->channel->encoding,
                      UA_SECURECHANNEL_ENCODING_BINARY);
    logicalChannelId = sessionBeforeReconnect->channel->securityToken.channelId;
    unlockServer(server);
    ck_assert_uint_eq(http->closeConnection(http, clientConnectionId),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 20; i++)
        config->eventLoop->run(config->eventLoop, 20);
    openRawHttpClient(http, port, true, &config->httpCertificate);
    lockServer(server);
    UA_Session *sessionAfterReconnect =
        getSessionByToken(server, &authenticationToken);
    ck_assert_ptr_nonnull(sessionAfterReconnect);
    ck_assert_ptr_nonnull(sessionAfterReconnect->channel);
    ck_assert_uint_eq(sessionAfterReconnect->channel->securityToken.channelId,
                      logicalChannelId);
    unlockServer(server);

    UA_ReadRequest readRequest;
    UA_ReadRequest_init(&readRequest);
    readRequest.requestHeader.timestamp = UA_DateTime_now();
    readRequest.requestHeader.requestHandle = 45;
    ck_assert_uint_eq(UA_NodeId_copy(&authenticationToken,
                                     &readRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    readRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    readRequest.nodesToRead =
        (UA_ReadValueId *)UA_Array_new(1, &UA_TYPES[UA_TYPES_READVALUEID]);
    ck_assert_ptr_nonnull(readRequest.nodesToRead);
    readRequest.nodesToReadSize = 1;
    readRequest.nodesToRead[0].nodeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    readRequest.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;
    sendServiceRequest(http, config->eventLoop, &path, &readRequest,
                       &UA_TYPES[UA_TYPES_READREQUEST]);
    UA_ReadRequest_clear(&readRequest);

    UA_ReadResponse readResponse;
    UA_ReadResponse_init(&readResponse);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_READRESPONSE], &offset);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &readResponse,
                                &UA_TYPES[UA_TYPES_READRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(readResponse.responseHeader.requestHandle, 45);
    ck_assert_uint_eq(readResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(readResponse.resultsSize, 1);
    ck_assert_uint_eq(readResponse.results[0].status, UA_STATUSCODE_GOOD);
    UA_ReadResponse_clear(&readResponse);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_CreateSubscriptionRequest subscriptionRequest;
    UA_CreateSubscriptionRequest_init(&subscriptionRequest);
    subscriptionRequest.requestHeader.timestamp = UA_DateTime_now();
    subscriptionRequest.requestHeader.requestHandle = 48;
    ck_assert_uint_eq(UA_NodeId_copy(
                          &authenticationToken,
                          &subscriptionRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    subscriptionRequest.requestedPublishingInterval = 50.0;
    subscriptionRequest.requestedLifetimeCount = 100;
    subscriptionRequest.requestedMaxKeepAliveCount = 1;
    subscriptionRequest.publishingEnabled = true;
    sendServiceRequest(http, config->eventLoop, &path, &subscriptionRequest,
                       &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST]);
    UA_CreateSubscriptionRequest_clear(&subscriptionRequest);

    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE], &offset);
    UA_CreateSubscriptionResponse subscriptionResponse;
    UA_CreateSubscriptionResponse_init(&subscriptionResponse);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(
            &responseBody, &offset, &subscriptionResponse,
            &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(subscriptionResponse.responseHeader.requestHandle, 48);
    ck_assert_uint_eq(subscriptionResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_UInt32 subscriptionId = subscriptionResponse.subscriptionId;
    ck_assert_uint_ne(subscriptionId, 0);
    UA_CreateSubscriptionResponse_clear(&subscriptionResponse);

    /* Publish is intentionally long-standing. Its HTTP request remains alive
     * until the subscription produces the keep-alive response. */
    UA_PublishRequest publishRequest;
    UA_PublishRequest_init(&publishRequest);
    publishRequest.requestHeader.timestamp = UA_DateTime_now();
    publishRequest.requestHeader.requestHandle = 49;
    publishRequest.requestHeader.timeoutHint = 5000;
    ck_assert_uint_eq(UA_NodeId_copy(
                          &authenticationToken,
                          &publishRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    sendServiceRequest(http, config->eventLoop, &path, &publishRequest,
                       &UA_TYPES[UA_TYPES_PUBLISHREQUEST]);
    UA_PublishRequest_clear(&publishRequest);

    UA_PublishResponse publishResponse;
    UA_PublishResponse_init(&publishResponse);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_PUBLISHRESPONSE], &offset);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &publishResponse,
                                &UA_TYPES[UA_TYPES_PUBLISHRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(publishResponse.responseHeader.requestHandle, 49);
    ck_assert_uint_eq(publishResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(publishResponse.subscriptionId, subscriptionId);
    UA_PublishResponse_clear(&publishResponse);
#endif

    /* A second logical client using the same endpoint and SecurityPolicy gets
     * a distinct internal channel. Its AuthenticationToken routes subsequent
     * physical HTTP requests back to that channel. */
    UA_CreateSessionRequest secondCreateRequest;
    UA_CreateSessionRequest_init(&secondCreateRequest);
    secondCreateRequest.requestHeader.timestamp = UA_DateTime_now();
    secondCreateRequest.requestHeader.requestHandle = 47;
    secondCreateRequest.clientDescription.applicationType =
        UA_APPLICATIONTYPE_CLIENT;
    secondCreateRequest.clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541:second-http-test-client");
    secondCreateRequest.clientDescription.productUri =
        UA_STRING_ALLOC("urn:open62541:http-test");
    secondCreateRequest.clientDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("en", "Second HTTP test client");
    ck_assert_uint_eq(UA_String_copy(&advertisedUrl,
                                     &secondCreateRequest.endpointUrl),
                      UA_STATUSCODE_GOOD);
    secondCreateRequest.sessionName =
        UA_STRING_ALLOC("Second HTTP test session");
    secondCreateRequest.requestedSessionTimeout = 60000.0;
    sendServiceRequest(http, config->eventLoop, &path, &secondCreateRequest,
                       &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]);
    UA_CreateSessionRequest_clear(&secondCreateRequest);

    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], &offset);
    UA_CreateSessionResponse secondCreateResponse;
    UA_CreateSessionResponse_init(&secondCreateResponse);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &secondCreateResponse,
                                &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(secondCreateResponse.responseHeader.requestHandle, 47);
    ck_assert_uint_eq(secondCreateResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(secondCreateResponse.authenticationToken.identifierType,
                      UA_NODEIDTYPE_BYTESTRING);
    ck_assert_uint_ge(
        secondCreateResponse.authenticationToken.identifier.byteString.length,
        32);
    ck_assert(!UA_NodeId_equal(&authenticationToken,
                               &secondCreateResponse.authenticationToken));
    UA_NodeId secondAuthenticationToken;
    UA_NodeId_init(&secondAuthenticationToken);
    ck_assert_uint_eq(UA_NodeId_copy(
                          &secondCreateResponse.authenticationToken,
                          &secondAuthenticationToken),
                      UA_STATUSCODE_GOOD);
    UA_CreateSessionResponse_clear(&secondCreateResponse);

    UA_ActivateSessionRequest secondActivateRequest;
    UA_ActivateSessionRequest_init(&secondActivateRequest);
    secondActivateRequest.requestHeader.timestamp = UA_DateTime_now();
    secondActivateRequest.requestHeader.requestHandle = 52;
    ck_assert_uint_eq(UA_NodeId_copy(
                          &secondAuthenticationToken,
                          &secondActivateRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    secondActivateRequest.userIdentityToken.encoding =
        UA_EXTENSIONOBJECT_DECODED;
    secondActivateRequest.userIdentityToken.content.decoded.type =
        &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN];
    secondActivateRequest.userIdentityToken.content.decoded.data =
        UA_AnonymousIdentityToken_new();
    ck_assert_ptr_nonnull(
        secondActivateRequest.userIdentityToken.content.decoded.data);
    UA_AnonymousIdentityToken *secondAnonymous =
        (UA_AnonymousIdentityToken *)
            secondActivateRequest.userIdentityToken.content.decoded.data;
    ck_assert_uint_eq(UA_String_copy(&anonymousPolicyId,
                                     &secondAnonymous->policyId),
                      UA_STATUSCODE_GOOD);
    sendServiceRequest(http, config->eventLoop, &path, &secondActivateRequest,
                       &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST]);
    UA_ActivateSessionRequest_clear(&secondActivateRequest);

    UA_ActivateSessionResponse secondActivateResponse;
    UA_ActivateSessionResponse_init(&secondActivateResponse);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], &offset);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &secondActivateResponse,
                                &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(secondActivateResponse.responseHeader.requestHandle, 52);
    ck_assert_uint_eq(secondActivateResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_ActivateSessionResponse_clear(&secondActivateResponse);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_CreateSubscriptionRequest secondSubscriptionRequest;
    UA_CreateSubscriptionRequest_init(&secondSubscriptionRequest);
    secondSubscriptionRequest.requestHeader.timestamp = UA_DateTime_now();
    secondSubscriptionRequest.requestHeader.requestHandle = 53;
    ck_assert_uint_eq(UA_NodeId_copy(
                          &secondAuthenticationToken,
                          &secondSubscriptionRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    secondSubscriptionRequest.requestedPublishingInterval = 5000.0;
    secondSubscriptionRequest.requestedLifetimeCount = 100;
    secondSubscriptionRequest.requestedMaxKeepAliveCount = 10;
    secondSubscriptionRequest.publishingEnabled = true;
    sendServiceRequest(http, config->eventLoop, &path,
                       &secondSubscriptionRequest,
                       &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST]);
    UA_CreateSubscriptionRequest_clear(&secondSubscriptionRequest);

    UA_CreateSubscriptionResponse secondSubscriptionResponse;
    UA_CreateSubscriptionResponse_init(&secondSubscriptionResponse);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE], &offset);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(
            &responseBody, &offset, &secondSubscriptionResponse,
            &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        secondSubscriptionResponse.responseHeader.serviceResult,
        UA_STATUSCODE_GOOD);
    UA_CreateSubscriptionResponse_clear(&secondSubscriptionResponse);
#endif

    lockServer(server);
    UA_Session *firstSession = getSessionByToken(server, &authenticationToken);
    UA_Session *secondSession =
        getSessionByToken(server, &secondAuthenticationToken);
    ck_assert_ptr_nonnull(firstSession);
    ck_assert_ptr_nonnull(secondSession);
    ck_assert_ptr_nonnull(firstSession->channel);
    ck_assert_ptr_nonnull(secondSession->channel);
    ck_assert_ptr_ne(firstSession->channel, secondSession->channel);
    ck_assert_uint_ne(firstSession->channel->securityToken.channelId,
                      secondSession->channel->securityToken.channelId);
    UA_UInt32 secondLogicalChannelId =
        secondSession->channel->securityToken.channelId;
    unlockServer(server);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Keep a Publish request accepted while its logical channel closes. HTTP
     * can still return a terminal ServiceFault while the channel is CLOSING. */
    UA_PublishRequest pendingPublishRequest;
    UA_PublishRequest_init(&pendingPublishRequest);
    pendingPublishRequest.requestHeader.timestamp = UA_DateTime_now();
    pendingPublishRequest.requestHeader.requestHandle = 54;
    pendingPublishRequest.requestHeader.timeoutHint = 60000;
    ck_assert_uint_eq(UA_NodeId_copy(
                          &secondAuthenticationToken,
                          &pendingPublishRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    startBinaryServiceRequest(http, &path, &pendingPublishRequest,
                              &UA_TYPES[UA_TYPES_PUBLISHREQUEST]);
    UA_PublishRequest_clear(&pendingPublishRequest);

    UA_Boolean publishQueued = false;
    for(size_t i = 0; i < 100 && !publishQueued; i++) {
        config->eventLoop->run(config->eventLoop, 1);
        lockServer(server);
        secondSession = getSessionByToken(server, &secondAuthenticationToken);
        publishQueued = secondSession && secondSession->responseQueueSize == 1;
        unlockServer(server);
    }
    ck_assert(publishQueued);
    ck_assert(!responseComplete);

    /* Dropping the HTTP carrier abandons this long-poll without detaching the
     * Session or leaving a stale Publish entry behind. */
    ck_assert_uint_eq(http->closeConnection(http, clientConnectionId),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 300; i++) {
        config->eventLoop->run(config->eventLoop, 10);
        lockServer(server);
        secondSession = getSessionByToken(server, &secondAuthenticationToken);
        UA_Boolean drained =
            secondSession && secondSession->responseQueueSize == 0;
        unlockServer(server);
        if(drained)
            break;
    }
    lockServer(server);
    secondSession = getSessionByToken(server, &secondAuthenticationToken);
    ck_assert_ptr_nonnull(secondSession);
    ck_assert_uint_eq(secondSession->responseQueueSize, 0);
    unlockServer(server);

    openRawHttpClient(http, port, true, &config->httpCertificate);
    UA_PublishRequest_init(&pendingPublishRequest);
    pendingPublishRequest.requestHeader.timestamp = UA_DateTime_now();
    pendingPublishRequest.requestHeader.requestHandle = 55;
    pendingPublishRequest.requestHeader.timeoutHint = 60000;
    ck_assert_uint_eq(UA_NodeId_copy(
                          &secondAuthenticationToken,
                          &pendingPublishRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    startBinaryServiceRequest(http, &path, &pendingPublishRequest,
                              &UA_TYPES[UA_TYPES_PUBLISHREQUEST]);
    UA_PublishRequest_clear(&pendingPublishRequest);
    publishQueued = false;
    for(size_t i = 0; i < 100 && !publishQueued; i++) {
        config->eventLoop->run(config->eventLoop, 1);
        lockServer(server);
        secondSession = getSessionByToken(server, &secondAuthenticationToken);
        publishQueued = secondSession && secondSession->responseQueueSize == 1;
        unlockServer(server);
    }
    ck_assert(publishQueued);
    ck_assert(!responseComplete);
#endif

    /* The public close API drains the logical HTTP channel without closing
     * the shared listener or the sibling client's channel. */
    ck_assert_uint_eq(UA_Server_closeSecureChannel(
                          server, secondLogicalChannelId,
                          UA_SHUTDOWNREASON_CLOSE),
                      UA_STATUSCODE_GOOD);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    for(size_t i = 0; i < 300 && !responseComplete; i++)
        config->eventLoop->run(config->eventLoop, 20);
    ck_assert(responseComplete);
    ck_assert_uint_eq(responseStatus, 200);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_SERVICEFAULT], &offset);
    UA_ServiceFault closeFault;
    UA_ServiceFault_init(&closeFault);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &closeFault,
                                &UA_TYPES[UA_TYPES_SERVICEFAULT], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeFault.responseHeader.requestHandle, 55);
    ck_assert_uint_eq(closeFault.responseHeader.serviceResult,
                      UA_STATUSCODE_BADSECURECHANNELCLOSED);
    UA_ServiceFault_clear(&closeFault);
#endif
    lockServer(server);
    firstSession = getSessionByToken(server, &authenticationToken);
    secondSession = getSessionByToken(server, &secondAuthenticationToken);
    ck_assert_ptr_nonnull(firstSession);
    ck_assert_ptr_nonnull(firstSession->channel);
    ck_assert_uint_eq(firstSession->channel->securityToken.channelId,
                      logicalChannelId);
    /* Activated Sessions survive transport loss detached from the channel. */
    ck_assert_ptr_nonnull(secondSession);
    ck_assert_ptr_null(secondSession->channel);
    UA_Session_remove(server, secondSession, UA_SHUTDOWNREASON_CLOSE);
    unlockServer(server);

    UA_CloseSessionRequest closeRequest;
    UA_CloseSessionRequest_init(&closeRequest);
    closeRequest.requestHeader.timestamp = UA_DateTime_now();
    closeRequest.requestHeader.requestHandle = 46;
    closeRequest.deleteSubscriptions = true;
    ck_assert_uint_eq(UA_NodeId_copy(&authenticationToken,
                                     &closeRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    sendServiceRequest(http, config->eventLoop, &path, &closeRequest,
                       &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST]);
    UA_CloseSessionRequest_clear(&closeRequest);

    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE], &offset);
    UA_CloseSessionResponse closeResponse;
    UA_CloseSessionResponse_init(&closeResponse);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &closeResponse,
                                &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeResponse.responseHeader.requestHandle, 46);
    ck_assert_uint_eq(closeResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_CloseSessionResponse_clear(&closeResponse);

    UA_String_clear(&anonymousPolicyId);
    UA_String_clear(&advertisedUrl);
    UA_ByteString_clear(&responseBody);

    lockServer(server);
    ck_assert_ptr_null(getSessionByToken(server, &authenticationToken));
    ck_assert_ptr_null(getSessionByToken(server, &secondAuthenticationToken));
    ck_assert_uint_eq(server->sessionCount, 0);
    UA_SecureChannel *remainingChannel;
    TAILQ_FOREACH(remainingChannel, &server->channels, serverEntry)
        ck_assert_uint_ne(remainingChannel->securityToken.channelId,
                          logicalChannelId);
    unlockServer(server);
    UA_NodeId_clear(&authenticationToken);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    lockServer(server);
    ck_assert_uint_eq(server->sessionCount, 0);
    unlockServer(server);
    UA_NodeId_clear(&secondAuthenticationToken);

    /* A restart recreates, rather than accumulates, HTTPS endpoint records. */
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(UA_String_equal(&drv->name, &httpDriverName))
            ck_assert_uint_eq(drv->state, UA_LIFECYCLESTATE_STARTED);
    }

    size_t httpsDiscoveryUrls = 0;
    port = 0;
    for(size_t i = 0; i < config->applicationDescription.discoveryUrlsSize;
        i++) {
        const UA_String *url = &config->applicationDescription.discoveryUrls[i];
        if(url->length < 12 || memcmp(url->data, "opc.https://", 12) != 0)
            continue;
        httpsDiscoveryUrls++;
        UA_String hostname = UA_STRING_NULL;
        UA_String urlPath = UA_STRING_NULL;
        ck_assert_uint_eq(UA_parseEndpointUrl(url, &hostname, &port, &urlPath),
                          UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(UA_String_copy(url, &advertisedUrl),
                          UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(httpsDiscoveryUrls, 1);
    ck_assert_uint_ne(port, 0);
    openRawHttpClient(http, port, true, &config->httpCertificate);
    findRequest.requestHeader.timestamp = UA_DateTime_now();
    findRequest.requestHeader.requestHandle = 50;
    sendServiceRequest(http, config->eventLoop, &path, &findRequest,
                       &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE], &offset);
    UA_FindServersResponse_init(&response);
    ck_assert_uint_eq(
        UA_decodeBinaryInternal(&responseBody, &offset, &response,
                                &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE], NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.responseHeader.requestHandle, 50);
    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_FindServersResponse_clear(&response);
    UA_String_clear(&advertisedUrl);
    UA_ByteString_clear(&responseBody);
    UA_FindServersRequest_clear(&findRequest);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST


START_TEST(oversizedServerResponseReturnsServiceFault) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *serverConfig = UA_Server_getConfig(server);
    serverConfig->tcpEnabled = false;
    serverConfig->httpEnabled = true;
    serverConfig->httpAllowUnencrypted = true;
    serverConfig->httpMaxMsgSize = 4096;
    serverConfig->httpListenAddress = UA_STRING_ALLOC("127.0.0.1");
    UA_Array_delete(serverConfig->serverUrls, serverConfig->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    serverConfig->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(serverConfig->serverUrls);
    serverConfig->serverUrlsSize = 1;
    serverConfig->serverUrls[0] =
        UA_STRING_ALLOC("opc.http://127.0.0.1:0/limited");

    UA_ByteString largeValue = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&largeValue, 16 * 1024),
                      UA_STATUSCODE_GOOD);
    memset(largeValue.data, 0x5a, largeValue.length);
    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    ck_assert_uint_eq(UA_Variant_setScalarCopy(
                          &attributes.value, &largeValue,
                          &UA_TYPES[UA_TYPES_BYTESTRING]),
                      UA_STATUSCODE_GOOD);
    const UA_NodeId largeValueNode = UA_NODEID_NUMERIC(1, 6002);
    ck_assert_uint_eq(UA_Server_addVariableNode(
                          server, largeValueNode,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                          UA_QUALIFIEDNAME(1, "LimitedHttpValue"),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                          attributes, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    UA_VariableAttributes_clear(&attributes);
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
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    clientConfig.httpAllowUnencrypted = true;
    clientConfig.httpMaxMsgSize = 32 * 1024;
    UA_Client *client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    ck_assert_uint_eq(UA_Client_connect(client, endpoint), UA_STATUSCODE_GOOD);

    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Client_readValueAttribute(client, largeValueNode, &value),
                      UA_STATUSCODE_BADRESPONSETOOLARGE);
    UA_Variant_clear(&value);

    /* The ServiceFault is request-local and does not poison the binding. */
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Client_readValueAttribute(
                          client, UA_NODEID_NUMERIC(
                              0, UA_NS0ID_SERVER_SERVERSTATUS_STATE),
                          &value),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);

    ck_assert_uint_eq(UA_Client_disconnect(client), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
    UA_free(endpoint);
    UA_String_clear(&endpointUrl);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

#ifdef UA_ENABLE_ENCRYPTION
static size_t
countAuthenticatedHttpSecureChannels(UA_Server *server) {
    size_t count = 0;
    lockServer(server);
    UA_SecureChannel *channel;
    TAILQ_FOREACH(channel, &server->channels, serverEntry) {
        if(channel->transport == UA_SECURECHANNEL_TRANSPORT_HTTP &&
           channel->remoteCertificate.length > 0) {
            ck_assert_uint_eq(channel->securityMode,
                              UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
            count++;
        }
    }
    unlockServer(server);
    return count;
}

START_TEST(securePolicyServiceOverHttps) {
    clientConnectionId = 0;
    responseStatus = 0;
    responseBody = UA_BYTESTRING_NULL;

    UA_ByteString serverCertificate = loadFile("server_cert.der");
    UA_ByteString serverPrivateKey = loadFile("server_key.der");
    UA_ByteString clientCertificate = loadFile("client_cert.der");
    UA_ByteString clientPrivateKey = loadFile("client_key.der");
    ck_assert_uint_gt(serverCertificate.length, 0);
    ck_assert_uint_gt(serverPrivateKey.length, 0);
    ck_assert_uint_gt(clientCertificate.length, 0);
    ck_assert_uint_gt(clientPrivateKey.length, 0);

    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpListenAddress = UA_STRING_ALLOC("");
    ck_assert_uint_eq(UA_ServerConfig_addSecurityPolicyBasic256Sha256(
                          config, &serverCertificate, &serverPrivateKey),
                      UA_STATUSCODE_GOOD);
    const UA_String securePolicyUri = UA_STRING_STATIC(
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    ck_assert_uint_eq(UA_ServerConfig_addEndpoint(
                          config, securePolicyUri,
                          UA_MESSAGESECURITYMODE_SIGN),
                      UA_STATUSCODE_GOOD);
    UA_UsernamePasswordLogin login = {
        UA_STRING_STATIC("user"), UA_STRING_STATIC("password")};
    ck_assert_uint_eq(UA_AccessControl_default(
                          config, true, &UA_SECURITY_POLICY_NONE_URI, 1,
                          &login),
                      UA_STATUSCODE_GOOD);
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    trustList.trustedCertificates = &clientCertificate;
    trustList.trustedCertificatesSize = 1;
    UA_NodeId applicationGroup = UA_NS0ID(
        SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    ck_assert_uint_eq(UA_CertificateGroup_Memorystore(
                          &config->secureChannelPKI, &applicationGroup,
                          &trustList, config->logging, NULL),
                      UA_STATUSCODE_GOOD);
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.unconfigured.application");
    config->httpCertificate = serverCertificate;
    config->httpPrivateKey = serverPrivateKey;
    serverCertificate = UA_BYTESTRING_NULL;
    serverPrivateKey = UA_BYTESTRING_NULL;

    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String*)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.https://127.0.0.1:0/secure");
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_String advertisedUrl = UA_STRING_NULL;
    for(size_t i = 0;
        i < config->applicationDescription.discoveryUrlsSize; i++) {
        const UA_String *url = &config->applicationDescription.discoveryUrls[i];
        if(url->length < 12 || memcmp(url->data, "opc.https://", 12) != 0)
            continue;
        UA_String hostname = UA_STRING_NULL;
        UA_String urlPath = UA_STRING_NULL;
        ck_assert_uint_eq(UA_parseEndpointUrl(url, &hostname, &port, &urlPath),
                          UA_STATUSCODE_GOOD);
        const UA_String expectedPath = UA_STRING_STATIC("secure");
        if(UA_String_equal(&urlPath, &expectedPath)) {
            ck_assert_uint_eq(UA_String_copy(url, &advertisedUrl),
                              UA_STATUSCODE_GOOD);
            break;
        }
    }
    ck_assert_uint_ne(port, 0);

    /* Exercise the same policy through the public client, including the TLS
     * trust path and the logical SecureChannel CreateSession signatures. */
    char *publicEndpoint = (char *)UA_malloc(advertisedUrl.length + 1);
    ck_assert_ptr_nonnull(publicEndpoint);
    memcpy(publicEndpoint, advertisedUrl.data, advertisedUrl.length);
    publicEndpoint[advertisedUrl.length] = 0;
    UA_ClientConfig publicConfig;
    memset(&publicConfig, 0, sizeof(publicConfig));
    publicConfig.eventLoop = config->eventLoop;
    publicConfig.externalEventLoop = true;
    ck_assert_uint_eq(
        UA_ClientConfig_setDefaultEncryption(
            &publicConfig, clientCertificate, clientPrivateKey,
            &config->httpCertificate, 1, NULL, 0),
        UA_STATUSCODE_GOOD);
    publicConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    publicConfig.securityPolicyUri = UA_STRING_ALLOC(
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    ck_assert_ptr_nonnull(publicConfig.securityPolicyUri.data);
    ck_assert_uint_eq(UA_ClientConfig_setAuthenticationUsername(
                          &publicConfig, "user", "password"),
                      UA_STATUSCODE_GOOD);
    UA_String_clear(&publicConfig.clientDescription.applicationUri);
    publicConfig.clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.client.application");
    ck_assert_ptr_nonnull(publicConfig.clientDescription.applicationUri.data);
    ck_assert_uint_eq(UA_ByteString_copy(&config->httpCertificate,
                                         &publicConfig.httpCaCertificate),
                      UA_STATUSCODE_GOOD);
    UA_Client *publicClient = UA_Client_newWithConfig(&publicConfig);
    ck_assert_ptr_nonnull(publicClient);
    ck_assert_uint_eq(UA_Client_connect(publicClient, publicEndpoint),
                      UA_STATUSCODE_GOOD);
    UA_MessageSecurityMode publicMode = UA_MESSAGESECURITYMODE_INVALID;
    UA_QualifiedName modeKey = UA_QUALIFIEDNAME(0, "securityMode");
    ck_assert_uint_eq(UA_Client_getConnectionAttribute_scalar(
                          publicClient, modeKey,
                          &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE], &publicMode),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(publicMode,
                      UA_MESSAGESECURITYMODE_SIGNANDENCRYPT);
    UA_Variant publicValue;
    UA_Variant_init(&publicValue);
    ck_assert_uint_eq(UA_Client_readValueAttribute(
                          publicClient,
                          UA_NODEID_NUMERIC(
                              0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
                          &publicValue),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&publicValue,
                                      &UA_TYPES[UA_TYPES_DATETIME]));
    UA_Variant_clear(&publicValue);

    /* Replacing the trust list revalidates existing HTTP SecureChannels. The
     * rejected channel is torn down without requiring a physical socket close. */
    ck_assert_uint_eq(countAuthenticatedHttpSecureChannels(server), 1);
    ck_assert_uint_eq(UA_Server_addCertificates(
                          server, applicationGroup, NULL, 0, NULL, 0,
                          true, false),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0;
        i < 20 && countAuthenticatedHttpSecureChannels(server) != 0; i++)
        config->eventLoop->run(config->eventLoop, 1);
    ck_assert_uint_eq(countAuthenticatedHttpSecureChannels(server), 0);

    UA_Client_delete(publicClient);
    UA_free(publicEndpoint);

    /* Restore the client trust for the raw HTTPS service tests below. */
    ck_assert_uint_eq(UA_Server_addCertificates(
                          server, applicationGroup, &clientCertificate, 1,
                          NULL, 0, true, false),
                      UA_STATUSCODE_GOOD);
    config->eventLoop->run(config->eventLoop, 1);

    UA_ConnectionManager *http = getHttpConnectionManager(config);
    ck_assert_ptr_nonnull(http);
    openRawHttpClient(http, port, true, &config->httpCertificate);

    UA_SecurityPolicy clientPolicy;
    memset(&clientPolicy, 0, sizeof(clientPolicy));
    ck_assert_uint_eq(UA_SecurityPolicy_Basic256Sha256(
                          &clientPolicy, clientCertificate, clientPrivateKey,
                          UA_Log_Stdout),
                      UA_STATUSCODE_GOOD);
    void *clientChannelContext = NULL;
    ck_assert_uint_eq(clientPolicy.newChannelContext(
                          &clientPolicy, &config->httpCertificate,
                          &clientChannelContext),
                      UA_STATUSCODE_GOOD);

    UA_ByteString clientNonce = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&clientNonce, 32),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < clientNonce.length; i++)
        clientNonce.data[i] = (UA_Byte)(0xa0 + i);

    UA_CreateSessionRequest createRequest;
    UA_CreateSessionRequest_init(&createRequest);
    createRequest.requestHeader.timestamp = UA_DateTime_now();
    createRequest.requestHeader.requestHandle = 100;
    createRequest.clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;
    createRequest.clientDescription.applicationUri =
        UA_STRING_ALLOC("urn:open62541.client.application");
    createRequest.clientDescription.productUri =
        UA_STRING_ALLOC("urn:open62541:http-secure-test");
    createRequest.clientDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("en", "HTTPS secure client");
    ck_assert_uint_eq(UA_ByteString_copy(&clientCertificate,
                                         &createRequest.clientCertificate),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ByteString_copy(&clientNonce,
                                         &createRequest.clientNonce),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_String_copy(&advertisedUrl,
                                     &createRequest.endpointUrl),
                      UA_STATUSCODE_GOOD);
    createRequest.sessionName = UA_STRING_ALLOC("HTTPS secure session");
    createRequest.requestedSessionTimeout = 60000.0;
    UA_String path = UA_STRING("/secure");
    sendSecureServiceRequest(http, config->eventLoop, &path, &securePolicyUri,
                             &createRequest,
                             &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]);
    UA_CreateSessionRequest_clear(&createRequest);

    size_t offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], &offset);
    UA_CreateSessionResponse createResponse;
    UA_CreateSessionResponse_init(&createResponse);
    ck_assert_uint_eq(UA_decodeBinaryInternal(
                          &responseBody, &offset, &createResponse,
                          &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&createResponse.serverCertificate,
                                  &config->httpCertificate));
    ck_assert_uint_ge(createResponse.serverNonce.length, 32);

    UA_ByteString createSignatureData = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(
                          &createSignatureData,
                          clientCertificate.length + clientNonce.length),
                      UA_STATUSCODE_GOOD);
    memcpy(createSignatureData.data, clientCertificate.data,
           clientCertificate.length);
    memcpy(createSignatureData.data + clientCertificate.length,
           clientNonce.data, clientNonce.length);
    ck_assert_uint_eq(clientPolicy.asymSignatureAlgorithm.verify(
                          &clientPolicy, clientChannelContext,
                          &createSignatureData,
                          &createResponse.serverSignature.signature),
                      UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&createSignatureData);

    UA_String anonymousPolicyId = UA_STRING_NULL;
    for(size_t i = 0; i < createResponse.serverEndpointsSize; i++) {
        UA_EndpointDescription *endpoint = &createResponse.serverEndpoints[i];
        if(!UA_String_equal(&endpoint->securityPolicyUri, &securePolicyUri))
            continue;
        for(size_t j = 0; j < endpoint->userIdentityTokensSize; j++) {
            UA_UserTokenPolicy *token = &endpoint->userIdentityTokens[j];
            if(token->tokenType == UA_USERTOKENTYPE_ANONYMOUS &&
               anonymousPolicyId.length == 0)
                ck_assert_uint_eq(UA_String_copy(&token->policyId,
                                                 &anonymousPolicyId),
                                  UA_STATUSCODE_GOOD);
        }
    }
    ck_assert_uint_gt(anonymousPolicyId.length, 0);

    UA_ActivateSessionRequest activateRequest;
    UA_ActivateSessionRequest_init(&activateRequest);
    activateRequest.requestHeader.timestamp = UA_DateTime_now();
    activateRequest.requestHeader.requestHandle = 101;
    ck_assert_uint_eq(UA_NodeId_copy(&createResponse.authenticationToken,
                                     &activateRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    setLegacyClientSignature(&clientPolicy, clientChannelContext,
                             &createResponse.serverCertificate,
                             &createResponse.serverNonce,
                             &activateRequest.clientSignature);
    activateRequest.clientSignature.signature.data[0] ^= 0x01;
    UA_AnonymousIdentityToken anonymousToken;
    UA_AnonymousIdentityToken_init(&anonymousToken);
    ck_assert_uint_eq(UA_String_copy(&anonymousPolicyId,
                                     &anonymousToken.policyId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ExtensionObject_setValueCopy(
                          &activateRequest.userIdentityToken, &anonymousToken,
                          &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]),
                      UA_STATUSCODE_GOOD);
    UA_AnonymousIdentityToken_clear(&anonymousToken);
    sendSecureServiceRequest(http, config->eventLoop, &path, &securePolicyUri,
                             &activateRequest,
                             &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST]);
    UA_ActivateSessionRequest_clear(&activateRequest);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_SERVICEFAULT], &offset);
    UA_ServiceFault fault;
    UA_ServiceFault_init(&fault);
    ck_assert_uint_eq(UA_decodeBinaryInternal(
                          &responseBody, &offset, &fault,
                          &UA_TYPES[UA_TYPES_SERVICEFAULT], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(fault.responseHeader.serviceResult,
                      UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID);
    UA_ServiceFault_clear(&fault);

    UA_ActivateSessionRequest_init(&activateRequest);
    activateRequest.requestHeader.timestamp = UA_DateTime_now();
    activateRequest.requestHeader.requestHandle = 102;
    ck_assert_uint_eq(UA_NodeId_copy(&createResponse.authenticationToken,
                                     &activateRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    setLegacyClientSignature(&clientPolicy, clientChannelContext,
                             &createResponse.serverCertificate,
                             &createResponse.serverNonce,
                             &activateRequest.clientSignature);
    UA_AnonymousIdentityToken_init(&anonymousToken);
    ck_assert_uint_eq(UA_String_copy(&anonymousPolicyId,
                                     &anonymousToken.policyId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_ExtensionObject_setValueCopy(
                          &activateRequest.userIdentityToken, &anonymousToken,
                          &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]),
                      UA_STATUSCODE_GOOD);
    UA_AnonymousIdentityToken_clear(&anonymousToken);
    sendSecureServiceRequest(http, config->eventLoop, &path, &securePolicyUri,
                             &activateRequest,
                             &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST]);
    UA_ActivateSessionRequest_clear(&activateRequest);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], &offset);
    UA_ActivateSessionResponse activateResponse;
    UA_ActivateSessionResponse_init(&activateResponse);
    ck_assert_uint_eq(UA_decodeBinaryInternal(
                          &responseBody, &offset, &activateResponse,
                          &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(activateResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_ActivateSessionResponse_clear(&activateResponse);

    UA_ReadRequest readRequest;
    UA_ReadRequest_init(&readRequest);
    readRequest.requestHeader.timestamp = UA_DateTime_now();
    readRequest.requestHeader.requestHandle = 103;
    ck_assert_uint_eq(UA_NodeId_copy(&createResponse.authenticationToken,
                                     &readRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    readRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    readRequest.nodesToRead =
        (UA_ReadValueId*)UA_Array_new(1, &UA_TYPES[UA_TYPES_READVALUEID]);
    ck_assert_ptr_nonnull(readRequest.nodesToRead);
    readRequest.nodesToReadSize = 1;
    readRequest.nodesToRead[0].nodeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    readRequest.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;
    sendSecureServiceRequest(http, config->eventLoop, &path, &securePolicyUri,
                             &readRequest, &UA_TYPES[UA_TYPES_READREQUEST]);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_READRESPONSE], &offset);
    UA_ReadResponse readResponse;
    UA_ReadResponse_init(&readResponse);
    ck_assert_uint_eq(UA_decodeBinaryInternal(
                          &responseBody, &offset, &readResponse,
                          &UA_TYPES[UA_TYPES_READRESPONSE], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(readResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_ReadResponse_clear(&readResponse);

    const UA_String nonePolicyUri = UA_SECURITY_POLICY_NONE_URI;
    sendSecureServiceRequest(http, config->eventLoop, &path, &nonePolicyUri,
                             &readRequest, &UA_TYPES[UA_TYPES_READREQUEST]);
    UA_ReadRequest_clear(&readRequest);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_SERVICEFAULT], &offset);
    UA_ServiceFault_init(&fault);
    ck_assert_uint_eq(UA_decodeBinaryInternal(
                          &responseBody, &offset, &fault,
                          &UA_TYPES[UA_TYPES_SERVICEFAULT], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(fault.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_ServiceFault_clear(&fault);

    UA_CloseSessionRequest closeRequest;
    UA_CloseSessionRequest_init(&closeRequest);
    closeRequest.requestHeader.timestamp = UA_DateTime_now();
    closeRequest.requestHeader.requestHandle = 104;
    ck_assert_uint_eq(UA_NodeId_copy(&createResponse.authenticationToken,
                                     &closeRequest.requestHeader.authenticationToken),
                      UA_STATUSCODE_GOOD);
    closeRequest.deleteSubscriptions = true;
    sendSecureServiceRequest(http, config->eventLoop, &path, &securePolicyUri,
                             &closeRequest,
                             &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST]);
    UA_CloseSessionRequest_clear(&closeRequest);
    offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE], &offset);
    UA_CloseSessionResponse closeResponse;
    UA_CloseSessionResponse_init(&closeResponse);
    ck_assert_uint_eq(UA_decodeBinaryInternal(
                          &responseBody, &offset, &closeResponse,
                          &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeResponse.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_CloseSessionResponse_clear(&closeResponse);
    lockServer(server);
    ck_assert_ptr_null(getSessionByToken(
        server, &createResponse.authenticationToken));
    unlockServer(server);

    UA_CreateSessionResponse_clear(&createResponse);
    UA_String_clear(&anonymousPolicyId);
    UA_String_clear(&advertisedUrl);
    UA_ByteString_clear(&clientNonce);
    UA_ByteString_clear(&responseBody);
    clientPolicy.deleteChannelContext(&clientPolicy, clientChannelContext);
    clientPolicy.clear(&clientPolicy);
    UA_ByteString_clear(&clientCertificate);
    UA_ByteString_clear(&clientPrivateKey);
    UA_StatusCode closeStatus =
        http->closeConnection(http, clientConnectionId);
    ck_assert(closeStatus == UA_STATUSCODE_GOOD ||
              closeStatus == UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST
#endif

START_TEST(unencryptedHttpRequiresExplicitOptIn) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.http://localhost:0/ua");
    ck_assert_uint_eq(UA_Server_run_startup(server),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    UA_Server_delete(server);
}
END_TEST

START_TEST(binaryServiceOverUnencryptedHttp) {
    clientConnectionId = 0;
    responseBody = UA_BYTESTRING_NULL;
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpAllowUnencrypted = true;
    config->httpListenAddress = UA_STRING_ALLOC("");
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.http://localhost:0/ua");
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    UA_String advertisedUrl = UA_STRING_NULL;
    UA_UInt16 port = getAdvertisedPort(config, "opc.http://", 11,
                                       &advertisedUrl);
    ck_assert_uint_ne(port, 0);
    UA_ConnectionManager *http = getHttpConnectionManager(config);
    ck_assert_ptr_nonnull(http);
    openRawHttpClient(http, port, false, NULL);

    UA_FindServersRequest request;
    UA_FindServersRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.requestHandle = 201;
    UA_String path = UA_STRING("/ua");
    sendServiceRequest(http, config->eventLoop, &path, &request,
                       &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    size_t offset = 0;
    decodeResponseType(&UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE], &offset);
    UA_FindServersResponse response;
    UA_FindServersResponse_init(&response);
    ck_assert_uint_eq(UA_decodeBinaryInternal(
                          &responseBody, &offset, &response,
                          &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE], NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.responseHeader.requestHandle, 201);
    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    UA_FindServersResponse_clear(&response);

    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    lockServer(server);
    ck_assert_uint_eq(setCurrentEndpointsArray(server, advertisedUrl, NULL, 0,
                                               &endpoints, &endpointsSize),
                      UA_STATUSCODE_GOOD);
    unlockServer(server);
    const UA_String binaryProfile = UA_STRING_STATIC(
        "http://open62541.org/UA-Profile/Transport/http-uabinary");
    UA_Boolean foundBinaryProfile = false;
    for(size_t i = 0; i < endpointsSize; i++) {
        if(UA_String_equal(&endpoints[i].transportProfileUri, &binaryProfile))
            foundBinaryProfile = true;
    }
    ck_assert(foundBinaryProfile);
    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    UA_FindServersRequest_clear(&request);
    UA_String_clear(&advertisedUrl);
    UA_ByteString_clear(&responseBody);
    UA_StatusCode closeStatus = http->closeConnection(http, clientConnectionId);
    ck_assert(closeStatus == UA_STATUSCODE_GOOD ||
              closeStatus == UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

#ifdef UA_ENABLE_ENCRYPTION
START_TEST(unencryptedHttpPasswordNeedsIndependentOptIn) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpAllowUnencrypted = true;
    config->httpListenAddress = UA_STRING_ALLOC("");

    UA_ByteString certificate = loadFile("server_cert.der");
    UA_ByteString privateKey = loadFile("server_key.der");
    ck_assert_uint_eq(UA_ServerConfig_addSecurityPolicyBasic256Sha256(
                          config, &certificate, &privateKey),
                      UA_STATUSCODE_GOOD);
    UA_UsernamePasswordLogin login = {
        UA_STRING_STATIC("user"), UA_STRING_STATIC("password")};
    ck_assert_uint_eq(UA_AccessControl_default(
                          config, true, &UA_SECURITY_POLICY_NONE_URI, 1,
                          &login),
                      UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);

    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.http://localhost:0/ua");
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    UA_String advertisedUrl = UA_STRING_NULL;
    ck_assert_uint_ne(getAdvertisedPort(config, "opc.http://", 11,
                                        &advertisedUrl),
                      0);
    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    lockServer(server);
    ck_assert_uint_eq(setCurrentEndpointsArray(server, advertisedUrl, NULL, 0,
                                               &endpoints, &endpointsSize),
                      UA_STATUSCODE_GOOD);
    unlockServer(server);
    UA_Boolean foundEncryptedUsername = false;
    for(size_t i = 0; i < endpointsSize; i++) {
        for(size_t j = 0; j < endpoints[i].userIdentityTokensSize; j++) {
            UA_UserTokenPolicy *utp = &endpoints[i].userIdentityTokens[j];
            if(utp->tokenType != UA_USERTOKENTYPE_USERNAME)
                continue;
            ck_assert_uint_gt(utp->securityPolicyUri.length, 0);
            ck_assert(!UA_String_equal(&utp->securityPolicyUri,
                                       &UA_SECURITY_POLICY_NONE_URI));
            foundEncryptedUsername = true;
        }
    }
    ck_assert(foundEncryptedUsername);
    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    /* Plaintext passwords require their own, independent opt-in. */
    config->allowNonePolicyPassword = true;
    endpoints = NULL;
    endpointsSize = 0;
    lockServer(server);
    ck_assert_uint_eq(setCurrentEndpointsArray(server, advertisedUrl, NULL, 0,
                                               &endpoints, &endpointsSize),
                      UA_STATUSCODE_GOOD);
    unlockServer(server);
    UA_Boolean foundPlainUsername = false;
    for(size_t i = 0; i < endpointsSize; i++) {
        for(size_t j = 0; j < endpoints[i].userIdentityTokensSize; j++) {
            UA_UserTokenPolicy *utp = &endpoints[i].userIdentityTokens[j];
            if(utp->tokenType == UA_USERTOKENTYPE_USERNAME &&
               utp->securityPolicyUri.length == 0)
                foundPlainUsername = true;
        }
    }
    ck_assert(foundPlainUsername);
    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_String_clear(&advertisedUrl);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST
#endif

START_TEST(emptyAdvertisedHostnameRejected) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpCertificate = loadFile("server_cert.der");
    config->httpPrivateKey = loadFile("server_key.der");
    config->httpListenAddress = UA_STRING_ALLOC("");
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.https://:0/ua");

    ck_assert_uint_eq(UA_Server_run_startup(server),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    UA_Server_delete(server);
}
END_TEST

START_TEST(duplicateHttpSchemeRejected) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpAllowUnencrypted = true;
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 2;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.http://localhost:0/one");
    config->serverUrls[1] = UA_STRING_ALLOC("opc.http://localhost:0/two");

    ck_assert_uint_eq(UA_Server_run_startup(server),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    UA_Server_delete(server);
}
END_TEST

START_TEST(httpAndHttpsListenersCoexist) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpAllowUnencrypted = true;
    config->httpCertificate = loadFile("server_cert.der");
    config->httpPrivateKey = loadFile("server_key.der");
    config->httpListenAddress = UA_STRING_ALLOC("");
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 2;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.http://localhost:0/plain");
    config->serverUrls[1] = UA_STRING_ALLOC("opc.https://localhost:0/secure");

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_UInt16 httpPort =
        getAdvertisedPort(config, "opc.http://", 11, NULL);
    UA_UInt16 httpsPort =
        getAdvertisedPort(config, "opc.https://", 12, NULL);
    ck_assert_uint_ne(httpPort, 0);
    ck_assert_uint_ne(httpsPort, 0);

    UA_ConnectionManager *http = getHttpConnectionManager(config);
    ck_assert_ptr_nonnull(http);
    UA_FindServersRequest request;
    UA_FindServersRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    UA_String plainPath = UA_STRING("/plain");
    UA_String securePath = UA_STRING("/secure");

    openRawHttpClient(http, httpPort, false, NULL);
    sendServiceRequest(http, config->eventLoop, &plainPath, &request,
                       &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    sendInvalidRequest(http, config->eventLoop, &securePath,
                       "application/octet-stream", NULL, "invalid", 404);
    ck_assert_uint_eq(http->closeConnection(http, clientConnectionId),
                      UA_STATUSCODE_GOOD);

    openRawHttpClient(http, httpsPort, true, &config->httpCertificate);
    sendServiceRequest(http, config->eventLoop, &securePath, &request,
                       &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    sendInvalidRequest(http, config->eventLoop, &plainPath,
                       "application/octet-stream", NULL, "invalid", 404);
    ck_assert_uint_eq(http->closeConnection(http, clientConnectionId),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

static UA_Server *
newHttpsServerForPath(const char *url) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpCertificate = loadFile("server_cert.der");
    config->httpPrivateKey = loadFile("server_key.der");
    config->httpListenAddress = UA_STRING_ALLOC("");
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_ptr_nonnull(config->serverUrls);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC(url);
    return server;
}

START_TEST(httpEndpointPathsAreCanonical) {
    UA_Server *server =
        newHttpsServerForPath("opc.https://localhost:0/");
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_String url = UA_STRING_NULL;
    UA_UInt16 port = getAdvertisedPort(config, "opc.https://", 12, &url);
    ck_assert_uint_ne(port, 0);
    ck_assert_uint_gt(url.length, 0);
    ck_assert_uint_eq(url.data[url.length - 1], '/');

    /* A canonical root endpoint accepts both its direct and conventional
     * discovery routes. JSON is a separate route derived from the same
     * endpoint instead of separately stored endpoint state. */
    UA_ConnectionManager *http = getHttpConnectionManager(config);
    ck_assert_ptr_nonnull(http);
    openRawHttpClient(http, port, true, &config->httpCertificate);
    UA_FindServersRequest request;
    UA_FindServersRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    UA_String rootPath = UA_STRING("/");
    UA_String rootDiscoveryPath = UA_STRING("/discovery");
    sendServiceRequest(http, config->eventLoop, &rootPath, &request,
                       &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    sendServiceRequest(http, config->eventLoop, &rootDiscoveryPath, &request,
                       &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
#ifdef UA_ENABLE_JSON_ENCODING
    UA_String rootJsonPath = UA_STRING("/json");
    UA_String rootJsonDiscoveryPath = UA_STRING("/json/discovery");
    sendServiceRequestJson(http, config->eventLoop, &rootJsonPath, &request,
                           &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    sendServiceRequestJson(http, config->eventLoop,
                           &rootJsonDiscoveryPath, &request,
                           &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
#endif
    ck_assert_uint_eq(http->closeConnection(http, clientConnectionId),
                      UA_STATUSCODE_GOOD);

    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    lockServer(server);
    ck_assert_uint_eq(setCurrentEndpointsArray(server, url, NULL, 0,
                                               &endpoints, &endpointsSize),
                      UA_STATUSCODE_GOOD);
    unlockServer(server);
#ifdef UA_ENABLE_JSON_ENCODING
    UA_Boolean foundRootJson = false;
    const UA_String jsonSuffix = UA_STRING_STATIC("/json");
    for(size_t i = 0; i < endpointsSize; i++) {
        UA_String *endpointUrl = &endpoints[i].endpointUrl;
        if(endpointUrl->length >= jsonSuffix.length &&
           memcmp(&endpointUrl->data[endpointUrl->length - jsonSuffix.length],
                  jsonSuffix.data, jsonSuffix.length) == 0)
            foundRootJson = true;
    }
    ck_assert(foundRootJson);

    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    endpoints = NULL;
    endpointsSize = 0;
    const UA_String mirroredJson =
        UA_STRING_STATIC("opc.https://mirror.invalid:1234/json");
    lockServer(server);
    ck_assert_uint_eq(setCurrentEndpointsArray(server, mirroredJson, NULL, 0,
                                               &endpoints, &endpointsSize),
                      UA_STATUSCODE_GOOD);
    unlockServer(server);
    UA_Boolean foundMirroredBinaryRoot = false;
    const UA_String mirroredBinary =
        UA_STRING_STATIC("opc.https://mirror.invalid:1234/");
    for(size_t i = 0; i < endpointsSize; i++) {
        if(UA_String_equal(&endpoints[i].endpointUrl, &mirroredBinary))
            foundMirroredBinaryRoot = true;
    }
    ck_assert(foundMirroredBinaryRoot);
#endif
    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_String_clear(&url);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);

    server = newHttpsServerForPath("opc.https://localhost:0/factory/");
    config = UA_Server_getConfig(server);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    port = getAdvertisedPort(config, "opc.https://", 12, &url);
    ck_assert_uint_ne(port, 0);
    const UA_String factoryPath = UA_STRING_STATIC("/factory");
    ck_assert_uint_ge(url.length, factoryPath.length);
    ck_assert_mem_eq(&url.data[url.length - factoryPath.length],
                     factoryPath.data, factoryPath.length);

    http = getHttpConnectionManager(config);
    ck_assert_ptr_nonnull(http);
    openRawHttpClient(http, port, true, &config->httpCertificate);
    sendServiceRequest(http, config->eventLoop, &factoryPath, &request,
                       &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    UA_String factoryDiscoveryPath = UA_STRING("/factory/discovery");
    sendServiceRequest(http, config->eventLoop, &factoryDiscoveryPath,
                       &request, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
#ifdef UA_ENABLE_JSON_ENCODING
    UA_String factoryJsonPath = UA_STRING("/factory/json");
    UA_String factoryJsonDiscoveryPath =
        UA_STRING("/factory/json/discovery");
    sendServiceRequestJson(http, config->eventLoop, &factoryJsonPath, &request,
                           &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
    sendServiceRequestJson(http, config->eventLoop,
                           &factoryJsonDiscoveryPath, &request,
                           &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]);
#endif
    ck_assert_uint_eq(http->closeConnection(http, clientConnectionId),
                      UA_STATUSCODE_GOOD);
    UA_String_clear(&url);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);

    server = newHttpsServerForPath("opc.https://localhost:0/factory/json");
    ck_assert_uint_eq(UA_Server_run_startup(server),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    UA_Server_delete(server);
}
END_TEST

static int
runPythonHttpServer(const char *portString, const char *completionMarker) {
    unsigned long parsedPort = strtoul(portString, NULL, 10);
    if(parsedPort == 0 || parsedPort > 65535)
        return EXIT_FAILURE;
    char url[128];
    int urlLength = snprintf(url, sizeof(url),
                             "opc.http://localhost:%lu/ua", parsedPort);
    if(urlLength <= 0 || (size_t)urlLength >= sizeof(url))
        return EXIT_FAILURE;

    UA_Server *server = UA_Server_new();
    if(!server)
        return EXIT_FAILURE;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->tcpEnabled = false;
    config->httpEnabled = true;
    config->httpAllowUnencrypted = true;
    config->httpListenAddress = UA_STRING_ALLOC("127.0.0.1");
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls =
        (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    if(!config->serverUrls) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC(url);
    UA_StatusCode res = UA_Server_run_startup(server);
    if(res == UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < 5000; i++) {
            config->eventLoop->run(config->eventLoop, 10);
            FILE *marker = fopen(completionMarker, "rb");
            if(marker) {
                fclose(marker);
                break;
            }
        }
        res = UA_Server_run_shutdown(server);
    }
    UA_Server_delete(server);
    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char **argv) {
    if(argc == 4 && strcmp(argv[1], "--python-http-server") == 0)
        return runPythonHttpServer(argv[2], argv[3]);
    Suite *suite = suite_create("OPC UA HTTP server");
    TCase *tc = tcase_create("binary services");
    tcase_set_timeout(tc, 30);
    tcase_add_test(tc, binaryServiceOverHttps);
    tcase_add_test(tc, unencryptedHttpRequiresExplicitOptIn);
    tcase_add_test(tc, binaryServiceOverUnencryptedHttp);
    tcase_add_test(tc, oversizedServerResponseReturnsServiceFault);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc, securePolicyServiceOverHttps);
    tcase_add_test(tc, unencryptedHttpPasswordNeedsIndependentOptIn);
#endif
    tcase_add_test(tc, emptyAdvertisedHostnameRejected);
    tcase_add_test(tc, duplicateHttpSchemeRejected);
    tcase_add_test(tc, httpAndHttpsListenersCoexist);
    tcase_add_test(tc, httpEndpointPathsAreCanonical);
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
