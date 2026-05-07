/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Server node management and context function tests */
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

static UA_StatusCode
myTestMethod(UA_Server *s,
             const UA_NodeId *sessionId, void *sessionContext,
             const UA_NodeId *methodId, void *methodContext,
             const UA_NodeId *objectId, void *objectContext,
             size_t inputSize, const UA_Variant *input,
             size_t outputSize, UA_Variant *output) {
    (void)s; (void)sessionId; (void)sessionContext;
    (void)methodId; (void)methodContext; (void)objectId; (void)objectContext;
    if(inputSize == 1) {
        UA_Int32 val = *(UA_Int32 *)input[0].data;
        val *= 3;
        UA_Variant_setScalarCopy(output, &val, &UA_TYPES[UA_TYPES_INT32]);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
myTestMethod2(UA_Server *s,
              const UA_NodeId *sessionId, void *sessionContext,
              const UA_NodeId *methodId, void *methodContext,
              const UA_NodeId *objectId, void *objectContext,
              size_t inputSize, const UA_Variant *input,
              size_t outputSize, UA_Variant *output) {
    (void)s; (void)sessionId; (void)sessionContext;
    (void)methodId; (void)methodContext; (void)objectId; (void)objectContext;
    (void)inputSize; (void)input; (void)outputSize; (void)output;
    return UA_STATUSCODE_GOOD;
}

static void
onRead(UA_Server *s, const UA_NodeId *sessionId,
       void *sessionContext, const UA_NodeId *nodeId,
       void *nodeContext, const UA_NumericRange *range,
       const UA_DataValue *dv) {
    (void)s; (void)sessionId; (void)sessionContext;
    (void)nodeId; (void)nodeContext; (void)range; (void)dv;
}

static void
onWrite(UA_Server *s, const UA_NodeId *sessionId,
        void *sessionContext, const UA_NodeId *nodeId,
        void *nodeContext, const UA_NumericRange *range,
        const UA_DataValue *dv) {
    (void)s; (void)sessionId; (void)sessionContext;
    (void)nodeId; (void)nodeContext; (void)range; (void)dv;
}

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    /* Variable node */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 100;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    vattr.writeMask = 0xFFFFFFFF;
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 80001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "NmgmtTestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    /* Method node */
    UA_Argument inputArg;
    UA_Argument_init(&inputArg);
    inputArg.name = UA_STRING("input");
    inputArg.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inputArg.valueRank = UA_VALUERANK_SCALAR;
    UA_Argument outputArg;
    UA_Argument_init(&outputArg);
    outputArg.name = UA_STRING("output");
    outputArg.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    outputArg.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes methAttr = UA_MethodAttributes_default;
    methAttr.executable = true;
    methAttr.userExecutable = true;
    UA_Server_addMethodNode(server,
        UA_NODEID_NUMERIC(1, 80002),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "NmgmtTestMethod"),
        methAttr, myTestMethod,
        1, &inputArg, 1, &outputArg, NULL, NULL);

    /* Second variable for triggering tests */
    UA_VariableAttributes vattr2 = UA_VariableAttributes_default;
    UA_Int32 val2 = 200;
    UA_Variant_setScalar(&vattr2.value, &val2, &UA_TYPES[UA_TYPES_INT32]);
    vattr2.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 80003),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "NmgmtTrigVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr2, NULL, NULL);

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

/* === getNodeContext / setNodeContext === */
START_TEST(nodeContext_getSet) {
    int myCtx = 42;
    UA_StatusCode res = UA_Server_setNodeContext(server,
        UA_NODEID_NUMERIC(1, 80001), &myCtx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    void *ctx = NULL;
    res = UA_Server_getNodeContext(server, UA_NODEID_NUMERIC(1, 80001), &ctx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(ctx, &myCtx);
} END_TEST

START_TEST(nodeContext_badNode) {
    void *ctx = NULL;
    UA_StatusCode res = UA_Server_getNodeContext(server,
        UA_NODEID_NUMERIC(1, 99999), &ctx);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* === setVariableNodeDynamic === */
START_TEST(setDynamic) {
    UA_StatusCode res = UA_Server_setVariableNodeDynamic(server,
        UA_NODEID_NUMERIC(1, 80001), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Set back */
    res = UA_Server_setVariableNodeDynamic(server,
        UA_NODEID_NUMERIC(1, 80001), false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === getMethodNodeCallback / setMethodNodeCallback === */
START_TEST(methodCallback_getSet) {
    UA_MethodCallback cb = NULL;
    UA_StatusCode res = UA_Server_getMethodNodeCallback(server,
        UA_NODEID_NUMERIC(1, 80002), &cb);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq((void*)(uintptr_t)cb, (void*)(uintptr_t)myTestMethod);

    /* Change callback */
    res = UA_Server_setMethodNodeCallback(server,
        UA_NODEID_NUMERIC(1, 80002), myTestMethod2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    cb = NULL;
    res = UA_Server_getMethodNodeCallback(server,
        UA_NODEID_NUMERIC(1, 80002), &cb);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq((void*)(uintptr_t)cb, (void*)(uintptr_t)myTestMethod2);
} END_TEST

/* === addMethodNodeEx === */
START_TEST(addMethodNodeEx_test) {
    UA_Argument inputArg;
    UA_Argument_init(&inputArg);
    inputArg.name = UA_STRING("in");
    inputArg.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inputArg.valueRank = UA_VALUERANK_SCALAR;
    UA_Argument outputArg;
    UA_Argument_init(&outputArg);
    outputArg.name = UA_STRING("out");
    outputArg.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    outputArg.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes methAttr = UA_MethodAttributes_default;
    methAttr.executable = true;
    methAttr.userExecutable = true;

    UA_NodeId outInputArgNodeId = UA_NODEID_NULL;
    UA_NodeId outOutputArgNodeId = UA_NODEID_NULL;
    UA_NodeId outMethodNodeId = UA_NODEID_NULL;

    UA_StatusCode res = UA_Server_addMethodNodeEx(server,
        UA_NODEID_NUMERIC(1, 80010),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "MethodEx"),
        methAttr, myTestMethod,
        1, &inputArg,
        UA_NODEID_NUMERIC(1, 80011), &outInputArgNodeId,
        1, &outputArg,
        UA_NODEID_NUMERIC(1, 80012), &outOutputArgNodeId,
        NULL, &outMethodNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&outMethodNodeId));
} END_TEST

/* === valueCallback === */
START_TEST(valueCallback_test) {
    UA_ValueCallback cb;
    cb.onRead = onRead;
    cb.onWrite = onWrite;
    UA_StatusCode res = UA_Server_setVariableNode_valueCallback(server,
        UA_NODEID_NUMERIC(1, 80001), cb);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read to trigger onRead */
    UA_Variant out;
    UA_Variant_init(&out);
    res = UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 80001), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* Write to trigger onWrite */
    UA_Int32 newVal = 555;
    UA_Variant wv;
    UA_Variant_setScalar(&wv, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 80001), wv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Client browse / browseNext / translateBrowsePath (highlevel) === */
START_TEST(clientBrowse_highlevel) {
    UA_Client *client = connectClient();
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);

    UA_BrowseResult res = UA_Client_browse(client, NULL, 2, &bd);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(res.referencesSize > 0);

    /* Always try browseNext, even if no continuationPoint */
    UA_BrowseResult next = UA_Client_browseNext(client, true,
                                                 res.continuationPoint);
    UA_BrowseResult_clear(&next);
    UA_BrowseResult_clear(&res);

    disconnectClient(client);
} END_TEST

START_TEST(clientTranslate_highlevel) {
    UA_Client *client = connectClient();
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.targetName = UA_QUALIFIEDNAME(0, "Server");
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    bp.relativePath.elements = &rpe;
    bp.relativePath.elementsSize = 1;

    UA_BrowsePathResult res = UA_Client_translateBrowsePathToNodeIds(client, &bp);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(res.targetsSize > 0);
    UA_BrowsePathResult_clear(&res);

    disconnectClient(client);
} END_TEST

/* === Async add node variants === */
static void asyncAddNodeCb(UA_Client *client, void *userdata,
                           UA_UInt32 requestId, UA_AddNodesResponse *resp) {
    (void)client; (void)userdata; (void)requestId; (void)resp;
}

START_TEST(asyncAddVariable) {
    UA_Client *client = connectClient();
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_addVariableNode_async(client,
        UA_NODEID_NUMERIC(1, 80100),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "AsyncVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, asyncAddNodeCb, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncAddObject) {
    UA_Client *client = connectClient();
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_addObjectNode_async(client,
        UA_NODEID_NUMERIC(1, 80101),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "AsyncObj"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        attr, NULL, asyncAddNodeCb, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncAddObjectType) {
    UA_Client *client = connectClient();
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_addObjectTypeNode_async(client,
        UA_NODEID_NUMERIC(1, 80102),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "AsyncObjType"),
        attr, NULL, asyncAddNodeCb, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncAddVariableType) {
    UA_Client *client = connectClient();
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_addVariableTypeNode_async(client,
        UA_NODEID_NUMERIC(1, 80103),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "AsyncVarType"),
        attr, NULL, asyncAddNodeCb, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncAddView) {
    UA_Client *client = connectClient();
    UA_ViewAttributes attr = UA_ViewAttributes_default;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_addViewNode_async(client,
        UA_NODEID_NUMERIC(1, 80104),
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "AsyncView"),
        attr, NULL, asyncAddNodeCb, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncAddRefType) {
    UA_Client *client = connectClient();
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_addReferenceTypeNode_async(client,
        UA_NODEID_NUMERIC(1, 80105),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "AsyncRefType"),
        attr, NULL, asyncAddNodeCb, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncAddDataType) {
    UA_Client *client = connectClient();
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_addDataTypeNode_async(client,
        UA_NODEID_NUMERIC(1, 80106),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "AsyncDataType"),
        attr, NULL, asyncAddNodeCb, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncAddMethod) {
    UA_Client *client = connectClient();
    UA_MethodAttributes attr = UA_MethodAttributes_default;
    attr.executable = true;
    attr.userExecutable = true;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_addMethodNode_async(client,
        UA_NODEID_NUMERIC(1, 80107),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "AsyncMethod"),
        attr, NULL, asyncAddNodeCb, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

/* === Async writes (additional) === */
static void asyncWriteResp(UA_Client *c, void *ud, UA_UInt32 rId,
                           UA_WriteResponse *wr) {
    (void)c; (void)ud; (void)rId; (void)wr;
}

START_TEST(asyncWriteNodeId) {
    UA_Client *client = connectClient();
    UA_NodeId nid = UA_NODEID_NUMERIC(1, 80001);
    UA_UInt32 reqId = 0;
    UA_Client_writeNodeIdAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80001), &nid, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteNodeClass) {
    UA_Client *client = connectClient();
    UA_NodeClass nc = UA_NODECLASS_VARIABLE;
    UA_UInt32 reqId = 0;
    UA_Client_writeNodeClassAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80001), &nc, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteBrowseName) {
    UA_Client *client = connectClient();
    UA_QualifiedName bn = UA_QUALIFIEDNAME(1, "newname");
    UA_UInt32 reqId = 0;
    UA_Client_writeBrowseNameAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80001), &bn, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteIsAbstract) {
    UA_Client *client = connectClient();
    UA_Boolean ia = false;
    UA_UInt32 reqId = 0;
    UA_Client_writeIsAbstractAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), &ia, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteSymmetric) {
    UA_Client *client = connectClient();
    UA_Boolean sym = false;
    UA_UInt32 reqId = 0;
    UA_Client_writeSymmetricAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &sym, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteInverseName) {
    UA_Client *client = connectClient();
    UA_LocalizedText inv = UA_LOCALIZEDTEXT("en", "InverseTest");
    UA_UInt32 reqId = 0;
    UA_Client_writeInverseNameAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &inv, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteContainsNoLoops) {
    UA_Client *client = connectClient();
    UA_Boolean cnl = false;
    UA_UInt32 reqId = 0;
    UA_Client_writeContainsNoLoopsAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER), &cnl, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteEventNotifier) {
    UA_Client *client = connectClient();
    UA_Byte en = 0;
    UA_UInt32 reqId = 0;
    UA_Client_writeEventNotifierAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &en, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteDataType) {
    UA_Client *client = connectClient();
    UA_NodeId dt = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    UA_UInt32 reqId = 0;
    UA_Client_writeDataTypeAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80001), &dt, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteValueRank) {
    UA_Client *client = connectClient();
    UA_Int32 vr = UA_VALUERANK_SCALAR;
    UA_UInt32 reqId = 0;
    UA_Client_writeValueRankAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80001), &vr, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteAccessLevel) {
    UA_Client *client = connectClient();
    UA_Byte al = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_UInt32 reqId = 0;
    UA_Client_writeAccessLevelAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80001), &al, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteMinSampling) {
    UA_Client *client = connectClient();
    UA_Double ms = 100.0;
    UA_UInt32 reqId = 0;
    UA_Client_writeMinimumSamplingIntervalAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80001), &ms, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteHistorizing) {
    UA_Client *client = connectClient();
    UA_Boolean h = false;
    UA_UInt32 reqId = 0;
    UA_Client_writeHistorizingAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80001), &h, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteExecutable) {
    UA_Client *client = connectClient();
    UA_Boolean e = true;
    UA_UInt32 reqId = 0;
    UA_Client_writeExecutableAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80002), &e, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

START_TEST(asyncWriteAccessLevelEx) {
    UA_Client *client = connectClient();
    UA_UInt32 ale = 3;
    UA_UInt32 reqId = 0;
    UA_Client_writeAccessLevelExAttribute_async(client,
        UA_NODEID_NUMERIC(1, 80001), &ale, asyncWriteResp, NULL, &reqId);
    for(int i = 0; i < 10; i++) UA_Client_run_iterate(client, 50);
    disconnectClient(client);
} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
/* === SetTriggering via client === */
START_TEST(setTriggering_test) {
    UA_Client *client = connectClient();

    /* Create subscription */
    UA_CreateSubscriptionRequest subReq = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse subResp =
        UA_Client_Subscriptions_create(client, subReq, NULL, NULL, NULL);
    ck_assert_uint_eq(subResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_UInt32 subId = subResp.subscriptionId;

    /* Create two monitored items */
    UA_MonitoredItemCreateRequest miReq1 =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(1, 80001));
    UA_MonitoredItemCreateResult miRes1 =
        UA_Client_MonitoredItems_createDataChange(client, subId,
            UA_TIMESTAMPSTORETURN_BOTH, miReq1, NULL, NULL, NULL);
    ck_assert_uint_eq(miRes1.statusCode, UA_STATUSCODE_GOOD);

    UA_MonitoredItemCreateRequest miReq2 =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(1, 80003));
    /* Set sampling mode so we can use it as triggered item */
    miReq2.monitoringMode = UA_MONITORINGMODE_SAMPLING;
    UA_MonitoredItemCreateResult miRes2 =
        UA_Client_MonitoredItems_createDataChange(client, subId,
            UA_TIMESTAMPSTORETURN_BOTH, miReq2, NULL, NULL, NULL);
    ck_assert_uint_eq(miRes2.statusCode, UA_STATUSCODE_GOOD);

    /* SetTriggering: Add link */
    UA_SetTriggeringRequest trigReq;
    UA_SetTriggeringRequest_init(&trigReq);
    trigReq.subscriptionId = subId;
    trigReq.triggeringItemId = miRes1.monitoredItemId;
    trigReq.linksToAddSize = 1;
    UA_UInt32 linkToAdd = miRes2.monitoredItemId;
    trigReq.linksToAdd = &linkToAdd;

    UA_SetTriggeringResponse trigResp =
        UA_Client_MonitoredItems_setTriggering(client, trigReq);
    ck_assert_uint_eq(trigResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_SetTriggeringResponse_clear(&trigResp);

    /* SetTriggering: Remove link */
    UA_SetTriggeringRequest trigReq2;
    UA_SetTriggeringRequest_init(&trigReq2);
    trigReq2.subscriptionId = subId;
    trigReq2.triggeringItemId = miRes1.monitoredItemId;
    trigReq2.linksToRemoveSize = 1;
    trigReq2.linksToRemove = &linkToAdd;

    UA_SetTriggeringResponse trigResp2 =
        UA_Client_MonitoredItems_setTriggering(client, trigReq2);
    ck_assert_uint_eq(trigResp2.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    UA_SetTriggeringResponse_clear(&trigResp2);

    /* Cleanup */
    UA_Client_Subscriptions_deleteSingle(client, subId);
    disconnectClient(client);
} END_TEST
#endif /* UA_ENABLE_SUBSCRIPTIONS */

static Suite *testSuite_nodeManagement(void) {
    TCase *tc_ctx = tcase_create("NodeContext");
    tcase_add_checked_fixture(tc_ctx, setup, teardown);
    tcase_add_test(tc_ctx, nodeContext_getSet);
    tcase_add_test(tc_ctx, nodeContext_badNode);
    tcase_add_test(tc_ctx, setDynamic);
    tcase_add_test(tc_ctx, methodCallback_getSet);
    tcase_add_test(tc_ctx, addMethodNodeEx_test);
    tcase_add_test(tc_ctx, valueCallback_test);

    TCase *tc_browse = tcase_create("ClientBrowseHL");
    tcase_add_checked_fixture(tc_browse, setup, teardown);
    tcase_add_test(tc_browse, clientBrowse_highlevel);
    tcase_add_test(tc_browse, clientTranslate_highlevel);

    TCase *tc_asyncAdd = tcase_create("AsyncAddNodes");
    tcase_add_checked_fixture(tc_asyncAdd, setup, teardown);
    tcase_add_test(tc_asyncAdd, asyncAddVariable);
    tcase_add_test(tc_asyncAdd, asyncAddObject);
    tcase_add_test(tc_asyncAdd, asyncAddObjectType);
    tcase_add_test(tc_asyncAdd, asyncAddVariableType);
    tcase_add_test(tc_asyncAdd, asyncAddView);
    tcase_add_test(tc_asyncAdd, asyncAddRefType);
    tcase_add_test(tc_asyncAdd, asyncAddDataType);
    tcase_add_test(tc_asyncAdd, asyncAddMethod);

    TCase *tc_asyncW = tcase_create("AsyncWriteExtra");
    tcase_add_checked_fixture(tc_asyncW, setup, teardown);
    tcase_add_test(tc_asyncW, asyncWriteNodeId);
    tcase_add_test(tc_asyncW, asyncWriteNodeClass);
    tcase_add_test(tc_asyncW, asyncWriteBrowseName);
    tcase_add_test(tc_asyncW, asyncWriteIsAbstract);
    tcase_add_test(tc_asyncW, asyncWriteSymmetric);
    tcase_add_test(tc_asyncW, asyncWriteInverseName);
    tcase_add_test(tc_asyncW, asyncWriteContainsNoLoops);
    tcase_add_test(tc_asyncW, asyncWriteEventNotifier);
    tcase_add_test(tc_asyncW, asyncWriteDataType);
    tcase_add_test(tc_asyncW, asyncWriteValueRank);
    tcase_add_test(tc_asyncW, asyncWriteAccessLevel);
    tcase_add_test(tc_asyncW, asyncWriteMinSampling);
    tcase_add_test(tc_asyncW, asyncWriteHistorizing);
    tcase_add_test(tc_asyncW, asyncWriteExecutable);
    tcase_add_test(tc_asyncW, asyncWriteAccessLevelEx);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    TCase *tc_trig = tcase_create("SetTriggering");
    tcase_add_checked_fixture(tc_trig, setup, teardown);
    tcase_add_test(tc_trig, setTriggering_test);
#endif

    Suite *s = suite_create("Node Management Extra");
    suite_add_tcase(s, tc_ctx);
    suite_add_tcase(s, tc_browse);
    suite_add_tcase(s, tc_asyncAdd);
    suite_add_tcase(s, tc_asyncW);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    suite_add_tcase(s, tc_trig);
#endif
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_nodeManagement();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
