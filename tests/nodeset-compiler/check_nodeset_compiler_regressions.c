/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/util.h>
#include "test_helpers.h"
#include "namespace_compiler_fixes_generated.h"
#include "namespace_compiler_negative_generated.h"
#include <check.h>
#include <string.h>
#include <stdlib.h>

static UA_Server *compiled;
static UA_DataTypeArray customTypes = {
    NULL, UA_TYPES_TESTS_COMPILER_FIXES_COUNT, UA_TYPES_TESTS_COMPILER_FIXES, false
};

static void
setup(void) {
    compiled = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(compiled, NULL);
    UA_Server_getConfig(compiled)->customDataTypes = &customTypes;
    ck_assert_uint_eq(namespace_compiler_fixes_generated(compiled), UA_STATUSCODE_GOOD);
}

static void
teardown(void) {
    UA_Server_delete(compiled);
}

static UA_Variant
readValue(UA_UInt32 id) {
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(compiled, UA_NODEID_NUMERIC(2, id), &value),
                      UA_STATUSCODE_GOOD);
    return value;
}

START_TEST(checkGeneratedValues) {
    UA_Variant value = readValue(1001);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE]));
    ck_assert(*(UA_Double*)value.data == 0.5);
    UA_Variant_clear(&value);
    value = readValue(1002);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_int_eq(*(UA_Int32*)value.data, 42);
    UA_Variant_clear(&value);
    value = readValue(1003);
    ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_uint_eq(value.arrayLength, 2);
    ck_assert_int_eq(((UA_Int32*)value.data)[0], 1);
    ck_assert_int_eq(((UA_Int32*)value.data)[1], 2);
    UA_Variant_clear(&value);
    value = readValue(1007);
    ck_assert(UA_Variant_isScalar(&value));
    ck_assert_uint_eq(value.type->typeKind, UA_DATATYPEKIND_UINT32);
    ck_assert_uint_eq(*(UA_UInt32*)value.data, 7);
    UA_Variant_clear(&value);
    value = readValue(1008);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING]));
    ck_assert_uint_eq(((UA_String*)value.data)->length, 0);
    UA_Variant_clear(&value);
    value = readValue(1009);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_int_eq(*(UA_Int32*)value.data, 42);
    UA_Variant_clear(&value);
    const UA_UInt32 typeIds[] = {11, 6, 6, 10, 6, 6, 7003, 12, 24};
    for(size_t i = 0; i < 9; i++) {
        UA_NodeId type = UA_NODEID_NULL;
        ck_assert_uint_eq(UA_Server_readDataType(
            compiled, UA_NODEID_NUMERIC(2, (UA_UInt32)(1001 + i)), &type), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(type.namespaceIndex, i == 6 ? 2 : 0);
        ck_assert_uint_eq(type.identifierType, UA_NODEIDTYPE_NUMERIC);
        ck_assert_uint_eq(type.identifier.numeric, typeIds[i]);
        UA_NodeId_clear(&type);
    }
}
END_TEST

START_TEST(preserveArrayDefaults) {
    UA_Variant value = readValue(1010);
    ck_assert(!UA_Variant_isScalar(&value));
    ck_assert_uint_eq(value.type->typeKind, UA_DATATYPEKIND_UINT32);
    ck_assert_uint_eq(value.arrayLength, 0);
    ck_assert_ptr_eq(value.data, UA_EMPTY_ARRAY_SENTINEL);
    UA_Variant_clear(&value);

    value = readValue(1011);
    ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]));
    ck_assert_uint_eq(value.arrayLength, 0);
    ck_assert_ptr_eq(value.data, UA_EMPTY_ARRAY_SENTINEL);
    UA_Variant_clear(&value);

    value = readValue(1012);
    ck_assert(!UA_Variant_isScalar(&value));
    ck_assert_uint_eq(value.type->typeKind, UA_DATATYPEKIND_UINT32);
    ck_assert_uint_eq(value.arrayLength, 2);
    ck_assert_uint_eq(((UA_UInt32*)value.data)[0], 0);
    ck_assert_uint_eq(((UA_UInt32*)value.data)[1], 42);
    UA_Variant_clear(&value);
}
END_TEST

static void
checkText(const UA_LocalizedText *text, const char *prefix, const char *chunk,
          const char *locale) {
    const char *suffix = "trailing ";
    size_t prefixSize = strlen(prefix), chunkSize = strlen(chunk), suffixSize = strlen(suffix);
    ck_assert_uint_eq(text->text.length, prefixSize + 110 * chunkSize + suffixSize);
    ck_assert(memcmp(text->text.data, prefix, prefixSize) == 0);
    for(size_t i = 0; i < 110; i++)
        ck_assert(memcmp(text->text.data + prefixSize + i * chunkSize, chunk, chunkSize) == 0);
    ck_assert(memcmp(text->text.data + prefixSize + 110 * chunkSize, suffix, suffixSize) == 0);
    UA_String expectedLocale = locale ? UA_STRING((char*)(uintptr_t)locale) : UA_STRING_NULL;
    ck_assert(UA_String_equal(&text->locale, &expectedLocale));
    if(!locale)
        ck_assert_ptr_eq(text->locale.data, NULL);
}

START_TEST(preserveLocalizedText) {
    UA_LocalizedText text;
    UA_LocalizedText_init(&text);
    ck_assert_uint_eq(UA_Server_readDisplayName(compiled, UA_NODEID_NUMERIC(2, 1000), &text),
                      UA_STATUSCODE_GOOD);
    checkText(&text, " leading ", "a\\b\"c ", NULL);
    UA_LocalizedText_clear(&text);
    ck_assert_uint_eq(UA_Server_readDescription(compiled, UA_NODEID_NUMERIC(2, 1000), &text),
                      UA_STATUSCODE_GOOD);
    checkText(&text, " leading ", "a\\b\"c ", "en");
    UA_LocalizedText_clear(&text);
    ck_assert_uint_eq(UA_Server_readInverseName(compiled, UA_NODEID_NUMERIC(2, 5000), &text),
                      UA_STATUSCODE_GOOD);
    checkText(&text, " inverse ", "x\\y\"z ", "en");
    UA_LocalizedText_clear(&text);
}
END_TEST

START_TEST(preserveRanksAndMatrixValues) {
    const UA_Int32 expected[] = {UA_VALUERANK_SCALAR, UA_VALUERANK_ANY,
                                 UA_VALUERANK_ANY, 1};
    for(size_t i = 0; i < 4; i++) {
        UA_Int32 rank = 99;
        ck_assert_uint_eq(UA_Server_readValueRank(
            compiled, UA_NODEID_NUMERIC(2, (UA_UInt32)(1001 + i)), &rank), UA_STATUSCODE_GOOD);
        ck_assert_int_eq(rank, expected[i]);
    }
    for(UA_UInt32 id = 1005; id <= 1006; id++) {
        UA_Variant value;
        UA_Variant_init(&value);
        ck_assert_uint_eq(UA_Server_readValue(compiled, UA_NODEID_NUMERIC(2, id), &value),
                          UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(value.arrayLength, 4);
        ck_assert_uint_eq(value.arrayDimensionsSize, 2);
        ck_assert_uint_eq(value.arrayDimensions[0], id == 1005 ? 2 : 1);
        ck_assert_uint_eq(value.arrayDimensions[1], id == 1005 ? 2 : 4);
        ck_assert_int_eq(((UA_Int32*)value.data)[3], 4);
        UA_Variant_clear(&value);
    }
}
END_TEST

START_TEST(preserveNativeMetadata) {
    UA_FixesKeywords value = {1, 2, 3, 4, 5};
    ck_assert_uint_eq(value._unsigned, 2);
    ck_assert_uint_eq(value._default, 3);
    ck_assert_uint_eq(value.ab, 4);
    ck_assert_uint_eq(value._and_eq, 5);
    UA__3DRecord record = {5};
    ck_assert_uint_eq(record.id, 5);
    const UA_DataType *keywords = &UA_TYPES_TESTS_COMPILER_FIXES[
        UA_TYPES_TESTS_COMPILER_FIXES_FIXESKEYWORDS];
    ck_assert(keywords->pointerFree);
    ck_assert_str_eq(keywords->members[0].memberName, "id");
    ck_assert_str_eq(keywords->members[1].memberName, "unsigned");
    ck_assert_str_eq(keywords->members[2].memberName, "default");
    ck_assert_str_eq(keywords->members[3].memberName, "a-b");
    ck_assert_str_eq(keywords->members[4].memberName, "and_eq");
    const UA_DataType *numeric = &UA_TYPES_TESTS_COMPILER_FIXES[
        UA_TYPES_TESTS_COMPILER_FIXES_COUNTER];
    ck_assert(numeric->pointerFree);
    ck_assert_uint_eq(numeric->typeKind, UA_DATATYPEKIND_UINT32);
    ck_assert_str_eq(UA_TYPES_TESTS_COMPILER_FIXES[
        UA_TYPES_TESTS_COMPILER_FIXES_3DRECORD].typeName, "3DRecord");
    ck_assert_int_eq(UA_FIXESLABELS_AB, 0);
    ck_assert_int_eq(UA_FIXESLABELS_AB_1, 1);
    ck_assert_int_eq(UA_FIXESLABELS_AB_2, 2);
    ck_assert_int_eq(UA_FIXESLABELS_AB_1_3, 3);
}
END_TEST

START_TEST(rejectNegativeUnsignedAlias) {
    ck_assert_uint_eq(namespace_compiler_negative_generated(compiled), UA_STATUSCODE_BADTYPEMISMATCH);
    UA_NodeClass nodeClass;
    ck_assert_uint_eq(UA_Server_readNodeClass(compiled, UA_NODEID_NUMERIC(2, 1999), &nodeClass),
                      UA_STATUSCODE_BADNODEIDUNKNOWN);

}
END_TEST

int
main(void) {
    Suite *suite = suite_create("Compiler regressions with BSD datatypes");
    TCase *testCase = tcase_create("Existing BSD compiler");
    tcase_add_checked_fixture(testCase, setup, teardown);
    tcase_add_test(testCase, checkGeneratedValues);
    tcase_add_test(testCase, preserveArrayDefaults);
    tcase_add_test(testCase, preserveLocalizedText);
    tcase_add_test(testCase, preserveRanksAndMatrixValues);
    tcase_add_test(testCase, preserveNativeMetadata);
    tcase_add_test(testCase, rejectNegativeUnsignedAlias);
    suite_add_tcase(suite, testCase);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
