/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Server node operation tests */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "testing_clock.h"
#include "thread_wrapper.h"
#include "test_helpers.h"
#include <check.h>
#include <stdlib.h>
#include <stdio.h>

/* Shared server for server-only tests */
static UA_Server *server;

static void setup_serveronly(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
}
static void teardown_serveronly(void) {
    UA_Server_delete(server);
}

/* Server+client setup */
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

/* === Add View node === */
START_TEST(addViewNode) {
    UA_ViewAttributes attr = UA_ViewAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestView");
    attr.description = UA_LOCALIZEDTEXT("en", "A test view");

    UA_StatusCode res = UA_Server_addViewNode(server,
        UA_NODEID_NUMERIC(1, 70001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestView"),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_LocalizedText out;
    UA_LocalizedText_init(&out);
    res = UA_Server_readDisplayName(server, UA_NODEID_NUMERIC(1, 70001), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&out);

    /* Write to it to trigger copy */
    UA_LocalizedText newName = UA_LOCALIZEDTEXT("en", "UpdatedView");
    res = UA_Server_writeDisplayName(server, UA_NODEID_NUMERIC(1, 70001), newName);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Add ObjectType node === */
START_TEST(addObjectTypeNode) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestObjectType");
    attr.description = UA_LOCALIZEDTEXT("en", "A test object type");

    UA_StatusCode res = UA_Server_addObjectTypeNode(server,
        UA_NODEID_NUMERIC(1, 70002),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestObjectType"),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write to it to trigger copy */
    UA_LocalizedText newDesc = UA_LOCALIZEDTEXT("en", "Updated description");
    res = UA_Server_writeDescription(server, UA_NODEID_NUMERIC(1, 70002), newDesc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Add VariableType node === */
START_TEST(addVariableTypeNode) {
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestVariableType");
    attr.valueRank = UA_VALUERANK_SCALAR;

    UA_StatusCode res = UA_Server_addVariableTypeNode(server,
        UA_NODEID_NUMERIC(1, 70003),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestVariableType"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Add DataType node === */
START_TEST(addDataTypeNode) {
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestDataType");

    UA_StatusCode res = UA_Server_addDataTypeNode(server,
        UA_NODEID_NUMERIC(1, 70004),
        UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestDataType"),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Add ReferenceType node === */
START_TEST(addReferenceTypeNode) {
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestRefType");
    attr.inverseName = UA_LOCALIZEDTEXT("en", "InverseTestRefType");

    UA_StatusCode res = UA_Server_addReferenceTypeNode(server,
        UA_NODEID_NUMERIC(1, 70005),
        UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestRefType"),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Attribute read operations === */
START_TEST(readAttributes_variable) {
    /* Add a variable */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en", "ReadTestVar");
    vattr.description = UA_LOCALIZEDTEXT("en", "Test variable for reads");
    UA_Int32 val = 42;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    vattr.valueRank = UA_VALUERANK_SCALAR;
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_StatusCode res = UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 70010),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ReadTestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read various attributes */
    UA_NodeClass nc;
    res = UA_Server_readNodeClass(server, UA_NODEID_NUMERIC(1, 70010), &nc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_VARIABLE);

    UA_QualifiedName bn;
    res = UA_Server_readBrowseName(server, UA_NODEID_NUMERIC(1, 70010), &bn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_QualifiedName_clear(&bn);

    UA_LocalizedText dn;
    res = UA_Server_readDisplayName(server, UA_NODEID_NUMERIC(1, 70010), &dn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&dn);

    UA_LocalizedText desc;
    res = UA_Server_readDescription(server, UA_NODEID_NUMERIC(1, 70010), &desc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&desc);

    UA_UInt32 writeMask;
    res = UA_Server_readWriteMask(server, UA_NODEID_NUMERIC(1, 70010), &writeMask);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId dataType;
    res = UA_Server_readDataType(server, UA_NODEID_NUMERIC(1, 70010), &dataType);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&dataType);

    UA_Int32 valueRank;
    res = UA_Server_readValueRank(server, UA_NODEID_NUMERIC(1, 70010), &valueRank);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Byte accessLevel;
    res = UA_Server_readAccessLevel(server, UA_NODEID_NUMERIC(1, 70010), &accessLevel);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Double minSamplingInterval;
    res = UA_Server_readMinimumSamplingInterval(server,
        UA_NODEID_NUMERIC(1, 70010), &minSamplingInterval);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Boolean historizing;
    res = UA_Server_readHistorizing(server, UA_NODEID_NUMERIC(1, 70010), &historizing);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Write attribute edge cases === */
START_TEST(writeAttributes_variable) {
    /* Add a variable */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 100;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    vattr.writeMask = UA_WRITEMASK_DISPLAYNAME | UA_WRITEMASK_DESCRIPTION |
                      UA_WRITEMASK_WRITEMASK | UA_WRITEMASK_ACCESSLEVEL |
                      UA_WRITEMASK_VALUERANK | UA_WRITEMASK_MINIMUMSAMPLINGINTERVAL |
                      UA_WRITEMASK_HISTORIZING;

    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 70011),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "WriteTestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    /* Write display name */
    UA_LocalizedText newName = UA_LOCALIZEDTEXT("en", "NewDisplayName");
    UA_StatusCode res = UA_Server_writeDisplayName(server,
        UA_NODEID_NUMERIC(1, 70011), newName);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write description */
    UA_LocalizedText newDesc = UA_LOCALIZEDTEXT("en", "New description");
    res = UA_Server_writeDescription(server, UA_NODEID_NUMERIC(1, 70011), newDesc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write value */
    UA_Int32 newVal = 200;
    UA_Variant wv;
    UA_Variant_setScalar(&wv, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 70011), wv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write access level */
    UA_Byte newAccess = UA_ACCESSLEVELMASK_READ;
    res = UA_Server_writeAccessLevel(server, UA_NODEID_NUMERIC(1, 70011), newAccess);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write minimum sampling interval */
    UA_Double newInterval = 500.0;
    res = UA_Server_writeMinimumSamplingInterval(server,
        UA_NODEID_NUMERIC(1, 70011), newInterval);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write historizing */
    UA_Boolean newHist = true;
    res = UA_Server_writeHistorizing(server, UA_NODEID_NUMERIC(1, 70011), newHist);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write value rank */
    UA_Int32 newVR = UA_VALUERANK_SCALAR;
    res = UA_Server_writeValueRank(server, UA_NODEID_NUMERIC(1, 70011), newVR);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Write writemask */
    UA_UInt32 newWM = 0;
    res = UA_Server_writeWriteMask(server, UA_NODEID_NUMERIC(1, 70011), newWM);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Read nonexistent attribute === */
START_TEST(readAttribute_nonexistent) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(1, 99999), &out);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);
} END_TEST

/* === Object node with many references === */
START_TEST(addNodeWithManyReferences) {
    /* Add a parent object */
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en", "RefTestParent");
    UA_Server_addObjectNode(server,
        UA_NODEID_NUMERIC(1, 70020),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefTestParent"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oattr, NULL, NULL);

    /* Add many child variables to create many references */
    for(int i = 0; i < 20; i++) {
        UA_VariableAttributes vattr = UA_VariableAttributes_default;
        UA_Int32 val = i;
        UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
        char name[32];
        snprintf(name, sizeof(name), "Child%d", i);
        vattr.displayName = UA_LOCALIZEDTEXT_ALLOC("en", name);

        UA_QualifiedName bn = UA_QUALIFIEDNAME_ALLOC(1, name);
        UA_Server_addVariableNode(server,
            UA_NODEID_NUMERIC(1, (UA_UInt32)(70030 + i)),
            UA_NODEID_NUMERIC(1, 70020),
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            bn,
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            vattr, NULL, NULL);
        UA_QualifiedName_clear(&bn);
        UA_LocalizedText_clear(&vattr.displayName);
    }

    /* Browse the parent to verify references */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(1, 70020);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(br.referencesSize >= 20);
    UA_BrowseResult_clear(&br);
} END_TEST

/* === Method node === */
static UA_StatusCode
testMethodCallback(UA_Server *methodServer,
                   const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *methodId, void *methodContext,
                   const UA_NodeId *objectId, void *objectContext,
                   size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    if(inputSize == 1 && input[0].type == &UA_TYPES[UA_TYPES_INT32]) {
        UA_Int32 inVal = *(UA_Int32 *)input[0].data;
        UA_Int32 outVal = inVal * 2;
        UA_Variant_setScalarCopy(&output[0], &outVal, &UA_TYPES[UA_TYPES_INT32]);
    }
    return UA_STATUSCODE_GOOD;
}

START_TEST(addMethodNode_withCallback) {
    UA_Argument inputArg;
    UA_Argument_init(&inputArg);
    inputArg.name = UA_STRING("Input");
    inputArg.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inputArg.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArg;
    UA_Argument_init(&outputArg);
    outputArg.name = UA_STRING("Output");
    outputArg.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    outputArg.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes mattr = UA_MethodAttributes_default;
    mattr.displayName = UA_LOCALIZEDTEXT("en", "TestMethod");
    mattr.executable = true;
    mattr.userExecutable = true;

    UA_StatusCode res = UA_Server_addMethodNode(server,
        UA_NODEID_NUMERIC(1, 70060),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "TestMethod"),
        mattr, testMethodCallback,
        1, &inputArg, 1, &outputArg,
        NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Call the method */
    UA_Int32 inVal = 5;
    UA_Variant inVar;
    UA_Variant_setScalar(&inVar, &inVal, &UA_TYPES[UA_TYPES_INT32]);

    UA_CallMethodRequest callReq;
    UA_CallMethodRequest_init(&callReq);
    callReq.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    callReq.methodId = UA_NODEID_NUMERIC(1, 70060);
    callReq.inputArguments = &inVar;
    callReq.inputArgumentsSize = 1;

    UA_CallMethodResult callRes = UA_Server_call(server, &callReq);
    ck_assert_uint_eq(callRes.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(callRes.outputArgumentsSize, 1);
    ck_assert_int_eq(*(UA_Int32 *)callRes.outputArguments[0].data, 10);
    UA_CallMethodResult_clear(&callRes);

    /* Write executable to trigger method node copy */
    UA_Boolean newExec = false;
    res = UA_Server_writeExecutable(server, UA_NODEID_NUMERIC(1, 70060), newExec);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Server lifecycle tests === */
START_TEST(server_lifecycle) {
    UA_Server *s = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(s, NULL);

    UA_ServerConfig *config = UA_Server_getConfig(s);
    ck_assert_ptr_ne(config, NULL);

    /* Start up */
    UA_StatusCode res = UA_Server_run_startup(s);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Run some iterations */
    for(int i = 0; i < 5; i++)
        UA_Server_run_iterate(s, false);

    /* Shutdown */
    res = UA_Server_run_shutdown(s);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_delete(s);
} END_TEST

/* === Discovery via client === */
START_TEST(findServers) {
    UA_Client *client = connectClient();

    UA_ApplicationDescription *appDescs = NULL;
    size_t appDescsSize = 0;
    UA_StatusCode res = UA_Client_findServers(client,
        "opc.tcp://localhost:4840", 0, NULL, 0, NULL,
        &appDescsSize, &appDescs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(appDescsSize > 0);
    UA_Array_delete(appDescs, appDescsSize, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

    disconnectClient(client);
} END_TEST

START_TEST(getEndpoints) {
    UA_Client *client = connectClient();

    UA_EndpointDescription *endpoints = NULL;
    size_t endpointsSize = 0;
    UA_StatusCode res = UA_Client_getEndpoints(client,
        "opc.tcp://localhost:4840", &endpointsSize, &endpoints);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(endpointsSize > 0);
    UA_Array_delete(endpoints, endpointsSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    disconnectClient(client);
} END_TEST

START_TEST(registerUnregisterNodes) {
    UA_Client *client = connectClient();

    /* Register nodes */
    UA_RegisterNodesRequest regReq;
    UA_RegisterNodesRequest_init(&regReq);
    UA_NodeId nodeIds[2];
    nodeIds[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    nodeIds[1] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    regReq.nodesToRegister = nodeIds;
    regReq.nodesToRegisterSize = 2;

    UA_RegisterNodesResponse regResp = UA_Client_Service_registerNodes(client, regReq);
    ck_assert_uint_eq(regResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(regResp.registeredNodeIdsSize, 2);

    /* Unregister */
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

/* === Translate browse paths === */
START_TEST(translateBrowsePaths) {
    UA_Client *client = connectClient();

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_RelativePathElement elem;
    UA_RelativePathElement_init(&elem);
    elem.targetName = UA_QUALIFIEDNAME(0, "Server");
    elem.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    elem.includeSubtypes = true;

    bp.relativePath.elements = &elem;
    bp.relativePath.elementsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsRequest tbpReq;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&tbpReq);
    tbpReq.browsePaths = &bp;
    tbpReq.browsePathsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsResponse tbpResp =
        UA_Client_Service_translateBrowsePathsToNodeIds(client, tbpReq);
    ck_assert_uint_eq(tbpResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(tbpResp.resultsSize, 1);
    ck_assert_uint_eq(tbpResp.results[0].statusCode, UA_STATUSCODE_GOOD);

    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&tbpResp);
    disconnectClient(client);
} END_TEST

/* === Browse with continuation === */
START_TEST(browseWithContinuation) {
    UA_Client *client = connectClient();

    /* Browse Objects folder expecting multiple refs */
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bReq.nodesToBrowse = &bd;
    bReq.nodesToBrowseSize = 1;
    bReq.requestedMaxReferencesPerNode = 2; /* Low limit to trigger continuation */

    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    ck_assert_uint_eq(bResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert(bResp.resultsSize > 0);

    /* If there is a continuation point, use BrowseNext */
    if(bResp.results[0].continuationPoint.length > 0) {
        UA_BrowseNextRequest bnReq;
        UA_BrowseNextRequest_init(&bnReq);
        bnReq.continuationPoints = &bResp.results[0].continuationPoint;
        bnReq.continuationPointsSize = 1;
        bnReq.releaseContinuationPoints = false;

        UA_BrowseNextResponse bnResp = UA_Client_Service_browseNext(client, bnReq);
        ck_assert_uint_eq(bnResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

        /* Release continuation point */
        if(bnResp.resultsSize > 0 &&
           bnResp.results[0].continuationPoint.length > 0) {
            UA_BrowseNextRequest relReq;
            UA_BrowseNextRequest_init(&relReq);
            relReq.continuationPoints = &bnResp.results[0].continuationPoint;
            relReq.continuationPointsSize = 1;
            relReq.releaseContinuationPoints = true;
            UA_BrowseNextResponse relResp = UA_Client_Service_browseNext(client, relReq);
            UA_BrowseNextResponse_clear(&relResp);
        }

        UA_BrowseNextResponse_clear(&bnResp);
    }

    UA_BrowseResponse_clear(&bResp);
    disconnectClient(client);
} END_TEST

/* === Delete nodes === */
START_TEST(addAndDeleteObject) {
    /* Add an object */
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en", "DeleteMe");
    UA_Server_addObjectNode(server,
        UA_NODEID_NUMERIC(1, 70070),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "DeleteMe"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oattr, NULL, NULL);

    /* Delete it */
    UA_StatusCode res = UA_Server_deleteNode(server,
        UA_NODEID_NUMERIC(1, 70070), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify it's gone */
    UA_Variant out;
    UA_Variant_init(&out);
    res = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 70070), &out);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* === Add/delete references === */
START_TEST(addDeleteReference) {
    /* Add two objects */
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en", "RefSrc");
    UA_Server_addObjectNode(server,
        UA_NODEID_NUMERIC(1, 70080),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefSrc"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oattr, NULL, NULL);

    oattr.displayName = UA_LOCALIZEDTEXT("en", "RefDst");
    UA_Server_addObjectNode(server,
        UA_NODEID_NUMERIC(1, 70081),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefDst"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oattr, NULL, NULL);

    /* Add a reference */
    UA_StatusCode res = UA_Server_addReference(server,
        UA_NODEID_NUMERIC(1, 70080),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_EXPANDEDNODEID_NUMERIC(1, 70081), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Delete the reference */
    res = UA_Server_deleteReference(server,
        UA_NODEID_NUMERIC(1, 70080),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        true,
        UA_EXPANDEDNODEID_NUMERIC(1, 70081), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Read/Write via client === */
START_TEST(clientReadWriteAttributes) {
    UA_Client *client = connectClient();

    /* Read display name of Objects folder */
    UA_LocalizedText dn;
    UA_StatusCode res = UA_Client_readDisplayNameAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &dn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&dn);

    /* Read description */
    UA_LocalizedText desc;
    res = UA_Client_readDescriptionAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &desc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&desc);

    /* Read browseName */
    UA_QualifiedName bn;
    res = UA_Client_readBrowseNameAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &bn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_QualifiedName_clear(&bn);

    /* Read NodeClass */
    UA_NodeClass nc;
    res = UA_Client_readNodeClassAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &nc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);

    /* Read NodeId */
    UA_NodeId nodeId;
    res = UA_Client_readNodeIdAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &nodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&nodeId);

    /* Read WriteMask */
    UA_UInt32 writeMask;
    res = UA_Client_readWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &writeMask);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read UserWriteMask */
    UA_UInt32 userWriteMask;
    res = UA_Client_readUserWriteMaskAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &userWriteMask);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read EventNotifier */
    UA_Byte eventNotifier;
    res = UA_Client_readEventNotifierAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &eventNotifier);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read server status */
    UA_Variant val;
    UA_Variant_init(&val);
    res = UA_Client_readValueAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS), &val);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&val);

    /* Read isAbstract on BaseObjectType */
    UA_Boolean isAbstract;
    res = UA_Client_readIsAbstractAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), &isAbstract);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read ContainsNoLoops on Organizes reftype.
     * This attribute was removed from ReferenceType nodes in OPC UA 1.04.
     * open62541 may return BadAttributeIdInvalid. */
    UA_Boolean containsNoLoops;
    res = UA_Client_readContainsNoLoopsAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &containsNoLoops);
    (void)res;

    /* Read Symmetric on Organizes reftype */
    UA_Boolean symmetric;
    res = UA_Client_readSymmetricAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &symmetric);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read InverseName */
    UA_LocalizedText inverseName;
    res = UA_Client_readInverseNameAttribute(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &inverseName);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&inverseName);

    disconnectClient(client);
} END_TEST

/* === Suite definition === */
static Suite *testSuite_serverOps(void) {
    TCase *tc_nodes = tcase_create("NodeOps");
    tcase_add_checked_fixture(tc_nodes, setup_serveronly, teardown_serveronly);
    tcase_add_test(tc_nodes, addViewNode);
    tcase_add_test(tc_nodes, addObjectTypeNode);
    tcase_add_test(tc_nodes, addVariableTypeNode);
    tcase_add_test(tc_nodes, addDataTypeNode);
    tcase_add_test(tc_nodes, addReferenceTypeNode);
    tcase_add_test(tc_nodes, readAttributes_variable);
    tcase_add_test(tc_nodes, writeAttributes_variable);
    tcase_add_test(tc_nodes, readAttribute_nonexistent);
    tcase_add_test(tc_nodes, addNodeWithManyReferences);
    tcase_add_test(tc_nodes, addMethodNode_withCallback);
    tcase_add_test(tc_nodes, addAndDeleteObject);
    tcase_add_test(tc_nodes, addDeleteReference);

    TCase *tc_lifecycle = tcase_create("Lifecycle");
    tcase_add_test(tc_lifecycle, server_lifecycle);

    TCase *tc_client = tcase_create("ClientOps");
    tcase_add_checked_fixture(tc_client, setup, teardown);
    tcase_set_timeout(tc_client, 30);
    tcase_add_test(tc_client, findServers);
    tcase_add_test(tc_client, getEndpoints);
    tcase_add_test(tc_client, registerUnregisterNodes);
    tcase_add_test(tc_client, translateBrowsePaths);
    tcase_add_test(tc_client, browseWithContinuation);
    tcase_add_test(tc_client, clientReadWriteAttributes);

    Suite *s = suite_create("Server Ops Extended");
    suite_add_tcase(s, tc_nodes);
    suite_add_tcase(s, tc_lifecycle);
    suite_add_tcase(s, tc_client);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_serverOps();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
