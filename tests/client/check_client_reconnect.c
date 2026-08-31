/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Regression test for the asynchronous reconnecting subscription client
 * (examples/client_subscription_reconnect_async.c). It drives the exact same
 * non-blocking loop - connectAsync (throttled) while the channel is closed,
 * disconnectAsync when the server is hung (subscription inactivity) or a
 * connect is stuck past a grace period, and re-create the subscription from the
 * stateCallback on every activated session - and asserts that the client
 * recovers on its own from:
 *   1. a server shutdown + restart (the connection is closed), and
 *   2. a hung/frozen server (TCP stays open, no responses) that is later
 *      resumed - only detectable through the subscriptionInactivityCallback. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "test_helpers.h"

#include <stdlib.h>

#include "check.h"
#include "testing_clock.h"

#define RECONNECT_INTERVAL 500  /* ms: throttle for reconnect attempts */
#define UNHEALTHY_GRACE    3000 /* ms: tear down a stuck connect after this */

static UA_Server *server = NULL;
static UA_Boolean serverFrozen = false;   /* simulate a hung server: stop servicing it */
static UA_UInt32 notificationCount = 0;
static UA_UInt32 subscriptionCount = 0;   /* number of created subscriptions */
static UA_Boolean mustReconnect = false;  /* set by the inactivity callback (server hung) */
static UA_UInt32 gotRw = 0;               /* incremented when the round-trip value arrives */
static UA_Int32 expectedRw = 999999;      /* value we wait for through the subscription */

static void startServer(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    /* A writable Int32 node "rw" for the write round-trip test. */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 zero = 0;
    UA_Variant_setScalar(&attr.value, &zero, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "rw"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "rw"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
}

static void stopServer(void) {
    if(!server)
        return;
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

static void
dataChangeHandler(UA_Client *client, UA_UInt32 subId, void *subContext,
                  UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    notificationCount++;
    if(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_INT32]) &&
       *(UA_Int32 *)value->value.data == expectedRw)
        gotRw++;
}

static void
inactivityCallback(UA_Client *client, UA_UInt32 subId, void *subContext) {
    mustReconnect = true;
}

static void
monitoredItemCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                      UA_CreateMonitoredItemsResponse *r) {
    (void)client; (void)userdata; (void)requestId; (void)r;
}

static void
createSubscriptionCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                           UA_CreateSubscriptionResponse *r) {
    if(r->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;
    subscriptionCount++;

    UA_MonitoredItemCreateRequest items[2];
    items[0] = UA_MonitoredItemCreateRequest_default(
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    items[1] = UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(1, "rw"));
    UA_CreateMonitoredItemsRequest req;
    UA_CreateMonitoredItemsRequest_init(&req);
    req.subscriptionId = r->subscriptionId;
    req.itemsToCreate = items;
    req.itemsToCreateSize = 2;

    UA_Client_DataChangeNotificationCallback cb[2] = {dataChangeHandler, dataChangeHandler};
    UA_Client_MonitoredItems_createDataChanges_async(client, req, NULL, cb, NULL,
                                                     monitoredItemCallback, NULL, NULL);
}

/* Re-create the subscription on every activated session (initial + reconnects) */
static void
stateCallback(UA_Client *client, UA_SecureChannelState channelState,
              UA_SessionState sessionState, UA_StatusCode recoveryStatus) {
    if(sessionState != UA_SESSIONSTATE_ACTIVATED)
        return;
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedPublishingInterval = 100.0;
    request.requestedMaxKeepAliveCount = 3;
    UA_Client_Subscriptions_create_async(client, request, NULL, NULL, NULL,
                                         createSubscriptionCallback, NULL, NULL);
}

/* State of the reconnection loop, kept across drive() calls (as the local
 * variables of the example's main loop would be). Times are in fake ms. */
typedef struct {
    long now;
    long lastConnect;
    long unhealthySince; /* 0 = healthy */
} DriveState;

/* One-to-one with the non-blocking loop of client_subscription_reconnect_async.c.
 * Server-side iteration is skipped while the server is "frozen" (hung). The fake
 * clock is advanced with UA_fakeSleep, so no real time is spent waiting. */
static void
drive(UA_Client *client, DriveState *s, UA_UInt32 maxIter,
      volatile UA_UInt32 *until, UA_UInt32 target) {
    for(UA_UInt32 i = 0; i < maxIter; i++) {
        if(until && *until >= target)
            break;

        if(server && !serverFrozen)
            UA_Server_run_iterate(server, false);

        UA_SecureChannelState ch = UA_SECURECHANNELSTATE_CLOSED;
        UA_SessionState ss = UA_SESSIONSTATE_CLOSED;
        UA_Client_getState(client, &ch, &ss, NULL);

        if(ss == UA_SESSIONSTATE_ACTIVATED && !mustReconnect) {
            s->unhealthySince = 0;
        } else {
            if(s->unhealthySince == 0)
                s->unhealthySince = s->now;
            if(ch == UA_SECURECHANNELSTATE_CLOSED) {
                if(s->now - s->lastConnect >= RECONNECT_INTERVAL) {
                    s->lastConnect = s->now;
                    UA_Client_connectAsync(client, "opc.tcp://127.0.0.1:4840");
                }
            } else if(mustReconnect ||
                      (ch != UA_SECURECHANNELSTATE_CLOSING &&
                       s->now - s->unhealthySince >= UNHEALTHY_GRACE)) {
                UA_Client_disconnectAsync(client);
                mustReconnect = false;
                s->unhealthySince = s->now;
            }
        }

        UA_Client_run_iterate(client, 1); /* 1ms real: lets the loopback sockets progress */
        UA_fakeSleep(1);                   /* advance the fake clock for the timers */
        s->now += 1;
    }
}

START_TEST(Client_reconnect_full) {
    notificationCount = 0;
    subscriptionCount = 0;
    mustReconnect = false;
    serverFrozen = false;

    startServer();

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->stateCallback = stateCallback;
    cc->subscriptionInactivityCallback = inactivityCallback;
    cc->timeout = 500; /* fail a stuck connect quickly (fake ms) */

    DriveState s = {1000, 0, 0}; /* now, lastConnect (=> immediate first connect), unhealthySince */

    /* 1) Connect, subscribe and receive the first notifications. */
    drive(client, &s, 5000, &notificationCount, 1);
    ck_assert_msg(notificationCount >= 1, "no notification before the outage");
    ck_assert_uint_eq(subscriptionCount, 1);

    /* 2) Server shutdown + restart on the same endpoint. */
    UA_UInt32 before = notificationCount;
    stopServer();
    drive(client, &s, 300, NULL, 0);    /* let the client notice the disconnect */
    startServer();
    drive(client, &s, 6000, &notificationCount, before + 1);
    ck_assert_msg(notificationCount > before,
                  "client did not recover after the server restart");
    ck_assert_msg(subscriptionCount >= 2,
                  "subscription was not re-created after the reconnect");

    /* 3) Hung server: stop servicing it (TCP stays open, no OPC UA responses).
     *    Only the subscriptionInactivityCallback can detect this. */
    UA_UInt32 beforeFreeze = notificationCount;
    UA_UInt32 subsBeforeFreeze = subscriptionCount;
    serverFrozen = true;
    /* Inactivity fires -> mustReconnect -> the drive loop tears the channel down
     * (disconnectAsync) and retries; the retries fail while the server is frozen. */
    drive(client, &s, 2000, NULL, 0);
    serverFrozen = false;                       /* server responds again */
    drive(client, &s, 6000, &notificationCount, beforeFreeze + 1);
    ck_assert_msg(notificationCount > beforeFreeze,
                  "client did not recover after the server hang");
    ck_assert_msg(subscriptionCount > subsBeforeFreeze,
                  "subscription was not re-created after the hang");

    /* 4) Write round-trip after recovery: write a distinct value and verify it
     *    comes back through the subscription - proves the write path works after
     *    a reconnect, not only the read/subscribe path. */
    expectedRw = 4242;
    gotRw = 0;
    UA_Int32 wv = 4242;
    UA_Variant var;
    UA_Variant_setScalar(&var, &wv, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(UA_Client_writeValueAttribute_async(
                          client, UA_NODEID_STRING(1, "rw"), &var, NULL, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    drive(client, &s, 4000, &gotRw, 1);
    ck_assert_msg(gotRw >= 1,
                  "the written value did not come back through the subscription");

    /* Graceful shutdown: pump the disconnect so UA_Client_delete does not block
     * (the close handshake needs the server to be iterated). */
    UA_Client_disconnectAsync(client);
    for(int k = 0; k < 200; k++) {
        UA_SecureChannelState ch = UA_SECURECHANNELSTATE_CLOSED;
        UA_Client_getState(client, &ch, NULL, NULL);
        if(ch == UA_SECURECHANNELSTATE_CLOSED)
            break;
        if(server)
            UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    UA_Client_delete(client);
    stopServer();
}
END_TEST

int main(void) {
    Suite *s = suite_create("Client Reconnect");
    TCase *tc = tcase_create("Reconnect");
    tcase_set_timeout(tc, 30);
    tcase_add_test(tc, Client_reconnect_full);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
