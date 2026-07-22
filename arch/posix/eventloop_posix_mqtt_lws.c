/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* MQTT ConnectionManager implemented with the libwebsockets MQTT role. */

#include <open62541/plugin/eventloop.h>

#include "eventloop_posix_lws.h"

#ifdef UA_ENABLE_LWS_MQTT

#include "../../deps/open62541_queue.h"
#include <limits.h>

static const UA_KeyValueRestriction mqttLwsConnectionParams[] = {
    {{0, UA_STRING_STATIC("address")}, &UA_TYPES[UA_TYPES_STRING], true, true, false},
    {{0, UA_STRING_STATIC("port")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("keep-alive")}, &UA_TYPES[UA_TYPES_UINT16], false, true, false},
    {{0, UA_STRING_STATIC("username")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("password")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("useSSL")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("certificate")}, &UA_TYPES[UA_TYPES_BYTESTRING], false, true, false},
    {{0, UA_STRING_STATIC("private-key")}, &UA_TYPES[UA_TYPES_BYTESTRING], false, true, false},
    {{0, UA_STRING_STATIC("private-key-password")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("validate")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("subscribe")}, &UA_TYPES[UA_TYPES_BOOLEAN], false, true, false},
    {{0, UA_STRING_STATIC("topic")}, &UA_TYPES[UA_TYPES_STRING], true, true, false}
};

typedef struct MQTTLwsConnectionManager MQTTLwsConnectionManager;
typedef struct MQTTLwsConnection MQTTLwsConnection;

typedef struct MQTTLwsPendingPublish {
    TAILQ_ENTRY(MQTTLwsPendingPublish) next;
    UA_ByteString payload;
} MQTTLwsPendingPublish;

struct MQTTLwsConnection {
    LIST_ENTRY(MQTTLwsConnection) next;
    MQTTLwsConnectionManager *manager;
    uintptr_t connectionId;
    struct lws *wsi;
    struct lws_vhost *vhost;

    char *address;
    char *topic;
    char *username;
    char *password;
    UA_UInt16 port;
    UA_Boolean subscribe;
    UA_Boolean useSSL;
    UA_Boolean established;
    UA_Boolean closing;
    UA_Boolean subscribePending;
    UA_Boolean unsubscribePending;
    UA_Boolean waitingUnsubscribe;
    UA_Boolean removed;
    UA_DelayedCallback freeCallback;

    lws_mqtt_client_connect_param_t connectParams;
    TAILQ_HEAD(, MQTTLwsPendingPublish) pendingPublishes;

    void *application;
    void *context;
    UA_ConnectionManager_connectionCallback callback;
};

struct MQTTLwsConnectionManager {
    UA_ConnectionManager cm;
    struct lws_context *lwsContext;
    LIST_HEAD(, MQTTLwsConnection) connections;
    uintptr_t lastConnectionId;
};

static int
callbackMqtt(struct lws *wsi, enum lws_callback_reasons reason,
             void *user, void *in, size_t len);

static const struct lws_protocols mqttProtocols[] = {
    {"mqtt", callbackMqtt, 0, 0, 0, NULL, 0},
    LWS_PROTOCOL_LIST_TERM
};

static char *
copyCString(const UA_String *src) {
    if(!src)
        return NULL;
    char *dst = (char*)UA_malloc(src->length + 1);
    if(!dst)
        return NULL;
    memcpy(dst, src->data, src->length);
    dst[src->length] = '\0';
    return dst;
}

static void
callbackConnectionState(MQTTLwsConnection *connection,
                        UA_ConnectionState state, UA_ByteString msg) {
    UA_String topic = {strlen(connection->topic), (UA_Byte*)connection->topic};
    UA_KeyValuePair kvp[2];
    kvp[0].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&kvp[0].value, &topic, &UA_TYPES[UA_TYPES_STRING]);
    kvp[1].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&kvp[1].value, &connection->subscribe,
                         &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap params = {2, kvp};
    connection->callback(&connection->manager->cm, connection->connectionId,
                         connection->application, &connection->context,
                         state, &params, msg);
}

static void
clearPendingPublishes(MQTTLwsConnection *connection) {
    MQTTLwsPendingPublish *pending, *tmp;
    TAILQ_FOREACH_SAFE(pending, &connection->pendingPublishes, next, tmp) {
        TAILQ_REMOVE(&connection->pendingPublishes, pending, next);
        UA_ByteString_clear(&pending->payload);
        UA_free(pending);
    }
}

static void
clearConnection(MQTTLwsConnection *connection) {
    if(connection->vhost)
        lws_vhost_destroy(connection->vhost);
    UA_free(connection->address);
    UA_free(connection->topic);
    UA_free(connection->username);
    UA_free(connection->password);
}

static void
freeConnection(void *application, void *context) {
    (void)application;
    MQTTLwsConnection *connection = (MQTTLwsConnection*)context;
    clearConnection(connection);
    UA_free(connection);
}

static void
removeConnection(MQTTLwsConnection *connection) {
    if(connection->removed)
        return;
    connection->removed = true;
    MQTTLwsConnectionManager *manager = connection->manager;
    if(connection->wsi)
        lws_set_opaque_user_data(connection->wsi, NULL);
    LIST_REMOVE(connection, next);
    clearPendingPublishes(connection);

    if(!connection->closing) {
        connection->closing = true;
        callbackConnectionState(connection, UA_CONNECTIONSTATE_CLOSING,
                                UA_BYTESTRING_NULL);
    }
    callbackConnectionState(connection, UA_CONNECTIONSTATE_CLOSED,
                            UA_BYTESTRING_NULL);

    connection->wsi = NULL;
    connection->freeCallback.callback = freeConnection;
    connection->freeCallback.application = NULL;
    connection->freeCallback.context = connection;
    manager->cm.eventSource.eventLoop->addDelayedCallback(
        manager->cm.eventSource.eventLoop, &connection->freeCallback);

    if(manager->cm.eventSource.state == UA_EVENTSOURCESTATE_STOPPING &&
       LIST_EMPTY(&manager->connections))
        manager->cm.eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
}

static int
sendSubscription(MQTTLwsConnection *connection, UA_Boolean unsubscribe) {
    lws_mqtt_topic_elem_t topic;
    memset(&topic, 0, sizeof(topic));
    topic.name = connection->topic;
    topic.qos = QOS0;

    lws_mqtt_subscribe_param_t params;
    memset(&params, 0, sizeof(params));
    params.topic = &topic;
    params.num_topics = 1;
    if(unsubscribe)
        return lws_mqtt_client_send_unsubcribe(connection->wsi, &params);
    return lws_mqtt_client_send_subcribe(connection->wsi, &params);
}

static int
sendPendingPublish(MQTTLwsConnection *connection) {
    MQTTLwsPendingPublish *pending = TAILQ_FIRST(&connection->pendingPublishes);
    if(!pending)
        return 0;

    lws_mqtt_publish_param_t params;
    memset(&params, 0, sizeof(params));
    params.topic = connection->topic;
    params.topic_len = (UA_UInt16)strlen(connection->topic);
    params.payload_len = (UA_UInt32)pending->payload.length;
    params.qos = QOS0;
    int res = lws_mqtt_client_send_publish(connection->wsi, &params,
                                            pending->payload.data,
                                            (UA_UInt32)pending->payload.length,
                                            LWS_MQTT_FINAL_PART);
    if(res)
        return res;

    TAILQ_REMOVE(&connection->pendingPublishes, pending, next);
    UA_ByteString_clear(&pending->payload);
    UA_free(pending);
    if(!TAILQ_EMPTY(&connection->pendingPublishes))
        UA_LWS_requestWritable(connection->wsi);
    return 0;
}

static int
callbackMqtt(struct lws *wsi, enum lws_callback_reasons reason,
             void *user, void *in, size_t len) {
    MQTTLwsConnection *connection = (MQTTLwsConnection*)user;
    if(!connection)
        connection = (MQTTLwsConnection*)lws_get_opaque_user_data(wsi);
    if(!connection)
        return 0;

    UA_EventLoop *el = connection->manager->cm.eventSource.eventLoop;
    switch(reason) {
    case LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED:
        /* LWS migrates an established MQTT client to a logical child WSI.
         * All subsequent MQTT operations must use that child so per-stream
         * acknowledgement and subscription state is updated correctly. */
        connection->wsi = wsi;
        if(connection->useSSL &&
           UA_LWS_verifyPeerCertificate(&connection->manager->cm, wsi) !=
               UA_STATUSCODE_GOOD)
            return -1;
        if(connection->subscribe) {
            connection->subscribePending = true;
            UA_LWS_requestWritable(wsi);
        } else {
            connection->established = true;
            callbackConnectionState(connection, UA_CONNECTIONSTATE_ESTABLISHED,
                                    UA_BYTESTRING_NULL);
        }
        return 0;

    case LWS_CALLBACK_MQTT_SUBSCRIBED:
        if(!connection->established) {
            connection->established = true;
            callbackConnectionState(connection, UA_CONNECTIONSTATE_ESTABLISHED,
                                    UA_BYTESTRING_NULL);
        }
        return 0;

    case LWS_CALLBACK_MQTT_CLIENT_WRITEABLE:
        if(connection->unsubscribePending) {
            connection->unsubscribePending = false;
            if(sendSubscription(connection, true))
                return -1;
            connection->waitingUnsubscribe = true;
            return 0;
        }
        if(connection->closing) {
            if(connection->waitingUnsubscribe)
                return 0;
            return -1;
        }
        if(connection->subscribePending) {
            connection->subscribePending = false;
            if(sendSubscription(connection, false))
                return -1;
        }
        if(sendPendingPublish(connection))
            return -1;
        return 0;

    case LWS_CALLBACK_MQTT_UNSUBSCRIBED:
        connection->waitingUnsubscribe = false;
        return -1;

    case LWS_CALLBACK_MQTT_CLIENT_RX: {
        lws_mqtt_publish_param_t *publish = (lws_mqtt_publish_param_t*)in;
        if(!publish || publish->topic_len != strlen(connection->topic) ||
           memcmp(publish->topic, connection->topic, publish->topic_len) != 0)
            return 0;
        UA_ByteString msg = {len, (UA_Byte*)(uintptr_t)publish->payload};
        callbackConnectionState(connection, UA_CONNECTIONSTATE_ESTABLISHED, msg);
        return 0;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_NETWORK,
                       "MQTT-LWS %u\t| Connection error: %.*s",
                       (unsigned)connection->connectionId, (int)len,
                       in ? (char*)in : "");
        removeConnection(connection);
        return 0;

    case LWS_CALLBACK_MQTT_CLIENT_CLOSED:
    case LWS_CALLBACK_MQTT_DROP_PROTOCOL:
        removeConnection(connection);
        return 0;

    default:
        return 0;
    }
}

static UA_StatusCode
eventSourceStart(UA_ConnectionManager *cm) {
    MQTTLwsConnectionManager *manager = (MQTTLwsConnectionManager*)cm;
    UA_EventLoop *el = cm->eventSource.eventLoop;
    if(!el || cm->eventSource.state != UA_EVENTSOURCESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(!manager->lwsContext) {
        manager->lwsContext = UA_LWS_acquireContext(el);
        if(!manager->lwsContext)
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    cm->eventSource.state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
closeConnection(UA_ConnectionManager *cm, uintptr_t connectionId);

static void
eventSourceStop(UA_ConnectionManager *cm) {
    if(cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPING ||
       cm->eventSource.state == UA_EVENTSOURCESTATE_STOPPED)
        return;

    MQTTLwsConnectionManager *manager = (MQTTLwsConnectionManager*)cm;
    if(LIST_EMPTY(&manager->connections)) {
        cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPED;
        return;
    }

    cm->eventSource.state = UA_EVENTSOURCESTATE_STOPPING;
    MQTTLwsConnection *connection, *tmp;
    LIST_FOREACH_SAFE(connection, &manager->connections, next, tmp)
        closeConnection(cm, connection->connectionId);
}

static UA_StatusCode
eventSourceDelete(UA_ConnectionManager *cm) {
    MQTTLwsConnectionManager *manager = (MQTTLwsConnectionManager*)cm;
    if(manager->lwsContext)
        UA_LWS_releaseContext(cm->eventSource.eventLoop);
    UA_LWS_clearCertificateGroup(cm);
    UA_String_clear(&cm->eventSource.name);
    UA_free(manager);
    return UA_STATUSCODE_GOOD;
}

static MQTTLwsConnection *
findConnection(MQTTLwsConnectionManager *manager, uintptr_t connectionId) {
    MQTTLwsConnection *connection;
    LIST_FOREACH(connection, &manager->connections, next) {
        if(connection->connectionId == connectionId)
            return connection;
    }
    return NULL;
}

static UA_StatusCode
openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
               void *application, void *context,
               UA_ConnectionManager_connectionCallback callback) {
    if(cm->eventSource.state != UA_EVENTSOURCESTATE_STARTED)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode res =
        UA_KeyValueRestriction_validate(cm->eventSource.eventLoop->logger,
                                        "MQTT-LWS", mqttLwsConnectionParams,
                                        sizeof(mqttLwsConnectionParams) /
                                            sizeof(mqttLwsConnectionParams[0]),
                                        params);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    const UA_Boolean *validate = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "validate"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(validate && *validate)
        return UA_STATUSCODE_GOOD;
    if(!callback)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    const UA_String *address = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "address"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    const UA_String *topic = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "topic"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if(!address || address->length == 0 || !topic || topic->length == 0 ||
       topic->length > UINT16_MAX)
        return UA_STATUSCODE_BADCONNECTIONREJECTED;

    MQTTLwsConnectionManager *manager = (MQTTLwsConnectionManager*)cm;
    MQTTLwsConnection *connection =
        (MQTTLwsConnection*)UA_calloc(1, sizeof(MQTTLwsConnection));
    if(!connection)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    connection->manager = manager;
    connection->connectionId = ++manager->lastConnectionId;
    connection->application = application;
    connection->context = context;
    connection->callback = callback;
    TAILQ_INIT(&connection->pendingPublishes);
    connection->address = copyCString(address);
    connection->topic = copyCString(topic);

    const UA_String *username = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "username"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    const UA_String *password = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "password"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    connection->username = copyCString(username);
    connection->password = copyCString(password);
    if(!connection->address || !connection->topic ||
       (username && !connection->username) || (password && !connection->password)) {
        clearConnection(connection);
        UA_free(connection);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "port"),
                                 &UA_TYPES[UA_TYPES_UINT16]);
    connection->port = port ? *port : 1883;
    const UA_Boolean *subscribe = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "subscribe"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    connection->subscribe = subscribe && *subscribe;
    const UA_Boolean *useSSL = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "useSSL"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    connection->useSSL = useSSL && *useSSL;
#ifdef LWS_WITH_MBEDTLS
    if(connection->useSSL && cm->certificateGroup) {
        clearConnection(connection);
        UA_free(connection);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }
#endif
    const UA_UInt16 *keepAlive = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "keep-alive"),
                                 &UA_TYPES[UA_TYPES_UINT16]);

    connection->connectParams.keep_alive = keepAlive ? *keepAlive : 400;
    connection->connectParams.clean_start = 1;
    connection->connectParams.username = connection->username;
    connection->connectParams.password = connection->password;
    connection->connectParams.username_nofree = 1;
    connection->connectParams.password_nofree = 1;

    const UA_ByteString *certificate = (const UA_ByteString*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "certificate"),
                                 &UA_TYPES[UA_TYPES_BYTESTRING]);
    const UA_ByteString *privateKey = (const UA_ByteString*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "private-key"),
                                 &UA_TYPES[UA_TYPES_BYTESTRING]);
    const UA_String *keyPassword = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "private-key-password"),
                                 &UA_TYPES[UA_TYPES_STRING]);
    if((certificate && !privateKey) || (!certificate && privateKey) ||
       (certificate && (certificate->length > UINT_MAX ||
                        privateKey->length > UINT_MAX)) ||
       (certificate && !connection->useSSL)) {
        clearConnection(connection);
        UA_free(connection);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    struct lws_context_creation_info vi;
    memset(&vi, 0, sizeof(vi));
    vi.port = CONTEXT_PORT_NO_LISTEN;
    vi.protocols = mqttProtocols;
    if(connection->useSSL)
        vi.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#ifdef LWS_WITH_TLS
    char *keyPasswordCString = copyCString(keyPassword);
    if(keyPassword && !keyPasswordCString) {
        clearConnection(connection);
        UA_free(connection);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    if(certificate) {
        vi.client_ssl_cert_mem = certificate->data;
        vi.client_ssl_cert_mem_len = (unsigned int)certificate->length;
        vi.client_ssl_key_mem = privateKey->data;
        vi.client_ssl_key_mem_len = (unsigned int)privateKey->length;
        vi.client_ssl_private_key_password = keyPasswordCString;
    }
#else
    char *keyPasswordCString = NULL;
    if(connection->useSSL || certificate) {
        clearConnection(connection);
        UA_free(connection);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }
#endif
    connection->vhost = lws_create_vhost(manager->lwsContext, &vi);
    if(connection->vhost && connection->useSSL &&
       lws_init_vhost_client_ssl(&vi, connection->vhost)) {
        lws_vhost_destroy(connection->vhost);
        connection->vhost = NULL;
    }
    UA_free(keyPasswordCString);
    if(!connection->vhost) {
        clearConnection(connection);
        UA_free(connection);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    struct lws_client_connect_info info;
    memset(&info, 0, sizeof(info));
    info.context = manager->lwsContext;
    info.vhost = connection->vhost;
    info.address = connection->address;
    info.host = connection->address;
    info.port = connection->port;
    info.protocol = "mqtt";
    info.method = "MQTT";
    info.alpn = "mqtt";
    info.mqtt_cp = &connection->connectParams;
    info.userdata = connection;
    info.opaque_user_data = connection;
    if(connection->useSSL) {
        info.ssl_connection |= LCCSCF_USE_SSL;
        if(cm->certificateGroup)
            info.ssl_connection |= LCCSCF_ALLOW_INSECURE |
                                   LCCSCF_ALLOW_SELFSIGNED |
                                   LCCSCF_ALLOW_EXPIRED;
    }

    LIST_INSERT_HEAD(&manager->connections, connection, next);
    struct lws *wsi = lws_client_connect_via_info(&info);
    if(connection->removed)
        return UA_STATUSCODE_BADNOTCONNECTED;
    connection->wsi = wsi;
    if(!wsi) {
        LIST_REMOVE(connection, next);
        clearConnection(connection);
        UA_free(connection);
        return UA_STATUSCODE_BADNOTCONNECTED;
    }

    callbackConnectionState(connection, UA_CONNECTIONSTATE_OPENING,
                            UA_BYTESTRING_NULL);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
allocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                   UA_ByteString *buf, size_t bufSize) {
    (void)cm;
    (void)connectionId;
    return UA_ByteString_allocBuffer(buf, bufSize);
}

static void
freeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                  UA_ByteString *buf) {
    (void)cm;
    (void)connectionId;
    UA_ByteString_clear(buf);
}

static UA_StatusCode
sendWithConnection(UA_ConnectionManager *cm, uintptr_t connectionId,
                   const UA_KeyValueMap *params, UA_ByteString *buf) {
    (void)params;
    MQTTLwsConnectionManager *manager = (MQTTLwsConnectionManager*)cm;
    MQTTLwsConnection *connection = findConnection(manager, connectionId);
    if(!connection || !connection->established || connection->closing || !buf ||
       buf->length > UINT32_MAX) {
        if(buf)
            UA_ByteString_clear(buf);
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }

    MQTTLwsPendingPublish *pending =
        (MQTTLwsPendingPublish*)UA_calloc(1, sizeof(MQTTLwsPendingPublish));
    if(!pending) {
        UA_ByteString_clear(buf);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    pending->payload = *buf;
    *buf = UA_BYTESTRING_NULL;
    TAILQ_INSERT_TAIL(&connection->pendingPublishes, pending, next);
    UA_LWS_requestWritable(connection->wsi);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
closeConnection(UA_ConnectionManager *cm, uintptr_t connectionId) {
    MQTTLwsConnectionManager *manager = (MQTTLwsConnectionManager*)cm;
    MQTTLwsConnection *connection = findConnection(manager, connectionId);
    if(!connection)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(connection->closing)
        return UA_STATUSCODE_GOOD;

    connection->closing = true;
    callbackConnectionState(connection, UA_CONNECTIONSTATE_CLOSING,
                            UA_BYTESTRING_NULL);
    if(connection->subscribe && connection->established) {
        connection->unsubscribePending = true;
        UA_LWS_requestWritable(connection->wsi);
    } else {
        lws_set_timeout(connection->wsi, PENDING_TIMEOUT_CLOSE_SEND,
                        LWS_TO_KILL_ASYNC);
        UA_LWS_requestWritable(connection->wsi);
    }
    return UA_STATUSCODE_GOOD;
}

UA_ConnectionManager *
UA_ConnectionManager_new_LWS_MQTT(const UA_String eventSourceName) {
    static const UA_String mqttProtocol = UA_STRING_STATIC("mqtt");
    MQTTLwsConnectionManager *manager =
        (MQTTLwsConnectionManager*)UA_calloc(1, sizeof(MQTTLwsConnectionManager));
    if(!manager)
        return NULL;

    manager->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    if(UA_String_copy(&eventSourceName, &manager->cm.eventSource.name) !=
       UA_STATUSCODE_GOOD) {
        UA_free(manager);
        return NULL;
    }
    manager->cm.eventSource.start =
        (UA_StatusCode (*)(UA_EventSource*))eventSourceStart;
    manager->cm.eventSource.stop = (void (*)(UA_EventSource*))eventSourceStop;
    manager->cm.eventSource.free =
        (UA_StatusCode (*)(UA_EventSource*))eventSourceDelete;
    manager->cm.protocol = mqttProtocol;
    manager->cm.openConnection = openConnection;
    manager->cm.allocNetworkBuffer = allocNetworkBuffer;
    manager->cm.freeNetworkBuffer = freeNetworkBuffer;
    manager->cm.sendWithConnection = sendWithConnection;
    manager->cm.closeConnection = closeConnection;
    return &manager->cm;
}

#endif /* UA_ENABLE_LWS_MQTT */
