/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Server service function tests */
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

/* === Server-side node operations === */
START_TEST(srv_readWriteNode) {
    /* Read a node attribute via server API */
    UA_Variant val;
    UA_Variant_init(&val);
    UA_StatusCode res = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(val.type == &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Variant_clear(&val);
} END_TEST

START_TEST(srv_readDisplayName) {
    UA_LocalizedText lt;
    UA_StatusCode res = UA_Server_readDisplayName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &lt);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(lt.text.length > 0);
    UA_LocalizedText_clear(&lt);
} END_TEST

START_TEST(srv_readDescription) {
    UA_LocalizedText lt;
    UA_StatusCode res = UA_Server_readDescription(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &lt);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&lt);
} END_TEST

START_TEST(srv_readWriteMask) {
    UA_UInt32 wm;
    UA_StatusCode res = UA_Server_readWriteMask(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &wm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_readNodeClass) {
    UA_NodeClass nc;
    UA_StatusCode res = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &nc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);
} END_TEST

START_TEST(srv_readBrowseName) {
    UA_QualifiedName bn;
    UA_StatusCode res = UA_Server_readBrowseName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &bn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_QualifiedName_clear(&bn);
} END_TEST

START_TEST(srv_readNodeId) {
    UA_NodeId nid;
    UA_StatusCode res = UA_Server_readNodeId(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &nid);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&nid);
} END_TEST

START_TEST(srv_readAccessLevel) {
    UA_Byte al;
    UA_StatusCode res = UA_Server_readAccessLevel(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &al);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_readValueRank) {
    UA_Int32 vr;
    UA_StatusCode res = UA_Server_readValueRank(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &vr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_readIsAbstract) {
    UA_Boolean ia;
    UA_StatusCode res = UA_Server_readIsAbstract(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), &ia);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_readInverseName) {
    UA_LocalizedText lt;
    UA_StatusCode res = UA_Server_readInverseName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &lt);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&lt);
} END_TEST

START_TEST(srv_readEventNotifier) {
    UA_Byte en;
    UA_StatusCode res = UA_Server_readEventNotifier(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &en);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Server-side write operations === */
START_TEST(srv_writeDisplayName) {
    /* Add a writable node first */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 1;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    attr.writeMask = UA_WRITEMASK_DISPLAYNAME;
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 63000);
    UA_Server_addVariableNode(server, newNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SrvWriteDN"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    UA_LocalizedText lt = UA_LOCALIZEDTEXT("en", "New Display Name");
    UA_StatusCode res = UA_Server_writeDisplayName(server, newNode, lt);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_writeValue) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 initVal = 1;
    UA_Variant_setScalar(&attr.value, &initVal, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 63001);
    UA_Server_addVariableNode(server, newNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SrvWriteVal"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    UA_Int32 newVal = 999;
    UA_Variant v;
    UA_Variant_setScalar(&v, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = UA_Server_writeValue(server, newNode, v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_Variant readVal;
    UA_Variant_init(&readVal);
    res = UA_Server_readValue(server, newNode, &readVal);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32 *)readVal.data, 999);
    UA_Variant_clear(&readVal);
} END_TEST

START_TEST(srv_writeAccessLevel) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 1;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    attr.writeMask = UA_WRITEMASK_ACCESSLEVEL;
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 63002);
    UA_Server_addVariableNode(server, newNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SrvWriteAL"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    UA_Byte al = UA_ACCESSLEVELMASK_READ;
    UA_StatusCode res = UA_Server_writeAccessLevel(server, newNode, al);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Node operations via client (services) === */
START_TEST(srv_registerNodes) {
    UA_Client *client = connectClient();

    UA_RegisterNodesRequest registerReq;
    UA_RegisterNodesRequest_init(&registerReq);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    registerReq.nodesToRegister = &nodeId;
    registerReq.nodesToRegisterSize = 1;

    UA_RegisterNodesResponse registerResp =
        UA_Client_Service_registerNodes(client, registerReq);
    ck_assert_uint_eq(registerResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Unregister */
    UA_UnregisterNodesRequest unregReq;
    UA_UnregisterNodesRequest_init(&unregReq);
    unregReq.nodesToUnregister = registerResp.registeredNodeIds;
    unregReq.nodesToUnregisterSize = registerResp.registeredNodeIdsSize;

    UA_UnregisterNodesResponse unregResp =
        UA_Client_Service_unregisterNodes(client, unregReq);
    ck_assert_uint_eq(unregResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    /* Prevent double-free: null out stack pointers before clearing */
    unregReq.nodesToUnregister = NULL;
    unregReq.nodesToUnregisterSize = 0;
    registerReq.nodesToRegister = NULL;
    registerReq.nodesToRegisterSize = 0;

    UA_UnregisterNodesResponse_clear(&unregResp);
    UA_RegisterNodesResponse_clear(&registerResp);

    disconnectClient(client);
} END_TEST

START_TEST(srv_multiRead) {
    UA_Client *client = connectClient();

    /* Read multiple attributes at once */
    UA_ReadRequest readReq;
    UA_ReadRequest_init(&readReq);
    UA_ReadValueId rvids[3];
    for(int i = 0; i < 3; i++)
        UA_ReadValueId_init(&rvids[i]);

    rvids[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    rvids[0].attributeId = UA_ATTRIBUTEID_VALUE;
    rvids[1].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvids[1].attributeId = UA_ATTRIBUTEID_BROWSENAME;
    rvids[2].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvids[2].attributeId = UA_ATTRIBUTEID_DISPLAYNAME;

    readReq.nodesToRead = rvids;
    readReq.nodesToReadSize = 3;

    UA_ReadResponse readResp = UA_Client_Service_read(client, readReq);
    ck_assert_uint_eq(readResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(readResp.resultsSize, 3);

    readReq.nodesToRead = NULL;
    readReq.nodesToReadSize = 0;
    UA_ReadResponse_clear(&readResp);
    disconnectClient(client);
} END_TEST

START_TEST(srv_multiWrite) {
    UA_Client *client = connectClient();

    /* Add writable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 1;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId node = UA_NODEID_NUMERIC(1, 63010);
    UA_Server_addVariableNode(server, node,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "MultiWrite"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    UA_WriteRequest writeReq;
    UA_WriteRequest_init(&writeReq);
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = node;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.hasValue = true;
    UA_Int32 newVal = 42;
    UA_Variant_setScalar(&wv.value.value, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    writeReq.nodesToWrite = &wv;
    writeReq.nodesToWriteSize = 1;

    UA_WriteResponse writeResp = UA_Client_Service_write(client, writeReq);
    ck_assert_uint_eq(writeResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    writeReq.nodesToWrite = NULL;
    writeReq.nodesToWriteSize = 0;
    UA_WriteResponse_clear(&writeResp);
    disconnectClient(client);
} END_TEST

/* === Server config operations === */
START_TEST(srv_getConfig) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert_ptr_ne(config, NULL);
    ck_assert_uint_gt(config->maxSecureChannels, 0);
} END_TEST

START_TEST(srv_addNamespace) {
    UA_UInt16 nsIdx = UA_Server_addNamespace(server, "http://test.example.com");
    ck_assert_uint_gt(nsIdx, 0);
    /* Adding same URI returns same index */
    UA_UInt16 nsIdx2 = UA_Server_addNamespace(server, "http://test.example.com");
    ck_assert_uint_eq(nsIdx, nsIdx2);
} END_TEST

START_TEST(srv_getNamespaceByName) {
    size_t nsIdx;
    UA_String uri = UA_STRING("http://opcfoundation.org/UA/");
    UA_StatusCode res = UA_Server_getNamespaceByName(server, uri, &nsIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nsIdx, 0);

    /* Non-existing namespace */
    uri = UA_STRING("http://nonexistent.example.com");
    res = UA_Server_getNamespaceByName(server, uri, &nsIdx);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Server browse === */
START_TEST(srv_browseSimplified) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "Server");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server,
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 1, &qn);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bpr.targetsSize, 0);
    UA_BrowsePathResult_clear(&bpr);
} END_TEST

/* === Server node lifecycle === */
START_TEST(srv_addDeleteVariable) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 64000);

    UA_StatusCode res = UA_Server_addVariableNode(server, newNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SrvDelVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_deleteNode(server, newNode, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_addObjectType) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "SrvObjType");
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 64001);

    UA_StatusCode res = UA_Server_addObjectTypeNode(server, newNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "SrvObjType"),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_addVariableType) {
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "SrvVarType");
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 64002);

    UA_StatusCode res = UA_Server_addVariableTypeNode(server, newNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "SrvVarType"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_addReferenceType) {
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "SrvRefType");
    attr.symmetric = false;
    attr.isAbstract = false;
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 64003);

    UA_StatusCode res = UA_Server_addReferenceTypeNode(server, newNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "SrvRefType"),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_addDataType) {
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "SrvDataType");
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 64004);

    UA_StatusCode res = UA_Server_addDataTypeNode(server, newNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "SrvDataType"),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_addViewNode) {
    UA_ViewAttributes attr = UA_ViewAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "SrvView");
    UA_NodeId newNode = UA_NODEID_NUMERIC(1, 64005);

    UA_StatusCode res = UA_Server_addViewNode(server, newNode,
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SrvView"),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(srv_addDeleteReference) {
    UA_NodeId source = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_NodeId target = UA_NODEID_NUMERIC(1, 64010);
    
    UA_Server_addObjectNode(server, target,
        source, UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefTarget"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        attr, NULL, NULL);

    /* Add another reference */
    UA_ExpandedNodeId expandedTarget;
    UA_ExpandedNodeId_init(&expandedTarget);
    expandedTarget.nodeId = target;
    UA_StatusCode res = UA_Server_addReference(server, source,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        expandedTarget, true);
    /* May succeed or fail */
    (void)res;

    /* Delete the reference */
    res = UA_Server_deleteReference(server, source,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        true, expandedTarget, true);
    (void)res;
} END_TEST

static Suite *testSuite_serverServices(void) {
    TCase *tc_read = tcase_create("Srv_Read");
    tcase_add_checked_fixture(tc_read, setup, teardown);
    tcase_add_test(tc_read, srv_readWriteNode);
    tcase_add_test(tc_read, srv_readDisplayName);
    tcase_add_test(tc_read, srv_readDescription);
    tcase_add_test(tc_read, srv_readWriteMask);
    tcase_add_test(tc_read, srv_readNodeClass);
    tcase_add_test(tc_read, srv_readBrowseName);
    tcase_add_test(tc_read, srv_readNodeId);
    tcase_add_test(tc_read, srv_readAccessLevel);
    tcase_add_test(tc_read, srv_readValueRank);
    tcase_add_test(tc_read, srv_readIsAbstract);
    tcase_add_test(tc_read, srv_readInverseName);
    tcase_add_test(tc_read, srv_readEventNotifier);

    TCase *tc_write = tcase_create("Srv_Write");
    tcase_add_checked_fixture(tc_write, setup, teardown);
    tcase_add_test(tc_write, srv_writeDisplayName);
    tcase_add_test(tc_write, srv_writeValue);
    tcase_add_test(tc_write, srv_writeAccessLevel);

    TCase *tc_svc = tcase_create("Srv_Services");
    tcase_add_checked_fixture(tc_svc, setup, teardown);
    tcase_add_test(tc_svc, srv_registerNodes);
    tcase_add_test(tc_svc, srv_multiRead);
    tcase_add_test(tc_svc, srv_multiWrite);

    TCase *tc_config = tcase_create("Srv_Config");
    tcase_add_checked_fixture(tc_config, setup, teardown);
    tcase_add_test(tc_config, srv_getConfig);
    tcase_add_test(tc_config, srv_addNamespace);
    tcase_add_test(tc_config, srv_getNamespaceByName);
    tcase_add_test(tc_config, srv_browseSimplified);

    TCase *tc_nodes = tcase_create("Srv_Nodes");
    tcase_add_checked_fixture(tc_nodes, setup, teardown);
    tcase_add_test(tc_nodes, srv_addDeleteVariable);
    tcase_add_test(tc_nodes, srv_addObjectType);
    tcase_add_test(tc_nodes, srv_addVariableType);
    tcase_add_test(tc_nodes, srv_addReferenceType);
    tcase_add_test(tc_nodes, srv_addDataType);
    tcase_add_test(tc_nodes, srv_addViewNode);
    tcase_add_test(tc_nodes, srv_addDeleteReference);

    Suite *s = suite_create("Server Services Extended");
    suite_add_tcase(s, tc_read);
    suite_add_tcase(s, tc_write);
    suite_add_tcase(s, tc_svc);
    suite_add_tcase(s, tc_config);
    suite_add_tcase(s, tc_nodes);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_serverServices();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
