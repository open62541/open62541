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
    UA_UInt16 port;
    UA_ByteString receive;
    size_t receiveSize;
    TAILQ_HEAD(, WSMessage) outgoing;
    UA_DelayedCallback freeCallback;
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
static const struct lws_protocols wsProtocols[] = {
    {"open62541", wsCallback, 0, 0, 0, NULL, 0},
    LWS_PROTOCOL_LIST_TERM
};

static const UA_KeyValueRestriction wsParams[] = {
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("port")}, &UA_TYPES[UA_TYPES_UINT16], true, true, false},
    {{0, UA_STRING_STATIC("listen")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("path")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
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
}

static void destroyConnection(WSConnection *c) {
    if(c->ownsVhost && c->vhost)
        lws_vhost_destroy(c->vhost);
    UA_free(c->address);
    UA_free(c->path);
    UA_ByteString_clear(&c->receive);
    clearMessages(c);
    UA_free(c);
}

static void freeConnection(void *application, void *context) {
    (void)application;
    destroyConnection((WSConnection*)context);
}

static void removeConnection(WSConnection *c) {
    if(c->state == WS_STATE_REMOVED)
        return;
    WSManager *m = c->manager;
    LIST_REMOVE(c, next);
    if(c->state != WS_STATE_CLOSING)
        notify(c, UA_CONNECTIONSTATE_CLOSING, UA_BYTESTRING_NULL, false);
    c->state = WS_STATE_REMOVED;
    c->wsi = NULL;
    c->freeCallback.callback = freeConnection;
    c->freeCallback.context = c;
    m->cm.eventSource.eventLoop->addDelayedCallback(m->cm.eventSource.eventLoop,
                                                     &c->freeCallback);
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
    if(written < 0 || (size_t)written != m->size)
        return -1;
    TAILQ_REMOVE(&c->outgoing, m, next);
    UA_free(m);
    if(!TAILQ_EMPTY(&c->outgoing))
        UA_LWS_requestWritable(c->wsi);
    return 0;
}

static int wsCallback(struct lws *wsi, enum lws_callback_reasons reason,
                      void *user, void *in, size_t len) {
    WSConnection *c = (WSConnection*)lws_get_opaque_user_data(wsi);
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
        if(!lws_frame_is_binary(wsi))
            return -1;
        if(lws_is_first_fragment(wsi)) {
            UA_ByteString_clear(&c->receive);
            c->receiveSize = 0;
        }
        size_t required = c->receiveSize + len +
                          lws_remaining_packet_payload(wsi);
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
    const UA_Boolean *listen = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "listen"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    if((!listen || !*listen) && (!address || !address->length))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    WSManager *m = (WSManager*)cm;
    WSConnection *c = newConnection(m);
    if(!c)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    c->listener = listen && *listen;
    c->address = copyString(address, "");
    if(!c->listener)
        c->path = copyString(path, "/");
    c->port = *port;
    c->application = application;
    c->context = context;
    c->callback = cb;
    if(!c->address || (!c->listener && !c->path))
        goto fail;

    struct lws_context_creation_info vi;
    memset(&vi, 0, sizeof(vi));
    vi.protocols = wsProtocols;
    vi.user = c;
    vi.port = c->listener ? c->port : CONTEXT_PORT_NO_LISTEN;
    vi.iface = c->listener && c->address[0] ? c->address : NULL;
    c->vhost = lws_create_vhost(m->lwsContext, &vi);
    if(!c->vhost)
        goto fail;
    c->ownsVhost = true;

    if(c->listener) {
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
    ci.protocol = "open62541";
    ci.local_protocol_name = "open62541";
    ci.opaque_user_data = c;
    c->wsi = lws_client_connect_via_info(&ci);
    if(!c->wsi)
        goto failVhost;
    notify(c, UA_CONNECTIONSTATE_OPENING, UA_BYTESTRING_NULL, true);
    return UA_STATUSCODE_GOOD;

failVhost:
    lws_vhost_destroy(c->vhost);
    c->vhost = NULL;
    c->ownsVhost = false;
fail:
    LIST_REMOVE(c, next);
    destroyConnection(c);
    return UA_STATUSCODE_BADNOTCONNECTED;
}

static UA_StatusCode allocBuffer(UA_ConnectionManager *cm, uintptr_t id,
                                 UA_ByteString *buf, size_t size) {
    (void)cm; (void)id;
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
    *buf = UA_BYTESTRING_NULL;
    TAILQ_INSERT_TAIL(&c->outgoing, msg, next);
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
    notify(c, UA_CONNECTIONSTATE_CLOSING, UA_BYTESTRING_NULL, false);
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
