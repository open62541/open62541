#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ua_types.h"
#include "server/nodestore/open62541_nodestore.h"
#include "ua_util.h"
#include "check.h"

#ifdef MULTITHREADING
#include <pthread.h>
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

START_TEST(test_open62541NodeStore) {
	open62541NodeStore *ns = UA_NULL;
	open62541NodeStore_new(&ns);
	open62541NodeStore_delete(ns);
}
END_TEST

UA_StatusCode createNode(UA_Node** p, UA_Int16 nsid, UA_Int32 id) {
	*p = (UA_Node *)UA_VariableNode_new();
	(*p)->nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
	(*p)->nodeId.namespaceIndex = nsid;
	(*p)->nodeId.identifier.numeric = id;
	(*p)->nodeClass = UA_NODECLASS_VARIABLE;
	return UA_STATUSCODE_GOOD;
}

START_TEST(findNodeInopen62541NodeStoreWithSingleEntry) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	open62541NodeStore *ns;
	open62541NodeStore_new(&ns);
	UA_Node* n1; createNode(&n1,0,2253);
	open62541NodeStore_insert(ns, &n1, UA_NODESTORE_INSERT_UNIQUE | UA_NODESTORE_INSERT_GETMANAGED);
	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	retval = open62541NodeStore_get(ns,&n1->nodeId,&nr);
	// then
	ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
	ck_assert_ptr_eq((void*)nr, (void*)n1);
	// finally
	open62541NodeStore_release(n1);
	open62541NodeStore_release(nr);
	open62541NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(failToFindNodeInOtheropen62541NodeStore) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	open62541NodeStore *ns = UA_NULL;
	open62541NodeStore_new(&ns);

	UA_Node* n1; createNode(&n1,0,2253); open62541NodeStore_insert(ns, &n1, 0);
	UA_Node* n2; createNode(&n2,0,2253); open62541NodeStore_insert(ns, &n2, 0);

	const UA_Node* nr = UA_NULL;
	// when
	UA_Node* n; createNode(&n,1,2255);
	UA_Int32 retval = open62541NodeStore_get(ns,&n->nodeId, &nr);
	// then
	ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
	// finally
	UA_Node_delete(n);
	open62541NodeStore_release(nr);
	open62541NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(findNodeInopen62541NodeStoreWithSeveralEntries) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	open62541NodeStore *ns;
	open62541NodeStore_new(&ns);
	UA_Node* n1; createNode(&n1,0,2253); open62541NodeStore_insert(ns, &n1, 0);
	UA_Node* n2; createNode(&n2,0,2255); open62541NodeStore_insert(ns, &n2, 0);
	UA_Node* n3; createNode(&n3,0,2257); open62541NodeStore_insert(ns, &n3, UA_NODESTORE_INSERT_GETMANAGED);
	UA_Node* n4; createNode(&n4,0,2200); open62541NodeStore_insert(ns, &n4, 0);
	UA_Node* n5; createNode(&n5,0,1); open62541NodeStore_insert(ns, &n5, 0);
	UA_Node* n6; createNode(&n6,0,12); open62541NodeStore_insert(ns, &n6, 0);

	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	retval = open62541NodeStore_get(ns,&(n3->nodeId),&nr);
	// then
	ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
	ck_assert_ptr_eq((void*)nr, (void*)n3);
	// finally
	open62541NodeStore_release(n3);
	open62541NodeStore_release(nr);
	open62541NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(iterateOveropen62541NodeStoreShallNotVisitEmptyNodes) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	open62541NodeStore *ns;
	open62541NodeStore_new(&ns);
	UA_Node* n1; createNode(&n1,0,2253); open62541NodeStore_insert(ns, &n1, 0);
	UA_Node* n2; createNode(&n2,0,2255); open62541NodeStore_insert(ns, &n2, 0);
	UA_Node* n3; createNode(&n3,0,2257); open62541NodeStore_insert(ns, &n3, 0);
	UA_Node* n4; createNode(&n4,0,2200); open62541NodeStore_insert(ns, &n4, 0);
	UA_Node* n5; createNode(&n5,0,1); open62541NodeStore_insert(ns, &n5, 0);
	UA_Node* n6; createNode(&n6,0,12); open62541NodeStore_insert(ns, &n6, 0);

	// when
	zeroCnt = 0;
	visitCnt = 0;
	open62541NodeStore_iterate(ns,checkZeroVisitor);
	// then
	ck_assert_int_eq(zeroCnt, 0);
	ck_assert_int_eq(visitCnt, 6);
	// finally
	open62541NodeStore_delete(ns);
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
	open62541NodeStore *ns;
	open62541NodeStore_new(&ns);
	UA_Node* n;
	UA_Int32 i=0;
	for (; i<200; i++) {
		createNode(&n,0,i); open62541NodeStore_insert(ns, &n, 0);
	}
	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	createNode(&n,0,25);
	retval = open62541NodeStore_get(ns,&(n->nodeId),&nr);
	// then
	ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
	ck_assert_int_eq(nr->nodeId.identifier.numeric,n->nodeId.identifier.numeric);
	// finally
	UA_free((void*)n);
	open62541NodeStore_release(nr);
	open62541NodeStore_delete(ns);
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
	open62541NodeStore *ns;
	open62541NodeStore_new(&ns);
	UA_Node* n;
	UA_Int32 i=0;
	for (; i<200; i++) {
		createNode(&n,0,i); open62541NodeStore_insert(ns, &n, 0);
	}
	// when
	zeroCnt = 0;
	visitCnt = 0;
	open62541NodeStore_iterate(ns,checkZeroVisitor);
	// then
	ck_assert_int_eq(zeroCnt, 0);
	ck_assert_int_eq(visitCnt, 200);
	// finally
	open62541NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(failToFindNonExistantNodeInopen62541NodeStoreWithSeveralEntries) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	open62541NodeStore *ns;
	open62541NodeStore_new(&ns);
	UA_Node* n1; createNode(&n1,0,2253); open62541NodeStore_insert(ns, &n1, 0);
	UA_Node* n2; createNode(&n2,0,2255); open62541NodeStore_insert(ns, &n2, 0);
	UA_Node* n3; createNode(&n3,0,2257); open62541NodeStore_insert(ns, &n3, 0);
	UA_Node* n4; createNode(&n4,0,2200); open62541NodeStore_insert(ns, &n4, 0);
	UA_Node* n5; createNode(&n5,0,1); open62541NodeStore_insert(ns, &n5, 0);
	UA_Node* n6; createNode(&n6,0,12); 

	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	retval = open62541NodeStore_get(ns, &(n6->nodeId), &nr);
	// then
	ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
	// finally
	UA_free((void *)n6);
	open62541NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

/************************************/
/* Performance Profiling Test Cases */
/************************************/

#ifdef MULTITHREADING
struct open62541NodeStoreProfileTest {
	open62541NodeStore *ns;
	UA_Int32 min_val;
	UA_Int32 max_val;
	UA_Int32 rounds;
};

void *profileGetThread(void *arg) {
   	rcu_register_thread();
	struct open62541NodeStoreProfileTest *test = (struct open62541NodeStoreProfileTest*) arg;
	UA_NodeId id = NS0NODEID(0);
	const UA_Node *cn;
	UA_Int32 max_val = test->max_val;
	open62541NodeStore *ns = test->ns;
	for(UA_Int32 x = 0; x<test->rounds; x++) {
		for (UA_Int32 i=test->min_val; i<max_val; i++) {
			id.identifier.numeric = i;
			open62541NodeStore_get(ns,&id, &cn);
			open62541NodeStore_release(cn);
		}
	}
	rcu_unregister_thread();
	
	return UA_NULL;
}
#endif

START_TEST(profileGetDelete) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif

#define N 1000000
	open62541NodeStore *ns;
	open62541NodeStore_new(&ns);
	UA_Int32 i=0;
	UA_Node *n;
	for (; i<N; i++) {
		createNode(&n,0,i); open62541NodeStore_insert(ns, &n, 0);
	}
	clock_t begin, end;
	begin = clock();
#ifdef MULTITHREADING
#define THREADS 4
    pthread_t t[THREADS];
	struct open62541NodeStoreProfileTest p[THREADS];
	for (int i = 0; i < THREADS; i++) {
		p[i] = (struct open62541NodeStoreProfileTest){ns, i*(N/THREADS), (i+1)*(N/THREADS), 50};
		pthread_create(&t[i], UA_NULL, profileGetThread, &p[i]);
	}
	for (int i = 0; i < THREADS; i++)
		pthread_join(t[i], UA_NULL);
	end = clock();
	printf("Time for %d create/get/delete on %d threads in a namespace: %fs.\n", N, THREADS, (double)(end - begin) / CLOCKS_PER_SEC);
#else
	const UA_Node *cn;
	UA_NodeId id = NS0NODEID(0);
	for(UA_Int32 x = 0; x<50; x++) {
	    for(i=0; i<N; i++) {
	        id.identifier.numeric = i;
			open62541NodeStore_get(ns,&id, &cn);
			open62541NodeStore_release(cn);
        }
    }
	end = clock();
	printf("Time for single-threaded %d create/get/delete in a namespace: %fs.\n", N, (double)(end - begin) / CLOCKS_PER_SEC);
#endif

	open62541NodeStore_delete(ns);

#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

Suite * namespace_suite (void) {
	Suite *s = suite_create ("open62541NodeStore");

	TCase *tc_cd = tcase_create ("Create/Delete");
	tcase_add_test (tc_cd, test_open62541NodeStore);
	suite_add_tcase (s, tc_cd);

	TCase* tc_find = tcase_create ("Find");
	tcase_add_test (tc_find, findNodeInopen62541NodeStoreWithSingleEntry);
	tcase_add_test (tc_find, findNodeInopen62541NodeStoreWithSeveralEntries);
	tcase_add_test (tc_find, findNodeInExpandedNamespace);
	tcase_add_test (tc_find, failToFindNonExistantNodeInopen62541NodeStoreWithSeveralEntries);
	tcase_add_test (tc_find, failToFindNodeInOtheropen62541NodeStore);
	suite_add_tcase (s, tc_find);

	TCase* tc_iterate = tcase_create ("Iterate");
	tcase_add_test (tc_iterate, iterateOveropen62541NodeStoreShallNotVisitEmptyNodes);
	tcase_add_test (tc_iterate, iterateOverExpandedNamespaceShallNotVisitEmptyNodes);
	suite_add_tcase (s, tc_iterate);
	
	/* TCase* tc_profile = tcase_create ("Profile"); */
	/* tcase_add_test (tc_profile, profileGetDelete); */
	/* suite_add_tcase (s, tc_profile); */

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
