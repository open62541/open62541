/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    relies heavily on concepts from libwebsockets minimal examples
 *    Copyright 2019 (c) Matthias Konnerth
 *    Copyright 2019 (c) Michael Derfler
 */

#define UA_INTERNAL

#include <open62541/network_ws.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>
#include "open62541_queue.h"
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

//one of these is created for each client connecting to us
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
} ServerNetworkLayerWS;

static UA_StatusCode
connection_getsendbuffer(UA_Connection *connection, size_t length, UA_ByteString *buf) {
    if(length > connection->config.sendBufferSize)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    return UA_ByteString_allocBuffer(buf, length);
}

static void
connection_releasesendbuffer(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static void
connection_releaserecvbuffer(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

static UA_StatusCode
connection_send(UA_Connection *connection, UA_ByteString *buf) {
    ConnectionUserData *buffer = (ConnectionUserData *)connection->handle;
    if(connection->state == UA_CONNECTION_CLOSED) {
        UA_ByteString_deleteMembers(buf);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    BufferEntry *entry = (BufferEntry *)malloc(sizeof(BufferEntry));
    entry->msg.length = buf->length;
    entry->msg.data = (UA_Byte *)malloc(LWS_PRE + buf->length);
    memcpy(entry->msg.data + LWS_PRE, buf->data, buf->length);
    UA_ByteString_deleteMembers(buf);
    SIMPLEQ_INSERT_TAIL(&buffer->messages, entry, next);
    lws_callback_on_writable(buffer->wsi);
    return UA_STATUSCODE_GOOD;
}

static void
ServerNetworkLayerWS_close(UA_Connection *connection) {
    if(connection->state == UA_CONNECTION_CLOSED)
        return;
    connection->state = UA_CONNECTION_CLOSED;
}

static void
freeConnection(UA_Connection *connection) {
    if(connection->handle) {
        UA_free(connection->handle);
    }
    UA_Connection_deleteMembers(connection);
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
            ServerNetworkLayerWS *layer = (ServerNetworkLayerWS*)lws_context_user(vhd->context);
            UA_Connection *c = (UA_Connection *)malloc(sizeof(UA_Connection));
            ConnectionUserData *buffer =
                (ConnectionUserData *)malloc(sizeof(ConnectionUserData));
            SIMPLEQ_INIT(&buffer->messages);
            buffer->wsi = wsi;
            memset(c, 0, sizeof(UA_Connection));
            c->sockfd = 0;
            c->handle = buffer;
            c->config = layer->config;
            c->send = connection_send;
            c->close = ServerNetworkLayerWS_close;
            c->free = freeConnection;
            c->getSendBuffer = connection_getsendbuffer;
            c->releaseSendBuffer = connection_releasesendbuffer;
            c->releaseRecvBuffer = connection_releaserecvbuffer;
            // stack sets the connection to established
            c->state = UA_CONNECTION_OPENING;
            c->openingDate = UA_DateTime_nowMonotonic();
            pss->connection = c;
            break;

        case LWS_CALLBACK_CLOSED:
            // notify server
            if(!pss->connection->state != UA_CONNECTION_CLOSED) {
                pss->connection->state = UA_CONNECTION_CLOSED;
            }

            layer = (ServerNetworkLayerWS*)lws_context_user(vhd->context);
            if(layer && layer->server)
            {
                UA_Server_removeConnection(layer->server, pss->connection);
            }
            
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            if(!pss->connection)
                break;

            ConnectionUserData *b = (ConnectionUserData *)pss->connection->handle;
            do {

                BufferEntry *entry = SIMPLEQ_FIRST(&b->messages);
                if(!entry)
                    break;

                int m = lws_write(wsi, entry->msg.data + LWS_PRE, entry->msg.length,
                                  LWS_WRITE_BINARY);
                if(m < (int)entry->msg.length) {
                    lwsl_err("ERROR %d writing to ws\n", m);
                    return -1;
                }
                UA_ByteString_deleteMembers(&entry->msg);
                UA_free(entry);
                SIMPLEQ_REMOVE_HEAD(&b->messages, next);
            } while(!lws_send_pipe_choked(wsi));

            // process remaining messages
            if(SIMPLEQ_FIRST(&b->messages)) {
                lws_callback_on_writable(wsi);
            }
            break;

        case LWS_CALLBACK_RECEIVE:
            if(!vhd->context)
                break;
            layer =
                (ServerNetworkLayerWS *)lws_context_user(vhd->context);
            if(!layer->server)
                break;

            UA_ByteString message = {len, (UA_Byte *)in};
            UA_Server_processBinaryMessage(layer->server, pss->connection, &message);
            break;

        default:
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {"http", lws_callback_http_dummy, 0, 0, 0, NULL, 0},
    {"opcua", callback_opcua, sizeof(struct SessionData), 0, 0, NULL, 0},
    {NULL, NULL, 0, 0, 0, NULL, 0}
};

// make the opcua protocol callback the default one
const struct lws_protocol_vhost_options pvo_opt = {NULL, NULL, "default", "1"};
const struct lws_protocol_vhost_options pvo = {NULL, &pvo_opt, "opcua", ""};

static UA_StatusCode
ServerNetworkLayerWS_start(UA_ServerNetworkLayer *nl, const UA_String *customHostname) {
    UA_initialize_architecture_network();

    ServerNetworkLayerWS *layer = (ServerNetworkLayerWS *)nl->handle;

    /* Get the discovery url from the hostname */
    UA_String du = UA_STRING_NULL;
    char discoveryUrlBuffer[256];
    char hostnameBuffer[256];
    if(customHostname->length) {
        du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "ws://%.*s:%d/",
                                        (int)customHostname->length, customHostname->data,
                                        layer->port);
        du.data = (UA_Byte *)discoveryUrlBuffer;
    } else {
        if(UA_gethostname(hostnameBuffer, 255) == 0) {
            du.length = (size_t)UA_snprintf(discoveryUrlBuffer, 255, "ws://%s:%d/",
                                            hostnameBuffer, layer->port);
            du.data = (UA_Byte *)discoveryUrlBuffer;
        } else {
            UA_LOG_ERROR(layer->logger, UA_LOGCATEGORY_NETWORK,
                         "Could not get the hostname");
        }
    }
    UA_String_copy(&du, &nl->discoveryUrl);

    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Websocket network layer listening on %.*s", (int)nl->discoveryUrl.length,
                nl->discoveryUrl.data);

    struct lws_context_creation_info info;
    int logLevel = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
    lws_set_log_level(logLevel, NULL);
    memset(&info, 0, sizeof info);
    info.port = layer->port;
    info.protocols = protocols;
    info.vhost_name = (char *)du.data;
    info.ws_ping_pong_interval = 10;
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    info.pvo = &pvo;
    info.user = layer;

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
    // set timeout to zero to return immediately if nothing to do
    lws_service(layer->context, 0);
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
ServerNetworkLayerWS_deleteMembers(UA_ServerNetworkLayer *nl) {
    UA_free(nl->handle);
    UA_String_deleteMembers(&nl->discoveryUrl);
}

UA_ServerNetworkLayer
UA_ServerNetworkLayerWS(UA_ConnectionConfig config, UA_UInt16 port, UA_Logger *logger) {
    UA_ServerNetworkLayer nl;
    memset(&nl, 0, sizeof(UA_ServerNetworkLayer));
    nl.deleteMembers = ServerNetworkLayerWS_deleteMembers;
    nl.localConnectionConfig = config;
    nl.start = ServerNetworkLayerWS_start;
    nl.listen = ServerNetworkLayerWS_listen;
    nl.stop = ServerNetworkLayerWS_stop;

    ServerNetworkLayerWS *layer =
        (ServerNetworkLayerWS *)UA_calloc(1, sizeof(ServerNetworkLayerWS));
    if(!layer)
        return nl;
    nl.handle = layer;
    layer->logger = logger;
    layer->port = port;
    layer->config = config;
    return nl;
}
