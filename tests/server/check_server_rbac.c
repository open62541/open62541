/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>

#include <stdlib.h>
#include <string.h>
#include <check.h>

#include "test_helpers.h"
#include "testing_clock.h"

#ifdef UA_ENABLE_RBAC

static UA_Server *server;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* Create a role for testing */
static UA_StatusCode
addTestRole(const char *name, UA_UInt16 nsIdx,
            UA_UInt32 numericId, UA_NodeId *outId)
{
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(nsIdx, numericId);
    role.roleName = UA_QUALIFIEDNAME(nsIdx, (char*)(uintptr_t)name);
    return UA_Server_addRole(server, &role, outId);
}

START_TEST(Role_initClearCopy) {
    UA_Role r;
    UA_Role_init(&r);
    ck_assert(UA_NodeId_isNull(&r.roleId));
    ck_assert_uint_eq(r.identityMappingRulesSize, 0);
    ck_assert_ptr_null(r.identityMappingRules);

    /* Set up a role with data */
    r.roleId = UA_NODEID_NUMERIC(0, 42);
    r.roleName = UA_QUALIFIEDNAME_ALLOC(0, "TestRole");

    UA_Role copy;
    UA_StatusCode res = UA_Role_copy(&r, &copy);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&r.roleId, &copy.roleId));
    ck_assert(UA_QualifiedName_equal(&r.roleName, &copy.roleName));

    ck_assert(UA_Role_equal(&r, &copy));

    UA_Role_clear(&r);
    UA_Role_clear(&copy);
}
END_TEST

START_TEST(addRole_basic) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 50000);
    role.roleName = UA_QUALIFIEDNAME(1, "MyCustomRole");

    UA_NodeId outId = UA_NODEID_NULL;
    UA_StatusCode res = UA_Server_addRole(server, &role, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&outId, &role.roleId));

    /* Verify via getRole (by roleName) */
    UA_Role fetched;
    res = UA_Server_getRole(server, role.roleName, &fetched);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&fetched.roleId, &role.roleId));
    ck_assert(UA_QualifiedName_equal(&fetched.roleName, &role.roleName));
    UA_Role_clear(&fetched);

    UA_NodeId_clear(&outId);
}
END_TEST

START_TEST(addRole_duplicateNameFails) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 60000);
    role.roleName = UA_QUALIFIEDNAME(1, "DuplicateTest");

    UA_NodeId outId = UA_NODEID_NULL;
    UA_StatusCode res = UA_Server_addRole(server, &role, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&outId);

    /* Adding with same roleName should fail */
    role.roleId = UA_NODEID_NUMERIC(1, 60001); /* different nodeId */
    res = UA_Server_addRole(server, &role, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADALREADYEXISTS);
}
END_TEST

START_TEST(addRole_nullRoleIdAllowed) {
    UA_Role role;
    UA_Role_init(&role);
    /* roleId is null => server accepts it as-is */
    role.roleName = UA_QUALIFIEDNAME(1, "NullIdRole");

    UA_NodeId outId = UA_NODEID_NULL;
    UA_StatusCode res = UA_Server_addRole(server, &role, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* The roleId in the registry is null (no auto-generation in this batch) */
    UA_NodeId_clear(&outId);
}
END_TEST

START_TEST(getRoles_empty) {
    size_t rolesSize = 99;
    UA_QualifiedName *roleNames = NULL;
    UA_StatusCode res = UA_Server_getRoles(server, &rolesSize, &roleNames);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(rolesSize, 8);

    for(size_t i = 0; i < rolesSize; i++)
        UA_QualifiedName_clear(&roleNames[i]);
    UA_free(roleNames);
}
END_TEST

START_TEST(getRoles_afterAdd) {
    UA_Role r1, r2;
    UA_Role_init(&r1);
    r1.roleId = UA_NODEID_NUMERIC(0, 70001);
    r1.roleName = UA_QUALIFIEDNAME(0, "RoleA");

    UA_Role_init(&r2);
    r2.roleId = UA_NODEID_NUMERIC(0, 70002);
    r2.roleName = UA_QUALIFIEDNAME(0, "RoleB");

    UA_StatusCode res = UA_Server_addRole(server, &r1, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_addRole(server, &r2, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    size_t rolesSize = 0;
    UA_QualifiedName *roleNames = NULL;
    res = UA_Server_getRoles(server, &rolesSize, &roleNames);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(rolesSize, 2);
    ck_assert_ptr_nonnull(roleNames);

    /* Check that both names are present */
    UA_Boolean found1 = false, found2 = false;
    for(size_t i = 0; i < rolesSize; i++) {
        if(UA_QualifiedName_equal(&roleNames[i], &r1.roleName))
            found1 = true;
        if(UA_QualifiedName_equal(&roleNames[i], &r2.roleName))
            found2 = true;
        UA_QualifiedName_clear(&roleNames[i]);
    }
    UA_free(roleNames);
    ck_assert(found1);
    ck_assert(found2);
}
END_TEST

START_TEST(getRole_notFound) {
    UA_QualifiedName badName = UA_QUALIFIEDNAME(0, "NonExistentRole");
    UA_Role out;
    UA_StatusCode res = UA_Server_getRole(server, badName, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNOTFOUND);
}
END_TEST

START_TEST(removeRole_basic) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 80001);
    role.roleName = UA_QUALIFIEDNAME(1, "RemovableRole");

    UA_StatusCode res = UA_Server_addRole(server, &role, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Remove by roleName */
    res = UA_Server_removeRole(server, role.roleName);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Should no longer be found */
    UA_Role fetched;
    res = UA_Server_getRole(server, role.roleName, &fetched);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNOTFOUND);
}
END_TEST

START_TEST(removeRole_notFound) {
    UA_QualifiedName badName = UA_QUALIFIEDNAME(0, "NoSuchRole");
    UA_StatusCode res = UA_Server_removeRole(server, badName);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNOTFOUND);
}
END_TEST

static UA_Server *serverWithConfigRoles;

static void setupWithConfigRoles(void) {
    /* Build a config with roles set BEFORE server creation,
     * since initRBAC runs during UA_Server_newWithConfig. */
    UA_ServerConfig sc;
    memset(&sc, 0, sizeof(UA_ServerConfig));
    sc.logging = UA_Log_Stdout_new(UA_LOGLEVEL_INFO);
    UA_ServerConfig_setMinimal(&sc, 4840, NULL);

    /* Add two config roles */
    sc.rolesSize = 2;
    sc.roles = (UA_Role*)UA_calloc(2, sizeof(UA_Role));
    ck_assert_ptr_nonnull(sc.roles);

    UA_Role_init(&sc.roles[0]);
    sc.roles[0].roleId = UA_NODEID_NUMERIC(0, 15001);
    sc.roles[0].roleName = UA_QUALIFIEDNAME_ALLOC(0, "ConfigOperator");

    UA_Role_init(&sc.roles[1]);
    sc.roles[1].roleId = UA_NODEID_NUMERIC(0, 15002);
    sc.roles[1].roleName = UA_QUALIFIEDNAME_ALLOC(0, "ConfigEngineer");

    serverWithConfigRoles = UA_Server_newWithConfig(&sc);
    ck_assert_ptr_nonnull(serverWithConfigRoles);

    UA_ServerConfig *config = UA_Server_getConfig(serverWithConfigRoles);
    config->eventLoop->dateTime_now = UA_DateTime_now_fake;
    config->eventLoop->dateTime_nowMonotonic = UA_DateTime_now_fake;
    config->tcpReuseAddr = true;

    UA_Server_run_startup(serverWithConfigRoles);
}

static void teardownWithConfigRoles(void) {
    UA_Server_run_shutdown(serverWithConfigRoles);
    UA_Server_delete(serverWithConfigRoles);
}

START_TEST(configRoles_areLoaded) {
    size_t rolesSize = 0;
    UA_QualifiedName *roleNames = NULL;
    UA_StatusCode res = UA_Server_getRoles(serverWithConfigRoles,
                                           &rolesSize, &roleNames);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(rolesSize, 2);

    for(size_t i = 0; i < rolesSize; i++)
        UA_QualifiedName_clear(&roleNames[i]);
    UA_free(roleNames);
}
END_TEST

START_TEST(configRoles_cannotBeRemoved) {
    UA_QualifiedName configRoleName = UA_QUALIFIEDNAME(0, "ConfigOperator");
    UA_StatusCode res = UA_Server_removeRole(serverWithConfigRoles,
                                             configRoleName);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADUSERACCESSDENIED);

    /* Still accessible */
    UA_Role out;
    res = UA_Server_getRole(serverWithConfigRoles, configRoleName, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Role_clear(&out);
}
END_TEST

START_TEST(configRoles_runtimeRolesCanBeRemoved) {
    /* Add a runtime role */
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 90001);
    role.roleName = UA_QUALIFIEDNAME(1, "RuntimeRole");

    UA_StatusCode res = UA_Server_addRole(serverWithConfigRoles, &role, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Can remove runtime role by name */
    res = UA_Server_removeRole(serverWithConfigRoles, role.roleName);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Config roles still intact */
    size_t rolesSize = 0;
    UA_QualifiedName *roleNames = NULL;
    res = UA_Server_getRoles(serverWithConfigRoles, &rolesSize, &roleNames);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(rolesSize, 2);

    for(size_t i = 0; i < rolesSize; i++)
        UA_QualifiedName_clear(&roleNames[i]);
    UA_free(roleNames);
}
END_TEST

START_TEST(identityManagement_basic) {
    UA_NodeId roleId;
    UA_StatusCode res = addTestRole("IdentityTestRole", 1, 50100, &roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Get the role, add an anonymous identity, update */
    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    role.identityMappingRules = (UA_IdentityMappingRuleType*)
        UA_calloc(1, sizeof(UA_IdentityMappingRuleType));
    ck_assert_ptr_nonnull(role.identityMappingRules);
    role.identityMappingRules[0].criteriaType = UA_IDENTITYCRITERIATYPE_ANONYMOUS;
    role.identityMappingRulesSize = 1;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify via getRole */
    UA_Role fetched;
    res = UA_Server_getRole(server, UA_QUALIFIEDNAME(1, "IdentityTestRole"), &fetched);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(fetched.identityMappingRulesSize, 1);
    ck_assert_uint_eq(fetched.identityMappingRules[0].criteriaType,
                      UA_IDENTITYCRITERIATYPE_ANONYMOUS);
    UA_Role_clear(&fetched);

    /* Update with empty identities to remove */
    res = UA_Server_getRoleById(server, roleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < role.identityMappingRulesSize; i++)
        UA_IdentityMappingRuleType_clear(&role.identityMappingRules[i]);
    UA_free(role.identityMappingRules);
    role.identityMappingRules = NULL;
    role.identityMappingRulesSize = 0;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify removed */
    res = UA_Server_getRole(server, UA_QUALIFIEDNAME(1, "IdentityTestRole"), &fetched);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(fetched.identityMappingRulesSize, 0);
    UA_Role_clear(&fetched);

    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(identityManagement_usernameRule) {
    UA_NodeId roleId;
    UA_StatusCode res = addTestRole("UsernameTestRole", 1, 50101, &roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Set two username identity rules via updateRole */
    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    role.identityMappingRules = (UA_IdentityMappingRuleType*)
        UA_calloc(2, sizeof(UA_IdentityMappingRuleType));
    ck_assert_ptr_nonnull(role.identityMappingRules);
    role.identityMappingRules[0].criteriaType = UA_IDENTITYCRITERIATYPE_USERNAME;
    UA_String u1 = UA_STRING("testuser");
    UA_String_copy(&u1, &role.identityMappingRules[0].criteria);
    role.identityMappingRules[1].criteriaType = UA_IDENTITYCRITERIATYPE_USERNAME;
    UA_String u2 = UA_STRING("anotheruser");
    UA_String_copy(&u2, &role.identityMappingRules[1].criteria);
    role.identityMappingRulesSize = 2;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify both exist */
    UA_Role fetched;
    UA_String username = UA_STRING("testuser");
    UA_String username2 = UA_STRING("anotheruser");
    res = UA_Server_getRole(server, UA_QUALIFIEDNAME(1, "UsernameTestRole"), &fetched);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(fetched.identityMappingRulesSize, 2);

    UA_Boolean found1 = false, found2 = false;
    for(size_t i = 0; i < fetched.identityMappingRulesSize; i++) {
        if(fetched.identityMappingRules[i].criteriaType == UA_IDENTITYCRITERIATYPE_USERNAME) {
            if(UA_String_equal(&fetched.identityMappingRules[i].criteria, &username))
                found1 = true;
            if(UA_String_equal(&fetched.identityMappingRules[i].criteria, &username2))
                found2 = true;
        }
    }
    ck_assert(found1);
    ck_assert(found2);
    UA_Role_clear(&fetched);

    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(applicationManagement_basic) {
    UA_NodeId roleId;
    UA_StatusCode res = addTestRole("AppTestRole", 1, 50110, &roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Set application URI via updateRole */
    UA_Role role;
    res = UA_Server_getRoleById(server, roleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    role.applications = (UA_String*)UA_calloc(1, sizeof(UA_String));
    ck_assert_ptr_nonnull(role.applications);
    UA_String appStr = UA_STRING("urn:test:application");
    UA_String_copy(&appStr, &role.applications[0]);
    role.applicationsSize = 1;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify */
    UA_Role fetched;
    res = UA_Server_getRole(server, UA_QUALIFIEDNAME(1, "AppTestRole"), &fetched);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(fetched.applicationsSize, 1);
    UA_Role_clear(&fetched);

    /* Remove by updating with empty applications */
    res = UA_Server_getRoleById(server, roleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < role.applicationsSize; i++)
        UA_String_clear(&role.applications[i]);
    UA_free(role.applications);
    role.applications = NULL;
    role.applicationsSize = 0;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify removed */
    res = UA_Server_getRole(server, UA_QUALIFIEDNAME(1, "AppTestRole"), &fetched);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(fetched.applicationsSize, 0);
    UA_Role_clear(&fetched);

    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(protectMandatoryRoles) {
    UA_NodeId anonymousRoleId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    UA_StatusCode res;

    /* Cannot update Anonymous via updateRole */
    UA_Role role;
    res = UA_Server_getRoleById(server, anonymousRoleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_updateRole(server, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Role_clear(&role);

    /* Cannot remove Anonymous role */
    res = UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "Anonymous"));
    ck_assert_uint_eq(res, UA_STATUSCODE_BADUSERACCESSDENIED);

    /* Cannot update AuthenticatedUser */
    UA_NodeId authUserRoleId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    res = UA_Server_getRoleById(server, authUserRoleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_updateRole(server, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Role_clear(&role);

    res = UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "AuthenticatedUser"));
    ck_assert_uint_eq(res, UA_STATUSCODE_BADUSERACCESSDENIED);
}
END_TEST

START_TEST(allowModifyingOptionalRoles) {
    UA_NodeId observerRoleId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    UA_StatusCode res;

    /* Can update Observer via updateRole */
    UA_Role role;
    res = UA_Server_getRoleById(server, observerRoleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Add an identity rule */
    role.identityMappingRules = (UA_IdentityMappingRuleType*)
        UA_calloc(1, sizeof(UA_IdentityMappingRuleType));
    ck_assert_ptr_nonnull(role.identityMappingRules);
    role.identityMappingRules[0].criteriaType = UA_IDENTITYCRITERIATYPE_USERNAME;
    role.identityMappingRulesSize = 1;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Update again to clear identity rules */
    res = UA_Server_getRoleById(server, observerRoleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < role.identityMappingRulesSize; i++)
        UA_IdentityMappingRuleType_clear(&role.identityMappingRules[i]);
    UA_free(role.identityMappingRules);
    role.identityMappingRules = NULL;
    role.identityMappingRulesSize = 0;

    res = UA_Server_updateRole(server, &role);
    UA_Role_clear(&role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(roleSetExists) {
    UA_NodeId roleSetNodeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET);
    UA_QualifiedName browseName;
    UA_StatusCode res = UA_Server_readBrowseName(server, roleSetNodeId,
                                                  &browseName);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String expectedName = UA_STRING("RoleSet");
    ck_assert(UA_String_equal(&browseName.name, &expectedName));
    UA_QualifiedName_clear(&browseName);
}
END_TEST

START_TEST(standardRolesWithCorrectIds) {
    struct {
        UA_UInt32 id;
        const char *name;
    } roles[] = {
        {UA_NS0ID_WELLKNOWNROLE_ANONYMOUS, "Anonymous"},
        {UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER, "AuthenticatedUser"},
        {UA_NS0ID_WELLKNOWNROLE_OBSERVER, "Observer"},
        {UA_NS0ID_WELLKNOWNROLE_OPERATOR, "Operator"},
        {UA_NS0ID_WELLKNOWNROLE_ENGINEER, "Engineer"},
        {UA_NS0ID_WELLKNOWNROLE_SUPERVISOR, "Supervisor"},
        {UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN, "ConfigureAdmin"},
        {UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN, "SecurityAdmin"}
    };

    for(size_t i = 0; i < sizeof(roles) / sizeof(roles[0]); i++) {
        UA_NodeId roleId = UA_NODEID_NUMERIC(0, roles[i].id);
        UA_QualifiedName browseName;
        UA_StatusCode res = UA_Server_readBrowseName(server, roleId,
                                                      &browseName);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        UA_String expectedName = UA_STRING((char*)(uintptr_t)roles[i].name);
        ck_assert(UA_String_equal(&browseName.name, &expectedName));
        UA_QualifiedName_clear(&browseName);
    }
}
END_TEST

START_TEST(getAllRoles_includesWellKnown) {
    size_t rolesSize = 0;
    UA_QualifiedName *roleNames = NULL;
    UA_StatusCode res = UA_Server_getRoles(server, &rolesSize, &roleNames);

    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(rolesSize, 8);
    ck_assert_ptr_nonnull(roleNames);

    for(size_t i = 0; i < rolesSize; i++)
        UA_QualifiedName_clear(&roleNames[i]);
    UA_free(roleNames);
}
END_TEST

START_TEST(identityMapping_wellKnownRoles) {
    UA_Role anonymousRole;
    UA_StatusCode res = UA_Server_getRole(server,
                                           UA_QUALIFIEDNAME(0, "Anonymous"),
                                           &anonymousRole);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(anonymousRole.identityMappingRulesSize, 2);

    UA_Boolean hasAnonymous = false, hasAuthUser = false;
    for(size_t i = 0; i < anonymousRole.identityMappingRulesSize; i++) {
        if(anonymousRole.identityMappingRules[i].criteriaType ==
           UA_IDENTITYCRITERIATYPE_ANONYMOUS)
            hasAnonymous = true;
        if(anonymousRole.identityMappingRules[i].criteriaType ==
           UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER)
            hasAuthUser = true;
    }
    ck_assert(hasAnonymous);
    ck_assert(hasAuthUser);
    UA_Role_clear(&anonymousRole);

    UA_Role authUserRole;
    res = UA_Server_getRole(server,
                             UA_QUALIFIEDNAME(0, "AuthenticatedUser"),
                             &authUserRole);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(authUserRole.identityMappingRulesSize, 1);
    ck_assert_uint_eq(authUserRole.identityMappingRules[0].criteriaType,
                      UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER);
    UA_Role_clear(&authUserRole);

    UA_Role observerRole;
    res = UA_Server_getRole(server,
                             UA_QUALIFIEDNAME(0, "Observer"),
                             &observerRole);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(observerRole.identityMappingRulesSize, 0);
    UA_Role_clear(&observerRole);
}
END_TEST

START_TEST(wellKnownRoles_nodeFields) {
    /* Verify DisplayName and NodeClass for well-known role nodes */
    struct {
        UA_UInt32 id;
        const char *name;
    } roles[] = {
        {UA_NS0ID_WELLKNOWNROLE_ANONYMOUS, "Anonymous"},
        {UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER, "AuthenticatedUser"},
        {UA_NS0ID_WELLKNOWNROLE_OBSERVER, "Observer"},
        {UA_NS0ID_WELLKNOWNROLE_OPERATOR, "Operator"},
        {UA_NS0ID_WELLKNOWNROLE_ENGINEER, "Engineer"},
        {UA_NS0ID_WELLKNOWNROLE_SUPERVISOR, "Supervisor"},
        {UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN, "ConfigureAdmin"},
        {UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN, "SecurityAdmin"}
    };
    for(size_t i = 0; i < sizeof(roles) / sizeof(roles[0]); i++) {
        UA_NodeId roleId = UA_NODEID_NUMERIC(0, roles[i].id);
        UA_String expectedName = UA_STRING((char*)(uintptr_t)roles[i].name);

        UA_NodeClass nc = UA_NODECLASS_UNSPECIFIED;
        UA_StatusCode res = UA_Server_readNodeClass(server, roleId, &nc);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);

        UA_LocalizedText displayName;
        res = UA_Server_readDisplayName(server, roleId, &displayName);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert(UA_String_equal(&displayName.text, &expectedName));
        UA_LocalizedText_clear(&displayName);
    }
}
END_TEST

START_TEST(addedRole_ns0NodeFields) {
    /* Add a custom role via the API and verify it is registered */
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 55000);
    role.roleName = UA_QUALIFIEDNAME(1, "FieldCheckRole");

    UA_NodeId outId = UA_NODEID_NULL;
    UA_StatusCode res = UA_Server_addRole(server, &role, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&outId, &role.roleId));

    /* Retrievable via getRole */
    UA_Role fetched;
    res = UA_Server_getRole(server, role.roleName, &fetched);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_QualifiedName_equal(&fetched.roleName, &role.roleName));
    ck_assert(UA_NodeId_equal(&fetched.roleId, &role.roleId));
    ck_assert_uint_eq(fetched.identityMappingRulesSize, 0);
    ck_assert_uint_eq(fetched.applicationsSize, 0);
    ck_assert_uint_eq(fetched.endpointsSize, 0);
    UA_Role_clear(&fetched);

    /* Retrievable via getRoleById */
    UA_Role byId;
    res = UA_Server_getRoleById(server, outId, &byId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_QualifiedName_equal(&byId.roleName, &role.roleName));
    UA_Role_clear(&byId);

    UA_NodeId_clear(&outId);
}
END_TEST

static Suite *testSuite_RolTypeAPI(void) {
    Suite *s = suite_create("RBAC Role Type API");
    TCase *tc = tcase_create("RoleType");
    tcase_add_test(tc, Role_initClearCopy);
    suite_add_tcase(s, tc);
    return s;
}

static Suite *testSuite_RoleManagement(void) {
    Suite *s = suite_create("RBAC Role Management");

    TCase *tc_add = tcase_create("AddRole");
    tcase_add_checked_fixture(tc_add, setup, teardown);
    tcase_add_test(tc_add, addRole_basic);
    tcase_add_test(tc_add, addRole_duplicateNameFails);
    tcase_add_test(tc_add, addRole_nullRoleIdAllowed);
    suite_add_tcase(s, tc_add);

    TCase *tc_get = tcase_create("GetRoles");
    tcase_add_checked_fixture(tc_get, setup, teardown);
    tcase_add_test(tc_get, getRoles_empty);
    tcase_add_test(tc_get, getRoles_afterAdd);
    tcase_add_test(tc_get, getRole_notFound);
    suite_add_tcase(s, tc_get);

    TCase *tc_rm = tcase_create("RemoveRole");
    tcase_add_checked_fixture(tc_rm, setup, teardown);
    tcase_add_test(tc_rm, removeRole_basic);
    tcase_add_test(tc_rm, removeRole_notFound);
    suite_add_tcase(s, tc_rm);

    return s;
}

static Suite *testSuite_ConfigRoles(void) {
    Suite *s = suite_create("RBAC Config Roles");
    TCase *tc = tcase_create("ConfigRoles");
    tcase_add_checked_fixture(tc, setupWithConfigRoles, teardownWithConfigRoles);
    tcase_add_test(tc, configRoles_areLoaded);
    tcase_add_test(tc, configRoles_cannotBeRemoved);
    tcase_add_test(tc, configRoles_runtimeRolesCanBeRemoved);
    suite_add_tcase(s, tc);
    return s;
}

static Suite *testSuite_IdentityAppMgmt(void) {
    Suite *s = suite_create("RBAC Identity/App Management");
    TCase *tc = tcase_create("IdentityApp");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, identityManagement_basic);
    tcase_add_test(tc, identityManagement_usernameRule);
    tcase_add_test(tc, applicationManagement_basic);
    suite_add_tcase(s, tc);
    return s;
}

static Suite *testSuite_InformationModel(void) {
    Suite *s = suite_create("RBAC Information Model");
    TCase *tc = tcase_create("NS0");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, roleSetExists);
    tcase_add_test(tc, standardRolesWithCorrectIds);
    tcase_add_test(tc, getAllRoles_includesWellKnown);
    tcase_add_test(tc, protectMandatoryRoles);
    tcase_add_test(tc, allowModifyingOptionalRoles);
    tcase_add_test(tc, identityMapping_wellKnownRoles);
    tcase_add_test(tc, wellKnownRoles_nodeFields);
    tcase_add_test(tc, addedRole_ns0NodeFields);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int number_failed = 0;
    SRunner *sr;

    sr = srunner_create(testSuite_RolTypeAPI());
    srunner_add_suite(sr, testSuite_RoleManagement());
    srunner_add_suite(sr, testSuite_ConfigRoles());
    srunner_add_suite(sr, testSuite_IdentityAppMgmt());
    srunner_add_suite(sr, testSuite_InformationModel());
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else /* UA_ENABLE_RBAC not defined */

int main(void) {
    return EXIT_SUCCESS;
}

#endif /* UA_ENABLE_RBAC */
