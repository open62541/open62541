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

/* On libcheck < 0.11 (ubuntu-20.04 ships 0.10), ck_assert_ptr_null /
 * ck_assert_ptr_nonnull are missing. Shim them to ck_assert_msg. */
#ifndef ck_assert_ptr_null
# define ck_assert_ptr_null(p) ck_assert_msg((p) == NULL, #p " is not NULL")
#endif
#ifndef ck_assert_ptr_nonnull
# define ck_assert_ptr_nonnull(p) ck_assert_msg((p) != NULL, #p " is NULL")
#endif

typedef struct {
    UA_Int16 required;
    UA_Float *optional;
    size_t optionalValuesSize;
    UA_Int32 *optionalValues;
} JsonOptionalStructure;

static UA_DataTypeMember jsonOptionalStructureMembers[3] = {
    {UA_TYPENAME("Required") &UA_TYPES[UA_TYPES_INT16], 0, false, false},
    {UA_TYPENAME("Optional") &UA_TYPES[UA_TYPES_FLOAT],
     offsetof(JsonOptionalStructure, optional) -
         offsetof(JsonOptionalStructure, required) - sizeof(UA_Int16),
     false, true},
    {UA_TYPENAME("OptionalValues") &UA_TYPES[UA_TYPES_INT32],
     offsetof(JsonOptionalStructure, optionalValuesSize) -
         offsetof(JsonOptionalStructure, optional) - sizeof(void*),
     true, true}
};

typedef struct {
    UA_UInt32 switchField;
    union {
        UA_Double number;
        UA_String text;
        struct {
            size_t valuesSize;
            UA_Int16 *values;
        } values;
    } fields;
} JsonUnion;

static UA_DataTypeMember jsonUnionMembers[3] = {
    {UA_TYPENAME("Number") &UA_TYPES[UA_TYPES_DOUBLE],
     offsetof(JsonUnion, fields.number), false, false},
    {UA_TYPENAME("Text") &UA_TYPES[UA_TYPES_STRING],
     offsetof(JsonUnion, fields.text), false, false},
    {UA_TYPENAME("Values") &UA_TYPES[UA_TYPES_INT16],
     offsetof(JsonUnion, fields.values), true, false}
};

static UA_DataType jsonCustomTypes[3] = {
    {UA_TYPENAME("JsonOptionalStructure")
     {1, UA_NODEIDTYPE_NUMERIC, {5001}},
     {1, UA_NODEIDTYPE_NUMERIC, {6001}},
     {1, UA_NODEIDTYPE_NUMERIC, {7001}},
     sizeof(JsonOptionalStructure), UA_DATATYPEKIND_OPTSTRUCT,
     false, false, 3, jsonOptionalStructureMembers},
    {UA_TYPENAME("JsonUnion")
     {1, UA_NODEIDTYPE_NUMERIC, {5002}},
     {1, UA_NODEIDTYPE_NUMERIC, {6002}},
     {1, UA_NODEIDTYPE_NUMERIC, {7002}},
     sizeof(JsonUnion), UA_DATATYPEKIND_UNION,
     false, false, 3, jsonUnionMembers},
    {UA_TYPENAME("JsonEnum")
     {1, UA_NODEIDTYPE_NUMERIC, {5003}},
     {1, UA_NODEIDTYPE_NUMERIC, {6003}},
     {1, UA_NODEIDTYPE_NUMERIC, {7003}},
     sizeof(UA_Int32), UA_DATATYPEKIND_ENUM,
     true, true, 0, NULL}
};

static UA_DataTypeArray jsonCustomTypeArray = {
    NULL, 3, jsonCustomTypes, false
};

static void
assertJsonEqual(const UA_ByteString *encoded, const char *expected) {
    size_t expectedLength = strlen(expected);
    ck_assert_uint_eq(encoded->length, expectedLength);
    ck_assert(memcmp(encoded->data, expected, expectedLength) == 0);
}

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

START_TEST(json_encode_datetime_bounds) {
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_DateTime val = 0;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DATETIME],
                                      &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "\"0001-01-01T00:00:00Z\"");
    UA_ByteString_clear(&buf);

    val = UA_INT64_MIN;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DATETIME], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "\"0001-01-01T00:00:00Z\"");
    UA_ByteString_clear(&buf);

    val = UA_INT64_MAX;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_DATETIME], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "\"9999-12-31T23:59:59.9999999Z\"");
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

    val.locale = UA_STRING_NULL;
    buf = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "{\"Text\":\"Hello World\"}");
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

START_TEST(json_encode_variant_enum_as_int32) {
    UA_Int32 value = 7;
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Variant_setScalar(&variant, &value, &jsonCustomTypes[2]);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&variant, &UA_TYPES[UA_TYPES_VARIANT],
                                      &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "{\"UaType\":6,\"Value\":7}");
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_variant_nullable_value_omitted) {
    UA_String value = UA_STRING_NULL;
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Variant_setScalar(&variant, &value, &UA_TYPES[UA_TYPES_STRING]);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&variant, &UA_TYPES[UA_TYPES_VARIANT],
                                      &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "{\"UaType\":12}");
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_variant_rejects_prohibited_types) {
    UA_Variant inner;
    UA_Variant_init(&inner);
    UA_Variant outer;
    UA_Variant_init(&outer);
    UA_Variant_setScalar(&outer, &inner, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&outer, &UA_TYPES[UA_TYPES_VARIANT],
                                      &buf, NULL);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);

    UA_DiagnosticInfo diagnostic;
    UA_DiagnosticInfo_init(&diagnostic);
    UA_Variant_setScalar(&outer, &diagnostic,
                         &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    res = UA_encodeJson(&outer, &UA_TYPES[UA_TYPES_VARIANT], &buf, NULL);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(json_encode_variant_array_of_variants) {
    UA_Int32 numbers[2] = {1, 2};
    UA_Variant elements[2];
    UA_Variant_init(&elements[0]);
    UA_Variant_init(&elements[1]);
    UA_Variant_setScalar(&elements[0], &numbers[0], &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&elements[1], &numbers[1], &UA_TYPES[UA_TYPES_INT32]);

    UA_Variant outer;
    UA_Variant_init(&outer);
    UA_Variant_setArray(&outer, elements, 2, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&outer, &UA_TYPES[UA_TYPES_VARIANT],
                                      &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf,
                    "{\"UaType\":24,\"Value\":["
                    "{\"UaType\":6,\"Value\":1},"
                    "{\"UaType\":6,\"Value\":2}]}");
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_nonnullable_array_defaults) {
    UA_StatusCode values[2] = {UA_STATUSCODE_GOOD, UA_STATUSCODE_GOOD};
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Variant_setArray(&variant, values, 2, &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&variant, &UA_TYPES[UA_TYPES_VARIANT],
                                      &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "{\"UaType\":19,\"Value\":[{},{}]}");
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

START_TEST(json_encode_datavalue_omits_defaults_and_rejects_nesting) {
    UA_DataValue value;
    UA_DataValue_init(&value);
    value.hasStatus = true;
    value.hasSourceTimestamp = true;
    value.hasSourcePicoseconds = true;
    value.hasServerTimestamp = true;
    value.hasServerPicoseconds = true;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&value, &UA_TYPES[UA_TYPES_DATAVALUE],
                                      &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "{}");
    UA_ByteString_clear(&buf);

    UA_DataValue inner;
    UA_DataValue_init(&inner);
    value.hasValue = true;
    UA_Variant_setScalar(&value.value, &inner, &UA_TYPES[UA_TYPES_DATAVALUE]);
    res = UA_encodeJson(&value, &UA_TYPES[UA_TYPES_DATAVALUE], &buf, NULL);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
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

START_TEST(json_encode_diagnosticinfo_omits_defaults) {
    UA_DiagnosticInfo value;
    UA_DiagnosticInfo_init(&value);
    value.hasSymbolicId = true;
    value.symbolicId = -1;
    value.hasNamespaceUri = true;
    value.namespaceUri = -1;
    value.hasLocalizedText = true;
    value.localizedText = -1;
    value.hasLocale = true;
    value.locale = -1;
    value.hasAdditionalInfo = true;
    value.hasInnerStatusCode = true;

    UA_DiagnosticInfo inner;
    UA_DiagnosticInfo_init(&inner);
    value.hasInnerDiagnosticInfo = true;
    value.innerDiagnosticInfo = &inner;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&value,
                                      &UA_TYPES[UA_TYPES_DIAGNOSTICINFO],
                                      &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "{}");
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

START_TEST(json_extensionobject_exact_wire_roundtrip) {
    UA_ReadValueId value;
    UA_ReadValueId_init(&value);
    value.nodeId = UA_NODEID_NUMERIC(0, 2255);
    value.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ExtensionObject source;
    UA_ExtensionObject_init(&source);
    source.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    source.content.decoded.type = &UA_TYPES[UA_TYPES_READVALUEID];
    source.content.decoded.data = &value;

    const char *expected =
        "{\"UaTypeId\":\"i=626\",\"NodeId\":\"i=2255\","
        "\"AttributeId\":13,\"IndexRange\":null,\"DataEncoding\":null}";
    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(
        &source, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &encoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&encoded, expected);

    UA_ExtensionObject decoded;
    UA_ExtensionObject_init(&decoded);
    res = UA_decodeJson(&encoded, &decoded,
                        &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(decoded.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert_ptr_eq(decoded.content.decoded.type,
                     &UA_TYPES[UA_TYPES_READVALUEID]);
    UA_ReadValueId *decodedValue =
        (UA_ReadValueId*)decoded.content.decoded.data;
    ck_assert(UA_NodeId_equal(&decodedValue->nodeId, &value.nodeId));
    ck_assert_uint_eq(decodedValue->attributeId, UA_ATTRIBUTEID_VALUE);

    UA_ByteString reencoded = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&decoded, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
                        &reencoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&reencoded, expected);

    UA_ByteString_clear(&reencoded);
    UA_ExtensionObject_clear(&decoded);
    UA_ByteString_clear(&encoded);
} END_TEST

START_TEST(json_optional_structure_roundtrip) {
    UA_Float optional = 1.5f;
    UA_Int32 optionalValues[2] = {3, 4};
    JsonOptionalStructure source = {
        7, &optional, 2, optionalValues
    };

    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&source, &jsonCustomTypes[0],
                                      &encoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&encoded,
                    "{\"Required\":7,\"Optional\":1.5,"
                    "\"OptionalValues\":[3,4]}");

    JsonOptionalStructure decoded;
    UA_DecodeJsonOptions options = {0};
    options.customTypes = &jsonCustomTypeArray;
    res = UA_decodeJson(&encoded, &decoded, &jsonCustomTypes[0], &options);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(decoded.required, 7);
    ck_assert_ptr_nonnull(decoded.optional);
    ck_assert_float_eq(*decoded.optional, 1.5f);
    ck_assert_uint_eq(decoded.optionalValuesSize, 2);
    ck_assert_int_eq(decoded.optionalValues[0], 3);
    ck_assert_int_eq(decoded.optionalValues[1], 4);
    UA_clear(&decoded, &jsonCustomTypes[0]);
    UA_ByteString_clear(&encoded);

    source.optional = NULL;
    source.optionalValuesSize = 0;
    source.optionalValues = NULL;
    res = UA_encodeJson(&source, &jsonCustomTypes[0], &encoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&encoded, "{\"Required\":7}");
    UA_ByteString_clear(&encoded);
} END_TEST

START_TEST(json_optional_structure_compact_decode) {
    UA_ByteString encoded = UA_BYTESTRING(
        "{\"OptionalValues\":[3,4],\"Required\":7,"
        "\"EncodingMask\":3,\"Optional\":1.5}");
    JsonOptionalStructure decoded;
    UA_StatusCode res = UA_decodeJson(&encoded, &decoded,
                                      &jsonCustomTypes[0], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_nonnull(decoded.optional);
    ck_assert_float_eq(*decoded.optional, 1.5f);
    ck_assert_uint_eq(decoded.optionalValuesSize, 2);
    UA_clear(&decoded, &jsonCustomTypes[0]);

    encoded = UA_BYTESTRING("{\"EncodingMask\":1,\"Required\":7}");
    res = UA_decodeJson(&encoded, &decoded, &jsonCustomTypes[0], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_nonnull(decoded.optional);
    ck_assert_float_eq(*decoded.optional, 0.0f);
    ck_assert_ptr_null(decoded.optionalValues);
    UA_clear(&decoded, &jsonCustomTypes[0]);
} END_TEST

START_TEST(json_optional_structure_rejects_bad_mask) {
    UA_ByteString encoded = UA_BYTESTRING(
        "{\"EncodingMask\":0,\"Required\":7,\"Optional\":1.5}");
    JsonOptionalStructure decoded;
    UA_StatusCode res = UA_decodeJson(&encoded, &decoded,
                                      &jsonCustomTypes[0], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADDECODINGERROR);

    encoded = UA_BYTESTRING("{\"EncodingMask\":4,\"Required\":7}");
    res = UA_decodeJson(&encoded, &decoded, &jsonCustomTypes[0], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADDECODINGERROR);
} END_TEST

START_TEST(json_union_roundtrip) {
    JsonUnion source;
    memset(&source, 0, sizeof(source));
    source.switchField = 2;
    source.fields.text = UA_STRING("hello");

    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&source, &jsonCustomTypes[1],
                                      &encoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&encoded, "{\"Text\":\"hello\"}");

    JsonUnion decoded;
    res = UA_decodeJson(&encoded, &decoded, &jsonCustomTypes[1], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(decoded.switchField, 2);
    const UA_String expected = UA_STRING_STATIC("hello");
    ck_assert(UA_String_equal(&decoded.fields.text, &expected));
    UA_clear(&decoded, &jsonCustomTypes[1]);
    UA_ByteString_clear(&encoded);

    encoded = UA_BYTESTRING("{\"Text\":\"hello\",\"SwitchField\":2}");
    res = UA_decodeJson(&encoded, &decoded, &jsonCustomTypes[1], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(decoded.switchField, 2);
    UA_clear(&decoded, &jsonCustomTypes[1]);

    UA_Int16 values[2] = {5, 6};
    memset(&source, 0, sizeof(source));
    source.switchField = 3;
    source.fields.values.valuesSize = 2;
    source.fields.values.values = values;
    encoded = UA_BYTESTRING_NULL;
    res = UA_encodeJson(&source, &jsonCustomTypes[1], &encoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&encoded, "{\"Values\":[5,6]}");
    res = UA_decodeJson(&encoded, &decoded, &jsonCustomTypes[1], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(decoded.switchField, 3);
    ck_assert_uint_eq(decoded.fields.values.valuesSize, 2);
    ck_assert_int_eq(decoded.fields.values.values[0], 5);
    ck_assert_int_eq(decoded.fields.values.values[1], 6);
    UA_clear(&decoded, &jsonCustomTypes[1]);
    UA_ByteString_clear(&encoded);
} END_TEST

START_TEST(json_union_rejects_ambiguous_selection) {
    JsonUnion decoded;
    UA_ByteString encoded = UA_BYTESTRING(
        "{\"Number\":3.5,\"Text\":\"ambiguous\"}");
    UA_StatusCode res = UA_decodeJson(&encoded, &decoded,
                                      &jsonCustomTypes[1], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADDECODINGERROR);

    encoded = UA_BYTESTRING("{\"SwitchField\":1,\"Text\":\"wrong\"}");
    res = UA_decodeJson(&encoded, &decoded, &jsonCustomTypes[1], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADDECODINGERROR);

    encoded = UA_BYTESTRING("{\"SwitchField\":4}");
    res = UA_decodeJson(&encoded, &decoded, &jsonCustomTypes[1], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADDECODINGERROR);
} END_TEST

START_TEST(json_structured_extensionobjects_roundtrip) {
    UA_Float optional = 2.5f;
    JsonOptionalStructure optionalSource = {9, &optional, 0, NULL};
    UA_ExtensionObject source;
    UA_ExtensionObject_init(&source);
    source.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    source.content.decoded.type = &jsonCustomTypes[0];
    source.content.decoded.data = &optionalSource;

    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(
        &source, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &encoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&encoded,
                    "{\"UaTypeId\":\"ns=1;i=5001\","
                    "\"Required\":9,\"Optional\":2.5}");

    UA_DecodeJsonOptions options = {0};
    options.customTypes = &jsonCustomTypeArray;
    UA_ExtensionObject decoded;
    UA_ExtensionObject_init(&decoded);
    res = UA_decodeJson(&encoded, &decoded,
                        &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &options);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(decoded.content.decoded.type, &jsonCustomTypes[0]);
    UA_ExtensionObject_clear(&decoded);
    UA_ByteString_clear(&encoded);

    JsonUnion unionSource;
    memset(&unionSource, 0, sizeof(unionSource));
    unionSource.switchField = 1;
    unionSource.fields.number = 3.5;
    source.content.decoded.type = &jsonCustomTypes[1];
    source.content.decoded.data = &unionSource;
    res = UA_encodeJson(&source, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
                        &encoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&encoded,
                    "{\"UaTypeId\":\"ns=1;i=5002\",\"Number\":3.5}");
    res = UA_decodeJson(&encoded, &decoded,
                        &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &options);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    JsonUnion *decodedUnion = (JsonUnion*)decoded.content.decoded.data;
    ck_assert_uint_eq(decodedUnion->switchField, 1);
    ck_assert_double_eq(decodedUnion->fields.number, 3.5);
    UA_ExtensionObject_clear(&decoded);
    UA_ByteString_clear(&encoded);
} END_TEST

START_TEST(json_structured_variant_arrays_roundtrip) {
    UA_DecodeJsonOptions options = {0};
    options.customTypes = &jsonCustomTypeArray;

    UA_Float optional = 1.25f;
    UA_Int32 optionalValuesData[2] = {10, 11};
    JsonOptionalStructure optionalValues[2] = {
        {1, &optional, 0, NULL},
        {2, NULL, 2, optionalValuesData}
    };
    UA_Variant source;
    UA_Variant_init(&source);
    UA_Variant_setArray(&source, optionalValues, 2, &jsonCustomTypes[0]);

    UA_ByteString encoded = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&source, &UA_TYPES[UA_TYPES_VARIANT],
                                      &encoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant decoded;
    UA_Variant_init(&decoded);
    res = UA_decodeJson(&encoded, &decoded, &UA_TYPES[UA_TYPES_VARIANT],
                        &options);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(decoded.type, &jsonCustomTypes[0]);
    ck_assert_uint_eq(decoded.arrayLength, 2);
    JsonOptionalStructure *decodedOptional =
        (JsonOptionalStructure*)decoded.data;
    ck_assert_int_eq(decodedOptional[0].required, 1);
    ck_assert_ptr_nonnull(decodedOptional[0].optional);
    ck_assert_float_eq(*decodedOptional[0].optional, 1.25f);
    ck_assert_ptr_null(decodedOptional[0].optionalValues);
    ck_assert_int_eq(decodedOptional[1].required, 2);
    ck_assert_ptr_null(decodedOptional[1].optional);
    ck_assert_uint_eq(decodedOptional[1].optionalValuesSize, 2);
    ck_assert_int_eq(decodedOptional[1].optionalValues[0], 10);
    ck_assert_int_eq(decodedOptional[1].optionalValues[1], 11);
    UA_Variant_clear(&decoded);
    UA_ByteString_clear(&encoded);

    JsonUnion unionValues[2];
    memset(unionValues, 0, sizeof(unionValues));
    unionValues[0].switchField = 1;
    unionValues[0].fields.number = 4.5;
    unionValues[1].switchField = 2;
    unionValues[1].fields.text = UA_STRING("array");
    UA_Variant_setArray(&source, unionValues, 2, &jsonCustomTypes[1]);
    res = UA_encodeJson(&source, &UA_TYPES[UA_TYPES_VARIANT], &encoded, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant_init(&decoded);
    res = UA_decodeJson(&encoded, &decoded, &UA_TYPES[UA_TYPES_VARIANT],
                        &options);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(decoded.type, &jsonCustomTypes[1]);
    ck_assert_uint_eq(decoded.arrayLength, 2);
    JsonUnion *decodedUnion = (JsonUnion*)decoded.data;
    ck_assert_uint_eq(decodedUnion[0].switchField, 1);
    ck_assert_double_eq(decodedUnion[0].fields.number, 4.5);
    ck_assert_uint_eq(decodedUnion[1].switchField, 2);
    const UA_String expected = UA_STRING_STATIC("array");
    ck_assert(UA_String_equal(&decodedUnion[1].fields.text, &expected));
    UA_Variant_clear(&decoded);
    UA_ByteString_clear(&encoded);
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
    assertJsonEqual(&buf,
                    "{\"UaTypeId\":\"i=999\",\"UaEncoding\":1,"
                    "\"UaBody\":\"dGVzdA==\"}");
    UA_ByteString_clear(&buf);
} END_TEST

START_TEST(json_encode_extensionobject_empty_object) {
    UA_ExtensionObject value;
    UA_ExtensionObject_init(&value);
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&value,
                                      &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
                                      &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    assertJsonEqual(&buf, "{}");
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

/* === JSON encoding NULL guards === */
START_TEST(json_encode_nullSource_rejected) {
    /* src/ua_types_encoding_json.c:1217-1218:
     *   if(!src || !type) return UA_STATUSCODE_BADINTERNALERROR;
     * The public UA_encodeJson must reject NULL source and NULL type. */
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(NULL, &UA_TYPES[UA_TYPES_INT32], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_ptr_null(buf.data);

    res = UA_encodeJson(NULL, NULL, &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
} END_TEST

START_TEST(json_encode_nullType_rejected) {
    UA_Int32 val = 42;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, NULL, &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_ptr_null(buf.data);
} END_TEST

START_TEST(json_encode_preAllocBufferTooSmall_keepsBuffer) {
    /* src/ua_types_encoding_json.c:1255-1256:
     *   else if(allocated) UA_ByteString_clear(outBuf);
     * The pre-allocated-buffer path (length > 0) does not free the
     * buffer on error -- the caller still owns it. */
    UA_Int32 val = 123456789;
    UA_ByteString buf;
    UA_StatusCode res = UA_ByteString_allocBuffer(&buf, 1); /* too small */
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_INT32], &buf, NULL);
    ck_assert(res != UA_STATUSCODE_GOOD);
    /* Caller still owns the buffer -- the library must not free it */
    ck_assert_ptr_nonnull(buf.data);
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
    tcase_add_test(tc_string, json_encode_datetime_bounds);
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
    tcase_add_test(tc_complex, json_encode_variant_enum_as_int32);
    tcase_add_test(tc_complex, json_encode_variant_nullable_value_omitted);
    tcase_add_test(tc_complex, json_encode_variant_rejects_prohibited_types);
    tcase_add_test(tc_complex, json_encode_variant_array_of_variants);
    tcase_add_test(tc_complex, json_encode_nonnullable_array_defaults);
    tcase_add_test(tc_complex, json_encode_datavalue);
    tcase_add_test(tc_complex,
                   json_encode_datavalue_omits_defaults_and_rejects_nesting);
    tcase_add_test(tc_complex, json_encode_diagnosticinfo);
    tcase_add_test(tc_complex, json_encode_diagnosticinfo_omits_defaults);
    tcase_add_test(tc_complex, json_encode_extensionobject_decoded);
    tcase_add_test(tc_complex, json_extensionobject_exact_wire_roundtrip);
    tcase_add_test(tc_complex, json_encode_extensionobject_bytestring);
    tcase_add_test(tc_complex, json_encode_extensionobject_empty_object);
    tcase_add_test(tc_complex, json_encode_variant_2d_array);
    tcase_add_test(tc_complex, json_optional_structure_roundtrip);
    tcase_add_test(tc_complex, json_optional_structure_compact_decode);
    tcase_add_test(tc_complex, json_optional_structure_rejects_bad_mask);
    tcase_add_test(tc_complex, json_union_roundtrip);
    tcase_add_test(tc_complex, json_union_rejects_ambiguous_selection);
    tcase_add_test(tc_complex, json_structured_extensionobjects_roundtrip);
    tcase_add_test(tc_complex, json_structured_variant_arrays_roundtrip);

    TCase *tc_options = tcase_create("JsonEncodingOptions");
    tcase_add_test(tc_options, json_encode_prettyprint);
    tcase_add_test(tc_options, json_encode_prettyprint_array);
    tcase_add_test(tc_options, json_encode_unquoted_keys);
    tcase_add_test(tc_options, json_encode_string_nodeids);
    tcase_add_test(tc_options, json_calcsize);

    TCase *tc_struct = tcase_create("StructJsonEncoding");
    tcase_add_test(tc_struct, json_encode_browseresult);
    tcase_add_test(tc_struct, json_encode_readrequest);
    tcase_add_test(tc_struct, json_encode_applicationdescription);
    tcase_add_test(tc_struct, json_encode_endpointdescription);

    TCase *tc_null = tcase_create("JsonEncodingNullGuards");
    tcase_add_test(tc_null, json_encode_nullSource_rejected);
    tcase_add_test(tc_null, json_encode_nullType_rejected);
    tcase_add_test(tc_null, json_encode_preAllocBufferTooSmall_keepsBuffer);

    Suite *s = suite_create("JSON Encoding Extended");
    suite_add_tcase(s, tc_basic);
    suite_add_tcase(s, tc_string);
    suite_add_tcase(s, tc_nodeid);
    suite_add_tcase(s, tc_complex);
    suite_add_tcase(s, tc_options);
    suite_add_tcase(s, tc_struct);
    suite_add_tcase(s, tc_null);
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
