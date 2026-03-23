/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Additional UA_Types coverage */

#include <open62541/types.h>
#include <open62541/util.h>
#include <open62541/types_generated.h>
#include <check.h>
#include <stdlib.h>
#include <string.h>

/* === StatusCode classification === */
START_TEST(statusCode_isUncertain) {
    /* Uncertain codes have 0x40xxxxxx pattern */
    ck_assert(UA_StatusCode_isUncertain(0x40000000));
    ck_assert(UA_StatusCode_isUncertain(UA_STATUSCODE_UNCERTAININITIALVALUE));
    ck_assert(!UA_StatusCode_isUncertain(UA_STATUSCODE_GOOD));
    ck_assert(!UA_StatusCode_isUncertain(UA_STATUSCODE_BADUNEXPECTEDERROR));
} END_TEST

START_TEST(statusCode_isBad) {
    ck_assert(UA_StatusCode_isBad(UA_STATUSCODE_BADUNEXPECTEDERROR));
    ck_assert(UA_StatusCode_isBad(UA_STATUSCODE_BADINTERNALERROR));
    ck_assert(!UA_StatusCode_isBad(UA_STATUSCODE_GOOD));
    ck_assert(!UA_StatusCode_isBad(0x40000000)); /* uncertain */
} END_TEST

/* === DateTime conversion === */
START_TEST(dateTime_toUnixTime) {
    UA_DateTime dt = UA_DateTime_now();
    UA_Int64 unixTs = UA_DateTime_toUnixTime(dt);
    /* Should be a reasonable Unix timestamp (> year 2000) */
    ck_assert_int_gt(unixTs, 946684800);
} END_TEST

START_TEST(dateTime_toStruct_and_back) {
    UA_DateTime now = UA_DateTime_now();
    UA_DateTimeStruct dts = UA_DateTime_toStruct(now);
    ck_assert_int_ge(dts.year, 2024);
    ck_assert_int_ge(dts.month, 1);
    ck_assert_int_le(dts.month, 12);
    ck_assert_int_ge(dts.day, 1);
    ck_assert_int_le(dts.day, 31);
    ck_assert_int_ge(dts.hour, 0);
    ck_assert_int_le(dts.hour, 23);
} END_TEST

/* === Variant array operations === */
START_TEST(variant_isArray) {
    UA_Variant v;
    UA_Variant_init(&v);

    /* Empty variant is not array */
    ck_assert(!UA_Variant_isArray(&v));

    /* Scalar is not array */
    UA_Int32 val = 42;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(!UA_Variant_isArray(&v));

    /* Array */
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArray(&v, arr, 3, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Variant_isArray(&v));
} END_TEST

START_TEST(variant_setRange) {
    /* Create a variant array */
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 *arr = (UA_Int32 *)UA_Array_new(5, &UA_TYPES[UA_TYPES_INT32]);
    for(int i = 0; i < 5; i++)
        arr[i] = i * 10;
    UA_Variant_setArray(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);

    /* Set a sub-range (copy semantics) */
    UA_NumericRange range;
    UA_NumericRangeDimension dim;
    dim.min = 1;
    dim.max = 3;
    range.dimensions = &dim;
    range.dimensionsSize = 1;

    UA_Int32 *newData = (UA_Int32 *)UA_Array_new(3, &UA_TYPES[UA_TYPES_INT32]);
    newData[0] = 100;
    newData[1] = 200;
    newData[2] = 300;

    UA_StatusCode res = UA_Variant_setRangeCopy(&v, newData, 3, range);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify */
    UA_Int32 *data = (UA_Int32 *)v.data;
    ck_assert_int_eq(data[0], 0);
    ck_assert_int_eq(data[1], 100);
    ck_assert_int_eq(data[2], 200);
    ck_assert_int_eq(data[3], 300);
    ck_assert_int_eq(data[4], 40);

    UA_Array_delete(newData, 3, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_clear(&v);
} END_TEST

/* === ExtensionObject === */
START_TEST(extensionObject_setValueCopy) {
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);

    UA_Int32 val = 42;
    UA_StatusCode res = UA_ExtensionObject_setValueCopy(&eo, &val,
        &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(eo.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert_int_eq(*(UA_Int32 *)eo.content.decoded.data, 42);
    UA_ExtensionObject_clear(&eo);
} END_TEST

START_TEST(extensionObject_setValueNoDelete) {
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);

    UA_Int32 val = 99;
    UA_ExtensionObject_setValueNoDelete(&eo, &val,
        &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(eo.encoding, UA_EXTENSIONOBJECT_DECODED_NODELETE);
    ck_assert_int_eq(*(UA_Int32 *)eo.content.decoded.data, 99);
    /* Don't clear - it's NODELETE */
} END_TEST

/* === DataType lookup === */
START_TEST(findDataType) {
    /* Find by NodeId */
    const UA_DataType *dt = UA_findDataType(
        &UA_TYPES[UA_TYPES_INT32].typeId);
    ck_assert_ptr_ne(dt, NULL);
    ck_assert(UA_NodeId_equal(&dt->typeId,
        &UA_TYPES[UA_TYPES_INT32].typeId));
} END_TEST

START_TEST(findDataType_unknown) {
    UA_NodeId unknownId = UA_NODEID_NUMERIC(99, 99999);
    const UA_DataType *dt = UA_findDataType(&unknownId);
    ck_assert_ptr_eq(dt, NULL);
} END_TEST

/* === Order/comparison functions === */
START_TEST(order_string) {
    UA_String a = UA_STRING("aaaa");
    UA_String b = UA_STRING("bbbb");
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_int_eq(o, UA_ORDER_LESS);

    o = UA_order(&b, &a, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_int_eq(o, UA_ORDER_MORE);

    o = UA_order(&a, &a, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_int_eq(o, UA_ORDER_EQ);
} END_TEST

START_TEST(order_int32) {
    UA_Int32 a = 10, b = 20;
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(o, UA_ORDER_LESS);

    o = UA_order(&b, &a, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(o, UA_ORDER_MORE);

    o = UA_order(&a, &a, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(o, UA_ORDER_EQ);
} END_TEST

START_TEST(order_nodeId) {
    UA_NodeId a = UA_NODEID_NUMERIC(0, 1);
    UA_NodeId b = UA_NODEID_NUMERIC(0, 2);
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

START_TEST(order_expandedNodeId) {
    UA_ExpandedNodeId a, b;
    UA_ExpandedNodeId_init(&a);
    UA_ExpandedNodeId_init(&b);
    a.nodeId = UA_NODEID_NUMERIC(0, 1);
    b.nodeId = UA_NODEID_NUMERIC(0, 2);
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

START_TEST(order_qualifiedName) {
    UA_QualifiedName a = UA_QUALIFIEDNAME(0, "Aaa");
    UA_QualifiedName b = UA_QUALIFIEDNAME(0, "Bbb");
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

START_TEST(order_localizedText) {
    UA_LocalizedText a = UA_LOCALIZEDTEXT("en", "Aaa");
    UA_LocalizedText b = UA_LOCALIZEDTEXT("en", "Bbb");
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

START_TEST(order_variant) {
    UA_Int32 va = 10, vb = 20;
    UA_Variant a, b;
    UA_Variant_setScalar(&a, &va, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&b, &vb, &UA_TYPES[UA_TYPES_INT32]);
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

/* === Array operations === */
START_TEST(array_copy) {
    UA_Int32 src[] = {1, 2, 3, 4, 5};
    void *dst = NULL;
    size_t dstSize = 0;
    UA_StatusCode res = UA_Array_copy(src, 5, &dst,
        &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Int32 *dstArr = (UA_Int32 *)dst;
    for(int i = 0; i < 5; i++)
        ck_assert_int_eq(dstArr[i], src[i]);
    UA_Array_delete(dst, 5, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

START_TEST(array_appendCopy) {
    UA_Int32 initial[] = {1, 2, 3};
    void *arr = NULL;
    size_t arrSize = 0;

    /* Copy initial array */
    UA_Array_copy(initial, 3, &arr, &UA_TYPES[UA_TYPES_INT32]);
    arrSize = 3;

    /* Append */
    UA_Int32 newVal = 4;
    UA_StatusCode res = UA_Array_appendCopy(&arr, &arrSize, &newVal,
        &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 4);
    ck_assert_int_eq(((UA_Int32 *)arr)[3], 4);

    UA_Array_delete(arr, arrSize, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

/* === DiagnosticInfo === */
START_TEST(diagnosticInfo_basic) {
    UA_DiagnosticInfo di;
    UA_DiagnosticInfo_init(&di);
    ck_assert(!di.hasSymbolicId);
    ck_assert(!di.hasNamespaceUri);
    ck_assert(!di.hasLocalizedText);
    ck_assert(!di.hasLocale);
    ck_assert(!di.hasAdditionalInfo);
    ck_assert(!di.hasInnerStatusCode);
    ck_assert(!di.hasInnerDiagnosticInfo);

    /* Copy */
    UA_DiagnosticInfo copy;
    UA_DiagnosticInfo_copy(&di, &copy);
    ck_assert(!copy.hasSymbolicId);
    UA_DiagnosticInfo_clear(&copy);
} END_TEST

START_TEST(diagnosticInfo_nested) {
    UA_DiagnosticInfo outer;
    UA_DiagnosticInfo_init(&outer);
    outer.hasSymbolicId = true;
    outer.symbolicId = 42;
    outer.hasInnerStatusCode = true;
    outer.innerStatusCode = UA_STATUSCODE_BADINTERNALERROR;
    outer.hasAdditionalInfo = true;
    outer.additionalInfo = UA_STRING_ALLOC("test additional info");

    /* Copy and verify */
    UA_DiagnosticInfo copy;
    UA_DiagnosticInfo_copy(&outer, &copy);
    ck_assert(copy.hasSymbolicId);
    ck_assert_int_eq(copy.symbolicId, 42);
    ck_assert(copy.hasInnerStatusCode);
    ck_assert_uint_eq(copy.innerStatusCode, UA_STATUSCODE_BADINTERNALERROR);

    UA_DiagnosticInfo_clear(&copy);
    UA_DiagnosticInfo_clear(&outer);
} END_TEST

/* === Type copy/comparison for complex types === */
START_TEST(dataValue_copy) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Int32 val = 42;
    UA_Variant_setScalarCopy(&dv.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    dv.hasSourceTimestamp = true;
    dv.sourceTimestamp = UA_DateTime_now();
    dv.hasServerTimestamp = true;
    dv.serverTimestamp = UA_DateTime_now();
    dv.hasSourcePicoseconds = true;
    dv.sourcePicoseconds = 100;
    dv.hasServerPicoseconds = true;
    dv.serverPicoseconds = 200;
    dv.hasStatus = true;
    dv.status = UA_STATUSCODE_GOOD;

    UA_DataValue copy;
    UA_DataValue_copy(&dv, &copy);
    ck_assert(copy.hasValue);
    ck_assert(copy.hasSourceTimestamp);
    ck_assert(copy.hasServerTimestamp);
    ck_assert(copy.hasSourcePicoseconds);
    ck_assert(copy.hasServerPicoseconds);
    ck_assert(copy.hasStatus);
    ck_assert_int_eq(*(UA_Int32 *)copy.value.data, 42);

    UA_DataValue_clear(&copy);
    UA_DataValue_clear(&dv);
} END_TEST

START_TEST(byteString_operations) {
    /* Create from allocating */
    UA_ByteString bs = UA_BYTESTRING_ALLOC("Hello World");
    ck_assert_uint_eq(bs.length, 11);

    /* Copy */
    UA_ByteString copy;
    UA_ByteString_copy(&bs, &copy);
    ck_assert_uint_eq(copy.length, 11);
    ck_assert(UA_ByteString_equal(&bs, &copy));

    /* Allocate buffer */
    UA_ByteString buf;
    UA_StatusCode res = UA_ByteString_allocBuffer(&buf, 100);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(buf.length, 100);
    UA_ByteString_clear(&buf);

    UA_ByteString_clear(&copy);
    UA_ByteString_clear(&bs);
} END_TEST

START_TEST(string_equal) {
    UA_String a = UA_STRING("hello");
    UA_String b = UA_STRING("hello");
    UA_String c = UA_STRING("world");
    UA_String empty = UA_STRING_NULL;

    ck_assert(UA_String_equal(&a, &b));
    ck_assert(!UA_String_equal(&a, &c));
    ck_assert(!UA_String_equal(&a, &empty));
    ck_assert(UA_String_equal(&empty, &empty));
} END_TEST

static Suite *testSuite_typesExtra(void) {
    TCase *tc_status = tcase_create("StatusCode");
    tcase_add_test(tc_status, statusCode_isUncertain);
    tcase_add_test(tc_status, statusCode_isBad);

    TCase *tc_datetime = tcase_create("DateTime");
    tcase_add_test(tc_datetime, dateTime_toUnixTime);
    tcase_add_test(tc_datetime, dateTime_toStruct_and_back);

    TCase *tc_variant = tcase_create("VariantExtra");
    tcase_add_test(tc_variant, variant_isArray);
    tcase_add_test(tc_variant, variant_setRange);

    TCase *tc_ext = tcase_create("ExtObj");
    tcase_add_test(tc_ext, extensionObject_setValueCopy);
    tcase_add_test(tc_ext, extensionObject_setValueNoDelete);

    TCase *tc_find = tcase_create("DataTypeLookup");
    tcase_add_test(tc_find, findDataType);
    tcase_add_test(tc_find, findDataType_unknown);

    TCase *tc_order = tcase_create("Order");
    tcase_add_test(tc_order, order_string);
    tcase_add_test(tc_order, order_int32);
    tcase_add_test(tc_order, order_nodeId);
    tcase_add_test(tc_order, order_expandedNodeId);
    tcase_add_test(tc_order, order_qualifiedName);
    tcase_add_test(tc_order, order_localizedText);
    tcase_add_test(tc_order, order_variant);

    TCase *tc_array = tcase_create("ArrayOps");
    tcase_add_test(tc_array, array_copy);
    tcase_add_test(tc_array, array_appendCopy);

    TCase *tc_diag = tcase_create("DiagInfo");
    tcase_add_test(tc_diag, diagnosticInfo_basic);
    tcase_add_test(tc_diag, diagnosticInfo_nested);

    TCase *tc_complex = tcase_create("ComplexCopy");
    tcase_add_test(tc_complex, dataValue_copy);
    tcase_add_test(tc_complex, byteString_operations);
    tcase_add_test(tc_complex, string_equal);

    Suite *s = suite_create("Types Extra Coverage");
    suite_add_tcase(s, tc_status);
    suite_add_tcase(s, tc_datetime);
    suite_add_tcase(s, tc_variant);
    suite_add_tcase(s, tc_ext);
    suite_add_tcase(s, tc_find);
    suite_add_tcase(s, tc_order);
    suite_add_tcase(s, tc_array);
    suite_add_tcase(s, tc_diag);
    suite_add_tcase(s, tc_complex);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_typesExtra();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
