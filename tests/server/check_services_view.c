/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "thread_wrapper.h"

UA_Server *server_translate_browse;
UA_Boolean *running_translate_browse;
THREAD_HANDLE server_thread_translate_browse;

THREAD_CALLBACK(serverloop_register) {
    while (*running_translate_browse)
        UA_Server_run_iterate(server_translate_browse, true);
    return 0;
}

static void setup_server(void) {
    // start server
    running_translate_browse = UA_Boolean_new();
    *running_translate_browse = true;

    server_translate_browse = UA_Server_newForUnitTest();
    UA_ServerConfig *server_translate_config = UA_Server_getConfig(server_translate_browse);

    UA_String_clear(&server_translate_config->applicationDescription.applicationUri);
    server_translate_config->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.test.server_translate_browse");
    UA_Server_run_startup(server_translate_browse);
    THREAD_CREATE(server_thread_translate_browse, serverloop_register);
}

static void teardown_server(void) {
    *running_translate_browse = false;
    THREAD_JOIN(server_thread_translate_browse);
    UA_Server_run_shutdown(server_translate_browse);
    UA_Boolean_delete(running_translate_browse);
    UA_Server_delete(server_translate_browse);
}

START_TEST(Service_Browse_CheckSubTypes) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_NodeId hierarchRefs = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    UA_ReferenceTypeSet indices;
    UA_StatusCode res = referenceTypeIndices(server, &hierarchRefs, &indices, true);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Check that the subtypes are included */
    UA_assert(UA_ReferenceTypeSet_contains(&indices, UA_REFERENCETYPEINDEX_ORGANIZES));
    UA_assert(UA_ReferenceTypeSet_contains(&indices, UA_REFERENCETYPEINDEX_HASPROPERTY));
    UA_assert(UA_ReferenceTypeSet_contains(&indices, UA_REFERENCETYPEINDEX_HASCHILD));
    UA_assert(UA_ReferenceTypeSet_contains(&indices, UA_REFERENCETYPEINDEX_AGGREGATES));

    /* Check that the non-subtypes are not included */
    UA_assert(!UA_ReferenceTypeSet_contains(&indices, !UA_REFERENCETYPEINDEX_NONHIERARCHICALREFERENCES));

    UA_Server_delete(server);
}
END_TEST

static size_t
browseWithMaxResults(UA_Server *server, UA_NodeId nodeId, UA_UInt32 maxResults) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = nodeId;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    UA_BrowseResult br = UA_Server_browse(server, maxResults, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(br.referencesSize > 0);
    ck_assert(br.referencesSize <= maxResults);

    size_t total = br.referencesSize;
    UA_ByteString cp = br.continuationPoint;
    br.continuationPoint = UA_BYTESTRING_NULL;
    UA_BrowseResult_clear(&br);

    while(cp.length > 0) {
        br = UA_Server_browseNext(server, false, &cp);
        ck_assert(br.referencesSize > 0);
        ck_assert(br.referencesSize <= maxResults);
        UA_ByteString_clear(&cp);
        cp = br.continuationPoint;
        br.continuationPoint = UA_BYTESTRING_NULL;
        total += br.referencesSize;
        UA_BrowseResult_clear(&br);
    }

    return total;
}

START_TEST(Service_Browse_WithMaxResults) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);

    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(br.referencesSize > 0);

    size_t total = br.referencesSize;
    UA_BrowseResult_clear(&br);

    for(UA_UInt32 i = 1; i <= total; i++) {
        size_t sum_total =
            browseWithMaxResults(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), i);
        ck_assert_uint_eq(total, sum_total);
    }

    UA_Server_delete(server);
}
END_TEST

START_TEST(Service_Browse_WithBrowseName) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);

    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(br.referencesSize > 0);
    ck_assert(!UA_String_equal(&br.references[0].browseName.name, &UA_STRING_NULL));

    UA_BrowseResult_clear(&br);
    UA_Server_delete(server);
}
END_TEST

START_TEST(Service_Browse_ClassMask) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    /* add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res = UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                                                  parentReferenceNodeId, myIntegerName,
                                                  UA_NODEID_NULL, attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* browse only objects */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_OBJECT;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    size_t vars = br.referencesSize;

    for(size_t i = 0; i < br.referencesSize; i++) {
        UA_NodeClass cl = UA_NODECLASS_UNSPECIFIED;
        UA_Server_readNodeClass(server, br.references[i].nodeId.nodeId, &cl);
        UA_assert(cl == UA_NODECLASS_OBJECT);
    }
    UA_BrowseResult_clear(&br);

    /* browse only variables */
    bd.nodeClassMask = UA_NODECLASS_VARIABLE;
    br = UA_Server_browse(server, 0, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    size_t objs = br.referencesSize;

    for(size_t i = 0; i < br.referencesSize; i++) {
        UA_NodeClass cl = UA_NODECLASS_UNSPECIFIED;
        UA_Server_readNodeClass(server, br.references[i].nodeId.nodeId, &cl);
        UA_assert(cl == UA_NODECLASS_VARIABLE);
    }
    UA_BrowseResult_clear(&br);

    /* browse variables or objects */
    bd.nodeClassMask = UA_NODECLASS_VARIABLE | UA_NODECLASS_OBJECT;
    br = UA_Server_browse(server, 0, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(br.referencesSize, vars + objs);

    for(size_t i = 0; i < br.referencesSize; i++) {
        UA_NodeClass cl = UA_NODECLASS_UNSPECIFIED;
        UA_Server_readNodeClass(server, br.references[i].nodeId.nodeId, &cl);
        UA_assert(cl == UA_NODECLASS_VARIABLE || cl == UA_NODECLASS_OBJECT);
    }
    UA_BrowseResult_clear(&br);

    UA_Server_delete(server);
}
END_TEST

START_TEST(Service_Browse_ReferenceTypes) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    /* add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res = UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                                                  parentReferenceNodeId, myIntegerName,
                                                  UA_NODEID_NULL, attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* browse all */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);

    /* get the bitfield index of the hassubtype reference */
    UA_ReferenceTypeSet subTypeIdx;
    UA_NodeId hasSubtype = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    UA_NodeId hierarchTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    referenceTypeIndices(server, &hasSubtype, &subTypeIdx, false);

    /* count how many references are hierarchical */
    size_t hierarch_refs = 0;
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(isNodeInTree(server, &br.references[i].referenceTypeId,
                        &hierarchTypeId, &subTypeIdx))
            hierarch_refs += 1;
    }

    UA_BrowseResult_clear(&br);

    /* browse hierarchical */
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    bd.includeSubtypes = true;
    br = UA_Server_browse(server, 0, &bd);

    ck_assert_uint_eq(br.referencesSize, hierarch_refs);
    UA_BrowseResult_clear(&br);

    UA_Server_delete(server);
}
END_TEST

START_TEST(Service_Browse_Recursive) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    size_t resultSize = 0;
    UA_ExpandedNodeId *result = NULL;

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACEARRAY);
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    bd.includeSubtypes = true;
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    UA_StatusCode retval = UA_Server_browseRecursive(server, &bd, &resultSize, &result);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resultSize, 3);

    UA_NodeId expected[3];
    expected[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    expected[1] = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    expected[2] = UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER);

    for(size_t i = 0; i < resultSize; i++) {
        ck_assert(UA_NodeId_equal(&expected[i], &result[i].nodeId));
    }

    UA_Array_delete(result, resultSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    UA_Server_delete(server);
}
END_TEST

START_TEST(Service_Browse_Localization) {
    UA_Server *server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_NodeId outerObjectId = UA_NODEID_STRING(1, "EntryPoint");

    {
        UA_ObjectAttributes attr = UA_ObjectAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "EntryPoint");
        UA_QualifiedName browseName = UA_QUALIFIEDNAME(0, "EntryPoint");
        UA_StatusCode result = UA_Server_addObjectNode(server, outerObjectId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), browseName,
                                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), attr, NULL, NULL);
        ck_assert_int_eq(result, UA_STATUSCODE_GOOD);
    }

    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "MyDisplayName");
    UA_NodeId objectId = UA_NODEID_STRING(1, "LocalizedObject");
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(0, "LocalizedObject");
    UA_StatusCode result = UA_Server_addObjectNode(server, objectId, outerObjectId,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), browseName,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), attr, NULL, NULL);
    ck_assert_int_eq(result, UA_STATUSCODE_GOOD);

    UA_LocalizedText germanDisplayName = UA_LOCALIZEDTEXT("de-DE", "MeinAnzeigeName");
    result = UA_Server_writeDisplayName(server, objectId, germanDisplayName);
    ck_assert_int_eq(result, UA_STATUSCODE_GOOD);

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = outerObjectId;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.nodeClassMask = UA_NODECLASS_OBJECT;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);

    /* Expect the english display name */
    server->adminSession.localeIdsSize = 1;
    server->adminSession.localeIds = UA_LocaleId_new();
    *server->adminSession.localeIds = UA_STRING_ALLOC("en-US");

    UA_BrowseResult br = UA_Server_browse(server, 100, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(br.referencesSize, 1);
    ck_assert(UA_String_equal(&br.references->displayName.locale, &attr.displayName.locale));
    ck_assert(UA_String_equal(&br.references->displayName.text, &attr.displayName.text));
    UA_BrowseResult_clear(&br);

    /* Expect the german display name */
    UA_LocaleId_clear(server->adminSession.localeIds);
    *server->adminSession.localeIds = UA_STRING_ALLOC("de-DE");

    br = UA_Server_browse(server, 100, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(br.referencesSize, 1);
    ck_assert(UA_String_equal(&br.references->displayName.locale, &germanDisplayName.locale));
    ck_assert(UA_String_equal(&br.references->displayName.text, &germanDisplayName.text));
    UA_BrowseResult_clear(&br);

    UA_Server_delete(server);
}
END_TEST

START_TEST(ServiceTest_TranslateBrowsePathsToNodeIds) {
    UA_Client *client = UA_Client_newForUnitTest();
    UA_StatusCode retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    // Just for testing we want to translate the following path to its corresponding node id
    // /Objects/Server/ServerStatus/State
    // Equals the following node IDs:
    // /85/2253/2256/2259

#define BROWSE_PATHS_SIZE 3
    char *paths[BROWSE_PATHS_SIZE] = {"Server", "ServerStatus", "State"};
    UA_UInt32 ids[BROWSE_PATHS_SIZE] = {UA_NS0ID_ORGANIZES, UA_NS0ID_HASCOMPONENT, UA_NS0ID_HASCOMPONENT};
    UA_BrowsePath browsePath;
    UA_BrowsePath_init(&browsePath);
    browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    browsePath.relativePath.elements = (UA_RelativePathElement*)UA_Array_new(BROWSE_PATHS_SIZE, &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
    browsePath.relativePath.elementsSize = BROWSE_PATHS_SIZE;

    for(size_t i = 0; i < BROWSE_PATHS_SIZE; i++) {
        UA_RelativePathElement *elem = &browsePath.relativePath.elements[i];
        elem->referenceTypeId = UA_NODEID_NUMERIC(0, ids[i]);
        elem->targetName = UA_QUALIFIEDNAME_ALLOC(0, paths[i]);
    }

    UA_TranslateBrowsePathsToNodeIdsRequest request;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
    request.browsePaths = &browsePath;
    request.browsePathsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsResponse response = UA_Client_Service_translateBrowsePathsToNodeIds(client, request);

    ck_assert_int_eq(response.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(response.resultsSize, 1);

    ck_assert_uint_eq(response.results[0].targetsSize, 1);
    ck_assert_int_eq(response.results[0].targets[0].targetId.nodeId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(response.results[0].targets[0].targetId.nodeId.identifier.numeric, UA_NS0ID_SERVER_SERVERSTATUS_STATE);

    UA_BrowsePath_clear(&browsePath);
    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&response);
    retVal = UA_Client_disconnect(client);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
}
END_TEST

/* Force a hash collision for the the browsename by using all zeros.. */
START_TEST(Service_TranslateBrowsePathsWithHashCollision) {
    UA_Byte browseNames[4] = {0, 0, 0, 0};

    for(size_t i = 0; i < 3; i++) {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        UA_QualifiedName browseName;
        browseName.namespaceIndex = 0;
        browseName.name.data = browseNames;
        browseName.name.length = i+1;
        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        UA_StatusCode res =
            UA_Server_addVariableNode(server_translate_browse, UA_NODEID_NULL, parentNodeId,
                                      parentReferenceNodeId, browseName,
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                      attr, NULL, NULL);
        ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    }

    UA_BrowsePath browsePath;
    UA_BrowsePath_init(&browsePath);
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    browsePath.relativePath.elements = &rpe;
    browsePath.relativePath.elementsSize = 1;

    for(size_t i = 0; i < 3; i++) {
        rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        rpe.targetName.name.data = browseNames;
        rpe.targetName.name.length = i+1;

        UA_BrowsePathResult bpr =
            UA_Server_translateBrowsePathToNodeIds(server_translate_browse, &browsePath);

        ck_assert_int_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(bpr.targetsSize, 1);

        UA_BrowsePathResult_clear(&bpr);
    }
}
END_TEST

START_TEST(Service_TranslateBrowsePathsNoMatches) {
    UA_BrowsePath browsePath;
    UA_BrowsePath_init(&browsePath);
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    browsePath.relativePath.elements = &rpe;
    browsePath.relativePath.elementsSize = 1;

    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe.targetName = UA_QUALIFIEDNAME(0, "NoMatchToBeFound");

    UA_BrowsePathResult bpr =
        UA_Server_translateBrowsePathToNodeIds(server_translate_browse, &browsePath);

    ck_assert_int_eq(bpr.statusCode, UA_STATUSCODE_BADNOMATCH);
    ck_assert_uint_eq(bpr.targetsSize, 0);

    UA_BrowsePathResult_clear(&bpr);
}
END_TEST

START_TEST(BrowseSimplifiedBrowsePath) {
    UA_QualifiedName objectsName = UA_QUALIFIEDNAME(0, "Objects");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server_translate_browse,
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                                             1, &objectsName);

    ck_assert_uint_eq(bpr.targetsSize, 1);

    UA_BrowsePathResult_clear(&bpr);
}
END_TEST

static Suite *testSuite_Service_TranslateBrowsePathsToNodeIds(void) {
    Suite *s = suite_create("Service_TranslateBrowsePathsToNodeIds");
    TCase *tc_browse = tcase_create("Browse Service");
    tcase_add_test(tc_browse, Service_Browse_CheckSubTypes);
    tcase_add_test(tc_browse, Service_Browse_WithBrowseName);
    tcase_add_test(tc_browse, Service_Browse_ClassMask);
    tcase_add_test(tc_browse, Service_Browse_ReferenceTypes);
    tcase_add_test(tc_browse, Service_Browse_WithMaxResults);
    tcase_add_test(tc_browse, Service_Browse_Recursive);
    tcase_add_test(tc_browse, Service_Browse_Localization);
    suite_add_tcase(s, tc_browse);

    TCase *tc_translate = tcase_create("TranslateBrowsePathsToNodeIds");
    tcase_add_unchecked_fixture(tc_translate, setup_server, teardown_server);
    tcase_add_test(tc_translate, ServiceTest_TranslateBrowsePathsToNodeIds);
    tcase_add_test(tc_translate, Service_TranslateBrowsePathsWithHashCollision);
    tcase_add_test(tc_translate, Service_TranslateBrowsePathsNoMatches);
    tcase_add_test(tc_translate, BrowseSimplifiedBrowsePath);

    suite_add_tcase(s, tc_translate);

    return s;
}

int main(void) {
    int number_failed = 0;

    Suite *s;
    SRunner *sr;

    s = testSuite_Service_TranslateBrowsePathsToNodeIds();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


