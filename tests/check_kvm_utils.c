/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "util/ua_util_internal.h"

#include <stdlib.h>
#include <stdio.h>

#include "check.h"

static UA_KeyValueMap*
keyValueMap_setup(size_t n, size_t offsetKey, size_t offsetValue) {
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();

    for(size_t i=0; i<n; i++) {
        char key[10];
        snprintf(key, 10, "key%02d", (UA_UInt16) (i + offsetKey));
        UA_UInt16 *value = UA_UInt16_new();
        *value = (UA_UInt16) (i + offsetValue);
        UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, key), value, &UA_TYPES[UA_TYPES_UINT16]);
        UA_free(value);
    }
    return kvm;
}

START_TEST(CheckNullArgs) {
        // UA_KeyValueMap_setScalar(&UA_KEYVALUEMAP_NULL, UA_QUALIFIEDNAME(0, "any"), 0, &UA_TYPES[UA_TYPES_UINT16]);
        //UA_KeyValueMap_get(&UA_KEYVALUEMAP_NULL, UA_QUALIFIEDNAME(0, "any"));
        // UA_KeyValueMap_isEmpty(&UA_KEYVALUEMAP_NULL);
        UA_KeyValueMap map;
        UA_StatusCode res = UA_KeyValueMap_copy(&UA_KEYVALUEMAP_NULL, &map);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(map.mapSize, UA_KEYVALUEMAP_NULL.mapSize);
        ck_assert(map.map == UA_KEYVALUEMAP_NULL.map);

        res = UA_KeyValueMap_copy(NULL, &map);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

        res = UA_KeyValueMap_copy(&UA_KEYVALUEMAP_NULL, NULL);
        ck_assert_uint_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

        res = UA_KeyValueMap_copy(NULL, NULL);
        ck_assert_uint_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);

        UA_KeyValueMap_clear(&map);

} END_TEST

START_TEST(CheckKVMContains) {
        UA_KeyValueMap *kvm = keyValueMap_setup(10, 0, 0);

        for(UA_UInt16 i=0; i<10; i++) {
            char key[6] = "key00";
            sprintf(&key[3], "%02d", i);
            UA_Boolean doesContain = UA_KeyValueMap_contains(kvm, UA_QUALIFIEDNAME(0, key));
            ck_assert(doesContain);
        }

        for(UA_UInt16 i=10; i<20; i++) {
            char key[6] = "key10";
            sprintf(&key[3], "%02d", i);
            UA_Boolean doesContain = UA_KeyValueMap_contains(kvm, UA_QUALIFIEDNAME(0, key));
            ck_assert(!doesContain);
        }

        UA_KeyValueMap_delete(kvm);
}
END_TEST

START_TEST(CheckKVMCopy) {
        UA_KeyValueMap *kvm = keyValueMap_setup(10, 0, 0);
        UA_KeyValueMap kvmCopy;
        UA_KeyValueMap_copy(kvm, &kvmCopy);

        for(size_t i = 0; i < 10; i++) {
            ck_assert(UA_QualifiedName_equal(&kvm->map[i].key, &kvmCopy.map[i].key));
        }
        UA_KeyValueMap_delete(kvm);
        UA_KeyValueMap_clear(&kvmCopy);
}
END_TEST

START_TEST(CheckKVMIsEmpty) {
        UA_KeyValueMap *kvm = UA_KeyValueMap_new();
        ck_assert(UA_KeyValueMap_isEmpty(kvm));

        UA_UInt16 value = 1;
        UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key01"), &value,
                                 &UA_TYPES[UA_TYPES_UINT16]);
        ck_assert(!UA_KeyValueMap_isEmpty(kvm));

        UA_KeyValueMap_remove(kvm, UA_QUALIFIEDNAME(0, "key01"));
        ck_assert(UA_KeyValueMap_isEmpty(kvm));

        ck_assert(UA_KeyValueMap_isEmpty(NULL));
        ck_assert(UA_KeyValueMap_isEmpty(&UA_KEYVALUEMAP_NULL));

        UA_KeyValueMap_delete(kvm);
}
END_TEST

START_TEST(CheckKVMRemove) {
        UA_KeyValueMap *kvm = keyValueMap_setup(10, 0, 0);
        
        for(size_t i = 0; i < 10; i++) {
            char key[10];
            snprintf(key, 10, "key%02d", (UA_UInt16) i);
            UA_StatusCode retval = UA_KeyValueMap_remove(kvm, UA_QUALIFIEDNAME(0, key));
            ck_assert_uint_eq(retval,UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(kvm->mapSize, 9-i);
        }
        UA_KeyValueMap_delete(kvm);
}
END_TEST

START_TEST(CheckKVMCountMergedIntersecting) {
        UA_KeyValueMap *kvmLhs = keyValueMap_setup(10, 0, 0);
        UA_KeyValueMap *kvmRhs = keyValueMap_setup(10, 5, 10);

        /**
         key00: 0
         key01: 1
            ...
         key09: 9
         ---------------------
         key05: 10
         key06: 11
            ...
         key14: 19
         */

        UA_KeyValueMap_merge(kvmLhs, kvmRhs);
        ck_assert_uint_eq(kvmLhs->mapSize, 15);

        for(size_t i = 0; i < kvmLhs->mapSize; i++) {
            char key[10];
            snprintf(key, 10, "key%02d", (UA_UInt16) i);
            const UA_UInt16 *value = (const UA_UInt16*)
                UA_KeyValueMap_getScalar(kvmLhs, UA_QUALIFIEDNAME(0, key),
                                         &UA_TYPES[UA_TYPES_UINT16]);
            if(i < 5) {
                ck_assert_uint_eq(*value, i);
            } else {
                ck_assert_uint_eq(*value, i+5);
            }
        }

        UA_KeyValueMap_delete(kvmLhs);
        UA_KeyValueMap_delete(kvmRhs);
    }
END_TEST


START_TEST(CheckKVMCountMergedCommon) {
        UA_KeyValueMap *kvmLhs = keyValueMap_setup(10, 0, 0);
        UA_KeyValueMap *kvmRhs = keyValueMap_setup(10, 0, 10);
        UA_KeyValueMap_merge(kvmLhs, kvmRhs);
        ck_assert_uint_eq(kvmLhs->mapSize, 10);

        for(size_t i = 0; i < kvmLhs->mapSize; i++) {
            char key[10];
            snprintf(key, 10, "key%02d", (UA_UInt16) i);
            const UA_UInt16 *value = (const UA_UInt16*)
                UA_KeyValueMap_getScalar(kvmLhs, UA_QUALIFIEDNAME(0, key),
                                         &UA_TYPES[UA_TYPES_UINT16]);

            ck_assert_uint_eq(*value, i+10);
        }

        UA_KeyValueMap_delete(kvmLhs);
        UA_KeyValueMap_delete(kvmRhs);
    }
END_TEST

START_TEST(CheckKVMCountMergedComplementary) {
        UA_KeyValueMap *kvmLhs = keyValueMap_setup(10, 0, 0);
        UA_KeyValueMap *kvmRhs = keyValueMap_setup(10, 10, 10);
        UA_KeyValueMap_merge(kvmLhs, kvmRhs);
        ck_assert_uint_eq(kvmLhs->mapSize, 20);

        for(size_t i = 0; i < kvmLhs->mapSize; i++) {
            char key[10];
            snprintf(key, 10, "key%02d", (UA_UInt16) i);
            const UA_UInt16 *value = (const UA_UInt16*)
                UA_KeyValueMap_getScalar(kvmLhs, UA_QUALIFIEDNAME(0, key),
                                         &UA_TYPES[UA_TYPES_UINT16]);
            ck_assert_uint_eq(*value, i);
        }

        UA_KeyValueMap_delete(kvmLhs);
        UA_KeyValueMap_delete(kvmRhs);
    }
END_TEST

START_TEST(CheckKVMGetMissing) {
    /* get on a never-added key returns NULL */
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    const UA_Variant *v =
        UA_KeyValueMap_get(kvm, UA_QUALIFIEDNAME(1, "does-not-exist"));
    ck_assert_ptr_eq(v, NULL);
    UA_KeyValueMap_delete(kvm);
}
END_TEST

START_TEST(CheckKVMGetScalarMissing) {
    /* getScalar on a never-added key returns NULL */
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    const void *p = UA_KeyValueMap_getScalar(
        kvm, UA_QUALIFIEDNAME(1, "does-not-exist"), &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_eq(p, NULL);
    UA_KeyValueMap_delete(kvm);
}
END_TEST

START_TEST(CheckKVMRemoveMissing) {
    /* Removing a non-existent key must not error. */
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_StatusCode r = UA_KeyValueMap_remove(
        kvm, UA_QUALIFIEDNAME(1, "never-added"));
    /* Behavior is implementation-defined; must not crash and must
     * return either GOOD or BADNOTFOUND. */
    ck_assert(r == UA_STATUSCODE_GOOD || r == UA_STATUSCODE_BADNOTFOUND);
    UA_KeyValueMap_delete(kvm);
}
END_TEST

START_TEST(CheckKVMSetArray) {
    /* set + get an array of UA_UInt32. */
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_UInt32 arr[3] = {10, 20, 30};
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setArrayCopy(&v, arr, 3, &UA_TYPES[UA_TYPES_UINT32]);
    UA_StatusCode r = UA_KeyValueMap_set(kvm, UA_QUALIFIEDNAME(1, "arr"), &v);
    ck_assert_uint_eq(r, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);

    const UA_Variant *out = UA_KeyValueMap_get(kvm, UA_QUALIFIEDNAME(1, "arr"));
    ck_assert_ptr_ne(out, NULL);
    ck_assert(out->type == &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(out->arrayLength, 3);
    ck_assert_uint_eq(((UA_UInt32 *)out->data)[0], 10);
    ck_assert_uint_eq(((UA_UInt32 *)out->data)[2], 30);

    UA_KeyValueMap_delete(kvm);
}
END_TEST

START_TEST(CheckKVMSetReplace) {
    /* Setting the same key twice must replace the value, not duplicate. */
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();
    UA_UInt32 v1 = 1, v2 = 2;
    UA_Variant va, vb;
    UA_Variant_init(&va);
    UA_Variant_init(&vb);
    UA_Variant_setScalar(&va, &v1, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Variant_setScalar(&vb, &v2, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(UA_KeyValueMap_set(kvm, UA_QUALIFIEDNAME(0, "k"), &va),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_KeyValueMap_set(kvm, UA_QUALIFIEDNAME(0, "k"), &vb),
                     UA_STATUSCODE_GOOD);

    const UA_Variant *out = UA_KeyValueMap_get(kvm, UA_QUALIFIEDNAME(0, "k"));
    ck_assert(out->type == &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(*(UA_UInt32 *)out->data, 2);

    UA_KeyValueMap_delete(kvm);
}
END_TEST

int main(void) {
    Suite *s  = suite_create("Test KeyValueMap Utilities");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, CheckNullArgs);
    tcase_add_test(tc, CheckKVMContains);
    tcase_add_test(tc, CheckKVMCopy);
    tcase_add_test(tc, CheckKVMIsEmpty);
    tcase_add_test(tc, CheckKVMRemove);
    tcase_add_test(tc, CheckKVMCountMergedIntersecting);
    tcase_add_test(tc, CheckKVMCountMergedComplementary);
    tcase_add_test(tc, CheckKVMCountMergedCommon);
    tcase_add_test(tc, CheckKVMGetMissing);
    tcase_add_test(tc, CheckKVMGetScalarMissing);
    tcase_add_test(tc, CheckKVMRemoveMissing);
    tcase_add_test(tc, CheckKVMSetArray);
    tcase_add_test(tc, CheckKVMSetReplace);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


