/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Type ordering, range operations and struct member tests */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <check.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * Custom type definitions for testing structureOrder with optional fields,
 * array members, and union types.
 * ======================================================================== */

/* --- Opt struct: struct with optional scalar and optional string fields --- */
typedef struct {
    UA_Int16 a;
    UA_Float *b;    /* optional */
    UA_Float *c;    /* optional */
    UA_String *d;   /* optional */
} Opt;

static UA_DataTypeMember Opt_members[4] = {
    {
        UA_TYPENAME("a")
        &UA_TYPES[UA_TYPES_INT16],
        0,
        false, /* isArray */
        false  /* isOptional */
    },
    {
        UA_TYPENAME("b")
        &UA_TYPES[UA_TYPES_FLOAT],
        offsetof(Opt,b) - offsetof(Opt,a) - sizeof(UA_Int16),
        false,
        true   /* optional */
    },
    {
        UA_TYPENAME("c")
        &UA_TYPES[UA_TYPES_FLOAT],
        offsetof(Opt,c) - offsetof(Opt,b) - sizeof(void *),
        false,
        true
    },
    {
        UA_TYPENAME("d")
        &UA_TYPES[UA_TYPES_STRING],
        offsetof(Opt,d) - offsetof(Opt,c) - sizeof(void *),
        false,
        true
    }
};

static UA_DataType OptType = {
    UA_TYPENAME("Opt")
    {1, UA_NODEIDTYPE_NUMERIC, {4242}},
    {1, UA_NODEIDTYPE_NUMERIC, {5}},
    {1, UA_NODEIDTYPE_NUMERIC, {6}},
    sizeof(Opt),
    UA_DATATYPEKIND_OPTSTRUCT,
    false,
    false,
    4,
    Opt_members
};

/* --- OptArray struct: struct with required array + optional array members --- */
typedef struct {
    UA_String description;
    size_t bSize;
    UA_String *b;
    size_t cSize;
    UA_Float *c;  /* optional array */
} OptArray;

static UA_DataTypeMember OptArray_members[3] = {
    {
        UA_TYPENAME("description")
        &UA_TYPES[UA_TYPES_STRING],
        0,
        false,
        false
    },
    {
        UA_TYPENAME("b")
        &UA_TYPES[UA_TYPES_STRING],
        offsetof(OptArray, bSize) - offsetof(OptArray, description) - sizeof(UA_String),
        true,   /* isArray */
        false
    },
    {
        UA_TYPENAME("c")
        &UA_TYPES[UA_TYPES_FLOAT],
        offsetof(OptArray, cSize) - offsetof(OptArray, b) - sizeof(void *),
        true,   /* isArray */
        true    /* optional */
    }
};

static UA_DataType OptArrayType = {
    UA_TYPENAME("OptArray")
    {1, UA_NODEIDTYPE_NUMERIC, {4243}},
    {1, UA_NODEIDTYPE_NUMERIC, {1337}},
    {1, UA_NODEIDTYPE_NUMERIC, {1338}},
    sizeof(OptArray),
    UA_DATATYPEKIND_OPTSTRUCT,
    false,
    false,
    3,
    OptArray_members
};

/* --- Union type --- */
typedef enum {
    UA_UNISWITCH_NONE = 0,
    UA_UNISWITCH_OPTIONA = 1,
    UA_UNISWITCH_OPTIONB = 2
} UA_UniSwitch;

typedef struct {
    UA_UniSwitch switchField;
    union {
        UA_Double optionA;
        UA_String optionB;
    } fields;
} Uni;

static UA_DataTypeMember Uni_members[2] = {
    {
        UA_TYPENAME("optionA")
        &UA_TYPES[UA_TYPES_DOUBLE],
        offsetof(Uni, fields.optionA),
        false,
        false
    },
    {
        UA_TYPENAME("optionB")
        &UA_TYPES[UA_TYPES_STRING],
        offsetof(Uni, fields.optionB),
        false,
        false
    }
};

static UA_DataType UniType = {
    UA_TYPENAME("Uni")
    {1, UA_NODEIDTYPE_NUMERIC, {4245}},
    {1, UA_NODEIDTYPE_NUMERIC, {13338}},
    {1, UA_NODEIDTYPE_NUMERIC, {13339}},
    sizeof(Uni),
    UA_DATATYPEKIND_UNION,
    false,
    false,
    2,
    Uni_members
};

/* ========================================================================
 * 1. structureOrder – comparing structures with optional/pointer/array members
 * ======================================================================== */

START_TEST(structureOrder_opt_both_null) {
    /* Both optional fields NULL => equal */
    Opt a, b;
    memset(&a, 0, sizeof(Opt));
    memset(&b, 0, sizeof(Opt));
    a.a = 5; b.a = 5;
    ck_assert_int_eq(UA_order(&a, &b, &OptType), UA_ORDER_EQ);
} END_TEST

START_TEST(structureOrder_opt_one_set) {
    /* a has optional b set, b does not => a > b (MORE) */
    Opt o1, o2;
    memset(&o1, 0, sizeof(Opt));
    memset(&o2, 0, sizeof(Opt));
    o1.a = 5; o2.a = 5;
    UA_Float val = 1.0f;
    o1.b = &val;
    ck_assert_int_eq(UA_order(&o1, &o2, &OptType), UA_ORDER_MORE);
    ck_assert_int_eq(UA_order(&o2, &o1, &OptType), UA_ORDER_LESS);
} END_TEST

START_TEST(structureOrder_opt_both_set_equal) {
    Opt o1, o2;
    memset(&o1, 0, sizeof(Opt));
    memset(&o2, 0, sizeof(Opt));
    o1.a = 5; o2.a = 5;
    UA_Float v1 = 3.0f, v2 = 3.0f;
    o1.b = &v1; o2.b = &v2;
    ck_assert_int_eq(UA_order(&o1, &o2, &OptType), UA_ORDER_EQ);
} END_TEST

START_TEST(structureOrder_opt_both_set_differ) {
    Opt o1, o2;
    memset(&o1, 0, sizeof(Opt));
    memset(&o2, 0, sizeof(Opt));
    o1.a = 5; o2.a = 5;
    UA_Float v1 = 1.0f, v2 = 9.0f;
    o1.b = &v1; o2.b = &v2;
    ck_assert_int_eq(UA_order(&o1, &o2, &OptType), UA_ORDER_LESS);
} END_TEST

START_TEST(structureOrder_opt_same_pointer) {
    /* Same pointer => EQ (shortcut in structureOrder) */
    Opt o1, o2;
    memset(&o1, 0, sizeof(Opt));
    memset(&o2, 0, sizeof(Opt));
    o1.a = 5; o2.a = 5;
    UA_Float v = 2.0f;
    o1.b = &v; o2.b = &v;
    ck_assert_int_eq(UA_order(&o1, &o2, &OptType), UA_ORDER_EQ);
} END_TEST

START_TEST(structureOrder_opt_string_differ) {
    /* Optional string field comparison (string order is length-first) */
    Opt o1, o2;
    memset(&o1, 0, sizeof(Opt));
    memset(&o2, 0, sizeof(Opt));
    o1.a = 5; o2.a = 5;
    UA_String s1 = UA_STRING("aaa");
    UA_String s2 = UA_STRING("zzz");
    o1.d = &s1; o2.d = &s2;
    ck_assert_int_eq(UA_order(&o1, &o2, &OptType), UA_ORDER_LESS);
} END_TEST

START_TEST(structureOrder_required_array_differ) {
    /* Required array member with different sizes */
    OptArray o1, o2;
    memset(&o1, 0, sizeof(OptArray));
    memset(&o2, 0, sizeof(OptArray));
    o1.description = UA_STRING("x");
    o2.description = UA_STRING("x");
    UA_String arr1[] = { UA_STRING_STATIC("a") };
    UA_String arr2[] = { UA_STRING_STATIC("a"), UA_STRING_STATIC("b") };
    o1.bSize = 1; o1.b = arr1;
    o2.bSize = 2; o2.b = arr2;
    ck_assert_int_eq(UA_order(&o1, &o2, &OptArrayType), UA_ORDER_LESS);
} END_TEST

START_TEST(structureOrder_required_array_content_differ) {
    /* Same-size arrays with different content */
    OptArray o1, o2;
    memset(&o1, 0, sizeof(OptArray));
    memset(&o2, 0, sizeof(OptArray));
    o1.description = UA_STRING("x");
    o2.description = UA_STRING("x");
    UA_String arr1[] = { UA_STRING_STATIC("aaa") };
    UA_String arr2[] = { UA_STRING_STATIC("zzz") };
    o1.bSize = 1; o1.b = arr1;
    o2.bSize = 1; o2.b = arr2;
    ck_assert_int_eq(UA_order(&o1, &o2, &OptArrayType), UA_ORDER_LESS);
} END_TEST

START_TEST(structureOrder_opt_array_differ) {
    /* Optional array field: one NULL, one present */
    OptArray o1, o2;
    memset(&o1, 0, sizeof(OptArray));
    memset(&o2, 0, sizeof(OptArray));
    o1.description = UA_STRING("x");
    o2.description = UA_STRING("x");
    /* both required arrays empty */
    o1.bSize = 0; o1.b = NULL;
    o2.bSize = 0; o2.b = NULL;
    /* optional array: o1 has none, o2 has one element */
    UA_Float arr[] = {1.0f, 2.0f};
    o2.cSize = 2; o2.c = arr;
    UA_Order o = UA_order(&o1, &o2, &OptArrayType);
    ck_assert(o != UA_ORDER_EQ);
} END_TEST

START_TEST(structureOrder_first_member_differs) {
    /* Compare on first (required) member */
    Opt o1, o2;
    memset(&o1, 0, sizeof(Opt));
    memset(&o2, 0, sizeof(Opt));
    o1.a = 1; o2.a = 2;
    ck_assert_int_eq(UA_order(&o1, &o2, &OptType), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&o2, &o1, &OptType), UA_ORDER_MORE);
} END_TEST

/* ========================================================================
 * 2. diagnosticInfoOrder – all sub-fields
 * ======================================================================== */

START_TEST(diagInfo_empty_equal) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_EQ);
} END_TEST

START_TEST(diagInfo_symbolicId_differ) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasSymbolicId = true; a.symbolicId = 1;
    b.hasSymbolicId = true; b.symbolicId = 5;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);
} END_TEST

START_TEST(diagInfo_symbolicId_has_vs_not) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    b.hasSymbolicId = true; b.symbolicId = 1;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&b, &a, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_MORE);
} END_TEST

START_TEST(diagInfo_namespaceUri_differ) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasNamespaceUri = true; a.namespaceUri = 10;
    b.hasNamespaceUri = true; b.namespaceUri = 20;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);
} END_TEST

START_TEST(diagInfo_localizedText_differ) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasLocalizedText = true; a.localizedText = 1;
    b.hasLocalizedText = true; b.localizedText = 2;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);
} END_TEST

START_TEST(diagInfo_locale_differ) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasLocale = true; a.locale = 1;
    b.hasLocale = true; b.locale = 5;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);
} END_TEST

START_TEST(diagInfo_additionalInfo_differ) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasAdditionalInfo = true;
    a.additionalInfo = UA_STRING("aaa");
    b.hasAdditionalInfo = true;
    b.additionalInfo = UA_STRING("zzz");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);
} END_TEST

START_TEST(diagInfo_additionalInfo_equal) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasAdditionalInfo = true;
    a.additionalInfo = UA_STRING("same");
    b.hasAdditionalInfo = true;
    b.additionalInfo = UA_STRING("same");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_EQ);
} END_TEST

START_TEST(diagInfo_innerStatusCode_differ) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasInnerStatusCode = true; a.innerStatusCode = UA_STATUSCODE_GOOD;
    b.hasInnerStatusCode = true; b.innerStatusCode = UA_STATUSCODE_BADUNEXPECTEDERROR;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);
} END_TEST

START_TEST(diagInfo_innerDiagnosticInfo_both_null) {
    /* Both have hasInnerDiagnosticInfo but both pointers NULL */
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasInnerDiagnosticInfo = true;
    b.hasInnerDiagnosticInfo = true;
    a.innerDiagnosticInfo = NULL;
    b.innerDiagnosticInfo = NULL;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_EQ);
} END_TEST

START_TEST(diagInfo_innerDiagnosticInfo_one_null) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasInnerDiagnosticInfo = true;
    b.hasInnerDiagnosticInfo = true;

    UA_DiagnosticInfo inner;
    UA_DiagnosticInfo_init(&inner);
    inner.hasSymbolicId = true;
    inner.symbolicId = 42;

    a.innerDiagnosticInfo = &inner;
    b.innerDiagnosticInfo = NULL;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_MORE);
    ck_assert_int_eq(UA_order(&b, &a, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);
} END_TEST

START_TEST(diagInfo_innerDiagnosticInfo_recurse) {
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasInnerDiagnosticInfo = true;
    b.hasInnerDiagnosticInfo = true;

    UA_DiagnosticInfo innerA, innerB;
    UA_DiagnosticInfo_init(&innerA);
    UA_DiagnosticInfo_init(&innerB);
    innerA.hasSymbolicId = true; innerA.symbolicId = 1;
    innerB.hasSymbolicId = true; innerB.symbolicId = 9;
    a.innerDiagnosticInfo = &innerA;
    b.innerDiagnosticInfo = &innerB;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_LESS);
} END_TEST

START_TEST(diagInfo_innerDiagnosticInfo_same_ptr) {
    /* Same pointer => EQ */
    UA_DiagnosticInfo a, b;
    UA_DiagnosticInfo_init(&a);
    UA_DiagnosticInfo_init(&b);
    a.hasInnerDiagnosticInfo = true;
    b.hasInnerDiagnosticInfo = true;

    UA_DiagnosticInfo inner;
    UA_DiagnosticInfo_init(&inner);
    inner.hasSymbolicId = true; inner.symbolicId = 42;
    a.innerDiagnosticInfo = &inner;
    b.innerDiagnosticInfo = &inner;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), UA_ORDER_EQ);
} END_TEST

/* ========================================================================
 * 3. dataValueOrder – all timestamp / picosecond fields
 * ======================================================================== */

START_TEST(dataValue_empty_equal) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_EQ);
} END_TEST

START_TEST(dataValue_hasValue_differ) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    a.hasValue = true;
    UA_Int32 v = 10;
    UA_Variant_setScalar(&a.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_MORE);
    ck_assert_int_eq(UA_order(&b, &a, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_LESS);
} END_TEST

START_TEST(dataValue_status_differ) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    a.hasStatus = true; a.status = UA_STATUSCODE_GOOD;
    b.hasStatus = true; b.status = UA_STATUSCODE_BADUNEXPECTEDERROR;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_LESS);
} END_TEST

START_TEST(dataValue_hasStatus_differ) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    a.hasStatus = false;
    b.hasStatus = true; b.status = UA_STATUSCODE_GOOD;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_LESS);
} END_TEST

START_TEST(dataValue_sourceTimestamp_differ) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    a.hasSourceTimestamp = true; a.sourceTimestamp = 100;
    b.hasSourceTimestamp = true; b.sourceTimestamp = 200;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_LESS);
} END_TEST

START_TEST(dataValue_serverTimestamp_differ) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    a.hasServerTimestamp = true; a.serverTimestamp = 50;
    b.hasServerTimestamp = true; b.serverTimestamp = 100;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_LESS);
} END_TEST

START_TEST(dataValue_sourcePicoseconds_differ) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    a.hasSourcePicoseconds = true; a.sourcePicoseconds = 1;
    b.hasSourcePicoseconds = true; b.sourcePicoseconds = 99;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_LESS);
} END_TEST

START_TEST(dataValue_serverPicoseconds_differ) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    a.hasServerPicoseconds = true; a.serverPicoseconds = 10;
    b.hasServerPicoseconds = true; b.serverPicoseconds = 20;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_LESS);
} END_TEST

START_TEST(dataValue_all_fields_equal) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    UA_Int32 v1 = 42, v2 = 42;
    a.hasValue = true; UA_Variant_setScalar(&a.value, &v1, &UA_TYPES[UA_TYPES_INT32]);
    b.hasValue = true; UA_Variant_setScalar(&b.value, &v2, &UA_TYPES[UA_TYPES_INT32]);
    a.hasStatus = true; a.status = UA_STATUSCODE_GOOD;
    b.hasStatus = true; b.status = UA_STATUSCODE_GOOD;
    a.hasSourceTimestamp = true; a.sourceTimestamp = 100;
    b.hasSourceTimestamp = true; b.sourceTimestamp = 100;
    a.hasServerTimestamp = true; a.serverTimestamp = 200;
    b.hasServerTimestamp = true; b.serverTimestamp = 200;
    a.hasSourcePicoseconds = true; a.sourcePicoseconds = 5;
    b.hasSourcePicoseconds = true; b.sourcePicoseconds = 5;
    a.hasServerPicoseconds = true; a.serverPicoseconds = 7;
    b.hasServerPicoseconds = true; b.serverPicoseconds = 7;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_EQ);
} END_TEST

START_TEST(dataValue_hasServerTimestamp_differ) {
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    b.hasServerTimestamp = true; b.serverTimestamp = 100;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_LESS);
} END_TEST

START_TEST(dataValue_hasPico_differ) {
    /* hasSourcePicoseconds differ */
    UA_DataValue a, b;
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    b.hasSourcePicoseconds = true; b.sourcePicoseconds = 1;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_LESS);
    /* hasServerPicoseconds differ */
    UA_DataValue_init(&a);
    UA_DataValue_init(&b);
    a.hasServerPicoseconds = true; a.serverPicoseconds = 1;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_DATAVALUE]), UA_ORDER_MORE);
} END_TEST

/* ========================================================================
 * 4. extensionObjectOrder
 * ======================================================================== */

START_TEST(extObj_nobody_equal) {
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    /* Default encoding is NOBODY */
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), UA_ORDER_EQ);
} END_TEST

START_TEST(extObj_encoding_differ) {
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    /* a is NOBODY (0), b is ENCODED_BYTESTRING (1) */
    b.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    b.content.encoded.typeId = UA_NODEID_NUMERIC(0, 1);
    b.content.encoded.body = UA_BYTESTRING("data");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), UA_ORDER_LESS);
} END_TEST

START_TEST(extObj_encoded_bytestring_differ) {
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    a.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    b.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    a.content.encoded.typeId = UA_NODEID_NUMERIC(0, 1);
    b.content.encoded.typeId = UA_NODEID_NUMERIC(0, 1);
    a.content.encoded.body = UA_BYTESTRING("aaa");
    b.content.encoded.body = UA_BYTESTRING("zzz");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), UA_ORDER_LESS);
} END_TEST

START_TEST(extObj_encoded_typeId_differ) {
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    a.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    b.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    a.content.encoded.typeId = UA_NODEID_NUMERIC(0, 1);
    b.content.encoded.typeId = UA_NODEID_NUMERIC(0, 2);
    a.content.encoded.body = UA_BYTESTRING("x");
    b.content.encoded.body = UA_BYTESTRING("x");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), UA_ORDER_LESS);
} END_TEST

START_TEST(extObj_encoded_xml) {
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    a.encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    b.encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    a.content.encoded.typeId = UA_NODEID_NUMERIC(0, 1);
    b.content.encoded.typeId = UA_NODEID_NUMERIC(0, 1);
    a.content.encoded.body = UA_BYTESTRING("<a/>");
    b.content.encoded.body = UA_BYTESTRING("<b/>");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), UA_ORDER_LESS);
} END_TEST

START_TEST(extObj_decoded_same_type) {
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    UA_Int32 v1 = 10, v2 = 20;
    a.encoding = UA_EXTENSIONOBJECT_DECODED;
    b.encoding = UA_EXTENSIONOBJECT_DECODED;
    a.content.decoded.type = &UA_TYPES[UA_TYPES_INT32];
    a.content.decoded.data = &v1;
    b.content.decoded.type = &UA_TYPES[UA_TYPES_INT32];
    b.content.decoded.data = &v2;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), UA_ORDER_LESS);
} END_TEST

START_TEST(extObj_decoded_diff_type) {
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    UA_Int32 v1 = 10;
    UA_Double v2 = 20.0;
    a.encoding = UA_EXTENSIONOBJECT_DECODED;
    b.encoding = UA_EXTENSIONOBJECT_DECODED;
    a.content.decoded.type = &UA_TYPES[UA_TYPES_INT32];
    a.content.decoded.data = &v1;
    b.content.decoded.type = &UA_TYPES[UA_TYPES_DOUBLE];
    b.content.decoded.data = &v2;
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert(o != UA_ORDER_EQ);
} END_TEST

START_TEST(extObj_decoded_nodelete) {
    /* DECODED_NODELETE should be normalized to DECODED for comparison */
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    UA_Int32 v1 = 42, v2 = 42;
    a.encoding = UA_EXTENSIONOBJECT_DECODED;
    b.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    a.content.decoded.type = &UA_TYPES[UA_TYPES_INT32];
    a.content.decoded.data = &v1;
    b.content.decoded.type = &UA_TYPES[UA_TYPES_INT32];
    b.content.decoded.data = &v2;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), UA_ORDER_EQ);
} END_TEST

START_TEST(extObj_decoded_null_type) {
    /* Both decoded with NULL type => EQ */
    UA_ExtensionObject a, b;
    UA_ExtensionObject_init(&a);
    UA_ExtensionObject_init(&b);
    a.encoding = UA_EXTENSIONOBJECT_DECODED;
    b.encoding = UA_EXTENSIONOBJECT_DECODED;
    a.content.decoded.type = NULL;
    b.content.decoded.type = NULL;
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), UA_ORDER_EQ);
} END_TEST

/* ========================================================================
 * 5. variantOrder – arrays, type mismatch, arrayDimensions
 * ======================================================================== */

START_TEST(variant_both_empty) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]), UA_ORDER_EQ);
} END_TEST

START_TEST(variant_type_mismatch) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);
    UA_Int32 v1 = 1;
    UA_Double v2 = 1.0;
    UA_Variant_setScalar(&a, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setScalar(&b, &v2, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert(o != UA_ORDER_EQ);
} END_TEST

START_TEST(variant_scalar_vs_array) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);
    UA_Int32 v1 = 1;
    UA_Int32 arr[] = {1, 2};
    UA_Variant_setScalar(&a, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setArray(&b, arr, 2, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]), UA_ORDER_LESS);
} END_TEST

START_TEST(variant_array_same_length_differ) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);
    UA_Int32 arr1[] = {1, 2, 3};
    UA_Int32 arr2[] = {1, 2, 4};
    UA_Variant_setArray(&a, arr1, 3, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setArray(&b, arr2, 3, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]), UA_ORDER_LESS);
} END_TEST

START_TEST(variant_array_diff_length) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);
    UA_Int32 arr1[] = {1, 2};
    UA_Int32 arr2[] = {1, 2, 3};
    UA_Variant_setArray(&a, arr1, 2, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setArray(&b, arr2, 3, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]), UA_ORDER_LESS);
} END_TEST

START_TEST(variant_arrayDimensions_differ) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);
    UA_Int32 arr1[] = {1, 2, 3, 4};
    UA_Int32 arr2[] = {1, 2, 3, 4};
    UA_Variant_setArray(&a, arr1, 4, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setArray(&b, arr2, 4, &UA_TYPES[UA_TYPES_INT32]);
    UA_UInt32 dimsA[] = {2, 2};
    UA_UInt32 dimsB[] = {4};
    a.arrayDimensions = dimsA;
    a.arrayDimensionsSize = 2;
    b.arrayDimensions = dimsB;
    b.arrayDimensionsSize = 1;
    /* a has 2 dimensions, b has 1 => a MORE */
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]), UA_ORDER_MORE);
    /* Reset to avoid double-free */
    a.arrayDimensions = NULL; a.arrayDimensionsSize = 0;
    b.arrayDimensions = NULL; b.arrayDimensionsSize = 0;
} END_TEST

START_TEST(variant_arrayDimensions_content_differ) {
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);
    UA_Int32 arr1[] = {1, 2, 3, 4, 5, 6};
    UA_Int32 arr2[] = {1, 2, 3, 4, 5, 6};
    UA_Variant_setArray(&a, arr1, 6, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setArray(&b, arr2, 6, &UA_TYPES[UA_TYPES_INT32]);
    UA_UInt32 dimsA[] = {2, 3};
    UA_UInt32 dimsB[] = {3, 2};
    a.arrayDimensions = dimsA; a.arrayDimensionsSize = 2;
    b.arrayDimensions = dimsB; b.arrayDimensionsSize = 2;
    /* Same dimensionsSize but different content */
    UA_Order o = UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]);
    ck_assert_int_eq(o, UA_ORDER_LESS);
    a.arrayDimensions = NULL; a.arrayDimensionsSize = 0;
    b.arrayDimensions = NULL; b.arrayDimensionsSize = 0;
} END_TEST

/* ========================================================================
 * 6. unionOrder
 * ======================================================================== */

START_TEST(union_both_none) {
    Uni a, b;
    memset(&a, 0, sizeof(Uni));
    memset(&b, 0, sizeof(Uni));
    ck_assert_int_eq(UA_order(&a, &b, &UniType), UA_ORDER_EQ);
} END_TEST

START_TEST(union_switch_differ) {
    Uni a, b;
    memset(&a, 0, sizeof(Uni));
    memset(&b, 0, sizeof(Uni));
    a.switchField = UA_UNISWITCH_OPTIONA;
    a.fields.optionA = 1.0;
    b.switchField = UA_UNISWITCH_OPTIONB;
    b.fields.optionB = UA_STRING("hello");
    ck_assert_int_eq(UA_order(&a, &b, &UniType), UA_ORDER_LESS);
} END_TEST

START_TEST(union_same_switch_optionA_differ) {
    Uni a, b;
    memset(&a, 0, sizeof(Uni));
    memset(&b, 0, sizeof(Uni));
    a.switchField = UA_UNISWITCH_OPTIONA;
    b.switchField = UA_UNISWITCH_OPTIONA;
    a.fields.optionA = 1.0;
    b.fields.optionA = 9.0;
    ck_assert_int_eq(UA_order(&a, &b, &UniType), UA_ORDER_LESS);
} END_TEST

START_TEST(union_same_switch_equal) {
    Uni a, b;
    memset(&a, 0, sizeof(Uni));
    memset(&b, 0, sizeof(Uni));
    a.switchField = UA_UNISWITCH_OPTIONA;
    b.switchField = UA_UNISWITCH_OPTIONA;
    a.fields.optionA = 5.5;
    b.fields.optionA = 5.5;
    ck_assert_int_eq(UA_order(&a, &b, &UniType), UA_ORDER_EQ);
} END_TEST

START_TEST(union_same_switch_optionB_differ) {
    Uni a, b;
    memset(&a, 0, sizeof(Uni));
    memset(&b, 0, sizeof(Uni));
    a.switchField = UA_UNISWITCH_OPTIONB;
    b.switchField = UA_UNISWITCH_OPTIONB;
    a.fields.optionB = UA_STRING("aaa");
    b.fields.optionB = UA_STRING("zzz");
    ck_assert_int_eq(UA_order(&a, &b, &UniType), UA_ORDER_LESS);
} END_TEST

/* ========================================================================
 * 7. UA_Variant_copyRange – multi-dimensional, strides
 * ======================================================================== */

START_TEST(copyRange_1d_basic) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Int32 arr[] = {10, 20, 30, 40, 50};
    UA_Variant_setArray(&src, arr, 5, &UA_TYPES[UA_TYPES_INT32]);

    UA_NumericRangeDimension dim = {1, 3};
    UA_NumericRange range = {1, &dim};
    UA_StatusCode ret = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.arrayLength, 3);
    UA_Int32 *out = (UA_Int32*)dst.data;
    ck_assert_int_eq(out[0], 20);
    ck_assert_int_eq(out[1], 30);
    ck_assert_int_eq(out[2], 40);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(copyRange_2d_matrix) {
    /* 3x4 matrix, copy a 2x2 sub-block */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Int32 arr[12];
    for(int i = 0; i < 12; i++) arr[i] = i;
    UA_Variant_setArray(&src, arr, 12, &UA_TYPES[UA_TYPES_INT32]);
    UA_UInt32 dims[] = {3, 4};
    src.arrayDimensions = dims;
    src.arrayDimensionsSize = 2;

    UA_NumericRangeDimension rdims[] = {{0, 1}, {1, 2}};
    UA_NumericRange range = {2, rdims};
    UA_StatusCode ret = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.arrayLength, 4);
    UA_Int32 *out = (UA_Int32*)dst.data;
    /* Row 0: elements 1,2; Row 1: elements 5,6 */
    ck_assert_int_eq(out[0], 1);
    ck_assert_int_eq(out[1], 2);
    ck_assert_int_eq(out[2], 5);
    ck_assert_int_eq(out[3], 6);
    src.arrayDimensions = NULL; src.arrayDimensionsSize = 0;
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(copyRange_scalar_string) {
    /* Copy range from a string (string-like scalar) */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_String s = UA_STRING("Hello World");
    UA_Variant_setScalar(&src, &s, &UA_TYPES[UA_TYPES_STRING]);

    UA_NumericRangeDimension dim = {0, 4};
    UA_NumericRange range = {1, &dim};
    UA_StatusCode ret = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    UA_String *out = (UA_String*)dst.data;
    ck_assert_uint_eq(out->length, 5);
    ck_assert(memcmp(out->data, "Hello", 5) == 0);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(copyRange_empty_variant) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_NumericRangeDimension dim = {0, 0};
    UA_NumericRange range = {1, &dim};
    ck_assert_uint_ne(UA_Variant_copyRange(&src, &dst, range), UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(copyRange_out_of_bounds) {
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArray(&src, arr, 3, &UA_TYPES[UA_TYPES_INT32]);

    UA_NumericRangeDimension dim = {10, 20};
    UA_NumericRange range = {1, &dim};
    ck_assert_uint_eq(UA_Variant_copyRange(&src, &dst, range),
                      UA_STATUSCODE_BADINDEXRANGENODATA);
} END_TEST

START_TEST(copyRange_dim_mismatch) {
    /* Array has 1 dim, range has 2 dims => BADINDEXRANGENODATA */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArray(&src, arr, 3, &UA_TYPES[UA_TYPES_INT32]);

    UA_NumericRangeDimension rdims[] = {{0, 1}, {0, 0}};
    UA_NumericRange range = {2, rdims};
    ck_assert_uint_eq(UA_Variant_copyRange(&src, &dst, range),
                      UA_STATUSCODE_BADINDEXRANGENODATA);
} END_TEST

/* ========================================================================
 * 8. Variant_setRange – multi-dimensional range-set
 * ======================================================================== */

START_TEST(setRange_1d_basic) {
    UA_Int32 *arr = (UA_Int32*)UA_Array_new(5, &UA_TYPES[UA_TYPES_INT32]);
    for(int i = 0; i < 5; i++) arr[i] = i * 10;
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setArray(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);

    UA_Int32 newVals[] = {99, 88};
    UA_NumericRangeDimension dim = {1, 2};
    UA_NumericRange range = {1, &dim};

    UA_StatusCode ret = UA_Variant_setRangeCopy(&v, newVals, 2, range);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((UA_Int32*)v.data)[1], 99);
    ck_assert_int_eq(((UA_Int32*)v.data)[2], 88);
    UA_Array_delete(arr, 5, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

START_TEST(setRange_size_mismatch) {
    UA_Int32 *arr = (UA_Int32*)UA_Array_new(5, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setArray(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);

    UA_Int32 newVals[] = {1}; /* Only 1 element but range is [1,3] => 3 elements */
    UA_NumericRangeDimension dim = {1, 3};
    UA_NumericRange range = {1, &dim};
    UA_StatusCode ret = UA_Variant_setRangeCopy(&v, newVals, 1, range);
    ck_assert_uint_ne(ret, UA_STATUSCODE_GOOD);
    UA_Array_delete(arr, 5, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

START_TEST(setRange_empty_variant) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 newVals[] = {1};
    UA_NumericRangeDimension dim = {0, 0};
    UA_NumericRange range = {1, &dim};
    ck_assert_uint_ne(UA_Variant_setRangeCopy(&v, newVals, 1, range), UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(setRangeCopy_string_array) {
    /* setRangeCopy on non-pointer-free type (uses copy path) */
    UA_String *arr = (UA_String*)UA_Array_new(3, &UA_TYPES[UA_TYPES_STRING]);
    arr[0] = UA_STRING_ALLOC("aaa");
    arr[1] = UA_STRING_ALLOC("bbb");
    arr[2] = UA_STRING_ALLOC("ccc");
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setArray(&v, arr, 3, &UA_TYPES[UA_TYPES_STRING]);

    UA_String newVal = UA_STRING("xxx");
    UA_NumericRangeDimension dim = {1, 1};
    UA_NumericRange range = {1, &dim};
    UA_StatusCode ret = UA_Variant_setRangeCopy(&v, &newVal, 1, range);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    UA_String *result = (UA_String*)v.data;
    ck_assert(UA_String_equal(&result[1], &newVal));
    UA_Array_delete(arr, 3, &UA_TYPES[UA_TYPES_STRING]);
} END_TEST

/* ========================================================================
 * 9. UA_Array_resize
 * ======================================================================== */

START_TEST(arrayResize_grow) {
    size_t size = 3;
    UA_Int32 *arr = (UA_Int32*)UA_Array_new(size, &UA_TYPES[UA_TYPES_INT32]);
    arr[0] = 1; arr[1] = 2; arr[2] = 3;
    void *p = arr;
    UA_StatusCode ret = UA_Array_resize(&p, &size, 5, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 5);
    arr = (UA_Int32*)p;
    ck_assert_int_eq(arr[0], 1);
    ck_assert_int_eq(arr[1], 2);
    ck_assert_int_eq(arr[2], 3);
    /* New elements zero-initialized */
    ck_assert_int_eq(arr[3], 0);
    ck_assert_int_eq(arr[4], 0);
    UA_Array_delete(arr, size, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

START_TEST(arrayResize_shrink) {
    size_t size = 5;
    UA_Int32 *arr = (UA_Int32*)UA_Array_new(size, &UA_TYPES[UA_TYPES_INT32]);
    for(size_t i = 0; i < 5; i++) arr[i] = (UA_Int32)(i * 10);
    void *p = arr;
    UA_StatusCode ret = UA_Array_resize(&p, &size, 2, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 2);
    arr = (UA_Int32*)p;
    ck_assert_int_eq(arr[0], 0);
    ck_assert_int_eq(arr[1], 10);
    UA_Array_delete(arr, size, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

START_TEST(arrayResize_to_zero) {
    size_t size = 3;
    UA_Int32 *arr = (UA_Int32*)UA_Array_new(size, &UA_TYPES[UA_TYPES_INT32]);
    void *p = arr;
    UA_StatusCode ret = UA_Array_resize(&p, &size, 0, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 0);
} END_TEST

START_TEST(arrayResize_same_size) {
    size_t size = 3;
    UA_Int32 *arr = (UA_Int32*)UA_Array_new(size, &UA_TYPES[UA_TYPES_INT32]);
    arr[0] = 42;
    void *p = arr;
    UA_StatusCode ret = UA_Array_resize(&p, &size, 3, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 3);
    ck_assert_int_eq(((UA_Int32*)p)[0], 42);
    UA_Array_delete(p, size, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

START_TEST(arrayResize_shrink_strings) {
    /* Non-pointer-free type => takes the deleteMembers path */
    size_t size = 3;
    UA_String *arr = (UA_String*)UA_Array_new(size, &UA_TYPES[UA_TYPES_STRING]);
    arr[0] = UA_STRING_ALLOC("hello");
    arr[1] = UA_STRING_ALLOC("world");
    arr[2] = UA_STRING_ALLOC("foo");
    void *p = arr;
    UA_StatusCode ret = UA_Array_resize(&p, &size, 1, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 1);
    UA_String *result = (UA_String*)p;
    UA_String expected = UA_STRING("hello");
    ck_assert(UA_String_equal(&result[0], &expected));
    UA_Array_delete(p, size, &UA_TYPES[UA_TYPES_STRING]);
} END_TEST

/* ========================================================================
 * 10. UA_DataType_getStructMember
 * ======================================================================== */

#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(getStructMember_found) {
    size_t offset = 0;
    const UA_DataType *mt = NULL;
    UA_Boolean isArray = false;
    UA_Boolean found = UA_DataType_getStructMember(
        &UA_TYPES[UA_TYPES_READVALUEID], "NodeId", &offset, &mt, &isArray);
    ck_assert(found);
    ck_assert_ptr_ne(mt, NULL);
    ck_assert(!isArray);
} END_TEST

START_TEST(getStructMember_not_found) {
    size_t offset = 0;
    const UA_DataType *mt = NULL;
    UA_Boolean isArray = false;
    UA_Boolean found = UA_DataType_getStructMember(
        &UA_TYPES[UA_TYPES_READVALUEID], "NonExistentField", &offset, &mt, &isArray);
    ck_assert(!found);
} END_TEST

START_TEST(getStructMember_array_field) {
    size_t offset = 0;
    const UA_DataType *mt = NULL;
    UA_Boolean isArray = false;
    UA_Boolean found = UA_DataType_getStructMember(
        &UA_TYPES[UA_TYPES_READREQUEST], "NodesToRead", &offset, &mt, &isArray);
    ck_assert(found);
    ck_assert(isArray);
} END_TEST

START_TEST(getStructMember_non_struct) {
    /* Passing a non-struct type should return false */
    size_t offset = 0;
    const UA_DataType *mt = NULL;
    UA_Boolean isArray = false;
    UA_Boolean found = UA_DataType_getStructMember(
        &UA_TYPES[UA_TYPES_INT32], "x", &offset, &mt, &isArray);
    ck_assert(!found);
} END_TEST

START_TEST(getStructMember_later_field) {
    /* Find a member that's not the first one */
    size_t offset = 0;
    const UA_DataType *mt = NULL;
    UA_Boolean isArray = false;
    UA_Boolean found = UA_DataType_getStructMember(
        &UA_TYPES[UA_TYPES_READVALUEID], "AttributeId", &offset, &mt, &isArray);
    ck_assert(found);
    ck_assert(offset > 0);
    ck_assert(!isArray);
} END_TEST
#endif

/* ========================================================================
 * 11. UA_String_format
 * ======================================================================== */

START_TEST(string_format_basic) {
    UA_String out = UA_STRING_NULL;
    UA_StatusCode ret = UA_String_format(&out, "answer=%d", 42);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    /* Verify content */
    char buf[64];
    memcpy(buf, out.data, out.length);
    buf[out.length] = '\0';
    ck_assert_str_eq(buf, "answer=42");
    UA_String_clear(&out);
} END_TEST

START_TEST(string_format_long_output) {
    /* Output > 128 chars to trigger internal realloc path */
    UA_String out = UA_STRING_NULL;
    char longStr[200];
    memset(longStr, 'X', 199);
    longStr[199] = '\0';
    UA_StatusCode ret = UA_String_format(&out, "%s%s", longStr, longStr);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert(out.length >= 398);
    UA_String_clear(&out);
} END_TEST

/* ========================================================================
 * 12. checkAdjustRange via Variant_copyRange error paths
 * ======================================================================== */

START_TEST(checkAdjustRange_badindexrangenodata) {
    /* Range dimension count doesn't match array dimensions */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArray(&src, arr, 3, &UA_TYPES[UA_TYPES_INT32]);

    /* Array has 1 dimension, provide 2 */
    UA_NumericRangeDimension rdims[] = {{0, 1}, {0, 0}};
    UA_NumericRange range = {2, rdims};
    UA_StatusCode ret = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(ret, UA_STATUSCODE_BADINDEXRANGENODATA);
} END_TEST

START_TEST(checkAdjustRange_invalid_range) {
    /* min > max => BADINDEXRANGEINVALID */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Int32 arr[] = {1, 2, 3};
    UA_Variant_setArray(&src, arr, 3, &UA_TYPES[UA_TYPES_INT32]);

    UA_NumericRangeDimension dim = {5, 2}; /* min > max */
    UA_NumericRange range = {1, &dim};
    UA_StatusCode ret = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(ret, UA_STATUSCODE_BADINDEXRANGEINVALID);
} END_TEST

START_TEST(checkAdjustRange_clamp_max) {
    /* max > array length => should be clamped */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Int32 arr[] = {10, 20, 30};
    UA_Variant_setArray(&src, arr, 3, &UA_TYPES[UA_TYPES_INT32]);

    /* range [1, 100] on 3-element array => clamped to [1, 2] */
    UA_NumericRangeDimension dim = {1, 100};
    UA_NumericRange range = {1, &dim};
    UA_StatusCode ret = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.arrayLength, 2);
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(checkAdjustRange_2d_dims_mismatch) {
    /* Array dimensions don't multiply to arrayLength => BADINTERNALERROR */
    UA_Variant src, dst;
    UA_Variant_init(&src);
    UA_Int32 arr[] = {1, 2, 3, 4, 5};
    UA_Variant_setArray(&src, arr, 5, &UA_TYPES[UA_TYPES_INT32]);
    UA_UInt32 dims[] = {2, 3}; /* 2*3=6 != 5 */
    src.arrayDimensions = dims;
    src.arrayDimensionsSize = 2;

    UA_NumericRangeDimension rdims[] = {{0, 1}, {0, 1}};
    UA_NumericRange range = {2, rdims};
    UA_StatusCode ret = UA_Variant_copyRange(&src, &dst, range);
    ck_assert_uint_eq(ret, UA_STATUSCODE_BADINTERNALERROR);
    src.arrayDimensions = NULL; src.arrayDimensionsSize = 0;
} END_TEST

/* ========================================================================
 * Additional edge-case tests
 * ======================================================================== */

START_TEST(order_variant_with_no_arrayDimensions) {
    /* Two equal array variants without explicit arrayDimensions => EQ including
     * the arrayDimensions comparison path for dimensionsSize == 0 */
    UA_Variant a, b;
    UA_Variant_init(&a);
    UA_Variant_init(&b);
    UA_Int32 arr1[] = {1, 2};
    UA_Int32 arr2[] = {1, 2};
    UA_Variant_setArray(&a, arr1, 2, &UA_TYPES[UA_TYPES_INT32]);
    UA_Variant_setArray(&b, arr2, 2, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_VARIANT]), UA_ORDER_EQ);
} END_TEST

START_TEST(order_readvalueid_structure) {
    /* Exercises structureOrder on a standard type with both required and array-like members */
    UA_ReadValueId a, b;
    UA_ReadValueId_init(&a);
    UA_ReadValueId_init(&b);
    a.nodeId = UA_NODEID_NUMERIC(0, 1);
    a.attributeId = UA_ATTRIBUTEID_VALUE;
    a.indexRange = UA_STRING("0:3");
    b.nodeId = UA_NODEID_NUMERIC(0, 1);
    b.attributeId = UA_ATTRIBUTEID_VALUE;
    b.indexRange = UA_STRING("0:5");
    ck_assert_int_eq(UA_order(&a, &b, &UA_TYPES[UA_TYPES_READVALUEID]), UA_ORDER_LESS);
    ck_assert_int_eq(UA_order(&a, &a, &UA_TYPES[UA_TYPES_READVALUEID]), UA_ORDER_EQ);
} END_TEST

/* ========================================================================
 * Suite definition
 * ======================================================================== */

static Suite *testSuite_types_order2(void) {
    Suite *s = suite_create("Types Order2");

    TCase *tc_struct = tcase_create("StructureOrder");
    tcase_add_test(tc_struct, structureOrder_opt_both_null);
    tcase_add_test(tc_struct, structureOrder_opt_one_set);
    tcase_add_test(tc_struct, structureOrder_opt_both_set_equal);
    tcase_add_test(tc_struct, structureOrder_opt_both_set_differ);
    tcase_add_test(tc_struct, structureOrder_opt_same_pointer);
    tcase_add_test(tc_struct, structureOrder_opt_string_differ);
    tcase_add_test(tc_struct, structureOrder_required_array_differ);
    tcase_add_test(tc_struct, structureOrder_required_array_content_differ);
    tcase_add_test(tc_struct, structureOrder_opt_array_differ);
    tcase_add_test(tc_struct, structureOrder_first_member_differs);
    suite_add_tcase(s, tc_struct);

    TCase *tc_diag = tcase_create("DiagnosticInfoOrder");
    tcase_add_test(tc_diag, diagInfo_empty_equal);
    tcase_add_test(tc_diag, diagInfo_symbolicId_differ);
    tcase_add_test(tc_diag, diagInfo_symbolicId_has_vs_not);
    tcase_add_test(tc_diag, diagInfo_namespaceUri_differ);
    tcase_add_test(tc_diag, diagInfo_localizedText_differ);
    tcase_add_test(tc_diag, diagInfo_locale_differ);
    tcase_add_test(tc_diag, diagInfo_additionalInfo_differ);
    tcase_add_test(tc_diag, diagInfo_additionalInfo_equal);
    tcase_add_test(tc_diag, diagInfo_innerStatusCode_differ);
    tcase_add_test(tc_diag, diagInfo_innerDiagnosticInfo_both_null);
    tcase_add_test(tc_diag, diagInfo_innerDiagnosticInfo_one_null);
    tcase_add_test(tc_diag, diagInfo_innerDiagnosticInfo_recurse);
    tcase_add_test(tc_diag, diagInfo_innerDiagnosticInfo_same_ptr);
    suite_add_tcase(s, tc_diag);

    TCase *tc_dv = tcase_create("DataValueOrder");
    tcase_add_test(tc_dv, dataValue_empty_equal);
    tcase_add_test(tc_dv, dataValue_hasValue_differ);
    tcase_add_test(tc_dv, dataValue_status_differ);
    tcase_add_test(tc_dv, dataValue_hasStatus_differ);
    tcase_add_test(tc_dv, dataValue_sourceTimestamp_differ);
    tcase_add_test(tc_dv, dataValue_serverTimestamp_differ);
    tcase_add_test(tc_dv, dataValue_sourcePicoseconds_differ);
    tcase_add_test(tc_dv, dataValue_serverPicoseconds_differ);
    tcase_add_test(tc_dv, dataValue_all_fields_equal);
    tcase_add_test(tc_dv, dataValue_hasServerTimestamp_differ);
    tcase_add_test(tc_dv, dataValue_hasPico_differ);
    suite_add_tcase(s, tc_dv);

    TCase *tc_eo = tcase_create("ExtensionObjectOrder");
    tcase_add_test(tc_eo, extObj_nobody_equal);
    tcase_add_test(tc_eo, extObj_encoding_differ);
    tcase_add_test(tc_eo, extObj_encoded_bytestring_differ);
    tcase_add_test(tc_eo, extObj_encoded_typeId_differ);
    tcase_add_test(tc_eo, extObj_encoded_xml);
    tcase_add_test(tc_eo, extObj_decoded_same_type);
    tcase_add_test(tc_eo, extObj_decoded_diff_type);
    tcase_add_test(tc_eo, extObj_decoded_nodelete);
    tcase_add_test(tc_eo, extObj_decoded_null_type);
    suite_add_tcase(s, tc_eo);

    TCase *tc_var = tcase_create("VariantOrder");
    tcase_add_test(tc_var, variant_both_empty);
    tcase_add_test(tc_var, variant_type_mismatch);
    tcase_add_test(tc_var, variant_scalar_vs_array);
    tcase_add_test(tc_var, variant_array_same_length_differ);
    tcase_add_test(tc_var, variant_array_diff_length);
    tcase_add_test(tc_var, variant_arrayDimensions_differ);
    tcase_add_test(tc_var, variant_arrayDimensions_content_differ);
    tcase_add_test(tc_var, order_variant_with_no_arrayDimensions);
    suite_add_tcase(s, tc_var);

    TCase *tc_union = tcase_create("UnionOrder");
    tcase_add_test(tc_union, union_both_none);
    tcase_add_test(tc_union, union_switch_differ);
    tcase_add_test(tc_union, union_same_switch_optionA_differ);
    tcase_add_test(tc_union, union_same_switch_equal);
    tcase_add_test(tc_union, union_same_switch_optionB_differ);
    suite_add_tcase(s, tc_union);

    TCase *tc_copyrange = tcase_create("VariantCopyRange");
    tcase_add_test(tc_copyrange, copyRange_1d_basic);
    tcase_add_test(tc_copyrange, copyRange_2d_matrix);
    tcase_add_test(tc_copyrange, copyRange_scalar_string);
    tcase_add_test(tc_copyrange, copyRange_empty_variant);
    tcase_add_test(tc_copyrange, copyRange_out_of_bounds);
    tcase_add_test(tc_copyrange, copyRange_dim_mismatch);
    suite_add_tcase(s, tc_copyrange);

    TCase *tc_setrange = tcase_create("VariantSetRange");
    tcase_add_test(tc_setrange, setRange_1d_basic);
    tcase_add_test(tc_setrange, setRange_size_mismatch);
    tcase_add_test(tc_setrange, setRange_empty_variant);
    tcase_add_test(tc_setrange, setRangeCopy_string_array);
    suite_add_tcase(s, tc_setrange);

    TCase *tc_resize = tcase_create("ArrayResize");
    tcase_add_test(tc_resize, arrayResize_grow);
    tcase_add_test(tc_resize, arrayResize_shrink);
    tcase_add_test(tc_resize, arrayResize_to_zero);
    tcase_add_test(tc_resize, arrayResize_same_size);
    tcase_add_test(tc_resize, arrayResize_shrink_strings);
    suite_add_tcase(s, tc_resize);

#ifdef UA_ENABLE_TYPEDESCRIPTION
    TCase *tc_member = tcase_create("GetStructMember");
    tcase_add_test(tc_member, getStructMember_found);
    tcase_add_test(tc_member, getStructMember_not_found);
    tcase_add_test(tc_member, getStructMember_array_field);
    tcase_add_test(tc_member, getStructMember_non_struct);
    tcase_add_test(tc_member, getStructMember_later_field);
    suite_add_tcase(s, tc_member);
#endif

    TCase *tc_fmt = tcase_create("StringFormat");
    tcase_add_test(tc_fmt, string_format_basic);
    tcase_add_test(tc_fmt, string_format_long_output);
    suite_add_tcase(s, tc_fmt);

    TCase *tc_adjust = tcase_create("CheckAdjustRange");
    tcase_add_test(tc_adjust, checkAdjustRange_badindexrangenodata);
    tcase_add_test(tc_adjust, checkAdjustRange_invalid_range);
    tcase_add_test(tc_adjust, checkAdjustRange_clamp_max);
    tcase_add_test(tc_adjust, checkAdjustRange_2d_dims_mismatch);
    suite_add_tcase(s, tc_adjust);

    TCase *tc_extra = tcase_create("ExtraEdgeCases");
    tcase_add_test(tc_extra, order_readvalueid_structure);
    suite_add_tcase(s, tc_extra);

    return s;
}

int main(void) {
    Suite *s = testSuite_types_order2();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int fails = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (fails == 0) ? 0 : 1;
}
