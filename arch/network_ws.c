/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    relies heavily on concepts from libwebsockets minimal examples
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2019 (c) Michael Derfler
 */

#define UA_INTERNAL

#include <open62541/config.h>
#include <open62541/network_ws.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>
#include "open62541_queue.h"
#include "ua_securechannel.h"
#include <libwebsockets.h>
#include <string.h>

struct BufferEntry {
    UA_ByteString msg;
    SIMPLEQ_ENTRY(BufferEntry) next;
};

typedef struct BufferEntry BufferEntry;

struct ConnectionUserData {
    struct lws *wsi;
    SIMPLEQ_HEAD(, BufferEntry) messages;
};

typedef struct ConnectionUserData ConnectionUserData;

// one of these is created for each client connecting to us
struct SessionData {
    UA_Connection *connection;
};

// one of these is created for each vhost our protocol is used with
struct VHostData {
    struct lws_context *context;
};

typedef struct {
    const UA_Logger *logger;
    UA_UInt16 port;
    struct lws_context *context;
    UA_Server *server;
    UA_ConnectionConfig config;
    UA_ByteString certificate;
    UA_ByteString privateKey;
} ServerNetworkLayerWS;

static UA_StatusCode
connection_ws_getsendbuffer(UA_Connection *connection, size_t length, UA_ByteString *buf) {
    UA_SecureChannel *channel = connection->channel;
    if(channel && channel->config.sendBufferSize < length)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    UA_StatusCode retval = UA_ByteString_allocBuffer(buf, LWS_PRE + length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    buf->data += LWS_PRE;
    buf->length -= LWS_PRE;

    return UA_STATUSCODE_GOOD;
}

static void
connection_ws_releasesendbuffer(UA_Connection *connection, UA_ByteString *buf) {
    buf->data -= LWS_PRE;
    buf->length += LWS_PRE;
    UA_ByteString_clear(buf);
}

static void
connection_ws_releaserecvbuffer(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

static UA_StatusCode
connection_send(UA_Connection *connection, UA_ByteString *buf) {
    /*  libwebsockets sends data only once lws_service is called
    and it gets a POLLOUT event. Effectively that means that this send is
    deferred until the next ServerNetworkLayerWS_listen. That may result
    in very poor throughput if there are any other network layers present. */

    ConnectionUserData *buffer = (ConnectionUserData *)connection->handle;
    if(connection->state == UA_CONNECTIONSTATE_CLOSED) {
        connection_ws_releasesendbuffer(connection, buf);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    BufferEntry *entry = (BufferEntry *)malloc(sizeof(BufferEntry));
    entry->msg = *buf;
    SIMPLEQ_INSERT_TAIL(&buffer->messages, entry, next);
    lws_callback_on_writable(buffer->wsi);
    return UA_STATUSCODE_GOOD;
}

static void
ServerNetworkLayerWS_close(UA_Connection *connection) {
    connection->state = UA_CONNECTIONSTATE_CLOSED;
    // trigger callback and close;
    lws_callback_on_writable(((ConnectionUserData *)connection->handle)->wsi);
}

static void
freeConnection(UA_Connection *connection) {
    if(connection->handle) {
        ConnectionUserData *userData = (ConnectionUserData *)connection->handle;
        while(!SIMPLEQ_EMPTY(&userData->messages)) {
            BufferEntry *entry = SIMPLEQ_FIRST(&userData->messages);
            connection_ws_releasesendbuffer(connection, &entry->msg);
            SIMPLEQ_REMOVE_HEAD(&userData->messages, next);
            UA_free(entry);
        }
        UA_free(connection->handle);
    }
    UA_free(connection);
}

static int
callback_opcua(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in,
               size_t len) {
    struct SessionData *pss = (struct SessionData *)user;
    struct VHostData *vhd =
        (struct VHostData *)lws_protocol_vh_priv_get(lws_get_vhost(wsi),
                                                                   lws_get_protocol(wsi));

    switch(reason) {
        case LWS_CALLBACK_PROTOCOL_INIT:
            vhd = (struct VHostData *)lws_protocol_vh_priv_zalloc(
                lws_get_vhost(wsi), lws_get_protocol(wsi),
                sizeof(struct VHostData));
            vhd->context = lws_get_context(wsi);
            break;

        case LWS_CALLBACK_ESTABLISHED:
            if(!wsi)
                break;
            ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)lws_context_user(vhd->context);
            UA_Connection *c = (UA_Connection *)malloc(sizeof(UA_Connection));
            ConnectionUserData *buffer =
                (ConnectionUserData *)malloc(sizeof(ConnectionUserData));
            SIMPLEQ_INIT(&buffer->messages);
            buffer->wsi = wsi;
            memset(c, 0, sizeof(UA_Connection));
            c->sockfd = UA_INVALID_SOCKET;
            c->handle = buffer;
            c->send = connection_send;
            c->close = ServerNetworkLayerWS_close;
            c->free = freeConnection;
            c->getSendBuffer = connection_ws_getsendbuffer;
            c->releaseSendBuffer = connection_ws_releasesendbuffer;
            c->releaseRecvBuffer = connection_ws_releaserecvbuffer;
            // stack sets the connection to established
            c->state = UA_CONNECTIONSTATE_OPENING;
            pss->connection = c;
            break;

        case LWS_CALLBACK_CLOSED:
            // notify server
            if(pss->connection->state != UA_CONNECTIONSTATE_CLOSED) {
                pss->connection->state = UA_CONNECTIONSTATE_CLOSED;
            }

            layer = (ServerNetworkLayerWS *)lws_context_user(vhd->context);
            if(layer && layer->server) {
                UA_Server_removeConnection(layer->server, pss->connection);
            }

            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            if(!pss->connection)
                break;

            ConnectionUserData *b = (ConnectionUserData *)pss->connection->handle;
            do {
                if(!pss->connection ||
                   pss->connection->state == UA_CONNECTIONSTATE_CLOSED) {
                    /*
                    connetion is closed signal it to lws:
                    lws documentation says:
                    When you want to close a connection,
                    you do it by returning -1 from a callback for that connection.
                    */
                    return -1;
                }

                BufferEntry *entry = SIMPLEQ_FIRST(&b->messages);
                if(!entry)
                    break;

                int m = lws_write(wsi, entry->msg.data, entry->msg.length,
                                  LWS_WRITE_BINARY);
                if(m < (int)entry->msg.length) {
                    lwsl_err("ERROR %d writing to ws\n", m);
                    return -1;
                }
                connection_ws_releasesendbuffer(pss->connection, &entry->msg);
                SIMPLEQ_REMOVE_HEAD(&b->messages, next);
                UA_free(entry);
            } while(!lws_send_pipe_choked(wsi));

            // process remaining messages
            if(SIMPLEQ_FIRST(&b->messages)) {
                lws_callback_on_writable(wsi);
            }
            break;

        case LWS_CALLBACK_RECEIVE: {
            if(!vhd->context)
                break;
            layer = (ServerNetworkLayerWS *)lws_context_user(vhd->context);
            if(!layer->server)
                break;

            UA_ByteString message = {len, (UA_Byte *)in};
            UA_Server_processBinaryMessage(layer->server, pss->connection, &message);
            break;
        }

        default:
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {"http", lws_callback_http_dummy, 0, 0, 0, NULL, 0},
    /* default protocol */
    {"opcua", callback_opcua, sizeof(struct SessionData), 0, 0, NULL, 0},
    /* defined protocols: https://reference.opcfoundation.org/v104/Core/docs/Part6/7.5.2/ */
    {"opcua+uacp", callback_opcua, sizeof(struct SessionData), 0, 0, NULL, 0},
    // {"opcua+json", callback_opcua, sizeof(struct SessionData), 0, 0, NULL, 0}, // <-- enable when json coding is fully supported
    {NULL, NULL, 0, 0, 0, NULL, 0}};

// make the opcua protocol callback the default one
const struct lws_protocol_vhost_options pvo_opt = {NULL, NULL, "default", "1"};
const struct lws_protocol_vhost_options pvo = {NULL, &pvo_opt, "opcua", ""};

static UA_StatusCode
ServerNetworkLayerWS_start(UA_ServerNetworkLayer *nl, const UA_Logger *logger,
                           const UA_String *customHostname) {
    UA_initialize_architecture_network();

    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;
    layer->logger = logger;

    UA_Boolean isSecure = layer->certificate.length && layer->privateKey.length;

    UA_String protocol = isSecure ? UA_STRING("wss") : UA_STRING("ws");

    /* Get the discovery url from the hostname */
    UA_String du = UA_STRING_NULL;
    char discoveryUrlBuffer[256];
    if(customHostname->length) {
        du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "%.*s://%.*s:%d/",
                                        (int)protocol.length, protocol.data,
                                        (int)customHostname->length, customHostname->data,
                                        layer->port);
        du.data = (UA_Byte *)discoveryUrlBuffer;
    } else {
        char hostnameBuffer[256];
        if(UA_gethostname(hostnameBuffer, 255) == 0) {
            du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "%.*s://%s:%d/",
                                            (int)protocol.length, protocol.data,
                                            hostnameBuffer, layer->port);
            du.data = (UA_Byte *)discoveryUrlBuffer;
        } else {
            UA_LOG_ERROR(layer->logger, UA_LOGCATEGORY_NETWORK,
                         "Could not get the hostname");
        }
    }
    // we need discoveryUrl.data as a null-terminated string for vhost_name
    nl->discoveryUrl.data = (UA_Byte *)UA_malloc(du.length + 1);
    strncpy((char *)nl->discoveryUrl.data, discoveryUrlBuffer, du.length);
    nl->discoveryUrl.data[du.length] = '\0';
    nl->discoveryUrl.length = du.length;

    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Websocket network layer listening on %.*s", (int)nl->discoveryUrl.length,
                nl->discoveryUrl.data);

    struct lws_context_creation_info info;
    int logLevel = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
    lws_set_log_level(logLevel, NULL);
    memset(&info, 0, sizeof info);
    info.port = layer->port;
    info.protocols = protocols;
    info.vhost_name = (char *)nl->discoveryUrl.data;
    info.ws_ping_pong_interval = 10;
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    info.pvo = &pvo;
    info.user = layer;

    if(isSecure) {
        info.server_ssl_cert_mem = layer->certificate.data;
        info.server_ssl_cert_mem_len = (unsigned int)layer->certificate.length;
        info.server_ssl_private_key_mem = layer->privateKey.data;
        info.server_ssl_private_key_mem_len = (unsigned int)layer->privateKey.length;

        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
          UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Websocket network layer listening using WSS");

    }

    struct lws_context *context = lws_create_context(&info);
    if(!context) {
        UA_LOG_ERROR(layer->logger, UA_LOGCATEGORY_NETWORK, "lws init failed");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    layer->context = context;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ServerNetworkLayerWS_listen(UA_ServerNetworkLayer *nl, UA_Server *server,
                            UA_UInt16 timeout) {
    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;
    layer->server = server;
    /*  N.B.: lws_service documentation says:
            "Since v3.2 internally the timeout wait is ignored, the lws scheduler
             is smart enough to stay asleep until an event is queued." */
    lws_service(layer->context, timeout);
    return UA_STATUSCODE_GOOD;
}

static void
ServerNetworkLayerWS_stop(UA_ServerNetworkLayer *nl, UA_Server *server) {
    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;
    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Shutting down the WS network layer");
    lws_context_destroy(layer->context);
    UA_deinitialize_architecture_network();
}

static void
ServerNetworkLayerWS_clear(UA_ServerNetworkLayer *nl) {
    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;

    if(layer->certificate.length) {
        UA_String_clear(&layer->certificate);
    }

    if(layer->privateKey.length) {
        UA_String_clear(&layer->privateKey);
    }

    UA_free(nl->handle);
    UA_String_clear(&nl->discoveryUrl);
}

UA_ServerNetworkLayer
UA_ServerNetworkLayerWS(UA_ConnectionConfig config, UA_UInt16 port,
                        const UA_ByteString* certificate,
                        const UA_ByteString* privateKey) {
    UA_ServerNetworkLayer nl;
    memset(&nl, 0, sizeof(UA_ServerNetworkLayer));
    nl.clear = ServerNetworkLayerWS_clear;
    nl.localConnectionConfig = config;
    nl.start = ServerNetworkLayerWS_start;
    nl.listen = ServerNetworkLayerWS_listen;
    nl.stop = ServerNetworkLayerWS_stop;

    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)
        UA_calloc(1, sizeof(ServerNetworkLayerWS));
    if(!layer)
        return nl;
    nl.handle = layer;
    layer->port = port;
    layer->config = config;

    if(certificate && privateKey) {
        UA_String_copy(certificate,&layer->certificate);
        UA_String_copy(privateKey,&layer->privateKey);
    }
    return nl;
}
