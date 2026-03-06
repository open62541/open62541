/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Client async read/write wrapper tests */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>
#include <check.h>
#include <stdio.h>
#include "testing_clock.h"
#include "thread_wrapper.h"
#include "test_helpers.h"

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;
static volatile UA_Boolean asyncCallbackDone;

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static UA_StatusCode
testMethodCallback(UA_Server *s,
                   const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *methodId, void *methodContext,
                   const UA_NodeId *objectId, void *objectContext,
                   size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    if(inputSize == 1 && input[0].type == &UA_TYPES[UA_TYPES_INT32]) {
        UA_Int32 val = *(UA_Int32 *)input[0].data;
        val *= 2;
        UA_Variant_setScalarCopy(output, &val, &UA_TYPES[UA_TYPES_INT32]);
    }
    return UA_STATUSCODE_GOOD;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    /* Add writable variable */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr.writeMask = 0xFFFFFFFF;
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 71001),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "AsyncTestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);

    /* Add a method */
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
        UA_NODEID_NUMERIC(1, 71002),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "AsyncTestMethod"),
        methAttr, testMethodCallback,
        1, &inputArg, 1, &outputArg, NULL, NULL);

    /* Add child nodes for browse testing */
    for(int i = 0; i < 5; i++) {
        UA_ObjectAttributes childAttr = UA_ObjectAttributes_default;
        char name[32];
        snprintf(name, sizeof(name), "BrowseChild%d", i);
        UA_Server_addObjectNode(server,
            UA_NODEID_NUMERIC(1, 71100 + i),
            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, name),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
            childAttr, NULL, NULL);
    }

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

static void iterateClient(UA_Client *client) {
    for(int i = 0; i < 30 && !asyncCallbackDone; i++)
        UA_Client_run_iterate(client, 50);
}

/* Typed callbacks for each async read function */
static void cbReadValue(UA_Client *c, void *ud, UA_UInt32 rId,
                        UA_StatusCode s, UA_DataValue *v) {
    (void)c; (void)ud; (void)rId; (void)v; (void)s;
    asyncCallbackDone = true;
}
static void cbReadDataType(UA_Client *c, void *ud, UA_UInt32 rId,
                           UA_StatusCode s, UA_NodeId *dt) {
    (void)c; (void)ud; (void)rId; (void)dt; (void)s;
    asyncCallbackDone = true;
}
static void cbReadArrayDims(UA_Client *c, void *ud, UA_UInt32 rId,
                            UA_StatusCode s, UA_Variant *ad) {
    (void)c; (void)ud; (void)rId; (void)ad; (void)s;
    asyncCallbackDone = true;
}
static void cbReadNodeClass(UA_Client *c, void *ud, UA_UInt32 rId,
                            UA_StatusCode s, UA_NodeClass *nc) {
    (void)c; (void)ud; (void)rId; (void)nc; (void)s;
    asyncCallbackDone = true;
}
static void cbReadBrowseName(UA_Client *c, void *ud, UA_UInt32 rId,
                             UA_StatusCode s, UA_QualifiedName *bn) {
    (void)c; (void)ud; (void)rId; (void)bn; (void)s;
    asyncCallbackDone = true;
}
static void cbReadDisplayName(UA_Client *c, void *ud, UA_UInt32 rId,
                              UA_StatusCode s, UA_LocalizedText *dn) {
    (void)c; (void)ud; (void)rId; (void)dn; (void)s;
    asyncCallbackDone = true;
}
static void cbReadDescription(UA_Client *c, void *ud, UA_UInt32 rId,
                              UA_StatusCode s, UA_LocalizedText *d) {
    (void)c; (void)ud; (void)rId; (void)d; (void)s;
    asyncCallbackDone = true;
}
static void cbReadWriteMask(UA_Client *c, void *ud, UA_UInt32 rId,
                            UA_StatusCode s, UA_UInt32 *wm) {
    (void)c; (void)ud; (void)rId; (void)wm; (void)s;
    asyncCallbackDone = true;
}
static void cbReadUserWriteMask(UA_Client *c, void *ud, UA_UInt32 rId,
                                UA_StatusCode s, UA_UInt32 *uwm) {
    (void)c; (void)ud; (void)rId; (void)uwm; (void)s;
    asyncCallbackDone = true;
}
static void cbReadIsAbstract(UA_Client *c, void *ud, UA_UInt32 rId,
                             UA_StatusCode s, UA_Boolean *ia) {
    (void)c; (void)ud; (void)rId; (void)ia; (void)s;
    asyncCallbackDone = true;
}
static void cbReadSymmetric(UA_Client *c, void *ud, UA_UInt32 rId,
                            UA_StatusCode s, UA_Boolean *sym) {
    (void)c; (void)ud; (void)rId; (void)sym; (void)s;
    asyncCallbackDone = true;
}
static void cbReadInverseName(UA_Client *c, void *ud, UA_UInt32 rId,
                              UA_StatusCode s, UA_LocalizedText *inv) {
    (void)c; (void)ud; (void)rId; (void)inv; (void)s;
    asyncCallbackDone = true;
}
static void cbReadContainsNoLoops(UA_Client *c, void *ud, UA_UInt32 rId,
                                  UA_StatusCode s, UA_Boolean *cnl) {
    (void)c; (void)ud; (void)rId; (void)cnl; (void)s;
    asyncCallbackDone = true;
}
static void cbReadEventNotifier(UA_Client *c, void *ud, UA_UInt32 rId,
                                UA_StatusCode s, UA_Byte *en) {
    (void)c; (void)ud; (void)rId; (void)en; (void)s;
    asyncCallbackDone = true;
}
static void cbReadValueRank(UA_Client *c, void *ud, UA_UInt32 rId,
                            UA_StatusCode s, UA_Int32 *vr) {
    (void)c; (void)ud; (void)rId; (void)vr; (void)s;
    asyncCallbackDone = true;
}
static void cbReadAccessLevel(UA_Client *c, void *ud, UA_UInt32 rId,
                              UA_StatusCode s, UA_Byte *al) {
    (void)c; (void)ud; (void)rId; (void)al; (void)s;
    asyncCallbackDone = true;
}
static void cbReadAccessLevelEx(UA_Client *c, void *ud, UA_UInt32 rId,
                                UA_StatusCode s, UA_UInt32 *ale) {
    (void)c; (void)ud; (void)rId; (void)ale; (void)s;
    asyncCallbackDone = true;
}
static void cbReadUserAccessLevel(UA_Client *c, void *ud, UA_UInt32 rId,
                                  UA_StatusCode s, UA_Byte *ual) {
    (void)c; (void)ud; (void)rId; (void)ual; (void)s;
    asyncCallbackDone = true;
}
static void cbReadMinSampling(UA_Client *c, void *ud, UA_UInt32 rId,
                              UA_StatusCode s, UA_Double *msi) {
    (void)c; (void)ud; (void)rId; (void)msi; (void)s;
    asyncCallbackDone = true;
}
static void cbReadHistorizing(UA_Client *c, void *ud, UA_UInt32 rId,
                              UA_StatusCode s, UA_Boolean *h) {
    (void)c; (void)ud; (void)rId; (void)h; (void)s;
    asyncCallbackDone = true;
}
static void cbReadExecutable(UA_Client *c, void *ud, UA_UInt32 rId,
                             UA_StatusCode s, UA_Boolean *e) {
    (void)c; (void)ud; (void)rId; (void)e; (void)s;
    asyncCallbackDone = true;
}
static void cbReadUserExecutable(UA_Client *c, void *ud, UA_UInt32 rId,
                                 UA_StatusCode s, UA_Boolean *ue) {
    (void)c; (void)ud; (void)rId; (void)ue; (void)s;
    asyncCallbackDone = true;
}
static void cbReadAttr(UA_Client *c, void *ud, UA_UInt32 rId,
                       UA_StatusCode s, UA_DataValue *dv) {
    (void)c; (void)ud; (void)rId; (void)dv; (void)s;
    asyncCallbackDone = true;
}
static void cbAsyncWrite(UA_Client *c, void *ud, UA_UInt32 rId,
                         UA_WriteResponse *wr) {
    (void)c; (void)ud; (void)rId; (void)wr;
    asyncCallbackDone = true;
}

/* === Async read tests === */
START_TEST(async_readValue) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readValueAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), cbReadValue, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readDataType) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readDataTypeAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), cbReadDataType, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readArrayDims) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readArrayDimensionsAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), cbReadArrayDims, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readNodeClass) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readNodeClassAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), cbReadNodeClass, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readBrowseName) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readBrowseNameAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), cbReadBrowseName, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readDisplayName) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readDisplayNameAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), cbReadDisplayName, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readDescription) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readDescriptionAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), cbReadDescription, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readWriteMask) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readWriteMaskAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), cbReadWriteMask, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readUserWriteMask) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readUserWriteMaskAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), cbReadUserWriteMask, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readIsAbstract) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readIsAbstractAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), cbReadIsAbstract, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readSymmetric) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readSymmetricAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), cbReadSymmetric, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readInverseName) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readInverseNameAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), cbReadInverseName, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readEventNotifier) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readEventNotifierAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), cbReadEventNotifier, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readValueRank) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readValueRankAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), cbReadValueRank, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readAccessLevel) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readAccessLevelAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), cbReadAccessLevel, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readAccessLevelEx) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readAccessLevelExAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), cbReadAccessLevelEx, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readUserAccessLevel) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readUserAccessLevelAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), cbReadUserAccessLevel, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readMinSampling) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readMinimumSamplingIntervalAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), cbReadMinSampling, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readHistorizing) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readHistorizingAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), cbReadHistorizing, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readExecutable) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readExecutableAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71002), cbReadExecutable, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readUserExecutable) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_StatusCode res = UA_Client_readUserExecutableAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71002), cbReadUserExecutable, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_readContainsNoLoops) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_Client_readContainsNoLoopsAttribute_async(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER), cbReadContainsNoLoops, NULL, &reqId);
    iterateClient(client);
    disconnectClient(client);
} END_TEST

START_TEST(async_readAttribute_generic) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvi.attributeId = UA_ATTRIBUTEID_BROWSENAME;
    UA_StatusCode res = UA_Client_readAttribute_async(client, &rvi,
        UA_TIMESTAMPSTORETURN_BOTH, cbReadAttr, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

/* === Async write tests === */
START_TEST(async_writeValue) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_Int32 val = 999;
    UA_Variant v;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode res = UA_Client_writeValueAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), &v, cbAsyncWrite, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_writeDisplayName) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_LocalizedText lt = UA_LOCALIZEDTEXT("en", "AsyncWritten");
    UA_StatusCode res = UA_Client_writeDisplayNameAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), &lt, cbAsyncWrite, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_writeDescription) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_LocalizedText lt = UA_LOCALIZEDTEXT("en", "AsyncDesc");
    UA_StatusCode res = UA_Client_writeDescriptionAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), &lt, cbAsyncWrite, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

START_TEST(async_writeWriteMask) {
    UA_Client *client = connectClient();
    asyncCallbackDone = false;
    UA_UInt32 reqId = 0;
    UA_UInt32 wm = 0;
    UA_StatusCode res = UA_Client_writeWriteMaskAttribute_async(client,
        UA_NODEID_NUMERIC(1, 71001), &wm, cbAsyncWrite, NULL, &reqId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    iterateClient(client);
    ck_assert(asyncCallbackDone);
    disconnectClient(client);
} END_TEST

/* === Method call === */
START_TEST(client_callMethod) {
    UA_Client *client = connectClient();
    UA_Int32 inputVal = 21;
    UA_Variant input;
    UA_Variant_setScalar(&input, &inputVal, &UA_TYPES[UA_TYPES_INT32]);
    size_t outputSize = 0;
    UA_Variant *output = NULL;
    UA_StatusCode res = UA_Client_call(client,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(1, 71002),
        1, &input, &outputSize, &output);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(outputSize, 1);
    ck_assert_int_eq(*(UA_Int32 *)output[0].data, 42);
    UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    disconnectClient(client);
} END_TEST

/* === BrowseNext === */
START_TEST(client_browseNext) {
    UA_Client *client = connectClient();

    /* Use the highlevel helper to avoid stack-pointer issues with Service_ API */
    UA_BrowseRequest browseReq;
    UA_BrowseRequest_init(&browseReq);
    UA_BrowseDescription *bd = UA_BrowseDescription_new();
    UA_BrowseDescription_init(bd);
    bd->nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd->resultMask = UA_BROWSERESULTMASK_ALL;
    bd->browseDirection = UA_BROWSEDIRECTION_FORWARD;
    browseReq.nodesToBrowse = bd;
    browseReq.nodesToBrowseSize = 1;
    browseReq.requestedMaxReferencesPerNode = 2;

    UA_BrowseResponse browseResp = UA_Client_Service_browse(client, browseReq);
    ck_assert_uint_eq(browseResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    if(browseResp.resultsSize > 0 &&
       browseResp.results[0].continuationPoint.length > 0) {
        UA_ByteString cp;
        UA_ByteString_copy(&browseResp.results[0].continuationPoint, &cp);

        UA_BrowseNextRequest nextReq;
        UA_BrowseNextRequest_init(&nextReq);
        nextReq.releaseContinuationPoints = false;
        nextReq.continuationPoints = &cp;
        nextReq.continuationPointsSize = 1;

        UA_BrowseNextResponse nextResp =
            UA_Client_Service_browseNext(client, nextReq);
        ck_assert_uint_eq(nextResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

        /* Release continuation before clearing */
        nextReq.continuationPoints = NULL;
        nextReq.continuationPointsSize = 0;
        UA_BrowseNextResponse_clear(&nextResp);
        UA_ByteString_clear(&cp);
    }

    UA_BrowseRequest_clear(&browseReq);
    UA_BrowseResponse_clear(&browseResp);
    disconnectClient(client);
} END_TEST

/* === TranslateBrowsePath === */
START_TEST(client_translateBrowsePath) {
    UA_Client *client = connectClient();

    /* Use heap-allocated structures to avoid stack-pointer free issues */
    UA_BrowsePath *bp = UA_BrowsePath_new();
    UA_BrowsePath_init(bp);
    bp->startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_RelativePathElement *rpe = UA_RelativePathElement_new();
    UA_RelativePathElement_init(rpe);
    rpe->targetName = UA_QUALIFIEDNAME_ALLOC(0, "Server");
    rpe->referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    bp->relativePath.elements = rpe;
    bp->relativePath.elementsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsRequest req;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&req);
    req.browsePaths = bp;
    req.browsePathsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsResponse resp =
        UA_Client_Service_translateBrowsePathsToNodeIds(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);

    UA_TranslateBrowsePathsToNodeIdsRequest_clear(&req);
    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&resp);
    disconnectClient(client);
} END_TEST

/* === Client state === */
START_TEST(client_getState) {
    UA_Client *client = connectClient();
    UA_SecureChannelState cState;
    UA_SessionState sState;
    UA_StatusCode connStatus;
    UA_Client_getState(client, &cState, &sState, &connStatus);
    ck_assert_int_eq(cState, UA_SECURECHANNELSTATE_OPEN);
    ck_assert_int_eq(sState, UA_SESSIONSTATE_ACTIVATED);
    ck_assert_uint_eq(connStatus, UA_STATUSCODE_GOOD);
    disconnectClient(client);
} END_TEST

START_TEST(client_disconnectSecureChannel) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode res = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Client_disconnectSecureChannel(client);
    for(int i = 0; i < 20; i++)
        UA_Client_run_iterate(client, 50);
    UA_Client_delete(client);
} END_TEST

static Suite *testSuite_clientAsync(void) {
    TCase *tc_asyncRead = tcase_create("AsyncRead");
    tcase_add_checked_fixture(tc_asyncRead, setup, teardown);
    tcase_add_test(tc_asyncRead, async_readValue);
    tcase_add_test(tc_asyncRead, async_readDataType);
    tcase_add_test(tc_asyncRead, async_readArrayDims);
    tcase_add_test(tc_asyncRead, async_readNodeClass);
    tcase_add_test(tc_asyncRead, async_readBrowseName);
    tcase_add_test(tc_asyncRead, async_readDisplayName);
    tcase_add_test(tc_asyncRead, async_readDescription);
    tcase_add_test(tc_asyncRead, async_readWriteMask);
    tcase_add_test(tc_asyncRead, async_readUserWriteMask);
    tcase_add_test(tc_asyncRead, async_readIsAbstract);
    tcase_add_test(tc_asyncRead, async_readSymmetric);
    tcase_add_test(tc_asyncRead, async_readInverseName);
    tcase_add_test(tc_asyncRead, async_readEventNotifier);
    tcase_add_test(tc_asyncRead, async_readValueRank);
    tcase_add_test(tc_asyncRead, async_readAccessLevel);
    tcase_add_test(tc_asyncRead, async_readAccessLevelEx);
    tcase_add_test(tc_asyncRead, async_readUserAccessLevel);
    tcase_add_test(tc_asyncRead, async_readMinSampling);
    tcase_add_test(tc_asyncRead, async_readHistorizing);
    tcase_add_test(tc_asyncRead, async_readExecutable);
    tcase_add_test(tc_asyncRead, async_readUserExecutable);
    tcase_add_test(tc_asyncRead, async_readContainsNoLoops);
    tcase_add_test(tc_asyncRead, async_readAttribute_generic);

    TCase *tc_asyncWrite = tcase_create("AsyncWrite");
    tcase_add_checked_fixture(tc_asyncWrite, setup, teardown);
    tcase_add_test(tc_asyncWrite, async_writeValue);
    tcase_add_test(tc_asyncWrite, async_writeDisplayName);
    tcase_add_test(tc_asyncWrite, async_writeDescription);
    tcase_add_test(tc_asyncWrite, async_writeWriteMask);

    TCase *tc_ops = tcase_create("ClientOps");
    tcase_add_checked_fixture(tc_ops, setup, teardown);
    tcase_add_test(tc_ops, client_getState);
    tcase_add_test(tc_ops, client_browseNext);
    tcase_add_test(tc_ops, client_translateBrowsePath);
    tcase_add_test(tc_ops, client_disconnectSecureChannel);

    Suite *s = suite_create("Client Async and Operations");
    suite_add_tcase(s, tc_asyncRead);
    suite_add_tcase(s, tc_asyncWrite);
    suite_add_tcase(s, tc_ops);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_clientAsync();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
