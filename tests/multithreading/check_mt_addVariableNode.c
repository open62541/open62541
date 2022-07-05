/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <check.h>
#include "thread_wrapper.h"
#include "mt_testing.h"

#define NUMBER_OF_WORKERS 10
#define ITERATIONS_PER_WORKER 10
#define NUMBER_OF_CLIENTS 10
#define ITERATIONS_PER_CLIENT 10

static void setup(void) {
    tc.running = true;
    tc.server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(tc.server));
    UA_Server_run_startup(tc.server);
    THREAD_CREATE(server_thread, serverloop);
}

static
void checkServer(void) {
    for (size_t i = 0; i < NUMBER_OF_WORKERS * ITERATIONS_PER_WORKER; i++) {
        UA_ReadValueId rvi;
        UA_ReadValueId_init(&rvi);
        char string_buf[20];
        snprintf(string_buf, sizeof(string_buf), "Server %u", (unsigned)i);
        rvi.nodeId = UA_NODEID_STRING(1, string_buf);
        rvi.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataValue resp = UA_Server_read(tc.server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

        ck_assert_int_eq(resp.status, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(0, resp.value.arrayLength);
        ck_assert(&UA_TYPES[UA_TYPES_INT32] == resp.value.type);
        ck_assert_int_eq(42, *(UA_Int32* )resp.value.data);
        UA_DataValue_clear(&resp);
    }

    for (size_t i = 0; i < NUMBER_OF_CLIENTS * ITERATIONS_PER_CLIENT; i++) {
        UA_ReadValueId rvi;
        UA_ReadValueId_init(&rvi);
        char string_buf[20];
        snprintf(string_buf, sizeof(string_buf), "Client %u", (unsigned)i);
        rvi.nodeId = UA_NODEID_STRING(1, string_buf);
        rvi.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataValue resp = UA_Server_read(tc.server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

        ck_assert_int_eq(resp.status, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(2, resp.value.arrayLength);
        ck_assert(&UA_TYPES[UA_TYPES_INT32] == resp.value.type);
        ck_assert_int_eq(10, *(UA_Int32 *)resp.value.data);
        ck_assert_int_eq(20, *((UA_Int32 *)resp.value.data + 1));
        UA_DataValue_clear(&resp);
    }
}

static
void server_addVariable(void* value) {
        ThreadContext tmp = (*(ThreadContext *) value);
        size_t offset = tmp.index * tmp.upperBound;
        size_t number = offset + tmp.counter;
        char string_buf[20];
        snprintf(string_buf, sizeof(string_buf), "Server %u", (unsigned)number);
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        UA_Int32 myInteger = 42;
        UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
        attr.description = UA_LOCALIZEDTEXT("en-US",string_buf);
        attr.displayName = UA_LOCALIZEDTEXT("en-US",string_buf);
        attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

        UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, string_buf);
        UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, string_buf);
        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        UA_StatusCode ret = UA_Server_addVariableNode(tc.server, myIntegerNodeId, parentNodeId,
                                                      parentReferenceNodeId, myIntegerName,
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
        ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
}

static
void client_addVariable(void* value){
    ThreadContext tmp = (*(ThreadContext *) value);
    size_t offset = tmp.index * tmp.upperBound;
    size_t number = offset + tmp.counter;
    char string_buf[20];
    snprintf(string_buf, sizeof(string_buf), "Client %u", (unsigned)number);
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", string_buf);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", string_buf);
    UA_NodeId nodeId = UA_NODEID_STRING(1, string_buf);
    UA_QualifiedName integerName = UA_QUALIFIEDNAME(1, string_buf);
    UA_Int32 values[2] = {10, 20};
    UA_Variant_setArray(&attr.value, values, 2, &UA_TYPES[UA_TYPES_INT32]);
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 arrayDims[1] = {2};
    attr.arrayDimensions = arrayDims;
    attr.arrayDimensionsSize = 1;
    UA_StatusCode retval = UA_Client_addVariableNode(tc.clients[tmp.index], nodeId,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                               integerName,
                                               UA_NODEID_NULL, attr, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}

static
void initTest(void) {
    initThreadContext(NUMBER_OF_WORKERS, NUMBER_OF_CLIENTS, checkServer);

    for (size_t i = 0; i < tc.numberOfWorkers; i++) {
        setThreadContext(&tc.workerContext[i], i, ITERATIONS_PER_WORKER, server_addVariable);
    }

    for (size_t i = 0; i < tc.numberofClients; i++) {
        setThreadContext(&tc.clientContext[i], i, ITERATIONS_PER_CLIENT, client_addVariable);
    }
}

START_TEST(addVariableNodes) {
        startMultithreading();
    }
END_TEST

static Suite* testSuite_immutableNodes(void) {
    Suite *s = suite_create("Multithreading");
    TCase *valueCallback = tcase_create("Add variable nodes");
    initTest();
    tcase_add_checked_fixture(valueCallback, setup, teardown);
    tcase_add_test(valueCallback, addVariableNodes);
    suite_add_tcase(s,valueCallback);
    return s;
}

int main(void) {
    Suite *s = testSuite_immutableNodes();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
