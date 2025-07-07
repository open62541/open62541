/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>

#include "client/ua_client_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

static void
addVariable(size_t size) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32* array = (UA_Int32*)UA_malloc(size * sizeof(UA_Int32));
    for(size_t i = 0; i < size; i++)
        array[i] = (UA_Int32)i;
    UA_Variant_setArray(&attr.value, array, size, &UA_TYPES[UA_TYPES_INT32]);

    char name[] = "my.variable";
    attr.description = UA_LOCALIZEDTEXT("en-US", name);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, name);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, name);
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);

   /* add displayname to variable */
    UA_Server_writeDisplayName(server, myIntegerNodeId,
                              UA_LOCALIZEDTEXT("de", "meine.Variable"));
    UA_free(array);
}

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

#define VARLENGTH 16366

static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin usernamePasswords[2] = {
    {UA_STRING_STATIC("user1"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1")}};

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_SecurityPolicy *sp = &config->securityPolicies[config->securityPoliciesSize-1];
    UA_AccessControl_default(config, true, &sp->policyUri,
                             usernamePasswordsSize, usernamePasswords);

    UA_Server_run_startup(server);
    addVariable(VARLENGTH);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void clearLocale(UA_ClientConfig *config) {
    if(config->sessionLocaleIdsSize > 0 && config->sessionLocaleIds) {
        UA_Array_delete(config->sessionLocaleIds,
                        config->sessionLocaleIdsSize, &UA_TYPES[UA_TYPES_LOCALEID]);
    }
    config->sessionLocaleIds = NULL;
    config->sessionLocaleIdsSize = 0;
}

static void changeLocale(UA_Client *client) {
    UA_LocalizedText loc;
    UA_ClientConfig *config = UA_Client_getConfig(client);
    clearLocale(config);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US"); 
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");
    UA_StatusCode retval = UA_Client_activateCurrentSession(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    const UA_NodeId nodeIdString = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readDisplayNameAttribute(client, nodeIdString, &loc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText newLocaleEng = UA_LOCALIZEDTEXT("en-US", "my.variable");
    ck_assert(UA_String_equal(&newLocaleEng.locale, &loc.locale));
    ck_assert(UA_String_equal(&newLocaleEng.text, &loc.text));
    UA_LocalizedText_clear(&loc);

    clearLocale(config);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("de"); 
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("en-US");
    retval = UA_Client_activateCurrentSession(client);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readDisplayNameAttribute(client, nodeIdString, &loc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    UA_LocalizedText newLocaleGerm = UA_LOCALIZEDTEXT("de", "meine.Variable");
    ck_assert(UA_String_equal(&newLocaleGerm.locale, &loc.locale));
    ck_assert(UA_String_equal(&newLocaleGerm.text, &loc.text));
    UA_LocalizedText_clear(&loc);
}

START_TEST(Client_activateSessionWithoutConnect) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");

    changeLocale(client);
    ck_assert_uint_eq(server->sessionCount, 0);
    UA_LocaleId_delete(&config->sessionLocaleIds[0]);
    UA_LocaleId_delete(&config->sessionLocaleIds[1]);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_activateSession) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    changeLocale(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_activateSession_username) {
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->allowNonePolicyPassword = true;

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");
    UA_StatusCode retval =
        UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    changeLocale(client);
    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_read) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    changeLocale(client);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(val.type == &UA_TYPES[UA_TYPES_INT32]);
    UA_Int32 *var = (UA_Int32*)val.data;
    for(size_t i = 0; i < VARLENGTH; i++)
        ck_assert_uint_eq((size_t)var[i], i);
    UA_Variant_clear(&val);

    UA_Client_delete(client);
}
END_TEST

START_TEST(Client_renewSecureChannel) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    changeLocale(client);

    /* Forward the time */
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_fakeSleep((UA_UInt32)((UA_Double)cc->secureChannelLifeTime * 0.8));

    /* Make the client renew the channel */
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Now read */
    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert(retval == UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_delete(client);

} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
START_TEST(Client_renewSecureChannelWithActiveSubscription) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");
    config->secureChannelLifeTime = 60000;

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    /* Force the server to send keep alive responses every second to trigg
     * the client to send new publish requests. Requests from the client
     * will make the server to change to the new SecurityToken after renewal.
     */
    request.requestedPublishingInterval = 1000;
    request.requestedMaxKeepAliveCount = 1;
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);

    UA_CreateSubscriptionResponse_clear(&response);
    changeLocale(client);

    /* manually control the server thread */
    running = false;
    THREAD_JOIN(server_thread);

    for(int i = 0; i < 15; ++i) {
        UA_fakeSleep(1000);
        UA_Server_run_iterate(server, true);
        retval = UA_Client_run_iterate(client, 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

 //   UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST
#endif

START_TEST(Client_switch) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    changeLocale(client);
    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_NodeId authenticationToken;
    UA_ByteString sessionNonce;
    UA_Client_getSessionAuthenticationToken(client, &authenticationToken,
                                            &sessionNonce);

    UA_Client *client2 = UA_Client_newForUnitTest();
    retval = UA_Client_connectSecureChannel(client2, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_activateSession(client2, authenticationToken, sessionNonce);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    retval = UA_Client_readValueAttribute(client2, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_NodeId_clear(&authenticationToken);
    UA_ByteString_clear(&sessionNonce);

    UA_Client_delete(client);
    UA_Client_delete(client2);
}
END_TEST

START_TEST(Client_activateSessionClose) {
    // restart server
    teardown();
    setup();
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    UA_Variant val;
    UA_NodeId nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);
    changeLocale(client);

    UA_Client_disconnect(client);
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    changeLocale(client);
    nodeId = UA_NODEID_STRING(1, "my.variable");
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_delete(client);
    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_activateSessionTimeout) {
    // restart server
    teardown();
    setup();
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId *)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en-US");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("de");
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(server->sessionCount, 1);
    changeLocale(client);
    ck_assert_uint_eq(server->sessionCount, 1);

    UA_Variant val;
    UA_Variant_init(&val);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Manually close the connection. The connection is internally closed at the
     * next iteration of the EventLoop. Hence the next request is sent out. But
     * the connection "actually closes" before receiving the response. */
    UA_ConnectionManager *cm = client->channel.connectionManager;
    uintptr_t connId = client->channel.connectionId;
    cm->closeConnection(cm, connId);

    UA_Variant_init(&val);
    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADCONNECTIONCLOSED);

    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    retval = UA_Client_readValueAttribute(client, nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    UA_Client_delete(client);

    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

START_TEST(Client_activateSessionLocaleIds) {
    // restart server
    teardown();
    setup();
    ck_assert_uint_eq(server->sessionCount, 0);

    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    config->sessionLocaleIdsSize = 2;
    config->sessionLocaleIds = (UA_LocaleId*)UA_Array_new(2, &UA_TYPES[UA_TYPES_LOCALEID]);
    config->sessionLocaleIds[0] = UA_STRING_ALLOC("en");
    config->sessionLocaleIds[1] = UA_STRING_ALLOC("fr");

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->sessionCount, 1);

    changeLocale(client);
    ck_assert_uint_eq(server->sessionCount, 1);

    UA_QualifiedName key = {0, UA_STRING_STATIC("localeIds")};
    UA_Variant locales;
    UA_Server_getSessionAttribute(server, &server->sessions.lh_first->session.sessionId,
                                  key, &locales);

    ck_assert_uint_eq(locales.arrayLength, 2);
    UA_String *localeIds = (UA_String*)locales.data;
    ck_assert(UA_String_equal(localeIds, &config->sessionLocaleIds[0]));

    UA_Client_delete(client);
    ck_assert_uint_eq(server->sessionCount, 0);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client");
    TCase *tc_client = tcase_create("Client Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_activateSession);
    tcase_add_test(tc_client, Client_activateSession_username);
    tcase_add_test(tc_client, Client_read);
    tcase_add_test(tc_client, Client_renewSecureChannel);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_client, Client_renewSecureChannelWithActiveSubscription);
#endif

    tcase_add_test(tc_client, Client_activateSessionClose);
    tcase_add_test(tc_client, Client_activateSessionTimeout);
    tcase_add_test(tc_client, Client_activateSessionLocaleIds);
    suite_add_tcase(s,tc_client);

    TCase *tc_client_reconnect = tcase_create("Client Session Switch");
    tcase_add_checked_fixture(tc_client_reconnect, setup, teardown);
    tcase_add_test(tc_client_reconnect, Client_switch);
    suite_add_tcase(s,tc_client_reconnect);
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
