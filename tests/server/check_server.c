/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2019 (c) basysKom GmbH <opensource@basyskom.com> (Author: Frank Meerkötter)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(checkGetConfig) {
    ck_assert_ptr_eq(UA_Server_getConfig(NULL), NULL);
    ck_assert_ptr_ne(UA_Server_getConfig(server), NULL);
} END_TEST

START_TEST(checkGetNamespaceByName) {
    size_t notFoundIndex = 62541;
    UA_StatusCode notFound = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/invalid"), &notFoundIndex);
    ck_assert_uint_eq(notFoundIndex, 62541); // not changed
    ck_assert_uint_eq(notFound, UA_STATUSCODE_BADNOTFOUND);

    size_t foundIndex = 62541;
    UA_StatusCode found = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/"), &foundIndex);
    ck_assert_uint_eq(foundIndex, 0); // this namespace always has index 0 (defined by the standard)
    ck_assert_uint_eq(found, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkGetNamespaceById) {
    UA_String searchResultNamespace;
    UA_StatusCode notFound = UA_Server_getNamespaceByIndex(server, 10, &searchResultNamespace);
    ck_assert_uint_eq(notFound, UA_STATUSCODE_BADNOTFOUND);

    UA_StatusCode found1 = UA_Server_getNamespaceByIndex(server, 1, &searchResultNamespace);
    ck_assert_uint_eq(found1, UA_STATUSCODE_GOOD);
    UA_String_clear(&searchResultNamespace);

    UA_StatusCode notFound2 = UA_Server_getNamespaceByIndex(server, 2, &searchResultNamespace);
    ck_assert_uint_eq(notFound2, UA_STATUSCODE_BADNOTFOUND);

    UA_String compareNamespace = UA_STRING("http://opcfoundation.org/UA/");
    UA_StatusCode found = UA_Server_getNamespaceByIndex(server, 0, &searchResultNamespace);
    ck_assert(UA_String_equal(&compareNamespace, &searchResultNamespace));
    ck_assert_uint_eq(found, UA_STATUSCODE_GOOD);
    UA_String_clear(&searchResultNamespace);
} END_TEST

static void timedCallbackHandler(UA_Server *s, void *data) {
    *((UA_Boolean*)data) = false;  // stop the server via a timedCallback
}

START_TEST(checkServer_run) {
    UA_Boolean running = true;
    // 0 is in the past so the server will terminate on the first iteration
    UA_StatusCode ret;
    ret = UA_Server_addTimedCallback(server, &timedCallbackHandler, &running, 0, NULL);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ret = UA_Server_run(server, &running);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
} END_TEST

static UA_Server *
newServerWithMaxReferences(UA_UInt32 maxReferencesPerNode) {
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode res = UA_ServerConfig_setDefault(&config);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    config.maxReferencesPerNode = maxReferencesPerNode;
    return UA_Server_newWithConfig(&config);
}

/* Browsing is paginated for a low limit. The internal lookups during startup
 * and node instantiation must not run out of continuation points. */
START_TEST(checkLowMaxReferencesPerNode) {
    UA_UInt32 caps[] = {1, 2, 10};
    for(size_t i = 0; i < 3; i++) {
        UA_Server *s = newServerWithMaxReferences(caps[i]);
        ck_assert_ptr_ne(s, NULL);
        ck_assert_uint_eq(s->adminSession.availableContinuationPoints,
                          UA_MAXCONTINUATIONPOINTS);
        UA_Server_delete(s);
    }
} END_TEST

#ifdef UA_GENERATED_NAMESPACE_ZERO
START_TEST(checkInstantiateWithLowMaxReferencesPerNode) {
    UA_Server *s = newServerWithMaxReferences(1);
    ck_assert_ptr_ne(s, NULL);

    /* A type with several mandatory children */
    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("", "ManyChildrenType");
    UA_StatusCode res =
        UA_Server_addObjectTypeNode(s, UA_NODEID_NUMERIC(1, 6000),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                    UA_QUALIFIEDNAME(1, "ManyChildrenType"),
                                    otAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const size_t childCount = 4;
    for(UA_UInt32 i = 0; i < childCount; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Child%u", (unsigned)i);
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        oAttr.displayName = UA_LOCALIZEDTEXT("", name);
        res = UA_Server_addObjectNode(s, UA_NODEID_NUMERIC(1, 6001 + i),
                                      UA_NODEID_NUMERIC(1, 6000),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                      UA_QUALIFIEDNAME(1, name),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                      oAttr, NULL, NULL);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        res = UA_Server_addReference(s, UA_NODEID_NUMERIC(1, 6001 + i),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
                                     true);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    }

    /* Instantiate the type */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("", "Instance");
    UA_NodeId instanceId;
    res = UA_Server_addObjectNode(s, UA_NODEID_NULL,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  UA_QUALIFIEDNAME(1, "Instance"),
                                  UA_NODEID_NUMERIC(1, 6000), oAttr, NULL, &instanceId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* All children have been copied */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = instanceId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_NONE;
    UA_BrowseResult br = UA_Server_browse(s, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    size_t found = br.referencesSize;
    while(br.continuationPoint.length > 0) {
        UA_ByteString cp = br.continuationPoint;
        UA_ByteString_init(&br.continuationPoint);
        UA_BrowseResult_clear(&br);
        br = UA_Server_browseNext(s, false, &cp);
        ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
        found += br.referencesSize;
        UA_ByteString_clear(&cp);
    }
    UA_BrowseResult_clear(&br);
    ck_assert_uint_eq(found, childCount);

    /* No continuation point is leaked */
    ck_assert_uint_eq(s->adminSession.availableContinuationPoints,
                      UA_MAXCONTINUATIONPOINTS);

    UA_NodeId_clear(&instanceId);
    UA_Server_delete(s);
} END_TEST
#endif

int main(void) {
    Suite *s = suite_create("server");

    TCase *tc_call = tcase_create("server - basics");
    tcase_add_checked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, checkGetConfig);
    tcase_add_test(tc_call, checkGetNamespaceByName);
    tcase_add_test(tc_call, checkGetNamespaceById);
    tcase_add_test(tc_call, checkServer_run);
    tcase_add_test(tc_call, checkLowMaxReferencesPerNode);
#ifdef UA_GENERATED_NAMESPACE_ZERO
    tcase_add_test(tc_call, checkInstantiateWithLowMaxReferencesPerNode);
#endif
    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
