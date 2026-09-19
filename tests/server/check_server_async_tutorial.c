/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "test_helpers.h"
#include "testing_clock.h"
#include "ua_server_internal.h"
#include "ua_services.h"
#include <open62541/client_subscriptions.h>
#include <check.h>

/* Exercise the actual tutorial callbacks, not a copy of the example. */
int tutorial_main(void);
#define main tutorial_main
#include "../../examples/tutorial_server_method_async.c"
#undef main

static size_t results;
static UA_StatusCode expectedStatus;

static void
methodResult(UA_Server *server, void *context, const UA_CallMethodResult *result) {
    results++;
    ck_assert_uint_eq(result->statusCode, expectedStatus);
    if(expectedStatus == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(result->outputArgumentsSize, 1);
        ck_assert(UA_Variant_hasScalarType(&result->outputArguments[0],
                                           &UA_TYPES[UA_TYPES_STRING]));
        UA_String expected = UA_STRING("Hello World");
        ck_assert(UA_String_equal((UA_String*)result->outputArguments[0].data, &expected));
    } else {
        ck_assert_uint_eq(result->outputArgumentsSize, 0);
    }
}

/* Normal completion, timeout, shutdown, and synchronous facade.
 * Every case leaves two independent timed workers outstanding. */
START_TEST(tutorial_completion) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    ck_assert_uint_eq(addHelloWorldMethod(server), UA_STATUSCODE_GOOD);
    server->config.asyncOperationTimeout = (_i == 1) ? 500 : 5000;
    if(_i < 3)
        ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    results = 0;
    expectedStatus = UA_STATUSCODE_GOOD;

    UA_String text = UA_STRING("World");
    UA_Variant input;
    UA_Variant_setScalar(&input, &text, &UA_TYPES[UA_TYPES_STRING]);
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NS0ID(OBJECTSFOLDER);
    request.methodId = UA_NODEID_NUMERIC(1, 62541);
    request.inputArgumentsSize = 1;
    request.inputArguments = &input;
    for(size_t i = 0; i < 2; i++) {
        if(_i >= 3) {
            UA_CallMethodResult result = UA_Server_call(server, &request);
            ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADWAITINGFORRESPONSE);
            UA_CallMethodResult_clear(&result);
        } else {
            ck_assert_uint_eq(UA_Server_call_async(server, &request, methodResult,
                                                    &results, _i == 1 ? 500 : 0),
                              UA_STATUSCODE_GOOD);
        }
    }
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 2);
    if(_i >= 3) {
        /* The server has not started, but the async driver owns workers. */
        ck_assert_int_eq(server->state, UA_LIFECYCLESTATE_STOPPED);
        ck_assert_int_eq(server->asyncManager.driver.state, UA_LIFECYCLESTATE_STOPPING);
        ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_BADINTERNALERROR);
        if(_i == 3)
            ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
        else {
            UA_EventLoop *el = server->config.eventLoop;
            if(el->state != UA_EVENTLOOPSTATE_STARTED)
                ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
        }
    }
    if(_i == 1) {
        expectedStatus = UA_STATUSCODE_BADTIMEOUT;
        UA_fakeSleep(1000);
        UA_Server_run_iterate(server, false);
        UA_Server_run_iterate(server, false);
    } else if(_i == 2) {
        expectedStatus = UA_STATUSCODE_BADSHUTDOWN;
        server->config.externalEventLoop = true;
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
        ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPING);
        UA_Server_run_iterate(server, false);
    }
    ck_assert_uint_eq(results, (_i == 1 || _i == 2) ? 2 : 0);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 2);

    UA_fakeSleep(2000);
    for(size_t i = 0; i < 10; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(results, _i >= 3 ? 0 : 2);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    if(_i == 4) {
        ck_assert_int_eq(server->asyncManager.driver.state, UA_LIFECYCLESTATE_STOPPED);
        ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    }
    if(_i == 2) {
        ck_assert_int_eq(UA_Server_getLifecycleState(server), UA_LIFECYCLESTATE_STOPPED);
        server->config.externalEventLoop = false;
    } else {
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static void
unexpectedRead(UA_Server *server, void *context, const UA_DataValue *result) {
    ck_abort_msg("Rejected request invoked its result callback");
}

static void
unexpectedWrite(UA_Server *server, void *context, UA_StatusCode result) {
    ck_abort_msg("Rejected request invoked its result callback");
}

static void
checkLocalAdmissionClosed(UA_Server *server) {
    UA_ReadValueId read;
    UA_ReadValueId_init(&read);
    ck_assert_uint_eq(UA_Server_read_async(server, &read, UA_TIMESTAMPSTORETURN_NEITHER,
                                           unexpectedRead, NULL, 0),
                      UA_STATUSCODE_BADSHUTDOWN);
    UA_WriteValue write;
    UA_WriteValue_init(&write);
    ck_assert_uint_eq(UA_Server_write_async(server, &write, unexpectedWrite, NULL, 0),
                      UA_STATUSCODE_BADSHUTDOWN);
    UA_CallMethodRequest call;
    UA_CallMethodRequest_init(&call);
    ck_assert_uint_eq(UA_Server_call_async(server, &call, methodResult, NULL, 0),
                      UA_STATUSCODE_BADSHUTDOWN);
}

static size_t stoppingNotifications;

static void
recursiveShutdown(UA_Server *server, UA_LifecycleState state) {
    if(state != UA_LIFECYCLESTATE_STOPPING)
        return;
    stoppingNotifications++;
    checkLocalAdmissionClosed(server);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(server->state, UA_LIFECYCLESTATE_STOPPING);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_BADINTERNALERROR);
}

/* No workers with an owned loop; pending workers with an external loop. */
START_TEST(shutdown_already_stopping) {
    UA_Server *server = UA_Server_newForUnitTest();
    checkLocalAdmissionClosed(server);
    ck_assert_uint_eq(addHelloWorldMethod(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    server->config.externalEventLoop = (_i == 1);
    server->config.notifyLifecycleState = recursiveShutdown;
    stoppingNotifications = 0;
    results = 0;
    expectedStatus = UA_STATUSCODE_BADSHUTDOWN;
    UA_String text = UA_STRING("World");
    UA_Variant input;
    UA_Variant_setScalar(&input, &text, &UA_TYPES[UA_TYPES_STRING]);
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NS0ID(OBJECTSFOLDER);
    request.methodId = UA_NODEID_NUMERIC(1, 62541);
    request.inputArgumentsSize = 1;
    request.inputArguments = &input;
    if(_i == 1)
        ck_assert_uint_eq(UA_Server_call_async(server, &request, methodResult, NULL, 0),
                          UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(stoppingNotifications, 1);
    if(_i == 1) {
        ck_assert_int_eq(server->state, UA_LIFECYCLESTATE_STOPPING);
        ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(stoppingNotifications, 1);
        ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 1);
        ck_assert_uint_eq(results, 0);
        UA_fakeSleep(2000);
        for(size_t i = 0; i < 10; i++)
            UA_Server_run_iterate(server, false);
    }
    ck_assert_int_eq(server->state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_int_eq(server->asyncManager.driver.state, UA_LIFECYCLESTATE_STOPPED);
    checkLocalAdmissionClosed(server);
    ck_assert_uint_eq(results, _i == 1 ? 1 : 0);
    server->config.externalEventLoop = false;
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

/* Also runs without multithreading: timer-driven services still need session
 * cancellation, and the response must not wait for the timer to finish. */
START_TEST(session_close_releases_response) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_uint_eq(addHelloWorldMethod(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_CreateSessionRequest create;
    UA_CreateSessionRequest_init(&create);
    UA_Session *session = NULL;
    lockServer(server);
    ck_assert_uint_eq(UA_Session_create(server, NULL, &create, &session),
                      UA_STATUSCODE_GOOD);
    UA_String text = UA_STRING("World");
    UA_Variant input;
    UA_Variant_setScalar(&input, &text, &UA_TYPES[UA_TYPES_STRING]);
    UA_CallMethodRequest method;
    UA_CallMethodRequest_init(&method);
    method.objectId = UA_NS0ID(OBJECTSFOLDER);
    method.methodId = UA_NODEID_NUMERIC(1, 62541);
    method.inputArgumentsSize = 1;
    method.inputArguments = &input;
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    request.methodsToCallSize = 1;
    request.methodsToCall = &method;
    UA_CallResponse response;
    UA_CallResponse_init(&response);
    ck_assert(!Service_Call(server, session, &request, &response));
    UA_AsyncResponse *ar = TAILQ_FIRST(&server->asyncManager.responses);
    ck_assert_ptr_ne(ar, NULL);
    ck_assert_ptr_eq(ar->session, session);
    UA_Session_remove(server, session, UA_SHUTDOWNREASON_CLOSE);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    UA_AsyncOperation *op = TAILQ_FIRST(&server->asyncManager.operations);
    ck_assert_ptr_ne(op, NULL);
    ck_assert_ptr_eq(op->handling.response, NULL);
    ck_assert(op->resultIndex == SIZE_MAX);
    unlockServer(server);
    UA_Server_run_iterate(server, false);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 1);
    UA_fakeSleep(2000);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    UA_CallResponse_clear(&response);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static void
checkDispatchGuard(UA_Server *server) {
    ck_assert_uint_gt(server->asyncManager.activeDispatch, 0);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(UA_Server_run_shutdown(server),
        server->state == UA_LIFECYCLESTATE_STOPPING ?
        UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_BADINTERNALERROR);
}

static UA_StatusCode
guardedMethod(UA_Server *server,
              const UA_NodeId *sessionId, void *sessionHandle,
              const UA_NodeId *methodId, void *methodContext,
              const UA_NodeId *objectId, void *objectContext,
              size_t inputSize, const UA_Variant *input,
              size_t outputSize, UA_Variant *output) {
    checkDispatchGuard(server);
    return helloWorldMethodCallback1(server, sessionId, sessionHandle, methodId,
                                    methodContext, objectId, objectContext,
                                    inputSize, input, outputSize, output);
}

static void
guardedResult(UA_Server *server, void *context, const UA_CallMethodResult *result) {
    checkDispatchGuard(server);
    methodResult(server, context, result);
}

static void
guardedCancel(UA_Server *server, const void *id) {
    checkDispatchGuard(server);
}

static void
guardedReadResult(UA_Server *server, void *context, const UA_DataValue *result) {
    checkDispatchGuard(server);
    ck_assert_uint_eq(result->status, UA_STATUSCODE_BADNODEIDUNKNOWN);
}

static size_t serviceEnds;
static void
closeAtServiceEnd(UA_Server *server, UA_ApplicationNotificationType type,
                  const UA_KeyValueMap payload) {
    if(type != UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_END)
        return;
    serviceEnds++;
    checkDispatchGuard(server);
    UA_AsyncResponse *ar = TAILQ_FIRST(&server->asyncManager.responses);
    ck_assert_ptr_ne(ar, NULL);
    ck_assert(!ar->dc.callback);
    ck_assert_uint_gt(ar->response.readResponse.resultsSize, 0);
    UA_Session *session = ar->session;
    ck_assert_ptr_ne(session, NULL);
    UA_AsyncManager_abandon(server, session->channel, ar->responseToken);
    ck_assert(ar->abandoned);
    UA_Session_remove(server, session, UA_SHUTDOWNREASON_CLOSE);
    ck_assert_int_eq(session->state, UA_SESSIONSTATE_CLOSED);
    ck_assert_ptr_eq(getSessionById(server, &session->sessionId), NULL);
}

START_TEST(service_dispatch_notification) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_uint_eq(addHelloWorldMethod(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_setMethodNodeCallback(server, UA_NODEID_NUMERIC(1, 62541),
                                                     guardedMethod), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_CreateSessionRequest create;
    UA_CreateSessionRequest_init(&create);
    UA_Session *session = NULL;
    lockServer(server);
    ck_assert_uint_eq(UA_Session_create(server, NULL, &create, &session), UA_STATUSCODE_GOOD);
    UA_String text = UA_STRING("World");
    UA_Variant input;
    UA_Variant_setScalar(&input, &text, &UA_TYPES[UA_TYPES_STRING]);
    UA_CallMethodRequest methods[2];
    for(size_t i = 0; i < 2; i++) {
        UA_CallMethodRequest_init(&methods[i]);
        methods[i].objectId = UA_NS0ID(OBJECTSFOLDER);
        methods[i].methodId = UA_NODEID_NUMERIC(1, 62541);
        methods[i].inputArgumentsSize = 1;
        methods[i].inputArguments = &input;
    }
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    request.methodsToCallSize = 2;
    request.methodsToCall = methods;
    UA_CallResponse response;
    UA_CallResponse_init(&response);
    ck_assert(!Service_Call(server, session, &request, &response));
    unlockServer(server);
    serviceEnds = 0;
    server->config.globalNotificationCallback = closeAtServiceEnd;
    UA_fakeSleep(2000);
    for(size_t i = 0; i < 4; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(serviceEnds, 1);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    server->config.globalNotificationCallback = NULL;
    UA_CallResponse_clear(&response);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(dispatch_guards_and_pool_bound) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_uint_eq(addHelloWorldMethod(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_setMethodNodeCallback(server, UA_NODEID_NUMERIC(1, 62541),
                                                     guardedMethod), UA_STATUSCODE_GOOD);
    server->config.asyncOperationCancelCallback = guardedCancel;
    UA_String text = UA_STRING("World");
    UA_Variant input;
    UA_Variant_setScalar(&input, &text, &UA_TYPES[UA_TYPES_STRING]);
    UA_CallMethodRequest request;
    UA_CallMethodRequest_init(&request);
    request.objectId = UA_NS0ID(OBJECTSFOLDER);
    request.methodId = UA_NODEID_NUMERIC(1, 62541);
    request.inputArgumentsSize = 1;
    request.inputArguments = &input;

    /* A pre-start facade must pin its dispatch frame as well. */
    UA_CallMethodResult result = UA_Server_call(server, &request);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_BADWAITINGFORRESPONSE);
    UA_CallMethodResult_clear(&result);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    results = 0;
    expectedStatus = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < 32; i++)
        ck_assert_uint_eq(UA_Server_call_async(server, &request, guardedResult, NULL, 0),
                          UA_STATUSCODE_GOOD);
    /* Check inline result callbacks, not just event-loop delivery. */
    UA_ReadValueId read;
    UA_ReadValueId_init(&read);
    read.nodeId = UA_NODEID_NUMERIC(1, 999999);
    ck_assert_uint_eq(UA_Server_read_async(server, &read, UA_TIMESTAMPSTORETURN_NEITHER,
                                           guardedReadResult, NULL, 0), UA_STATUSCODE_GOOD);
    UA_fakeSleep(2000);
    for(size_t i = 0; i < 4; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(results, 32);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    ck_assert_uint_eq(server->asyncManager.freeOpsSize, 16);
    size_t cached = 0;
    UA_AsyncOperation *op;
    for(op = server->asyncManager.freeOps; op; op = TAILQ_NEXT(op, pointers))
        cached++;
    ck_assert_uint_eq(cached, 16);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static size_t initialSamples;
static void
initialSample(UA_Server *server, UA_UInt32 id, void *context,
              const UA_NodeId *nodeId, void *nodeContext, UA_UInt32 attributeId,
              const UA_DataValue *value) {
    ck_assert_uint_eq(value->status, UA_STATUSCODE_GOOD);
    ck_assert(value->hasValue);
    initialSamples++;
}

START_TEST(monitored_item_before_startup) {
    UA_Server *server = UA_Server_newForUnitTest();
    initialSamples = 0;
    UA_MonitoredItemCreateRequest request = UA_MonitoredItemCreateRequest_default(
        UA_NS0ID(SERVER_SERVERSTATUS_CURRENTTIME));
    UA_MonitoredItemCreateResult result = UA_Server_createDataChangeMonitoredItem(
        server, UA_TIMESTAMPSTORETURN_SOURCE, request, NULL, initialSample);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_MonitoredItem *mon = UA_Subscription_getMonitoredItem(server->adminSubscription,
                                                            result.monitoredItemId);
    ck_assert_ptr_ne(mon, NULL);
    ck_assert(!mon->lastValue.hasValue);
    ck_assert_uint_eq(initialSamples, 0);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    UA_MonitoredItemCreateResult_clear(&result);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert(!mon->lastValue.hasValue);
    ck_assert_uint_eq(initialSamples, 0);
    UA_fakeSleep(1000);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_ge(initialSamples, 1);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static UA_StatusCode (*savedAddTimer)(UA_EventLoop*, UA_Callback, void*, void*, UA_Double,
                                    UA_DateTime*, UA_TimerPolicy, UA_UInt64*);
static void *rejectedTimerData;

static UA_StatusCode
rejectTimer(UA_EventLoop *el, UA_Callback cb, void *application, void *data,
            UA_Double interval, UA_DateTime *baseTime, UA_TimerPolicy policy,
            UA_UInt64 *timerId) {
    if(data == rejectedTimerData)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    return savedAddTimer(el, cb, application, data, interval, baseTime, policy, timerId);
}

START_TEST(timeout_registration_failure) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_EventLoop *el = server->config.eventLoop;
    /* Exercise both housekeeping and async-driver startup failures. */
    if(el->state != UA_EVENTLOOPSTATE_STARTED)
        ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    savedAddTimer = el->addTimer;
    rejectedTimerData = _i == 0 ? NULL : &server->asyncManager;
    el->addTimer = rejectTimer;
    volatile UA_Boolean running = false;
    UA_StatusCode status = _i == 2 ? UA_Server_run(server, &running) :
        (_i == 3 ? UA_Server_runUntilInterrupt(server) : UA_Server_run_startup(server));
    ck_assert_uint_eq(status, UA_STATUSCODE_BADOUTOFMEMORY);
    ck_assert_int_eq(server->state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(server->asyncManager.checkTimeoutCallbackId, 0);
    checkLocalAdmissionClosed(server);
    el->addTimer = savedAddTimer;
    ck_assert_int_eq(server->state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_int_eq(server->asyncManager.driver.state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(server->asyncManager.checkTimeoutCallbackId, 0);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static UA_DataValue *driverOutputs[2];
static size_t driverReads, driverCancels;
static int driverCase;

static UA_StatusCode
driverRead(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
           const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimestamp,
           const UA_NumericRange *range, UA_DataValue *value) {
    ck_assert_uint_lt(driverReads, 2);
    driverOutputs[driverReads++] = value;
    if(driverCase == 4 || (driverCase == 2 && driverReads == 2)) {
        UA_Int32 number = 1;
        ck_assert_uint_eq(UA_Variant_setScalarCopy(&value->value, &number,
                                                  &UA_TYPES[UA_TYPES_INT32]),
                          UA_STATUSCODE_GOOD);
        value->hasValue = true;
    }
    if(driverCase == 1 || (driverCase == 2 && driverReads == 2)) {
        UA_Driver *driver = &server->asyncManager.driver;
        driver->stop(driver);
        ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STOPPING);
        ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_BADINVALIDSTATE);
        if(driverCase == 2) {
            /* Canceling earlier operations must not touch this in-flight slot. */
            ck_assert(value->hasValue);
            ck_assert(UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_INT32]));
            ck_assert_int_eq(*(UA_Int32*)value->value.data, 1);
        }
    }
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static void
driverCancel(UA_Server *server, const void *operation) {
    driverCancels++;
    UA_Driver *driver = &server->asyncManager.driver;
    ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STOPPING);
    driver->stop(driver); /* Reentrant stop must not repeat notifications. */
    if(driverCase == 0 || driverCase == 3)
        ck_assert_uint_eq(results, 1); /* Result delivery precedes cancellation notice. */
    if(driverCase == 3) {
        ck_assert_ptr_eq(operation, driverOutputs[0]);
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, driverOutputs[0]),
                          UA_STATUSCODE_GOOD);
    }
    ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STOPPING);
}

static void
driverReadResult(UA_Server *server, void *context, const UA_DataValue *value) {
    results++;
    ck_assert_uint_eq(value->status, driverCase == 4 ?
                      UA_STATUSCODE_GOOD : UA_STATUSCODE_BADSHUTDOWN);
    ck_assert_int_eq(server->asyncManager.driver.state, UA_LIFECYCLESTATE_STOPPING);
}

/* Explicit stop, stop during initiation, stop during a multi-operation service,
 * immediate worker acknowledgement from cancellation, and an already-ready result. */
START_TEST(async_driver_lifecycle) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Driver *driver = &server->asyncManager.driver;
    UA_Driver *registered = server->drivers;
    while(registered && registered != driver)
        registered = registered->next;
    ck_assert_ptr_eq(registered, driver);
    ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STOPPED);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 number = 1;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Variant_setScalar(&attr.value, &number, &UA_TYPES[UA_TYPES_INT32]);
    UA_NodeId id = UA_NODEID_NUMERIC(1, 60001);
    ck_assert_uint_eq(UA_Server_addVariableNode(server, id, UA_NS0ID(OBJECTSFOLDER),
        UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "driver"), UA_NS0ID(BASEDATAVARIABLETYPE),
        attr, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_CallbackValueSource source = {driverRead, NULL};
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, id, source),
                      UA_STATUSCODE_GOOD);
    server->config.asyncOperationCancelCallback = driverCancel;
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STARTED);
    results = driverReads = driverCancels = 0;
    driverCase = _i;
    UA_ReadValueId reads[2];
    UA_ReadValueId_init(&reads[0]);
    reads[0].nodeId = id;
    reads[0].attributeId = UA_ATTRIBUTEID_VALUE;
    reads[1] = reads[0];
    if(_i == 2) {
        UA_ReadRequest request;
        UA_ReadRequest_init(&request);
        request.nodesToRead = reads;
        request.nodesToReadSize = 2;
        UA_ReadResponse response;
        UA_ReadResponse_init(&response);
        lockServer(server);
        ck_assert(Service_Read(server, &server->adminSession, &request, &response));
        unlockServer(server);
        ck_assert_uint_eq(response.responseHeader.serviceResult, UA_STATUSCODE_BADSHUTDOWN);
        for(size_t i = 0; i < 2; i++)
            ck_assert_uint_eq(response.results[i].status, UA_STATUSCODE_BADSHUTDOWN);
        UA_ReadResponse_clear(&response);
    } else {
        ck_assert_uint_eq(UA_Server_read_async(server, reads, UA_TIMESTAMPSTORETURN_NEITHER,
            driverReadResult, NULL, 0), _i == 1 ? UA_STATUSCODE_BADSHUTDOWN : UA_STATUSCODE_GOOD);
    }
    if(_i == 4)
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, driverOutputs[0]),
                          UA_STATUSCODE_GOOD);
    lockServer(server);
    driver->stop(driver);
    ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STOPPING);
    ck_assert_uint_eq(driver->free(driver), UA_STATUSCODE_BADINVALIDSTATE);
    unlockServer(server);
    ck_assert_uint_eq(driverCancels, _i == 1 ? 1 : 0);
    ck_assert_int_eq(server->state, UA_LIFECYCLESTATE_STARTED);
    checkLocalAdmissionClosed(server);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(driverCancels, _i == 4 ? 0 : driverReads);
    ck_assert_uint_eq(results, (_i == 0 || _i >= 3) ? 1 : 0);
    if(_i < 3) {
        ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STOPPING);
        for(size_t i = 0; i < driverReads; i++)
            ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, driverOutputs[i]),
                              UA_STATUSCODE_GOOD);
    }
    ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    lockServer(server);
    ck_assert_uint_eq(driver->start(driver), UA_STATUSCODE_GOOD);
    unlockServer(server);
    ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STARTED);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(driver->state, UA_LIFECYCLESTATE_STOPPED);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static UA_DataValue *emptyReadOutput;
static UA_StatusCode
emptyAsyncRead(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
               const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimestamp,
               const UA_NumericRange *range, UA_DataValue *value) {
    emptyReadOutput = value;
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static void
normalizedReadResult(UA_Server *server, void *context, const UA_DataValue *value) {
    results++;
    ck_assert(value->hasStatus);
    ck_assert_uint_eq(value->status, UA_STATUSCODE_BADWAITINGFORINITIALDATA);
    ck_assert(!value->hasValue);
    ck_assert(value->hasSourceTimestamp);
    ck_assert(value->hasServerTimestamp);
}

static UA_Session *closingSession;
static size_t closingEnds, closingCancels, closingNotifications;
static UA_Boolean completeDuringClose, iterateDuringClose;

static void
sessionCloseNotification(UA_Server *server, UA_ApplicationNotificationType type,
                         const UA_KeyValueMap payload) {
    if(type == UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_END) {
        const UA_NodeId *id = (const UA_NodeId*)UA_KeyValueMap_getScalar(
            &payload, UA_QUALIFIEDNAME(0, "session-id"), &UA_TYPES[UA_TYPES_NODEID]);
        const UA_UInt32 *channelId = (const UA_UInt32*)UA_KeyValueMap_getScalar(
            &payload, UA_QUALIFIEDNAME(0, "securechannel-id"), &UA_TYPES[UA_TYPES_UINT32]);
        ck_assert_ptr_ne(id, NULL);
        ck_assert_ptr_ne(channelId, NULL);
        ck_assert(UA_NodeId_equal(id, &closingSession->sessionId));
        ck_assert_uint_eq(*channelId, 123);
        ck_assert_ptr_ne(closingSession->channel, NULL);
        ck_assert_ptr_eq(findSessionByToken(server, &closingSession->authenticationToken),
                         closingSession);
        ck_assert_int_eq(closingSession->state, UA_SESSIONSTATE_CLOSED);
        ck_assert_uint_eq(closingNotifications, 0);
        closingEnds++;
        /* Reentry must not detach the Session beneath this notification. */
        UA_Session_remove(server, closingSession, UA_SHUTDOWNREASON_CLOSE);
        ck_assert_ptr_ne(closingSession->channel, NULL);
        if(iterateDuringClose) {
            UA_AsyncResponse *ar = TAILQ_FIRST(&server->asyncManager.responses);
            while(ar->session != closingSession)
                ar = TAILQ_NEXT(ar, pointers);
            UA_Server_run_iterate(server, false);
            /* Inline delivery still owns ar even if its queued callback ran. */
            UA_AsyncResponse *cached;
            for(cached = server->asyncManager.freeResponses; cached;
                cached = TAILQ_NEXT(cached, pointers))
                ck_assert_ptr_ne(cached, ar);
            ck_assert_ptr_eq(ar->session, closingSession);
        }
    } else if(type == UA_APPLICATIONNOTIFICATIONTYPE_SESSION_CLOSED) {
        ck_assert_uint_eq(closingEnds, 2);
        ck_assert_ptr_eq(closingSession->channel, NULL);
        closingNotifications++;
    }
}

static void
sessionCloseCancellation(UA_Server *server, const void *id) {
    ck_assert_uint_gt(closingEnds, 0);
    ck_assert_uint_eq(closingNotifications, 0);
    closingCancels++;
    if(completeDuringClose)
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, (UA_DataValue*)(uintptr_t)id),
                          UA_STATUSCODE_GOOD);
}

static void
closeSessionFromDelayedCallback(void *application, void *context) {
    UA_Server *server = (UA_Server*)application;
    UA_Session *session = (UA_Session*)context;
    lockServer(server);
    UA_Session_remove(server, session, session->validTill == 0 ?
                       UA_SHUTDOWNREASON_TIMEOUT : UA_SHUTDOWNREASON_CLOSE);
    unlockServer(server);
}

/* Pending and already-ready responses, including an expired Session and
 * reentrant completion/iteration. Also close from the same delayed-callback
 * batch as the ready response, so it cannot be removed from the pending queue. */
START_TEST(session_close_finishes_services) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_NodeId id = UA_NODEID_NUMERIC(1, 60002);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    ck_assert_uint_eq(UA_Server_addVariableNode(server, id, UA_NS0ID(OBJECTSFOLDER),
        UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "closing"), UA_NS0ID(BASEDATAVARIABLETYPE),
        attr, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_CallbackValueSource source = {emptyAsyncRead, NULL};
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, id, source),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    lockServer(server);
    UA_CreateSessionRequest create;
    UA_CreateSessionRequest_init(&create);
    ck_assert_uint_eq(UA_Session_create(server, NULL, &create, &closingSession),
                      UA_STATUSCODE_GOOD);
    UA_SecureChannel channel;
    UA_SecureChannel_init(&channel);
    channel.securityToken.channelId = 123;
    UA_Session_attachToSecureChannel(server, closingSession, &channel);
    UA_ReadValueId read;
    UA_ReadValueId_init(&read);
    read.nodeId = id;
    read.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToRead = &read;
    request.nodesToReadSize = 1;
    UA_DataValue *outputs[2];
    for(size_t i = 0; i < 2; i++) {
        UA_ReadResponse response;
        UA_ReadResponse_init(&response);
        ck_assert(!Service_Read(server, closingSession, &request, &response));
        outputs[i] = emptyReadOutput;
        UA_ReadResponse_clear(&response);
    }
    UA_Boolean ready = (_i & 1) != 0;
    UA_DelayedCallback close = {0};
    if(_i & 16) {
        close.callback = closeSessionFromDelayedCallback;
        close.application = server;
        close.context = closingSession;
        server->config.eventLoop->addDelayedCallback(server->config.eventLoop, &close);
    }
    if(ready)
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, outputs[0]), UA_STATUSCODE_GOOD);
    if(_i & 2)
        closingSession->validTill = 0;
    completeDuringClose = (_i & 4) != 0;
    iterateDuringClose = (_i & 8) != 0;
    closingEnds = closingCancels = closingNotifications = 0;
    server->config.globalNotificationCallback = sessionCloseNotification;
    server->config.asyncOperationCancelCallback = sessionCloseCancellation;
    if(_i & 16)
        UA_Server_run_iterate(server, false);
    else
        UA_Session_remove(server, closingSession, (_i & 2) ?
                           UA_SHUTDOWNREASON_TIMEOUT : UA_SHUTDOWNREASON_CLOSE);
    ck_assert_uint_eq(closingEnds, 2);
    ck_assert_uint_eq(closingNotifications, 1);
    ck_assert_uint_eq(closingCancels, ready ? 1 : 2);
    ck_assert_ptr_eq(channel.sessions, NULL);
    if(!completeDuringClose) {
        for(size_t i = ready ? 1 : 0; i < 2; i++)
            ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, outputs[i]), UA_STATUSCODE_GOOD);
    }
    if(ready && !iterateDuringClose && !(_i & 16)) {
        UA_AsyncResponse *ar = TAILQ_FIRST(&server->asyncManager.responses);
        ck_assert_ptr_nonnull(ar);
        ck_assert_ptr_null(ar->session);
        ck_assert_uint_eq(ar->response.readResponse.resultsSize, 0);
        ck_assert(ar->dc.callback); /* Queued cleanup, no second notification */
    } else {
        ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    }
    unlockServer(server);
    UA_Server_run_iterate(server, false);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    ck_assert_uint_eq(closingEnds, 2);
    server->config.globalNotificationCallback = NULL;
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static UA_ReadValueId pooledRead;
static size_t pooledResults, pooledCancels;

START_TEST(response_callback_pool) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_NodeId id = UA_NODEID_NUMERIC(1, 60003);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    ck_assert_uint_eq(UA_Server_addVariableNode(server, id, UA_NS0ID(OBJECTSFOLDER),
        UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "response-pool"), UA_NS0ID(BASEDATAVARIABLETYPE),
        attr, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_CallbackValueSource source = {emptyAsyncRead, NULL};
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, id, source),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_ReadValueId read;
    UA_ReadValueId_init(&read);
    read.nodeId = id;
    read.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToRead = &read;
    request.nodesToReadSize = 1;
    UA_ReadResponse response;
    UA_ReadResponse_init(&response);
    UA_AsyncManager *am = &server->asyncManager;
    UA_AsyncResponse *records[32];
    UA_DataValue *outputs[32];
    lockServer(server);
    for(size_t i = 0; i < 32; i++) {
        ck_assert(!Service_Read(server, &server->adminSession, &request, &response));
        records[i] = (i == 0) ? TAILQ_FIRST(&am->responses) :
            TAILQ_NEXT(records[i - 1], pointers);
        outputs[i] = emptyReadOutput;
    }
    /* Complete later responses first, leaving the list head pending. */
    for(size_t i = 16; i < 32; i++)
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, outputs[i]), UA_STATUSCODE_GOOD);
    unlockServer(server);
    UA_Server_run_iterate(server, false);
    ck_assert_ptr_eq(TAILQ_FIRST(&am->responses), records[0]);
    ck_assert_uint_eq(am->freeResponsesSize, 16);
    UA_AsyncResponse *cached = am->freeResponses;
    for(size_t i = 32; i-- > 16;) {
        ck_assert_ptr_eq(cached, records[i]);
        cached = TAILQ_NEXT(cached, pointers);
    }
    ck_assert_ptr_null(cached);
    lockServer(server);
    for(size_t i = 0; i < 16; i++)
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, outputs[i]), UA_STATUSCODE_GOOD);
    unlockServer(server);
    UA_Server_run_iterate(server, false);
    ck_assert(TAILQ_EMPTY(&am->responses));
    ck_assert_uint_eq(am->freeResponsesSize, 16); /* Bound the retained peak */

    /* Cancellation releases the response independently of outstanding output. */
    UA_AsyncResponse *reused = am->freeResponses;
    lockServer(server);
    ck_assert(!Service_Read(server, &server->adminSession, &request, &response));
    ck_assert_ptr_eq(TAILQ_FIRST(&am->responses), reused);
    UA_DataValue *late = emptyReadOutput;
    ck_assert_uint_eq(UA_AsyncManager_cancel(server, &server->adminSession, 0), 1);
    unlockServer(server);
    UA_Server_run_iterate(server, false);
    ck_assert_ptr_eq(am->freeResponses, reused);
    ck_assert_uint_eq(am->trackedOpsCount, 1);
    lockServer(server);
    ck_assert(!Service_Read(server, &server->adminSession, &request, &response));
    ck_assert_ptr_eq(TAILQ_FIRST(&am->responses), reused);
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, late), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(reused->pendingResults, 1);
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, emptyReadOutput), UA_STATUSCODE_GOOD);
    unlockServer(server);
    UA_Server_run_iterate(server, false);
    ck_assert(TAILQ_EMPTY(&am->responses));
    ck_assert_uint_eq(am->trackedOpsCount, 0);
    ck_assert_ptr_eq(am->freeResponses, reused);
    /* Synchronous dispatch returns the payload, but immediately recycles metadata. */
    read.attributeId = UA_ATTRIBUTEID_BROWSENAME;
    lockServer(server);
    ck_assert(Service_Read(server, &server->adminSession, &request, &response));
    unlockServer(server);
    ck_assert_uint_eq(response.resultsSize, 1);
    ck_assert(response.results[0].hasValue);
    ck_assert_ptr_eq(am->freeResponses, reused);
    UA_ReadResponse_clear(&response);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static void
pooledReadResult(UA_Server *server, void *context, const UA_DataValue *result) {
    pooledResults++;
}

static void
recycleCanceledServiceOperation(UA_Server *server, const void *id) {
    pooledCancels++;
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, (UA_DataValue*)(uintptr_t)id),
                      UA_STATUSCODE_GOOD);
    /* Reuse the just-completed service operation for local delivery while
     * the cancellation pass is still traversing outstanding operations. */
    ck_assert_uint_eq(UA_Server_read_async(server, &pooledRead, UA_TIMESTAMPSTORETURN_NEITHER,
        pooledReadResult, NULL, 0), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(emptyReadOutput, id);
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, emptyReadOutput), UA_STATUSCODE_GOOD);
}

START_TEST(service_and_local_operation_pool) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_NodeId id = UA_NODEID_NUMERIC(1, 60003);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    ck_assert_uint_eq(UA_Server_addVariableNode(server, id, UA_NS0ID(OBJECTSFOLDER),
        UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "pool"), UA_NS0ID(BASEDATAVARIABLETYPE),
        attr, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_CallbackValueSource source = {emptyAsyncRead, NULL};
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, id, source),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_ReadValueId_init(&pooledRead);
    pooledRead.nodeId = id;
    pooledRead.attributeId = UA_ATTRIBUTEID_VALUE;
    pooledResults = pooledCancels = 0;
    ck_assert_uint_eq(UA_Server_read_async(server, &pooledRead, UA_TIMESTAMPSTORETURN_NEITHER,
        pooledReadResult, NULL, 0), UA_STATUSCODE_GOOD);
    UA_DataValue *firstOutput = emptyReadOutput;
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, firstOutput), UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(pooledResults, 1);

    UA_ReadValueId reads[32];
    for(size_t i = 0; i < 32; i++)
        reads[i] = pooledRead;
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToRead = reads;
    request.nodesToReadSize = 1;
    UA_ReadResponse response;
    UA_ReadResponse_init(&response);
    lockServer(server);
    ck_assert(!Service_Read(server, &server->adminSession, &request, &response));
    ck_assert_ptr_eq(emptyReadOutput, firstOutput); /* Local -> service */
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, emptyReadOutput), UA_STATUSCODE_GOOD);
    ck_assert(!TAILQ_EMPTY(&server->asyncManager.responses));
    ck_assert_uint_eq(UA_Server_read_async(server, &pooledRead, UA_TIMESTAMPSTORETURN_NEITHER,
        pooledReadResult, NULL, 0), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(emptyReadOutput, firstOutput); /* Service -> local, before delivery */
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, emptyReadOutput), UA_STATUSCODE_GOOD);
    unlockServer(server);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(pooledResults, 2);

    server->config.asyncOperationCancelCallback = recycleCanceledServiceOperation;
    request.nodesToReadSize = 2;
    lockServer(server);
    ck_assert(!Service_Read(server, &server->adminSession, &request, &response));
    ck_assert_uint_eq(UA_AsyncManager_cancel(server, &server->adminSession, 0), 1);
    unlockServer(server);
    for(size_t i = 0; i < 3; i++)
        UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(pooledCancels, 2);
    ck_assert_uint_eq(pooledResults, 4);
    ck_assert(TAILQ_EMPTY(&server->asyncManager.responses));
    ck_assert(TAILQ_EMPTY(&server->asyncManager.operations));

    /* Completed service operations obey the same bounded warm-pool policy. */
    request.nodesToReadSize = 32;
    lockServer(server);
    ck_assert(!Service_Read(server, &server->adminSession, &request, &response));
    UA_AsyncOperation *op;
    while((op = TAILQ_FIRST(&server->asyncManager.operations)))
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, &op->output.read), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->asyncManager.freeOpsSize, 16);
    unlockServer(server);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    UA_ReadResponse_clear(&response);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(async_read_normalization) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 number = 1;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Variant_setScalar(&attr.value, &number, &UA_TYPES[UA_TYPES_INT32]);
    UA_NodeId id = UA_NODEID_NUMERIC(1, 60000);
    ck_assert_uint_eq(UA_Server_addVariableNode(server, id, UA_NS0ID(OBJECTSFOLDER),
        UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "empty"), UA_NS0ID(BASEDATAVARIABLETYPE),
        attr, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_CallbackValueSource source = {emptyAsyncRead, NULL};
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, id, source),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    UA_ReadValueId read;
    UA_ReadValueId_init(&read);
    read.nodeId = id;
    read.attributeId = UA_ATTRIBUTEID_VALUE;
    results = 0;
    ck_assert_uint_eq(UA_Server_read_async(server, &read, UA_TIMESTAMPSTORETURN_BOTH,
        normalizedReadResult, NULL, 0), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, emptyReadOutput), UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(results, 1);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static size_t earlyReads;
static UA_Boolean earlySecondAsync;
static UA_DataValue *earlyReadOutput;
static UA_Boolean earlyRejection;
static UA_DataValue *earlyRejectedOutput;
static size_t earlyCancelCount;

static void
earlyCancellation(UA_Server *server, const void *operation) {
    ck_assert_ptr_eq(operation, earlyRejectedOutput);
    ck_assert_uint_eq(++earlyCancelCount, 1);
    UA_AsyncOperation *op = TAILQ_FIRST(&server->asyncManager.operations);
    ck_assert_ptr_ne(op, NULL);
    ck_assert_ptr_eq(op->handling.response, NULL);
    UA_AsyncResponse *ar = TAILQ_FIRST(&server->asyncManager.responses);
    ck_assert_ptr_ne(ar, NULL);
    ck_assert_uint_eq(ar->response.readResponse.resultsSize, 0);
    ck_assert_ptr_null(ar->session);
    ck_assert(!ar->dc.callback);
}

static UA_StatusCode
completeEarlierRead(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                     const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimestamp,
                     const UA_NumericRange *range, UA_DataValue *value) {
    UA_Int32 number = 42;
    ck_assert_uint_eq(UA_Variant_setScalarCopy(&value->value, &number, &UA_TYPES[UA_TYPES_INT32]),
                      UA_STATUSCODE_GOOD);
    value->hasValue = true;
    if(earlyReads++ == 0) {
        earlyReadOutput = value;
        return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
    }
    if(earlyRejection && earlyReads == 2) {
        earlyRejectedOutput = value;
        return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
    }

    /* Earlier operations can complete into the response array during dispatch. */
    UA_AsyncOperation *op = TAILQ_FIRST(&server->asyncManager.operations);
    ck_assert_ptr_ne(op, NULL);
    UA_AsyncResponse *ar = op->handling.response;
    ck_assert(!ar->dc.callback);
    ck_assert_uint_eq(ar->response.readResponse.resultsSize, earlyRejection ? 3 : 2);
    ck_assert_uint_eq(op->resultIndex, 0);
    UA_DataValue *destination = &ar->response.readResponse.results[op->resultIndex];
    ck_assert_ptr_ne(destination, earlyReadOutput);
    ck_assert_ptr_eq(&op->output.read, earlyReadOutput);
    ck_assert(!destination->hasValue);
    if(earlyRejection) {
        server->config.maxAsyncOperationQueueSize = 0;
    }
    ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, earlyReadOutput), UA_STATUSCODE_GOOD);
    ck_assert(!ar->dc.callback);
    ck_assert_uint_eq(ar->response.readResponse.resultsSize, earlyRejection ? 3 : 2);
    ck_assert_uint_eq(ar->pendingResults, 0);
    ck_assert(destination->hasValue);
    earlyReadOutput = value;
    return earlySecondAsync ? UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY : UA_STATUSCODE_GOOD;
}

START_TEST(completion_before_response_registration) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_NodeId id = UA_NODEID_NUMERIC(1, 60001);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 number = 1;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Variant_setScalar(&attr.value, &number, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(UA_Server_addVariableNode(server, id, UA_NS0ID(OBJECTSFOLDER),
        UA_NS0ID(ORGANIZES), UA_QUALIFIEDNAME(1, "early"), UA_NS0ID(BASEDATAVARIABLETYPE),
        attr, NULL, NULL), UA_STATUSCODE_GOOD);
    UA_CallbackValueSource source = {completeEarlierRead, NULL};
    ck_assert_uint_eq(UA_Server_setVariableNode_callbackValueSource(server, id, source),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);
    earlyReads = 0;
    earlySecondAsync = (_i % 2 != 0);
    earlyRejection = (_i >= 2);
    earlyCancelCount = 0;
    if(earlyRejection) {
        server->config.maxAsyncOperationQueueSize = 1;
        server->config.asyncOperationCancelCallback = earlyCancellation;
    }
    size_t itemsSize = earlyRejection ? 3 : 2;
    UA_ReadValueId items[3];
    for(size_t i = 0; i < itemsSize; i++) {
        UA_ReadValueId_init(&items[i]);
        items[i].nodeId = id;
        items[i].attributeId = UA_ATTRIBUTEID_VALUE;
    }
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToRead = items;
    request.nodesToReadSize = itemsSize;
    UA_ReadResponse response;
    UA_ReadResponse_init(&response);
    lockServer(server);
    ck_assert_int_eq(Service_Read(server, &server->adminSession, &request, &response),
                     !earlySecondAsync);
    if(earlySecondAsync) {
        UA_AsyncResponse *ar = TAILQ_FIRST(&server->asyncManager.responses);
        ck_assert_ptr_ne(ar, NULL);
        ck_assert(ar->dc.callback && ar->pendingResults > 0);
        ck_assert_ptr_eq(ar->session, &server->adminSession);
        if(earlyRejection) {
            ck_assert(ar->response.readResponse.results[0].hasValue);
            ck_assert_uint_eq(ar->response.readResponse.results[1].status,
                              UA_STATUSCODE_BADTOOMANYOPERATIONS);
            UA_AsyncOperation *op;
            TAILQ_FOREACH(op, &server->asyncManager.operations, pointers) {
                if(&op->output.read == earlyReadOutput)
                    break;
            }
            ck_assert_ptr_ne(op, NULL);
            ck_assert_uint_eq(op->resultIndex, 2);
            ck_assert_ptr_ne(&ar->response.readResponse.results[op->resultIndex],
                             earlyReadOutput);
        }
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, earlyReadOutput), UA_STATUSCODE_GOOD);
        ck_assert_ptr_eq(TAILQ_FIRST(&server->asyncManager.responses), ar);
        ck_assert(ar->dc.callback && ar->pendingResults == 0);
        ck_assert(ar->response.readResponse.results[itemsSize - 1].hasValue);
    } else {
        ck_assert_uint_eq(response.resultsSize, itemsSize);
        ck_assert(response.results[0].hasValue && response.results[itemsSize - 1].hasValue);
        if(earlyRejection) {
            ck_assert_uint_eq(response.results[1].status, UA_STATUSCODE_BADTOOMANYOPERATIONS);
            UA_AsyncResponse *ar = TAILQ_FIRST(&server->asyncManager.responses);
            ck_assert_ptr_ne(ar, NULL);
            ck_assert(ar->dc.callback);
            ck_assert_ptr_eq(ar->response.readResponse.results, NULL);
            ck_assert_uint_eq(ar->response.readResponse.resultsSize, 0);
            ck_assert_ptr_null(ar->session);
        }
    }
    ck_assert_uint_eq(server->asyncManager.activeDispatch, 0);
    unlockServer(server);
    UA_ReadResponse_clear(&response);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(earlyCancelCount, earlyRejection ? 1 : 0);
    if(earlyRejection)
        ck_assert_uint_eq(UA_Server_setAsyncReadResult(server, earlyRejectedOutput),
                          UA_STATUSCODE_GOOD);
    UA_Server_run_iterate(server, false);
    ck_assert_uint_eq(earlyCancelCount, earlyRejection ? 1 : 0);
    ck_assert_uint_eq(server->asyncManager.trackedOpsCount, 0);
    ck_assert_uint_eq(UA_Server_run_shutdown(server), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

static size_t nestedNotifications;

static void
nestedServiceNotification(UA_Server *server, UA_ApplicationNotificationType type,
                           const UA_KeyValueMap payload) {
    nestedNotifications++;
    UA_Boolean outer = (type == UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_BEGIN);
    if(outer)
        notifyService(server, UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_ASYNC,
                      8, UA_NODEID_NUMERIC(1, 2), 44, UA_NS0ID(WRITEREQUEST));
    else
        ck_assert(type == UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_ASYNC);

    /* Re-read the outer map after the nested call, not cached value pointers. */
    const UA_UInt32 *channelId = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        &payload, UA_QUALIFIEDNAME(0, "securechannel-id"), &UA_TYPES[UA_TYPES_UINT32]);
    const UA_UInt32 *requestId = (const UA_UInt32*)UA_KeyValueMap_getScalar(
        &payload, UA_QUALIFIEDNAME(0, "request-id"), &UA_TYPES[UA_TYPES_UINT32]);
    const UA_NodeId *sessionId = (const UA_NodeId*)UA_KeyValueMap_getScalar(
        &payload, UA_QUALIFIEDNAME(0, "session-id"), &UA_TYPES[UA_TYPES_NODEID]);
    const UA_NodeId *serviceId = (const UA_NodeId*)UA_KeyValueMap_getScalar(
        &payload, UA_QUALIFIEDNAME(0, "service-type"), &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_ptr_ne(channelId, NULL);
    ck_assert_ptr_ne(requestId, NULL);
    ck_assert_ptr_ne(sessionId, NULL);
    ck_assert_ptr_ne(serviceId, NULL);
    ck_assert_uint_eq(*channelId, outer ? 7 : 8);
    ck_assert_uint_eq(*requestId, outer ? 43 : 44);
    UA_NodeId expectedSession = UA_NODEID_NUMERIC(1, outer ? 1 : 2);
    UA_NodeId expectedService = outer ? UA_NS0ID(READREQUEST) : UA_NS0ID(WRITEREQUEST);
    ck_assert(UA_NodeId_equal(sessionId, &expectedSession));
    ck_assert(UA_NodeId_equal(serviceId, &expectedService));
}

START_TEST(service_notification_reentrancy) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    server->config.serviceNotificationCallback = nestedServiceNotification;
    nestedNotifications = 0;
    notifyService(server, UA_APPLICATIONNOTIFICATIONTYPE_SERVICE_BEGIN,
                  7, UA_NODEID_NUMERIC(1, 1), 43, UA_NS0ID(READREQUEST));
    ck_assert_uint_eq(nestedNotifications, 2);
    ck_assert_uint_eq(UA_Server_delete(server), UA_STATUSCODE_GOOD);
} END_TEST

int main(void) {
    Suite *s = suite_create("Async tutorial");
    TCase *tc = tcase_create("Ownership");
    tcase_add_loop_test(tc, tutorial_completion, 0, 5);
    tcase_add_loop_test(tc, shutdown_already_stopping, 0, 2);
    tcase_add_test(tc, session_close_releases_response);
    tcase_add_loop_test(tc, session_close_finishes_services, 0, 32);
    tcase_add_test(tc, response_callback_pool);
    tcase_add_test(tc, service_and_local_operation_pool);
    tcase_add_test(tc, dispatch_guards_and_pool_bound);
    tcase_add_test(tc, service_dispatch_notification);
    tcase_add_test(tc, service_notification_reentrancy);
    tcase_add_test(tc, monitored_item_before_startup);
    tcase_add_loop_test(tc, timeout_registration_failure, 0, 4);
    tcase_add_loop_test(tc, async_driver_lifecycle, 0, 5);
    tcase_add_test(tc, async_read_normalization);
    tcase_add_loop_test(tc, completion_before_response_registration, 0, 4);
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
