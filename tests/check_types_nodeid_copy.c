/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Extended tests for ua_types.c coverage */
#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <check.h>
#include <math.h>

/* === Variant operations === */
START_TEST(variant_setScalar_null) {
    UA_Variant v;
    UA_Variant_init(&v);
    ck_assert(!UA_Variant_isScalar(&v));
    ck_assert(UA_Variant_isEmpty(&v));
} END_TEST

START_TEST(variant_setScalar_int) {
    UA_Variant v;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Variant_isScalar(&v));
    ck_assert(!UA_Variant_isEmpty(&v));
    ck_assert_int_eq(*(UA_Int32 *)v.data, 42);
} END_TEST

START_TEST(variant_setScalarCopy) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_String str = UA_STRING("test string");
    UA_StatusCode res = UA_Variant_setScalarCopy(&v, &str, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_isScalar(&v));
    ck_assert(UA_String_equal((UA_String *)v.data, &str));
    UA_Variant_clear(&v);
} END_TEST

START_TEST(variant_setArray) {
    UA_Variant v;
    UA_Int32 arr[] = {1, 2, 3, 4, 5};
    UA_Variant_setArray(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(!UA_Variant_isScalar(&v));
    ck_assert(!UA_Variant_isEmpty(&v));
    ck_assert_uint_eq(v.arrayLength, 5);
} END_TEST

START_TEST(variant_setArrayCopy) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 arr[] = {10, 20, 30};
    UA_StatusCode res = UA_Variant_setArrayCopy(&v, arr, 3, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v.arrayLength, 3);
    ck_assert_int_eq(((UA_Int32 *)v.data)[0], 10);
    ck_assert_int_eq(((UA_Int32 *)v.data)[2], 30);
    UA_Variant_clear(&v);
} END_TEST

START_TEST(variant_hasScalarType) {
    UA_Variant v;
    UA_Int32 val = 5;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert(!UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_UINT32]));
} END_TEST

START_TEST(variant_hasArrayType) {
    UA_Variant v;
    UA_Int32 arr[] = {1, 2};
    UA_Variant_setArray(&v, arr, 2, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Variant_hasArrayType(&v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert(!UA_Variant_hasArrayType(&v, &UA_TYPES[UA_TYPES_DOUBLE]));
} END_TEST

/* === NodeId operations === */
START_TEST(nodeId_numeric) {
    UA_NodeId id = UA_NODEID_NUMERIC(2, 1234);
    ck_assert_uint_eq(id.namespaceIndex, 2);
    ck_assert(id.identifierType == UA_NODEIDTYPE_NUMERIC);
    ck_assert_uint_eq(id.identifier.numeric, 1234);
    ck_assert(!UA_NodeId_isNull(&id));
} END_TEST

START_TEST(nodeId_string) {
    UA_NodeId id = UA_NODEID_STRING(1, "hello");
    ck_assert(id.identifierType == UA_NODEIDTYPE_STRING);
    ck_assert(!UA_NodeId_isNull(&id));
} END_TEST

START_TEST(nodeId_guid) {
    UA_Guid g = UA_Guid_random();
    UA_NodeId id = UA_NODEID_GUID(3, g);
    ck_assert(id.identifierType == UA_NODEIDTYPE_GUID);
    ck_assert(!UA_NodeId_isNull(&id));
} END_TEST

START_TEST(nodeId_null) {
    UA_NodeId id = UA_NODEID_NULL;
    ck_assert(UA_NodeId_isNull(&id));
} END_TEST

START_TEST(nodeId_equal) {
    UA_NodeId a = UA_NODEID_NUMERIC(0, 100);
    UA_NodeId b = UA_NODEID_NUMERIC(0, 100);
    UA_NodeId c = UA_NODEID_NUMERIC(0, 200);
    ck_assert(UA_NodeId_equal(&a, &b));
    ck_assert(!UA_NodeId_equal(&a, &c));
} END_TEST

START_TEST(nodeId_copy) {
    UA_NodeId src = UA_NODEID_STRING_ALLOC(1, "testcopy");
    UA_NodeId dst;
    UA_StatusCode res = UA_NodeId_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src, &dst));
    UA_NodeId_clear(&src);
    UA_NodeId_clear(&dst);
} END_TEST

START_TEST(nodeId_hash) {
    UA_NodeId id = UA_NODEID_NUMERIC(0, 255);
    UA_UInt32 h = UA_NodeId_hash(&id);
    ck_assert_uint_gt(h, 0);
} END_TEST

START_TEST(nodeId_order) {
    UA_NodeId a = UA_NODEID_NUMERIC(0, 1);
    UA_NodeId b = UA_NODEID_NUMERIC(0, 2);
    ck_assert_int_eq(UA_NodeId_order(&a, &a), UA_ORDER_EQ);
    ck_assert_int_eq(UA_NodeId_order(&a, &b), UA_ORDER_LESS);
    ck_assert_int_eq(UA_NodeId_order(&b, &a), UA_ORDER_MORE);
} END_TEST

/* === ExpandedNodeId === */
START_TEST(expandedNodeId_basic) {
    UA_ExpandedNodeId en = UA_EXPANDEDNODEID_NUMERIC(0, 100);
    ck_assert(en.serverIndex == 0);
    ck_assert(UA_ExpandedNodeId_isLocal(&en));

    /* Non-local: set serverIndex != 0 */
    en.serverIndex = 1;
    ck_assert(!UA_ExpandedNodeId_isLocal(&en));

    UA_ExpandedNodeId en2 = UA_EXPANDEDNODEID_STRING(1, "test");
    ck_assert(UA_ExpandedNodeId_isLocal(&en2));
} END_TEST

START_TEST(expandedNodeId_copy_equal) {
    UA_ExpandedNodeId src = UA_EXPANDEDNODEID_NUMERIC(0, 42);
    UA_ExpandedNodeId dst;
    UA_StatusCode res = UA_ExpandedNodeId_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_ExpandedNodeId_equal(&src, &dst));
    UA_ExpandedNodeId_clear(&dst);
} END_TEST

START_TEST(expandedNodeId_order) {
    UA_ExpandedNodeId a = UA_EXPANDEDNODEID_NUMERIC(0, 1);
    UA_ExpandedNodeId b = UA_EXPANDEDNODEID_NUMERIC(0, 2);
    ck_assert_int_eq(UA_ExpandedNodeId_order(&a, &b), UA_ORDER_LESS);
    ck_assert_int_eq(UA_ExpandedNodeId_order(&a, &a), UA_ORDER_EQ);
} END_TEST

START_TEST(expandedNodeId_hash) {
    UA_ExpandedNodeId en = UA_EXPANDEDNODEID_NUMERIC(0, 255);
    UA_UInt32 h = UA_ExpandedNodeId_hash(&en);
    ck_assert_uint_gt(h, 0);
} END_TEST

/* === QualifiedName === */
START_TEST(qualifiedName_basic) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "testName");
    ck_assert_uint_eq(qn.namespaceIndex, 0);
    ck_assert(!UA_QualifiedName_isNull(&qn));
} END_TEST

START_TEST(qualifiedName_null) {
    UA_QualifiedName qn;
    UA_QualifiedName_init(&qn);
    ck_assert(UA_QualifiedName_isNull(&qn));
} END_TEST

START_TEST(qualifiedName_equal) {
    UA_QualifiedName a = UA_QUALIFIEDNAME(0, "same");
    UA_QualifiedName b = UA_QUALIFIEDNAME(0, "same");
    UA_QualifiedName c = UA_QUALIFIEDNAME(0, "diff");
    ck_assert(UA_QualifiedName_equal(&a, &b));
    ck_assert(!UA_QualifiedName_equal(&a, &c));
} END_TEST

START_TEST(qualifiedName_hash) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "hashMe");
    UA_UInt32 h = UA_QualifiedName_hash(&qn);
    ck_assert_uint_gt(h, 0);
} END_TEST

/* === LocalizedText === */
START_TEST(localizedText_basic) {
    UA_LocalizedText lt = UA_LOCALIZEDTEXT("en", "Hello");
    ck_assert(lt.locale.length > 0);
    ck_assert(lt.text.length > 0);
} END_TEST

START_TEST(localizedText_copy) {
    UA_LocalizedText src = UA_LOCALIZEDTEXT("en", "Hello World");
    UA_LocalizedText dst;
    UA_StatusCode res = UA_LocalizedText_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.text, &dst.text));
    ck_assert(UA_String_equal(&src.locale, &dst.locale));
    UA_LocalizedText_clear(&dst);
} END_TEST

/* === DateTime === */
START_TEST(dateTime_now) {
    UA_DateTime now = UA_DateTime_now();
    ck_assert(now > 0);
} END_TEST

START_TEST(dateTime_nowMonotonic) {
    UA_DateTime m1 = UA_DateTime_nowMonotonic();
    UA_DateTime m2 = UA_DateTime_nowMonotonic();
    ck_assert(m2 >= m1);
} END_TEST

START_TEST(dateTime_toStruct) {
    UA_DateTime dt = UA_DateTime_now();
    UA_DateTimeStruct dts = UA_DateTime_toStruct(dt);
    ck_assert(dts.year > 2000);
    ck_assert(dts.month >= 1 && dts.month <= 12);
    ck_assert(dts.day >= 1 && dts.day <= 31);
} END_TEST

/* === StatusCode operations === */
START_TEST(statusCode_name) {
    const char *name = UA_StatusCode_name(UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(name, NULL);
    ck_assert_str_eq(name, "Good");
} END_TEST

START_TEST(statusCode_name_bad) {
    const char *name = UA_StatusCode_name(UA_STATUSCODE_BADUNEXPECTEDERROR);
    ck_assert_ptr_ne(name, NULL);
    ck_assert(strlen(name) > 0);
} END_TEST

/* === DataType operations === */
START_TEST(dataType_findByNodeId) {
    UA_NodeId intId = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    const UA_DataType *dt = UA_findDataType(&intId);
    ck_assert_ptr_ne(dt, NULL);
} END_TEST

START_TEST(dataType_findByNodeId_notFound) {
    UA_NodeId fakeId = UA_NODEID_NUMERIC(0, 999999);
    const UA_DataType *dt = UA_findDataType(&fakeId);
    ck_assert_ptr_eq(dt, NULL);
} END_TEST

/* === Array operations === */
START_TEST(array_new_delete) {
    size_t size = 10;
    void *arr = UA_Array_new(size, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_ptr_ne(arr, NULL);
    UA_Array_delete(arr, size, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

START_TEST(array_copy) {
    UA_Int32 src[] = {1, 2, 3, 4, 5};
    UA_Int32 *dst = NULL;
    size_t dstSize = 0;
    UA_StatusCode res = UA_Array_copy(src, 5, (void **)&dst, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(dst, NULL);
    for(int i = 0; i < 5; i++)
        ck_assert_int_eq(dst[i], src[i]);
    UA_Array_delete(dst, 5, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

START_TEST(array_appendCopy) {
    UA_Int32 *arr = NULL;
    size_t arrSize = 0;

    UA_Int32 val1 = 10;
    UA_StatusCode res = UA_Array_appendCopy((void **)&arr, &arrSize, &val1,
                                            &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 1);
    ck_assert_int_eq(arr[0], 10);

    UA_Int32 val2 = 20;
    res = UA_Array_appendCopy((void **)&arr, &arrSize, &val2,
                              &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 2);

    UA_Array_delete(arr, arrSize, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

/* === String operations === */
START_TEST(string_equal) {
    UA_String a = UA_STRING("hello");
    UA_String b = UA_STRING("hello");
    UA_String c = UA_STRING("world");
    ck_assert(UA_String_equal(&a, &b));
    ck_assert(!UA_String_equal(&a, &c));
} END_TEST

START_TEST(string_copy) {
    UA_String src = UA_STRING("copy me");
    UA_String dst;
    UA_StatusCode res = UA_String_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src, &dst));
    UA_String_clear(&dst);
} END_TEST

START_TEST(string_null) {
    UA_String s = UA_STRING_NULL;
    ck_assert_uint_eq(s.length, 0);
    ck_assert_ptr_eq(s.data, NULL);
} END_TEST

/* === Numeric range parsing === */
START_TEST(numericRange_parse) {
    UA_NumericRange range;
    UA_StatusCode res = UA_NumericRange_parse(&range, UA_STRING("1:5"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensionsSize, 1);
    ck_assert_uint_eq(range.dimensions[0].min, 1);
    ck_assert_uint_eq(range.dimensions[0].max, 5);
    UA_free(range.dimensions);
} END_TEST

START_TEST(numericRange_parse_multi) {
    UA_NumericRange range;
    UA_StatusCode res = UA_NumericRange_parse(&range, UA_STRING("0:3,0:2"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensionsSize, 2);
    UA_free(range.dimensions);
} END_TEST

START_TEST(numericRange_parse_invalid) {
    UA_NumericRange range;
    UA_StatusCode res = UA_NumericRange_parse(&range, UA_STRING("abc"));
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Guid operations === */
START_TEST(guid_equal) {
    UA_Guid a = UA_Guid_random();
    UA_Guid b = a;
    ck_assert(UA_Guid_equal(&a, &b));
} END_TEST

START_TEST(guid_order) {
    UA_Guid a, b;
    memset(&a, 0, sizeof(a));
    memset(&b, 0xFF, sizeof(b));
    /* Just test that memcmp-based comparison works */
    ck_assert(!UA_Guid_equal(&a, &b));
    ck_assert(UA_Guid_equal(&a, &a));
} END_TEST

/* === Copy/clear for complex types === */
START_TEST(copy_extensionObject) {
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);

    UA_ExtensionObject copy;
    UA_StatusCode res = UA_ExtensionObject_copy(&eo, &copy);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ExtensionObject_clear(&copy);
} END_TEST

START_TEST(copy_diagnosticInfo) {
    UA_DiagnosticInfo di;
    UA_DiagnosticInfo_init(&di);
    di.hasAdditionalInfo = true;
    di.additionalInfo = UA_STRING_ALLOC("test info");
    di.hasSymbolicId = true;
    di.symbolicId = 42;

    UA_DiagnosticInfo copy;
    UA_StatusCode res = UA_DiagnosticInfo_copy(&di, &copy);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(copy.hasAdditionalInfo);
    ck_assert_int_eq(copy.symbolicId, 42);
    UA_DiagnosticInfo_clear(&di);
    UA_DiagnosticInfo_clear(&copy);
} END_TEST

START_TEST(copy_dataValue) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Int32 val = 77;
    UA_Variant_setScalar(&dv.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    dv.hasSourceTimestamp = true;
    dv.sourceTimestamp = UA_DateTime_now();
    dv.hasServerTimestamp = true;
    dv.serverTimestamp = UA_DateTime_now();
    dv.hasStatus = true;
    dv.status = UA_STATUSCODE_GOOD;

    UA_DataValue copy;
    UA_StatusCode res = UA_DataValue_copy(&dv, &copy);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(copy.hasValue);
    ck_assert(copy.hasSourceTimestamp);
    ck_assert(copy.hasServerTimestamp);
    ck_assert(copy.hasStatus);
    UA_DataValue_clear(&copy);
} END_TEST

static Suite *testSuite_types_extended(void) {
    TCase *tc_variant = tcase_create("Variant");
    tcase_add_test(tc_variant, variant_setScalar_null);
    tcase_add_test(tc_variant, variant_setScalar_int);
    tcase_add_test(tc_variant, variant_setScalarCopy);
    tcase_add_test(tc_variant, variant_setArray);
    tcase_add_test(tc_variant, variant_setArrayCopy);
    tcase_add_test(tc_variant, variant_hasScalarType);
    tcase_add_test(tc_variant, variant_hasArrayType);

    TCase *tc_nodeid = tcase_create("NodeId");
    tcase_add_test(tc_nodeid, nodeId_numeric);
    tcase_add_test(tc_nodeid, nodeId_string);
    tcase_add_test(tc_nodeid, nodeId_guid);
    tcase_add_test(tc_nodeid, nodeId_null);
    tcase_add_test(tc_nodeid, nodeId_equal);
    tcase_add_test(tc_nodeid, nodeId_copy);
    tcase_add_test(tc_nodeid, nodeId_hash);
    tcase_add_test(tc_nodeid, nodeId_order);

    TCase *tc_expanded = tcase_create("ExpandedNodeId");
    tcase_add_test(tc_expanded, expandedNodeId_basic);
    tcase_add_test(tc_expanded, expandedNodeId_copy_equal);
    tcase_add_test(tc_expanded, expandedNodeId_order);
    tcase_add_test(tc_expanded, expandedNodeId_hash);

    TCase *tc_qn = tcase_create("QualifiedName");
    tcase_add_test(tc_qn, qualifiedName_basic);
    tcase_add_test(tc_qn, qualifiedName_null);
    tcase_add_test(tc_qn, qualifiedName_equal);
    tcase_add_test(tc_qn, qualifiedName_hash);

    TCase *tc_lt = tcase_create("LocalizedText");
    tcase_add_test(tc_lt, localizedText_basic);
    tcase_add_test(tc_lt, localizedText_copy);

    TCase *tc_dt = tcase_create("DateTime");
    tcase_add_test(tc_dt, dateTime_now);
    tcase_add_test(tc_dt, dateTime_nowMonotonic);
    tcase_add_test(tc_dt, dateTime_toStruct);

    TCase *tc_sc = tcase_create("StatusCode");
    tcase_add_test(tc_sc, statusCode_name);
    tcase_add_test(tc_sc, statusCode_name_bad);

    TCase *tc_datatype = tcase_create("DataType");
    tcase_add_test(tc_datatype, dataType_findByNodeId);
    tcase_add_test(tc_datatype, dataType_findByNodeId_notFound);

    TCase *tc_array = tcase_create("Array");
    tcase_add_test(tc_array, array_new_delete);
    tcase_add_test(tc_array, array_copy);
    tcase_add_test(tc_array, array_appendCopy);

    TCase *tc_string = tcase_create("String");
    tcase_add_test(tc_string, string_equal);
    tcase_add_test(tc_string, string_copy);
    tcase_add_test(tc_string, string_null);

    TCase *tc_range = tcase_create("NumericRange");
    tcase_add_test(tc_range, numericRange_parse);
    tcase_add_test(tc_range, numericRange_parse_multi);
    tcase_add_test(tc_range, numericRange_parse_invalid);

    TCase *tc_guid = tcase_create("Guid");
    tcase_add_test(tc_guid, guid_equal);
    tcase_add_test(tc_guid, guid_order);

    TCase *tc_complex = tcase_create("ComplexTypes");
    tcase_add_test(tc_complex, copy_extensionObject);
    tcase_add_test(tc_complex, copy_diagnosticInfo);
    tcase_add_test(tc_complex, copy_dataValue);

    Suite *s = suite_create("Extended Types Tests");
    suite_add_tcase(s, tc_variant);
    suite_add_tcase(s, tc_nodeid);
    suite_add_tcase(s, tc_expanded);
    suite_add_tcase(s, tc_qn);
    suite_add_tcase(s, tc_lt);
    suite_add_tcase(s, tc_dt);
    suite_add_tcase(s, tc_sc);
    suite_add_tcase(s, tc_datatype);
    suite_add_tcase(s, tc_array);
    suite_add_tcase(s, tc_string);
    suite_add_tcase(s, tc_range);
    suite_add_tcase(s, tc_guid);
    suite_add_tcase(s, tc_complex);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_types_extended();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
