/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 - 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "testing_clock.h"
#include "test_helpers.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

static UA_Server *server;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(permissionConfigAdd) {
    UA_RolePermissionEntry entries[2];
    entries[0].roleId = UA_NODEID_NUMERIC(0, 1000);
    entries[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | 
                              UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_READROLEPERMISSIONS | 
                              UA_PERMISSIONTYPE_WRITEATTRIBUTE;
    entries[1].roleId = UA_NODEID_NUMERIC(0, 2000);
    entries[1].permissions = UA_PERMISSIONTYPE_BROWSE;
    
    UA_PermissionIndex index;
    UA_StatusCode res = UA_Server_addRolePermission(server, 2, entries, &index);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(index, 0);
    
    /* Adding the same config again should return the same index */
    UA_PermissionIndex index2;
    res = UA_Server_addRolePermission(server, 2, entries, &index2);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(index2, index);
} END_TEST

START_TEST(permissionConfigGet) {
    UA_RolePermissionEntry entries[1];
    entries[0].roleId = UA_NODEID_NUMERIC(0, 1000);
    entries[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READROLEPERMISSIONS | 
                              UA_PERMISSIONTYPE_WRITEATTRIBUTE | UA_PERMISSIONTYPE_WRITEROLEPERMISSIONS;
    
    UA_PermissionIndex index;
    UA_StatusCode res = UA_Server_addRolePermission(server, 1, entries, &index);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    const UA_RolePermissions *rp = UA_Server_getRolePermissionConfig(server, index);
    ck_assert_ptr_ne(rp, NULL);
    ck_assert_int_eq(rp->entriesSize, 1);
    ck_assert(UA_NodeId_equal(&rp->entries[0].roleId, &entries[0].roleId));
    ck_assert_int_eq(rp->entries[0].permissions, entries[0].permissions);
} END_TEST

START_TEST(permissionConfigUpdate) {
    UA_RolePermissionEntry entries[1];
    entries[0].roleId = UA_NODEID_NUMERIC(0, 1000);
    entries[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READROLEPERMISSIONS | 
                              UA_PERMISSIONTYPE_WRITEATTRIBUTE | UA_PERMISSIONTYPE_WRITEROLEPERMISSIONS;
    
    UA_PermissionIndex index;
    UA_StatusCode res = UA_Server_addRolePermission(server, 1, entries, &index);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    /* Update with new permissions */
    entries[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | 
                              UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_READROLEPERMISSIONS | 
                              UA_PERMISSIONTYPE_WRITEATTRIBUTE;
    res = UA_Server_updateRolePermissionConfig(server, index, 1, entries);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    const UA_RolePermissions *rp = UA_Server_getRolePermissionConfig(server, index);
    ck_assert_int_eq(rp->entries[0].permissions, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | 
                                                  UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_READROLEPERMISSIONS | 
                                                  UA_PERMISSIONTYPE_WRITEATTRIBUTE);
} END_TEST

START_TEST(nodePermissionSetIndex) {
    /* Add a permission config */
    UA_RolePermissionEntry entries[1];
    entries[0].roleId = UA_NODEID_NUMERIC(0, 1000);
    entries[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | 
                              UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_READROLEPERMISSIONS | 
                              UA_PERMISSIONTYPE_WRITEATTRIBUTE;
    
    UA_PermissionIndex index;
    UA_StatusCode res = UA_Server_addRolePermission(server, 1, entries, &index);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    /* Set permission index on a node */
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    res = UA_Server_setNodePermissionIndex(server, nodeId, index, UA_FALSE);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    /* Get it back */
    UA_PermissionIndex retrievedIndex;
    res = UA_Server_getNodePermissionIndex(server, nodeId, &retrievedIndex);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(retrievedIndex, index);
} END_TEST

static Suite* testSuite_RBACPermissions(void) {
    Suite *s = suite_create("Server RBAC Permissions");
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    
    tcase_add_test(tc_core, permissionConfigAdd);
    tcase_add_test(tc_core, permissionConfigGet);
    tcase_add_test(tc_core, permissionConfigUpdate);
    tcase_add_test(tc_core, nodePermissionSetIndex);
    
    suite_add_tcase(s, tc_core);
    return s;
}

int main(void) {
    Suite *s = testSuite_RBACPermissions();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
