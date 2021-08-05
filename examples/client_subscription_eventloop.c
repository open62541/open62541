/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Client disconnect handling
 * --------------------------
 * This example shows you how to handle a client disconnect, e.g., if the server
 * is shut down while the client is connected. You just need to call connect
 * again and the client will automatically reconnect.
 *
 * This example is very similar to the tutorial_client_firststeps.c. */

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include <ua_util_internal.h>

#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received Ctrl-C");
    running = 0;
}

typedef struct {
    UA_Boolean isInitial;
    UA_ConnectionManager *cm;
    UA_Client *client;
} UA_BasicConnectionContext;

typedef struct {
    UA_BasicConnectionContext base;
    uintptr_t connectionId;
    UA_Connection connection;
} UA_ConnectionContext;


static void
handler_currentTimeChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                           UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "currentTime has changed!");
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *) value->value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                    dts.day, dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
    }
}

static void
deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId, void *subscriptionContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Subscription Id %u was deleted", subscriptionId);
}

static void
subscriptionInactivityCallback (UA_Client *client, UA_UInt32 subId, void *subContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Inactivity for subscription %u", subId);
}

static void
stateCallback(UA_Client *client, UA_SecureChannelState channelState,
              UA_SessionState sessionState, UA_StatusCode recoveryStatus) {
    switch(channelState) {
    case UA_SECURECHANNELSTATE_FRESH:
    case UA_SECURECHANNELSTATE_CLOSED:
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "The client is disconnected");
        break;
    case UA_SECURECHANNELSTATE_HEL_SENT:
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Waiting for ack");
        break;
    case UA_SECURECHANNELSTATE_OPN_SENT:
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Waiting for OPN Response");
        break;
    case UA_SECURECHANNELSTATE_OPEN:
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "A SecureChannel to the server is open");
        break;
    default:
        break;
    }

    switch(sessionState) {
    case UA_SESSIONSTATE_ACTIVATED: {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "A session with the server is activated");
        /* A new session was created. We need to create the subscription. */
        /* Create a subscription */
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        UA_CreateSubscriptionResponse response =
            UA_Client_Subscriptions_create(client, request, NULL, NULL, deleteSubscriptionCallback);
            if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "Create subscription succeeded, id %u",
                            response.subscriptionId);
            else
                return;

            /* Add a MonitoredItem */
            UA_NodeId currentTimeNode =
                UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
            UA_MonitoredItemCreateRequest monRequest =
                UA_MonitoredItemCreateRequest_default(currentTimeNode);

            UA_MonitoredItemCreateResult monResponse =
                UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                          UA_TIMESTAMPSTORETURN_BOTH, monRequest,
                                                          NULL, handler_currentTimeChanged, NULL);
            if(monResponse.statusCode == UA_STATUSCODE_GOOD)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "Monitoring UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME', id %u",
                            monResponse.monitoredItemId);
        }
        break;
    case UA_SESSIONSTATE_CLOSED:
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Session disconnected");
        break;
    default:
        break;
    }
}
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
        newCtx->base.client = ctx->client;
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

    // UA_ConnectionContext *conCtx = (UA_ConnectionContext *) *connectionContext;

    // if (msg.length > 0) {
    //     // (ctx->server, &conCtx->connection, &msg);
    // }
}


static void
UA_Client_setupEventLoop(UA_Client *client) {

    UA_BasicConnectionContext *ctx = (UA_BasicConnectionContext*) UA_malloc(sizeof(UA_ConnectionContext));
    memset(ctx, 0, sizeof(UA_BasicConnectionContext));

    ctx->client = client;
    ctx->isInitial = true;

    UA_ConnectionManager *cm = UA_ConnectionManager_TCP_new(UA_STRING("tcpCM"));
    cm->connectionCallback = connectionCallback;
    cm->initialConnectionContext = ctx;

    ctx->cm = cm;
    UA_EventLoop_registerEventSource(UA_Client_getConfig(client)->eventLoop, (UA_EventSource *) cm);

    UA_Client_getConfig(client)->cm = cm;
}



int
main(void) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);
    UA_Client_setupEventLoop(client);

    UA_StatusCode rv = UA_EventLoop_start(cc->eventLoop);
    UA_CHECK_STATUS(rv, goto cleanup);

    /* Set stateCallback */
    cc->stateCallback = stateCallback;
    cc->subscriptionInactivityCallback = subscriptionInactivityCallback;

    /* Endless loop runAsync */
    while(running) {
        /* if already connected, this will return GOOD and do nothing */
        /* if the connection is closed/errored, the connection will be reset and then reconnected */
        /* Alternatively you can also use UA_Client_getState to get the current state */
        // UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        UA_StatusCode retval = UA_Client_connect(client, "localhost:4840");
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Not connected. Retrying to connect in 1 second");
            /* The connect may timeout after 1 second (see above) or it may fail immediately on network errors */
            /* E.g. name resolution errors or unreachable network. Thus there should be a small sleep here */
            UA_sleep_ms(1000);
            continue;
        }

        UA_EventLoop_run(cc->eventLoop, 1000);

        // UA_Client_run_iterate(client, 1000);
    };

cleanup:
    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
