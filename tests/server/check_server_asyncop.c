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
#include "thread_wrapper.h"

#include <check.h>

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
clientReceiveCallback(UA_Client *client, void *userdata,
                      UA_UInt32 requestId, UA_CallResponse *cr) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT, "Received call response");
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
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
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
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *clientConfig = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(clientConfig);

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
    ck_assert_uint_eq(clientCounter, 0);
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    UA_Server_run_iterate(server, true);
    UA_Client_run_iterate(client, 0);
    ck_assert_uint_eq(clientCounter, 1);

    /* Process the async method call for the server */
    UA_AsyncOperationType aot;
    const UA_AsyncOperationRequest *request;
    void *context = NULL;
    UA_DateTime timeout = 0;
    UA_Boolean haveAsync =
        UA_Server_getAsyncOperationNonBlocking(server, &aot, &request, &context, &timeout);
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
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *clientConfig = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(clientConfig);

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

/* Force a timeout when the operation is checked out with the worker */
START_TEST(Async_timeout_worker) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *clientConfig = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(clientConfig);

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
    UA_Server_run_iterate(server, true);

    /* Process the async method call for the server */
    UA_AsyncOperationType aot;
    const UA_AsyncOperationRequest *request;
    void *context = NULL;
    UA_DateTime timeout = 0;
    UA_Boolean haveAsync =
        UA_Server_getAsyncOperationNonBlocking(server, &aot, &request, &context, &timeout);
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

static Suite* method_async_suite(void) {
    /* set up unit test for internal data structures */
    Suite *s = suite_create("Async Method");

    TCase* tc_manager = tcase_create("AsyncMethod");
    tcase_add_checked_fixture(tc_manager, setup, teardown);
    tcase_add_test(tc_manager, Async_call);
    tcase_add_test(tc_manager, Async_timeout);
    tcase_add_test(tc_manager, Async_timeout_worker);
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
