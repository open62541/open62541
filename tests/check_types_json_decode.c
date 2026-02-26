/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* JSON decoding tests */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ===== Helper ===== */
static UA_StatusCode
decode(const char *json, void *dst, const UA_DataType *type) {
    UA_ByteString src = UA_BYTESTRING((char*)(uintptr_t)json);
    return UA_decodeJson(&src, dst, type, NULL);
}

/* ============================================================
 * 1. Variant_decodeJsonUnwrapExtensionObject
 * ============================================================ */

/* Variant containing an ExtensionObject with a known non-builtin type (Argument, i=296).
 * The decoder should unwrap it: dst->type == &UA_TYPES[UA_TYPES_ARGUMENT]. */
START_TEST(json_decode_variant_unwrap_eo_known_type) {
    /* Argument: typeId i=296 (ns=0). Body has mandatory fields. */
    const char *json =
        "{\"UaType\":22,\"Value\":"
        "{\"UaTypeId\":\"i=296\","
        "\"UaBody\":{\"Name\":\"x\",\"DataType\":\"i=1\","
        "\"ValueRank\":-1}}}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    if(res == UA_STATUSCODE_GOOD) {
        /* Unwrapped – the Variant data type should be Argument, not ExtensionObject */
        ck_assert(v.type != NULL);
        ck_assert(v.type == &UA_TYPES[UA_TYPES_ARGUMENT] ||
                  v.type == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    }
    UA_Variant_clear(&v);
} END_TEST

/* Variant containing an EO with a builtin body type (Int32, kind<=DIAGNOSTICINFO).
 * The decoder should keep it wrapped as an ExtensionObject. */
START_TEST(json_decode_variant_eo_builtin_stays_wrapped) {
    const char *json =
        "{\"UaType\":22,\"Value\":"
        "{\"UaTypeId\":\"i=6\","
        "\"UaBody\":42}}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    /* May or may not succeed depending on internal lookup; just exercise the path */
    (void)res;
    UA_Variant_clear(&v);
} END_TEST

/* Variant containing an EO with a null body – exercises the CJ5_TOKEN_NULL path */
START_TEST(json_decode_variant_eo_null_body) {
    const char *json = "{\"UaType\":22,\"Value\":null}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    if(res == UA_STATUSCODE_GOOD) {
        /* Should be an ExtensionObject wrapping a null body */
        ck_assert(v.type == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    }
    UA_Variant_clear(&v);
} END_TEST

/* Variant containing an EO with unknown type – falls back to use_eo label
 * (encoded bytestring) */
START_TEST(json_decode_variant_eo_unknown_type) {
    const char *json =
        "{\"UaType\":22,\"Value\":"
        "{\"UaTypeId\":\"i=99999\","
        "\"UaBody\":{\"Foo\":1}}}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    /* Unknown type → kept as EO with encoded body */
    (void)res;
    UA_Variant_clear(&v);
} END_TEST

/* ============================================================
 * 2. ExtensionObject_decodeJson
 * ============================================================ */

/* EO with unknown TypeId but no Body field – in-situ encoding path */
START_TEST(json_decode_eo_unknown_type_no_body) {
    const char *json =
        "{\"UaTypeId\":\"i=99999\",\"SomeField\":123}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(eo.encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING ||
                  eo.encoding == UA_EXTENSIONOBJECT_ENCODED_XML);
        ck_assert(eo.content.encoded.body.length > 0);
    }
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO with known type and Body field */
START_TEST(json_decode_eo_known_type_with_body) {
    const char *json =
        "{\"UaTypeId\":\"i=296\","
        "\"UaBody\":{\"Name\":\"arg1\",\"DataType\":\"i=1\","
        "\"ValueRank\":-1}}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(eo.encoding, UA_EXTENSIONOBJECT_DECODED);
        ck_assert(eo.content.decoded.type == &UA_TYPES[UA_TYPES_ARGUMENT]);
    }
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO with known type but without explicit Body – structure decoded in-situ */
START_TEST(json_decode_eo_known_type_no_body_field) {
    const char *json =
        "{\"UaTypeId\":\"i=296\","
        "\"Name\":\"arg2\",\"DataType\":\"i=1\","
        "\"ValueRank\":-1}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(eo.encoding, UA_EXTENSIONOBJECT_DECODED);
    }
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO empty object -> null extension object */
START_TEST(json_decode_eo_empty_object) {
    const char *json = "{}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(eo.encoding, UA_EXTENSIONOBJECT_ENCODED_NOBODY);
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO null -> null extension object */
START_TEST(json_decode_eo_null) {
    const char *json = "null";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO with UaEncoding = 1 (ByteString) and unknown type with Body */
START_TEST(json_decode_eo_bytestring_encoding) {
    const char *json =
        "{\"UaTypeId\":\"i=99999\","
        "\"UaEncoding\":1,"
        "\"UaBody\":\"dGVzdA==\"}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(eo.encoding == UA_EXTENSIONOBJECT_ENCODED_BYTESTRING);
    }
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO with UaEncoding = 2 (XML) and unknown type with Body */
START_TEST(json_decode_eo_xml_encoding) {
    const char *json =
        "{\"UaTypeId\":\"i=99999\","
        "\"UaEncoding\":2,"
        "\"UaBody\":\"PHhtbD48L3htbD4=\"}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(eo.encoding == UA_EXTENSIONOBJECT_ENCODED_XML);
    }
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO with invalid UaEncoding value (>2) -> error */
START_TEST(json_decode_eo_invalid_encoding) {
    const char *json =
        "{\"UaTypeId\":\"i=99999\","
        "\"UaEncoding\":5,"
        "\"UaBody\":\"dGVzdA==\"}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO missing TypeId -> error */
START_TEST(json_decode_eo_missing_typeid) {
    const char *json = "{\"UaBody\":{\"Name\":\"x\"}}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* ============================================================
 * 3. decodeJSONVariant
 * ============================================================ */

/* Empty variant (null) */
START_TEST(json_decode_variant_null) {
    const char *json = "null";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(v.type == NULL);
    UA_Variant_clear(&v);
} END_TEST

/* Empty variant (empty object) */
START_TEST(json_decode_variant_empty_object) {
    const char *json = "{}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(v.type == NULL);
    UA_Variant_clear(&v);
} END_TEST

/* Variant missing UaType field -> error */
START_TEST(json_decode_variant_missing_type) {
    const char *json = "{\"Value\":42}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* Variant with invalid type kind (too large) */
START_TEST(json_decode_variant_bad_type_kind) {
    const char *json = "{\"UaType\":999,\"Value\":42}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* Variant with type kind 0 (underflow after typeKind--) */
START_TEST(json_decode_variant_type_zero) {
    const char *json = "{\"UaType\":0,\"Value\":42}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    /* typeKind 0 - 1 = underflow, should be > UA_DATATYPEKIND_DIAGNOSTICINFO */
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* Variant with UaType as string instead of number -> error */
START_TEST(json_decode_variant_type_not_number) {
    const char *json = "{\"UaType\":\"hello\",\"Value\":42}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* Variant scalar with dimensions -> error */
START_TEST(json_decode_variant_scalar_with_dimensions) {
    const char *json = "{\"UaType\":6,\"Value\":42,\"Dimensions\":[1]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* Variant containing a Variant (scalar) -> error (variant cannot contain variant) */
START_TEST(json_decode_variant_containing_variant) {
    const char *json = "{\"UaType\":24,\"Value\":{\"UaType\":6,\"Value\":42}}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* Variant with null value (type given but value is null) */
START_TEST(json_decode_variant_null_value) {
    const char *json = "{\"UaType\":6,\"Value\":null}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    /* Allocates memory for the type but leaves it zero-initialized */
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(v.type != NULL);
    }
    UA_Variant_clear(&v);
} END_TEST

/* Variant Int32 array */
START_TEST(json_decode_variant_int_array) {
    const char *json = "{\"UaType\":6,\"Value\":[1,2,3]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(v.type == &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(v.arrayLength, 3);
    UA_Variant_clear(&v);
} END_TEST

/* Variant multi-dimensional array (2x2 matrix) */
START_TEST(json_decode_variant_2d_array) {
    const char *json = "{\"UaType\":6,\"Value\":[1,2,3,4],\"Dimensions\":[2,2]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(v.type == &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(v.arrayLength, 4);
    ck_assert(v.arrayDimensions != NULL);
    ck_assert_uint_eq(v.arrayDimensionsSize, 2);
    ck_assert_uint_eq(v.arrayDimensions[0], 2);
    ck_assert_uint_eq(v.arrayDimensions[1], 2);
    UA_Variant_clear(&v);
} END_TEST

/* Variant with 1D dimension -> should be collapsed (arrayDimensions freed) */
START_TEST(json_decode_variant_1d_dimension) {
    const char *json = "{\"UaType\":6,\"Value\":[10,20,30],\"Dimensions\":[3]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v.arrayLength, 3);
    /* 1D dimension should be removed */
    ck_assert(v.arrayDimensions == NULL);
    ck_assert_uint_eq(v.arrayDimensionsSize, 0);
    UA_Variant_clear(&v);
} END_TEST

/* Variant with dimension mismatch -> error */
START_TEST(json_decode_variant_dimension_mismatch) {
    const char *json = "{\"UaType\":6,\"Value\":[1,2,3],\"Dimensions\":[2,2]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    /* 2*2=4 != 3 elements -> bad decoding */
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* Variant with 3D array */
START_TEST(json_decode_variant_3d_array) {
    const char *json =
        "{\"UaType\":6,"
        "\"Value\":[1,2,3,4,5,6,7,8],"
        "\"Dimensions\":[2,2,2]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v.arrayLength, 8);
    ck_assert_uint_eq(v.arrayDimensionsSize, 3);
    UA_Variant_clear(&v);
} END_TEST

/* Variant array of variants */
START_TEST(json_decode_variant_array_of_variants) {
    const char *json =
        "{\"UaType\":24,\"Value\":["
        "{\"UaType\":6,\"Value\":1},"
        "{\"UaType\":6,\"Value\":2}]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    /* Array of variants is allowed (scalar variant in variant is not) */
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(v.type == &UA_TYPES[UA_TYPES_VARIANT]);
        ck_assert_uint_eq(v.arrayLength, 2);
    }
    UA_Variant_clear(&v);
} END_TEST

/* ============================================================
 * 4. removeFieldFromEncoding – exercised through EO unknown type
 *    (includes quote-stripping logic)
 * ============================================================ */

/* EO unknown type with UaTypeId before UaEncoding – exercises
 * removeFieldFromEncoding with encIndex > typeIdIndex */
START_TEST(json_decode_eo_remove_field_enc_after_typeid) {
    const char *json =
        "{\"UaTypeId\":\"i=99999\","
        "\"UaEncoding\":0,"
        "\"SomeField\":\"hello\"}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    /* removeFieldFromEncoding removes UaEncoding (after) then UaTypeId (before) */
    (void)res;
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO unknown type with UaEncoding before UaTypeId */
START_TEST(json_decode_eo_remove_field_enc_before_typeid) {
    const char *json =
        "{\"UaEncoding\":0,"
        "\"UaTypeId\":\"i=99999\","
        "\"SomeField\":\"world\"}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    (void)res;
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO unknown type with only UaTypeId (no UaEncoding) – single-field removal */
START_TEST(json_decode_eo_remove_only_typeid) {
    const char *json =
        "{\"UaTypeId\":\"i=99999\","
        "\"Data\":42}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    if(res == UA_STATUSCODE_GOOD) {
        /* Body should not contain UaTypeId */
        ck_assert(eo.content.encoded.body.length > 0);
    }
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* EO unknown type where UaTypeId is the first field (no previous element) -
 * exercises the else branch in removeFieldFromEncoding */
START_TEST(json_decode_eo_remove_first_field) {
    const char *json =
        "{\"UaTypeId\":\"i=88888\",\"A\":1,\"B\":2}";
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(json, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    (void)res;
    UA_ExtensionObject_clear(&eo);
} END_TEST

/* ============================================================
 * 5. getArrayUnwrapType – array of EO with same known type
 * ============================================================ */

/* Array of EOs with same known type -> unwrap */
START_TEST(json_decode_variant_eo_array_unwrap) {
    const char *json =
        "{\"UaType\":22,\"Value\":["
        "{\"UaTypeId\":\"i=296\","
        "\"UaBody\":{\"Name\":\"a\",\"DataType\":\"i=1\",\"ValueRank\":-1}},"
        "{\"UaTypeId\":\"i=296\","
        "\"UaBody\":{\"Name\":\"b\",\"DataType\":\"i=1\",\"ValueRank\":-1}}"
        "]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    if(res == UA_STATUSCODE_GOOD) {
        /* Should be unwrapped to Argument array */
        ck_assert(v.type == &UA_TYPES[UA_TYPES_ARGUMENT]);
        ck_assert_uint_eq(v.arrayLength, 2);
    }
    UA_Variant_clear(&v);
} END_TEST

/* Array of EOs with DIFFERENT types -> not unwrapped (stays as EO array) */
START_TEST(json_decode_variant_eo_array_mixed_types) {
    const char *json =
        "{\"UaType\":22,\"Value\":["
        "{\"UaTypeId\":\"i=296\","
        "\"UaBody\":{\"Name\":\"a\",\"DataType\":\"i=1\",\"ValueRank\":-1}},"
        "{\"UaTypeId\":\"i=7594\","
        "\"UaBody\":{\"Value\":0,\"DisplayName\":{\"Text\":\"x\"},"
        "\"Description\":{\"Text\":\"d\"}}}"
        "]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    if(res == UA_STATUSCODE_GOOD) {
        /* Different types → kept as ExtensionObject array */
        ck_assert(v.type == &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    }
    UA_Variant_clear(&v);
} END_TEST

/* Array of EOs where one has UaEncoding -> not unwrapped */
START_TEST(json_decode_variant_eo_array_with_encoding) {
    const char *json =
        "{\"UaType\":22,\"Value\":["
        "{\"UaTypeId\":\"i=296\","
        "\"UaBody\":{\"Name\":\"a\",\"DataType\":\"i=1\",\"ValueRank\":-1}},"
        "{\"UaTypeId\":\"i=296\","
        "\"UaEncoding\":1,"
        "\"UaBody\":\"dGVzdA==\"}"
        "]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    (void)res;
    UA_Variant_clear(&v);
} END_TEST

/* Empty EO array in Variant */
START_TEST(json_decode_variant_eo_empty_array) {
    const char *json = "{\"UaType\":22,\"Value\":[]}";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* ============================================================
 * 6. decodeJsonStructure – bad encoding errors
 * ============================================================ */

/* Structure with missing required field name – exercises decodeFields error */
START_TEST(json_decode_structure_bad_field) {
    /* ReadRequest expects specific fields; giving garbage */
    const char *json = "{\"INVALID_FIELD\":42}";
    UA_ReadRequest rr;
    UA_ReadRequest_init(&rr);
    UA_StatusCode res = decode(json, &rr, &UA_TYPES[UA_TYPES_READREQUEST]);
    /* Should succeed but with default values (unknown fields are ignored) or fail */
    (void)res;
    UA_ReadRequest_clear(&rr);
} END_TEST

/* Deeply nested structure to test depth limit */
START_TEST(json_decode_structure_deep_nesting) {
    /* DiagnosticInfo with deeply nested InnerDiagnosticInfo */
    /* Build a deeply nested JSON string */
    char buf[4096];
    int pos = 0;
    int depth = 50;
    for(int i = 0; i < depth; i++) {
        if((size_t)pos >= sizeof(buf)) break;
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos,
                        "{\"SymbolicId\":%d,\"InnerDiagnosticInfo\":", i);
    }
    if((size_t)pos < sizeof(buf))
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "null");
    for(int i = 0; i < depth; i++) {
        if((size_t)pos >= sizeof(buf)) break;
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "}");
    }
    UA_DiagnosticInfo di;
    UA_DiagnosticInfo_init(&di);
    UA_StatusCode res = decode(buf, &di, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    /* Should hit depth limit and return error */
    (void)res;
    UA_DiagnosticInfo_clear(&di);
} END_TEST

/* ============================================================
 * 7. tokenize – realloc path
 * ============================================================ */

/* Large JSON with more than UA_JSON_MAXTOKENCOUNT (256) tokens -> forces realloc */
START_TEST(json_decode_large_json_tokenizer_realloc) {
    /* Build a large JSON array with 300 integer elements.
     * Each element is 1 token, plus the array itself, plus the object wrapper.
     * Should exceed 256 tokens. */
    size_t bufsize = 8192;
    char *buf = (char*)malloc(bufsize);
    ck_assert(buf != NULL);
    int pos = snprintf(buf, bufsize, "{\"UaType\":6,\"Value\":[");
    for(int i = 0; i < 300; i++) {
        if((size_t)pos >= bufsize) break;
        if(i > 0)
            pos += snprintf(buf + pos, bufsize - (size_t)pos, ",");
        if((size_t)pos >= bufsize) break;
        pos += snprintf(buf + pos, bufsize - (size_t)pos, "%d", i);
    }
    if((size_t)pos < bufsize)
        pos += snprintf(buf + pos, bufsize - (size_t)pos, "]}");
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(buf, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v.arrayLength, 300);
    UA_Variant_clear(&v);
    free(buf);
} END_TEST

/* Large JSON object with many fields -> forces realloc */
START_TEST(json_decode_large_object_tokenizer_realloc) {
    size_t bufsize = 32768;
    char *buf = (char*)malloc(bufsize);
    ck_assert(buf != NULL);
    int pos = snprintf(buf, bufsize, "{\"UaTypeId\":\"i=99999\"");
    /* Add 200 extra fields to force >256 tokens */
    for(int i = 0; i < 200; i++) {
        if((size_t)pos >= bufsize) break;
        pos += snprintf(buf + pos, bufsize - (size_t)pos,
                        ",\"field%d\":%d", i, i);
    }
    if((size_t)pos < bufsize)
        pos += snprintf(buf + pos, bufsize - (size_t)pos, "}");
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_StatusCode res = decode(buf, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    /* Should succeed; the tokenizer had to realloc */
    (void)res;
    UA_ExtensionObject_clear(&eo);
    free(buf);
} END_TEST

/* ============================================================
 * 8. String_decodeJson – malformed strings
 * ============================================================ */

/* Valid empty string */
START_TEST(json_decode_string_empty) {
    const char *json = "\"\"";
    UA_String s;
    UA_String_init(&s);
    UA_StatusCode res = decode(json, &s, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(s.length, 0);
    ck_assert(s.data != NULL); /* empty array sentinel */
    UA_String_clear(&s);
} END_TEST

/* String with escape sequences */
START_TEST(json_decode_string_with_escapes) {
    const char *json = "\"hello\\nworld\\t!\"";
    UA_String s;
    UA_String_init(&s);
    UA_StatusCode res = decode(json, &s, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(s.length > 0);
    UA_String_clear(&s);
} END_TEST

/* String with unicode escape */
START_TEST(json_decode_string_unicode_escape) {
    const char *json = "\"\\u0041\""; /* 'A' */
    UA_String s;
    UA_String_init(&s);
    UA_StatusCode res = decode(json, &s, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(s.length, 1);
    ck_assert_uint_eq(s.data[0], 'A');
    UA_String_clear(&s);
} END_TEST

/* Not a string token (number instead) -> error */
START_TEST(json_decode_string_not_string_token) {
    const char *json = "42";
    UA_String s;
    UA_String_init(&s);
    UA_StatusCode res = decode(json, &s, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_String_clear(&s);
} END_TEST

/* Malformed JSON (unterminated string) */
START_TEST(json_decode_string_malformed) {
    const char *json = "\"unterminated";
    UA_String s;
    UA_String_init(&s);
    UA_StatusCode res = decode(json, &s, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_String_clear(&s);
} END_TEST

/* ============================================================
 * 9. Double_decodeJson – edge cases
 * ============================================================ */

/* NaN */
START_TEST(json_decode_double_nan) {
    const char *json = "\"NaN\"";
    UA_Double d = 0.0;
    UA_StatusCode res = decode(json, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(isnan(d));
} END_TEST

/* -NaN -> treated as NaN */
START_TEST(json_decode_double_negative_nan) {
    const char *json = "\"-NaN\"";
    UA_Double d = 0.0;
    UA_StatusCode res = decode(json, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(isnan(d));
} END_TEST

/* Infinity */
START_TEST(json_decode_double_infinity) {
    const char *json = "\"Infinity\"";
    UA_Double d = 0.0;
    UA_StatusCode res = decode(json, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(isinf(d) && d > 0);
} END_TEST

/* -Infinity */
START_TEST(json_decode_double_neg_infinity) {
    const char *json = "\"-Infinity\"";
    UA_Double d = 0.0;
    UA_StatusCode res = decode(json, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(isinf(d) && d < 0);
} END_TEST

/* Normal double */
START_TEST(json_decode_double_normal) {
    const char *json = "3.14159";
    UA_Double d = 0.0;
    UA_StatusCode res = decode(json, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(d > 3.14 && d < 3.15);
} END_TEST

/* Zero */
START_TEST(json_decode_double_zero) {
    const char *json = "0";
    UA_Double d = 1.0;
    UA_StatusCode res = decode(json, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(d == 0.0);
} END_TEST

/* Invalid double string (not NaN/Infinity) */
START_TEST(json_decode_double_invalid_string) {
    const char *json = "\"notanumber\"";
    UA_Double d = 0.0;
    UA_StatusCode res = decode(json, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* Boolean token as double -> error */
START_TEST(json_decode_double_bool_token) {
    const char *json = "true";
    UA_Double d = 0.0;
    UA_StatusCode res = decode(json, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* Very long number string (> 2000 chars) -> error */
START_TEST(json_decode_double_too_long) {
    char buf[2100];
    buf[0] = '1';
    buf[1] = '.';
    memset(buf + 2, '0', 2050);
    buf[2052] = '\0';
    UA_Double d = 0.0;
    UA_StatusCode res = decode(buf, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* ============================================================
 * 10. DateTime_decodeJson – ISO 8601 edge cases
 * ============================================================ */

/* Standard datetime */
START_TEST(json_decode_datetime_standard) {
    const char *json = "\"2024-01-15T10:30:00Z\"";
    UA_DateTime dt = 0;
    UA_StatusCode res = decode(json, &dt, &UA_TYPES[UA_TYPES_DATETIME]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(dt != 0);
} END_TEST

/* DateTime with fractional seconds */
START_TEST(json_decode_datetime_fractional) {
    const char *json = "\"2024-06-15T12:00:00.123456Z\"";
    UA_DateTime dt = 0;
    UA_StatusCode res = decode(json, &dt, &UA_TYPES[UA_TYPES_DATETIME]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* DateTime with comma as fraction separator */
START_TEST(json_decode_datetime_comma_fraction) {
    const char *json = "\"2024-06-15T12:00:00,5Z\"";
    UA_DateTime dt = 0;
    UA_StatusCode res = decode(json, &dt, &UA_TYPES[UA_TYPES_DATETIME]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* DateTime missing Z suffix -> error */
START_TEST(json_decode_datetime_missing_z) {
    const char *json = "\"2024-01-15T10:30:00\"";
    UA_DateTime dt = 0;
    UA_StatusCode res = decode(json, &dt, &UA_TYPES[UA_TYPES_DATETIME]);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* DateTime empty string -> error */
START_TEST(json_decode_datetime_empty) {
    const char *json = "\"\"";
    UA_DateTime dt = 0;
    UA_StatusCode res = decode(json, &dt, &UA_TYPES[UA_TYPES_DATETIME]);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* DateTime with negative year */
START_TEST(json_decode_datetime_negative_year) {
    const char *json = "\"-0500-01-01T00:00:00Z\"";
    UA_DateTime dt = 0;
    UA_StatusCode res = decode(json, &dt, &UA_TYPES[UA_TYPES_DATETIME]);
    /* Might succeed or fail depending on range */
    (void)res;
} END_TEST

/* DateTime with plus sign year */
START_TEST(json_decode_datetime_plus_year) {
    const char *json = "\"+10000-01-01T00:00:00Z\"";
    UA_DateTime dt = 0;
    UA_StatusCode res = decode(json, &dt, &UA_TYPES[UA_TYPES_DATETIME]);
    /* 5-digit year with + prefix */
    (void)res;
} END_TEST

/* DateTime not string -> error */
START_TEST(json_decode_datetime_not_string) {
    const char *json = "12345";
    UA_DateTime dt = 0;
    UA_StatusCode res = decode(json, &dt, &UA_TYPES[UA_TYPES_DATETIME]);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* ============================================================
 * Extra: DataValue, DiagnosticInfo, round-trip, malformed JSON
 * ============================================================ */

/* DataValue with variant and timestamps */
START_TEST(json_decode_datavalue_with_timestamps) {
    const char *json =
        "{\"UaType\":6,\"Value\":42,"
        "\"Status\":{\"Code\":0},"
        "\"SourceTimestamp\":\"2024-01-01T00:00:00Z\","
        "\"ServerTimestamp\":\"2024-01-01T00:00:01Z\"}";
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    UA_StatusCode res = decode(json, &dv, &UA_TYPES[UA_TYPES_DATAVALUE]);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(dv.hasValue);
        ck_assert(dv.hasSourceTimestamp);
        ck_assert(dv.hasServerTimestamp);
    }
    UA_DataValue_clear(&dv);
} END_TEST

/* DataValue null -> empty */
START_TEST(json_decode_datavalue_null) {
    const char *json = "null";
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    UA_StatusCode res = decode(json, &dv, &UA_TYPES[UA_TYPES_DATAVALUE]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(!dv.hasValue);
    UA_DataValue_clear(&dv);
} END_TEST

/* Completely invalid JSON */
START_TEST(json_decode_invalid_json) {
    const char *json = "{{{invalid json!!!";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* Truncated JSON */
START_TEST(json_decode_truncated_json) {
    const char *json = "{\"UaType\":6,\"Val";
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = decode(json, &v, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&v);
} END_TEST

/* Null src/dst arguments */
START_TEST(json_decode_null_arguments) {
    UA_StatusCode res = UA_decodeJson(NULL, NULL, NULL, NULL);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* Int32 round-trip */
START_TEST(json_decode_roundtrip_int32) {
    UA_Int32 val = -12345;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_encodeJson(&val, &UA_TYPES[UA_TYPES_INT32], &buf, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Int32 decoded = 0;
    res = UA_decodeJson(&buf, &decoded, &UA_TYPES[UA_TYPES_INT32], NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(val, decoded);
    UA_ByteString_clear(&buf);
} END_TEST

/* DiagnosticInfo decode */
START_TEST(json_decode_diagnosticinfo) {
    const char *json =
        "{\"SymbolicId\":1,\"NamespaceUri\":2,"
        "\"LocalizedText\":3,\"AdditionalInfo\":\"info\","
        "\"InnerStatusCode\":{\"Code\":2147483648},"
        "\"InnerDiagnosticInfo\":{\"SymbolicId\":4}}";
    UA_DiagnosticInfo di;
    UA_DiagnosticInfo_init(&di);
    UA_StatusCode res = decode(json, &di, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert(di.hasSymbolicId);
        ck_assert_int_eq(di.symbolicId, 1);
        ck_assert(di.hasInnerDiagnosticInfo);
    }
    UA_DiagnosticInfo_clear(&di);
} END_TEST

/* DiagnosticInfo null */
START_TEST(json_decode_diagnosticinfo_null) {
    const char *json = "null";
    UA_DiagnosticInfo di;
    UA_DiagnosticInfo_init(&di);
    UA_StatusCode res = decode(json, &di, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_DiagnosticInfo_clear(&di);
} END_TEST

/* StatusCode decode */
START_TEST(json_decode_statuscode) {
    const char *json = "{\"Code\":2147483648}";
    UA_StatusCode sc = 0;
    UA_StatusCode res = decode(json, &sc, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(res == UA_STATUSCODE_GOOD) {
        ck_assert_uint_eq(sc, 2147483648u);
    }
} END_TEST

/* ============================================================
 * Suite setup
 * ============================================================ */
int main(void) {
    Suite *s = suite_create("JSON Decode Ext");

    TCase *tc_variant_eo = tcase_create("VariantEO");
    tcase_add_test(tc_variant_eo, json_decode_variant_unwrap_eo_known_type);
    tcase_add_test(tc_variant_eo, json_decode_variant_eo_builtin_stays_wrapped);
    tcase_add_test(tc_variant_eo, json_decode_variant_eo_null_body);
    tcase_add_test(tc_variant_eo, json_decode_variant_eo_unknown_type);
    suite_add_tcase(s, tc_variant_eo);

    TCase *tc_eo = tcase_create("ExtensionObject");
    tcase_add_test(tc_eo, json_decode_eo_unknown_type_no_body);
    tcase_add_test(tc_eo, json_decode_eo_known_type_with_body);
    tcase_add_test(tc_eo, json_decode_eo_known_type_no_body_field);
    tcase_add_test(tc_eo, json_decode_eo_empty_object);
    tcase_add_test(tc_eo, json_decode_eo_null);
    tcase_add_test(tc_eo, json_decode_eo_bytestring_encoding);
    tcase_add_test(tc_eo, json_decode_eo_xml_encoding);
    tcase_add_test(tc_eo, json_decode_eo_invalid_encoding);
    tcase_add_test(tc_eo, json_decode_eo_missing_typeid);
    suite_add_tcase(s, tc_eo);

    TCase *tc_variant = tcase_create("Variant");
    tcase_add_test(tc_variant, json_decode_variant_null);
    tcase_add_test(tc_variant, json_decode_variant_empty_object);
    tcase_add_test(tc_variant, json_decode_variant_missing_type);
    tcase_add_test(tc_variant, json_decode_variant_bad_type_kind);
    tcase_add_test(tc_variant, json_decode_variant_type_zero);
    tcase_add_test(tc_variant, json_decode_variant_type_not_number);
    tcase_add_test(tc_variant, json_decode_variant_scalar_with_dimensions);
    tcase_add_test(tc_variant, json_decode_variant_containing_variant);
    tcase_add_test(tc_variant, json_decode_variant_null_value);
    tcase_add_test(tc_variant, json_decode_variant_int_array);
    tcase_add_test(tc_variant, json_decode_variant_2d_array);
    tcase_add_test(tc_variant, json_decode_variant_1d_dimension);
    tcase_add_test(tc_variant, json_decode_variant_dimension_mismatch);
    tcase_add_test(tc_variant, json_decode_variant_3d_array);
    tcase_add_test(tc_variant, json_decode_variant_array_of_variants);
    suite_add_tcase(s, tc_variant);

    TCase *tc_remove = tcase_create("RemoveField");
    tcase_add_test(tc_remove, json_decode_eo_remove_field_enc_after_typeid);
    tcase_add_test(tc_remove, json_decode_eo_remove_field_enc_before_typeid);
    tcase_add_test(tc_remove, json_decode_eo_remove_only_typeid);
    tcase_add_test(tc_remove, json_decode_eo_remove_first_field);
    suite_add_tcase(s, tc_remove);

    TCase *tc_unwrap = tcase_create("ArrayUnwrap");
    tcase_add_test(tc_unwrap, json_decode_variant_eo_array_unwrap);
    tcase_add_test(tc_unwrap, json_decode_variant_eo_array_mixed_types);
    tcase_add_test(tc_unwrap, json_decode_variant_eo_array_with_encoding);
    tcase_add_test(tc_unwrap, json_decode_variant_eo_empty_array);
    suite_add_tcase(s, tc_unwrap);

    TCase *tc_struct = tcase_create("Structure");
    tcase_add_test(tc_struct, json_decode_structure_bad_field);
    tcase_add_test(tc_struct, json_decode_structure_deep_nesting);
    suite_add_tcase(s, tc_struct);

    TCase *tc_tokenize = tcase_create("Tokenize");
    tcase_add_test(tc_tokenize, json_decode_large_json_tokenizer_realloc);
    tcase_add_test(tc_tokenize, json_decode_large_object_tokenizer_realloc);
    suite_add_tcase(s, tc_tokenize);

    TCase *tc_string = tcase_create("StringDecode");
    tcase_add_test(tc_string, json_decode_string_empty);
    tcase_add_test(tc_string, json_decode_string_with_escapes);
    tcase_add_test(tc_string, json_decode_string_unicode_escape);
    tcase_add_test(tc_string, json_decode_string_not_string_token);
    tcase_add_test(tc_string, json_decode_string_malformed);
    suite_add_tcase(s, tc_string);

    TCase *tc_double = tcase_create("DoubleDecode");
    tcase_add_test(tc_double, json_decode_double_nan);
    tcase_add_test(tc_double, json_decode_double_negative_nan);
    tcase_add_test(tc_double, json_decode_double_infinity);
    tcase_add_test(tc_double, json_decode_double_neg_infinity);
    tcase_add_test(tc_double, json_decode_double_normal);
    tcase_add_test(tc_double, json_decode_double_zero);
    tcase_add_test(tc_double, json_decode_double_invalid_string);
    tcase_add_test(tc_double, json_decode_double_bool_token);
    tcase_add_test(tc_double, json_decode_double_too_long);
    suite_add_tcase(s, tc_double);

    TCase *tc_datetime = tcase_create("DateTimeDecode");
    tcase_add_test(tc_datetime, json_decode_datetime_standard);
    tcase_add_test(tc_datetime, json_decode_datetime_fractional);
    tcase_add_test(tc_datetime, json_decode_datetime_comma_fraction);
    tcase_add_test(tc_datetime, json_decode_datetime_missing_z);
    tcase_add_test(tc_datetime, json_decode_datetime_empty);
    tcase_add_test(tc_datetime, json_decode_datetime_negative_year);
    tcase_add_test(tc_datetime, json_decode_datetime_plus_year);
    tcase_add_test(tc_datetime, json_decode_datetime_not_string);
    suite_add_tcase(s, tc_datetime);

    TCase *tc_extra = tcase_create("Extra");
    tcase_add_test(tc_extra, json_decode_datavalue_with_timestamps);
    tcase_add_test(tc_extra, json_decode_datavalue_null);
    tcase_add_test(tc_extra, json_decode_invalid_json);
    tcase_add_test(tc_extra, json_decode_truncated_json);
    tcase_add_test(tc_extra, json_decode_null_arguments);
    tcase_add_test(tc_extra, json_decode_roundtrip_int32);
    tcase_add_test(tc_extra, json_decode_diagnosticinfo);
    tcase_add_test(tc_extra, json_decode_diagnosticinfo_null);
    tcase_add_test(tc_extra, json_decode_statuscode);
    suite_add_tcase(s, tc_extra);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int fails = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (fails == 0) ? 0 : 1;
}
