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

static Suite *testSuite_definition(void) {
    TCase *tc = tcase_create("DataTypeDefinition");
    tcase_add_test(tc, definition_roundtrip_structure);
    tcase_add_test(tc, definition_roundtrip_structure_nested);
    tcase_add_test(tc, definition_roundtrip_simpleType);
    tcase_add_test(tc, definition_roundtrip_enum);
    tcase_add_test(tc, definition_fromDescription_invalidExtObj);
    tcase_add_test(tc, definition_fromStructure_unknownMember);

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
