/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include <check.h>

typedef struct {
    uintptr_t listenerId, acceptedId, clientId;
    UA_UInt16 port;
    UA_Boolean clientEstablished;
    size_t serverMessages, clientMessages;
} TestContext;

static void callback(UA_ConnectionManager *cm, uintptr_t id, void *application,
                     void **connectionContext, UA_ConnectionState state,
                     const UA_KeyValueMap *params, UA_ByteString msg) {
    (void)cm; (void)connectionContext;
    TestContext *ctx = (TestContext*)application;
    const UA_UInt16 *port = (const UA_UInt16*)UA_KeyValueMap_getScalar(
        params, UA_QUALIFIEDNAME(0, "listen-port"), &UA_TYPES[UA_TYPES_UINT16]);
    if(port) {
        ctx->listenerId = id;
        ctx->port = *port;
        return;
    }
    if(state == UA_CONNECTIONSTATE_ESTABLISHED && id != ctx->clientId &&
       ctx->clientId != 0 && ctx->acceptedId == 0)
        ctx->acceptedId = id;
    if(state == UA_CONNECTIONSTATE_ESTABLISHED && id == ctx->clientId)
        ctx->clientEstablished = true;
    if(msg.length) {
        if(id == ctx->clientId)
            ctx->clientMessages++;
        else
            ctx->serverMessages++;
    }
}

static void run(UA_EventLoop *el, UA_Boolean *done) {
    for(size_t i = 0; i < 200 && !*done; i++)
        el->run(el, 50);
    ck_assert(*done);
}

START_TEST(clientServerBinary) {
    TestContext ctx = {0};
    UA_ConnectionManager *ws =
        UA_ConnectionManager_new_LWS_WebSocket(UA_STRING("ws"));
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    ck_assert_ptr_nonnull(ws); ck_assert_ptr_nonnull(el);
    ck_assert_uint_eq(el->registerEventSource(el, &ws->eventSource), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);

    UA_UInt16 port = 0;
    UA_Boolean listen = true;
    UA_KeyValuePair lp[2];
    lp[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&lp[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    lp[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&lp[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap lpm = {2, lp};
    ck_assert_uint_eq(ws->openConnection(ws, &lpm, &ctx, &ctx, callback), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(ctx.port, 0);

    UA_String address = UA_STRING("127.0.0.1");
    UA_KeyValuePair cp[2];
    cp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&cp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    cp[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&cp[1].value, &ctx.port, &UA_TYPES[UA_TYPES_UINT16]);
    UA_KeyValueMap cpm = {2, cp};
    ck_assert_uint_eq(ws->openConnection(ws, &cpm, &ctx, &ctx, callback), UA_STATUSCODE_GOOD);
    ctx.clientId = ctx.listenerId + 1;
    UA_Boolean connected = false;
    for(size_t i = 0; i < 200 && !connected; i++) {
        el->run(el, 50);
        connected = ctx.acceptedId != 0 && ctx.clientEstablished;
    }
    ck_assert(connected);

    UA_ByteString msg = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(ws->allocNetworkBuffer(ws, ctx.clientId, &msg, 4), UA_STATUSCODE_GOOD);
    memcpy(msg.data, "ping", 4);
    ck_assert_uint_eq(ws->sendWithConnection(ws, ctx.clientId, &UA_KEYVALUEMAP_NULL, &msg), UA_STATUSCODE_GOOD);
    UA_Boolean gotServer = false;
    for(size_t i = 0; i < 200 && !gotServer; i++) { el->run(el, 50); gotServer = ctx.serverMessages == 1; }
    ck_assert(gotServer);

    ck_assert_uint_eq(ws->allocNetworkBuffer(ws, ctx.acceptedId, &msg, 4), UA_STATUSCODE_GOOD);
    memcpy(msg.data, "pong", 4);
    ck_assert_uint_eq(ws->sendWithConnection(ws, ctx.acceptedId, &UA_KEYVALUEMAP_NULL, &msg), UA_STATUSCODE_GOOD);
    UA_Boolean gotClient = false;
    for(size_t i = 0; i < 200 && !gotClient; i++) { el->run(el, 50); gotClient = ctx.clientMessages == 1; }
    ck_assert(gotClient);

    el->stop(el);
    UA_Boolean stopped = false;
    for(size_t i = 0; i < 200 && !stopped; i++) { el->run(el, 50); stopped = el->state == UA_EVENTLOOPSTATE_STOPPED; }
    ck_assert(stopped);
    el->free(el);
}
END_TEST

int main(void) {
    Suite *s = suite_create("LWS WebSocket ConnectionManager");
    TCase *tc = tcase_create("integration");
    tcase_add_test(tc, clientServerBinary); suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s); srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL); int failed = srunner_ntests_failed(sr);
    srunner_free(sr); return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
