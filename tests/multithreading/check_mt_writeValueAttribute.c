/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "thread_wrapper.h"
#include "mt_testing.h"

#define NUMBER_OF_WORKERS 10
#define ITERATIONS_PER_WORKER 10
#define NUMBER_OF_CLIENTS 10
#define ITERATIONS_PER_CLIENT 10

UA_NodeId pumpTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};

static
void addVariableNode(void) {
    /* Add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","Temperature");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","Temperature");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "Temperature");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);

    UA_StatusCode res =
            UA_Server_addVariableNode(tc.server, pumpTypeId, parentNodeId,
                                      parentReferenceNodeId, myIntegerName,
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                      attr, NULL, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
}

static void setup(void) {
    tc.running = true;
    tc.server = UA_Server_newForUnitTest();
    ck_assert(tc.server != NULL);
    addVariableNode();
    UA_Server_run_startup(tc.server);
    THREAD_CREATE(server_thread, serverloop);
}

static void checkServer(void) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_StatusCode ret = UA_Server_readValue(tc.server, pumpTypeId, &var);
    ck_assert_int_eq(42, *(UA_Int32 *)var.data);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, ret);
    UA_Variant_clear(&var);
}

static
void server_writeValueAttribute(void *value) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 testValue = 42;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_INT32]);
    wValue.nodeId = pumpTypeId;
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(tc.server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}

static
void client_writeValueAttribute(void *value) {
    ThreadContext tmp = (*(ThreadContext *) value);
    UA_Variant val;
    UA_Int32 testValue = 42;
    UA_Variant_setScalar(&val, &testValue, &UA_TYPES[UA_TYPES_INT32]);
    UA_StatusCode retval = UA_Client_writeValueAttribute(tc.clients[tmp.index], pumpTypeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

static
void initTest(void) {
    tc.numberOfWorkers = NUMBER_OF_WORKERS;
    tc.numberofClients = NUMBER_OF_CLIENTS;
    tc.checkServerNodes = checkServer;
    tc.workerContext = (ThreadContext*) UA_calloc(tc.numberOfWorkers, sizeof(ThreadContext));
    tc.clients =  (UA_Client**) UA_calloc(tc.numberofClients, sizeof(UA_Client*));
    tc.clientContext = (ThreadContext*) UA_calloc(tc.numberofClients, sizeof(ThreadContext));
    for (size_t i = 0; i < tc.numberOfWorkers; i++) {
        tc.workerContext[i].index = i;
        tc.workerContext[i].upperBound = ITERATIONS_PER_WORKER;
        tc.workerContext[i].func = server_writeValueAttribute;
    }

    for (size_t i = 0; i < tc.numberofClients; i++) {
        tc.clientContext[i].index = i;
        tc.clientContext[i].upperBound = ITERATIONS_PER_CLIENT;
        tc.clientContext[i].func = client_writeValueAttribute;
    }
}

START_TEST(writeValueAttribute) {
        startMultithreading();
    }
END_TEST

static Suite* testSuite_immutableNodes(void) {
    Suite *s = suite_create("Multithreading");
    TCase *valueCallback = tcase_create("Write value attribute");
    initTest();
    tcase_add_checked_fixture(valueCallback, setup, teardown);
    tcase_add_test(valueCallback, writeValueAttribute);
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

