/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/types.h>

#ifdef UA_ENABLE_SUBSCRIPTIONS
#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#endif

#include <stdlib.h>
#include <check.h>

#include "test_helpers.h"
#include "thread_wrapper.h"

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin usernamePasswords[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->allowNonePolicyPassword = true;

    /* Instatiate a new AccessControl plugin that knows username/pw */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_SecurityPolicy *sp = &config->securityPolicies[config->securityPoliciesSize-1];
    UA_AccessControl_default(config, true, &sp->policyUri,
                             usernamePasswordsSize, usernamePasswords);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Client_anonymous) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_user_pass_ok) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_user_fail) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user0", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Client_pass_fail) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "secret");
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

static UA_Boolean
allowBrowseNode(UA_Server *s, UA_AccessControl *ac,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext) {
    UA_Variant attribute;
    UA_Variant_init(&attribute);
    UA_Server_getSessionAttribute(s, sessionId,
                                  UA_QUALIFIEDNAME(0, "my_custom_attribute"),
                                  &attribute);
    return true;
}

static void setCustomAccessControl(UA_ServerConfig* config) {
    UA_Boolean allowAnonymous = true;
    UA_AccessControl_default(config, allowAnonymous, NULL, 0, NULL);
    config->accessControl.allowBrowseNode = allowBrowseNode;
}

START_TEST(Server_sessionParameter) {
    server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    setCustomAccessControl(config);
    UA_Server_run_startup(server);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    bd.browseDirection = UA_BROWSEDIRECTION_BOTH;
    bd.includeSubtypes = true;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);

    UA_BrowseRequest request;
    UA_BrowseRequest_init(&request);
    request.nodesToBrowse = &bd;
    request.nodesToBrowseSize = 1;

    UA_BrowseResponse br = UA_Client_Service_browse(client, request);
    UA_assert(br.resultsSize == 1);
    UA_assert(br.results[0].referencesSize > 0);
    UA_assert(br.results[0].statusCode == UA_STATUSCODE_GOOD);
    UA_BrowseResponse_clear(&br);

    UA_Client_disconnect(client);
    UA_Client_delete(client);

    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
} END_TEST

static UA_Byte
customGetUserAccessLevel(UA_Server *server, UA_AccessControl *ac,
                         const UA_NodeId *sessionId, void *sessionContext,
                         const UA_NodeId *nodeId, void *nodeContext) {
    UA_QualifiedName key = UA_QUALIFIEDNAME(1, "test");
    UA_Variant variant;
    double value = 11.11;
    UA_Variant_setScalar(&variant, &value, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_StatusCode status = UA_Server_setSessionAttribute(server, sessionId, key, &variant);
    ck_assert_uint_eq(status, UA_STATUSCODE_GOOD);
    return 0xFF;
}

START_TEST(Server_setSessionParameter) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    setCustomAccessControl(config);
    config->accessControl.getUserAccessLevel = customGetUserAccessLevel;

    UA_Server_run_startup(server);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_disconnect(client);
    UA_Client_delete(client);

    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
/* Always denies subscription creation */
static UA_Boolean
denyCreateSubscription(UA_Server *s, UA_AccessControl *ac,
                       const UA_NodeId *sessionId, void *sessionContext) {
    return false;
}

/* Regression test for feat/accesscontrol-allow-create-subscription:
 * When allowCreateSubscription denies the request, CreateSubscription
 * must return BadUserAccessDenied instead of succeeding.
 * Uses a direct service call (no network round-trip) to avoid
 * timing-related flakiness. */
START_TEST(Server_denyCreateSubscription) {
    server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_AccessControl_default(config, true, NULL, 0, NULL);
    config->accessControl.allowCreateSubscription = denyCreateSubscription;
    UA_Server_run_startup(server);

    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);
    UA_CreateSubscriptionResponse response;
    UA_CreateSubscriptionResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, &server->adminSession,
                               &request, &response);
    UA_UNLOCK(&server->serviceMutex);

    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_BADUSERACCESSDENIED);

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
} END_TEST

/* When allowCreateSubscription is NULL (not configured), subscription
 * creation is allowed for backward compatibility — matching the pattern
 * of allowAddNode, allowDeleteNode, etc. */
START_TEST(Server_nullCreateSubscriptionCallback) {
    server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_AccessControl_default(config, true, NULL, 0, NULL);
    config->accessControl.allowCreateSubscription = NULL;
    UA_Server_run_startup(server);

    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);
    request.requestedPublishingInterval = 500.0;
    request.requestedLifetimeCount = 10000;
    request.requestedMaxKeepAliveCount = 10;
    request.maxNotificationsPerPublish = 0;
    request.publishingEnabled = true;
    UA_CreateSubscriptionResponse response;
    UA_CreateSubscriptionResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, &server->adminSession,
                               &request, &response);
    UA_UNLOCK(&server->serviceMutex);

    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(response.subscriptionId, 0);

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
} END_TEST
/* When the default allowCreateSubscription callback is used (returns true),
 * CreateSubscription must succeed. */
START_TEST(Server_allowCreateSubscription) {
    server = UA_Server_newForUnitTest();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_AccessControl_default(config, true, NULL, 0, NULL);
    UA_Server_run_startup(server);

    UA_CreateSubscriptionRequest request;
    UA_CreateSubscriptionRequest_init(&request);
    request.requestedPublishingInterval = 500.0;
    request.requestedLifetimeCount = 10000;
    request.requestedMaxKeepAliveCount = 10;
    request.maxNotificationsPerPublish = 0;
    request.publishingEnabled = true;
    UA_CreateSubscriptionResponse response;
    UA_CreateSubscriptionResponse_init(&response);

    UA_LOCK(&server->serviceMutex);
    Service_CreateSubscription(server, &server->adminSession,
                               &request, &response);
    UA_UNLOCK(&server->serviceMutex);

    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(response.subscriptionId, 0);

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
} END_TEST
#endif /* UA_ENABLE_SUBSCRIPTIONS */

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client_user = tcase_create("Client User/Password");
    tcase_add_checked_fixture(tc_client_user, setup, teardown);
    tcase_add_test(tc_client_user, Client_anonymous);
    tcase_add_test(tc_client_user, Client_user_pass_ok);
    tcase_add_test(tc_client_user, Client_user_fail);
    tcase_add_test(tc_client_user, Client_pass_fail);
    suite_add_tcase(s,tc_client_user);

    TCase *tc_server = tcase_create("Server-Side Access Control");
    tcase_add_test(tc_server, Server_sessionParameter);
    tcase_add_test(tc_server, Server_setSessionParameter);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_server, Server_allowCreateSubscription);
    tcase_add_test(tc_server, Server_denyCreateSubscription);
    tcase_add_test(tc_server, Server_nullCreateSubscriptionCallback);
#endif
    suite_add_tcase(s,tc_server);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
