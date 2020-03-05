/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/plugin/log_stdout.h>
#include "testing_clock.h"
#include "thread_wrapper.h"

#include <check.h>

static UA_INLINE THREAD_HANDLE THREAD_SELF(void) {
#ifndef _WIN32
    return pthread_self();
#else
    return GetCurrentThread();
#endif
}

static UA_INLINE bool THREAD_EQUAL(THREAD_HANDLE th1, THREAD_HANDLE th2) {
#ifndef _WIN32
    return pthread_equal(th1, th2);
#else
    return GetThreadId(th1) == GetThreadId(th2);
#endif
}

volatile UA_Boolean running;
THREAD_HANDLE mainThread;
THREAD_HANDLE serverThread;
THREAD_HANDLE workerThread;
static UA_Server *server;
static UA_Client *glClient;
static size_t clientCounter;
UA_CallRequest callRequest;

static UA_StatusCode
methodCallback(UA_Server *serverArg,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output) {
    bool async = *(bool*)input->data;
    bool isCorrectThread = async != THREAD_EQUAL(THREAD_SELF(), mainThread);
    UA_Variant_setScalarCopy(output, &isCorrectThread, &UA_TYPES[UA_TYPES_BOOLEAN]);

    return UA_STATUSCODE_GOOD;
}

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

THREAD_CALLBACK(workerloop) {
    UA_Server_runAsync(server);
    return 0;
}

static void fillCallRequest(UA_CallRequest *cr) {
    cr->methodsToCallSize = 2;
    cr->methodsToCall = (UA_CallMethodRequest*)
        UA_Array_new(cr->methodsToCallSize, &UA_TYPES[UA_TYPES_CALLMETHODREQUEST]);
    cr->methodsToCall[0].objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    cr->methodsToCall[0].methodId = UA_NODEID_STRING_ALLOC(1, "method");
    cr->methodsToCall[0].inputArgumentsSize = 1;
    cr->methodsToCall[0].inputArguments = UA_Variant_new();
    UA_Boolean isAsync = false;
    UA_Variant_setScalarCopy(cr->methodsToCall[0].inputArguments, &isAsync,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);
    cr->methodsToCall[1].objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    cr->methodsToCall[1].methodId = UA_NODEID_STRING_ALLOC(1, "asyncMethod");
    cr->methodsToCall[1].inputArgumentsSize = 1;
    cr->methodsToCall[1].inputArguments = UA_Variant_new();
    isAsync = true;
    UA_Variant_setScalarCopy(cr->methodsToCall[1].inputArguments, &isAsync,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);
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

    UA_Argument arg;
    UA_Argument_init(&arg);
    arg.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    arg.valueRank = UA_VALUERANK_SCALAR;

    /* Synchronous Method */
    UA_StatusCode res =
        UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "method"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "method"),
                            methodAttr, &methodCallback,
                            1, &arg, 1, &arg, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Asynchronous Method */
    res = UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "asyncMethod"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "asyncMethod"),
                            methodAttr, &methodCallback,
                            1, &arg, 1, &arg, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_setMethodNodeAsync(server, UA_NODEID_STRING(1, "asyncMethod"), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    fillCallRequest(&callRequest);

    UA_Server_run_startup(server);
    THREAD_CREATE(serverThread, serverloop);
    THREAD_CREATE(workerThread, workerloop);

    glClient = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(glClient));

    UA_StatusCode retval = UA_Client_connect(glClient, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    running = false;
    THREAD_JOIN(serverThread);

    mainThread = THREAD_SELF();
}

static void teardown(void) {
    UA_Server_stopAsync(server);
    THREAD_JOIN(workerThread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_Client_delete(glClient);
    UA_CallRequest_clear(&callRequest);
}

static void
checkMethodResult(const UA_CallMethodResult *result, bool shouldTimeOut) {
    UA_StatusCode sc = shouldTimeOut ? UA_STATUSCODE_BADTIMEOUT : UA_STATUSCODE_GOOD;
    ck_assert_msg(result->statusCode == sc, "Unexpected result status code");
    if(!shouldTimeOut) {
        ck_assert_uint_eq(result->outputArgumentsSize, 1);
        ck_assert_msg(result->outputArguments->type == &UA_TYPES[UA_TYPES_BOOLEAN], "Unexpected output type");
        ck_assert_msg(*(bool*)result->outputArguments->data, "Called in wrong thread");
    }
}

static void
clientMethodCallback(UA_Client *client, void *userdata,
                     UA_UInt32 requestId, void *response) {
    /* Copy response to check it outside of the callback so that
     * we do not abort while inside the library */
    *(UA_CallResponse**)userdata = UA_CallResponse_new();
    UA_CallResponse_copy((UA_CallResponse*)response, *(UA_CallResponse**)userdata);
}

START_TEST(Async_call) {
    UA_CallResponse *response = NULL;
    UA_StatusCode retval = UA_Client_sendAsyncRequest(glClient,
        &callRequest, &UA_TYPES[UA_TYPES_CALLREQUEST], clientMethodCallback,
        &UA_TYPES[UA_TYPES_CALLRESPONSE], &response, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);

    for(size_t i = 0; i < 100 && !response; ++i) {
        /* Give the worker thread some time to wake up
         * and process requests */
        UA_realSleep(10);
        UA_fakeSleep(101);
        UA_Server_run_iterate(server, false);
        UA_Client_run_iterate(glClient, 0);
    }
    ck_assert_msg(response, "Expected response");

    ck_assert_uint_eq(response->resultsSize, 2);
    ck_assert_uint_eq(response->responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    checkMethodResult(&response->results[0], false);
    checkMethodResult(&response->results[1], false);
    UA_CallResponse_delete(response);
} END_TEST

START_TEST(Async_timeout) {
    UA_CallResponse *response = NULL;
    UA_StatusCode retval = UA_Client_sendAsyncRequest(glClient,
        &callRequest, &UA_TYPES[UA_TYPES_CALLREQUEST], clientMethodCallback,
        &UA_TYPES[UA_TYPES_CALLRESPONSE], &response, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);

    /* Force timeout */
    UA_fakeSleep(10000);
    UA_Server_run_iterate(server, false);
    UA_Client_run_iterate(glClient, 0);
    ck_assert_msg(response, "Expected response");

    ck_assert_uint_eq(response->resultsSize, 2);
    ck_assert_uint_eq(response->responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    checkMethodResult(&response->results[0], false);
    checkMethodResult(&response->results[1], true);
    UA_CallResponse_delete(response);
} END_TEST

static Suite* method_async_suite(void) {
    /* set up unit test for internal data structures */
    Suite *s = suite_create("Async Method");

    TCase* tc_manager = tcase_create("AsyncMethod");
    tcase_add_checked_fixture(tc_manager, setup, teardown);
    tcase_add_test(tc_manager, Async_call);
    tcase_add_test(tc_manager, Async_timeout);
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
