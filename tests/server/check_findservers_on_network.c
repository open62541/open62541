/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <check.h>
#include <pthread.h>

#define LARGE_DISCOVERY_CACHE_SIZE 50000

typedef struct {
    UA_Server *server;
    UA_StatusCode serviceResult;
    size_t serversSize;
    UA_UInt32 firstRecordId;
    UA_UInt32 lastRecordId;
} FindServersOnNetworkContext;

static void *
findServersOnNetworkSmallStack(void *data) {
    FindServersOnNetworkContext *ctx = (FindServersOnNetworkContext*)data;
    UA_FindServersOnNetworkRequest request;
    UA_FindServersOnNetworkRequest_init(&request);
    UA_FindServersOnNetworkResponse response;
    UA_FindServersOnNetworkResponse_init(&response);

    UA_LOCK(&ctx->server->serviceMutex);
    Service_FindServersOnNetwork(ctx->server, &ctx->server->adminSession,
                                 &request, &response);
    UA_UNLOCK(&ctx->server->serviceMutex);

    ctx->serviceResult = response.responseHeader.serviceResult;
    ctx->serversSize = response.serversSize;
    if(response.serversSize > 0) {
        ctx->firstRecordId = response.servers[0].recordId;
        ctx->lastRecordId = response.servers[response.serversSize - 1].recordId;
    }
    UA_FindServersOnNetworkResponse_clear(&response);
    return NULL;
}

START_TEST(find_on_network_large_cache_small_stack) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_ne(server, NULL);
    server->config.serversOnNetworkEnabled = true;
    server->serversOnNetwork = (UA_ServerOnNetwork*)
        UA_calloc(LARGE_DISCOVERY_CACHE_SIZE, sizeof(UA_ServerOnNetwork));
    ck_assert_ptr_ne(server->serversOnNetwork, NULL);
    server->serversOnNetworkSize = LARGE_DISCOVERY_CACHE_SIZE;

    for(UA_UInt32 i = 0; i < LARGE_DISCOVERY_CACHE_SIZE; i++) {
        UA_ServerOnNetwork_init(&server->serversOnNetwork[i]);
        server->serversOnNetwork[i].recordId = i + 1;
    }

    FindServersOnNetworkContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.server = server;

    pthread_attr_t attr;
    ck_assert_int_eq(pthread_attr_init(&attr), 0);
    ck_assert_int_eq(pthread_attr_setstacksize(&attr, 128 * 1024), 0);
    pthread_t worker;
    ck_assert_int_eq(pthread_create(&worker, &attr,
                                    findServersOnNetworkSmallStack, &ctx), 0);
    ck_assert_int_eq(pthread_attr_destroy(&attr), 0);
    ck_assert_int_eq(pthread_join(worker, NULL), 0);

    ck_assert_uint_eq(ctx.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ctx.serversSize, LARGE_DISCOVERY_CACHE_SIZE);
    ck_assert_uint_eq(ctx.firstRecordId, 1);
    ck_assert_uint_eq(ctx.lastRecordId, LARGE_DISCOVERY_CACHE_SIZE);

    UA_Server_delete(server);
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("FindServersOnNetwork");
    TCase *tc = tcase_create("large cache");
    tcase_add_test(tc, find_on_network_large_cache_small_stack);
    suite_add_tcase(suite, tc);
    return suite;
}

int
main(void) {
    Suite *suite = testSuite();
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
