/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include "eventloop_posix_http_internal.h"
#include "eventloop_posix_http_compression.h"

#include <limits.h>

struct HTTPClientRequest {
    LIST_ENTRY(HTTPClientRequest) next;
    HTTPConnection *connection;
    UA_KeyValueMap params;
    HTTPPayload payload;
    size_t receivedLength;
    UA_KeyValuePair *responseHeaders;
    size_t responseHeadersSize;
    UA_UInt16 responseStatus;
    UA_UInt16 timeout;
    UA_UInt64 timeoutId;
    UA_HTTPContentEncoding responseEncoding;
    UA_StatusCode completionStatus;
    UA_DelayedCallback completionCallback;
    UA_ByteString compressedBody;
    UA_Boolean completed;
    struct lws *wsi;
};

static UA_StatusCode
setRequestPayload(HTTPClientRequest *request, UA_ByteString *source) {
    size_t length = source ? source->length : 0;
    if(length > SIZE_MAX - LWS_PRE)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(length > 0) {
        UA_StatusCode res = UA_ByteString_allocBuffer(
            &request->payload.data, LWS_PRE + length);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        memcpy(request->payload.data.data + LWS_PRE, source->data, length);
    }
    if(source)
        UA_ByteString_clear(source);
    return UA_STATUSCODE_GOOD;
}

static void
clearClientRequest(HTTPClientRequest *request) {
    UA_KeyValueMap_clear(&request->params);
    UA_ByteString_clear(&request->payload.data);
    UA_ByteString_clear(&request->compressedBody);
    UA_HTTP_clearHeaders(&request->responseHeaders, &request->responseHeadersSize);
    UA_free(request);
}

static void
removeClientRequestTimeout(HTTPClientRequest *request) {
    if(!request->timeoutId)
        return;
    UA_EventLoop *el = request->connection->manager->cm.eventSource.eventLoop;
    el->removeTimer(el, request->timeoutId);
    request->timeoutId = 0;
}

static void
removeClientRequest(HTTPClientRequest *request) {
    LIST_REMOVE(request, next);
    clearClientRequest(request);
}

static void
notifyClientResponse(HTTPClientRequest *request, struct lws *wsi,
                     UA_ByteString message, UA_Boolean includeLength,
                     UA_Boolean complete, UA_StatusCode requestStatus) {
    HTTPConnection *connection = request->connection;
    if(connection->closeNotified)
        return;

    UA_KeyValuePair params[6] = {0};
    size_t paramsSize = 0;
    params[paramsSize].key = UA_QUALIFIEDNAME(0, "status-code");
    UA_Variant_setScalar(&params[paramsSize++].value, &request->responseStatus,
                         &UA_TYPES[UA_TYPES_UINT16]);
    params[paramsSize].key = UA_QUALIFIEDNAME(0, "headers");
    UA_Variant_setArray(&params[paramsSize++].value, request->responseHeaders,
                        request->responseHeadersSize,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    UA_UInt64 contentLength = 0;
    if(includeLength) {
        char value[32];
        if(lws_hdr_copy(wsi, value, sizeof(value),
                        WSI_TOKEN_HTTP_CONTENT_LENGTH) > 0)
            contentLength = strtoull(value, NULL, 10);
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "content-length");
        UA_Variant_setScalar(&params[paramsSize++].value, &contentLength,
                             &UA_TYPES[UA_TYPES_UINT64]);
    }
    const UA_Variant *handle = UA_KeyValueMap_get(
        &request->params, UA_QUALIFIEDNAME(0, "handle"));
    if(handle) {
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "handle");
        params[paramsSize++].value = *handle;
    }
    if(complete) {
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "response-complete");
        UA_Variant_setScalar(&params[paramsSize++].value, &complete,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);
        params[paramsSize].key = UA_QUALIFIEDNAME(0, "request-status");
        UA_Variant_setScalar(&params[paramsSize++].value, &requestStatus,
                             &UA_TYPES[UA_TYPES_STATUSCODE]);
    }
    UA_KeyValueMap map = {paramsSize, params};
    connection->callback(&connection->manager->cm, connection->connectionId,
                         connection->application, &connection->context,
                         UA_CONNECTIONSTATE_ESTABLISHED, &map, message);
}

static void
finishClientRequestNow(HTTPClientRequest *request, struct lws *wsi,
                       UA_StatusCode status) {
    notifyClientResponse(request, wsi, UA_BYTESTRING_NULL, false, true, status);
    request->wsi = NULL;
    if(wsi)
        lws_set_wsi_user(wsi, NULL);
    HTTPConnection *connection = request->connection;
    removeClientRequest(request);
    UA_HTTP_removeConnection(connection);
}

static void
finishClientRequestDeferred(void *application, void *context) {
    (void)application;
    HTTPClientRequest *request = (HTTPClientRequest*)context;
    finishClientRequestNow(request, NULL, request->completionStatus);
}

static void
finishClientRequest(HTTPClientRequest *request, struct lws *wsi,
                    UA_StatusCode status) {
    if(request->completed)
        return;
    request->completed = true;
    request->completionStatus = status;
    removeClientRequestTimeout(request);
    request->wsi = NULL;
    if(wsi)
        lws_set_wsi_user(wsi, NULL);
    /* Terminal callbacks can be raised synchronously while libwebsockets still
     * owns the active stack frame. Notify the application and release storage
     * only after returning to the EventLoop. */
    request->completionCallback.callback = finishClientRequestDeferred;
    request->completionCallback.context = request;
    UA_EventLoop *el = request->connection->manager->cm.eventSource.eventLoop;
    el->addDelayedCallback(el, &request->completionCallback);
}

static void
clientRequestTimeout(void *application, void *context) {
    (void)application;
    HTTPClientRequest *request = (HTTPClientRequest *)context;
    request->timeoutId = 0;
    struct lws *wsi = request->wsi;
    if(wsi)
        lws_set_timeout(wsi, PENDING_TIMEOUT_USER_OK, LWS_TO_KILL_ASYNC);
    finishClientRequest(request, wsi, UA_STATUSCODE_BADTIMEOUT);
}

static const UA_String *
responseHeader(const HTTPClientRequest *request, const char *name,
               UA_Boolean *duplicate, UA_Boolean *invalid) {
    const UA_String *result = NULL;
    UA_Boolean seen = false;
    for(size_t i = 0; i < request->responseHeadersSize; i++) {
        const UA_KeyValuePair *header = &request->responseHeaders[i];
        if(!UA_HTTP_stringEqualIgnoreCase(&header->key.name, name))
            continue;
        if(seen) {
            *duplicate = true;
            continue;
        }
        seen = true;
        if(!UA_Variant_hasScalarType(&header->value,
                                     &UA_TYPES[UA_TYPES_STRING])) {
            *invalid = true;
            continue;
        }
        result = (const UA_String *)header->value.data;
    }
    return result;
}

static UA_StatusCode
appendCompressedBody(HTTPClientRequest *request, const void *data,
                     size_t length) {
    size_t current = request->compressedBody.length;
    if(length > SIZE_MAX - current)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;
    size_t required = current + length;
    size_t limit = request->connection->recvMaxMessageSize;
    if(limit > 0 && required > limit)
        return UA_STATUSCODE_BADRESPONSETOOLARGE;
    UA_String fragment = {length, (UA_Byte *)(uintptr_t)data};
    return UA_String_append(&request->compressedBody, fragment);
}

static UA_StatusCode
deliverCompressedResponse(HTTPClientRequest *request, struct lws *wsi) {
    if(request->responseEncoding == UA_HTTP_CONTENT_ENCODING_IDENTITY)
        return UA_STATUSCODE_GOOD;
    size_t limit = request->connection->recvMaxDecompressedMessageSize;
    if(limit == 0)
        limit = HTTP_DEFAULT_DECOMPRESSED_LIMIT;
    UA_ByteString body = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_HTTP_decompress(request->responseEncoding,
                                           &request->compressedBody, limit,
                                           &body);
    if(res == UA_STATUSCODE_BADREQUESTTOOLARGE)
        res = UA_STATUSCODE_BADRESPONSETOOLARGE;
    if(res != UA_STATUSCODE_GOOD)
        return res;
    notifyClientResponse(request, wsi, body, false, false,
                         UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&body);
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_HTTP_COMPRESSION
static UA_Boolean
requestHasHeader(const HTTPClientRequest *request, const char *name) {
    const UA_Variant *value = UA_KeyValueMap_get(
        &request->params, UA_QUALIFIEDNAME(0, "headers"));
    if(!value ||
       !UA_Variant_hasArrayType(value, &UA_TYPES[UA_TYPES_KEYVALUEPAIR]))
        return false;
    const UA_KeyValuePair *headers = (const UA_KeyValuePair *)value->data;
    for(size_t i = 0; i < value->arrayLength; i++) {
        if(UA_HTTP_stringEqualIgnoreCase(&headers[i].key.name, name))
            return true;
    }
    return false;
}
#endif

static int
callbackHttpClient(struct lws *wsi, enum lws_callback_reasons reason,
                   void *user, void *in, size_t len) {
    switch(reason) {
    case LWS_CALLBACK_WSI_DESTROY:
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
    case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
    case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
        break;
    default:
        /* TLS and vhost initialization callbacks use protocol callback data
         * that is not an HTTPClientRequest. */
        return 0;
    }

    HTTPClientRequest *request = (HTTPClientRequest*)user;
    if(!request)
        return 0;
    HTTPConnection *connection = request->connection;
    UA_ConnectionManager *cm = &connection->manager->cm;
    UA_EventLoop *eventLoop = cm->eventSource.eventLoop;

    switch(reason) {
    case LWS_CALLBACK_COMPLETED_CLIENT_HTTP: {
        UA_StatusCode res = deliverCompressedResponse(request, wsi);
        finishClientRequest(request, wsi, res);
        return 0;
    }
    case LWS_CALLBACK_WSI_DESTROY:
        finishClientRequest(request, wsi, UA_STATUSCODE_BADCONNECTIONCLOSED);
        return 0;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        UA_LOG_WARNING(eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                       "HTTP %u\t| Connection Error %.*s",
                       (unsigned)connection->connectionId, (int)len, (char*)in);
        finishClientRequest(request, wsi, UA_STATUSCODE_BADCONNECTIONCLOSED);
        return 0;
    case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP: {
        int responseStatus = lws_http_client_http_response(wsi);
        if(responseStatus < 100 || responseStatus > 599 ||
           UA_HTTP_collectHeaders(wsi, &request->responseHeaders,
                                  &request->responseHeadersSize) !=
               UA_STATUSCODE_GOOD) {
            finishClientRequest(request, wsi, UA_STATUSCODE_BADDECODINGERROR);
            return -1;
        }
        request->responseStatus = (UA_UInt16)responseStatus;
        UA_Boolean duplicateEncoding = false, invalidEncoding = false;
        if(lws_hdr_fragment_length(wsi, WSI_TOKEN_HTTP_CONTENT_ENCODING, 1) > 0)
            duplicateEncoding = true;
        const UA_String *contentEncoding = responseHeader(
            request, "content-encoding", &duplicateEncoding, &invalidEncoding);
        request->responseEncoding =
            UA_HTTP_parseContentEncoding(contentEncoding);
        if(duplicateEncoding || invalidEncoding ||
           request->responseEncoding == UA_HTTP_CONTENT_ENCODING_UNSUPPORTED) {
            finishClientRequest(request, wsi, UA_STATUSCODE_BADDECODINGERROR);
            return -1;
        }
        notifyClientResponse(request, wsi, UA_BYTESTRING_NULL, false, false,
                             UA_STATUSCODE_GOOD);
        return 0;
    }
    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ: {
        if(len > SIZE_MAX - request->receivedLength) {
            finishClientRequest(request, wsi,
                                UA_STATUSCODE_BADRESPONSETOOLARGE);
            return -1;
        }
        request->receivedLength += len;
        if(connection->recvMaxMessageSize > 0 &&
           request->receivedLength > connection->recvMaxMessageSize) {
            finishClientRequest(request, wsi,
                                UA_STATUSCODE_BADRESPONSETOOLARGE);
            return -1;
        }
        if(request->responseEncoding == UA_HTTP_CONTENT_ENCODING_IDENTITY) {
            UA_ByteString message = {len, (UA_Byte*)in};
            notifyClientResponse(request, wsi, message, true, false,
                                 UA_STATUSCODE_GOOD);
        } else {
            UA_StatusCode res = appendCompressedBody(request, in, len);
            if(res != UA_STATUSCODE_GOOD) {
                finishClientRequest(request, wsi, res);
                return -1;
            }
        }
        return 0;
    }
    case LWS_CALLBACK_RECEIVE_CLIENT_HTTP: {
        char buffer[1024 + LWS_PRE];
        char *position = buffer + LWS_PRE;
        int available = sizeof(buffer) - LWS_PRE;
        return lws_http_client_read(wsi, &position, &available) < 0 ? -1 : 0;
    }
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: {
        unsigned char **position = (unsigned char**)in;
        unsigned char *end = *position + len;
#ifdef UA_ENABLE_HTTP_COMPRESSION
        if(connection->recvMaxDecompressedMessageSize > 0 &&
           !requestHasHeader(request, "accept-encoding")) {
            static const unsigned char name[] = "accept-encoding:";
            static const unsigned char value[] = "gzip, deflate";
            if(lws_add_http_header_by_name(wsi, name, value,
                                           sizeof(value) - 1, position, end))
                return -1;
        }
#endif
        size_t payloadLength = UA_HTTP_payloadLength(&request->payload);
        if(payloadLength > 0) {
            char lengthString[32];
            int written = mp_snprintf(lengthString, sizeof(lengthString), "%zu",
                                      payloadLength);
            if(written < 0 || written >= (int)sizeof(lengthString) ||
               lws_add_http_header_by_token(
                   wsi, WSI_TOKEN_HTTP_CONTENT_LENGTH,
                   (unsigned char*)lengthString, written, position, end))
                return -1;
            lws_client_http_body_pending(wsi, 1);
        }
        return UA_HTTP_addHeaders(wsi, &request->params, position, end);
    }
    case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE: {
        int result = UA_HTTP_writePayload(wsi, &request->payload);
        if(result <= 0)
            return result;
        lws_client_http_body_pending(wsi, 0);
        return 0;
    }
    default:
        return 0;
    }
}

const struct lws_protocols UA_HTTP_clientProtocols[] = {
    {"http", callbackHttpClient, 0, 0, 0, NULL, 0},
    LWS_PROTOCOL_LIST_TERM
};

void
UA_HTTP_closeClientRequests(HTTPConnection *connection) {
    HTTPClientRequest *request;
    LIST_FOREACH(request, &connection->clientRequests, next) {
        UA_HTTP_closeWsi(request->wsi);
        request->wsi = NULL;
    }
}

static const UA_String slashPath = UA_STRING_STATIC("/");
static const UA_String getMethod = UA_STRING_STATIC("GET");

UA_StatusCode
UA_HTTP_sendClientRequest(HTTPConnection *connection,
                          const UA_KeyValueMap *params, UA_ByteString *buffer) {
    if(connection->closing)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    size_t payloadLength = buffer ? buffer->length : 0;
    if(connection->sendMaxMessageSize > 0 &&
       payloadLength > connection->sendMaxMessageSize)
        return UA_STATUSCODE_BADREQUESTTOOLARGE;

    const UA_QualifiedName handleName = UA_QUALIFIEDNAME(0, "handle");
    const UA_Variant *handle = UA_KeyValueMap_get(params, handleName);
    HTTPClientRequest *outstanding;
    LIST_FOREACH(outstanding, &connection->clientRequests, next) {
        if(outstanding->completed)
            continue;
        const UA_Variant *outstandingHandle =
            UA_KeyValueMap_get(&outstanding->params, handleName);
        if(!handle || !outstandingHandle ||
           UA_equal(handle, outstandingHandle, &UA_TYPES[UA_TYPES_VARIANT]))
            return UA_STATUSCODE_BADINVALIDSTATE;
    }

    HTTPClientRequest *request = (HTTPClientRequest*)UA_calloc(1, sizeof(*request));
    if(!request)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    request->connection = connection;
    const UA_UInt16 *timeout = (const UA_UInt16*)GET_PARAM(
        params, "timeout", UA_TYPES_UINT16);
    request->timeout = (timeout && *timeout) ? *timeout : connection->timeout;
    UA_StatusCode res = UA_KeyValueMap_copy(params, &request->params);
    if(res != UA_STATUSCODE_GOOD) {
        clearClientRequest(request);
        return res;
    }
    res = setRequestPayload(request, buffer);
    if(res != UA_STATUSCODE_GOOD) {
        clearClientRequest(request);
        return res;
    }

    const UA_String *path = (const UA_String*)GET_PARAM(
        params, "path", UA_TYPES_STRING);
    const UA_String *method = (const UA_String*)GET_PARAM(
        params, "method", UA_TYPES_STRING);
    if(!path)
        path = connection->path.length > 0 ? &connection->path : &slashPath;
    if(!method)
        method = &getMethod;
    char *addressString = UA_HTTP_copyCString(&connection->address, "");
    char *pathString = UA_HTTP_copyCString(path, "/");
    char *methodString = UA_HTTP_copyCString(method, "GET");
    if(!addressString || !pathString || !methodString) {
        UA_free(addressString); UA_free(pathString); UA_free(methodString);
        clearClientRequest(request);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    struct lws_client_connect_info info;
    memset(&info, 0, sizeof(info));
    UA_EventLoopPOSIX *eventLoop =
        (UA_EventLoopPOSIX*)connection->manager->cm.eventSource.eventLoop;
    info.context = (struct lws_context*)eventLoop->lwsContext;
    info.vhost = connection->vhost;
    info.userdata = request;
    info.protocol = "http";
    info.address = addressString;
    info.port = connection->port;
    info.path = pathString;
    info.method = methodString;
    info.host = info.address;
    info.origin = info.address;
    info.ssl_connection |= LCCSCF_H2_QUIRK_OVERFLOWS_TXCR |
        LCCSCF_H2_QUIRK_NGHTTP2_END_STREAM;
    if(connection->useSSL)
        info.ssl_connection |= LCCSCF_USE_SSL;

    LIST_INSERT_HEAD(&connection->clientRequests, request, next);
    struct lws *wsi = lws_client_connect_via_info(&info);
    UA_free(addressString); UA_free(pathString); UA_free(methodString);
    if(request->completed)
        return UA_STATUSCODE_GOOD;
    request->wsi = wsi;
    if(!wsi) {
        removeClientRequest(request);
        return UA_STATUSCODE_BADNOTCONNECTED;
    }
    if(request->timeout) {
        UA_EventLoop *el = connection->manager->cm.eventSource.eventLoop;
        res = el->addTimer(el, clientRequestTimeout, NULL, request,
                           (UA_Double)request->timeout * 1000.0, NULL,
                           UA_TIMERPOLICY_ONCE, &request->timeoutId);
        if(res != UA_STATUSCODE_GOOD) {
            struct lws *wsi = request->wsi;
            request->wsi = NULL;
            lws_set_wsi_user(wsi, NULL);
            lws_set_timeout(wsi, PENDING_TIMEOUT_USER_OK,
                            LWS_TO_KILL_ASYNC);
            removeClientRequest(request);
            UA_HTTP_removeConnection(connection);
            return res;
        }
    }
    return UA_STATUSCODE_GOOD;
}
