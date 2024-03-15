/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

static UA_Server *server;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(addVariable) {
    /* add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);

    clock_t begin, finish;
    begin = clock();
    for(int i = 0; i < 3000; i++) {
        UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                                  parentReferenceNodeId, myIntegerName,
                                  UA_NODEID_NULL, attr, NULL, NULL);

        if(i % 1000 == 0) {
            finish = clock();
            double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
            printf("%i nodes:\t Duration was %f s\n", i, time_spent);
            begin = clock();
        }
    }
}
END_TEST

static Suite * service_speed_suite (void) {
    Suite *s = suite_create ("Service Speed");

    TCase* tc_addnodes = tcase_create ("AddNodes");
    tcase_add_checked_fixture(tc_addnodes, setup, teardown);
    tcase_add_test(tc_addnodes, addVariable);
    suite_add_tcase(s, tc_addnodes);

    return s;
}

int main (void) {
    int number_failed = 0;
    Suite *s = service_speed_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr,CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed (sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
