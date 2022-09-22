/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* This test, if it needs to be exactly like within nodeset-compiler
 * submodule, the nodeset loader still lacks exposing of custom data
 * type in form similar to `UA_TYPES_DI[UA_TYPES_DI_COUNT]`. The
 * following tests are missing:
 * 
 *      - checkScalarValues
 *      - checkSelfContainingUnion
 *      - check1dimValues
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodesetloader_default.h>
#include <open62541/types.h>
#include <string.h>

#include "check.h"
#include "testing_clock.h"
#include "unistd.h"

UA_Server *server = NULL;
UA_UInt16 testNamespaceIndex = (UA_UInt16) -1;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_Server_run_startup(server);
    UA_NodesetLoader_Init(server);
}

static void teardown(void) {
    UA_NodesetLoader_Delete(server);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_loadTestNodeset) {
    bool retVal = UA_NodesetLoader_LoadNodeset(server,
        OPEN62541_TESTNODESET_DIR "testnodeset.xml");
    ck_assert_uint_eq(retVal, true);
    size_t nsIndex = (size_t) -1;
    UA_Server_getNamespaceByName(server, UA_STRING("http://yourorganisation.org/test/"), &nsIndex);
    ck_assert(nsIndex != (size_t)-1);
    testNamespaceIndex = (UA_UInt16) nsIndex;
}
END_TEST

START_TEST(readValueRank) {
    UA_Int32 rank;
    UA_Variant dims;
    // scalar
    UA_Server_readValueRank(server, UA_NODEID_NUMERIC(testNamespaceIndex, 10002), &rank);
    ck_assert_int_eq(rank, -2);
    UA_Variant_init(&dims);
    UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(testNamespaceIndex, 10002), &dims);
    ck_assert_uint_eq(dims.arrayLength, 0);
    UA_Variant_clear(&dims);
    // 1-dim
    UA_Server_readValueRank(server, UA_NODEID_NUMERIC(testNamespaceIndex, 10007), &rank);
    ck_assert_int_eq(rank, 1);
    UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(testNamespaceIndex, 10007), &dims);
    ck_assert_uint_eq(dims.arrayLength, 1);
    ck_assert_int_eq(*((UA_UInt32 *)dims.data), 0);
    UA_Variant_clear(&dims);
    UA_Server_readValueRank(server, UA_NODEID_NUMERIC(testNamespaceIndex, 10004), &rank);
    ck_assert_int_eq(rank, 1);
    UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(testNamespaceIndex, 10004), &dims);
    ck_assert_uint_eq(dims.arrayLength, 1);
    ck_assert_int_eq(*((UA_UInt32 *)dims.data), 4);
    UA_Variant_clear(&dims);
    // 2-dim
    UA_Server_readValueRank(server, UA_NODEID_NUMERIC(testNamespaceIndex, 10006), &rank);
    ck_assert_int_eq(rank, 2);
    UA_Server_readArrayDimensions(server, UA_NODEID_NUMERIC(testNamespaceIndex, 10006), &dims);
    ck_assert_uint_eq(dims.arrayLength, 2);
    UA_UInt32 *dimensions = (UA_UInt32 *)dims.data;
    ck_assert_int_eq(dimensions[0], 2);
    ck_assert_int_eq(dimensions[1], 2);
    UA_Variant_clear(&dims);
}
END_TEST

START_TEST(checkFrameValues) {
        UA_Variant out;
        UA_Variant_init(&out);
        // Frame variable
        UA_Server_readValue(server, UA_NODEID_NUMERIC(testNamespaceIndex, 15235), &out);
        ck_assert(out.type == &UA_TYPES[UA_TYPES_THREEDFRAME]);
        UA_ThreeDFrame *f = (UA_ThreeDFrame *)out.data;
        ck_assert(UA_Variant_isScalar(&out));
        ck_assert(out.arrayLength == 0);
        ck_assert(out.arrayDimensionsSize == 0);
        ck_assert(f[0].cartesianCoordinates.x == (UA_Double)0.123);
        ck_assert(f[0].cartesianCoordinates.y == (UA_Double)456.7);
        ck_assert(f[0].cartesianCoordinates.z == (UA_Double)89);
        ck_assert(f[0].orientation.a == (UA_Double)0.12);
        ck_assert(f[0].orientation.b == (UA_Double)3.4);
        ck_assert(f[0].orientation.c == (UA_Double)56);
        UA_Variant_clear(&out);
    }
END_TEST

START_TEST(checkInputArguments) {
    UA_Variant out;
    UA_Variant_init(&out);
    // Check argument
    UA_StatusCode status = UA_Server_readValue(server, UA_NODEID_NUMERIC(testNamespaceIndex, 6020), &out);
    ck_assert(status == UA_STATUSCODE_GOOD);
    ck_assert(out.type == &UA_TYPES[UA_TYPES_ARGUMENT]);
    UA_Argument *p = (UA_Argument *)out.data;
    ck_assert(p->dataType.identifierType == UA_NODEIDTYPE_NUMERIC);
    ck_assert(p->dataType.identifier.numeric == (UA_UInt32) 3006);
    ck_assert(p->dataType.namespaceIndex == (UA_UInt16) testNamespaceIndex);
    UA_Variant_clear(&out);
}
END_TEST

START_TEST(checkGuid) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode status = UA_Server_readValue(server, UA_NODEID_NUMERIC(testNamespaceIndex, 7051), &out);
    ck_assert(status == UA_STATUSCODE_GOOD);
    ck_assert(out.type == &UA_TYPES[UA_TYPES_GUID]);
    UA_Guid *scalarData = (UA_Guid *)out.data;
    UA_Guid scalarGuidVal = UA_GUID("7822a391-de79-4a59-b08d-b70bc63fecec");
    ck_assert(UA_Guid_equal(scalarData, &scalarGuidVal));
    UA_Variant_clear(&out);
    status = UA_Server_readValue(server, UA_NODEID_NUMERIC(testNamespaceIndex, 7052), &out);
    ck_assert(status == UA_STATUSCODE_GOOD);
    ck_assert(out.type == &UA_TYPES[UA_TYPES_GUID]);
    ck_assert(out.arrayLength == 3);
    UA_Guid *ArrayData = (UA_Guid *)out.data;
    UA_Guid ArrayGuidVal[3] = {UA_GUID("7822a391-1111-4a59-b08d-b70bc63fecec"),
                               UA_GUID("7822a391-2222-4a59-b08d-b70bc63fecec"),
                               UA_GUID("7822a391-3333-4a59-b08d-b70bc63fecec")};
    ck_assert(UA_Guid_equal(&ArrayData[0], &ArrayGuidVal[0]));
    ck_assert(UA_Guid_equal(&ArrayData[1], &ArrayGuidVal[1]));
    ck_assert(UA_Guid_equal(&ArrayData[2], &ArrayGuidVal[2]));
    UA_Variant_clear(&out);
}
END_TEST

START_TEST(checkDataSetMetaData) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_StatusCode status = UA_Server_readValue(server, UA_NODEID_NUMERIC(testNamespaceIndex, 6021), &out);
    ck_assert(status == UA_STATUSCODE_GOOD);
    ck_assert(out.type == &UA_TYPES[UA_TYPES_DATASETMETADATATYPE]);
    UA_DataSetMetaDataType *p = (UA_DataSetMetaDataType *)out.data;
    UA_String dataSetName = UA_STRING("DataSetName");
    ck_assert(UA_String_equal(&p->name, &dataSetName) == UA_TRUE);
    ck_assert(p->fieldsSize == 1);
    UA_String fieldName = UA_STRING("FieldName");
    ck_assert(UA_String_equal(&p->fields[0].name, &fieldName) == UA_TRUE);
    UA_Guid guid = UA_GUID("10000000-2000-3000-4000-500000000000");
    ck_assert(UA_Guid_equal(&p->dataSetClassId, &guid) == UA_TRUE);

    UA_Variant_clear(&out);
}
END_TEST

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Compiler");
    TCase *tc_server = tcase_create("Server Testnodeset");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_loadTestNodeset);
    tcase_add_test(tc_server, readValueRank);
    tcase_add_test(tc_server, checkFrameValues);
    tcase_add_test(tc_server, checkInputArguments);
    tcase_add_test(tc_server, checkGuid);
    tcase_add_test(tc_server, checkDataSetMetaData);
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
