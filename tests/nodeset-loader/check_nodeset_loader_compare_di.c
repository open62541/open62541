/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodesetloader.h>
#include <open62541/types.h>

#include "check.h"
#include "tests/namespace_nodesetloader_di_generated.h"
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

START_TEST(Server_compareDiNodeset) {
    UA_StatusCode retVal = UA_Server_loadNodeset(server,
        OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml", NULL);
    ck_assert(UA_StatusCode_isGood(retVal));

    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert(config->customDataTypes);

    UA_UInt16 nsIndex = UA_Server_addNamespace(server, "http://opcfoundation.org/UA/DI/");

    for(int i = 0; i < UA_TYPES_NODESETLOADER_DI_COUNT; ++i) {
        UA_TYPES_NODESETLOADER_DI[i].typeId.namespaceIndex = nsIndex;
        UA_TYPES_NODESETLOADER_DI[i].binaryEncodingId.namespaceIndex = nsIndex;

        const UA_DataType *compiledType = &UA_TYPES_NODESETLOADER_DI[i];
        const UA_DataType *loadedType = UA_Server_findDataType(server, &compiledType->typeId);

        ck_assert(loadedType != NULL);
        ck_assert(compiledType->typeKind == loadedType->typeKind);
        ck_assert(compiledType->membersSize == loadedType->membersSize);
        ck_assert(compiledType->memSize == loadedType->memSize);
        ck_assert(compiledType->overlayable == loadedType->overlayable);
        ck_assert(compiledType->pointerFree == loadedType->pointerFree);
        ck_assert(!strcmp(compiledType->typeName, loadedType->typeName));

        for(int j = 0; j < compiledType->membersSize; ++j) {
            const UA_DataTypeMember *compMember = &compiledType->members[j];
            const UA_DataTypeMember *loadMember = &loadedType->members[j];

            ck_assert(compMember->isArray == loadMember->isArray);
            ck_assert(UA_NodeId_equal(
                &compMember->memberType->typeId, &loadMember->memberType->typeId));
            ck_assert(compMember->padding == loadMember->padding);
            ck_assert(compMember->isOptional == loadMember->isOptional);
        }
    }
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Loader");
    TCase *tc_server = tcase_create("Compare DI Nodeset");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_compareDiNodeset);
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
