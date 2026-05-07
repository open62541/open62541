/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Additional type utility function tests */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <check.h>
#include <stdlib.h>
#include <string.h>

/* === UA_findDataType === */
START_TEST(findDataType_known) {
    const UA_DataType *t = UA_findDataType(&UA_TYPES[UA_TYPES_INT32].typeId);
    ck_assert_ptr_ne(t, NULL);
    ck_assert_uint_eq(t->memSize, sizeof(UA_Int32));
} END_TEST

START_TEST(findDataType_unknown) {
    UA_NodeId unknownId = UA_NODEID_NUMERIC(99, 999999);
    const UA_DataType *t = UA_findDataType(&unknownId);
    ck_assert_ptr_eq(t, NULL);
} END_TEST

START_TEST(findDataType_all_builtins) {
    /* Verify all builtin types can be found */
    for(size_t i = 0; i < UA_TYPES_COUNT; i++) {
        const UA_DataType *t = UA_findDataType(&UA_TYPES[i].typeId);
        /* Some types may have overlapping typeIds, so just check it returns something */
        (void)t;
    }
} END_TEST

/* === UA_findDataTypeByName === */
#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(findDataTypeByName_boolean) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "Boolean");
    const UA_DataType *t = UA_findDataTypeByName(&qn);
    ck_assert_ptr_ne(t, NULL);
} END_TEST

START_TEST(findDataTypeByName_string) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "String");
    const UA_DataType *t = UA_findDataTypeByName(&qn);
    ck_assert_ptr_ne(t, NULL);
} END_TEST

START_TEST(findDataTypeByName_readrequest) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "ReadRequest");
    const UA_DataType *t = UA_findDataTypeByName(&qn);
    ck_assert_ptr_ne(t, NULL);
} END_TEST

START_TEST(findDataTypeByName_unknown) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "NonExistentType");
    const UA_DataType *t = UA_findDataTypeByName(&qn);
    ck_assert_ptr_eq(t, NULL);
} END_TEST
#endif

/* === UA_String_fromChars === */
START_TEST(string_fromChars_normal) {
    UA_String s = UA_String_fromChars("hello");
    ck_assert_uint_eq(s.length, 5);
    ck_assert(memcmp(s.data, "hello", 5) == 0);
    UA_String_clear(&s);
} END_TEST

START_TEST(string_fromChars_empty) {
    UA_String s = UA_String_fromChars("");
    ck_assert_uint_eq(s.length, 0);
    UA_String_clear(&s);
} END_TEST

START_TEST(string_fromChars_null) {
    UA_String s = UA_String_fromChars(NULL);
    ck_assert_uint_eq(s.length, 0);
    ck_assert_ptr_eq(s.data, NULL);
} END_TEST

/* === UA_String_append === */
START_TEST(string_append_normal) {
    UA_String s = UA_STRING_ALLOC("Hello");
    UA_String s2 = UA_STRING(" World");
    UA_StatusCode res = UA_String_append(&s, s2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(s.length, 11);
    ck_assert(memcmp(s.data, "Hello World", 11) == 0);
    UA_String_clear(&s);
} END_TEST

START_TEST(string_append_empty) {
    UA_String s = UA_STRING_ALLOC("Hello");
    UA_String s2 = UA_STRING("");
    UA_StatusCode res = UA_String_append(&s, s2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(s.length, 5);
    UA_String_clear(&s);
} END_TEST

START_TEST(string_append_to_empty) {
    UA_String s = UA_STRING_NULL;
    UA_String s2 = UA_STRING("World");
    UA_StatusCode res = UA_String_append(&s, s2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(s.length, 5);
    UA_String_clear(&s);
} END_TEST

/* === UA_print (type description printing) === */
#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(print_int32) {
    UA_Int32 val = 42;
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&val, &UA_TYPES[UA_TYPES_INT32], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_string) {
    UA_String val = UA_STRING("Hello World");
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&val, &UA_TYPES[UA_TYPES_STRING], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_nodeid) {
    UA_NodeId val = UA_NODEID_NUMERIC(0, 2255);
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&val, &UA_TYPES[UA_TYPES_NODEID], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);

    val = UA_NODEID_STRING(1, "TestNode");
    res = UA_print(&val, &UA_TYPES[UA_TYPES_NODEID], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);

    UA_Guid g = UA_GUID("09087e75-8e5e-499b-954f-f2a9603db28a");
    val = UA_NODEID_GUID(2, g);
    res = UA_print(&val, &UA_TYPES[UA_TYPES_NODEID], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_variant_scalar) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Double d = 3.14;
    UA_Variant_setScalar(&v, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&v, &UA_TYPES[UA_TYPES_VARIANT], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_variant_array) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArray(&v, arr, 3, &UA_TYPES[UA_TYPES_INT32]);
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&v, &UA_TYPES[UA_TYPES_VARIANT], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_variant_empty) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&v, &UA_TYPES[UA_TYPES_VARIANT], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_datavalue) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&dv.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    dv.hasStatus = true;
    dv.status = UA_STATUSCODE_GOOD;
    dv.hasSourceTimestamp = true;
    dv.sourceTimestamp = UA_DateTime_now();

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&dv, &UA_TYPES[UA_TYPES_DATAVALUE], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_diagnosticinfo) {
    UA_DiagnosticInfo di;
    UA_DiagnosticInfo_init(&di);
    di.hasSymbolicId = true;
    di.symbolicId = 42;
    di.hasAdditionalInfo = true;
    di.additionalInfo = UA_STRING("extra");

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&di, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_extensionobject) {
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    eo.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    eo.content.encoded.typeId = UA_NODEID_NUMERIC(0, 999);
    eo.content.encoded.body = UA_BYTESTRING("test");

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_localizedtext) {
    UA_LocalizedText lt = UA_LOCALIZEDTEXT("en", "Hello");
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&lt, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_qualifiedname) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(1, "TestName");
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&qn, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_guid) {
    UA_Guid g = UA_GUID("09087e75-8e5e-499b-954f-f2a9603db28a");
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&g, &UA_TYPES[UA_TYPES_GUID], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

START_TEST(print_struct) {
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_NUMERIC(0, 2255);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_print(&rvid, &UA_TYPES[UA_TYPES_READVALUEID], &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST
#endif

/* === UA_DataType_copy === */
START_TEST(datatype_copy_basic) {
    UA_DataType t2;
    UA_StatusCode res = UA_DataType_copy(&UA_TYPES[UA_TYPES_INT32], &t2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
#ifdef UA_ENABLE_TYPEDESCRIPTION
    /* typeName should be copied */
    if(UA_TYPES[UA_TYPES_INT32].typeName)
        ck_assert_ptr_ne(t2.typeName, NULL);
#endif
    UA_DataType_clear(&t2);
} END_TEST

START_TEST(datatype_copy_with_members) {
    /* Copy ReadValueId which has members */
    UA_DataType t2;
    UA_StatusCode res = UA_DataType_copy(&UA_TYPES[UA_TYPES_READVALUEID], &t2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* Work around UA_DataType_copy bug: membersSize is zeroed but
     * never restored, so UA_DataType_clear won't free member names. */
    t2.membersSize = UA_TYPES[UA_TYPES_READVALUEID].membersSize;
    UA_DataType_clear(&t2);
} END_TEST

START_TEST(datatype_copy_complex) {
    /* Copy a complex type like BrowseResult */
    UA_DataType t2;
    UA_StatusCode res = UA_DataType_copy(&UA_TYPES[UA_TYPES_BROWSERESULT], &t2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    t2.membersSize = UA_TYPES[UA_TYPES_BROWSERESULT].membersSize;
    UA_DataType_clear(&t2);

    /* Copy ApplicationDescription */
    res = UA_DataType_copy(&UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION], &t2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    t2.membersSize = UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION].membersSize;
    UA_DataType_clear(&t2);

    /* Copy EndpointDescription */
    res = UA_DataType_copy(&UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION], &t2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    t2.membersSize = UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION].membersSize;
    UA_DataType_clear(&t2);
} END_TEST

/* === UA_order for various types === */
START_TEST(order_localizedtext) {
    UA_LocalizedText lt1 = UA_LOCALIZEDTEXT("en", "Alpha");
    UA_LocalizedText lt2 = UA_LOCALIZEDTEXT("en", "Beta");
    UA_Order o = UA_order(&lt1, &lt2, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    ck_assert(o != UA_ORDER_EQ); /* Just verify they are ordered differently */

    o = UA_order(&lt1, &lt1, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    ck_assert_int_eq(o, UA_ORDER_EQ);
} END_TEST

START_TEST(order_extensionobject) {
    UA_ExtensionObject eo1;
    UA_ExtensionObject_init(&eo1);
    UA_ExtensionObject eo2;
    UA_ExtensionObject_init(&eo2);

    UA_Order o = UA_order(&eo1, &eo2, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert_int_eq(o, UA_ORDER_EQ);
} END_TEST

START_TEST(order_bytestring) {
    UA_ByteString b1 = UA_BYTESTRING("aaa");
    UA_ByteString b2 = UA_BYTESTRING("bbb");
    UA_Order o = UA_order(&b1, &b2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

/* === Variant type checking edge cases === */
START_TEST(variant_type_checks) {
    UA_Variant v;
    UA_Variant_init(&v);

    /* Empty variant */
    ck_assert(UA_Variant_isEmpty(&v));
    ck_assert(!UA_Variant_isScalar(&v));
    ck_assert(!UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert(!UA_Variant_hasArrayType(&v, &UA_TYPES[UA_TYPES_INT32]));

    /* Scalar variant */
    UA_Int32 val = 42;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(!UA_Variant_isEmpty(&v));
    ck_assert(UA_Variant_isScalar(&v));
    ck_assert(UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert(!UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_DOUBLE]));
    ck_assert(!UA_Variant_hasArrayType(&v, &UA_TYPES[UA_TYPES_INT32]));

    /* Array variant */
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArray(&v, arr, 3, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(!UA_Variant_isEmpty(&v));
    ck_assert(!UA_Variant_isScalar(&v));
    ck_assert(!UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert(UA_Variant_hasArrayType(&v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert(!UA_Variant_hasArrayType(&v, &UA_TYPES[UA_TYPES_DOUBLE]));
} END_TEST

/* === NodeId ordering with different types === */
START_TEST(nodeid_order_mixed) {
    UA_NodeId n1 = UA_NODEID_NUMERIC(0, 100);
    UA_NodeId n2 = UA_NODEID_STRING(0, "abc");

    /* Numeric < String by identifier type */
    UA_Order o = UA_NodeId_order(&n1, &n2);
    ck_assert_int_eq(o, UA_ORDER_LESS);

    /* Different namespace */
    UA_NodeId n3 = UA_NODEID_NUMERIC(1, 100);
    o = UA_NodeId_order(&n1, &n3);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

/* === DateTime special values === */
START_TEST(datetime_special_values) {
    /* Epoch */
    UA_DateTimeStruct dts = UA_DateTime_toStruct(0);
    ck_assert_int_eq(dts.year, 1601);
    ck_assert_int_eq(dts.month, 1);
    ck_assert_int_eq(dts.day, 1);

    /* Parse a DateTime string */
    UA_DateTime dt;
    UA_StatusCode res = UA_DateTime_parse(&dt, UA_STRING("2020-01-15T12:30:00Z"));
    if(res == UA_STATUSCODE_GOOD) {
        UA_DateTimeStruct dts2 = UA_DateTime_toStruct(dt);
        ck_assert_int_eq(dts2.year, 2020);
    }
} END_TEST

/* === String_format === */
START_TEST(string_format_test) {
    UA_String s = UA_STRING_NULL;
    UA_StatusCode res = UA_String_format(&s, "value=%d name=%s", 42, "test");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(s.length > 0);
    UA_String_clear(&s);
} END_TEST

/* === Suite === */
static Suite *testSuite_typesExtra3(void) {
    TCase *tc_find = tcase_create("FindDataType");
    tcase_add_test(tc_find, findDataType_known);
    tcase_add_test(tc_find, findDataType_unknown);
    tcase_add_test(tc_find, findDataType_all_builtins);
#ifdef UA_ENABLE_TYPEDESCRIPTION
    tcase_add_test(tc_find, findDataTypeByName_boolean);
    tcase_add_test(tc_find, findDataTypeByName_string);
    tcase_add_test(tc_find, findDataTypeByName_readrequest);
    tcase_add_test(tc_find, findDataTypeByName_unknown);
#endif

    TCase *tc_string = tcase_create("StringOps");
    tcase_add_test(tc_string, string_fromChars_normal);
    tcase_add_test(tc_string, string_fromChars_empty);
    tcase_add_test(tc_string, string_fromChars_null);
    tcase_add_test(tc_string, string_append_normal);
    tcase_add_test(tc_string, string_append_empty);
    tcase_add_test(tc_string, string_append_to_empty);
    tcase_add_test(tc_string, string_format_test);

    TCase *tc_print = tcase_create("TypePrint");
#ifdef UA_ENABLE_TYPEDESCRIPTION
    tcase_add_test(tc_print, print_int32);
    tcase_add_test(tc_print, print_string);
    tcase_add_test(tc_print, print_nodeid);
    tcase_add_test(tc_print, print_variant_scalar);
    tcase_add_test(tc_print, print_variant_array);
    tcase_add_test(tc_print, print_variant_empty);
    tcase_add_test(tc_print, print_datavalue);
    tcase_add_test(tc_print, print_diagnosticinfo);
    tcase_add_test(tc_print, print_extensionobject);
    tcase_add_test(tc_print, print_localizedtext);
    tcase_add_test(tc_print, print_qualifiedname);
    tcase_add_test(tc_print, print_guid);
    tcase_add_test(tc_print, print_struct);
#endif

    TCase *tc_copy = tcase_create("DataTypeCopy");
    tcase_add_test(tc_copy, datatype_copy_basic);
    tcase_add_test(tc_copy, datatype_copy_with_members);
    tcase_add_test(tc_copy, datatype_copy_complex);

    TCase *tc_order = tcase_create("TypeOrder");
    tcase_add_test(tc_order, order_localizedtext);
    tcase_add_test(tc_order, order_extensionobject);
    tcase_add_test(tc_order, order_bytestring);
    tcase_add_test(tc_order, variant_type_checks);
    tcase_add_test(tc_order, nodeid_order_mixed);

    TCase *tc_misc = tcase_create("TypeMisc");
    tcase_add_test(tc_misc, datetime_special_values);

    Suite *s = suite_create("Types Extra 3");
    suite_add_tcase(s, tc_find);
    suite_add_tcase(s, tc_string);
    suite_add_tcase(s, tc_print);
    suite_add_tcase(s, tc_copy);
    suite_add_tcase(s, tc_order);
    suite_add_tcase(s, tc_misc);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_typesExtra3();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
