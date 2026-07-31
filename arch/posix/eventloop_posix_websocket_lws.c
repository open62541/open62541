/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include "eventloop_posix_lws.h"

#ifdef UA_ENABLE_LWS

#include "../../deps/open62541_queue.h"

typedef struct WSManager WSManager;
typedef struct WSConnection WSConnection;

typedef struct WSMessage {
    TAILQ_ENTRY(WSMessage) next;
    size_t size;
} WSMessage;

typedef enum {
    WS_STATE_OPENING,
    WS_STATE_ESTABLISHED,
    WS_STATE_CLOSING,
    WS_STATE_REMOVED
} WSState;

struct WSConnection {
    LIST_ENTRY(WSConnection) next;
    WSManager *manager;
    struct lws *wsi;
    struct lws_vhost *vhost;
    uintptr_t id;
    UA_Boolean listener;
    UA_Boolean ownsVhost;
    WSState state;
    char *address;
    char *path;
    char *subprotocol;
    struct lws_protocols protocols[2];
    UA_UInt16 port;
    UA_Boolean useSSL;
    UA_Boolean binaryOnly;
    UA_ByteString receive;
    size_t receiveSize;
    size_t recvMaxMessageSize;
    size_t sendMaxMessageSize;
    size_t sendMaxQueueSize;
    size_t sendQueueSize;
    TAILQ_HEAD(, WSMessage) outgoing;
    UA_DelayedCallback removeCallback;
    void *application;
    void *context;
    UA_ConnectionManager_connectionCallback callback;
};

struct WSManager {
    UA_ConnectionManager cm;
    struct lws_context *lwsContext;
    LIST_HEAD(, WSConnection) connections;
    uintptr_t lastId;
};

static int wsCallback(struct lws*, enum lws_callback_reasons, void*, void*, size_t);
static const UA_KeyValueRestriction wsParams[] = {
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("port")}, &UA_TYPES[UA_TYPES_UINT16], true, true, false},
    {{0, UA_STRING_STATIC("listen")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("path")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("subprotocol")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("binary-only")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("useSSL")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("certificate")}, &UA_TYPES[UA_TYPES_BYTESTRING], false, true, false},
    {{0, UA_STRING_STATIC("private-key")}, &UA_TYPES[UA_TYPES_BYTESTRING], false, true, false},
    {{0, UA_STRING_STATIC("private-key-password")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("ca-certificate")}, &UA_TYPES[UA_TYPES_BYTESTRING], false, true, false},
    {{0, UA_STRING_STATIC("recv-max-message-size")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-max-message-size")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("send-max-queue-size")}, &UA_TYPES[UA_TYPES_UINT32], false, true, false},
    {{0, UA_STRING_STATIC("validate")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false}
};

static char *copyString(const UA_String *s, const char *fallback) {
    size_t len = s ? s->length : strlen(fallback);
    char *out = (char*)UA_malloc(len + 1);
    if(!out)
        return NULL;
    memcpy(out, s ? s->data : (const UA_Byte*)fallback, len);
    out[len] = 0;
    return out;
}

static void notify(WSConnection *c, UA_ConnectionState state, UA_ByteString msg,
                   UA_Boolean metadata) {
    UA_KeyValuePair kv[2];
    size_t size = 0;
    UA_String address = {c->address ? strlen(c->address) : 0,
                         (UA_Byte*)c->address};
    UA_UInt16 port = c->port;
    if(metadata) {
        kv[size].key = UA_QUALIFIEDNAME(
            0, c->listener ? "listen-address" : "remote-address");
        UA_Variant_setScalar(&kv[size++].value, &address,
                             &UA_TYPES[UA_TYPES_STRING]);
        if(c->listener) {
            kv[size].key = UA_QUALIFIEDNAME(0, "listen-port");
            UA_Variant_setScalar(&kv[size++].value, &port,
                                 &UA_TYPES[UA_TYPES_UINT16]);
        }
    }
    UA_KeyValueMap params = {size, kv};
    c->callback(&c->manager->cm, c->id, c->application, &c->context,
                state, &params, msg);
}

static void clearMessages(WSConnection *c) {
    WSMessage *m, *tmp;
    TAILQ_FOREACH_SAFE(m, &c->outgoing, next, tmp) {
        TAILQ_REMOVE(&c->outgoing, m, next);
        UA_free(m);
    }
    c->sendQueueSize = 0;
}

static void destroyConnection(WSConnection *c) {
    if(c->ownsVhost && c->vhost)
        lws_vhost_destroy(c->vhost);
    UA_free(c->address);
    UA_free(c->path);
    UA_free(c->subprotocol);
    UA_ByteString_clear(&c->receive);
    clearMessages(c);
    UA_free(c);
}

static void removeConnectionDelayed(void *application, void *context) {
    (void)application;
    WSConnection *c = (WSConnection*)context;
    notify(c, UA_CONNECTIONSTATE_CLOSING, UA_BYTESTRING_NULL, false);
    destroyConnection(c);
}

static void removeConnection(WSConnection *c) {
    if(c->state == WS_STATE_REMOVED)
        return;
    WSManager *m = c->manager;
    LIST_REMOVE(c, next);
    c->state = WS_STATE_REMOVED;
    c->wsi = NULL;
    c->removeCallback.callback = removeConnectionDelayed;
    c->removeCallback.context = c;
    m->cm.eventSource.eventLoop->addDelayedCallback(m->cm.eventSource.eventLoop,
                                                     &c->removeCallback);
    if(m->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING &&
       LIST_EMPTY(&m->connections))
        m->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
}

static WSConnection *newConnection(WSManager *m) {
    WSConnection *c = (WSConnection*)UA_calloc(1, sizeof(*c));
    if(!c)
        return NULL;
    c->manager = m;
    c->id = ++m->lastId;
    c->state = WS_STATE_OPENING;
    TAILQ_INIT(&c->outgoing);
    LIST_INSERT_HEAD(&m->connections, c, next);
    return c;
}

static int writePending(WSConnection *c) {
    WSMessage *m = TAILQ_FIRST(&c->outgoing);
    if(!m)
        return 0;
    UA_Byte *payload = (UA_Byte*)(m + 1) + LWS_PRE;
    int written = lws_write(c->wsi, payload, m->size, LWS_WRITE_BINARY);
    if(written < 0 || (size_t)written != m->size) {
        UA_LOG_WARNING(c->manager->cm.eventSource.eventLoop->logger,
                       UA_LOGCATEGORY_NETWORK,
                       "WebSocket %lu\t| Could not write the queued frame",
                       (unsigned long)c->id);
        return -1;
    }
    TAILQ_REMOVE(&c->outgoing, m, next);
    c->sendQueueSize -= m->size;
    UA_free(m);
    if(!TAILQ_EMPTY(&c->outgoing))
        UA_LWS_requestWritable(c->wsi);
    return 0;
}

static int wsCallback(struct lws *wsi, enum lws_callback_reasons reason,
                      void *user, void *in, size_t len) {
    WSConnection *c = (WSConnection*)lws_get_opaque_user_data(wsi);
    if(reason == LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION) {
        WSConnection *listener =
            (WSConnection*)lws_get_vhost_user(lws_get_vhost(wsi));
        if(!listener)
            return -1;

        int protocolLength = lws_hdr_total_length(wsi, WSI_TOKEN_PROTOCOL);
        if(listener->subprotocol) {
            if(protocolLength <= 0) {
                UA_LOG_DEBUG(listener->manager->cm.eventSource.eventLoop->logger,
                             UA_LOGCATEGORY_NETWORK,
                             "WebSocket listener %lu\t| Client did not offer "
                             "the required subprotocol",
                             (unsigned long)listener->id);
                return -1;
            }
            char *offered = (char*)UA_malloc((size_t)protocolLength + 1);
            if(!offered)
                return -1;
            int copied = lws_hdr_copy(wsi, offered, protocolLength + 1,
                                      WSI_TOKEN_PROTOCOL);
            UA_Boolean supported = false;
            if(copied == protocolLength) {
                const char *pos = offered;
                size_t expected = strlen(listener->subprotocol);
                while(*pos) {
                    while(*pos == ' ' || *pos == '\t' || *pos == ',')
                        pos++;
                    const char *end = pos;
                    while(*end && *end != ',')
                        end++;
                    const char *trimmed = end;
                    while(trimmed > pos &&
                          (trimmed[-1] == ' ' || trimmed[-1] == '\t'))
                        trimmed--;
                    if((size_t)(trimmed - pos) == expected &&
                       memcmp(pos, listener->subprotocol, expected) == 0) {
                        supported = true;
                        break;
                    }
                    pos = end;
                }
            }
            UA_free(offered);
            if(!supported) {
                UA_LOG_DEBUG(listener->manager->cm.eventSource.eventLoop->logger,
                             UA_LOGCATEGORY_NETWORK,
                             "WebSocket listener %lu\t| Client offered no "
                             "supported subprotocol",
                             (unsigned long)listener->id);
                return -1;
            }
        } else if(protocolLength > 0) {
            UA_LOG_DEBUG(listener->manager->cm.eventSource.eventLoop->logger,
                         UA_LOGCATEGORY_NETWORK,
                         "WebSocket listener %lu\t| Unexpected subprotocol",
                         (unsigned long)listener->id);
            return -1;
        }

        int pathLength = lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI);
        size_t expectedLength = strlen(listener->path);
        if(pathLength < 0 || (size_t)pathLength != expectedLength)
            return -1;
        char *requestedPath = (char*)UA_malloc((size_t)pathLength + 1);
        if(!requestedPath)
            return -1;
        int copied = lws_hdr_copy(wsi, requestedPath, pathLength + 1,
                                  WSI_TOKEN_GET_URI);
        UA_Boolean pathMatches =
            copied == pathLength &&
            memcmp(requestedPath, listener->path, expectedLength) == 0;
        UA_free(requestedPath);
        if(!pathMatches)
            UA_LOG_DEBUG(listener->manager->cm.eventSource.eventLoop->logger,
                         UA_LOGCATEGORY_NETWORK,
                         "WebSocket listener %lu\t| Requested path does not match",
                         (unsigned long)listener->id);
        return pathMatches ? 0 : -1;
    }
    if(reason == LWS_CALLBACK_ESTABLISHED && !c) {
        WSConnection *listener = (WSConnection*)lws_get_vhost_user(lws_get_vhost(wsi));
        if(!listener)
            return -1;
        c = newConnection(listener->manager);
        if(!c)
            return -1;
        c->wsi = wsi;
        c->vhost = listener->vhost;
        char peer[128] = {0};
        lws_get_peer_simple(wsi, peer, sizeof(peer));
        c->address = copyString(NULL, peer);
        c->application = listener->application;
        c->context = listener->context;
        c->callback = listener->callback;
        c->binaryOnly = listener->binaryOnly;
        c->recvMaxMessageSize = listener->recvMaxMessageSize;
        c->sendMaxMessageSize = listener->sendMaxMessageSize;
        c->sendMaxQueueSize = listener->sendMaxQueueSize;
        lws_set_opaque_user_data(wsi, c);
    }
    if(!c)
        return lws_callback_http_dummy(wsi, reason, user, in, len);

    switch(reason) {
    case LWS_CALLBACK_ESTABLISHED:
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        c->wsi = wsi;
        c->state = WS_STATE_ESTABLISHED;
        notify(c, UA_CONNECTIONSTATE_ESTABLISHED, UA_BYTESTRING_NULL, true);
        UA_free(c->path);
        c->path = NULL;
        return 0;
    case LWS_CALLBACK_RECEIVE:
    case LWS_CALLBACK_CLIENT_RECEIVE: {
        if(c->binaryOnly && !lws_frame_is_binary(wsi)) {
            UA_LOG_WARNING(c->manager->cm.eventSource.eventLoop->logger,
                           UA_LOGCATEGORY_NETWORK,
                           "WebSocket %lu\t| Text frame rejected on a "
                           "binary-only connection", (unsigned long)c->id);
            return -1;
        }
        if(lws_is_first_fragment(wsi)) {
            UA_ByteString_clear(&c->receive);
            c->receiveSize = 0;
        }
        size_t remaining = lws_remaining_packet_payload(wsi);
        if(len > SIZE_MAX - c->receiveSize ||
           remaining > SIZE_MAX - c->receiveSize - len)
            return -1;
        size_t required = c->receiveSize + len + remaining;
        if(c->recvMaxMessageSize > 0 && required > c->recvMaxMessageSize) {
            UA_LOG_WARNING(c->manager->cm.eventSource.eventLoop->logger,
                           UA_LOGCATEGORY_NETWORK,
                           "WebSocket %lu\t| Incoming frame exceeds the "
                           "configured limit", (unsigned long)c->id);
            lws_close_reason(wsi, LWS_CLOSE_STATUS_MESSAGE_TOO_LARGE, NULL, 0);
            return -1;
        }
        if(required > c->receive.length) {
            UA_Byte *data = (UA_Byte*)UA_realloc(c->receive.data, required);
            if(!data)
                return -1;
            c->receive.data = data;
            c->receive.length = required;
        }
        memcpy(c->receive.data + c->receiveSize, in, len);
        c->receiveSize += len;
        if(lws_is_final_fragment(wsi) && lws_remaining_packet_payload(wsi) == 0) {
            UA_ByteString message = {c->receiveSize, c->receive.data};
            notify(c, UA_CONNECTIONSTATE_ESTABLISHED, message, false);
            UA_ByteString_clear(&c->receive);
            c->receiveSize = 0;
        }
        return 0;
    }
    case LWS_CALLBACK_SERVER_WRITEABLE:
    case LWS_CALLBACK_CLIENT_WRITEABLE:
        if(c->state == WS_STATE_CLOSING)
            return -1;
        return writePending(c);
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        UA_LOG_WARNING(c->manager->cm.eventSource.eventLoop->logger,
                       UA_LOGCATEGORY_NETWORK,
                       "WebSocket %lu\t| Connection failed%s%s",
                       (unsigned long)c->id, in ? ": " : "",
                       in ? (const char*)in : "");
        removeConnection(c);
        return 0;
    case LWS_CALLBACK_CLOSED:
    case LWS_CALLBACK_CLIENT_CLOSED:
        removeConnection(c);
        return 0;
    default:
        return lws_callback_http_dummy(wsi, reason, user, in, len);
    }
}

static UA_StatusCode start(UA_ConnectionManager *cm) {
    WSManager *m = (WSManager*)cm;
    m->lwsContext = UA_LWS_acquireContext(cm->eventSource.eventLoop);
    if(!m->lwsContext)
        return UA_STATUSCODE_BADINTERNALERROR;
    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode closeConnection(UA_ConnectionManager*, uintptr_t);

static void stop(UA_ConnectionManager *cm) {
    WSManager *m = (WSManager*)cm;
    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;
    WSConnection *c, *tmp;
    LIST_FOREACH_SAFE(c, &m->connections, next, tmp)
        closeConnection(cm, c->id);
    if(LIST_EMPTY(&m->connections))
        cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
}

static UA_StatusCode deleteManager(UA_ConnectionManager *cm) {
    WSManager *m = (WSManager*)cm;
    if(m->lwsContext)
        UA_LWS_releaseContext(cm->eventSource.eventLoop);
    UA_KeyValueMap_clear(&cm->eventSource.params);
    UA_String_clear(&cm->eventSource.name);
    UA_free(m);
    return UA_STATUSCODE_GOOD;
}

static WSConnection *findConnection(WSManager *m, uintptr_t id) {
    WSConnection *c;
    LIST_FOREACH(c, &m->connections, next)
        if(c->id == id)
            return c;
    return NULL;
}

static UA_StatusCode openConnection(UA_ConnectionManager *cm,
                                    const UA_KeyValueMap *params,
                                    void *application, void *context,
                                    UA_ConnectionManager_connectionCallback cb) {
    UA_StatusCode res = UA_KeyValueRestriction_validate(
        cm->eventSource.eventLoop->logger, "WebSocket", wsParams,
        sizeof(wsParams) / sizeof(wsParams[0]), params);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    const UA_Boolean *validate = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "validate"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(validate && *validate)
        return UA_STATUSCODE_GOOD;
    if(!cb || cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_UInt16 *port = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "port"), &UA_TYPES[UA_TYPES_UINT16]);
    const UA_String *address = (const UA_String*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "address"), &UA_TYPES[UA_TYPES_STRING]);
    const UA_String *path = (const UA_String*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "path"), &UA_TYPES[UA_TYPES_STRING]);
    const UA_String *subprotocol = (const UA_String*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "subprotocol"), &UA_TYPES[UA_TYPES_STRING]);
    const UA_Boolean *binaryOnly = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "binary-only"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_Boolean *listen = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "listen"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_Boolean *useSSL = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "useSSL"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_ByteString *certificate = (const UA_ByteString*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "certificate"), &UA_TYPES[UA_TYPES_BYTESTRING]);
    const UA_ByteString *privateKey = (const UA_ByteString*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "private-key"), &UA_TYPES[UA_TYPES_BYTESTRING]);
    const UA_String *keyPassword = (const UA_String*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "private-key-password"), &UA_TYPES[UA_TYPES_STRING]);
    const UA_ByteString *caCertificate =
        (const UA_ByteString*)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, "ca-certificate"),
            &UA_TYPES[UA_TYPES_BYTESTRING]);
    const UA_UInt32 *recvMaxMessageSize = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "recv-max-message-size"), &UA_TYPES[UA_TYPES_UINT32]);
    const UA_UInt32 *sendMaxMessageSize = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "send-max-message-size"), &UA_TYPES[UA_TYPES_UINT32]);
    const UA_UInt32 *sendMaxQueueSize = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "send-max-queue-size"), &UA_TYPES[UA_TYPES_UINT32]);
    if((!listen || !*listen) && (!address || !address->length))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if((path && path->length > 0 && path->data[0] != '/') ||
       (subprotocol && subprotocol->length == 0))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Boolean secure = useSSL && *useSSL;
    if((certificate && !privateKey) || (!certificate && privateKey) ||
       (certificate && (certificate->length > UINT_MAX ||
                        privateKey->length > UINT_MAX)) ||
       (caCertificate && caCertificate->length > UINT_MAX) ||
       (certificate && !secure) || (caCertificate && !secure) ||
       ((listen && *listen) && secure && !certificate) ||
       ((listen && *listen) && caCertificate))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode failureStatus = UA_STATUSCODE_BADNOTCONNECTED;
    WSManager *m = (WSManager*)cm;
    WSConnection *c = newConnection(m);
    if(!c)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    c->listener = listen && *listen;
    c->useSSL = secure;
    c->binaryOnly = !binaryOnly || *binaryOnly;
    c->recvMaxMessageSize = recvMaxMessageSize ? *recvMaxMessageSize : 0;
    c->sendMaxMessageSize = sendMaxMessageSize ? *sendMaxMessageSize : 0;
    c->sendMaxQueueSize = sendMaxQueueSize ? *sendMaxQueueSize : 0;
    c->address = copyString(address, "");
    c->path = copyString(path && path->length > 0 ? path : NULL, "/");
    if(subprotocol)
        c->subprotocol = copyString(subprotocol, "");
    c->port = *port;
    c->application = application;
    c->context = context;
    c->callback = cb;
    if(!c->address || !c->path || (subprotocol && !c->subprotocol)) {
        failureStatus = UA_STATUSCODE_BADOUTOFMEMORY;
        goto fail;
    }

    /* libwebsockets needs a local protocol name for callback dispatch. This
     * fallback is not sent as a WebSocket subprotocol. */
    c->protocols[0].name = c->subprotocol ? c->subprotocol : "websocket";
    c->protocols[0].callback = wsCallback;

    struct lws_context_creation_info vi;
    memset(&vi, 0, sizeof(vi));
    vi.protocols = c->protocols;
    vi.user = c;
    vi.port = c->listener ? c->port : CONTEXT_PORT_NO_LISTEN;
    vi.iface = c->listener && c->address[0] ? c->address : NULL;
    if(c->useSSL)
        vi.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#ifdef LWS_WITH_TLS
    char *password = copyString(keyPassword, "");
    if(keyPassword && !password) {
        failureStatus = UA_STATUSCODE_BADOUTOFMEMORY;
        goto fail;
    }
    if(certificate) {
        if(c->listener) {
            vi.server_ssl_cert_mem = certificate->data;
            vi.server_ssl_cert_mem_len = (unsigned int)certificate->length;
            vi.server_ssl_private_key_mem = privateKey->data;
            vi.server_ssl_private_key_mem_len = (unsigned int)privateKey->length;
            vi.ssl_private_key_password = password;
        } else {
            vi.client_ssl_cert_mem = certificate->data;
            vi.client_ssl_cert_mem_len = (unsigned int)certificate->length;
            vi.client_ssl_key_mem = privateKey->data;
            vi.client_ssl_key_mem_len = (unsigned int)privateKey->length;
            vi.client_ssl_private_key_password = password;
        }
    }
    if(caCertificate) {
        vi.client_ssl_ca_mem = caCertificate->data;
        vi.client_ssl_ca_mem_len = (unsigned int)caCertificate->length;
    }
#else
    char *password = NULL;
    if(c->useSSL || certificate || caCertificate) {
        failureStatus = UA_STATUSCODE_BADNOTSUPPORTED;
        goto fail;
    }
#endif
    c->vhost = lws_create_vhost(m->lwsContext, &vi);
    if(c->vhost && c->useSSL && !c->listener &&
       lws_init_vhost_client_ssl(&vi, c->vhost)) {
        lws_vhost_destroy(c->vhost);
        c->vhost = NULL;
    }
    UA_free(password);
    if(!c->vhost) {
        UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                       "WebSocket %lu\t| Could not create the %s vhost for "
                       "%s:%u", (unsigned long)c->id,
                       c->listener ? "listener" : "client", c->address,
                       (unsigned)c->port);
        failureStatus = UA_STATUSCODE_BADCONNECTIONREJECTED;
        goto fail;
    }
    c->ownsVhost = true;

    if(c->listener) {
        if(c->address[0] == 0) {
            const char *hostname = lws_canonical_hostname(m->lwsContext);
            char *hostnameCopy = copyString(NULL, hostname ? hostname : "");
            if(!hostnameCopy)
                goto failVhost;
            UA_free(c->address);
            c->address = hostnameCopy;
        }
        c->port = (UA_UInt16)lws_get_vhost_listen_port(c->vhost);
        c->state = WS_STATE_ESTABLISHED;
        notify(c, UA_CONNECTIONSTATE_ESTABLISHED, UA_BYTESTRING_NULL, true);
        return UA_STATUSCODE_GOOD;
    }

    struct lws_client_connect_info ci;
    memset(&ci, 0, sizeof(ci));
    ci.context = m->lwsContext;
    ci.vhost = c->vhost;
    ci.address = c->address;
    ci.host = c->address;
    ci.origin = c->address;
    ci.port = c->port;
    ci.path = c->path;
    ci.protocol = c->subprotocol;
    ci.local_protocol_name = c->protocols[0].name;
    ci.opaque_user_data = c;
    if(c->useSSL)
        ci.ssl_connection |= LCCSCF_USE_SSL;
    c->wsi = lws_client_connect_via_info(&ci);
    if(!c->wsi) {
        /* The connection-error callback can run before
         * lws_client_connect_via_info returns. In that case removal and the
         * closing notification have already been scheduled. */
        if(c->state == WS_STATE_REMOVED)
            return UA_STATUSCODE_GOOD;
        UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                       "WebSocket %lu\t| Could not start the connection to %s:%u%s",
                       (unsigned long)c->id, c->address, (unsigned)c->port, c->path);
        goto failVhost;
    }
    notify(c, UA_CONNECTIONSTATE_OPENING, UA_BYTESTRING_NULL, true);
    return UA_STATUSCODE_GOOD;

failVhost:
    lws_vhost_destroy(c->vhost);
    c->vhost = NULL;
    c->ownsVhost = false;
fail:
    LIST_REMOVE(c, next);
    destroyConnection(c);
    return failureStatus;
}

static UA_StatusCode allocBuffer(UA_ConnectionManager *cm, uintptr_t id,
                                 UA_ByteString *buf, size_t size) {
    WSConnection *c = findConnection((WSManager*)cm, id);
    if(c && c->sendMaxMessageSize > 0 && size > c->sendMaxMessageSize)
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(size > SIZE_MAX - sizeof(WSMessage) - LWS_PRE)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    WSMessage *msg = (WSMessage*)UA_malloc(sizeof(WSMessage) + LWS_PRE + size);
    if(!msg)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(msg, 0, sizeof(*msg));
    msg->size = size;
    buf->length = size;
    buf->data = (UA_Byte*)(msg + 1) + LWS_PRE;
    return UA_STATUSCODE_GOOD;
}
static void freeBuffer(UA_ConnectionManager *cm, uintptr_t id, UA_ByteString *buf) {
    (void)cm; (void)id;
    if(buf && buf->data) {
        WSMessage *msg = (WSMessage*)(buf->data - LWS_PRE) - 1;
        UA_free(msg);
        *buf = UA_BYTESTRING_NULL;
    }
}

static UA_StatusCode sendConnection(UA_ConnectionManager *cm, uintptr_t id,
                                    const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)params;
    WSConnection *c = findConnection((WSManager*)cm, id);
    if(!c || c->listener || c->state != WS_STATE_ESTABLISHED || !buf) {
        freeBuffer(cm, id, buf);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }
    WSMessage *msg = (WSMessage*)(buf->data - LWS_PRE) - 1;
    if(buf->length > msg->size) {
        freeBuffer(cm, id, buf);
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    }
    /* The caller can shorten the allocated buffer to the encoded message.
     * Send exactly that length as one WebSocket message. */
    msg->size = buf->length;
    if(c->sendMaxQueueSize > 0 &&
       (msg->size > c->sendMaxQueueSize ||
        c->sendQueueSize > c->sendMaxQueueSize - msg->size)) {
        UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                       "WebSocket %lu\t| Outgoing queue limit exceeded",
                       (unsigned long)id);
        freeBuffer(cm, id, buf);
        return UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    }
    *buf = UA_BYTESTRING_NULL;
    TAILQ_INSERT_TAIL(&c->outgoing, msg, next);
    c->sendQueueSize += msg->size;
    UA_LWS_requestWritable(c->wsi);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode closeConnection(UA_ConnectionManager *cm, uintptr_t id) {
    WSConnection *c = findConnection((WSManager*)cm, id);
    if(!c)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    if(c->state == WS_STATE_CLOSING)
        return UA_STATUSCODE_GOOD;
    c->state = WS_STATE_CLOSING;
    if(c->listener) {
        lws_vhost_destroy(c->vhost);
        c->vhost = NULL;
        c->ownsVhost = false;
        removeConnection(c);
    } else {
        lws_set_timeout(c->wsi, PENDING_TIMEOUT_CLOSE_SEND, LWS_TO_KILL_ASYNC);
        UA_LWS_requestWritable(c->wsi);
    }
    return UA_STATUSCODE_GOOD;
}

UA_ConnectionManager *
UA_ConnectionManager_new_LWS_WebSocket(const UA_String name) {
    static const UA_String protocol = UA_STRING_STATIC("websocket");
    WSManager *m = (WSManager*)UA_calloc(1, sizeof(*m));
    if(!m)
        return NULL;
    m->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    if(UA_String_copy(&name, &m->cm.eventSource.name) != UA_STATUSCODE_GOOD) {
        UA_free(m); return NULL;
    }
    m->cm.eventSource.start = (UA_StatusCode(*)(UA_EventSource*))start;
    m->cm.eventSource.stop = (void(*)(UA_EventSource*))stop;
    m->cm.eventSource.free = (UA_StatusCode(*)(UA_EventSource*))deleteManager;
    m->cm.protocol = protocol;
    m->cm.openConnection = openConnection;
    m->cm.allocNetworkBuffer = allocBuffer;
    m->cm.freeNetworkBuffer = freeBuffer;
    m->cm.sendWithConnection = sendConnection;
    m->cm.closeConnection = closeConnection;
    return &m->cm;
}

#endif
