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
#include <open62541/server_config_default.h>
#include <check.h>
#include <stdio.h>

UA_StatusCode namespace_runtime_ab_generated(UA_Server *server, UA_Boolean copy_types);

UA_StatusCode namespace_runtime_unpinned_generated(UA_Server *server, UA_Boolean copy_types);
UA_StatusCode namespace_runtime_unpinned_b_generated(UA_Server *server, UA_Boolean copy_types);

#define A_URI "urn:open62541:test:types-a"
#define B_URI "urn:open62541:test:types-b"

/* Private loader tests construct the same source-to-server map as the loader. */
static UA_StatusCode
loadTypesA(UA_Server *server, UA_Boolean copy) {
    if(!copy)
        return namespace_runtime_a_generated_loadTypes(server, false, NULL);
    UA_UInt16 indexes[sizeof(namespace_runtime_a_generated_namespaceUris) / sizeof(UA_String)] = {0};
    for(size_t i = 0; i < sizeof(indexes) / sizeof(UA_UInt16); i++) {
        if(!namespace_runtime_a_generated_namespaceUris[i].length) continue;
        size_t index;
        UA_StatusCode res = UA_Server_getNamespaceByName(server, namespace_runtime_a_generated_namespaceUris[i], &index);
        if(res) return res;
        indexes[i] = (UA_UInt16)index;
    }
    UA_NamespaceMapping mapping = {0};
    mapping.local2remote = indexes;
    mapping.local2remoteSize = sizeof(indexes) / sizeof(UA_UInt16);
    return namespace_runtime_a_generated_loadTypes(server, true, &mapping);
}

static UA_UInt16
namespaceIndex(UA_Server *server, const char *uri) {
    size_t index = 0;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(
        server, UA_STRING((char*)(uintptr_t)uri), &index), UA_STATUSCODE_GOOD);
    return (UA_UInt16)index;
}

static const UA_DataType *
findTypeA(UA_Server *server, UA_NodeId id) {
    id.namespaceIndex = namespaceIndex(server, A_URI);
    return UA_Server_findDataType(server, &id);
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
    ck_assert_uint_eq(holder->memSize, sizeof(UA_AliasHolder));
    ck_assert_uint_eq(holder->members[1].padding,
                      UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_ALIASHOLDER].members[1].padding);
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
    ck_assert_uint_eq(loadTypesA(server, false), UA_STATUSCODE_BADNODEIDEXISTS);
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
    UA_Server_delete(server);
} END_TEST

START_TEST(unpinnedStaticWithMatchingIndexes) {
    UA_ServerConfig config;
    memset(&config, 0, sizeof(config));
    ck_assert_uint_eq(UA_ServerConfig_setMinimal(&config, 0, NULL), UA_STATUSCODE_GOOD);
    UA_String_clear(&config.applicationDescription.applicationUri);
    config.applicationDescription.applicationUri = UA_STRING_ALLOC(A_URI);
    UA_Server *server = UA_Server_newWithConfig(&config);
    ck_assert_ptr_ne(server, NULL);
    ck_assert_uint_eq(namespaceIndex(server, A_URI), 1);
    ck_assert_uint_eq(namespace_runtime_unpinned_generated(server, false), UA_STATUSCODE_GOOD);
    UA_NodeId id = UA_NODEID_NUMERIC(1, 3001);
    const UA_DataType *type = UA_Server_findDataType(server, &id);
    ck_assert_ptr_ne(type, NULL);
    ck_assert_ptr_ne(UA_Server_getDataTypes(server), NULL);
    UA_DataTypeArray *entry = UA_Server_getConfig(server)->customDataTypes;
    ck_assert_ptr_ne(entry, NULL);
    ck_assert(entry->cleanup && entry->borrowed);
    ck_assert_ptr_eq(UA_findDataTypeWithCustom(&id, entry), type);
#ifdef UA_ENABLE_XML_ENCODING
    UA_Variant value;
    ck_assert_uint_eq(UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 6001), &value),
                      UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(value.type, type);
    ck_assert_uint_eq(((UA_RecordA*)value.data)->number, 42);
    UA_Variant_clear(&value);
#endif
    UA_Server_delete(server);
} END_TEST

START_TEST(staticUsesBakedIndexes) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, "urn:unrelated");
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    ck_assert_uint_eq(namespaceIndex(server, A_URI), 3);
    /* Registration trusts the baked indexes even with no mapping supplied. */
    ck_assert_uint_eq(loadTypesA(server, false), UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < UA_TYPES_RUNTIME_A_COUNT; i++) {
        const UA_DataType *type = &UA_TYPES_RUNTIME_A[i];
        ck_assert_uint_eq(type->typeId.namespaceIndex, 2);
        ck_assert_ptr_eq(UA_Server_findDataType(server, &type->typeId), type);
    }
    UA_Server_delete(server);
} END_TEST

START_TEST(copyUsesNodeNamespaceMapping) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, "urn:unrelated");
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    /* Reuse the node loader's map without a destination URI table. */
    UA_UInt16 indexes[] = {0, 0, 3, 4};
    UA_NamespaceMapping mapping = {0};
    mapping.local2remote = indexes;
    mapping.local2remoteSize = sizeof(indexes) / sizeof(UA_UInt16);
    const UA_DataType *original = &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_STRINGRECORD];
    ck_assert_uint_eq(namespace_runtime_a_generated_copyType(server, original, &mapping), UA_STATUSCODE_GOOD);
    const UA_DataType *loaded = findTypeA(server, original->typeId);
    ck_assert_ptr_ne(loaded, NULL);
    ck_assert_uint_eq(loaded->typeId.namespaceIndex, 3);
    ck_assert_uint_eq(loaded->binaryEncodingId.namespaceIndex, 4);
    ck_assert_uint_eq(loaded->xmlEncodingId.namespaceIndex, 4);
    ck_assert_uint_eq(namespace_runtime_a_generated_copyType(server, original, &mapping), UA_STATUSCODE_BADNODEIDEXISTS);
    ck_assert_ptr_eq(findTypeA(server, original->typeId), loaded);
    UA_Server_delete(server);
} END_TEST

START_TEST(crossArraySharedPlan) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, B_URI);
    UA_Server_addNamespace(server, "urn:unrelated");
    /* Batch-generated arrays share source indexes; server indexes still differ. */
    ck_assert_uint_eq(namespace_runtime_unpinned_generated(server, true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_unpinned_b_generated(server, true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespaceIndex(server, A_URI), 4);
    ck_assert_uint_eq(namespaceIndex(server, B_URI), 2);
    checkGraph(server, true, true);
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
    const UA_DataType *t1 = findTypeA(first, UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA].typeId);
    const UA_DataType *t2 = findTypeA(second, UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA].typeId);
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

START_TEST(registrationFailureAndRetry) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    UA_Server_addNamespace(server, B_URI);
    ck_assert_uint_eq(namespace_runtime_b_generated(server, true), UA_STATUSCODE_BADNOTFOUND);
    UA_NodeId bId = UA_NODEID_NUMERIC(namespaceIndex(server, B_URI), 3001);
    ck_assert_ptr_eq(UA_Server_findDataType(server, &bId), NULL);
    ck_assert_uint_eq(namespace_runtime_a_generated(server, true), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_b_generated(server, true), UA_STATUSCODE_GOOD);
    checkGraph(server, true, false);
    const UA_DataTypeArray *head = UA_Server_getDataTypes(server);
    ck_assert_uint_eq(loadTypesA(server, true), UA_STATUSCODE_BADNODEIDEXISTS);
    ck_assert_ptr_eq(head, UA_Server_getDataTypes(server));
    ck_assert_uint_eq(loadTypesA(server, false), UA_STATUSCODE_BADNODEIDEXISTS);
    UA_Server_delete(server);
} END_TEST

/* Check both modes with an independently registered type at either end of
 * the conversion order. In particular, checking only the first type is unsafe. */
START_TEST(partialRegistrationRejected) {
    UA_Boolean copy = _i / 2;
    size_t index = UA_TYPES_RUNTIME_A_ORDER[
        (_i % 2) ? UA_TYPES_RUNTIME_A_COUNT - 1 : 0];
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    UA_DataType existing = UA_TYPES_RUNTIME_A[index];
    UA_DataTypeArray partial = {NULL, 1, &existing, false, false};
    UA_Server_getConfig(server)->customDataTypes = &partial;
    ck_assert_uint_eq(loadTypesA(server, copy), UA_STATUSCODE_BADNODEIDEXISTS);
    ck_assert_ptr_eq(UA_Server_getConfig(server)->customDataTypes, &partial);
    ck_assert_ptr_eq(findTypeA(server, existing.typeId), &existing);
    if(!copy) {
        size_t other = UA_TYPES_RUNTIME_A_ORDER[(_i % 2) ? 0 : UA_TYPES_RUNTIME_A_COUNT - 1];
        ck_assert_ptr_eq(findTypeA(server, UA_TYPES_RUNTIME_A[other].typeId), NULL);
    }
    UA_Server_delete(server);
} END_TEST

START_TEST(conversionOrderDiffersFromArrayOrder) {
    ck_assert_uint_lt(UA_TYPES_RUNTIME_A_FORWARDCONTAINER, UA_TYPES_RUNTIME_A_LATERRECORD);
    size_t containerOrder = 0, recordOrder = 0;
    for(size_t i = 0; i < UA_TYPES_RUNTIME_A_COUNT; i++) {
        if(UA_TYPES_RUNTIME_A_ORDER[i] == UA_TYPES_RUNTIME_A_FORWARDCONTAINER) containerOrder = i;
        if(UA_TYPES_RUNTIME_A_ORDER[i] == UA_TYPES_RUNTIME_A_LATERRECORD) recordOrder = i;
    }
    ck_assert_uint_lt(recordOrder, containerOrder);
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    ck_assert_uint_eq(loadTypesA(server, true), UA_STATUSCODE_GOOD);
    const UA_DataType *container = findTypeA(server, UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_FORWARDCONTAINER].typeId);
    const UA_DataType *record = findTypeA(server, UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_LATERRECORD].typeId);
    ck_assert_ptr_ne(container, NULL);
    ck_assert_ptr_ne(record, NULL);
    ck_assert_ptr_eq(container->members[0].memberType, record);
    ck_assert_ptr_eq(UA_Server_getConfig(server)->customDataTypes, NULL);
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

START_TEST(duplicatesAfterNodeCreationFailure) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_GlobalNodeLifecycle lifecycle;
    memset(&lifecycle, 0, sizeof(lifecycle));
    lifecycle.earlyConstructor = rejectVariable;
    UA_Server_getConfig(server)->nodeLifecycle = &lifecycle;
    ck_assert_uint_eq(namespace_runtime_a_generated(server, true),
                      UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    UA_Server_getConfig(server)->nodeLifecycle = NULL;
    const UA_DataType *type = findTypeA(server, UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA].typeId);
    ck_assert_ptr_ne(type, NULL);
    UA_NodeClass nodeClass;
    ck_assert_uint_ne(UA_Server_readNodeClass(server, type->typeId, &nodeClass),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namespace_runtime_a_generated(server, true), UA_STATUSCODE_BADNODEIDEXISTS);
    ck_assert_ptr_eq(type, findTypeA(server, UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA].typeId));
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
    UA_StatusCode res = loadTypesA(server, false);
    failingAllocation = (size_t)-1;
    ck_assert_uint_eq(res, UA_STATUSCODE_BADOUTOFMEMORY);
    ck_assert_ptr_eq(UA_Server_getDataTypes(server), NULL);
    ck_assert_uint_eq(loadTypesA(server, false), UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(findTypeA(server, UA_TYPES_RUNTIME_A[0].typeId), &UA_TYPES_RUNTIME_A[0]);
    UA_Server_delete(server);
} END_TEST

START_TEST(allocationFailures) {
    UA_Server *server = UA_Server_newForUnitTest();
    UA_Server_addNamespace(server, A_URI);
    UA_Server_addNamespace(server, "urn:open62541:test:encodings");
    allocationIndex = 0;
    ck_assert_uint_eq(loadTypesA(server, true), UA_STATUSCODE_GOOD);
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
        UA_StatusCode res = loadTypesA(server, true);
        failingAllocation = (size_t)-1;
        if(res != UA_STATUSCODE_GOOD) {
            const UA_DataType *first = findTypeA(
                server, UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_ORDER[0]].typeId);
            ck_assert_uint_eq(loadTypesA(server, true),
                              first ? UA_STATUSCODE_BADNODEIDEXISTS : UA_STATUSCODE_GOOD);
            if(first) {
                ck_assert_ptr_eq(first, findTypeA(server, first->typeId));
                UA_Server_delete(server);
                continue;
            }
        }
        ck_assert_ptr_eq(UA_Server_getConfig(server)->customDataTypes, NULL);
        const UA_DataType *loaded = findTypeA(server, UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_RECORDA].typeId);
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
        &UA_TYPES_RUNTIME_A[UA_TYPES_RUNTIME_A_STRINGRECORD];
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
    tcase_add_test(tc, copyUsesNodeNamespaceMapping);
    tcase_add_test(tc, crossArraySharedPlan);
    tcase_add_test(tc, copyIsolation);
    tcase_add_test(tc, unpinnedStaticWithMatchingIndexes);
    tcase_add_test(tc, staticUsesBakedIndexes);
    tcase_add_test(tc, inPlaceRejectsCopiedDependency);
    tcase_add_test(tc, multipleFilesAndNamespaces);
    tcase_add_test(tc, registrationFailureAndRetry);
    tcase_add_loop_test(tc, partialRegistrationRejected, 0, 4);
    tcase_add_test(tc, conversionOrderDiffersFromArrayOrder);
    tcase_add_test(tc, duplicatesAfterNodeCreationFailure);
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
