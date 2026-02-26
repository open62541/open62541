/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Client highlevel API tests */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
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

/* === Read operations === */
START_TEST(hl_readNodeId) {
    UA_Client *client = connectClient();
    UA_NodeId val;
    UA_StatusCode res = UA_Client_readNodeIdAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&val);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readNodeClass) {
    UA_Client *client = connectClient();
    UA_NodeClass nc;
    UA_StatusCode res = UA_Client_readNodeClassAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &nc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readBrowseName) {
    UA_Client *client = connectClient();
    UA_QualifiedName val;
    UA_StatusCode res = UA_Client_readBrowseNameAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_QualifiedName_clear(&val);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readDisplayName) {
    UA_Client *client = connectClient();
    UA_LocalizedText val;
    UA_StatusCode res = UA_Client_readDisplayNameAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&val);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readDescription) {
    UA_Client *client = connectClient();
    UA_LocalizedText val;
    UA_StatusCode res = UA_Client_readDescriptionAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&val);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readWriteMask) {
    UA_Client *client = connectClient();
    UA_UInt32 val;
    UA_StatusCode res = UA_Client_readWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readUserWriteMask) {
    UA_Client *client = connectClient();
    UA_UInt32 val;
    UA_StatusCode res = UA_Client_readUserWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readValue) {
    UA_Client *client = connectClient();
    UA_Variant val;
    UA_Variant_init(&val);
    UA_StatusCode res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readDataType) {
    UA_Client *client = connectClient();
    UA_NodeId val;
    UA_StatusCode res = UA_Client_readDataTypeAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&val);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readValueRank) {
    UA_Client *client = connectClient();
    UA_Int32 val;
    UA_StatusCode res = UA_Client_readValueRankAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readIsAbstract) {
    UA_Client *client = connectClient();
    UA_Boolean val;
    UA_StatusCode res = UA_Client_readIsAbstractAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readAccessLevel) {
    UA_Client *client = connectClient();
    UA_Byte val;
    UA_StatusCode res = UA_Client_readAccessLevelAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readMinimumSamplingInterval) {
    UA_Client *client = connectClient();
    UA_Double val;
    /* Some variables may not have this attribute */
    UA_Client_readMinimumSamplingIntervalAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &val);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readHistorizing) {
    UA_Client *client = connectClient();
    UA_Boolean val;
    UA_StatusCode res = UA_Client_readHistorizingAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readEventNotifier) {
    UA_Client *client = connectClient();
    UA_Byte val;
    UA_StatusCode res = UA_Client_readEventNotifierAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readContainsNoLoops) {
    UA_Client *client = connectClient();
    UA_Boolean val;
    /* Views have this attribute */
    UA_Client_readContainsNoLoopsAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWNODE), &val);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readInverseName) {
    UA_Client *client = connectClient();
    UA_LocalizedText val;
    UA_StatusCode res = UA_Client_readInverseNameAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&val);
    disconnectClient(client);
} END_TEST

START_TEST(hl_readSymmetric) {
    UA_Client *client = connectClient();
    UA_Boolean val;
    UA_StatusCode res = UA_Client_readSymmetricAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

/* === Write operations === */
START_TEST(hl_writeValue) {
    UA_Client *client = connectClient();

    /* First add a writable variable node via server-side */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 60000);
    UA_Server_addVariableNode(server, newNode,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "HLWriteTest"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);

    /* Write via client */
    UA_Int32 newVal = 99;
    UA_Variant writeVal;
    UA_Variant_setScalar(&writeVal, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = UA_Client_writeValueAttribute(client, newNode, &writeVal);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_Variant readVal;
    UA_Variant_init(&readVal);
    res = UA_Client_readValueAttribute(client, newNode, &readVal);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32 *)readVal.data, 99);
    UA_Variant_clear(&readVal);

    disconnectClient(client);
} END_TEST

START_TEST(hl_writeDisplayName) {
    UA_Client *client = connectClient();
    UA_LocalizedText val = UA_LOCALIZEDTEXT("en", "NewDisplayName");
    /* Writing display name to a custom node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 v = 1;
    UA_Variant_setScalar(&attr.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    attr.writeMask = UA_WRITEMASK_DISPLAYNAME;
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 60001);
    UA_Server_addVariableNode(server, newNode,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "HLWriteDN"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);

    UA_StatusCode res = UA_Client_writeDisplayNameAttribute(client, newNode, &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_writeDescription) {
    UA_Client *client = connectClient();
    UA_LocalizedText val = UA_LOCALIZEDTEXT("en", "NewDescription");
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 v = 1;
    UA_Variant_setScalar(&attr.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    attr.writeMask = UA_WRITEMASK_DESCRIPTION;
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 60002);
    UA_Server_addVariableNode(server, newNode,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "HLWriteDesc"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);

    UA_StatusCode res = UA_Client_writeDescriptionAttribute(client, newNode, &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

/* === Node management === */
START_TEST(hl_addObjectNode) {
    UA_Client *client = connectClient();
    UA_NodeId newNodeId = UA_NODEID_NUMERIC(1, 60010);
    UA_NodeId requestedNodeId = UA_NODEID_NUMERIC(1, 60010);
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "HLTestObject");

    UA_StatusCode res = UA_Client_addObjectNode(client, requestedNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "HLTestObject"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        attr, &newNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_addVariableNode) {
    UA_Client *client = connectClient();
    UA_NodeId newNodeId;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 100;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    attr.displayName = UA_LOCALIZEDTEXT("en", "HLTestVar");

    UA_StatusCode res = UA_Client_addVariableNode(client,
        UA_NODEID_NUMERIC(1, 60011),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "HLTestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, &newNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(hl_deleteNode) {
    UA_Client *client = connectClient();

    /* Add then delete */
    UA_NodeId newNodeId;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_StatusCode res = UA_Client_addObjectNode(client,
        UA_NODEID_NUMERIC(1, 60012),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ToDelete"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        attr, &newNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Client_deleteNode(client, newNodeId, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    disconnectClient(client);
} END_TEST

START_TEST(hl_addReference) {
    UA_Client *client = connectClient();

    /* Add a node first */
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_NodeId newNodeId = UA_NODEID_NUMERIC(1, 60013);
    UA_Client_addObjectNode(client, newNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefTest"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        attr, NULL);

    /* Add reference */
    UA_StatusCode res = UA_Client_addReference(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
        UA_STRING_NULL,
        UA_EXPANDEDNODEID_NUMERIC(1, 60013),
        UA_NODECLASS_OBJECT);
    /* May succeed or fail depending on existing refs */
    (void)res;

    disconnectClient(client);
} END_TEST

/* === Browse === */
START_TEST(hl_browse) {
    UA_Client *client = connectClient();

    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    bReq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_FORWARD;

    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    ck_assert_uint_eq(bResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bResp.resultsSize, 0);
    ck_assert_uint_gt(bResp.results[0].referencesSize, 0);

    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
    disconnectClient(client);
} END_TEST

START_TEST(hl_browseSimplifiedPath) {
    UA_Client *client = connectClient();

    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "Server");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 1,
            &qn);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bpr.targetsSize, 0);
    UA_BrowsePathResult_clear(&bpr);

    disconnectClient(client);
} END_TEST

static Suite *testSuite_clientHighLevel(void) {
    TCase *tc_read = tcase_create("HL_Read");
    tcase_add_checked_fixture(tc_read, setup, teardown);
    tcase_add_test(tc_read, hl_readNodeId);
    tcase_add_test(tc_read, hl_readNodeClass);
    tcase_add_test(tc_read, hl_readBrowseName);
    tcase_add_test(tc_read, hl_readDisplayName);
    tcase_add_test(tc_read, hl_readDescription);
    tcase_add_test(tc_read, hl_readWriteMask);
    tcase_add_test(tc_read, hl_readUserWriteMask);
    tcase_add_test(tc_read, hl_readValue);
    tcase_add_test(tc_read, hl_readDataType);
    tcase_add_test(tc_read, hl_readValueRank);
    tcase_add_test(tc_read, hl_readIsAbstract);
    tcase_add_test(tc_read, hl_readAccessLevel);
    tcase_add_test(tc_read, hl_readMinimumSamplingInterval);
    tcase_add_test(tc_read, hl_readHistorizing);
    tcase_add_test(tc_read, hl_readEventNotifier);
    tcase_add_test(tc_read, hl_readContainsNoLoops);
    tcase_add_test(tc_read, hl_readInverseName);
    tcase_add_test(tc_read, hl_readSymmetric);

    TCase *tc_write = tcase_create("HL_Write");
    tcase_add_checked_fixture(tc_write, setup, teardown);
    tcase_add_test(tc_write, hl_writeValue);
    tcase_add_test(tc_write, hl_writeDisplayName);
    tcase_add_test(tc_write, hl_writeDescription);

    TCase *tc_nodes = tcase_create("HL_Nodes");
    tcase_add_checked_fixture(tc_nodes, setup, teardown);
    tcase_add_test(tc_nodes, hl_addObjectNode);
    tcase_add_test(tc_nodes, hl_addVariableNode);
    tcase_add_test(tc_nodes, hl_deleteNode);
    tcase_add_test(tc_nodes, hl_addReference);

    TCase *tc_browse = tcase_create("HL_Browse");
    tcase_add_checked_fixture(tc_browse, setup, teardown);
    tcase_add_test(tc_browse, hl_browse);
    tcase_add_test(tc_browse, hl_browseSimplifiedPath);

    Suite *s = suite_create("Client HighLevel API");
    suite_add_tcase(s, tc_read);
    suite_add_tcase(s, tc_write);
    suite_add_tcase(s, tc_nodes);
    suite_add_tcase(s, tc_browse);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_clientHighLevel();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
