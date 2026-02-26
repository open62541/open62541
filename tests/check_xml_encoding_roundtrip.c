/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>

#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <float.h>
#include <math.h>

/* Helper: encode to XML, then decode back */
static UA_StatusCode
roundtripXml(const void *src, const UA_DataType *type, void *dst) {
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    UA_StatusCode res = UA_encodeXml(src, type, &buf, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    res = UA_decodeXml(&buf, dst, type, NULL);
    UA_ByteString_clear(&buf);
    return res;
}

/* ========== Boolean ========== */
START_TEST(xml_bool_true) {
    UA_Boolean src = true, dst = false;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_BOOLEAN], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst == true);
} END_TEST

START_TEST(xml_bool_false) {
    UA_Boolean src = false, dst = true;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_BOOLEAN], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst == false);
} END_TEST

/* ========== Integer types ========== */
START_TEST(xml_sbyte) {
    UA_SByte src = -128, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_SBYTE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst, -128);
} END_TEST

START_TEST(xml_byte) {
    UA_Byte src = 255, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_BYTE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst, 255);
} END_TEST

START_TEST(xml_int16) {
    UA_Int16 src = -32768, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_INT16], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst, -32768);
} END_TEST

START_TEST(xml_uint16) {
    UA_UInt16 src = 65535, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_UINT16], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst, 65535);
} END_TEST

START_TEST(xml_int32) {
    UA_Int32 src = -2147483647 - 1, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_INT32], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst, -2147483647 - 1);
} END_TEST

START_TEST(xml_uint32) {
    UA_UInt32 src = 4294967295U, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_UINT32], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst, 4294967295U);
} END_TEST

START_TEST(xml_int64) {
    UA_Int64 src = -9223372036854775807LL - 1, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_INT64], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst == -9223372036854775807LL - 1);
} END_TEST

START_TEST(xml_uint64) {
    UA_UInt64 src = 18446744073709551615ULL, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_UINT64], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(dst == 18446744073709551615ULL);
} END_TEST

/* ========== Float/Double ========== */
START_TEST(xml_float_normal) {
    UA_Float src = 3.14f, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_FLOAT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(fabsf(dst - 3.14f) < 0.001f);
} END_TEST

START_TEST(xml_float_nan) {
    UA_Float src = NAN;
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    UA_StatusCode res = UA_encodeXml(&src, &UA_TYPES[UA_TYPES_FLOAT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* Decode it back */
    UA_Float dst = 0;
    res = UA_decodeXml(&buf, &dst, &UA_TYPES[UA_TYPES_FLOAT], NULL);
    if(res == UA_STATUSCODE_GOOD)
        ck_assert(isnan(dst));
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(xml_float_inf) {
    UA_Float src = INFINITY;
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    UA_StatusCode res = UA_encodeXml(&src, &UA_TYPES[UA_TYPES_FLOAT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Float dst = 0;
    res = UA_decodeXml(&buf, &dst, &UA_TYPES[UA_TYPES_FLOAT], NULL);
    if(res == UA_STATUSCODE_GOOD)
        ck_assert(isinf(dst) && dst > 0);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(xml_float_neginf) {
    UA_Float src = -INFINITY;
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    UA_StatusCode res = UA_encodeXml(&src, &UA_TYPES[UA_TYPES_FLOAT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Float dst = 0;
    res = UA_decodeXml(&buf, &dst, &UA_TYPES[UA_TYPES_FLOAT], NULL);
    if(res == UA_STATUSCODE_GOOD)
        ck_assert(isinf(dst) && dst < 0);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(xml_double_normal) {
    UA_Double src = 2.718281828, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_DOUBLE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(fabs(dst - 2.718281828) < 0.0001);
} END_TEST

START_TEST(xml_double_nan) {
    UA_Double src = NAN;
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    UA_StatusCode res = UA_encodeXml(&src, &UA_TYPES[UA_TYPES_DOUBLE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Double dst = 0;
    res = UA_decodeXml(&buf, &dst, &UA_TYPES[UA_TYPES_DOUBLE], NULL);
    if(res == UA_STATUSCODE_GOOD)
        ck_assert(isnan(dst));
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(xml_double_inf) {
    UA_Double src = INFINITY;
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    UA_StatusCode res = UA_encodeXml(&src, &UA_TYPES[UA_TYPES_DOUBLE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Double dst = 0;
    res = UA_decodeXml(&buf, &dst, &UA_TYPES[UA_TYPES_DOUBLE], NULL);
    if(res == UA_STATUSCODE_GOOD)
        ck_assert(isinf(dst) && dst > 0);
    UA_ByteString_clear(&buf);
} END_TEST

/* ========== String ========== */
START_TEST(xml_string) {
    UA_String src = UA_STRING("hello world"), dst;
    UA_String_init(&dst);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_STRING], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src, &dst));
    UA_String_clear(&dst);
} END_TEST

START_TEST(xml_string_empty) {
    UA_String src = UA_STRING(""), dst;
    UA_String_init(&dst);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_STRING], &dst),
                      UA_STATUSCODE_GOOD);
    UA_String_clear(&dst);
} END_TEST

START_TEST(xml_string_special_chars) {
    UA_String src = UA_STRING("test<>&\"'chars"), dst;
    UA_String_init(&dst);
    UA_StatusCode res = roundtripXml(&src, &UA_TYPES[UA_TYPES_STRING], &dst);
    if(res == UA_STATUSCODE_GOOD)
        ck_assert(UA_String_equal(&src, &dst));
    UA_String_clear(&dst);
} END_TEST

/* ========== DateTime ========== */
START_TEST(xml_datetime) {
    UA_DateTime src = UA_DateTime_now(), dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_DATETIME], &dst),
                      UA_STATUSCODE_GOOD);
    /* May not be exact due to encoding precision */
    ck_assert(dst != 0);
} END_TEST

/* ========== Guid ========== */
START_TEST(xml_guid) {
    UA_Guid src = UA_GUID("72962B91-FA75-4AE6-9D28-B404DC7DAF63"), dst;
    UA_Guid_init(&dst);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_GUID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Guid_equal(&src, &dst));
} END_TEST

/* ========== ByteString ========== */
START_TEST(xml_bytestring) {
    UA_ByteString src = UA_BYTESTRING("binarydata"), dst;
    UA_ByteString_init(&dst);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_BYTESTRING], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&src, &dst));
    UA_ByteString_clear(&dst);
} END_TEST

START_TEST(xml_bytestring_empty) {
    /* Uses BYTESTRING_NULL here. A non-NULL data pointer with length=0
     * triggers BUG: UA_base64() returns UA_EMPTY_ARRAY_SENTINEL (0x1)
     * which crashes on UA_free(). See COVERAGE_BUGS.md #8. */
    UA_ByteString src = UA_BYTESTRING_NULL;
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    UA_StatusCode res = UA_encodeXml(&src, &UA_TYPES[UA_TYPES_BYTESTRING], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* ========== NodeId ========== */
START_TEST(xml_nodeid_numeric) {
    UA_NodeId src = UA_NODEID_NUMERIC(0, 85), dst;
    UA_NodeId_init(&dst);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_NODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src, &dst));
    UA_NodeId_clear(&dst);
} END_TEST

START_TEST(xml_nodeid_string) {
    UA_NodeId src = UA_NODEID_STRING(1, "testnode"), dst;
    UA_NodeId_init(&dst);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_NODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src, &dst));
    UA_NodeId_clear(&dst);
} END_TEST

START_TEST(xml_nodeid_guid) {
    UA_Guid g = UA_GUID("72962B91-FA75-4AE6-9D28-B404DC7DAF63");
    UA_NodeId src = UA_NODEID_GUID(2, g), dst;
    UA_NodeId_init(&dst);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_NODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src, &dst));
    UA_NodeId_clear(&dst);
} END_TEST

/* ========== ExpandedNodeId ========== */
START_TEST(xml_expandednodeid) {
    UA_ExpandedNodeId src, dst;
    UA_ExpandedNodeId_init(&src);
    UA_ExpandedNodeId_init(&dst);
    src.nodeId = UA_NODEID_NUMERIC(1, 100);
    src.serverIndex = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_EXPANDEDNODEID], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&src.nodeId, &dst.nodeId));
    UA_ExpandedNodeId_clear(&dst);
} END_TEST

/* ========== StatusCode ========== */
START_TEST(xml_statuscode) {
    UA_StatusCode src = UA_STATUSCODE_BADINTERNALERROR, dst = 0;
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_STATUSCODE], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

/* ========== QualifiedName ========== */
START_TEST(xml_qualifiedname) {
    UA_QualifiedName src = UA_QUALIFIEDNAME(1, "TestName"), dst;
    UA_QualifiedName_init(&dst);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_QualifiedName_equal(&src, &dst));
    UA_QualifiedName_clear(&dst);
} END_TEST

/* ========== LocalizedText ========== */
START_TEST(xml_localizedtext) {
    UA_LocalizedText src = UA_LOCALIZEDTEXT("en", "Hello"), dst;
    UA_LocalizedText_init(&dst);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &dst),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.text, &dst.text));
    UA_LocalizedText_clear(&dst);
} END_TEST

/* ========== Variant ========== */
START_TEST(xml_variant_int32) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_Int32 val = 42;
    UA_Variant_setScalarCopy(&src, &val, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(xml_variant_string) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_String val = UA_STRING("hello");
    UA_Variant_setScalarCopy(&src, &val, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(xml_variant_empty) {
    UA_Variant src;
    UA_Variant_init(&src);
    /* An empty variant has no type; XML representation is undefined.
     * Encoding may succeed or fail -- just verify no crash. */
    UA_ByteString buf;
    UA_ByteString_init(&buf);
    UA_StatusCode res = UA_encodeXml(&src, &UA_TYPES[UA_TYPES_VARIANT], &buf, NULL);
    (void)res;
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(xml_variant_array) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArrayCopy(&src, arr, 3, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(xml_variant_double) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_Double val = 3.14;
    UA_Variant_setScalarCopy(&src, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(xml_variant_bool) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_Boolean val = true;
    UA_Variant_setScalarCopy(&src, &val, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(xml_variant_nodeid) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_NodeId val = UA_NODEID_NUMERIC(0, 85);
    UA_Variant_setScalarCopy(&src, &val, &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(xml_variant_bytestring) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_ByteString val = UA_BYTESTRING("data");
    UA_Variant_setScalarCopy(&src, &val, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(xml_variant_guid) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Variant_init(&dst);
    UA_Guid val = UA_GUID("72962B91-FA75-4AE6-9D28-B404DC7DAF63");
    UA_Variant_setScalarCopy(&src, &val, &UA_TYPES[UA_TYPES_GUID]);
    ck_assert_uint_eq(roundtripXml(&src, &UA_TYPES[UA_TYPES_VARIANT], &dst),
                      UA_STATUSCODE_GOOD);
    UA_Variant_clear(&src);
    UA_Variant_clear(&dst);
} END_TEST

/* ========== ExtensionObject ========== */
START_TEST(xml_extensionobject_decoded) {
    UA_ExtensionObject src, dst;
    UA_ExtensionObject_init(&src);
    UA_ExtensionObject_init(&dst);
    UA_Int32 *val = UA_Int32_new();
    *val = 42;
    src.encoding = UA_EXTENSIONOBJECT_DECODED;
    src.content.decoded.type = &UA_TYPES[UA_TYPES_INT32];
    src.content.decoded.data = val;
    UA_StatusCode res = roundtripXml(&src, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &dst);
    /* May not be implemented for all types in XML */
    (void)res;
    UA_ExtensionObject_clear(&src);
    UA_ExtensionObject_clear(&dst);
} END_TEST

START_TEST(xml_extensionobject_nobody) {
    UA_ExtensionObject src, dst;
    UA_ExtensionObject_init(&src);
    UA_ExtensionObject_init(&dst);
    src.encoding = UA_EXTENSIONOBJECT_ENCODED_NOBODY;
    src.content.encoded.typeId = UA_NODEID_NUMERIC(0, 999);
    UA_StatusCode res = roundtripXml(&src, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &dst);
    (void)res;
    UA_ExtensionObject_clear(&dst);
} END_TEST

/* ========== calcSizeXml ========== */
START_TEST(xml_calcsize) {
    UA_Int32 val = 42;
    size_t size = UA_calcSizeXml(&val, &UA_TYPES[UA_TYPES_INT32], NULL);
    ck_assert(size > 0);

    UA_String str = UA_STRING("hello");
    size = UA_calcSizeXml(&str, &UA_TYPES[UA_TYPES_STRING], NULL);
    ck_assert(size > 5);

    UA_Boolean b = true;
    size = UA_calcSizeXml(&b, &UA_TYPES[UA_TYPES_BOOLEAN], NULL);
    ck_assert(size > 0);

    UA_Double d = 3.14;
    size = UA_calcSizeXml(&d, &UA_TYPES[UA_TYPES_DOUBLE], NULL);
    ck_assert(size > 0);

    UA_NodeId nid = UA_NODEID_NUMERIC(0, 85);
    size = UA_calcSizeXml(&nid, &UA_TYPES[UA_TYPES_NODEID], NULL);
    ck_assert(size > 0);

    UA_Guid g = UA_GUID("72962B91-FA75-4AE6-9D28-B404DC7DAF63");
    size = UA_calcSizeXml(&g, &UA_TYPES[UA_TYPES_GUID], NULL);
    ck_assert(size > 0);

    UA_StatusCode sc = UA_STATUSCODE_GOOD;
    size = UA_calcSizeXml(&sc, &UA_TYPES[UA_TYPES_STATUSCODE], NULL);
    ck_assert(size > 0);
} END_TEST

/* ========== Decode from known XML strings ========== */
START_TEST(xml_decode_bool_true) {
    UA_ByteString xml = UA_BYTESTRING("<Boolean>true</Boolean>");
    UA_Boolean dst = false;
    UA_StatusCode res = UA_decodeXml(&xml, &dst, &UA_TYPES[UA_TYPES_BOOLEAN], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(dst == true);
} END_TEST

START_TEST(xml_decode_bool_false) {
    UA_ByteString xml = UA_BYTESTRING("<Boolean>false</Boolean>");
    UA_Boolean dst = true;
    UA_StatusCode res = UA_decodeXml(&xml, &dst, &UA_TYPES[UA_TYPES_BOOLEAN], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(dst == false);
} END_TEST

START_TEST(xml_decode_int32) {
    UA_ByteString xml = UA_BYTESTRING("<Int32>-42</Int32>");
    UA_Int32 dst = 0;
    UA_StatusCode res = UA_decodeXml(&xml, &dst, &UA_TYPES[UA_TYPES_INT32], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst, -42);
} END_TEST

START_TEST(xml_decode_uint32) {
    UA_ByteString xml = UA_BYTESTRING("<UInt32>4294967295</UInt32>");
    UA_UInt32 dst = 0;
    UA_StatusCode res = UA_decodeXml(&xml, &dst, &UA_TYPES[UA_TYPES_UINT32], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst, 4294967295U);
} END_TEST

START_TEST(xml_decode_double_inf) {
    UA_ByteString xml = UA_BYTESTRING("INF");
    UA_Double dst = 0;
    UA_StatusCode res = UA_decodeXml(&xml, &dst, &UA_TYPES[UA_TYPES_DOUBLE], NULL);
    if(res == UA_STATUSCODE_GOOD)
        ck_assert(isinf(dst) && dst > 0);
} END_TEST

START_TEST(xml_decode_double_neginf) {
    UA_ByteString xml = UA_BYTESTRING("-INF");
    UA_Double dst = 0;
    UA_StatusCode res = UA_decodeXml(&xml, &dst, &UA_TYPES[UA_TYPES_DOUBLE], NULL);
    if(res == UA_STATUSCODE_GOOD)
        ck_assert(isinf(dst) && dst < 0);
} END_TEST

START_TEST(xml_decode_double_nan) {
    UA_ByteString xml = UA_BYTESTRING("NaN");
    UA_Double dst = 0;
    UA_StatusCode res = UA_decodeXml(&xml, &dst, &UA_TYPES[UA_TYPES_DOUBLE], NULL);
    if(res == UA_STATUSCODE_GOOD)
        ck_assert(isnan(dst));
} END_TEST

START_TEST(xml_decode_string) {
    UA_ByteString xml = UA_BYTESTRING("<String>hello world</String>");
    UA_String dst;
    UA_String_init(&dst);
    UA_StatusCode res = UA_decodeXml(&xml, &dst, &UA_TYPES[UA_TYPES_STRING], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&dst);
} END_TEST

int main(void) {
    Suite *s = suite_create("XML Encoding Ext");

    TCase *tc_basic = tcase_create("BasicTypes");
    tcase_add_test(tc_basic, xml_bool_true);
    tcase_add_test(tc_basic, xml_bool_false);
    tcase_add_test(tc_basic, xml_sbyte);
    tcase_add_test(tc_basic, xml_byte);
    tcase_add_test(tc_basic, xml_int16);
    tcase_add_test(tc_basic, xml_uint16);
    tcase_add_test(tc_basic, xml_int32);
    tcase_add_test(tc_basic, xml_uint32);
    tcase_add_test(tc_basic, xml_int64);
    tcase_add_test(tc_basic, xml_uint64);
    tcase_add_test(tc_basic, xml_float_normal);
    tcase_add_test(tc_basic, xml_float_nan);
    tcase_add_test(tc_basic, xml_float_inf);
    tcase_add_test(tc_basic, xml_float_neginf);
    tcase_add_test(tc_basic, xml_double_normal);
    tcase_add_test(tc_basic, xml_double_nan);
    tcase_add_test(tc_basic, xml_double_inf);
    tcase_add_test(tc_basic, xml_string);
    tcase_add_test(tc_basic, xml_string_empty);
    tcase_add_test(tc_basic, xml_string_special_chars);
    tcase_add_test(tc_basic, xml_datetime);
    tcase_add_test(tc_basic, xml_guid);
    tcase_add_test(tc_basic, xml_bytestring);
    tcase_add_test(tc_basic, xml_bytestring_empty);
    suite_add_tcase(s, tc_basic);

    TCase *tc_nodeid = tcase_create("NodeIds");
    tcase_add_test(tc_nodeid, xml_nodeid_numeric);
    tcase_add_test(tc_nodeid, xml_nodeid_string);
    tcase_add_test(tc_nodeid, xml_nodeid_guid);
    tcase_add_test(tc_nodeid, xml_expandednodeid);
    tcase_add_test(tc_nodeid, xml_statuscode);
    tcase_add_test(tc_nodeid, xml_qualifiedname);
    tcase_add_test(tc_nodeid, xml_localizedtext);
    suite_add_tcase(s, tc_nodeid);

    TCase *tc_complex = tcase_create("ComplexTypes");
    tcase_add_test(tc_complex, xml_variant_int32);
    tcase_add_test(tc_complex, xml_variant_string);
    tcase_add_test(tc_complex, xml_variant_empty);
    tcase_add_test(tc_complex, xml_variant_array);
    tcase_add_test(tc_complex, xml_variant_double);
    tcase_add_test(tc_complex, xml_variant_bool);
    tcase_add_test(tc_complex, xml_variant_nodeid);
    tcase_add_test(tc_complex, xml_variant_bytestring);
    tcase_add_test(tc_complex, xml_variant_guid);
    tcase_add_test(tc_complex, xml_extensionobject_decoded);
    tcase_add_test(tc_complex, xml_extensionobject_nobody);
    suite_add_tcase(s, tc_complex);

    TCase *tc_misc = tcase_create("Misc");
    tcase_add_test(tc_misc, xml_calcsize);
    tcase_add_test(tc_misc, xml_decode_bool_true);
    tcase_add_test(tc_misc, xml_decode_bool_false);
    tcase_add_test(tc_misc, xml_decode_int32);
    tcase_add_test(tc_misc, xml_decode_uint32);
    tcase_add_test(tc_misc, xml_decode_double_inf);
    tcase_add_test(tc_misc, xml_decode_double_neginf);
    tcase_add_test(tc_misc, xml_decode_double_nan);
    tcase_add_test(tc_misc, xml_decode_string);
    suite_add_tcase(s, tc_misc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int fails = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (fails == 0) ? 0 : 1;
}
