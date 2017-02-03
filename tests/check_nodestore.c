/* This Source Code Form is subject to the terms of the Mozilla Public 
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ua_types.h"
#include "ua_types_generated_handling.h"
#include "server/ua_nodestore.h"
#include "server/ua_server_internal.h"
#include "ua_util.h"
#include "check.h"

#ifdef UA_ENABLE_MULTITHREADING
#include <pthread.h>
#include <urcu.h>
#endif

int zeroCnt = 0;
int visitCnt = 0;
static void checkZeroVisitor(void *visitorHandle, const UA_Node* node) {
    visitCnt++;
    if (node == NULL) zeroCnt++;
}

static void printVisitor(const UA_Node* node) {
    printf("%d\n", node->nodeId.identifier.numeric);
}

static UA_Node* createNode(UA_Int16 nsid, UA_Int32 id) {
    UA_Node *p = (UA_Node *)UA_NodeStore_newNode(UA_NODECLASS_VARIABLE);
    p->nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
    p->nodeId.namespaceIndex = nsid;
    p->nodeId.identifier.numeric = id;
    p->nodeClass = UA_NODECLASS_VARIABLE;
    return p;
}

START_TEST(replaceExistingNode) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0, 2253);
    UA_Node* n2 = UA_NodeStore_getCopy(ns, &in1);
    UA_StatusCode retval = UA_NodeStore_replace(ns, n2);
    
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    
    UA_NodeStore_delete(ns, 0);
    UA_free(ns);
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

START_TEST(replaceOldNode) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0,2253);
    UA_Node* n2 = UA_NodeStore_getCopy(ns, &in1);
    UA_Node* n3 = UA_NodeStore_getCopy(ns, &in1);

    /* shall succeed */
    UA_StatusCode retval = UA_NodeStore_replace(ns, n2);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* shall fail */
    retval = UA_NodeStore_replace(ns, n3);
    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
    
    //UA_Node_deleteMembersAnyNodeClass(n3);
    UA_NodeStore_delete(ns, 0);
    UA_free(ns);
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

START_TEST(findNodeInUA_NodeStoreWithSingleEntry) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif
    // given
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0,2253);
    const UA_Node* nr = UA_NodeStore_get(ns, &in1);
    // then
    ck_assert_int_eq((uintptr_t)n1, (uintptr_t)nr);
    // finally
    UA_NodeStore_delete(ns, 0);
    UA_free(ns);
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

START_TEST(failToFindNodeInOtherUA_NodeStore) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif
    // given
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);

    UA_Node* n1 = createNode(0,2255);
    UA_NodeStore_insert(ns, n1, NULL);

    // when
    UA_NodeId in1 = UA_NODEID_NUMERIC(1, 2255);
    const UA_Node* nr = UA_NodeStore_get(ns, &in1);
    // then
    ck_assert_int_eq((uintptr_t)nr, 0);
    // finally
    UA_NodeStore_delete(ns, 0);
    UA_free(ns);
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

START_TEST(findNodeInUA_NodeStoreWithSeveralEntries) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif
    // given
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1, NULL);
    UA_Node* n2 = createNode(0,2255);
    UA_NodeStore_insert(ns, n2, NULL);
    UA_Node* n3 = createNode(0,2257);
    UA_NodeStore_insert(ns, n3, NULL);
    UA_Node* n4 = createNode(0,2200);
    UA_NodeStore_insert(ns, n4, NULL);
    UA_Node* n5 = createNode(0,1);
    UA_NodeStore_insert(ns, n5, NULL);
    UA_Node* n6 = createNode(0,12);
    UA_NodeStore_insert(ns, n6, NULL);

    // when
    UA_NodeId in3 = UA_NODEID_NUMERIC(0, 2257);
    const UA_Node* nr = UA_NodeStore_get(ns, &in3);
    // then
    ck_assert_int_eq((uintptr_t)nr, (uintptr_t)n3);
    // finally
    UA_NodeStore_delete(ns, 0);
    UA_free(ns);
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

START_TEST(iterateOverUA_NodeStoreShallNotVisitEmptyNodes) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif
    // given
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1, NULL);
    UA_Node* n2 = createNode(0,2255);
    UA_NodeStore_insert(ns, n2, NULL);
    UA_Node* n3 = createNode(0,2257);
    UA_NodeStore_insert(ns, n3, NULL);
    UA_Node* n4 = createNode(0,2200);
    UA_NodeStore_insert(ns, n4, NULL);
    UA_Node* n5 = createNode(0,1);
    UA_NodeStore_insert(ns, n5, NULL);
    UA_Node* n6 = createNode(0,12);
    UA_NodeStore_insert(ns, n6, NULL);

    // when
    zeroCnt = 0;
    visitCnt = 0;
    UA_NodeStore_iterate(ns, NULL, checkZeroVisitor);
    // then
    ck_assert_int_eq(zeroCnt, 0);
    ck_assert_int_eq(visitCnt, 6);
    // finally
    UA_NodeStore_delete(ns, 0);
    UA_free(ns);
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

START_TEST(findNodeInExpandedNamespace) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif
    // given
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);
    UA_Node* n;
    UA_Int32 i=0;
    for (; i<200; i++) {
        n = createNode(0,i);
        UA_NodeStore_insert(ns, n, NULL);
    }
    // when
    UA_Node *n2 = createNode(0,25);
    const UA_Node* nr = UA_NodeStore_get(ns,&n2->nodeId);
    // then
    ck_assert_int_eq(nr->nodeId.identifier.numeric,n2->nodeId.identifier.numeric);
    // finally
    UA_NodeStore_deleteNode(n2);
    UA_NodeStore_delete(ns, 0);
    UA_free(ns);
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

START_TEST(iterateOverExpandedNamespaceShallNotVisitEmptyNodes) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif
    // given
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);
    UA_Node* n;
    UA_Int32 i=0;
    for (; i<200; i++) {
        n = createNode(0,i);
        UA_NodeStore_insert(ns, n, NULL);
    }
    // when
    zeroCnt = 0;
    visitCnt = 0;
    UA_NodeStore_iterate(ns, NULL, checkZeroVisitor);
    // then
    ck_assert_int_eq(zeroCnt, 0);
    ck_assert_int_eq(visitCnt, 200);
    // finally
    UA_NodeStore_delete(ns, 0);
    UA_free(ns);
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

START_TEST(failToFindNonExistantNodeInUA_NodeStoreWithSeveralEntries) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif
    // given
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);
    UA_Node* n1 = createNode(0,2253);
    UA_NodeStore_insert(ns, n1, NULL);
    UA_Node* n2 = createNode(0,2255);
    UA_NodeStore_insert(ns, n2, NULL);
    UA_Node* n3 = createNode(0,2257);
    UA_NodeStore_insert(ns, n3, NULL);
    UA_Node* n4 = createNode(0,2200);
    UA_NodeStore_insert(ns, n4, NULL);
    UA_Node* n5 = createNode(0,1);
    UA_NodeStore_insert(ns, n5, NULL);

    UA_NodeId id = UA_NODEID_NUMERIC(0, 12);

    // when
    const UA_Node* nr = UA_NodeStore_get(ns, &id);
    // then
    ck_assert_int_eq((uintptr_t)nr, 0);
    // finally
    UA_NodeStore_delete(ns, 0);
    UA_free(ns);
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

/************************************/
/* Performance Profiling Test Cases */
/************************************/

#ifdef UA_ENABLE_MULTITHREADING
struct UA_NodeStoreProfileTest {
    UA_NodeStore *ns;
    UA_Int32 min_val;
    UA_Int32 max_val;
    UA_Int32 rounds;
};

static void *profileGetThread(void *arg) {
    UA_RCU_LOCK();
    struct UA_NodeStoreProfileTest *test = (struct UA_NodeStoreProfileTest*) arg;
    UA_NodeId id;
    UA_NodeId_init(&id);
    UA_Int32 max_val = test->max_val;
    UA_NodeStore *ns = test->ns;
    for(UA_Int32 x = 0; x<test->rounds; x++) {
        for(UA_Int32 i=test->min_val; i<max_val; i++) {
            id.identifier.numeric = i;
            UA_NodeStore_get(ns,&id);
        }
    }
    UA_RCU_UNLOCK();
    
    return NULL;
}
#endif

START_TEST(profileGetDelete) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_LOCK();
#endif

#define N 1000000
    UA_NodeStore *ns = UA_NodeStore_new();
    UA_NodeStore_linkNamespace(ns,0);
    UA_Node *n;
    for (int i=0; i<N; i++) {
        n = createNode(0,i);
        UA_NodeStore_insert(ns, n, NULL);
    }
    clock_t begin, end;
    begin = clock();
#ifdef UA_ENABLE_MULTITHREADING
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

    UA_NodeStore_delete(ns, 0);
    UA_free(ns);

#ifdef UA_ENABLE_MULTITHREADING
    UA_RCU_UNLOCK();
#endif
}
END_TEST

static Suite * namespace_suite (void) {
    Suite *s = suite_create ("UA_NodeStore");

    TCase* tc_find = tcase_create ("Find");
    tcase_add_test (tc_find, findNodeInUA_NodeStoreWithSingleEntry);
    tcase_add_test (tc_find, findNodeInUA_NodeStoreWithSeveralEntries);
    tcase_add_test (tc_find, findNodeInExpandedNamespace);
    tcase_add_test (tc_find, failToFindNonExistantNodeInUA_NodeStoreWithSeveralEntries);
    tcase_add_test (tc_find, failToFindNodeInOtherUA_NodeStore);
    suite_add_tcase (s, tc_find);

    TCase *tc_replace = tcase_create("Replace");
    tcase_add_test (tc_replace, replaceExistingNode);
    tcase_add_test (tc_replace, replaceOldNode);
    suite_add_tcase (s, tc_replace);

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
    int number_failed = 0;
    Suite *s = namespace_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr,CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed (sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
