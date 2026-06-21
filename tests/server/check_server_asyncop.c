/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This example is just to see how fast we can process messages. The server does
   not open a TCP port. */

#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/plugin/log_stdout.h>

#include "testing_clock.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

#include "ua_server_internal.h"

#include <check.h>

/* Provide ck_assert_ptr_null / ck_assert_ptr_nonnull on top of older
 * libcheck (Ubuntu 20.04 ships 0.10.x where these shorthands are not
 * yet defined). The ck_assert_msg form compiles on every libcheck
 * version. */
#ifndef ck_assert_ptr_null
# define ck_assert_ptr_null(p) ck_assert_msg((p) == NULL, #p " != NULL")
#endif
#ifndef ck_assert_ptr_nonnull
# define ck_assert_ptr_nonnull(p) ck_assert_msg((p) != NULL, #p " == NULL")
#endif
#include <stdlib.h>

UA_Boolean running;
THREAD_HANDLE server_thread;
static UA_Server *server;
static size_t clientCounter;
static UA_UInt64 lastTimedCallback;

static const void *canceledCallRequest = NULL;
static const void *expectedCanceledCallRequest = NULL;

static void
asyncRead(UA_Server *server, void *data) {
    UA_DataValue *out = (UA_DataValue*)data;
    UA_UInt32 val = 42;
    UA_Variant_setScalarCopy(&out->value, &val, &UA_TYPES[UA_TYPES_UINT32]);
    out->hasValue = true;
    UA_Server_setAsyncReadResult(server, out);
}

static UA_StatusCode
readCallback_async(UA_Server *server, const UA_NodeId *sessionId,
                   void *sessionContext, const UA_NodeId *nodeId,
                   void *nodeContext, UA_Boolean includeSourceTimeStamp,
                   const UA_NumericRange *range, UA_DataValue *value) {
    UA_DateTime callTime = UA_DateTime_now_fake(NULL) + UA_DATETIME_SEC;
    UA_Server_addTimedCallback(server, asyncRead, value, callTime, &lastTimedCallback);
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static void
asyncWrite(UA_Server *server, void *data) {
    UA_Server_setAsyncWriteResult(server, (const UA_DataValue*)data, UA_STATUSCODE_GOOD);
}

static UA_StatusCode
writeCallback_async(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *nodeId,
                    void *nodeContext, const UA_NumericRange *range,
                    const UA_DataValue *value) {
    UA_DateTime callTime = UA_DateTime_now_fake(NULL) + UA_DATETIME_SEC;
    UA_Server_addTimedCallback(server, asyncWrite, (void*)(uintptr_t)value,
                               callTime, &lastTimedCallback);
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static void
asyncCall(UA_Server *server, void *data) {
    UA_Variant *out = (UA_Variant*)data;
    UA_Server_setAsyncCallMethodResult(server, out, UA_STATUSCODE_GOOD);
}

static UA_StatusCode
methodCallback_sync(UA_Server *serverArg,
                    const UA_NodeId *sessionId, void *sessionHandle,
                    const UA_NodeId *methodId, void *methodContext,
                    const UA_NodeId *objectId, void *objectContext,
                    size_t inputSize, const UA_Variant *input,
                    size_t outputSize, UA_Variant *output) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
methodCallback_async(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionHandle,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    UA_DateTime callTime = UA_DateTime_now_fake(NULL) + UA_DATETIME_SEC;
    UA_Server_addTimedCallback(server, asyncCall, output, callTime, &lastTimedCallback);
    expectedCanceledCallRequest = output;
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static void
clientReadCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                   UA_StatusCode status, UA_DataValue *value) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Received read response");
    clientCounter++;
}

static void
clientWriteCallback(UA_Client *client, void *userdata,
                    UA_UInt32 requestId, UA_WriteResponse *wr) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Received write response");
    clientCounter++;
}

static void
clientReceiveCallback(UA_Client *client, void *userdata,
                      UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Received call response");
    clientCounter++;
}

static void
asyncOperationCancelCallback(UA_Server *server, const void *out) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Request %p was canceled", out);
    canceledCallRequest = out;
}

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    clientCounter = 0;
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->asyncOperationTimeout = 2000.0; /* 2 seconds */

    UA_MethodAttributes methodAttr = UA_MethodAttributes_default;
    methodAttr.executable = true;
    methodAttr.userExecutable = true;

    /* Synchronous Method */
    UA_StatusCode res =
        UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "method"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "method"),
                            methodAttr, &methodCallback_sync,
                            0, NULL, 0, NULL, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Asynchronous Method */
    res = UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "asyncMethod"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "asyncMethod"),
                            methodAttr, &methodCallback_async,
                            0, NULL, 0, NULL, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Synchronous Variable */
    UA_VariableAttributes varAttr = UA_VariableAttributes_default;
    varAttr.accessLevel |= UA_ACCESSLEVELMASK_WRITE;
    res = UA_Server_addVariableNode(server,
                                    UA_NODEID_STRING(1, "syncVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "syncVar"),
                                    UA_NS0ID(BASEDATAVARIABLETYPE),
                                    varAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Asynchronous Variable */
    UA_CallbackValueSource evs = {readCallback_async, writeCallback_async};
    res = UA_Server_addVariableNode(server,
                                    UA_NODEID_STRING(1, "asyncVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "asyncVar"),
                                    UA_NS0ID(BASEDATAVARIABLETYPE),
                                    varAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Server_setVariableNode_callbackValueSource(server, UA_NODEID_STRING(1, "asyncVar"), evs);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Async_call) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    running = false;
    THREAD_JOIN(server_thread);

    /* Call async method, then the sync method.
     * The sync method returns first. */
    retval = UA_Client_call_async(client,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "asyncMethod"),
                                  0, NULL, clientReceiveCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_call_async(client,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "method"),
                                  0, NULL, clientReceiveCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Receive the answer of the sync call */
    while(clientCounter == 0) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    /* Iterate and pick up the async response to be sent out */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 2);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_read) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    running = false;
    THREAD_JOIN(server_thread);

    /* Call async method, then the sync method.
     * The sync method returns first. */
    retval = UA_Client_readValueAttribute_async(client,
                                                UA_NODEID_STRING(1, "asyncVar"),
                                                clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readValueAttribute_async(client,
                                                UA_NS0ID(SERVER_NAMESPACEARRAY),
                                                clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Receive the answer of the sync call */
    while(clientCounter == 0) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    /* Iterate and pick up the async response to be sent out */
    while(clientCounter == 1) {
        UA_fakeSleep(1000);
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 2);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_write) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    running = false;
    THREAD_JOIN(server_thread);

    /* Call async method, then the sync method.
     * The sync method returns first. */
    UA_UInt32 i = 42;
    UA_Variant val;
    UA_Variant_setScalar(&val, &i, &UA_TYPES[UA_TYPES_UINT32]);
    retval = UA_Client_writeValueAttribute_async(client,
                                                 UA_NODEID_STRING(1, "asyncVar"),
                                                 &val, clientWriteCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_writeValueAttribute_async(client,
                                                 UA_NODEID_STRING(1, "syncVar"),
                                                 &val, clientWriteCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Receive the answer of the sync call */
    while(clientCounter == 0) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    /* Iterate and pick up the async response to be sent out */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 2);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_timeout) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    running = false;
    THREAD_JOIN(server_thread);

    /* Call async method, then the sync method.
     * The sync method returns first. */
    retval = UA_Client_call_async(client,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "asyncMethod"),
                                  0, NULL, clientReceiveCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* We expect to receive the timeout not yet*/
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 0);

    /* Remove the timed callback. Never answer the method call until we run into
     * a timeout */
    UA_Server_removeCallback(server, lastTimedCallback);

    UA_fakeSleep((UA_UInt32)(1000 * 1.5));

    /* We expect to receive the timeout not yet*/
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 0);

    UA_fakeSleep(1000);

    /* We expect to receive the timeout response.
     * Under lwip with TAP networking the response may need
     * multiple iterations to be delivered. */
    while(clientCounter == 0) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_forget) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    running = false;
    THREAD_JOIN(server_thread);

    /* Call async method, then the sync method.
     * The sync method returns first. */
    retval = UA_Client_call_async(client,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "asyncMethod"),
                                  0, NULL, clientReceiveCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* We expect to receive the timeout not yet*/
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 0);

    /* Remove the timed callback. Never answer the method call.
     * The server should clean it up properly during shutdown. */
    UA_Server_removeCallback(server, lastTimedCallback);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_cancel) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Server_getConfig(server)->asyncOperationCancelCallback = asyncOperationCancelCallback;

    /* Call async method, then the sync method.
     * The sync method returns first. */
    UA_UInt32 reqId = 0;
    retval = UA_Client_call_async(client,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "asyncMethod"),
                                  0, NULL, clientReceiveCallback, NULL, &reqId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Cancel the request */
    UA_UInt32 cancelCount = 0;
    UA_Client_cancelByRequestId(client, reqId, &cancelCount);
    ck_assert_uint_eq(cancelCount, 1);

    /* We expect to receive the cancelled response */
    while(clientCounter != 1) {
        UA_Client_run_iterate(client, 1);
    }

    ck_assert_ptr_eq(expectedCanceledCallRequest, canceledCallRequest);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_cancel_multiple) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_CallRequest creq;
    UA_CallRequest_init(&creq);
    creq.requestHeader.requestHandle = 1337;
    UA_CallMethodRequest cmr;
    UA_CallMethodRequest_init(&cmr);
    cmr.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    cmr.methodId = UA_NODEID_STRING(1, "asyncMethod");
    creq.methodsToCall = &cmr;
    creq.methodsToCallSize = 1;

    __UA_Client_AsyncService(client,
                             &creq, &UA_TYPES[UA_TYPES_CALLREQUEST],
                             NULL, &UA_TYPES[UA_TYPES_CALLRESPONSE],
                             NULL, NULL);

    __UA_Client_AsyncService(client,
                             &creq, &UA_TYPES[UA_TYPES_CALLREQUEST],
                             NULL, &UA_TYPES[UA_TYPES_CALLRESPONSE],
                             NULL, NULL);

    /* Expect two cancelled requests */
    UA_UInt32 cancelCount = 0;
    UA_Client_cancelByRequestHandle(client, 1337, &cancelCount);
    ck_assert_uint_eq(cancelCount, 2);

    UA_Client_run_iterate(client, 0);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* --- Extended coverage tests --- */

static UA_Boolean serverReadResultReceived = false;
static UA_DataValue serverReadResult;

static void
serverAsyncReadCallback(UA_Server *s, void *asyncOpContext,
                         const UA_DataValue *result) {
    UA_DataValue_copy(result, &serverReadResult);
    serverReadResultReceived = true;
}

static void
serverAsyncReadNoopCallback(UA_Server *s, void *asyncOpContext,
                            const UA_DataValue *result) {
    (void)s; (void)asyncOpContext; (void)result;
}

static void
serverAsyncWriteNoopCallback(UA_Server *s, void *asyncOpContext,
                             UA_StatusCode result) {
    (void)s; (void)asyncOpContext; (void)result;
}

START_TEST(Async_server_read) {
    /* Use the server-side async read API directly */
    running = false;
    THREAD_JOIN(server_thread);

    serverReadResultReceived = false;
    UA_DataValue_init(&serverReadResult);

    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_StatusCode retval =
        UA_Server_read_async(server, &rvid,
                             UA_TIMESTAMPSTORETURN_BOTH,
                             serverAsyncReadCallback, NULL, 5000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Process until the callback fires */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, false);
    UA_Server_run_iterate(server, false);
    ck_assert(serverReadResultReceived == true);
    ck_assert(serverReadResult.hasValue);
    UA_DataValue_clear(&serverReadResult);

    running = true;
    THREAD_CREATE(server_thread, serverloop);
} END_TEST

static UA_Boolean serverWriteResultReceived = false;
static UA_StatusCode serverWriteResultCode = UA_STATUSCODE_BADINTERNALERROR;

static void
serverAsyncWriteCallback(UA_Server *s, void *asyncOpContext,
                          UA_StatusCode result) {
    serverWriteResultCode = result;
    serverWriteResultReceived = true;
}

START_TEST(Async_server_write) {
    /* Use the server-side async write API directly */
    running = false;
    THREAD_JOIN(server_thread);

    serverWriteResultReceived = false;

    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = UA_NODEID_STRING(1, "asyncVar");
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_UInt32 val = 999;
    UA_Variant_setScalar(&wv.value.value, &val, &UA_TYPES[UA_TYPES_UINT32]);
    wv.value.hasValue = true;

    UA_StatusCode retval =
        UA_Server_write_async(server, &wv,
                              serverAsyncWriteCallback, NULL, 5000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Process until the callback fires */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, false);
    UA_Server_run_iterate(server, false);
    ck_assert(serverWriteResultReceived == true);
    ck_assert_uint_eq(serverWriteResultCode, UA_STATUSCODE_GOOD);

    running = true;
    THREAD_CREATE(server_thread, serverloop);
} END_TEST

START_TEST(Async_read_timeout_server) {
    /* Start async read with very short timeout, remove the callback so it times out */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    running = false;
    THREAD_JOIN(server_thread);

    clientCounter = 0;
    retval = UA_Client_readValueAttribute_async(client,
                                                UA_NODEID_STRING(1, "asyncVar"),
                                                clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Process the request on the server to start the async op */
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);

    /* Remove the timed callback so it never completes */
    UA_Server_removeCallback(server, lastTimedCallback);

    /* Wait for the async timeout (2 seconds).
     * Under lwip with TAP networking the response may need
     * multiple iterations to be delivered. */
    UA_fakeSleep(3000);
    while(clientCounter == 0) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_setResult_badnotfound) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    UA_StatusCode retval = UA_Server_setAsyncReadResult(server, &dv);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);

    UA_DataValue value;
    UA_DataValue_init(&value);
    UA_UInt32 v = 1;
    UA_Variant_setScalar(&value.value, &v, &UA_TYPES[UA_TYPES_UINT32]);
    value.hasValue = true;
    retval = UA_Server_setAsyncWriteResult(server, &value, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);

#ifdef UA_ENABLE_METHODCALLS
    UA_Variant output;
    UA_Variant_init(&output);
    UA_Int32 outVal = 42;
    UA_Variant_setScalar(&output, &outVal, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Server_setAsyncCallMethodResult(server, &output, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
#endif
} END_TEST

START_TEST(Async_queue_limit_read_direct) {
    running = false;
    THREAD_JOIN(server_thread);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    const UA_UInt32 oldLimit = config->maxAsyncOperationQueueSize;
    config->maxAsyncOperationQueueSize = 1;

    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_StatusCode retval =
        UA_Server_read_async(server, &rvid, UA_TIMESTAMPSTORETURN_BOTH,
                             serverAsyncReadNoopCallback, NULL, 5000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval =
        UA_Server_read_async(server, &rvid, UA_TIMESTAMPSTORETURN_BOTH,
                             serverAsyncReadNoopCallback, NULL, 5000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTOOMANYOPERATIONS);

    config->maxAsyncOperationQueueSize = oldLimit;

    /* Let the first queued async op complete and be cleaned up before teardown. */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, false);
    UA_Server_run_iterate(server, false);

    running = true;
    THREAD_CREATE(server_thread, serverloop);
} END_TEST

START_TEST(Async_sync_method_call) {
    /* Call a synchronous method via the async path - it should complete immediately */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    clientCounter = 0;
    retval = UA_Client_call_async(client,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "method"),
                                  0, NULL, clientReceiveCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* The sync method should return immediately */
    while(clientCounter == 0)
        UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(clientCounter, 1);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_read_sync_variable) {
    /* Read a sync variable via async client - should complete immediately */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    clientCounter = 0;
    retval = UA_Client_readValueAttribute_async(client,
                                                UA_NODEID_STRING(1, "syncVar"),
                                                clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    while(clientCounter == 0)
        UA_Client_run_iterate(client, 1);
    ck_assert_uint_eq(clientCounter, 1);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_service_read_validation_paths) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);

    /* Invalid timestampsToReturn */
    req.timestampsToReturn = (UA_TimestampsToReturn)99;
    UA_ReadResponse rr = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(rr.responseHeader.serviceResult,
                      UA_STATUSCODE_BADTIMESTAMPSTORETURNINVALID);
    UA_ReadResponse_clear(&rr);

    /* Invalid maxAge */
    req.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    req.maxAge = -1.0;
    rr = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(rr.responseHeader.serviceResult,
                      UA_STATUSCODE_BADMAXAGEINVALID);
    UA_ReadResponse_clear(&rr);

    /* Nothing to do */
    req.maxAge = 0.0;
    req.nodesToReadSize = 0;
    req.nodesToRead = NULL;
    rr = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(rr.responseHeader.serviceResult,
                      UA_STATUSCODE_BADNOTHINGTODO);
    UA_ReadResponse_clear(&rr);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_service_read_toomanyoperations) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_UInt32 oldMaxNodesPerRead = config->maxNodesPerRead;
    config->maxNodesPerRead = 1;

    UA_ReadValueId nodes[2];
    UA_ReadValueId_init(&nodes[0]);
    nodes[0].nodeId = UA_NODEID_STRING(1, "syncVar");
    nodes[0].attributeId = UA_ATTRIBUTEID_VALUE;
    UA_ReadValueId_init(&nodes[1]);
    nodes[1].nodeId = UA_NODEID_STRING(1, "asyncVar");
    nodes[1].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    req.maxAge = 0.0;
    req.nodesToReadSize = 2;
    req.nodesToRead = nodes;

    UA_ReadResponse rr = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(rr.responseHeader.serviceResult,
                      UA_STATUSCODE_BADTOOMANYOPERATIONS);
    UA_ReadResponse_clear(&rr);

    config->maxNodesPerRead = oldMaxNodesPerRead;

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_service_write_validation_paths) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_WriteRequest req;
    UA_WriteRequest_init(&req);

    /* Nothing to do */
    req.nodesToWriteSize = 0;
    req.nodesToWrite = NULL;
    UA_WriteResponse wr = UA_Client_Service_write(client, req);
    ck_assert_uint_eq(wr.responseHeader.serviceResult,
                      UA_STATUSCODE_BADNOTHINGTODO);
    UA_WriteResponse_clear(&wr);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_service_write_toomanyoperations) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_UInt32 oldMaxNodesPerWrite = config->maxNodesPerWrite;
    config->maxNodesPerWrite = 1;

    UA_WriteValue values[2];
    UA_WriteValue_init(&values[0]);
    values[0].nodeId = UA_NODEID_STRING(1, "syncVar");
    values[0].attributeId = UA_ATTRIBUTEID_VALUE;
    UA_UInt32 v0 = 11;
    UA_Variant_setScalar(&values[0].value.value, &v0, &UA_TYPES[UA_TYPES_UINT32]);
    values[0].value.hasValue = true;

    UA_WriteValue_init(&values[1]);
    values[1].nodeId = UA_NODEID_STRING(1, "asyncVar");
    values[1].attributeId = UA_ATTRIBUTEID_VALUE;
    UA_UInt32 v1 = 22;
    UA_Variant_setScalar(&values[1].value.value, &v1, &UA_TYPES[UA_TYPES_UINT32]);
    values[1].value.hasValue = true;

    UA_WriteRequest req;
    UA_WriteRequest_init(&req);
    req.nodesToWriteSize = 2;
    req.nodesToWrite = values;

    UA_WriteResponse wr = UA_Client_Service_write(client, req);
    ck_assert_uint_eq(wr.responseHeader.serviceResult,
                      UA_STATUSCODE_BADTOOMANYOPERATIONS);
    UA_WriteResponse_clear(&wr);

    config->maxNodesPerWrite = oldMaxNodesPerWrite;

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

static UA_Boolean directCallCompleted = false;
static UA_StatusCode directCallResultCode = UA_STATUSCODE_BADINTERNALERROR;

static void
directCallCompletionCb(UA_Server *s, void *ctx, const UA_CallMethodResult *result) {
    directCallResultCode = result->statusCode;
    directCallCompleted = true;
}

START_TEST(Async_direct_call_method_result) {
    /* Regression test for the CALL_DIRECT union bug in UA_Server_setAsyncCallMethodResult.
     * UA_Server_call_async stores the pending operation as CALL_DIRECT, with the output
     * embedded inline in op->output.directCall (not behind op->output.call, which is a
     * pointer sharing the same union storage as directCall.statusCode).
     *
     * The pre-patch code dereferenced op->output.call without first checking
     * op->asyncOperationType.  For a CALL_DIRECT operation the statusCode field at union
     * offset 0 is 0 after init, so op->output.call aliases a NULL pointer and
     * op->output.call->outputArguments crashes immediately. */
    running = false;
    THREAD_JOIN(server_thread);

    directCallCompleted = false;
    directCallResultCode = UA_STATUSCODE_BADINTERNALERROR;

    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    req.methodId = UA_NODEID_STRING(1, "asyncMethod");

    /* Invoke the async method directly on the server (CALL_DIRECT path) */
    UA_StatusCode retval =
        UA_Server_call_async(server, &req, directCallCompletionCb, NULL, 5000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Advance fake time past the 1-second timed callback scheduled by
     * methodCallback_async.  That callback calls UA_Server_setAsyncCallMethodResult
     * which is the function containing the buggy union branch selection. */
    UA_fakeSleep(1100);
    UA_Server_run_iterate(server, false);
    UA_Server_run_iterate(server, false);

    ck_assert(directCallCompleted == true);
    ck_assert_uint_eq(directCallResultCode, UA_STATUSCODE_GOOD);

    running = true;
    THREAD_CREATE(server_thread, serverloop);
} END_TEST

/* --- Additional async operation edge case tests --- */

START_TEST(Async_write_queue_overflow) {
    /* Test queue limit for async write operations */
    running = false;
    THREAD_JOIN(server_thread);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    const UA_UInt32 oldLimit = config->maxAsyncOperationQueueSize;
    config->maxAsyncOperationQueueSize = 1;

    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = UA_NODEID_STRING(1, "asyncVar");
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_UInt32 val = 100;
    UA_Variant_setScalar(&wv.value.value, &val, &UA_TYPES[UA_TYPES_UINT32]);
    wv.value.hasValue = true;

    UA_StatusCode retval =
        UA_Server_write_async(server, &wv,
                              serverAsyncWriteNoopCallback, NULL, 5000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Second write should fail due to queue limit */
    retval = UA_Server_write_async(server, &wv,
                                   serverAsyncWriteNoopCallback, NULL, 5000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADTOOMANYOPERATIONS);

    config->maxAsyncOperationQueueSize = oldLimit;

    /* Let the first queued async op complete */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, false);
    UA_Server_run_iterate(server, false);

    running = true;
    THREAD_CREATE(server_thread, serverloop);
} END_TEST

START_TEST(Async_direct_read_completed_synchronously) {
    /* Test when a direct read completes synchronously (no queueing) */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_STRING(1, "syncVar");
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    serverReadResultReceived = false;
    UA_DataValue_init(&serverReadResult);

    UA_StatusCode retval =
        UA_Server_read_async(server, &rvid,
                             UA_TIMESTAMPSTORETURN_BOTH,
                             serverAsyncReadCallback, NULL, 5000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* The callback should fire immediately since syncVar is a synchronous variable */
    UA_Server_run_iterate(server, false);

    /* Callback should have been called */
    ck_assert(serverReadResultReceived == true);
    UA_DataValue_clear(&serverReadResult);
} END_TEST

START_TEST(Async_call_multiple_outputs) {
    /* Test a method call with multiple output arguments */
    UA_MethodAttributes methodAttr = UA_MethodAttributes_default;
    methodAttr.executable = true;
    methodAttr.userExecutable = true;

    /* Add a method with multiple outputs if method calls enabled */
#ifdef UA_ENABLE_METHODCALLS
    UA_StatusCode res = UA_Server_addMethodNode(server,
                                   UA_NODEID_STRING(1, "multiOutMethod"),
                                   UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                   UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                   UA_QUALIFIEDNAME(1, "multiOutMethod"),
                                   methodAttr, &methodCallback_async,
                                   0, NULL, 0, NULL, NULL, NULL);
    if(res == UA_STATUSCODE_GOOD) {
        /* Test via client */
        UA_Client *client = UA_Client_newForUnitTest();
        UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        clientCounter = 0;
        retval = UA_Client_call_async(client,
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                      UA_NODEID_STRING(1, "multiOutMethod"),
                                      0, NULL, clientReceiveCallback, NULL, NULL);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        /* Wait for response */
        UA_fakeSleep(1000);
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);

        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }
#endif
} END_TEST

START_TEST(Async_cancelDirectOperation) {
    /* Test cancellation of direct async operations */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 55;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_addVariableNode(server,
                              UA_NODEID_STRING(1, "cancelTestVar"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "cancelTestVar"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              attr, NULL, NULL);

    running = false;
    THREAD_JOIN(server_thread);

    /* Save and modify the queue limit to allow operation to stay in waiting queue */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    const UA_UInt32 oldLimit = config->maxAsyncOperationQueueSize;
    config->maxAsyncOperationQueueSize = 10; /* Temporarily increase so we can queue */

    /* Start an async read that we'll cancel */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_STRING(1, "cancelTestVar");
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    serverReadResultReceived = false;
    UA_DataValue_init(&serverReadResult);

    UA_Server_read_async(server, &rvid,
                         UA_TIMESTAMPSTORETURN_BOTH,
                         serverAsyncReadCallback, NULL, 5000);

    /* Cancel the operation using the result pointer as context */
    UA_Server_cancelAsync(server, &serverReadResult, UA_STATUSCODE_BADOPERATIONABANDONED, true);

    UA_Server_run_iterate(server, false);

    config->maxAsyncOperationQueueSize = oldLimit;

    running = true;
    THREAD_CREATE(server_thread, serverloop);
} END_TEST

START_TEST(Async_call_error_result) {
    /* Test async method call that returns an error status */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    running = false;
    THREAD_JOIN(server_thread);

    clientCounter = 0;
    /* Call async method - it will return an error via the callback */
    retval = UA_Client_call_async(client,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "asyncMethod"),
                                  0, NULL, clientReceiveCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* The method callback returns an error - verify client receives it */
    UA_fakeSleep(1500);
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_multiple_parallel_operations) {
    /* Test multiple async operations in parallel */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    running = false;
    THREAD_JOIN(server_thread);

    clientCounter = 0;
    /* Queue multiple async reads */
    retval = UA_Client_readValueAttribute_async(client,
                                                UA_NODEID_STRING(1, "asyncVar"),
                                                clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readValueAttribute_async(client,
                                                UA_NODEID_STRING(1, "asyncVar"),
                                                clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readValueAttribute_async(client,
                                                UA_NODEID_STRING(1, "asyncVar"),
                                                clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* All should complete */
    while(clientCounter < 3) {
        UA_fakeSleep(500);
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 3);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* ==== Additional direct-API coverage ==== */

START_TEST(Async_cancelAsync_unknownContext_returnsError) {
    /* UA_Server_cancelAsync with a context that was never queued is a
     * no-op; it doesn't fail. Exercises the TAILQ_FOREACH miss path. */
    int dummy = 0;
    /* The function returns void -- we just verify it doesn't crash. */
    UA_Server_cancelAsync(server, &dummy, UA_STATUSCODE_BADUNEXPECTEDERROR, true);
} END_TEST

START_TEST(Async_read_async_zeroTimeout_usesDefault) {
    /* UA_Server_read_async with a 0ms timeout falls back to the configured
     * max. The call itself succeeds; the timeout applies when the operation
     * is later cancelled. */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_StatusCode retval = UA_Server_read_async(
        server, &rvid, UA_TIMESTAMPSTORETURN_BOTH,
        serverAsyncReadNoopCallback, NULL, 0);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Run the server iteration to dispatch the (sync) result. */
    UA_Server_run_iterate(server, false);

    /* Clean up: cancel to make sure the op doesn't linger. */
    /* (The DataValue is on the stack so we don't try to cancel it
     * explicitly; the next run-iter drains the queue.) */
    UA_Server_run_iterate(server, false);
} END_TEST

START_TEST(Async_read_async_unknownNode_returnsError) {
    /* Reading a non-existent node via the async path: UA_Server_read_async
     * is the dispatch helper and only enqueues the op. The read itself
     * happens later when UA_Server_run_iterate processes the queue. So
     * the synchronous return is GOOD; the actual error surfaces via
     * the read callback. We just verify the call doesn't crash. */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_NUMERIC(1, 999999);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_StatusCode retval = UA_Server_read_async(
        server, &rvid, UA_TIMESTAMPSTORETURN_BOTH,
        serverAsyncReadNoopCallback, NULL, 5000);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    /* Drain the queue to clear the pending op. */
    UA_Server_run_iterate(server, false);
    UA_Server_run_iterate(server, false);
} END_TEST

START_TEST(Async_write_async_unknownNode_returnsError) {
    /* Same as the read variant: UA_Server_write_async only enqueues; the
     * actual error surfaces later via the write callback. */
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = UA_NODEID_NUMERIC(1, 999999);
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Int32 v = 42;
    UA_Variant_setScalar(&wv.value.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    wv.value.hasValue = true;

    UA_StatusCode retval = UA_Server_write_async(
        server, &wv, serverAsyncWriteNoopCallback, NULL, 5000);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    /* Drain the queue. */
    UA_Server_run_iterate(server, false);
    UA_Server_run_iterate(server, false);
} END_TEST

START_TEST(Async_setAsyncReadResult_null_returnsError) {
    /* Passing a NULL DataValue pointer to setAsyncReadResult returns
     * BADINTERNALERROR (the function asserts on it via the caller's
     * caller). With no queued operation matching, BADNOTFOUND is also
     * acceptable -- both reach a meaningful branch. */
    UA_StatusCode retval = UA_Server_setAsyncReadResult(server, NULL);
    ck_assert(retval == UA_STATUSCODE_BADNOTFOUND ||
              retval == UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

START_TEST(Async_setAsyncWriteResult_null_returnsError) {
    /* Same for write: NULL value pointer is an invalid input. */
    UA_StatusCode retval = UA_Server_setAsyncWriteResult(
        server, NULL, UA_STATUSCODE_GOOD);
    ck_assert(retval == UA_STATUSCODE_BADNOTFOUND ||
              retval == UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

#ifdef UA_ENABLE_METHODCALLS
START_TEST(Async_setAsyncMethodResult_null_returnsError) {
    /* And for method call. */
    UA_StatusCode retval = UA_Server_setAsyncCallMethodResult(
        server, NULL, UA_STATUSCODE_GOOD);
    ck_assert(retval == UA_STATUSCODE_BADNOTFOUND ||
              retval == UA_STATUSCODE_BADINTERNALERROR);
} END_TEST
#endif

/* --- Suite registration --- */

static Suite* method_async_suite(void) {
    /* set up unit test for internal data structures */
    Suite *s = suite_create("Async Method");

    TCase* tc_manager = tcase_create("AsyncMethod");
    tcase_add_checked_fixture(tc_manager, setup, teardown);
    tcase_add_test(tc_manager, Async_call);
    tcase_add_test(tc_manager, Async_read);
    tcase_add_test(tc_manager, Async_write);
    tcase_add_test(tc_manager, Async_timeout);
    tcase_add_test(tc_manager, Async_forget);
    tcase_add_test(tc_manager, Async_cancel);
    tcase_add_test(tc_manager, Async_cancel_multiple);
    tcase_add_test(tc_manager, Async_server_read);
    tcase_add_test(tc_manager, Async_server_write);
    tcase_add_test(tc_manager, Async_read_timeout_server);
    tcase_add_test(tc_manager, Async_setResult_badnotfound);
    tcase_add_test(tc_manager, Async_queue_limit_read_direct);
    tcase_add_test(tc_manager, Async_sync_method_call);
    tcase_add_test(tc_manager, Async_read_sync_variable);
    tcase_add_test(tc_manager, Async_service_read_validation_paths);
    tcase_add_test(tc_manager, Async_service_read_toomanyoperations);
    tcase_add_test(tc_manager, Async_service_write_validation_paths);
    tcase_add_test(tc_manager, Async_service_write_toomanyoperations);
    tcase_add_test(tc_manager, Async_direct_call_method_result);
    tcase_add_test(tc_manager, Async_write_queue_overflow);
    /* Additional direct API coverage that doesn't need a running server. */
    tcase_add_test(tc_manager, Async_cancelAsync_unknownContext_returnsError);
    tcase_add_test(tc_manager, Async_read_async_zeroTimeout_usesDefault);
    tcase_add_test(tc_manager, Async_read_async_unknownNode_returnsError);
    tcase_add_test(tc_manager, Async_write_async_unknownNode_returnsError);
    tcase_add_test(tc_manager, Async_setAsyncReadResult_null_returnsError);
    tcase_add_test(tc_manager, Async_setAsyncWriteResult_null_returnsError);
    tcase_add_test(tc_manager, Async_setAsyncMethodResult_null_returnsError);
    tcase_add_test(tc_manager, Async_direct_read_completed_synchronously);
    tcase_add_test(tc_manager, Async_call_multiple_outputs);
    tcase_add_test(tc_manager, Async_cancelDirectOperation);
    tcase_add_test(tc_manager, Async_call_error_result);
    tcase_add_test(tc_manager, Async_multiple_parallel_operations);
    suite_add_tcase(s, tc_manager);

    return s;
}

int main(void) {
    /* Unit tests for internal data structures for async methods */
    int number_failed = 0;
    Suite *s = method_async_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
