/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include "eventloop_posix_http_internal.h"
#include "eventloop_posix_http_compression.h"
#include "eventloop_posix_http_parser.h"

#include <stdio.h>

typedef enum {
    HTTP_ACCEPTED_CONNECTION_IDLE,
    HTTP_ACCEPTED_CONNECTION_RECEIVING,
    HTTP_ACCEPTED_CONNECTION_REQUEST_ACTIVE,
    HTTP_ACCEPTED_CONNECTION_RESPONDING,
    HTTP_ACCEPTED_CONNECTION_CLOSING
} HTTPAcceptedConnectionState;

struct HTTPAcceptedConnection {
    LIST_ENTRY(HTTPAcceptedConnection) next;
    HTTPListenConnection *listener;
    struct lws *wsi;
    uintptr_t connectionId;
    UA_HTTPParser parser;
    UA_UInt64 timeoutId;
    HTTPAcceptedConnectionState state;
    void *context;
    UA_String remoteAddress;

    /* Inline state for the current HTTP/1.1 request and response. */
    UA_String method;
    UA_String path;
    UA_Byte requestRandom[32];
    UA_KeyValuePair *headers;
    size_t headersSize;
    UA_ByteString requestBody;
    HTTPPayload response;
    UA_Boolean closeAfterResponse;
    UA_Boolean identityResponseAllowed;
    UA_Boolean gzipResponseAllowed;
    UA_UInt16 rejectionStatus;
    UA_HTTPContentEncoding requestEncoding;
    UA_HTTPContentEncoding responseEncoding;
};

static void requestAcceptedConnectionClose(HTTPAcceptedConnection *connection,
                                           const char *reason);

static const char *
connectionStateName(HTTPAcceptedConnectionState state) {
    switch(state) {
    case HTTP_ACCEPTED_CONNECTION_IDLE: return "idle";
    case HTTP_ACCEPTED_CONNECTION_RECEIVING: return "receiving";
    case HTTP_ACCEPTED_CONNECTION_REQUEST_ACTIVE: return "request-active";
    case HTTP_ACCEPTED_CONNECTION_RESPONDING: return "responding";
    case HTTP_ACCEPTED_CONNECTION_CLOSING: return "closing";
    default: return "invalid";
    }
}

static UA_Boolean
connectionTransitionAllowed(HTTPAcceptedConnectionState from,
                            HTTPAcceptedConnectionState to) {
    if(from == to)
        return true;
    if(to == HTTP_ACCEPTED_CONNECTION_CLOSING)
        return true;
    switch(from) {
    case HTTP_ACCEPTED_CONNECTION_IDLE:
        return to == HTTP_ACCEPTED_CONNECTION_RECEIVING;
    case HTTP_ACCEPTED_CONNECTION_RECEIVING:
        return to == HTTP_ACCEPTED_CONNECTION_REQUEST_ACTIVE;
    case HTTP_ACCEPTED_CONNECTION_REQUEST_ACTIVE:
        return to == HTTP_ACCEPTED_CONNECTION_RESPONDING;
    case HTTP_ACCEPTED_CONNECTION_RESPONDING:
        return to == HTTP_ACCEPTED_CONNECTION_IDLE;
    default:
        return false;
    }
}

static void
setConnectionState(HTTPAcceptedConnection *connection,
                   HTTPAcceptedConnectionState state, const char *reason) {
    if(!connection || connection->state == state)
        return;
    UA_assert(connectionTransitionAllowed(connection->state, state));
    UA_LOG_DEBUG(connection->listener->manager->cm.eventSource.eventLoop->logger,
                 UA_LOGCATEGORY_NETWORK,
                 "HTTP accepted connection %" PRIuPTR ": %s -> %s (%s)",
                 connection->connectionId,
                 connectionStateName(connection->state),
                 connectionStateName(state), reason);
    connection->state = state;
}

static void
clearActiveRequest(HTTPAcceptedConnection *connection) {
    UA_String_clear(&connection->method);
    UA_String_clear(&connection->path);
    UA_HTTP_clearHeaders(&connection->headers, &connection->headersSize);
    UA_ByteString_clear(&connection->requestBody);
    UA_ByteString_clear(&connection->response.data);
    connection->response.offset = 0;
    connection->closeAfterResponse = false;
    connection->identityResponseAllowed = true;
    connection->gzipResponseAllowed = false;
    connection->rejectionStatus = 0;
    connection->requestEncoding = UA_HTTP_CONTENT_ENCODING_IDENTITY;
    connection->responseEncoding = UA_HTTP_CONTENT_ENCODING_IDENTITY;
}

HTTPAcceptedConnection *
UA_HTTP_findAcceptedConnection(HTTPConnectionManager *manager,
                               uintptr_t connectionId) {
    HTTPAcceptedConnection *connection;
    LIST_FOREACH(connection, &manager->acceptedConnections, next) {
        if(connection->connectionId == connectionId)
            return connection;
    }
    return NULL;
}

void
UA_HTTP_closeAcceptedConnection(HTTPAcceptedConnection *connection) {
    requestAcceptedConnectionClose(connection, "application close");
}

static void
acceptedConnectionTimeout(void *application, void *data) {
    (void)application;
    HTTPAcceptedConnection *connection = (HTTPAcceptedConnection*)data;
    connection->timeoutId = 0;
    if(!connection->wsi)
        return;
    requestAcceptedConnectionClose(connection, "inactivity timeout");
}

static void
removeAcceptedConnectionTimeout(HTTPAcceptedConnection *connection) {
    if(!connection || !connection->timeoutId)
        return;
    UA_EventLoop *el = connection->listener->manager->cm.eventSource.eventLoop;
    el->removeTimer(el, connection->timeoutId);
    connection->timeoutId = 0;
}

static UA_StatusCode
armAcceptedConnectionTimeout(HTTPAcceptedConnection *connection) {
    if(!connection ||
       connection->state >= HTTP_ACCEPTED_CONNECTION_CLOSING)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_EventLoop *el = connection->listener->manager->cm.eventSource.eventLoop;
    UA_Double interval = (UA_Double)connection->listener->timeout * 1000.0;
    if(connection->timeoutId)
        return el->modifyTimer(el, connection->timeoutId, interval, NULL,
                               UA_TIMERPOLICY_ONCE);
    return el->addTimer(el, acceptedConnectionTimeout, NULL, connection,
                        interval, NULL, UA_TIMERPOLICY_ONCE,
                        &connection->timeoutId);
}

static void
requestAcceptedConnectionClose(HTTPAcceptedConnection *connection,
                               const char *reason) {
    if(!connection ||
       connection->state >= HTTP_ACCEPTED_CONNECTION_CLOSING)
        return;
    removeAcceptedConnectionTimeout(connection);
    setConnectionState(connection, HTTP_ACCEPTED_CONNECTION_CLOSING, reason);
    UA_HTTP_closeWsi(connection->wsi);
}

static void
deliverServerRequest(HTTPAcceptedConnection *connection) {
    UA_KeyValuePair params[5] = {0};
    params[0].key = UA_QUALIFIEDNAME(0, "method");
    UA_Variant_setScalar(&params[0].value, &connection->method,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[1].key = UA_QUALIFIEDNAME(0, "path");
    UA_Variant_setScalar(&params[1].value, &connection->path,
                         &UA_TYPES[UA_TYPES_STRING]);
    params[2].key = UA_QUALIFIEDNAME(0, "headers");
    UA_Variant_setArray(&params[2].value, connection->headers,
                        connection->headersSize,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    params[3].key = UA_QUALIFIEDNAME(0, "remote-address");
    UA_Variant_setScalar(&params[3].value, &connection->remoteAddress,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_ByteString requestRandom = {sizeof(connection->requestRandom),
                                   connection->requestRandom};
    params[4].key = UA_QUALIFIEDNAME(0, "request-random");
    UA_Variant_setScalar(&params[4].value, &requestRandom,
                         &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_KeyValueMap map = {5, params};
    HTTPListenConnection *listener = connection->listener;
    listener->callback(&listener->manager->cm, connection->connectionId,
                       listener->application, &connection->context,
                       UA_CONNECTIONSTATE_ESTABLISHED, &map,
                       connection->requestBody);

    UA_ByteString_clear(&connection->requestBody);
    UA_HTTP_clearHeaders(&connection->headers, &connection->headersSize);
}

static UA_StatusCode
appendRequestBody(HTTPAcceptedConnection *connection,
                  const void *data, size_t length) {
    size_t current = connection->requestBody.length;
    if(length > SIZE_MAX - current)
        return UA_STATUSCODE_BADREQUESTTOOLARGE;
    size_t required = current + length;
    size_t limit = connection->listener->recvMaxMessageSize;
    if(limit > 0 && required > limit)
        return UA_STATUSCODE_BADREQUESTTOOLARGE;
    UA_String fragment = {length, (UA_Byte *)(uintptr_t)data};
    return UA_String_append(&connection->requestBody, fragment);
}

static UA_StatusCode
beginServerRequest(HTTPAcceptedConnection *connection,
                   const char *path, size_t pathLength,
                   const char *method, size_t methodLength) {
    clearActiveRequest(connection);
    if(lws_get_random(lws_get_context(connection->wsi),
                      connection->requestRandom,
                      sizeof(connection->requestRandom)) !=
       sizeof(connection->requestRandom))
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_String methodString = {methodLength, (UA_Byte*)(uintptr_t)method};
    UA_String pathString = {pathLength, (UA_Byte*)(uintptr_t)path};
    UA_StatusCode res = UA_String_copy(&methodString, &connection->method);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_String_copy(&pathString, &connection->path);
    if(res != UA_STATUSCODE_GOOD)
        clearActiveRequest(connection);
    return res;
}

static const UA_String *
getRequestHeader(const HTTPAcceptedConnection *connection, const char *name,
                 UA_Boolean *duplicate) {
    UA_String expected = UA_STRING((char*)(uintptr_t)name);
    const UA_String *value = NULL;
    *duplicate = false;
    for(size_t i = 0; i < connection->headersSize; i++) {
        if(!UA_String_equal(&connection->headers[i].key.name, &expected))
            continue;
        if(value) {
            *duplicate = true;
            return NULL;
        }
        value = (const UA_String*)connection->headers[i].value.data;
    }
    return value;
}

static UA_StatusCode
combineRequestHeader(const HTTPAcceptedConnection *connection, const char *name,
                     UA_String *combined) {
    UA_String expected = UA_STRING((char*)(uintptr_t)name);
    size_t length = 0;
    size_t matches = 0;
    for(size_t i = 0; i < connection->headersSize; i++) {
        if(!UA_String_equal(&connection->headers[i].key.name, &expected))
            continue;
        const UA_String *value =
            (const UA_String*)connection->headers[i].value.data;
        size_t separator = matches > 0 ? 1 : 0;
        if(separator > SIZE_MAX - length ||
           value->length > SIZE_MAX - length - separator)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        length += separator + value->length;
        matches++;
    }
    if(matches == 0)
        return UA_STATUSCODE_GOOD;
    UA_StatusCode res = UA_ByteString_allocBuffer(
        (UA_ByteString*)combined, length);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    size_t offset = 0;
    matches = 0;
    for(size_t i = 0; i < connection->headersSize; i++) {
        if(!UA_String_equal(&connection->headers[i].key.name, &expected))
            continue;
        const UA_String *value =
            (const UA_String*)connection->headers[i].value.data;
        if(matches++ > 0)
            combined->data[offset++] = ',';
        memcpy(&combined->data[offset], value->data, value->length);
        offset += value->length;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
responseHasContentEncoding(const UA_KeyValueMap *params) {
    const UA_Variant *headers = UA_KeyValueMap_get(
        params, UA_QUALIFIEDNAME(0, "headers"));
    if(!headers)
        return false;
    const UA_KeyValuePair *array = (const UA_KeyValuePair*)headers->data;
    for(size_t i = 0; i < headers->arrayLength; i++) {
        if(UA_HTTP_stringEqualIgnoreCase(&array[i].key.name,
                                         "content-encoding"))
            return true;
    }
    return false;
}

static UA_StatusCode
parserHeaders(void *context, const char *method, size_t methodLength,
              const char *path, size_t pathLength,
              const struct phr_header *headers, size_t headersSize,
              UA_Boolean closeAfterResponse) {
    HTTPAcceptedConnection *connection = (HTTPAcceptedConnection*)context;
    UA_StatusCode res = beginServerRequest(connection, path, pathLength,
                                           method, methodLength);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    connection->closeAfterResponse = closeAfterResponse;
    for(size_t i = 0; i < headersSize; i++) {
        res = UA_HTTP_appendHeader(
            &connection->headers, &connection->headersSize, headers[i].name,
            headers[i].name_len, headers[i].value, headers[i].value_len);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    UA_Boolean duplicate = false;
    const UA_String *contentEncoding = getRequestHeader(
        connection, "content-encoding", &duplicate);
    if(duplicate)
        connection->rejectionStatus = 400;
    else {
        connection->requestEncoding =
            UA_HTTP_parseContentEncoding(contentEncoding);
        if(connection->requestEncoding ==
           UA_HTTP_CONTENT_ENCODING_UNSUPPORTED)
            connection->rejectionStatus = 415;
    }

    UA_String acceptEncoding = UA_STRING_NULL;
    res = combineRequestHeader(connection, "accept-encoding", &acceptEncoding);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_HTTPCompressionPreference preference =
        UA_HTTP_selectResponseEncoding(
            acceptEncoding.data ? &acceptEncoding : NULL);
    UA_String_clear(&acceptEncoding);
    connection->responseEncoding = preference.encoding;
    connection->identityResponseAllowed = preference.identityAllowed;
    connection->gzipResponseAllowed = preference.gzipAllowed;
    if(!preference.acceptable)
        connection->rejectionStatus = 406;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
parserBody(void *context, const UA_Byte *data, size_t length) {
    return appendRequestBody((HTTPAcceptedConnection*)context, data, length);
}

static void
sendAutomaticResponse(HTTPAcceptedConnection *connection, UA_UInt16 status) {
    UA_KeyValuePair parameter = {0};
    parameter.key = UA_QUALIFIEDNAME(0, "status-code");
    UA_Variant_setScalar(&parameter.value, &status,
                         &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap params = {1, &parameter};
    UA_ByteString empty = UA_BYTESTRING_NULL;
    if(UA_HTTP_sendServerResponse(connection, &params, &empty) !=
       UA_STATUSCODE_GOOD)
        requestAcceptedConnectionClose(connection, "automatic response failed");
}

static void
parserComplete(void *context) {
    HTTPAcceptedConnection *connection = (HTTPAcceptedConnection*)context;
    setConnectionState(connection, HTTP_ACCEPTED_CONNECTION_REQUEST_ACTIVE,
                       "request complete");
    if(connection->rejectionStatus) {
        sendAutomaticResponse(connection, connection->rejectionStatus);
        return;
    }
    if(connection->requestEncoding != UA_HTTP_CONTENT_ENCODING_IDENTITY) {
        UA_ByteString decompressed = UA_BYTESTRING_NULL;
        size_t limit = connection->listener->recvMaxDecompressedMessageSize;
        if(limit == 0)
            limit = HTTP_DEFAULT_DECOMPRESSED_LIMIT;
        UA_StatusCode res = UA_HTTP_decompress(
            connection->requestEncoding, &connection->requestBody, limit,
            &decompressed);
        if(res != UA_STATUSCODE_GOOD) {
            sendAutomaticResponse(connection,
                res == UA_STATUSCODE_BADREQUESTTOOLARGE ? 413 : 400);
            return;
        }
        UA_ByteString_clear(&connection->requestBody);
        connection->requestBody = decompressed;
    }
    deliverServerRequest(connection);
}

static const UA_HTTPParserCallbacks parserCallbacks = {
    NULL, parserHeaders, parserBody, parserComplete
};

static int
completeServerResponse(HTTPAcceptedConnection *connection) {
    UA_Boolean closeConnection = connection->closeAfterResponse;
    clearActiveRequest(connection);
    UA_HTTPParser_reset(&connection->parser);
    if(closeConnection) {
        /* Returning -1 here can discard TLS data buffered by LWS. Close only
         * after the final response bytes have reached the transport. */
        lws_set_timeout(connection->wsi,
                        PENDING_FLUSH_STORED_SEND_BEFORE_CLOSE,
                        1);
        setConnectionState(connection, HTTP_ACCEPTED_CONNECTION_CLOSING,
                           "response requested close");
        return 0;
    }
    setConnectionState(connection, HTTP_ACCEPTED_CONNECTION_IDLE,
                       "response complete");
    return armAcceptedConnectionTimeout(connection) == UA_STATUSCODE_GOOD ?
        0 : -1;
}

static int
writeServerResponse(HTTPAcceptedConnection *connection) {
    if(connection->state != HTTP_ACCEPTED_CONNECTION_RESPONDING)
        return 0;

    UA_LOG_DEBUG(connection->listener->manager->cm.eventSource.eventLoop->logger,
                 UA_LOGCATEGORY_NETWORK,
                 "HTTP server response write: %zu/%zu",
                 connection->response.offset,
                 UA_HTTP_payloadLength(&connection->response));

    /* The response already contains the complete HTTP/1.1 framing. The raw
     * socket protocol does not need an additional libwebsockets final marker. */
    int result = UA_HTTP_writePayload(connection->wsi,
                                      &connection->response);
    if(result < 0)
        requestAcceptedConnectionClose(connection, "response write failed");
    return result <= 0 ? result : completeServerResponse(connection);
}

static int
receiveAcceptedConnectionData(HTTPAcceptedConnection *connection,
                              void *data, size_t length) {
    if(!connection)
        return -1;
    if(connection->state == HTTP_ACCEPTED_CONNECTION_IDLE)
        setConnectionState(connection, HTTP_ACCEPTED_CONNECTION_RECEIVING,
                           "request bytes received");
    if(connection->state != HTTP_ACCEPTED_CONNECTION_RECEIVING) {
        UA_LOG_DEBUG(connection->listener->manager->cm.eventSource.eventLoop->logger,
                     UA_LOGCATEGORY_NETWORK,
                     "HTTP server rejected an overlapping request on "
                     "connection %" PRIuPTR, connection->connectionId);
        requestAcceptedConnectionClose(connection,
                                       "overlapping request received");
        return -1;
    }
    if(armAcceptedConnectionTimeout(connection) != UA_STATUSCODE_GOOD)
        return -1;

    UA_HTTPParserCallbacks callbacks = parserCallbacks;
    callbacks.context = connection;
    UA_StatusCode res = UA_HTTPParser_process(
        &connection->parser, data, length,
        connection->listener->recvMaxMessageSize,
        &callbacks);
    if(res == UA_STATUSCODE_GOOD)
        return 0;
    UA_LOG_DEBUG(connection->listener->manager->cm.eventSource.eventLoop->logger,
                 UA_LOGCATEGORY_NETWORK,
                 "HTTP server rejected request: %s (%s)",
                 UA_HTTPParser_diagnosticName(connection->parser.diagnostic),
                 UA_StatusCode_name(res));
    clearActiveRequest(connection);
    UA_HTTPParser_reset(&connection->parser);
    requestAcceptedConnectionClose(connection, "request parse failed");
    return -1;
}

static HTTPAcceptedConnection *
registerAcceptedConnection(HTTPListenConnection *listener,
                           struct lws *wsi) {
    HTTPAcceptedConnection *connection =
        (HTTPAcceptedConnection*)UA_calloc(1, sizeof(*connection));
    if(!connection)
        return NULL;
    connection->listener = listener;
    connection->connectionId = UA_HTTP_nextConnectionId(listener->manager);
    if(connection->connectionId == 0) {
        UA_free(connection);
        return NULL;
    }
    connection->wsi = wsi;
    connection->context = listener->context;
    clearActiveRequest(connection);

    char peer[128] = {0};
    lws_get_peer_simple(wsi, peer, sizeof(peer));
    UA_String peerString = UA_STRING(peer);
    if(UA_String_copy(&peerString, &connection->remoteAddress) !=
       UA_STATUSCODE_GOOD) {
        UA_free(connection);
        return NULL;
    }

    LIST_INSERT_HEAD(&listener->manager->acceptedConnections,
                     connection, next);
    lws_set_opaque_user_data(wsi, connection);
    UA_HTTPParser_reset(&connection->parser);
    UA_KeyValuePair parameter = {0};
    parameter.key = UA_QUALIFIEDNAME(0, "remote-address");
    UA_Variant_setScalar(&parameter.value, &connection->remoteAddress,
                         &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap params = {1, &parameter};
    listener->callback(&listener->manager->cm, connection->connectionId,
                       listener->application, &connection->context,
                       UA_CONNECTIONSTATE_ESTABLISHED, &params,
                       UA_BYTESTRING_NULL);
    if(connection->state < HTTP_ACCEPTED_CONNECTION_CLOSING &&
       armAcceptedConnectionTimeout(connection) != UA_STATUSCODE_GOOD)
        requestAcceptedConnectionClose(connection,
                                       "connection registration failed");
    return connection;
}

static void
unregisterAcceptedConnection(HTTPAcceptedConnection *connection) {
    if(!connection)
        return;
    HTTPListenConnection *listener = connection->listener;
    HTTPConnectionManager *manager = listener->manager;
    removeAcceptedConnectionTimeout(connection);
    if(connection->state < HTTP_ACCEPTED_CONNECTION_CLOSING)
        setConnectionState(connection, HTTP_ACCEPTED_CONNECTION_CLOSING,
                           "connection released");
    clearActiveRequest(connection);
    UA_HTTPParser_clear(&connection->parser);
    listener->callback(&manager->cm, connection->connectionId,
                       listener->application, &connection->context,
                       UA_CONNECTIONSTATE_CLOSING,
                       &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    LIST_REMOVE(connection, next);
    connection->wsi = NULL;
    UA_String_clear(&connection->remoteAddress);
    UA_free(connection);
    UA_HTTP_updateStoppedState(manager);
}

void
UA_HTTP_closeAcceptedConnections(HTTPListenConnection *listener) {
    HTTPConnectionManager *manager = listener->manager;
    HTTPAcceptedConnection *connection, *connectionNext;
    LIST_FOREACH_SAFE(connection, &manager->acceptedConnections, next,
                      connectionNext) {
        if(connection->listener != listener)
            continue;
        requestAcceptedConnectionClose(connection, "listener shutdown");
        lws_set_opaque_user_data(connection->wsi, NULL);
        unregisterAcceptedConnection(connection);
    }
}

static int
callbackHttpServer(struct lws *wsi, enum lws_callback_reasons reason,
                   void *user, void *in, size_t len) {
    (void)user;
    HTTPAcceptedConnection *connection =
        (HTTPAcceptedConnection*)lws_get_opaque_user_data(wsi);
    HTTPListenConnection *listener = NULL;

    if(reason == LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL) {
        listener = (HTTPListenConnection*)
            lws_get_vhost_user(lws_get_vhost(wsi));
        /* The protocol list is shared with outgoing HTTP connections. Do not
         * take over their opaque user data. */
        if(!listener || !listener->listener)
            return 0;
    }
    if(reason == LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL && !connection) {
        connection = registerAcceptedConnection(listener, wsi);
        if(!connection)
            return -1;
    }

    switch(reason) {
    case LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL:
        return 0;
    case LWS_CALLBACK_RAW_ADOPT:
        return 0;
    case LWS_CALLBACK_RAW_RX:
        return receiveAcceptedConnectionData(connection, in, len);
    case LWS_CALLBACK_RAW_WRITEABLE:
        if(!connection)
            return 0;
        return writeServerResponse(connection);
    case LWS_CALLBACK_RAW_CLOSE:
    case LWS_CALLBACK_RAW_SKT_DROP_PROTOCOL:
        /* libwebsockets may report both callbacks. Clearing the opaque pointer
         * before freeing makes teardown idempotent. */
        lws_set_opaque_user_data(wsi, NULL);
        unregisterAcceptedConnection(connection);
        return 0;
    default:
        return 0;
    }
}

const struct lws_protocols UA_HTTP_serverProtocols[] = {
    {"http-raw", callbackHttpServer, 0, 0, 0, NULL, 0},
    LWS_PROTOCOL_LIST_TERM
};

static const char *
statusReason(UA_UInt16 status) {
    switch(status) {
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 406: return "Not Acceptable";
    case 413: return "Content Too Large";
    case 415: return "Unsupported Media Type";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 503: return "Service Unavailable";
    default: return "";
    }
}

static UA_StatusCode
buildResponse(HTTPAcceptedConnection *connection,
              const UA_KeyValueMap *params, const UA_ByteString *body) {
    size_t capacity = UA_HTTP_responseHeaderBufferSize(params);
    size_t bodyLength = body->length;
    if(capacity == 0 || capacity > SIZE_MAX - LWS_PRE ||
       bodyLength > SIZE_MAX - LWS_PRE - capacity)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    UA_ByteString output = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_allocBuffer(
        &output, LWS_PRE + capacity + bodyLength);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_Byte *header = output.data + LWS_PRE;

    const UA_UInt16 *statusCode = (const UA_UInt16*)GET_PARAM(
        params, "status-code", UA_TYPES_UINT16);
    UA_UInt16 status = statusCode ? *statusCode : HTTP_STATUS_OK;
    int written = snprintf((char*)header, capacity,
                           "HTTP/1.1 %u %s\r\n", (unsigned)status,
                           statusReason(status));
    if(written < 0 || (size_t)written >= capacity) {
        UA_ByteString_clear(&output);
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    }
    size_t offset = (size_t)written;

    const UA_Variant *headers = UA_KeyValueMap_get(
        params, UA_QUALIFIEDNAME(0, "headers"));
    size_t headersSize = headers ? headers->arrayLength : 0;
    const UA_KeyValuePair *array = headers ?
        (const UA_KeyValuePair*)headers->data : NULL;
    for(size_t i = 0; i < headersSize; i++) {
        const UA_String *name = &array[i].key.name;
        const UA_String *value = (const UA_String*)array[i].value.data;
        if(name->length > capacity - offset ||
           value->length > capacity - offset - name->length ||
           capacity - offset - name->length - value->length < 4) {
            UA_ByteString_clear(&output);
            return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
        }
        memcpy(&header[offset], name->data, name->length);
        offset += name->length;
        memcpy(&header[offset], ": ", 2);
        offset += 2;
        memcpy(&header[offset], value->data, value->length);
        offset += value->length;
        memcpy(&header[offset], "\r\n", 2);
        offset += 2;
    }

    const char *encoding = UA_HTTP_contentEncodingName(
        connection->responseEncoding);
    const char *encodingHeader =
        connection->responseEncoding == UA_HTTP_CONTENT_ENCODING_IDENTITY ? "" :
        "Content-Encoding: ";
    const char *encodingEnd = encodingHeader[0] ? "\r\n" : "";
    written = snprintf((char*)&header[offset], capacity - offset,
                       "%s%s%sContent-Length: %zu\r\nConnection: %s\r\n\r\n",
                       encodingHeader, encodingHeader[0] ? encoding : "",
                       encodingEnd, bodyLength,
                       connection->closeAfterResponse ? "close" : "keep-alive");
    if(written < 0 || (size_t)written >= capacity - offset) {
        UA_ByteString_clear(&output);
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    }
    size_t headerLength = offset + (size_t)written;
    if(bodyLength > 0)
        memcpy(&header[headerLength], body->data, bodyLength);
    output.length = LWS_PRE + headerLength + bodyLength;
    connection->response.data = output;
    connection->response.offset = 0;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_HTTP_sendServerResponse(HTTPAcceptedConnection *connection,
                           const UA_KeyValueMap *params, UA_ByteString *buffer) {
    if(connection->state != HTTP_ACCEPTED_CONNECTION_REQUEST_ACTIVE)
        return UA_STATUSCODE_BADINVALIDSTATE;
    if(responseHasContentEncoding(params))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_String *codingPolicy = (const UA_String *)GET_PARAM(
        params, "content-coding-policy", UA_TYPES_STRING);
    if(codingPolicy) {
        const UA_String identity = UA_STRING_STATIC("identity");
        const UA_String gzip = UA_STRING_STATIC("gzip");
        if(UA_String_equal(codingPolicy, &identity)) {
            if(!connection->identityResponseAllowed)
                return UA_STATUSCODE_BADNOTSUPPORTED;
            connection->responseEncoding = UA_HTTP_CONTENT_ENCODING_IDENTITY;
        } else if(UA_String_equal(codingPolicy, &gzip)) {
            if(connection->responseEncoding == UA_HTTP_CONTENT_ENCODING_DEFLATE) {
                if(connection->gzipResponseAllowed)
                    connection->responseEncoding = UA_HTTP_CONTENT_ENCODING_GZIP;
                else if(connection->identityResponseAllowed)
                    connection->responseEncoding = UA_HTTP_CONTENT_ENCODING_IDENTITY;
                else
                    return UA_STATUSCODE_BADNOTSUPPORTED;
            }
        } else {
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    }
    size_t bodyLength = buffer ? buffer->length : 0;
    /* An empty error response has no representation to encode. In particular,
     * this allows a 406 response when no requested representation is usable. */
    if(bodyLength == 0)
        connection->responseEncoding = UA_HTTP_CONTENT_ENCODING_IDENTITY;
    if(connection->listener->sendMaxMessageSize > 0 &&
       bodyLength > connection->listener->sendMaxMessageSize)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;
    UA_ByteString body = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(connection->responseEncoding != UA_HTTP_CONTENT_ENCODING_IDENTITY &&
       (bodyLength >= 1024 || !connection->identityResponseAllowed)) {
        UA_ByteString encoded = UA_BYTESTRING_NULL;
        res = UA_HTTP_compress(connection->responseEncoding, buffer, &encoded);
        if(res == UA_STATUSCODE_GOOD &&
           (encoded.length < bodyLength || !connection->identityResponseAllowed)) {
            UA_ByteString_clear(buffer);
            body = encoded;
        } else {
            UA_ByteString_clear(&encoded);
            if(!connection->identityResponseAllowed)
                return res == UA_STATUSCODE_GOOD ?
                    UA_STATUSCODE_BADENCODINGERROR : res;
            connection->responseEncoding = UA_HTTP_CONTENT_ENCODING_IDENTITY;
            if(buffer) {
                body = *buffer;
                UA_ByteString_init(buffer);
            }
        }
    } else {
        connection->responseEncoding = UA_HTTP_CONTENT_ENCODING_IDENTITY;
        if(buffer) {
            body = *buffer;
            UA_ByteString_init(buffer);
        }
    }
    res = buildResponse(connection, params, &body);
    UA_ByteString_clear(&body);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    removeAcceptedConnectionTimeout(connection);
    setConnectionState(connection, HTTP_ACCEPTED_CONNECTION_RESPONDING,
                       "response queued");
    UA_LWS_requestWritable(connection->wsi);
    return UA_STATUSCODE_GOOD;
}
