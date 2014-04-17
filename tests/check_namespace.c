/*
 ============================================================================
 Name        : check_stack.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "opcua.h"
#include "ua_namespace.h"
#include "check.h"



START_TEST(test_Namespace) {
	namespace *ns = UA_NULL;
	create_ns(&ns, 512);
	delete_ns(ns);
}
END_TEST

UA_Int32 createNode(UA_Node** p, UA_Int16 nsid, UA_Int32 id) {
	UA_Node_new(p);
	(*p)->nodeId.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	(*p)->nodeId.namespace = nsid;
	(*p)->nodeId.identifier.numeric = id;
	return UA_SUCCESS;
}

START_TEST(findNodeInNamespaceWithSingleEntry) {
	// given
	namespace *ns;
	create_ns(&ns, 512);
	UA_Node* n1; createNode(&n1,0,2253); insert_node(ns,n1);
	UA_Node* nr = UA_NULL;
	ns_lock* nl = UA_NULL;
	UA_Int32 retval;
	// when
	retval = get_node(ns,&(n1->nodeId),&nr,&nl);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_ptr_eq(nr,&n1);
	// finally
	delete_ns(ns);
}
END_TEST

START_TEST(findNodeInNamespaceWithTwoEntries) {
	// given
	namespace *ns;
	create_ns(&ns, 512);
	UA_Node* n1; createNode(&n1,0,2253); insert_node(ns,n1);
	UA_Node* n2; createNode(&n2,0,2255); insert_node(ns,n2);

	UA_Node* nr = UA_NULL;
	ns_lock* nl = UA_NULL;
	UA_Int32 retval;
	// when
	retval = get_node(ns,&(n2->nodeId),&nr,&nl);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_ptr_eq(nr,&n1);
	// finally
	delete_ns(ns);
}
END_TEST

Suite * namespace_suite (void) {
	Suite *s = suite_create ("Namespace");

	TCase *tc_cd = tcase_create ("Create/Delete");
	tcase_add_test (tc_cd, test_Namespace);
	suite_add_tcase (s, tc_cd);

	TCase* tc_find = tcase_create ("Find");
	tcase_add_test (tc_find, findNodeInNamespaceWithSingleEntry);
	tcase_add_test (tc_find, findNodeInNamespaceWithTwoEntries);
	suite_add_tcase (s, tc_find);

	return s;
}

int main (void) {
	int number_failed =0;
	Suite *s = namespace_suite ();
	SRunner *sr = srunner_create (s);
	srunner_set_fork_status(sr,CK_NOFORK);
	srunner_run_all (sr, CK_NORMAL);
	number_failed += srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
