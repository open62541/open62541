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

UA_Nodestore *ns;

static void setupZipTree(void) {
    ns = UA_Nodestore_ZipTree();
}

static void teardown(void) {
    ns->free(ns);
}

static int zeroCnt = 0;
static int visitCnt = 0;
static void checkZeroVisitor(void *context, const UA_Node* node) {
    visitCnt++;
    if (node == NULL) zeroCnt++;
}

static UA_Node* createNode(UA_UInt16 nsid, UA_UInt32 id) {
    UA_Node *p = ns->newNode(ns, UA_NODECLASS_VARIABLE);
    p->head.nodeId = UA_NODEID_NUMERIC(nsid, id);
    p->head.nodeClass = UA_NODECLASS_VARIABLE;
    return p;
}

START_TEST(replaceExistingNode) {
    UA_Node* n1 = createNode(0,2253);
    ns->insertNode(ns, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0, 2253);
    UA_Node* n2;
    ns->getNodeCopy(ns, &in1, &n2);
    UA_StatusCode retval = ns->replaceNode(ns, n2);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(replaceOldNode) {
    UA_Node* n1 = createNode(0,2253);
    ns->insertNode(ns, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0,2253);
    UA_Node* n2;
    UA_Node* n3;
    ns->getNodeCopy(ns, &in1, &n2);
    ns->getNodeCopy(ns, &in1, &n3);

    /* shall succeed */
    UA_StatusCode retval = ns->replaceNode(ns, n2);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* shall fail */
    retval = ns->replaceNode(ns, n3);
    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(findNodeInUA_NodeStoreWithSingleEntry) {
    UA_Node* n1 = createNode(0,2253);
    ns->insertNode(ns, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(0,2253);
    const UA_Node* nr = ns->getNode(ns, &in1, ~(UA_UInt32)0,
                                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_uint_eq((uintptr_t)n1, (uintptr_t)nr);
    ns->releaseNode(ns, nr);
}
END_TEST

START_TEST(failToFindNodeInOtherUA_NodeStore) {
    UA_Node* n1 = createNode(0,2255);
    ns->insertNode(ns, n1, NULL);
    UA_NodeId in1 = UA_NODEID_NUMERIC(1, 2255);
    const UA_Node* nr = ns->getNode(ns, &in1, ~(UA_UInt32)0,
                                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_uint_eq((uintptr_t)nr, 0);
}
END_TEST

START_TEST(findNodeInUA_NodeStoreWithSeveralEntries) {
    UA_Node* n1 = createNode(0,2253);
    ns->insertNode(ns, n1, NULL);
    UA_Node* n2 = createNode(0,2255);
    ns->insertNode(ns, n2, NULL);
    UA_Node* n3 = createNode(0,2257);
    ns->insertNode(ns, n3, NULL);
    UA_Node* n4 = createNode(0,2200);
    ns->insertNode(ns, n4, NULL);
    UA_Node* n5 = createNode(0,1);
    ns->insertNode(ns, n5, NULL);
    UA_Node* n6 = createNode(0,12);
    ns->insertNode(ns, n6, NULL);

    UA_NodeId in3 = UA_NODEID_NUMERIC(0, 2257);
    const UA_Node* nr = ns->getNode(ns, &in3, ~(UA_UInt32)0,
                                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_uint_eq((uintptr_t)nr, (uintptr_t)n3);
    ns->releaseNode(ns, nr);
}
END_TEST

START_TEST(iterateOverUA_NodeStoreShallNotVisitEmptyNodes) {
    UA_Node* n1 = createNode(0,2253);
    ns->insertNode(ns, n1, NULL);
    UA_Node* n2 = createNode(0,2255);
    ns->insertNode(ns, n2, NULL);
    UA_Node* n3 = createNode(0,2257);
    ns->insertNode(ns, n3, NULL);
    UA_Node* n4 = createNode(0,2200);
    ns->insertNode(ns, n4, NULL);
    UA_Node* n5 = createNode(0,1);
    ns->insertNode(ns, n5, NULL);
    UA_Node* n6 = createNode(0,12);
    ns->insertNode(ns, n6, NULL);

    zeroCnt = 0;
    visitCnt = 0;
    ns->iterate(ns, checkZeroVisitor, NULL);
    ck_assert_int_eq(zeroCnt, 0);
    ck_assert_int_eq(visitCnt, 6);
}
END_TEST

START_TEST(findNodeInExpandedNamespace) {
    for(UA_UInt32 i = 0; i < 200; i++) {
        UA_Node* n = createNode(0,i);
        ns->insertNode(ns, n, NULL);
    }
    // when
    UA_Node *n2 = createNode(0,25);
    const UA_Node* nr = ns->getNode(ns, &n2->head.nodeId, ~(UA_UInt32)0,
                                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_int_eq(nr->head.nodeId.identifier.numeric, n2->head.nodeId.identifier.numeric);
    ns->releaseNode(ns, nr);
    ns->deleteNode(ns, n2);
}
END_TEST

START_TEST(iterateOverExpandedNamespaceShallNotVisitEmptyNodes) {
    for(UA_UInt32 i = 0; i < 200; i++) {
        UA_Node* n = createNode(0,i+1);
        ns->insertNode(ns, n, NULL);
    }
    // when
    zeroCnt = 0;
    visitCnt = 0;
    ns->iterate(ns, checkZeroVisitor, NULL);
    // then
    ck_assert_int_eq(zeroCnt, 0);
    ck_assert_int_eq(visitCnt, 200);
}
END_TEST

START_TEST(failToFindNonExistentNodeInUA_NodeStoreWithSeveralEntries) {
    UA_Node* n1 = createNode(0,2253);
    ns->insertNode(ns, n1, NULL);
    UA_Node* n2 = createNode(0,2255);
    ns->insertNode(ns, n2, NULL);
    UA_Node* n3 = createNode(0,2257);
    ns->insertNode(ns, n3, NULL);
    UA_Node* n4 = createNode(0,2200);
    ns->insertNode(ns, n4, NULL);
    UA_Node* n5 = createNode(0,1);
    ns->insertNode(ns, n5, NULL);

    UA_NodeId id = UA_NODEID_NUMERIC(0, 12);
    const UA_Node* nr = ns->getNode(ns, &id, ~(UA_UInt32)0,
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
            const UA_Node* n = ns->getNode(ns, &id, ~(UA_UInt32)0,
                                           UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
            ns->releaseNode(ns, n);
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
        ns->insertNode(ns, n, NULL);
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
        const UA_Node *node = ns->getNode(ns, &id, ~(UA_UInt32)0,
                                          UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
        ns->releaseNode(ns, node);
    }
    end = clock();
    printf("Time for single-threaded %d create/get/delete in a namespace: %fs.\n", N,
           (double)(end - begin) / CLOCKS_PER_SEC);
#endif
}
END_TEST

/* --- Extended coverage tests --- */

START_TEST(insertAndDeleteNode) {
    UA_Node *n = createNode(0, 5000);
    UA_StatusCode retval = ns->insertNode(ns, n, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Remove the node from the store */
    UA_NodeId id = UA_NODEID_NUMERIC(0, 5000);
    retval = ns->removeNode(ns, &id);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Should be gone */
    const UA_Node *nr = ns->getNode(ns, &id, ~(UA_UInt32)0,
                                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_ptr_eq(nr, NULL);
} END_TEST

START_TEST(insertDuplicateNode) {
    UA_Node *n1 = createNode(0, 6000);
    UA_StatusCode retval = ns->insertNode(ns, n1, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Try to insert another node with the same NodeId */
    UA_Node *n2 = createNode(0, 6000);
    retval = ns->insertNode(ns, n2, NULL);
    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(getNodeCopy_modifyAndReplace) {
    UA_Node *n = createNode(0, 7000);
    ns->insertNode(ns, n, NULL);

    UA_NodeId id = UA_NODEID_NUMERIC(0, 7000);
    UA_Node *copy;
    UA_StatusCode retval = ns->getNodeCopy(ns, &id, &copy);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Modify the copy */
    copy->head.browseName = UA_QUALIFIEDNAME_ALLOC(0, "ModifiedName");

    /* Replace with the modified copy */
    retval = ns->replaceNode(ns, copy);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify the change */
    const UA_Node *nr = ns->getNode(ns, &id, ~(UA_UInt32)0,
                                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_ptr_ne(nr, NULL);
    UA_QualifiedName expected = UA_QUALIFIEDNAME(0, "ModifiedName");
    ck_assert(UA_QualifiedName_equal(&nr->head.browseName, &expected));
    ns->releaseNode(ns, nr);
} END_TEST

START_TEST(getNodeCopy_nonExistent) {
    UA_NodeId id = UA_NODEID_NUMERIC(0, 99999);
    UA_Node *copy;
    UA_StatusCode retval = ns->getNodeCopy(ns, &id, &copy);
    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(newNodeAllClasses) {
    /* Create and insert nodes of different classes */
    UA_NodeClass classes[] = {
        UA_NODECLASS_OBJECT,
        UA_NODECLASS_VARIABLE,
        UA_NODECLASS_METHOD,
        UA_NODECLASS_OBJECTTYPE,
        UA_NODECLASS_VARIABLETYPE,
        UA_NODECLASS_DATATYPE,
        UA_NODECLASS_REFERENCETYPE,
        UA_NODECLASS_VIEW
    };
    for(size_t i = 0; i < 8; i++) {
        UA_Node *n = ns->newNode(ns, classes[i]);
        ck_assert_ptr_ne(n, NULL);
        n->head.nodeId = UA_NODEID_NUMERIC(0, (UA_UInt32)(8000 + i));
        n->head.nodeClass = classes[i];
        UA_StatusCode retval = ns->insertNode(ns, n, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    }

    /* Verify all can be retrieved */
    for(size_t i = 0; i < 8; i++) {
        UA_NodeId id = UA_NODEID_NUMERIC(0, (UA_UInt32)(8000 + i));
        const UA_Node *nr = ns->getNode(ns, &id, ~(UA_UInt32)0,
                                        UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
        ck_assert_ptr_ne(nr, NULL);
        ck_assert_int_eq(nr->head.nodeClass, classes[i]);
        ns->releaseNode(ns, nr);
    }
} END_TEST

START_TEST(insertNodeWithOutNodeId) {
    /* Insert with outNodeId to capture the assigned ID */
    UA_Node *n = createNode(0, 9000);
    UA_NodeId outId;
    UA_NodeId_init(&outId);
    UA_StatusCode retval = ns->insertNode(ns, n, &outId);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(outId.identifier.numeric, 9000);
    UA_NodeId_clear(&outId);
} END_TEST

START_TEST(iterateEmptyStore) {
    zeroCnt = 0;
    visitCnt = 0;
    ns->iterate(ns, checkZeroVisitor, NULL);
    ck_assert_int_eq(visitCnt, 0);
    ck_assert_int_eq(zeroCnt, 0);
} END_TEST

START_TEST(removeNodeThenFind) {
    /* Insert two nodes, remove one, verify the other is still there */
    UA_Node *n1 = createNode(0, 10001);
    ns->insertNode(ns, n1, NULL);
    UA_Node *n2 = createNode(0, 10002);
    ns->insertNode(ns, n2, NULL);

    /* Remove first node */
    UA_NodeId id1 = UA_NODEID_NUMERIC(0, 10001);
    UA_StatusCode retval = ns->removeNode(ns, &id1);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* First should be gone, second should remain */
    const UA_Node *nr1 = ns->getNode(ns, &id1, ~(UA_UInt32)0,
                                     UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_ptr_eq(nr1, NULL);

    UA_NodeId id2 = UA_NODEID_NUMERIC(0, 10002);
    const UA_Node *nr2 = ns->getNode(ns, &id2, ~(UA_UInt32)0,
                                     UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    ck_assert_ptr_ne(nr2, NULL);
    ns->releaseNode(ns, nr2);
} END_TEST

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

    TCase* tc_ext = tcase_create ("Extended-ZipTree");
    tcase_add_checked_fixture(tc_ext, setupZipTree, teardown);
    tcase_add_test (tc_ext, insertAndDeleteNode);
    tcase_add_test (tc_ext, insertDuplicateNode);
    tcase_add_test (tc_ext, getNodeCopy_modifyAndReplace);
    tcase_add_test (tc_ext, getNodeCopy_nonExistent);
    tcase_add_test (tc_ext, newNodeAllClasses);
    tcase_add_test (tc_ext, insertNodeWithOutNodeId);
    tcase_add_test (tc_ext, iterateEmptyStore);
    tcase_add_test (tc_ext, removeNodeThenFind);
    suite_add_tcase (s, tc_ext);

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
