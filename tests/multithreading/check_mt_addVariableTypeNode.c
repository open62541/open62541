/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <check.h>
#include <stdlib.h>
#include <stdio.h>

#include "thread_wrapper.h"
#include "test_helpers.h"
#include "mt_testing.h"

#define NUMBER_OF_WORKERS 10
#define ITERATIONS_PER_WORKER 10
#define NUMBER_OF_CLIENTS 10
#define ITERATIONS_PER_CLIENT 10

static void setup(void) {
    tc.running = true;
    tc.server = UA_Server_newForUnitTest();
    ck_assert(tc.server != NULL);
    UA_Server_run_startup(tc.server);
    THREAD_CREATE(server_thread, serverloop);
}

static void checkServer(void) {
    for (size_t i = 0; i <  NUMBER_OF_WORKERS * ITERATIONS_PER_WORKER; i++) {
        char string_buf[20];
        snprintf(string_buf, sizeof(string_buf), "Server %u", (unsigned)i);
        UA_NodeId reqNodeId = UA_NODEID_STRING(1, string_buf);
        UA_NodeId resNodeId;
        UA_StatusCode ret = UA_Server_readNodeId(tc.server, reqNodeId, &resNodeId);

        ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
        ck_assert(UA_NodeId_equal(&reqNodeId, &resNodeId) == UA_TRUE);
        UA_NodeId_clear(&resNodeId);
    }

    for (size_t i = 0; i < NUMBER_OF_CLIENTS * ITERATIONS_PER_CLIENT; i++) {
        char string_buf[20];
        snprintf(string_buf, sizeof(string_buf), "Client %u", (unsigned)i);
        UA_NodeId reqNodeId = UA_NODEID_STRING(1, string_buf);
        UA_NodeId resNodeId;
        UA_StatusCode ret = UA_Server_readNodeId(tc.server, reqNodeId, &resNodeId);

        ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
        ck_assert(UA_NodeId_equal(&reqNodeId, &resNodeId) == UA_TRUE);
        UA_NodeId_clear(&resNodeId);
    }
}

static
void server_addVariableType(void* value) {
    ThreadContext tmp = (*(ThreadContext *) value);
    size_t offset = tmp.index * tmp.upperBound;
    size_t number = offset + tmp.counter;
    char string_buf[20];
    snprintf(string_buf, sizeof(string_buf), "Server %u", (unsigned)number);
    UA_NodeId myNodeId = UA_NODEID_STRING(1, string_buf);
    UA_VariableTypeAttributes vtAttr = UA_VariableTypeAttributes_default;
    vtAttr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    vtAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 arrayDims[1] = {2};
    vtAttr.arrayDimensions = arrayDims;
    vtAttr.arrayDimensionsSize = 1;
    vtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "2DPoint Type");

    /* a matching default value is required */
    UA_Double zero[2] = {0.0, 0.0};
    UA_Variant_setArray(&vtAttr.value, zero, 2, &UA_TYPES[UA_TYPES_DOUBLE]);

    UA_StatusCode res =
            UA_Server_addVariableTypeNode(tc.server, myNodeId,
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                          UA_QUALIFIEDNAME(1, "2DPoint Type"), UA_NODEID_NULL,
                                          vtAttr, NULL, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);

}

static
void client_addVariableType(void* value){
    ThreadContext tmp = (*(ThreadContext *) value);
    size_t offset = tmp.index * tmp.upperBound;
    size_t number = offset + tmp.counter;
    char string_buf[20];
    snprintf(string_buf, sizeof(string_buf), "Client %u", (unsigned)number);
    UA_NodeId myNodeId = UA_NODEID_STRING(1, string_buf);

    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 arrayDims[1] = {2};
    attr.arrayDimensions = arrayDims;
    attr.arrayDimensionsSize = 1;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "PointType");

    /* a matching default value is required */
    UA_Double zero[2] = {0.0, 0.0};
    UA_Variant_setArray(&attr.value, zero, 2, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode  retval = UA_Client_addVariableTypeNode(tc.clients[tmp.index], myNodeId,
                                                          UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),
                                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                          UA_QUALIFIEDNAME(1, "PointType"), attr, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}

static
void initTest(void) {
    for (size_t i = 0; i < tc.numberOfWorkers; i++) {
        setThreadContext(&tc.workerContext[i], i, ITERATIONS_PER_WORKER, server_addVariableType);
    }

    for (size_t i = 0; i < tc.numberofClients; i++) {
        setThreadContext(&tc.clientContext[i], i, ITERATIONS_PER_CLIENT, client_addVariableType);
    }
}

START_TEST(addVariableTypeNodes) {
        startMultithreading();
    }
END_TEST

static Suite* testSuite_immutableNodes(void) {
    Suite *s = suite_create("Multithreading");
    TCase *valueCallback = tcase_create("Add variable type nodes");
    tcase_add_checked_fixture(valueCallback, setup, teardown);
    tcase_add_test(valueCallback, addVariableTypeNodes);
    suite_add_tcase(s,valueCallback);
    return s;
}

int main(void) {
    Suite *s = testSuite_immutableNodes();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);

    createThreadContext(NUMBER_OF_WORKERS, NUMBER_OF_CLIENTS, checkServer);
    initTest();
    srunner_run_all(sr, CK_NORMAL);
    deleteThreadContext();

    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
