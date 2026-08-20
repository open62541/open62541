/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Round-trip tests for the DataType <-> DataTypeDescription translation
 * (src/ua_types_definition.c). */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <check.h>
#include <stdlib.h>

#ifdef UA_TYPES_STRUCTUREDESCRIPTION

/* Convert a builtin type to its description and back. Verify that the
 * regenerated type faithfully reproduces the memory layout. */
static void
roundtrip(const UA_DataType *orig) {
    UA_ExtensionObject descr;
    UA_StatusCode res = UA_DataType_toDescription(orig, &descr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_DataType regen;
    memset(&regen, 0, sizeof(regen));
    res = UA_DataType_fromDescription(&regen, &descr, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* The reconstructed type must have the same memory size and kind */
    ck_assert_uint_eq(regen.memSize, orig->memSize);
    ck_assert_uint_eq(regen.typeKind, orig->typeKind);
    ck_assert_uint_eq(regen.membersSize, orig->membersSize);

    UA_DataType_clear(&regen);
    UA_ExtensionObject_clear(&descr);
}

START_TEST(definition_roundtrip_structure) {
    roundtrip(&UA_TYPES[UA_TYPES_READVALUEID]);
} END_TEST

START_TEST(definition_roundtrip_structure_nested) {
    roundtrip(&UA_TYPES[UA_TYPES_READREQUEST]);
} END_TEST

START_TEST(definition_roundtrip_simpleType) {
    roundtrip(&UA_TYPES[UA_TYPES_INT32]);
    roundtrip(&UA_TYPES[UA_TYPES_STRING]);
} END_TEST

START_TEST(definition_roundtrip_enum) {
    roundtrip(&UA_TYPES[UA_TYPES_MESSAGESECURITYMODE]);
} END_TEST

START_TEST(definition_fromDescription_invalidExtObj) {
    /* An ExtensionObject that does not carry a description type fails */
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_Int32 dummy = 5;
    UA_ExtensionObject_setValue(&eo, &dummy, &UA_TYPES[UA_TYPES_INT32]);

    UA_DataType regen;
    memset(&regen, 0, sizeof(regen));
    UA_StatusCode res = UA_DataType_fromDescription(&regen, &eo, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

START_TEST(definition_fromStructure_unknownMember) {
    /* Build a StructureDescription whose member references an unknown type.
     * The reconstruction must fail with BADNOTFOUND. */
    UA_ExtensionObject descr;
    UA_StatusCode res =
        UA_DataType_toDescription(&UA_TYPES[UA_TYPES_READVALUEID], &descr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_StructureDescription *sd =
        (UA_StructureDescription*)descr.content.decoded.data;
    ck_assert_uint_gt(sd->structureDefinition.fieldsSize, 0);
    /* Point the first field at a non-existent data type */
    sd->structureDefinition.fields[0].dataType = UA_NODEID_NUMERIC(99, 424242);

    UA_DataType regen;
    memset(&regen, 0, sizeof(regen));
    res = UA_DataType_fromDescription(&regen, &descr, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNOTFOUND);

    UA_ExtensionObject_clear(&descr);
} END_TEST

START_TEST(definition_fromStructure_inheritsBaseLayout) {
    /* Describe a base layout that is already registered. The derived
     * StructureDefinition contains only its newly introduced field. */
    UA_DataTypeMember baseMember;
    memset(&baseMember, 0, sizeof(baseMember));
#ifdef UA_ENABLE_TYPEDESCRIPTION
    baseMember.memberName = "BaseValue";
#endif
    baseMember.memberType = &UA_TYPES[UA_TYPES_INT32];

    UA_DataType baseType;
    memset(&baseType, 0, sizeof(baseType));
#ifdef UA_ENABLE_TYPEDESCRIPTION
    baseType.typeName = "BaseRecord";
#endif
    baseType.typeId = UA_NODEID_NUMERIC(1, 6001);
    baseType.memSize = sizeof(UA_Int32);
    baseType.typeKind = UA_DATATYPEKIND_STRUCTURE;
    baseType.pointerFree = true;
    baseType.overlayable = UA_TYPES[UA_TYPES_INT32].overlayable;
    baseType.membersSize = 1;
    baseType.members = &baseMember;
    UA_DataTypeArray customTypes = {NULL, 1, &baseType, false};

    UA_StructureField ownField;
    UA_StructureField_init(&ownField);
    ownField.name = UA_STRING("OwnValue");
    ownField.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    ownField.valueRank = UA_VALUERANK_SCALAR;

    UA_StructureDescription description;
    UA_StructureDescription_init(&description);
    description.dataTypeId = UA_NODEID_NUMERIC(1, 6002);
    description.name = UA_QUALIFIEDNAME(1, "DerivedRecord");
    description.structureDefinition.defaultEncodingId =
        UA_NODEID_NUMERIC(1, 5002);
    description.structureDefinition.baseDataType = baseType.typeId;
    description.structureDefinition.structureType =
        UA_STRUCTURETYPE_STRUCTURE;
    description.structureDefinition.fieldsSize = 1;
    description.structureDefinition.fields = &ownField;

    UA_ExtensionObject encodedDescription;
    UA_ExtensionObject_setValueNoDelete(
        &encodedDescription, &description,
        &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION]);

    UA_DataType derivedType;
    UA_StatusCode res = UA_DataType_fromDescription(
        &derivedType, &encodedDescription, &customTypes);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(derivedType.membersSize, 2);
    ck_assert_ptr_eq(derivedType.members[0].memberType,
                     &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_ptr_eq(derivedType.members[1].memberType,
                     &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_ge(derivedType.memSize,
                      sizeof(UA_Int32) + sizeof(UA_String));
#ifdef UA_ENABLE_TYPEDESCRIPTION
    ck_assert_str_eq(derivedType.members[0].memberName, "BaseValue");
    ck_assert_str_eq(derivedType.members[1].memberName, "OwnValue");
#endif
    UA_DataType_clear(&derivedType);
} END_TEST

static Suite *testSuite_definition(void) {
    TCase *tc = tcase_create("DataTypeDefinition");
    tcase_add_test(tc, definition_roundtrip_structure);
    tcase_add_test(tc, definition_roundtrip_structure_nested);
    tcase_add_test(tc, definition_roundtrip_simpleType);
    tcase_add_test(tc, definition_roundtrip_enum);
    tcase_add_test(tc, definition_fromDescription_invalidExtObj);
    tcase_add_test(tc, definition_fromStructure_unknownMember);
    tcase_add_test(tc, definition_fromStructure_inheritsBaseLayout);

    Suite *s = suite_create("DataType Definition");
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_definition();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else /* !UA_TYPES_STRUCTUREDESCRIPTION */

int main(void) {
    return EXIT_SUCCESS;
}

#endif
