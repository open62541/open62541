/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <server/ua_server_internal.h>

#include "check.h"
#include "ua_server.h"
#include "ua_config_default.h"
#include "ua_network_tcp.h"
#include "thread_wrapper.h"

UA_Server *server_translate_browse;
UA_ServerConfig *server_translate_config;
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
    server_translate_config = UA_ServerConfig_new_default();
    UA_String_deleteMembers(&server_translate_config->applicationDescription.applicationUri);
    server_translate_config->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.test.server_translate_browse");
    server_translate_browse = UA_Server_new(server_translate_config);
    UA_Server_run_startup(server_translate_browse);
    THREAD_CREATE(server_thread_translate_browse, serverloop_register);
}

static void teardown_server(void) {
    *running_translate_browse = false;
    THREAD_JOIN(server_thread_translate_browse);
    UA_Server_run_shutdown(server_translate_browse);
    UA_Boolean_delete(running_translate_browse);
    UA_Server_delete(server_translate_browse);
    UA_ServerConfig_delete(server_translate_config);
}

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

    size_t total = br.referencesSize;
    UA_ByteString cp = br.continuationPoint;
    br.continuationPoint = UA_BYTESTRING_NULL;
    UA_BrowseResult_deleteMembers(&br);

    while(cp.length > 0) {
        br = UA_Server_browseNext(server, false, &cp);
        ck_assert(br.referencesSize > 0);
        UA_ByteString_deleteMembers(&cp);
        cp = br.continuationPoint;
        br.continuationPoint = UA_BYTESTRING_NULL;
        total += br.referencesSize;
        UA_BrowseResult_deleteMembers(&br);
    }

    return total;
}

START_TEST(Service_Browse_WithMaxResults) {
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);

    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(br.referencesSize > 0);

    size_t total = br.referencesSize;
    UA_BrowseResult_deleteMembers(&br);

    for(UA_UInt32 i = 1; i <= total; i++) {
        size_t sum_total =
            browseWithMaxResults(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), i);
        ck_assert_int_eq(total, sum_total);
    }
    
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}
END_TEST

START_TEST(Service_Browse_WithBrowseName) {
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

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

    UA_BrowseResult_deleteMembers(&br);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}
END_TEST

START_TEST(Service_TranslateBrowsePathsToNodeIds) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);

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
    ck_assert_int_eq(response.resultsSize, 1);

    ck_assert_int_eq(response.results[0].targetsSize, 1);
    ck_assert_int_eq(response.results[0].targets[0].targetId.nodeId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(response.results[0].targets[0].targetId.nodeId.identifier.numeric, UA_NS0ID_SERVER_SERVERSTATUS_STATE);

    UA_BrowsePath_deleteMembers(&browsePath);
    UA_TranslateBrowsePathsToNodeIdsResponse_deleteMembers(&response);
    retVal = UA_Client_disconnect(client);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_Client_delete(client);
}
END_TEST

START_TEST(BrowseSimplifiedBrowsePath) {
    UA_QualifiedName objectsName = UA_QUALIFIEDNAME(0, "Objects");
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server_translate_browse,
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                                             1, &objectsName);

    ck_assert_int_eq(bpr.targetsSize, 1);

    UA_BrowsePathResult_deleteMembers(&bpr);
}
END_TEST

static Suite *testSuite_Service_TranslateBrowsePathsToNodeIds(void) {
    Suite *s = suite_create("Service_TranslateBrowsePathsToNodeIds");
    TCase *tc_browse = tcase_create("Browse Service");
    tcase_add_test(tc_browse, Service_Browse_WithBrowseName);
    tcase_add_test(tc_browse, Service_Browse_WithMaxResults);
    suite_add_tcase(s, tc_browse);

    TCase *tc_translate = tcase_create("TranslateBrowsePathsToNodeIds");
    tcase_add_unchecked_fixture(tc_translate, setup_server, teardown_server);
    tcase_add_test(tc_translate, Service_TranslateBrowsePathsToNodeIds);
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


