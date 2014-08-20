#include "open62541.h"
#include "open62541-server.h"
#include "check.h"

START_TEST(addingVariableNodeTwiceShallRedirectPointer) {
	//GIVEN
	UA_Int32 myInteger = 0, otherInteger = 0;
	UA_NodeId myIntegerNode = {1, UA_NODEIDTYPE_NUMERIC, 50};

	//WHEN
	UA_Application_addVariableNode(application, &myIntegerNode, UA_INT32, &myInteger);
	UA_Application_addVariableNode(application, &myIntegerNode, UA_INT32, &otherInteger);

	//THEN
	ck_assert_ptr_eq(otherInteger,UA_Application_getVariableNodeAppPtr(application, &myIntegerNode);
}
END_TEST

START_TEST(addingDifferentVariableNodesWithSameMemoryShallResultInTwoViewsOnOneMemoryLocation) {
	//GIVEN
	UA_Int32 myInteger = 0;
	UA_NodeId myIntegerNode = {1, UA_NODEIDTYPE_NUMERIC, 50};
	UA_NodeId otherIntegerNode = {1, UA_NODEIDTYPE_NUMERIC, 51};

	//WHEN
	UA_Application_addVariableNode(application, &myIntegerNode, UA_INT32, &myInteger);
	UA_Application_addVariableNode(application, &otherIntegerNode, UA_INT32, &myInteger);

	//THEN
	ck_assert_ptr_eq(myInteger,UA_Application_getVariableNodeAppPtr(application, &myIntegerNode);
	ck_assert_ptr_eq(myInteger,UA_Application_getVariableNodeAppPtr(application, &otherIntegerNode);
}
END_TEST

Suite *testSuite_builtin(void) {
	Suite *s = suite_create("Test api");

	TCase *tc_ns0 = tcase_create("populating namespaces");
	tcase_add_test(tc_ns0, addingVariableNodeTwiceShallRedirectPointer);
	tcase_add_test(tc_ns0, addingDifferentVariableNodesWithSameMemoryShallResultInTwoViewsOnOneMemoryLocation);
	suite_add_tcase(s, tc_ns0);

	return s;
}


int main(void) {
	int      number_failed = 0;
	Suite   *s;
	SRunner *sr;

	s  = testSuite_builtin();
	sr = srunner_create(s);
	//srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
