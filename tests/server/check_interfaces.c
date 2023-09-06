/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2021 (c) basysKom GmbH <opensource@basyskom.com>
 */

#include <namespace_tests_interfaces_generated.h>

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "test_helpers.h"
#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_StatusCode setupResult = namespace_tests_interfaces_generated(server);
    ck_assert_int_eq(setupResult, UA_STATUSCODE_GOOD);

    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    setupResult = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 1234), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, (char *)"TestObject"),
                                          UA_NODEID_NUMERIC(2, 1006), attr, NULL, NULL);

    ck_assert_int_eq(setupResult, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(check_interface_instantiation) {
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(1, 1234);
    bp.relativePath.elementsSize = 1;

    UA_RelativePathElement el;
    UA_RelativePathElement_init(&el);
    el.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    el.targetName = UA_QUALIFIEDNAME(2, (char *)"ObjectWithInterfaces");

    bp.relativePath.elements = &el;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_int_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bpr.targetsSize, 1);

    const UA_NodeId objectWithInterfacesId = bpr.targets->targetId.nodeId;
    UA_BrowsePathResult_clear(&bpr);

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);

    bd.nodeId = objectWithInterfacesId;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES);
    bd.includeSubtypes = UA_TRUE;
    bd.nodeClassMask = 0xFFFFFFFF;

    UA_BrowseResult br = UA_Server_browse(server, 1000, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(br.referencesSize, 0);

    bool found = false;
    for (size_t i = 0; i < br.referencesSize; ++i) {
        UA_NodeId expectedRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASINTERFACE);
        UA_NodeId expectedId = UA_NODEID_NUMERIC(2, 1002);
        if (UA_NodeId_equal(&br.references[i].referenceTypeId, &expectedRef) &&
                UA_NodeId_equal(&br.references[i].nodeId.nodeId, &expectedId))
        {
            found = true;
            break;
        }
    }
    ck_assert(found == true);

    found = false;
    for (size_t i = 0; i < br.referencesSize; ++i) {
        UA_NodeId expectedRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASINTERFACE);
        UA_NodeId expectedId = UA_NODEID_NUMERIC(2, 1005);
        if (UA_NodeId_equal(&br.references[i].referenceTypeId, &expectedRef) &&
                UA_NodeId_equal(&br.references[i].nodeId.nodeId, &expectedId))
        {
            found = true;
            break;
        }
    }
    ck_assert(found == true);

    found = false;
    for (size_t i = 0; i < br.referencesSize; ++i) {
        UA_NodeId expectedRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
        UA_QualifiedName expectedName = UA_QUALIFIEDNAME(2, (char *)"Interface1Property");
        if (UA_NodeId_equal(&br.references[i].referenceTypeId, &expectedRef) &&
                UA_QualifiedName_equal(&br.references[i].browseName, &expectedName))
        {
            found = true;
            break;
        }
    }
    ck_assert(found == true);

    found = false;
    for (size_t i = 0; i < br.referencesSize; ++i) {
        UA_NodeId expectedRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
        UA_QualifiedName expectedName = UA_QUALIFIEDNAME(2, (char *)"MyOtherInterfaceProperty");
        if (UA_NodeId_equal(&br.references[i].referenceTypeId, &expectedRef) &&
                UA_QualifiedName_equal(&br.references[i].browseName, &expectedName))
        {
            found = true;
            break;
        }
    }
    ck_assert(found == true);

    found = false;
    for (size_t i = 0; i < br.referencesSize; ++i) {
        UA_NodeId expectedRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
        UA_QualifiedName expectedName = UA_QUALIFIEDNAME(2, (char *)"MyTestObjectProperty");
        if (UA_NodeId_equal(&br.references[i].referenceTypeId, &expectedRef) &&
                UA_QualifiedName_equal(&br.references[i].browseName, &expectedName))
        {
            found = true;
            break;
        }
    }
    ck_assert(found == true);

    UA_BrowseResult_clear(&br);

    UA_RelativePathElement_init(&el);
    el.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    el.targetName = UA_QUALIFIEDNAME(2, (char *)"BaseObjectWithInterface");

    bp.relativePath.elements = &el;

    bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_int_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bpr.targetsSize, 1);

    const UA_NodeId baseObjectWithInterfaceId = bpr.targets->targetId.nodeId;
    UA_BrowsePathResult_clear(&bpr);

    bd.nodeId = baseObjectWithInterfaceId;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFERENCES);
    bd.includeSubtypes = UA_TRUE;
    bd.nodeClassMask = 0xFFFFFFFF;

    br = UA_Server_browse(server, 1000, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(br.referencesSize, 0);

    found = false;
    for (size_t i = 0; i < br.referencesSize; ++i) {
        UA_NodeId expectedRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASINTERFACE);
        UA_NodeId expectedId = UA_NODEID_NUMERIC(2, 1005);
        if (UA_NodeId_equal(&br.references[i].referenceTypeId, &expectedRef) &&
                UA_NodeId_equal(&br.references[i].nodeId.nodeId, &expectedId))
        {
            found = true;
            break;
        }
    }
    ck_assert(found == true);

    found = false;
    for (size_t i = 0; i < br.referencesSize; ++i) {
        UA_NodeId expectedRef = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
        UA_QualifiedName expectedName = UA_QUALIFIEDNAME(2, (char *)"MyOtherInterfaceProperty");
        if (UA_NodeId_equal(&br.references[i].referenceTypeId, &expectedRef) &&
                UA_QualifiedName_equal(&br.references[i].browseName, &expectedName))
        {
            found = true;
            break;
        }
    }
    ck_assert(found == true);

} END_TEST

int main(void) {
    Suite *s = suite_create("server");

    TCase *tc_call = tcase_create("server - basics");
    tcase_add_checked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, check_interface_instantiation);
    suite_add_tcase(s, tc_call);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
