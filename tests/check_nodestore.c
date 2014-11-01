#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ua_types.h"
#include "server/ua_nodestore.h"
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

START_TEST(test_UA_NodeStore) {
	UA_NodeStore *ns = UA_NULL;
	UA_NodeStore_new(&ns);
	UA_NodeStore_delete(ns);
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

START_TEST(findNodeInUA_NodeStoreWithSingleEntry) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	UA_NodeStore *ns;
	UA_NodeStore_new(&ns);
	UA_Node* n1; createNode(&n1,0,2253);
	UA_NodeStore_insert(ns, &n1, UA_NODESTORE_INSERT_UNIQUE | UA_NODESTORE_INSERT_GETMANAGED);
	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	retval = UA_NodeStore_get(ns,&n1->nodeId,&nr);
	// then
	ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
	ck_assert_ptr_eq((void*)nr, (void*)n1);
	// finally
	UA_NodeStore_release(n1);
	UA_NodeStore_release(nr);
	UA_NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(failToFindNodeInOtherUA_NodeStore) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	UA_NodeStore *ns = UA_NULL;
	UA_NodeStore_new(&ns);

	UA_Node* n1; createNode(&n1,0,2253); UA_NodeStore_insert(ns, &n1, 0);
	UA_Node* n2; createNode(&n2,0,2253); UA_NodeStore_insert(ns, &n2, 0);

	const UA_Node* nr = UA_NULL;
	// when
	UA_Node* n; createNode(&n,1,2255);
	UA_Int32 retval = UA_NodeStore_get(ns,&n->nodeId, &nr);
	// then
	ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
	// finally
	UA_Node_delete(n);
	UA_NodeStore_release(nr);
	UA_NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(findNodeInUA_NodeStoreWithSeveralEntries) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	UA_NodeStore *ns;
	UA_NodeStore_new(&ns);
	UA_Node* n1; createNode(&n1,0,2253); UA_NodeStore_insert(ns, &n1, 0);
	UA_Node* n2; createNode(&n2,0,2255); UA_NodeStore_insert(ns, &n2, 0);
	UA_Node* n3; createNode(&n3,0,2257); UA_NodeStore_insert(ns, &n3, UA_NODESTORE_INSERT_GETMANAGED);
	UA_Node* n4; createNode(&n4,0,2200); UA_NodeStore_insert(ns, &n4, 0);
	UA_Node* n5; createNode(&n5,0,1); UA_NodeStore_insert(ns, &n5, 0);
	UA_Node* n6; createNode(&n6,0,12); UA_NodeStore_insert(ns, &n6, 0);

	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	retval = UA_NodeStore_get(ns,&(n3->nodeId),&nr);
	// then
	ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
	ck_assert_ptr_eq((void*)nr, (void*)n3);
	// finally
	UA_NodeStore_release(n3);
	UA_NodeStore_release(nr);
	UA_NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(iterateOverUA_NodeStoreShallNotVisitEmptyNodes) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	UA_NodeStore *ns;
	UA_NodeStore_new(&ns);
	UA_Node* n1; createNode(&n1,0,2253); UA_NodeStore_insert(ns, &n1, 0);
	UA_Node* n2; createNode(&n2,0,2255); UA_NodeStore_insert(ns, &n2, 0);
	UA_Node* n3; createNode(&n3,0,2257); UA_NodeStore_insert(ns, &n3, 0);
	UA_Node* n4; createNode(&n4,0,2200); UA_NodeStore_insert(ns, &n4, 0);
	UA_Node* n5; createNode(&n5,0,1); UA_NodeStore_insert(ns, &n5, 0);
	UA_Node* n6; createNode(&n6,0,12); UA_NodeStore_insert(ns, &n6, 0);

	// when
	zeroCnt = 0;
	visitCnt = 0;
	UA_NodeStore_iterate(ns,checkZeroVisitor);
	// then
	ck_assert_int_eq(zeroCnt, 0);
	ck_assert_int_eq(visitCnt, 6);
	// finally
	UA_NodeStore_delete(ns);
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
	UA_NodeStore *ns;
	UA_NodeStore_new(&ns);
	UA_Node* n;
	UA_Int32 i=0;
	for (; i<200; i++) {
		createNode(&n,0,i); UA_NodeStore_insert(ns, &n, 0);
	}
	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	createNode(&n,0,25);
	retval = UA_NodeStore_get(ns,&(n->nodeId),&nr);
	// then
	ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
	ck_assert_int_eq(nr->nodeId.identifier.numeric,n->nodeId.identifier.numeric);
	// finally
	UA_free((void*)n);
	UA_NodeStore_release(nr);
	UA_NodeStore_delete(ns);
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
	UA_NodeStore *ns;
	UA_NodeStore_new(&ns);
	UA_Node* n;
	UA_Int32 i=0;
	for (; i<200; i++) {
		createNode(&n,0,i); UA_NodeStore_insert(ns, &n, 0);
	}
	// when
	zeroCnt = 0;
	visitCnt = 0;
	UA_NodeStore_iterate(ns,checkZeroVisitor);
	// then
	ck_assert_int_eq(zeroCnt, 0);
	ck_assert_int_eq(visitCnt, 200);
	// finally
	UA_NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

START_TEST(failToFindNonExistantNodeInUA_NodeStoreWithSeveralEntries) {
#ifdef MULTITHREADING
   	rcu_register_thread();
#endif
	// given
	UA_NodeStore *ns;
	UA_NodeStore_new(&ns);
	UA_Node* n1; createNode(&n1,0,2253); UA_NodeStore_insert(ns, &n1, 0);
	UA_Node* n2; createNode(&n2,0,2255); UA_NodeStore_insert(ns, &n2, 0);
	UA_Node* n3; createNode(&n3,0,2257); UA_NodeStore_insert(ns, &n3, 0);
	UA_Node* n4; createNode(&n4,0,2200); UA_NodeStore_insert(ns, &n4, 0);
	UA_Node* n5; createNode(&n5,0,1); UA_NodeStore_insert(ns, &n5, 0);
	UA_Node* n6; createNode(&n6,0,12); 

	const UA_Node* nr = UA_NULL;
	UA_Int32 retval;
	// when
	retval = UA_NodeStore_get(ns, &(n6->nodeId), &nr);
	// then
	ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
	// finally
	UA_free((void *)n6);
	UA_NodeStore_delete(ns);
#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

/************************************/
/* Performance Profiling Test Cases */
/************************************/

#ifdef MULTITHREADING
struct UA_NodeStoreProfileTest {
	UA_NodeStore *ns;
	UA_Int32 min_val;
	UA_Int32 max_val;
	UA_Int32 rounds;
};

void *profileGetThread(void *arg) {
   	rcu_register_thread();
	struct UA_NodeStoreProfileTest *test = (struct UA_NodeStoreProfileTest*) arg;
	UA_NodeId id = NS0NODEID(0);
	const UA_Node *cn;
	UA_Int32 max_val = test->max_val;
	UA_NodeStore *ns = test->ns;
	for(UA_Int32 x = 0; x<test->rounds; x++) {
		for (UA_Int32 i=test->min_val; i<max_val; i++) {
			id.identifier.numeric = i;
			UA_NodeStore_get(ns,&id, &cn);
			UA_NodeStore_release(cn);
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
	UA_NodeStore *ns;
	UA_NodeStore_new(&ns);
	UA_Int32 i=0;
	UA_Node *n;
	for (; i<N; i++) {
		createNode(&n,0,i); UA_NodeStore_insert(ns, &n, 0);
	}
	clock_t begin, end;
	begin = clock();
#ifdef MULTITHREADING
#define THREADS 4
    pthread_t t[THREADS];
	struct UA_NodeStoreProfileTest p[THREADS];
	for (int i = 0; i < THREADS; i++) {
		p[i] = (struct UA_NodeStoreProfileTest){ns, i*(N/THREADS), (i+1)*(N/THREADS), 50};
		pthread_create(&t[i], NULL, profileGetThread, &p[i]);
	}
	for (int i = 0; i < THREADS; i++)
		pthread_join(t[i], NULL);
	end = clock();
	printf("Time for %d create/get/delete on %d threads in a namespace: %fs.\n", N, THREADS, (double)(end - begin) / CLOCKS_PER_SEC);
#else
	const UA_Node *cn;
	UA_NodeId id = NS0NODEID(0);
	for(UA_Int32 x = 0; x<50; x++) {
	    for(i=0; i<N; i++) {
	        id.identifier.numeric = i;
			UA_NodeStore_get(ns,&id, &cn);
			UA_NodeStore_release(cn);
        }
    }
	end = clock();
	printf("Time for single-threaded %d create/get/delete in a namespace: %fs.\n", N, (double)(end - begin) / CLOCKS_PER_SEC);
#endif

	UA_NodeStore_delete(ns);

#ifdef MULTITHREADING
	rcu_unregister_thread();
#endif
}
END_TEST

Suite * namespace_suite (void) {
	Suite *s = suite_create ("UA_NodeStore");

	TCase *tc_cd = tcase_create ("Create/Delete");
	tcase_add_test (tc_cd, test_UA_NodeStore);
	suite_add_tcase (s, tc_cd);

	TCase* tc_find = tcase_create ("Find");
	tcase_add_test (tc_find, findNodeInUA_NodeStoreWithSingleEntry);
	tcase_add_test (tc_find, findNodeInUA_NodeStoreWithSeveralEntries);
	tcase_add_test (tc_find, findNodeInExpandedNamespace);
	tcase_add_test (tc_find, failToFindNonExistantNodeInUA_NodeStoreWithSeveralEntries);
	tcase_add_test (tc_find, failToFindNodeInOtherUA_NodeStore);
	suite_add_tcase (s, tc_find);

	TCase* tc_iterate = tcase_create ("Iterate");
	tcase_add_test (tc_iterate, iterateOverUA_NodeStoreShallNotVisitEmptyNodes);
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
