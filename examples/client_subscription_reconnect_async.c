/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Robust non-blocking reconnecting subscription client
 * ----------------------------------------------------
 * This example extends client_subscription_loop.c into a client that recovers
 * on its own from every failure mode of a remote server, without ever blocking
 * the thread:
 *
 *  - graceful shutdown and restart of the server,
 *  - abrupt disconnect (crash, cable pull),
 *  - a *hung* server that keeps the TCP connection open but stops answering
 *    (the session stays ACTIVATED, so this is only visible through the
 *    subscriptionInactivityCallback).
 *
 * UA_Client_run_iterate() drives the state machine; the (re)connection is
 * issued from the main loop with UA_Client_connectAsync() /
 * UA_Client_disconnectAsync(). A single connect only tries once, so it is
 * re-issued (throttled) while the channel stays closed. A connection that is
 * stuck (mid-handshake, or hung server) is torn down after a grace period,
 * which brings it back to CLOSED where the reconnect takes over. On every new
 * session the subscription and its monitored item are re-created from the
 * stateCallback. */

#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include <signal.h>

#define ENDPOINT_URL       "opc.tcp://localhost:4840"
#define RECONNECT_INTERVAL (2 * UA_DATETIME_SEC) /* throttle for reconnect attempts */
#define UNHEALTHY_GRACE    (5 * UA_DATETIME_SEC) /* tear down a stuck connection after this */

typedef struct {
    UA_Boolean running;
    UA_Boolean mustReconnect; /* set by the inactivity callback: the server hung */
} AppState;

static AppState app = {true, false};

static void stopHandler(int sign) {
    (void)sign;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "Received Ctrl-C");
    app.running = false;
}

static void
handler_currentTimeChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                           UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "currentTime has changed!");
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime *) value->value.data;
        UA_DateTimeStruct dts = UA_DateTime_toStruct(raw_date);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "date is: %02u-%02u-%04u %02u:%02u:%02u.%03u",
                    dts.day, dts.month, dts.year, dts.hour, dts.min,
                    dts.sec, dts.milliSec);
    }
}

static void
deleteSubscriptionCallback(UA_Client *client, UA_UInt32 subscriptionId,
                           void *subscriptionContext) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Subscription Id %u was deleted", subscriptionId);
}

static void
subscriptionInactivityCallback(UA_Client *client, UA_UInt32 subId, void *subContext) {
    /* The server no longer sends PublishResponses while the session is still
     * ACTIVATED: it is hung. This is the only signal for that case, so force a
     * reconnection cycle. */
    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Inactivity for subscription %u -> reconnect", subId);
    app.mustReconnect = true;
}

static void
monCallback(UA_Client *client, void *userdata,
            UA_UInt32 requestId, UA_CreateMonitoredItemsResponse *r) {
    if(0 < r->resultsSize && r->results[0].statusCode == UA_STATUSCODE_GOOD)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Monitoring SERVER_SERVERSTATUS_CURRENTTIME, id %u",
                    r->results[0].monitoredItemId);
}

static void
createSubscriptionCallback(UA_Client *client, void *userdata,
                           UA_UInt32 requestId, UA_CreateSubscriptionResponse *r) {
    if(r->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "Create subscription failed, serviceResult %s",
                    UA_StatusCode_name(r->responseHeader.serviceResult));
        return;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                "Create subscription succeeded, id %u", r->subscriptionId);

    /* Add a MonitoredItem on the current server time */
    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    UA_CreateMonitoredItemsRequest req;
    UA_CreateMonitoredItemsRequest_init(&req);
    req.subscriptionId = r->subscriptionId;
    req.itemsToCreate = &monRequest;
    req.itemsToCreateSize = 1;

    UA_Client_DataChangeNotificationCallback cb[1] = {handler_currentTimeChanged};
    UA_StatusCode retval =
        UA_Client_MonitoredItems_createDataChanges_async(client, req, NULL, cb, NULL,
                                                          monCallback, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "createDataChanges_async failed: %s", UA_StatusCode_name(retval));
}

static void
stateCallback(UA_Client *client, UA_SecureChannelState channelState,
              UA_SessionState sessionState, UA_StatusCode recoveryStatus) {
    if(channelState == UA_SECURECHANNELSTATE_OPEN)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "A SecureChannel to the server is open");
    if(channelState == UA_SECURECHANNELSTATE_CLOSED)
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION, "The client is disconnected");

    /* A new session was activated (initial connect or after a reconnect):
     * (re-)create the subscription. */
    if(sessionState == UA_SESSIONSTATE_ACTIVATED) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                    "A session with the server is activated");
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        UA_StatusCode retval =
            UA_Client_Subscriptions_create_async(client, request, NULL, NULL,
                                                 deleteSubscriptionCallback,
                                                 createSubscriptionCallback, NULL, NULL);
        if(retval != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                        "Subscriptions_create_async failed: %s", UA_StatusCode_name(retval));
    }
}

int main(void) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);
    cc->stateCallback = stateCallback;
    cc->subscriptionInactivityCallback = subscriptionInactivityCallback;
    /* Bound the connect/service latency so reconnection stays snappy. */
    cc->timeout = 2000;

    UA_DateTime lastConnect = 0;
    UA_DateTime unhealthySince = 0;

    while(app.running) {
        /* Drive the client state machine without blocking. */
        UA_Client_run_iterate(client, 100);

        UA_SecureChannelState channelState;
        UA_SessionState sessionState;
        UA_Client_getState(client, &channelState, &sessionState, NULL);

        /* Healthy: session activated and no inactivity reported -> nothing to do. */
        if(sessionState == UA_SESSIONSTATE_ACTIVATED && !app.mustReconnect) {
            unhealthySince = 0;
            continue;
        }

        UA_DateTime now = UA_DateTime_nowMonotonic();
        if(unhealthySince == 0)
            unhealthySince = now;

        if(channelState == UA_SECURECHANNELSTATE_CLOSED) {
            /* Closed -> (re)connect, throttled. */
            if(now - lastConnect > RECONNECT_INTERVAL) {
                lastConnect = now;
                UA_Client_connectAsync(client, ENDPOINT_URL);
            }
        } else if(app.mustReconnect ||
                  (channelState != UA_SECURECHANNELSTATE_CLOSING &&
                   now - unhealthySince > UNHEALTHY_GRACE)) {
            /* Hung server (session still ACTIVATED but no publishes), or a connect
             * stuck past the grace period -> tear the channel down asynchronously.
             * It ends up CLOSED, where the reconnect above takes over. */
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_APPLICATION,
                        "Connection unhealthy -> async teardown");
            UA_Client_disconnectAsync(client);
            app.mustReconnect = false;
            unhealthySince = now;
        }
        /* else: a connect is in progress within the grace period -> keep iterating. */
    }

    /* Clean up */
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
