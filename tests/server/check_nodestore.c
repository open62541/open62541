/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/util.h>
#include <open62541/plugin/nodestore_default.h>
#include "open62541/plugin/nodestore.h"
#include "open62541/types_generated.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "check.h"

#if UA_MULTITHREADING >= 200
#include <pthread.h>
#endif

UA_Nodestore ns;

static void setupZipTree(void) {
    UA_Nodestore_ZipTree(&ns);
}

static void setupHashMap(void) {
    UA_Nodestore_HashMap(&ns);
}

static void teardown(void) {
    ns.clear(ns.context);
}

static int zeroCnt = 0;
static int visitCnt = 0;
static void checkZeroVisitor(void *context, const UA_Node* node) {
    visitCnt++;
    if (node == NULL) zeroCnt++;
}

static UA_Node* createNode(UA_UInt16 nsid, UA_UInt32 id) {
    UA_Node *p = ns.newNode(&ns.context, UA_NODECLASS_VARIABLE);
    p->head.nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
    p->head.nodeId.namespaceIndex = nsid;
    p->head.nodeId.identifier.numeric = id;
    p->head.nodeClass = UA_NODECLASS_VARIABLE;
    return p;
}

START_TEST(replaceExistingNode) {
    UA_Node* n1 = createNode(0,2253);
    ns.insertNode(ns.context, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0, 2253);
    UA_Node* n2;
    ns.getNodeCopy(ns.context, &in1, &n2);
    UA_StatusCode retval = ns.replaceNode(ns.context, n2);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(replaceOldNode) {
    UA_Node* n1 = createNode(0,2253);
    ns.insertNode(ns.context, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0,2253);
    UA_Node* n2;
    UA_Node* n3;
    ns.getNodeCopy(ns.context, &in1, &n2);
    ns.getNodeCopy(ns.context, &in1, &n3);

    /* shall succeed */
    UA_StatusCode retval = ns.replaceNode(ns.context, n2);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* shall fail */
    retval = ns.replaceNode(ns.context, n3);
    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(findNodeInUA_NodeStoreWithSingleEntry) {
    UA_Node* n1 = createNode(0,2253);
    ns.insertNode(ns.context, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0,2253);
    const UA_Node* nr = ns.getNode(ns.context, &in1, ~(UA_UInt32)0,
                                   UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_uint_eq((uintptr_t)n1, (uintptr_t)nr);
    ns.releaseNode(ns.context, nr);
}
END_TEST

START_TEST(failToFindNodeInOtherUA_NodeStore) {
    UA_Node* n1 = createNode(0,2255);
    ns.insertNode(ns.context, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(1, 2255);
    const UA_Node* nr = ns.getNode(ns.context, &in1, ~(UA_UInt32)0,
                                   UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_uint_eq((uintptr_t)nr, 0);
}
END_TEST

START_TEST(findNodeInUA_NodeStoreWithSeveralEntries) {
    UA_Node* n1 = createNode(0,2253);
    ns.insertNode(ns.context, n1, NULL);
    UA_Node* n2 = createNode(0,2255);
    ns.insertNode(ns.context, n2, NULL);
    UA_Node* n3 = createNode(0,2257);
    ns.insertNode(ns.context, n3, NULL);
    UA_Node* n4 = createNode(0,2200);
    ns.insertNode(ns.context, n4, NULL);
    UA_Node* n5 = createNode(0,1);
    ns.insertNode(ns.context, n5, NULL);
    UA_Node* n6 = createNode(0,12);
    ns.insertNode(ns.context, n6, NULL);

    UA_NodeId in3 = UA_NODEID_NUMERIC(0, 2257);
    const UA_Node* nr = ns.getNode(ns.context, &in3, ~(UA_UInt32)0,
                                   UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_uint_eq((uintptr_t)nr, (uintptr_t)n3);
    ns.releaseNode(ns.context, nr);
}
END_TEST

START_TEST(iterateOverUA_NodeStoreShallNotVisitEmptyNodes) {
    UA_Node* n1 = createNode(0,2253);
    ns.insertNode(ns.context, n1, NULL);
    UA_Node* n2 = createNode(0,2255);
    ns.insertNode(ns.context, n2, NULL);
    UA_Node* n3 = createNode(0,2257);
    ns.insertNode(ns.context, n3, NULL);
    UA_Node* n4 = createNode(0,2200);
    ns.insertNode(ns.context, n4, NULL);
    UA_Node* n5 = createNode(0,1);
    ns.insertNode(ns.context, n5, NULL);
    UA_Node* n6 = createNode(0,12);
    ns.insertNode(ns.context, n6, NULL);

    zeroCnt = 0;
    visitCnt = 0;
    ns.iterate(ns.context, checkZeroVisitor, NULL);
    ck_assert_int_eq(zeroCnt, 0);
    ck_assert_int_eq(visitCnt, 6);
}
END_TEST

START_TEST(findNodeInExpandedNamespace) {
    for(UA_UInt32 i = 0; i < 200; i++) {
        UA_Node* n = createNode(0,i);
        ns.insertNode(ns.context, n, NULL);
    }
    // when
    UA_Node *n2 = createNode(0,25);
    const UA_Node* nr = ns.getNode(ns.context, &n2->head.nodeId, ~(UA_UInt32)0,
                                   UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_int_eq(nr->head.nodeId.identifier.numeric, n2->head.nodeId.identifier.numeric);
    ns.releaseNode(ns.context, nr);
    ns.deleteNode(ns.context, n2);
}
END_TEST

START_TEST(iterateOverExpandedNamespaceShallNotVisitEmptyNodes) {
    for(UA_UInt32 i = 0; i < 200; i++) {
        UA_Node* n = createNode(0,i+1);
        ns.insertNode(ns.context, n, NULL);
    }
    // when
    zeroCnt = 0;
    visitCnt = 0;
    ns.iterate(ns.context, checkZeroVisitor, NULL);
    // then
    ck_assert_int_eq(zeroCnt, 0);
    ck_assert_int_eq(visitCnt, 200);
}
END_TEST

START_TEST(failToFindNonExistentNodeInUA_NodeStoreWithSeveralEntries) {
    UA_Node* n1 = createNode(0,2253);
    ns.insertNode(ns.context, n1, NULL);
    UA_Node* n2 = createNode(0,2255);
    ns.insertNode(ns.context, n2, NULL);
    UA_Node* n3 = createNode(0,2257);
    ns.insertNode(ns.context, n3, NULL);
    UA_Node* n4 = createNode(0,2200);
    ns.insertNode(ns.context, n4, NULL);
    UA_Node* n5 = createNode(0,1);
    ns.insertNode(ns.context, n5, NULL);

    UA_NodeId id = UA_NODEID_NUMERIC(0, 12);
    const UA_Node* nr = ns.getNode(ns.context, &id, ~(UA_UInt32)0,
                                   UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_uint_eq((uintptr_t)nr, 0);
}
END_TEST

/************************************/
/* Performance Profiling Test Cases */
/************************************/

#if UA_MULTITHREADING >= 200
struct UA_NodeStoreProfileTest {
    UA_Int32 min_val;
    UA_Int32 max_val;
    UA_Int32 rounds;
};

static void *profileGetThread(void *arg) {
    struct UA_NodeStoreProfileTest *test = (struct UA_NodeStoreProfileTest*) arg;
    UA_NodeId id;
    UA_NodeId_init(&id);
    UA_Int32 max_val = test->max_val;
    for(UA_Int32 x = 0; x<test->rounds; x++) {
        for(UA_Int32 i=test->min_val; i<max_val; i++) {
            id.identifier.numeric = (UA_UInt32)(i+1);
            const UA_Node* n = ns.getNode(ns.context, &id, ~(UA_UInt32)0,
                                          UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
            ns.releaseNode(ns.context, n);
        }
    }
    return NULL;
}
#endif

#define N 10000 /* make bigger to test */

START_TEST(profileGetDelete) {
    clock_t begin, end;
    begin = clock();

    for(UA_UInt32 i = 0; i < N; i++) {
        UA_Node *n = createNode(0,i+1);
        ns.insertNode(ns.context, n, NULL);
    }

#if UA_MULTITHREADING >= 200
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
    UA_NodeId id = UA_NODEID_NULL;
    for(size_t i = 0; i < N; i++) {
        id.identifier.numeric = (UA_UInt32)i+1;
        const UA_Node *node = ns.getNode(ns.context, &id, ~(UA_UInt32)0,
                                         UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
        ns.releaseNode(ns.context, node);
    }
    end = clock();
    printf("Time for single-threaded %d create/get/delete in a namespace: %fs.\n", N,
           (double)(end - begin) / CLOCKS_PER_SEC);
#endif
}
END_TEST

static Suite * namespace_suite (void) {
    Suite *s = suite_create ("UA_NodeStore");

    TCase* tc_find = tcase_create ("Find-ZipTree");
    tcase_add_checked_fixture(tc_find, setupZipTree, teardown);
    tcase_add_test (tc_find, findNodeInUA_NodeStoreWithSingleEntry);
    tcase_add_test (tc_find, findNodeInUA_NodeStoreWithSeveralEntries);
    tcase_add_test (tc_find, findNodeInExpandedNamespace);
    tcase_add_test (tc_find, failToFindNonExistentNodeInUA_NodeStoreWithSeveralEntries);
    tcase_add_test (tc_find, failToFindNodeInOtherUA_NodeStore);
    suite_add_tcase (s, tc_find);

    TCase *tc_replace = tcase_create("Replace-ZipTree");
    tcase_add_checked_fixture(tc_replace, setupZipTree, teardown);
    tcase_add_test (tc_replace, replaceExistingNode);
    tcase_add_test (tc_replace, replaceOldNode);
    suite_add_tcase (s, tc_replace);

    TCase* tc_iterate = tcase_create ("Iterate-ZipTree");
    tcase_add_checked_fixture(tc_iterate, setupZipTree, teardown);
    tcase_add_test (tc_iterate, iterateOverUA_NodeStoreShallNotVisitEmptyNodes);
    tcase_add_test (tc_iterate, iterateOverExpandedNamespaceShallNotVisitEmptyNodes);
    suite_add_tcase (s, tc_iterate);

    TCase* tc_profile = tcase_create ("Profile-ZipTree");
    tcase_add_checked_fixture(tc_profile, setupZipTree, teardown);
    tcase_add_test (tc_profile, profileGetDelete);
    suite_add_tcase (s, tc_profile);

    TCase* tc_find_hm = tcase_create ("Find-HashMap");
    tcase_add_checked_fixture(tc_find_hm, setupHashMap, teardown);
    tcase_add_test (tc_find_hm, findNodeInUA_NodeStoreWithSingleEntry);
    tcase_add_test (tc_find_hm, findNodeInUA_NodeStoreWithSeveralEntries);
    tcase_add_test (tc_find_hm, findNodeInExpandedNamespace);
    tcase_add_test (tc_find_hm, failToFindNonExistentNodeInUA_NodeStoreWithSeveralEntries);
    tcase_add_test (tc_find_hm, failToFindNodeInOtherUA_NodeStore);
    suite_add_tcase (s, tc_find_hm);

    TCase *tc_replace_hm = tcase_create("Replace-HashMap");
    tcase_add_checked_fixture(tc_replace_hm, setupHashMap, teardown);
    tcase_add_test (tc_replace_hm, replaceExistingNode);
    tcase_add_test (tc_replace_hm, replaceOldNode);
    suite_add_tcase (s, tc_replace_hm);

    TCase* tc_iterate_hm = tcase_create ("Iterate-HashMap");
    tcase_add_checked_fixture(tc_iterate_hm, setupHashMap, teardown);
    tcase_add_test (tc_iterate_hm, iterateOverUA_NodeStoreShallNotVisitEmptyNodes);
    tcase_add_test (tc_iterate_hm, iterateOverExpandedNamespaceShallNotVisitEmptyNodes);
    suite_add_tcase (s, tc_iterate_hm);

    TCase* tc_profile_hm = tcase_create ("Profile-HashMap");
    tcase_add_checked_fixture(tc_profile_hm, setupHashMap, teardown);
    tcase_add_test (tc_profile_hm, profileGetDelete);
    suite_add_tcase (s, tc_profile_hm);

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
