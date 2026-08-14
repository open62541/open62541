/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/types.h"
#include "open62541/types_generated.h"
#include "open62541/util.h"
#include "eventloop_posix_http_compression.h"

#include "testing_clock.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <check.h>

#ifndef _WIN32
# include <arpa/inet.h>
# include <errno.h>
# include <sys/socket.h>
# include <unistd.h>
#endif

#include "testing-plugins/testing_clock.h"

#define COUNT 2

unsigned int messageCount = 0;
static UA_UInt16 httpPort = 8000;

static void
setup(void) {
    messageCount = 0;
    const char *portString = getenv("OPEN62541_TEST_HTTP_PORT");
    if(!portString)
        return;

    unsigned long port = strtoul(portString, NULL, 10);
    ck_assert_uint_gt(port, 0);
    ck_assert_uint_le(port, UINT16_MAX);
    httpPort = (UA_UInt16)port;
}

static void teardown(void) {}

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

static void
stopEventLoop(UA_EventLoop *eventLoop) {
    eventLoop->stop(eventLoop);
    for(size_t i = 0; i < 200 &&
        eventLoop->state != UA_EVENTLOOPSTATE_STOPPED; i++)
        eventLoop->run(eventLoop, 20);
    ck_assert_uint_eq(eventLoop->state, UA_EVENTLOOPSTATE_STOPPED);
    eventLoop->free(eventLoop);
}


static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void *application, void **connectionContext,
                   UA_ConnectionState status,
                   const UA_KeyValueMap *params,
                   UA_ByteString msg) {
    const UA_UInt64 *contentLength;
    if(params) {
        contentLength = (const UA_UInt64 *)
            UA_KeyValueMap_getScalar(params,
                                     UA_QUALIFIEDNAME(0, "content-length"),
                                     &UA_TYPES[UA_TYPES_UINT64]);
    }

    if(status == UA_CONNECTIONSTATE_CLOSED) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "USER\t| Closing connection");
        messageCount = 0;
    } else if(msg.length > 0) {
        static UA_UInt64 consumed = 0;
        char data[msg.length + 1];
        memcpy(data, msg.data, msg.length);
        data[msg.length] = '\0';
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "USER %lu\t| Received Data %zu bytes: %s", connectionId, msg.length, data);

        consumed += msg.length;
        if(consumed >= *contentLength) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "USER %lu\t| End of Response", connectionId);
            consumed = 0;
            messageCount++;
        }
    } else if(status == UA_CONNECTIONSTATE_OPENING) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "USER\t| Opening connection");
        uintptr_t *id = *(uintptr_t**)connectionContext;
        *id = connectionId;
    } else if(status == UA_CONNECTIONSTATE_ESTABLISHED) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "USER\t| Established connection");
    }
}

START_TEST(sendGetRequest) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("httpCM"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    el->registerEventSource(el, &cm->eventSource);
    el->start(el);

    UA_String address = UA_STRING("127.0.0.1");
    UA_UInt16 port = httpPort;
    UA_Boolean useSSL = false;
    UA_UInt16 timeout = 30;

    size_t paramsSize = 4;
    UA_KeyValuePair params[paramsSize];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[2].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&params[2].value, &useSSL, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[3].key = UA_QUALIFIEDNAME(0, "timeout");
    UA_Variant_setScalar(&params[3].value, &timeout, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap kvm = {paramsSize, params};

    uintptr_t connectionIds[COUNT] = {0};
    UA_StatusCode res =
        cm->openConnection(cm, &kvm, NULL, &connectionIds[0], connectionCallback);
    ck_assert(res == UA_STATUSCODE_GOOD);

    /* A second connection on the shared context can override its timeout. */
    timeout = 15;
    res = cm->openConnection(cm, &kvm, NULL, &connectionIds[1], connectionCallback);
    ck_assert(res == UA_STATUSCODE_GOOD);

    for(int i = 0; i < COUNT; i++) {
        res = cm->sendWithConnection(cm, connectionIds[i],
                                     &UA_KEYVALUEMAP_NULL, NULL);
        ck_assert(res == UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(messageCount, 0);
    for(int i = 0; i < 20; i++) {
        el->run(el,100);
    }
    ck_assert_uint_eq(messageCount, COUNT);

    for(int i = 0; i < COUNT; i++) {
        res = cm->closeConnection(cm, connectionIds[i]);
        ck_assert(res == UA_STATUSCODE_GOOD);
    }

    for(int i = 0; i < 20; i++) {
        el->run(el,100);
    }

    /* Stop the EventLoop */
    int max_stop_iteration_count = 20;
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

START_TEST(sendPostRequest) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("httpCM"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    el->registerEventSource(el, &cm->eventSource);
    el->start(el);

    UA_String address = UA_STRING("127.0.0.1");
    UA_UInt16 port = httpPort;
    UA_Boolean useSSL = false;

    size_t paramsSize = 3;
    UA_KeyValuePair params[paramsSize];
    params[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&params[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[2].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&params[2].value, &useSSL, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap kvm = {paramsSize, params};

    UA_String path = UA_STRING("/post");
    UA_String method = UA_STRING("POST");

    size_t sendParamsSize = 3;
    UA_KeyValuePair sendParams[sendParamsSize];
    sendParams[0].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&sendParams[0].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    sendParams[1].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&sendParams[1].value, &method, &UA_TYPES[UA_TYPES_STRING]);
    UA_UInt32 requestHandle = 0;
    sendParams[2].key = UA_QUALIFIEDNAME(0, "handle");
    UA_Variant_setScalar(&sendParams[2].value, &requestHandle,
                         &UA_TYPES[UA_TYPES_UINT32]);

    UA_KeyValueMap sendKvm = {sendParamsSize, sendParams};

    uintptr_t connectionId = 0;
    UA_StatusCode res = cm->openConnection(cm, &kvm, NULL, &connectionId,
                                           connectionCallback);
    ck_assert(res == UA_STATUSCODE_GOOD);

    for(int i = 0; i < COUNT; i++) {
        requestHandle = (UA_UInt32)i;
        UA_ByteString msg = UA_BYTESTRING_ALLOC("text=hallo&send=data");
        res = cm->sendWithConnection(cm, connectionId, &sendKvm, &msg);
        ck_assert(res == UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(messageCount, 0);
    for(int i = 0; i < 20; i++) {
        el->run(el,100);
    }
    ck_assert_uint_eq(messageCount, COUNT);

    res = cm->closeConnection(cm, connectionId);
    ck_assert(res == UA_STATUSCODE_GOOD);

    for(int i = 0; i < 20; i++) {
        el->run(el,100);
    }

    /* Stop the EventLoop */
    int max_stop_iteration_count = 20;
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

typedef struct {
    uintptr_t listenerId;
    uintptr_t clientId;
    uintptr_t acceptedConnectionId;
    UA_UInt16 port;
    UA_Boolean requestReceived;
    UA_Boolean responseReceived;
    UA_Boolean responseHeaderReceived;
    UA_UInt16 responseStatus;
} HTTPServerTestContext;

static void
serverConnectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                         void *application, void **connectionContext,
                         UA_ConnectionState state,
                         const UA_KeyValueMap *params, UA_ByteString msg) {
    (void)connectionContext;
    HTTPServerTestContext *ctx = (HTTPServerTestContext*)application;
    const UA_UInt16 *listenPort = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "listen-port"), &UA_TYPES[UA_TYPES_UINT16]);
    if(listenPort) {
        ctx->listenerId = connectionId;
        ctx->port = *listenPort;
        return;
    }

    if(state == UA_CONNECTIONSTATE_OPENING) {
        ctx->clientId = connectionId;
        return;
    }

    const UA_String *method = (const UA_String*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "method"), &UA_TYPES[UA_TYPES_STRING]);
    const UA_String *path = (const UA_String*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "path"), &UA_TYPES[UA_TYPES_STRING]);
    if(method && path && connectionId != ctx->clientId) {
        const UA_Byte expected[] = {0x00, 0x01, 0x7f, 0xff};
        const UA_String expectedMethod = UA_STRING_STATIC("POST");
        const UA_String expectedPath = UA_STRING_STATIC("/binary");
        ck_assert(UA_String_equal(method, &expectedMethod));
        ck_assert(UA_String_equal(path, &expectedPath));
        ck_assert_uint_eq(msg.length, sizeof(expected));
        ck_assert_mem_eq(msg.data, expected, sizeof(expected));
        const UA_Variant *headerValue = UA_KeyValueMap_get(
            params, UA_QUALIFIEDNAME(0, "headers"));
        ck_assert_ptr_nonnull(headerValue);
        ck_assert_ptr_eq(headerValue->type, &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
        UA_KeyValuePair *requestHeaders = (UA_KeyValuePair*)headerValue->data;
        UA_Boolean foundSecurityPolicy = false;
        for(size_t i = 0; i < headerValue->arrayLength; i++) {
            const UA_String expectedHeader =
                UA_STRING_STATIC("opcua-securitypolicy");
            if(!UA_String_equal(&requestHeaders[i].key.name, &expectedHeader))
                continue;
            ck_assert(UA_Variant_hasScalarType(
                &requestHeaders[i].value, &UA_TYPES[UA_TYPES_STRING]));
            const UA_String expectedValue = UA_STRING_STATIC(
                "http://opcfoundation.org/UA/SecurityPolicy#None");
            ck_assert(UA_String_equal(
                (UA_String*)requestHeaders[i].value.data, &expectedValue));
            foundSecurityPolicy = true;
        }
        ck_assert(foundSecurityPolicy);
        ctx->acceptedConnectionId = connectionId;
        ctx->requestReceived = true;

        UA_String contentType = UA_STRING("application/octet-stream");
        UA_KeyValuePair headers[1] = {0};
        headers[0].key = UA_QUALIFIEDNAME(0, "content-type");
        UA_Variant_setScalar(&headers[0].value, &contentType,
                             &UA_TYPES[UA_TYPES_STRING]);
        UA_UInt16 statusCode = 201;
        UA_KeyValuePair responseParameters[2] = {0};
        responseParameters[0].key = UA_QUALIFIEDNAME(0, "status-code");
        UA_Variant_setScalar(&responseParameters[0].value, &statusCode,
                             &UA_TYPES[UA_TYPES_UINT16]);
        responseParameters[1].key = UA_QUALIFIEDNAME(0, "headers");
        UA_Variant_setArray(&responseParameters[1].value, headers, 1,
                            &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
        UA_KeyValueMap responseMap = {2, responseParameters};
        UA_ByteString response = UA_BYTESTRING_ALLOC("binary-response");
        ck_assert_uint_eq(cm->sendWithConnection(cm, connectionId, &responseMap,
                                                 &response), UA_STATUSCODE_GOOD);
        return;
    }

    if(connectionId == ctx->clientId) {
        const UA_UInt16 *statusCode =
            (const UA_UInt16*)UA_KeyValueMap_getScalar(
                params, UA_QUALIFIEDNAME(0, "status-code"),
                &UA_TYPES[UA_TYPES_UINT16]);
        if(statusCode)
            ctx->responseStatus = *statusCode;
        const UA_Variant *responseHeaders = UA_KeyValueMap_get(
            params, UA_QUALIFIEDNAME(0, "headers"));
        if(responseHeaders) {
            UA_KeyValuePair *headers =
                (UA_KeyValuePair*)responseHeaders->data;
            for(size_t i = 0; i < responseHeaders->arrayLength; i++) {
                const UA_String contentTypeName =
                    UA_STRING_STATIC("content-type");
                if(UA_String_equal(&headers[i].key.name, &contentTypeName))
                    ctx->responseHeaderReceived = true;
            }
        }
    }

    if(connectionId == ctx->clientId && msg.length > 0) {
        ck_assert_uint_eq(msg.length, strlen("binary-response"));
        ck_assert_mem_eq(msg.data, "binary-response", msg.length);
        ctx->responseReceived = true;
    }
}

START_TEST(serverBinaryRoundtrip) {
    HTTPServerTestContext ctx = {0};
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("httpCM"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm);
    ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);

    UA_String address = UA_STRING("127.0.0.1");
    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_KeyValuePair listenerParameters[3] = {0};
    listenerParameters[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&listenerParameters[0].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    listenerParameters[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&listenerParameters[1].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    listenerParameters[2].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&listenerParameters[2].value, &listen,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap listenerMap = {3, listenerParameters};
    ck_assert_uint_eq(cm->openConnection(cm, &listenerMap, &ctx, NULL,
                                         serverConnectionCallback),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(ctx.listenerId, 0);
    ck_assert_uint_ne(ctx.port, 0);

    port = ctx.port;
    UA_KeyValuePair clientParameters[2] = {0};
    clientParameters[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&clientParameters[0].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    clientParameters[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&clientParameters[1].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap clientMap = {2, clientParameters};
    ck_assert_uint_eq(cm->openConnection(cm, &clientMap, &ctx, NULL,
                                         serverConnectionCallback),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(ctx.clientId, 0);

    UA_String method = UA_STRING("POST");
    UA_String path = UA_STRING("/binary");
    UA_String securityPolicy = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_KeyValuePair headers[1] = {0};
    headers[0].key = UA_QUALIFIEDNAME(0, "opcua-securitypolicy");
    UA_Variant_setScalar(&headers[0].value, &securityPolicy,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValuePair sendParameters[3] = {0};
    sendParameters[0].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&sendParameters[0].value, &method,
                         &UA_TYPES[UA_TYPES_STRING]);
    sendParameters[1].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&sendParameters[1].value, &path,
                         &UA_TYPES[UA_TYPES_STRING]);
    sendParameters[2].key = UA_QUALIFIEDNAME(0, "headers");
    UA_Variant_setArray(&sendParameters[2].value, headers, 1,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    UA_KeyValueMap sendMap = {3, sendParameters};
    const UA_Byte requestData[] = {0x00, 0x01, 0x7f, 0xff};
    UA_ByteString request = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&request, sizeof(requestData)),
                      UA_STATUSCODE_GOOD);
    memcpy(request.data, requestData, sizeof(requestData));
    ck_assert_uint_eq(cm->sendWithConnection(cm, ctx.clientId, &sendMap, &request),
                      UA_STATUSCODE_GOOD);

    for(size_t i = 0; i < 200 && !ctx.responseReceived; i++)
        eventLoop->run(eventLoop, 20);
    ck_assert(ctx.requestReceived);
    ck_assert(ctx.responseReceived);
    ck_assert_uint_eq(ctx.responseStatus, 201);
    ck_assert(ctx.responseHeaderReceived);

    UA_StatusCode closeStatus = cm->closeConnection(cm, ctx.clientId);
    ck_assert(closeStatus == UA_STATUSCODE_GOOD ||
              closeStatus == UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    eventLoop->stop(eventLoop);
    for(size_t i = 0; i < 100 &&
        eventLoop->state != UA_EVENTLOOPSTATE_STOPPED; i++)
        eventLoop->run(eventLoop, 20);
    ck_assert_uint_eq(eventLoop->state, UA_EVENTLOOPSTATE_STOPPED);
    eventLoop->free(eventLoop);
} END_TEST

typedef enum {
    HTTP_TEST_ECHO,
    HTTP_TEST_EMPTY_RESPONSE,
    HTTP_TEST_NO_RESPONSE,
    HTTP_TEST_DUPLICATE_RESPONSE,
    HTTP_TEST_CLOSE_CONNECTION,
    HTTP_TEST_CLOSE_LISTENER,
    HTTP_TEST_FIXED_RESPONSE,
    HTTP_TEST_ONE_REQUEST_FAILS,
    HTTP_TEST_ONE_REQUEST_STALLS,
    HTTP_TEST_DUPLICATE_ENCODING_RESPONSE
} HTTPTestMode;

typedef struct {
    uintptr_t listenerId;
    uintptr_t clientId;
    uintptr_t acceptedConnectionId;
    uintptr_t stalledConnectionId;
    UA_UInt16 port;
    HTTPTestMode mode;
    size_t requestCount;
    size_t acceptedConnectionCount;
    size_t acceptedClosingCount;
    size_t clientClosingCount;
    size_t responseCount;
    size_t responseCompleteCount;
    size_t scalarHandleCallbacks;
    size_t arrayHandleCallbacks;
    size_t serverBodyBytes;
    size_t clientBodyBytes;
    size_t fixedResponseSize;
    UA_Boolean bodyMismatch;
    UA_Boolean responseHeaderReceived;
    UA_StatusCode firstSendStatus;
    UA_StatusCode secondSendStatus;
    UA_StatusCode sendAfterCloseStatus;
    UA_UInt16 responseStatus;
    UA_StatusCode lastRequestStatus;
    size_t goodRequestCompletions;
    size_t badRequestCompletions;
} HTTPTestContext;

static void
fillPattern(UA_Byte *data, size_t length, size_t offset) {
    for(size_t i = 0; i < length; i++)
        data[i] = (UA_Byte)((offset + i) % 251);
}

static void
advancedHTTPCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                     void *application, void **connectionContext,
                     UA_ConnectionState state,
                     const UA_KeyValueMap *params, UA_ByteString msg) {
    HTTPTestContext *ctx = (HTTPTestContext*)application;
    const UA_UInt16 *listenPort = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "listen-port"), &UA_TYPES[UA_TYPES_UINT16]);
    if(listenPort) {
        ctx->listenerId = connectionId;
        ctx->port = *listenPort;
        return;
    }
    if(state == UA_CONNECTIONSTATE_OPENING) {
        ctx->clientId = connectionId;
        return;
    }
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        ck_assert_ptr_nonnull(connectionContext);
        if(connectionId == ctx->clientId)
            ctx->clientClosingCount++;
        else if(connectionId != ctx->listenerId)
            ctx->acceptedClosingCount++;
        return;
    }

    const UA_String *method = (const UA_String*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "method"), &UA_TYPES[UA_TYPES_STRING]);
    if(!method && connectionId != ctx->clientId) {
        ck_assert_ptr_nonnull(connectionContext);
        ck_assert_ptr_null(*connectionContext);
        ck_assert_uint_ne(connectionId, ctx->listenerId);
        *connectionContext = ctx;
        ctx->acceptedConnectionCount++;
        return;
    }
    if(method) {
        ck_assert_ptr_nonnull(connectionContext);
        ck_assert_ptr_eq(*connectionContext, ctx);
        const UA_ByteString *requestRandom =
            (const UA_ByteString*)UA_KeyValueMap_getScalar(
                params, UA_QUALIFIEDNAME(0, "request-random"),
                &UA_TYPES[UA_TYPES_BYTESTRING]);
        ck_assert_ptr_nonnull(requestRandom);
        ck_assert_uint_eq(requestRandom->length, 32);
        ctx->acceptedConnectionId = connectionId;
        ctx->requestCount++;
        for(size_t i = 0; i < msg.length; i++) {
            if(msg.data[i] != (UA_Byte)((ctx->serverBodyBytes + i) % 251))
                ctx->bodyMismatch = true;
        }
        ctx->serverBodyBytes += msg.length;

        if(ctx->mode == HTTP_TEST_CLOSE_LISTENER) {
            ck_assert_uint_eq(cm->closeConnection(cm, ctx->listenerId),
                              UA_STATUSCODE_GOOD);
            return;
        }

        const UA_String *path =
            (const UA_String *)UA_KeyValueMap_getScalar(
                params, UA_QUALIFIEDNAME(0, "path"),
                &UA_TYPES[UA_TYPES_STRING]);
        const UA_String failPath = UA_STRING_STATIC("/fail");
        if(ctx->mode == HTTP_TEST_ONE_REQUEST_FAILS && path &&
           UA_String_equal(path, &failPath)) {
            ck_assert_uint_eq(cm->closeConnection(cm, connectionId),
                              UA_STATUSCODE_GOOD);
            return;
        }

        const UA_String slowPath = UA_STRING_STATIC("/slow");
        if(ctx->mode == HTTP_TEST_ONE_REQUEST_STALLS && path &&
           UA_String_equal(path, &slowPath)) {
            ctx->stalledConnectionId = connectionId;
            return;
        }

        if(ctx->mode == HTTP_TEST_NO_RESPONSE)
            return;
        if(ctx->mode == HTTP_TEST_CLOSE_CONNECTION) {
            ck_assert_uint_eq(cm->closeConnection(cm, connectionId),
                              UA_STATUSCODE_GOOD);
            UA_ByteString late = UA_BYTESTRING_ALLOC("late");
            ctx->sendAfterCloseStatus = cm->sendWithConnection(
                cm, connectionId, &UA_KEYVALUEMAP_NULL, &late);
            ck_assert_ptr_null(late.data);
            return;
        }

        UA_ByteString response = UA_BYTESTRING_NULL;
        size_t responseSize = msg.length;
        if(ctx->mode == HTTP_TEST_EMPTY_RESPONSE)
            responseSize = 0;
        else if(ctx->mode == HTTP_TEST_FIXED_RESPONSE ||
                ctx->mode == HTTP_TEST_ONE_REQUEST_FAILS)
            responseSize = ctx->fixedResponseSize;
        if(responseSize > 0) {
            ck_assert_uint_eq(UA_ByteString_allocBuffer(&response, responseSize),
                              UA_STATUSCODE_GOOD);
            fillPattern(response.data, response.length, 0);
        }

        UA_UInt16 statusCode = 202;
        UA_String responseHeader = UA_STRING("present");
        UA_String identityEncoding = UA_STRING("identity");
        UA_KeyValuePair headers[3] = {0};
        headers[0].key = UA_QUALIFIEDNAME(0, "x-response");
        UA_Variant_setScalar(&headers[0].value, &responseHeader,
                             &UA_TYPES[UA_TYPES_STRING]);
        size_t headersSize = 1;
        if(ctx->mode == HTTP_TEST_DUPLICATE_ENCODING_RESPONSE) {
            headers[1].key = UA_QUALIFIEDNAME(0, "content-encoding");
            UA_Variant_setScalar(&headers[1].value, &identityEncoding,
                                 &UA_TYPES[UA_TYPES_STRING]);
            headers[2].key = UA_QUALIFIEDNAME(0, "Content-Encoding");
            UA_Variant_setScalar(&headers[2].value, &identityEncoding,
                                 &UA_TYPES[UA_TYPES_STRING]);
            headersSize = 3;
        }
        UA_KeyValuePair responseParams[2] = {0};
        responseParams[0].key = UA_QUALIFIEDNAME(0, "status-code");
        UA_Variant_setScalar(&responseParams[0].value, &statusCode,
                             &UA_TYPES[UA_TYPES_UINT16]);
        responseParams[1].key = UA_QUALIFIEDNAME(0, "headers");
        UA_Variant_setArray(&responseParams[1].value, headers, headersSize,
                            &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
        UA_KeyValueMap responseMap = {2, responseParams};
        ctx->firstSendStatus = cm->sendWithConnection(
            cm, connectionId, &responseMap, &response);
        if(ctx->mode == HTTP_TEST_DUPLICATE_RESPONSE) {
            UA_ByteString duplicate = UA_BYTESTRING_ALLOC("duplicate");
            ctx->secondSendStatus = cm->sendWithConnection(
                cm, connectionId, &responseMap, &duplicate);
            ck_assert_ptr_null(duplicate.data);
        }
        return;
    }

    if(connectionId != ctx->clientId)
        return;
    const UA_Variant *requestHandle = UA_KeyValueMap_get(
        params, UA_QUALIFIEDNAME(0, "handle"));
    if(requestHandle) {
        if(UA_Variant_hasScalarType(requestHandle,
                                    &UA_TYPES[UA_TYPES_UINT32])) {
            ck_assert_uint_eq(*(UA_UInt32*)requestHandle->data, 17);
            ctx->scalarHandleCallbacks++;
        } else if(UA_Variant_hasArrayType(
                      requestHandle, &UA_TYPES[UA_TYPES_UINT16])) {
            ck_assert(UA_Variant_hasArrayType(requestHandle,
                                              &UA_TYPES[UA_TYPES_UINT16]));
            ck_assert_uint_eq(requestHandle->arrayLength, 2);
            const UA_UInt16 *values = (const UA_UInt16*)requestHandle->data;
            ck_assert_uint_eq(values[0], 23);
            ck_assert_uint_eq(values[1], 42);
            ctx->arrayHandleCallbacks++;
        } else
            ck_assert(UA_Variant_hasScalarType(
                requestHandle, &UA_TYPES[UA_TYPES_UINT64]));
    }
    const UA_Boolean *responseComplete =
        (const UA_Boolean*)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "response-complete"),
            &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(responseComplete) {
        ck_assert(*responseComplete);
        ctx->responseCompleteCount++;
        const UA_StatusCode *requestStatus =
            (const UA_StatusCode *)UA_KeyValueMap_getScalar(
                params, UA_QUALIFIEDNAME(0, "request-status"),
                &UA_TYPES[UA_TYPES_STATUSCODE]);
        ck_assert_ptr_nonnull(requestStatus);
        ctx->lastRequestStatus = *requestStatus;
        if(*requestStatus == UA_STATUSCODE_GOOD)
            ctx->goodRequestCompletions++;
        else
            ctx->badRequestCompletions++;
    }
    const UA_UInt16 *statusCode = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "status-code"), &UA_TYPES[UA_TYPES_UINT16]);
    if(statusCode) {
        ctx->responseStatus = *statusCode;
        if(msg.length == 0 && !responseComplete)
            ctx->responseCount++;
    }
    const UA_Variant *responseHeaders = UA_KeyValueMap_get(
        params, UA_QUALIFIEDNAME(0, "headers"));
    if(responseHeaders) {
        UA_KeyValuePair *headers = (UA_KeyValuePair*)responseHeaders->data;
        const UA_String expectedName = UA_STRING_STATIC("x-response");
        for(size_t i = 0; i < responseHeaders->arrayLength; i++) {
            if(UA_String_equal(&headers[i].key.name, &expectedName))
                ctx->responseHeaderReceived = true;
        }
    }
    for(size_t i = 0; i < msg.length; i++) {
        if(msg.data[i] != (UA_Byte)((ctx->clientBodyBytes + i) % 251))
            ctx->bodyMismatch = true;
    }
    ctx->clientBodyBytes += msg.length;
}

static void
runUntilCount(UA_EventLoop *eventLoop, const size_t *value,
              size_t expected, size_t iterations) {
    for(size_t i = 0; i < iterations && *value < expected; i++)
        eventLoop->run(eventLoop, 20);
    ck_assert_uint_ge(*value, expected);
}

#ifndef _WIN32
static void
runUntilSocketReadable(UA_EventLoop *eventLoop, int fd, size_t iterations) {
    for(size_t i = 0; i < iterations; i++) {
        char byte;
        ssize_t received = recv(fd, &byte, 1, MSG_PEEK | MSG_DONTWAIT);
        if(received >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
            return;
        eventLoop->run(eventLoop, 20);
    }
    ck_abort_msg("HTTP socket did not become readable");
}

static ssize_t
readSocketUntilClosed(UA_EventLoop *eventLoop, int fd, UA_Byte *buffer,
                      size_t capacity, size_t iterations) {
    size_t total = 0;
    for(size_t i = 0; i < iterations && total < capacity; i++) {
        ssize_t received = recv(fd, &buffer[total], capacity - total,
                                MSG_DONTWAIT);
        if(received > 0) {
            total += (size_t)received;
            continue;
        }
        if(received == 0)
            return (ssize_t)total;
        if(errno != EAGAIN && errno != EWOULDBLOCK)
            return total > 0 ? (ssize_t)total : -1;
        eventLoop->run(eventLoop, 20);
    }
    return (ssize_t)total;
}
#endif

static void
closeHTTPConnectionIfPresent(UA_ConnectionManager *cm, uintptr_t connectionId) {
    UA_StatusCode status = cm->closeConnection(cm, connectionId);
    ck_assert(status == UA_STATUSCODE_GOOD ||
              status == UA_STATUSCODE_BADNOTFOUND);
}

static void
openHTTPListener(UA_ConnectionManager *cm, HTTPTestContext *ctx,
                 UA_UInt16 timeout, UA_UInt32 recvMax, UA_UInt32 sendMax,
                 UA_Boolean useSSL, const UA_ByteString *certificate,
                 const UA_ByteString *privateKey,
                 const UA_ByteString *caCertificate) {
    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_KeyValuePair parameters[8] = {0};
    size_t size = 0;
    parameters[size].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&parameters[size++].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    parameters[size].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&parameters[size++].value, &listen,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    parameters[size].key = UA_QUALIFIEDNAME(0, "timeout");
    UA_Variant_setScalar(&parameters[size++].value, &timeout,
                         &UA_TYPES[UA_TYPES_UINT16]);
    if(recvMax) {
        parameters[size].key = UA_QUALIFIEDNAME(0, "recv-max-message-size");
        UA_Variant_setScalar(&parameters[size++].value, &recvMax,
                             &UA_TYPES[UA_TYPES_UINT32]);
    }
    if(sendMax) {
        parameters[size].key = UA_QUALIFIEDNAME(0, "send-max-message-size");
        UA_Variant_setScalar(&parameters[size++].value, &sendMax,
                             &UA_TYPES[UA_TYPES_UINT32]);
    }
    if(useSSL) {
        parameters[size].key = UA_QUALIFIEDNAME(0, "useSSL");
        UA_Variant_setScalar(&parameters[size++].value, &useSSL,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);
        parameters[size].key = UA_QUALIFIEDNAME(0, "certificate");
        UA_Variant_setScalar(&parameters[size++].value,
                             (void*)(uintptr_t)certificate,
                             &UA_TYPES[UA_TYPES_BYTESTRING]);
        parameters[size].key = UA_QUALIFIEDNAME(0, "private-key");
        UA_Variant_setScalar(&parameters[size++].value,
                             (void*)(uintptr_t)privateKey,
                             &UA_TYPES[UA_TYPES_BYTESTRING]);
    }
    if(caCertificate) {
        parameters[size].key = UA_QUALIFIEDNAME(0, "ca-certificate");
        UA_Variant_setScalar(&parameters[size++].value,
                             (void*)(uintptr_t)caCertificate,
                             &UA_TYPES[UA_TYPES_BYTESTRING]);
    }
    UA_KeyValueMap map = {size, parameters};
    ck_assert_uint_eq(cm->openConnection(cm, &map, ctx, NULL,
                                         advancedHTTPCallback),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(ctx->listenerId, 0);
    ck_assert_uint_ne(ctx->port, 0);
}

static void
openHTTPClient(UA_ConnectionManager *cm, HTTPTestContext *ctx,
               UA_UInt32 recvMax, UA_UInt32 sendMax, UA_Boolean useSSL,
               const UA_ByteString *certificate,
               const UA_ByteString *privateKey,
               const UA_ByteString *caCertificate) {
    UA_String address = UA_STRING("127.0.0.1");
    UA_KeyValuePair parameters[8] = {0};
    size_t size = 0;
    parameters[size].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&parameters[size++].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    parameters[size].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&parameters[size++].value, &ctx->port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    if(recvMax) {
        parameters[size].key = UA_QUALIFIEDNAME(0, "recv-max-message-size");
        UA_Variant_setScalar(&parameters[size++].value, &recvMax,
                             &UA_TYPES[UA_TYPES_UINT32]);
    }
    if(sendMax) {
        parameters[size].key = UA_QUALIFIEDNAME(0, "send-max-message-size");
        UA_Variant_setScalar(&parameters[size++].value, &sendMax,
                             &UA_TYPES[UA_TYPES_UINT32]);
    }
    if(useSSL) {
        parameters[size].key = UA_QUALIFIEDNAME(0, "useSSL");
        UA_Variant_setScalar(&parameters[size++].value, &useSSL,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);
        if(certificate) {
            parameters[size].key = UA_QUALIFIEDNAME(0, "certificate");
            UA_Variant_setScalar(&parameters[size++].value,
                                 (void*)(uintptr_t)certificate,
                                 &UA_TYPES[UA_TYPES_BYTESTRING]);
            parameters[size].key = UA_QUALIFIEDNAME(0, "private-key");
            UA_Variant_setScalar(&parameters[size++].value,
                                 (void*)(uintptr_t)privateKey,
                                 &UA_TYPES[UA_TYPES_BYTESTRING]);
        }
        parameters[size].key = UA_QUALIFIEDNAME(0, "ca-certificate");
        UA_Variant_setScalar(&parameters[size++].value,
                             (void*)(uintptr_t)caCertificate,
                             &UA_TYPES[UA_TYPES_BYTESTRING]);
    }
    UA_KeyValueMap map = {size, parameters};
    ck_assert_uint_eq(cm->openConnection(cm, &map, ctx, NULL,
                                         advancedHTTPCallback),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(ctx->clientId, 0);
}

static void
openHTTPClientWithTimeout(UA_ConnectionManager *cm, HTTPTestContext *ctx,
                          UA_UInt16 timeout) {
    UA_String address = UA_STRING("127.0.0.1");
    UA_KeyValuePair parameters[3] = {0};
    parameters[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&parameters[0].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    parameters[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&parameters[1].value, &ctx->port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    parameters[2].key = UA_QUALIFIEDNAME(0, "timeout");
    UA_Variant_setScalar(&parameters[2].value, &timeout,
                         &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap map = {3, parameters};
    ck_assert_uint_eq(cm->openConnection(cm, &map, ctx, NULL,
                                         advancedHTTPCallback),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(ctx->clientId, 0);
}

static void
openHTTPClientAt(UA_ConnectionManager *cm, HTTPTestContext *ctx,
                 UA_String *address, UA_UInt16 port) {
    UA_KeyValuePair parameters[2] = {0};
    parameters[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&parameters[0].value, address,
                         &UA_TYPES[UA_TYPES_STRING]);
    parameters[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&parameters[1].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap map = {2, parameters};
    ck_assert_uint_eq(cm->openConnection(cm, &map, ctx, NULL,
                                         advancedHTTPCallback),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(ctx->clientId, 0);
}

static void
sendPatternRequest(UA_ConnectionManager *cm, HTTPTestContext *ctx,
                   size_t size) {
    UA_String method = UA_STRING("POST");
    UA_String path = UA_STRING("/test");
    UA_KeyValuePair parameters[2] = {0};
    parameters[0].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&parameters[0].value, &method,
                         &UA_TYPES[UA_TYPES_STRING]);
    parameters[1].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&parameters[1].value, &path,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap map = {2, parameters};
    UA_ByteString body = UA_BYTESTRING_NULL;
    if(size > 0) {
        ck_assert_uint_eq(UA_ByteString_allocBuffer(&body, size),
                          UA_STATUSCODE_GOOD);
        fillPattern(body.data, body.length, 0);
    }
    ck_assert_uint_eq(cm->sendWithConnection(cm, ctx->clientId, &map,
                                             size ? &body : NULL),
                      UA_STATUSCODE_GOOD);
}

static void
sendHandledRequestWithTimeout(UA_ConnectionManager *cm, HTTPTestContext *ctx,
                              const char *pathValue, UA_UInt16 timeout) {
    UA_String method = UA_STRING("POST");
    UA_String path = UA_STRING((char *)(uintptr_t)pathValue);
    UA_UInt64 requestHandle = 5381;
    for(const char *pos = pathValue; *pos; pos++)
        requestHandle = requestHandle * 33u + (UA_Byte)*pos;
    UA_KeyValuePair parameters[4] = {0};
    parameters[0].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&parameters[0].value, &method,
                         &UA_TYPES[UA_TYPES_STRING]);
    parameters[1].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&parameters[1].value, &path,
                         &UA_TYPES[UA_TYPES_STRING]);
    parameters[2].key = UA_QUALIFIEDNAME(0, "handle");
    UA_Variant_setScalar(&parameters[2].value, &requestHandle,
                         &UA_TYPES[UA_TYPES_UINT64]);
    size_t parametersSize = 3;
    if(timeout > 0) {
        parameters[parametersSize].key = UA_QUALIFIEDNAME(0, "timeout");
        UA_Variant_setScalar(&parameters[parametersSize++].value, &timeout,
                             &UA_TYPES[UA_TYPES_UINT16]);
    }
    UA_KeyValueMap map = {parametersSize, parameters};
    ck_assert_uint_eq(cm->sendWithConnection(cm, ctx->clientId, &map, NULL),
                      UA_STATUSCODE_GOOD);
}

static void
sendHandledRequest(UA_ConnectionManager *cm, HTTPTestContext *ctx,
                   const char *pathValue) {
    sendHandledRequestWithTimeout(cm, ctx, pathValue, 0);
}

START_TEST(serverLargeAndEmptyBodies) {
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_ECHO;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 0, 0, false, NULL, NULL, NULL);

    const size_t largeSize = 128 * 1024 + 17;
    sendPatternRequest(cm, &ctx, largeSize);
    runUntilCount(eventLoop, &ctx.clientBodyBytes, largeSize, 500);
    ck_assert_uint_eq(ctx.serverBodyBytes, largeSize);
    ck_assert_uint_eq(ctx.responseStatus, 202);
    ck_assert(ctx.responseHeaderReceived);
    ck_assert(!ctx.bodyMismatch);

    ctx.mode = HTTP_TEST_EMPTY_RESPONSE;
    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.responseCount, 2, 200);
    ck_assert_uint_eq(ctx.requestCount, 2);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(clientRequestHandles) {
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_FIXED_RESPONSE;
    ctx.fixedResponseSize = 2;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 0, 0, false, NULL, NULL, NULL);

    UA_String method = UA_STRING("POST");
    UA_String path = UA_STRING("/handles");
    UA_UInt32 scalarHandle = 17;
    UA_KeyValuePair scalarParams[3] = {0};
    scalarParams[0].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&scalarParams[0].value, &method,
                         &UA_TYPES[UA_TYPES_STRING]);
    scalarParams[1].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&scalarParams[1].value, &path,
                         &UA_TYPES[UA_TYPES_STRING]);
    scalarParams[2].key = UA_QUALIFIEDNAME(0, "handle");
    UA_Variant_setScalar(&scalarParams[2].value, &scalarHandle,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_KeyValueMap scalarMap = {3, scalarParams};
    ck_assert_uint_eq(cm->sendWithConnection(cm, ctx.clientId, &scalarMap, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cm->sendWithConnection(cm, ctx.clientId, &scalarMap, NULL),
                      UA_STATUSCODE_BADINVALIDSTATE);
    scalarHandle = 99; /* The request retained a deep copy. */

    UA_UInt16 arrayHandle[2] = {23, 42};
    UA_KeyValuePair arrayParams[3] = {0};
    arrayParams[0] = scalarParams[0];
    arrayParams[1] = scalarParams[1];
    arrayParams[2].key = UA_QUALIFIEDNAME(0, "handle");
    UA_Variant_setArray(&arrayParams[2].value, arrayHandle, 2,
                        &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap arrayMap = {3, arrayParams};
    ck_assert_uint_eq(cm->sendWithConnection(cm, ctx.clientId, &arrayMap, NULL),
                      UA_STATUSCODE_GOOD);
    arrayHandle[0] = 100;
    arrayHandle[1] = 101;

    runUntilCount(eventLoop, &ctx.responseCompleteCount, 2, 300);
    ck_assert_uint_eq(ctx.requestCount, 2);
    ck_assert_uint_eq(ctx.clientBodyBytes, 4);
    ck_assert_uint_ge(ctx.scalarHandleCallbacks, 3);
    ck_assert_uint_ge(ctx.arrayHandleCallbacks, 3);

    /* Legacy requests remain supported, but ambiguous concurrency is rejected. */
    ctx.mode = HTTP_TEST_NO_RESPONSE;
    ck_assert_uint_eq(cm->sendWithConnection(cm, ctx.clientId,
                                             &UA_KEYVALUEMAP_NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cm->sendWithConnection(cm, ctx.clientId,
                                             &UA_KEYVALUEMAP_NULL, NULL),
                      UA_STATUSCODE_BADINVALIDSTATE);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(serverRequestLifecycle) {
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_DUPLICATE_RESPONSE;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 0, 0, false, NULL, NULL, NULL);
    sendPatternRequest(cm, &ctx, 4);
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 1, 200);
    ck_assert_uint_eq(ctx.firstSendStatus, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ctx.secondSendStatus, UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(ctx.acceptedClosingCount, 0);

    ctx.mode = HTTP_TEST_CLOSE_CONNECTION;
    sendPatternRequest(cm, &ctx, 4);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 1, 200);
    ck_assert(ctx.sendAfterCloseStatus == UA_STATUSCODE_BADINVALIDSTATE ||
              ctx.sendAfterCloseStatus == UA_STATUSCODE_BADNOTFOUND);
    ck_assert_uint_eq(ctx.acceptedClosingCount, 1);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(serverBrokenCarrierIsolation) {
    HTTPTestContext first = {0};
    first.mode = HTTP_TEST_NO_RESPONSE;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &first, 30, 0, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &first, 0, 0, false, NULL, NULL, NULL);

    /* Leave one request outstanding, then break only its client-side carrier. */
    sendPatternRequest(cm, &first, 0);
    runUntilCount(eventLoop, &first.requestCount, 1, 100);

    HTTPTestContext sibling = {0};
    sibling.port = first.port;
    openHTTPClient(cm, &sibling, 0, 0, false, NULL, NULL, NULL);
    ck_assert_uint_eq(cm->closeConnection(cm, first.clientId),
                      UA_STATUSCODE_GOOD);
    runUntilCount(eventLoop, &first.acceptedClosingCount, 1, 100);
    runUntilCount(eventLoop, &first.clientClosingCount, 1, 100);

    /* The sibling uses the same listener and must remain fully usable. */
    first.mode = HTTP_TEST_FIXED_RESPONSE;
    first.fixedResponseSize = 2;
    sendPatternRequest(cm, &sibling, 0);
    runUntilCount(eventLoop, &sibling.responseCompleteCount, 1, 100);
    ck_assert_uint_eq(first.requestCount, 2);
    ck_assert_uint_eq(sibling.clientBodyBytes, 2);
    ck_assert_uint_eq(first.acceptedClosingCount, 1);

    closeHTTPConnectionIfPresent(cm, sibling.clientId);
    ck_assert_uint_eq(cm->closeConnection(cm, first.listenerId),
                      UA_STATUSCODE_GOOD);
    runUntilCount(eventLoop, &first.acceptedClosingCount, 2, 100);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(clientRequestFailureIsolation) {
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_ONE_REQUEST_FAILS;
    ctx.fixedResponseSize = 2;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 0, 0, false, NULL, NULL, NULL);

    sendHandledRequest(cm, &ctx, "/fail");
    sendHandledRequest(cm, &ctx, "/ok");
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 2, 200);
    ck_assert_uint_eq(ctx.goodRequestCompletions, 1);
    ck_assert_uint_eq(ctx.badRequestCompletions, 1);
    ck_assert_uint_eq(ctx.clientBodyBytes, 2);
    ck_assert_uint_eq(ctx.clientClosingCount, 0);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    closeHTTPConnectionIfPresent(cm, ctx.listenerId);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(clientSynchronousConnectionFailure) {
    HTTPTestContext ctx = {0};
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);

    /* Some libwebsockets resolvers report a DNS failure from inside
     * lws_client_connect_via_info. The callback must see an already-linked
     * request and cleanup must defer freeing it until the connect call
     * returns. */
    UA_String address = UA_STRING("open62541.invalid");
    openHTTPClientAt(cm, &ctx, &address, 80);
    sendHandledRequest(cm, &ctx, "/dns-failure");
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 1, 100);
    ck_assert_uint_eq(ctx.badRequestCompletions, 1);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(clientRequestTimeoutIsolation) {
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_ONE_REQUEST_STALLS;
    ctx.fixedResponseSize = 2;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 0, 0, false, NULL, NULL, NULL);

    /* The slow request overrides the binding timeout. Its sibling must finish
     * normally and the shared client binding must remain usable. */
    sendHandledRequestWithTimeout(cm, &ctx, "/slow", 1);
    sendHandledRequest(cm, &ctx, "/ok");
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 2, 200);
    ck_assert_uint_eq(ctx.goodRequestCompletions, 1);
    ck_assert_uint_eq(ctx.badRequestCompletions, 1);
    ck_assert_uint_eq(ctx.clientClosingCount, 0);
    ck_assert_uint_ne(ctx.stalledConnectionId, 0);

    /* A response attempted after the timed-out carrier was destroyed cannot
     * generate a second callback or become associated with another request. */
    UA_UInt16 statusCode = 200;
    UA_KeyValuePair responseParam = {
        UA_QUALIFIEDNAME(0, "status-code"), {0}};
    UA_Variant_setScalar(&responseParam.value, &statusCode,
                         &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap responseMap = {1, &responseParam};
    UA_ByteString late = UA_BYTESTRING_ALLOC("late");
    UA_StatusCode lateStatus = cm->sendWithConnection(
        cm, ctx.stalledConnectionId, &responseMap, &late);
    ck_assert(lateStatus == UA_STATUSCODE_GOOD ||
              lateStatus == UA_STATUSCODE_BADNOTFOUND);
    ck_assert_ptr_null(late.data);
    for(size_t i = 0; i < 20; i++)
        eventLoop->run(eventLoop, 10);
    ck_assert_uint_eq(ctx.responseCompleteCount, 2);

    sendHandledRequest(cm, &ctx, "/ok");
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 3, 100);
    ck_assert_uint_eq(ctx.goodRequestCompletions, 2);
    ck_assert_uint_eq(ctx.badRequestCompletions, 1);
    ck_assert_uint_eq(ctx.clientClosingCount, 0);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    closeHTTPConnectionIfPresent(cm, ctx.listenerId);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(serverResponseFailureIsolation) {
#ifndef _WIN32
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_FIXED_RESPONSE;
    ctx.fixedResponseSize = 8 * 1024 * 1024;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(ctx.port);
    ck_assert_int_eq(inet_pton(AF_INET, "127.0.0.1",
                               &serverAddress.sin_addr), 1);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    const char request[] =
        "POST /large HTTP/1.1\r\nHost: localhost\r\n"
        "Content-Length: 0\r\nConnection: keep-alive\r\n\r\n";
    ck_assert_int_eq(send(fd, request, sizeof(request) - 1, 0),
                     (ssize_t)(sizeof(request) - 1));
    runUntilCount(eventLoop, &ctx.requestCount, 1, 100);
    struct linger reset = {1, 0};
    ck_assert_int_eq(setsockopt(fd, SOL_SOCKET, SO_LINGER, &reset,
                               sizeof(reset)), 0);
    close(fd);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 1, 200);

    /* A failed response carrier does not affect a new sibling carrier. */
    ctx.fixedResponseSize = 2;
    openHTTPClient(cm, &ctx, 0, 0, false, NULL, NULL, NULL);
    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 1, 100);
    ck_assert_uint_eq(ctx.requestCount, 2);
    ck_assert_uint_eq(ctx.clientBodyBytes, 2);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId),
                      UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);
#endif
} END_TEST

START_TEST(serverListenerCloseWithIdleCarrier) {
#ifndef _WIN32
    HTTPTestContext ctx = {0};
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(ctx.port);
    ck_assert_int_eq(inet_pton(AF_INET, "127.0.0.1",
                               &serverAddress.sin_addr), 1);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    for(size_t i = 0; i < 10; i++)
        eventLoop->run(eventLoop, 20);

    /* Shutdown must retain the listener until this idle carrier is detached. */
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 10; i++)
        eventLoop->run(eventLoop, 20);
    close(fd);
    stopEventLoop(eventLoop);
#endif
} END_TEST

START_TEST(serverListenerCloseFromRequestCallback) {
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_CLOSE_LISTENER;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm);
    ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop,
                                                     &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 0, 0, false, NULL, NULL, NULL);

    /* Listener teardown is requested from inside the accepted connection's
     * request callback. Destruction is deferred until the callback has fully
     * returned, then the accepted carrier and client request complete once. */
    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 1, 100);
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 1, 100);
    ck_assert_uint_eq(ctx.requestCount, 1);
    ck_assert_uint_eq(ctx.acceptedClosingCount, 1);
    ck_assert_uint_eq(ctx.responseCompleteCount, 1);
    ck_assert_uint_ne(ctx.lastRequestStatus, UA_STATUSCODE_GOOD);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    stopEventLoop(eventLoop);
} END_TEST

#ifdef UA_ENABLE_HTTP_COMPRESSION
static int
connectRawHttp(UA_UInt16 port) {
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    ck_assert_int_eq(inet_pton(AF_INET, "127.0.0.1",
                               &serverAddress.sin_addr), 1);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    return fd;
}

static size_t
findHeaderEnd(const UA_Byte *response, size_t length) {
    for(size_t i = 0; i + 3 < length; i++) {
        if(memcmp(&response[i], "\r\n\r\n", 4) == 0)
            return i + 4;
    }
    return 0;
}

START_TEST(serverContentEncodingAndExpansionLimit) {
#ifndef _WIN32
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_ECHO;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_UInt16 timeout = 30;
    UA_UInt32 wireLimit = 1024;
    UA_UInt32 expandedLimit = 8192;
    UA_Boolean listen = true;
    UA_KeyValuePair parameters[5] = {0};
    parameters[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&parameters[0].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    parameters[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&parameters[1].value, &listen,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    parameters[2].key = UA_QUALIFIEDNAME(0, "timeout");
    UA_Variant_setScalar(&parameters[2].value, &timeout,
                         &UA_TYPES[UA_TYPES_UINT16]);
    parameters[3].key = UA_QUALIFIEDNAME(0, "recv-max-message-size");
    UA_Variant_setScalar(&parameters[3].value, &wireLimit,
                         &UA_TYPES[UA_TYPES_UINT32]);
    parameters[4].key = UA_QUALIFIEDNAME(
        0, "recv-max-decompressed-message-size");
    UA_Variant_setScalar(&parameters[4].value, &expandedLimit,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_KeyValueMap map = {5, parameters};
    ck_assert_uint_eq(cm->openConnection(cm, &map, &ctx, NULL,
                                         advancedHTTPCallback),
                      UA_STATUSCODE_GOOD);

    UA_ByteString input = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&input, 4096),
                      UA_STATUSCODE_GOOD);
    fillPattern(input.data, input.length, 0);
    UA_ByteString compressed = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_HTTP_compress(UA_HTTP_CONTENT_ENCODING_GZIP,
                                       &input, &compressed),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_lt(compressed.length, wireLimit);

    int fd = connectRawHttp(ctx.port);
    char header[512];
    int headerLength = snprintf(
        header, sizeof(header),
        "POST /compressed HTTP/1.1\r\nHost: localhost\r\n"
        "Content-Encoding: gzip\r\nAccept-Encoding: gzip\r\n"
        "Content-Length: %zu\r\nConnection: close\r\n\r\n",
        compressed.length);
    ck_assert_int_gt(headerLength, 0);
    ck_assert_int_eq(send(fd, header, (size_t)headerLength, 0), headerLength);
    ck_assert_int_eq(send(fd, compressed.data, compressed.length, 0),
                     (ssize_t)compressed.length);
    runUntilCount(eventLoop, &ctx.requestCount, 1, 200);
    runUntilSocketReadable(eventLoop, fd, 100);
    UA_Byte response[16384];
    ssize_t received = readSocketUntilClosed(
        eventLoop, fd, response, sizeof(response) - 1, 100);
    ck_assert_int_gt(received, 0);
    close(fd);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 1, 200);
    response[received] = 0;
    ck_assert_ptr_nonnull(strstr((char*)response, "HTTP/1.1 202"));
    ck_assert_ptr_nonnull(strstr((char*)response, "Content-Encoding: gzip"));
    size_t bodyOffset = findHeaderEnd(response, (size_t)received);
    ck_assert_uint_gt(bodyOffset, 0);
    UA_ByteString encodedResponse = {
        (size_t)received - bodyOffset, &response[bodyOffset]
    };
    UA_ByteString decodedResponse = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_HTTP_decompress(UA_HTTP_CONTENT_ENCODING_GZIP,
                                         &encodedResponse, 8192,
                                         &decodedResponse),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&input, &decodedResponse));
    UA_ByteString_clear(&decodedResponse);
    UA_ByteString_clear(&compressed);
    UA_ByteString_clear(&input);

    /* A small wire body that expands past the independent limit gets 413 and
     * is never delivered to the application. */
    ck_assert_uint_eq(UA_ByteString_allocBuffer(&input, expandedLimit + 1),
                      UA_STATUSCODE_GOOD);
    memset(input.data, 0, input.length);
    ck_assert_uint_eq(UA_HTTP_compress(UA_HTTP_CONTENT_ENCODING_GZIP,
                                       &input, &compressed),
                      UA_STATUSCODE_GOOD);
    fd = connectRawHttp(ctx.port);
    headerLength = snprintf(
        header, sizeof(header),
        "POST /bomb HTTP/1.1\r\nHost: localhost\r\n"
        "Content-Encoding: gzip\r\nContent-Length: %zu\r\n"
        "Connection: close\r\n\r\n", compressed.length);
    ck_assert_int_eq(send(fd, header, (size_t)headerLength, 0), headerLength);
    ck_assert_int_eq(send(fd, compressed.data, compressed.length, 0),
                     (ssize_t)compressed.length);
    runUntilSocketReadable(eventLoop, fd, 100);
    received = readSocketUntilClosed(
        eventLoop, fd, response, sizeof(response) - 1, 100);
    ck_assert_int_gt(received, 0);
    close(fd);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 2, 200);
    response[received] = 0;
    ck_assert_ptr_nonnull(strstr((char*)response, "HTTP/1.1 413"));
    ck_assert_uint_eq(ctx.requestCount, 1);
    UA_ByteString_clear(&compressed);
    UA_ByteString_clear(&input);

    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId),
                      UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);
#endif
} END_TEST

START_TEST(clientCompressedResponse) {
#ifndef _WIN32
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_FIXED_RESPONSE;
    ctx.fixedResponseSize = 4096;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);

    UA_String address = UA_STRING("127.0.0.1");
    UA_UInt32 wireLimit = 1024;
    UA_UInt32 expandedLimit = 8192;
    UA_KeyValuePair parameters[4] = {0};
    parameters[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&parameters[0].value, &address,
                         &UA_TYPES[UA_TYPES_STRING]);
    parameters[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&parameters[1].value, &ctx.port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    parameters[2].key = UA_QUALIFIEDNAME(0, "recv-max-message-size");
    UA_Variant_setScalar(&parameters[2].value, &wireLimit,
                         &UA_TYPES[UA_TYPES_UINT32]);
    parameters[3].key = UA_QUALIFIEDNAME(
        0, "recv-max-decompressed-message-size");
    UA_Variant_setScalar(&parameters[3].value, &expandedLimit,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_KeyValueMap map = {4, parameters};
    ck_assert_uint_eq(cm->openConnection(cm, &map, &ctx, NULL,
                                         advancedHTTPCallback),
                      UA_STATUSCODE_GOOD);

    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 1, 200);
    ck_assert_uint_eq(ctx.clientBodyBytes, ctx.fixedResponseSize);
    ck_assert(!ctx.bodyMismatch);
    ck_assert_uint_eq(ctx.responseStatus, 202);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    closeHTTPConnectionIfPresent(cm, ctx.listenerId);
    stopEventLoop(eventLoop);
#endif
} END_TEST
#endif

START_TEST(clientRejectsDuplicateContentEncoding) {
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_DUPLICATE_ENCODING_RESPONSE;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);
    openHTTPClientWithTimeout(cm, &ctx, 1);

    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 1, 200);
    ck_assert_uint_eq(ctx.responseCompleteCount, 1);
    ck_assert_uint_ne(ctx.lastRequestStatus, UA_STATUSCODE_GOOD);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    closeHTTPConnectionIfPresent(cm, ctx.listenerId);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(serverTimeoutAndListenerClose) {
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_NO_RESPONSE;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 1, 0, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 0, 0, false, NULL, NULL, NULL);
    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 1, 150);
    ck_assert_uint_eq(ctx.acceptedClosingCount, 1);

    runUntilCount(eventLoop, &ctx.responseCompleteCount, 1, 100);
    ck_assert_uint_ne(ctx.lastRequestStatus, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ctx.clientClosingCount, 0);
    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.requestCount, 2, 100);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId),
                      UA_STATUSCODE_GOOD);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 2, 100);
    ck_assert_uint_eq(ctx.acceptedClosingCount, 2);
    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(serverReceiveAndKeepAliveTimeouts) {
#ifndef _WIN32
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_FIXED_RESPONSE;
    ctx.fixedResponseSize = 2;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 1, 0, 0, false, NULL, NULL, NULL);

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(ctx.port);
    ck_assert_int_eq(inet_pton(AF_INET, "127.0.0.1",
                               &serverAddress.sin_addr), 1);

    /* A client that never completes its headers cannot retain the raw WSI. */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    const char partialHeader[] = "POST /slow HTTP/1.1\r\nHost:";
    ck_assert_int_eq(send(fd, partialHeader, sizeof(partialHeader) - 1, 0),
                     (ssize_t)(sizeof(partialHeader) - 1));
    for(size_t i = 0; i < 75; i++)
        eventLoop->run(eventLoop, 20);
    char byte;
    ssize_t received = recv(fd, &byte, 1, MSG_DONTWAIT);
    ck_assert(received == 0 ||
              (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK));
    ck_assert_uint_eq(ctx.requestCount, 0);
    close(fd);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 1, 100);

    /* An incomplete request body times out the accepted connection without
     * delivering a request to the application. */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    const char partialBody[] =
        "POST /slow-body HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Content-Length: 4\r\n\r\n"
        "x";
    ck_assert_int_eq(send(fd, partialBody, sizeof(partialBody) - 1, 0),
                     (ssize_t)(sizeof(partialBody) - 1));
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 2, 100);
    ck_assert_uint_eq(ctx.requestCount, 0);
    close(fd);

    /* Body progress refreshes the inactivity timeout. The keep-alive socket is
     * then bounded again after the response completes. */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    const char progressHeader[] =
        "POST /progress HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Content-Length: 3\r\n"
        "Connection: keep-alive\r\n\r\n";
    ck_assert_int_eq(send(fd, progressHeader, sizeof(progressHeader) - 1, 0),
                     (ssize_t)(sizeof(progressHeader) - 1));
    for(UA_Byte value = 0; value < 3; value++) {
        ck_assert_int_eq(send(fd, &value, 1, 0), 1);
        if(value < 2) {
            for(size_t i = 0; i < 30; i++)
                eventLoop->run(eventLoop, 20);
        }
    }
    runUntilCount(eventLoop, &ctx.requestCount, 1, 100);
    ck_assert_uint_eq(ctx.requestCount, 1);
    char response[1024];
    runUntilSocketReadable(eventLoop, fd, 100);
    received = recv(fd, response, sizeof(response), 0);
    ck_assert_int_gt(received, 0);
    /* LWS can write the header and body in separate callbacks. Drain both
     * before checking that the later keep-alive timeout closed the socket. */
    for(size_t i = 0; i < 10; i++) {
        eventLoop->run(eventLoop, 10);
        while(recv(fd, response, sizeof(response), MSG_DONTWAIT) > 0) {}
    }
    for(size_t i = 0; i < 75; i++)
        eventLoop->run(eventLoop, 20);
    received = recv(fd, &byte, 1, MSG_DONTWAIT);
    ck_assert(received == 0 ||
              (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK));
    close(fd);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 3, 100);

    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);
#endif
} END_TEST

START_TEST(clientServerMutualTls) {
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_EMPTY_RESPONSE;
    UA_ByteString certificate = loadFile("server_cert.der");
    UA_ByteString privateKey = loadFile("server_key.der");
    ck_assert_ptr_nonnull(certificate.data);
    ck_assert_ptr_nonnull(privateKey.data);
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("https"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, true, &certificate, &privateKey,
                     &certificate);
    openHTTPClient(cm, &ctx, 0, 0, true, &certificate, &privateKey,
                   &certificate);

    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.responseCount, 1, 300);
    ck_assert_uint_eq(ctx.requestCount, 1);
    ck_assert_uint_eq(ctx.responseStatus, 202);
    ck_assert(ctx.responseHeaderReceived);

    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    runUntilCount(eventLoop, &ctx.clientClosingCount, 1, 100);

    /* The listener CA makes a client certificate mandatory. */
    openHTTPClient(cm, &ctx, 0, 0, true, NULL, NULL, &certificate);
    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 2, 200);
    ck_assert_uint_ne(ctx.lastRequestStatus, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ctx.requestCount, 1);
    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    runUntilCount(eventLoop, &ctx.clientClosingCount, 2, 100);

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(messageSizeLimits) {
    /* Client send limit. */
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_EMPTY_RESPONSE;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 4, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 0, 4, false, NULL, NULL, NULL);
    UA_ByteString tooLarge = UA_BYTESTRING_ALLOC("12345");
    ck_assert_uint_eq(cm->sendWithConnection(cm, ctx.clientId,
                                             &UA_KEYVALUEMAP_NULL, &tooLarge),
                      UA_STATUSCODE_BADREQUESTTOOLARGE);
    ck_assert_ptr_null(tooLarge.data);

    /* Listener receive limit on a client without a send limit. */
    sendPatternRequest(cm, &ctx, 4);
    runUntilCount(eventLoop, &ctx.responseCount, 1, 200);
    ck_assert_uint_eq(ctx.requestCount, 1);
    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    runUntilCount(eventLoop, &ctx.clientClosingCount, 1, 100);
    openHTTPClient(cm, &ctx, 0, 0, false, NULL, NULL, NULL);
    sendPatternRequest(cm, &ctx, 5);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 2, 200);
    ck_assert_uint_eq(ctx.requestCount, 1);
    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);

    /* Listener response and client receive limits. */
    memset(&ctx, 0, sizeof(ctx));
    ctx.mode = HTTP_TEST_FIXED_RESPONSE;
    ctx.fixedResponseSize = 5;
    cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 4, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 4, 0, false, NULL, NULL, NULL);
    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.requestCount, 1, 100);
    ck_assert_uint_eq(ctx.firstSendStatus, UA_STATUSCODE_BADRESPONSETOOLARGE);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.acceptedConnectionId),
                      UA_STATUSCODE_GOOD);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 1, 100);
    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);

    memset(&ctx, 0, sizeof(ctx));
    ctx.mode = HTTP_TEST_FIXED_RESPONSE;
    ctx.fixedResponseSize = 5;
    cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);
    openHTTPClient(cm, &ctx, 4, 0, false, NULL, NULL, NULL);
    sendPatternRequest(cm, &ctx, 0);
    runUntilCount(eventLoop, &ctx.responseCompleteCount, 1, 200);
    ck_assert_uint_eq(ctx.lastRequestStatus,
                      UA_STATUSCODE_BADRESPONSETOOLARGE);
    ck_assert_uint_eq(ctx.clientBodyBytes, 0);
    closeHTTPConnectionIfPresent(cm, ctx.clientId);
    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);
} END_TEST

START_TEST(parameterValidation) {
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_Boolean validate = true;
    UA_KeyValuePair parameters[4] = {0};
    parameters[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&parameters[0].value, &port,
                         &UA_TYPES[UA_TYPES_UINT16]);
    parameters[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&parameters[1].value, &listen,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    parameters[2].key = UA_QUALIFIEDNAME(0, "validate");
    UA_Variant_setScalar(&parameters[2].value, &validate,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap validMap = {3, parameters};
    ck_assert_uint_eq(cm->openConnection(cm, &validMap, NULL, NULL, NULL),
                      UA_STATUSCODE_GOOD);

    UA_Boolean useSSL = true;
    parameters[3].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&parameters[3].value, &useSSL,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap incompleteTlsMap = {4, parameters};
    ck_assert_uint_eq(cm->openConnection(cm, &incompleteTlsMap,
                                         NULL, NULL, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    const char *oldStringParameters[] = {
        "username", "password", "client-key-password"
    };
    UA_String oldStringValue = UA_STRING("unused");
    for(size_t i = 0; i < 3; i++) {
        parameters[3].key = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)oldStringParameters[i]);
        UA_Variant_setScalar(&parameters[3].value, &oldStringValue,
                             &UA_TYPES[UA_TYPES_STRING]);
        UA_KeyValueMap oldMap = {4, parameters};
        ck_assert_uint_ne(cm->openConnection(cm, &oldMap, NULL, NULL, NULL),
                          UA_STATUSCODE_GOOD);
    }
    const char *oldByteStringParameters[] = {
        "ca-cert", "client-cert", "client-key"
    };
    UA_ByteString oldByteStringValue = UA_BYTESTRING("unused");
    for(size_t i = 0; i < 3; i++) {
        parameters[3].key = UA_QUALIFIEDNAME(
            0, (char*)(uintptr_t)oldByteStringParameters[i]);
        UA_Variant_setScalar(&parameters[3].value, &oldByteStringValue,
                             &UA_TYPES[UA_TYPES_BYTESTRING]);
        UA_KeyValueMap oldMap = {4, parameters};
        ck_assert_uint_ne(cm->openConnection(cm, &oldMap, NULL, NULL, NULL),
                          UA_STATUSCODE_GOOD);
    }

    UA_String oldHeader = UA_STRING("x-test=value");
    UA_KeyValuePair oldSendParameter = {0};
    oldSendParameter.key = UA_QUALIFIEDNAME(0, "header");
    UA_Variant_setScalar(&oldSendParameter.value, &oldHeader,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap oldSendMap = {1, &oldSendParameter};
    UA_ByteString body = UA_BYTESTRING_ALLOC("consumed");
    ck_assert_uint_ne(cm->sendWithConnection(cm, 1, &oldSendMap, &body),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_null(body.data);

    UA_UInt16 invalidStatus = 99;
    UA_KeyValuePair invalidStatusParameter = {0};
    invalidStatusParameter.key = UA_QUALIFIEDNAME(0, "status-code");
    UA_Variant_setScalar(&invalidStatusParameter.value, &invalidStatus,
                         &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap invalidStatusMap = {1, &invalidStatusParameter};
    ck_assert_uint_eq(cm->sendWithConnection(cm, 1, &invalidStatusMap, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_String contentLength = UA_STRING("5");
    UA_KeyValuePair managedHeader = {0};
    managedHeader.key = UA_QUALIFIEDNAME(0, "content-length");
    UA_Variant_setScalar(&managedHeader.value, &contentLength,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValuePair headersParameter = {0};
    headersParameter.key = UA_QUALIFIEDNAME(0, "headers");
    UA_Variant_setArray(&headersParameter.value, &managedHeader, 1,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    UA_KeyValueMap managedHeaderMap = {1, &headersParameter};
    ck_assert_uint_eq(cm->sendWithConnection(cm, 1, &managedHeaderMap, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_String injectedValue = UA_STRING("ok\r\nx-injected: yes");
    managedHeader.key = UA_QUALIFIEDNAME(0, "x-test");
    UA_Variant_setScalar(&managedHeader.value, &injectedValue,
                         &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(cm->sendWithConnection(cm, 1, &managedHeaderMap, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_String validValue = UA_STRING("value");
    managedHeader.key = UA_QUALIFIEDNAME(0, "x invalid");
    UA_Variant_setScalar(&managedHeader.value, &validValue,
                         &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(cm->sendWithConnection(cm, 1, &managedHeaderMap, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    stopEventLoop(eventLoop);
} END_TEST

START_TEST(chunkedRequestAndKeepAlive) {
#ifndef _WIN32
    HTTPTestContext ctx = {0};
    ctx.mode = HTTP_TEST_FIXED_RESPONSE;
    ctx.fixedResponseSize = 2;
    UA_ConnectionManager *cm = UA_ConnectionManager_new_HTTP(UA_STRING("http"));
    UA_EventLoop *eventLoop = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(cm); ck_assert_ptr_nonnull(eventLoop);
    ck_assert_uint_eq(eventLoop->registerEventSource(eventLoop, &cm->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eventLoop->start(eventLoop), UA_STATUSCODE_GOOD);
    openHTTPListener(cm, &ctx, 30, 0, 0, false, NULL, NULL, NULL);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(ctx.port);
    ck_assert_int_eq(inet_pton(AF_INET, "127.0.0.1",
                               &serverAddress.sin_addr), 1);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);

    int fd2 = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd2, 0);
    ck_assert_int_eq(connect(fd2, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);

    const char chunkedHeaders[] =
        "POST /chunked-a HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: keep-alive\r\n\r\n";
    const char chunkedHeaders2[] =
        "POST /chunked-b HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: keep-alive\r\n\r\n";
    ck_assert_int_eq(send(fd, chunkedHeaders, sizeof(chunkedHeaders) - 1, 0),
                     (ssize_t)(sizeof(chunkedHeaders) - 1));
    ck_assert_int_eq(send(fd2, chunkedHeaders2, sizeof(chunkedHeaders2) - 1, 0),
                     (ssize_t)(sizeof(chunkedHeaders2) - 1));
    const UA_Byte firstChunk[] = {'2', '\r', '\n', 0, 1, '\r', '\n'};
    const UA_Byte secondChunk[] = {
        '3', '\r', '\n', 2, 3, 4, '\r', '\n', '0', '\r', '\n', '\r', '\n'
    };
    const UA_Byte firstChunk2[] = {'2', '\r', '\n', 5, 6, '\r', '\n'};
    const UA_Byte secondChunk2[] = {
        '3', ';', 'x', '=', 'y', '\r', '\n', 7, 8, 9, '\r', '\n',
        '0', '\r', '\n', 'X', '-', 'T', 'r', 'a', 'i', 'l', 'e', 'r', ':',
        ' ', 'o', 'k', '\r', '\n', '\r', '\n'
    };
    /* Feed the first chunk across every possible callback boundary and
     * interleave a second client's chunks. Each raw WSI owns its parser. */
    for(size_t i = 0; i < sizeof(firstChunk); i++) {
        ck_assert_int_eq(send(fd, &firstChunk[i], 1, 0), 1);
        eventLoop->run(eventLoop, 0);
    }
    ck_assert_int_eq(send(fd2, firstChunk2, sizeof(firstChunk2), 0),
                     (ssize_t)sizeof(firstChunk2));
    ck_assert_int_eq(send(fd, secondChunk, sizeof(secondChunk), 0),
                     (ssize_t)sizeof(secondChunk));
    runUntilCount(eventLoop, &ctx.requestCount, 1, 200);
    ck_assert_uint_eq(ctx.requestCount, 1);
    ck_assert_uint_eq(ctx.acceptedConnectionCount, 2);
    ck_assert_uint_eq(ctx.serverBodyBytes, 5);
    ck_assert(!ctx.bodyMismatch);
    char response[2048];
    runUntilSocketReadable(eventLoop, fd, 100);
    ssize_t received = recv(fd, response, sizeof(response), 0);
    ck_assert_int_gt(received, 0);
    close(fd);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 1, 200);

    ck_assert_int_eq(send(fd2, secondChunk2, sizeof(secondChunk2), 0),
                     (ssize_t)sizeof(secondChunk2));
    runUntilCount(eventLoop, &ctx.requestCount, 2, 200);
    ck_assert_uint_eq(ctx.requestCount, 2);
    ck_assert_uint_eq(ctx.serverBodyBytes, 10);
    ck_assert(!ctx.bodyMismatch);
    runUntilSocketReadable(eventLoop, fd2, 100);
    received = recv(fd2, response, sizeof(response), 0);
    ck_assert_int_gt(received, 0);
    close(fd2);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 2, 200);

    /* Two requests over one HTTP/1.1 connection exercise keep-alive reuse. */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    const char firstRequest[] =
        "CUSTOM /first HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: keep-alive\r\n\r\n";
    ck_assert_int_eq(send(fd, firstRequest, sizeof(firstRequest) - 1, 0),
                     (ssize_t)(sizeof(firstRequest) - 1));
    runUntilCount(eventLoop, &ctx.requestCount, 3, 200);
    ck_assert_uint_eq(ctx.requestCount, 3);
    ck_assert_uint_eq(ctx.acceptedConnectionCount, 3);
    uintptr_t keepAliveConnectionId = ctx.acceptedConnectionId;
    runUntilSocketReadable(eventLoop, fd, 100);
    received = recv(fd, response, sizeof(response), 0);
    ck_assert_int_gt(received, 0);

    const char secondRequest[] =
        "GET /second HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n\r\n";
    ck_assert_int_eq(send(fd, secondRequest, sizeof(secondRequest) - 1, 0),
                     (ssize_t)(sizeof(secondRequest) - 1));
    runUntilCount(eventLoop, &ctx.requestCount, 4, 200);
    ck_assert_uint_eq(ctx.requestCount, 4);
    ck_assert_uint_eq(ctx.acceptedConnectionId, keepAliveConnectionId);
    ck_assert_uint_eq(ctx.acceptedConnectionCount, 3);
    runUntilSocketReadable(eventLoop, fd, 100);
    received = recv(fd, response, sizeof(response), 0);
    ck_assert_int_gt(received, 0);
    close(fd);
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 3, 200);

    /* Ambiguous framing is rejected before delivery. */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    const char ambiguousRequest[] =
        "POST /ambiguous HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Content-Length: 1\r\n"
        "Transfer-Encoding: chunked\r\n\r\n";
    ck_assert_int_eq(send(fd, ambiguousRequest, sizeof(ambiguousRequest) - 1, 0),
                     (ssize_t)(sizeof(ambiguousRequest) - 1));
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 4, 200);
    ck_assert_uint_eq(ctx.requestCount, 4);
    ck_assert_uint_eq(ctx.acceptedConnectionCount, 4);
    close(fd);

    /* Malformed chunk framing is rejected by the vendored parser. */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    const char malformedChunk[] =
        "POST /malformed HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Transfer-Encoding: chunked\r\n\r\n"
        "z\r\n";
    ck_assert_int_eq(send(fd, malformedChunk, sizeof(malformedChunk) - 1, 0),
                     (ssize_t)(sizeof(malformedChunk) - 1));
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 5, 200);
    ck_assert_uint_eq(ctx.requestCount, 4);
    ck_assert_uint_eq(ctx.acceptedConnectionCount, 5);
    close(fd);

    /* A second request must not enter while the first request on the same
     * accepted connection is still waiting for its response. */
    ctx.mode = HTTP_TEST_NO_RESPONSE;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    ck_assert_int_eq(send(fd, firstRequest, sizeof(firstRequest) - 1, 0),
                     (ssize_t)(sizeof(firstRequest) - 1));
    runUntilCount(eventLoop, &ctx.requestCount, 5, 200);
    ck_assert_uint_eq(ctx.acceptedConnectionCount, 6);
    ck_assert_int_eq(send(fd, secondRequest, sizeof(secondRequest) - 1, 0),
                     (ssize_t)(sizeof(secondRequest) - 1));
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 6, 200);
    ck_assert_uint_eq(ctx.requestCount, 5);
    close(fd);

    /* The same invariant applies when two requests arrive in one read. The
     * parser delivers the first request only and rejects the pipelined tail. */
    ctx.mode = HTTP_TEST_FIXED_RESPONSE;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    ck_assert_int_ge(fd, 0);
    ck_assert_int_eq(connect(fd, (struct sockaddr*)&serverAddress,
                             sizeof(serverAddress)), 0);
    const char pipelinedRequests[] =
        "GET /pipeline-first HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: keep-alive\r\n\r\n"
        "GET /pipeline-second HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Connection: close\r\n\r\n";
    ck_assert_int_eq(send(fd, pipelinedRequests,
                          sizeof(pipelinedRequests) - 1, 0),
                     (ssize_t)(sizeof(pipelinedRequests) - 1));
    runUntilCount(eventLoop, &ctx.acceptedClosingCount, 7, 200);
    ck_assert_uint_eq(ctx.requestCount, 6);
    ck_assert_uint_eq(ctx.acceptedConnectionCount, 7);
    close(fd);

    ck_assert_uint_eq(cm->closeConnection(cm, ctx.listenerId), UA_STATUSCODE_GOOD);
    stopEventLoop(eventLoop);
#endif
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test HTTP EventLoop");
    TCase *tc = tcase_create("test cases");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, sendGetRequest);
    tcase_add_test(tc, sendPostRequest);
    tcase_add_test(tc, serverBinaryRoundtrip);
    suite_add_tcase(s, tc);

    TCase *largeBodies = tcase_create("large-and-empty-bodies");
    tcase_add_test(largeBodies, serverLargeAndEmptyBodies);
    suite_add_tcase(s, largeBodies);
    TCase *lifecycle = tcase_create("request-lifecycle");
    tcase_add_test(lifecycle, serverRequestLifecycle);
    tcase_add_test(lifecycle, serverBrokenCarrierIsolation);
    tcase_add_test(lifecycle, clientRequestFailureIsolation);
    tcase_add_test(lifecycle, clientSynchronousConnectionFailure);
    tcase_add_test(lifecycle, clientRequestTimeoutIsolation);
    tcase_add_test(lifecycle, serverResponseFailureIsolation);
    tcase_add_test(lifecycle, serverListenerCloseWithIdleCarrier);
    tcase_add_test(lifecycle, serverListenerCloseFromRequestCallback);
    suite_add_tcase(s, lifecycle);
    TCase *timeouts = tcase_create("timeouts-and-listener-close");
    tcase_add_test(timeouts, serverTimeoutAndListenerClose);
    tcase_add_test(timeouts, serverReceiveAndKeepAliveTimeouts);
    suite_add_tcase(s, timeouts);
    TCase *tls = tcase_create("mutual-tls");
    tcase_add_test(tls, clientServerMutualTls);
    suite_add_tcase(s, tls);
    TCase *limits = tcase_create("message-size-limits");
    tcase_add_test(limits, messageSizeLimits);
    suite_add_tcase(s, limits);
#ifdef UA_ENABLE_HTTP_COMPRESSION
    TCase *compression = tcase_create("content-encoding");
    tcase_add_test(compression, serverContentEncodingAndExpansionLimit);
    tcase_add_test(compression, clientCompressedResponse);
    suite_add_tcase(s, compression);
#endif
    TCase *validation = tcase_create("parameter-validation");
    tcase_add_test(validation, parameterValidation);
    suite_add_tcase(s, validation);
    TCase *handles = tcase_create("client-request-handles");
    tcase_add_test(handles, clientRequestHandles);
    tcase_add_test(handles, clientRejectsDuplicateContentEncoding);
    suite_add_tcase(s, handles);
    TCase *http11 = tcase_create("chunked-and-keep-alive");
    tcase_add_test(http11, chunkedRequestAndKeepAlive);
    suite_add_tcase(s, http11);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
