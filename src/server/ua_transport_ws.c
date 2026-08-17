/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"
#include "mp_printf.h"

static UA_Boolean
isWebSocketUrl(const UA_String *url) {
    const UA_String wss = UA_STRING_STATIC("opc.wss://");
    const UA_String ws  = UA_STRING_STATIC("opc.ws://");
    return (url->length >= wss.length &&
            memcmp(url->data, wss.data, wss.length) == 0) ||
           (url->length >= ws.length &&
            memcmp(url->data, ws.data, ws.length) == 0);
}

static UA_Boolean
isSecureWebSocketUrl(const UA_String *url) {
    const UA_String wss = UA_STRING_STATIC("opc.wss://");
    return url->length >= wss.length &&
           memcmp(url->data, wss.data, wss.length) == 0;
}

static void
addWebSocketDiscoveryUrls(UA_BinaryProtocolManager *bpm,
                          const UA_KeyValueMap *params) {
    const UA_UInt16 *listenPort = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "listen-port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    const UA_String *listenAddress = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "listen-address"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!listenPort || !listenAddress)
        return;

    UA_Server *server = bpm->drv.server;
    for(size_t i = 0; i < server->config.serverUrlsSize; i++) {
        const UA_String *serverUrl = &server->config.serverUrls[i];
        if(!isWebSocketUrl(serverUrl))
            continue;

        const UA_Boolean secure = isSecureWebSocketUrl(serverUrl);
        UA_String hostname = UA_STRING_NULL;
        UA_String path = UA_STRING_NULL;
        UA_UInt16 port = secure ? 443 : 4840;
        UA_StatusCode res =
            UA_parseEndpointUrl(serverUrl, &hostname, &port, &path);
        if(res != UA_STATUSCODE_GOOD ||
           (port != 0 && port != *listenPort))
            continue;

        const char *scheme = secure ? "opc.wss" : "opc.ws";
        const UA_String advertisedHost =
            hostname.length > 0 ? hostname : *listenAddress;
        char urlstr[1024];
        if(path.length > 0) {
            mp_snprintf(urlstr, sizeof(urlstr), "%s://%S:%u/%S",
                        scheme, advertisedHost, (unsigned)*listenPort, path);
        } else {
            mp_snprintf(urlstr, sizeof(urlstr), "%s://%S:%u",
                        scheme, advertisedHost, (unsigned)*listenPort);
        }
        UA_String url = UA_STRING(urlstr);
        addServerDiscoveryUrl(server, &url);
    }
}

static UA_StatusCode
createWebSocketServerConnection(UA_BinaryProtocolManager *bpm,
                                const UA_String *serverUrl) {
    if(!isWebSocketUrl(serverUrl))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_LOCK_ASSERT(&bpm->drv.server->serviceMutex);

    UA_Boolean secure = isSecureWebSocketUrl(serverUrl);
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_UInt16 port = secure ? 443 : 4840;
    UA_StatusCode res =
        UA_parseEndpointUrl(serverUrl, &hostname, &port, &path);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(path.length == SIZE_MAX)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_String websocketPath = UA_STRING_NULL;
    res = UA_ByteString_allocBuffer((UA_ByteString*)&websocketPath,
                                    path.length + 1);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    websocketPath.data[0] = '/';
    if(path.length > 0)
        memcpy(&websocketPath.data[1], path.data, path.length);

    UA_String websocket = UA_STRING("websocket");
    UA_Server *server = bpm->drv.server;
    UA_ServerConfig *config = &server->config;
    UA_StatusCode openResult = UA_STATUSCODE_BADINTERNALERROR;
    UA_Boolean foundWebSocketManager = false;
    for(UA_EventSource *es = config->eventLoop->eventSources;
        es != NULL; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(!UA_String_equal(&websocket, &cm->protocol))
            continue;
        foundWebSocketManager = true;

        UA_KeyValuePair params[13];
        size_t paramsSize = 0;

        params[paramsSize].key = UA_QUALIFIEDNAME(0, "port");
        UA_Variant_setScalar(&params[paramsSize++].value, &port,
                             &UA_TYPES[UA_TYPES_UINT16]);

        UA_Boolean listen = true;
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "listen");
        UA_Variant_setScalar(&params[paramsSize++].value, &listen,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);

        params[paramsSize].key = UA_QUALIFIEDNAME(0, "useSSL");
        UA_Variant_setScalar(&params[paramsSize++].value, &secure,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);

        params[paramsSize].key = UA_QUALIFIEDNAME(0, "path");
        UA_Variant_setScalar(&params[paramsSize++].value, &websocketPath,
                             &UA_TYPES[UA_TYPES_STRING]);

        UA_String subprotocol = UA_STRING("opcua+uacp");
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "subprotocol");
        UA_Variant_setScalar(&params[paramsSize++].value, &subprotocol,
                             &UA_TYPES[UA_TYPES_STRING]);

        UA_Boolean binaryOnly = true;
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "binary-only");
        UA_Variant_setScalar(&params[paramsSize++].value, &binaryOnly,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);

        params[paramsSize].key = UA_QUALIFIEDNAME(0, "recv-max-message-size");
        UA_Variant_setScalar(&params[paramsSize++].value,
                             &config->webSocketBufSize,
                             &UA_TYPES[UA_TYPES_UINT32]);
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "send-max-message-size");
        UA_Variant_setScalar(&params[paramsSize++].value,
                             &config->webSocketBufSize,
                             &UA_TYPES[UA_TYPES_UINT32]);
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "send-max-queue-size");
        UA_Variant_setScalar(&params[paramsSize++].value,
                             &config->webSocketMaxQueueSize,
                             &UA_TYPES[UA_TYPES_UINT32]);

        if(hostname.length > 0) {
            params[paramsSize].key = UA_QUALIFIEDNAME(0, "address");
            UA_Variant_setScalar(&params[paramsSize++].value, &hostname,
                                 &UA_TYPES[UA_TYPES_STRING]);
        }

        /* TLS credentials — only required for opc.wss:// */
        if(secure) {
            /* TLS credentials are configured on the WebSocket ConnectionManager.
             * They are separate from the keys used by the UACP SecurityPolicies.
             * Do not forward empty values. The TLS listener requires a non-empty
             * certificate/private-key pair. */
            UA_ByteString *certificate = &config->webSocketCertificate;
            UA_ByteString *privateKey = &config->webSocketPrivateKey;
            if(certificate->length == 0 || privateKey->length == 0) {
                UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                             "The WebSocket server (opc.wss://) requires a "
                             "non-empty TLS certificate and private key");
                openResult = UA_STATUSCODE_BADINVALIDARGUMENT;
                continue;
            }
            params[paramsSize].key = UA_QUALIFIEDNAME(0, "certificate");
            UA_Variant_setScalar(&params[paramsSize++].value, certificate,
                                 &UA_TYPES[UA_TYPES_BYTESTRING]);
            params[paramsSize].key = UA_QUALIFIEDNAME(0, "private-key");
            UA_Variant_setScalar(&params[paramsSize++].value, privateKey,
                                 &UA_TYPES[UA_TYPES_BYTESTRING]);

            if(config->webSocketPrivateKeyPassword.length > 0) {
                params[paramsSize].key =
                    UA_QUALIFIEDNAME(0, "private-key-password");
                UA_Variant_setScalar(&params[paramsSize++].value,
                                     &config->webSocketPrivateKeyPassword,
                                     &UA_TYPES[UA_TYPES_STRING]);
            }
        }

        UA_KeyValueMap paramsMap = {paramsSize, params};
        openResult = cm->openConnection(cm, &paramsMap, bpm, NULL,
                                        serverNetworkCallback);
        if(openResult == UA_STATUSCODE_GOOD) {
            UA_String_clear(&websocketPath);
            return openResult;
        }
    }

    UA_String_clear(&websocketPath);
    if(!foundWebSocketManager)
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "No WebSocket ConnectionManager is configured in the EventLoop");
    return openResult;
}

static UA_StatusCode
startWebSocketTransport(UA_BinaryProtocolManager *bpm) {
    UA_Server *server = bpm->drv.server;
    UA_ServerConfig *config = &server->config;
    UA_Boolean haveWebSocketUrl = false;
    UA_Boolean haveServerSocket = false;

    if(!config->webSocketEnabled)
        return UA_STATUSCODE_GOOD;

    /* Fail early if any opc.wss:// URL is configured without TLS credentials */
    for(size_t i = 0; i < config->serverUrlsSize; i++) {
        if(!isSecureWebSocketUrl(&config->serverUrls[i]))
            continue;
        if(config->webSocketCertificate.length == 0 ||
           config->webSocketPrivateKey.length == 0) {
            UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                         "WebSocket transport is enabled but its TLS certificate "
                         "or private key is empty");
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }
        break;
    }

    UA_BinaryConnectionConfig_set(&bpm->connectionConfig,
                                  config->webSocketBufSize,
                                  config->webSocketMaxMsgSize,
                                  config->webSocketMaxChunks);

    for(size_t i = 0; i < config->serverUrlsSize; i++) {
        const UA_String *serverUrl = &config->serverUrls[i];
        if(!isWebSocketUrl(serverUrl))
            continue;
        haveWebSocketUrl = true;
        UA_StatusCode res = createWebSocketServerConnection(bpm, serverUrl);
        if(res != UA_STATUSCODE_GOOD)
            continue;
        haveServerSocket = true;

        /* The callback adds URLs for wildcard and dynamically assigned
         * listeners. Preserve configured URLs with explicit hostnames. */
        UA_String hostname = UA_STRING_NULL;
        UA_UInt16 port = 443;
        if(UA_parseEndpointUrl(serverUrl, &hostname, &port, NULL) ==
               UA_STATUSCODE_GOOD &&
           hostname.length > 0)
            addServerDiscoveryUrl(server, serverUrl);
    }

    if(!haveWebSocketUrl) {
        UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                     "WebSocket transport is enabled but no opc.ws:// or "
                     "opc.wss:// ServerUrl is configured");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }
    if(haveServerSocket)
        return UA_STATUSCODE_GOOD;

    UA_LOG_ERROR(config->logging, UA_LOGCATEGORY_SERVER,
                 "The server has no WebSocket server socket");
    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_Driver *
UA_WebSocketProtocolManager_new(void) {
    UA_BinaryProtocolManager *bpm = (UA_BinaryProtocolManager*)
        UA_calloc(1, sizeof(UA_BinaryProtocolManager));
    if(!bpm)
        return NULL;

    UA_BinaryProtocolManager_init(bpm, UA_STRING("binary-ws"),
                                  UA_STRING("WebSocket"),
                                  startWebSocketTransport,
                                  addWebSocketDiscoveryUrls);
    return &bpm->drv;
}
