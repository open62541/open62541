/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/nodeids.h>

#include "ua_server_rbac.h"

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

/* Remove a role by QualifiedName */
static UA_StatusCode
removeTestRole(const char *name, UA_UInt16 nsIdx)
{
    return UA_Server_removeRole(server,
                                UA_QUALIFIEDNAME(nsIdx, (char*)(uintptr_t)name));
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
    /* roleId is null => server auto-generates a numeric NodeId */
    role.roleName = UA_QUALIFIEDNAME(1, "NullIdRole");

    UA_NodeId outId = UA_NODEID_NULL;
    UA_StatusCode res = UA_Server_addRole(server, &role, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify the generated ID: numeric, non-zero */
    ck_assert(!UA_NodeId_isNull(&outId));
    ck_assert_uint_eq(outId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert(outId.identifier.numeric != 0);

    removeTestRole("NullIdRole", 1);
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

START_TEST(removeRole_andVerifyGetRoles) {
    /* Record initial role count */
    size_t initialSize = 0;
    UA_QualifiedName *initialNames = NULL;
    UA_StatusCode res = UA_Server_getRoles(server, &initialSize, &initialNames);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < initialSize; i++)
        UA_QualifiedName_clear(&initialNames[i]);
    UA_free(initialNames);

    /* Add a role */
    UA_NodeId outId;
    res = addTestRole("TempRole", 1, 80010, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Remove it */
    res = removeTestRole("TempRole", 1);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify count is back to initial */
    size_t afterSize = 0;
    UA_QualifiedName *afterNames = NULL;
    res = UA_Server_getRoles(server, &afterSize, &afterNames);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(afterSize, initialSize);

    for(size_t i = 0; i < afterSize; i++)
        UA_QualifiedName_clear(&afterNames[i]);
    UA_free(afterNames);
    UA_NodeId_clear(&outId);
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

#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
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
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */

#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
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
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */

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

#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
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
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */

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

START_TEST(sessionRoleManagement) {
    UA_NodeId adminSessionId = UA_NODEID_GUID(0,
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});

    /* Initially no roles: returns empty array */
    UA_Variant out;
    UA_StatusCode res = UA_Server_getSessionAttributeCopy(server, &adminSessionId,
                                                          UA_QUALIFIEDNAME(0, "roles"),
                                                          &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.arrayLength, 0);
    UA_Variant_clear(&out);

    /* Set two roles */
    UA_NodeId rolesToSet[2];
    rolesToSet[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    rolesToSet[1] = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    UA_Variant v;
    UA_Variant_setArray(&v, rolesToSet, 2, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_getSessionAttributeCopy(server, &adminSessionId,
                                            UA_QUALIFIEDNAME(0, "roles"), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.arrayLength, 2);
    ck_assert_ptr_nonnull(out.data);

    UA_NodeId *gotRoles = (UA_NodeId*)out.data;
    UA_Boolean foundObserver = false, foundOperator = false;
    for(size_t i = 0; i < out.arrayLength; i++) {
        if(UA_NodeId_equal(&gotRoles[i], &rolesToSet[0])) foundObserver = true;
        if(UA_NodeId_equal(&gotRoles[i], &rolesToSet[1])) foundOperator = true;
    }
    ck_assert(foundObserver);
    ck_assert(foundOperator);
    UA_Variant_clear(&out);

    /* Update to a different set */
    UA_NodeId newRoles[1];
    newRoles[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    UA_Variant_setArray(&v, newRoles, 1, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_getSessionAttributeCopy(server, &adminSessionId,
                                            UA_QUALIFIEDNAME(0, "roles"), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.arrayLength, 1);
    ck_assert(UA_NodeId_equal((UA_NodeId*)out.data, &newRoles[0]));
    UA_Variant_clear(&out);

    /* Clear all roles */
    res = UA_Server_deleteSessionAttribute(server, &adminSessionId,
                                           UA_QUALIFIEDNAME(0, "roles"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_getSessionAttributeCopy(server, &adminSessionId,
                                            UA_QUALIFIEDNAME(0, "roles"), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.arrayLength, 0);
    UA_Variant_clear(&out);

    /* Invalid session ID */
    UA_NodeId invalidSessionId = UA_NODEID_NUMERIC(0, 999999);
    res = UA_Server_getSessionAttributeCopy(server, &invalidSessionId,
                                            UA_QUALIFIEDNAME(0, "roles"), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADSESSIONIDINVALID);

    /* Invalid role ID */
    UA_NodeId invalidRole = UA_NODEID_NUMERIC(0, 999999);
    UA_Variant_setArray(&v, &invalidRole, 1, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);
}
END_TEST

START_TEST(addSessionRole) {
    UA_NodeId adminSessionId = UA_NODEID_GUID(0,
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});

    /* Add one role */
    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    UA_Variant v;
    UA_Variant_setArray(&v, &observerRole, 1, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                                      UA_QUALIFIEDNAME(0, "roles"),
                                                      &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant out;
    res = UA_Server_getSessionAttributeCopy(server, &adminSessionId,
                                            UA_QUALIFIEDNAME(0, "roles"), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.arrayLength, 1);
    ck_assert(UA_NodeId_equal((UA_NodeId*)out.data, &observerRole));
    UA_Variant_clear(&out);

    /* Append a second role by setting a two-element array */
    UA_NodeId operatorRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    UA_NodeId twoRoles[2] = { observerRole, operatorRole };
    UA_Variant_setArray(&v, twoRoles, 2, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_getSessionAttributeCopy(server, &adminSessionId,
                                            UA_QUALIFIEDNAME(0, "roles"), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.arrayLength, 2);
    UA_Variant_clear(&out);

    /* Setting the same set again replaces (idempotent result) */
    UA_Variant_setArray(&v, twoRoles, 2, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_getSessionAttributeCopy(server, &adminSessionId,
                                            UA_QUALIFIEDNAME(0, "roles"), &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.arrayLength, 2);
    UA_Variant_clear(&out);

    /* Invalid role ID */
    UA_NodeId invalidRole = UA_NODEID_NUMERIC(0, 999999);
    UA_Variant_setArray(&v, &invalidRole, 1, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);

    /* Clear */
    res = UA_Server_deleteSessionAttribute(server, &adminSessionId,
                                           UA_QUALIFIEDNAME(0, "roles"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(nodePermissions_basic) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId testNodeId;
    UA_StatusCode res = UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestVariable"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &testNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex permIdx;
    res = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);

    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;

    res = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                       permissions, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(permIdx != UA_PERMISSION_INDEX_INVALID);

    const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissionsSize, 1);
    ck_assert(UA_NodeId_equal(&rp->rolePermissions[0].roleId, &observerRole));
    ck_assert_uint_eq(rp->rolePermissions[0].permissions, permissions);

    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(nodePermissions_multipleRoles) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "MultiRoleVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 123;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId testNodeId;
    UA_StatusCode res = UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "MultiRoleVariable"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &testNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    res = UA_Server_addRolePermissions(server, testNodeId, observerRole,
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId operatorRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    res = UA_Server_addRolePermissions(server, testNodeId, operatorRole,
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE,
        false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex permIdx;
    res = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissionsSize, 2);

    UA_Boolean foundObserver = false, foundOperator = false;
    for(size_t i = 0; i < rp->rolePermissionsSize; i++) {
        if(UA_NodeId_equal(&rp->rolePermissions[i].roleId, &observerRole)) {
            foundObserver = true;
            ck_assert_uint_eq(rp->rolePermissions[i].permissions,
                              UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
        }
        if(UA_NodeId_equal(&rp->rolePermissions[i].roleId, &operatorRole)) {
            foundOperator = true;
            ck_assert_uint_eq(rp->rolePermissions[i].permissions,
                              UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                              UA_PERMISSIONTYPE_WRITE);
        }
    }
    ck_assert(foundObserver);
    ck_assert(foundOperator);

    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(nodePermissions_update) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "UpdateVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 456;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId testNodeId;
    UA_StatusCode res = UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "UpdateVariable"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &testNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    res = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                       UA_PERMISSIONTYPE_BROWSE, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_addRolePermissions(server, testNodeId, observerRole,
        UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_READROLEPERMISSIONS,
        false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex permIdx;
    res = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissionsSize, 1);
    ck_assert_uint_eq(rp->rolePermissions[0].permissions,
                      UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                      UA_PERMISSIONTYPE_READROLEPERMISSIONS);

    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(nodePermissions_invalidRole) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "ErrorTestVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 789;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId testNodeId;
    UA_StatusCode res = UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ErrorTestVariable"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &testNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId invalidRole = UA_NODEID_NUMERIC(0, 999999);
    res = UA_Server_addRolePermissions(server, testNodeId, invalidRole,
                                       UA_PERMISSIONTYPE_BROWSE, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);

    UA_PermissionIndex permIdx;
    res = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);

    UA_NodeId invalidNode = UA_NODEID_NUMERIC(0, 999998);
    UA_NodeId validRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    res = UA_Server_addRolePermissions(server, invalidNode, validRole,
                                       UA_PERMISSIONTYPE_BROWSE, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNODEIDUNKNOWN);

    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(nodePermissions_overwrite) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "OverwriteVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 999;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId testNodeId;
    UA_StatusCode res = UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "OverwriteVariable"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &testNodeId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    res = UA_Server_addRolePermissions(server, testNodeId, observerRole,
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE,
        false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                       UA_PERMISSIONTYPE_BROWSE, true, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex permIdx;
    res = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissionsSize, 1);
    ck_assert_uint_eq(rp->rolePermissions[0].permissions, UA_PERMISSIONTYPE_BROWSE);

    res = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                       UA_PERMISSIONTYPE_READ, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissionsSize, 1);
    ck_assert_uint_eq(rp->rolePermissions[0].permissions,
                      UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);

    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(nodePermissions_recursive) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ParentObject");
    UA_NodeId parentId;
    UA_StatusCode res = UA_Server_addObjectNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ParentObject"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &parentId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Child1");
    UA_NodeId child1Id;
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, parentId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Child1"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &child1Id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Child2");
    UA_NodeId child2Id;
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, parentId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Child2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &child2Id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "GrandChild1");
    UA_NodeId grandChild1Id;
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, child2Id,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "GrandChild1"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &grandChild1Id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId operatorRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    res = UA_Server_addRolePermissions(server, parentId, operatorRole,
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ, false, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId nodes[] = {parentId, child1Id, child2Id, grandChild1Id};
    for(size_t i = 0; i < 4; i++) {
        UA_PermissionIndex permIdx;
        res = UA_Server_getNodePermissionIndex(server, nodes[i], &permIdx);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_ne(permIdx, UA_PERMISSION_INDEX_INVALID);

        const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, permIdx);
        ck_assert_ptr_nonnull(rp);
        ck_assert_uint_eq(rp->rolePermissionsSize, 1);
        ck_assert(UA_NodeId_equal(&rp->rolePermissions[0].roleId, &operatorRole));
    }

    UA_Server_deleteNode(server, parentId, true);
    UA_NodeId_clear(&parentId);
    UA_NodeId_clear(&child1Id);
    UA_NodeId_clear(&child2Id);
    UA_NodeId_clear(&grandChild1Id);
}
END_TEST

START_TEST(removePermissions_recursive) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ParentNode");
    UA_NodeId parentId;
    UA_StatusCode res = UA_Server_addObjectNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ParentNode"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &parentId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ChildNode");
    UA_NodeId childId;
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, parentId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "ChildNode"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &childId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId engineerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    res = UA_Server_addRolePermissions(server, parentId, engineerRole,
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE,
        false, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_removeRolePermissions(server, parentId, engineerRole,
                                          UA_PERMISSIONTYPE_WRITE, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex permIdx;
    res = UA_Server_getNodePermissionIndex(server, parentId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissions[0].permissions,
                      UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);

    res = UA_Server_getNodePermissionIndex(server, childId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissions[0].permissions,
                      UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);

    res = UA_Server_removeRolePermissions(server, parentId, engineerRole,
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_getNodePermissionIndex(server, parentId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);

    res = UA_Server_getNodePermissionIndex(server, childId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);

    UA_Server_deleteNode(server, parentId, true);
    UA_NodeId_clear(&parentId);
    UA_NodeId_clear(&childId);
}
END_TEST

START_TEST(setPermissionIndex_recursive) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "RootNode");
    UA_NodeId rootId;
    UA_StatusCode res = UA_Server_addObjectNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RootNode"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &rootId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "SubNode");
    UA_NodeId subId;
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, rootId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SubNode"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &subId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId supervisorRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    UA_RolePermission entry;
    res = UA_NodeId_copy(&supervisorRole, &entry.roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    entry.permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                        UA_PERMISSIONTYPE_WRITE;

    UA_PermissionIndex configIdx;
    res = UA_Server_addRolePermissionConfig(server, 1, &entry, &configIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&entry.roleId);

    res = UA_Server_setNodePermissionIndex(server, rootId, configIdx, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex permIdx;
    res = UA_Server_getNodePermissionIndex(server, rootId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, configIdx);

    res = UA_Server_getNodePermissionIndex(server, subId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, configIdx);

    res = UA_Server_setNodePermissionIndex(server, rootId,
                                            UA_PERMISSION_INDEX_INVALID, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_getNodePermissionIndex(server, rootId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);

    res = UA_Server_getNodePermissionIndex(server, subId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);

    UA_Server_deleteNode(server, rootId, true);
    UA_NodeId_clear(&rootId);
    UA_NodeId_clear(&subId);
}
END_TEST

START_TEST(nodePermissions_sharedConfig) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 100;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);

    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SharedNode1");
    UA_NodeId node1;
    UA_StatusCode res = UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SharedNode1"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &node1);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SharedNode2");
    UA_NodeId node2;
    res = UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SharedNode2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &node2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;

    res = UA_Server_addRolePermissions(server, node1, observerRole, permissions, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_addRolePermissions(server, node2, observerRole, permissions, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex idx1, idx2;
    res = UA_Server_getNodePermissionIndex(server, node1, &idx1);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(idx1 != UA_PERMISSION_INDEX_INVALID);
    res = UA_Server_getNodePermissionIndex(server, node2, &idx2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(idx2 != UA_PERMISSION_INDEX_INVALID);

    const UA_RolePermissionSet *rp1 = UA_Server_getRolePermissionConfig(server, idx1);
    const UA_RolePermissionSet *rp2 = UA_Server_getRolePermissionConfig(server, idx2);
    ck_assert_ptr_nonnull(rp1);
    ck_assert_ptr_nonnull(rp2);
    ck_assert_uint_eq(rp1->rolePermissionsSize, 1);
    ck_assert_uint_eq(rp2->rolePermissionsSize, 1);

    UA_Server_deleteNode(server, node1, true);
    UA_Server_deleteNode(server, node2, true);
    UA_NodeId_clear(&node1);
    UA_NodeId_clear(&node2);
}
END_TEST

START_TEST(recursivePermissions_onBuildInfo) {
    UA_NodeId buildInfoId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
    UA_NodeId operatorRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                                    UA_PERMISSIONTYPE_READROLEPERMISSIONS | UA_PERMISSIONTYPE_WRITE;

    UA_StatusCode res = UA_Server_addRolePermissions(server, buildInfoId, operatorRole,
                                                     permissions, false, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_UInt32 buildInfoChildren[] = {
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI,
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME,
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME,
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_SOFTWAREVERSION,
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDNUMBER,
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_BUILDDATE
    };

    for(size_t i = 0; i < 6; i++) {
        UA_NodeId childId = UA_NODEID_NUMERIC(0, buildInfoChildren[i]);
        UA_PermissionIndex permIdx;
        res = UA_Server_getNodePermissionIndex(server, childId, &permIdx);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
        ck_assert_uint_ne(permIdx, UA_PERMISSION_INDEX_INVALID);

        const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, permIdx);
        ck_assert_ptr_nonnull(rp);
        ck_assert_uint_ge(rp->rolePermissionsSize, 1);

        UA_Boolean foundOperator = false;
        for(size_t j = 0; j < rp->rolePermissionsSize; j++) {
            if(UA_NodeId_equal(&rp->rolePermissions[j].roleId, &operatorRole)) {
                foundOperator = true;
                ck_assert_uint_eq(rp->rolePermissions[j].permissions, permissions);
                break;
            }
        }
        ck_assert_msg(foundOperator, "Operator role not found in child node %u",
                       buildInfoChildren[i]);
    }

    /* Read RolePermissions attribute via read service */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    {
        UA_Variant rv;
        UA_Variant_setArray(&rv, &operatorRole, 1, &UA_TYPES[UA_TYPES_NODEID]);
        res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                            UA_QUALIFIEDNAME(0, "roles"), &rv);
        ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    }

    UA_NodeId productUriId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = productUriId;
    rvid.attributeId = UA_ATTRIBUTEID_ROLEPERMISSIONS;

    UA_DataValue dv = UA_Server_read(server, &rvid, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert(!dv.hasStatus || dv.status == UA_STATUSCODE_GOOD);
    ck_assert(dv.hasValue);
    ck_assert(dv.value.type == &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    size_t rpCount = dv.value.arrayLength;
    if(rpCount == 0 && dv.value.data) rpCount = 1;
    ck_assert_uint_ge(rpCount, 1);

    UA_Boolean foundOperator = false;
    UA_RolePermissionType *rpArray = (UA_RolePermissionType*)dv.value.data;
    for(size_t i = 0; i < rpCount; i++) {
        if(UA_NodeId_equal(&rpArray[i].roleId, &operatorRole)) {
            foundOperator = true;
            ck_assert_uint_eq(rpArray[i].permissions, permissions);
            break;
        }
    }
    ck_assert_msg(foundOperator, "Operator role not found in ProductUri RolePermissions");
    UA_DataValue_clear(&dv);
}
END_TEST

START_TEST(effectivePermissions_logicalOR) {
    UA_NodeId role1Id, role2Id;
    UA_StatusCode res = addTestRole("EffRole1", 1, 51001, &role1Id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = addTestRole("EffRole2", 1, 51002, &role2Id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId testNodeId = UA_NODEID_STRING(1, "TestEffectivePerms");
    res = UA_Server_addVariableNode(server, testNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_addRolePermissions(server, testNodeId, role1Id,
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_addRolePermissions(server, testNodeId, role2Id,
        UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex permIdx;
    res = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissionsSize, 2);

    UA_PermissionType role1Perms = 0, role2Perms = 0;
    for(size_t i = 0; i < rp->rolePermissionsSize; i++) {
        if(UA_NodeId_equal(&rp->rolePermissions[i].roleId, &role1Id))
            role1Perms = rp->rolePermissions[i].permissions;
        if(UA_NodeId_equal(&rp->rolePermissions[i].roleId, &role2Id))
            role2Perms = rp->rolePermissions[i].permissions;
    }
    ck_assert_uint_eq(role1Perms, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
    ck_assert_uint_eq(role2Perms, UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL);

    UA_PermissionType effective = role1Perms | role2Perms;
    ck_assert_uint_eq(effective, UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                                 UA_PERMISSIONTYPE_WRITE | UA_PERMISSIONTYPE_CALL);

    UA_Server_deleteNode(server, testNodeId, true);
    removeTestRole("EffRole1", 1);
    removeTestRole("EffRole2", 1);
    UA_NodeId_clear(&role1Id);
    UA_NodeId_clear(&role2Id);
}
END_TEST

START_TEST(userRolePermissions_array) {
    UA_NodeId role1Id, role2Id, role3Id;
    UA_StatusCode res = addTestRole("URPRole1", 1, 51010, &role1Id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = addTestRole("URPRole2", 1, 51011, &role2Id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = addTestRole("URPRole3", 1, 51012, &role3Id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVar");
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId testNodeId = UA_NODEID_STRING(1, "TestUserRolePerms");
    res = UA_Server_addVariableNode(server, testNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_addRolePermissions(server, testNodeId, role1Id,
                                       UA_PERMISSIONTYPE_BROWSE, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_addRolePermissions(server, testNodeId, role2Id,
                                       UA_PERMISSIONTYPE_READ, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_addRolePermissions(server, testNodeId, role3Id,
                                       UA_PERMISSIONTYPE_WRITE, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex permIdx;
    res = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissionsSize, 3);

    UA_Server_deleteNode(server, testNodeId, true);
    removeTestRole("URPRole1", 1);
    removeTestRole("URPRole2", 1);
    removeTestRole("URPRole3", 1);
    UA_NodeId_clear(&role1Id);
    UA_NodeId_clear(&role2Id);
    UA_NodeId_clear(&role3Id);
}
END_TEST

START_TEST(allPermissionsForAnonymousRole_config) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert(config->allPermissionsForAnonymousRole == true);
    config->allPermissionsForAnonymousRole = false;
    ck_assert(config->allPermissionsForAnonymousRole == false);
    config->allPermissionsForAnonymousRole = true;
}
END_TEST

START_TEST(permissionConfig_addAndGet) {
    UA_NodeId roleId;
    UA_StatusCode res = addTestRole("PCRole", 1, 51070, &roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_RolePermission entry;
    res = UA_NodeId_copy(&roleId, &entry.roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    entry.permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;

    UA_PermissionIndex configIdx;
    res = UA_Server_addRolePermissionConfig(server, 1, &entry, &configIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&entry.roleId);

    const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, configIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissionsSize, 1);
    ck_assert(UA_NodeId_equal(&rp->rolePermissions[0].roleId, &roleId));
    ck_assert_uint_eq(rp->rolePermissions[0].permissions,
                      UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);

    removeTestRole("PCRole", 1);
    UA_NodeId_clear(&roleId);
}
END_TEST

/* Test getSessionRoleNames returns QualifiedNames for assigned roles */
START_TEST(sessionRoleNames) {
    UA_NodeId adminSessionId = UA_NODEID_GUID(0,
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});

    /* Set two roles on the session */
    UA_NodeId rolesToSet[2];
    rolesToSet[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    rolesToSet[1] = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    UA_Variant v;
    UA_Variant_setArray(&v, rolesToSet, 2, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                                      UA_QUALIFIEDNAME(0, "roles"), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Query role names */
    size_t namesSize = 0;
    UA_QualifiedName *names = NULL;
    res = UA_Server_getSessionRoleNames(server, &adminSessionId,
                                        &namesSize, &names);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(namesSize, 2);
    ck_assert_ptr_nonnull(names);

    UA_QualifiedName observerName = UA_QUALIFIEDNAME(0, "Observer");
    UA_QualifiedName operatorName = UA_QUALIFIEDNAME(0, "Operator");
    UA_Boolean foundObserver = false, foundOperator = false;
    for(size_t i = 0; i < namesSize; i++) {
        if(UA_QualifiedName_equal(&names[i], &observerName)) foundObserver = true;
        if(UA_QualifiedName_equal(&names[i], &operatorName)) foundOperator = true;
    }
    ck_assert(foundObserver);
    ck_assert(foundOperator);

    UA_Array_delete(names, namesSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);

    /* Invalid session */
    UA_NodeId badSession = UA_NODEID_NUMERIC(0, 999999);
    res = UA_Server_getSessionRoleNames(server, &badSession,
                                        &namesSize, &names);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);

    /* Clean up */
    UA_Server_deleteSessionAttribute(server, &adminSessionId,
                                     UA_QUALIFIEDNAME(0, "roles"));
}
END_TEST

/* Test free-slot reuse: adding permissions, removing them (refCount→0),
 * then adding different permissions reuses the freed slot index. */
START_TEST(permissionEntry_slotReuse) {
    UA_NodeId roleId;
    UA_StatusCode res = addTestRole("SlotRole", 1, 51080, &roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Create two nodes */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SlotVar1");
    UA_Int32 val = 0;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    UA_NodeId node1;
    res = UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SlotVar1"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &node1);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SlotVar2");
    UA_NodeId node2;
    res = UA_Server_addVariableNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SlotVar2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &node2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Assign BROWSE permissions to node1 (creates an entry, refCount=1) */
    res = UA_Server_addRolePermissions(server, node1, roleId,
                                       UA_PERMISSIONTYPE_BROWSE, false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex idx1;
    res = UA_Server_getNodePermissionIndex(server, node1, &idx1);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(idx1 != UA_PERMISSION_INDEX_INVALID);

    /* Remove all permissions from node1 (refCount→0) */
    res = UA_Server_removeNodeRolePermissions(server, node1, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Assign DIFFERENT permissions to node2 — should reuse the freed slot */
    res = UA_Server_addRolePermissions(server, node2, roleId,
                                       UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE,
                                       false, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex idx2;
    res = UA_Server_getNodePermissionIndex(server, node2, &idx2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(idx2 != UA_PERMISSION_INDEX_INVALID);

    /* The new entry should have reused the old slot index */
    ck_assert_uint_eq(idx1, idx2);

    /* Verify the slot now has the new permissions */
    const UA_RolePermissionSet *rp = UA_Server_getRolePermissionConfig(server, idx2);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->rolePermissionsSize, 1);
    ck_assert_uint_eq(rp->rolePermissions[0].permissions,
                      UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE);

    UA_Server_deleteNode(server, node1, true);
    UA_Server_deleteNode(server, node2, true);
    UA_NodeId_clear(&node1);
    UA_NodeId_clear(&node2);
    removeTestRole("SlotRole", 1);
    UA_NodeId_clear(&roleId);
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
    tcase_add_test(tc_rm, removeRole_andVerifyGetRoles);
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

static Suite *testSuite_PermissionMapping(void) {
    Suite *s = suite_create("RBAC Permission Mapping");
    TCase *tc = tcase_create("PermMapping");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, effectivePermissions_logicalOR);
    tcase_add_test(tc, userRolePermissions_array);
    tcase_add_test(tc, permissionConfig_addAndGet);
    tcase_add_test(tc, permissionEntry_slotReuse);
    tcase_add_test(tc, allPermissionsForAnonymousRole_config);
    suite_add_tcase(s, tc);
    return s;
}

static Suite *testSuite_InformationModel(void) {
    Suite *s = suite_create("RBAC Information Model");
    TCase *tc = tcase_create("NS0");
    tcase_add_unchecked_fixture(tc, setup, teardown);
#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
    tcase_add_test(tc, roleSetExists);
    tcase_add_test(tc, standardRolesWithCorrectIds);
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */
    tcase_add_test(tc, getAllRoles_includesWellKnown);
    tcase_add_test(tc, protectMandatoryRoles);
    tcase_add_test(tc, allowModifyingOptionalRoles);
    tcase_add_test(tc, identityMapping_wellKnownRoles);
#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
    tcase_add_test(tc, wellKnownRoles_nodeFields);
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */
    tcase_add_test(tc, addedRole_ns0NodeFields);
    suite_add_tcase(s, tc);

    TCase *tc_session = tcase_create("SessionRoles");
    tcase_add_unchecked_fixture(tc_session, setup, teardown);
    tcase_add_test(tc_session, sessionRoleManagement);
    tcase_add_test(tc_session, addSessionRole);
    tcase_add_test(tc_session, sessionRoleNames);
    suite_add_tcase(s, tc_session);

    TCase *tc_perms = tcase_create("NodePermissions");
    tcase_add_unchecked_fixture(tc_perms, setup, teardown);
    tcase_add_test(tc_perms, nodePermissions_basic);
    tcase_add_test(tc_perms, nodePermissions_multipleRoles);
    tcase_add_test(tc_perms, nodePermissions_update);
    tcase_add_test(tc_perms, nodePermissions_invalidRole);
    tcase_add_test(tc_perms, nodePermissions_overwrite);
    tcase_add_test(tc_perms, nodePermissions_recursive);
    tcase_add_test(tc_perms, removePermissions_recursive);
    tcase_add_test(tc_perms, setPermissionIndex_recursive);
    tcase_add_test(tc_perms, nodePermissions_sharedConfig);
    tcase_add_test(tc_perms, recursivePermissions_onBuildInfo);
    suite_add_tcase(s, tc_perms);

    return s;
}

int main(void) {
    int number_failed = 0;
    SRunner *sr;

    sr = srunner_create(testSuite_RolTypeAPI());
    srunner_add_suite(sr, testSuite_RoleManagement());
    srunner_add_suite(sr, testSuite_ConfigRoles());
    srunner_add_suite(sr, testSuite_IdentityAppMgmt());
    srunner_add_suite(sr, testSuite_PermissionMapping());
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
