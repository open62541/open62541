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

START_TEST(definition_unionNativeAlignment) {
    typedef struct {
        UA_UInt32 switchField;
        union { UA_Byte byte; UA_UInt16 word; } fields;
    } SmallUnion;
    typedef struct {
        UA_UInt32 switchField;
        union { UA_UInt32 number; UA_String text; } fields;
    } MixedUnion;
    UA_DataTypeMember members[2] = {
        {UA_TYPENAME("First") &UA_TYPES[UA_TYPES_BYTE],
         offsetof(SmallUnion, fields.byte), false, false},
        {UA_TYPENAME("Second") &UA_TYPES[UA_TYPES_UINT16],
         offsetof(SmallUnion, fields.word), false, false}};
    UA_DataType original;
    memset(&original, 0, sizeof(original));
#ifdef UA_ENABLE_TYPEDESCRIPTION
    original.typeName = "NativeUnion";
#endif
    original.typeId = UA_NODEID_NUMERIC(1, 9001);
    original.typeKind = UA_DATATYPEKIND_UNION;
    original.memSize = sizeof(SmallUnion);
    original.membersSize = 2;
    original.members = members;
    if(_i) {
        original.memSize = sizeof(MixedUnion);
        members[0].memberType = &UA_TYPES[UA_TYPES_UINT32];
        members[0].padding = offsetof(MixedUnion, fields.number);
        members[1].memberType = &UA_TYPES[UA_TYPES_STRING];
        members[1].padding = offsetof(MixedUnion, fields.text);
    }
    UA_ExtensionObject description;
    ck_assert_uint_eq(UA_DataType_toDescription(&original, &description), UA_STATUSCODE_GOOD);
    UA_DataType reconstructed;
    ck_assert_uint_eq(UA_DataType_fromDescription(&reconstructed, &description, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(reconstructed.memSize, original.memSize);
    for(size_t i = 0; i < 2; i++)
        ck_assert_uint_eq(reconstructed.members[i].padding, members[i].padding);
    /* Encoding through the reconstructed descriptor must use the C union's
     * offset even for the member with weaker alignment. */
    MixedUnion value;
    memset(&value, 0, sizeof(value));
    value.switchField = 1;
    if(_i)
        value.fields.number = 42;
    else
        ((SmallUnion*)(void*)&value)->fields.byte = 42;
    UA_ByteString expected = UA_BYTESTRING_NULL;
    UA_ByteString actual = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(&value, &original, &expected, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_encodeBinary(&value, &reconstructed, &actual, NULL), UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&expected, &actual));
    UA_ByteString_clear(&expected);
    UA_ByteString_clear(&actual);
    UA_ExtensionObject_clear(&description);
    UA_DataType_clear(&reconstructed);
} END_TEST

START_TEST(definition_copyOwnedNodeIds) {
    UA_DataType original = UA_TYPES[UA_TYPES_ARGUMENT];
    original.typeId = UA_NODEID_STRING(2, "Argument");
    original.binaryEncodingId = UA_NODEID_STRING(3, "Argument.Binary");
    original.xmlEncodingId = UA_NODEID_STRING(4, "Argument.XML");
    UA_DataType copied;
    ck_assert_uint_eq(UA_DataType_copy(&original, &copied), UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&original.typeId, &copied.typeId));
    ck_assert_ptr_ne(original.typeId.identifier.string.data,
                     copied.typeId.identifier.string.data);
    ck_assert(UA_NodeId_equal(&original.binaryEncodingId, &copied.binaryEncodingId));
    ck_assert(UA_NodeId_equal(&original.xmlEncodingId, &copied.xmlEncodingId));
    UA_DataType_clear(&copied);
    /* Clearing the copy must leave the borrowed original strings intact. */
    ck_assert_uint_eq(original.xmlEncodingId.identifier.string.data[0], 'A');
} END_TEST

START_TEST(definition_invalidStructureKindClearsOutput) {
    UA_StructureDescription description;
    UA_StructureDescription_init(&description);
    description.dataTypeId = UA_NODEID_STRING(1, "Unsupported");
    description.name = UA_QUALIFIEDNAME(1, "Unsupported");
    description.structureDefinition.structureType = (_i == 0) ?
        UA_STRUCTURETYPE_STRUCTUREWITHSUBTYPEDVALUES : (UA_StructureType)99;
    UA_ExtensionObject object;
    UA_ExtensionObject_setValueNoDelete(&object, &description, &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION]);
    UA_DataType output;
    ck_assert_uint_ne(UA_DataType_fromDescription(&output, &object, NULL), UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_isNull(&output.typeId));
#ifdef UA_ENABLE_TYPEDESCRIPTION
    ck_assert_ptr_eq(output.typeName, NULL);
#endif
    UA_DataType_clear(&output);
} END_TEST

static Suite *testSuite_definition(void) {
    TCase *tc = tcase_create("DataTypeDefinition");
    tcase_add_loop_test(tc, definition_invalidStructureKindClearsOutput, 0, 2);
    tcase_add_test(tc, definition_copyOwnedNodeIds);
    tcase_add_loop_test(tc, definition_unionNativeAlignment, 0, 2);
    tcase_add_test(tc, definition_roundtrip_structure);
    tcase_add_test(tc, definition_roundtrip_structure_nested);
    tcase_add_test(tc, definition_roundtrip_simpleType);
    tcase_add_test(tc, definition_roundtrip_enum);
    tcase_add_test(tc, definition_fromDescription_invalidExtObj);
    tcase_add_test(tc, definition_fromStructure_unknownMember);
    tcase_add_test(tc, definition_emptyStructureHasStorage);

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
