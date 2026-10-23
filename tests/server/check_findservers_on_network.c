/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "test_helpers.h"

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

START_TEST(discovery_runtime_flags) {
    UA_Server *server = UA_Server_new();
    ck_assert_ptr_ne(server, NULL);

    UA_String url = UA_STRING("opc.tcp://localhost:4841");
    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", "Registered server");
    UA_RegisteredServer registered;
    UA_RegisteredServer_init(&registered);
    registered.serverUri = UA_STRING("urn:test:registered-server");
    registered.serverNames = &name;
    registered.serverNamesSize = 1;
    registered.discoveryUrls = &url;
    registered.discoveryUrlsSize = 1;
    registered.isOnline = true;

    ck_assert_uint_eq(UA_Server_registerServer(server, &registered, 0, NULL, NULL),
                      UA_STATUSCODE_BADSERVICEUNSUPPORTED);
    server->config.registeredServersEnabled = true;
    ck_assert_uint_eq(UA_Server_registerServer(server, &registered, 0, NULL, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->registeredServersSize, 1);

    server->config.registeredServersEnabled = false;
    UA_FindServersRequest request;
    UA_FindServersRequest_init(&request);
    UA_FindServersResponse response;
    UA_FindServersResponse_init(&response);
    UA_LOCK(&server->serviceMutex);
    Service_FindServers(server, &server->adminSession, &request, &response);
    UA_UNLOCK(&server->serviceMutex);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.serversSize, 1);
    UA_FindServersResponse_clear(&response);

    FindServersOnNetworkContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.server = server;
    findServersOnNetworkSmallStack(&ctx);
    ck_assert_uint_eq(ctx.serviceResult, UA_STATUSCODE_BADNOTIMPLEMENTED);
    server->config.serversOnNetworkEnabled = true;
    findServersOnNetworkSmallStack(&ctx);
    ck_assert_uint_eq(ctx.serviceResult, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
}
END_TEST

#define REGISTERED_SERVER_URI "urn:test:registered-server"
#define REGISTERED_DISCOVERY_URL "opc.tcp://localhost:4841"
#define REGISTERED_SERVER_NAME "Registered server"
#define REGISTERED_MDNS_NAME "registered-mdns"

static UA_Server *
newRegistrationServer(void) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    server->config.registeredServersEnabled = true;
    server->config.serversOnNetworkEnabled = true;
    server->config.registeredServerCleanupTimeout = 10; /* seconds */
    return server;
}

/* The request points to the given ServerName and to static strings. Without a
 * ServerName, the request has none. */
static UA_RegisteredServer
registeredServerRequest(UA_LocalizedText *serverName) {
    static UA_String url = UA_STRING_STATIC(REGISTERED_DISCOVERY_URL);
    UA_RegisteredServer registered;
    UA_RegisteredServer_init(&registered);
    registered.serverUri = UA_STRING(REGISTERED_SERVER_URI);
    registered.serverNames = serverName;
    registered.serverNamesSize = (serverName) ? 1 : 0;
    registered.discoveryUrls = &url;
    registered.discoveryUrlsSize = 1;
    registered.isOnline = true;
    return registered;
}

/* Adds a MdnsDiscoveryConfiguration (as in RegisterServer2) if an mDNS name is
 * given */
static UA_StatusCode
registerTestServer(UA_Server *server, const UA_RegisteredServer *registered,
                   const UA_String *mdnsServerName) {
    if(!mdnsServerName)
        return UA_Server_registerServer(server, registered, 0, NULL, NULL);

    UA_MdnsDiscoveryConfiguration mdnsConfig;
    UA_MdnsDiscoveryConfiguration_init(&mdnsConfig);
    mdnsConfig.mdnsServerName = *mdnsServerName;
    UA_ExtensionObject discoveryConfig;
    UA_ExtensionObject_setValue(&discoveryConfig, &mdnsConfig,
                                &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION]);
    UA_StatusCode configResult = UA_STATUSCODE_GOOD;
    return UA_Server_registerServer(server, registered, 1,
                                    &discoveryConfig, &configResult);
}

/* Part 4, 7.31: The ServerNames shall have at least one valid entry. An mDNS
 * name does not replace them. */
START_TEST(register_server_without_server_names_rejected) {
    UA_Server *server = newRegistrationServer();
    UA_RegisteredServer registered = registeredServerRequest(NULL);
    UA_String mdnsServerName = UA_STRING(REGISTERED_MDNS_NAME);

    ck_assert_uint_eq(registerTestServer(server, &registered, NULL),
                      UA_STATUSCODE_BADSERVERNAMEMISSING);
    ck_assert_uint_eq(registerTestServer(server, &registered, &mdnsServerName),
                      UA_STATUSCODE_BADSERVERNAMEMISSING);

    ck_assert_uint_eq(server->registeredServersSize, 0);
    ck_assert_uint_eq(server->serversOnNetworkSize, 0);

    UA_Server_delete(server);
}
END_TEST

/* Without an mDNS name, the first ServerName is the mDNS name. If that is empty,
 * no name was specified. */
START_TEST(register_server_with_empty_server_name_rejected) {
    UA_Server *server = newRegistrationServer();
    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", "");
    UA_RegisteredServer registered = registeredServerRequest(&name);
    UA_String emptyMdnsServerName = UA_STRING("");

    ck_assert_uint_eq(registerTestServer(server, &registered, NULL),
                      UA_STATUSCODE_BADSERVERNAMEMISSING);
    ck_assert_uint_eq(registerTestServer(server, &registered, &emptyMdnsServerName),
                      UA_STATUSCODE_BADSERVERNAMEMISSING);

    ck_assert_uint_eq(server->registeredServersSize, 0);
    ck_assert_uint_eq(server->serversOnNetworkSize, 0);

    UA_Server_delete(server);
}
END_TEST

/* Part 4, 7.13.2: If the mDNS name is null (_i == 0) or empty (_i == 1), the
 * first ServerName is used */
START_TEST(register_server_empty_mdns_name_uses_server_name) {
    UA_Server *server = newRegistrationServer();
    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", REGISTERED_SERVER_NAME);
    UA_RegisteredServer registered = registeredServerRequest(&name);
    UA_String mdnsServerName = (_i == 0) ? UA_STRING_NULL : UA_STRING("");

    ck_assert_uint_eq(registerTestServer(server, &registered, &mdnsServerName),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->registeredServersSize, 1);

    UA_Server_delete(server);
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("FindServersOnNetwork");
    TCase *tc = tcase_create("large cache");
    tcase_add_test(tc, find_on_network_large_cache_small_stack);
    tcase_add_test(tc, discovery_runtime_flags);
    suite_add_tcase(suite, tc);
    TCase *tc_names = tcase_create("server names");
    tcase_add_test(tc_names, register_server_without_server_names_rejected);
    tcase_add_test(tc_names, register_server_with_empty_server_name_rejected);
    tcase_add_loop_test(tc_names, register_server_empty_mdns_name_uses_server_name, 0, 2);
    suite_add_tcase(suite, tc_names);
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
