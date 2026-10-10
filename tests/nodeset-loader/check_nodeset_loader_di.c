/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodesetloader.h>

#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "testing_clock.h"
#include "test_helpers.h"

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_loadDiNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadDiNodeset_browseNameNamespace) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));

    size_t diIndex = 0;
    retVal = UA_Server_getNamespaceByName(server,
        UA_STRING("http://opcfoundation.org/UA/DI/"), &diIndex);
    ck_assert(UA_StatusCode_isGood(retVal));
    ck_assert_uint_ne(diIndex, 1);

    /* DeviceSet (i=5001) carries the BrowseName 1:DeviceSet in the nodeset */
    UA_NodeId deviceSet = UA_NODEID_NUMERIC((UA_UInt16)diIndex, 5001);
    UA_QualifiedName browseName;
    retVal = UA_Server_readBrowseName(server, deviceSet, &browseName);
    ck_assert(UA_StatusCode_isGood(retVal));
    UA_QualifiedName expected = UA_QUALIFIEDNAME((UA_UInt16)diIndex, "DeviceSet");
    ck_assert(UA_QualifiedName_equal(&browseName, &expected));
    UA_QualifiedName_clear(&browseName);

    /* Root -> 0:Objects -> <di>:DeviceSet */
    UA_RelativePathElement rpe[2];
    memset(rpe, 0, sizeof(rpe));
    rpe[0].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    rpe[0].includeSubtypes = true;
    rpe[0].targetName = UA_QUALIFIEDNAME(0, "Objects");
    rpe[1].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    rpe[1].includeSubtypes = true;
    rpe[1].targetName = expected;

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER);
    bp.relativePath.elements = rpe;
    bp.relativePath.elementsSize = 2;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bpr.targetsSize, 1);
    ck_assert(UA_NodeId_equal(&bpr.targets[0].targetId.nodeId, &deviceSet));
    UA_BrowsePathResult_clear(&bpr);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Loader");
    TCase *tc_server = tcase_create("Server DI nodeset");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_loadDiNodeset);
    suite_add_tcase(s, tc_server);
    TCase *tc_ns = tcase_create("Server DI namespace mapping");
    tcase_add_checked_fixture(tc_ns, setup, teardown);
    tcase_add_test(tc_ns, Server_loadDiNodeset_browseNameNamespace);
    suite_add_tcase(s, tc_ns);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
