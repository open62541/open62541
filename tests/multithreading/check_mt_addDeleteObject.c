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
#include <time.h>

#include "test_helpers.h"
#include "thread_wrapper.h"
#include "deviceObjectType.h"
#include "mt_testing.h"

#define NUMBER_OF_WORKERS 10
#define ITERATIONS_PER_WORKER 10
#define NUMBER_OF_CLIENTS 10
#define ITERATIONS_PER_CLIENT 10


static void setup(void) {
    tc.running = true;
    tc.server = UA_Server_newForUnitTest();
    ck_assert(tc.server != NULL);
    defineObjectTypes();
    addPumpTypeConstructor(tc.server);
    UA_Server_run_startup(tc.server);
    THREAD_CREATE(server_thread, serverloop);
}

static void checkServer(void) {
    for (size_t i = 0; i < NUMBER_OF_WORKERS * ITERATIONS_PER_WORKER; i++) {
        char string_buf[20];
        snprintf(string_buf, sizeof(string_buf), "Server %u", (unsigned)i);
        UA_NodeId reqNodeId = UA_NODEID_STRING(1, string_buf);
        UA_NodeId resNodeId;
        UA_StatusCode ret = UA_Server_readNodeId(tc.server, reqNodeId, &resNodeId);

        ck_assert_int_eq(ret, UA_STATUSCODE_BADNODEIDUNKNOWN);
    }
}

static
void server_addObject(void* value) {
    ThreadContext tmp = (*(ThreadContext *) value);
    size_t offset = tmp.index * tmp.upperBound;
    size_t number = offset + tmp.counter;
    char string_buf[20];
    snprintf(string_buf, sizeof(string_buf), "Server %u", (unsigned)number);
    UA_NodeId reqId = UA_NODEID_STRING(1, string_buf);
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", string_buf);

    //Entering the constructor of the object (callback to user space) it is possible that a thread deletes the object while initialization
    UA_StatusCode retval = UA_Server_addObjectNode(tc.server, reqId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, string_buf),
                            pumpTypeId,
                            oAttr, NULL, NULL);
    ck_assert(retval == UA_STATUSCODE_GOOD || retval ==  UA_STATUSCODE_BADNODEIDUNKNOWN);
}


static
void server_deleteObject(void* value){
    ThreadContext tmp = (*(ThreadContext *) value);
    size_t offset = tmp.index * tmp.upperBound;
    size_t number = offset + tmp.counter;
    char string_buf[20];
    snprintf(string_buf, sizeof(string_buf), "Server %u", (unsigned)number);
    
    /* In concurrent tests, nodes may not exist yet (being created by racing add operations)
     * or may get deleted during construction. Retry with sleep to allow time for add operations.
     * Both GOOD (successful delete) and BADNODEIDUNKNOWN (node deleted during construction
     * or never created due to failed add) are acceptable outcomes. */
    UA_StatusCode ret;
    for(size_t retries = 0; retries < 100; retries++) {
        ret = UA_Server_deleteNode(tc.server, UA_NODEID_STRING(1, string_buf), true);
        if(ret == UA_STATUSCODE_GOOD)
            break;
        /* Sleep 10ms between retries to give add operations time to complete.
         * This is especially important under valgrind where operations are slower. */
        if(ret == UA_STATUSCODE_BADNODEIDUNKNOWN) {
#ifndef _WIN32
            struct timespec ts = {0, 10000000}; /* 10ms */
            nanosleep(&ts, NULL);
#else
            Sleep(10);
#endif
        } else {
            /* Unexpected error, stop retrying */
            break;
        }
    }
    ck_assert(ret == UA_STATUSCODE_GOOD || ret == UA_STATUSCODE_BADNODEIDUNKNOWN);
}

static
void initTest(void) {
    for (size_t i = 0; i < tc.numberOfWorkers; i++) {
        setThreadContext(&tc.workerContext[i], i, ITERATIONS_PER_WORKER, server_addObject);
    }

    for (size_t i = 0; i < tc.numberofClients; i++) {
        setThreadContext(&tc.clientContext[i], i, ITERATIONS_PER_CLIENT, server_deleteObject);
    }
}

START_TEST(addDeleteObjectTypeNodes) {
        startMultithreading();
    }
END_TEST

static Suite* testSuite_immutableNodes(void) {
    Suite *s = suite_create("Multithreading");
    TCase *valueCallback = tcase_create("Add-Delete nodes");
    tcase_add_checked_fixture(valueCallback, setup, teardown);
    tcase_add_test(valueCallback, addDeleteObjectTypeNodes);
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
