/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Client highlevel write, browse and method call tests */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <check.h>
#include "testing_clock.h"
#include "thread_wrapper.h"
#include "test_helpers.h"

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
    ck_assert(server != NULL);

    /* Add a method node for testing */
    UA_MethodAttributes methAttr = UA_MethodAttributes_default;
    methAttr.displayName = UA_LOCALIZEDTEXT("en", "TestMethod");
    methAttr.executable = true;
    methAttr.userExecutable = true;
    UA_Server_addMethodNode(server,
        UA_NODEID_NUMERIC(1, 62000),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "TestMethod"),
        methAttr, NULL, 0, NULL, 0, NULL, NULL, NULL);

    /* Add a fully writable variable for writing tests */
    UA_VariableAttributes varAttr = UA_VariableAttributes_default;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&varAttr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    varAttr.displayName = UA_LOCALIZEDTEXT("en", "WritableVar");
    varAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    varAttr.writeMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION |
        UA_WRITEMASK_WRITEMASK | UA_WRITEMASK_ACCESSLEVEL |
        UA_WRITEMASK_MINIMUMSAMPLINGINTERVAL | UA_WRITEMASK_HISTORIZING |
        UA_WRITEMASK_VALUERANK | UA_WRITEMASK_ARRRAYDIMENSIONS |
        UA_WRITEMASK_DATATYPE;
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 62001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "WritableVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        varAttr, NULL, NULL);

    running = true;
    UA_Server_run_startup(server);
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

/* === Additional write attribute tests === */
START_TEST(hl_writeWriteMask) {
    UA_Client *client = connectClient();
    UA_UInt32 wm = UA_WRITEMASK_DISPLAYNAME;
    UA_StatusCode res = UA_Client_writeWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &wm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeAccessLevel) {
    UA_Client *client = connectClient();
    UA_Byte al = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_StatusCode res = UA_Client_writeAccessLevelAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &al);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeMinSampling) {
    UA_Client *client = connectClient();
    UA_Double val = 100.0;
    UA_StatusCode res = UA_Client_writeMinimumSamplingIntervalAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeHistorizing) {
    UA_Client *client = connectClient();
    UA_Boolean val = false;
    UA_StatusCode res = UA_Client_writeHistorizingAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeValueRank) {
    UA_Client *client = connectClient();
    UA_Int32 val = UA_VALUERANK_SCALAR;
    UA_StatusCode res = UA_Client_writeValueRankAttribute(client,
        UA_NODEID_NUMERIC(1, 62001), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeValueScalar) {
    UA_Client *client = connectClient();
    UA_Int32 val = 999;
    UA_StatusCode res = UA_Client_writeValueAttribute_scalar(client,
        UA_NODEID_NUMERIC(1, 62001), &val, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeValueEx) {
    UA_Client *client = connectClient();
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Int32 val = 777;
    UA_Variant_setScalar(&dv.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    dv.hasSourceTimestamp = true;
    dv.sourceTimestamp = UA_DateTime_now();
    UA_StatusCode res = UA_Client_writeValueAttributeEx(client,
        UA_NODEID_NUMERIC(1, 62001), &dv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

/* === Add various node types === */
START_TEST(hl_addVariableTypeNode) {
    UA_Client *client = connectClient();
    UA_NodeId outNodeId;
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestVarType");
    UA_StatusCode res = UA_Client_addVariableTypeNode(client,
        UA_NODEID_NUMERIC(1, 62100),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestVarType"),
        attr, &outNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_addObjectTypeNode) {
    UA_Client *client = connectClient();
    UA_NodeId outNodeId;
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestObjType");
    UA_StatusCode res = UA_Client_addObjectTypeNode(client,
        UA_NODEID_NUMERIC(1, 62101),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestObjType"),
        attr, &outNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_addReferenceTypeNode) {
    UA_Client *client = connectClient();
    UA_NodeId outNodeId;
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestRefType");
    attr.isAbstract = false;
    attr.symmetric = false;
    UA_StatusCode res = UA_Client_addReferenceTypeNode(client,
        UA_NODEID_NUMERIC(1, 62102),
        UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestRefType"),
        attr, &outNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_addDataTypeNode) {
    UA_Client *client = connectClient();
    UA_NodeId outNodeId;
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestDataType");
    UA_StatusCode res = UA_Client_addDataTypeNode(client,
        UA_NODEID_NUMERIC(1, 62103),
        UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestDataType"),
        attr, &outNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_addViewNode) {
    UA_Client *client = connectClient();
    UA_NodeId outNodeId;
    UA_ViewAttributes attr = UA_ViewAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestView");
    UA_StatusCode res = UA_Client_addViewNode(client,
        UA_NODEID_NUMERIC(1, 62104),
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestView"),
        attr, &outNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

/* === Delete reference === */
START_TEST(hl_deleteReference) {
    UA_Client *client = connectClient();
    /* Add node with reference, then delete the reference */
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_NodeId newNodeId;
    UA_Client_addObjectNode(client,
        UA_NODEID_NUMERIC(1, 62110),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "DelRefTest"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        attr, &newNodeId);

    UA_StatusCode res = UA_Client_deleteReference(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), true,
        UA_EXPANDEDNODEID_NUMERIC(1, 62110), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

/* === Method call === */
START_TEST(hl_callMethod) {
    UA_Client *client = connectClient();
    /* Call GetMonitoredItems - a server-provided method */
    UA_Variant input;
    UA_UInt32 subId = 0;
    UA_Variant_setScalar(&input, &subId, &UA_TYPES[UA_TYPES_UINT32]);

    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode res = UA_Client_call(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS),
        1, &input, &outputSize, &output);
    /* May fail if subscription doesn't exist */
    (void)res;
    if(output)
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    disconnectClient(client);
} END_TEST

/* === Browse and translate === */
START_TEST(hl_browseNext) {
    UA_Client *client = connectClient();
    /* Browse with limited results to get continuation point */
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 1;
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;

    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    ck_assert_uint_eq(bResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* If continuation point exists, browse next */
    if(bResp.resultsSize > 0 && bResp.results[0].continuationPoint.length > 0) {
        UA_BrowseNextRequest bnReq;
        UA_BrowseNextRequest_init(&bnReq);
        bnReq.releaseContinuationPoints = false;
        bnReq.continuationPointsSize = 1;
        bnReq.continuationPoints = &bResp.results[0].continuationPoint;

        UA_BrowseNextResponse bnResp =
            UA_Client_Service_browseNext(client, bnReq);
        ck_assert_uint_eq(bnResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

        /* Release continuation point */
        bnReq.releaseContinuationPoints = true;
        UA_BrowseNextResponse bnResp2 =
            UA_Client_Service_browseNext(client, bnReq);
        (void)bnResp2;

        /* Avoid double-free */
        bnReq.continuationPoints = NULL;
        bnReq.continuationPointsSize = 0;
        UA_BrowseNextRequest_clear(&bnReq);
        UA_BrowseNextResponse_clear(&bnResp);
        UA_BrowseNextResponse_clear(&bnResp2);
    }

    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
    disconnectClient(client);
} END_TEST

START_TEST(hl_translateBrowsePath) {
    UA_Client *client = connectClient();

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER);
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe.includeSubtypes = true;
    rpe.targetName = UA_QUALIFIEDNAME(0, "Objects");
    bp.relativePath.elements = &rpe;
    bp.relativePath.elementsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsRequest tbReq;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&tbReq);
    tbReq.browsePaths = &bp;
    tbReq.browsePathsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsResponse tbResp =
        UA_Client_Service_translateBrowsePathsToNodeIds(client, tbReq);
    ck_assert_uint_eq(tbResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(tbResp.resultsSize, 0);
    ck_assert_uint_eq(tbResp.results[0].statusCode, UA_STATUSCODE_GOOD);

    /* Prevent cleanup of stack-allocated data */
    tbReq.browsePaths = NULL;
    tbReq.browsePathsSize = 0;
    UA_TranslateBrowsePathsToNodeIdsRequest_clear(&tbReq);
    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&tbResp);
    disconnectClient(client);
} END_TEST

START_TEST(hl_namespaceGetIndex) {
    UA_Client *client = connectClient();
    UA_UInt16 nsIdx = 0;
    UA_String nsUri = UA_STRING("http://opcfoundation.org/UA/");
    UA_StatusCode res = UA_Client_NamespaceGetIndex(client, &nsUri, &nsIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nsIdx, 0);
    disconnectClient(client);
} END_TEST

/* === forEachChildNodeCall === */
static UA_StatusCode
childCallback(UA_NodeId childId, UA_Boolean isInverse,
              UA_NodeId referenceTypeId, void *handle) {
    UA_UInt32 *count = (UA_UInt32 *)handle;
    (*count)++;
    return UA_STATUSCODE_GOOD;
}

START_TEST(hl_forEachChildNode) {
    UA_Client *client = connectClient();
    UA_UInt32 count = 0;
    UA_StatusCode res = UA_Client_forEachChildNodeCall(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), childCallback, &count);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(count, 0);
    disconnectClient(client);
} END_TEST

/* === Read additional attributes === */
START_TEST(hl_readArrayDimensions) {
    UA_Client *client = connectClient();
    UA_UInt32 *dims = NULL;
    size_t dimsSize = 0;
    UA_StatusCode res = UA_Client_readArrayDimensionsAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
        &dimsSize, &dims);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    if(dims)
        UA_Array_delete(dims, dimsSize, &UA_TYPES[UA_TYPES_UINT32]);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readExecutable) {
    UA_Client *client = connectClient();
    UA_Boolean val;
    UA_StatusCode res = UA_Client_readExecutableAttribute(client,
        UA_NODEID_NUMERIC(1, 62000), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(val == true);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readUserExecutable) {
    UA_Client *client = connectClient();
    UA_Boolean val;
    UA_StatusCode res = UA_Client_readUserExecutableAttribute(client,
        UA_NODEID_NUMERIC(1, 62000), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readUserAccessLevel) {
    UA_Client *client = connectClient();
    UA_Byte val;
    UA_StatusCode res = UA_Client_readUserAccessLevelAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
/* === Subscription tests === */
static void dataChangeCallback(UA_Client *client, UA_UInt32 subId,
                                void *subContext, UA_UInt32 monId,
                                void *monContext, UA_DataValue *value) {
    /* Callback for data change notification */
    (void)client; (void)subId; (void)subContext;
    (void)monId; (void)monContext; (void)value;
}

START_TEST(hl_subscription_create_delete) {
    UA_Client *client = connectClient();

    UA_CreateSubscriptionRequest subReq = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResp =
        UA_Client_Subscriptions_create(client, subReq, NULL, NULL, NULL);
    ck_assert_uint_eq(subResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = subResp.subscriptionId;

    /* Delete subscription */
    UA_StatusCode res = UA_Client_Subscriptions_deleteSingle(client, subId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

START_TEST(hl_subscription_modify) {
    UA_Client *client = connectClient();

    UA_CreateSubscriptionRequest subReq = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResp =
        UA_Client_Subscriptions_create(client, subReq, NULL, NULL, NULL);
    ck_assert_uint_eq(subResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = subResp.subscriptionId;

    /* Modify */
    UA_ModifySubscriptionRequest modReq;
    UA_ModifySubscriptionRequest_init(&modReq);
    modReq.subscriptionId = subId;
    modReq.requestedPublishingInterval = 500.0;
    modReq.requestedMaxKeepAliveCount = 10;
    modReq.requestedLifetimeCount = 100;
    modReq.maxNotificationsPerPublish = 100;
    UA_ModifySubscriptionResponse modResp =
        UA_Client_Subscriptions_modify(client, modReq);
    ck_assert_uint_eq(modResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_Client_Subscriptions_deleteSingle(client, subId);
    disconnectClient(client);
} END_TEST

START_TEST(hl_monitored_item_create) {
    UA_Client *client = connectClient();

    UA_CreateSubscriptionRequest subReq = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResp =
        UA_Client_Subscriptions_create(client, subReq, NULL, NULL, NULL);
    ck_assert_uint_eq(subResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = subResp.subscriptionId;

    /* Create a monitored item */
    UA_MonitoredItemCreateRequest monReq =
        UA_MonitoredItemCreateRequest_default(
            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME));
    UA_MonitoredItemCreateResult monResp =
        UA_Client_MonitoredItems_createDataChange(client, subId,
            UA_TIMESTAMPSTORETURN_BOTH, monReq,
            NULL, dataChangeCallback, NULL);
    ck_assert_uint_eq(monResp.statusCode, UA_STATUSCODE_GOOD);

    /* Run a few iterations to get data */
    UA_Client_run_iterate(client, 100);
    UA_Client_run_iterate(client, 100);

    UA_Client_Subscriptions_deleteSingle(client, subId);
    disconnectClient(client);
} END_TEST

START_TEST(hl_subscription_setPublishingMode) {
    UA_Client *client = connectClient();

    UA_CreateSubscriptionRequest subReq = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResp =
        UA_Client_Subscriptions_create(client, subReq, NULL, NULL, NULL);
    ck_assert_uint_eq(subResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = subResp.subscriptionId;

    /* Disable publishing */
    UA_SetPublishingModeRequest spmReq;
    UA_SetPublishingModeRequest_init(&spmReq);
    spmReq.publishingEnabled = false;
    spmReq.subscriptionIds = &subId;
    spmReq.subscriptionIdsSize = 1;
    UA_SetPublishingModeResponse spmResp =
        UA_Client_Subscriptions_setPublishingMode(client, spmReq);
    ck_assert_uint_eq(spmResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    spmReq.subscriptionIds = NULL;
    spmReq.subscriptionIdsSize = 0;
    UA_SetPublishingModeResponse_clear(&spmResp);

    /* Re-enable */
    spmReq.publishingEnabled = true;
    spmReq.subscriptionIds = &subId;
    spmReq.subscriptionIdsSize = 1;
    spmResp = UA_Client_Subscriptions_setPublishingMode(client, spmReq);
    ck_assert_uint_eq(spmResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    spmReq.subscriptionIds = NULL;
    spmReq.subscriptionIdsSize = 0;
    UA_SetPublishingModeResponse_clear(&spmResp);

    UA_Client_Subscriptions_deleteSingle(client, subId);
    disconnectClient(client);
} END_TEST
#endif /* UA_ENABLE_SUBSCRIPTIONS */

/* === Generic read/write === */
START_TEST(hl_genericRead) {
    UA_Client *client = connectClient();

    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataValue dv = UA_Client_read(client, &rvi);
    ck_assert(dv.hasValue);
    UA_DataValue_clear(&dv);
    disconnectClient(client);
} END_TEST

START_TEST(hl_genericWrite) {
    UA_Client *client = connectClient();

    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = UA_NODEID_NUMERIC(1, 62001);
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.hasValue = true;
    UA_Int32 val = 555;
    UA_Variant_setScalar(&wv.value.value, &val, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode res = UA_Client_write(client, &wv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

/* === Client state and endpoints === */
START_TEST(hl_getEndpoints) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    UA_StatusCode res = UA_Client_getEndpoints(client,
        "opc.tcp://localhost:4840", &endpointsSize, &endpoints);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(endpointsSize, 0);
    UA_Array_delete(endpoints, endpointsSize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
} END_TEST

START_TEST(hl_findServers) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ApplicationDescription *apps = NULL;
    size_t appsSize = 0;
    UA_StatusCode res = UA_Client_findServers(client,
        "opc.tcp://localhost:4840", 0, NULL, 0, NULL, &appsSize, &apps);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(appsSize, 0);
    UA_Array_delete(apps, appsSize,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    UA_Client_delete(client);
} END_TEST

START_TEST(hl_connectDisconnect) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Check state */
    UA_SecureChannelState scState;
    UA_SessionState sessState;
    UA_StatusCode connectStatus;
    UA_Client_getState(client, &scState, &sessState, &connectStatus);
    ck_assert_uint_eq(connectStatus, UA_STATUSCODE_GOOD);

    /* Disconnect */
    UA_Client_disconnect(client);
    UA_Client_getState(client, &scState, &sessState, &connectStatus);

    UA_Client_delete(client);
} END_TEST

START_TEST(hl_connectFailed) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    cc->timeout = 1000;
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:54321");
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
} END_TEST

static Suite *testSuite_clientHL2(void) {
    TCase *tc_write = tcase_create("HL_Write2");
    tcase_add_checked_fixture(tc_write, setup, teardown);
    tcase_add_test(tc_write, hl_writeWriteMask);
    tcase_add_test(tc_write, hl_writeAccessLevel);
    tcase_add_test(tc_write, hl_writeMinSampling);
    tcase_add_test(tc_write, hl_writeHistorizing);
    tcase_add_test(tc_write, hl_writeValueRank);
    tcase_add_test(tc_write, hl_writeValueScalar);
    tcase_add_test(tc_write, hl_writeValueEx);
    tcase_add_test(tc_write, hl_genericRead);
    tcase_add_test(tc_write, hl_genericWrite);

    TCase *tc_nodes = tcase_create("HL_NodeTypes");
    tcase_add_checked_fixture(tc_nodes, setup, teardown);
    tcase_add_test(tc_nodes, hl_addVariableTypeNode);
    tcase_add_test(tc_nodes, hl_addObjectTypeNode);
    tcase_add_test(tc_nodes, hl_addReferenceTypeNode);
    tcase_add_test(tc_nodes, hl_addDataTypeNode);
    tcase_add_test(tc_nodes, hl_addViewNode);
    tcase_add_test(tc_nodes, hl_deleteReference);

    TCase *tc_browse = tcase_create("HL_Browse2");
    tcase_add_checked_fixture(tc_browse, setup, teardown);
    tcase_add_test(tc_browse, hl_browseNext);
    tcase_add_test(tc_browse, hl_translateBrowsePath);
    tcase_add_test(tc_browse, hl_namespaceGetIndex);
    tcase_add_test(tc_browse, hl_forEachChildNode);
    tcase_add_test(tc_browse, hl_callMethod);

    TCase *tc_read = tcase_create("HL_Read2");
    tcase_add_checked_fixture(tc_read, setup, teardown);
    tcase_add_test(tc_read, hl_readArrayDimensions);
    tcase_add_test(tc_read, hl_readExecutable);
    tcase_add_test(tc_read, hl_readUserExecutable);
    tcase_add_test(tc_read, hl_readUserAccessLevel);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    TCase *tc_sub = tcase_create("HL_Subscriptions");
    tcase_add_checked_fixture(tc_sub, setup, teardown);
    tcase_add_test(tc_sub, hl_subscription_create_delete);
    tcase_add_test(tc_sub, hl_subscription_modify);
    tcase_add_test(tc_sub, hl_monitored_item_create);
    tcase_add_test(tc_sub, hl_subscription_setPublishingMode);
#endif

    TCase *tc_conn = tcase_create("HL_Connect");
    tcase_add_checked_fixture(tc_conn, setup, teardown);
    tcase_add_test(tc_conn, hl_getEndpoints);
    tcase_add_test(tc_conn, hl_findServers);
    tcase_add_test(tc_conn, hl_connectDisconnect);
    tcase_add_test(tc_conn, hl_connectFailed);

    Suite *s = suite_create("Client HighLevel API Extended");
    suite_add_tcase(s, tc_write);
    suite_add_tcase(s, tc_nodes);
    suite_add_tcase(s, tc_browse);
    suite_add_tcase(s, tc_read);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    suite_add_tcase(s, tc_sub);
#endif
    suite_add_tcase(s, tc_conn);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_clientHL2();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
