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

START_TEST(definition_largeEnum) {
    const size_t fieldsSize = 387;
    UA_EnumField *fields = (UA_EnumField*)
        calloc(fieldsSize, sizeof(UA_EnumField));
    ck_assert_ptr_nonnull(fields);
    for(size_t i = 0; i < fieldsSize; i++) {
        fields[i].value = (UA_Int64)i;
        fields[i].name = UA_STRING("Value");
    }

    UA_EnumDescription description;
    UA_EnumDescription_init(&description);
    description.dataTypeId = UA_NODEID_NUMERIC(1, 5004);
    description.name = UA_QUALIFIEDNAME(1, "LargeEnum");
    description.builtInType = UA_DATATYPEKIND_INT32 + 1;
    description.enumDefinition.fieldsSize = fieldsSize;
    description.enumDefinition.fields = fields;

    UA_ExtensionObject descriptionObject;
    UA_ExtensionObject_setValueNoDelete(
        &descriptionObject, &description, &UA_TYPES[UA_TYPES_ENUMDESCRIPTION]);
    UA_DataType type;
    memset(&type, 0, sizeof(type));
    UA_StatusCode res =
        UA_DataType_fromDescription(&type, &descriptionObject, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(type.membersSize, fieldsSize);
    ck_assert_int_eq((UA_Int64)(uintptr_t)type.members[fieldsSize - 1].memberType,
                     (UA_Int64)(fieldsSize - 1));

    UA_ExtensionObject regenerated;
    res = UA_DataType_toDescription(&type, &regenerated);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_EnumDescription *regeneratedDescription =
        (UA_EnumDescription*)regenerated.content.decoded.data;
    ck_assert_uint_eq(regeneratedDescription->enumDefinition.fieldsSize,
                      fieldsSize);

    UA_ExtensionObject_clear(&regenerated);
    UA_DataType_clear(&type);
    free(fields);
} END_TEST

START_TEST(definition_tooManyEnumFields) {
    const size_t fieldsSize = UA_DATATYPE_MEMBERS_MAX + 1;
    UA_EnumField *fields = (UA_EnumField*)
        calloc(fieldsSize, sizeof(UA_EnumField));
    ck_assert_ptr_nonnull(fields);

    UA_EnumDescription description;
    UA_EnumDescription_init(&description);
    description.dataTypeId = UA_NODEID_NUMERIC(1, 5005);
    description.name = UA_QUALIFIEDNAME(1, "OversizedEnum");
    description.builtInType = UA_DATATYPEKIND_INT32 + 1;
    description.enumDefinition.fieldsSize = fieldsSize;
    description.enumDefinition.fields = fields;

    UA_ExtensionObject descriptionObject;
    UA_ExtensionObject_setValueNoDelete(
        &descriptionObject, &description, &UA_TYPES[UA_TYPES_ENUMDESCRIPTION]);
    UA_DataType type;
    memset(&type, 0, sizeof(type));
    UA_StatusCode res =
        UA_DataType_fromDescription(&type, &descriptionObject, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    free(fields);
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

START_TEST(definition_emptyStructureHasStorage) {
    UA_StructureDescription description;
    UA_StructureDescription_init(&description);
    description.dataTypeId = UA_NODEID_NUMERIC(1, 5006);
    description.name = UA_QUALIFIEDNAME(1, "EmptyStructure");
    description.structureDefinition.baseDataType = UA_NS0ID(STRUCTURE);
    description.structureDefinition.structureType = UA_STRUCTURETYPE_STRUCTURE;

    UA_ExtensionObject descriptionObject;
    UA_ExtensionObject_setValueNoDelete(
        &descriptionObject, &description, &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION]);
    UA_DataType type;
    memset(&type, 0, sizeof(type));
    UA_StatusCode res = UA_DataType_fromDescription(&type, &descriptionObject, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(type.memSize, 1);
    ck_assert_uint_eq(type.membersSize, 0);

    void *array = UA_Array_new(2, &type);
    ck_assert_ptr_ne(array, NULL);
    UA_Array_delete(array, 2, &type);
    UA_DataType_clear(&type);
} END_TEST

static UA_StructureDescription
structureDescription(UA_StructureField *fields, size_t fieldsSize) {
    UA_StructureDescription description;
    UA_StructureDescription_init(&description);
    description.dataTypeId = UA_NODEID_NUMERIC(1, 5007);
    description.name = UA_QUALIFIEDNAME(1, "TestStructure");
    description.structureDefinition.baseDataType = UA_NS0ID(STRUCTURE);
    description.structureDefinition.structureType = UA_STRUCTURETYPE_STRUCTURE;
    description.structureDefinition.fields = fields;
    description.structureDefinition.fieldsSize = fieldsSize;
    return description;
}

static UA_StatusCode
fromStructureDescription(UA_DataType *type, UA_StructureDescription *description,
                         const UA_DataTypeArray *customTypes) {
    UA_ExtensionObject descriptionObject;
    UA_ExtensionObject_setValueNoDelete(
        &descriptionObject, description, &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION]);
    return UA_DataType_fromDescription(type, &descriptionObject, customTypes);
}

START_TEST(definition_structureArrayDimensions) {
    /* An unspecified or wildcard length describes a variable-length array.
     * Fixed-size arrays and multiple dimensions cannot describe a C member. */
    UA_UInt32 dimensions[] = {0, 0};
    UA_StructureField field;
    UA_StructureField_init(&field);
    field.name = UA_STRING("Values");
    field.dataType = UA_NS0ID(DOUBLE);
    field.valueRank = UA_VALUERANK_ONE_DIMENSION;
    field.arrayDimensions = dimensions;
    field.arrayDimensionsSize = (_i == 0) ? 0 : ((_i == 3) ? 2 : 1);
    if(_i == 2)
        dimensions[0] = 3;

    UA_StructureDescription description = structureDescription(&field, 1);
    UA_DataType type;
    memset(&type, 0, sizeof(type));
    UA_StatusCode res = fromStructureDescription(&type, &description, NULL);
    if(_i < 2) {
        struct ArrayMember {
            size_t valuesSize;
            UA_Double *values;
        };
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(type.memSize, sizeof(struct ArrayMember));
        ck_assert_uint_eq(type.membersSize, 1);
        ck_assert(type.members[0].isArray);
        ck_assert(!type.pointerFree);
        ck_assert_ptr_eq(type.members[0].memberType, &UA_TYPES[UA_TYPES_DOUBLE]);
    } else {
        ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
        ck_assert_uint_eq(type.membersSize, 0);
        ck_assert_ptr_null(type.members);
    }
    UA_DataType_clear(&type);
} END_TEST

START_TEST(definition_structureUnsupportedValueRank) {
    const UA_Int32 ranks[] = {
        UA_VALUERANK_SCALAR_OR_ONE_DIMENSION, UA_VALUERANK_ANY,
        UA_VALUERANK_ONE_OR_MORE_DIMENSIONS, UA_VALUERANK_TWO_DIMENSIONS,
        UA_VALUERANK_THREE_DIMENSIONS
    };
    UA_StructureField field;
    UA_StructureField_init(&field);
    field.name = UA_STRING("Value");
    field.dataType = UA_NS0ID(DOUBLE);
    field.valueRank = ranks[_i];

    UA_StructureDescription description = structureDescription(&field, 1);
    UA_DataType type;
    memset(&type, 0, sizeof(type));
    UA_StatusCode res = fromStructureDescription(&type, &description, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(type.membersSize, 0);
    ck_assert_ptr_null(type.members);
    UA_DataType_clear(&type);
} END_TEST

START_TEST(definition_structureUnexpectedOptionalField) {
    UA_StructureField field;
    UA_StructureField_init(&field);
    field.name = UA_STRING("Value");
    field.dataType = UA_NS0ID(DOUBLE);
    field.valueRank = UA_VALUERANK_SCALAR;
    field.isOptional = true;

    UA_StructureDescription description = structureDescription(&field, 1);
    if(_i == 1) {
        description.structureDefinition.baseDataType = UA_NS0ID(UNION);
        description.structureDefinition.structureType = UA_STRUCTURETYPE_UNION;
    }
    UA_DataType type;
    memset(&type, 0, sizeof(type));
    UA_StatusCode res = fromStructureDescription(&type, &description, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(type.membersSize, 0);
    ck_assert_ptr_null(type.members);
    UA_DataType_clear(&type);
} END_TEST

START_TEST(definition_structureMemberLimit) {
    const size_t fieldsSize = UA_DATATYPE_MEMBERS_MAX + (size_t)_i;
    UA_StructureField *fields = (UA_StructureField*)
        calloc(fieldsSize, sizeof(UA_StructureField));
    ck_assert_ptr_nonnull(fields);
    for(size_t i = 0; i < fieldsSize; i++) {
        fields[i].name = UA_STRING("Value");
        fields[i].dataType = UA_NS0ID(DOUBLE);
        fields[i].valueRank = UA_VALUERANK_SCALAR;
    }

    UA_StructureDescription description = structureDescription(fields, fieldsSize);
    UA_DataType type;
    memset(&type, 0, sizeof(type));
    UA_StatusCode res = fromStructureDescription(&type, &description, NULL);
    if(_i == 0) {
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(type.membersSize, fieldsSize);
        ck_assert_uint_eq(type.memSize, fieldsSize * sizeof(UA_Double));
        ck_assert_ptr_eq(type.members[fieldsSize - 1].memberType,
                         &UA_TYPES[UA_TYPES_DOUBLE]);
    } else {
        ck_assert_uint_eq(res, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
        ck_assert_uint_eq(type.membersSize, 0);
        ck_assert_ptr_null(type.members);
    }
    UA_DataType_clear(&type);
    free(fields);
} END_TEST

START_TEST(definition_structureSizeLimit) {
    /* A nested 1024-byte structure lets us reach the 16-bit memory-size limit
     * without exceeding the independent limit on the number of members. */
    UA_StructureField blockFields[128];
    for(size_t i = 0; i < 128; i++) {
        UA_StructureField_init(&blockFields[i]);
        blockFields[i].name = UA_STRING("Value");
        blockFields[i].dataType = UA_NS0ID(DOUBLE);
        blockFields[i].valueRank = UA_VALUERANK_SCALAR;
    }
    UA_StructureDescription blockDescription = structureDescription(blockFields, 128);
    blockDescription.dataTypeId = UA_NODEID_NUMERIC(1, 5008);
    UA_DataType blockType;
    memset(&blockType, 0, sizeof(blockType));
    UA_StatusCode res = fromStructureDescription(&blockType, &blockDescription, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(blockType.memSize, 1024);
    UA_DataTypeArray customTypes = {NULL, 1, &blockType, false};

    /* 65528 bytes fit; 65536 bytes overflow during member accumulation;
     * 65535 bytes overflow only when the final alignment padding is added. */
    const size_t blockCounts[] = {63, 64, 63};
    const size_t doubleCounts[] = {127, 0, 127};
    const size_t byteCounts[] = {0, 0, 7};
    const size_t fieldsSize = blockCounts[_i] + doubleCounts[_i] + byteCounts[_i];
    UA_StructureField fields[197];
    for(size_t i = 0; i < fieldsSize; i++) {
        UA_StructureField_init(&fields[i]);
        fields[i].name = UA_STRING("Value");
        fields[i].valueRank = UA_VALUERANK_SCALAR;
        if(i < blockCounts[_i])
            fields[i].dataType = blockType.typeId;
        else if(i < blockCounts[_i] + doubleCounts[_i])
            fields[i].dataType = UA_NS0ID(DOUBLE);
        else
            fields[i].dataType = UA_NS0ID(BYTE);
    }

    UA_StructureDescription description = structureDescription(fields, fieldsSize);
    UA_DataType type;
    memset(&type, 0, sizeof(type));
    res = fromStructureDescription(&type, &description, &customTypes);
    if(_i == 0) {
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(type.memSize, 65528);
        ck_assert_uint_eq(type.membersSize, fieldsSize);
        ck_assert(type.pointerFree);
        ck_assert(type.overlayable);
    } else {
        ck_assert_uint_eq(res, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
        ck_assert_uint_eq(type.membersSize, 0);
        ck_assert_ptr_null(type.members);
        ck_assert_uint_eq(type.memSize, 0);
    }
    UA_DataType_clear(&type);
    UA_DataType_clear(&blockType);
} END_TEST

static Suite *testSuite_definition(void) {
    TCase *tc = tcase_create("DataTypeDefinition");
    tcase_add_test(tc, definition_roundtrip_structure);
    tcase_add_test(tc, definition_roundtrip_structure_nested);
    tcase_add_test(tc, definition_roundtrip_simpleType);
    tcase_add_test(tc, definition_roundtrip_enum);
    tcase_add_test(tc, definition_largeEnum);
    tcase_add_test(tc, definition_tooManyEnumFields);
    tcase_add_test(tc, definition_fromDescription_invalidExtObj);
    tcase_add_test(tc, definition_fromStructure_unknownMember);
    tcase_add_test(tc, definition_emptyStructureHasStorage);
    tcase_add_loop_test(tc, definition_structureArrayDimensions, 0, 4);
    tcase_add_loop_test(tc, definition_structureUnsupportedValueRank, 0, 5);
    tcase_add_loop_test(tc, definition_structureUnexpectedOptionalField, 0, 2);
    tcase_add_loop_test(tc, definition_structureMemberLimit, 0, 2);
    tcase_add_loop_test(tc, definition_structureSizeLimit, 0, 3);

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
