/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Additional type and utility coverage tests */
#include <open62541/types.h>
#include <open62541/util.h>
#include <open62541/server.h>
#include <check.h>
#include <stdlib.h>
#include <string.h>

/* === NumericRange tests === */
START_TEST(numericRange_parse_simple) {
    UA_NumericRange nr;
    UA_String s = UA_STRING("0:3");
    UA_StatusCode res = UA_NumericRange_parse(&nr, s);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nr.dimensionsSize, 1);
    ck_assert_uint_eq(nr.dimensions[0].min, 0);
    ck_assert_uint_eq(nr.dimensions[0].max, 3);
    UA_free(nr.dimensions);
} END_TEST

START_TEST(numericRange_parse_multidim) {
    UA_NumericRange nr;
    UA_String s = UA_STRING("1:5,2:4");
    UA_StatusCode res = UA_NumericRange_parse(&nr, s);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nr.dimensionsSize, 2);
    ck_assert_uint_eq(nr.dimensions[0].min, 1);
    ck_assert_uint_eq(nr.dimensions[0].max, 5);
    ck_assert_uint_eq(nr.dimensions[1].min, 2);
    ck_assert_uint_eq(nr.dimensions[1].max, 4);
    UA_free(nr.dimensions);
} END_TEST

START_TEST(numericRange_parse_single) {
    UA_NumericRange nr;
    UA_String s = UA_STRING("7");
    UA_StatusCode res = UA_NumericRange_parse(&nr, s);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(nr.dimensionsSize, 1);
    ck_assert_uint_eq(nr.dimensions[0].min, 7);
    ck_assert_uint_eq(nr.dimensions[0].max, 7);
    UA_free(nr.dimensions);
} END_TEST

START_TEST(numericRange_parse_invalid) {
    UA_NumericRange nr;
    UA_String s = UA_STRING("abc");
    UA_StatusCode res = UA_NumericRange_parse(&nr, s);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(numericRange_parse_empty) {
    UA_NumericRange nr;
    UA_String s = UA_STRING("");
    UA_StatusCode res = UA_NumericRange_parse(&nr, s);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(numericRange_convenience) {
    UA_NumericRange nr = UA_NUMERICRANGE("2:5");
    ck_assert_uint_eq(nr.dimensionsSize, 1);
    ck_assert_uint_eq(nr.dimensions[0].min, 2);
    ck_assert_uint_eq(nr.dimensions[0].max, 5);
    UA_free(nr.dimensions);
} END_TEST

/* === DataType lookup === */
#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(findDataTypeByName_found) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "Int32");
    const UA_DataType *dt = UA_findDataTypeByName(&qn);
    ck_assert_ptr_ne(dt, NULL);
} END_TEST

START_TEST(findDataTypeByName_notFound) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "NonExistentType");
    const UA_DataType *dt = UA_findDataTypeByName(&qn);
    ck_assert_ptr_eq(dt, NULL);
} END_TEST

START_TEST(getStructMember_found) {
    const UA_DataType *dt = &UA_TYPES[UA_TYPES_READVALUEID];
    size_t offset = 0;
    const UA_DataType *memberType = NULL;
    UA_Boolean isArray = false;
    UA_Boolean found = UA_DataType_getStructMember(dt, "NodeId", &offset,
                                                    &memberType, &isArray);
    ck_assert(found);
    ck_assert_ptr_ne(memberType, NULL);
    ck_assert(!isArray);
} END_TEST

START_TEST(getStructMember_notFound) {
    const UA_DataType *dt = &UA_TYPES[UA_TYPES_READVALUEID];
    size_t offset = 0;
    const UA_DataType *memberType = NULL;
    UA_Boolean isArray = false;
    UA_Boolean found = UA_DataType_getStructMember(dt, "noSuchField", &offset,
                                                    &memberType, &isArray);
    ck_assert(!found);
} END_TEST

START_TEST(getStructMember_array) {
    /* BrowseResult has 'references' which is an array */
    const UA_DataType *dt = &UA_TYPES[UA_TYPES_BROWSERESULT];
    size_t offset = 0;
    const UA_DataType *memberType = NULL;
    UA_Boolean isArray = false;
    UA_Boolean found = UA_DataType_getStructMember(dt, "References", &offset,
                                                    &memberType, &isArray);
    ck_assert(found);
    ck_assert(isArray);
} END_TEST
#endif

/* === DataType_isNumeric === */
START_TEST(dataType_isNumeric) {
    ck_assert(UA_DataType_isNumeric(&UA_TYPES[UA_TYPES_INT32]));
    ck_assert(UA_DataType_isNumeric(&UA_TYPES[UA_TYPES_UINT32]));
    ck_assert(UA_DataType_isNumeric(&UA_TYPES[UA_TYPES_FLOAT]));
    ck_assert(UA_DataType_isNumeric(&UA_TYPES[UA_TYPES_DOUBLE]));
    ck_assert(UA_DataType_isNumeric(&UA_TYPES[UA_TYPES_INT16]));
    ck_assert(UA_DataType_isNumeric(&UA_TYPES[UA_TYPES_BYTE]));
    ck_assert(!UA_DataType_isNumeric(&UA_TYPES[UA_TYPES_STRING]));
    ck_assert(!UA_DataType_isNumeric(&UA_TYPES[UA_TYPES_BOOLEAN]));
    ck_assert(!UA_DataType_isNumeric(&UA_TYPES[UA_TYPES_NODEID]));
} END_TEST

/* === NamespaceMapping === */
START_TEST(nsMapping_local2Remote) {
    UA_NamespaceMapping nm;
    memset(&nm, 0, sizeof(nm));
    UA_UInt16 l2r[] = {0, 2, 1};
    nm.local2remote = l2r;
    nm.local2remoteSize = 3;

    ck_assert_uint_eq(UA_NamespaceMapping_local2Remote(&nm, 0), 0);
    ck_assert_uint_eq(UA_NamespaceMapping_local2Remote(&nm, 1), 2);
    ck_assert_uint_eq(UA_NamespaceMapping_local2Remote(&nm, 2), 1);
    /* Out of range */
    UA_UInt16 result = UA_NamespaceMapping_local2Remote(&nm, 10);
    ck_assert(result >= 0xFFF0 || result == 10);  /* UINT16_MAX - index or passthrough */
} END_TEST

START_TEST(nsMapping_remote2Local) {
    UA_NamespaceMapping nm;
    memset(&nm, 0, sizeof(nm));
    UA_UInt16 r2l[] = {0, 2, 1};
    nm.remote2local = r2l;
    nm.remote2localSize = 3;

    ck_assert_uint_eq(UA_NamespaceMapping_remote2Local(&nm, 0), 0);
    ck_assert_uint_eq(UA_NamespaceMapping_remote2Local(&nm, 1), 2);
    ck_assert_uint_eq(UA_NamespaceMapping_remote2Local(&nm, 2), 1);
} END_TEST

START_TEST(nsMapping_uri2Index) {
    UA_NamespaceMapping nm;
    memset(&nm, 0, sizeof(nm));
    UA_String uris[] = {UA_STRING_STATIC("urn:ns0"), UA_STRING_STATIC("urn:ns1"), UA_STRING_STATIC("urn:ns2")};
    nm.namespaceUris = uris;
    nm.namespaceUrisSize = 3;

    UA_UInt16 idx = 0;
    UA_StatusCode res = UA_NamespaceMapping_uri2Index(&nm, UA_STRING("urn:ns1"), &idx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(idx, 1);

    res = UA_NamespaceMapping_uri2Index(&nm, UA_STRING("urn:ns2"), &idx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(idx, 2);

    res = UA_NamespaceMapping_uri2Index(&nm, UA_STRING("urn:unknown"), &idx);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(nsMapping_index2Uri) {
    UA_NamespaceMapping nm;
    memset(&nm, 0, sizeof(nm));
    UA_String uris[] = {UA_STRING_STATIC("urn:ns0"), UA_STRING_STATIC("urn:ns1")};
    nm.namespaceUris = uris;
    nm.namespaceUrisSize = 2;

    UA_String uri = UA_STRING_NULL;
    UA_StatusCode res = UA_NamespaceMapping_index2Uri(&nm, 0, &uri);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&uri, &uris[0]));

    res = UA_NamespaceMapping_index2Uri(&nm, 1, &uri);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&uri, &uris[1]));

    res = UA_NamespaceMapping_index2Uri(&nm, 99, &uri);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* === Variant setRange === */
START_TEST(variant_setRange_test) {
    /* Create an array variant */
    UA_Int32 arr[] = {10, 20, 30, 40, 50};
    UA_Variant v;
    UA_Variant_setArray(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);

    /* Set sub-range */
    UA_Int32 newVals[] = {99, 88};
    UA_NumericRange nr = UA_NUMERICRANGE("1:2");
    UA_StatusCode res = UA_Variant_setRange(&v, newVals, 2, nr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(arr[1], 99);
    ck_assert_int_eq(arr[2], 88);
    UA_free(nr.dimensions);
} END_TEST

START_TEST(variant_setRangeCopy_test) {
    UA_Int32 arr[] = {1, 2, 3, 4, 5};
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setArrayCopy(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);

    UA_Int32 newVals[] = {77, 88, 66};
    UA_NumericRange nr = UA_NUMERICRANGE("1:3");
    UA_StatusCode res = UA_Variant_setRangeCopy(&v, newVals, 3, nr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((UA_Int32 *)v.data)[1], 77);
    ck_assert_int_eq(((UA_Int32 *)v.data)[2], 88);
    ck_assert_int_eq(((UA_Int32 *)v.data)[3], 66);
    UA_free(nr.dimensions);
    UA_Variant_clear(&v);
} END_TEST

/* === StatusCode name === */
#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(statusCode_name) {
    const char *name = UA_StatusCode_name(UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(name, NULL);
    ck_assert_str_eq(name, "Good");

    name = UA_StatusCode_name(UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_ptr_ne(name, NULL);
    ck_assert(strstr(name, "BadNodeIdUnknown") != NULL);

    name = UA_StatusCode_name(0xDEADBEEF);
    ck_assert_ptr_ne(name, NULL);  /* Should return "Unknown" or similar */
} END_TEST
#endif

/* === KeyValueMap === */
START_TEST(kvm_new_clear) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    ck_assert_ptr_ne(map, NULL);
    ck_assert_uint_eq(map->mapSize, 0);
    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_setScalar_getScalar) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    ck_assert_ptr_ne(map, NULL);

    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "TestKey");
    UA_Int32 val = 42;
    UA_StatusCode res = UA_KeyValueMap_setScalar(map, key, &val,
                                                  &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(map->mapSize, 1);

    const UA_Int32 *got = (const UA_Int32 *)
        UA_KeyValueMap_getScalar(map, key, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_ptr_ne(got, NULL);
    ck_assert_int_eq(*got, 42);

    /* Update existing key */
    UA_Int32 val2 = 99;
    res = UA_KeyValueMap_setScalar(map, key, &val2, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(map->mapSize, 1);
    got = (const UA_Int32 *)
        UA_KeyValueMap_getScalar(map, key, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*got, 99);

    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_contains_remove) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    ck_assert_ptr_ne(map, NULL);

    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "MyKey");
    UA_Int32 val = 7;
    UA_KeyValueMap_setScalar(map, key, &val, &UA_TYPES[UA_TYPES_INT32]);

    ck_assert(UA_KeyValueMap_contains(map, key));
    UA_QualifiedName key2 = UA_QUALIFIEDNAME(0, "NoKey");
    ck_assert(!UA_KeyValueMap_contains(map, key2));

    UA_StatusCode res = UA_KeyValueMap_remove(map, key);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(map->mapSize, 0);
    ck_assert(!UA_KeyValueMap_contains(map, key));

    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_set_get_variant) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    ck_assert_ptr_ne(map, NULL);

    /* Set an array via UA_KeyValueMap_set with a Variant */
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "ArrKey");
    UA_Variant v;
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArrayCopy(&v, arr, 3, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode res = UA_KeyValueMap_set(map, key, &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);

    const UA_Variant *got = UA_KeyValueMap_get(map, key);
    ck_assert_ptr_ne(got, NULL);
    ck_assert_uint_eq(got->arrayLength, 3);
    const UA_Int32 *data = (const UA_Int32 *)got->data;
    ck_assert_int_eq(data[0], 1);
    ck_assert_int_eq(data[2], 3);

    /* Copy the map */
    UA_KeyValueMap dst;
    memset(&dst, 0, sizeof(dst));
    res = UA_KeyValueMap_copy(map, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.mapSize, 1);
    UA_KeyValueMap_clear(&dst);

    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_setScalarShallow) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    ck_assert_ptr_ne(map, NULL);

    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "ShallowKey");
    UA_Int32 val = 123;
    UA_StatusCode res = UA_KeyValueMap_setScalarShallow(map, key, &val,
                                                        &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(map->mapSize, 1);

    /* Don't clear because shallow — manually reset data to avoid freeing stack */
    map->map[0].value.data = NULL;
    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_merge) {
    UA_KeyValueMap *map1 = UA_KeyValueMap_new();
    UA_KeyValueMap *map2 = UA_KeyValueMap_new();

    UA_QualifiedName key1 = UA_QUALIFIEDNAME(0, "k1");
    UA_QualifiedName key2 = UA_QUALIFIEDNAME(0, "k2");
    UA_Int32 v1 = 1, v2 = 2;
    UA_KeyValueMap_setScalar(map1, key1, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_KeyValueMap_setScalar(map2, key2, &v2, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode res = UA_KeyValueMap_merge(map1, map2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(map1->mapSize, 2);
    ck_assert(UA_KeyValueMap_contains(map1, key2));

    UA_KeyValueMap_delete(map1);
    UA_KeyValueMap_delete(map2);
} END_TEST

/* === Array operations === */
START_TEST(array_resize) {
    void *arr = NULL;
    size_t size = 0;

    /* Grow from 0 */
    UA_StatusCode res = UA_Array_resize(&arr, &size, 3,
                                         &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 3);
    ((UA_Int32*)arr)[0] = 10; ((UA_Int32*)arr)[1] = 20; ((UA_Int32*)arr)[2] = 30;

    /* Grow */
    res = UA_Array_resize(&arr, &size, 5, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 5);
    ck_assert_int_eq(((UA_Int32*)arr)[0], 10);
    ck_assert_int_eq(((UA_Int32*)arr)[2], 30);

    /* Shrink */
    res = UA_Array_resize(&arr, &size, 2, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 2);
    ck_assert_int_eq(((UA_Int32*)arr)[0], 10);
    ck_assert_int_eq(((UA_Int32*)arr)[1], 20);

    /* Shrink to 0 */
    res = UA_Array_resize(&arr, &size, 0, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 0);
} END_TEST

/* === DataType copy === */
START_TEST(datatype_copy_test) {
    UA_DataType src = UA_TYPES[UA_TYPES_INT32];
    UA_DataType dst;
    UA_StatusCode res = UA_DataType_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.memSize, src.memSize);
    ck_assert_uint_eq(dst.typeKind, src.typeKind);
    UA_DataType_clear(&dst);
} END_TEST

static Suite *testSuite_Types2(void) {
    TCase *tc_nr = tcase_create("NumericRange");
    tcase_add_test(tc_nr, numericRange_parse_simple);
    tcase_add_test(tc_nr, numericRange_parse_multidim);
    tcase_add_test(tc_nr, numericRange_parse_single);
    tcase_add_test(tc_nr, numericRange_parse_invalid);
    tcase_add_test(tc_nr, numericRange_parse_empty);
    tcase_add_test(tc_nr, numericRange_convenience);

    TCase *tc_dt = tcase_create("DataTypeMisc");
#ifdef UA_ENABLE_TYPEDESCRIPTION
    tcase_add_test(tc_dt, findDataTypeByName_found);
    tcase_add_test(tc_dt, findDataTypeByName_notFound);
    tcase_add_test(tc_dt, getStructMember_found);
    tcase_add_test(tc_dt, getStructMember_notFound);
    tcase_add_test(tc_dt, getStructMember_array);
    tcase_add_test(tc_dt, statusCode_name);
#endif
    tcase_add_test(tc_dt, dataType_isNumeric);
    tcase_add_test(tc_dt, datatype_copy_test);

    TCase *tc_ns = tcase_create("NamespaceMapping");
    tcase_add_test(tc_ns, nsMapping_local2Remote);
    tcase_add_test(tc_ns, nsMapping_remote2Local);
    tcase_add_test(tc_ns, nsMapping_uri2Index);
    tcase_add_test(tc_ns, nsMapping_index2Uri);

    TCase *tc_var = tcase_create("VariantRange");
    tcase_add_test(tc_var, variant_setRange_test);
    tcase_add_test(tc_var, variant_setRangeCopy_test);
    tcase_add_test(tc_var, array_resize);

    TCase *tc_kvm = tcase_create("KeyValueMap");
    tcase_add_test(tc_kvm, kvm_new_clear);
    tcase_add_test(tc_kvm, kvm_setScalar_getScalar);
    tcase_add_test(tc_kvm, kvm_contains_remove);
    tcase_add_test(tc_kvm, kvm_set_get_variant);
    tcase_add_test(tc_kvm, kvm_setScalarShallow);
    tcase_add_test(tc_kvm, kvm_merge);

    Suite *s = suite_create("Types Extra 2");
    suite_add_tcase(s, tc_nr);
    suite_add_tcase(s, tc_dt);
    suite_add_tcase(s, tc_ns);
    suite_add_tcase(s, tc_var);
    suite_add_tcase(s, tc_kvm);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_Types2();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
