/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "eventloop_posix_http_internal.h"

#include <ctype.h>
#include <limits.h>

/***********************/
/* Parameters and Types */
/***********************/

static const UA_KeyValueRestriction httpConnectionParams[] = {
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("path")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("port")}, &UA_TYPES[UA_TYPES_UINT16], true, true, false},
    {{0, UA_STRING_STATIC("timeout")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("listen")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("useSSL")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("certificate")}, &UA_TYPES[UA_TYPES_BYTESTRING], false, true, false},
    {{0, UA_STRING_STATIC("private-key")}, &UA_TYPES[UA_TYPES_BYTESTRING], false, true, false},
    {{0, UA_STRING_STATIC("private-key-password")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("ca-certificate")}, &UA_TYPES[UA_TYPES_BYTESTRING], false, true, false},
    {{0, UA_STRING_STATIC("recv-max-message-size")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("recv-max-decompressed-message-size")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-max-message-size")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("validate")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false}
};

static const UA_KeyValueRestriction httpSendParams[] = {
    {{0, UA_STRING_STATIC("path")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("method")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("status-code")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("headers")}, &UA_TYPES[UA_TYPES_KEYVALUEPAIR], false, false, true},
    {{0, UA_STRING_STATIC("handle")}, NULL, false, true, true},
    {{0, UA_STRING_STATIC("content-coding-policy")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("timeout")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false}
};

typedef struct {
    const UA_String *address;
    const UA_String *path;
    const UA_ByteString *certificate;
    const UA_ByteString *privateKey;
    const UA_String *privateKeyPassword;
    const UA_ByteString *caCertificate;
    UA_UInt16 port;
    UA_UInt16 timeout;
    UA_UInt32 recvMaxMessageSize;
    UA_UInt32 recvMaxDecompressedMessageSize;
    UA_UInt32 sendMaxMessageSize;
    UA_Boolean listener;
    UA_Boolean useSSL;
} HTTPConfig;


/******************/
/* Header Handling */
/******************/

char *
UA_HTTP_copyCString(const UA_String *src, const char *fallback) {
    size_t length = src ? src->length : strlen(fallback);
    if(length == SIZE_MAX)
        return NULL;
    char *dst = (char*)UA_malloc(length + 1);
    if(!dst)
        return NULL;
    memcpy(dst, src ? src->data : (const UA_Byte*)fallback, length);
    dst[length] = 0;
    return dst;
}

/* Returns one when complete, zero when more write callbacks are needed. */
int
UA_HTTP_writePayload(struct lws *wsi, HTTPPayload *payload) {
    size_t payloadLength = UA_HTTP_payloadLength(payload);
    if(payload->offset >= payloadLength)
        return 1;
    size_t remaining = payloadLength - payload->offset;
    size_t length = UA_MIN(remaining, (size_t)HTTP_WRITE_CHUNK_SIZE);
    lws_fileofs_t allowance = lws_get_peer_write_allowance(wsi);
    if(allowance == 0) {
        UA_LWS_requestWritable(wsi);
        return 0;
    }
    if(allowance > 0 && (lws_fileofs_t)length > allowance)
        length = (size_t)allowance;
    int written = lws_write(wsi, payload->data.data + LWS_PRE + payload->offset,
                            length, LWS_WRITE_HTTP);
    /* lws_write can report more than the payload length when transport
     * framing is included. A short write means that the connection failed. */
    if(written < 0 || (size_t)written < length)
        return -1;
    payload->offset += length;
    if(payload->offset >= payloadLength)
        return 1;
    UA_LWS_requestWritable(wsi);
    return 0;
}

UA_Boolean
UA_HTTP_stringEqualIgnoreCase(const UA_String *value, const char *literal) {
    size_t length = strlen(literal);
    if(value->length != length)
        return false;
    for(size_t i = 0; i < length; i++) {
        if(tolower((unsigned char)value->data[i]) !=
           tolower((unsigned char)literal[i]))
            return false;
    }
    return true;
}

void
UA_HTTP_clearHeaders(UA_KeyValuePair **headers, size_t *headersSize) {
    if(*headers)
        UA_Array_delete(*headers, *headersSize, &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    *headers = NULL;
    *headersSize = 0;
}

UA_StatusCode
UA_HTTP_appendHeader(UA_KeyValuePair **headers, size_t *headersSize,
                     const char *name, size_t nameLength,
                     const char *value, size_t valueLength) {
    while(nameLength > 0 && name[nameLength - 1] == ':')
        nameLength--;
    if(nameLength == 0)
        return UA_STATUSCODE_GOOD;

    if(*headersSize == SIZE_MAX / sizeof(UA_KeyValuePair))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    size_t newSize = *headersSize + 1;
    UA_KeyValuePair *newHeaders = (UA_KeyValuePair*)UA_realloc(
        *headers, newSize * sizeof(UA_KeyValuePair));
    if(!newHeaders)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *headers = newHeaders;
    UA_KeyValuePair *header = &newHeaders[*headersSize];
    memset(header, 0, sizeof(*header));

    UA_StatusCode res = UA_ByteString_allocBuffer(
        (UA_ByteString*)&header->key.name, nameLength);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    for(size_t i = 0; i < nameLength; i++)
        header->key.name.data[i] = (UA_Byte)tolower((unsigned char)name[i]);

    UA_String headerValue = {valueLength, (UA_Byte*)(uintptr_t)value};
    res = UA_Variant_setScalarCopy(&header->value, &headerValue,
                                   &UA_TYPES[UA_TYPES_STRING]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_QualifiedName_clear(&header->key);
        return res;
    }
    *headersSize = newSize;
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_LWS_HTTP_CUSTOM_HEADERS
typedef struct {
    struct lws *wsi;
    UA_KeyValuePair **headers;
    size_t *headersSize;
    UA_StatusCode status;
} CustomHeaderContext;

static void
appendCustomHeader(const char *name, int nameLength, void *opaque) {
    CustomHeaderContext *ctx = (CustomHeaderContext*)opaque;
    if(ctx->status != UA_STATUSCODE_GOOD || nameLength <= 0)
        return;
    int valueLength = lws_hdr_custom_length(ctx->wsi, name, nameLength);
    if(valueLength < 0)
        return;
    char *value = (char*)UA_malloc((size_t)valueLength + 1);
    if(!value) {
        ctx->status = UA_STATUSCODE_BADOUTOFMEMORY;
        return;
    }
    int copied = lws_hdr_custom_copy(ctx->wsi, value, valueLength + 1,
                                     name, nameLength);
    if(copied < 0)
        ctx->status = UA_STATUSCODE_BADINTERNALERROR;
    else
        ctx->status = UA_HTTP_appendHeader(ctx->headers, ctx->headersSize, name,
                                          (size_t)nameLength, value,
                                          (size_t)copied);
    UA_free(value);
}
#endif

UA_StatusCode
UA_HTTP_collectHeaders(struct lws *wsi, UA_KeyValuePair **headers,
                       size_t *headersSize) {
    /* Method URI tokens precede HOST. URI arguments and parser-only tokens
     * follow the last normal HTTP header token. */
    for(int token = (int)WSI_TOKEN_HOST;
        token < (int)WSI_TOKEN_HTTP_URI_ARGS; token++) {
        enum lws_token_indexes index = (enum lws_token_indexes)token;
        int valueLength = lws_hdr_total_length(wsi, index);
        if(valueLength <= 0)
            continue;
        const unsigned char *name = lws_token_to_string(index);
        if(!name || name[0] == ':' || name[0] == '_')
            continue;
        char *value = (char*)UA_malloc((size_t)valueLength + 1);
        if(!value)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        int copied = lws_hdr_copy(wsi, value, valueLength + 1, index);
        UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
        if(copied >= 0)
            res = UA_HTTP_appendHeader(headers, headersSize, (const char*)name,
                                       strlen((const char*)name), value,
                                       (size_t)copied);
        UA_free(value);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

#ifdef UA_LWS_HTTP_CUSTOM_HEADERS
    CustomHeaderContext ctx = {wsi, headers, headersSize, UA_STATUSCODE_GOOD};
    if(lws_hdr_custom_name_foreach(wsi, appendCustomHeader, &ctx) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    return ctx.status;
#else
    return UA_STATUSCODE_GOOD;
#endif
}

static const UA_KeyValuePair *
getHeaderArray(const UA_KeyValueMap *params, size_t *headersSize) {
    *headersSize = 0;
    const UA_Variant *headers = UA_KeyValueMap_get(
        params, UA_QUALIFIEDNAME(0, "headers"));
    if(!headers)
        return NULL;
    *headersSize = headers->arrayLength;
    return (const UA_KeyValuePair*)headers->data;
}

static UA_Boolean
isManagedHeader(const UA_String *name) {
    return UA_HTTP_stringEqualIgnoreCase(name, "content-length") ||
        UA_HTTP_stringEqualIgnoreCase(name, "transfer-encoding") ||
        UA_HTTP_stringEqualIgnoreCase(name, "connection");
}

static UA_Boolean
validHeaderName(const UA_String *name) {
    if(name->length == 0 || name->length > (size_t)INT_MAX)
        return false;
    for(size_t i = 0; i < name->length; i++) {
        unsigned char c = name->data[i];
        if(isalnum(c) || c == '!' || c == '#' || c == '$' || c == '%' ||
           c == '&' || c == '\'' || c == '*' || c == '+' || c == '-' ||
           c == '.' || c == '^' || c == '_' || c == '`' || c == '|' ||
           c == '~')
            continue;
        return false;
    }
    return true;
}

static UA_Boolean
validHeaderValue(const UA_String *value) {
    if(value->length > (size_t)INT_MAX)
        return false;
    for(size_t i = 0; i < value->length; i++) {
        unsigned char c = value->data[i];
        if((c < 0x20 && c != '\t') || c == 0x7f)
            return false;
    }
    return true;
}

static UA_StatusCode
validateHeaders(const UA_KeyValueMap *params) {
    size_t headersSize = 0;
    const UA_KeyValuePair *headers = getHeaderArray(params, &headersSize);
    for(size_t i = 0; i < headersSize; i++) {
        if(headers[i].key.namespaceIndex != 0 ||
           !validHeaderName(&headers[i].key.name) ||
           !UA_Variant_hasScalarType(&headers[i].value,
                                     &UA_TYPES[UA_TYPES_STRING]) ||
           isManagedHeader(&headers[i].key.name))
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        const UA_String *value = (const UA_String*)headers[i].value.data;
        if(!validHeaderValue(value))
            return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
validateSendParameters(const UA_KeyValueMap *params) {
    UA_StatusCode res = validateHeaders(params);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    const UA_UInt16 *statusCode = (const UA_UInt16*)GET_PARAM(
        params, "status-code", UA_TYPES_UINT16);
    if(statusCode && (*statusCode < 100 || *statusCode > 599))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
rejectUnknownParameters(const UA_KeyValueRestriction *restrictions,
                        size_t restrictionsSize,
                        const UA_KeyValueMap *params) {
    for(size_t i = 0; i < params->mapSize; i++) {
        UA_Boolean known = false;
        for(size_t j = 0; j < restrictionsSize; j++) {
            if(UA_QualifiedName_equal(&params->map[i].key,
                                      &restrictions[j].name)) {
                known = true;
                break;
            }
        }
        if(!known)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    return UA_STATUSCODE_GOOD;
}

int
UA_HTTP_addHeaders(struct lws *wsi, const UA_KeyValueMap *params,
                   unsigned char **position, unsigned char *end) {
    size_t headersSize = 0;
    const UA_KeyValuePair *headers = getHeaderArray(params, &headersSize);
    for(size_t i = 0; i < headersSize; i++) {
        const UA_String *value = (const UA_String*)headers[i].value.data;
        const UA_String *name = &headers[i].key.name;
        char *headerName = (char*)UA_malloc(name->length + 2);
        if(!headerName)
            return -1;
        memcpy(headerName, name->data, name->length);
        headerName[name->length] = ':';
        headerName[name->length + 1] = 0;
        int res = lws_add_http_header_by_name(
            wsi, (const unsigned char*)headerName, value->data,
            (int)value->length, position, end);
        UA_free(headerName);
        if(res)
            return -1;
    }
    return 0;
}

size_t
UA_HTTP_responseHeaderBufferSize(const UA_KeyValueMap *params) {
    size_t size = 512;
    size_t headersSize = 0;
    const UA_KeyValuePair *headers = getHeaderArray(params, &headersSize);
    for(size_t i = 0; i < headersSize; i++) {
        const UA_String *value = (const UA_String*)headers[i].value.data;
        size_t nameLength = headers[i].key.name.length;
        if(nameLength > SIZE_MAX - size ||
           value->length > SIZE_MAX - size - nameLength ||
           8 > SIZE_MAX - size - nameLength - value->length)
            return 0;
        size += nameLength + value->length + 8;
    }
    return size;
}

static HTTPConnection *
findHTTPConnection(HTTPConnectionManager *manager, uintptr_t connectionId) {
    HTTPConnection *connection;
    LIST_FOREACH(connection, &manager->connections, next) {
        if(connection->connectionId == connectionId)
            return connection;
    }
    return NULL;
}

UA_UInt32
UA_HTTP_nextConnectionId(HTTPConnectionManager *manager) {
    UA_UInt32 first = manager->lastConnectionId + 1;
    if(first == 0)
        first = 1;
    UA_UInt32 candidate = first;
    do {
        if(!findHTTPConnection(manager, candidate) &&
           !UA_HTTP_findAcceptedConnection(manager, candidate)) {
            manager->lastConnectionId = candidate;
            return candidate;
        }
        candidate++;
        if(candidate == 0)
            candidate = 1;
    } while(candidate != first);
    return 0;
}

/***********************/
/* Connection Lifecycle */
/***********************/

void
UA_HTTP_closeWsi(struct lws *wsi) {
    if(!wsi)
        return;
    lws_set_timeout(wsi, PENDING_TIMEOUT_CLOSE_SEND, LWS_TO_KILL_ASYNC);
    UA_LWS_requestWritable(wsi);
}

void
UA_HTTP_updateStoppedState(HTTPConnectionManager *manager) {
    if(manager->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING &&
       LIST_EMPTY(&manager->connections) &&
       LIST_EMPTY(&manager->acceptedConnections))
        manager->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
}

void
UA_HTTP_removeConnection(HTTPConnection *connection) {
    if(!connection->closing || !LIST_EMPTY(&connection->clientRequests) ||
       connection->closeScheduled)
        return;

    LIST_REMOVE(connection, next);
    if(connection->vhost) {
        struct lws_vhost *vhost = connection->vhost;
        connection->vhost = NULL;
        lws_vhost_destroy(vhost);
    }

    HTTPConnectionManager *manager = connection->manager;
    if(!connection->closeNotified) {
        connection->closeNotified = true;
        connection->callback(&manager->cm, connection->connectionId,
                             connection->application, &connection->context,
                             UA_CONNECTIONSTATE_CLOSING,
                             &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    }
    UA_String_clear(&connection->address);
    UA_String_clear(&connection->path);
    UA_free(connection);
    UA_LWS_releaseContext(manager->cm.eventSource.eventLoop);

    UA_HTTP_updateStoppedState(manager);
}

static void
closeConnectionDelayed(void *application, void *context) {
    (void)application;
    HTTPConnection *connection = (HTTPConnection*)context;
    connection->closeScheduled = false;
    if(!connection->closeNotified) {
        connection->closeNotified = true;
        HTTPConnectionManager *manager = connection->manager;
        connection->callback(&manager->cm, connection->connectionId,
                             connection->application, &connection->context,
                             UA_CONNECTIONSTATE_CLOSING,
                             &UA_KEYVALUEMAP_NULL, UA_BYTESTRING_NULL);
    }
    if(connection->listener && connection->vhost) {
        struct lws_vhost *vhost = connection->vhost;
        connection->vhost = NULL;
        UA_HTTP_closeAcceptedConnections(connection);
        lws_vhost_destroy(vhost);
    }
    UA_HTTP_removeConnection(connection);
}

void
UA_HTTP_closeConnection(HTTPConnection *connection) {
    if(connection->closing)
        return;
    connection->closing = true;

    /* Publish the logical close before individual request failures drain. No
     * request callback is emitted after CLOSING, while libwebsockets-owned
     * storage remains alive until its callbacks have unwound. */
    connection->closeScheduled = true;
    connection->closeCallback.callback = closeConnectionDelayed;
    connection->closeCallback.context = connection;
    UA_EventLoop *el = connection->manager->cm.eventSource.eventLoop;
    el->addDelayedCallback(el, &connection->closeCallback);

    UA_HTTP_closeClientRequests(connection);

    /* Listener destruction and client-request storage both remain deferred.
     * In particular, a listener can be closed from an accepted callback. */
}

static UA_StatusCode
parseConfig(const UA_KeyValueMap *params, HTTPConfig *config) {
    memset(config, 0, sizeof(*config));
    config->address = (const UA_String*)GET_PARAM(params, "address", UA_TYPES_STRING);
    config->path = (const UA_String*)GET_PARAM(params, "path", UA_TYPES_STRING);
    config->certificate = (const UA_ByteString*)GET_PARAM(
        params, "certificate", UA_TYPES_BYTESTRING);
    config->privateKey = (const UA_ByteString*)GET_PARAM(
        params, "private-key", UA_TYPES_BYTESTRING);
    config->privateKeyPassword = (const UA_String*)GET_PARAM(
        params, "private-key-password", UA_TYPES_STRING);
    config->caCertificate = (const UA_ByteString*)GET_PARAM(
        params, "ca-certificate", UA_TYPES_BYTESTRING);
    const UA_UInt16 *port = (const UA_UInt16*)GET_PARAM(
        params, "port", UA_TYPES_UINT16);
    const UA_UInt16 *timeout = (const UA_UInt16*)GET_PARAM(
        params, "timeout", UA_TYPES_UINT16);
    const UA_UInt32 *recvMax = (const UA_UInt32*)GET_PARAM(
        params, "recv-max-message-size", UA_TYPES_UINT32);
    const UA_UInt32 *recvMaxDecompressed = (const UA_UInt32*)GET_PARAM(
        params, "recv-max-decompressed-message-size", UA_TYPES_UINT32);
    const UA_UInt32 *sendMax = (const UA_UInt32*)GET_PARAM(
        params, "send-max-message-size", UA_TYPES_UINT32);
    const UA_Boolean *listen = (const UA_Boolean*)GET_PARAM(
        params, "listen", UA_TYPES_BOOLEAN);
    const UA_Boolean *useSSL = (const UA_Boolean*)GET_PARAM(
        params, "useSSL", UA_TYPES_BOOLEAN);
    config->port = port ? *port : 0;
    config->timeout = (timeout && *timeout) ? *timeout : 30;
    config->recvMaxMessageSize = recvMax ? *recvMax : 0;
    config->recvMaxDecompressedMessageSize = recvMaxDecompressed ?
        *recvMaxDecompressed : config->recvMaxMessageSize;
    config->sendMaxMessageSize = sendMax ? *sendMax : 0;
    config->listener = listen && *listen;
    config->useSSL = useSSL && *useSSL;
    if(!config->listener && (!config->address || config->address->length == 0))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if((config->certificate && !config->privateKey) ||
       (!config->certificate && config->privateKey) ||
       (config->certificate &&
        (config->certificate->length == 0 || config->privateKey->length == 0 ||
         config->certificate->length > UINT_MAX ||
         config->privateKey->length > UINT_MAX)) ||
       (config->caCertificate && (config->caCertificate->length == 0 ||
                                  config->caCertificate->length > UINT_MAX)) ||
       ((config->certificate || config->caCertificate) && !config->useSSL) ||
       (config->listener && config->useSSL && !config->certificate))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
#ifndef LWS_WITH_TLS
    if(config->useSSL || config->certificate || config->caCertificate)
        return UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    return UA_STATUSCODE_GOOD;
}

/**************************/
/* Vhost and Public API   */
/**************************/

static struct lws_vhost *
createLwsVhost(HTTPConnection *connection, struct lws_context *context,
               const HTTPConfig *config) {
    char *interfaceName = connection->listener ?
        UA_HTTP_copyCString(config->address, "") : NULL;
    char *password = UA_HTTP_copyCString(config->privateKeyPassword, "");
    if((connection->listener && !interfaceName) || !password) {
        UA_free(interfaceName);
        UA_free(password);
        return NULL;
    }

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    UA_LWS_useContextLogger(&info, context);
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    if(connection->useSSL)
        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = connection->listener ? config->port : CONTEXT_PORT_NO_LISTEN;
    info.iface = connection->listener && interfaceName[0] ? interfaceName : NULL;
    info.protocols = connection->listener ?
        UA_HTTP_serverProtocols : UA_HTTP_clientProtocols;
    if(connection->listener) {
        info.options |= LWS_SERVER_OPTION_ADOPT_APPLY_LISTEN_ACCEPT_CONFIG;
        info.listen_accept_role = "raw-skt";
        info.listen_accept_protocol = "http-raw";
    }
    info.user = connection->listener ? connection : NULL;
    info.timeout_secs = config->timeout;
    info.connect_timeout_secs = config->timeout;
#ifdef LWS_WITH_TLS
    if(config->certificate) {
        if(connection->listener) {
            info.server_ssl_cert_mem = config->certificate->data;
            info.server_ssl_cert_mem_len = (unsigned int)config->certificate->length;
            info.server_ssl_private_key_mem = config->privateKey->data;
            info.server_ssl_private_key_mem_len = (unsigned int)config->privateKey->length;
            info.ssl_private_key_password = password;
        } else {
            info.client_ssl_cert_mem = config->certificate->data;
            info.client_ssl_cert_mem_len = (unsigned int)config->certificate->length;
            info.client_ssl_key_mem = config->privateKey->data;
            info.client_ssl_key_mem_len = (unsigned int)config->privateKey->length;
            info.client_ssl_private_key_password = password;
        }
    }
    if(config->caCertificate) {
        if(connection->listener) {
            info.server_ssl_ca_mem = config->caCertificate->data;
            info.server_ssl_ca_mem_len = (unsigned int)config->caCertificate->length;
            info.options |= LWS_SERVER_OPTION_REQUIRE_VALID_OPENSSL_CLIENT_CERT;
        } else {
            info.client_ssl_ca_mem = config->caCertificate->data;
            info.client_ssl_ca_mem_len = (unsigned int)config->caCertificate->length;
        }
    }
#endif

    struct lws_vhost *vhost = lws_create_vhost(context, &info);
    if(vhost && connection->useSSL && !connection->listener &&
       lws_init_vhost_client_ssl(&info, vhost)) {
        lws_vhost_destroy(vhost);
        vhost = NULL;
    }
    UA_LWS_clearActiveLogger();
    UA_free(interfaceName);
    UA_free(password);
    return vhost;
}

static UA_StatusCode
HTTP_openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                    void *application, void *context,
                    UA_ConnectionManager_connectionCallback callback) {
    UA_StatusCode res = UA_KeyValueRestriction_validate(
        cm->eventSource.eventLoop->logger, "HTTP", httpConnectionParams,
        ARRAY_SIZE(httpConnectionParams), params);
    if(res == UA_STATUSCODE_GOOD)
        res = rejectUnknownParameters(httpConnectionParams,
                                      ARRAY_SIZE(httpConnectionParams), params);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    HTTPConfig config;
    res = parseConfig(params, &config);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    const UA_Boolean *validate = (const UA_Boolean*)GET_PARAM(
        params, "validate", UA_TYPES_BOOLEAN);
    if(validate && *validate)
        return UA_STATUSCODE_GOOD;
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED || !callback)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    HTTPConnectionManager *manager = (HTTPConnectionManager*)cm;
    HTTPConnection *connection = (HTTPConnection*)UA_calloc(1, sizeof(*connection));
    if(!connection)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    connection->manager = manager;
    connection->connectionId = UA_HTTP_nextConnectionId(manager);
    if(connection->connectionId == 0) {
        UA_free(connection);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    connection->application = application;
    connection->context = context;
    connection->callback = callback;
    LIST_INIT(&connection->clientRequests);
    connection->port = config.port;
    connection->listener = config.listener;
    connection->useSSL = config.useSSL;
    connection->timeout = config.timeout;
    connection->recvMaxMessageSize = config.recvMaxMessageSize;
    connection->recvMaxDecompressedMessageSize =
        config.recvMaxDecompressedMessageSize;
    connection->sendMaxMessageSize = config.sendMaxMessageSize;
    if((config.address &&
        UA_String_copy(config.address, &connection->address) != UA_STATUSCODE_GOOD) ||
       (config.path &&
        UA_String_copy(config.path, &connection->path) != UA_STATUSCODE_GOOD)) {
        UA_String_clear(&connection->address);
        UA_String_clear(&connection->path);
        UA_free(connection);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    struct lws_context *lwsContext = UA_LWS_acquireContext(cm->eventSource.eventLoop);
    if(!lwsContext) {
        UA_String_clear(&connection->address);
        UA_String_clear(&connection->path);
        UA_free(connection);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }
    connection->vhost = createLwsVhost(connection, lwsContext, &config);
    if(!connection->vhost) {
        UA_LWS_releaseContext(cm->eventSource.eventLoop);
        UA_String_clear(&connection->address);
        UA_String_clear(&connection->path);
        UA_free(connection);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }
    LIST_INSERT_HEAD(&manager->connections, connection, next);

    if(connection->listener) {
        UA_UInt16 listenPort =
            (UA_UInt16)lws_get_vhost_listen_port(connection->vhost);
        UA_KeyValuePair callbackParams[2];
        callbackParams[0].key = UA_QUALIFIEDNAME(0, "listen-address");
        UA_Variant_setScalar(&callbackParams[0].value, &connection->address,
                             &UA_TYPES[UA_TYPES_STRING]);
        callbackParams[1].key = UA_QUALIFIEDNAME(0, "listen-port");
        UA_Variant_setScalar(&callbackParams[1].value, &listenPort,
                             &UA_TYPES[UA_TYPES_UINT16]);
        UA_KeyValueMap callbackMap = {2, callbackParams};
        callback(cm, connection->connectionId, application, &connection->context,
                 UA_CONNECTIONSTATE_ESTABLISHED, &callbackMap, UA_BYTESTRING_NULL);
    } else {
        callback(cm, connection->connectionId, application, &connection->context,
                 UA_CONNECTIONSTATE_OPENING, &UA_KEYVALUEMAP_NULL,
                 UA_BYTESTRING_NULL);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
finishSend(UA_StatusCode result, UA_ByteString *buffer) {
    if(result != UA_STATUSCODE_GOOD && buffer)
        UA_ByteString_clear(buffer);
    return result;
}

static UA_StatusCode
HTTP_sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                        const UA_KeyValueMap *params, UA_ByteString *buffer) {
    UA_StatusCode res = UA_KeyValueRestriction_validate(
        cm->eventSource.eventLoop->logger, "HTTP", httpSendParams,
        ARRAY_SIZE(httpSendParams), params);
    if(res == UA_STATUSCODE_GOOD)
        res = rejectUnknownParameters(httpSendParams,
                                      ARRAY_SIZE(httpSendParams), params);
    if(res == UA_STATUSCODE_GOOD)
        res = validateSendParameters(params);
    if(res != UA_STATUSCODE_GOOD)
        return finishSend(res, buffer);

    HTTPConnectionManager *manager = (HTTPConnectionManager*)cm;
    HTTPConnection *connection = findHTTPConnection(manager, connectionId);
    if(connection) {
        if(connection->listener || UA_KeyValueMap_get(
               params, UA_QUALIFIEDNAME(0, "status-code")) ||
           UA_KeyValueMap_get(
               params, UA_QUALIFIEDNAME(0, "content-coding-policy"))) {
            return finishSend(UA_STATUSCODE_BADINVALIDARGUMENT, buffer);
        }
        return finishSend(UA_HTTP_sendClientRequest(connection, params, buffer),
                          buffer);
    }

    HTTPAcceptedConnection *accepted =
        UA_HTTP_findAcceptedConnection(manager, connectionId);
    if(!accepted)
        return finishSend(UA_STATUSCODE_BADNOTFOUND, buffer);
    if(UA_KeyValueMap_get(params, UA_QUALIFIEDNAME(0, "path")) ||
       UA_KeyValueMap_get(params, UA_QUALIFIEDNAME(0, "method")))
        return finishSend(UA_STATUSCODE_BADINVALIDARGUMENT, buffer);
    return finishSend(UA_HTTP_sendServerResponse(accepted, params, buffer),
                      buffer);
}

static UA_StatusCode
HTTP_closeConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    HTTPConnectionManager *manager = (HTTPConnectionManager*)cm;
    HTTPConnection *connection = findHTTPConnection(manager, connectionId);
    if(connection) {
        UA_HTTP_closeConnection(connection);
        return UA_STATUSCODE_GOOD;
    }
    HTTPAcceptedConnection *accepted =
        UA_HTTP_findAcceptedConnection(manager, connectionId);
    if(!accepted)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_HTTP_closeAcceptedConnection(accepted);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
HTTP_eventSourceStart(UA_ConnectionManager *cm) {
    UA_EventLoop *eventLoop = cm->eventSource.eventLoop;
    if(!eventLoop || cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
HTTP_eventSourceStop(UA_ConnectionManager *cm) {
    if(cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPING ||
       cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPED)
        return;
    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;
    HTTPConnectionManager *manager = (HTTPConnectionManager*)cm;
    HTTPConnection *connection, *connectionTmp;
    LIST_FOREACH_SAFE(connection, &manager->connections, next, connectionTmp)
        UA_HTTP_closeConnection(connection);
    UA_HTTP_updateStoppedState(manager);
}

static UA_StatusCode
HTTP_eventSourceDelete(UA_ConnectionManager *cm) {
    if(cm->eventSource.state >= UA_EVENTSOURCESTATE_STARTING)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_String_clear(&cm->eventSource.name);
    UA_KeyValueMap_clear(&cm->eventSource.params);
    UA_free(cm);
    return UA_STATUSCODE_GOOD;
}

UA_ConnectionManager *
UA_ConnectionManager_new_HTTP(const UA_String eventSourceName) {
    HTTPConnectionManager *manager =
        (HTTPConnectionManager*)UA_calloc(1, sizeof(*manager));
    if(!manager)
        return NULL;
    manager->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    UA_StatusCode res =
        UA_String_copy(&eventSourceName, &manager->cm.eventSource.name);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(manager);
        return NULL;
    }
    manager->cm.eventSource.start =
        (UA_StatusCode (*)(UA_EventSource*))HTTP_eventSourceStart;
    manager->cm.eventSource.stop =
        (void (*)(UA_EventSource*))HTTP_eventSourceStop;
    manager->cm.eventSource.free =
        (UA_StatusCode (*)(UA_EventSource*))HTTP_eventSourceDelete;
    manager->cm.protocol = UA_STRING((char*)(uintptr_t)"http");
    manager->cm.openConnection = HTTP_openConnection;
    manager->cm.sendWithConnection = HTTP_sendWithConnection;
    manager->cm.closeConnection = HTTP_closeConnection;
    return &manager->cm;
}
