#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"
#include "server/ua_nodestore.h"
#include "server/ua_services.h"
#include "ua_client.h"
#include "ua_nodeids.h"
#include "ua_statuscodes.h"
#include "ua_types.h"
#include "ua_util.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_MULTITHREADING
#include <pthread.h>
#include <urcu.h>
#endif

START_TEST(AddVariableNode) {
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);

    /* add a variable node to the address space */
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
                                                  myIntegerName, UA_NODEID_NULL, attr, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
    UA_Server_delete(server);
} END_TEST

START_TEST(AddNodeTwiceGivesError) {
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);

    /* add a variable node to the address space */
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
                                                  myIntegerName, UA_NODEID_NULL, attr, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
    res = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
                                    myIntegerName, UA_NODEID_NULL, attr, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADNODEIDEXISTS);
    UA_Server_delete(server);
} END_TEST

static Suite * testSuite_services_nodemanagement(void) {
	Suite *s = suite_create("services_nodemanagement");

	TCase *tc_addnodes = tcase_create("addnodes");
	tcase_add_test(tc_addnodes, AddVariableNode);
	tcase_add_test(tc_addnodes, AddNodeTwiceGivesError);

	suite_add_tcase(s, tc_addnodes);
	return s;
}

int main(void) {
	int number_failed = 0;
	Suite *s;
	s = testSuite_services_nodemanagement();
	SRunner *sr = srunner_create(s);
	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
