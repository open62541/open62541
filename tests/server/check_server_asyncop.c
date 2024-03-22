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

#include <check.h>
#include <stdlib.h>

UA_Boolean running;
THREAD_HANDLE server_thread;
static UA_Server *server;
static size_t clientCounter;

static UA_StatusCode
methodCallback(UA_Server *serverArg,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    return UA_STATUSCODE_GOOD;
}

static void
clientReceiveCallbackCall(UA_Client *client, void *userdata,
                      UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Received call response");
    clientCounter++;
}

static void
clientReceiveCallbackRead(UA_Client *client, void *userdata,
                      UA_UInt32 requestId, UA_ReadResponse *rr) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Received read response");
    clientCounter++;
}

static void
clientReceiveCallbackWrite(UA_Client *client, void *userdata,
                           UA_UInt32 requestId, UA_WriteResponse *wr) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Received write response");
    clientCounter++;
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
                            methodAttr, &methodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Asynchronous Method */
    res = UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "asyncMethod"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "asyncMethod"),
                            methodAttr, &methodCallback,
                            0, NULL, 0, NULL, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_setMethodNodeAsync(server, UA_NODEID_STRING(1, "asyncMethod"), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Asynchronous Variable */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","asyncVariable");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","asyncVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.asyncVariable");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "asyncVariable");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableNodeId;
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, &variableNodeId);
    UA_Server_setVariableNodeAsync(server, variableNodeId, UA_TRUE);

    /* Synchronous Variable */

    attr.displayName = UA_LOCALIZEDTEXT("en-US","syncVariable");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                              parentReferenceNodeId, UA_QUALIFIEDNAME(0, "syncVariable"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
    UA_NodeId_clear(&variableNodeId);
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
    retval = UA_Client_call_async(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "asyncMethod"), 0, NULL,
                                  clientReceiveCallbackCall, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_call_async(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "method"), 0, NULL,
                                  clientReceiveCallbackCall, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientCounter, 0);

    /* Receive the answer of the sync call */
    while(clientCounter == 0) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    /* Process the async method call for the server */
    UA_AsyncOperationType aot;
    const UA_AsyncOperationRequest *request;
    void *context = NULL;
    UA_DateTime timeout = 0;
    UA_Boolean haveAsync =
        UA_Server_getAsyncOperationNonBlocking(server, &aot, &request, &context, &timeout, NULL);
    ck_assert_uint_eq(haveAsync, true);
    UA_AsyncOperationResponse response;
    UA_CallMethodResult_init(&response.callMethodResult);
    UA_Server_setAsyncOperationResult(server, &response, context);

    /* Iterate and pick up the async response to be sent out */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, true);

    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, true);

    /* Process async responses during 1s */
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
    retval = UA_Client_call_async(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "asyncMethod"), 0, NULL,
                                  clientReceiveCallbackCall, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* We expect to receive the timeout not yet*/
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 0);

    UA_fakeSleep((UA_UInt32)(1000 * 1.5));

    /* We expect to receive the timeout not yet*/
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 0);

    UA_fakeSleep(1000);

    /* We expect to receive the timeout response */
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 1);

    running = true;
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
    retval = UA_Client_call_async(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "asyncMethod"), 0, NULL,
                                  clientReceiveCallbackCall, NULL, &reqId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Cancel the request */
    UA_UInt32 cancelCount = 0;
    UA_Client_cancelByRequestId(client, reqId, &cancelCount);
    ck_assert_uint_eq(cancelCount, 1);

    /* We expect to receive the cancelled response */
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 1);

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

/* Force a timeout when the operation is checked out with the worker */
START_TEST(Async_timeout_worker) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Stop the server thread. Iterate manually from now on */
    running = false;
    THREAD_JOIN(server_thread);

    /* Call async method, then the sync method.
     * The sync method returns first. */
    retval = UA_Client_call_async(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_STRING(1, "asyncMethod"), 0, NULL,
                                  clientReceiveCallbackCall, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, true);

    /* Process the async method call for the server */
    UA_AsyncOperationType aot;
    const UA_AsyncOperationRequest *request;
    void *context = NULL;
    UA_DateTime timeout = 0;
    UA_Boolean haveAsync =
        UA_Server_getAsyncOperationNonBlocking(server, &aot, &request, &context, &timeout, NULL);
    ck_assert_uint_eq(haveAsync, true);
    UA_AsyncOperationResponse response;
    UA_CallMethodResult_init(&response.callMethodResult);

    /* Force a timeout */
    UA_fakeSleep(2500);
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 1);

    /* Return the late response */
    UA_Server_setAsyncOperationResult(server, &response, context);

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

    UA_NodeId variableNode = UA_NODEID_STRING(1, "the.asyncVariable");
    UA_ReadValueId item;
    UA_ReadValueId_init(&item);
    item.nodeId = variableNode;
    item.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_ReadRequest readRequest;
    UA_ReadRequest_init(&readRequest);
    readRequest.nodesToRead = &item;
    readRequest.nodesToReadSize = 1;

    /* Read async first, then the sync method.
     * The sync method returns first. */
    retval = UA_Client_sendAsyncReadRequest(client, &readRequest, clientReceiveCallbackRead, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    variableNode = UA_NODEID_STRING(1, "the.syncVariable");
    item.nodeId = variableNode;
    retval = UA_Client_sendAsyncReadRequest(client, &readRequest, clientReceiveCallbackRead, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(clientCounter, 0);

    /* Receive the answer of the sync call */
    while(clientCounter == 0) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    /* Process the async method call for the server */
    UA_AsyncOperationType aot;
    const UA_AsyncOperationRequest *request;
    void *context = NULL;
    UA_DateTime timeout = 0;
    UA_Boolean haveAsync =
        UA_Server_getAsyncOperationNonBlocking(server, &aot, &request, &context, &timeout, NULL);
    ck_assert_uint_eq(haveAsync, true);

    UA_DataValue readResponse;
    readResponse = UA_Server_readWithSession(server, &request->readValueId, UA_TIMESTAMPSTORETURN_BOTH, NULL);
    UA_Server_setAsyncOperationResult(server, (UA_AsyncOperationResponse*) &readResponse, context);
    UA_DataValue_clear(&readResponse);

    /* Iterate and pick up the async response to be sent out */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, true);

    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, true);

    /* Process async responses during 1s */
    UA_Client_run_iterate(client, 0);
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

    UA_NodeId variableNode = UA_NODEID_STRING(1, "the.asyncVariable");
    UA_WriteValue writeValue;
    UA_WriteValue_init(&writeValue);
    UA_Int32 numeric_value = 42;
    UA_Variant_setScalar(&writeValue.value.value, &numeric_value, &UA_TYPES[UA_TYPES_INT32]);
    writeValue.attributeId = UA_ATTRIBUTEID_VALUE;
    writeValue.nodeId = variableNode;
    UA_WriteRequest writeRequest;
    UA_WriteRequest_init(&writeRequest);
    writeRequest.nodesToWriteSize = 1;
    writeRequest.nodesToWrite = &writeValue;

    /* Write async first, then the sync method.
     * The sync method returns first. */
    retval = UA_Client_sendAsyncWriteRequest(client, &writeRequest, clientReceiveCallbackWrite, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    variableNode = UA_NODEID_STRING(1, "the.syncVariable");
    writeValue.nodeId = variableNode;
    retval = UA_Client_sendAsyncWriteRequest(client, &writeRequest, clientReceiveCallbackWrite, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Receive the answer of the sync call */
    while(clientCounter == 0) {
        UA_Server_run_iterate(server, true);
        UA_Client_run_iterate(client, 0);
    }
    ck_assert_uint_eq(clientCounter, 1);

    /* Process the async method call for the server */
    UA_AsyncOperationType aot;
    const UA_AsyncOperationRequest *request;
    void *context = NULL;
    UA_DateTime timeout = 0;
    UA_Boolean haveAsync =
        UA_Server_getAsyncOperationNonBlocking(server, &aot, &request, &context, &timeout, NULL);
    ck_assert_uint_eq(haveAsync, true);

    UA_StatusCode result;
    result = UA_Server_write(server, &request->writeValue);
    UA_Server_setAsyncOperationResult(server, (UA_AsyncOperationResponse*) &result, context);
    ck_assert_uint_eq(result, UA_STATUSCODE_GOOD);

    /* Iterate and pick up the async response to be sent out */
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, true);

    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, true);

    /* Process async responses during 1s */
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 2);

    running = true;
    THREAD_CREATE(server_thread, serverloop);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
} END_TEST

static Suite* method_async_suite(void) {
    /* set up unit test for internal data structures */
    Suite *s = suite_create("Async Method");

    TCase* tc_manager = tcase_create("AsyncMethod");
    tcase_add_checked_fixture(tc_manager, setup, teardown);
    tcase_add_test(tc_manager, Async_call);
    tcase_add_test(tc_manager, Async_timeout);
    tcase_add_test(tc_manager, Async_cancel);
    tcase_add_test(tc_manager, Async_cancel_multiple);
    tcase_add_test(tc_manager, Async_timeout_worker);
    tcase_add_test(tc_manager, Async_read);
    tcase_add_test(tc_manager, Async_write);
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
