/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer) */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "nodeset_loader_test.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

/* The order follows the RequiredModel dependencies. Older and newer revisions
 * of the same model are deliberately both covered. */
static const char *const nodesetFiles[] = {
#include "companion_models.h"
};

static const size_t nodesetFilesSize = sizeof(nodesetFiles) / sizeof(nodesetFiles[0]);

static const UA_UInt32 bacnetDataTypeIds[] = {
    3046, 103002, 103039, 103040, 103032, 103034, 3068, 103031, 3050, 3051, 103017,
};

static const UA_UInt32 amlInheritedValueRankIds[] = {
    2, 3, 5, 6, 8, 9, 11, 12, 14, 15,
};

static const UA_UInt32 amlArgumentDescriptorIds[] = {
    46, 49, 67, 104, 113, 116, 119,
};

static const UA_UInt32 bacnetEnumValueIds[] = {
    6007,
    6056,
    6360,
};

static const UA_UInt32 powerlinkInheritedDataTypeIds[] = {
    14, 1491, 1506, 1521, 1536, 1551, 1566, 2152, 2182,
};

static const UA_UInt32 i4aasValueIds[] = {
    6003, 6002, 6004, 6005, 6112, 6113, 6076, 6077, 6079,
};

static UA_Server *
newServer(void) {
    UA_ServerConfig config;
    memset(&config, 0, sizeof(config));
    config.logging = UA_Log_Stdout_new(UA_LOGLEVEL_WARNING);
    if(!config.logging)
        return NULL;

    UA_StatusCode res = UA_ServerConfig_setMinimal(&config, 0, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clear(&config);
        return NULL;
    }
    return UA_Server_newWithConfig(&config);
}

static void
assertNodeExists(UA_Server *server, const char *namespaceUri, UA_UInt32 numericId) {
    size_t namespaceIndex;
    UA_StatusCode res = UA_Server_getNamespaceByName(
        server, UA_STRING((char *)(uintptr_t)namespaceUri), &namespaceIndex);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(namespaceIndex <= UA_UINT16_MAX);

    UA_NodeClass nodeClass;
    UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)namespaceIndex, numericId);
    res = UA_Server_readNodeClass(server, id, &nodeClass);
    ck_assert_msg(res == UA_STATUSCODE_GOOD, "Node %u in %s was not added", (unsigned)numericId,
                  namespaceUri);
}

static UA_NodeId
nodeIdForNamespace(UA_Server *server, const char *namespaceUri, UA_UInt32 numericId) {
    size_t namespaceIndex;
    UA_StatusCode res = UA_Server_getNamespaceByName(
        server, UA_STRING((char *)(uintptr_t)namespaceUri), &namespaceIndex);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(namespaceIndex <= UA_UINT16_MAX);
    return UA_NODEID_NUMERIC((UA_UInt16)namespaceIndex, numericId);
}

static void
assertArrayDimensions(UA_Server *server, const char *namespaceUri, UA_UInt32 numericId,
                      size_t expectedSize, const UA_UInt32 *expected) {
    UA_NodeId id = nodeIdForNamespace(server, namespaceUri, numericId);
    UA_Variant dimensions;
    UA_Variant_init(&dimensions);
    UA_StatusCode res = UA_Server_readArrayDimensions(server, id, &dimensions);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&dimensions, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(dimensions.arrayLength, expectedSize);
    for(size_t i = 0; i < expectedSize; i++)
        ck_assert_uint_eq(((UA_UInt32 *)dimensions.data)[i], expected[i]);
    UA_Variant_clear(&dimensions);
}

static void
assertUInt32ArrayValue(UA_Server *server, const char *namespaceUri, UA_UInt32 numericId) {
    UA_NodeId id = nodeIdForNamespace(server, namespaceUri, numericId);
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode res = UA_Server_readValue(server, id, &value);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(value.type, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(value.arrayLength, 1);
    ck_assert_uint_eq(*(UA_UInt32 *)value.data, 0);
    UA_Variant_clear(&value);
}

START_TEST(loadAllNodesetsIntoOneServer) {
    UA_Server *server = newServer();
    ck_assert_ptr_nonnull(server);

    UA_StatusCode res = UA_Server_run_startup(server);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    for(size_t i = 0; i < nodesetFilesSize; i++) {
        fprintf(stderr, "Loading NodeSet %lu/%lu: %s\n", (unsigned long)(i + 1),
                (unsigned long)nodesetFilesSize, nodesetFiles[i]);
        res = loadNodesetFile(server, nodesetFiles[i], NULL);
        ck_assert_msg(res == UA_STATUSCODE_GOOD, "Loading %s failed with %s", nodesetFiles[i],
                      UA_StatusCode_name(res));
    }

    size_t bacnetNamespace;
    res = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/BACnet_V2/"),
                                       &bacnetNamespace);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(bacnetNamespace <= UA_UINT16_MAX);
    for(size_t i = 0; i < sizeof(bacnetDataTypeIds) / sizeof(bacnetDataTypeIds[0]); i++) {
        UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)bacnetNamespace, bacnetDataTypeIds[i]);
        const UA_DataType *type = UA_Server_findDataType(server, &id);
        ck_assert_msg(type != NULL, "BACnet DataType %u was not registered",
                      (unsigned)bacnetDataTypeIds[i]);
        if(bacnetDataTypeIds[i] == 3046) {
            ck_assert_uint_eq(type->typeKind, UA_DATATYPEKIND_ENUM);
            ck_assert_uint_eq(type->membersSize, 387);
        }
    }

    assertNodeExists(server, "http://opcfoundation.org/UA/MachineVision/AMCM/", 5006);
    assertNodeExists(server, "http://opcfoundation.org/UA/RSL/", 6038);

    size_t amlNamespace;
    res = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/AMLLibs/"),
                                       &amlNamespace);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(amlNamespace <= UA_UINT16_MAX);
    for(size_t i = 0; i < sizeof(amlInheritedValueRankIds) / sizeof(amlInheritedValueRankIds[0]);
        i++) {
        UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)amlNamespace, amlInheritedValueRankIds[i]);
        UA_Int32 valueRank;
        res = UA_Server_readValueRank(server, id, &valueRank);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(valueRank, UA_VALUERANK_ANY);

        UA_Variant value;
        UA_Variant_init(&value);
        res = UA_Server_readValue(server, id, &value);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(value.arrayLength, 1);
        UA_Variant_clear(&value);
    }

    for(size_t i = 0; i < sizeof(amlArgumentDescriptorIds) / sizeof(amlArgumentDescriptorIds[0]);
        i++) {
        UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)amlNamespace, amlArgumentDescriptorIds[i]);
        UA_Variant value;
        UA_Variant_init(&value);
        res = UA_Server_readValue(server, id, &value);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert(UA_Variant_isScalar(&value));
        const UA_DataType *expectedType = &UA_TYPES[UA_TYPES_STRING];
        if(amlArgumentDescriptorIds[i] == 67)
            expectedType = &UA_TYPES[UA_TYPES_BOOLEAN];
        ck_assert_ptr_eq(value.type, expectedType);
        UA_Variant_clear(&value);
    }

    for(size_t i = 0; i < sizeof(bacnetEnumValueIds) / sizeof(bacnetEnumValueIds[0]); i++) {
        UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)bacnetNamespace, bacnetEnumValueIds[i]);
        UA_Variant value;
        UA_Variant_init(&value);
        res = UA_Server_readValue(server, id, &value);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert(UA_Variant_isScalar(&value));
        UA_NodeId enumTypeId = UA_NODEID_NUMERIC((UA_UInt16)bacnetNamespace, 3003);
        ck_assert(UA_NodeId_equal(&value.type->typeId, &enumTypeId));
        ck_assert_int_eq(*(UA_Int32 *)value.data, 0);
        UA_Variant_clear(&value);
    }

    size_t powerlinkNamespace;
    res = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/POWERLINK/"),
                                       &powerlinkNamespace);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(powerlinkNamespace <= UA_UINT16_MAX);
    for(size_t i = 0;
        i < sizeof(powerlinkInheritedDataTypeIds) / sizeof(powerlinkInheritedDataTypeIds[0]); i++) {
        UA_NodeId id =
            UA_NODEID_NUMERIC((UA_UInt16)powerlinkNamespace, powerlinkInheritedDataTypeIds[i]);
        UA_NodeId dataType;
        UA_NodeId_init(&dataType);
        res = UA_Server_readDataType(server, id, &dataType);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert(UA_NodeId_equal(&dataType, &UA_TYPES[UA_TYPES_BYTE].typeId));
        UA_NodeId_clear(&dataType);
    }

    static const UA_UInt32 expectedIOLinkDimensions[] = {0, 0};
    assertArrayDimensions(server, "http://opcfoundation.org/UA/IOLink/", 6146, 2,
                          expectedIOLinkDimensions);

    UA_NodeId i4aasType = nodeIdForNamespace(server, "http://opcfoundation.org/UA/I4AAS/", 3011);
    for(size_t i = 0; i < sizeof(i4aasValueIds) / sizeof(i4aasValueIds[0]); i++) {
        UA_NodeId id =
            nodeIdForNamespace(server, "http://opcfoundation.org/UA/I4AAS/", i4aasValueIds[i]);
        UA_Variant value;
        UA_Variant_init(&value);
        res = UA_Server_readValue(server, id, &value);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(value.arrayLength, 1);
        ck_assert_ptr_nonnull(value.type);
        ck_assert(UA_NodeId_equal(&value.type->typeId, &i4aasType));
        UA_Variant_clear(&value);
    }

    assertUInt32ArrayValue(server, "http://opcfoundation.org/UA/PADIM/", 6016);
    assertUInt32ArrayValue(server, "http://opcfoundation.org/UA/PADIM/", 6031);
    static const UA_UInt32 expected20[] = {20};
    static const UA_UInt32 expected6[] = {6};
    assertArrayDimensions(server, "http://opcfoundation.org/UA/FX/AC/", 6084, 1, expected20);
    assertArrayDimensions(server, "http://opcfoundation.org/UA/FX/AC/", 6162, 1, expected6);
    assertArrayDimensions(server, "http://opcfoundation.org/UA/FX/CM/", 6244, 1, expected6);

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}
END_TEST

int
main(void) {
    Suite *suite = suite_create("All Nodesets");
    TCase *tc = tcase_create("Load");
    tcase_set_timeout(tc, 1800);
    tcase_add_test(tc, loadAllNodesetsIntoOneServer);
    suite_add_tcase(suite, tc);

    SRunner *runner = srunner_create(suite);
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
