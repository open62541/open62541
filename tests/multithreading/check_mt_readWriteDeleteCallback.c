/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <check.h>
#include <stdlib.h>
#include <testing_clock.h>

#include "test_helpers.h"
#include "thread_wrapper.h"
#include "mt_testing.h"


#define NUMBER_OF_READ_WORKERS 10
#define NUMBER_OF_WRITE_WORKERS 10
#define ITERATIONS_PER_WORKER 100

#define NUMBER_OF_READ_CLIENTS 10
#define NUMBER_OF_WRITE_CLIENTS 10
#define ITERATIONS_PER_CLIENT 100

UA_NodeId pumpTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};
UA_Int32 temperature = 42;

UA_Lock mu;

static UA_StatusCode
readTemperature(UA_Server *tmpServer,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
    UA_LOCK(&mu);
    UA_Variant_setScalarCopy(&dataValue->value, &temperature, &UA_TYPES[UA_TYPES_INT32]);
    UA_UNLOCK(&mu);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeTemperature(UA_Server *tmpServer,
                 const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 const UA_NumericRange *range, const UA_DataValue *data) {
    UA_LOCK(&mu);
    temperature = *(UA_Int32 *) data->value.data;
    UA_UNLOCK(&mu);
    return UA_STATUSCODE_GOOD;
}

static
void AddVariableNode(void) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Temperature");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_DataSource temperatureSource;
    temperatureSource.read = readTemperature;
    temperatureSource.write = writeTemperature;
    UA_StatusCode retval = UA_Server_addDataSourceVariableNode(tc.server, pumpTypeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Temperature"),
                                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr,
                                                               temperatureSource, NULL, NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}

static void setup(void) {
    tc.running = true;
    tc.server = UA_Server_newForUnitTest();
    ck_assert(tc.server != NULL);
    AddVariableNode();
    UA_Server_run_startup(tc.server);
    THREAD_CREATE(server_thread, serverloop);
}

static
void server_deleteValue(void *value) {
    UA_StatusCode ret = UA_Server_deleteNode(tc.server, pumpTypeId, true);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, ret);
}

static
void server_readValue(void *value) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = pumpTypeId;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_Variant var;
    UA_Variant_init(&var);
    UA_StatusCode retval = UA_Server_readValue(tc.server, rvi.nodeId, &var);
    if (retval == UA_STATUSCODE_GOOD) {
        ck_assert_int_eq(42, *(UA_Int32 *)var.data);
        ck_assert_int_eq(UA_STATUSCODE_GOOD, retval);
        UA_Variant_clear(&var);
    }
    else {
        ck_assert_int_eq(retval, UA_STATUSCODE_BADNODEIDUNKNOWN);
    }
}

static
void server_writeValue(void *value) {
        UA_WriteValue wValue;
        UA_WriteValue_init(&wValue);
        UA_Int32 testValue = 42;
        UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_INT32]);
        wValue.nodeId = pumpTypeId;
        wValue.attributeId = UA_ATTRIBUTEID_VALUE;
        wValue.value.hasValue = true;
        UA_StatusCode retval = UA_Server_write(tc.server, &wValue);
        ck_assert(retval == UA_STATUSCODE_BADNODEIDUNKNOWN || retval == UA_STATUSCODE_GOOD);
}

static
void client_writeValue(void *value) {
    ThreadContext tmp = (*(ThreadContext *) value);
    UA_Variant val;
    UA_Int32 testValue = 42;
    UA_Variant_setScalar(&val, &testValue, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(tc.clients[tmp.index], pumpTypeId, &val);
    ck_assert(retval == UA_STATUSCODE_BADNODEIDUNKNOWN || retval == UA_STATUSCODE_GOOD);
}

static
void client_readValue(void *value) {
    ThreadContext tmp = (*(ThreadContext *) value);
    UA_Variant val;
    UA_NodeId nodeId = pumpTypeId;
    UA_StatusCode retval = UA_Client_readValueAttribute(tc.clients[tmp.index], nodeId, &val);
    if (retval == UA_STATUSCODE_GOOD) {
        ck_assert_int_eq(42, *(UA_Int32 *)val.data);
        UA_Variant_clear(&val);
    }
    else {
        ck_assert_int_eq(retval, UA_STATUSCODE_BADNODEIDUNKNOWN);
    }

}

static
void initTest(void) {
    UA_LOCK_INIT(&mu);

    size_t i = 0;
    for (; i < NUMBER_OF_READ_WORKERS; i++) {
        setThreadContext(&tc.workerContext[i], i, ITERATIONS_PER_WORKER, server_readValue);
    }
    for (; i < NUMBER_OF_READ_WORKERS + NUMBER_OF_WRITE_WORKERS; i++) {
        setThreadContext(&tc.workerContext[i], i, ITERATIONS_PER_WORKER, server_writeValue);
    }

    //Thread deleting the variable
    setThreadContext(&tc.workerContext[tc.numberOfWorkers - 1], i, 1, server_deleteValue);

    i = 0;
    for (; i < NUMBER_OF_READ_CLIENTS; i++) {
        setThreadContext(&tc.clientContext[i], i, ITERATIONS_PER_CLIENT, client_readValue);
    }
    for (; i < NUMBER_OF_READ_CLIENTS + NUMBER_OF_WRITE_CLIENTS; i++) {
        setThreadContext(&tc.clientContext[i], i, ITERATIONS_PER_CLIENT, client_writeValue);
    }
}

START_TEST(readWriteDeleteCallback) {
        startMultithreading();
    }
END_TEST

static Suite* testSuite_immutableNodes(void) {
    Suite *s = suite_create("Multithreading");
    TCase *valueCallback = tcase_create("Read-Write-Delete-Callback");
    tcase_add_checked_fixture(valueCallback, setup, teardown);
    tcase_add_test(valueCallback, readWriteDeleteCallback);
    suite_add_tcase(s,valueCallback);
    return s;
}

int main(void) {
    Suite *s = testSuite_immutableNodes();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);

    createThreadContext(NUMBER_OF_READ_WORKERS + NUMBER_OF_WRITE_WORKERS + 1,
                        NUMBER_OF_READ_CLIENTS + NUMBER_OF_WRITE_CLIENTS, NULL);
    initTest();
    srunner_run_all(sr, CK_NORMAL);
    deleteThreadContext();

    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

