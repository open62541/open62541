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

/* Core Type Tests */

START_TEST(roleInitClear) {
    UA_Role role;
    UA_Role_init(&role);
    
    ck_assert(UA_NodeId_isNull(&role.roleId));
    ck_assert(UA_QualifiedName_isNull(&role.roleName));
    ck_assert_int_eq(role.identityMappingRulesSize, 0);
    ck_assert_ptr_eq(role.identityMappingRules, NULL);
    ck_assert_int_eq(role.applicationsSize, 0);
    ck_assert_ptr_eq(role.applications, NULL);
    ck_assert_int_eq(role.endpointsSize, 0);
    ck_assert_ptr_eq(role.endpoints, NULL);
    
    UA_Role_clear(&role);
} END_TEST

START_TEST(roleCopy) {
    UA_Role src;
    UA_Role_init(&src);
    src.roleId = UA_NODEID_NUMERIC(0, 1000);
    src.roleName = UA_QUALIFIEDNAME_ALLOC(0, "TestRole");
    
    UA_Role dst;
    UA_StatusCode res = UA_Role_copy(&src, &dst);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    ck_assert(UA_NodeId_equal(&src.roleId, &dst.roleId));
    ck_assert(UA_QualifiedName_equal(&src.roleName, &dst.roleName));
    
    UA_Role_clear(&src);
    UA_Role_clear(&dst);
} END_TEST

START_TEST(roleEqual) {
    UA_Role r1;
    UA_Role_init(&r1);
    r1.roleId = UA_NODEID_NUMERIC(0, 1000);
    r1.roleName = UA_QUALIFIEDNAME_ALLOC(0, "TestRole");
    
    UA_Role r2;
    UA_StatusCode res = UA_Role_copy(&r1, &r2);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    ck_assert(UA_Role_equal(&r1, &r2));
    
    r2.roleId = UA_NODEID_NUMERIC(0, 2000);
    ck_assert(!UA_Role_equal(&r1, &r2));
    
    UA_Role_clear(&r1);
    UA_Role_clear(&r2);
} END_TEST

START_TEST(rolePermissionsInitClear) {
    UA_RolePermissions rp;
    UA_RolePermissions_init(&rp);
    
    ck_assert_int_eq(rp.entriesSize, 0);
    ck_assert_ptr_eq(rp.entries, NULL);
    ck_assert_int_eq(rp.refCount, 0);
    
    UA_RolePermissions_clear(&rp);
} END_TEST

START_TEST(rolePermissionsCopy) {
    UA_RolePermissions src;
    UA_RolePermissions_init(&src);
    
    /* Add some entries */
    src.entriesSize = 2;
    src.entries = (UA_RolePermissionEntry*)UA_calloc(2, sizeof(UA_RolePermissionEntry));
    src.entries[0].roleId = UA_NODEID_NUMERIC(0, 1000);
    src.entries[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | 
                                  UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_READROLEPERMISSIONS | 
                                  UA_PERMISSIONTYPE_WRITEATTRIBUTE;
    src.entries[1].roleId = UA_NODEID_NUMERIC(0, 2000);
    src.entries[1].permissions = UA_PERMISSIONTYPE_BROWSE;
    
    UA_RolePermissions dst;
    UA_StatusCode res = UA_RolePermissions_copy(&src, &dst);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst.entriesSize, 2);
    ck_assert(UA_NodeId_equal(&dst.entries[0].roleId, &src.entries[0].roleId));
    ck_assert_int_eq(dst.entries[0].permissions, src.entries[0].permissions);
    
    UA_RolePermissions_clear(&src);
    UA_RolePermissions_clear(&dst);
} END_TEST

/* Role Management API Tests */

START_TEST(roleAdd) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(0, 10000);
    role.roleName = UA_QUALIFIEDNAME_ALLOC(0, "TestRole");
    
    UA_StatusCode res = UA_Server_addRole(server, &role);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    UA_Role_clear(&role);
} END_TEST

START_TEST(roleAddDuplicate) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(0, 10000);
    role.roleName = UA_QUALIFIEDNAME_ALLOC(0, "TestRole");
    
    UA_StatusCode res = UA_Server_addRole(server, &role);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    /* Try to add again - should fail */
    UA_Role role2;
    UA_Role_init(&role2);
    role2.roleId = UA_NODEID_NUMERIC(0, 10000);
    role2.roleName = UA_QUALIFIEDNAME_ALLOC(0, "TestRole2");
    res = UA_Server_addRole(server, &role2);
    ck_assert_int_eq(res, UA_STATUSCODE_BADNODEIDEXISTS);
    
    UA_Role_clear(&role);
    UA_Role_clear(&role2);
} END_TEST

START_TEST(roleRemove) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(0, 10000);
    role.roleName = UA_QUALIFIEDNAME_ALLOC(0, "TestRole");
    
    UA_StatusCode res = UA_Server_addRole(server, &role);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    UA_NodeId roleIdCopy;
    UA_NodeId_copy(&role.roleId, &roleIdCopy);
    UA_Role_clear(&role);
    
    /* Remove it */
    res = UA_Server_removeRole(server, roleIdCopy);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    /* Try to remove again - should fail */
    res = UA_Server_removeRole(server, roleIdCopy);
    ck_assert_int_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);
    
    UA_NodeId_clear(&roleIdCopy);
} END_TEST

START_TEST(roleGetAll) {
    /* Add multiple roles */
    for(int i = 0; i < 3; i++) {
        UA_Role role;
        UA_Role_init(&role);
        role.roleId = UA_NODEID_NUMERIC(0, 10000 + i);
        char nameBuf[32];
        snprintf(nameBuf, sizeof(nameBuf), "TestRole%d", i);
        role.roleName = UA_QUALIFIEDNAME_ALLOC(0, nameBuf);
        
        UA_StatusCode res = UA_Server_addRole(server, &role);
        ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
        UA_Role_clear(&role);
    }
    
    /* Get all roles */
    size_t rolesSize = 0;
    UA_Role *roles = NULL;
    UA_StatusCode res = UA_Server_getRoles(server, &rolesSize, &roles);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(rolesSize, 3);
    
    /* Clean up */
    for(size_t i = 0; i < rolesSize; i++) {
        UA_Role_clear(&roles[i]);
    }
    UA_free(roles);
} END_TEST

START_TEST(roleGetById) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(0, 10000);
    role.roleName = UA_QUALIFIEDNAME_ALLOC(0, "TestRole");
    
    UA_StatusCode res = UA_Server_addRole(server, &role);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    
    UA_NodeId roleIdCopy;
    UA_NodeId_copy(&role.roleId, &roleIdCopy);
    UA_Role_clear(&role);
    
    /* Get it by ID */
    UA_Role retrieved;
    res = UA_Server_getRoleById(server, roleIdCopy, &retrieved);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&retrieved.roleId, &roleIdCopy));
    
    UA_NodeId_clear(&roleIdCopy);
    UA_Role_clear(&retrieved);
} END_TEST

START_TEST(roleGetNonexistent) {
    UA_Role role;
    UA_NodeId nonexistentId = UA_NODEID_NUMERIC(0, 99999);
    
    UA_StatusCode res = UA_Server_getRoleById(server, nonexistentId, &role);
    ck_assert_int_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);
} END_TEST

/* Integration Tests */

START_TEST(rbacFullWorkflow) {
    /* 1. Create roles */
    UA_Role adminRole;
    UA_Role_init(&adminRole);
    adminRole.roleId = UA_NODEID_NUMERIC(0, 10001);
    adminRole.roleName = UA_QUALIFIEDNAME_ALLOC(0, "Admin");
    UA_Server_addRole(server, &adminRole);
    
    UA_Role userRole;
    UA_Role_init(&userRole);
    userRole.roleId = UA_NODEID_NUMERIC(0, 10002);
    userRole.roleName = UA_QUALIFIEDNAME_ALLOC(0, "User");
    UA_Server_addRole(server, &userRole);
    
    /* 2. Create permission configurations */
    UA_RolePermissionEntry adminEntries[1];
    adminEntries[0].roleId = adminRole.roleId;
    adminEntries[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | 
                                   UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_READROLEPERMISSIONS | 
                                   UA_PERMISSIONTYPE_WRITEATTRIBUTE;
    
    UA_PermissionIndex adminIndex;
    UA_Server_addRolePermission(server, 1, adminEntries, &adminIndex);
    
    UA_RolePermissionEntry userEntries[1];
    userEntries[0].roleId = userRole.roleId;
    userEntries[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READROLEPERMISSIONS;
    
    UA_PermissionIndex userIndex;
    UA_Server_addRolePermission(server, 1, userEntries, &userIndex);
    
    /* 3. Assign permissions to nodes */
    UA_NodeId serverNode = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_Server_setNodePermissionIndex(server, serverNode, adminIndex, UA_FALSE);
    
    /* 4. Verify everything is set up correctly */
    size_t rolesSize;
    UA_Role *roles;
    UA_StatusCode res = UA_Server_getRoles(server, &rolesSize, &roles);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(rolesSize, 2);
    
    /* Clean up */
    for(size_t i = 0; i < rolesSize; i++) {
        UA_Role_clear(&roles[i]);
    }
    UA_free(roles);
    UA_Role_clear(&adminRole);
    UA_Role_clear(&userRole);
} END_TEST

static Suite* testSuite_RBAC(void) {
    Suite *s = suite_create("Server RBAC");
    
    TCase *tc_types = tcase_create("Types");
    tcase_add_test(tc_types, roleInitClear);
    tcase_add_test(tc_types, roleCopy);
    tcase_add_test(tc_types, roleEqual);
    tcase_add_test(tc_types, rolePermissionsInitClear);
    tcase_add_test(tc_types, rolePermissionsCopy);
    suite_add_tcase(s, tc_types);
    
    TCase *tc_roles = tcase_create("Role Management");
    tcase_add_checked_fixture(tc_roles, setup, teardown);
    tcase_add_test(tc_roles, roleAdd);
    tcase_add_test(tc_roles, roleAddDuplicate);
    tcase_add_test(tc_roles, roleRemove);
    tcase_add_test(tc_roles, roleGetAll);
    tcase_add_test(tc_roles, roleGetById);
    tcase_add_test(tc_roles, roleGetNonexistent);
    tcase_add_test(tc_roles, rbacFullWorkflow);
    suite_add_tcase(s, tc_roles);
    
    return s;
}

int main(void) {
    Suite *s = testSuite_RBAC();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
