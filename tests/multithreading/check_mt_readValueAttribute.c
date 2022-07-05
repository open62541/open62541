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

UA_NodeId pumpTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};

static
void addVariableNode(void) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","Temperature");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","Temperature");
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
    tc.server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(tc.server));
    addVariableNode();
    UA_Server_run_startup(tc.server);
    THREAD_CREATE(server_thread, serverloop);
}

static
void server_readValueAttribute(void * value) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = pumpTypeId;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    // read 1
    UA_DataValue resp = UA_Server_read(tc.server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    ck_assert_int_eq(true, resp.hasValue);
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_INT32] == resp.value.type);
    ck_assert_int_eq(42, *(UA_Int32* )resp.value.data);
    UA_DataValue_clear(&resp);

    // read 2
    UA_Variant var;
    UA_Variant_init(&var);
    UA_StatusCode ret = UA_Server_readValue(tc.server, rvi.nodeId, &var);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, ret);
    ck_assert_int_eq(42, *(UA_Int32 *)var.data);

    UA_Variant_clear(&var);
}


static
void client_readValueAttribute(void * value) {
    ThreadContext tmp = (*(ThreadContext *) value);
    UA_Variant val;
    UA_NodeId nodeId = pumpTypeId;
    UA_StatusCode retval = UA_Client_readValueAttribute(tc.clients[tmp.index], nodeId, &val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(42, *(UA_Int32 *)val.data);
    UA_Variant_clear(&val);
}

static
void initTest(void) {
    initThreadContext(NUMBER_OF_WORKERS, NUMBER_OF_CLIENTS, NULL);

    for (size_t i = 0; i < tc.numberOfWorkers; i++) {
        setThreadContext(&tc.workerContext[i], i, ITERATIONS_PER_WORKER, server_readValueAttribute);
    }

    for (size_t i = 0; i < tc.numberofClients; i++) {
        setThreadContext(&tc.clientContext[i], i, ITERATIONS_PER_CLIENT, client_readValueAttribute);
    }
}

START_TEST(readValueAttribute) {
        startMultithreading();
    }
END_TEST

static Suite* testSuite_immutableNodes(void) {
    Suite *s = suite_create("Multithreading");
    TCase *valueCallback = tcase_create("Read Write attribute");
    initTest();
    tcase_add_checked_fixture(valueCallback, setup, teardown);
    tcase_add_test(valueCallback, readValueAttribute);
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
