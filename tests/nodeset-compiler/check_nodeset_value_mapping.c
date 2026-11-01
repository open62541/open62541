/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "namespace_tests_value_mapping_generated.h"
#include "test_helpers.h"
#include <check.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef UA_ENABLE_XML_ENCODING
START_TEST(loadValues) {
    typedef struct { UA_NodeId target; UA_QualifiedName label; } MappingRecord;
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    UA_Server_addNamespace(server, "urn:unrelated:first");
    UA_Server_addNamespace(server, "urn:unrelated:second");
    UA_UInt16 a = UA_Server_addNamespace(server, "urn:open62541:test:map-a");
    UA_DataTypeMember members[] = {
        {UA_TYPENAME("Target") &UA_TYPES[UA_TYPES_NODEID], 0, false, false},
        {UA_TYPENAME("Label") &UA_TYPES[UA_TYPES_QUALIFIEDNAME],
         offsetof(MappingRecord, label) - sizeof(UA_NodeId), false, false}};
    UA_DataType type;
    memset(&type, 0, sizeof(type));
#ifdef UA_ENABLE_TYPEDESCRIPTION
    type.typeName = "MappingRecord";
#endif
    type.typeId = UA_NODEID_NUMERIC(a, 9000);
    type.binaryEncodingId = UA_NODEID_NUMERIC(a, 9001);
    type.xmlEncodingId = UA_NODEID_NUMERIC(a, 9002);
    type.typeKind = UA_DATATYPEKIND_STRUCTURE;
    type.memSize = sizeof(MappingRecord); type.membersSize = 2; type.members = members;
    ck_assert_uint_eq(UA_Server_addDataTypeNode(server, type.typeId, UA_NODEID_NUMERIC(0, 22),
        UA_NODEID_NUMERIC(0, 45), UA_QUALIFIEDNAME(a, "MappingRecord"),
        UA_DataTypeAttributes_default, NULL, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addDataType(server, UA_NODEID_NUMERIC(0, 22), &type), UA_STATUSCODE_GOOD);
    /* This descriptor lives in the internal registry, not config.customDataTypes. */
    ck_assert_ptr_eq(UA_Server_getConfig(server)->customDataTypes, NULL);
    ck_assert_uint_eq(namespace_tests_value_mapping_generated(server, true), UA_STATUSCODE_GOOD);
    size_t bIndex = 0;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(server,
        UA_STRING("urn:open62541:test:map-b"), &bIndex), UA_STATUSCODE_GOOD);
    UA_UInt16 b = (UA_UInt16)bIndex;
    for(size_t model = 0; model < 2; model++) {
        UA_UInt16 ns = model ? b : a;
        UA_UInt32 base = model ? 2000 : 1000;
        UA_Variant value;
        ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(ns, base+1), &value), UA_STATUSCODE_GOOD);
        ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_NODEID]));
        ck_assert_uint_eq(((UA_NodeId*)value.data)->namespaceIndex, a);
        UA_Variant_clear(&value);
        ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(ns, base+2), &value), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(((UA_QualifiedName*)value.data)->namespaceIndex, a);
        UA_Variant_clear(&value);
        ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(ns, base+3), &value), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(((UA_ExpandedNodeId*)value.data)->nodeId.namespaceIndex, a);
        UA_Variant_clear(&value);
    }
    UA_Variant value;
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(a, 1004), &value), UA_STATUSCODE_GOOD);
    UA_String expected = UA_STRING("first\n  Grüße\n last");
    ck_assert(UA_String_equal((UA_String*)value.data, &expected));
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(a, 1005), &value), UA_STATUSCODE_GOOD);
    UA_String pattern = UA_STRING("Grüße<&\\");
    UA_String *text = (UA_String*)value.data;
    ck_assert_uint_eq(text->length, pattern.length * 4000);
    for(size_t i = 0; i < 4000; i++)
        ck_assert_int_eq(memcmp(text->data + i * pattern.length, pattern.data, pattern.length), 0);
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(a, 1007), &value), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(value.type, UA_Server_findDataType(server, &type.typeId));
    MappingRecord *record = (MappingRecord*)value.data;
    ck_assert_uint_eq(record->target.namespaceIndex, a);
    ck_assert_uint_eq(record->label.namespaceIndex, a);
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(b, 2004), &value), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(value.arrayLength, 1);
    ck_assert_uint_eq(*(UA_UInt32*)value.data, 2);
    UA_Variant_clear(&value);
    UA_Server_delete(server);
} END_TEST
#endif

int main(void) {
    Suite *s = suite_create("Generated XML value namespaces and text");
    TCase *tc = tcase_create("Multiple input files");
#ifdef UA_ENABLE_XML_ENCODING
    tcase_add_test(tc, loadValues);
#endif
    suite_add_tcase(s, tc);
    SRunner *runner = srunner_create(s);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
