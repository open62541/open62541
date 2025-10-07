/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>
#include <open62541/nodeids.h>

#include "test_helpers.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_new();
    ck_assert(server != NULL);
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_StatusCode retval = UA_Server_run_startup(server);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    if(server) {
        UA_Server_run_shutdown(server);
        UA_Server_delete(server);
        server = NULL;
    }
}

START_TEST(Server_rbacSimpleValidation) {
    ck_assert(server != NULL);
}
END_TEST

START_TEST(Server_rbacRoleSetExists) {
    UA_NodeId roleSetNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET);
    UA_QualifiedName browseName;
    UA_StatusCode retval = UA_Server_readBrowseName(server, roleSetNodeId, &browseName);
    
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    UA_String expectedName = UA_STRING("RoleSet");
    ck_assert(UA_String_equal(&browseName.name, &expectedName));
    
    UA_QualifiedName_clear(&browseName);
}
END_TEST

START_TEST(Server_rbacDefaultRolesExist) {
    UA_NodeId anonymousRoleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    UA_QualifiedName browseName;
    UA_StatusCode retval = UA_Server_readBrowseName(server, anonymousRoleId, &browseName);
    
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    UA_String expectedName = UA_STRING("Anonymous");
    ck_assert(UA_String_equal(&browseName.name, &expectedName));
    
    UA_QualifiedName_clear(&browseName);
}
END_TEST

static Suite *testSuite_Server_RBAC(void) {
    Suite *s = suite_create("Server RBAC");
    TCase *tc_rbac = tcase_create("RBAC Information Model");
    tcase_add_checked_fixture(tc_rbac, setup, teardown);
    tcase_add_test(tc_rbac, Server_rbacSimpleValidation);
    tcase_add_test(tc_rbac, Server_rbacRoleSetExists);
    tcase_add_test(tc_rbac, Server_rbacDefaultRolesExist);
    suite_add_tcase(s, tc_rbac);
    return s;
}

int main(void) {
    Suite *s = testSuite_Server_RBAC();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
