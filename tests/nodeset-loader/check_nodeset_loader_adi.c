/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "nodeset_loader_test.h"
#include <stdlib.h>

#include "check.h"
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
    UA_StatusCode retVal = loadNodesetFile(server,
        OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));
}
END_TEST

START_TEST(Server_loadAdiNodeset) {
    UA_StatusCode retVal = loadNodesetFile(server,
        OPEN62541_NODESET_DIR "ADI/Opc.Ua.Adi.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));

    /* ActiveBackground omits ValueRank but instantiates YArrayItemType. */
    size_t ns = 0;
    retVal = UA_Server_getNamespaceByName(
        server, UA_STRING("http://opcfoundation.org/UA/ADI/"), &ns);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_le(ns, UA_UINT16_MAX);
    UA_Int32 rank = -1;
    retVal = UA_Server_readValueRank(server, UA_NODEID_NUMERIC((UA_UInt16)ns, 10575), &rank);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(rank, 1);
}
END_TEST

START_TEST(Server_loadVariableRankDefaults) {
    UA_StatusCode res = loadNodesetFile(server,
        OPEN62541_TESTNODESET_DIR "variable_rank_defaults.xml", NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    size_t ns = 0;
    res = UA_Server_getNamespaceByName(
        server, UA_STRING("urn:open62541:test:variable-rank-defaults"), &ns);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_le(ns, UA_UINT16_MAX);
    const UA_Int32 expected[] = {1, 1, UA_VALUERANK_ANY, UA_VALUERANK_SCALAR, UA_VALUERANK_ANY};
    for(size_t i = 0; i < 5; i++) {
        UA_Int32 rank = 99;
        res = UA_Server_readValueRank(
            server, UA_NODEID_NUMERIC((UA_UInt16)ns, (UA_UInt32)(i + 1)), &rank);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(rank, expected[i]);
    }
    UA_Variant value;
    UA_Variant_init(&value);
    res = UA_Server_readValue(server, UA_NODEID_NUMERIC((UA_UInt16)ns, 2), &value);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_FLOAT]));
    ck_assert_uint_eq(value.arrayLength, 1);
    ck_assert(((UA_Float*)value.data)[0] == (UA_Float)1.5);
    UA_Variant_clear(&value);
    /* Explicitly incompatible ranks must still reject the node. */
    UA_NodeClass nodeClass;
    res = UA_Server_readNodeClass(server, UA_NODEID_NUMERIC((UA_UInt16)ns, 6), &nodeClass);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Loader");
    TCase *tc_server = tcase_create("Server DI and ADI nodeset");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_loadDiNodeset);
    tcase_add_test(tc_server, Server_loadAdiNodeset);
    tcase_add_test(tc_server, Server_loadVariableRankDefaults);
    suite_add_tcase(s, tc_server);
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
