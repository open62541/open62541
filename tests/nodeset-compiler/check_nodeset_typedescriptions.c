/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "check.h"
#include "namespace_typedescriptions_generated.h"
#include "test_helpers.h"

static UA_Server *server;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
}

static void
teardown(void) {
    UA_Server_delete(server);
}

static const UA_DataType *
findType(size_t namespaceIndex, UA_UInt32 identifier) {
    UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)namespaceIndex, identifier);
    return UA_Server_findDataType(server, &id);
}

START_TEST(Server_loadTypeDefinitionsFromNodeset) {
    /* Load the namespace and resolve its assigned runtime namespace index. */
    UA_StatusCode retval = namespace_typedescriptions_generated(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t namespaceIndex = 0;
    retval = UA_Server_getNamespaceByName(
        server, UA_STRING("http://model.o6-automation.com"), &namespaceIndex);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify nested and optional structure layouts. */
    const UA_DataType *address = findType(namespaceIndex, 3002);
    ck_assert_ptr_nonnull(address);
    ck_assert_ptr_eq(address, &UA_TYPES_TYPEDESCRIPTIONS[
                                  UA_TYPES_TYPEDESCRIPTIONS_ADDRESS]);
    ck_assert_uint_eq(address->typeKind, UA_DATATYPEKIND_STRUCTURE);
    ck_assert_uint_eq(address->membersSize, 2);

    const UA_DataType *person = findType(namespaceIndex, 3000);
    ck_assert_ptr_nonnull(person);
    ck_assert_ptr_eq(person, &UA_TYPES_TYPEDESCRIPTIONS[
                                 UA_TYPES_TYPEDESCRIPTIONS_PERSON]);
    ck_assert_uint_eq(person->typeKind, UA_DATATYPEKIND_OPTSTRUCT);
    ck_assert_uint_eq(person->membersSize, 4);
    ck_assert_ptr_eq(person->members[1].memberType, address);
    ck_assert(person->members[2].isArray);
    ck_assert(person->members[3].isOptional);

    /* Verify enum, union and OptionSet representations. */
    const UA_DataType *enumeration = findType(namespaceIndex, 3001);
    ck_assert_ptr_nonnull(enumeration);
    ck_assert_uint_eq(enumeration->typeKind, UA_DATATYPEKIND_ENUM);
    ck_assert_uint_eq(enumeration->membersSize, 3);
    ck_assert_int_eq((UA_Int64)(intptr_t)enumeration->members[0].memberType, -7);
    ck_assert_int_eq((UA_Int64)(intptr_t)enumeration->members[1].memberType, 42);
    ck_assert_int_eq((UA_Int64)(intptr_t)enumeration->members[2].memberType, 3);

    const UA_DataType *measurement = findType(namespaceIndex, 3003);
    ck_assert_ptr_nonnull(measurement);
    ck_assert_uint_eq(measurement->typeKind, UA_DATATYPEKIND_UNION);
    ck_assert_uint_eq(measurement->membersSize, 2);

    const UA_DataType *optionSet = findType(namespaceIndex, 3004);
    ck_assert_ptr_nonnull(optionSet);
    ck_assert_uint_eq(optionSet->typeKind, UA_DATATYPEKIND_UINT32);
    ck_assert_uint_eq(optionSet->memSize, sizeof(UA_UInt32));
    ck_assert_uint_eq(optionSet->membersSize, 0);
    ck_assert_uint_eq(sizeof(UA_O6OptionSet), sizeof(UA_UInt32));

    /* Exercise copy and clear for a self-recursive array member. */
    const UA_DataType *recursive = findType(namespaceIndex, 3005);
    ck_assert_ptr_nonnull(recursive);
    ck_assert_uint_eq(recursive->typeKind, UA_DATATYPEKIND_STRUCTURE);
    ck_assert_uint_eq(recursive->membersSize, 2);
    ck_assert(recursive->members[1].isArray);
    ck_assert_ptr_eq(recursive->members[1].memberType, recursive);

    UA_RecursiveRecord source = {0};
    source.name = UA_STRING_ALLOC("root");
    source.childrenSize = 1;
    source.children = (UA_RecursiveRecord *)UA_calloc(
        source.childrenSize, sizeof(UA_RecursiveRecord));
    ck_assert_ptr_nonnull(source.children);
    source.children[0].name = UA_STRING_ALLOC("child");
    UA_RecursiveRecord copy = {0};
    retval = UA_copy(&source, &copy, recursive);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(copy.childrenSize, 1);
    ck_assert(UA_String_equal(&copy.children[0].name,
                              &source.children[0].name));
    UA_clear(&source, recursive);
    UA_clear(&copy, recursive);

    /* Non-negative ValueRanks share the size-plus-pointer array layout. */
    const UA_DataType *ranked = findType(namespaceIndex, 3006);
    ck_assert_ptr_nonnull(ranked);
    ck_assert_uint_eq(ranked->typeKind, UA_DATATYPEKIND_STRUCTURE);
    ck_assert_uint_eq(ranked->membersSize, 2);
    ck_assert(ranked->members[0].isArray);
    ck_assert(ranked->members[1].isArray);

#ifdef UA_ENABLE_TYPEDESCRIPTION
    /* Verify metadata reconstructed on demand from static layouts. */
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC((UA_UInt16)namespaceIndex, 3001);
    rvi.attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;
    UA_DataValue definitionValue =
        UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(definitionValue.status, UA_STATUSCODE_GOOD);
    ck_assert(definitionValue.hasValue);
    ck_assert_ptr_eq(definitionValue.value.type,
                     &UA_TYPES[UA_TYPES_ENUMDEFINITION]);
    UA_EnumDefinition *enumDefinition =
        (UA_EnumDefinition *)definitionValue.value.data;
    ck_assert_uint_eq(enumDefinition->fieldsSize, 3);
    ck_assert_int_eq(enumDefinition->fields[0].value, -7);
    ck_assert_int_eq(enumDefinition->fields[1].value, 42);
    ck_assert_int_eq(enumDefinition->fields[2].value, 3);
    UA_DataValue_clear(&definitionValue);

    rvi.nodeId = UA_NODEID_NUMERIC((UA_UInt16)namespaceIndex, 3006);
    definitionValue =
        UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(definitionValue.status, UA_STATUSCODE_GOOD);
    ck_assert(definitionValue.hasValue);
    ck_assert_ptr_eq(definitionValue.value.type,
                     &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]);
    UA_StructureDefinition *definition =
        (UA_StructureDefinition *)definitionValue.value.data;
    ck_assert_uint_eq(definition->fieldsSize, 2);
    /* DataTypeDefinition is materialized on demand from the static datatype
     * layout. The C ABI distinguishes scalar and array fields, but deliberately
     * normalizes arbitrary/multidimensional ranks to one-dimensional arrays. */
    ck_assert_int_eq(definition->fields[0].valueRank, 1);
    ck_assert_uint_eq(definition->fields[0].arrayDimensionsSize, 1);
    ck_assert_uint_eq(definition->fields[0].arrayDimensions[0], 0);
    ck_assert_uint_eq(definition->fields[0].maxStringLength, 0);
    ck_assert_int_eq(definition->fields[1].valueRank, 1);
    ck_assert_uint_eq(definition->fields[1].arrayDimensionsSize, 1);
    ck_assert_uint_eq(definition->fields[1].arrayDimensions[0], 0);
    UA_DataValue_clear(&definitionValue);
#endif
}
END_TEST

static Suite *
testSuite(void) {
    Suite *suite = suite_create("NodeSet datatype definitions");
    TCase *testCase = tcase_create("NodeSet2 Definition parsing");
    tcase_add_checked_fixture(testCase, setup, teardown);
    tcase_add_test(testCase, Server_loadTypeDefinitionsFromNodeset);
    suite_add_tcase(suite, testCase);
    return suite;
}

int
main(void) {
    Suite *suite = testSuite();
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
