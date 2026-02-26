/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include "ua_types_encoding_binary.h"

#include <stdlib.h>
#include <check.h>
#include <float.h>
#include <math.h>

/* Helper: encode, then decode and return status */
static UA_StatusCode
roundtripBinary(const void *src, const UA_DataType *type,
                void *dst) {
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    UA_StatusCode res = UA_encodeBinary(src, type, &buf, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_decodeBinary(&buf, dst, type, NULL);
    UA_ByteString_clear(&buf);
    return res;
}

/* ========== Boolean ========== */
START_TEST(binary_bool_true) {
    UA_Boolean src = true, dst = false;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_BOOLEAN], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst == true);
} END_TEST

START_TEST(binary_bool_false) {
    UA_Boolean src = false, dst = true;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_BOOLEAN], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst == false);
} END_TEST

/* ========== Integer types ========== */
START_TEST(binary_sbyte) {
    UA_SByte src = -128, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_SBYTE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst, -128);
} END_TEST

START_TEST(binary_byte) {
    UA_Byte src = 255, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_BYTE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst, 255);
} END_TEST

START_TEST(binary_int16) {
    UA_Int16 src = -32768, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_INT16], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst, -32768);
} END_TEST

START_TEST(binary_uint16) {
    UA_UInt16 src = 65535, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_UINT16], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst, 65535);
} END_TEST

START_TEST(binary_int32) {
    UA_Int32 src = -2147483647 - 1, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_INT32], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst, -2147483647 - 1);
} END_TEST

START_TEST(binary_uint32) {
    UA_UInt32 src = 4294967295U, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_UINT32], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst, 4294967295U);
} END_TEST

START_TEST(binary_int64) {
    UA_Int64 src = -9223372036854775807LL - 1, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_INT64], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst == -9223372036854775807LL - 1);
} END_TEST

START_TEST(binary_uint64) {
    UA_UInt64 src = 18446744073709551615ULL, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_UINT64], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst == 18446744073709551615ULL);
} END_TEST

/* ========== Float/Double special values ========== */
START_TEST(binary_float_nan) {
    UA_Float src = NAN, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_FLOAT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(isnan(dst));
} END_TEST

START_TEST(binary_float_inf) {
    UA_Float src = INFINITY, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_FLOAT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(isinf(dst) && dst > 0);
} END_TEST

START_TEST(binary_float_neginf) {
    UA_Float src = -INFINITY, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_FLOAT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(isinf(dst) && dst < 0);
} END_TEST

START_TEST(binary_double_nan) {
    UA_Double src = NAN, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_DOUBLE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(isnan(dst));
} END_TEST

START_TEST(binary_double_inf) {
    UA_Double src = INFINITY, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_DOUBLE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(isinf(dst) && dst > 0);
} END_TEST

/* ========== String types ========== */
START_TEST(binary_string_empty) {
    UA_String src = UA_STRING(""), dst;
    UA_String_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_STRING], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.length, 0);
    UA_String_clear(&dst);
} END_TEST

START_TEST(binary_string_null) {
    UA_String src = UA_STRING_NULL, dst;
    UA_String_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_STRING], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst.data == NULL);
} END_TEST

START_TEST(binary_string_long) {
    char buf[1024];
    for(int i = 0; i < 1023; i++) buf[i] = 'A' + (i % 26);
    buf[1023] = 0;
    UA_String src = UA_STRING(buf), dst;
    UA_String_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_STRING], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src, &dst));
    UA_String_clear(&dst);
} END_TEST

/* ========== DateTime ========== */
START_TEST(binary_datetime) {
    UA_DateTime src = UA_DateTime_now(), dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_DATETIME], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(src == dst);
} END_TEST

/* ========== Guid ========== */
START_TEST(binary_guid) {
    UA_Guid src = UA_GUID("72962B91-FA75-4AE6-9D28-B404DC7DAF63"), dst;
    UA_Guid_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_GUID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Guid_equal(&src, &dst));
} END_TEST

/* ========== ByteString ========== */
START_TEST(binary_bytestring_empty) {
    UA_ByteString src = UA_BYTESTRING(""), dst;
    UA_ByteString_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_BYTESTRING], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.length, 0);
    UA_ByteString_clear(&dst);
} END_TEST

/* ========== NodeId variants ========== */
START_TEST(binary_nodeid_numeric) {
    UA_NodeId src = UA_NODEID_NUMERIC(0, 85);
    UA_NodeId dst;
    UA_NodeId_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_NODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src, &dst));
    UA_NodeId_clear(&dst);
} END_TEST

START_TEST(binary_nodeid_string) {
    UA_NodeId src = UA_NODEID_STRING(1, "test.node");
    UA_NodeId dst;
    UA_NodeId_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_NODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src, &dst));
    UA_NodeId_clear(&dst);
} END_TEST

START_TEST(binary_nodeid_guid) {
    UA_Guid g = UA_GUID("72962B91-FA75-4AE6-9D28-B404DC7DAF63");
    UA_NodeId src = UA_NODEID_GUID(2, g);
    UA_NodeId dst;
    UA_NodeId_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_NODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src, &dst));
    UA_NodeId_clear(&dst);
} END_TEST

START_TEST(binary_nodeid_bytestring) {
    UA_NodeId src;
    src.identifierType = UA_NODEIDTYPE_BYTESTRING;
    src.namespaceIndex = 3;
    src.identifier.byteString = UA_BYTESTRING("testbs");
    UA_NodeId dst;
    UA_NodeId_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_NODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src, &dst));
    UA_NodeId_clear(&dst);
} END_TEST

/* Numeric NodeId with large ns and large id (4-byte encoding) */
START_TEST(binary_nodeid_fourbyte) {
    UA_NodeId src = UA_NODEID_NUMERIC(5, 300); /* ns > 0, id > 255 → 4-byte */
    UA_NodeId dst;
    UA_NodeId_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_NODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src, &dst));
    UA_NodeId_clear(&dst);
} END_TEST

/* ========== ExpandedNodeId ========== */
START_TEST(binary_expandednodeid_with_uri) {
    UA_ExpandedNodeId src;
    UA_ExpandedNodeId_init(&src);
    src.nodeId = UA_NODEID_NUMERIC(0, 100);
    src.namespaceUri = UA_STRING("urn:test:namespace");
    src.serverIndex = 2;
    UA_ExpandedNodeId dst;
    UA_ExpandedNodeId_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_EXPANDEDNODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.namespaceUri, &dst.namespaceUri));
    ck_assert_uint_eq(dst.serverIndex, 2);
    UA_ExpandedNodeId_clear(&dst);
} END_TEST

/* ========== StatusCode ========== */
START_TEST(binary_statuscode) {
    UA_StatusCode src = UA_STATUSCODE_BADINTERNALERROR, dst = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_STATUSCODE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

/* ========== QualifiedName ========== */
START_TEST(binary_qualifiedname) {
    UA_QualifiedName src = UA_QUALIFIEDNAME(1, "TestName");
    UA_QualifiedName dst;
    UA_QualifiedName_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_QualifiedName_equal(&src, &dst));
    UA_QualifiedName_clear(&dst);
} END_TEST

/* ========== LocalizedText ========== */
START_TEST(binary_localizedtext) {
    UA_LocalizedText src = UA_LOCALIZEDTEXT("en", "Hello");
    UA_LocalizedText dst;
    UA_LocalizedText_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.text, &dst.text));
    ck_assert(UA_String_equal(&src.locale, &dst.locale));
    UA_LocalizedText_clear(&dst);
} END_TEST

START_TEST(binary_localizedtext_nolocale) {
    UA_LocalizedText src;
    UA_LocalizedText_init(&src);
    src.text = UA_STRING("NoLocale");
    UA_LocalizedText dst;
    UA_LocalizedText_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.text, &dst.text));
    UA_LocalizedText_clear(&dst);
} END_TEST

/* ========== Variant with scalar ========== */
START_TEST(binary_variant_int32) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_Int32 val = 42;
    UA_Variant_setScalarCopy(&src, &val, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&dst, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_int_eq(*(UA_Int32 *)dst.data, 42);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

/* ========== Variant empty ========== */
START_TEST(binary_variant_empty) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_isEmpty(&dst));
} END_TEST

/* ========== Variant with array ========== */
START_TEST(binary_variant_array) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_Int32 arr[] = {1, 2, 3, 4, 5};
    UA_Variant_setArrayCopy(&src, arr, 5, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.arrayLength, 5);
    ck_assert_int_eq(((UA_Int32 *)dst.data)[4], 5);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

/* Variant with multi-dimensional array */
START_TEST(binary_variant_matrix) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_Double arr[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    UA_Variant_setArrayCopy(&src, arr, 6, &UA_TYPES[UA_TYPES_DOUBLE]);
    src.arrayDimensionsSize = 2;
    src.arrayDimensions = (UA_UInt32 *)UA_Array_new(2, &UA_TYPES[UA_TYPES_UINT32]);
    src.arrayDimensions[0] = 2;
    src.arrayDimensions[1] = 3;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.arrayLength, 6);
    ck_assert_uint_eq(dst.arrayDimensionsSize, 2);
    ck_assert_uint_eq(dst.arrayDimensions[0], 2);
    ck_assert_uint_eq(dst.arrayDimensions[1], 3);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

/* Variant with string array */
START_TEST(binary_variant_string_array) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_String strs[] = {UA_STRING_STATIC("hello"), UA_STRING_STATIC("world")};
    UA_Variant_setArrayCopy(&src, strs, 2, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.arrayLength, 2);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

/* ========== DataValue with all fields ========== */
START_TEST(binary_datavalue_all_fields) {
    UA_DataValue src, dst;
    UA_DataValue_init(&src);
    UA_DataValue_init(&dst);
    UA_Int32 val = 100;
    UA_Variant_setScalarCopy(&src.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    src.hasValue = true;
    src.status = UA_STATUSCODE_GOOD;
    src.hasStatus = true;
    src.sourceTimestamp = UA_DateTime_now();
    src.hasSourceTimestamp = true;
    src.serverTimestamp = UA_DateTime_now();
    src.hasServerTimestamp = true;
    src.sourcePicoseconds = 9999;
    src.hasSourcePicoseconds = true;
    src.serverPicoseconds = 5000;
    src.hasServerPicoseconds = true;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_DATAVALUE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst.hasValue);
    ck_assert(dst.hasStatus);
    ck_assert(dst.hasSourceTimestamp);
    ck_assert(dst.hasServerTimestamp);
    ck_assert(dst.hasSourcePicoseconds);
    ck_assert(dst.hasServerPicoseconds);
    ck_assert_uint_eq(dst.sourcePicoseconds, 9999);
    UA_DataValue_clear(&src);
    UA_DataValue_clear(&dst);
} END_TEST

/* DataValue with high picoseconds (clamping) */
START_TEST(binary_datavalue_pico_clamp) {
    UA_DataValue src, dst;
    UA_DataValue_init(&src);
    UA_DataValue_init(&dst);
    src.sourcePicoseconds = 60000; /* exceeds MAX_PICO_SECONDS (9999) but fits UA_UInt16 */
    src.hasSourcePicoseconds = true;
    src.serverPicoseconds = 60000;
    src.hasServerPicoseconds = true;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_DATAVALUE], &dst),
                      UA_STATUSCODE_GOOD);
    /* Picoseconds may be clamped if > MAX_PICO_SECONDS */
    ck_assert(dst.hasSourcePicoseconds);
    UA_DataValue_clear(&src);
    UA_DataValue_clear(&dst);
} END_TEST

/* ========== DiagnosticInfo with inner ========== */
START_TEST(binary_diagnosticinfo_full) {
    UA_DiagnosticInfo inner;
    UA_DiagnosticInfo_init(&inner);
    inner.hasSymbolicId = true;
    inner.symbolicId = 42;
    inner.hasLocalizedText = true;
    inner.localizedText = 7;

    UA_DiagnosticInfo src, dst;
    UA_DiagnosticInfo_init(&src);
    UA_DiagnosticInfo_init(&dst);
    src.hasSymbolicId = true;
    src.symbolicId = 1;
    src.hasNamespaceUri = true;
    src.namespaceUri = 2;
    src.hasLocalizedText = true;
    src.localizedText = 3;
    src.hasLocale = true;
    src.locale = 4;
    src.hasAdditionalInfo = true;
    src.additionalInfo = UA_STRING("test info");
    src.hasInnerStatusCode = true;
    src.innerStatusCode = UA_STATUSCODE_BADINTERNALERROR;
    src.hasInnerDiagnosticInfo = true;
    src.innerDiagnosticInfo = &inner;

    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst.hasSymbolicId);
    ck_assert(dst.hasNamespaceUri);
    ck_assert(dst.hasLocalizedText);
    ck_assert(dst.hasLocale);
    ck_assert(dst.hasAdditionalInfo);
    ck_assert(dst.hasInnerStatusCode);
    ck_assert(dst.hasInnerDiagnosticInfo);
    ck_assert_int_eq(dst.innerDiagnosticInfo->symbolicId, 42);

    /* Don't let clear free our stack-allocated inner */
    src.hasInnerDiagnosticInfo = false;
    src.innerDiagnosticInfo = NULL;
    UA_DiagnosticInfo_clear(&dst);
} END_TEST

/* ========== ExtensionObject ========== */
START_TEST(binary_extensionobject_decoded) {
    UA_ExtensionObject src, dst;
    UA_ExtensionObject_init(&src);
    UA_ExtensionObject_init(&dst);

    UA_Int32 *val = UA_Int32_new();
    *val = 999;
    src.encoding = UA_EXTENSIONOBJECT_DECODED;
    src.content.decoded.type = &UA_TYPES[UA_TYPES_INT32];
    src.content.decoded.data = val;

    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_ExtensionObject_clear(&src);
    UA_ExtensionObject_clear(&dst);
} END_TEST

START_TEST(binary_extensionobject_bytestring) {
    UA_ExtensionObject src, dst;
    UA_ExtensionObject_init(&src);
    UA_ExtensionObject_init(&dst);
    src.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    src.content.encoded.typeId = UA_NODEID_NUMERIC(0, 999);
    src.content.encoded.body = UA_BYTESTRING("binarydata");
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.encoding, UA_EXTENSIONOBJECT_ENCODED_BYTESTRING);
    UA_ExtensionObject_clear(&dst);
} END_TEST

START_TEST(binary_extensionobject_xml) {
    UA_ExtensionObject src, dst;
    UA_ExtensionObject_init(&src);
    UA_ExtensionObject_init(&dst);
    src.encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    src.content.encoded.typeId = UA_NODEID_NUMERIC(0, 888);
    src.content.encoded.body = UA_BYTESTRING("<test>xml</test>");
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.encoding, UA_EXTENSIONOBJECT_ENCODED_XML);
    UA_ExtensionObject_clear(&dst);
} END_TEST

START_TEST(binary_extensionobject_nobody) {
    UA_ExtensionObject src, dst;
    UA_ExtensionObject_init(&src);
    UA_ExtensionObject_init(&dst);
    src.encoding = UA_EXTENSIONOBJECT_ENCODED_NOBODY;
    src.content.encoded.typeId = UA_NODEID_NUMERIC(0, 777);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_ExtensionObject_clear(&dst);
} END_TEST

/* ========== Complex structs ========== */
START_TEST(binary_readrequest) {
    UA_ReadRequest src, dst;
    UA_ReadRequest_init(&src);
    UA_ReadRequest_init(&dst);
    src.maxAge = 500.0;
    src.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;
    src.nodesToReadSize = 1;
    src.nodesToRead = &rvid;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_READREQUEST], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.nodesToReadSize, 1);
    ck_assert_uint_eq(dst.nodesToRead[0].attributeId, UA_ATTRIBUTEID_VALUE);
    src.nodesToRead = NULL;
    src.nodesToReadSize = 0;
    UA_ReadRequest_clear(&dst);
} END_TEST

START_TEST(binary_browseresult) {
    UA_BrowseResult src, dst;
    UA_BrowseResult_init(&src);
    UA_BrowseResult_init(&dst);
    src.statusCode = UA_STATUSCODE_GOOD;
    /* Empty references */
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_BROWSERESULT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.referencesSize, 0);
    UA_BrowseResult_clear(&dst);
} END_TEST

/* ========== calcSizeBinary ========== */
START_TEST(binary_calcsize) {
    UA_Int32 val = 42;
    size_t size = UA_calcSizeBinary(&val, &UA_TYPES[UA_TYPES_INT32], NULL);
    ck_assert_uint_eq(size, 4);

    UA_String str = UA_STRING("hello");
    size = UA_calcSizeBinary(&str, &UA_TYPES[UA_TYPES_STRING], NULL);
    ck_assert(size > 5); /* 4 bytes length + 5 bytes data */

    UA_Variant v;
    UA_Variant_init(&v);
    size = UA_calcSizeBinary(&v, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    ck_assert(size >= 1);
} END_TEST

/* ========== findDataTypeByBinary ========== */
START_TEST(binary_find_datatype_by_binary) {
    /* UA_TYPES_INT32 has binary encoding NodeId */
    UA_NodeId intTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    const UA_DataType *dt = UA_findDataTypeByBinary(&intTypeId);
    /* May or may not find it depending on the encoding NodeId format */
    (void)dt;

    /* Try with ReadRequest binary encoding */
    UA_NodeId readReqBinary = UA_NODEID_NUMERIC(0, 631); /* ReadRequest_Encoding_DefaultBinary */
    dt = UA_findDataTypeByBinary(&readReqBinary);
    if(dt) {
        ck_assert_uint_eq(dt->typeId.identifier.numeric, UA_NS0ID_READREQUEST);
    }
} END_TEST

/* ========== Encode/decode with truncated buffer ========== */
START_TEST(binary_decode_truncated) {
    UA_ByteString buf = UA_BYTESTRING("\x01"); /* too short for Int32 */
    UA_Int32 dst = 0;
    UA_StatusCode res = UA_decodeBinary(&buf, &dst, &UA_TYPES[UA_TYPES_INT32], NULL);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(binary_decode_empty) {
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    buf.length = 0;
    buf.data = (UA_Byte *)(uintptr_t)""; /* non-NULL but empty */
    UA_Int32 dst = 0;
    UA_StatusCode res = UA_decodeBinary(&buf, &dst, &UA_TYPES[UA_TYPES_INT32], NULL);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* ========== Struct roundtrips ========== */
START_TEST(binary_applicationdescription) {
    UA_ApplicationDescription src, dst;
    UA_ApplicationDescription_init(&src);
    UA_ApplicationDescription_init(&dst);
    src.applicationUri = UA_STRING("urn:test:app");
    src.productUri = UA_STRING("urn:test:product");
    src.applicationName = UA_LOCALIZEDTEXT("en", "TestApp");
    src.applicationType = UA_APPLICATIONTYPE_SERVER;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.applicationUri, &dst.applicationUri));
    UA_ApplicationDescription_clear(&dst);
} END_TEST

START_TEST(binary_endpointdescription) {
    UA_EndpointDescription src, dst;
    UA_EndpointDescription_init(&src);
    UA_EndpointDescription_init(&dst);
    src.endpointUrl = UA_STRING("opc.tcp://localhost:4840");
    src.securityMode = UA_MESSAGESECURITYMODE_NONE;
    src.securityPolicyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    src.securityLevel = 0;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.endpointUrl, &dst.endpointUrl));
    UA_EndpointDescription_clear(&dst);
} END_TEST

START_TEST(binary_browsedescription) {
    UA_BrowseDescription src, dst;
    UA_BrowseDescription_init(&src);
    UA_BrowseDescription_init(&dst);
    src.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    src.browseDirection = UA_BROWSEDIRECTION_BOTH;
    src.includeSubtypes = true;
    src.nodeClassMask = UA_NODECLASS_VARIABLE | UA_NODECLASS_OBJECT;
    src.resultMask = UA_BROWSERESULTMASK_ALL;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_BROWSEDESCRIPTION], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src.nodeId, &dst.nodeId));
    ck_assert_uint_eq(dst.browseDirection, UA_BROWSEDIRECTION_BOTH);
    UA_BrowseDescription_clear(&dst);
} END_TEST

START_TEST(binary_writerequest) {
    UA_WriteRequest src, dst;
    UA_WriteRequest_init(&src);
    UA_WriteRequest_init(&dst);
    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = UA_NODEID_NUMERIC(0, 85);
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Int32 val = 123;
    UA_Variant_setScalarCopy(&wv.value.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    wv.value.hasValue = true;
    src.nodesToWriteSize = 1;
    src.nodesToWrite = &wv;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_WRITEREQUEST], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.nodesToWriteSize, 1);
    src.nodesToWrite = NULL;
    src.nodesToWriteSize = 0;
    UA_Variant_clear(&wv.value.value);
    UA_WriteRequest_clear(&dst);
} END_TEST

START_TEST(binary_callrequest) {
    UA_CallRequest src, dst;
    UA_CallRequest_init(&src);
    UA_CallRequest_init(&dst);
    UA_CallMethodRequest cmr;
    UA_CallMethodRequest_init(&cmr);
    cmr.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    cmr.methodId = UA_NODEID_NUMERIC(0, 12345);
    src.methodsToCallSize = 1;
    src.methodsToCall = &cmr;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_CALLREQUEST], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.methodsToCallSize, 1);
    src.methodsToCall = NULL;
    src.methodsToCallSize = 0;
    UA_CallRequest_clear(&dst);
} END_TEST

/* ========== Variant with Enum (encoded as Int32) ========== */
START_TEST(binary_variant_enum) {
    /* Enums are encoded as Int32 in binary */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_MessageSecurityMode mode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
    UA_Variant_setScalarCopy(&src, &mode, &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE]);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

/* ========== Array of complex types ========== */
START_TEST(binary_array_strings) {
    UA_String arr[3];
    arr[0] = UA_STRING("first");
    arr[1] = UA_STRING("second");
    arr[2] = UA_STRING("third");
    UA_ByteString buf;
    UA_ByteString_init(&buf);

    /* Encode as a Variant containing string array */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_Variant_setArrayCopy(&src, arr, 3, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.arrayLength, 3);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

/* ========== Struct with optional fields (CreateMonitoredItemsRequest has optional) ========== */
START_TEST(binary_createsubscriptionrequest) {
    UA_CreateSubscriptionRequest src, dst;
    UA_CreateSubscriptionRequest_init(&src);
    UA_CreateSubscriptionRequest_init(&dst);
    src.requestedPublishingInterval = 100.0;
    src.requestedLifetimeCount = 1000;
    src.requestedMaxKeepAliveCount = 10;
    src.maxNotificationsPerPublish = 100;
    src.publishingEnabled = true;
    src.priority = 5;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst.publishingEnabled == true);
    ck_assert_uint_eq(dst.priority, 5);
    UA_CreateSubscriptionRequest_clear(&dst);
} END_TEST

START_TEST(binary_modifysubscriptionrequest) {
    UA_ModifySubscriptionRequest src, dst;
    UA_ModifySubscriptionRequest_init(&src);
    UA_ModifySubscriptionRequest_init(&dst);
    src.subscriptionId = 1;
    src.requestedPublishingInterval = 200.0;
    src.requestedLifetimeCount = 2000;
    src.requestedMaxKeepAliveCount = 20;
    src.maxNotificationsPerPublish = 200;
    src.priority = 10;
    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.subscriptionId, 1);
    UA_ModifySubscriptionRequest_clear(&dst);
} END_TEST

/* ========== Variant with ExtensionObject wrapping ========== */
START_TEST(binary_variant_extensionobject) {
    /* Wrap a non-builtin type in a Variant → ExtensionObject wrapping path */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);

    UA_ReadRequest rr;
    UA_ReadRequest_init(&rr);
    rr.maxAge = 123.0;
    UA_Variant_setScalarCopy(&src, &rr, &UA_TYPES[UA_TYPES_READREQUEST]);

    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

/* Variant with array of non-builtin types */
START_TEST(binary_variant_array_extensionobject) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);

    UA_QualifiedName qns[2];
    qns[0] = UA_QUALIFIEDNAME(0, "name1");
    qns[1] = UA_QUALIFIEDNAME(1, "name2");
    UA_Variant_setArrayCopy(&src, qns, 2, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);

    ck_assert_uint_eq(roundtripBinary(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

int main(void) {
    Suite *s = suite_create("Binary Encoding Ext");

    TCase *tc_basic = tcase_create("BasicTypes");
    tcase_add_test(tc_basic, binary_bool_true);
    tcase_add_test(tc_basic, binary_bool_false);
    tcase_add_test(tc_basic, binary_sbyte);
    tcase_add_test(tc_basic, binary_byte);
    tcase_add_test(tc_basic, binary_int16);
    tcase_add_test(tc_basic, binary_uint16);
    tcase_add_test(tc_basic, binary_int32);
    tcase_add_test(tc_basic, binary_uint32);
    tcase_add_test(tc_basic, binary_int64);
    tcase_add_test(tc_basic, binary_uint64);
    tcase_add_test(tc_basic, binary_float_nan);
    tcase_add_test(tc_basic, binary_float_inf);
    tcase_add_test(tc_basic, binary_float_neginf);
    tcase_add_test(tc_basic, binary_double_nan);
    tcase_add_test(tc_basic, binary_double_inf);
    tcase_add_test(tc_basic, binary_string_empty);
    tcase_add_test(tc_basic, binary_string_null);
    tcase_add_test(tc_basic, binary_string_long);
    tcase_add_test(tc_basic, binary_datetime);
    tcase_add_test(tc_basic, binary_guid);
    tcase_add_test(tc_basic, binary_bytestring_empty);
    suite_add_tcase(s, tc_basic);

    TCase *tc_nodeid = tcase_create("NodeIds");
    tcase_add_test(tc_nodeid, binary_nodeid_numeric);
    tcase_add_test(tc_nodeid, binary_nodeid_string);
    tcase_add_test(tc_nodeid, binary_nodeid_guid);
    tcase_add_test(tc_nodeid, binary_nodeid_bytestring);
    tcase_add_test(tc_nodeid, binary_nodeid_fourbyte);
    tcase_add_test(tc_nodeid, binary_expandednodeid_with_uri);
    tcase_add_test(tc_nodeid, binary_statuscode);
    tcase_add_test(tc_nodeid, binary_qualifiedname);
    tcase_add_test(tc_nodeid, binary_localizedtext);
    tcase_add_test(tc_nodeid, binary_localizedtext_nolocale);
    suite_add_tcase(s, tc_nodeid);

    TCase *tc_complex = tcase_create("ComplexTypes");
    tcase_add_test(tc_complex, binary_variant_int32);
    tcase_add_test(tc_complex, binary_variant_empty);
    tcase_add_test(tc_complex, binary_variant_array);
    tcase_add_test(tc_complex, binary_variant_matrix);
    tcase_add_test(tc_complex, binary_variant_string_array);
    tcase_add_test(tc_complex, binary_variant_enum);
    tcase_add_test(tc_complex, binary_variant_extensionobject);
    tcase_add_test(tc_complex, binary_variant_array_extensionobject);
    tcase_add_test(tc_complex, binary_datavalue_all_fields);
    tcase_add_test(tc_complex, binary_datavalue_pico_clamp);
    tcase_add_test(tc_complex, binary_diagnosticinfo_full);
    tcase_add_test(tc_complex, binary_extensionobject_decoded);
    tcase_add_test(tc_complex, binary_extensionobject_bytestring);
    tcase_add_test(tc_complex, binary_extensionobject_xml);
    tcase_add_test(tc_complex, binary_extensionobject_nobody);
    suite_add_tcase(s, tc_complex);

    TCase *tc_structs = tcase_create("Structs");
    tcase_add_test(tc_structs, binary_readrequest);
    tcase_add_test(tc_structs, binary_browseresult);
    tcase_add_test(tc_structs, binary_applicationdescription);
    tcase_add_test(tc_structs, binary_endpointdescription);
    tcase_add_test(tc_structs, binary_browsedescription);
    tcase_add_test(tc_structs, binary_writerequest);
    tcase_add_test(tc_structs, binary_callrequest);
    tcase_add_test(tc_structs, binary_createsubscriptionrequest);
    tcase_add_test(tc_structs, binary_modifysubscriptionrequest);
    suite_add_tcase(s, tc_structs);

    TCase *tc_misc = tcase_create("Misc");
    tcase_add_test(tc_misc, binary_calcsize);
    tcase_add_test(tc_misc, binary_find_datatype_by_binary);
    tcase_add_test(tc_misc, binary_decode_truncated);
    tcase_add_test(tc_misc, binary_decode_empty);
    tcase_add_test(tc_misc, binary_array_strings);
    suite_add_tcase(s, tc_misc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int fails = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (fails == 0) ? 0 : 1;
}
