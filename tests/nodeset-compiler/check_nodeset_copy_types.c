/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "namespace_runtime_a_generated.h"
#include "namespace_runtime_b_generated.h"

/* Include one loader implementation to exercise its file-local support. */
#include "namespace_runtime_a_generated.c"
#include "types_runtime_a_generated_internal.h"
#include "types_runtime_b_generated_internal.h"

#include "test_helpers.h"
#include <check.h>
#include <stdio.h>

UA_StatusCode namespace_runtime_ab_generated(UA_Server *server, UA_Boolean copy_types);

static const UA_GeneratedTypeSource sourceA = UA_TYPES_RUNTIME_A_SOURCE;
static const UA_GeneratedTypeSource sourceB = UA_TYPES_RUNTIME_B_SOURCE;

#define A_URI "urn:open62541:test:types-a"
#define B_URI "urn:open62541:test:types-b"

static UA_UInt16
namespaceIndex(UA_Server *server, const char *uri) {
    size_t index = 0;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(
        server, UA_STRING((char*)(uintptr_t)uri), &index), UA_STATUSCODE_GOOD);
    return (UA_UInt16)index;
}

static void
checkGraph(UA_Server *server, UA_Boolean copied, UA_Boolean combined) {
    UA_UInt16 a = namespaceIndex(server, A_URI);
    UA_UInt16 b = namespaceIndex(server, B_URI);
    UA_NodeId aId = UA_NODEID_NUMERIC(a, 3001);
    UA_NodeId bId = UA_NODEID_NUMERIC(b, 3001);
    const UA_DataType *ta = UA_Server_findDataType(server, &aId);
    const UA_DataType *tb = UA_Server_findDataType(server, &bId);
    ck_assert_ptr_ne(ta, NULL);
    ck_assert_ptr_ne(tb, NULL);
    ck_assert_ptr_eq(tb->members[0].memberType, ta);
    ck_assert_uint_eq(ta->binaryEncodingId.namespaceIndex, a);
    ck_assert_uint_eq(ta->binaryEncodingId.identifier.numeric, 5001);
    ck_assert_uint_eq(ta->xmlEncodingId.namespaceIndex, a);
    ck_assert_uint_eq(ta->xmlEncodingId.identifier.numeric, 5002);
    ck_assert_uint_eq(tb->binaryEncodingId.namespaceIndex, b);
    if(!combined) {
        const UA_DataType *exported = &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA];
        if(copied)
            ck_assert_ptr_ne(ta, exported);
        else
            ck_assert_ptr_eq(ta, exported);
        ck_assert_ptr_eq(ta, namespace_runtime_a_generated_getDataType(
            server, &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA]));
    }
    UA_NodeId recursiveId = UA_NODEID_NUMERIC(a, 3011);
    const UA_DataType *recursive = UA_Server_findDataType(server, &recursiveId);
    ck_assert_ptr_ne(recursive, NULL);
    ck_assert_ptr_eq(recursive->members[1].memberType, recursive);
    UA_NodeId aliasId = UA_NODEID_NUMERIC(a, 3050);
    UA_NodeId holderId = UA_NODEID_NUMERIC(a, 3051);
    const UA_DataType *alias = UA_Server_findDataType(server, &aliasId);
    const UA_DataType *holder = UA_Server_findDataType(server, &holderId);
    ck_assert_ptr_ne(alias, NULL);
    ck_assert_ptr_ne(holder, NULL);
    ck_assert_ptr_eq(holder->members[1].memberType, alias);
    ck_assert_uint_eq(alias->memSize, ta->memSize);
    ck_assert_uint_eq(alias->membersSize, ta->membersSize);
    UA_NodeId flexibleId = UA_NODEID_NUMERIC(a, 3031);
    const UA_DataType *flexible = UA_Server_findDataType(server, &flexibleId);
    ck_assert_ptr_ne(flexible, NULL);
    ck_assert_ptr_eq(flexible->members[0].memberType, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    ck_assert_ptr_eq(flexible->members[1].memberType, &UA_TYPES[UA_TYPES_VARIANT]);
    UA_NodeId flagsId = UA_NODEID_NUMERIC(a, 3032);
    const UA_DataType *flags = UA_Server_findDataType(server, &flagsId);
    ck_assert_ptr_ne(flags, NULL);
    ck_assert_uint_eq(flags->typeKind, UA_DATATYPEKIND_STRUCTURE);
    ck_assert_uint_eq(flags->memSize, sizeof(UA_OptionSet));
    ck_assert_uint_eq(flags->membersSize, 2);
    ck_assert_ptr_eq(flags->members[0].memberType, &UA_TYPES[UA_TYPES_BYTESTRING]);
    ck_assert_ptr_eq(flags->members[1].memberType, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_NodeId stringId = UA_NODEID_STRING(a, "StringRecord");
    const UA_DataType *stringType = UA_Server_findDataType(server, &stringId);
    ck_assert_ptr_ne(stringType, NULL);
    UA_UInt16 encNs = namespaceIndex(server, "urn:open62541:test:encodings");
    ck_assert_uint_eq(stringType->binaryEncodingId.namespaceIndex, encNs);
    ck_assert_uint_eq(stringType->xmlEncodingId.namespaceIndex, encNs);
    UA_NodeId guidId = UA_NODEID_GUID(a, UA_GUID("00000001-0002-0003-0004-000000000005"));
    ck_assert_ptr_ne(UA_Server_findDataType(server, &guidId), NULL);
    UA_Byte opaqueBytes[] = {0, 1, 2, 255};
    UA_NodeId opaqueId;
    memset(&opaqueId, 0, sizeof(opaqueId));
    opaqueId.namespaceIndex = a;
    opaqueId.identifierType = UA_NODEIDTYPE_BYTESTRING;
    opaqueId.identifier.byteString.data = opaqueBytes;
    opaqueId.identifier.byteString.length = sizeof(opaqueBytes);
    ck_assert_ptr_ne(UA_Server_findDataType(server, &opaqueId), NULL);
    UA_NodeId escapedId = UA_NODEID_STRING(a, "semi;quote\"slash\\");
    ck_assert_ptr_ne(UA_Server_findDataType(server, &escapedId), NULL);
#ifdef UA_ENABLE_XML_ENCODING
    UA_Variant value;
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(b, 6001), &value),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(value.type, tb);
    const UA_RecordB *record = (const UA_RecordB*)value.data;
    ck_assert_uint_eq(record->child.number, 42);
    ck_assert_uint_eq(record->child.target.namespaceIndex, a);
    ck_assert_uint_eq(record->child.label.namespaceIndex, a);

    /* Real wire roundtrips must retain encoding IDs and server-owned types. */
    UA_ByteString binary = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeBinary(&value, &UA_TYPES[UA_TYPES_VARIANT], &binary, NULL),
                      UA_STATUSCODE_GOOD);
    UA_DecodeBinaryOptions binaryOptions;
    memset(&binaryOptions, 0, sizeof(binaryOptions));
    binaryOptions.customTypes = UA_Server_getDataTypes(server);
    UA_Variant decoded;
    ck_assert_uint_eq(UA_decodeBinary(&binary, &decoded, &UA_TYPES[UA_TYPES_VARIANT],
                                      &binaryOptions), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(decoded.type, tb);
    ck_assert(UA_order(value.data, decoded.data, tb) == UA_ORDER_EQ);
    UA_Variant_clear(&decoded);
    UA_ByteString_clear(&binary);
    UA_ByteString xml = UA_BYTESTRING_NULL;
    ck_assert_uint_eq(UA_encodeXml(&value, &UA_TYPES[UA_TYPES_VARIANT], &xml, NULL),
                      UA_STATUSCODE_GOOD);
    UA_DecodeXmlOptions options;
    memset(&options, 0, sizeof(options));
    options.customTypes = UA_Server_getDataTypes(server);
    ck_assert_uint_eq(UA_decodeXml(&xml, &decoded, &UA_TYPES[UA_TYPES_VARIANT], &options),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(decoded.type, tb);
    ck_assert(UA_order(value.data, decoded.data, tb) == UA_ORDER_EQ);
    UA_Variant_clear(&decoded);
    UA_ByteString_clear(&xml);
    UA_Variant_clear(&value);

    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(b, 6002), &value),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_NODEID]));
    ck_assert_uint_eq(((UA_NodeId*)value.data)->namespaceIndex, a);
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(b, 6003), &value),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]));
    ck_assert_uint_eq(((UA_ExpandedNodeId*)value.data)->nodeId.namespaceIndex, a);
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(b, 6004), &value),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(value.type, tb);
    record = (const UA_RecordB*)value.data;
    ck_assert_uint_eq(record->child.number, 42);
    ck_assert_uint_eq(record->child.target.namespaceIndex, a);
    ck_assert_uint_eq(record->child.label.namespaceIndex, a);
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(b, 6005), &value),
                      UA_STATUSCODE_GOOD);
    UA_String expected = UA_STRING("first\n  Grüße\n last");
    ck_assert(UA_String_equal((UA_String*)value.data, &expected));
    UA_Variant_clear(&value);
    ck_assert_uint_eq(UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(b, 6006), &value),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(value.arrayLength, 1);
    ck_assert_uint_eq(*(UA_UInt32*)value.data, 2);
    UA_Variant_clear(&value);
#endif
}

START_TEST(inPlaceBinding) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    UA_Server_addNamespace(server, B_URI);
    ck_assert_uint_eq(namespace_runtime_a_generated(server, false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_b_generated(server, false), UA_STATUSCODE_GOOD);
    checkGraph(server, false, false);
    const UA_DataTypeArray *head = UA_Server_getDataTypes(server);
    ck_assert_uint_eq(namespace_runtime_a_generated(server, false), UA_STATUSCODE_BADNODEIDEXISTS);
    ck_assert_ptr_eq(UA_Server_getDataTypes(server), head);
    UA_Server *second = UA_Server_newForUnitTest();
    UA_Server_addNamespace(second, A_URI);
    UA_Server_addNamespace(second, "urn:open62541:test:encodings");
    UA_Server_addNamespace(second, B_URI);
    ck_assert_uint_eq(namespace_runtime_a_generated(second, false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_b_generated(second, false), UA_STATUSCODE_GOOD);
    checkGraph(second, false, false);
    UA_Server_delete(second);
    checkGraph(server, false, false);
    UA_Server *wrong = UA_Server_newForUnitTest();
    UA_Server_addNamespace(wrong, "urn:unrelated");
    ck_assert_uint_eq(namespace_runtime_a_generated(wrong, false), UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert_ptr_eq(UA_Server_getDataTypes(wrong), NULL);
    UA_Server_delete(wrong);
    UA_Server_delete(server);
} END_TEST

START_TEST(unpinnedStaticRejected) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_GeneratedTypeSource source = sourceA;
    source.pinned = false;
    ck_assert_uint_eq(UA_GeneratedTypes_register(server, &source, 0, NULL, false),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert_ptr_eq(UA_Server_getDataTypes(server), NULL);
    UA_Server_delete(server);
} END_TEST

START_TEST(inconsistentPinnedPlan) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    UA_Server_addNamespace(server, B_URI);
    const UA_String uris[] = {UA_STRING_STATIC("http://opcfoundation.org/UA/"),
        UA_STRING_STATIC(""), UA_STRING_STATIC(A_URI),
        UA_STRING_STATIC("urn:open62541:test:encodings"),
        UA_STRING_STATIC(""), UA_STRING_STATIC(B_URI)};
    UA_GeneratedTypeSource source = sourceA;
    source.namespaceUris = uris;
    source.namespaceUrisSize = 6;
    ck_assert_uint_eq(UA_GeneratedTypes_register(server, &source, 0, NULL, false),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert_ptr_eq(UA_Server_getDataTypes(server), NULL);
    UA_Server_delete(server);
} END_TEST

START_TEST(copyIsolation) {
    UA_Server *first = UA_Server_newForUnitTest();
    UA_Server *second = UA_Server_newForUnitTest();
    UA_Server_addNamespace(second, B_URI);
    UA_Server_addNamespace(second, "urn:unrelated");
    ck_assert_uint_eq(namespace_runtime_a_generated(first, true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_b_generated(first, true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_a_generated(second, true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_b_generated(second, true), UA_STATUSCODE_GOOD);
    checkGraph(first, true, false);
    checkGraph(second, true, false);
    const UA_DataType *t1 = namespace_runtime_a_generated_getDataType(
        first, &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA]);
    const UA_DataType *t2 = namespace_runtime_a_generated_getDataType(
        second, &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA]);
    ck_assert_ptr_ne(t1, t2);
    ck_assert_uint_ne(t1->typeId.namespaceIndex, t2->typeId.namespaceIndex);
    UA_Server_delete(first);
    checkGraph(second, true, false);
    UA_Server_delete(second);
} END_TEST

START_TEST(inPlaceRejectsCopiedDependency) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_uint_eq(namespace_runtime_a_generated(server, true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_b_generated(server, false),
                      UA_STATUSCODE_BADCONFIGURATIONERROR);
    UA_NodeId bId = UA_NODEID_NUMERIC(namespaceIndex(server, B_URI), 3001);
    ck_assert_ptr_eq(UA_Server_findDataType(server, &bId), NULL);
    ck_assert_uint_eq(namespace_runtime_b_generated(server, true), UA_STATUSCODE_GOOD);
    checkGraph(server, true, false);
    UA_Server_delete(server);
} END_TEST

START_TEST(multipleFilesAndNamespaces) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, B_URI);
    UA_Server_addNamespace(server, "urn:unrelated");
    ck_assert_uint_eq(namespace_runtime_ab_generated(server, true), UA_STATUSCODE_GOOD);
    checkGraph(server, true, true);
    UA_Server_delete(server);
} END_TEST

START_TEST(staticDescriptorLookup) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    const UA_DataType *first = &UA_TYPES_RUNTIME_A[0];
    const UA_DataType *last = &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_COUNT - 1];
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(NULL, first), NULL);
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(server, NULL), NULL);
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(server, first), NULL);
    ck_assert_uint_eq(namespace_runtime_a_generated(server, true), UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(namespace_runtime_a_generated_getDataType(server, first), NULL);
    const UA_DataType *resolved = namespace_runtime_a_generated_getDataType(server, last);
    ck_assert_ptr_ne(resolved, NULL);
    ck_assert_ptr_ne(resolved, last);
    ck_assert_ptr_eq(resolved, UA_Server_findDataType(server, &resolved->typeId));
    /* Lookup identifies an array element by address, without reading its ID. */
    UA_DataType unrelated = *resolved;
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(server, &unrelated), NULL);
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(server, resolved), NULL);
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(server, UA_TYPES_RUNTIME_B), NULL);
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(
        server, UA_TYPES_RUNTIME_A + UA_TYPES_RUNTIME_A_COUNT), NULL);
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(server, &UA_TYPES[UA_TYPES_UINT32]), NULL);
    UA_Server_delete(server);
} END_TEST

START_TEST(registrationFailureAndRetry) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    UA_DataType origin[UA_TYPES_RUNTIME_A_COUNT];
    memcpy(origin, sourceA.types, sizeof(origin));
    UA_GeneratedTypeSource source = sourceA;
    source.types = origin;
    const UA_GeneratedTypeSource *dependencies[] = {&sourceA};
    /* The SDK retains completed types when a later dependency is missing. */
    UA_DataTypeMember member = origin[UA_TYPES_RUNTIME_A_STRINGRECORD].members[0];
    UA_DataType missing = UA_TYPES[UA_TYPES_UINT32];
    missing.typeId = UA_NODEID_NUMERIC(0, 999999);
    member.memberType = &missing;
    origin[UA_TYPES_RUNTIME_A_STRINGRECORD].members = &member;
    ck_assert_uint_eq(UA_GeneratedTypes_register(server, &source, 1, dependencies, true),
                      UA_STATUSCODE_BADNOTFOUND);
    const UA_DataType *first = namespace_runtime_a_generated_getDataType(server, &sourceA.types[0]);
    ck_assert_ptr_ne(first, NULL);
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(
        server, &sourceA.types[UA_TYPES_RUNTIME_A_STRINGRECORD]), NULL);
    memcpy(origin, sourceA.types, sizeof(origin));
    ck_assert_uint_eq(UA_GeneratedTypes_register(server, &source, 1, dependencies, true),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(first, namespace_runtime_a_generated_getDataType(server, &sourceA.types[0]));
    ck_assert_ptr_eq(UA_Server_getConfig(server)->customDataTypes, NULL);
    const UA_DataTypeArray *head = UA_Server_getDataTypes(server);
    ck_assert_uint_eq(UA_GeneratedTypes_register(server, &source, 1, dependencies, true),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(head, UA_Server_getDataTypes(server));
    ck_assert_uint_eq(UA_GeneratedTypes_register(server, &source, 1, dependencies, false),
                      UA_STATUSCODE_BADINVALIDSTATE);
    UA_Server_delete(server);
} END_TEST

START_TEST(partialRegistrationResumed) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    UA_DataType existing = sourceA.types[0];
    UA_DataTypeArray partial;
    memset(&partial, 0, sizeof(partial));
    partial.types = &existing;
    partial.typesSize = 1;
    UA_Server_getConfig(server)->customDataTypes = &partial;
    ck_assert_uint_eq(UA_GeneratedTypes_register(server, &sourceA, 0, NULL, true),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(UA_Server_getConfig(server)->customDataTypes, &partial);
    ck_assert_ptr_ne(namespace_runtime_a_generated_getDataType(
        server, &sourceA.types[UA_TYPES_RUNTIME_A_COUNT - 1]), NULL);
    UA_Server_delete(server);
} END_TEST

START_TEST(conversionOrderDiffersFromArrayOrder) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    UA_DataType reversed[UA_TYPES_RUNTIME_A_COUNT];
    size_t order[UA_TYPES_RUNTIME_A_COUNT];
    for(size_t i = 0; i < UA_TYPES_RUNTIME_A_COUNT; i++) {
        reversed[i] = sourceA.types[UA_TYPES_RUNTIME_A_COUNT - 1 - i];
        order[i] = UA_TYPES_RUNTIME_A_COUNT - 1 - sourceA.order[i];
    }
    UA_GeneratedTypeSource source = sourceA;
    source.types = reversed;
    source.order = order;
    const UA_GeneratedTypeSource *dependencies[] = {&sourceA};
    ck_assert_uint_eq(UA_GeneratedTypes_register(server, &source, 1, dependencies, true),
                      UA_STATUSCODE_GOOD);
    const UA_DataType *holder = UA_Server_findDataType(server, &sourceA.types[UA_TYPES_RUNTIME_A_ALIASHOLDER].typeId);
    const UA_DataType *alias = UA_Server_findDataType(server, &sourceA.types[UA_TYPES_RUNTIME_A_ALIASRECORD].typeId);
    ck_assert_ptr_ne(holder, NULL);
    ck_assert_ptr_ne(alias, NULL);
    ck_assert_ptr_eq(holder->members[1].memberType, alias);
    const UA_DataType *expected = &sourceA.types[UA_TYPES_RUNTIME_A_ALIASHOLDER];
    ck_assert_uint_eq(holder->memSize, sizeof(UA_AliasHolder));
    ck_assert_uint_eq(holder->members[1].padding, expected->members[1].padding);
    UA_Server_delete(server);
} END_TEST

static UA_StatusCode
rejectVariable(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void **nodeContext) {
    if(nodeId->identifierType == UA_NODEIDTYPE_NUMERIC &&
       nodeId->identifier.numeric == 6001)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    return UA_STATUSCODE_GOOD;
}

START_TEST(retryAfterNodeCreationFailure) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_GlobalNodeLifecycle lifecycle;
    memset(&lifecycle, 0, sizeof(lifecycle));
    lifecycle.earlyConstructor = rejectVariable;
    UA_Server_getConfig(server)->nodeLifecycle = &lifecycle;
    ck_assert_uint_eq(namespace_runtime_a_generated(server, true),
                      UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    UA_Server_getConfig(server)->nodeLifecycle = NULL;
    const UA_DataType *type = namespace_runtime_a_generated_getDataType(
        server, &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA]);
    ck_assert_ptr_ne(type, NULL);
    UA_NodeClass nodeClass;
    ck_assert_uint_ne(UA_Server_readNodeClass(server, type->typeId, &nodeClass),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_a_generated(server, true), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(type, namespace_runtime_a_generated_getDataType(
        server, &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA]));
    UA_Server_delete(server);
} END_TEST

#ifdef UA_TEST_GENERATED_TYPES_ALLOCATION_FAILURES
static size_t allocationIndex;
static size_t failingAllocation = (size_t)-1;
void *generatedTypesTestMalloc(size_t size) {
    return allocationIndex++ == failingAllocation ? NULL : malloc(size);
}
void *generatedTypesTestCalloc(size_t count, size_t size) {
    return allocationIndex++ == failingAllocation ? NULL : calloc(count, size);
}
void *generatedTypesTestRealloc(void *ptr, size_t size) {
    return allocationIndex++ == failingAllocation ? NULL : realloc(ptr, size);
}
START_TEST(staticAllocationFailure) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    allocationIndex = 0;
    failingAllocation = 0;
    UA_StatusCode res = UA_GeneratedTypes_register(
        server, &sourceA, 0, NULL, false);
    failingAllocation = (size_t)-1;
    ck_assert_uint_eq(res, UA_STATUSCODE_BADOUTOFMEMORY);
    ck_assert_ptr_eq(UA_Server_getDataTypes(server), NULL);
    ck_assert_uint_eq(UA_GeneratedTypes_register(
        server, &sourceA, 0, NULL, false), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(namespace_runtime_a_generated_getDataType(
        server, &UA_TYPES_RUNTIME_A[0]), &UA_TYPES_RUNTIME_A[0]);
    UA_Server_delete(server);
} END_TEST

START_TEST(allocationFailures) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    allocationIndex = 0;
    ck_assert_uint_eq(UA_GeneratedTypes_register(
        server, &sourceA, 0, NULL, true), UA_STATUSCODE_GOOD);
    size_t allocations = allocationIndex;
    UA_Server_delete(server);
    ck_assert_uint_gt(allocations, 10);
    size_t failure;
    for(failure = 0; failure < allocations; failure++) {
        server = UA_Server_newForUnitTest();
        UA_Server_addNamespace(server, A_URI);
        UA_Server_addNamespace(server, "urn:open62541:test:encodings");
        allocationIndex = 0;
        failingAllocation = failure;
        UA_StatusCode res = UA_GeneratedTypes_register(
            server, &sourceA, 0, NULL, true);
        failingAllocation = (size_t)-1;
        if(res != UA_STATUSCODE_GOOD) {
            const UA_DataType *first = namespace_runtime_a_generated_getDataType(server, &sourceA.types[0]);
            ck_assert_uint_eq(UA_GeneratedTypes_register(
                server, &sourceA, 0, NULL, true), UA_STATUSCODE_GOOD);
            if(first) ck_assert_ptr_eq(first, namespace_runtime_a_generated_getDataType(server, &sourceA.types[0]));
        }
        ck_assert_ptr_eq(UA_Server_getConfig(server)->customDataTypes, NULL);
        const UA_DataType *loaded = namespace_runtime_a_generated_getDataType(
            server, &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA]);
        ck_assert_ptr_ne(loaded, NULL);
        ck_assert_uint_eq(loaded->membersSize, 3);
#ifdef UA_ENABLE_TYPEDESCRIPTION
        ck_assert_str_eq(loaded->typeName, "RecordA");
        ck_assert_str_eq(loaded->members[0].memberName, "Number");
        ck_assert_str_eq(loaded->members[1].memberName, "Target");
        ck_assert_str_eq(loaded->members[2].memberName, "Label");
#endif
        UA_Server_delete(server);
    }
    const UA_DataType *original =
        &sourceA.types[UA_TYPES_RUNTIME_A_STRINGRECORD];
    for(failure = 0; failure < 100; failure++) {
        allocationIndex = 0;
        failingAllocation = failure;
        UA_DataType copy;
        UA_StatusCode res = UA_DataType_copy(original, &copy);
        failingAllocation = (size_t)-1;
        UA_DataType_clear(&copy);
        if(res == UA_STATUSCODE_GOOD)
            break;
    }
    ck_assert_uint_lt(failure, 100);
    ck_assert_uint_gt(failure, 2);
} END_TEST
#endif

int main(void) {
    Suite *suite = suite_create("Generated datatype ownership and namespaces");
    TCase *tc = tcase_create("Copy and in-place");
    tcase_add_test(tc, inPlaceBinding);
    tcase_add_test(tc, copyIsolation);
    tcase_add_test(tc, unpinnedStaticRejected);
    tcase_add_test(tc, inconsistentPinnedPlan);
    tcase_add_test(tc, inPlaceRejectsCopiedDependency);
    tcase_add_test(tc, multipleFilesAndNamespaces);
    tcase_add_test(tc, registrationFailureAndRetry);
    tcase_add_test(tc, partialRegistrationResumed);
    tcase_add_test(tc, conversionOrderDiffersFromArrayOrder);
    tcase_add_test(tc, staticDescriptorLookup);
    tcase_add_test(tc, retryAfterNodeCreationFailure);
#ifdef UA_TEST_GENERATED_TYPES_ALLOCATION_FAILURES
    tcase_set_timeout(tc, 180);
    tcase_add_test(tc, allocationFailures);
    tcase_add_test(tc, staticAllocationFailure);
#endif
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
