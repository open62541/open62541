/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Tests for the DataTypeDefinition fallback for dynamically loaded Enumeration
 * DataType nodes that have no compiled C-struct members (membersSize == 0).
 * In that case the server falls back to reading EnumValues or EnumStrings
 * properties from the address space to build a valid EnumDefinition.
 *
 * OPC UA Part 3 v1.05, §5.8.3: The DataTypeDefinition attribute shall be
 * mandatory for DataTypes derived from Enumeration and OptionSet. An
 * Enumeration DataType shall have either an EnumStrings property or an
 * EnumValues property (but not both).
 */

#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "testing_clock.h"
#include "test_helpers.h"

#include <check.h>
#include <stdlib.h>

/* Namespace 0 numeric IDs used in the tests */
#define NS0_ENUMERATION   29u  /* Enumeration DataType */
#define NS0_HASPROPERTY   46u  /* HasProperty reference type */
#define NS0_PROPERTYTYPE  68u  /* PropertyType variable type */
#define NS0_ENUMVALUETYPE 7594u /* EnumValueType DataType */

/* Local numeric ids of the dynamic enum DataType nodes used in the tests. */
#define DT_ENUMVALUES 60001u
#define DT_ENUMSTRINGS 60002u
#define DT_BOTH 60003u
#define DT_NONE 60004u
#define DT_EMPTYVALUES 60005u
#define DT_WRONGVALUES 60006u

static UA_Server *server;

/* Custom enum UA_DataType entries with no compiled members, mimicking enums
 * loaded from a NodeSet. Registered so UA_findDataTypeWithCustom() resolves the
 * node to typeKind ENUM and the server takes the property fallback path. */
#define DYN_ENUM_TYPE(id, nameLit)                   \
    {                                                \
        UA_TYPENAME(nameLit)                         \
        .typeId      = {1, UA_NODEIDTYPE_NUMERIC, {id}}, \
        .memSize     = sizeof(UA_Int32),             \
        .typeKind    = UA_DATATYPEKIND_ENUM,         \
        .membersSize = 0,                            \
        .members     = NULL,                         \
    }

static UA_DataType dynEnumTypes[6] = {
    DYN_ENUM_TYPE(DT_ENUMVALUES, "TestEnumEV"),
    DYN_ENUM_TYPE(DT_ENUMSTRINGS, "TestEnumES"),
    DYN_ENUM_TYPE(DT_BOTH, "TestEnumBoth"),
    DYN_ENUM_TYPE(DT_NONE, "TestEnumNone"),
    DYN_ENUM_TYPE(DT_EMPTYVALUES, "TestEnumEmptyEV"),
    DYN_ENUM_TYPE(DT_WRONGVALUES, "TestEnumWrongEV"),
};

static UA_DataTypeArray dynEnumTypesArray = {
    NULL, 6, dynEnumTypes, false
};

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    dynEnumTypesArray.next = config->customDataTypes;
    config->customDataTypes = &dynEnumTypesArray;

    UA_StatusCode rc = UA_Server_run_startup(server);
    ck_assert_int_eq(rc, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

#ifdef UA_ENABLE_TYPEDESCRIPTION
/* Helper: compare a UA_String against a C string literal. UA_STRING() returns
 * a value (not an lvalue), so its address cannot be taken directly. */
static UA_Boolean
str_eq(const UA_String *s, const char *lit) {
    UA_String expected = UA_STRING((char *)(uintptr_t)lit);
    return UA_String_equal(s, &expected);
}
#endif

/* Helper: add a DataType node that is a subtype of Enumeration but has no
 * compiled members (membersSize == 0, i.e. dynamically loaded). */
static UA_NodeId
addDynEnumDataType(const char *name, UA_UInt32 localNumericId) {
    UA_DataTypeAttributes dtAttr = UA_DataTypeAttributes_default;
    dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", (char *)(uintptr_t)name);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(1, localNumericId);
    UA_StatusCode rc = UA_Server_addDataTypeNode(
        server, nodeId,
        UA_NODEID_NUMERIC(0, NS0_ENUMERATION),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, (char *)(uintptr_t)name),
        dtAttr, NULL, NULL);
    ck_assert_int_eq(rc, UA_STATUSCODE_GOOD);
    return nodeId;
}

#ifdef UA_ENABLE_TYPEDESCRIPTION
/* Helper: attach an EnumValues property to a DataType node. */
static void
addEnumValuesProperty(const UA_NodeId *dtId,
                      UA_EnumValueType *values, size_t valuesSize) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setArray(&vAttr.value, values, valuesSize,
                        &UA_TYPES[UA_TYPES_ENUMVALUETYPE]);
    vAttr.dataType = UA_TYPES[UA_TYPES_ENUMVALUETYPE].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 arrayDims[1] = {0}; /* one dimension, unspecified length */
    vAttr.arrayDimensions = arrayDims;
    vAttr.arrayDimensionsSize = 1;
    UA_StatusCode rc = UA_Server_addVariableNode(
        server, UA_NODEID_NULL,
        *dtId,
        UA_NODEID_NUMERIC(0, NS0_HASPROPERTY),
        UA_QUALIFIEDNAME(0, "EnumValues"),
        UA_NODEID_NUMERIC(0, NS0_PROPERTYTYPE),
        vAttr, NULL, NULL);
    ck_assert_int_eq(rc, UA_STATUSCODE_GOOD);
}

/* Helper: attach an EnumStrings property to a DataType node. */
static void
addEnumStringsProperty(const UA_NodeId *dtId,
                       UA_LocalizedText *strings, size_t stringsSize) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setArray(&vAttr.value, strings, stringsSize,
                        &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    vAttr.dataType = UA_TYPES[UA_TYPES_LOCALIZEDTEXT].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 arrayDims[1] = {0}; /* one dimension, unspecified length */
    vAttr.arrayDimensions = arrayDims;
    vAttr.arrayDimensionsSize = 1;
    UA_StatusCode rc = UA_Server_addVariableNode(
        server, UA_NODEID_NULL,
        *dtId,
        UA_NODEID_NUMERIC(0, NS0_HASPROPERTY),
        UA_QUALIFIEDNAME(0, "EnumStrings"),
        UA_NODEID_NUMERIC(0, NS0_PROPERTYTYPE),
        vAttr, NULL, NULL);
    ck_assert_int_eq(rc, UA_STATUSCODE_GOOD);
}
#endif /* UA_ENABLE_TYPEDESCRIPTION */

/* Helper: read the DataTypeDefinition attribute and return an ExtensionObject.
 * Caller must call UA_DataValue_clear on the returned value. */
static UA_DataValue
readDataTypeDefinition(const UA_NodeId *nodeId) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = *nodeId;
    rvi.attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;
    return UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
}

/* ===== Test 1: EnumValues property → correct EnumDefinition ===== */

#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(dtdef_fallback_enumvalues) {
    /* Build a 3-value enum: {0="Off", 1="On", 2="Error"} using EnumValues
     * (non-zero-based gaps are why EnumValues exists, Part 3 v1.05 §5.8.3). */
    UA_EnumValueType vals[3];
    UA_EnumValueType_init(&vals[0]);
    vals[0].value = 0;
    vals[0].displayName = UA_LOCALIZEDTEXT("en-US", "Off");
    vals[0].description = UA_LOCALIZEDTEXT("en-US", "Device is off");

    UA_EnumValueType_init(&vals[1]);
    vals[1].value = 1;
    vals[1].displayName = UA_LOCALIZEDTEXT("en-US", "On");
    vals[1].description = UA_LOCALIZEDTEXT("en-US", "Device is on");

    UA_EnumValueType_init(&vals[2]);
    vals[2].value = 5;  /* deliberate gap to exercise the non-trivial path */
    vals[2].displayName = UA_LOCALIZEDTEXT("en-US", "Error");
    vals[2].description = UA_LOCALIZEDTEXT("en-US", "Device error");

    UA_NodeId dtId = addDynEnumDataType("TestEnumEV", DT_ENUMVALUES);
    addEnumValuesProperty(&dtId, vals, 3);

    UA_DataValue dv = readDataTypeDefinition(&dtId);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_GOOD);
    ck_assert(dv.hasValue);
    ck_assert_ptr_ne(dv.value.type, NULL);
    ck_assert(dv.value.type == &UA_TYPES[UA_TYPES_ENUMDEFINITION]);

    UA_EnumDefinition *def = (UA_EnumDefinition *)dv.value.data;
    ck_assert_uint_eq(def->fieldsSize, 3);

    /* Field 0: value=0, name="Off" */
    ck_assert_int_eq(def->fields[0].value, 0);
    ck_assert(str_eq(&def->fields[0].name, "Off"));

    /* Field 1: value=1, name="On" */
    ck_assert_int_eq(def->fields[1].value, 1);
    ck_assert(str_eq(&def->fields[1].name, "On"));

    /* Field 2: value=5 (gap), name="Error" */
    ck_assert_int_eq(def->fields[2].value, 5);
    ck_assert(str_eq(&def->fields[2].name, "Error"));

    /* displayName text must match the original EnumValueType displayName */
    ck_assert(str_eq(&def->fields[0].displayName.text, "Off"));
    ck_assert(str_eq(&def->fields[2].displayName.text, "Error"));

    /* description must be carried over from the EnumValueType entries */
    ck_assert(str_eq(&def->fields[0].description.text, "Device is off"));
    ck_assert(str_eq(&def->fields[2].description.text, "Device error"));

    UA_DataValue_clear(&dv);
} END_TEST
#endif

/* ===== Test 2: EnumStrings property → implicit 0..N-1 values ===== */

#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(dtdef_fallback_enumstrings) {
    /* EnumStrings: zero-based, implicit values 0,1,2. */
    UA_LocalizedText strs[3];
    strs[0] = UA_LOCALIZEDTEXT("en-US", "Alpha");
    strs[1] = UA_LOCALIZEDTEXT("en-US", "Beta");
    strs[2] = UA_LOCALIZEDTEXT("en-US", "Gamma");

    UA_NodeId dtId = addDynEnumDataType("TestEnumES", DT_ENUMSTRINGS);
    addEnumStringsProperty(&dtId, strs, 3);

    UA_DataValue dv = readDataTypeDefinition(&dtId);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_GOOD);
    ck_assert(dv.hasValue);
    ck_assert(dv.value.type == &UA_TYPES[UA_TYPES_ENUMDEFINITION]);

    UA_EnumDefinition *def = (UA_EnumDefinition *)dv.value.data;
    ck_assert_uint_eq(def->fieldsSize, 3);

    /* Implicit values 0, 1, 2 */
    ck_assert_int_eq(def->fields[0].value, 0);
    ck_assert_int_eq(def->fields[1].value, 1);
    ck_assert_int_eq(def->fields[2].value, 2);

    /* The displayName text is taken from the EnumStrings entries */
    ck_assert(str_eq(&def->fields[0].displayName.text, "Alpha"));
    ck_assert(str_eq(&def->fields[1].displayName.text, "Beta"));
    ck_assert(str_eq(&def->fields[2].displayName.text, "Gamma"));

    /* name is derived from the EnumStrings entry text */
    ck_assert(str_eq(&def->fields[0].name, "Alpha"));

    UA_DataValue_clear(&dv);
} END_TEST
#endif

/* ===== Test 3: EnumValues wins over EnumStrings when both are present ===== */

#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(dtdef_fallback_enumvalues_wins) {
    /* A node with both properties (spec-illegal) must prefer EnumValues. */
    UA_EnumValueType vals[2];
    UA_EnumValueType_init(&vals[0]);
    vals[0].value = 10;
    vals[0].displayName = UA_LOCALIZEDTEXT("en-US", "First");

    UA_EnumValueType_init(&vals[1]);
    vals[1].value = 20;
    vals[1].displayName = UA_LOCALIZEDTEXT("en-US", "Second");

    UA_LocalizedText strs[3];
    strs[0] = UA_LOCALIZEDTEXT("en-US", "A");
    strs[1] = UA_LOCALIZEDTEXT("en-US", "B");
    strs[2] = UA_LOCALIZEDTEXT("en-US", "C");

    UA_NodeId dtId = addDynEnumDataType("TestEnumBoth", DT_BOTH);
    /* Preference is by browse name, independent of insertion order. */
    addEnumValuesProperty(&dtId, vals, 2);
    addEnumStringsProperty(&dtId, strs, 3);

    UA_DataValue dv = readDataTypeDefinition(&dtId);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_GOOD);
    ck_assert(dv.hasValue);
    ck_assert(dv.value.type == &UA_TYPES[UA_TYPES_ENUMDEFINITION]);

    UA_EnumDefinition *def = (UA_EnumDefinition *)dv.value.data;
    /* EnumValues yields 2 fields; EnumStrings would yield 3. */
    ck_assert_uint_eq(def->fieldsSize, 2);
    ck_assert_int_eq(def->fields[0].value, 10);
    ck_assert_int_eq(def->fields[1].value, 20);

    UA_DataValue_clear(&dv);
} END_TEST
#endif

/* ===== Test 4: No EnumValues, no EnumStrings → BadAttributeIdInvalid ===== */

START_TEST(dtdef_fallback_no_properties) {
    /* No enum properties: DataTypeDefinition must be BadAttributeIdInvalid. */
    UA_NodeId dtId = addDynEnumDataType("TestEnumNone", DT_NONE);

    UA_DataValue dv = readDataTypeDefinition(&dtId);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_BADATTRIBUTEIDINVALID);

    UA_DataValue_clear(&dv);
} END_TEST

/* ===== Test 5: Compiled enum (membersSize > 0) still works (regression) ===== */

#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(dtdef_compiled_enum_regression) {
    /* MessageSecurityMode is a compiled NS0 enum (membersSize > 0): the
     * existing path must still return a valid EnumDefinition. */
    UA_NodeId msmId = UA_TYPES[UA_TYPES_MESSAGESECURITYMODE].typeId;

    UA_DataValue dv = readDataTypeDefinition(&msmId);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_GOOD);
    ck_assert(dv.hasValue);
    ck_assert(dv.value.type == &UA_TYPES[UA_TYPES_ENUMDEFINITION]);

    UA_EnumDefinition *def = (UA_EnumDefinition *)dv.value.data;
    ck_assert_uint_gt(def->fieldsSize, 0);

    UA_DataValue_clear(&dv);
} END_TEST
#endif

/* ===== Test 6: Empty EnumValues array → fall through to EnumStrings ===== */

#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(dtdef_fallback_empty_enumvalues) {
    /* An empty EnumValues array carries no fields; the server must fall
     * through to the EnumStrings property. */
    UA_NodeId dtId = addDynEnumDataType("TestEnumEmptyEV", DT_EMPTYVALUES);
    addEnumValuesProperty(&dtId, (UA_EnumValueType*)UA_EMPTY_ARRAY_SENTINEL, 0);

    UA_LocalizedText strs[2];
    strs[0] = UA_LOCALIZEDTEXT("en-US", "Zero");
    strs[1] = UA_LOCALIZEDTEXT("en-US", "One");
    addEnumStringsProperty(&dtId, strs, 2);

    UA_DataValue dv = readDataTypeDefinition(&dtId);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_GOOD);
    ck_assert(dv.hasValue);
    ck_assert(dv.value.type == &UA_TYPES[UA_TYPES_ENUMDEFINITION]);

    UA_EnumDefinition *def = (UA_EnumDefinition *)dv.value.data;
    ck_assert_uint_eq(def->fieldsSize, 2);
    ck_assert_int_eq(def->fields[1].value, 1);
    ck_assert(str_eq(&def->fields[1].name, "One"));

    UA_DataValue_clear(&dv);
} END_TEST
#endif

/* ===== Test 7: Mistyped EnumValues → fall through to EnumStrings ===== */

#ifdef UA_ENABLE_TYPEDESCRIPTION
START_TEST(dtdef_fallback_mistyped_enumvalues) {
    /* An EnumValues property that is not an EnumValueType array must be
     * ignored; the server falls through to the EnumStrings property. */
    UA_NodeId dtId = addDynEnumDataType("TestEnumWrongEV", DT_WRONGVALUES);

    UA_Int32 wrong[2] = {1, 2};
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setArray(&vAttr.value, wrong, 2, &UA_TYPES[UA_TYPES_INT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 arrayDims[1] = {0};
    vAttr.arrayDimensions = arrayDims;
    vAttr.arrayDimensionsSize = 1;
    UA_StatusCode rc = UA_Server_addVariableNode(
        server, UA_NODEID_NULL, dtId,
        UA_NODEID_NUMERIC(0, NS0_HASPROPERTY),
        UA_QUALIFIEDNAME(0, "EnumValues"),
        UA_NODEID_NUMERIC(0, NS0_PROPERTYTYPE),
        vAttr, NULL, NULL);
    ck_assert_int_eq(rc, UA_STATUSCODE_GOOD);

    UA_LocalizedText strs[1];
    strs[0] = UA_LOCALIZEDTEXT("en-US", "Only");
    addEnumStringsProperty(&dtId, strs, 1);

    UA_DataValue dv = readDataTypeDefinition(&dtId);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_GOOD);
    ck_assert(dv.hasValue);
    ck_assert(dv.value.type == &UA_TYPES[UA_TYPES_ENUMDEFINITION]);

    UA_EnumDefinition *def = (UA_EnumDefinition *)dv.value.data;
    ck_assert_uint_eq(def->fieldsSize, 1);
    ck_assert_int_eq(def->fields[0].value, 0);
    ck_assert(str_eq(&def->fields[0].name, "Only"));

    UA_DataValue_clear(&dv);
} END_TEST
#endif

/* ===== Test suite wiring ===== */

static Suite *
testSuite_enum_datatypedefinition(void) {
    Suite *s = suite_create("server_enum_datatypedefinition");

    TCase *tc = tcase_create("fallback");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, dtdef_fallback_no_properties);
#ifdef UA_ENABLE_TYPEDESCRIPTION
    tcase_add_test(tc, dtdef_fallback_enumvalues);
    tcase_add_test(tc, dtdef_fallback_enumstrings);
    tcase_add_test(tc, dtdef_fallback_enumvalues_wins);
    tcase_add_test(tc, dtdef_compiled_enum_regression);
    tcase_add_test(tc, dtdef_fallback_empty_enumvalues);
    tcase_add_test(tc, dtdef_fallback_mistyped_enumvalues);
#endif
    suite_add_tcase(s, tc);

    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_enum_datatypedefinition();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
