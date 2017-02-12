/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ua_types.h"
#include "server/ua_nodestore.h"
#include "server/ua_server_internal.h"
#include "ua_util.h"
#include "check.h"

#ifdef UA_ENABLE_MULTITHREADING
#include <pthread.h>
#include <urcu.h>
#endif

UA_NodeStore *ns;

static void setup(void) {
    ns = UA_NodeStore_new();
    UA_RCU_LOCK();
}

static void teardown(void) {
    UA_NodeStore_delete(ns);
    UA_RCU_UNLOCK();
}

int zeroCnt = 0;
int visitCnt = 0;
static void checkZeroVisitor(const UA_Node* node) {
    visitCnt++;
    if (node == NULL) zeroCnt++;
}

static void printVisitor(const UA_Node* node) {
    printf("%d\n", node->nodeId.identifier.numeric);
}

static UA_Node* createNode(UA_Int16 nsid, UA_Int32 id) {
    UA_Node *p = (UA_Node *)UA_NodeStore_newVariableNode();
    p->nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
    p->nodeId.namespaceIndex = nsid;
    p->nodeId.identifier.numeric = id;
    p->nodeClass = UA_NODECLASS_VARIABLE;
    return p;
}

START_TEST(replaceExistingNode) {
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0, 2253);
    UA_Node* n2 = UA_NodeStore_getCopy(ns, &in1);
    UA_StatusCode retval = UA_NodeStore_replace(ns, n2);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(replaceOldNode) {
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0,2253);
    UA_Node* n2 = UA_NodeStore_getCopy(ns, &in1);
    UA_Node* n3 = UA_NodeStore_getCopy(ns, &in1);

    /* shall succeed */
    UA_StatusCode retval = UA_NodeStore_replace(ns, n2);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* shall fail */
    retval = UA_NodeStore_replace(ns, n3);
    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(findNodeInUA_NodeStoreWithSingleEntry) {
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0,2253);
    const UA_Node* nr = UA_NodeStore_get(ns, &in1);
    ck_assert_int_eq((uintptr_t)n1, (uintptr_t)nr);
}
END_TEST

START_TEST(failToFindNodeInOtherUA_NodeStore) {
    UA_Node* n1 = createNode(0,2255);
    UA_NodeStore_insert(ns, n1);
    UA_NodeId in1 = UA_NODEID_NUMERIC(1, 2255);
    const UA_Node* nr = UA_NodeStore_get(ns, &in1);
    ck_assert_int_eq((uintptr_t)nr, 0);
}
END_TEST

START_TEST(findNodeInUA_NodeStoreWithSeveralEntries) {
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1);
    UA_Node* n2 = createNode(0,2255);
    UA_NodeStore_insert(ns, n2);
    UA_Node* n3 = createNode(0,2257);
    UA_NodeStore_insert(ns, n3);
    UA_Node* n4 = createNode(0,2200);
    UA_NodeStore_insert(ns, n4);
    UA_Node* n5 = createNode(0,1);
    UA_NodeStore_insert(ns, n5);
    UA_Node* n6 = createNode(0,12);
    UA_NodeStore_insert(ns, n6);

    UA_NodeId in3 = UA_NODEID_NUMERIC(0, 2257);
    const UA_Node* nr = UA_NodeStore_get(ns, &in3);
    ck_assert_int_eq((uintptr_t)nr, (uintptr_t)n3);
}
END_TEST

START_TEST(iterateOverUA_NodeStoreShallNotVisitEmptyNodes) {
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1);
    UA_Node* n2 = createNode(0,2255);
    UA_NodeStore_insert(ns, n2);
    UA_Node* n3 = createNode(0,2257);
    UA_NodeStore_insert(ns, n3);
    UA_Node* n4 = createNode(0,2200);
    UA_NodeStore_insert(ns, n4);
    UA_Node* n5 = createNode(0,1);
    UA_NodeStore_insert(ns, n5);
    UA_Node* n6 = createNode(0,12);
    UA_NodeStore_insert(ns, n6);

    zeroCnt = 0;
    visitCnt = 0;
    UA_NodeStore_iterate(ns,checkZeroVisitor);
    ck_assert_int_eq(zeroCnt, 0);
    ck_assert_int_eq(visitCnt, 6);
}
END_TEST

START_TEST(findNodeInExpandedNamespace) {
    for(UA_UInt32 i = 0; i < 200; i++) {
        UA_Node* n = createNode(0,i);
        UA_NodeStore_insert(ns, n);
    }
    // when
    UA_Node *n2 = createNode(0,25);
    const UA_Node* nr = UA_NodeStore_get(ns,&n2->nodeId);
    ck_assert_int_eq(nr->nodeId.identifier.numeric,n2->nodeId.identifier.numeric);
    UA_NodeStore_deleteNode(n2);
}
END_TEST

START_TEST(iterateOverExpandedNamespaceShallNotVisitEmptyNodes) {
    for(UA_UInt32 i = 0; i < 200; i++) {
        UA_Node* n = createNode(0,i);
        UA_NodeStore_insert(ns, n);
    }
    // when
    zeroCnt = 0;
    visitCnt = 0;
    UA_NodeStore_iterate(ns,checkZeroVisitor);
    // then
    ck_assert_int_eq(zeroCnt, 0);
    ck_assert_int_eq(visitCnt, 200);
}
END_TEST

START_TEST(failToFindNonExistantNodeInUA_NodeStoreWithSeveralEntries) {
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1);
    UA_Node* n2 = createNode(0,2255);
    UA_NodeStore_insert(ns, n2);
    UA_Node* n3 = createNode(0,2257);
    UA_NodeStore_insert(ns, n3);
    UA_Node* n4 = createNode(0,2200);
    UA_NodeStore_insert(ns, n4);
    UA_Node* n5 = createNode(0,1);
    UA_NodeStore_insert(ns, n5);

    UA_NodeId id = UA_NODEID_NUMERIC(0, 12);
    const UA_Node* nr = UA_NodeStore_get(ns, &id);
    ck_assert_int_eq((uintptr_t)nr, 0);
}
END_TEST

/************************************/
/* Performance Profiling Test Cases */
/************************************/

#ifdef UA_ENABLE_MULTITHREADING
struct UA_NodeStoreProfileTest {
    UA_Int32 min_val;
    UA_Int32 max_val;
    UA_Int32 rounds;
};

static void *profileGetThread(void *arg) {
    rcu_register_thread();
    UA_RCU_LOCK();
    struct UA_NodeStoreProfileTest *test = (struct UA_NodeStoreProfileTest*) arg;
    UA_NodeId id;
    UA_NodeId_init(&id);
    UA_Int32 max_val = test->max_val;
    for(UA_Int32 x = 0; x<test->rounds; x++) {
        for(UA_Int32 i=test->min_val; i<max_val; i++) {
            id.identifier.numeric = i;
            UA_NodeStore_get(ns,&id);
        }
    }
    UA_RCU_UNLOCK();
    rcu_unregister_thread();
    return NULL;
}
#endif

START_TEST(profileGetDelete) {
#define N 1000000
    for(UA_UInt32 i = 0; i < N; i++) {
        UA_Node *n = createNode(0,i);
        UA_NodeStore_insert(ns, n);
    }
    clock_t begin, end;
    begin = clock();
#ifdef UA_ENABLE_MULTITHREADING
#define THREADS 4
    pthread_t t[THREADS];
    struct UA_NodeStoreProfileTest p[THREADS];
    for (int i = 0; i < THREADS; i++) {
        p[i] = (struct UA_NodeStoreProfileTest){i*(N/THREADS), (i+1)*(N/THREADS), 50};
        pthread_create(&t[i], NULL, profileGetThread, &p[i]);
    }
    for (int i = 0; i < THREADS; i++)
        pthread_join(t[i], NULL);
    end = clock();
    printf("Time for %d create/get/delete on %d threads in a namespace: %fs.\n", N, THREADS, (double)(end - begin) / CLOCKS_PER_SEC);
#else
    UA_NodeId id;
    UA_NodeId_init(&id);
    for(UA_Int32 x = 0; x<50; x++) {
        for(int i=0; i<N; i++) {
            id.identifier.numeric = i;
            UA_NodeStore_get(ns,&id);
        }
    }
    end = clock();
    printf("Time for single-threaded %d create/get/delete in a namespace: %fs.\n", N, (double)(end - begin) / CLOCKS_PER_SEC);
#endif
}
END_TEST

static Suite * namespace_suite (void) {
    Suite *s = suite_create ("UA_NodeStore");

    TCase* tc_find = tcase_create ("Find");
    tcase_add_checked_fixture(tc_find, setup, teardown);
    tcase_add_test (tc_find, findNodeInUA_NodeStoreWithSingleEntry);
    tcase_add_test (tc_find, findNodeInUA_NodeStoreWithSeveralEntries);
    tcase_add_test (tc_find, findNodeInExpandedNamespace);
    tcase_add_test (tc_find, failToFindNonExistantNodeInUA_NodeStoreWithSeveralEntries);
    tcase_add_test (tc_find, failToFindNodeInOtherUA_NodeStore);
    suite_add_tcase (s, tc_find);

    TCase *tc_replace = tcase_create("Replace");
    tcase_add_checked_fixture(tc_replace, setup, teardown);
    tcase_add_test (tc_replace, replaceExistingNode);
    tcase_add_test (tc_replace, replaceOldNode);
    suite_add_tcase (s, tc_replace);

    TCase* tc_iterate = tcase_create ("Iterate");
    tcase_add_checked_fixture(tc_iterate, setup, teardown);
    tcase_add_test (tc_iterate, iterateOverUA_NodeStoreShallNotVisitEmptyNodes);
    tcase_add_test (tc_iterate, iterateOverExpandedNamespaceShallNotVisitEmptyNodes);
    suite_add_tcase (s, tc_iterate);
    
    /* TCase* tc_profile = tcase_create ("Profile"); */
    /* tcase_add_checked_fixture(tc_profile, setup, teardown); */
    /* tcase_add_test (tc_profile, profileGetDelete); */
    /* suite_add_tcase (s, tc_profile); */

    return s;
}


int main (void) {
#ifdef UA_ENABLE_MULTITHREADING
    rcu_init();
    rcu_register_thread();
#endif
    int number_failed = 0;
    Suite *s = namespace_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr,CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed (sr);
    srunner_free(sr);
#ifdef UA_ENABLE_MULTITHREADING
    rcu_barrier();
    rcu_unregister_thread();
#endif
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
