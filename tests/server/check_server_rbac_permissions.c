/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

/* RBAC effective permissions API tests */

#include <open62541/server_config_default.h>
#include <open62541/nodeids.h>

#include "test_helpers.h"
#include "ua_server_rbac.h"

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

/* Helper: create a custom role with a given name in NS0 */
static UA_StatusCode
addTestRole(UA_Server *s, const char *name, UA_NodeId *outId) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleName = UA_QUALIFIEDNAME(0, (char*)(uintptr_t)name);
    return UA_Server_addRole(s, &role, outId);
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
    retval = addTestRole(server, "TestEffPermRole", &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add identity mapping for this role */
    UA_Role role;
    retval = UA_Server_getRoleById(server, roleId, &role);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    role.identityMappingRules = (UA_IdentityMappingRuleType*)
        UA_calloc(1, sizeof(UA_IdentityMappingRuleType));
    ck_assert_ptr_nonnull(role.identityMappingRules);
    role.identityMappingRules[0].criteriaType =
        UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    role.identityMappingRulesSize = 1;
    retval = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add permissions */
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    retval = UA_Server_addRolePermissions(server, testNodeId, roleId,
                                          permissions, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Get the admin session ID */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});

    /* Assign the role to the session */
    UA_Variant v;
    UA_Variant_setArray(&v, &roleId, 1, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_setSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Get effective permissions */
    UA_UInt32 effectivePerms = 0;
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId,
                                               &testNodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(effectivePerms, permissions);

    /* Clean up */
    UA_Server_deleteSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"));
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "TestEffPermRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with multiple roles (OR behavior) */
START_TEST(Server_getEffectivePermissions_MultipleRoles_OR) {
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
    retval = addTestRole(server, "Role1", &role1Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = addTestRole(server, "Role2", &role2Id);
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
    UA_NodeId roles[2];
    roles[0] = role1Id;
    roles[1] = role2Id;
    UA_Variant v;
    UA_Variant_setArray(&v, roles, 2, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_setSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Get effective permissions - should be OR of both roles */
    UA_UInt32 effectivePerms = 0;
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId,
                                               &testNodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_PermissionType expected = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                                 UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL;
    ck_assert_uint_eq(effectivePerms, expected);

    /* Clean up */
    UA_Server_deleteSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"));
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "Role1"));
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "Role2"));
    UA_NodeId_clear(&role1Id);
    UA_NodeId_clear(&role2Id);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with no roles assigned */
START_TEST(Server_getEffectivePermissions_NoRoles) {
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
    retval = addTestRole(server, "TestRole", &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_addRolePermissions(server, testNodeId, roleId,
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Get the admin session ID - it has no roles assigned */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});

    /* Clear any existing roles */
    retval = UA_Server_deleteSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Get effective permissions - should be 0 (no roles) */
    UA_UInt32 effectivePerms = 0;
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId,
                                               &testNodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(effectivePerms, 0);

    /* Clean up */
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "TestRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with no permissions on node */
START_TEST(Server_getEffectivePermissions_NoPermissionsOnNode) {
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

    /* No permissions and no NS1 defaults -> with allPermissionsForAnonymousRole
     * (the default), unconfigured nodes are fully permissive. */
    UA_UInt32 effectivePerms = 0;
    retval = UA_Server_getEffectivePermissions(server, &adminSessionId,
                                               &testNodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(effectivePerms, 0xFFFFFFFF);

    UA_Server_deleteNode(server, testNodeId, true);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with invalid node ID */
START_TEST(Server_getEffectivePermissions_InvalidNode) {
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId invalidNodeId = UA_NODEID_NUMERIC(0, 999999);

    UA_UInt32 effectivePerms = 0;
    UA_StatusCode retval = UA_Server_getEffectivePermissions(server, &adminSessionId,
                                                              &invalidNodeId,
                                                              &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDUNKNOWN);
}
END_TEST

/* Test UA_Server_getEffectivePermissions with NULL parameters */
START_TEST(Server_getEffectivePermissions_NullParams) {
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    UA_UInt32 effectivePerms = 0;

    UA_StatusCode retval = UA_Server_getEffectivePermissions(NULL, &adminSessionId,
                                                              &nodeId, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    retval = UA_Server_getEffectivePermissions(server, &adminSessionId, NULL, &effectivePerms);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    retval = UA_Server_getEffectivePermissions(server, &adminSessionId, &nodeId, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with single matching role */
START_TEST(Server_getUserRolePermissions_SingleRole) {
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

    UA_NodeId roleId;
    retval = addTestRole(server, "TestUserRolePermRole", &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    retval = UA_Server_addRolePermissions(server, testNodeId, roleId,
                                          permissions, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_Variant v;
    UA_Variant_setArray(&v, &roleId, 1, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_setSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId,
                                              &testNodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(entriesSize, 1);
    ck_assert_ptr_nonnull(entries);
    ck_assert(UA_NodeId_equal(&entries[0].roleId, &roleId));
    ck_assert_uint_eq(entries[0].permissions, permissions);

    UA_Array_delete(entries, entriesSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_deleteSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"));
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "TestUserRolePermRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with multiple roles */
START_TEST(Server_getUserRolePermissions_MultipleRoles) {
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

    UA_NodeId role1Id, role2Id;
    retval = addTestRole(server, "URP_Role1", &role1Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = addTestRole(server, "URP_Role2", &role2Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_addRolePermissions(server, testNodeId, role1Id,
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_addRolePermissions(server, testNodeId, role2Id,
                                          UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId roles[2];
    roles[0] = role1Id;
    roles[1] = role2Id;
    UA_Variant v;
    UA_Variant_setArray(&v, roles, 2, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_setSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId,
                                              &testNodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(entriesSize, 2);
    ck_assert_ptr_nonnull(entries);

    UA_Boolean foundRole1 = false, foundRole2 = false;
    for(size_t i = 0; i < entriesSize; i++) {
        if(UA_NodeId_equal(&entries[i].roleId, &role1Id)) {
            foundRole1 = true;
            ck_assert_uint_eq(entries[i].permissions,
                              UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
        }
        if(UA_NodeId_equal(&entries[i].roleId, &role2Id)) {
            foundRole2 = true;
            ck_assert_uint_eq(entries[i].permissions,
                              UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL);
        }
    }
    ck_assert(foundRole1);
    ck_assert(foundRole2);

    UA_Array_delete(entries, entriesSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_deleteSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"));
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "URP_Role1"));
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "URP_Role2"));
    UA_NodeId_clear(&role1Id);
    UA_NodeId_clear(&role2Id);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with role that has no permissions on node */
START_TEST(Server_getUserRolePermissions_RoleNotOnNode) {
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

    UA_NodeId role1Id, role2Id;
    retval = addTestRole(server, "URPRoleOnNode", &role1Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = addTestRole(server, "URPRoleNotOnNode", &role2Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_addRolePermissions(server, testNodeId, role1Id,
                                          UA_PERMISSIONTYPE_BROWSE, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId roles[2];
    roles[0] = role1Id;
    roles[1] = role2Id;
    UA_Variant v;
    UA_Variant_setArray(&v, roles, 2, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_setSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId,
                                              &testNodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(entriesSize, 1);
    ck_assert_ptr_nonnull(entries);
    ck_assert(UA_NodeId_equal(&entries[0].roleId, &role1Id));

    UA_Array_delete(entries, entriesSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_deleteSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"));
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "URPRoleOnNode"));
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "URPRoleNotOnNode"));
    UA_NodeId_clear(&role1Id);
    UA_NodeId_clear(&role2Id);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with no permissions on node */
START_TEST(Server_getUserRolePermissions_NoPermissionsOnNode) {
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

    UA_NodeId roleId;
    retval = addTestRole(server, "URPRole", &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_Variant v;
    UA_Variant_setArray(&v, &roleId, 1, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_setSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;
    retval = UA_Server_getUserRolePermissions(server, &adminSessionId,
                                              &testNodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(entriesSize, 0);
    ck_assert_ptr_null(entries);

    UA_Server_deleteSessionAttribute(server, &adminSessionId, UA_QUALIFIEDNAME(0, "roles"));
    UA_Server_deleteNode(server, testNodeId, true);
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "URPRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST

/* Test UA_Server_getUserRolePermissions with NULL parameters */
START_TEST(Server_getUserRolePermissions_NullParams) {
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    size_t entriesSize = 0;
    UA_RolePermissionType *entries = NULL;

    UA_StatusCode retval = UA_Server_getUserRolePermissions(NULL, &adminSessionId,
                                                            &nodeId, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    retval = UA_Server_getUserRolePermissions(server, &adminSessionId,
                                              NULL, &entriesSize, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    retval = UA_Server_getUserRolePermissions(server, &adminSessionId,
                                              &nodeId, NULL, &entries);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);

    retval = UA_Server_getUserRolePermissions(server, &adminSessionId,
                                              &nodeId, &entriesSize, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
}
END_TEST

static Suite *testSuite_Server_RBAC_EffectivePerms(void) {
    Suite *s = suite_create("Server RBAC Effective Permissions");
    TCase *tc_effperms = tcase_create("Effective Permissions API");
    tcase_add_unchecked_fixture(tc_effperms, setup, teardown);

    tcase_add_test(tc_effperms, Server_getEffectivePermissions_SingleRole);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_MultipleRoles_OR);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_NoRoles);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_NoPermissionsOnNode);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_InvalidNode);
    tcase_add_test(tc_effperms, Server_getEffectivePermissions_NullParams);

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
