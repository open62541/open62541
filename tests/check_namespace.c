#include <stdio.h>
#include <stdlib.h>

#include "ua_types.h"
#include "ua_namespace.h"
#include "check.h"

#ifdef MULTITHREADING
#include <urcu.h>
#endif

int zeroCnt = 0;
int visitCnt = 0;
void checkZeroVisitor(const UA_Node* node) {
	visitCnt++;
	if (node == UA_NULL) zeroCnt++;
}

void printVisitor(const UA_Node* node) {
	printf("%d\n", node->nodeId.identifier.numeric);
}

START_TEST(test_Namespace) {
	Namespace *ns = UA_NULL;
	Namespace_new(&ns, 0);
	Namespace_delete(ns);
}
END_TEST

UA_Int32 createNode(const UA_Node** p, UA_Int16 nsid, UA_Int32 id) {
	UA_VariableNode * p2;
	UA_VariableNode_new(&p2);
	p2->nodeId.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	p2->nodeId.namespace = nsid;
	p2->nodeId.identifier.numeric = id;
	p2->nodeClass = UA_NODECLASS_VARIABLE;
	*p = (const UA_Node *)p2;
	return UA_SUCCESS;
}

START_TEST(findNodeInNamespaceWithSingleEntry) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	Namespace *ns;
	Namespace_new(&ns, 0);
	const UA_Node* n1; createNode(&n1,0,2253);
	Namespace_insert(ns, &n1, NAMESPACE_INSERT_UNIQUE | NAMESPACE_INSERT_GETMANAGED);
	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	retval = Namespace_get(ns,&(n1->nodeId),&nr);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_ptr_eq(nr,n1);
	// finally
	Namespace_releaseManagedNode(n1);
	Namespace_releaseManagedNode(nr);
	Namespace_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(failToFindNodeInOtherNamespace) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	Namespace *ns = UA_NULL;
	Namespace_new(&ns, 0);

	const UA_Node* n1 = UA_NULL; createNode(&n1,0,2253); Namespace_insert(ns, &n1, 0);
	const UA_Node* n2 = UA_NULL; createNode(&n1,0,2253); Namespace_insert(ns, &n2, 0);

	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	const UA_Node* n; createNode(&n,1,2255);
	retval = Namespace_get(ns,&(n->nodeId), &nr);
	// then
	ck_assert_int_ne(retval, UA_SUCCESS);
	// finally
	UA_free((void *)n);
	Namespace_releaseManagedNode(nr);
	Namespace_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(findNodeInNamespaceWithSeveralEntries) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	Namespace *ns;
	Namespace_new(&ns, 0);
	const UA_Node* n1; createNode(&n1,0,2253); Namespace_insert(ns, &n1, 0);
	const UA_Node* n2; createNode(&n2,0,2255); Namespace_insert(ns, &n2, 0);
	const UA_Node* n3; createNode(&n3,0,2257); Namespace_insert(ns, &n3, NAMESPACE_INSERT_GETMANAGED);
	const UA_Node* n4; createNode(&n4,0,2200); Namespace_insert(ns, &n4, 0);
	const UA_Node* n5; createNode(&n5,0,1); Namespace_insert(ns, &n5, 0);
	const UA_Node* n6; createNode(&n6,0,12); Namespace_insert(ns, &n6, 0);

	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	retval = Namespace_get(ns,&(n3->nodeId),&nr);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_ptr_eq(nr,n3);
	// finally
	Namespace_releaseManagedNode(n3);
	Namespace_releaseManagedNode(nr);
	Namespace_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(iterateOverNamespaceShallNotVisitEmptyNodes) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	Namespace *ns;
	Namespace_new(&ns, 0);
	const UA_Node* n1; createNode(&n1,0,2253); Namespace_insert(ns, &n1, 0);
	const UA_Node* n2; createNode(&n2,0,2255); Namespace_insert(ns, &n2, 0);
	const UA_Node* n3; createNode(&n3,0,2257); Namespace_insert(ns, &n3, 0);
	const UA_Node* n4; createNode(&n4,0,2200); Namespace_insert(ns, &n4, 0);
	const UA_Node* n5; createNode(&n5,0,1); Namespace_insert(ns, &n5, 0);
	const UA_Node* n6; createNode(&n6,0,12); Namespace_insert(ns, &n6, 0);

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
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(findNodeInExpandedNamespace) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	Namespace *ns;
	Namespace_new(&ns, 0);
	const UA_Node* n;
	UA_Int32 i=0;
	for (; i<200; i++) {
		createNode(&n,0,i); Namespace_insert(ns, &n, 0);
	}
	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	createNode(&n,0,25);
	retval = Namespace_get(ns,&(n->nodeId),&nr);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_int_eq(nr->nodeId.identifier.numeric,n->nodeId.identifier.numeric);
	// finally
	UA_free((void*)n);
	Namespace_releaseManagedNode(nr);
	Namespace_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(iterateOverExpandedNamespaceShallNotVisitEmptyNodes) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	Namespace *ns;
	Namespace_new(&ns, 0);
	const UA_Node* n;
	UA_Int32 i=0;
	for (; i<200; i++) {
		createNode(&n,0,i); Namespace_insert(ns, &n, 0);
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
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(failToFindNonExistantNodeInNamespaceWithSeveralEntries) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	Namespace *ns;
	Namespace_new(&ns, 0);
	const UA_Node* n1; createNode(&n1,0,2253); Namespace_insert(ns, &n1, 0);
	const UA_Node* n2; createNode(&n2,0,2255); Namespace_insert(ns, &n2, 0);
	const UA_Node* n3; createNode(&n3,0,2257); Namespace_insert(ns, &n3, 0);
	const UA_Node* n4; createNode(&n4,0,2200); Namespace_insert(ns, &n4, 0);
	const UA_Node* n5; createNode(&n5,0,1); Namespace_insert(ns, &n5, 0);
	const UA_Node* n6; createNode(&n6,0,12); 

	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	retval = Namespace_get(ns, &(n6->nodeId), &nr);
	// then
	ck_assert_int_ne(retval, UA_SUCCESS);
	// finally
	UA_free((void *)n6);
	Namespace_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

Suite * namespace_suite (void) {
	Suite *s = suite_create ("Namespace");

	TCase *tc_cd = tcase_create ("Create/Delete");
	tcase_add_test (tc_cd, test_Namespace);
	suite_add_tcase (s, tc_cd);

	TCase* tc_find = tcase_create ("Find");
	tcase_add_test (tc_find, findNodeInNamespaceWithSingleEntry);
	tcase_add_test (tc_find, findNodeInNamespaceWithSeveralEntries);
	tcase_add_test (tc_find, findNodeInExpandedNamespace);
	tcase_add_test (tc_find, failToFindNonExistantNodeInNamespaceWithSeveralEntries);
	tcase_add_test (tc_find, failToFindNodeInOtherNamespace);
	suite_add_tcase (s, tc_find);

	TCase* tc_iterate = tcase_create ("Iterate");
	tcase_add_test (tc_iterate, iterateOverNamespaceShallNotVisitEmptyNodes);
	tcase_add_test (tc_iterate, iterateOverExpandedNamespaceShallNotVisitEmptyNodes);
	suite_add_tcase (s, tc_iterate);

	return s;
}


int main (void) {
	int number_failed =0;
	Suite *s = namespace_suite ();
	SRunner *sr = srunner_create (s);
	//srunner_set_fork_status(sr,CK_NOFORK);
	srunner_run_all (sr, CK_NORMAL);
	number_failed += srunner_ntests_failed (sr);
	srunner_free (sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
