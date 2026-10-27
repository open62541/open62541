/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "test_helpers.h"
#include "testing_clock.h"

#include <check.h>
#include <pthread.h>
#include <stdio.h>

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

static size_t
countServersOnNetwork(UA_Server *server, const char *serverName) {
    UA_String name = UA_STRING((char*)(uintptr_t)serverName);
    size_t count = 0;
    for(size_t i = 0; i < server->serversOnNetworkSize; i++) {
        if(UA_String_equal(&server->serversOnNetwork[i].serverName, &name))
            count++;
    }
    return count;
}

#define REGISTERED_SERVER_URI "urn:test:registered-server"
#define REGISTERED_DISCOVERY_URL "opc.tcp://localhost:4841"
#define REGISTERED_SERVER_NAME "Registered server"
#define REGISTERED_MDNS_NAME "registered-mdns"
#define REGISTERED_SEMAPHORE_PATH "/tmp/open62541-unit-test-semaphore-son"

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

/* Adds the MdnsDiscoveryConfiguration (as in RegisterServer2) if it is given */
static UA_StatusCode
registerWithMdnsConfig(UA_Server *server, const UA_RegisteredServer *registered,
                       UA_MdnsDiscoveryConfiguration *mdnsConfig) {
    if(!mdnsConfig)
        return UA_Server_registerServer(server, registered, 0, NULL, NULL);

    UA_ExtensionObject discoveryConfig;
    UA_ExtensionObject_setValue(&discoveryConfig, mdnsConfig,
                                &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION]);
    UA_StatusCode configResult = UA_STATUSCODE_GOOD;
    return UA_Server_registerServer(server, registered, 1,
                                    &discoveryConfig, &configResult);
}

/* Adds a MdnsDiscoveryConfiguration (as in RegisterServer2) if an mDNS name is
 * given */
static UA_StatusCode
registerTestServer(UA_Server *server, const UA_RegisteredServer *registered,
                   const UA_String *mdnsServerName) {
    if(!mdnsServerName)
        return registerWithMdnsConfig(server, registered, NULL);

    UA_MdnsDiscoveryConfiguration mdnsConfig;
    UA_MdnsDiscoveryConfiguration_init(&mdnsConfig);
    mdnsConfig.mdnsServerName = *mdnsServerName;
    return registerWithMdnsConfig(server, registered, &mdnsConfig);
}

/* The mDNS name differs from the ServerName. The removal paths only have the
 * stored record and must still find the entry. */
static void
registerWithMdnsName(UA_Server *server, char *semaphoreFilePath) {
    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", REGISTERED_SERVER_NAME);
    UA_RegisteredServer registered = registeredServerRequest(&name);
    registered.semaphoreFilePath = UA_STRING(semaphoreFilePath);
    UA_String mdnsServerName = UA_STRING(REGISTERED_MDNS_NAME);

    ck_assert_uint_eq(registerTestServer(server, &registered, &mdnsServerName),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->registeredServersSize, 1);
    ck_assert_uint_eq(countServersOnNetwork(server, REGISTERED_MDNS_NAME), 1);
}

/* The purge of the server housekeeping */
static void
cleanupRegistrations(UA_Server *server) {
    UA_LOCK(&server->serviceMutex);
    cleanupRegisteredServers(server);
    UA_UNLOCK(&server->serviceMutex);
}

START_TEST(registered_server_timeout_removes_server_on_network) {
    UA_Server *server = newRegistrationServer();
    registerWithMdnsName(server, NULL);

    /* Age the registration beyond the cleanup timeout */
    UA_fakeSleep(11 * 1000);
    cleanupRegistrations(server);

    ck_assert_uint_eq(server->registeredServersSize, 0);
    ck_assert_uint_eq(countServersOnNetwork(server, REGISTERED_MDNS_NAME), 0);

    UA_Server_delete(server);
}
END_TEST

#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
START_TEST(registered_server_semaphore_removes_server_on_network) {
    FILE *semaphore = fopen(REGISTERED_SEMAPHORE_PATH, "w");
    ck_assert_ptr_ne(semaphore, NULL);
    fclose(semaphore);

    UA_Server *server = newRegistrationServer();
    registerWithMdnsName(server, REGISTERED_SEMAPHORE_PATH);

    /* The registration stays while the semaphore file exists */
    cleanupRegistrations(server);
    ck_assert_uint_eq(server->registeredServersSize, 1);
    ck_assert_uint_eq(countServersOnNetwork(server, REGISTERED_MDNS_NAME), 1);

    /* Deleting the semaphore file removes the registration */
    ck_assert_int_eq(remove(REGISTERED_SEMAPHORE_PATH), 0);
    cleanupRegistrations(server);

    ck_assert_uint_eq(server->registeredServersSize, 0);
    ck_assert_uint_eq(countServersOnNetwork(server, REGISTERED_MDNS_NAME), 0);

    UA_Server_delete(server);
}
END_TEST
#endif

START_TEST(deregister_server_removes_server_on_network) {
    UA_Server *server = newRegistrationServer();
    registerWithMdnsName(server, NULL);

    UA_String url = UA_STRING(REGISTERED_DISCOVERY_URL);
    ck_assert_uint_eq(UA_Server_deregisterServer(server,
                                                 UA_STRING(REGISTERED_SERVER_URI),
                                                 1, &url),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->registeredServersSize, 0);
    ck_assert_uint_eq(countServersOnNetwork(server, REGISTERED_MDNS_NAME), 0);

    UA_Server_delete(server);
}
END_TEST

/* Registering again keeps a single entry. Under a different mDNS name, the
 * entry of the previous name is removed. */
START_TEST(reregister_server_replaces_server_on_network) {
    UA_Server *server = newRegistrationServer();
    registerWithMdnsName(server, NULL);
    registerWithMdnsName(server, NULL);

    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", REGISTERED_SERVER_NAME);
    UA_RegisteredServer registered = registeredServerRequest(&name);
    UA_String renamed = UA_STRING("renamed-mdns");
    ck_assert_uint_eq(registerTestServer(server, &registered, &renamed),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->registeredServersSize, 1);
    ck_assert_uint_eq(server->serversOnNetworkSize, 1);
    ck_assert_uint_eq(countServersOnNetwork(server, REGISTERED_MDNS_NAME), 0);
    ck_assert_uint_eq(countServersOnNetwork(server, "renamed-mdns"), 1);

    UA_Server_delete(server);
}
END_TEST

/* Without capabilities, the entry gets "NA" (no capability information is
 * available, Part 12, Annex D) as in the mDNS announcement. Given capabilities
 * are kept. _i == 0: no MdnsDiscoveryConfiguration, _i == 1: a configuration
 * without capabilities, _i == 2: a configuration with two capabilities. */
START_TEST(register_server_capabilities_default_to_na) {
    UA_Server *server = newRegistrationServer();
    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", REGISTERED_SERVER_NAME);
    UA_RegisteredServer registered = registeredServerRequest(&name);

    UA_String caps[2] = {UA_STRING("DA"), UA_STRING("HD")};
    UA_String na = UA_STRING("NA");
    UA_MdnsDiscoveryConfiguration mdnsConfig;
    UA_MdnsDiscoveryConfiguration_init(&mdnsConfig);
    if(_i == 2) {
        mdnsConfig.serverCapabilities = caps;
        mdnsConfig.serverCapabilitiesSize = 2;
    }

    ck_assert_uint_eq(registerWithMdnsConfig(server, &registered,
                                             (_i == 0) ? NULL : &mdnsConfig),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->serversOnNetworkSize, 1);
    const UA_ServerOnNetwork *son = &server->serversOnNetwork[0];
    UA_String serverName = UA_STRING(REGISTERED_SERVER_NAME);
    ck_assert(UA_String_equal(&son->serverName, &serverName));

    const UA_String *expected = (_i == 2) ? caps : &na;
    size_t expectedSize = (_i == 2) ? 2 : 1;
    ck_assert_uint_eq(son->serverCapabilitiesSize, expectedSize);
    for(size_t i = 0; i < expectedSize; i++)
        ck_assert(UA_String_equal(&son->serverCapabilities[i], &expected[i]));

    UA_Server_delete(server);
}
END_TEST

/* Servers register again periodically. An unchanged entry keeps its RecordId.
 * A changed entry is updated and all entries get new RecordIds. */
START_TEST(reregister_unchanged_server_keeps_record_ids) {
    UA_Server *server = newRegistrationServer();
    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", REGISTERED_SERVER_NAME);
    UA_RegisteredServer registered = registeredServerRequest(&name);
    UA_MdnsDiscoveryConfiguration mdnsConfig;
    UA_MdnsDiscoveryConfiguration_init(&mdnsConfig);
    mdnsConfig.mdnsServerName = UA_STRING(REGISTERED_MDNS_NAME);

    ck_assert_uint_eq(registerWithMdnsConfig(server, &registered, &mdnsConfig),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->serversOnNetworkSize, 1);
    UA_UInt32 recordId = server->serversOnNetwork[0].recordId;

    ck_assert_uint_eq(registerWithMdnsConfig(server, &registered, &mdnsConfig),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->serversOnNetworkSize, 1);
    ck_assert_uint_eq(server->serversOnNetwork[0].recordId, recordId);

    UA_String cap = UA_STRING("DA");
    mdnsConfig.serverCapabilities = &cap;
    mdnsConfig.serverCapabilitiesSize = 1;
    ck_assert_uint_eq(registerWithMdnsConfig(server, &registered, &mdnsConfig),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->serversOnNetworkSize, 1);
    ck_assert_uint_eq(server->serversOnNetwork[0].serverCapabilitiesSize, 1);
    ck_assert(UA_String_equal(&server->serversOnNetwork[0].serverCapabilities[0],
                              &cap));
    ck_assert_uint_ne(server->serversOnNetwork[0].recordId, recordId);

    UA_Server_delete(server);
}
END_TEST

#define REGISTERED_HTTPS_URL "opc.https://localhost:4843"

/* The entries are announced as _opcua-tcp, which is for opc.tcp urls only
 * (Part 12, C.3). _i == 0: the opc.tcp url after another url is used,
 * _i == 1: a registration without an opc.tcp url gets no entry. */
START_TEST(register_server_uses_tcp_discovery_url) {
    UA_Server *server = newRegistrationServer();
    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", REGISTERED_SERVER_NAME);
    UA_RegisteredServer registered = registeredServerRequest(&name);
    UA_String urls[2] = {UA_STRING(REGISTERED_HTTPS_URL),
                         UA_STRING(REGISTERED_DISCOVERY_URL)};
    registered.discoveryUrls = urls;
    registered.discoveryUrlsSize = (_i == 0) ? 2 : 1;

    ck_assert_uint_eq(registerTestServer(server, &registered, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->registeredServersSize, 1);

    if(_i == 0) {
        ck_assert_uint_eq(server->serversOnNetworkSize, 1);
        ck_assert(UA_String_equal(&server->serversOnNetwork[0].discoveryUrl,
                                  &urls[1]));
    } else {
        ck_assert_uint_eq(server->serversOnNetworkSize, 0);
    }

    UA_Server_delete(server);
}
END_TEST

/* The registration is updated (it has the https url in common) and no longer
 * has an opc.tcp url */
START_TEST(reregister_server_without_tcp_url_removes_server_on_network) {
    UA_Server *server = newRegistrationServer();
    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", REGISTERED_SERVER_NAME);
    UA_RegisteredServer registered = registeredServerRequest(&name);
    UA_String urls[2] = {UA_STRING(REGISTERED_HTTPS_URL),
                         UA_STRING(REGISTERED_DISCOVERY_URL)};
    registered.discoveryUrls = urls;
    registered.discoveryUrlsSize = 2;
    ck_assert_uint_eq(registerTestServer(server, &registered, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->serversOnNetworkSize, 1);

    registered.discoveryUrlsSize = 1;
    ck_assert_uint_eq(registerTestServer(server, &registered, NULL),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->registeredServersSize, 1);
    ck_assert_uint_eq(server->serversOnNetworkSize, 0);

    UA_Server_delete(server);
}
END_TEST

/* A registration without an opc.tcp url has no entry. Its removal must not
 * remove an entry of the same name from another source (e.g. mDNS). */
START_TEST(deregister_server_without_tcp_url_keeps_other_entry) {
    UA_Server *server = newRegistrationServer();
    UA_ServerOnNetwork other;
    UA_ServerOnNetwork_init(&other);
    other.serverName = UA_STRING(REGISTERED_SERVER_NAME);
    other.discoveryUrl = UA_STRING("opc.tcp://otherhost:4840");
    ck_assert_uint_eq(UA_Server_registerServerOnNetwork(server, &other,
                                                        UA_KEYVALUEMAP_NULL),
                      UA_STATUSCODE_GOOD);

    UA_LocalizedText name = UA_LOCALIZEDTEXT("en", REGISTERED_SERVER_NAME);
    UA_RegisteredServer registered = registeredServerRequest(&name);
    UA_String url = UA_STRING(REGISTERED_HTTPS_URL);
    registered.discoveryUrls = &url;
    ck_assert_uint_eq(registerTestServer(server, &registered, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_deregisterServer(server,
                                                 UA_STRING(REGISTERED_SERVER_URI),
                                                 1, &url),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->registeredServersSize, 0);
    ck_assert_uint_eq(server->serversOnNetworkSize, 1);
    ck_assert(UA_String_equal(&server->serversOnNetwork[0].discoveryUrl,
                              &other.discoveryUrl));

    UA_Server_delete(server);
}
END_TEST

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
    ck_assert_uint_eq(server->serversOnNetworkSize, 1);
    ck_assert_uint_eq(countServersOnNetwork(server, REGISTERED_SERVER_NAME), 1);

    UA_Server_delete(server);
}
END_TEST

/* Notifications about ServerOnNetwork entries and the last 0:announce flag */
static size_t sonNotifications;
static UA_Boolean lastAnnounce;

static void
announceNotificationCallback(UA_Server *server,
                             UA_ApplicationNotificationType type,
                             const UA_KeyValueMap payload) {
    (void)server;
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_DISCOVERY_SERVERONNETWORK)
        return;
    const UA_Boolean *announce = (const UA_Boolean*)
        UA_KeyValueMap_getScalar(&payload, UA_QUALIFIEDNAME(0, "announce"),
                                 &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_ptr_ne(announce, NULL);
    lastAnnounce = *announce;
    sonNotifications++;
}

/* The entry of a registered server is created in either case. It is announced
 * only with announceRegisteredServers. */
START_TEST(register_server_announce_follows_config) {
    UA_Boolean announceRegistered = (_i == 1);
    UA_Server *server = newRegistrationServer();
    server->config.announceRegisteredServers = announceRegistered;
    server->config.discoveryNotificationCallback = announceNotificationCallback;
    sonNotifications = 0;

    registerWithMdnsName(server, NULL);
    ck_assert_uint_eq(sonNotifications, 1);
    ck_assert(lastAnnounce == announceRegistered);

    UA_Server_delete(server);
}
END_TEST

/* Entries from the public API are announced only on request. Received entries
 * are added without the parameter. */
START_TEST(register_server_on_network_announce_param) {
    UA_Server *server = newRegistrationServer();
    server->config.discoveryNotificationCallback = announceNotificationCallback;
    sonNotifications = 0;

    UA_String caps = UA_STRING("NA");
    UA_ServerOnNetwork son;
    UA_ServerOnNetwork_init(&son);
    son.serverName = UA_STRING("local-entry");
    son.discoveryUrl = UA_STRING(REGISTERED_DISCOVERY_URL);
    son.serverCapabilitiesSize = 1;
    son.serverCapabilities = &caps;

    ck_assert_uint_eq(UA_Server_registerServerOnNetwork(server, &son,
                                                        UA_KEYVALUEMAP_NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sonNotifications, 1);
    ck_assert(!lastAnnounce);

    UA_Boolean announce = true;
    UA_KeyValuePair param;
    param.key = UA_QUALIFIEDNAME(0, "announce");
    UA_Variant_setScalar(&param.value, &announce, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_KeyValueMap params = {1, &param};
    ck_assert_uint_eq(UA_Server_registerServerOnNetwork(server, &son, params),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sonNotifications, 2);
    ck_assert(lastAnnounce);

    UA_Server_delete(server);
}
END_TEST

/* The entry with the mDNS name of the registration as for registerWithMdnsName.
 * For example received over mDNS. */
static UA_ServerOnNetwork
registeredServerOnNetwork(UA_String *caps) {
    UA_ServerOnNetwork son;
    UA_ServerOnNetwork_init(&son);
    son.serverName = UA_STRING(REGISTERED_MDNS_NAME);
    son.discoveryUrl = UA_STRING(REGISTERED_DISCOVERY_URL);
    son.serverCapabilitiesSize = 1;
    son.serverCapabilities = caps;
    return son;
}

/* Received information does not change or remove the entry of a registered
 * server. That is possible again after the deregistration. */
START_TEST(public_api_keeps_registered_server_on_network) {
    UA_Server *server = newRegistrationServer();
    registerWithMdnsName(server, NULL);

    UA_String caps = UA_STRING("DA");
    UA_ServerOnNetwork son = registeredServerOnNetwork(&caps);
    son.discoveryUrl = UA_STRING("opc.tcp://other-host:4841");
    ck_assert_uint_eq(UA_Server_registerServerOnNetwork(server, &son,
                                                        UA_KEYVALUEMAP_NULL),
                      UA_STATUSCODE_BADNOTWRITABLE);
    ck_assert_uint_eq(UA_Server_deregisterServerOnNetwork(server, son.serverName),
                      UA_STATUSCODE_BADNODELETERIGHTS);
    ck_assert_uint_eq(server->serversOnNetworkSize, 1);
    UA_String url = UA_STRING(REGISTERED_DISCOVERY_URL);
    ck_assert(UA_String_equal(&server->serversOnNetwork[0].discoveryUrl, &url));

    ck_assert_uint_eq(UA_Server_deregisterServer(server,
                                                 UA_STRING(REGISTERED_SERVER_URI),
                                                 1, &url),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_registerServerOnNetwork(server, &son,
                                                        UA_KEYVALUEMAP_NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_deregisterServerOnNetwork(server, son.serverName),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->serversOnNetworkSize, 0);

    UA_Server_delete(server);
}
END_TEST

/* The registered server announced itself over mDNS before it registered. The
 * received entry is identical. The registration must replace it nevertheless,
 * so that the entry gets announced. */
START_TEST(register_server_replaces_identical_received_entry) {
    UA_Server *server = newRegistrationServer();
    server->config.announceRegisteredServers = true;
    server->config.discoveryNotificationCallback = announceNotificationCallback;

    UA_String caps = UA_STRING("NA");
    UA_ServerOnNetwork son = registeredServerOnNetwork(&caps);
    ck_assert_uint_eq(UA_Server_registerServerOnNetwork(server, &son,
                                                        UA_KEYVALUEMAP_NULL),
                      UA_STATUSCODE_GOOD);

    sonNotifications = 0;
    registerWithMdnsName(server, NULL);
    ck_assert_uint_eq(sonNotifications, 1);
    ck_assert(lastAnnounce);

    UA_Server_delete(server);
}
END_TEST

/* A changed announceRegisteredServers applies with the next registration also
 * if the registration is unchanged. The default config announces. */
START_TEST(reregister_server_applies_changed_announce_config) {
    UA_Server *server = newRegistrationServer();
    server->config.discoveryNotificationCallback = announceNotificationCallback;
    sonNotifications = 0;
    registerWithMdnsName(server, NULL);
    ck_assert_uint_eq(sonNotifications, 1);
    ck_assert(lastAnnounce);

    sonNotifications = 0;
    server->config.announceRegisteredServers = false;
    registerWithMdnsName(server, NULL);
    ck_assert_uint_eq(sonNotifications, 1);
    ck_assert(!lastAnnounce);

    /* Unchanged again */
    registerWithMdnsName(server, NULL);
    ck_assert_uint_eq(sonNotifications, 1);

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
    TCase *tc_registered = tcase_create("registered servers");
    tcase_add_test(tc_registered, registered_server_timeout_removes_server_on_network);
#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
    tcase_add_test(tc_registered, registered_server_semaphore_removes_server_on_network);
#endif
    tcase_add_test(tc_registered, deregister_server_removes_server_on_network);
    tcase_add_test(tc_registered, reregister_server_replaces_server_on_network);
    tcase_add_loop_test(tc_registered, register_server_capabilities_default_to_na, 0, 3);
    tcase_add_test(tc_registered, reregister_unchanged_server_keeps_record_ids);
    tcase_add_loop_test(tc_registered, register_server_uses_tcp_discovery_url, 0, 2);
    tcase_add_test(tc_registered, reregister_server_without_tcp_url_removes_server_on_network);
    tcase_add_test(tc_registered, deregister_server_without_tcp_url_keeps_other_entry);
    tcase_add_loop_test(tc_registered, register_server_announce_follows_config, 0, 2);
    tcase_add_test(tc_registered, register_server_on_network_announce_param);
    tcase_add_test(tc_registered, public_api_keeps_registered_server_on_network);
    tcase_add_test(tc_registered, register_server_replaces_identical_received_entry);
    tcase_add_test(tc_registered, reregister_server_applies_changed_announce_config);
    suite_add_tcase(suite, tc_registered);
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
