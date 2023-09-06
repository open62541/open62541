/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "client/ua_client_internal.h"
#include "server/ua_server_internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "check.h"
#include "testing_clock.h"
#include "thread_wrapper.h"

UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;
static UA_Boolean noNewSubscription; /* Don't create a subscription when the
                                        session activates */

#define VARLENGTH 16366

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

static void setup(void) {
    noNewSubscription = false;
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->maxPublishReqPerSession = 5;
    UA_Server_run_startup(server);
    addVariable(VARLENGTH);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    if(!server)
        return;

    running = false;
    THREAD_JOIN(server_thread);

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

UA_Boolean notificationReceived = false;
UA_UInt32 countNotificationReceived = 0;
UA_Double publishingInterval = 500.0;

static void
dataChangeHandler(UA_Client *client, UA_UInt32 subId, void *subContext,
                  UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    notificationReceived = true;
    countNotificationReceived++;
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

START_TEST(Client_subscription_createDataChanges) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);
    ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    changeLocale(client);
    UA_UInt32 subId = response.subscriptionId;

    UA_MonitoredItemCreateRequest items[3];
    UA_UInt32 newMonitoredItemIds[3];
    UA_Client_DataChangeNotificationCallback callbacks[3];
    UA_Client_DeleteMonitoredItemCallback deleteCallbacks[3];
    void *contexts = NULL;
    changeLocale(client);

    /* monitor the server state */
    items[0] = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));
    callbacks[0] = dataChangeHandler;
    deleteCallbacks[0] = NULL;

    /* monitor invalid node */
    items[1] = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, 999999));
    callbacks[1] = dataChangeHandler;
    deleteCallbacks[1] = NULL;

    /* monitor current time */
    items[2] = UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    callbacks[2] = dataChangeHandler;
 
    deleteCallbacks[2] = NULL;

    UA_CreateMonitoredItemsRequest createRequest;
    UA_CreateMonitoredItemsRequest_init(&createRequest);
    createRequest.subscriptionId = subId;
    createRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createRequest.itemsToCreate = items;
    createRequest.itemsToCreateSize = 3;
    UA_CreateMonitoredItemsResponse createResponse =
        UA_Client_MonitoredItems_createDataChanges(client, createRequest, NULL,
                                                   callbacks, deleteCallbacks);

    ck_assert_uint_eq(createResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResponse.resultsSize, 3);
    ck_assert_uint_eq(createResponse.results[0].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[0] = createResponse.results[0].monitoredItemId;
    ck_assert_uint_eq(createResponse.results[1].statusCode, UA_STATUSCODE_BADNODEIDUNKNOWN);
    newMonitoredItemIds[1] = createResponse.results[1].monitoredItemId;
    ck_assert_uint_eq(newMonitoredItemIds[1], 0);
    ck_assert_uint_eq(createResponse.results[2].statusCode, UA_STATUSCODE_GOOD);
    newMonitoredItemIds[2] = createResponse.results[2].monitoredItemId;
    ck_assert_uint_eq(createResponse.results[2].statusCode, UA_STATUSCODE_GOOD);
    UA_CreateMonitoredItemsResponse_clear(&createResponse);

    changeLocale(client);
    /* manually control the server thread */
    running = false;
    THREAD_JOIN(server_thread);

    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    countNotificationReceived = 0;
    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 2);

    UA_fakeSleep((UA_UInt32)publishingInterval + 1);
    UA_Server_run_iterate(server, true);

    notificationReceived = false;
    retval = UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(notificationReceived, true);
    ck_assert_uint_eq(countNotificationReceived, 3);

    /* run the server in an independent thread again */
    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_DeleteMonitoredItemsRequest deleteRequest;
    UA_DeleteMonitoredItemsRequest_init(&deleteRequest);
    deleteRequest.subscriptionId = subId;
    deleteRequest.monitoredItemIds = newMonitoredItemIds;
    deleteRequest.monitoredItemIdsSize = 3;

    UA_DeleteMonitoredItemsResponse deleteResponse =
        UA_Client_MonitoredItems_delete(client, deleteRequest);

    ck_assert_uint_eq(deleteResponse.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.resultsSize, 3);
    ck_assert_uint_eq(deleteResponse.results[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(deleteResponse.results[1], UA_STATUSCODE_BADMONITOREDITEMIDINVALID);
    ck_assert_uint_eq(deleteResponse.results[2], UA_STATUSCODE_GOOD);

    UA_DeleteMonitoredItemsResponse_clear(&deleteResponse);

    retval = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Client_delete(client);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Client Subscription");
    TCase *tc_client = tcase_create("Client Subscription Basic");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_add_test(tc_client, Client_subscription_createDataChanges);
    suite_add_tcase(s, tc_client);
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
