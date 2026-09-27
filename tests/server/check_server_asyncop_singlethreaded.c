/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* Local async operations without a server thread. check_server_asyncop.c
 * drives the server from a second thread and needs UA_MULTITHREADING >= 100. */

#include <open62541/server_config_default.h>
#include <open62541/server.h>

#include "test_helpers.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

static UA_Server *server = NULL;

/* The DataValue the value source was handed, kept so the test can complete the
 * operation afterwards. */
static UA_DataValue *pendingRead = NULL;

/* Set by the result callback. */
static UA_Boolean resultCalled = false;
static UA_Int32 resultValue = 0;

/* AddNodes reads the value source itself, so answer synchronously until the
 * node exists */
static UA_Boolean answerAsync = false;

static UA_StatusCode
readAsyncSource(UA_Server *s, const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean includeSourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *value) {
    if(!answerAsync) {
        UA_Int32 zero = 0;
        UA_StatusCode res =
            UA_Variant_setScalarCopy(&value->value, &zero,
                                     &UA_TYPES[UA_TYPES_INT32]);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        value->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    pendingRead = value;
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static void
readResult(UA_Server *s, void *context, const UA_DataValue *result) {
    resultCalled = true;
    if(result->hasValue && result->value.type == &UA_TYPES[UA_TYPES_INT32])
        resultValue = *(UA_Int32*)result->value.data;
}

static void setup(void) {
    pendingRead = NULL;
    resultCalled = false;
    resultValue = 0;
    answerAsync = false;

    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);

    UA_CallbackValueSource evs;
    memset(&evs, 0, sizeof(UA_CallbackValueSource));
    evs.read = readAsyncSource;

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "async-source");

    UA_StatusCode res = UA_Server_addCallbackValueSourceVariableNode(
        server, UA_NODEID_STRING(1, "async-source"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "async-source"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, evs, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* processReadyLater() does nothing while the server is stopped */
    ck_assert_uint_eq(UA_Server_run_startup(server), UA_STATUSCODE_GOOD);

    /* From here on the source is asynchronous. */
    answerAsync = true;
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

/* TAILQ_EMPTY cannot tell a zeroed head from an initialised one, but
 * TAILQ_INSERT_TAIL dereferences tqh_last */
START_TEST(AsyncOp_managerInitialisedWithoutMultithreading) {
    UA_AsyncManager *am = &server->asyncManager;
    ck_assert_ptr_ne(am->waitingOps.tqh_last, NULL);
    ck_assert_ptr_ne(am->readyOps.tqh_last, NULL);
    ck_assert_ptr_ne(am->waitingResponses.tqh_last, NULL);
    ck_assert_ptr_ne(am->readyResponses.tqh_last, NULL);
} END_TEST

/* Only a value source answering GoodCompletesAsynchronously enqueues */
START_TEST(AsyncOp_readAsyncCompletesWithoutMultithreading) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "async-source");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_StatusCode res =
        UA_Server_read_async(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER,
                             readResult, NULL, 0);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Asynchronous, so nothing has been reported yet, and the operation is
     * accounted for. */
    ck_assert_uint_eq(resultCalled, false);
    ck_assert_uint_eq(server->asyncManager.opsCount, 1);

    /* Complete it from outside, the way an application would. */
    UA_Int32 answer = 42;
    ck_assert_ptr_ne(pendingRead, NULL);
    UA_Variant_setScalarCopy(&pendingRead->value, &answer,
                             &UA_TYPES[UA_TYPES_INT32]);
    pendingRead->hasValue = true;
    pendingRead->status = UA_STATUSCODE_GOOD;

    res = UA_Server_setAsyncReadResult(server, pendingRead);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* The result callback runs in the next EventLoop iteration. */
    UA_Server_run_iterate(server, false);

    ck_assert_uint_eq(resultCalled, true);
    ck_assert_int_eq(resultValue, 42);
    ck_assert_uint_eq(server->asyncManager.opsCount, 0);
} END_TEST

static Suite* testSuite_AsyncOpSingleThreaded(void) {
    Suite *s = suite_create("Local async operations, single-threaded");
    TCase *tc = tcase_create("async without multithreading");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, AsyncOp_managerInitialisedWithoutMultithreading);
    tcase_add_test(tc, AsyncOp_readAsyncCompletesWithoutMultithreading);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    SRunner *sr = srunner_create(testSuite_AsyncOpSingleThreaded());
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
