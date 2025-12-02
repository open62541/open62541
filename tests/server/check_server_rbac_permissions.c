/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* RBAC Effective Permissions API Tests
 * 
 * This file tests the RBAC API functions for computing effective permissions:
 * - UA_Server_getEffectivePermissions() - compute OR of permissions for session
 * - UA_Server_getUserRolePermissions() - get array of RolePermissionType for session
 * 
 * These tests complement check_server_rbac.c which tests the configuration/storage
 * of permissions, while this file tests the runtime permission computation.
 */

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
}

static void teardown(void) {
    if(server) {
        UA_Server_delete(server);
        server = NULL;
    }
}

/* Test UA_Server_getEffectivePermissions with a single role */
START_TEST(Server_getEffectivePermissions_SingleRole) {
    /* Create a test variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId = UA_NODEID_STRING(1, "EffPermSingle");
    UA_StatusCode retval = UA_Server_addVariableNode(server, testNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_QUALIFIEDNAME(1, "TestVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create a custom role */
    UA_NodeId roleId;
    retval = UA_Server_addRole(server, UA_STRING("TestEffPermRole"), UA_STRING_NULL, NULL, &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add identity mapping for this role */
    retval = UA_Server_addRoleIdentity(server, roleId,
                                       UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER, UA_STRING_NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add permissions */
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    retval = UA_Server_addRolePermissions(server, testNodeId, roleId, permissions, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get the admin session ID */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    
    /* Assign the role to the session */
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 1, &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get effective permissions */
    UA_UInt32 effectivePerms = 0;
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId, &testNodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(effectivePerms, permissions);
    
    /* Clean up */
    UA_Server_setSessionRoles(server, &adminSessionId, 0, NULL);
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, roleId);
    UA_NodeId_clear(&roleId);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with multiple roles (OR behavior) */
START_TEST(Server_getEffectivePermissions_MultipleRoles_OR) {
    /* Create a test variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId = UA_NODEID_STRING(1, "EffPermMulti");
    UA_StatusCode retval = UA_Server_addVariableNode(server, testNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_QUALIFIEDNAME(1, "TestVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create two custom roles with different permissions */
    UA_NodeId role1Id, role2Id;
    retval = UA_Server_addRole(server, UA_STRING("Role1"), UA_STRING_NULL, NULL, &role1Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_addRole(server, UA_STRING("Role2"), UA_STRING_NULL, NULL, &role2Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Role1: Browse + Read */
    retval = UA_Server_addRolePermissions(server, testNodeId, role1Id,
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Role2: Write + Call */
    retval = UA_Server_addRolePermissions(server, testNodeId, role2Id,
                                          UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get the admin session ID */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    
    /* Assign both roles to the session */
    UA_NodeId roles[2] = {role1Id, role2Id};
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 2, roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get effective permissions - should be OR of both roles */
    UA_UInt32 effectivePerms = 0;
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId, &testNodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Expected: Browse | Read | Write | Call */
    UA_PermissionType expected = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                                 UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL;
    ck_assert_uint_eq(effectivePerms, expected);
    
    /* Clean up */
    UA_Server_setSessionRoles(server, &adminSessionId, 0, NULL);
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, role1Id);
    UA_Server_removeRole(server, role2Id);
    UA_NodeId_clear(&role1Id);
    UA_NodeId_clear(&role2Id);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with no roles assigned */
START_TEST(Server_getEffectivePermissions_NoRoles) {
    /* Create a test variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId = UA_NODEID_STRING(1, "EffPermNoRole");
    UA_StatusCode retval = UA_Server_addVariableNode(server, testNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_QUALIFIEDNAME(1, "TestVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create a role and add permissions to the node */
    UA_NodeId roleId;
    retval = UA_Server_addRole(server, UA_STRING("TestRole"), UA_STRING_NULL, NULL, &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_addRolePermissions(server, testNodeId, roleId,
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get the admin session ID - it has no roles assigned */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    
    /* Clear any existing roles */
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 0, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get effective permissions - should be 0 (no roles) */
    UA_UInt32 effectivePerms = 0;
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId, &testNodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(effectivePerms, 0);
    
    /* Clean up */
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, roleId);
    UA_NodeId_clear(&roleId);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with no permissions on node */
START_TEST(Server_getEffectivePermissions_NoPermissionsOnNode) {
    /* Create a test variable node without any permissions */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId = UA_NODEID_STRING(1, "EffPermNoPerm");
    UA_StatusCode retval = UA_Server_addVariableNode(server, testNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_QUALIFIEDNAME(1, "TestVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get the admin session ID */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    
    /* Get effective permissions - node has no permissions, should return all perms (0xFFFFFFFF) */
    UA_UInt32 effectivePerms = 0;
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId, &testNodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(effectivePerms, 0xFFFFFFFF);  /* All permissions when no RolePermissions defined */
    
    /* Clean up */
    UA_Server_deleteNode(server, testNodeId, true);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with invalid node ID */
START_TEST(Server_getEffectivePermissions_InvalidNode) {
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId invalidNodeId = UA_NODEID_NUMERIC(0, 999999);
    
    UA_UInt32 effectivePerms = 0;
    UA_StatusCode retval = UA_Server_getEffectivePermissions(server, &adminSessionId, &invalidNodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDUNKNOWN);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with NULL parameters */
START_TEST(Server_getEffectivePermissions_NullParams) {
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_UInt32 effectivePerms = 0;
    
    /* NULL server */
    UA_StatusCode retval = UA_Server_getEffectivePermissions(NULL, &adminSessionId, &nodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    
    /* NULL nodeId */
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId, NULL, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    
    /* NULL effectivePermissions */
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId, &nodeId, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with single matching role */
START_TEST(Server_getUserRolePermissions_SingleRole) {
    /* Create a test variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId = UA_NODEID_STRING(1, "UserRolePermSingle");
    UA_StatusCode retval = UA_Server_addVariableNode(server, testNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_QUALIFIEDNAME(1, "TestVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create a custom role */
    UA_NodeId roleId;
    retval = UA_Server_addRole(server, UA_STRING("TestUserRolePermRole"), UA_STRING_NULL, NULL, &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add permissions */
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    retval = UA_Server_addRolePermissions(server, testNodeId, roleId, permissions, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get the admin session ID */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    
    /* Assign the role to the session */
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 1, &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get user role permissions */
    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId, &testNodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(entriesSize, 1);
    ck_assert_ptr_nonnull(entries);
    ck_assert(UA_NodeId_equal(&entries[0].roleId, &roleId));
    ck_assert_uint_eq(entries[0].permissions, permissions);
    
    /* Clean up */
    UA_Array_delete(entries, entriesSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_setSessionRoles(server, &adminSessionId, 0, NULL);
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, roleId);
    UA_NodeId_clear(&roleId);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with multiple roles (returns array) */
START_TEST(Server_getUserRolePermissions_MultipleRoles) {
    /* Create a test variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId = UA_NODEID_STRING(1, "UserRolePermMulti");
    UA_StatusCode retval = UA_Server_addVariableNode(server, testNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_QUALIFIEDNAME(1, "TestVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create two custom roles */
    UA_NodeId role1Id, role2Id;
    retval = UA_Server_addRole(server, UA_STRING("URP_Role1"), UA_STRING_NULL, NULL, &role1Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_addRole(server, UA_STRING("URP_Role2"), UA_STRING_NULL, NULL, &role2Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add permissions for both roles */
    retval = UA_Server_addRolePermissions(server, testNodeId, role1Id,
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_addRolePermissions(server, testNodeId, role2Id,
                                          UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get the admin session ID */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    
    /* Assign both roles to the session */
    UA_NodeId roles[2] = {role1Id, role2Id};
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 2, roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get user role permissions - should return array with both roles */
    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId, &testNodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(entriesSize, 2);
    ck_assert_ptr_nonnull(entries);
    
    /* Verify both roles are in the array */
    UA_Boolean foundRole1 = false, foundRole2 = false;
    for(size_t i = 0; i < entriesSize; i++) {
        if(UA_NodeId_equal(&entries[i].roleId, &role1Id)) {
            foundRole1 = true;
            ck_assert_uint_eq(entries[i].permissions, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
        }
        if(UA_NodeId_equal(&entries[i].roleId, &role2Id)) {
            foundRole2 = true;
            ck_assert_uint_eq(entries[i].permissions, UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL);
        }
    }
    ck_assert(foundRole1);
    ck_assert(foundRole2);
    
    /* Clean up */
    UA_Array_delete(entries, entriesSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_setSessionRoles(server, &adminSessionId, 0, NULL);
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, role1Id);
    UA_Server_removeRole(server, role2Id);
    UA_NodeId_clear(&role1Id);
    UA_NodeId_clear(&role2Id);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with role that has no permissions on node */
START_TEST(Server_getUserRolePermissions_RoleNotOnNode) {
    /* Create a test variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId = UA_NODEID_STRING(1, "UserRolePermNotOnNode");
    UA_StatusCode retval = UA_Server_addVariableNode(server, testNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_QUALIFIEDNAME(1, "TestVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create two roles but only add permissions for one */
    UA_NodeId role1Id, role2Id;
    retval = UA_Server_addRole(server, UA_STRING("URPRoleOnNode"), UA_STRING_NULL, NULL, &role1Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_addRole(server, UA_STRING("URPRoleNotOnNode"), UA_STRING_NULL, NULL, &role2Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add permissions for role1 only */
    retval = UA_Server_addRolePermissions(server, testNodeId, role1Id,
                                          UA_PERMISSIONTYPE_BROWSE,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get the admin session ID */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    
    /* Assign both roles to the session */
    UA_NodeId roles[2] = {role1Id, role2Id};
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 2, roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get user role permissions - should return only role1 (role2 has no permissions on node) */
    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId, &testNodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(entriesSize, 1);  /* Only role1 should be returned */
    ck_assert_ptr_nonnull(entries);
    ck_assert(UA_NodeId_equal(&entries[0].roleId, &role1Id));
    
    /* Clean up */
    UA_Array_delete(entries, entriesSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_setSessionRoles(server, &adminSessionId, 0, NULL);
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, role1Id);
    UA_Server_removeRole(server, role2Id);
    UA_NodeId_clear(&role1Id);
    UA_NodeId_clear(&role2Id);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with no permissions on node */
START_TEST(Server_getUserRolePermissions_NoPermissionsOnNode) {
    /* Create a test variable node without any permissions */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId = UA_NODEID_STRING(1, "UserRolePermNoPerm");
    UA_StatusCode retval = UA_Server_addVariableNode(server, testNodeId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_QUALIFIEDNAME(1, "TestVar"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create a role and assign it to the session */
    UA_NodeId roleId;
    retval = UA_Server_addRole(server, UA_STRING("URPRole"), UA_STRING_NULL, NULL, &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 1, &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get user role permissions - node has no permissions, should return empty array */
    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId, &testNodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(entriesSize, 0);
    ck_assert_ptr_null(entries);
    
    /* Clean up */
    UA_Server_setSessionRoles(server, &adminSessionId, 0, NULL);
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, roleId);
    UA_NodeId_clear(&roleId);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with NULL parameters */
START_TEST(Server_getUserRolePermissions_NullParams) {
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;
    
    /* NULL server */
    UA_StatusCode retval = UA_Server_getUserRolePermissions(NULL, &adminSessionId, &nodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    
    /* NULL nodeId */
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId, NULL, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    
    /* NULL entriesSize */
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId, &nodeId, NULL, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    
    /* NULL entries */
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId, &nodeId, &entriesSize, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
}
END_TEST

static Suite *testSuite_Server_RBAC_EffectivePerms(void) {
    Suite *s = suite_create("Server RBAC Effective Permissions");
    TCase *tc_effperms = tcase_create("Effective Permissions API");
    tcase_add_unchecked_fixture(tc_effperms, setup, teardown);
    
    /* UA_Server_getEffectivePermissions tests */
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_SingleRole);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_MultipleRoles_OR);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_NoRoles);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_NoPermissionsOnNode);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_InvalidNode);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_NullParams);
    
    /* UA_Server_getUserRolePermissions tests */
    tcase_add_test(tc_effperms, Server_getUserRolePermissions_SingleRole);
    tcase_add_test(tc_effperms, Server_getUserRolePermissions_MultipleRoles);
    tcase_add_test(tc_effperms, Server_getUserRolePermissions_RoleNotOnNode);
    tcase_add_test(tc_effperms, Server_getUserRolePermissions_NoPermissionsOnNode);
    tcase_add_test(tc_effperms, Server_getUserRolePermissions_NullParams);
    
    suite_add_tcase(s, tc_effperms);
    return s;
}

int main(void) {
    Suite *s = testSuite_Server_RBAC_EffectivePerms();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
