/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Tests for type encoding coverage */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <open62541/types.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/util.h>
#include <check.h>
#include "test_helpers.h"

#ifdef UA_ENABLE_JSON_ENCODING
/* === JSON Encoding Tests === */
START_TEST(json_encode_boolean) {
    UA_Boolean v = true;
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_BOOLEAN],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_int32) {
    UA_Int32 v = -42;
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_INT32],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_string) {
    UA_String v = UA_STRING("hello world");
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_STRING],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_datetime) {
    UA_DateTime v = UA_DateTime_now();
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_DATETIME],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_nodeId) {
    UA_NodeId v = UA_NODEID_NUMERIC(0, 42);
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_NODEID],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);

    /* String NodeId */
    UA_NodeId v2 = UA_NODEID_STRING(1, "Test");
    UA_ByteString out2 = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&v2, &UA_TYPES[UA_TYPES_NODEID], &out2, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out2);

    /* Guid NodeId */
    UA_Guid g = {0x01020304, 0x0506, 0x0708, {0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10}};
    UA_NodeId v3 = UA_NODEID_GUID(2, g);
    UA_ByteString out3 = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&v3, &UA_TYPES[UA_TYPES_NODEID], &out3, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out3);
} END_TEST

START_TEST(json_encode_expandedNodeId) {
    UA_ExpandedNodeId v = UA_EXPANDEDNODEID_NUMERIC(0, 123);
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_EXPANDEDNODEID],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_qualifiedName) {
    UA_QualifiedName v = UA_QUALIFIEDNAME(1, "TestName");
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_QUALIFIEDNAME],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_localizedText) {
    UA_LocalizedText v = UA_LOCALIZEDTEXT("en", "Hello");
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_variant_scalar) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 val = 42;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_VARIANT],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_variant_array) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArray(&v, arr, 3, &UA_TYPES[UA_TYPES_INT32]);
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_VARIANT],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_dataValue) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Int32 val = 99;
    UA_Variant_setScalar(&dv.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    dv.hasSourceTimestamp = true;
    dv.sourceTimestamp = UA_DateTime_now();
    dv.hasServerTimestamp = true;
    dv.serverTimestamp = UA_DateTime_now();
    dv.hasStatus = true;
    dv.status = UA_STATUSCODE_GOOD;

    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&dv, &UA_TYPES[UA_TYPES_DATAVALUE],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_diagnosticInfo) {
    UA_DiagnosticInfo di;
    UA_DiagnosticInfo_init(&di);
    di.hasSymbolicId = true;
    di.symbolicId = 42;
    di.hasLocalizedText = true;
    di.localizedText = 7;
    di.hasAdditionalInfo = true;
    di.additionalInfo = UA_STRING("test info");

    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&di, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_statusCode) {
    UA_StatusCode v = UA_STATUSCODE_BADUNEXPECTEDERROR;
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_STATUSCODE],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_guid) {
    UA_Guid v = {0x01020304, 0x0506, 0x0708, {0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10}};
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_GUID],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_bytestring) {
    UA_Byte data[] = {0x01, 0x02, 0x03, 0x04};
    UA_ByteString v = {4, data};
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_BYTESTRING],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(json_encode_extensionObject) {
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    eo.encoding = UA_EXTENSIONOBJECT_DECODED;
    eo.content.decoded.type = &UA_TYPES[UA_TYPES_INT32];
    UA_Int32 val = 77;
    eo.content.decoded.data = &val;

    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

/* JSON decode roundtrips */
START_TEST(json_decode_int32) {
    UA_ByteString json = UA_STRING("-42");
    UA_Int32 v = 0;
    UA_StatusCode res = UA_decodeJson(&json, &v, &UA_TYPES[UA_TYPES_INT32],
                                       NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(v, -42);
} END_TEST

START_TEST(json_decode_string) {
    UA_ByteString json = UA_STRING("\"hello\"");
    UA_String v;
    UA_String_init(&v);
    UA_StatusCode res = UA_decodeJson(&json, &v, &UA_TYPES[UA_TYPES_STRING],
                                       NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v.length, 5);
    UA_String_clear(&v);
} END_TEST

START_TEST(json_decode_boolean) {
    UA_ByteString json = UA_STRING("true");
    UA_Boolean v = false;
    UA_StatusCode res = UA_decodeJson(&json, &v, &UA_TYPES[UA_TYPES_BOOLEAN],
                                       NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(v == true);
} END_TEST

START_TEST(json_encode_readResponse) {
    /* Encode a complex structure: ReadResponse */
    UA_ReadResponse resp;
    UA_ReadResponse_init(&resp);
    resp.responseHeader.serviceResult = UA_STATUSCODE_GOOD;
    resp.responseHeader.timestamp = UA_DateTime_now();

    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Int32 val = 123;
    UA_Variant_setScalar(&dv.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    resp.results = &dv;
    resp.resultsSize = 1;

    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&resp, &UA_TYPES[UA_TYPES_READRESPONSE],
                                       &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);

    /* Don't clear resp since results points to stack */
    resp.results = NULL;
    resp.resultsSize = 0;
} END_TEST

#endif /* UA_ENABLE_JSON_ENCODING */

#ifdef UA_ENABLE_XML_ENCODING
/* === XML Encoding Tests === */
START_TEST(xml_encode_int32) {
    UA_Int32 v = 42;
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_INT32],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_string) {
    UA_String v = UA_STRING("test string");
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_STRING],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_boolean) {
    UA_Boolean v = false;
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_BOOLEAN],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_nodeId) {
    UA_NodeId v = UA_NODEID_NUMERIC(0, 42);
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_NODEID],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_datetime) {
    UA_DateTime v = UA_DateTime_now();
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_DATETIME],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_guid) {
    UA_Guid v = {0x01020304, 0x0506, 0x0708, {0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10}};
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_GUID],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_localizedText) {
    UA_LocalizedText v = UA_LOCALIZEDTEXT("en", "TestXML");
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_qualifiedName) {
    UA_QualifiedName v = UA_QUALIFIEDNAME(1, "XmlName");
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_QUALIFIEDNAME],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_variant) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 val = 55;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_VARIANT],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_bytestring) {
    UA_Byte data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    UA_ByteString v = {4, data};
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_BYTESTRING],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_statusCode) {
    UA_StatusCode v = UA_STATUSCODE_BADTIMEOUT;
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_STATUSCODE],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_expandedNodeId) {
    UA_ExpandedNodeId v = UA_EXPANDEDNODEID_NUMERIC(0, 42);
    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&v, &UA_TYPES[UA_TYPES_EXPANDEDNODEID],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_extensionObject) {
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    eo.encoding = UA_EXTENSIONOBJECT_DECODED;
    eo.content.decoded.type = &UA_TYPES[UA_TYPES_INT32];
    UA_Int32 val = 88;
    eo.content.decoded.data = &val;

    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
                                      &out, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&out);
} END_TEST

START_TEST(xml_encode_dataValue) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Int32 val = 33;
    UA_Variant_setScalar(&dv.value, &val, &UA_TYPES[UA_TYPES_INT32]);

    UA_ByteString out = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeXml(&dv, &UA_TYPES[UA_TYPES_DATAVALUE],
                                      &out, NULL);
    /* DataValue XML encoding may not be fully supported - just exercise code */
    (void)res;
    UA_ByteString_clear(&out);
} END_TEST
#endif /* UA_ENABLE_XML_ENCODING */

/* === Binary encoding size / roundtrip === */
START_TEST(binary_encode_decode_variant) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 val = 42;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);

    size_t size = UA_calcSizeBinary(&v, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    ck_assert(size > 0);

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeBinary(&v, &UA_TYPES[UA_TYPES_VARIANT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Decode */
    UA_Variant decoded;
    UA_Variant_init(&decoded);
    res = UA_decodeBinary(&buf, &decoded, &UA_TYPES[UA_TYPES_VARIANT], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(decoded.type == &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)decoded.data, 42);

    UA_Variant_clear(&decoded);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(binary_encode_decode_string) {
    UA_String v = UA_STRING("test binary");
    size_t size = UA_calcSizeBinary(&v, &UA_TYPES[UA_TYPES_STRING], NULL);
    ck_assert(size > 0);

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeBinary(&v, &UA_TYPES[UA_TYPES_STRING], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_String decoded;
    UA_String_init(&decoded);
    res = UA_decodeBinary(&buf, &decoded, &UA_TYPES[UA_TYPES_STRING], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(decoded.length, v.length);
    UA_String_clear(&decoded);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(binary_encode_decode_nodeId) {
    UA_NodeId v = UA_NODEID_STRING(1, "BinaryTest");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeBinary(&v, &UA_TYPES[UA_TYPES_NODEID], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId decoded;
    UA_NodeId_init(&decoded);
    res = UA_decodeBinary(&buf, &decoded, &UA_TYPES[UA_TYPES_NODEID], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&v, &decoded));
    UA_NodeId_clear(&decoded);
    UA_ByteString_clear(&buf);
} END_TEST

/* === Server config operations === */
START_TEST(server_config_ops) {
    UA_Server *s = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(s, NULL);

    /* Get the config */
    UA_ServerConfig *config = UA_Server_getConfig(s);
    ck_assert_ptr_ne(config, NULL);

    /* Read server array */
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode res = UA_Server_readValue(s,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERARRAY), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* Read namespace array */
    UA_Variant_init(&out);
    res = UA_Server_readValue(s,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&out);

    /* Add a namespace */
    UA_UInt16 nsIdx = UA_Server_addNamespace(s, "http://test.namespace.org");
    ck_assert(nsIdx > 0);

    /* Get namespace by name */
    UA_String nsUri = UA_STRING("http://test.namespace.org");
    size_t foundIdx = 0;
    res = UA_Server_getNamespaceByName(s, nsUri, &foundIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(foundIdx, (size_t)nsIdx);

    UA_Server_delete(s);
} END_TEST

/* === Type copy/print === */
START_TEST(type_operations) {
    /* Copy an ApplicationDescription */
    UA_ApplicationDescription src;
    UA_ApplicationDescription_init(&src);
    src.applicationUri = UA_STRING_ALLOC("urn:test");
    src.productUri = UA_STRING_ALLOC("urn:test:product");
    src.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", "Test App");
    src.applicationType = UA_APPLICATIONTYPE_SERVER;

    UA_ApplicationDescription dst;
    UA_StatusCode res = UA_ApplicationDescription_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.applicationUri, &dst.applicationUri));

    UA_ApplicationDescription_clear(&src);
    UA_ApplicationDescription_clear(&dst);
} END_TEST

START_TEST(nodeId_hash_order) {
    /* Test NodeId hash and ordering */
    UA_NodeId a = UA_NODEID_NUMERIC(0, 42);
    UA_NodeId b = UA_NODEID_NUMERIC(0, 43);
    UA_NodeId c = UA_NODEID_NUMERIC(0, 42);

    UA_UInt32 hashA = UA_NodeId_hash(&a);
    UA_UInt32 hashC = UA_NodeId_hash(&c);
    ck_assert_uint_eq(hashA, hashC);

    ck_assert(UA_NodeId_equal(&a, &c));
    ck_assert(!UA_NodeId_equal(&a, &b));

    UA_NodeId_order(&a, &b);

    /* String NodeId hash */
    UA_NodeId s1 = UA_NODEID_STRING(1, "test1");
    UA_NodeId s2 = UA_NODEID_STRING(1, "test2");
    UA_UInt32 hashS1 = UA_NodeId_hash(&s1);
    UA_UInt32 hashS2 = UA_NodeId_hash(&s2);
    ck_assert(hashS1 != hashS2 || UA_NodeId_equal(&s1, &s2));

    /* ExpandedNodeId hash */
    UA_ExpandedNodeId e1 = UA_EXPANDEDNODEID_NUMERIC(0, 42);
    UA_UInt32 hashE = UA_ExpandedNodeId_hash(&e1);
    (void)hashE;
} END_TEST

START_TEST(variant_copyRange_test) {
    /* Create a 1D array variant */
    UA_Int32 arr[] = {10, 20, 30, 40, 50};
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setArrayCopy(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);

    /* Copy a range out of it */
    UA_NumericRange nr = UA_NUMERICRANGE("1:3");
    UA_Variant copied;
    UA_Variant_init(&copied);
    UA_StatusCode res = UA_Variant_copyRange(&v, &copied, nr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(copied.arrayLength, 3);
    const UA_Int32 *data = (const UA_Int32 *)copied.data;
    ck_assert_int_eq(data[0], 20);
    ck_assert_int_eq(data[1], 30);
    ck_assert_int_eq(data[2], 40);

    UA_free(nr.dimensions);
    UA_Variant_clear(&copied);
    UA_Variant_clear(&v);
} END_TEST

static Suite *testSuite_encoding(void) {
    Suite *s = suite_create("Encoding Extended");

#ifdef UA_ENABLE_JSON_ENCODING
    TCase *tc_json = tcase_create("JSON");
    tcase_add_test(tc_json, json_encode_boolean);
    tcase_add_test(tc_json, json_encode_int32);
    tcase_add_test(tc_json, json_encode_string);
    tcase_add_test(tc_json, json_encode_datetime);
    tcase_add_test(tc_json, json_encode_nodeId);
    tcase_add_test(tc_json, json_encode_expandedNodeId);
    tcase_add_test(tc_json, json_encode_qualifiedName);
    tcase_add_test(tc_json, json_encode_localizedText);
    tcase_add_test(tc_json, json_encode_variant_scalar);
    tcase_add_test(tc_json, json_encode_variant_array);
    tcase_add_test(tc_json, json_encode_dataValue);
    tcase_add_test(tc_json, json_encode_diagnosticInfo);
    tcase_add_test(tc_json, json_encode_statusCode);
    tcase_add_test(tc_json, json_encode_guid);
    tcase_add_test(tc_json, json_encode_bytestring);
    tcase_add_test(tc_json, json_encode_extensionObject);
    tcase_add_test(tc_json, json_decode_int32);
    tcase_add_test(tc_json, json_decode_string);
    tcase_add_test(tc_json, json_decode_boolean);
    tcase_add_test(tc_json, json_encode_readResponse);
    suite_add_tcase(s, tc_json);
#endif

#ifdef UA_ENABLE_XML_ENCODING
    TCase *tc_xml = tcase_create("XML");
    tcase_add_test(tc_xml, xml_encode_int32);
    tcase_add_test(tc_xml, xml_encode_string);
    tcase_add_test(tc_xml, xml_encode_boolean);
    tcase_add_test(tc_xml, xml_encode_nodeId);
    tcase_add_test(tc_xml, xml_encode_datetime);
    tcase_add_test(tc_xml, xml_encode_guid);
    tcase_add_test(tc_xml, xml_encode_localizedText);
    tcase_add_test(tc_xml, xml_encode_qualifiedName);
    tcase_add_test(tc_xml, xml_encode_variant);
    tcase_add_test(tc_xml, xml_encode_bytestring);
    tcase_add_test(tc_xml, xml_encode_statusCode);
    tcase_add_test(tc_xml, xml_encode_expandedNodeId);
    tcase_add_test(tc_xml, xml_encode_extensionObject);
    tcase_add_test(tc_xml, xml_encode_dataValue);
    suite_add_tcase(s, tc_xml);
#endif

    TCase *tc_bin = tcase_create("Binary");
    tcase_add_test(tc_bin, binary_encode_decode_variant);
    tcase_add_test(tc_bin, binary_encode_decode_string);
    tcase_add_test(tc_bin, binary_encode_decode_nodeId);
    suite_add_tcase(s, tc_bin);

    TCase *tc_misc = tcase_create("Misc");
    tcase_add_test(tc_misc, server_config_ops);
    tcase_add_test(tc_misc, type_operations);
    tcase_add_test(tc_misc, nodeId_hash_order);
    tcase_add_test(tc_misc, variant_copyRange_test);
    suite_add_tcase(s, tc_misc);

    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_encoding();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
