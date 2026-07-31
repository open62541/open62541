/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <check.h>
#include <libwebsockets.h>
#include <stdio.h>

#ifdef UA_ENABLE_DISCOVERY
#include "server/ua_server_internal.h"
#endif

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

typedef struct {
    uintptr_t listenerId, acceptedId, clientId;
    UA_UInt16 port;
    UA_Boolean clientEstablished;
    UA_Boolean clientClosed;
    UA_Boolean openingConnection;
    UA_Boolean closingWasReentrant;
    size_t serverMessages, clientMessages;
} TestContext;

typedef struct {
    struct lws_context *context;
    struct lws *wsi;
    unsigned writeStep;
    UA_Boolean established;
    UA_Boolean closed;
    unsigned char buffer[LWS_PRE + 4];
} RawWebSocketClient;

static int
rawWebSocketCallback(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len) {
    (void)user; (void)in; (void)len;
    RawWebSocketClient *raw =
        (RawWebSocketClient*)lws_context_user(lws_get_context(wsi));
    switch(reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        raw->established = true;
        raw->wsi = wsi;
        lws_callback_on_writable(wsi);
        break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
        if(raw->writeStep == 0) {
            memcpy(raw->buffer + LWS_PRE, "pi", 2);
            if(lws_write(wsi, raw->buffer + LWS_PRE, 2,
                         (enum lws_write_protocol)
                         (LWS_WRITE_BINARY | LWS_WRITE_NO_FIN)) != 2)
                return -1;
        } else if(raw->writeStep == 1) {
            memcpy(raw->buffer + LWS_PRE, "ng", 2);
            if(lws_write(wsi, raw->buffer + LWS_PRE, 2,
                         LWS_WRITE_CONTINUATION) != 2)
                return -1;
        } else if(raw->writeStep == 2) {
            memcpy(raw->buffer + LWS_PRE, "text", 4);
            if(lws_write(wsi, raw->buffer + LWS_PRE, 4,
                         LWS_WRITE_TEXT) != 4)
                return -1;
        }
        raw->writeStep++;
        if(raw->writeStep < 3)
            lws_callback_on_writable(wsi);
        break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    case LWS_CALLBACK_CLIENT_CLOSED:
        raw->closed = true;
        raw->wsi = NULL;
        break;
    default:
        break;
    }
    return 0;
}

static const struct lws_protocols rawWebSocketProtocols[] = {
    {.name = "opcua+uacp", .callback = rawWebSocketCallback},
    {0}
};

static void callback(UA_ConnectionManager *cm, uintptr_t id, void *application,
                     void **connectionContext, UA_ConnectionState state,
                     const UA_KeyValueMap *params, UA_ByteString msg) {
    (void)cm; (void)connectionContext;
    TestContext *ctx = (TestContext*)application;
    const UA_UInt16 *port = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "listen-port"), &UA_TYPES[UA_TYPES_UINT16]);
    if(port) {
        ctx->listenerId = id;
        ctx->port = *port;
        return;
    }
    if(state == UA_CONNECTIONSTATE_ESTABLISHED && id != ctx->clientId &&
       ctx->clientId != 0 && ctx->acceptedId == 0)
        ctx->acceptedId = id;
    if(state == UA_CONNECTIONSTATE_ESTABLISHED && id == ctx->clientId)
        ctx->clientEstablished = true;
    if(state == UA_CONNECTIONSTATE_CLOSING && id == ctx->clientId) {
        ctx->clientClosed = true;
        ctx->closingWasReentrant = ctx->openingConnection;
    }
    if(msg.length) {
        if(id == ctx->clientId)
            ctx->clientMessages++;
        else
            ctx->serverMessages++;
    }
}

static void run(UA_EventLoop *el, UA_Boolean *done) {
    for(size_t i = 0; i < 200 && !*done; i++)
        el->run(el, 50);
    ck_assert(*done);
}

static UA_ConnectionManager *
findWebSocketConnectionManager(UA_EventLoop *el) {
    const UA_String websocket = UA_STRING_STATIC("websocket");
    for(UA_EventSource *es = el->eventSources; es != NULL; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(UA_String_equal(&cm->protocol, &websocket))
            return cm;
    }
    return NULL;
}

static UA_Client *
newTrustedWebSocketClient(UA_ServerConfig *serverConfig) {
    UA_ClientConfig clientConfig;
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_ByteString_copy(&serverConfig->webSocketCertificate,
                           &clientConfig.webSocketCaCertificate),
        UA_STATUSCODE_GOOD);
    UA_Client *client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    return client;
}

START_TEST(clientDefaultHasWebSocketManager) {
    UA_ClientConfig config;
    memset(&config, 0, sizeof(config));
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&config),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_nonnull(findWebSocketConnectionManager(config.eventLoop));
    ck_assert_uint_eq(config.webSocketMaxQueueSize, 1U << 20);
    UA_ClientConfig_clear(&config);
}
END_TEST

START_TEST(clientConnectionFailureIsNotReentrant) {
    TestContext ctx = {0};
    UA_ConnectionManager *ws =
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("ws-failure"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(ws); ck_assert_ptr_nonnull(el);
    ck_assert_uint_eq(el->registerEventSource(el, &ws->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_String address = UA_STRING("does-not-exist.invalid");
    UA_UInt16 port = 4843;
    UA_KeyValuePair cp[2];
    cp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&cp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    cp[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&cp[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap cpm = {2, cp};

    ctx.clientId = 1;
    ctx.openingConnection = true;
    ck_assert_uint_eq(ws->openConnection(ws, &cpm, &ctx, &ctx, callback),
                      UA_STATUSCODE_GOOD);
    ctx.openingConnection = false;
    ck_assert(!ctx.clientClosed);

    run(el, &ctx.clientClosed);
    ck_assert(!ctx.closingWasReentrant);

    el->stop(el);
    UA_Boolean stopped = false;
    for(size_t i = 0; i < 200 && !stopped; i++) {
        el->run(el, 50);
        stopped = el->state == UA_EVENTLOOPSTATE_STOPPED;
    }
    ck_assert(stopped);
    el->free(el);
}
END_TEST

START_TEST(clientServerBinary) {
    TestContext ctx = {0};
    UA_ConnectionManager *ws =
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("ws"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(ws); ck_assert_ptr_nonnull(el);
    ck_assert_uint_eq(el->registerEventSource(el, &ws->eventSource), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_String path = UA_STRING("/opcua");
    UA_String subprotocol = UA_STRING("opcua+uacp");
    UA_Boolean binaryOnly = true;
    UA_UInt32 recvMaxMessageSize = 4;
    UA_UInt32 sendMaxMessageSize = 4;
    UA_KeyValuePair lp[7];
    lp[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&lp[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    lp[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&lp[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    lp[2].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&lp[2].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    lp[3].key = UA_QUALIFIEDNAME(0, "subprotocol");
    UA_Variant_setScalar(&lp[3].value, &subprotocol, &UA_TYPES[UA_TYPES_STRING]);
    lp[4].key = UA_QUALIFIEDNAME(0, "binary-only");
    UA_Variant_setScalar(&lp[4].value, &binaryOnly, &UA_TYPES[UA_TYPES_BOOLEAN]);
    lp[5].key = UA_QUALIFIEDNAME(0, "recv-max-message-size");
    UA_Variant_setScalar(&lp[5].value, &recvMaxMessageSize,
                         &UA_TYPES[UA_TYPES_UINT32]);
    lp[6].key = UA_QUALIFIEDNAME(0, "send-max-message-size");
    UA_Variant_setScalar(&lp[6].value, &sendMaxMessageSize,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_KeyValueMap lpm = {7, lp};
    ck_assert_uint_eq(ws->openConnection(ws, &lpm, &ctx, &ctx, callback), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(ctx.port, 0);

    UA_String address = UA_STRING("127.0.0.1");
    UA_UInt32 sendMaxQueueSize = 5;
    UA_KeyValuePair cp[5];
    cp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&cp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    cp[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&cp[1].value, &ctx.port, &UA_TYPES[UA_TYPES_UINT16]);
    cp[2].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&cp[2].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    cp[3].key = UA_QUALIFIEDNAME(0, "subprotocol");
    UA_Variant_setScalar(&cp[3].value, &subprotocol, &UA_TYPES[UA_TYPES_STRING]);
    cp[4].key = UA_QUALIFIEDNAME(0, "send-max-queue-size");
    UA_Variant_setScalar(&cp[4].value, &sendMaxQueueSize,
                         &UA_TYPES[UA_TYPES_UINT32]);
    UA_KeyValueMap cpm = {5, cp};
    ck_assert_uint_eq(ws->openConnection(ws, &cpm, &ctx, &ctx, callback), UA_STATUSCODE_GOOD);
    ctx.clientId = ctx.listenerId + 1;
    UA_Boolean connected = false;
    for(size_t i = 0; i < 200 && !connected; i++) {
        el->run(el, 50);
        connected = ctx.acceptedId != 0 && ctx.clientEstablished;
    }
    ck_assert(connected);

    UA_ByteString msg = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(ws->allocNetworkBuffer(ws, ctx.clientId, &msg, 4), UA_STATUSCODE_GOOD);
    memcpy(msg.data, "ping", 4);
    ck_assert_uint_eq(ws->sendWithConnection(ws, ctx.clientId, &UA_KEYVALUEMAP_NULL, &msg), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ws->allocNetworkBuffer(ws, ctx.clientId, &msg, 2),
                      UA_STATUSCODE_GOOD);
    memcpy(msg.data, "xx", 2);
    ck_assert_uint_eq(
        ws->sendWithConnection(ws, ctx.clientId, &UA_KEYVALUEMAP_NULL, &msg),
        UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_Boolean gotServer = false;
    for(size_t i = 0; i < 200 && !gotServer; i++) { el->run(el, 50); gotServer = ctx.serverMessages == 1; }
    ck_assert(gotServer);

    ck_assert_uint_eq(ws->allocNetworkBuffer(ws, ctx.acceptedId, &msg, 4), UA_STATUSCODE_GOOD);
    memcpy(msg.data, "pong", 4);
    ck_assert_uint_eq(ws->sendWithConnection(ws, ctx.acceptedId, &UA_KEYVALUEMAP_NULL, &msg), UA_STATUSCODE_GOOD);
    UA_Boolean gotClient = false;
    for(size_t i = 0; i < 200 && !gotClient; i++) { el->run(el, 50); gotClient = ctx.clientMessages == 1; }
    ck_assert(gotClient);

    ck_assert_uint_eq(
        ws->allocNetworkBuffer(ws, ctx.acceptedId, &msg, 5),
        UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    ck_assert_uint_eq(ws->allocNetworkBuffer(ws, ctx.clientId, &msg, 5),
                      UA_STATUSCODE_GOOD);
    memcpy(msg.data, "large", 5);
    ck_assert_uint_eq(
        ws->sendWithConnection(ws, ctx.clientId, &UA_KEYVALUEMAP_NULL, &msg),
        UA_STATUSCODE_GOOD);
    run(el, &ctx.clientClosed);

    /* Closing a connection with queued output must discard the queue without
     * re-entering the application callback or delaying EventLoop shutdown. */
    ctx.clientId = ctx.acceptedId + 1;
    ctx.acceptedId = 0;
    ctx.clientEstablished = false;
    ctx.clientClosed = false;
    ck_assert_uint_eq(ws->openConnection(ws, &cpm, &ctx, &ctx, callback),
                      UA_STATUSCODE_GOOD);
    connected = false;
    for(size_t i = 0; i < 200 && !connected; i++) {
        el->run(el, 50);
        connected = ctx.acceptedId != 0 && ctx.clientEstablished;
    }
    ck_assert(connected);
    ck_assert_uint_eq(ws->allocNetworkBuffer(ws, ctx.clientId, &msg, 4),
                      UA_STATUSCODE_GOOD);
    memcpy(msg.data, "drop", 4);
    ck_assert_uint_eq(
        ws->sendWithConnection(ws, ctx.clientId, &UA_KEYVALUEMAP_NULL, &msg),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ws->closeConnection(ws, ctx.clientId),
                      UA_STATUSCODE_GOOD);
    run(el, &ctx.clientClosed);

    el->stop(el);
    UA_Boolean stopped = false;
    for(size_t i = 0; i < 200 && !stopped; i++) { el->run(el, 50); stopped = el->state == UA_EVENTLOOPSTATE_STOPPED; }
    ck_assert(stopped);
    el->free(el);
}
END_TEST

START_TEST(rejectsInvalidConnectionParameters) {
    TestContext ctx = {0};
    UA_ConnectionManager *ws =
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("ws-invalid"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(ws); ck_assert_ptr_nonnull(el);
    ck_assert_uint_eq(el->registerEventSource(el, &ws->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_String invalidPath = UA_STRING("opcua");
    UA_String emptySubprotocol = UA_STRING("");
    UA_Boolean useSSL = true;
    UA_ByteString certificate = loadFile("server_cert.der");
    ck_assert_ptr_nonnull(certificate.data);
    UA_KeyValuePair params[5];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[2].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&params[2].value, &invalidPath,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap paramsMap = {3, params};
    ck_assert_uint_eq(
        ws->openConnection(ws, &paramsMap, &ctx, &ctx, callback),
        UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_String path = UA_STRING("/opcua");
    UA_Variant_setScalar(&params[2].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    params[3].key = UA_QUALIFIEDNAME(0, "subprotocol");
    UA_Variant_setScalar(&params[3].value, &emptySubprotocol,
                         &UA_TYPES[UA_TYPES_STRING]);
    paramsMap.mapSize = 4;
    ck_assert_uint_eq(
        ws->openConnection(ws, &paramsMap, &ctx, &ctx, callback),
        UA_STATUSCODE_BADINVALIDARGUMENT);

    params[3].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&params[3].value, &useSSL,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[4].key = UA_QUALIFIEDNAME(0, "certificate");
    UA_Variant_setScalar(&params[4].value, &certificate,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    paramsMap.mapSize = 5;
    ck_assert_uint_eq(
        ws->openConnection(ws, &paramsMap, &ctx, &ctx, callback),
        UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_ByteString_clear(&certificate);

    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED)
        el->run(el, 10);
    el->free(el);
}
END_TEST

START_TEST(fragmentedBinaryAndTextRejection) {
    TestContext ctx = {0};
    UA_ConnectionManager *ws =
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("ws-frames"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(ws); ck_assert_ptr_nonnull(el);
    ck_assert_uint_eq(el->registerEventSource(el, &ws->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_Boolean binaryOnly = true;
    UA_String path = UA_STRING("/opcua");
    UA_String subprotocol = UA_STRING("opcua+uacp");
    UA_KeyValuePair params[5];
    params[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&params[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    params[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&params[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    params[2].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&params[2].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    params[3].key = UA_QUALIFIEDNAME(0, "subprotocol");
    UA_Variant_setScalar(&params[3].value, &subprotocol,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[4].key = UA_QUALIFIEDNAME(0, "binary-only");
    UA_Variant_setScalar(&params[4].value, &binaryOnly,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap paramsMap = {5, params};
    ck_assert_uint_eq(ws->openConnection(ws, &paramsMap, &ctx, &ctx, callback),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(ctx.port, 0);

    RawWebSocketClient raw = {0};
    struct lws_context_creation_info contextInfo;
    memset(&contextInfo, 0, sizeof(contextInfo));
    contextInfo.port = CONTEXT_PORT_NO_LISTEN;
    contextInfo.protocols = rawWebSocketProtocols;
    contextInfo.user = &raw;
    raw.context = lws_create_context(&contextInfo);
    ck_assert_ptr_nonnull(raw.context);

    struct lws_client_connect_info connectInfo;
    memset(&connectInfo, 0, sizeof(connectInfo));
    connectInfo.context = raw.context;
    connectInfo.address = "127.0.0.1";
    connectInfo.host = connectInfo.address;
    connectInfo.origin = connectInfo.address;
    connectInfo.port = ctx.port;
    connectInfo.path = "/opcua";
    connectInfo.protocol = "opcua+uacp";
    connectInfo.local_protocol_name = "opcua+uacp";
    raw.wsi = lws_client_connect_via_info(&connectInfo);
    ck_assert_ptr_nonnull(raw.wsi);

    for(size_t i = 0; i < 500 && !raw.closed; i++) {
        el->run(el, 10);
        lws_service(raw.context, 0);
    }
    ck_assert(raw.established);
    ck_assert(raw.closed);
    ck_assert_uint_eq(ctx.serverMessages, 1);
    lws_context_destroy(raw.context);

    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED)
        el->run(el, 10);
    el->free(el);
}
END_TEST

static UA_Server *
startWebSocketServer(UA_UInt16 *portOut, UA_ConnectionManager **wsOut,
                     UA_Boolean addSecureEndpoint) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert(config->tcpEnabled);
    ck_assert(!config->webSocketEnabled);
    ck_assert_uint_eq(config->webSocketMaxQueueSize,
                      config->webSocketBufSize * 16);
    UA_ConnectionManager *ws =
        findWebSocketConnectionManager(config->eventLoop);
    ck_assert_ptr_nonnull(ws);

    UA_ByteString certificate = loadFile("server_cert.der");
    UA_ByteString privateKey = loadFile("server_key.der");
    ck_assert_ptr_nonnull(certificate.data);
    ck_assert_ptr_nonnull(privateKey.data);
#ifdef UA_ENABLE_ENCRYPTION
    if(addSecureEndpoint) {
        ck_assert_uint_eq(
            UA_ServerConfig_addSecurityPolicyBasic256Sha256(
                config, &certificate, &privateKey),
            UA_STATUSCODE_GOOD);
        const UA_String policyUri = UA_STRING_STATIC(
            "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
        ck_assert_uint_eq(
            UA_ServerConfig_addEndpoint(
                config, policyUri,
                UA_MESSAGESECURITYMODE_SIGNANDENCRYPT),
            UA_STATUSCODE_GOOD);
    }
#else
    (void)addSecureEndpoint;
#endif
    config->webSocketEnabled = true;
    config->webSocketCertificate = certificate;
    config->webSocketPrivateKey = privateKey;

    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls = NULL;
    config->serverUrlsSize = 0;
    const UA_String url = UA_STRING_STATIC("opc.wss://:0/opcua");
    ck_assert_uint_eq(UA_Array_appendCopy((void**)&config->serverUrls,
                                         &config->serverUrlsSize, &url,
                                         &UA_TYPES[UA_TYPES_STRING]),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(config->applicationDescription.discoveryUrlsSize, 1);
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    *portOut = 0;
    ck_assert_uint_eq(
        UA_parseEndpointUrl(&config->applicationDescription.discoveryUrls[0],
                            &hostname, portOut, &path),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(hostname.length, 0);
    ck_assert_uint_ne(*portOut, 0);
    const UA_String expectedPath = UA_STRING_STATIC("opcua");
    ck_assert(UA_String_equal(&path, &expectedPath));
    *wsOut = ws;
    return server;
}

START_TEST(serverWebSocketListener) {
    UA_UInt16 port = 0;
    UA_ConnectionManager *ws = NULL;
    UA_Server *server = startWebSocketServer(&port, &ws, false);
    UA_ServerConfig *config = UA_Server_getConfig(server);

#ifdef UA_ENABLE_DISCOVERY
    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    ck_assert_uint_eq(
        setCurrentEndpointsArray(
            server, config->applicationDescription.discoveryUrls[0], NULL, 0,
            &endpoints, &endpointsSize),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(endpointsSize, 0);
    const UA_String wssTransport = UA_STRING_STATIC(
        "http://opcfoundation.org/UA-Profile/Transport/wss-uasc-uabinary");
    for(size_t i = 0; i < endpointsSize; i++)
        ck_assert(UA_String_equal(&endpoints[i].transportProfileUri,
                                  &wssTransport));
    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
#endif

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

START_TEST(clientServerWebSocketRoundtrip) {
    UA_UInt16 port = 0;
    UA_ConnectionManager *ws = NULL;
    UA_Server *server = startWebSocketServer(&port, &ws, false);
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* Connect a full OPC UA client over opcua+uacp WebSockets and execute a
     * service request. Share the EventLoop so synchronous client calls also
     * drive the server in this single-threaded test. */
    UA_Client *client = newTrustedWebSocketClient(config);

    char clientUrl[128];
    snprintf(clientUrl, sizeof(clientUrl),
             "opc.wss://127.0.0.1:%u/opcua", (unsigned)port);

    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    ck_assert_uint_eq(UA_Client_getEndpoints(client, clientUrl,
                                             &endpointsSize, &endpoints),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(endpointsSize, 0);
    const UA_String expectedEndpointUrl = UA_STRING(clientUrl);
    const UA_String expectedProfile = UA_STRING_STATIC(
        "http://opcfoundation.org/UA-Profile/Transport/wss-uasc-uabinary");
    for(size_t i = 0; i < endpointsSize; i++) {
        ck_assert(UA_String_equal(&endpoints[i].endpointUrl,
                                  &expectedEndpointUrl));
        ck_assert(UA_String_equal(&endpoints[i].transportProfileUri,
                                  &expectedProfile));
    }
    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    ck_assert_uint_eq(UA_Client_connect(client, clientUrl), UA_STATUSCODE_GOOD);

    /* Keep two independent SecureChannels active on the listener. */
    UA_Client *secondClient = newTrustedWebSocketClient(config);
    ck_assert_uint_eq(UA_Client_connect(secondClient, clientUrl),
                      UA_STATUSCODE_GOOD);

    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(
        UA_Client_readValueAttribute(
            client,
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
            &value),
        UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value,
                                      &UA_TYPES[UA_TYPES_DATETIME]));
    UA_Variant_clear(&value);

    ck_assert_uint_eq(
        UA_Client_readValueAttribute(
            secondClient,
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
            &value),
        UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value,
                                      &UA_TYPES[UA_TYPES_DATETIME]));
    UA_Variant_clear(&value);

    /* Re-open the SecureChannel and reactivate the existing Session. */
    ck_assert_uint_eq(UA_Client_disconnectSecureChannel(client),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Client_connect(client, clientUrl), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_Client_readValueAttribute(
            client,
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
            &value),
        UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value,
                                      &UA_TYPES[UA_TYPES_DATETIME]));
    UA_Variant_clear(&value);

    ck_assert_uint_eq(UA_Client_disconnect(client), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
    ck_assert_uint_eq(UA_Client_disconnect(secondClient), UA_STATUSCODE_GOOD);
    UA_Client_delete(secondClient);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

START_TEST(serverRejectsIncompleteWebSocketConfiguration) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->webSocketEnabled = true;
    ck_assert_uint_eq(UA_Server_run_startup(server),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    UA_Server_delete(server);

    server = UA_Server_new();
    ck_assert_ptr_nonnull(server);
    config = UA_Server_getConfig(server);
    config->webSocketEnabled = true;
    UA_Array_delete(config->serverUrls, config->serverUrlsSize,
                    &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls = NULL;
    config->serverUrlsSize = 0;
    UA_ByteString certificate = loadFile("server_cert.der");
    UA_ByteString privateKey = loadFile("server_key.der");
    config->webSocketCertificate = certificate;
    config->webSocketPrivateKey = privateKey;
    const UA_String tcpUrl = UA_STRING_STATIC("opc.tcp://localhost:4840");
    ck_assert_uint_eq(UA_Array_appendCopy((void**)&config->serverUrls,
                                         &config->serverUrlsSize, &tcpUrl,
                                         &UA_TYPES[UA_TYPES_STRING]),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    UA_Server_delete(server);
}
END_TEST

START_TEST(clientRejectsUntrustedWebSocketServer) {
    UA_UInt16 port = 0;
    UA_ConnectionManager *ws = NULL;
    UA_Server *server = startWebSocketServer(&port, &ws, false);
    UA_ServerConfig *serverConfig = UA_Server_getConfig(server);

    UA_ClientConfig clientConfig;
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    clientConfig.timeout = 1000;
    clientConfig.noReconnect = true;
    UA_Client *client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    char clientUrl[128];
    snprintf(clientUrl, sizeof(clientUrl),
             "opc.wss://127.0.0.1:%u/opcua", (unsigned)port);

    ck_assert_uint_ne(UA_Client_connect(client, clientUrl), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

START_TEST(clientRejectsWebSocketHostnameMismatch) {
    UA_UInt16 port = 0;
    UA_ConnectionManager *ws = NULL;
    UA_Server *server = startWebSocketServer(&port, &ws, false);
    UA_ServerConfig *serverConfig = UA_Server_getConfig(server);

    UA_ClientConfig clientConfig;
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(UA_ClientConfig_setDefault(&clientConfig),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(
        UA_ByteString_copy(&serverConfig->webSocketCertificate,
                           &clientConfig.webSocketCaCertificate),
        UA_STATUSCODE_GOOD);
    clientConfig.timeout = 1000;
    clientConfig.noReconnect = true;
    UA_Client *client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    char clientUrl[128];
    snprintf(clientUrl, sizeof(clientUrl),
             "opc.wss://localhost:%u/opcua", (unsigned)port);

    ck_assert_uint_ne(UA_Client_connect(client, clientUrl), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

#ifdef UA_ENABLE_ENCRYPTION
START_TEST(clientServerEncryptedWebSocketRoundtrip) {
    UA_UInt16 port = 0;
    UA_ConnectionManager *ws = NULL;
    UA_Server *server = startWebSocketServer(&port, &ws, true);
    UA_ServerConfig *serverConfig = UA_Server_getConfig(server);

    UA_ByteString certificate = loadFile("server_cert.der");
    UA_ByteString privateKey = loadFile("server_key.der");
    ck_assert_ptr_nonnull(certificate.data);
    ck_assert_ptr_nonnull(privateKey.data);

    UA_ClientConfig clientConfig;
    memset(&clientConfig, 0, sizeof(clientConfig));
    clientConfig.eventLoop = serverConfig->eventLoop;
    clientConfig.externalEventLoop = true;
    ck_assert_uint_eq(
        UA_ClientConfig_setDefaultEncryption(
            &clientConfig, certificate, privateKey, &certificate, 1,
            NULL, 0),
        UA_STATUSCODE_GOOD);
    clientConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    clientConfig.securityPolicyUri = UA_STRING_ALLOC(
        "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
    ck_assert_ptr_nonnull(clientConfig.securityPolicyUri.data);
    ck_assert_uint_eq(UA_ByteString_copy(
                          &certificate,
                          &clientConfig.webSocketCaCertificate),
                      UA_STATUSCODE_GOOD);

    UA_Client *client = UA_Client_newWithConfig(&clientConfig);
    ck_assert_ptr_nonnull(client);
    char clientUrl[128];
    snprintf(clientUrl, sizeof(clientUrl),
             "opc.wss://127.0.0.1:%u/opcua", (unsigned)port);
    ck_assert_uint_eq(UA_Client_connect(client, clientUrl), UA_STATUSCODE_GOOD);

    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(
        UA_Client_readValueAttribute(
            client,
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
            &value),
        UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value,
                                      &UA_TYPES[UA_TYPES_DATETIME]));
    UA_Variant_clear(&value);

    ck_assert_uint_eq(UA_Client_disconnect(client), UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST
#endif

START_TEST(clientServerRejectsPolicyMismatch) {
    TestContext ctx = {0};
    UA_ConnectionManager *ws =
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("ws-policy"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(ws); ck_assert_ptr_nonnull(el);
    ck_assert_uint_eq(el->registerEventSource(el, &ws->eventSource),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_String path = UA_STRING("/opcua");
    UA_String subprotocol = UA_STRING("opcua+uacp");
    UA_KeyValuePair lp[4];
    lp[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&lp[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    lp[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&lp[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    lp[2].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&lp[2].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    lp[3].key = UA_QUALIFIEDNAME(0, "subprotocol");
    UA_Variant_setScalar(&lp[3].value, &subprotocol, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap lpm = {4, lp};
    ck_assert_uint_eq(ws->openConnection(ws, &lpm, &ctx, &ctx, callback),
                      UA_STATUSCODE_GOOD);

    UA_String address = UA_STRING("127.0.0.1");
    UA_String unsupported = UA_STRING("unsupported");
    UA_KeyValuePair cp[4];
    cp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&cp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    cp[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&cp[1].value, &ctx.port, &UA_TYPES[UA_TYPES_UINT16]);
    cp[2].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&cp[2].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    cp[3].key = UA_QUALIFIEDNAME(0, "subprotocol");
    UA_Variant_setScalar(&cp[3].value, &unsupported, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap cpm = {4, cp};
    ck_assert_uint_eq(ws->openConnection(ws, &cpm, &ctx, &ctx, callback),
                      UA_STATUSCODE_GOOD);
    ctx.clientId = ctx.listenerId + 1;
    run(el, &ctx.clientClosed);
    ck_assert(!ctx.clientEstablished);
    ck_assert_uint_eq(ctx.acceptedId, 0);

    ctx.clientClosed = false;
    UA_String wrongPath = UA_STRING("/wrong");
    UA_Variant_setScalar(&cp[2].value, &wrongPath, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&cp[3].value, &subprotocol, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(ws->openConnection(ws, &cpm, &ctx, &ctx, callback),
                      UA_STATUSCODE_GOOD);
    ctx.clientId++;
    run(el, &ctx.clientClosed);
    ck_assert(!ctx.clientEstablished);
    ck_assert_uint_eq(ctx.acceptedId, 0);

    ctx.clientClosed = false;
    UA_String offeredProtocols = UA_STRING("unsupported, opcua+uacp");
    UA_Variant_setScalar(&cp[2].value, &path, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&cp[3].value, &offeredProtocols,
                         &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(ws->openConnection(ws, &cpm, &ctx, &ctx, callback),
                      UA_STATUSCODE_GOOD);
    ctx.clientId++;
    UA_Boolean connected = false;
    for(size_t i = 0; i < 200 && !connected; i++) {
        el->run(el, 50);
        connected = ctx.acceptedId != 0 && ctx.clientEstablished;
    }
    ck_assert(connected);

    el->stop(el);
    UA_Boolean stopped = false;
    for(size_t i = 0; i < 200 && !stopped; i++) {
        el->run(el, 50);
        stopped = el->state == UA_EVENTLOOPSTATE_STOPPED;
    }
    ck_assert(stopped);
    el->free(el);
}
END_TEST

START_TEST(clientServerTlsCaCertificate) {
    TestContext ctx = {0};
    UA_ConnectionManager *ws =
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("wss"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(ws); ck_assert_ptr_nonnull(el);
    ck_assert_uint_eq(el->registerEventSource(el, &ws->eventSource), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_Boolean useSSL = true;
    UA_ByteString certificate = loadFile("server_cert.der");
    UA_ByteString privateKey = loadFile("server_key.der");
    ck_assert_ptr_nonnull(certificate.data);
    ck_assert_ptr_nonnull(privateKey.data);
    UA_KeyValuePair lp[5] = {0};
    lp[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&lp[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    lp[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&lp[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    lp[2].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&lp[2].value, &useSSL, &UA_TYPES[UA_TYPES_BOOLEAN]);
    lp[3].key = UA_QUALIFIEDNAME(0, "certificate");
    UA_Variant_setScalar(&lp[3].value, &certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
    lp[4].key = UA_QUALIFIEDNAME(0, "private-key");
    UA_Variant_setScalar(&lp[4].value, &privateKey, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_KeyValueMap lpm = {5, lp};
    ck_assert_uint_eq(ws->openConnection(ws, &lpm, &ctx, &ctx, callback),
                      UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&privateKey);

    UA_String address = UA_STRING("127.0.0.1");
    UA_KeyValuePair cp[4] = {0};
    cp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&cp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    cp[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&cp[1].value, &ctx.port, &UA_TYPES[UA_TYPES_UINT16]);
    cp[2].key = UA_QUALIFIEDNAME(0, "useSSL");
    UA_Variant_setScalar(&cp[2].value, &useSSL, &UA_TYPES[UA_TYPES_BOOLEAN]);
    cp[3].key = UA_QUALIFIEDNAME(0, "ca-certificate");
    UA_Variant_setScalar(&cp[3].value, &certificate,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_KeyValueMap cpm = {4, cp};
    ck_assert_uint_eq(ws->openConnection(ws, &cpm, &ctx, &ctx, callback),
                      UA_STATUSCODE_GOOD);
    ctx.clientId = ctx.listenerId + 1;
    UA_Boolean connected = false;
    for(size_t i = 0; i < 200 && !connected; i++) {
        el->run(el, 50);
        connected = ctx.acceptedId != 0 && ctx.clientEstablished;
    }
    ck_assert(connected);
    UA_ByteString_clear(&certificate);

    el->stop(el);
    UA_Boolean stopped = false;
    for(size_t i = 0; i < 200 && !stopped; i++) {
        el->run(el, 50);
        stopped = el->state == UA_EVENTLOOPSTATE_STOPPED;
    }
    ck_assert(stopped);
    el->free(el);
}
END_TEST

int main(void) {
    Suite *s = suite_create("LWS WebSocket ConnectionManager");
    TCase *tc = tcase_create("integration");
    tcase_add_test(tc, clientDefaultHasWebSocketManager);
    tcase_add_test(tc, clientConnectionFailureIsNotReentrant);
    tcase_add_test(tc, clientServerBinary);
    tcase_add_test(tc, rejectsInvalidConnectionParameters);
    tcase_add_test(tc, fragmentedBinaryAndTextRejection);
    tcase_add_test(tc, serverWebSocketListener);
    tcase_add_test(tc, serverRejectsIncompleteWebSocketConfiguration);
    tcase_add_test(tc, clientServerWebSocketRoundtrip);
    tcase_add_test(tc, clientRejectsUntrustedWebSocketServer);
    tcase_add_test(tc, clientRejectsWebSocketHostnameMismatch);
#ifdef UA_ENABLE_ENCRYPTION
    tcase_add_test(tc, clientServerEncryptedWebSocketRoundtrip);
#endif
    tcase_add_test(tc, clientServerRejectsPolicyMismatch);
    tcase_add_test(tc, clientServerTlsCaCertificate);
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s); srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL); int failed = srunner_ntests_failed(sr);
    srunner_free(sr); return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
