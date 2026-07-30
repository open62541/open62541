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
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
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

    /* No permissions and no NS1 defaults -> with allPermissionsForAnonymous
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

/* A second recursive setNodeRolePermissions call on a subtree must not
 * change the permissions of nodes outside of it. */
START_TEST(Server_setNodeRolePermissions_subtreeKeepsOthersIntact) {
    UA_NodeId anonymous = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    UA_NodeId secAdmin = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);

    /* Call 1: default permissions on the whole address space */
    UA_RolePermission def[2];
    def[0].roleId = anonymous;
    def[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
        UA_PERMISSIONTYPE_RECEIVEEVENTS | UA_PERMISSIONTYPE_CALL;
    def[1].roleId = secAdmin;
    def[1].permissions = UA_PERMISSIONTYPE_READROLEPERMISSIONS;
    UA_StatusCode res = UA_Server_setNodeRolePermissions(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER), 2, def, true, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Call 2: different permissions on the Server subtree */
    UA_RolePermission sub[2];
    sub[0].roleId = anonymous;
    sub[0].permissions = 0;
    sub[1].roleId = secAdmin;
    sub[1].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
        UA_PERMISSIONTYPE_READROLEPERMISSIONS | UA_PERMISSIONTYPE_CALL;
    res = UA_Server_setNodeRolePermissions(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), 2, sub, true, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* The TypesFolder is not below the Server object: it must still carry
     * the permissions from call 1 */
    size_t sz = 0;
    UA_RolePermission *rp = NULL;
    res = UA_Server_getNodeRolePermissions(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_TYPESFOLDER), &sz, &rp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sz, 2);
    ck_assert(UA_NodeId_equal(&rp[0].roleId, &anonymous));
    ck_assert_uint_eq(rp[0].permissions, def[0].permissions);
    ck_assert_uint_eq(rp[1].permissions, def[1].permissions);
    for(size_t i = 0; i < sz; i++)
        UA_NodeId_clear(&rp[i].roleId);
    UA_free(rp);
}
END_TEST

/* Object instantiation copies the type children including their
 * permissionIndex without adjusting the shared entry's refCount. A
 * permission entry whose refCount dropped to zero must not be rewritten
 * for a new permission set while such copies exist. */
START_TEST(Server_setNodeRolePermissions_staleEntryNotRewritten) {
    UA_NodeId anonymous = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);

    /* Create an ObjectType with a mandatory variable child */
    UA_NodeId typeId = UA_NODEID_NUMERIC(1, 61001);
    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("", "PermTestType");
    UA_StatusCode res = UA_Server_addObjectTypeNode(
        server, typeId, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "PermTestType"),
        otAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId typeChildId = UA_NODEID_NUMERIC(1, 61002);
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.displayName = UA_LOCALIZEDTEXT("", "Child");
    res = UA_Server_addVariableNode(
        server, typeChildId, typeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Child"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_addReference(
        server, typeChildId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Permission set P1 on the type subtree (covers the type child) */
    UA_RolePermission p1[1];
    p1[0].roleId = anonymous;
    p1[0].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    res = UA_Server_setNodeRolePermissions(server, typeId, 1, p1, true, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Instantiate: the mandatory child is copied from the type child */
    UA_NodeId instanceId = UA_NODEID_NUMERIC(1, 61003);
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("", "PermTestInstance");
    res = UA_Server_addObjectNode(
        server, instanceId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "PermTestInstance"), typeId,
        oAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Find the instance child */
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = instanceId;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    rpe.includeSubtypes = true;
    rpe.targetName = UA_QUALIFIEDNAME(1, "Child");
    bp.relativePath.elements = &rpe;
    bp.relativePath.elementsSize = 1;
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(bpr.targetsSize, 1);
    UA_NodeId instanceChildId;
    ck_assert_uint_eq(UA_NodeId_copy(&bpr.targets[0].targetId.nodeId,
                                     &instanceChildId), UA_STATUSCODE_GOOD);
    UA_BrowsePathResult_clear(&bpr);

    /* The instance child must NOT inherit the explicit permissions of the
     * type child (it falls back to the namespace defaults) */
    size_t sz = 0;
    UA_RolePermission *rp = NULL;
    res = UA_Server_getNodeRolePermissions(server, instanceChildId, &sz, &rp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sz, 0);

    /* Reassign the type subtree to P2 so that the refCount of the entry
     * holding P1 drops to zero */
    UA_RolePermission p2[1];
    p2[0].roleId = anonymous;
    p2[0].permissions = UA_PERMISSIONTYPE_BROWSE;
    res = UA_Server_setNodeRolePermissions(server, typeId, 1, p2, true, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* A new distinct permission set must NOT recycle the P1 entry */
    UA_RolePermission p3[1];
    p3[0].roleId = anonymous;
    p3[0].permissions = UA_PERMISSIONTYPE_CALL;
    res = UA_Server_setNodeRolePermissions(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), 1, p3, false, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* The type child still carries P2, the ObjectsFolder P3 */
    res = UA_Server_getNodeRolePermissions(server, typeChildId, &sz, &rp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sz, 1);
    ck_assert_uint_eq(rp[0].permissions, UA_PERMISSIONTYPE_BROWSE);
    UA_NodeId_clear(&rp[0].roleId);
    UA_free(rp);

    res = UA_Server_getNodeRolePermissions(
        server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &sz, &rp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(sz, 1);
    ck_assert_uint_eq(rp[0].permissions, UA_PERMISSIONTYPE_CALL);
    UA_NodeId_clear(&rp[0].roleId);
    UA_free(rp);

    UA_NodeId_clear(&instanceChildId);
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

    TCase *tc_slots = tcase_create("Permission Entry Slots");
    tcase_add_checked_fixture(tc_slots, setup, teardown);
    tcase_add_test(tc_slots, Server_setNodeRolePermissions_subtreeKeepsOthersIntact);
    tcase_add_test(tc_slots, Server_setNodeRolePermissions_staleEntryNotRewritten);
    suite_add_tcase(s, tc_slots);

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
