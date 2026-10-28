/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <open62541/types.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>

static UA_UInt16 indices[] = {0, 2, 1};
static UA_NamespaceMapping mapping;

START_TEST(binaryQualifiedName) {
    UA_QualifiedName value = UA_QUALIFIEDNAME(2, "Mapped");
    UA_EncodeBinaryOptions encode;
    memset(&encode, 0, sizeof(encode));
    encode.namespaceMapping = &mapping;
    UA_ByteString bytes = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(&value, &UA_TYPES[UA_TYPES_QUALIFIEDNAME],
                                      &bytes, &encode), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bytes.data[0], 1);
    UA_DecodeBinaryOptions decode;
    memset(&decode, 0, sizeof(decode));
    decode.namespaceMapping = &mapping;
    UA_QualifiedName output;
    ck_assert_uint_eq(UA_decodeBinary(&bytes, &output, &UA_TYPES[UA_TYPES_QUALIFIEDNAME],
                                      &decode), UA_STATUSCODE_GOOD);
    ck_assert(UA_QualifiedName_equal(&value, &output));
    UA_QualifiedName_clear(&output);
    UA_ByteString_clear(&bytes);
} END_TEST

#ifdef UA_ENABLE_XML_ENCODING
START_TEST(xmlBinaryBody) {
    typedef struct { UA_NodeId id; } Child;
    typedef struct { UA_NodeId id; UA_QualifiedName name; UA_ExtensionObject child; } Parent;
    UA_DataTypeMember childMember = {UA_TYPENAME("Id") &UA_TYPES[UA_TYPES_NODEID], 0, false, false};
    UA_DataTypeMember parentMembers[] = {
        {UA_TYPENAME("Id") &UA_TYPES[UA_TYPES_NODEID], 0, false, false},
        {UA_TYPENAME("Name") &UA_TYPES[UA_TYPES_QUALIFIEDNAME],
         offsetof(Parent, name) - sizeof(UA_NodeId), false, false},
        {UA_TYPENAME("Child") &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
         offsetof(Parent, child) - offsetof(Parent, name) - sizeof(UA_QualifiedName), false, false}};
    UA_DataType types[2];
    memset(types, 0, sizeof(types));
    for(size_t i = 0; i < 2; i++) {
        types[i].typeId = UA_NODEID_NUMERIC(2, 9000 + (UA_UInt32)i * 10);
        types[i].binaryEncodingId = UA_NODEID_NUMERIC(2, 9001 + (UA_UInt32)i * 10);
        types[i].typeKind = UA_DATATYPEKIND_STRUCTURE;
    }
    types[0].memSize = sizeof(Child); types[0].membersSize = 1; types[0].members = &childMember;
    types[1].memSize = sizeof(Parent); types[1].membersSize = 3; types[1].members = parentMembers;
    UA_DataTypeArray custom = {NULL, 2, types, false};
    Child child = {UA_NODEID_NUMERIC(2, 42)};
    Parent parent;
    memset(&parent, 0, sizeof(parent));
    parent.id = child.id;
    parent.name = UA_QUALIFIEDNAME(2, "Target");
    UA_ExtensionObject_setValueNoDelete(&parent.child, &child, &types[0]);
    UA_EncodeBinaryOptions encode;
    memset(&encode, 0, sizeof(encode)); encode.namespaceMapping = &mapping;
    UA_ByteString bytes = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(&parent, &types[1], &bytes, &encode), UA_STATUSCODE_GOOD);
    UA_String base64;
    ck_assert_uint_eq(UA_ByteString_toBase64(&bytes, &base64), UA_STATUSCODE_GOOD);
    char xml[2048];
    /* Accept the datatype ID for legacy XML and the proper binary encoding ID. */
    int length = snprintf(xml, sizeof(xml), "<ExtensionObject><TypeId><Identifier>ns=1;i=%u</Identifier>"
        "</TypeId><Body><ByteString>%.*s</ByteString></Body></ExtensionObject>",
        _i ? 9011u : 9010u, (int)base64.length, (char*)base64.data);
    ck_assert_int_gt(length, 0);
    ck_assert_int_lt(length, (int)sizeof(xml));
    UA_ByteString input = {(size_t)length, (UA_Byte*)xml};
    UA_DecodeXmlOptions decode;
    memset(&decode, 0, sizeof(decode));
    decode.namespaceMapping = &mapping; decode.customTypes = &custom;
    UA_ExtensionObject result;
    ck_assert_uint_eq(UA_decodeXml(&input, &result, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &decode),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_ExtensionObject_hasDecodedType(&result, &types[1]));
    Parent *output = (Parent*)result.content.decoded.data;
    ck_assert_uint_eq(output->id.namespaceIndex, 2);
    ck_assert_uint_eq(output->name.namespaceIndex, 2);
    ck_assert(UA_ExtensionObject_hasDecodedType(&output->child, &types[0]));
    ck_assert_uint_eq(((Child*)output->child.content.decoded.data)->id.namespaceIndex, 2);
    UA_ExtensionObject_clear(&result);
    UA_ByteString_clear(&bytes);
    UA_String_clear(&base64);
} END_TEST
#endif

int main(void) {
    memset(&mapping, 0, sizeof(mapping));
    mapping.local2remote = indices; mapping.local2remoteSize = 3;
    mapping.remote2local = indices; mapping.remote2localSize = 3;
    Suite *s = suite_create("Codec namespace mapping");
    TCase *tc = tcase_create("Mapping and nested custom types");
    tcase_add_test(tc, binaryQualifiedName);
#ifdef UA_ENABLE_XML_ENCODING
    tcase_add_loop_test(tc, xmlBinaryBody, 0, 2);
#endif
    suite_add_tcase(s, tc);
    SRunner *runner = srunner_create(s);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
