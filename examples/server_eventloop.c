/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <server/ua_server_internal.h>
#include <ua_util_internal.h>

#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}
typedef struct {
    UA_Boolean isInitial;
    UA_ConnectionManager *cm;
    UA_Server *server;
} UA_BasicConnectionContext;

typedef struct {
    UA_BasicConnectionContext base;
    uintptr_t connectionId;
    UA_Connection connection;
} UA_ConnectionContext;

/* In this example, we integrate the server into an external "mainloop". This
   can be for example the event-loop used in GUI toolkits, such as Qt or GTK. */

// UA_Connection connection;

static UA_StatusCode UA_Connection_getSendBuffer(UA_Connection *connection, size_t length,
                               UA_ByteString *buf) {
    UA_ConnectionContext *ctx = (UA_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicConnectionContext *)ctx)->cm;
    return cm->allocNetworkBuffer(cm, ctx->connectionId, buf, length);
}

static UA_StatusCode UA_Connection_send(UA_Connection *connection, UA_ByteString *buf) {
    UA_ConnectionContext *ctx = (UA_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicConnectionContext *)ctx)->cm;
    return cm->sendWithConnection(cm, ctx->connectionId, buf);
}

static
void UA_Connection_releaseBuffer (UA_Connection *connection, UA_ByteString *buf) {
    UA_ConnectionContext *ctx = (UA_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicConnectionContext *)ctx)->cm;
    cm->freeNetworkBuffer(cm, ctx->connectionId, buf);
}

static void UA_Connection_close(UA_Connection *connection) {
    UA_ConnectionContext *ctx = (UA_ConnectionContext *) connection->handle;
    UA_ConnectionManager *cm = ((UA_BasicConnectionContext *)ctx)->cm;
    cm->closeConnection(cm, ctx->connectionId);
}


// /* Release the send buffer manually */
// void UA_Connection_releaseSendBuffer(UA_Connection *connection, UA_ByteString *buf) {
//
// }

static void
connectionCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                   void **connectionContext, UA_StatusCode stat,
                   UA_ByteString msg) {

    UA_LOG_DEBUG(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_SERVER,
                 "connection callback for id: %lu", connectionId);

    UA_BasicConnectionContext *ctx = (UA_BasicConnectionContext *) *connectionContext;

    if (stat != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_EventLoop_getLogger(cm->eventSource.eventLoop), UA_LOGCATEGORY_SERVER, "closing connection");

        if (!ctx->isInitial) {
            free(*connectionContext);
        }
        return;
    }

    if (ctx->isInitial) {
        UA_ConnectionContext *newCtx = (UA_ConnectionContext*) calloc(1, sizeof(UA_ConnectionContext));
        newCtx->base.isInitial = false;
        newCtx->base.cm = ctx->cm;
        newCtx->base.server = ctx->server;
        newCtx->connectionId = connectionId;
        newCtx->connection.close = UA_Connection_close;
        newCtx->connection.free = NULL;
        newCtx->connection.getSendBuffer = UA_Connection_getSendBuffer;
        newCtx->connection.recv = NULL;
        newCtx->connection.releaseRecvBuffer = UA_Connection_releaseBuffer;
        newCtx->connection.releaseSendBuffer = UA_Connection_releaseBuffer;
        newCtx->connection.send = UA_Connection_send;
        newCtx->connection.state = UA_CONNECTIONSTATE_CLOSED;

        newCtx->connection.handle = newCtx;

        *connectionContext = newCtx;
    }

    UA_ConnectionContext *conCtx = (UA_ConnectionContext *) *connectionContext;

    if (msg.length > 0) {
        UA_Server_processBinaryMessage(ctx->server, &conCtx->connection, &msg);
    }
}

static void
UA_Server_setupEventLoop(UA_Server *server) {

    UA_BasicConnectionContext *ctx = (UA_BasicConnectionContext*) UA_malloc(sizeof(UA_ConnectionContext));
    memset(ctx, 0, sizeof(UA_BasicConnectionContext));

    ctx->server = server;
    ctx->isInitial = true;

    UA_UInt16 port = 4840;
    UA_Variant portVar;
    UA_Variant_setScalar(&portVar, &port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_ConnectionManager *cm = UA_ConnectionManager_TCP_new(UA_STRING("tcpCM"));
    ctx->cm = cm;
    cm->connectionCallback = connectionCallback;
    cm->initialConnectionContext = ctx;
    UA_ConfigParameter_setParameter(&cm->eventSource.parameters, "listen-port", &portVar);

    UA_EventLoop_registerEventSource(UA_Server_getConfig(server)->eventLoop, (UA_EventSource *) cm);
}


int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_Server_setupEventLoop(server);

    /* Should the server networklayer block (with a timeout) until a message
       arrives or should it return immediately? */
    // UA_Boolean waitInternal = false;

    UA_StatusCode rv = UA_EventLoop_start(server->config.eventLoop);
    UA_CHECK_STATUS(rv, goto cleanup);

    rv = UA_Server_run_startup(server);
    if(rv != UA_STATUSCODE_GOOD)
        goto cleanup;

    while(running) {
        /* timeout is the maximum possible delay (in millisec) until the next
           _iterate call. Otherwise, the server might miss an internal timeout
           or cannot react to messages with the promised responsiveness. */
        /* If multicast discovery server is enabled, the timeout does not not consider new input data (requests) on the mDNS socket.
         * It will be handled on the next call, which may be too late for requesting clients.
         * if needed, the select with timeout on the multicast socket server->mdnsSocket (see example in mdnsd library)
         */

        UA_EventLoop_run(server->config.eventLoop, 1000000);
    }
    rv = UA_Server_run_shutdown(server);

 cleanup:
    UA_Server_delete(server);
    return rv == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
