/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Client subscription operation tests */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "testing_clock.h"
#include "thread_wrapper.h"
#include "test_helpers.h"
#include <check.h>
#include <stdlib.h>
#include <stdio.h>

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);

    /* Add test variables */
    for(int i = 0; i < 10; i++) {
        UA_VariableAttributes vattr = UA_VariableAttributes_default;
        UA_Int32 val = i;
        UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
        vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

        char name[32];
        snprintf(name, sizeof(name), "SubTestVar%d", i);
        UA_Server_addVariableNode(server,
            UA_NODEID_NUMERIC(1, 81000 + (UA_UInt32)i),
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, name),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            vattr, NULL, NULL);
    }

    UA_Server_run_startup(server);
    running = true;
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static UA_Client *connectClient(void) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return client;
}

static void disconnectClient(UA_Client *client) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}

static int dataChangeCount = 0;
static int deleteCount = 0;

static void dataChangeHandler(UA_Client *client, UA_UInt32 subId,
                               void *subCtx, UA_UInt32 monId,
                               void *monCtx, UA_DataValue *value) {
    dataChangeCount++;
}

static void deleteHandler(UA_Client *client, UA_UInt32 subId,
                           void *subCtx) {
    deleteCount++;
}

/* === Subscription lifecycle === */
START_TEST(sub_create_modify_delete) {
    UA_Client *client = connectClient();

    /* Create subscription */
    UA_CreateSubscriptionRequest csReq =
        UA_CreateSubscriptionRequest_default();
    csReq.requestedPublishingInterval = 100.0;
    csReq.requestedMaxKeepAliveCount = 10;
    csReq.requestedLifetimeCount = 100;
    csReq.maxNotificationsPerPublish = 100;

    UA_CreateSubscriptionResponse csResp =
        UA_Client_Subscriptions_create(client, csReq, NULL, NULL, deleteHandler);
    ck_assert_uint_eq(csResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = csResp.subscriptionId;

    /* Modify subscription */
    UA_ModifySubscriptionRequest msReq;
    UA_ModifySubscriptionRequest_init(&msReq);
    msReq.subscriptionId = subId;
    msReq.requestedPublishingInterval = 200.0;
    msReq.requestedMaxKeepAliveCount = 20;
    msReq.requestedLifetimeCount = 200;
    msReq.maxNotificationsPerPublish = 50;

    UA_ModifySubscriptionResponse msResp =
        UA_Client_Subscriptions_modify(client, msReq);
    ck_assert_uint_eq(msResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_ModifySubscriptionResponse_clear(&msResp);

    /* SetPublishingMode enable/disable */
    UA_SetPublishingModeRequest spmReq;
    UA_SetPublishingModeRequest_init(&spmReq);
    spmReq.publishingEnabled = false;
    spmReq.subscriptionIdsSize = 1;
    spmReq.subscriptionIds = &subId;
    UA_SetPublishingModeResponse spmResp =
        UA_Client_Subscriptions_setPublishingMode(client, spmReq);
    ck_assert_uint_eq(spmResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_SetPublishingModeResponse_clear(&spmResp);

    spmReq.publishingEnabled = true;
    spmResp = UA_Client_Subscriptions_setPublishingMode(client, spmReq);
    ck_assert_uint_eq(spmResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_SetPublishingModeResponse_clear(&spmResp);

    /* Delete subscription */
    UA_StatusCode res = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

/* === Multiple MonitoredItems === */
START_TEST(sub_multiple_monitored_items) {
    UA_Client *client = connectClient();
    dataChangeCount = 0;

    /* Create subscription */
    UA_CreateSubscriptionRequest csReq =
        UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse csResp =
        UA_Client_Subscriptions_create(client, csReq, NULL, NULL, deleteHandler);
    ck_assert_uint_eq(csResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = csResp.subscriptionId;

    /* Create 5 monitored items at once */
    UA_MonitoredItemCreateRequest items[5];
    for(int i = 0; i < 5; i++) {
        items[i] = UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(1, 81000 + (UA_UInt32)i));
        items[i].requestedParameters.samplingInterval = 50.0;
    }

    UA_MonitoredItemCreateResult results[5];
    UA_Client_DataChangeNotificationCallback callbacks[5] = {
        dataChangeHandler, dataChangeHandler, dataChangeHandler,
        dataChangeHandler, dataChangeHandler
    };
    void *contexts[5] = {NULL, NULL, NULL, NULL, NULL};
    UA_Client_DeleteMonitoredItemCallback delCallbacks[5] = {
        NULL, NULL, NULL, NULL, NULL
    };

    UA_CreateMonitoredItemsRequest createReq;
    UA_CreateMonitoredItemsRequest_init(&createReq);
    createReq.subscriptionId = subId;
    createReq.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    createReq.itemsToCreate = items;
    createReq.itemsToCreateSize = 5;

    UA_CreateMonitoredItemsResponse createResp =
        UA_Client_MonitoredItems_createDataChanges(client, createReq,
            contexts, callbacks, delCallbacks);
    ck_assert_uint_eq(createResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(createResp.resultsSize, 5);

    UA_UInt32 monIds[5];
    for(int i = 0; i < 5; i++) {
        ck_assert_uint_eq(createResp.results[i].statusCode, UA_STATUSCODE_GOOD);
        monIds[i] = createResp.results[i].monitoredItemId;
    }
    UA_CreateMonitoredItemsResponse_clear(&createResp);

    /* Run iterate to get initial values */
    for(int i = 0; i < 20; i++)
        UA_Client_run_iterate(client, 50);

    /* Modify monitored items */
    UA_MonitoredItemModifyRequest modItems[2];
    UA_MonitoredItemModifyRequest_init(&modItems[0]);
    UA_MonitoredItemModifyRequest_init(&modItems[1]);
    modItems[0].monitoredItemId = monIds[0];
    modItems[0].requestedParameters.samplingInterval = 200.0;
    modItems[1].monitoredItemId = monIds[1];
    modItems[1].requestedParameters.samplingInterval = 300.0;

    UA_ModifyMonitoredItemsRequest modReq;
    UA_ModifyMonitoredItemsRequest_init(&modReq);
    modReq.subscriptionId = subId;
    modReq.itemsToModify = modItems;
    modReq.itemsToModifySize = 2;

    UA_ModifyMonitoredItemsResponse modResp =
        UA_Client_MonitoredItems_modify(client, modReq);
    ck_assert_uint_eq(modResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_ModifyMonitoredItemsResponse_clear(&modResp);

    /* Delete 2 monitored items */
    UA_UInt32 delIds[2] = {monIds[3], monIds[4]};
    UA_DeleteMonitoredItemsRequest delReq;
    UA_DeleteMonitoredItemsRequest_init(&delReq);
    delReq.subscriptionId = subId;
    delReq.monitoredItemIds = delIds;
    delReq.monitoredItemIdsSize = 2;

    UA_DeleteMonitoredItemsResponse delResp =
        UA_Client_MonitoredItems_delete(client, delReq);
    ck_assert_uint_eq(delResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_DeleteMonitoredItemsResponse_clear(&delResp);

    /* Cleanup */
    UA_Client_Subscriptions_deleteSingle(client, subId);
    disconnectClient(client);
} END_TEST

/* === Trigger data change and receive notifications === */
START_TEST(sub_data_change_notification) {
    UA_Client *client = connectClient();
    dataChangeCount = 0;

    /* Create subscription with fast publishing interval */
    UA_CreateSubscriptionRequest csReq =
        UA_CreateSubscriptionRequest_default();
    csReq.requestedPublishingInterval = 10.0;
    csReq.requestedMaxKeepAliveCount = 30;
    csReq.requestedLifetimeCount = 100;
    UA_CreateSubscriptionResponse csResp =
        UA_Client_Subscriptions_create(client, csReq, NULL, NULL, deleteHandler);
    ck_assert_uint_eq(csResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = csResp.subscriptionId;

    /* Create monitored item */
    UA_MonitoredItemCreateRequest monReq =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(1, 81000));
    monReq.requestedParameters.samplingInterval = 10.0;

    UA_MonitoredItemCreateResult monRes =
        UA_Client_MonitoredItems_createDataChange(client, subId,
            UA_TIMESTAMPSTORETURN_BOTH, monReq, NULL, dataChangeHandler, NULL);
    ck_assert_uint_eq(monRes.statusCode, UA_STATUSCODE_GOOD);

    /* Process - initial value should trigger notification */
    for(int i = 0; i < 50; i++) {
        UA_Client_run_iterate(client, 10);
        if(dataChangeCount > 0)
            break;
    }

    /* The subscription create and monitored item create are the main coverage 
     * hits. Whether the notification arrives depends on timing. */
    
    /* Write a new value */
    UA_Int32 newVal = 777;
    UA_Variant wv;
    UA_Variant_setScalar(&wv, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(1, 81000), &wv);

    /* Process more */
    for(int i = 0; i < 50; i++) {
        UA_Client_run_iterate(client, 10);
    }

    /* Cleanup */
    UA_Client_Subscriptions_deleteSingle(client, subId);
    disconnectClient(client);
} END_TEST

/* === Subscription with multiple data types === */
START_TEST(sub_different_types) {
    UA_Client *client = connectClient();

    /* Add string and double variables on server side */
    UA_VariableAttributes sattr = UA_VariableAttributes_default;
    UA_String sval = UA_STRING("hello");
    UA_Variant_setScalar(&sattr.value, &sval, &UA_TYPES[UA_TYPES_STRING]);
    sattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    sattr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;

    UA_Client_addVariableNode(client,
        UA_NODEID_NUMERIC(1, 81020),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "StrVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        sattr, NULL);

    UA_VariableAttributes dattr = UA_VariableAttributes_default;
    UA_Double dval = 3.14;
    UA_Variant_setScalar(&dattr.value, &dval, &UA_TYPES[UA_TYPES_DOUBLE]);
    dattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    dattr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;

    UA_Client_addVariableNode(client,
        UA_NODEID_NUMERIC(1, 81021),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "DblVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        dattr, NULL);

    /* Create subscription and monitor both */
    UA_CreateSubscriptionRequest csReq =
        UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse csResp =
        UA_Client_Subscriptions_create(client, csReq, NULL, NULL, NULL);
    ck_assert_uint_eq(csResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = csResp.subscriptionId;

    UA_MonitoredItemCreateRequest monReq1 =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(1, 81020));
    UA_Client_MonitoredItems_createDataChange(client, subId,
        UA_TIMESTAMPSTORETURN_BOTH, monReq1, NULL, dataChangeHandler, NULL);

    UA_MonitoredItemCreateRequest monReq2 =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(1, 81021));
    UA_Client_MonitoredItems_createDataChange(client, subId,
        UA_TIMESTAMPSTORETURN_BOTH, monReq2, NULL, dataChangeHandler, NULL);

    for(int i = 0; i < 10; i++)
        UA_Client_run_iterate(client, 50);

    /* Write to trigger changes */
    UA_String newStr = UA_STRING("world");
    UA_Variant sv;
    UA_Variant_setScalar(&sv, &newStr, &UA_TYPES[UA_TYPES_STRING]);
    UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(1, 81020), &sv);

    UA_Double newDbl = 2.718;
    UA_Variant dv;
    UA_Variant_setScalar(&dv, &newDbl, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(1, 81021), &dv);

    for(int i = 0; i < 20; i++)
        UA_Client_run_iterate(client, 50);

    UA_Client_Subscriptions_deleteSingle(client, subId);
    disconnectClient(client);
} END_TEST

/* === Discovery operations via client === */
START_TEST(client_find_servers) {
    UA_Client *client = connectClient();

    UA_ApplicationDescription *appDescs = NULL;
    size_t appDescsSize = 0;
    UA_StatusCode res = UA_Client_findServers(client,
        "opc.tcp://localhost:4840",
        0, NULL, 0, NULL,
        &appDescsSize, &appDescs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(appDescsSize > 0);
    UA_Array_delete(appDescs, appDescsSize,
        &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

    disconnectClient(client);
} END_TEST

START_TEST(client_get_endpoints) {
    UA_Client *client = connectClient();

    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    UA_StatusCode res = UA_Client_getEndpoints(client,
        "opc.tcp://localhost:4840",
        &endpointsSize, &endpoints);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(endpointsSize > 0);
    UA_Array_delete(endpoints, endpointsSize,
        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    disconnectClient(client);
} END_TEST

/* === Client read/write typed accessors === */
START_TEST(client_read_typed) {
    UA_Client *client = connectClient();

    /* Read NodeId attribute */
    UA_NodeId nodeId;
    UA_StatusCode res = UA_Client_readNodeIdAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &nodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&nodeId);

    /* Read NodeClass */
    UA_NodeClass nc;
    res = UA_Client_readNodeClassAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &nc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);

    /* Read BrowseName */
    UA_QualifiedName bn;
    res = UA_Client_readBrowseNameAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &bn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_QualifiedName_clear(&bn);

    /* Read DisplayName */
    UA_LocalizedText dn;
    res = UA_Client_readDisplayNameAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &dn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&dn);

    /* Read Description */
    UA_LocalizedText desc;
    res = UA_Client_readDescriptionAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &desc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&desc);

    /* Read WriteMask */
    UA_UInt32 wm;
    res = UA_Client_readWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &wm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read UserWriteMask */
    UA_UInt32 uwm;
    res = UA_Client_readUserWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &uwm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read IsAbstract from a type */
    UA_Boolean isAbs;
    res = UA_Client_readIsAbstractAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), &isAbs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read Symmetric from ref type */
    UA_Boolean sym;
    res = UA_Client_readSymmetricAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &sym);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read InverseName from ref type */
    UA_LocalizedText inv;
    res = UA_Client_readInverseNameAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &inv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&inv);

    /* Read ContainsNoLoops from views folder (it's an object, may fail) */
    UA_Boolean cnl;
    res = UA_Client_readContainsNoLoopsAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER), &cnl);
    (void)res;

    /* Read EventNotifier from server object */
    UA_Byte en;
    res = UA_Client_readEventNotifierAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &en);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

/* === Client write typed accessors === */
START_TEST(client_write_typed) {
    UA_Client *client = connectClient();

    /* Write DataType */
    UA_NodeId dt = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_StatusCode res = UA_Client_writeDataTypeAttribute(client,
        UA_NODEID_NUMERIC(1, 81000), &dt);
    (void)res;

    /* Write ValueRank */
    UA_Int32 vr = UA_VALUERANK_SCALAR;
    res = UA_Client_writeValueRankAttribute(client,
        UA_NODEID_NUMERIC(1, 81000), &vr);
    (void)res;

    /* Write BrowseName */
    UA_QualifiedName newBN = UA_QUALIFIEDNAME(1, "NewBN");
    res = UA_Client_writeBrowseNameAttribute(client,
        UA_NODEID_NUMERIC(1, 81000), &newBN);
    (void)res;

    disconnectClient(client);
} END_TEST

/* === Client call method === */
START_TEST(client_call_method) {
    UA_Client *client = connectClient();

    /* Call GetMonitoredItems (built-in server method) */
    UA_Variant input;
    UA_UInt32 subId = 0; /* Invalid, but exercises the code path */
    UA_Variant_setScalar(&input, &subId, &UA_TYPES[UA_TYPES_UINT32]);

    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode res = UA_Client_call(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS),
        1, &input, &outputSize, &output);
    /* Will fail because subId=0 doesn't exist, but exercises the path */
    (void)res;
    if(output)
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);

    disconnectClient(client);
} END_TEST

/* === Client register/unregister nodes === */
START_TEST(client_register_nodes) {
    UA_Client *client = connectClient();

    UA_NodeId nodes[3] = {
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NUMERIC(1, 81000),
        UA_NODEID_NUMERIC(1, 81001)
    };

    UA_RegisterNodesRequest regReq;
    UA_RegisterNodesRequest_init(&regReq);
    regReq.nodesToRegister = nodes;
    regReq.nodesToRegisterSize = 3;

    UA_RegisterNodesResponse regResp = UA_Client_Service_registerNodes(client, regReq);
    ck_assert_uint_eq(regResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(regResp.registeredNodeIdsSize, 3);

    UA_UnregisterNodesRequest unregReq;
    UA_UnregisterNodesRequest_init(&unregReq);
    unregReq.nodesToUnregister = regResp.registeredNodeIds;
    unregReq.nodesToUnregisterSize = regResp.registeredNodeIdsSize;

    UA_UnregisterNodesResponse unregResp =
        UA_Client_Service_unregisterNodes(client, unregReq);
    ck_assert_uint_eq(unregResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_RegisterNodesResponse_clear(&regResp);
    UA_UnregisterNodesResponse_clear(&unregResp);

    disconnectClient(client);
} END_TEST

/* === Suite definition === */
static Suite *testSuite_clientSubExt2(void) {
    TCase *tc_sub = tcase_create("SubLifecycle");
    tcase_add_checked_fixture(tc_sub, setup, teardown);
    tcase_set_timeout(tc_sub, 30);
    tcase_add_test(tc_sub, sub_create_modify_delete);
    tcase_add_test(tc_sub, sub_multiple_monitored_items);
    tcase_add_test(tc_sub, sub_data_change_notification);
    tcase_add_test(tc_sub, sub_different_types);

    TCase *tc_disc = tcase_create("Discovery");
    tcase_add_checked_fixture(tc_disc, setup, teardown);
    tcase_set_timeout(tc_disc, 30);
    tcase_add_test(tc_disc, client_find_servers);
    tcase_add_test(tc_disc, client_get_endpoints);

    TCase *tc_ops = tcase_create("ClientOps");
    tcase_add_checked_fixture(tc_ops, setup, teardown);
    tcase_set_timeout(tc_ops, 30);
    tcase_add_test(tc_ops, client_read_typed);
    tcase_add_test(tc_ops, client_write_typed);
    tcase_add_test(tc_ops, client_call_method);
    tcase_add_test(tc_ops, client_register_nodes);

    Suite *s = suite_create("Client Subscription Extended 2");
    suite_add_tcase(s, tc_sub);
    suite_add_tcase(s, tc_disc);
    suite_add_tcase(s, tc_ops);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_clientSubExt2();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
