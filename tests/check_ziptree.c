/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include "ziptree.h"

#define TEST_ITERATIONS 50

static enum ZIP_CMP
compareKeys(const void *k1, const void *k2) {
    unsigned int key1 = *(const unsigned int*)k1;
    unsigned int key2 = *(const unsigned int*)k2;
    if(key1 == key2)
        return ZIP_CMP_EQ;
    return (key1 < key2) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
}

struct treeEntry {
    unsigned int key;
    ZIP_ENTRY(treeEntry) pointers;
};

ZIP_HEAD(tree, treeEntry);
ZIP_FUNCTIONS(tree, treeEntry, pointers, unsigned int, key, compareKeys)

static void
checkTreeInternal(struct treeEntry *e,
                  unsigned int min_key, unsigned int max_key) {
    ck_assert_uint_ge(e->key, min_key);
    ck_assert_uint_le(e->key, max_key);

    struct treeEntry *left = e->pointers.left;
    if(left) {
        ck_assert_uint_le(left->key, e->key);
        checkTreeInternal(left, min_key, e->key);
    }

    struct treeEntry *right = e->pointers.right;
    if(right) {
        ck_assert_uint_ge(right->key, e->key);
        checkTreeInternal(right, e->key, max_key);
    }
}

static void
checkTree(struct tree *e) {
    if(!e->root)
        return;
    struct treeEntry *max_entry = ZIP_MAX(tree, e);
    struct treeEntry *min_entry = ZIP_MIN(tree, e);
    checkTreeInternal(e->root, min_entry->key, max_entry->key);
}

START_TEST(randTree) {
    srand(0);
    struct tree t1 = {NULL};
    for(unsigned int i = 0; i < TEST_ITERATIONS; i++) {
        struct treeEntry *e1 = (struct treeEntry*)malloc(sizeof(struct treeEntry));
        e1->key = (unsigned int)rand();
        ZIP_INSERT(tree, &t1, e1);
    }

    checkTree(&t1);

    while(t1.root) {
        checkTree(&t1);
        struct treeEntry *left = t1.root->pointers.left;
        struct treeEntry *right = t1.root->pointers.right;
        free(t1.root);
        t1.root = ZIP_ZIP(tree, left, right);
    }
} END_TEST

START_TEST(mergeTrees) {
    struct tree t1 = {NULL};
    struct tree t2 = {NULL};
    for(unsigned int i = 0; i < TEST_ITERATIONS; i++) {
        struct treeEntry *e1 = (struct treeEntry*)malloc(sizeof(struct treeEntry));
        struct treeEntry *e2 = (struct treeEntry*)malloc(sizeof(struct treeEntry));
        e1->key = i;
        e2->key = i + 1000;
        ZIP_INSERT(tree, &t1, e1);
        ZIP_INSERT(tree, &t2, e2);
    }

    checkTree(&t1);
    checkTree(&t2);

    struct tree t3;
    t3.root = ZIP_ZIP(tree, t1.root, t2.root);

    checkTree(&t3);

    while(t3.root) {
        checkTree(&t3);
        struct treeEntry *left = t3.root->pointers.left;
        struct treeEntry *right = t3.root->pointers.right;
        free(t3.root);
        t3.root = ZIP_ZIP(tree, left, right);
    }
} END_TEST

START_TEST(splitTree) {
    struct tree t1 = {NULL};
    for(unsigned int i = 0; i < TEST_ITERATIONS; i++) {
        for(size_t j = 0; j < 5; j++) {
            struct treeEntry *e = (struct treeEntry*)malloc(sizeof(struct treeEntry));
            e->key = i;
            ZIP_INSERT(tree, &t1, e);
        }
    }

    checkTree(&t1);

    for(unsigned int split_key = 0; split_key < 110; split_key++) {
        struct tree t2;
        struct tree t3;
        ZIP_UNZIP(tree, &t1, &split_key, &t2, &t3);
        checkTree(&t2);
        checkTree(&t3);

        struct treeEntry *find_right = ZIP_FIND(tree, &t3, &split_key);
        ck_assert(find_right == NULL);
        struct treeEntry *smallest_right = ZIP_MIN(tree, &t3);
        if(smallest_right)
            ck_assert_uint_gt(smallest_right->key, split_key);

        struct treeEntry *largest_left = ZIP_MAX(tree, &t2);
        if(largest_left)
            ck_assert_uint_le(largest_left->key, split_key);

        t1.root = ZIP_ZIP(tree, t2.root, t3.root);
        checkTree(&t1);
    }

    while(t1.root) {
        checkTree(&t1);
        struct treeEntry *left = t1.root->pointers.left;
        struct treeEntry *right = t1.root->pointers.right;
        free(t1.root);
        t1.root = ZIP_ZIP(tree, left, right);
    }
} END_TEST

START_TEST(splitTreeRand) {
    struct tree t1 = {NULL};
    for(unsigned int i = 0; i < TEST_ITERATIONS; i++) {
        unsigned int key = (unsigned int)rand();
        unsigned int num = (unsigned int)rand() % 10;
        for(size_t j = 0; j < num; j++) {
            struct treeEntry *e = (struct treeEntry*)malloc(sizeof(struct treeEntry));
            e->key = key;
            ZIP_INSERT(tree, &t1, e);
        }
    }

    checkTree(&t1);

    for(unsigned int split_key = 0; split_key < 110; split_key++) {
        struct tree t2;
        struct tree t3;
        ZIP_UNZIP(tree, &t1, &split_key, &t2, &t3);
        checkTree(&t2);
        checkTree(&t3);

        struct treeEntry *find_right = ZIP_FIND(tree, &t3, &split_key);
        ck_assert(find_right == NULL);
        struct treeEntry *smallest_right = ZIP_MIN(tree, &t3);
        if(smallest_right)
            ck_assert_uint_gt(smallest_right->key, split_key);

        struct treeEntry *largest_left = ZIP_MAX(tree, &t2);
        if(largest_left)
            ck_assert_uint_le(largest_left->key, split_key);

        t1.root = ZIP_ZIP(tree, t2.root, t3.root);
        checkTree(&t1);
    }

    while(t1.root) {
        checkTree(&t1);
        struct treeEntry *left = t1.root->pointers.left;
        struct treeEntry *right = t1.root->pointers.right;
        free(t1.root);
        t1.root = ZIP_ZIP(tree, left, right);
    }
} END_TEST

/* --- Additional coverage tests --- */

START_TEST(emptyTree) {
    struct tree t = {NULL};
    /* MIN and MAX on empty tree */
    ck_assert_ptr_eq(ZIP_MIN(tree, &t), NULL);
    ck_assert_ptr_eq(ZIP_MAX(tree, &t), NULL);
    /* FIND on empty tree */
    unsigned int key = 42;
    ck_assert_ptr_eq(ZIP_FIND(tree, &t, &key), NULL);
} END_TEST

START_TEST(singleElement) {
    struct tree t = {NULL};
    struct treeEntry *e = (struct treeEntry*)malloc(sizeof(struct treeEntry));
    e->key = 100;
    ZIP_INSERT(tree, &t, e);

    /* MIN and MAX are the same element */
    ck_assert_ptr_eq(ZIP_MIN(tree, &t), e);
    ck_assert_ptr_eq(ZIP_MAX(tree, &t), e);
    /* FIND for existing key */
    ck_assert_ptr_eq(ZIP_FIND(tree, &t, &e->key), e);
    /* FIND for non-existing key */
    unsigned int nokey = 999;
    ck_assert_ptr_eq(ZIP_FIND(tree, &t, &nokey), NULL);

    /* REMOVE the single element */
    struct treeEntry *removed = ZIP_REMOVE(tree, &t, e);
    ck_assert_ptr_eq(removed, e);
    ck_assert_ptr_eq(t.root, NULL);
    free(e);
} END_TEST

START_TEST(findExistingKeys) {
    struct tree t = {NULL};
    struct treeEntry entries[10];
    for(unsigned int i = 0; i < 10; i++) {
        entries[i].key = (i + 1) * 10; /* 10,20,...,100 */
        ZIP_INSERT(tree, &t, &entries[i]);
    }
    checkTree(&t);

    /* Find each inserted key */
    for(unsigned int i = 0; i < 10; i++) {
        unsigned int key = (i + 1) * 10;
        struct treeEntry *found = ZIP_FIND(tree, &t, &key);
        ck_assert_ptr_ne(found, NULL);
        ck_assert_uint_eq(found->key, key);
    }

    /* Find non-existing keys */
    unsigned int nokey1 = 5;
    unsigned int nokey2 = 15;
    unsigned int nokey3 = 200;
    ck_assert_ptr_eq(ZIP_FIND(tree, &t, &nokey1), NULL);
    ck_assert_ptr_eq(ZIP_FIND(tree, &t, &nokey2), NULL);
    ck_assert_ptr_eq(ZIP_FIND(tree, &t, &nokey3), NULL);

    /* Cleanup via remove */
    for(unsigned int i = 0; i < 10; i++) {
        ZIP_REMOVE(tree, &t, &entries[i]);
    }
    ck_assert_ptr_eq(t.root, NULL);
} END_TEST

START_TEST(removeElements) {
    struct tree t = {NULL};
    struct treeEntry *entries[20];
    for(unsigned int i = 0; i < 20; i++) {
        entries[i] = (struct treeEntry*)malloc(sizeof(struct treeEntry));
        entries[i]->key = i;
        ZIP_INSERT(tree, &t, entries[i]);
    }
    checkTree(&t);

    /* Remove elements in mixed order */
    unsigned int removeOrder[] = {5, 0, 19, 10, 15, 3, 7, 18, 1, 12,
                                  2, 4, 6, 8, 9, 11, 13, 14, 16, 17};
    for(unsigned int i = 0; i < 20; i++) {
        struct treeEntry *r = ZIP_REMOVE(tree, &t, entries[removeOrder[i]]);
        ck_assert_ptr_eq(r, entries[removeOrder[i]]);
        if(t.root)
            checkTree(&t);
        free(r);
    }
    ck_assert_ptr_eq(t.root, NULL);
} END_TEST

static void *
countIter(void *context, void *elm) {
    (void)elm;
    unsigned int *count = (unsigned int*)context;
    (*count)++;
    return NULL; /* continue iteration */
}

START_TEST(iterAll) {
    struct tree t = {NULL};
    struct treeEntry entries[15];
    for(unsigned int i = 0; i < 15; i++) {
        entries[i].key = i * 3;
        ZIP_INSERT(tree, &t, &entries[i]);
    }

    unsigned int count = 0;
    ZIP_ITER(tree, &t, (tree_cb)countIter, &count);
    ck_assert_uint_eq(count, 15);

    /* Cleanup */
    for(unsigned int i = 0; i < 15; i++)
        ZIP_REMOVE(tree, &t, &entries[i]);
} END_TEST

START_TEST(iterEmpty) {
    struct tree t = {NULL};
    unsigned int count = 0;
    ZIP_ITER(tree, &t, (tree_cb)countIter, &count);
    ck_assert_uint_eq(count, 0);
} END_TEST

static void *
abortIter(void *context, void *elm) {
    unsigned int *count = (unsigned int*)context;
    (*count)++;
    if(*count >= 3)
        return elm; /* abort after 3 */
    return NULL;
}

START_TEST(iterAbort) {
    struct tree t = {NULL};
    struct treeEntry entries[10];
    for(unsigned int i = 0; i < 10; i++) {
        entries[i].key = i;
        ZIP_INSERT(tree, &t, &entries[i]);
    }

    unsigned int count = 0;
    void *result = ZIP_ITER(tree, &t, (tree_cb)abortIter, &count);
    ck_assert_ptr_ne(result, NULL);
    ck_assert_uint_eq(count, 3);

    for(unsigned int i = 0; i < 10; i++)
        ZIP_REMOVE(tree, &t, &entries[i]);
} END_TEST

START_TEST(iterKey) {
    struct tree t = {NULL};
    /* Insert 3 entries with key=5 and 3 with key=10 */
    struct treeEntry entries[6];
    for(unsigned int i = 0; i < 3; i++) {
        entries[i].key = 5;
        ZIP_INSERT(tree, &t, &entries[i]);
    }
    for(unsigned int i = 3; i < 6; i++) {
        entries[i].key = 10;
        ZIP_INSERT(tree, &t, &entries[i]);
    }

    /* Iterate only key=5 elements */
    unsigned int count5 = 0;
    unsigned int key5 = 5;
    ZIP_ITER_KEY(tree, &t, &key5, (tree_cb)countIter, &count5);
    ck_assert_uint_eq(count5, 3);

    /* Iterate only key=10 elements */
    unsigned int count10 = 0;
    unsigned int key10 = 10;
    ZIP_ITER_KEY(tree, &t, &key10, (tree_cb)countIter, &count10);
    ck_assert_uint_eq(count10, 3);

    /* Iterate non-existing key */
    unsigned int count99 = 0;
    unsigned int key99 = 99;
    ZIP_ITER_KEY(tree, &t, &key99, (tree_cb)countIter, &count99);
    ck_assert_uint_eq(count99, 0);

    for(unsigned int i = 0; i < 6; i++)
        ZIP_REMOVE(tree, &t, &entries[i]);
} END_TEST

START_TEST(removeNonExisting) {
    struct tree t = {NULL};
    struct treeEntry e1;
    e1.key = 42;
    ZIP_INSERT(tree, &t, &e1);

    /* Try to remove an element not in the tree */
    struct treeEntry e2;
    e2.key = 99;
    struct treeEntry *r = ZIP_REMOVE(tree, &t, &e2);
    ck_assert_ptr_eq(r, NULL);

    /* Original still there */
    ck_assert_ptr_eq(ZIP_FIND(tree, &t, &e1.key), &e1);
    ZIP_REMOVE(tree, &t, &e1);
} END_TEST

START_TEST(duplicateKeys) {
    struct tree t = {NULL};
    struct treeEntry entries[5];
    for(unsigned int i = 0; i < 5; i++) {
        entries[i].key = 42; /* all same key */
        ZIP_INSERT(tree, &t, &entries[i]);
    }
    checkTree(&t);

    /* Find returns the topmost */
    unsigned int key = 42;
    ck_assert_ptr_ne(ZIP_FIND(tree, &t, &key), NULL);

    /* Remove all one by one */
    for(unsigned int i = 0; i < 5; i++) {
        struct treeEntry *r = ZIP_REMOVE(tree, &t, &entries[i]);
        ck_assert_ptr_eq(r, &entries[i]);
    }
    ck_assert_ptr_eq(t.root, NULL);
} END_TEST

START_TEST(zipInit) {
    struct tree t;
    ZIP_INIT(&t);
    ck_assert_ptr_eq(t.root, NULL);
    ck_assert_ptr_eq(ZIP_ROOT(&t), NULL);
} END_TEST

START_TEST(leftRightAccess) {
    struct tree t = {NULL};
    struct treeEntry e1, e2, e3;
    e1.key = 50;
    e2.key = 25;
    e3.key = 75;
    ZIP_INSERT(tree, &t, &e1);
    ZIP_INSERT(tree, &t, &e2);
    ZIP_INSERT(tree, &t, &e3);
    checkTree(&t);

    /* At least one of left/right should be non-null for root */
    struct treeEntry *root = ZIP_ROOT(&t);
    ck_assert_ptr_ne(root, NULL);
    /* The tree has 3 elements so the root must have at least one child */
    ck_assert(ZIP_LEFT(root, pointers) != NULL ||
              ZIP_RIGHT(root, pointers) != NULL);

    ZIP_REMOVE(tree, &t, &e1);
    ZIP_REMOVE(tree, &t, &e2);
    ZIP_REMOVE(tree, &t, &e3);
} END_TEST

static void *
freeIter(void *context, void *elm) {
    (void)context;
    free(elm);
    return NULL;
}

START_TEST(iterFreeAll) {
    struct tree t = {NULL};
    for(unsigned int i = 0; i < 30; i++) {
        struct treeEntry *e = (struct treeEntry*)malloc(sizeof(struct treeEntry));
        e->key = i;
        ZIP_INSERT(tree, &t, e);
    }
    /* Use ITER to free all nodes (documented use case) */
    ZIP_ITER(tree, &t, (tree_cb)freeIter, NULL);
} END_TEST

int main(void) {
    int number_failed = 0;
    TCase *tc_parse = tcase_create("ziptree");
    tcase_add_test(tc_parse, randTree);
    tcase_add_test(tc_parse, mergeTrees);
    tcase_add_test(tc_parse, splitTree);
    tcase_add_test(tc_parse, splitTreeRand);

    TCase *tc_ops = tcase_create("ziptree_ops");
    tcase_add_test(tc_ops, emptyTree);
    tcase_add_test(tc_ops, singleElement);
    tcase_add_test(tc_ops, findExistingKeys);
    tcase_add_test(tc_ops, removeElements);
    tcase_add_test(tc_ops, iterAll);
    tcase_add_test(tc_ops, iterEmpty);
    tcase_add_test(tc_ops, iterAbort);
    tcase_add_test(tc_ops, iterKey);
    tcase_add_test(tc_ops, removeNonExisting);
    tcase_add_test(tc_ops, duplicateKeys);
    tcase_add_test(tc_ops, zipInit);
    tcase_add_test(tc_ops, leftRightAccess);
    tcase_add_test(tc_ops, iterFreeAll);

    Suite *s = suite_create("Test ziptree library");
    suite_add_tcase(s, tc_parse);
    suite_add_tcase(s, tc_ops);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
