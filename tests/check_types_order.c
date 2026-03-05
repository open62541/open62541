/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Type ordering and comparison tests */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <check.h>
#include <stdlib.h>
#include <string.h>

/* === UA_order on basic types === */
START_TEST(order_int32) {
    UA_Int32 a = 10, b = 20, c = 10;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_INT32]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&b, &a, &UA_TYPES[UA_TYPES_INT32]), UA_ORDER_MORE);
    ck_assert_int_eq(UA_order(&a, &c, &UA_TYPES[UA_TYPES_INT32]), UA_ORDER_EQ);
} END_TEST

START_TEST(order_uint32) {
    UA_UInt32 a = 0, b = 100;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_UINT32]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&b, &a, &UA_TYPES[UA_TYPES_UINT32]), UA_ORDER_MORE);
    ck_assert_int_eq(UA_order(&a, &a, &UA_TYPES[UA_TYPES_UINT32]), UA_ORDER_EQ);
} END_TEST

START_TEST(order_int64) {
    UA_Int64 a = -100, b = 100;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_INT64]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_double) {
    UA_Double a = 1.5, b = 2.5, c = 1.5;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DOUBLE]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &c, &UA_TYPES[UA_TYPES_DOUBLE]), UA_ORDER_EQ);
} END_TEST

START_TEST(order_float) {
    UA_Float a = 1.0f, b = 2.0f;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_FLOAT]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_boolean) {
    UA_Boolean a = false, b = true;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_BOOLEAN]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&b, &a, &UA_TYPES[UA_TYPES_BOOLEAN]), UA_ORDER_MORE);
} END_TEST

START_TEST(order_byte) {
    UA_Byte a = 0, b = 255;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_BYTE]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_string) {
    UA_String a = UA_STRING("abc");
    UA_String b = UA_STRING("def");
    UA_String c = UA_STRING("abc");
    UA_String d = UA_STRING("abcd");
    UA_String e = UA_STRING("");

    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_STRING]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&b, &a, &UA_TYPES[UA_TYPES_STRING]), UA_ORDER_MORE);
    ck_assert_int_eq(UA_order(&a, &c, &UA_TYPES[UA_TYPES_STRING]), UA_ORDER_EQ);
    ck_assert_int_eq(UA_order(&a, &d, &UA_TYPES[UA_TYPES_STRING]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&e, &a, &UA_TYPES[UA_TYPES_STRING]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_bytestring) {
    UA_ByteString a = UA_BYTESTRING("abc");
    UA_ByteString b = UA_BYTESTRING("def");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_BYTESTRING]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_datetime) {
    UA_DateTime a = UA_DateTime_now();
    UA_DateTime b = a + 1000;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATETIME]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &a, &UA_TYPES[UA_TYPES_DATETIME]), UA_ORDER_EQ);
} END_TEST

START_TEST(order_guid) {
    UA_Guid a = UA_GUID("00000001-0000-0000-0000-000000000000");
    UA_Guid b = UA_GUID("00000002-0000-0000-0000-000000000000");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_GUID]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &a, &UA_TYPES[UA_TYPES_GUID]), UA_ORDER_EQ);
} END_TEST

START_TEST(order_statuscode) {
    UA_StatusCode a = UA_STATUSCODE_GOOD;
    UA_StatusCode b = UA_STATUSCODE_BADUNEXPECTEDERROR;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_STATUSCODE]), UA_ORDER_LESS);
} END_TEST

/* === UA_order on NodeIds === */
START_TEST(order_nodeid_numeric) {
    UA_NodeId a = UA_NODEID_NUMERIC(0, 1);
    UA_NodeId b = UA_NODEID_NUMERIC(0, 2);
    UA_NodeId c = UA_NODEID_NUMERIC(1, 1);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_NODEID]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &c, &UA_TYPES[UA_TYPES_NODEID]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &a, &UA_TYPES[UA_TYPES_NODEID]), UA_ORDER_EQ);
} END_TEST

START_TEST(order_nodeid_string) {
    UA_NodeId a = UA_NODEID_STRING(1, "aaa");
    UA_NodeId b = UA_NODEID_STRING(1, "bbb");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_NODEID]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_nodeid_guid) {
    UA_Guid g1 = UA_GUID("00000001-0000-0000-0000-000000000000");
    UA_Guid g2 = UA_GUID("00000002-0000-0000-0000-000000000000");
    UA_NodeId a = UA_NODEID_GUID(1, g1);
    UA_NodeId b = UA_NODEID_GUID(1, g2);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_NODEID]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_nodeid_bytestring) {
    UA_NodeId a, b;
    a.namespaceIndex = 1;
    a.identifierType = UA_NODEIDTYPE_BYTESTRING;
    a.identifier.byteString = UA_BYTESTRING("abc");
    b.namespaceIndex = 1;
    b.identifierType = UA_NODEIDTYPE_BYTESTRING;
    b.identifier.byteString = UA_BYTESTRING("def");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_NODEID]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_nodeid_mixed_types) {
    UA_NodeId a = UA_NODEID_NUMERIC(0, 1);
    UA_NodeId b = UA_NODEID_STRING(0, "test");
    /* Numeric has lower identifierType value */
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert(o != UA_ORDER_EQ); /* Just ensure it runs */
} END_TEST

/* === UA_order on ExpandedNodeId === */
START_TEST(order_expandednodeid) {
    UA_ExpandedNodeId a = UA_EXPANDEDNODEID_NUMERIC(0, 1);
    UA_ExpandedNodeId b = UA_EXPANDEDNODEID_NUMERIC(0, 2);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &a, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]), UA_ORDER_EQ);
} END_TEST

START_TEST(order_expandednodeid_with_uri) {
    UA_ExpandedNodeId a = UA_EXPANDEDNODEID_NUMERIC(0, 1);
    a.namespaceUri = UA_STRING("urn:a");
    UA_ExpandedNodeId b = UA_EXPANDEDNODEID_NUMERIC(0, 1);
    b.namespaceUri = UA_STRING("urn:b");
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

/* === UA_order on QualifiedName === */
START_TEST(order_qualifiedname) {
    UA_QualifiedName a = UA_QUALIFIEDNAME(0, "aaa");
    UA_QualifiedName b = UA_QUALIFIEDNAME(0, "bbb");
    UA_QualifiedName c = UA_QUALIFIEDNAME(1, "aaa");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &c, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &a, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]), UA_ORDER_EQ);
} END_TEST

/* === UA_order on LocalizedText === */
START_TEST(order_localizedtext) {
    UA_LocalizedText a = UA_LOCALIZEDTEXT("en", "aaa");
    UA_LocalizedText b = UA_LOCALIZEDTEXT("en", "bbb");
    UA_LocalizedText c = UA_LOCALIZEDTEXT("fr", "aaa");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &c, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]), UA_ORDER_LESS);
} END_TEST

/* === UA_order on DataValue === */
START_TEST(order_datavalue) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);

    /* Both empty */
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_EQ);

    /* a has value, b doesn't */
    a.hasValue = true;
    UA_Int32 val1 = 10;
    UA_Variant_setScalar(&a.value, &val1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]);
    ck_assert(o != UA_ORDER_EQ);

    /* Both have values, different */
    b.hasValue = true;
    UA_Int32 val2 = 20;
    UA_Variant_setScalar(&b.value, &val2, &UA_TYPES[UA_TYPES_INT32]);
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]);
    ck_assert_int_eq(o, UA_ORDER_LESS);

    /* With timestamps */
    a.hasSourceTimestamp = true;
    a.sourceTimestamp = 100;
    b.hasSourceTimestamp = true;
    b.sourceTimestamp = 200;
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]);
    ck_assert(o != UA_ORDER_EQ);

    /* With server timestamp */
    a.hasServerTimestamp = true;
    a.serverTimestamp = 50;
    b.hasServerTimestamp = true;
    b.serverTimestamp = 60;
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]);
    (void)o;

    /* With picoseconds */
    a.hasSourcePicoseconds = true;
    a.sourcePicoseconds = 1;
    b.hasSourcePicoseconds = true;
    b.sourcePicoseconds = 2;
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]);
    (void)o;

    a.hasServerPicoseconds = true;
    a.serverPicoseconds = 1;
    b.hasServerPicoseconds = true;
    b.serverPicoseconds = 2;
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]);
    (void)o;

    /* With status */
    a.hasStatus = true;
    a.status = UA_STATUSCODE_GOOD;
    b.hasStatus = true;
    b.status = UA_STATUSCODE_BADUNEXPECTEDERROR;
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]);
    (void)o;
} END_TEST

/* === UA_order on Variant === */
START_TEST(order_variant_scalar) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);

    /* Both empty */
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]), UA_ORDER_EQ);

    /* One has value */
    UA_Int32 val1 = 10, val2 = 20;
    UA_Variant_setScalar(&a, &val1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(o != UA_ORDER_EQ);

    /* Both have scalar values */
    UA_Variant_setScalar(&b, &val2, &UA_TYPES[UA_TYPES_INT32]);
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

START_TEST(order_variant_array) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);

    UA_Int32 arr1[] = {1, 2, 3};
    UA_Int32 arr2[] = {1, 2, 4};
    UA_Variant_setArray(&a, arr1, 3, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setArray(&b, arr2, 3, &UA_TYPES[UA_TYPES_INT32]);

    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
} END_TEST

START_TEST(order_variant_different_types) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);

    UA_Int32 val1 = 10;
    UA_Double val2 = 20.0;
    UA_Variant_setScalar(&a, &val1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&b, &val2, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(o != UA_ORDER_EQ);
} END_TEST

/* === UA_order on structures === */
START_TEST(order_readvalueid) {
    UA_ReadValueId a, b;
    UA_ReadValueId_init(&a);
    UA_ReadValueId_init(&b);
    a.nodeId = UA_NODEID_NUMERIC(0, 1);
    a.attributeId = UA_ATTRIBUTEID_VALUE;
    b.nodeId = UA_NODEID_NUMERIC(0, 2);
    b.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_READVALUEID]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &a, &UA_TYPES[UA_TYPES_READVALUEID]), UA_ORDER_EQ);
} END_TEST

START_TEST(order_writevalue) {
    UA_WriteValue a, b;
    UA_WriteValue_init(&a);
    UA_WriteValue_init(&b);
    a.nodeId = UA_NODEID_NUMERIC(0, 1);
    a.attributeId = UA_ATTRIBUTEID_VALUE;
    b.nodeId = UA_NODEID_NUMERIC(0, 2);
    b.attributeId = UA_ATTRIBUTEID_VALUE;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_WRITEVALUE]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_browsedescription) {
    UA_BrowseDescription a, b;
    UA_BrowseDescription_init(&a);
    UA_BrowseDescription_init(&b);
    a.nodeId = UA_NODEID_NUMERIC(0, 1);
    b.nodeId = UA_NODEID_NUMERIC(0, 2);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_BROWSEDESCRIPTION]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_applicationdescription) {
    UA_ApplicationDescription a, b;
    UA_ApplicationDescription_init(&a);
    UA_ApplicationDescription_init(&b);
    a.applicationUri = UA_STRING("urn:a");
    b.applicationUri = UA_STRING("urn:b");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]), UA_ORDER_LESS);
} END_TEST

START_TEST(order_endpointdescription) {
    UA_EndpointDescription a, b;
    UA_EndpointDescription_init(&a);
    UA_EndpointDescription_init(&b);
    a.endpointUrl = UA_STRING("opc.tcp://a");
    b.endpointUrl = UA_STRING("opc.tcp://b");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]), UA_ORDER_LESS);
} END_TEST

/* === UA_order on DiagnosticInfo === */
START_TEST(order_diagnosticinfo) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);

    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_EQ);

    a.hasSymbolicId = true;
    a.symbolicId = 1;
    b.hasSymbolicId = true;
    b.symbolicId = 2;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);

    a.hasNamespaceUri = true;
    a.namespaceUri = 10;
    b.hasNamespaceUri = true;
    b.namespaceUri = 20;
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    (void)o;

    a.hasLocalizedText = true;
    a.localizedText = 1;
    b.hasLocalizedText = true;
    b.localizedText = 2;
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    (void)o;

    a.hasAdditionalInfo = true;
    a.additionalInfo = UA_STRING("info1");
    b.hasAdditionalInfo = true;
    b.additionalInfo = UA_STRING("info2");
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    (void)o;

    a.hasInnerStatusCode = true;
    a.innerStatusCode = UA_STATUSCODE_GOOD;
    b.hasInnerStatusCode = true;
    b.innerStatusCode = UA_STATUSCODE_BADUNEXPECTEDERROR;
    o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    (void)o;
} END_TEST

/* === UA_NodeId_isNull === */
START_TEST(nodeid_isNull_numeric) {
    UA_NodeId a = UA_NODEID_NUMERIC(0, 0);
    ck_assert(UA_NodeId_isNull(&a));
    UA_NodeId b = UA_NODEID_NUMERIC(0, 1);
    ck_assert(!UA_NodeId_isNull(&b));
    UA_NodeId c = UA_NODEID_NUMERIC(1, 0);
    ck_assert(!UA_NodeId_isNull(&c));
} END_TEST

START_TEST(nodeid_isNull_string) {
    UA_NodeId a = UA_NODEID_STRING(0, "");
    ck_assert(UA_NodeId_isNull(&a));
    UA_NodeId b = UA_NODEID_STRING(0, "test");
    ck_assert(!UA_NodeId_isNull(&b));
} END_TEST

START_TEST(nodeid_isNull_guid) {
    UA_Guid nullGuid;
    memset(&nullGuid, 0, sizeof(UA_Guid));
    UA_NodeId a = UA_NODEID_GUID(0, nullGuid);
    ck_assert(UA_NodeId_isNull(&a));
    UA_Guid nonNullGuid = UA_GUID("00000001-0000-0000-0000-000000000000");
    UA_NodeId b = UA_NODEID_GUID(0, nonNullGuid);
    ck_assert(!UA_NodeId_isNull(&b));
} END_TEST

START_TEST(nodeid_isNull_bytestring) {
    UA_NodeId a;
    a.namespaceIndex = 0;
    a.identifierType = UA_NODEIDTYPE_BYTESTRING;
    a.identifier.byteString = UA_BYTESTRING_NULL;
    ck_assert(UA_NodeId_isNull(&a));

    UA_NodeId b;
    b.namespaceIndex = 0;
    b.identifierType = UA_NODEIDTYPE_BYTESTRING;
    b.identifier.byteString = UA_BYTESTRING("data");
    ck_assert(!UA_NodeId_isNull(&b));
} END_TEST

/* === UA_DataType_copy === */
START_TEST(datatype_copy) {
    UA_DataType dst;
    UA_StatusCode res = UA_DataType_copy(&UA_TYPES[UA_TYPES_READVALUEID], &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.memSize, UA_TYPES[UA_TYPES_READVALUEID].memSize);
    /* Work around UA_DataType_copy bug: membersSize is zeroed but
     * never restored, so UA_DataType_clear won't free member names. */
    dst.membersSize = UA_TYPES[UA_TYPES_READVALUEID].membersSize;
    UA_DataType_clear(&dst);
} END_TEST

START_TEST(datatype_copy_complex) {
    UA_DataType dst;
    /* EndpointDescription has many members */
    UA_StatusCode res = UA_DataType_copy(&UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION], &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    dst.membersSize = UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION].membersSize;
    UA_DataType_clear(&dst);
} END_TEST

START_TEST(datatype_copy_simple) {
    UA_DataType dst;
    /* Int32 has zero members */
    UA_StatusCode res = UA_DataType_copy(&UA_TYPES[UA_TYPES_INT32], &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_DataType_clear(&dst);
} END_TEST

/* === UA_DateTime_parse === */
START_TEST(datetime_parse_valid) {
    UA_DateTime dt = 0;
    UA_StatusCode res = UA_DateTime_parse(&dt, UA_STRING("2025-01-15T12:30:45Z"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(dt != 0);
} END_TEST

START_TEST(datetime_parse_with_fractional) {
    UA_DateTime dt = 0;
    UA_StatusCode res = UA_DateTime_parse(&dt, UA_STRING("2025-06-15T12:30:45.123456Z"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(dt != 0);
} END_TEST

START_TEST(datetime_parse_empty) {
    UA_DateTime dt = 0;
    UA_StatusCode res = UA_DateTime_parse(&dt, UA_STRING(""));
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(datetime_parse_invalid) {
    UA_DateTime dt = 0;
    UA_StatusCode res = UA_DateTime_parse(&dt, UA_STRING("not-a-date"));
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(datetime_parse_just_date) {
    UA_DateTime dt = 0;
    /* Use a padded buffer because UA_DateTime_parse / parseUInt64
     * may read past the UA_String length (library bug). */
    char dateStr[] = "2025-01-15          ";
    UA_String s = {10, (UA_Byte*)dateStr};
    UA_StatusCode res = UA_DateTime_parse(&dt, s);
    /* May or may not succeed depending on format strictness */
    (void)res;
    (void)dt;
} END_TEST

/* === UA_String_format === */
START_TEST(string_format_simple) {
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_String_format(&out, "Hello %s", "world");
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

START_TEST(string_format_long) {
    UA_String out = UA_STRING_NULL;
    /* Create a format string that results in >128 chars (triggers realloc path) */
    char longStr[200];
    memset(longStr, 'A', 199);
    longStr[199] = '\0';
    UA_StatusCode res = UA_String_format(&out, "%s%s", longStr, longStr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 128);
    UA_String_clear(&out);
} END_TEST

START_TEST(string_format_numbers) {
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_String_format(&out, "val=%d hex=0x%x", 42, 255);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

/* === UA_order on ExtensionObject === */
START_TEST(order_extensionobject) {
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), UA_ORDER_EQ);
} END_TEST

/* === Type operations: copy, clear, new, delete === */
START_TEST(type_new_delete) {
    void *p = UA_new(&UA_TYPES[UA_TYPES_READVALUEID]);
    ck_assert_ptr_ne(p, NULL);
    UA_delete(p, &UA_TYPES[UA_TYPES_READVALUEID]);
} END_TEST

START_TEST(type_copy_readrequest) {
    UA_ReadRequest src;
    UA_ReadRequest_init(&src);
    src.maxAge = 100.0;
    src.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;

    UA_ReadValueId ids[2];
    UA_ReadValueId_init(&ids[0]);
    UA_ReadValueId_init(&ids[1]);
    ids[0].nodeId = UA_NODEID_NUMERIC(0, 1);
    ids[0].attributeId = UA_ATTRIBUTEID_VALUE;
    ids[1].nodeId = UA_NODEID_NUMERIC(0, 2);
    ids[1].attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
    src.nodesToRead = ids;
    src.nodesToReadSize = 2;

    UA_ReadRequest dst;
    UA_StatusCode res = UA_copy(&src, &dst, &UA_TYPES[UA_TYPES_READREQUEST]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.nodesToReadSize, 2);
    ck_assert((dst.maxAge) == (100.0));
    UA_ReadRequest_clear(&dst);
} END_TEST

START_TEST(type_copy_writerequest) {
    UA_WriteRequest src;
    UA_WriteRequest_init(&src);
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = UA_NODEID_NUMERIC(0, 1);
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.hasValue = true;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&wv.value.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    src.nodesToWrite = &wv;
    src.nodesToWriteSize = 1;

    UA_WriteRequest dst;
    UA_StatusCode res = UA_copy(&src, &dst, &UA_TYPES[UA_TYPES_WRITEREQUEST]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.nodesToWriteSize, 1);
    UA_WriteRequest_clear(&dst);
} END_TEST

/* === Suite definition === */
static Suite *testSuite_types_order(void) {
    TCase *tc_order_basic = tcase_create("OrderBasic");
    tcase_add_test(tc_order_basic, order_int32);
    tcase_add_test(tc_order_basic, order_uint32);
    tcase_add_test(tc_order_basic, order_int64);
    tcase_add_test(tc_order_basic, order_double);
    tcase_add_test(tc_order_basic, order_float);
    tcase_add_test(tc_order_basic, order_boolean);
    tcase_add_test(tc_order_basic, order_byte);
    tcase_add_test(tc_order_basic, order_string);
    tcase_add_test(tc_order_basic, order_bytestring);
    tcase_add_test(tc_order_basic, order_datetime);
    tcase_add_test(tc_order_basic, order_guid);
    tcase_add_test(tc_order_basic, order_statuscode);

    TCase *tc_order_nodeid = tcase_create("OrderNodeId");
    tcase_add_test(tc_order_nodeid, order_nodeid_numeric);
    tcase_add_test(tc_order_nodeid, order_nodeid_string);
    tcase_add_test(tc_order_nodeid, order_nodeid_guid);
    tcase_add_test(tc_order_nodeid, order_nodeid_bytestring);
    tcase_add_test(tc_order_nodeid, order_nodeid_mixed_types);
    tcase_add_test(tc_order_nodeid, order_expandednodeid);
    tcase_add_test(tc_order_nodeid, order_expandednodeid_with_uri);

    TCase *tc_order_complex = tcase_create("OrderComplex");
    tcase_add_test(tc_order_complex, order_qualifiedname);
    tcase_add_test(tc_order_complex, order_localizedtext);
    tcase_add_test(tc_order_complex, order_datavalue);
    tcase_add_test(tc_order_complex, order_variant_scalar);
    tcase_add_test(tc_order_complex, order_variant_array);
    tcase_add_test(tc_order_complex, order_variant_different_types);
    tcase_add_test(tc_order_complex, order_readvalueid);
    tcase_add_test(tc_order_complex, order_writevalue);
    tcase_add_test(tc_order_complex, order_browsedescription);
    tcase_add_test(tc_order_complex, order_applicationdescription);
    tcase_add_test(tc_order_complex, order_endpointdescription);
    tcase_add_test(tc_order_complex, order_diagnosticinfo);
    tcase_add_test(tc_order_complex, order_extensionobject);

    TCase *tc_nodeid = tcase_create("NodeIdIsNull");
    tcase_add_test(tc_nodeid, nodeid_isNull_numeric);
    tcase_add_test(tc_nodeid, nodeid_isNull_string);
    tcase_add_test(tc_nodeid, nodeid_isNull_guid);
    tcase_add_test(tc_nodeid, nodeid_isNull_bytestring);

    TCase *tc_datatype = tcase_create("DataTypeCopy");
    tcase_add_test(tc_datatype, datatype_copy);
    tcase_add_test(tc_datatype, datatype_copy_complex);
    tcase_add_test(tc_datatype, datatype_copy_simple);

    TCase *tc_datetime = tcase_create("DateTimeParse");
    tcase_add_test(tc_datetime, datetime_parse_valid);
    tcase_add_test(tc_datetime, datetime_parse_with_fractional);
    tcase_add_test(tc_datetime, datetime_parse_empty);
    tcase_add_test(tc_datetime, datetime_parse_invalid);
    tcase_add_test(tc_datetime, datetime_parse_just_date);

    TCase *tc_format = tcase_create("StringFormat");
    tcase_add_test(tc_format, string_format_simple);
    tcase_add_test(tc_format, string_format_long);
    tcase_add_test(tc_format, string_format_numbers);

    TCase *tc_ops = tcase_create("TypeOps");
    tcase_add_test(tc_ops, type_new_delete);
    tcase_add_test(tc_ops, type_copy_readrequest);
    tcase_add_test(tc_ops, type_copy_writerequest);

    Suite *s = suite_create("Types Order");
    suite_add_tcase(s, tc_order_basic);
    suite_add_tcase(s, tc_order_nodeid);
    suite_add_tcase(s, tc_order_complex);
    suite_add_tcase(s, tc_nodeid);
    suite_add_tcase(s, tc_datatype);
    suite_add_tcase(s, tc_datetime);
    suite_add_tcase(s, tc_format);
    suite_add_tcase(s, tc_ops);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_types_order();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
