#include <stdio.h>
#include <stdlib.h>

#include "opcua.h"
#include "ua_namespace.h"
#include "check.h"

int zeroCnt = 0;
int visitCnt = 0;
void checkZeroVisitor(const UA_Node* node) {
	visitCnt++;
	if (node == UA_NULL) zeroCnt++;
}

START_TEST(test_Namespace) {
	Namespace *ns = UA_NULL;
	Namespace_new(&ns, 512, 0);
	Namespace_delete(ns);
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
	Namespace *ns;
	Namespace_new(&ns, 512, 0);
	UA_Node* n1; createNode(&n1,0,2253); Namespace_insert(ns,n1);
	const UA_Node* nr = UA_NULL;
	Namespace_Entry_Lock* nl = UA_NULL;
	UA_Int32 retval;
	// when
	retval = Namespace_get(ns,&(n1->nodeId),&nr,&nl);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_ptr_eq(nr,n1);
	// finally
	Namespace_delete(ns);
}
END_TEST

START_TEST(findNodeInNamespaceWithTwoEntries) {
	// given
	Namespace *ns;
	Namespace_new(&ns, 512, 0);
	UA_Node* n1; createNode(&n1,0,2253); Namespace_insert(ns,n1);
	UA_Node* n2; createNode(&n2,0,2255); Namespace_insert(ns,n2);

	const UA_Node* nr = UA_NULL;
	Namespace_Entry_Lock* nl = UA_NULL;
	UA_Int32 retval;
	// when
	retval = Namespace_get(ns,&(n2->nodeId),&nr,&nl);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_ptr_eq(nr,n2);
	// finally
	Namespace_delete(ns);
}
END_TEST

START_TEST(failToFindNodeInOtherNamespace) {
	// given
	Namespace *ns;
	Namespace_new(&ns, 512, 0);
	UA_Node* n1; createNode(&n1,0,2253); Namespace_insert(ns,n1);
	UA_Node* n2; createNode(&n2,0,2255); Namespace_insert(ns,n2);

	const UA_Node* nr = UA_NULL;
	Namespace_Entry_Lock* nl = UA_NULL;
	UA_Int32 retval;
	// when
	UA_Node* n; createNode(&n,1,2255);
	retval = Namespace_get(ns,&(n->nodeId),&nr,&nl);
	// then
	ck_assert_int_ne(retval, UA_SUCCESS);
	// finally
	UA_free(n);
	Namespace_delete(ns);
}
END_TEST

START_TEST(findNodeInNamespaceWithSeveralEntries) {
	// given
	Namespace *ns;
	Namespace_new(&ns, 512, 0);
	UA_Node* n1; createNode(&n1,0,2253); Namespace_insert(ns,n1);
	UA_Node* n2; createNode(&n2,0,2255); Namespace_insert(ns,n2);
	UA_Node* n3; createNode(&n3,0,2257); Namespace_insert(ns,n3);
	UA_Node* n4; createNode(&n4,0,2200); Namespace_insert(ns,n4);
	UA_Node* n5; createNode(&n5,0,1); Namespace_insert(ns,n5);
	UA_Node* n6; createNode(&n6,0,12); Namespace_insert(ns,n6);

	const UA_Node* nr = UA_NULL;
	Namespace_Entry_Lock* nl = UA_NULL;
	UA_Int32 retval;
	// when
	retval = Namespace_get(ns,&(n3->nodeId),&nr,&nl);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_ptr_eq(nr,n3);
	// finally
	Namespace_delete(ns);
}
END_TEST

START_TEST(iterateOverNamespaceShallNotVisitEmptyNodes) {
	// given
	Namespace *ns;
	Namespace_new(&ns, 512, 0);
	UA_Node* n1; createNode(&n1,0,2253); Namespace_insert(ns,n1);
	UA_Node* n2; createNode(&n2,0,2255); Namespace_insert(ns,n2);
	UA_Node* n3; createNode(&n3,0,2257); Namespace_insert(ns,n3);
	UA_Node* n4; createNode(&n4,0,2200); Namespace_insert(ns,n4);
	UA_Node* n5; createNode(&n5,0,1); Namespace_insert(ns,n5);
	UA_Node* n6; createNode(&n6,0,12); Namespace_insert(ns,n6);

	UA_Int32 retval;
	// when
	zeroCnt = 0;
	visitCnt = 0;
	retval = Namespace_iterate(ns,checkZeroVisitor);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_int_eq(zeroCnt, 0);
	ck_assert_int_eq(visitCnt, 6);
	// finally
	Namespace_delete(ns);
}
END_TEST

START_TEST(findNodeInExpandedNamespace) {
	// given
	Namespace *ns;
	Namespace_new(&ns, 10, 0);
	UA_Node* n;
	for (UA_Int32 i=0; i<200; i++) {
		createNode(&n,0,i); Namespace_insert(ns,n);
	}
	const UA_Node* nr = UA_NULL;
	Namespace_Entry_Lock* nl = UA_NULL;
	UA_Int32 retval;
	// when
	createNode(&n,0,25);
	retval = Namespace_get(ns,&(n->nodeId),&nr,&nl);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_int_eq(nr->nodeId.identifier.numeric,n->nodeId.identifier.numeric);
	// finally
	UA_free(n);
	Namespace_delete(ns);
}
END_TEST

START_TEST(iterateOverExpandedNamespaceShallNotVisitEmptyNodes) {
	// given
	Namespace *ns;
	Namespace_new(&ns, 10, 0);
	UA_Node* n;
	for (UA_Int32 i=0; i<200; i++) {
		createNode(&n,0,i); Namespace_insert(ns,n);
	}
	// when
	UA_Int32 retval;
	zeroCnt = 0;
	visitCnt = 0;
	retval = Namespace_iterate(ns,checkZeroVisitor);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_int_eq(zeroCnt, 0);
	ck_assert_int_eq(visitCnt, 200);
	// finally
	Namespace_delete(ns);
}
END_TEST

START_TEST(failToFindNonExistantNodeInNamespaceWithSeveralEntries) {
	// given
	Namespace *ns;
	Namespace_new(&ns, 512, 0);
	UA_Node* n1; createNode(&n1,0,2253); Namespace_insert(ns,n1);
	UA_Node* n2; createNode(&n2,0,2255); Namespace_insert(ns,n2);
	UA_Node* n3; createNode(&n3,0,2257); Namespace_insert(ns,n3);
	UA_Node* n4; createNode(&n4,0,2200); Namespace_insert(ns,n4);
	UA_Node* n5; createNode(&n5,0,1); Namespace_insert(ns,n5);
	UA_Node* n6; createNode(&n6,0,12);

	const UA_Node* nr = UA_NULL;
	Namespace_Entry_Lock* nl = UA_NULL;
	UA_Int32 retval;
	// when
	retval = Namespace_get(ns,&(n6->nodeId),&nr,&nl);
	// then
	ck_assert_int_ne(retval, UA_SUCCESS);
	// finally
	UA_free(n6);
	Namespace_delete(ns);
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
	tcase_add_test (tc_find, findNodeInNamespaceWithSeveralEntries);
	tcase_add_test (tc_find, findNodeInExpandedNamespace);
	tcase_add_test (tc_find, failToFindNonExistantNodeInNamespaceWithSeveralEntries);
	tcase_add_test (tc_find, failToFindNodeInOtherNamespace);
	tcase_add_test (tc_find, iterateOverNamespaceShallNotVisitEmptyNodes);
	tcase_add_test (tc_find, iterateOverExpandedNamespaceShallNotVisitEmptyNodes);
	suite_add_tcase (s, tc_find);


	return s;
}


int main (void) {
	int number_failed =0;
	Suite *s = namespace_suite ();
	SRunner *sr = srunner_create (s);
	// srunner_set_fork_status(sr,CK_NOFORK);
	srunner_run_all (sr, CK_NORMAL);
	number_failed += srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
