/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Tests for the DataTypeDefinition fallback for dynamically loaded Enumeration
 * DataType nodes that have no compiled C-struct members (membersSize == 0).
 * In that case the server falls back to reading EnumValues or EnumStrings
 * properties from the address space to build a valid EnumDefinition.
 *
 * OPC UA Part 3 v1.05.06, §5.8.3: The DataTypeDefinition attribute shall be
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

static UA_Server *server;

/* Custom enum UA_DataType entries with no compiled members (membersSize == 0),
 * mimicking enum DataTypes that were loaded dynamically from a NodeSet. They
 * must be registered so UA_findDataTypeWithCustom() resolves the node to a
 * typeKind == UA_DATATYPEKIND_ENUM and the server takes the property fallback
 * path instead of returning BadAttributeIdInvalid. */
#define DYN_ENUM_TYPE(id, nameLit)                   \
    {                                                \
        UA_TYPENAME(nameLit)                         \
        .typeId      = {1, UA_NODEIDTYPE_NUMERIC, {id}}, \
        .memSize     = sizeof(UA_Int32),             \
        .typeKind    = UA_DATATYPEKIND_ENUM,         \
        .membersSize = 0,                            \
        .members     = NULL,                         \
    }

static UA_DataType dynEnumTypes[4] = {
    DYN_ENUM_TYPE(DT_ENUMVALUES, "TestEnumEV"),
    DYN_ENUM_TYPE(DT_ENUMSTRINGS, "TestEnumES"),
    DYN_ENUM_TYPE(DT_BOTH, "TestEnumBoth"),
    DYN_ENUM_TYPE(DT_NONE, "TestEnumNone"),
};

static UA_DataTypeArray dynEnumTypesArray = {
    NULL, 4, dynEnumTypes, false
};

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    dynEnumTypesArray.next = config->customDataTypes;
    config->customDataTypes = &dynEnumTypesArray;
}

static void teardown(void) {
    UA_Server_delete(server);
}

/* Helper: compare a UA_String against a C string literal. UA_STRING() returns
 * a value (not an lvalue), so its address cannot be taken directly. */
static UA_Boolean
str_eq(const UA_String *s, const char *lit) {
    UA_String expected = UA_STRING((char *)(uintptr_t)lit);
    return UA_String_equal(s, &expected);
}

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

START_TEST(dtdef_fallback_enumvalues) {
    /* Build a 3-value enum: {0="Off", 1="On", 2="Error"} using EnumValues
     * (non-zero-based gaps are the reason EnumValues exists per §5.8.3). */
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

    UA_NodeId dtId = addDynEnumDataType("TestEnumEV", 60001);
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

    UA_DataValue_clear(&dv);
} END_TEST

/* ===== Test 2: EnumStrings property → implicit 0..N-1 values ===== */

START_TEST(dtdef_fallback_enumstrings) {
    /* EnumStrings: zero-based, implicit values 0,1,2. */
    UA_LocalizedText strs[3];
    strs[0] = UA_LOCALIZEDTEXT("en-US", "Alpha");
    strs[1] = UA_LOCALIZEDTEXT("en-US", "Beta");
    strs[2] = UA_LOCALIZEDTEXT("en-US", "Gamma");

    UA_NodeId dtId = addDynEnumDataType("TestEnumES", 60002);
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

    /* name is derived from displayName.text per the mapping in
     * Server_readEnumUnified (EnumStrings path). */
    ck_assert(str_eq(&def->fields[0].name, "Alpha"));

    UA_DataValue_clear(&dv);
} END_TEST

/* ===== Test 3: EnumValues wins over EnumStrings when both are present ===== */

START_TEST(dtdef_fallback_enumvalues_wins) {
    /* Per OPC UA Part 3 §5.8.3 an Enumeration shall have either EnumStrings
     * OR EnumValues. When a badly-configured node provides both, our
     * implementation must prefer EnumValues (which encodes richer data). */
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

    UA_NodeId dtId = addDynEnumDataType("TestEnumBoth", 60003);
    /* Add EnumValues first so it is found first during browse */
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

/* ===== Test 4: No EnumValues, no EnumStrings → BadAttributeIdInvalid ===== */

START_TEST(dtdef_fallback_no_properties) {
    /* A dynamic enum DataType node without any enum properties must return
     * BadAttributeIdInvalid for the DataTypeDefinition attribute. */
    UA_NodeId dtId = addDynEnumDataType("TestEnumNone", 60004);

    UA_DataValue dv = readDataTypeDefinition(&dtId);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_BADATTRIBUTEIDINVALID);

    UA_DataValue_clear(&dv);
} END_TEST

/* ===== Test 5: Compiled enum (membersSize > 0) still works (regression) ===== */

START_TEST(dtdef_compiled_enum_regression) {
    /* MessageSecurityMode is a well-known compiled Enumeration in NS0. It has
     * membersSize > 0, so the existing path must still return a valid
     * EnumDefinition without touching the fallback code. */
#ifdef UA_ENABLE_TYPEDESCRIPTION
    UA_NodeId msmId = UA_TYPES[UA_TYPES_MESSAGESECURITYMODE].typeId;

    UA_DataValue dv = readDataTypeDefinition(&msmId);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_GOOD);
    ck_assert(dv.hasValue);
    ck_assert(dv.value.type == &UA_TYPES[UA_TYPES_ENUMDEFINITION]);

    UA_EnumDefinition *def = (UA_EnumDefinition *)dv.value.data;
    ck_assert_uint_gt(def->fieldsSize, 0);

    UA_DataValue_clear(&dv);
#endif
} END_TEST

/* ===== Test suite wiring ===== */

static Suite *
testSuite_enum_datatypedefinition(void) {
    Suite *s = suite_create("server_enum_datatypedefinition");

    TCase *tc = tcase_create("fallback");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, dtdef_fallback_enumvalues);
    tcase_add_test(tc, dtdef_fallback_enumstrings);
    tcase_add_test(tc, dtdef_fallback_enumvalues_wins);
    tcase_add_test(tc, dtdef_fallback_no_properties);
    tcase_add_test(tc, dtdef_compiled_enum_regression);
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
