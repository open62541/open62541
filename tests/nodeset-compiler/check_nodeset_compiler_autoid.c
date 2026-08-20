/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "check.h"
#include "testing_clock.h"
#include "test_helpers.h"
#include "tests/namespace_tests_di_generated.h"
#include "tests/namespace_tests_autoid_generated.h"
#include <stdlib.h>

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

START_TEST(Server_addDiNodeset) {
    UA_StatusCode retval = namespace_tests_di_generated(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_addAutoIDNodeset) {
    UA_StatusCode retval = namespace_tests_autoid_generated(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t namespaceIndex = 0;
    retval = UA_Server_getNamespaceByName(
        server, UA_STRING("http://opcfoundation.org/UA/AutoID/"),
        &namespaceIndex);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeId accessResultId = UA_NODEID_NUMERIC(namespaceIndex, 3017);
    UA_NodeId rfidAccessResultId = UA_NODEID_NUMERIC(namespaceIndex, 3018);
    UA_NodeId locationId = UA_NODEID_NUMERIC(namespaceIndex, 3008);
    const UA_DataType *accessResult =
        UA_Server_findDataType(server, &accessResultId);
    const UA_DataType *rfidAccessResult =
        UA_Server_findDataType(server, &rfidAccessResultId);
    const UA_DataType *location = UA_Server_findDataType(server, &locationId);
    ck_assert_ptr_nonnull(accessResult);
    ck_assert_ptr_nonnull(rfidAccessResult);
    ck_assert_ptr_nonnull(location);
    ck_assert_uint_eq(accessResult->membersSize, 3);
    ck_assert_uint_eq(rfidAccessResult->membersSize, 10);
    ck_assert_uint_eq(rfidAccessResult->typeKind, UA_DATATYPEKIND_OPTSTRUCT);
    ck_assert_uint_eq(location->typeKind, UA_DATATYPEKIND_UNION);

#ifdef UA_ENABLE_TYPEDESCRIPTION
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = rfidAccessResultId;
    rvi.attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;
    UA_DataValue definitionValue =
        UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(definitionValue.status, UA_STATUSCODE_GOOD);
    ck_assert(definitionValue.hasValue);
    ck_assert_ptr_eq(definitionValue.value.type,
                     &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]);
    UA_StructureDefinition *definition =
        (UA_StructureDefinition*)definitionValue.value.data;
    UA_NodeId structureId = UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE);
    ck_assert(UA_NodeId_equal(&definition->baseDataType, &structureId));
    /* The on-demand definition reflects the flattened static C layout. */
    ck_assert_uint_eq(definition->fieldsSize, 10);
    UA_DataValue_clear(&definitionValue);
#endif
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Compiler");
    TCase *tc_server = tcase_create("Server DI and AutoID nodeset");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_addDiNodeset);
    tcase_add_test(tc_server, Server_addAutoIDNodeset);
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
