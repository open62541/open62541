/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "testing_clock.h"
#include "tests/namespace_tests_testnodeset_generated.h"
#include "unistd.h"

UA_Server *server = NULL;
UA_DataTypeArray customTypesArray = { NULL, UA_TYPES_TESTS_TESTNODESET_COUNT, UA_TYPES_TESTS_TESTNODESET};

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    config->customDataTypes = &customTypesArray;
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_addTestNodeset) {
    UA_StatusCode retval = namespace_tests_testnodeset_generated(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(checkScalarValues) {
    UA_Variant out;
    UA_Variant_init(&out);
    // Point_scalar_Init
    UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 10002), &out);
    ck_assert(UA_Variant_isScalar(&out));
    UA_Point *p = (UA_Point *)out.data;
    ck_assert(p->x == (UA_Double)1.0);
    ck_assert(p->y == (UA_Double)2.0);
    UA_Variant_clear(&out);
    // Point_scalar_noInit
    UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 10005), &out);
    ck_assert(out.data == NULL);
    UA_Variant_clear(&out);
}
END_TEST

START_TEST(check1dimValues) {
    UA_Variant out;
    UA_Variant_init(&out);
    // Point_1dim_noInit
    UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 10007), &out);
    ck_assert(!UA_Variant_isScalar(&out));
    ck_assert(out.arrayDimensionsSize == 0);
    UA_Variant_clear(&out);
    // Point_1dim_init
    UA_Server_readValue(server, UA_NODEID_NUMERIC(2, 10004), &out);
    UA_Point *p = (UA_Point *)out.data;
    ck_assert(!UA_Variant_isScalar(&out));
    ck_assert(out.arrayLength == 4);
    ck_assert(p[0].x == (UA_Double)1.0);
    ck_assert(p[3].y == (UA_Double)8.0);
    UA_Variant_clear(&out);
}
END_TEST

START_TEST(readValueRank) {
    UA_Int32 rank;
    UA_Variant dims;
    // scalar
    UA_Server_readValueRank(server, UA_NODEID_NUMERIC(2, 10002), &rank);
    ck_assert_int_eq(rank, -1);
    UA_Server_readValueRank(server, UA_NODEID_NUMERIC(2, 10002), &rank);
    ck_assert_int_eq(rank, -1);
    UA_Variant_init(&dims);
    UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(2, 10002), &dims);
    ck_assert_int_eq(dims.arrayLength, 0);
    UA_Variant_clear(&dims);
    // 1-dim
    UA_Server_readValueRank(server, UA_NODEID_NUMERIC(2, 10007), &rank);
    ck_assert_int_eq(rank, 1);
    UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(2, 10007), &dims);
    ck_assert_int_eq(dims.arrayLength, 1);
    ck_assert_int_eq(*((UA_UInt32 *)dims.data), 0);
    UA_Variant_clear(&dims);
    UA_Server_readValueRank(server, UA_NODEID_NUMERIC(2, 10004), &rank);
    ck_assert_int_eq(rank, 1);
    UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(2, 10004), &dims);
    ck_assert_int_eq(dims.arrayLength, 1);
    ck_assert_int_eq(*((UA_UInt32 *)dims.data), 4);
    UA_Variant_clear(&dims);
    // 2-dim
    UA_Server_readValueRank(server, UA_NODEID_NUMERIC(2, 10006), &rank);
    ck_assert_int_eq(rank, 2);
    UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(2, 10006), &dims);
    ck_assert_int_eq(dims.arrayLength, 2);
    UA_UInt32 *dimensions = (UA_UInt32 *)dims.data;
    ck_assert_int_eq(dimensions[0], 2);
    ck_assert_int_eq(dimensions[1], 2);
    UA_Variant_clear(&dims);
}
END_TEST

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Compiler");
    TCase *tc_server = tcase_create("Server Testnodeset");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_addTestNodeset);
    tcase_add_test(tc_server, checkScalarValues);
    tcase_add_test(tc_server, check1dimValues);
    tcase_add_test(tc_server, readValueRank);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
