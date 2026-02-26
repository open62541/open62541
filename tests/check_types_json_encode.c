/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* JSON encoding tests */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* === Boolean JSON encoding === */
START_TEST(json_encode_boolean_true) {
    UA_Boolean val = true;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_BOOLEAN], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(buf.length > 0);
    ck_assert(memcmp(buf.data, "true", 4) == 0);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_boolean_false) {
    UA_Boolean val = false;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_BOOLEAN], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(memcmp(buf.data, "false", 5) == 0);
    UA_ByteString_clear(&buf);
} END_TEST

/* === Integer JSON encoding === */
START_TEST(json_encode_integers) {
    UA_Byte b = 255;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&b, &UA_TYPES[UA_TYPES_BYTE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    UA_Int16 i16 = -1234;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&i16, &UA_TYPES[UA_TYPES_INT16], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    UA_UInt32 u32 = 4294967295u;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&u32, &UA_TYPES[UA_TYPES_UINT32], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    UA_Int64 i64 = -9223372036854775807LL;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&i64, &UA_TYPES[UA_TYPES_INT64], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    UA_UInt64 u64 = 18446744073709551615ULL;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&u64, &UA_TYPES[UA_TYPES_UINT64], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === Float/Double with special values === */
START_TEST(json_encode_float_nan) {
    UA_Float val = NAN;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_FLOAT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(buf.length > 0);
    /* NaN is encoded as "NaN" in JSON */
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_float_infinity) {
    UA_Float val = INFINITY;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_FLOAT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    val = -INFINITY;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_FLOAT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_double_nan) {
    UA_Double val = NAN;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DOUBLE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_double_infinity) {
    UA_Double val = INFINITY;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DOUBLE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    val = -INFINITY;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DOUBLE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_double_normal) {
    UA_Double val = 3.141592653589793;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DOUBLE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    val = 0.0;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DOUBLE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    val = -0.0;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DOUBLE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === String JSON encoding === */
START_TEST(json_encode_string) {
    UA_String val = UA_STRING("Hello World");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_STRING], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_string_empty) {
    UA_String val = UA_STRING("");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_STRING], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_string_null) {
    UA_String val = UA_STRING_NULL;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_STRING], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_string_escape) {
    /* String with special characters needing escape */
    UA_String val = UA_STRING("ab\"cd\\ef\n\t\r");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_STRING], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* Should contain escaped chars */
    UA_ByteString_clear(&buf);
} END_TEST

/* === DateTime JSON encoding === */
START_TEST(json_encode_datetime) {
    UA_DateTime val = UA_DateTime_now();
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DATETIME], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* Should be ISO 8601 string */
    UA_ByteString_clear(&buf);
} END_TEST

/* === Guid JSON encoding === */
START_TEST(json_encode_guid) {
    UA_Guid val = UA_GUID("09087e75-8e5e-499b-954f-f2a9603db28a");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_GUID], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === ByteString JSON encoding (Base64) === */
START_TEST(json_encode_bytestring) {
    UA_ByteString val = UA_BYTESTRING("Hello");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_BYTESTRING], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_bytestring_null) {
    UA_ByteString val = UA_BYTESTRING_NULL;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_BYTESTRING], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === NodeId JSON encoding === */
START_TEST(json_encode_nodeid_numeric) {
    UA_NodeId val = UA_NODEID_NUMERIC(0, 2255);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_NODEID], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_nodeid_string) {
    UA_NodeId val = UA_NODEID_STRING(1, "TestNode");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_NODEID], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_nodeid_guid) {
    UA_Guid g = UA_GUID("09087e75-8e5e-499b-954f-f2a9603db28a");
    UA_NodeId val = UA_NODEID_GUID(2, g);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_NODEID], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_nodeid_bytestring) {
    UA_ByteString bs = UA_BYTESTRING("test");
    UA_NodeId val;
    val.namespaceIndex = 3;
    val.identifierType = UA_NODEIDTYPE_BYTESTRING;
    val.identifier.byteString = bs;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_NODEID], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === ExpandedNodeId JSON encoding === */
START_TEST(json_encode_expandednodeid) {
    UA_ExpandedNodeId val;
    UA_ExpandedNodeId_init(&val);
    val.nodeId = UA_NODEID_NUMERIC(1, 5555);
    val.serverIndex = 2;
    val.namespaceUri = UA_STRING("urn:test:namespace");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_EXPANDEDNODEID], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === StatusCode JSON encoding === */
START_TEST(json_encode_statuscode) {
    UA_StatusCode val = UA_STATUSCODE_BADNOTFOUND;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_STATUSCODE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    val = UA_STATUSCODE_GOOD;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_STATUSCODE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === QualifiedName JSON encoding === */
START_TEST(json_encode_qualifiedname) {
    UA_QualifiedName val = UA_QUALIFIEDNAME(1, "TestName");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    val = UA_QUALIFIEDNAME(0, "DefaultNs");
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === LocalizedText JSON encoding === */
START_TEST(json_encode_localizedtext) {
    UA_LocalizedText val = UA_LOCALIZEDTEXT("en-US", "Hello World");
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === Variant JSON encoding === */
START_TEST(json_encode_variant_scalar_int) {
    UA_Variant val;
    UA_Variant_init(&val);
    UA_Int32 i = 42;
    UA_Variant_setScalar(&val, &i, &UA_TYPES[UA_TYPES_INT32]);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_VARIANT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_variant_array_int) {
    UA_Variant val;
    UA_Variant_init(&val);
    UA_Int32 arr[] = {1, 2, 3, 4, 5};
    UA_Variant_setArray(&val, arr, 5, &UA_TYPES[UA_TYPES_INT32]);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_VARIANT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_variant_string) {
    UA_Variant val;
    UA_Variant_init(&val);
    UA_String s = UA_STRING("test");
    UA_Variant_setScalar(&val, &s, &UA_TYPES[UA_TYPES_STRING]);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_VARIANT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_variant_empty) {
    UA_Variant val;
    UA_Variant_init(&val);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_VARIANT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === DataValue JSON encoding === */
START_TEST(json_encode_datavalue) {
    UA_DataValue val;
    UA_DataValue_init(&val);
    val.hasValue = true;
    UA_Double d = 3.14;
    UA_Variant_setScalar(&val.value, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    val.hasStatus = true;
    val.status = UA_STATUSCODE_GOOD;
    val.hasSourceTimestamp = true;
    val.sourceTimestamp = UA_DateTime_now();
    val.hasServerTimestamp = true;
    val.serverTimestamp = UA_DateTime_now();
    val.hasSourcePicoseconds = true;
    val.sourcePicoseconds = 100;
    val.hasServerPicoseconds = true;
    val.serverPicoseconds = 200;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DATAVALUE], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === DiagnosticInfo JSON encoding === */
START_TEST(json_encode_diagnosticinfo) {
    UA_DiagnosticInfo val;
    UA_DiagnosticInfo_init(&val);
    val.hasSymbolicId = true;
    val.symbolicId = 1;
    val.hasNamespaceUri = true;
    val.namespaceUri = 2;
    val.hasLocalizedText = true;
    val.localizedText = 3;
    val.hasLocale = true;
    val.locale = 4;
    val.hasAdditionalInfo = true;
    val.additionalInfo = UA_STRING("extra");
    val.hasInnerStatusCode = true;
    val.innerStatusCode = UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === ExtensionObject JSON encoding === */
START_TEST(json_encode_extensionobject_decoded) {
    UA_ExtensionObject val;
    UA_ExtensionObject_init(&val);

    UA_ReadValueId *rvid = UA_ReadValueId_new();
    rvid->nodeId = UA_NODEID_NUMERIC(0, 2255);
    rvid->attributeId = UA_ATTRIBUTEID_VALUE;
    val.encoding = UA_EXTENSIONOBJECT_DECODED;
    val.content.decoded.type = &UA_TYPES[UA_TYPES_READVALUEID];
    val.content.decoded.data = rvid;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    UA_ExtensionObject_clear(&val);
} END_TEST

START_TEST(json_encode_extensionobject_bytestring) {
    UA_ExtensionObject val;
    UA_ExtensionObject_init(&val);
    val.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    val.content.encoded.typeId = UA_NODEID_NUMERIC(0, 999);
    val.content.encoded.body = UA_BYTESTRING("test");

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === Pretty print JSON encoding === */
START_TEST(json_encode_prettyprint) {
    UA_EncodeJsonOptions opts;
    memset(&opts, 0, sizeof(opts));
    opts.prettyPrint = true;

    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_NUMERIC(0, 2255);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&rvid, &UA_TYPES[UA_TYPES_READVALUEID], &buf, &opts);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* Should contain newlines and spaces */
    UA_Boolean hasNewline = false;
    for(size_t i = 0; i < buf.length; i++) {
        if(buf.data[i] == '\n') {
            hasNewline = true;
            break;
        }
    }
    ck_assert(hasNewline);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_prettyprint_array) {
    UA_EncodeJsonOptions opts;
    memset(&opts, 0, sizeof(opts));
    opts.prettyPrint = true;

    /* Array of numeric (primitive) values - "distinct" false path */
    UA_Variant val;
    UA_Variant_init(&val);
    UA_Int32 arr[] = {10, 20, 30};
    UA_Variant_setArray(&val, arr, 3, &UA_TYPES[UA_TYPES_INT32]);

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_VARIANT], &buf, &opts);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    /* Array of complex values (strings) - "distinct" true path */
    UA_String sarr[] = {UA_STRING_STATIC("a"), UA_STRING_STATIC("b"), UA_STRING_STATIC("c")};
    UA_Variant_setArray(&val, sarr, 3, &UA_TYPES[UA_TYPES_STRING]);
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_VARIANT], &buf, &opts);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_unquoted_keys) {
    UA_EncodeJsonOptions opts;
    memset(&opts, 0, sizeof(opts));
    opts.unquotedKeys = true;

    UA_NodeId val = UA_NODEID_NUMERIC(0, 2255);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_NODEID], &buf, &opts);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_string_nodeids) {
    UA_EncodeJsonOptions opts;
    memset(&opts, 0, sizeof(opts));
    opts.stringNodeIds = true;

    UA_NodeId val = UA_NODEID_NUMERIC(0, 2255);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_NODEID], &buf, &opts);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === Complex struct JSON encoding === */
START_TEST(json_encode_browseresult) {
    UA_BrowseResult br;
    UA_BrowseResult_init(&br);
    br.statusCode = UA_STATUSCODE_GOOD;
    br.referencesSize = 1;
    br.references = (UA_ReferenceDescription*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION]);
    br.references[0].nodeId.nodeId = UA_NODEID_NUMERIC(0, 2255);
    br.references[0].browseName = UA_QUALIFIEDNAME_ALLOC(0, "TestBN");
    br.references[0].displayName = UA_LOCALIZEDTEXT_ALLOC("en", "TestDN");
    br.references[0].isForward = true;
    br.references[0].nodeClass = UA_NODECLASS_VARIABLE;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&br, &UA_TYPES[UA_TYPES_BROWSERESULT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
    UA_BrowseResult_clear(&br);
} END_TEST

START_TEST(json_encode_readrequest) {
    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.maxAge = 0;
    req.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    req.nodesToReadSize = 2;
    req.nodesToRead = (UA_ReadValueId*)
        UA_Array_new(2, &UA_TYPES[UA_TYPES_READVALUEID]);
    req.nodesToRead[0].nodeId = UA_NODEID_NUMERIC(0, 2255);
    req.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;
    req.nodesToRead[1].nodeId = UA_NODEID_NUMERIC(0, 2256);
    req.nodesToRead[1].attributeId = UA_ATTRIBUTEID_DISPLAYNAME;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&req, &UA_TYPES[UA_TYPES_READREQUEST], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
    UA_ReadRequest_clear(&req);
} END_TEST

/* === calcSizeJson === */
START_TEST(json_calcsize) {
    UA_Int32 val = 42;
    size_t sz = UA_calcSizeJson(&val, &UA_TYPES[UA_TYPES_INT32], NULL);
    ck_assert(sz > 0);

    UA_String s = UA_STRING("hello");
    sz = UA_calcSizeJson(&s, &UA_TYPES[UA_TYPES_STRING], NULL);
    ck_assert(sz > 0);

    UA_NodeId nid = UA_NODEID_STRING(1, "test");
    sz = UA_calcSizeJson(&nid, &UA_TYPES[UA_TYPES_NODEID], NULL);
    ck_assert(sz > 0);

    /* With pretty print */
    UA_EncodeJsonOptions opts;
    memset(&opts, 0, sizeof(opts));
    opts.prettyPrint = true;
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_NUMERIC(0, 2255);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;
    sz = UA_calcSizeJson(&rvid, &UA_TYPES[UA_TYPES_READVALUEID], &opts);
    ck_assert(sz > 0);
} END_TEST

/* === Variant with 2D array dimensions === */
START_TEST(json_encode_variant_2d_array) {
    UA_Variant val;
    UA_Variant_init(&val);
    UA_Int32 matrix[] = {1, 2, 3, 4, 5, 6};
    UA_Variant_setArray(&val, matrix, 6, &UA_TYPES[UA_TYPES_INT32]);
    UA_UInt32 dims[] = {2, 3};
    val.arrayDimensionsSize = 2;
    val.arrayDimensions = dims;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_VARIANT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    /* Don't clear val - we used stack arrays */
} END_TEST

/* === ApplicationDescription JSON encoding (many fields) === */
START_TEST(json_encode_applicationdescription) {
    UA_ApplicationDescription val;
    UA_ApplicationDescription_init(&val);
    val.applicationUri = UA_STRING("urn:test");
    val.productUri = UA_STRING("urn:product");
    val.applicationName = UA_LOCALIZEDTEXT("en", "TestApp");
    val.applicationType = UA_APPLICATIONTYPE_SERVER;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === Reversible encoding with namespace mapping === */
START_TEST(json_encode_reversible) {
    UA_EncodeJsonOptions opts;
    memset(&opts, 0, sizeof(opts));
    opts.useReversible = true;

    UA_NodeId val = UA_NODEID_NUMERIC(0, 2255);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_NODEID], &buf, &opts);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);

    /* Variant with reversible encoding */
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Double d = 1.5;
    UA_Variant_setScalar(&v, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&v, &UA_TYPES[UA_TYPES_VARIANT], &buf, &opts);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

/* === endpointdescription === */
START_TEST(json_encode_endpointdescription) {
    UA_EndpointDescription val;
    UA_EndpointDescription_init(&val);
    val.endpointUrl = UA_STRING("opc.tcp://localhost:4840");
    val.securityMode = UA_MESSAGESECURITYMODE_NONE;
    val.securityPolicyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    val.securityLevel = 1;
    val.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");
    val.server.applicationUri = UA_STRING("urn:test");
    val.server.applicationName = UA_LOCALIZEDTEXT("en", "TestServer");

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ByteString_clear(&buf);
} END_TEST

static Suite *testSuite_jsonEncoding(void) {
    TCase *tc_basic = tcase_create("BasicJsonEncoding");
    tcase_add_test(tc_basic, json_encode_boolean_true);
    tcase_add_test(tc_basic, json_encode_boolean_false);
    tcase_add_test(tc_basic, json_encode_integers);
    tcase_add_test(tc_basic, json_encode_float_nan);
    tcase_add_test(tc_basic, json_encode_float_infinity);
    tcase_add_test(tc_basic, json_encode_double_nan);
    tcase_add_test(tc_basic, json_encode_double_infinity);
    tcase_add_test(tc_basic, json_encode_double_normal);

    TCase *tc_string = tcase_create("StringJsonEncoding");
    tcase_add_test(tc_string, json_encode_string);
    tcase_add_test(tc_string, json_encode_string_empty);
    tcase_add_test(tc_string, json_encode_string_null);
    tcase_add_test(tc_string, json_encode_string_escape);
    tcase_add_test(tc_string, json_encode_datetime);
    tcase_add_test(tc_string, json_encode_guid);
    tcase_add_test(tc_string, json_encode_bytestring);
    tcase_add_test(tc_string, json_encode_bytestring_null);

    TCase *tc_nodeid = tcase_create("NodeIdJsonEncoding");
    tcase_add_test(tc_nodeid, json_encode_nodeid_numeric);
    tcase_add_test(tc_nodeid, json_encode_nodeid_string);
    tcase_add_test(tc_nodeid, json_encode_nodeid_guid);
    tcase_add_test(tc_nodeid, json_encode_nodeid_bytestring);
    tcase_add_test(tc_nodeid, json_encode_expandednodeid);
    tcase_add_test(tc_nodeid, json_encode_statuscode);
    tcase_add_test(tc_nodeid, json_encode_qualifiedname);
    tcase_add_test(tc_nodeid, json_encode_localizedtext);

    TCase *tc_complex = tcase_create("ComplexJsonEncoding");
    tcase_add_test(tc_complex, json_encode_variant_scalar_int);
    tcase_add_test(tc_complex, json_encode_variant_array_int);
    tcase_add_test(tc_complex, json_encode_variant_string);
    tcase_add_test(tc_complex, json_encode_variant_empty);
    tcase_add_test(tc_complex, json_encode_datavalue);
    tcase_add_test(tc_complex, json_encode_diagnosticinfo);
    tcase_add_test(tc_complex, json_encode_extensionobject_decoded);
    tcase_add_test(tc_complex, json_encode_extensionobject_bytestring);
    tcase_add_test(tc_complex, json_encode_variant_2d_array);

    TCase *tc_options = tcase_create("JsonEncodingOptions");
    tcase_add_test(tc_options, json_encode_prettyprint);
    tcase_add_test(tc_options, json_encode_prettyprint_array);
    tcase_add_test(tc_options, json_encode_unquoted_keys);
    tcase_add_test(tc_options, json_encode_string_nodeids);
    tcase_add_test(tc_options, json_encode_reversible);
    tcase_add_test(tc_options, json_calcsize);

    TCase *tc_struct = tcase_create("StructJsonEncoding");
    tcase_add_test(tc_struct, json_encode_browseresult);
    tcase_add_test(tc_struct, json_encode_readrequest);
    tcase_add_test(tc_struct, json_encode_applicationdescription);
    tcase_add_test(tc_struct, json_encode_endpointdescription);

    Suite *s = suite_create("JSON Encoding Extended");
    suite_add_tcase(s, tc_basic);
    suite_add_tcase(s, tc_string);
    suite_add_tcase(s, tc_nodeid);
    suite_add_tcase(s, tc_complex);
    suite_add_tcase(s, tc_options);
    suite_add_tcase(s, tc_struct);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_jsonEncoding();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
