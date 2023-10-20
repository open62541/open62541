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

int main(void) {
    Suite *s  = suite_create("Test KeyValueMap Utilities");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, CheckNullArgs);
    tcase_add_test(tc, CheckKVMContains);
    tcase_add_test(tc, CheckKVMCopy);
    tcase_add_test(tc, CheckKVMCountMergedIntersecting);
    tcase_add_test(tc, CheckKVMCountMergedComplementary);
    tcase_add_test(tc, CheckKVMCountMergedCommon);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


