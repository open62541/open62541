/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This example is just to see how fast we can process messages. The server does
   not open a TCP port. */

#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include "testing_clock.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

#include "ua_server_internal.h"
#include "ua_services.h"

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
#include <time.h>

static UA_atomic(uintptr_t) running;
THREAD_HANDLE server_thread;
static UA_Server *server;
static size_t clientCounter;
static UA_UInt64 lastTimedCallback;
static UA_StatusCode closeFromReadResult;
static UA_Boolean closeAtServiceAsync;
static UA_StatusCode closeAtServiceAsyncResult;
static size_t closeServiceAsyncCount;
static size_t closeServiceEndCount;

static const void *canceledCallRequest = NULL;
static const void *expectedCanceledCallRequest = NULL;
static UA_Boolean completeCanceledRead;
static UA_StatusCode completeCanceledReadResult;
static UA_Boolean acknowledgeCancellation;

// Store active async reads and remove when cancelled
static void *activeReads[16];
static void *activeCalls[16];

static void
asyncOperationCancelCallback(UA_Server *server, const void *out) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Request %p was canceled", out);
    canceledCallRequest = out;
    for(size_t i = 0; i < 16; i++) {
        if(activeReads[i] == out) {
            activeReads[i] = NULL;
            if(!acknowledgeCancellation)
                return;
            UA_StatusCode res = UA_Server_setAsyncReadResult(
                server, (UA_DataValue*)(uintptr_t)out);
            if(completeCanceledRead) {
                completeCanceledRead = false;
                completeCanceledReadResult = res;
            }
            return;
        }
    }
    for(size_t i = 0; i < 16; i++) {
        if(activeCalls[i] == out) {
            activeCalls[i] = NULL;
            UA_Server_setAsyncCallMethodResult(
                server, (UA_Variant*)(uintptr_t)out,
                UA_STATUSCODE_BADOPERATIONABANDONED);
            return;
        }
    }
}

static void
closeFromAsyncServiceNotification(
    UA_Server *server, UA_ApplicationNotificationType type,
    const UA_KeyValueMap payload) {
    if(type == UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_END) {
        closeServiceEndCount++;
        return;
    }
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_ASYNC)
        return;

    closeServiceAsyncCount++;
    if(!closeAtServiceAsync)
        return;
    closeAtServiceAsync = false;
    const UA_NodeId *sessionId = (const UA_NodeId*)payload.map[1].value.data;
    closeAtServiceAsyncResult = UA_Server_closeSession(server, sessionId);
    ck_assert_uint_eq(closeServiceEndCount, 1);
}

static void
asyncRead(UA_Server *server, void *data) {
    /* Already cancelled? */
    size_t i = 0;
    for(;i < 16; i++) {
        if(activeReads[i] == data)
            break;
    }
    if(i >= 16)
        return;

    activeReads[i] = NULL; /* Free the slot*/

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
    size_t i = 0;
    for(;i < 16; i++) {
        if(activeReads[i] == NULL)
            break;
    }
    if(i >= 16)
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;

    UA_DateTime callTime = UA_DateTime_now_fake(NULL) + UA_DATETIME_SEC;
    UA_Server_addTimedCallback(server, asyncRead, value, callTime, &lastTimedCallback);
    activeReads[i] = value; /* store to see if canceled */
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static UA_StatusCode
readCallback_closeSession(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *nodeId,
                          void *nodeContext, UA_Boolean includeSourceTimeStamp,
                          const UA_NumericRange *range, UA_DataValue *value) {
    closeFromReadResult = UA_Server_closeSession(server, sessionId);
    return UA_STATUSCODE_GOOD;
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
    size_t i = 0;
    for(; i < 16; i++) {
        if(activeCalls[i] == out)
            break;
    }
    if(i == 16)
        return;
    activeCalls[i] = NULL;
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
    size_t i = 0;
    for(; i < 16; i++) {
        if(!activeCalls[i])
            break;
    }
    if(i == 16)
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;
    activeCalls[i] = output;
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

THREAD_CALLBACK(serverloop) {
    while(UA_atomic_load(&running))
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    clientCounter = 0;
    canceledCallRequest = NULL;
    expectedCanceledCallRequest = NULL;
    completeCanceledRead = false;
    acknowledgeCancellation = true;
    closeAtServiceAsync = false;
    closeServiceAsyncCount = 0;
    closeServiceEndCount = 0;
    memset(activeReads, 0, sizeof(activeReads));
    memset(activeCalls, 0, sizeof(activeCalls));
    UA_atomic_store(&running, true);
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->asyncOperationTimeout = 2000.0; /* 2 seconds */
    config->asyncOperationCancelCallback = asyncOperationCancelCallback;

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

    /* Variable that closes the calling Session from its read callback */
    UA_CallbackValueSource closeSessionSource = {readCallback_closeSession, NULL};
    res = UA_Server_addVariableNode(server,
                                    UA_NODEID_STRING(1, "closeSessionVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "closeSessionVar"),
                                    UA_NS0ID(BASEDATAVARIABLETYPE),
                                    varAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Server_setVariableNode_callbackValueSource(
        server, UA_NODEID_STRING(1, "closeSessionVar"), closeSessionSource);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    if(UA_atomic_load(&running)) {
        UA_atomic_store(&running, false);
        THREAD_JOIN(server_thread);
    }
    if(UA_Server_getLifecycleState(server) == UA_LIFECYCLESTATE_STARTED)
        UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Async_call) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    UA_atomic_store(&running, false);
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
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    /* Iterate and pick up the async response to be sent out */
    UA_fakeSleep(1000);
    while(clientCounter == 1) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 2);

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_read) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    UA_atomic_store(&running, false);
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
        UA_Server_run_iterate(server, false);
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

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_multiRead_closingSessionCancelsPendingOperation) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_ReadValueId nodes[2];
    UA_ReadValueId_init(&nodes[0]);
    nodes[0].nodeId = UA_NODEID_STRING(1, "asyncVar");
    nodes[0].attributeId = UA_ATTRIBUTEID_VALUE;
    UA_ReadValueId_init(&nodes[1]);
    nodes[1].nodeId = UA_NODEID_STRING(1, "closeSessionVar");
    nodes[1].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToRead = nodes;
    request.nodesToReadSize = 2;

    closeFromReadResult = UA_STATUSCODE_BADUNEXPECTEDERROR;
    canceledCallRequest = NULL;
    completeCanceledRead = true;
    completeCanceledReadResult = UA_STATUSCODE_BADUNEXPECTEDERROR;
    UA_ReadResponse response = UA_Client_Service_read(client, request);
    ck_assert_uint_eq(closeFromReadResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_BADSESSIONCLOSED);
    /* The response can arrive before the subsequent cancellation notification.
     * Join the server thread before inspecting its application-owned state. */
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    for(size_t i = 0; i < 20 && !canceledCallRequest; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_ptr_nonnull(canceledCallRequest);
    ck_assert_uint_eq(completeCanceledReadResult, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 16; i++)
        ck_assert_ptr_null(activeReads[i]);
    UA_ReadResponse_clear(&response);

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_serviceNotificationCloseCancelsPersistedResponse) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_Client_getConfig(client)->noReconnect = true;
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->serviceNotificationCallback = closeFromAsyncServiceNotification;
    closeAtServiceAsync = true;
    closeAtServiceAsyncResult = UA_STATUSCODE_BADUNEXPECTEDERROR;
    canceledCallRequest = NULL;
    completeCanceledRead = true;
    completeCanceledReadResult = UA_STATUSCODE_BADUNEXPECTEDERROR;

    retval = UA_Client_readValueAttribute_async(
        client, UA_NODEID_STRING(1, "asyncVar"),
        clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    for(size_t i = 0;
        i < 20 && closeAtServiceAsyncResult == UA_STATUSCODE_BADUNEXPECTEDERROR;
        i++) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(closeAtServiceAsyncResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(closeServiceAsyncCount, 1);
    for(size_t i = 0; i < 20 && !canceledCallRequest; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_ptr_nonnull(canceledCallRequest);
    ck_assert_uint_eq(completeCanceledReadResult, UA_STATUSCODE_GOOD);

    /* Session closure already emitted SERVICE_END. Completion must not emit
     * another notification. */
    ck_assert_uint_eq(closeServiceEndCount, 1);

    lockServer(server);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    ck_assert(TAILQ_EMPTY(&server->asyncManager.operations));
    unlockServer(server);

    config->serviceNotificationCallback = NULL;
    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

static size_t abandonedServiceStage;
static UA_UInt32 abandonedRequestId;

static void
abandonFromServiceNotification(UA_Server *serverArg,
                               UA_ApplicationNotificationType type,
                               const UA_KeyValueMap payload) {
    const UA_UInt32 *requestId = (const UA_UInt32*)payload.map[2].value.data;
    if(type == UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_BEGIN) {
        ck_assert_uint_eq(abandonedServiceStage, 0);
        abandonedRequestId = *requestId;
        abandonedServiceStage = 1;
    } else if(type == UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_ASYNC) {
        ck_assert_uint_eq(abandonedServiceStage, 1);
        ck_assert_uint_eq(*requestId, abandonedRequestId);
        const UA_NodeId *sessionId = (const UA_NodeId*)payload.map[1].value.data;
        UA_Session *session = getSessionById(serverArg, sessionId);
        ck_assert_ptr_nonnull(session);
        /* For TCP, the response token is the request id. Abandon only the
         * response carrier, leaving the service and its session alive. */
        abandonServiceRequest(serverArg, session->channel, *requestId);
        abandonedServiceStage = 2;
    } else if(type == UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_END) {
        ck_assert_uint_eq(abandonedServiceStage, 2);
        ck_assert_uint_eq(*requestId, abandonedRequestId);
        abandonedServiceStage = 3;
    }
}

START_TEST(Async_abandonedResponseStillNotifiesServiceEnd) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connect(client, "opc.tcp://localhost:4840"),
                      UA_STATUSCODE_GOOD);
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    abandonedServiceStage = 0;
    clientCounter = 0;
    server->config.serviceNotificationCallback = abandonFromServiceNotification;

    ck_assert_uint_eq(UA_Client_readValueAttribute_async(
        client, UA_NODEID_STRING(1, "asyncVar"), clientReadCallback, NULL, NULL),
        UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 20 && abandonedServiceStage < 2; i++) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(abandonedServiceStage, 2);
    ck_assert_ptr_nonnull(activeReads[0]);
    UA_Server_removeCallback(server, lastTimedCallback);
    asyncRead(server, activeReads[0]);
    for(size_t i = 0; i < 20; i++) {
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(abandonedServiceStage, 3);
    ck_assert_uint_eq(clientCounter, 0); /* No response was transmitted. */

    server->config.serviceNotificationCallback = NULL;
    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_write) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    UA_atomic_store(&running, false);
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
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    /* Delivery can require another socket poll, especially with lwIP. */
    UA_fakeSleep(1000);
    for(size_t attempt = 0; attempt < 100 && clientCounter == 1; attempt++) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 10);
    }
    ck_assert_uint_eq(clientCounter, 2);

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_timeout) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    UA_atomic_store(&running, false);
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

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_forget) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    UA_atomic_store(&running, false);
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

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_cancel) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

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

    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    ck_assert_ptr_eq(expectedCanceledCallRequest, canceledCallRequest);

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);
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

START_TEST(Async_request_handles_are_per_response) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    server->config.asyncOperationCancelCallback = NULL;
    UA_ReadValueId item;
    UA_ReadValueId_init(&item);
    item.nodeId = UA_NODEID_STRING(1, "asyncVar");
    item.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = &item;
    request.nodesToReadSize = 1;
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;

    lockServer(server);
    for(UA_UInt32 handle = 0; handle < 2; handle++) {
        request.requestHeader.requestHandle = handle;
        UA_ReadResponse response;
        UA_ReadResponse_init(&response);
        ck_assert(!Service_Read(server, &server->adminSession, &request, &response));
        UA_ReadResponse_clear(&response);
    }
    UA_AsyncResponse *first = TAILQ_FIRST(&server->asyncManager.responses);
    UA_AsyncResponse *second = TAILQ_NEXT(first, pointers);
    ck_assert_ptr_nonnull(second);
    ck_assert_uint_eq(first->response.readResponse.responseHeader.requestHandle, 0);
    ck_assert_uint_eq(second->response.readResponse.responseHeader.requestHandle, 1);
    ck_assert_uint_eq(UA_AsyncManager_cancel(server, &server->adminSession, 0), 1);
    ck_assert_uint_eq(first->pendingResults, 0);
    ck_assert_uint_eq(second->pendingResults, 1);
    ck_assert_uint_eq(UA_AsyncManager_cancel(server, &server->adminSession, 0), 0);
    ck_assert_uint_eq(UA_AsyncManager_cancel(server, &server->adminSession, 1), 1);
    unlockServer(server);

    UA_fakeSleep(2000);
    for(size_t i = 0; i < 4; i++)
        UA_Server_run_iterate(server, false);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    ck_assert(TAILQ_EMPTY(&server->asyncManager.operations));
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
    UA_atomic_store(&running, false);
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

    UA_atomic_store(&running, true);
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
    UA_atomic_store(&running, false);
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

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);
} END_TEST

START_TEST(Async_read_timeout_server) {
    /* Start async read with very short timeout, remove the callback so it times out */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);

    clientCounter = 0;
    retval = UA_Client_readValueAttribute_async(client,
                                                UA_NODEID_STRING(1, "asyncVar"),
                                                clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Process the request on the server to start the async op */
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);

    UA_DataValue *workerResult = NULL;
    for(size_t i = 0; i < 16; i++) {
        if(activeReads[i]) {
            workerResult = (UA_DataValue*)activeReads[i];
            break;
        }
    }
    ck_assert_ptr_nonnull(workerResult);

    /* Remove the timed callback so it never completes */
    UA_Server_removeCallback(server, lastTimedCallback);
    acknowledgeCancellation = false;

    /* Wait for the async timeout (2 seconds).
     * Under lwip with TAP networking the response may need
     * multiple iterations to be delivered. */
    UA_fakeSleep(3000);
    while(clientCounter == 0) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    /* Encoding the response does not touch the operation's output, which
     * remains valid until ownership is returned through the setter. */
    UA_UInt32 value = 42;
    UA_Variant_setScalarCopy(&workerResult->value, &value,
                             &UA_TYPES[UA_TYPES_UINT32]);
    workerResult->hasValue = true;
    retval = UA_Server_setAsyncReadResult(server, workerResult);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Server_setAsyncReadResult(server, workerResult);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
    acknowledgeCancellation = true;

    UA_atomic_store(&running, true);
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
    UA_atomic_store(&running, false);
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

    UA_atomic_store(&running, true);
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

START_TEST(Async_service_read_allocation_size_overflow) {
    UA_ReadValueId node;
    UA_ReadValueId_init(&node);

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToRead = &node;
    request.nodesToReadSize =
        SIZE_MAX / UA_TYPES[UA_TYPES_DATAVALUE].memSize + 1;

    UA_ReadResponse response;
    UA_ReadResponse_init(&response);

    lockServer(server);
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_UInt32 oldMaxNodesPerRead = config->maxNodesPerRead;
    config->maxNodesPerRead = 0;
    UA_Boolean done = Service_Read(server, &server->adminSession,
                                   &request, &response);
    config->maxNodesPerRead = oldMaxNodesPerRead;
    unlockServer(server);

    ck_assert(done);
    ck_assert_uint_eq(response.responseHeader.serviceResult,
                      UA_STATUSCODE_BADOUTOFMEMORY);
    ck_assert_ptr_null(response.results);
    ck_assert_uint_eq(response.resultsSize, 0);
    UA_ReadResponse_clear(&response);
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
    /* Complete a local method using the operation's inline result storage. */
    UA_atomic_store(&running, false);
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

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);
} END_TEST

/* --- Additional async operation edge case tests --- */

START_TEST(Async_write_queue_overflow) {
    /* Test queue limit for async write operations */
    UA_atomic_store(&running, false);
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

    UA_atomic_store(&running, true);
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

START_TEST(Async_shutdown_waits_for_direct_operation) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);

    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_StatusCode res =
        UA_Server_read_async(server, &rvid, UA_TIMESTAMPSTORETURN_NEITHER,
                             serverAsyncReadNoopCallback, NULL, 0);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_DataValue *workerResult = NULL;
    for(size_t i = 0; i < 16; i++) {
        if(activeReads[i]) {
            workerResult = (UA_DataValue*)activeReads[i];
            break;
        }
    }
    ck_assert_ptr_nonnull(workerResult);

    /* Shutdown cancels the local operation, but stays in STOPPING until the
     * worker hands its result slot back. */
    acknowledgeCancellation = false;
    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->externalEventLoop = true;
    res = UA_Server_run_shutdown(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getLifecycleState(server),
                     UA_LIFECYCLESTATE_STOPPING);

    res = UA_Server_setAsyncReadResult(server, workerResult);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    for(size_t i = 0;
        i < 100 && UA_Server_getLifecycleState(server) !=
                   UA_LIFECYCLESTATE_STOPPED;
        i++)
        UA_Server_run_iterate(server, false);
    ck_assert_int_eq(UA_Server_getLifecycleState(server),
                     UA_LIFECYCLESTATE_STOPPED);
    config->externalEventLoop = false;
} END_TEST

/* A network CancelRequest must ignore locally initiated direct async
 * operations that share the async-manager queue. */
START_TEST(Async_service_cancel_with_direct_operation) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(client);
    UA_StatusCode retval =
        UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);

    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;
    retval = UA_Server_read_async(server, &rvid,
                                  UA_TIMESTAMPSTORETURN_BOTH,
                                  serverAsyncReadNoopCallback, NULL, 5000);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);

    UA_UInt32 cancelCount = 0;
    retval = UA_Client_cancelByRequestHandle(client, 0x12345678, &cancelCount);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(cancelCount, 0);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

#ifdef UA_ENABLE_SUBSCRIPTIONS
static void
unexpectedDataChange(UA_Server *serverArg, UA_UInt32 monitoredItemId,
                     void *monitoredItemContext, const UA_NodeId *nodeId,
                     void *nodeContext, UA_UInt32 attributeId,
                     const UA_DataValue *value) {
    ck_abort_msg("Deleted monitored item delivered a sample");
}

/* Deleting a monitored item cancels neither its sample nor a request-backed
 * read in the shared queue. Only the service result is delivered afterwards. */
START_TEST(Async_monitored_item_deletion_with_service_operation) {
    UA_Client *client = UA_Client_newForUnitTest();
    ck_assert_ptr_nonnull(client);
    UA_StatusCode retval =
        UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);

    retval = UA_Client_readValueAttribute_async(
        client, UA_NODEID_STRING(1, "asyncVar"),
        clientReadCallback, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_DateTime deadline = UA_DateTime_nowMonotonic() + 10 * UA_DATETIME_SEC;
    while(server->asyncManager.trackedOpsCount == 0) {
        ck_assert(UA_DateTime_nowMonotonic() < deadline);
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }

    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 1);
    UA_AsyncOperation *serviceOp = TAILQ_FIRST(&server->asyncManager.operations);
    ck_assert_ptr_nonnull(serviceOp);
    ck_assert_int_eq(serviceOp->asyncOperationType, UA_ASYNCOPERATIONTYPE_READ_REQUEST);
    UA_MonitoredItemCreateRequest request =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(1, "asyncVar"));
    request.requestedParameters.samplingInterval = 10000;
    UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_NEITHER, request, NULL, unexpectedDataChange);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 2);
    ck_assert_uint_eq(UA_Server_deleteMonitoredItem(server, result.monitoredItemId),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 5; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 2);
    ck_assert_ptr_eq(TAILQ_FIRST(&server->asyncManager.operations), serviceOp);

    UA_fakeSleep(1001);
    deadline = UA_DateTime_nowMonotonic() + 10 * UA_DATETIME_SEC;
    while(server->asyncManager.trackedOpsCount > 0 || clientCounter == 0) {
        ck_assert(UA_DateTime_nowMonotonic() < deadline);
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    ck_assert_uint_eq(clientCounter, 1);

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST
#endif

START_TEST(Async_call_error_result) {
    /* Test async method call that returns an error status */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_atomic_store(&running, false);
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

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

START_TEST(Async_multiple_parallel_operations) {
    /* Test multiple async operations in parallel */
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_atomic_store(&running, false);
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

    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

/* ==== Additional direct-API coverage ==== */

START_TEST(Async_read_async_zeroTimeout_disablesTimeout) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    server->config.asyncOperationTimeout = 1;
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_uint_eq(UA_Server_read_async(server, &rvid, UA_TIMESTAMPSTORETURN_BOTH,
                                           serverAsyncReadNoopCallback, NULL, 0),
                      UA_STATUSCODE_GOOD);
    UA_Server_removeCallback(server, lastTimedCallback);
    UA_fakeSleep(2000);
    UA_Server_run_iterate(server, false);
    ck_assert_ptr_null(canceledCallRequest);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 1);
    ck_assert_ptr_nonnull(activeReads[0]);
    asyncRead(server, activeReads[0]);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
} END_TEST

START_TEST(Async_read_async_unknownNode_returnsError) {
    /* Dispatch succeeds; the unknown-node result is delivered inline. */
    serverReadResultReceived = false;
    UA_DataValue_init(&serverReadResult);
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_NUMERIC(1, 999999);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_StatusCode retval = UA_Server_read_async(
        server, &rvid, UA_TIMESTAMPSTORETURN_BOTH,
        serverAsyncReadCallback, NULL, 5000);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(serverReadResultReceived);
    ck_assert_uint_eq(serverReadResult.status, UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_DataValue_clear(&serverReadResult);
} END_TEST

START_TEST(Async_write_async_unknownNode_returnsError) {
    /* The write error is likewise delivered before the API returns. */
    serverWriteResultReceived = false;
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = UA_NODEID_NUMERIC(1, 999999);
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Int32 v = 42;
    UA_Variant_setScalar(&wv.value.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    wv.value.hasValue = true;

    UA_StatusCode retval = UA_Server_write_async(
        server, &wv, serverAsyncWriteCallback, NULL, 5000);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(serverWriteResultReceived);
    ck_assert_uint_eq(serverWriteResultCode, UA_STATUSCODE_BADNODEIDUNKNOWN);
} END_TEST

START_TEST(Async_setAsyncReadResult_null_returnsError) {
    /* NULL does not identify any outstanding operation. */
    UA_StatusCode retval = UA_Server_setAsyncReadResult(server, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
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

/* A timed-out worker can acknowledge from inside its result callback. */
typedef struct {
    UA_DataValue *workers[2];
    size_t callbacks;
} ReentrantResults;

static void
acknowledgeFromResultCallback(UA_Server *serverArg, void *context,
                              const UA_DataValue *result) {
    ReentrantResults *results = (ReentrantResults*)context;
    size_t index = results->callbacks++;
    ck_assert_uint_lt(index, 2);
    ck_assert_uint_eq(result->status, UA_STATUSCODE_BADTIMEOUT);
    activeReads[index] = NULL;
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(serverArg, results->workers[index]),
                      UA_STATUSCODE_GOOD);
}

START_TEST(Async_result_callback_acknowledges_timed_out_worker) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    acknowledgeCancellation = false;
    ReentrantResults results = {{NULL, NULL}, 0};
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    for(size_t i = 0; i < 2; i++) {
        ck_assert_uint_eq(UA_Server_read_async(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER,
                                               acknowledgeFromResultCallback, &results, 100),
                          UA_STATUSCODE_GOOD);
        results.workers[i] = (UA_DataValue*)activeReads[i];
        UA_Server_removeCallback(server, lastTimedCallback);
    }
    UA_fakeSleep(1500);
    for(size_t i = 0; i < 3 && results.callbacks < 2; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(results.callbacks, 2);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    ck_assert_ptr_null(canceledCallRequest); /* Both workers acknowledged during delivery. */
} END_TEST

static size_t localResultCount;
static UA_StatusCode localResultStatus;

static void
recordLocalReadResult(UA_Server *serverArg, void *context, const UA_DataValue *result) {
    localResultCount++;
    localResultStatus = result->hasStatus ? result->status : UA_STATUSCODE_GOOD;
}

/* A ready local result lives in the EventLoop queue, not the outstanding
 * operation list. Cancellation may return ownership before or after delivery. */
START_TEST(Async_local_delivery_lifetime) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    acknowledgeCancellation = false;
    localResultCount = 0;
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_uint_eq(UA_Server_read_async(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER,
                                           recordLocalReadResult, NULL, 0),
                      UA_STATUSCODE_GOOD);
    UA_Server_removeCallback(server, lastTimedCallback);
    UA_DataValue *output = (UA_DataValue*)activeReads[0];
    UA_AsyncOperation *op = TAILQ_FIRST(&server->asyncManager.operations);
    ck_assert_ptr_nonnull(op);
    ck_assert(op->handling.callback.dc.callback == NULL);

    if(_i != 0) {
        server->config.externalEventLoop = true;
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
        ck_assert_int_eq(server->asyncManager.driver.state, UA_LIFECYCLESTATE_STOPPING);
        ck_assert(op->handling.callback.dc.callback != NULL);
    }
    if(_i == 2) {
        UA_Server_run_iterate(server, false);
        ck_assert_uint_eq(localResultCount, 1);
        ck_assert_ptr_eq(TAILQ_FIRST(&server->asyncManager.operations), op);
        ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 1);
        ck_assert_ptr_eq(canceledCallRequest, output);
        ck_assert(op->resultIndex == SIZE_MAX);
        ck_assert(op->handling.callback.dc.callback == NULL);
    }

    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, output), UA_STATUSCODE_GOOD);
    activeReads[0] = NULL;
    ck_assert(TAILQ_EMPTY(&server->asyncManager.operations));
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, output), UA_STATUSCODE_BADNOTFOUND);
    if(_i != 2) {
        ck_assert_uint_eq(localResultCount, 0);
        ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 1);
        ck_assert(op->handling.callback.dc.callback != NULL);
        ck_assert_ptr_null(op->pointers.tqe_prev);
        if(_i == 1)
            ck_assert_int_eq(server->asyncManager.driver.state, UA_LIFECYCLESTATE_STOPPING);
    }
    for(size_t i = 0; i < 3; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(localResultCount, 1);
    ck_assert_uint_eq(localResultStatus, _i == 0 ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADSHUTDOWN);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    if(_i != 2)
        ck_assert_ptr_null(canceledCallRequest);
    if(_i != 0) {
        ck_assert_int_eq(server->asyncManager.driver.state, UA_LIFECYCLESTATE_STOPPED);
        server->config.externalEventLoop = false;
    }
} END_TEST

START_TEST(Async_shutdown_delivers_already_ready_results) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    localResultCount = 0;
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_uint_eq(UA_Server_read_async(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER,
                                           recordLocalReadResult, NULL, 0),
                      UA_STATUSCODE_GOOD);
    UA_Server_removeCallback(server, lastTimedCallback);
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, (UA_DataValue*)activeReads[0]),
                      UA_STATUSCODE_GOOD);
    activeReads[0] = NULL;
    ck_assert_uint_eq(localResultCount, 0);
    server->config.externalEventLoop = true;
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 100 && UA_Server_getLifecycleState(server) !=
        UA_LIFECYCLESTATE_STOPPED; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(localResultCount, 1);
    ck_assert_uint_eq(localResultStatus, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    server->config.externalEventLoop = false;
} END_TEST

START_TEST(Async_local_timeout_without_global_timeout) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    localResultCount = 0;
    server->config.asyncOperationTimeout = 0;
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_uint_eq(UA_Server_read_async(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER,
                                           recordLocalReadResult, NULL, 100),
                      UA_STATUSCODE_GOOD);
    UA_Server_removeCallback(server, lastTimedCallback);
    UA_fakeSleep(1500);
    for(size_t i = 0; i < 3 && localResultCount == 0; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(localResultCount, 1);
    ck_assert_uint_eq(localResultStatus, UA_STATUSCODE_BADTIMEOUT);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
} END_TEST

static size_t batchCancelNotifications;
static UA_Boolean driveDuringBatchCancel;

static size_t scanCallbacks[16];

static void
completeEarlierOperation(UA_Server *serverArg, void *context, const UA_DataValue *result) {
    size_t index = (size_t)(uintptr_t)context;
    ck_assert_uint_lt(index, 16);
    ck_assert_uint_eq(++scanCallbacks[index], 1);
    ck_assert_uint_eq(result->status, index == 0 || index == 15 ?
                      UA_STATUSCODE_GOOD : UA_STATUSCODE_BADSHUTDOWN);
    if(index == 15) {
        ck_assert_uint_eq(scanCallbacks[0], 0);
        asyncRead(serverArg, activeReads[0]);
    }
}

START_TEST(Async_result_callback_queues_another_result) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    memset(scanCallbacks, 0, sizeof(scanCallbacks));
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    for(size_t i = 0; i < 16; i++) {
        ck_assert_uint_eq(UA_Server_read_async(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER,
                                               completeEarlierOperation, (void*)(uintptr_t)i, 0),
                          UA_STATUSCODE_GOOD);
        UA_Server_removeCallback(server, lastTimedCallback);
    }
    /* Delivery queues another result for a following EventLoop cycle. */
    asyncRead(server, activeReads[15]);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(scanCallbacks[15], 1);
    for(size_t i = 0; i < 3 && scanCallbacks[0] == 0; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(scanCallbacks[0], 1);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 14);

    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 16; i++)
        ck_assert_uint_eq(scanCallbacks[i], 1);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.operations));
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
} END_TEST

static void
acknowledgeAllReads(UA_Server *serverArg, const void *out) {
    batchCancelNotifications++;
    UA_AsyncResponse *response = TAILQ_FIRST(&serverArg->asyncManager.responses);
    ck_assert_ptr_nonnull(response);
    ck_assert(!response->dc.callback);
    ck_assert_ptr_null(response->response.readResponse.results);
    ck_assert_uint_eq(response->response.readResponse.resultsSize, 0);
    for(size_t i = 0; i < 16; i++) {
        if(!activeReads[i])
            continue;
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(serverArg, (UA_DataValue*)activeReads[i]),
                          UA_STATUSCODE_GOOD);
        activeReads[i] = NULL;
    }
    if(driveDuringBatchCancel) {
        /* Returning all operation ownership leaves the response with delivery.
         * Reentrant cancellation cannot invalidate its notification pass. */
        ck_assert_uint_eq(localResultCount, 1);
        ck_assert_uint_eq(response->pendingResults, 0);
        ck_assert_ptr_eq(TAILQ_FIRST(&serverArg->asyncManager.responses), response);
        UA_AsyncManager_cancel(serverArg, &serverArg->adminSession, 0);
    }
}

START_TEST(Async_cancel_callback_acknowledges_entire_request) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    batchCancelNotifications = 0;
    driveDuringBatchCancel = (_i >= 2);
    UA_ReadValueId items[2];
    for(size_t i = 0; i < 2; i++) {
        UA_ReadValueId_init(&items[i]);
        items[i].nodeId = UA_NODEID_STRING(1, "asyncVar");
        items[i].attributeId = UA_ATTRIBUTEID_VALUE;
    }
    if(driveDuringBatchCancel) {
        localResultCount = 0;
        ck_assert_uint_eq(UA_Server_read_async(server, items, UA_TIMESTAMPSTORETURN_NEITHER,
                                               recordLocalReadResult, NULL, 0),
                          UA_STATUSCODE_GOOD);
        UA_Server_removeCallback(server, lastTimedCallback);
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, (UA_DataValue*)activeReads[0]),
                          UA_STATUSCODE_GOOD);
        activeReads[0] = NULL;
    }
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = items;
    request.nodesToReadSize = 2;
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    UA_ReadResponse response;
    UA_ReadResponse_init(&response);
    server->config.asyncOperationCancelCallback = acknowledgeAllReads;
    /* Leave room for only one request operation. The second is canceled during
     * initiation, but its notification must wait for the complete response. */
    if(_i % 2 == 1)
        server->config.maxAsyncOperationQueueSize = driveDuringBatchCancel ? 2 : 1;
    lockServer(server);
    UA_Boolean done = Service_Read(server, &server->adminSession, &request, &response);
    ck_assert(!done);
    ck_assert_uint_eq(batchCancelNotifications, 0);
    if(_i % 2 == 0) {
        /* One notification can return all operations of several responses.
         * Each response stays alive until its notification pass ends. */
        if(_i == 4) {
            for(size_t i = 0; i < 2; i++)
                ck_assert(!Service_Read(server, &server->adminSession, &request, &response));
        }
        UA_AsyncManager_cancel(server, &server->adminSession, 0);
    } else {
        UA_AsyncResponse *ar = TAILQ_FIRST(&server->asyncManager.responses);
        ck_assert_ptr_nonnull(ar);
        ck_assert_uint_eq(ar->response.readResponse.results[1].status,
                          UA_STATUSCODE_BADTOOMANYOPERATIONS);
        asyncRead(server, activeReads[0]);
    }
    unlockServer(server);
    ck_assert_uint_eq(batchCancelNotifications, 0);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(batchCancelNotifications, 1);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    ck_assert(TAILQ_EMPTY(&server->asyncManager.operations));
    UA_ReadResponse_clear(&response);
} END_TEST

START_TEST(Async_sync_read_trampoline_reuse) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    acknowledgeCancellation = false;
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataValue value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);
    ck_assert_uint_eq(value.status, UA_STATUSCODE_BADWAITINGFORRESPONSE);
    ck_assert(!value.hasValue);
    UA_DataValue_clear(&value);
    UA_DataValue *first = (UA_DataValue*)(uintptr_t)canceledCallRequest;

    value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(value.status, UA_STATUSCODE_BADWAITINGFORRESPONSE);
    UA_DataValue_clear(&value);
    UA_DataValue *second = (UA_DataValue*)(uintptr_t)canceledCallRequest;
    ck_assert_ptr_ne(first, second);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 2);

    server->config.maxAsyncOperationQueueSize = 2;
    /* Queue saturation must not prevent an ordinary synchronous read. */
    rvi.attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
    value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(value.status, UA_STATUSCODE_GOOD);
    ck_assert(value.hasValue);
    ck_assert_ptr_eq(canceledCallRequest, second);
    UA_DataValue_clear(&value);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    server->config.maxAsyncOperationQueueSize = 0;

    /* The worker can finish writing after the synchronous caller has returned. */
    UA_String late = UA_STRING("late result");
    ck_assert_uint_eq(UA_Variant_setScalarCopy(&first->value, &late,
                                              &UA_TYPES[UA_TYPES_STRING]),
                      UA_STATUSCODE_GOOD);
    first->hasValue = true;
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, first), UA_STATUSCODE_GOOD);

    value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_ptr_eq(canceledCallRequest, first);
    ck_assert_uint_eq(value.status, UA_STATUSCODE_BADWAITINGFORRESPONSE);
    UA_DataValue_clear(&value);
    ck_assert(!first->hasValue);
    ck_assert_ptr_null(first->value.data);
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, second), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, first), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);

    /* Reentrant acknowledgement must recycle without the facade touching op again. */
    acknowledgeCancellation = true;
    completeCanceledRead = true;
    value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(value.status, UA_STATUSCODE_BADWAITINGFORRESPONSE);
    ck_assert_uint_eq(completeCanceledReadResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    UA_DataValue_clear(&value);

    /* Ordinary synchronous results are moved out, not retained by the pool. */
    UA_String text = UA_STRING("owned result");
    UA_Variant input;
    UA_Variant_setScalar(&input, &text, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(UA_Server_writeValue(server, UA_NODEID_STRING(1, "syncVar"), input),
                      UA_STATUSCODE_GOOD);
    rvi.nodeId = UA_NODEID_STRING(1, "syncVar");
    value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);
    ck_assert(value.hasValue && value.hasServerTimestamp && value.hasSourceTimestamp);
    ck_assert(UA_String_equal((UA_String*)value.value.data, &text));
    UA_DataValue_clear(&value);
} END_TEST

static void
captureTrampolineCancellation(UA_Server *serverArg, const void *out) {
    canceledCallRequest = out;
}

START_TEST(Async_sync_write_and_call_trampolines) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    server->config.asyncOperationCancelCallback = captureTrampolineCancellation;

    /* The caller's input object can leave scope before acknowledgement. */
    const UA_DataValue *writeId;
    {
        UA_WriteValue wv;
        UA_WriteValue_init(&wv);
        wv.nodeId = UA_NODEID_STRING(1, "asyncVar");
        wv.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_Int32 number = 42;
        UA_Variant_setScalar(&wv.value.value, &number, &UA_TYPES[UA_TYPES_INT32]);
        wv.value.hasValue = true;
        ck_assert_uint_eq(UA_Server_write(server, &wv), UA_STATUSCODE_BADWAITINGFORRESPONSE);
        writeId = (const UA_DataValue*)canceledCallRequest;
        ck_assert_ptr_ne(writeId, &wv.value);
        UA_Server_removeCallback(server, lastTimedCallback);
    }
    ck_assert_uint_eq(UA_Server_setAsyncWriteResult(server, writeId, UA_STATUSCODE_GOOD),
                      UA_STATUSCODE_GOOD);

    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    request.methodId = UA_NODEID_STRING(1, "asyncMethod");
    UA_CallMethodResult result = UA_Server_call(server, &request);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADWAITINGFORRESPONSE);
    ck_assert_uint_eq(result.outputArgumentsSize, 0);
    ck_assert_ptr_null(result.outputArguments);
    UA_CallMethodResult_clear(&result);
    UA_Variant *callId = (UA_Variant*)(uintptr_t)canceledCallRequest;
    ck_assert_ptr_nonnull(callId);
    UA_Server_removeCallback(server, lastTimedCallback);
    memset(activeCalls, 0, sizeof(activeCalls));
    ck_assert_uint_eq(UA_Server_setAsyncCallMethodResult(server, callId, UA_STATUSCODE_GOOD),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);

    request.methodId = UA_NODEID_STRING(1, "method");
    result = UA_Server_call(server, &request);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
} END_TEST

START_TEST(Async_internal_read_trampolines_and_shutdown) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    acknowledgeCancellation = false;
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "asyncVar");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue value;
    UA_DataValue_init(&value);

    lockServer(server);
    const UA_Node *node = UA_NODESTORE_GET(server, &rvi.nodeId);
    ck_assert_ptr_nonnull(node);
    readNoAsync(server, &server->adminSession, node,
                UA_TIMESTAMPSTORETURN_NEITHER, &rvi, &value);
    ck_assert_uint_eq(value.status, UA_STATUSCODE_BADWAITINGFORRESPONSE);
    UA_DataValue *first = (UA_DataValue*)(uintptr_t)canceledCallRequest;
    UA_DataValue_clear(&value);
    ck_assert_uint_eq(readValueAttribute(server, &server->adminSession,
                                         (const UA_VariableNode*)node, &value),
                      UA_STATUSCODE_BADWAITINGFORRESPONSE);
    UA_DataValue *second = (UA_DataValue*)(uintptr_t)canceledCallRequest;
    ck_assert_ptr_ne(first, second);
    UA_DataValue_clear(&value);
    UA_NODESTORE_RELEASE(server, node);
    unlockServer(server);

    server->config.externalEventLoop = true;
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPING);
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, first), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPING);
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, second), UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 100 && UA_Server_getLifecycleState(server) !=
        UA_LIFECYCLESTATE_STOPPED; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
    server->config.externalEventLoop = false;
} END_TEST

/* Real workers own their result storage exclusively until the setter returns.
 * Atomics coordinate the test phases, not access to the result contents. */
typedef struct {
    THREAD_HANDLE thread;
    UA_Boolean joinable;
    UA_DataValue *read;
    UA_Variant *call;
    const UA_DataValue *write;
    UA_atomic(uintptr_t) started;
    UA_atomic(uintptr_t) finish;
    UA_atomic(uintptr_t) done;
    UA_StatusCode status;
} ConcurrentWorker;

/* Check can leave a failing test via longjmp. The fixture, not its stack frame,
 * owns every thread and callback context until teardown has joined the threads. */
static ConcurrentWorker workers[2];
static UA_Client *concurrentClient;

static void
yieldWorker(void) {
#ifdef UA_ARCHITECTURE_WIN32
    Sleep(1);
#else
    struct timespec delay = {0, 1000000};
    nanosleep(&delay, NULL);
#endif
}

static void
joinResultWorker(ConcurrentWorker *w) {
    if(!w->joinable)
        return;
    UA_atomic_store(&w->finish, true);
    THREAD_JOIN(w->thread);
#ifdef UA_ARCHITECTURE_WIN32
    CloseHandle(w->thread);
#endif
    w->joinable = false;
}

THREAD_CALLBACK_PARAM(resultWorker, data) {
    ConcurrentWorker *w = (ConcurrentWorker*)data;
    UA_String text = UA_STRING("worker-owned output");
    do {
        if(w->read) {
            UA_DataValue_clear(w->read);
            UA_Variant_setScalarCopy(&w->read->value, &text, &UA_TYPES[UA_TYPES_STRING]);
            w->read->hasValue = true;
            w->read->hasSourceTimestamp = true;
            w->read->sourceTimestamp = 123;
        } else if(w->call) {
            UA_Variant_clear(w->call);
            UA_Variant_setScalarCopy(w->call, &text, &UA_TYPES[UA_TYPES_STRING]);
        }
        UA_atomic_store(&w->started, true);
        /* Let the event-loop thread progress under instrumentation as well. */
        yieldWorker();
    } while(!UA_atomic_load(&w->finish));

    if(w->read)
        w->status = UA_Server_setAsyncReadResult(server, w->read);
    else if(w->call)
        w->status = UA_Server_setAsyncCallMethodResult(server, w->call, UA_STATUSCODE_GOOD);
    else
        w->status = UA_Server_setAsyncWriteResult(server, w->write, UA_STATUSCODE_GOOD);
    UA_atomic_store(&w->done, true);
    return 0;
}

static UA_StatusCode
concurrentRead(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimestamp,
               const UA_NumericRange *range, UA_DataValue *value) {
    ConcurrentWorker *w = (ConcurrentWorker*)nodeContext;
    w->read = value;
    THREAD_CREATE_PARAM(w->thread, resultWorker, *w);
    w->joinable = true;
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static UA_StatusCode
concurrentWrite(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                const UA_NumericRange *range, const UA_DataValue *value) {
    ConcurrentWorker *w = (ConcurrentWorker*)nodeContext;
    w->write = value; /* Identifier only; input contents are not retained. */
    THREAD_CREATE_PARAM(w->thread, resultWorker, *w);
    w->joinable = true;
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static UA_StatusCode
concurrentCall(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    ConcurrentWorker *w = (ConcurrentWorker*)methodContext;
    w->call = output;
    THREAD_CREATE_PARAM(w->thread, resultWorker, *w);
    w->joinable = true;
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

typedef struct {
    size_t kind;
    size_t received;
    UA_StatusCode expectedStatus;
    const UA_DataType *type;
    void *response;
} ConcurrentResponse;

static ConcurrentResponse received;

static void
concurrentResponse(UA_Client *client, void *context, UA_UInt32 id, void *response) {
    ConcurrentResponse *r = (ConcurrentResponse*)context;
    r->received++;
    r->response = UA_new(r->type);
    ck_assert_uint_eq(UA_copy(response, r->response, r->type), UA_STATUSCODE_GOOD);
}

static void
checkConcurrentResponse(ConcurrentResponse *r) {
    void *response = r->response;
    UA_ResponseHeader *header = (UA_ResponseHeader*)response;
    ck_assert_uint_eq(header->requestHandle, 1337);
    /* The Cancel service returns a ServiceFault rather than per-item results.
     * Timeouts retain a normal response with the already completed items. */
    if(r->expectedStatus == UA_STATUSCODE_BADOPERATIONABANDONED) {
        ck_assert_uint_eq(header->serviceResult, UA_STATUSCODE_BADREQUESTCANCELLEDBYCLIENT);
        return;
    }
    ck_assert_uint_eq(header->serviceResult, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 2; i++) {
        UA_StatusCode expected = (i == 0) ? UA_STATUSCODE_GOOD : r->expectedStatus;
        const UA_Variant *value = NULL;
        if(r->kind == 0) {
            UA_ReadResponse *rr = (UA_ReadResponse*)response;
            ck_assert_uint_eq(rr->resultsSize, 2);
            ck_assert_uint_eq(rr->results[i].status, expected);
            if(expected == UA_STATUSCODE_GOOD) {
                ck_assert(rr->results[i].hasValue);
                ck_assert(rr->results[i].hasServerTimestamp);
                ck_assert_int_eq(rr->results[i].sourceTimestamp, 123);
                value = &rr->results[i].value;
            } else {
                ck_assert(!rr->results[i].hasValue);
            }
        } else if(r->kind == 1) {
            UA_CallResponse *cr = (UA_CallResponse*)response;
            ck_assert_uint_eq(cr->resultsSize, 2);
            ck_assert_uint_eq(cr->results[i].statusCode, expected);
            ck_assert_uint_eq(cr->results[i].outputArgumentsSize,
                              expected == UA_STATUSCODE_GOOD ? 1 : 0);
            if(expected == UA_STATUSCODE_GOOD)
                value = &cr->results[i].outputArguments[0];
        } else {
            UA_WriteResponse *wr = (UA_WriteResponse*)response;
            ck_assert_uint_eq(wr->resultsSize, 2);
            ck_assert_uint_eq(wr->results[i], expected);
        }
        if(value) {
            ck_assert(UA_Variant_hasScalarType(value, &UA_TYPES[UA_TYPES_STRING]));
            UA_String expectedText = UA_STRING("worker-owned output");
            ck_assert(UA_String_equal((UA_String*)value->data, &expectedText));
        }
    }
}

/* Three result types, with normal completion, Cancel service, and timeout.
 * The completed first result must survive cancellation of the second while
 * its worker repeatedly edits/frees/reallocates output during wire encoding. */
START_TEST(Async_concurrent_worker_ownership) {
    UA_Client *client = concurrentClient = UA_Client_newForUnitTest();
    ck_assert_uint_eq(UA_Client_connect(client, "opc.tcp://localhost:4840"),
                      UA_STATUSCODE_GOOD);
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    server->config.asyncOperationCancelCallback = NULL;
    UA_atomic_store(&workers[0].finish, true);
    size_t kind = (size_t)_i / 3;
    size_t mode = (size_t)_i % 3;
    received = (ConcurrentResponse){kind, 0, mode == 0 ? UA_STATUSCODE_GOOD :
        (mode == 1 ? UA_STATUSCODE_BADOPERATIONABANDONED : UA_STATUSCODE_BADTIMEOUT),
        &UA_TYPES[kind == 0 ? UA_TYPES_READRESPONSE :
                  (kind == 1 ? UA_TYPES_CALLRESPONSE : UA_TYPES_WRITERESPONSE)], NULL};

    UA_ReadValueId reads[2];
    UA_CallMethodRequest calls[2];
    UA_WriteValue writes[2];
    UA_Int32 input = 42;
    for(size_t i = 0; i < 2; i++) {
        UA_NodeId nodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)(60000 + i));
        if(kind == 1) {
            UA_MethodAttributes attr = UA_MethodAttributes_default;
            attr.executable = attr.userExecutable = true;
            UA_Argument output;
            UA_Argument_init(&output);
            output.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
            output.valueRank = UA_VALUERANK_SCALAR;
            ck_assert_uint_eq(UA_Server_addMethodNode(server, nodeId,
                UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                UA_QUALIFIEDNAME(1, i == 0 ? "worker0" : "worker1"), attr, concurrentCall,
                0, NULL, 1, &output, &workers[i], NULL), UA_STATUSCODE_GOOD);
        } else {
            UA_VariableAttributes attr = UA_VariableAttributes_default;
            attr.accessLevel |= UA_ACCESSLEVELMASK_WRITE;
            ck_assert_uint_eq(UA_Server_addVariableNode(server, nodeId,
                UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                UA_QUALIFIEDNAME(1, i == 0 ? "worker0" : "worker1"), UA_NS0ID(BASEDATAVARIABLETYPE),
                attr, &workers[i], NULL), UA_STATUSCODE_GOOD);
            UA_CallbackValueSource source = {concurrentRead, concurrentWrite};
            ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(
                server, nodeId, source), UA_STATUSCODE_GOOD);
        }
        UA_ReadValueId_init(&reads[i]);
        reads[i].nodeId = nodeId;
        reads[i].attributeId = UA_ATTRIBUTEID_VALUE;
        UA_CallMethodRequest_init(&calls[i]);
        calls[i].objectId = UA_NS0ID(OBJECTSFOLDER);
        calls[i].methodId = nodeId;
        UA_WriteValue_init(&writes[i]);
        writes[i].nodeId = nodeId;
        writes[i].attributeId = UA_ATTRIBUTEID_VALUE;
        writes[i].value.hasValue = true;
        UA_Variant_setScalar(&writes[i].value.value, &input, &UA_TYPES[UA_TYPES_INT32]);
    }
    if(kind == 0) {
        UA_ReadRequest req;
        UA_ReadRequest_init(&req);
        req.requestHeader.requestHandle = 1337;
        req.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
        req.nodesToRead = reads;
        req.nodesToReadSize = 2;
        ck_assert_uint_eq(__UA_Client_AsyncService(client, &req,
            &UA_TYPES[UA_TYPES_READREQUEST], concurrentResponse,
            &UA_TYPES[UA_TYPES_READRESPONSE], &received, NULL), UA_STATUSCODE_GOOD);
    } else if(kind == 1) {
        UA_CallRequest req;
        UA_CallRequest_init(&req);
        req.requestHeader.requestHandle = 1337;
        req.methodsToCall = calls;
        req.methodsToCallSize = 2;
        ck_assert_uint_eq(__UA_Client_AsyncService(client, &req,
            &UA_TYPES[UA_TYPES_CALLREQUEST], concurrentResponse,
            &UA_TYPES[UA_TYPES_CALLRESPONSE], &received, NULL), UA_STATUSCODE_GOOD);
    } else {
        UA_WriteRequest req;
        UA_WriteRequest_init(&req);
        req.requestHeader.requestHandle = 1337;
        req.nodesToWrite = writes;
        req.nodesToWriteSize = 2;
        ck_assert_uint_eq(__UA_Client_AsyncService(client, &req,
            &UA_TYPES[UA_TYPES_WRITEREQUEST], concurrentResponse,
            &UA_TYPES[UA_TYPES_WRITERESPONSE], &received, NULL), UA_STATUSCODE_GOOD);
    }
    UA_DateTime deadline = UA_DateTime_nowMonotonic() + 60 * UA_DATETIME_SEC;
    while(!UA_atomic_load(&workers[0].done) || !UA_atomic_load(&workers[1].started)) {
        ck_assert(UA_DateTime_nowMonotonic() < deadline);
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
        yieldWorker();
    }
    joinResultWorker(&workers[0]);
    ck_assert_uint_eq(workers[0].status, UA_STATUSCODE_GOOD);
    if(mode == 0) {
        UA_atomic_store(&workers[1].finish, true);
    } else if(mode == 1) {
        UA_CancelRequest req;
        UA_CancelRequest_init(&req);
        req.requestHandle = 1337;
        ck_assert_uint_eq(__UA_Client_AsyncService(client, &req,
            &UA_TYPES[UA_TYPES_CANCELREQUEST], NULL,
            &UA_TYPES[UA_TYPES_CANCELRESPONSE], NULL, NULL), UA_STATUSCODE_GOOD);
    } else {
        UA_fakeSleep(2500);
    }
    deadline = UA_DateTime_nowMonotonic() + 60 * UA_DATETIME_SEC;
    while(received.received == 0) {
        ck_assert(UA_DateTime_nowMonotonic() < deadline);
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(client, 0);
        yieldWorker();
    }
    if(mode != 0) {
        ck_assert(!UA_atomic_load(&workers[1].done));
        ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 1);
    }
    joinResultWorker(&workers[1]);
    ck_assert_uint_eq(workers[1].status, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    checkConcurrentResponse(&received);
    UA_atomic_store(&running, true);
    THREAD_CREATE(server_thread, serverloop);
    UA_Client_disconnect(client);
} END_TEST

START_TEST(Async_local_worker_ownership) {
    UA_atomic_store(&running, false);
    THREAD_JOIN(server_thread);
    server->config.asyncOperationCancelCallback = NULL;
    ConcurrentWorker *worker = &workers[0];
    UA_NodeId node = UA_NODEID_STRING(1, "asyncVar");
    ck_assert_uint_eq(UA_Server_setNodeContext(server, node, worker), UA_STATUSCODE_GOOD);
    UA_CallbackValueSource source = {concurrentRead, NULL};
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, node, source),
                      UA_STATUSCODE_GOOD);
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = node;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    localResultCount = 0;
    if(_i == 2) {
        UA_DataValue value = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);
        ck_assert_uint_eq(value.status, UA_STATUSCODE_BADWAITINGFORRESPONSE);
        UA_DataValue_clear(&value);
    } else {
        ck_assert_uint_eq(UA_Server_read_async(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH,
            recordLocalReadResult, worker, _i == 0 ? 500 : 0), UA_STATUSCODE_GOOD);
    }
    UA_DateTime deadline = UA_DateTime_nowMonotonic() + 60 * UA_DATETIME_SEC;
    while(!UA_atomic_load(&worker->started)) {
        ck_assert(UA_DateTime_nowMonotonic() < deadline);
        yieldWorker();
    }
    if(_i == 0) {
        UA_fakeSleep(1500);
    } else if(_i == 1) {
        server->config.externalEventLoop = true;
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    }
    for(size_t i = 0; i < 10; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(localResultCount, _i == 2 ? 0 : 1);
    if(_i != 2)
        ck_assert_uint_eq(localResultStatus, _i == 0 ?
                          UA_STATUSCODE_BADTIMEOUT : UA_STATUSCODE_BADSHUTDOWN);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 1);
    if(_i == 1)
        ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPING);
    joinResultWorker(worker);
    ck_assert_uint_eq(worker->status, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 10; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(localResultCount, _i == 2 ? 0 : 1);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    if(_i == 1) {
        ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
        server->config.externalEventLoop = false;
    }
} END_TEST

static void
setupWorkers(void) {
    memset(workers, 0, sizeof(workers));
    memset(&received, 0, sizeof(received));
    concurrentClient = NULL;
    setup();
}

static void
teardownWorkers(void) {
    if(UA_atomic_load(&running)) {
        UA_atomic_store(&running, false);
        THREAD_JOIN(server_thread);
    }
    for(size_t i = 0; i < 2; i++)
        joinResultWorker(&workers[i]);
    if(concurrentClient) {
        UA_Client_disconnectAsync(concurrentClient);
        UA_Client_delete(concurrentClient);
    }
    if(received.response)
        UA_delete(received.response, received.type);
    /* A failed shutdown test can leave an external event loop configured. */
    server->config.externalEventLoop = false;
    while(UA_Server_getLifecycleState(server) == UA_LIFECYCLESTATE_STOPPING)
        UA_Server_run_iterate(server, false);
    teardown();
}

static Suite* method_async_suite(void) {
    /* set up unit test for internal data structures */
    Suite *s = suite_create("Async Method");

    TCase *tc_workers = tcase_create("Workers");
    tcase_add_checked_fixture(tc_workers, setupWorkers, teardownWorkers);
    tcase_add_loop_test(tc_workers, Async_concurrent_worker_ownership, 0, 9);
    tcase_add_loop_test(tc_workers, Async_local_worker_ownership, 0, 3);
    suite_add_tcase(s, tc_workers);

    TCase* tc_manager = tcase_create("AsyncMethod");
    tcase_add_checked_fixture(tc_manager, setup, teardown);
    tcase_add_test(tc_manager, Async_result_callback_acknowledges_timed_out_worker);
    tcase_add_loop_test(tc_manager, Async_local_delivery_lifetime, 0, 3);
    tcase_add_test(tc_manager, Async_shutdown_delivers_already_ready_results);
    tcase_add_test(tc_manager, Async_local_timeout_without_global_timeout);
    tcase_add_loop_test(tc_manager, Async_cancel_callback_acknowledges_entire_request, 0, 5);
    tcase_add_test(tc_manager, Async_result_callback_queues_another_result);
    tcase_add_test(tc_manager, Async_sync_read_trampoline_reuse);
    tcase_add_test(tc_manager, Async_sync_write_and_call_trampolines);
    tcase_add_test(tc_manager, Async_internal_read_trampolines_and_shutdown);
    tcase_add_test(tc_manager, Async_call);
    tcase_add_test(tc_manager, Async_read);
    tcase_add_test(tc_manager, Async_multiRead_closingSessionCancelsPendingOperation);
    tcase_add_test(tc_manager,
                   Async_serviceNotificationCloseCancelsPersistedResponse);
    tcase_add_test(tc_manager, Async_write);
    tcase_add_test(tc_manager, Async_abandonedResponseStillNotifiesServiceEnd);
    tcase_add_test(tc_manager, Async_timeout);
    tcase_add_test(tc_manager, Async_forget);
    tcase_add_test(tc_manager, Async_cancel);
    tcase_add_test(tc_manager, Async_cancel_multiple);
    tcase_add_test(tc_manager, Async_request_handles_are_per_response);
    tcase_add_test(tc_manager, Async_server_read);
    tcase_add_test(tc_manager, Async_server_write);
    tcase_add_test(tc_manager, Async_read_timeout_server);
    tcase_add_test(tc_manager, Async_setResult_badnotfound);
    tcase_add_test(tc_manager, Async_queue_limit_read_direct);
    tcase_add_test(tc_manager, Async_sync_method_call);
    tcase_add_test(tc_manager, Async_read_sync_variable);
    tcase_add_test(tc_manager, Async_service_read_validation_paths);
    tcase_add_test(tc_manager, Async_service_read_allocation_size_overflow);
    tcase_add_test(tc_manager, Async_service_read_toomanyoperations);
    tcase_add_test(tc_manager, Async_service_write_validation_paths);
    tcase_add_test(tc_manager, Async_service_write_toomanyoperations);
    tcase_add_test(tc_manager, Async_direct_call_method_result);
    tcase_add_test(tc_manager, Async_write_queue_overflow);
    /* Additional direct API coverage that doesn't need a running server. */
    tcase_add_test(tc_manager, Async_read_async_zeroTimeout_disablesTimeout);
    tcase_add_test(tc_manager, Async_read_async_unknownNode_returnsError);
    tcase_add_test(tc_manager, Async_write_async_unknownNode_returnsError);
    tcase_add_test(tc_manager, Async_setAsyncReadResult_null_returnsError);
    tcase_add_test(tc_manager, Async_setAsyncWriteResult_null_returnsError);
    tcase_add_test(tc_manager, Async_setAsyncMethodResult_null_returnsError);
    tcase_add_test(tc_manager, Async_direct_read_completed_synchronously);
    tcase_add_test(tc_manager, Async_call_multiple_outputs);
    tcase_add_test(tc_manager, Async_shutdown_waits_for_direct_operation);
    tcase_add_test(tc_manager, Async_service_cancel_with_direct_operation);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    tcase_add_test(tc_manager, Async_monitored_item_deletion_with_service_operation);
#endif
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
