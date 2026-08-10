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
#include <stdio.h>
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

/* Create a role carrying a single identity mapping rule */
static UA_NodeId
addRoleWithRule(const char *name, UA_IdentityCriteriaType ct, const char *criteria)
{
    UA_Role role;
    UA_Role_init(&role);
    role.roleName = UA_QUALIFIEDNAME(1, (char*)(uintptr_t)name);
    UA_IdentityMappingRuleType rule;
    UA_IdentityMappingRuleType_init(&rule);
    rule.criteriaType = ct;
    rule.criteria = UA_STRING((char*)(uintptr_t)criteria);
    role.identityMappingRules = &rule;
    role.identityMappingRulesSize = 1;
    UA_NodeId id = UA_NODEID_NULL;
    UA_StatusCode res = UA_Server_addRole(server, &role, &id);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    return id;
}

/* Evaluate the roles for a context and report whether roleId is among them */
static UA_Boolean
roleGrantedForContext(const UA_SessionIdentityContext *ctx, const UA_NodeId *roleId)
{
    size_t size = 0;
    UA_NodeId *ids = NULL;
    ck_assert_uint_eq(UA_Server_evaluateSessionRoles(server, ctx, &size, &ids),
                      UA_STATUSCODE_GOOD);
    UA_Boolean found = false;
    for(size_t i = 0; i < size; i++) {
        if(UA_NodeId_equal(&ids[i], roleId)) {
            found = true;
            break;
        }
    }
    UA_Array_delete(ids, size, &UA_TYPES[UA_TYPES_NODEID]);
    return found;
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

/* Verify that a canonical Thumbprint criterion is accepted and retained. */
START_TEST(addRole_unsupportedCriteriaStored) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleName = UA_QUALIFIEDNAME(0, "ThumbprintRole");

    /* SHA-1 thumbprints are 40 uppercase hexadecimal characters. */
    UA_IdentityMappingRuleType rule;
    UA_IdentityMappingRuleType_init(&rule);
    rule.criteriaType = UA_IDENTITYCRITERIATYPE_THUMBPRINT;
    rule.criteria = UA_STRING("00112233445566778899AABBCCDDEEFF00112233");
    role.identityMappingRules = &rule;
    role.identityMappingRulesSize = 1;

    UA_NodeId outId;
    UA_StatusCode res = UA_Server_addRole(server, &role, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify the role was stored with the identity rule */
    UA_Role retrieved;
    res = UA_Server_getRole(server, UA_QUALIFIEDNAME(0, "ThumbprintRole"), &retrieved);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(retrieved.identityMappingRulesSize, 1);
    ck_assert_uint_eq(retrieved.identityMappingRules[0].criteriaType,
                      UA_IDENTITYCRITERIATYPE_THUMBPRINT);
    UA_Role_clear(&retrieved);
    removeTestRole("ThumbprintRole", 0);
    UA_NodeId_clear(&outId);
}
END_TEST

/* Verify that adding a role with application/endpoint filters succeeds
 * (stored but not evaluated for role assignment). */
START_TEST(addRole_applicationFiltersStored) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleName = UA_QUALIFIEDNAME(0, "AppFilterRole");

    /* Add application filter */
    UA_String app = UA_STRING("urn:example:app");
    role.applications = &app;
    role.applicationsSize = 1;
    role.applicationsExclude = false;

    UA_NodeId outId;
    UA_StatusCode res = UA_Server_addRole(server, &role, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Verify the role was stored with the application filter */
    UA_Role retrieved;
    res = UA_Server_getRole(server, UA_QUALIFIEDNAME(0, "AppFilterRole"), &retrieved);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(retrieved.applicationsSize, 1);
    ck_assert(UA_String_equal(&retrieved.applications[0], &app));
    UA_Role_clear(&retrieved);
    removeTestRole("AppFilterRole", 0);
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
    /* Protected (config) roles yield Bad_RequestNotAllowed per Part 18 §4.2.3 */
    ck_assert_uint_eq(res, UA_STATUSCODE_BADREQUESTNOTALLOWED);

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

/* UA_Server_updateRole identifies the Role by roleId, by roleName, or by both.
 * Content validation must not reintroduce a roleName requirement, which would
 * make the roleId-only form unreachable. */
START_TEST(updateRole_identifiedByEitherKey) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 62500);
    role.roleName = UA_QUALIFIEDNAME(1, "UpdateKeyRole");
    role.applicationsExclude = true;
    ck_assert_uint_eq(UA_Server_addRole(server, &role, NULL), UA_STATUSCODE_GOOD);

    /* roleId only */
    UA_Role byId;
    UA_Role_init(&byId);
    byId.roleId = role.roleId;
    byId.applicationsExclude = false;
    ck_assert_uint_eq(UA_Server_updateRole(server, &byId), UA_STATUSCODE_GOOD);

    UA_Role fetched;
    ck_assert_uint_eq(UA_Server_getRoleById(server, role.roleId, &fetched),
                      UA_STATUSCODE_GOOD);
    ck_assert(!fetched.applicationsExclude);
    /* The stored roleName is untouched by an update that does not carry one */
    ck_assert(UA_QualifiedName_equal(&fetched.roleName, &role.roleName));
    UA_Role_clear(&fetched);

    /* roleName only */
    UA_Role byName;
    UA_Role_init(&byName);
    byName.roleName = role.roleName;
    byName.applicationsExclude = true;
    ck_assert_uint_eq(UA_Server_updateRole(server, &byName), UA_STATUSCODE_GOOD);

    /* Neither key is still rejected */
    UA_Role neither;
    UA_Role_init(&neither);
    ck_assert_uint_eq(UA_Server_updateRole(server, &neither),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    /* addRole still requires a roleName */
    UA_Role unnamed;
    UA_Role_init(&unnamed);
    unnamed.roleId = UA_NODEID_NUMERIC(1, 62501);
    ck_assert_uint_eq(UA_Server_addRole(server, &unnamed, NULL),
                      UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_Server_removeRole(server, role.roleName);
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

    /* Cannot remove Anonymous role - Part 18 §4.2.3 returns Bad_RequestNotAllowed
     * for a Role that cannot be removed (the missing-Permissions case
     * Bad_UserAccessDenied is handled by checkRBACMethodAccess on the Method). */
    res = UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "Anonymous"));
    ck_assert_uint_eq(res, UA_STATUSCODE_BADREQUESTNOTALLOWED);

    /* Cannot update AuthenticatedUser */
    UA_NodeId authUserRoleId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    res = UA_Server_getRoleById(server, authUserRoleId, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_updateRole(server, &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_Role_clear(&role);

    res = UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "AuthenticatedUser"));
    ck_assert_uint_eq(res, UA_STATUSCODE_BADREQUESTNOTALLOWED);
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
    role.identityMappingRules[0].criteria = UA_STRING_ALLOC("observer");
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
/* Is targetId reachable from RoleSet via a forward HasComponent reference? */
static UA_Boolean
roleSetHasComponent(UA_NodeId targetId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET);
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_OBJECT;
    bd.resultMask = UA_BROWSERESULTMASK_NONE;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    UA_Boolean found = false;
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(UA_NodeId_equal(&br.references[i].nodeId.nodeId, &targetId)) {
            found = true;
            break;
        }
    }
    UA_BrowseResult_clear(&br);
    return found;
}

static UA_StatusCode
findRoleChild(UA_NodeId parentId, const char *name, UA_NodeClass nodeClass,
              UA_UInt32 referenceTypeId, UA_NodeId *childId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = parentId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, referenceTypeId);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = nodeClass;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    UA_StatusCode res = br.statusCode;
    if(res == UA_STATUSCODE_GOOD) {
        res = UA_STATUSCODE_BADNOTFOUND;
        UA_String want = UA_STRING((char*)(uintptr_t)name);
        for(size_t i = 0; i < br.referencesSize; i++) {
            if(UA_String_equal(&br.references[i].browseName.name, &want)) {
                res = UA_NodeId_copy(&br.references[i].nodeId.nodeId, childId);
                break;
            }
        }
    }
    UA_BrowseResult_clear(&br);
    return res;
}

static UA_StatusCode
findRoleProperty(UA_NodeId parentId, const char *name, UA_NodeId *childId) {
    return findRoleChild(parentId, name, UA_NODECLASS_VARIABLE,
                         UA_NS0ID_HASPROPERTY, childId);
}

static UA_StatusCode
findRoleMethod(UA_NodeId parentId, const char *name, UA_NodeId *childId) {
    return findRoleChild(parentId, name, UA_NODECLASS_METHOD,
                         UA_NS0ID_HASCOMPONENT, childId);
}

/* A role added/removed through the C API is mirrored under the RoleSet, the
 * same way other subsystems reflect their config in NS0. */
START_TEST(addRole_cApiPublishesRoleObject) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 55123);
    role.roleName = UA_QUALIFIEDNAME(1, "CApiRole");
    UA_NodeId outId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &role, &outId), UA_STATUSCODE_GOOD);

    UA_QualifiedName bn;
    ck_assert_uint_eq(UA_Server_readBrowseName(server, outId, &bn), UA_STATUSCODE_GOOD);
    UA_String expected = UA_STRING("CApiRole");
    ck_assert(UA_String_equal(&bn.name, &expected));
    UA_QualifiedName_clear(&bn);
    ck_assert(roleSetHasComponent(outId));

    /* Removing it through the C API drops the node again */
    ck_assert_uint_eq(UA_Server_removeRole(server, role.roleName), UA_STATUSCODE_GOOD);
    ck_assert(!roleSetHasComponent(outId));
    ck_assert_uint_ne(UA_Server_readBrowseName(server, outId, &bn), UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&outId);
}
END_TEST
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */

#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
/* The RoleSet AddRole/RemoveRole Methods must create/remove the Role Object
 * under Server/ServerCapabilities/RoleSet so a browsing client sees roles
 * added or removed at runtime (Part 18 §4.2.2, §4.2.3, §4.3). */
START_TEST(addRemoveRoleMethod_updatesAddressSpace) {
    UA_NodeId roleSetId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET);

    /* Call AddRole. An empty NamespaceUri maps to NS1 (per spec). */
    UA_String roleName = UA_STRING("RuntimeRole");
    UA_String nsUri = UA_STRING_NULL;
    UA_Variant addInput[2];
    UA_Variant_setScalar(&addInput[0], &roleName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&addInput[1], &nsUri, &UA_TYPES[UA_TYPES_STRING]);

    UA_CallMethodRequest addReq;
    UA_CallMethodRequest_init(&addReq);
    addReq.objectId = roleSetId;
    addReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET_ADDROLE);
    addReq.inputArguments = addInput;
    addReq.inputArgumentsSize = 2;

    UA_CallMethodResult addRes = UA_Server_call(server, &addReq);
    ck_assert_uint_eq(addRes.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(addRes.outputArgumentsSize, 1);
    ck_assert(addRes.outputArguments[0].type == &UA_TYPES[UA_TYPES_NODEID]);
    UA_NodeId newRoleId;
    ck_assert_uint_eq(UA_NodeId_copy((UA_NodeId*)addRes.outputArguments[0].data,
                                     &newRoleId), UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&addRes);

    /* The new Role Object is now browseable as a HasComponent of the RoleSet */
    UA_QualifiedName bn;
    ck_assert_uint_eq(UA_Server_readBrowseName(server, newRoleId, &bn),
                      UA_STATUSCODE_GOOD);
    UA_String expected = UA_STRING("RuntimeRole");
    ck_assert(UA_String_equal(&bn.name, &expected));
    UA_QualifiedName_clear(&bn);
    ck_assert(roleSetHasComponent(newRoleId));

    /* Call RemoveRole with the assigned NodeId */
    UA_Variant rmInput;
    UA_Variant_setScalar(&rmInput, &newRoleId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_CallMethodRequest rmReq;
    UA_CallMethodRequest_init(&rmReq);
    rmReq.objectId = roleSetId;
    rmReq.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET_REMOVEROLE);
    rmReq.inputArguments = &rmInput;
    rmReq.inputArgumentsSize = 1;

    UA_CallMethodResult rmRes = UA_Server_call(server, &rmReq);
    ck_assert_uint_eq(rmRes.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&rmRes);

    /* The Role Object is gone from the AddressSpace again */
    ck_assert_uint_ne(UA_Server_readBrowseName(server, newRoleId, &bn),
                      UA_STATUSCODE_GOOD);
    ck_assert(!roleSetHasComponent(newRoleId));
    UA_NodeId_clear(&newRoleId);
}
END_TEST
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL && UA_ENABLE_METHODCALLS */

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

/* The well-known TrustedApplication role is registered, protected and carries
 * the TrustedApplication identity criteria (Part 18 §4.3). */
START_TEST(trustedApplication_roleRegistered) {
    UA_Role role;
    UA_StatusCode res = UA_Server_getRole(server,
                                          UA_QUALIFIEDNAME(0, "TrustedApplication"),
                                          &role);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId expectedId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_TRUSTEDAPPLICATION);
    ck_assert(UA_NodeId_equal(&role.roleId, &expectedId));

    UA_Boolean hasTA = false;
    for(size_t i = 0; i < role.identityMappingRulesSize; i++)
        if(role.identityMappingRules[i].criteriaType ==
           UA_IDENTITYCRITERIATYPE_TRUSTEDAPPLICATION)
            hasTA = true;
    ck_assert(hasTA);
    UA_Role_clear(&role);

    /* Per spec the role must not be removable - Bad_RequestNotAllowed per
     * Part 18 §4.2.3 Table 3 (the missing-Permissions case is handled by
     * checkRBACMethodAccess on the Method entry point). */
    res = UA_Server_removeRole(server, UA_QUALIFIEDNAME(0, "TrustedApplication"));
    ck_assert_uint_eq(res, UA_STATUSCODE_BADREQUESTNOTALLOWED);
}
END_TEST

/* An anonymous session is granted the TrustedApplication role only when the
 * client application is trusted (encrypted SecureChannel). */
START_TEST(trustedApplication_assignedWhenTrusted) {
    UA_NodeId taId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_TRUSTEDAPPLICATION);

    /* Trusted application -> role is assigned */
    UA_SessionIdentityContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.isAnonymous = true;
    ctx.trustedApplication = true;

    size_t size = 0;
    UA_NodeId *ids = NULL;
    UA_StatusCode res =
        UA_Server_evaluateSessionRoles(server, &ctx, &size, &ids);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_Boolean found = false;
    for(size_t i = 0; i < size; i++)
        if(UA_NodeId_equal(&ids[i], &taId))
            found = true;
    ck_assert(found);
    UA_Array_delete(ids, size, &UA_TYPES[UA_TYPES_NODEID]);

    /* Untrusted application -> role is not assigned */
    ctx.trustedApplication = false;
    size = 0;
    ids = NULL;
    res = UA_Server_evaluateSessionRoles(server, &ctx, &size, &ids);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    found = false;
    for(size_t i = 0; i < size; i++)
        if(UA_NodeId_equal(&ids[i], &taId))
            found = true;
    ck_assert(!found);
    UA_Array_delete(ids, size, &UA_TYPES[UA_TYPES_NODEID]);
}
END_TEST

/* The Anonymous Role is assigned to every Session regardless of the identity
 * token (Part 18 §4.3). */
START_TEST(anonymousRole_alwaysAssigned) {
    UA_NodeId anonId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);

    /* Anonymous identity token */
    UA_SessionIdentityContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.isAnonymous = true;

    size_t size = 0;
    UA_NodeId *ids = NULL;
    ck_assert_uint_eq(UA_Server_evaluateSessionRoles(server, &ctx,
                                                     &size, &ids),
                      UA_STATUSCODE_GOOD);
    UA_Boolean found = false;
    for(size_t i = 0; i < size; i++)
        if(UA_NodeId_equal(&ids[i], &anonId))
            found = true;
    ck_assert(found);
    UA_Array_delete(ids, size, &UA_TYPES[UA_TYPES_NODEID]);

    /* Authenticated (username) session still receives the Anonymous Role */
    memset(&ctx, 0, sizeof(ctx));
    ctx.isAnonymous = false;
    ctx.userName = UA_STRING("nobody");

    size = 0;
    ids = NULL;
    ck_assert_uint_eq(UA_Server_evaluateSessionRoles(server, &ctx,
                                                     &size, &ids),
                      UA_STATUSCODE_GOOD);
    found = false;
    for(size_t i = 0; i < size; i++)
        if(UA_NodeId_equal(&ids[i], &anonId))
            found = true;
    ck_assert(found);
    UA_Array_delete(ids, size, &UA_TYPES[UA_TYPES_NODEID]);
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

/* The Identities property of the well-known role nodes is backed by the role
 * registry: reads return the currently configured identity mapping rules. */
START_TEST(wellKnownRoles_identitiesFromRegistry) {
    /* Anonymous carries its two default rules (Anonymous, AuthenticatedUser) */
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS_IDENTITIES), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(v.type == &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]);
    ck_assert_uint_eq(v.arrayLength, 2);
    UA_IdentityMappingRuleType *rules = (UA_IdentityMappingRuleType*)v.data;
    ck_assert_uint_eq(rules[0].criteriaType, UA_IDENTITYCRITERIATYPE_ANONYMOUS);
    ck_assert_uint_eq(rules[1].criteriaType, UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER);
    UA_Variant_clear(&v);

    /* Observer starts without rules */
    res = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER_IDENTITIES), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v.arrayLength, 0);
    UA_Variant_clear(&v);

    /* A rule added to the registry shows up in the NS0 value */
    UA_Role observer;
    res = UA_Server_getRole(server, UA_QUALIFIEDNAME(0, "Observer"), &observer);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_IdentityMappingRuleType *newRules = (UA_IdentityMappingRuleType*)
        UA_realloc(observer.identityMappingRules,
                   (observer.identityMappingRulesSize + 1) *
                   sizeof(UA_IdentityMappingRuleType));
    ck_assert_ptr_nonnull(newRules);
    observer.identityMappingRules = newRules;
    UA_IdentityMappingRuleType_init(&newRules[observer.identityMappingRulesSize]);
    newRules[observer.identityMappingRulesSize].criteriaType =
        UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    observer.identityMappingRulesSize++;
    res = UA_Server_updateRole(server, &observer);
    UA_Role_clear(&observer);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER_IDENTITIES), &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v.arrayLength, 1);
    rules = (UA_IdentityMappingRuleType*)v.data;
    ck_assert_uint_eq(rules[0].criteriaType,
                      UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER);
    UA_Variant_clear(&v);

    /* Restore Observer without identity mapping rules */
    res = UA_Server_getRole(server, UA_QUALIFIEDNAME(0, "Observer"), &observer);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < observer.identityMappingRulesSize; i++)
        UA_IdentityMappingRuleType_clear(&observer.identityMappingRules[i]);
    UA_free(observer.identityMappingRules);
    observer.identityMappingRules = NULL;
    observer.identityMappingRulesSize = 0;
    res = UA_Server_updateRole(server, &observer);
    UA_Role_clear(&observer);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
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

START_TEST(namespaceDefault_setAndGet) {
    UA_NodeId roleId;
    UA_StatusCode res = addTestRole("NsDefaultRole", 1, 51040, &roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_RolePermission entry;
    res = UA_NodeId_copy(&roleId, &entry.roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    entry.permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;

    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 1, &entry);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&entry.roleId);

    size_t retrievedSize = 0;
    UA_RolePermission *retrievedEntries = NULL;
    res = UA_Server_getNamespaceDefaultRolePermissions(server, 1,
                                                       &retrievedSize,
                                                       &retrievedEntries);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(retrievedSize, 1);
    ck_assert_ptr_nonnull(retrievedEntries);
    ck_assert(UA_NodeId_equal(&retrievedEntries[0].roleId, &roleId));
    ck_assert_uint_eq(retrievedEntries[0].permissions,
                      UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
    for(size_t i = 0; i < retrievedSize; i++)
        UA_NodeId_clear(&retrievedEntries[i].roleId);
    UA_free(retrievedEntries);

    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 0, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    retrievedEntries = NULL;
    res = UA_Server_getNamespaceDefaultRolePermissions(server, 1,
                                                       &retrievedSize,
                                                       &retrievedEntries);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(retrievedSize, 0);
    ck_assert_ptr_null(retrievedEntries);

    removeTestRole("NsDefaultRole", 1);
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(namespaceDefault_explicitOverrides) {
    UA_NodeId roleId;
    UA_StatusCode res = addTestRole("OverrideRole", 1, 51060, &roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_RolePermission defaultEntry;
    res = UA_NodeId_copy(&roleId, &defaultEntry.roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    defaultEntry.permissions = UA_PERMISSIONTYPE_BROWSE;
    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 1, &defaultEntry);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&defaultEntry.roleId);

    UA_NodeId newNodeId = UA_NODEID_STRING(1, "NodeWithExplicitPerms");
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Node With Explicit Permissions");
    res = UA_Server_addObjectNode(server, newNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "NodeWithExplicitPerms"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_addRolePermissions(server, newNodeId, roleId,
        UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE,
        true, false);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionIndex explicitPermIdx;
    res = UA_Server_getNodePermissionIndex(server, newNodeId, &explicitPermIdx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(explicitPermIdx != UA_PERMISSION_INDEX_INVALID);

    UA_NodeId adminSessionId = UA_NODEID_GUID(0,
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId rolesToSet[1] = {roleId};
    UA_Variant rv;
    UA_Variant_setArray(&rv, rolesToSet, 1, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &rv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionType eff = 0;
    res = UA_Server_getEffectivePermissions(server, &adminSessionId,
                                            &newNodeId, &eff);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_msg((eff & UA_PERMISSIONTYPE_BROWSE) != 0,
                  "Explicit permissions must keep BROWSE (eff=0x%08x)", eff);
    ck_assert_msg((eff & UA_PERMISSIONTYPE_READ) != 0,
                  "Explicit permissions must keep READ (eff=0x%08x)", eff);
    ck_assert_msg((eff & UA_PERMISSIONTYPE_WRITE) != 0,
                  "Explicit permissions must keep WRITE (eff=0x%08x)", eff);

    (void)UA_Server_deleteSessionAttribute(server, &adminSessionId,
                                           UA_QUALIFIEDNAME(0, "roles"));

    UA_Server_deleteNode(server, newNodeId, true);
    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 0, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    removeTestRole("OverrideRole", 1);
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(namespaceDefault_effectiveFallback) {
    UA_NodeId roleId;
    UA_StatusCode res = addTestRole("NsFallbackRole", 1, 51061, &roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_RolePermission defaultEntry;
    res = UA_NodeId_copy(&roleId, &defaultEntry.roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    defaultEntry.permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 1, &defaultEntry);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&defaultEntry.roleId);

    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "NsDefaultFallbackVar");
    UA_Int32 v = 1;
    UA_Variant_setScalar(&attr.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    UA_NodeId testNodeId = UA_NODEID_STRING(1, "NsDefaultFallbackVar");
    res = UA_Server_addVariableNode(server, testNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "NsDefaultFallbackVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId adminSessionId = UA_NODEID_GUID(0,
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId rolesToSet[1] = {roleId};
    UA_Variant rv;
    UA_Variant_setArray(&rv, rolesToSet, 1, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &rv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionType eff = 0;
    res = UA_Server_getEffectivePermissions(server, &adminSessionId,
                                            &testNodeId, &eff);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_msg((eff & UA_PERMISSIONTYPE_BROWSE) != 0,
                  "Namespace default fallback must grant BROWSE (eff=0x%08x)", eff);
    ck_assert_msg((eff & UA_PERMISSIONTYPE_READ) != 0,
                  "Namespace default fallback must grant READ (eff=0x%08x)", eff);
    ck_assert_msg((eff & UA_PERMISSIONTYPE_WRITE) == 0,
                  "Namespace default fallback must not grant WRITE (eff=0x%08x)", eff);

    (void)UA_Server_deleteSessionAttribute(server, &adminSessionId,
                                           UA_QUALIFIEDNAME(0, "roles"));
    UA_Server_deleteNode(server, testNodeId, true);
    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 0, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    removeTestRole("NsFallbackRole", 1);
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(namespaceDefault_noRoleMatchDenied) {
    UA_NodeId defaultRoleId;
    UA_NodeId otherRoleId;
    UA_StatusCode res = addTestRole("NsDefaultRoleNoMatch", 1, 51062, &defaultRoleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = addTestRole("NsOtherRoleNoMatch", 1, 51063, &otherRoleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_RolePermission defaultEntry;
    res = UA_NodeId_copy(&defaultRoleId, &defaultEntry.roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    defaultEntry.permissions = UA_PERMISSIONTYPE_BROWSE;
    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 1, &defaultEntry);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&defaultEntry.roleId);

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "NsNoMatchObj");
    UA_NodeId nodeId = UA_NODEID_STRING(1, "NsNoMatchObj");
    res = UA_Server_addObjectNode(server, nodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "NsNoMatchObj"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId adminSessionId = UA_NODEID_GUID(0,
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId rolesToSet[1] = {otherRoleId};
    UA_Variant rv;
    UA_Variant_setArray(&rv, rolesToSet, 1, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &rv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionType eff = UA_PERMISSIONTYPE_ALL;
    res = UA_Server_getEffectivePermissions(server, &adminSessionId, &nodeId, &eff);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_msg(eff == 0,
                  "Configured namespace defaults with no matching role must deny "
                  "(eff=0x%08x)", eff);

    (void)UA_Server_deleteSessionAttribute(server, &adminSessionId,
                                           UA_QUALIFIEDNAME(0, "roles"));
    UA_Server_deleteNode(server, nodeId, true);
    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 0, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    removeTestRole("NsDefaultRoleNoMatch", 1);
    removeTestRole("NsOtherRoleNoMatch", 1);
    UA_NodeId_clear(&defaultRoleId);
    UA_NodeId_clear(&otherRoleId);
}
END_TEST

START_TEST(namespaceDefault_perNamespaceIsolation) {
    UA_NodeId roleId;
    UA_StatusCode res = addTestRole("NsIsolationRole", 1, 51064, &roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_UInt16 ns2 = UA_Server_addNamespace(server, "urn:open62541:test:rbac:ns2");
    ck_assert_msg(ns2 > 1, "Expected a dynamic namespace index > 1 (got %u)", ns2);

    UA_RolePermission ns1Entry;
    res = UA_NodeId_copy(&roleId, &ns1Entry.roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ns1Entry.permissions = UA_PERMISSIONTYPE_BROWSE;
    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 1, &ns1Entry);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&ns1Entry.roleId);

    UA_RolePermission ns2Entry;
    res = UA_NodeId_copy(&roleId, &ns2Entry.roleId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ns2Entry.permissions = UA_PERMISSIONTYPE_READ;
    res = UA_Server_setNamespaceDefaultRolePermissions(server, ns2, 1, &ns2Entry);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&ns2Entry.roleId);

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Ns1Obj");
    UA_NodeId nodeNs1 = UA_NODEID_STRING(1, "Ns1DefaultObj");
    res = UA_Server_addObjectNode(server, nodeNs1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Ns1DefaultObj"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Ns2Obj");
    UA_NodeId nodeNs2 = UA_NODEID_STRING(ns2, "Ns2DefaultObj");
    res = UA_Server_addObjectNode(server, nodeNs2,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(ns2, "Ns2DefaultObj"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId adminSessionId = UA_NODEID_GUID(0,
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_NodeId rolesToSet[1] = {roleId};
    UA_Variant rv;
    UA_Variant_setArray(&rv, rolesToSet, 1, &UA_TYPES[UA_TYPES_NODEID]);
    res = UA_Server_setSessionAttribute(server, &adminSessionId,
                                        UA_QUALIFIEDNAME(0, "roles"), &rv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_PermissionType eff1 = 0, eff2 = 0;
    res = UA_Server_getEffectivePermissions(server, &adminSessionId, &nodeNs1, &eff1);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_getEffectivePermissions(server, &adminSessionId, &nodeNs2, &eff2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    ck_assert_msg((eff1 & UA_PERMISSIONTYPE_BROWSE) != 0 &&
                  (eff1 & UA_PERMISSIONTYPE_READ) == 0,
                  "Namespace 1 defaults must apply only ns1 mapping (eff1=0x%08x)", eff1);
    ck_assert_msg((eff2 & UA_PERMISSIONTYPE_READ) != 0 &&
                  (eff2 & UA_PERMISSIONTYPE_BROWSE) == 0,
                  "Namespace 2 defaults must apply only ns2 mapping (eff2=0x%08x)", eff2);

    (void)UA_Server_deleteSessionAttribute(server, &adminSessionId,
                                           UA_QUALIFIEDNAME(0, "roles"));
    UA_Server_deleteNode(server, nodeNs1, true);
    UA_Server_deleteNode(server, nodeNs2, true);
    res = UA_Server_setNamespaceDefaultRolePermissions(server, 1, 0, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_setNamespaceDefaultRolePermissions(server, ns2, 0, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    removeTestRole("NsIsolationRole", 1);
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(namespaceDefault_invalidNamespaceIndex) {
    size_t entriesSize = 0;
    UA_RolePermission *entries = NULL;

    UA_StatusCode res = UA_Server_setNamespaceDefaultRolePermissions(server,
                                                                      (UA_UInt16)65535,
                                                                      0, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINDEXRANGEINVALID);

    res = UA_Server_getNamespaceDefaultRolePermissions(server,
                                                       (UA_UInt16)65535,
                                                       &entriesSize,
                                                       &entries);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADINDEXRANGEINVALID);
    ck_assert_uint_eq(entriesSize, 0);
    ck_assert_ptr_null(entries);
}
END_TEST

START_TEST(namespaceDefault_explicitEmptyDenies) {
    ck_assert(UA_Server_getConfig(server)->allPermissionsForAnonymous);
    UA_NodeId nodeId = UA_NODEID_NUMERIC(1, 51099);
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    ck_assert_uint_eq(UA_Server_addObjectNode(
        server, nodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ExplicitEmptyNamespaceDefault"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), attr, NULL, NULL),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_setNamespaceDefaultRolePermissions(
        server, 1, 0, NULL), UA_STATUSCODE_GOOD);
    UA_PermissionType effective = UA_PERMISSIONTYPE_ALL;
    ck_assert_uint_eq(UA_Server_getEffectivePermissions(
        server, NULL, &nodeId, &effective), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(effective, 0);
    UA_Server_deleteNode(server, nodeId, true);
}
END_TEST

START_TEST(allPermissionsForAnonymous_config) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    ck_assert(config->allPermissionsForAnonymous == true);
    config->allPermissionsForAnonymous = false;
    ck_assert(config->allPermissionsForAnonymous == false);
    config->allPermissionsForAnonymous = true;
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
    res = UA_Server_getSessionRoleNames(server, adminSessionId,
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
    res = UA_Server_getSessionRoleNames(server, badSession,
                                        &namesSize, &names);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);

    /* Clean up */
    UA_Server_deleteSessionAttribute(server, &adminSessionId,
                                     UA_QUALIFIEDNAME(0, "roles"));
}
END_TEST

/* Adding permissions, removing them (refCount→0), then adding a different
 * permission set must NOT recycle the freed slot: copied nodes (e.g. type
 * children instantiated into objects) can still reference the slot without
 * being counted, so rewriting it would corrupt their permissions.
 * The freed slot is left untouched and the new set gets a fresh entry. */
START_TEST(permissionEntry_slotNoUnsafeReuse) {
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

    /* The freed slot must NOT be recycled for the different permission set */
    ck_assert_uint_ne(idx1, idx2);

    /* Verify the new slot carries the new permissions */
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

/* Removing a Role must delete all RolePermission entries that reference it, so
 * no stale roleId lingers in a shared (deduplicated) permission entry. */
START_TEST(removeRole_purgesRolePermissions) {
    UA_NodeId purgeRoleId;
    ck_assert_uint_eq(addTestRole("PurgeRole", 1, 55321, &purgeRoleId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId observerId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);

    /* A plain Variable node carrying permissions for both Roles */
    UA_NodeId nodeId = UA_NODEID_NUMERIC(1, 60001);
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_UInt32 val = 7;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(UA_Server_addVariableNode(server, nodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "PurgeVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL), UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_Server_addRolePermissions(server, nodeId, purgeRoleId,
        UA_PERMISSIONTYPE_READ, false, false), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, nodeId, observerId,
        UA_PERMISSIONTYPE_BROWSE, false, false), UA_STATUSCODE_GOOD);

    /* A second Node has only the Role that will be removed. Its resulting
     * empty permission set must stay deny-all even with the permissive
     * unconfigured-node compatibility option enabled. */
    UA_NodeId denyNodeId = UA_NODEID_NUMERIC(1, 60002);
    ck_assert_uint_eq(UA_Server_addVariableNode(server, denyNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "PurgeDenyVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_addRolePermissions(server, denyNodeId, purgeRoleId,
        UA_PERMISSIONTYPE_READ, false, false), UA_STATUSCODE_GOOD);

    UA_NodeId sessionId = UA_NODEID_GUID(0,
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    UA_Variant rolesValue;
    UA_Variant_setArray(&rolesValue, &purgeRoleId, 1,
                        &UA_TYPES[UA_TYPES_NODEID]);
    ck_assert_uint_eq(UA_Server_setSessionAttribute(server, &sessionId,
        UA_QUALIFIEDNAME(0, "roles"), &rolesValue), UA_STATUSCODE_GOOD);

    UA_PermissionIndex idx;
    ck_assert_uint_eq(UA_Server_getNodePermissionIndex(server, nodeId, &idx),
                      UA_STATUSCODE_GOOD);
    const UA_RolePermissionSet *set = UA_Server_getRolePermissionConfig(server, idx);
    ck_assert_ptr_nonnull(set);
    ck_assert_uint_eq(set->rolePermissionsSize, 2);

    /* Remove the Role -> its permission entry must be purged, Observer stays */
    ck_assert_uint_eq(removeTestRole("PurgeRole", 1), UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_Server_getNodePermissionIndex(server, nodeId, &idx),
                      UA_STATUSCODE_GOOD);
    set = UA_Server_getRolePermissionConfig(server, idx);
    ck_assert_ptr_nonnull(set);
    ck_assert_uint_eq(set->rolePermissionsSize, 1);
    ck_assert(UA_NodeId_equal(&set->rolePermissions[0].roleId, &observerId));

    UA_PermissionType effective = UA_PERMISSIONTYPE_ALL;
    ck_assert_uint_eq(UA_Server_getEffectivePermissions(server, &sessionId,
        &denyNodeId, &effective), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(effective, 0);

    UA_Server_deleteNode(server, nodeId, true);
    UA_Server_deleteNode(server, denyNodeId, true);
    (void)UA_Server_deleteSessionAttribute(server, &sessionId,
                                           UA_QUALIFIEDNAME(0, "roles"));
    UA_NodeId_clear(&purgeRoleId);
}
END_TEST

/* AddRole is bounded by UA_RBAC_MAX_ROLES to prevent unbounded growth. */
START_TEST(addRole_quotaEnforced) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(UA_UInt32 i = 0; i < UA_RBAC_MAX_ROLES + 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "QuotaRole%u", i);
        UA_Role role;
        UA_Role_init(&role);
        role.roleName = UA_QUALIFIEDNAME(1, name);
        res = UA_Server_addRole(server, &role, NULL);
        if(res != UA_STATUSCODE_GOOD)
            break;
    }
    ck_assert_uint_eq(res, UA_STATUSCODE_BADTOOMANYOPERATIONS);

    size_t rolesSize = 0;
    UA_QualifiedName *names = NULL;
    ck_assert_uint_eq(UA_Server_getRoles(server, &rolesSize, &names),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_le(rolesSize, UA_RBAC_MAX_ROLES);
    for(size_t i = 0; i < rolesSize; i++)
        UA_QualifiedName_clear(&names[i]);
    UA_free(names);
}
END_TEST

#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
/* The RoleSet AddRole Method must only grant CALL to SecurityAdmin, while the
 * public Roles keep BROWSE only (Part 18). */
START_TEST(roleSetMethods_restrictedToAdmin) {
    UA_NodeId addRoleId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET_ADDROLE);
    UA_PermissionIndex idx;
    ck_assert_uint_eq(UA_Server_getNodePermissionIndex(server, addRoleId, &idx),
                      UA_STATUSCODE_GOOD);
    const UA_RolePermissionSet *set = UA_Server_getRolePermissionConfig(server, idx);
    ck_assert_ptr_nonnull(set);

    UA_NodeId secAdmin = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    UA_NodeId anon = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    UA_Boolean adminCanCall = false, anonCanCall = false;
    for(size_t i = 0; i < set->rolePermissionsSize; i++) {
        if(UA_NodeId_equal(&set->rolePermissions[i].roleId, &secAdmin) &&
           (set->rolePermissions[i].permissions & UA_PERMISSIONTYPE_CALL))
            adminCanCall = true;
        if(UA_NodeId_equal(&set->rolePermissions[i].roleId, &anon) &&
           (set->rolePermissions[i].permissions & UA_PERMISSIONTYPE_CALL))
            anonCanCall = true;
    }
    ck_assert(adminCanCall);
    ck_assert(!anonCanCall);
}
END_TEST
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL && UA_ENABLE_METHODCALLS */

/* The Thumbprint, X509Subject, Application and access-token Role criteria are
 * evaluated during role resolution (Part 18 §4.4.2). */
START_TEST(identityCriteria_extended) {
    UA_NodeId thumb = addRoleWithRule(
        "ThumbRole", UA_IDENTITYCRITERIATYPE_THUMBPRINT,
        "00112233445566778899AABBCCDDEEFF00112233");
    UA_NodeId subj = addRoleWithRule("SubjRole",
                                     UA_IDENTITYCRITERIATYPE_X509SUBJECT,
                                     "CN=\"alice\"");
    UA_NodeId app = addRoleWithRule("AppRole",
                                    UA_IDENTITYCRITERIATYPE_APPLICATION, "urn:app:x");
    UA_NodeId tokenRole = addRoleWithRule("TokenRole",
                                          UA_IDENTITYCRITERIATYPE_ROLE,
                                          "issuer.example/operator");
    /* This must not match merely because AppRole is assigned. */
    UA_NodeId notAChain = addRoleWithRule("NotAChain",
                                          UA_IDENTITYCRITERIATYPE_ROLE,
                                          "AppRole");

    UA_SessionIdentityContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.userThumbprint =
        UA_STRING("00112233445566778899aabbccddeeff00112233");
    ctx.userSubject = UA_STRING("CN=\"alice\"");
    ctx.applicationUri = UA_STRING("urn:app:x");
    ctx.trustedApplication = true;
    UA_String claims[] = {UA_STRING("issuer.example/operator")};
    ctx.tokenRoles = claims;
    ctx.tokenRolesSize = 1;

    size_t size = 0;
    UA_NodeId *ids = NULL;
    ck_assert_uint_eq(UA_Server_evaluateSessionRoles(server, &ctx, &size, &ids),
                      UA_STATUSCODE_GOOD);
    UA_Boolean fThumb = false, fSubj = false, fApp = false;
    UA_Boolean fToken = false, fNotAChain = false;
    for(size_t i = 0; i < size; i++) {
        if(UA_NodeId_equal(&ids[i], &thumb)) fThumb = true;
        if(UA_NodeId_equal(&ids[i], &subj)) fSubj = true;
        if(UA_NodeId_equal(&ids[i], &app)) fApp = true;
        if(UA_NodeId_equal(&ids[i], &tokenRole)) fToken = true;
        if(UA_NodeId_equal(&ids[i], &notAChain)) fNotAChain = true;
    }
    ck_assert(fThumb);
    ck_assert(fSubj);
    ck_assert(fApp);
    ck_assert(fToken);
    ck_assert(!fNotAChain);
    UA_Array_delete(ids, size, &UA_TYPES[UA_TYPES_NODEID]);

    /* A client-declared ApplicationUri without an authenticated application
     * certificate must not satisfy the Application criterion. */
    ctx.trustedApplication = false;
    ck_assert(!roleGrantedForContext(&ctx, &app));
    ctx.trustedApplication = true;

    /* A context with none of the values matches none of the four roles */
    UA_SessionIdentityContext empty;
    memset(&empty, 0, sizeof(empty));
    size = 0;
    ids = NULL;
    ck_assert_uint_eq(UA_Server_evaluateSessionRoles(server, &empty, &size, &ids),
                      UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < size; i++) {
        ck_assert(!UA_NodeId_equal(&ids[i], &thumb));
        ck_assert(!UA_NodeId_equal(&ids[i], &subj));
        ck_assert(!UA_NodeId_equal(&ids[i], &app));
        ck_assert(!UA_NodeId_equal(&ids[i], &tokenRole));
        ck_assert(!UA_NodeId_equal(&ids[i], &notAChain));
    }
    UA_Array_delete(ids, size, &UA_TYPES[UA_TYPES_NODEID]);

    UA_NodeId_clear(&thumb);
    UA_NodeId_clear(&subj);
    UA_NodeId_clear(&app);
    UA_NodeId_clear(&tokenRole);
    UA_NodeId_clear(&notAChain);
}
END_TEST

/* The Application and Endpoint role filters gate role assignment (Part 18
 * §4.4.1), including the Exclude variants. */
START_TEST(roleFilters_evaluated) {
    UA_IdentityMappingRuleType authRule;
    UA_IdentityMappingRuleType_init(&authRule);
    authRule.criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;

    /* Application include filter */
    UA_Role incl;
    UA_Role_init(&incl);
    incl.roleName = UA_QUALIFIEDNAME(1, "AppInclude");
    incl.identityMappingRules = &authRule;
    incl.identityMappingRulesSize = 1;
    UA_String allowed = UA_STRING("urn:allowed");
    incl.applications = &allowed;
    incl.applicationsSize = 1;
    incl.applicationsExclude = false;
    UA_NodeId inclId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &incl, &inclId), UA_STATUSCODE_GOOD);

    /* Application exclude filter */
    UA_Role excl;
    UA_Role_init(&excl);
    excl.roleName = UA_QUALIFIEDNAME(1, "AppExclude");
    excl.identityMappingRules = &authRule;
    excl.identityMappingRulesSize = 1;
    UA_String blocked = UA_STRING("urn:blocked");
    excl.applications = &blocked;
    excl.applicationsSize = 1;
    excl.applicationsExclude = true;
    UA_NodeId exclId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &excl, &exclId), UA_STATUSCODE_GOOD);

    /* Endpoint include filter */
    UA_Role ep;
    UA_Role_init(&ep);
    ep.roleName = UA_QUALIFIEDNAME(1, "EpInclude");
    ep.identityMappingRules = &authRule;
    ep.identityMappingRulesSize = 1;
    UA_EndpointType epFilter;
    UA_EndpointType_init(&epFilter);
    epFilter.endpointUrl = UA_STRING("opc.tcp://host:4840");
    ep.endpoints = &epFilter;
    ep.endpointsSize = 1;
    ep.endpointsExclude = false;
    UA_NodeId epId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &ep, &epId), UA_STATUSCODE_GOOD);

    UA_SessionIdentityContext ctx;

    /* Include: matching application granted, others denied */
    memset(&ctx, 0, sizeof(ctx));
    ctx.applicationUri = UA_STRING("urn:allowed");
    ctx.trustedApplication = true;
    ck_assert(roleGrantedForContext(&ctx, &inclId));
    ctx.applicationUri = UA_STRING("urn:other");
    ck_assert(!roleGrantedForContext(&ctx, &inclId));

    /* Exclude: listed application denied, others granted */
    ctx.applicationUri = UA_STRING("urn:blocked");
    ck_assert(!roleGrantedForContext(&ctx, &exclId));
    ctx.applicationUri = UA_STRING("urn:other");
    ck_assert(roleGrantedForContext(&ctx, &exclId));

    /* A matching self-declared URI is insufficient without a trusted client
     * certificate and signed SecureChannel. */
    ctx.applicationUri = UA_STRING("urn:allowed");
    ctx.trustedApplication = false;
    ck_assert(!roleGrantedForContext(&ctx, &inclId));

    /* Endpoint include: matching endpoint granted, others denied */
    memset(&ctx, 0, sizeof(ctx));
    ctx.endpointUrl = UA_STRING("opc.tcp://host:4840");
    ck_assert(roleGrantedForContext(&ctx, &epId));
    ctx.endpointUrl = UA_STRING("opc.tcp://other:4840");
    ck_assert(!roleGrantedForContext(&ctx, &epId));

    UA_NodeId_clear(&inclId);
    UA_NodeId_clear(&exclId);
    UA_NodeId_clear(&epId);
}
END_TEST

/* The GroupId criterion matches the GroupIds captured for the session
 * (Part 18 §4.4.2). */
START_TEST(identityCriteria_groupId) {
    UA_NodeId grp = addRoleWithRule("GroupRole",
                                    UA_IDENTITYCRITERIATYPE_GROUPID, "admins");

    UA_String inGroup[1] = { UA_STRING("admins") };
    UA_SessionIdentityContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.groups = inGroup;
    ctx.groupsSize = 1;
    ck_assert(roleGrantedForContext(&ctx, &grp));

    UA_String otherGroup[1] = { UA_STRING("users") };
    ctx.groups = otherGroup;
    ck_assert(!roleGrantedForContext(&ctx, &grp));

    UA_NodeId_clear(&grp);
}
END_TEST

/* AccessRestrictions can be set/read via the C API and the attribute service,
 * and fall back to the namespace default (Part 3 §5.2.11). */
START_TEST(accessRestrictions_setGetRead) {
    UA_NodeId x = UA_NODEID_NUMERIC(1, 61000);
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_UInt32 val = 1;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(UA_Server_addVariableNode(server, x,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ArVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL), UA_STATUSCODE_GOOD);

    /* Default is NONE */
    UA_AccessRestrictionType ar = 0xFFFF;
    ck_assert_uint_eq(UA_Server_getNodeAccessRestrictions(server, x, &ar),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ar, UA_ACCESSRESTRICTIONTYPE_NONE);

    /* Set and read back */
    ck_assert_uint_eq(UA_Server_setNodeAccessRestrictions(server, x,
                          UA_ACCESSRESTRICTIONTYPE_ENCRYPTIONREQUIRED),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_getNodeAccessRestrictions(server, x, &ar),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ar, UA_ACCESSRESTRICTIONTYPE_ENCRYPTIONREQUIRED);

    /* The attribute service returns the value (admin session is exempt from
     * enforcement) */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = x;
    rvid.attributeId = UA_ATTRIBUTEID_ACCESSRESTRICTIONS;
    UA_DataValue dv = UA_Server_read(server, &rvid, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(dv.value.type, &UA_TYPES[UA_TYPES_ACCESSRESTRICTIONTYPE]);
    ck_assert_uint_eq(*(UA_AccessRestrictionType*)dv.value.data,
                      UA_ACCESSRESTRICTIONTYPE_ENCRYPTIONREQUIRED);
    UA_DataValue_clear(&dv);

    /* Namespace default fallback for a node without explicit restrictions */
    UA_NodeId y = UA_NODEID_NUMERIC(1, 61001);
    ck_assert_uint_eq(UA_Server_addVariableNode(server, y,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ArVar2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_setNamespaceDefaultAccessRestrictions(server, 1,
                          UA_ACCESSRESTRICTIONTYPE_SIGNINGREQUIRED),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_getNodeAccessRestrictions(server, y, &ar),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(ar, UA_ACCESSRESTRICTIONTYPE_SIGNINGREQUIRED);

    UA_Server_deleteNode(server, x, true);
    UA_Server_deleteNode(server, y, true);
}
END_TEST

#ifdef UA_ENABLE_AUDITING
static UA_Boolean roleMappingAuditSeen = false;
static void
rbacAuditNotificationCallback(UA_Server *s, UA_ApplicationNotificationType type,
                              const UA_KeyValueMap payload) {
    (void)s; (void)payload;
    if(type == UA_APPLICATIONNOTIFICATIONTYPE_AUDIT_UPDATE_METHOD_ROLEMAPPINGRULECHANGED)
        roleMappingAuditSeen = true;
}

/* Changing a role's identity mapping rules emits a
 * RoleMappingRuleChangedAuditEventType (Part 18). */
START_TEST(auditRoleMappingRuleChanged_emitted) {
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->auditingEnabled = true;
    cfg->auditNotificationCallback = rbacAuditNotificationCallback;
    roleMappingAuditSeen = false;

    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 62000);
    role.roleName = UA_QUALIFIEDNAME(1, "AuditRole");
    ck_assert_uint_eq(UA_Server_addRole(server, &role, NULL), UA_STATUSCODE_GOOD);

    /* Change the identity mapping rules through updateRole */
    UA_Role upd;
    ck_assert_uint_eq(UA_Server_getRoleById(server, role.roleId, &upd),
                      UA_STATUSCODE_GOOD);
    UA_IdentityMappingRuleType *rules = (UA_IdentityMappingRuleType*)
        UA_realloc(upd.identityMappingRules,
                   (upd.identityMappingRulesSize + 1) * sizeof(*rules));
    ck_assert_ptr_nonnull(rules);
    upd.identityMappingRules = rules;
    UA_IdentityMappingRuleType_init(&rules[upd.identityMappingRulesSize]);
    rules[upd.identityMappingRulesSize].criteriaType = UA_IDENTITYCRITERIATYPE_USERNAME;
    rules[upd.identityMappingRulesSize].criteria = UA_STRING_ALLOC("bob");
    upd.identityMappingRulesSize++;
    ck_assert_uint_eq(UA_Server_updateRole(server, &upd), UA_STATUSCODE_GOOD);
    UA_Role_clear(&upd);

    ck_assert(roleMappingAuditSeen);

    cfg->auditNotificationCallback = NULL;
    UA_Server_removeRole(server, role.roleName);
}
END_TEST

/* Adding a role emits a RoleMappingRuleChangedAuditEvent (Part 18 §4.5). */
START_TEST(auditRoleMapping_addRoleEmits) {
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->auditingEnabled = true;
    cfg->auditNotificationCallback = rbacAuditNotificationCallback;
    roleMappingAuditSeen = false;

    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 62101);
    role.roleName = UA_QUALIFIEDNAME(1, "AuditAddRole");
    ck_assert_uint_eq(UA_Server_addRole(server, &role, NULL), UA_STATUSCODE_GOOD);
    ck_assert(roleMappingAuditSeen);

    cfg->auditNotificationCallback = NULL;
    UA_Server_removeRole(server, role.roleName);
}
END_TEST

/* Removing a role emits a RoleMappingRuleChangedAuditEvent (Part 18 §4.5). */
START_TEST(auditRoleMapping_removeRoleEmits) {
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 62102);
    role.roleName = UA_QUALIFIEDNAME(1, "AuditRemoveRole");
    ck_assert_uint_eq(UA_Server_addRole(server, &role, NULL), UA_STATUSCODE_GOOD);

    cfg->auditingEnabled = true;
    cfg->auditNotificationCallback = rbacAuditNotificationCallback;
    roleMappingAuditSeen = false;

    ck_assert_uint_eq(UA_Server_removeRole(server, role.roleName),
                      UA_STATUSCODE_GOOD);
    ck_assert(roleMappingAuditSeen);

    cfg->auditNotificationCallback = NULL;
}
END_TEST
#endif /* UA_ENABLE_AUDITING */

#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
/* The RoleType methods are reachable over the wire on concrete Role objects.
 * A client may pass the RoleType method NodeId; the Call service resolves the
 * same-BrowseName Method child on the Role instance before dispatch. */
START_TEST(roleTypeInstanceMethods_addIdentity) {
    UA_NodeId roleId;
    ck_assert_uint_eq(addTestRole("DupIdentityRole", 1, 62200, &roleId),
                      UA_STATUSCODE_GOOD);

    UA_NodeId instanceMethodId = UA_NODEID_NULL;
    ck_assert_uint_eq(findRoleMethod(roleId, "AddIdentity", &instanceMethodId),
                      UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&instanceMethodId);

    UA_IdentityMappingRuleType rule;
    UA_IdentityMappingRuleType_init(&rule);
    rule.criteriaType = UA_IDENTITYCRITERIATYPE_USERNAME;
    rule.criteria = UA_STRING("alice");
    UA_ExtensionObject ext;
    UA_ExtensionObject_init(&ext);
    UA_ExtensionObject_setValue(&ext, &rule, &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]);

    UA_Variant input;
    UA_Variant_setScalar(&input, &ext, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = roleId;
    req.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDIDENTITY);
    req.inputArguments = &input;
    req.inputArgumentsSize = 1;

    UA_CallMethodResult res = UA_Server_call(server, &req);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);

    UA_Role fetched;
    ck_assert_uint_eq(UA_Server_getRoleById(server, roleId, &fetched),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(fetched.identityMappingRulesSize, 1);
    ck_assert(UA_IdentityMappingRuleType_equal(&fetched.identityMappingRules[0],
                                               &rule));
    UA_Role_clear(&fetched);

    res = UA_Server_call(server, &req);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADALREADYEXISTS);
    UA_CallMethodResult_clear(&res);

    /* rule.criteria is a static literal - do not UA_String_clear it */
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(1, "DupIdentityRole"));
    UA_NodeId_clear(&roleId);
}
END_TEST
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL && UA_ENABLE_METHODCALLS */

/* CustomConfiguration is stored, copied and compared (Part 18 §4.4.1). */
START_TEST(customConfiguration_storedAndCopied) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 62300);
    role.roleName = UA_QUALIFIEDNAME(1, "CustomRole");
    role.customConfiguration = true;
    UA_NodeId outId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &role, &outId), UA_STATUSCODE_GOOD);
    /* role.roleName.name is a string literal - do not UA_Role_clear it */

    /* Round-trip through the registry preserves the flag */
    UA_Role fetched;
    ck_assert_uint_eq(UA_Server_getRoleById(server, outId, &fetched),
                      UA_STATUSCODE_GOOD);
    ck_assert(fetched.customConfiguration);
    UA_Role_clear(&fetched);

    /* UA_Role_copy preserves the flag */
    UA_Role copy;
    ck_assert_uint_eq(UA_Server_getRoleById(server, outId, &copy), UA_STATUSCODE_GOOD);
    UA_Role dup;
    ck_assert_uint_eq(UA_Role_copy(&copy, &dup), UA_STATUSCODE_GOOD);
    ck_assert(dup.customConfiguration);
    UA_Role_clear(&copy);
    UA_Role_clear(&dup);

    /* UA_Role_equal distinguishes the flag. 'other' uses a static
     * roleName literal, so it is compared but never UA_Role_clear'd. */
    UA_Role other;
    UA_Role_init(&other);
    other.roleId = outId;
    other.roleName = UA_QUALIFIEDNAME(1, "CustomRole");
    other.customConfiguration = false;
    UA_Role refTrue;
    ck_assert_uint_eq(UA_Server_getRoleById(server, outId, &refTrue), UA_STATUSCODE_GOOD);
    ck_assert(!UA_Role_equal(&refTrue, &other));
    other.customConfiguration = true;
    ck_assert(UA_Role_equal(&refTrue, &other));
    UA_Role_clear(&refTrue);
    /* other holds only static literals - no clear needed */

    UA_Server_removeRole(server, UA_QUALIFIEDNAME(1, "CustomRole"));
    UA_NodeId_clear(&outId);
}
END_TEST

/* A non-custom Role with empty Identities cannot be granted to any Session
 * (Part 18 §4.4.1). A custom Role with empty Identities can be granted. */
START_TEST(customConfiguration_grantEnforcement) {
    /* Non-custom role with no identity rules */
    UA_Role nc;
    UA_Role_init(&nc);
    nc.roleName = UA_QUALIFIEDNAME(1, "EmptyNonCustom");
    nc.customConfiguration = false;
    UA_NodeId ncId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &nc, &ncId), UA_STATUSCODE_GOOD);

    /* Custom role with no identity rules */
    UA_Role cr;
    UA_Role_init(&cr);
    cr.roleName = UA_QUALIFIEDNAME(1, "EmptyCustom");
    cr.customConfiguration = true;
    UA_NodeId crId = UA_NODEID_NULL;
    ck_assert_uint_eq(UA_Server_addRole(server, &cr, &crId), UA_STATUSCODE_GOOD);

    UA_SessionIdentityContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.isAnonymous = true;

    /* The non-custom empty role is NOT granted */
    ck_assert(!roleGrantedForContext(&ctx, &ncId));
    /* The custom empty role CAN be granted (custom roles bypass the
     * empty-Identities restriction) */
    ck_assert(roleGrantedForContext(&ctx, &crId));

    UA_Server_removeRole(server, UA_QUALIFIEDNAME(1, "EmptyNonCustom"));
    UA_Server_removeRole(server, UA_QUALIFIEDNAME(1, "EmptyCustom"));
    UA_NodeId_clear(&ncId);
    UA_NodeId_clear(&crId);
}
END_TEST

#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
/* The CustomConfiguration Property of a runtime-added role is readable from
 * the AddressSpace and reflects the registry value (Part 18 §4.4.1). */
START_TEST(customConfiguration_propertyReadable) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 62400);
    role.roleName = UA_QUALIFIEDNAME(1, "CustomPropRole");
    role.customConfiguration = true;
    ck_assert_uint_eq(UA_Server_addRole(server, &role, NULL), UA_STATUSCODE_GOOD);

    /* Browse the role's HasProperty children to find CustomConfiguration */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = role.roleId;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.nodeClassMask = UA_NODECLASS_VARIABLE;
    bd.resultMask = UA_BROWSERESULTMASK_BROWSENAME;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    UA_NodeId propId = UA_NODEID_NULL;
    UA_String want = UA_STRING("CustomConfiguration");
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(UA_String_equal(&br.references[i].browseName.name, &want)) {
            UA_NodeId_copy(&br.references[i].nodeId.nodeId, &propId);
            break;
        }
    }
    UA_BrowseResult_clear(&br);
    ck_assert(!UA_NodeId_isNull(&propId));

    UA_Variant v;
    ck_assert_uint_eq(UA_Server_readValue(server, propId, &v),
                     UA_STATUSCODE_GOOD);
    ck_assert(v.type == &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert(*(UA_Boolean*)v.data);
    UA_Variant_clear(&v);
    UA_NodeId_clear(&propId);

    UA_Server_removeRole(server, role.roleName);
}
END_TEST
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */

#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
/* ApplicationsExclude and EndpointsExclude are the two RoleType Properties
 * configured through the Write service (Part 18 §4.4.1). */
START_TEST(excludeProperties_readWriteRegistry) {
    UA_Role role;
    UA_Role_init(&role);
    role.roleId = UA_NODEID_NUMERIC(1, 62410);
    role.roleName = UA_QUALIFIEDNAME(1, "ExcludePropRole");
    role.applicationsExclude = true;
    role.endpointsExclude = true;
    ck_assert_uint_eq(UA_Server_addRole(server, &role, NULL), UA_STATUSCODE_GOOD);

    UA_NodeId appExcludeId = UA_NODEID_NULL;
    UA_NodeId epExcludeId = UA_NODEID_NULL;
    ck_assert_uint_eq(findRoleProperty(role.roleId, "ApplicationsExclude",
                                       &appExcludeId), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(findRoleProperty(role.roleId, "EndpointsExclude",
                                       &epExcludeId), UA_STATUSCODE_GOOD);

    UA_Boolean falseValue = false;
    UA_Variant writeValue;
    UA_Variant_setScalar(&writeValue, &falseValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(UA_Server_writeValue(server, appExcludeId, writeValue),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_writeValue(server, epExcludeId, writeValue),
                      UA_STATUSCODE_GOOD);

    UA_Role fetched;
    ck_assert_uint_eq(UA_Server_getRoleById(server, role.roleId, &fetched),
                      UA_STATUSCODE_GOOD);
    ck_assert(!fetched.applicationsExclude);
    ck_assert(!fetched.endpointsExclude);
    UA_Role_clear(&fetched);

    UA_Variant readValue;
    ck_assert_uint_eq(UA_Server_readValue(server, appExcludeId, &readValue),
                      UA_STATUSCODE_GOOD);
    ck_assert(readValue.type == &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert(!*(UA_Boolean*)readValue.data);
    UA_Variant_clear(&readValue);

    ck_assert_uint_eq(UA_Server_readValue(server, epExcludeId, &readValue),
                      UA_STATUSCODE_GOOD);
    ck_assert(readValue.type == &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert(!*(UA_Boolean*)readValue.data);
    UA_Variant_clear(&readValue);

    UA_NodeId_clear(&appExcludeId);
    UA_NodeId_clear(&epExcludeId);
    UA_Server_removeRole(server, role.roleName);
}
END_TEST
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */

static Suite *testSuite_RolTypeAPI(void) {
    Suite *s = suite_create("RBAC Role Type API");
    TCase *tc = tcase_create("RoleType");
    tcase_add_test(tc, Role_initClearCopy);
    suite_add_tcase(s, tc);

    TCase *tc_cc = tcase_create("CustomConfiguration");
    tcase_add_checked_fixture(tc_cc, setup, teardown);
    tcase_add_test(tc_cc, customConfiguration_storedAndCopied);
    tcase_add_test(tc_cc, customConfiguration_grantEnforcement);
    suite_add_tcase(s, tc_cc);
    return s;
}

static Suite *testSuite_RoleManagement(void) {
    Suite *s = suite_create("RBAC Role Management");

    TCase *tc_add = tcase_create("AddRole");
    tcase_add_checked_fixture(tc_add, setup, teardown);
    tcase_add_test(tc_add, addRole_basic);
    tcase_add_test(tc_add, addRole_duplicateNameFails);
    tcase_add_test(tc_add, addRole_nullRoleIdAllowed);
    tcase_add_test(tc_add, addRole_unsupportedCriteriaStored);
    tcase_add_test(tc_add, addRole_applicationFiltersStored);
    tcase_add_test(tc_add, addRole_quotaEnforced);
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
    tcase_add_test(tc_rm, removeRole_purgesRolePermissions);
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
    tcase_add_test(tc, identityCriteria_extended);
    tcase_add_test(tc, identityCriteria_groupId);
    tcase_add_test(tc, roleFilters_evaluated);
#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
    tcase_add_test(tc, roleTypeInstanceMethods_addIdentity);
#endif
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
    tcase_add_test(tc, permissionEntry_slotNoUnsafeReuse);
    tcase_add_test(tc, allPermissionsForAnonymous_config);
    tcase_add_test(tc, accessRestrictions_setGetRead);
#ifdef UA_ENABLE_AUDITING
    tcase_add_test(tc, auditRoleMappingRuleChanged_emitted);
    tcase_add_test(tc, auditRoleMapping_addRoleEmits);
    tcase_add_test(tc, auditRoleMapping_removeRoleEmits);
#endif
    suite_add_tcase(s, tc);
    return s;
}

static Suite *testSuite_NamespaceDefaults(void) {
    Suite *s = suite_create("RBAC Namespace Defaults");
    TCase *tc = tcase_create("NsDefaults");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, namespaceDefault_setAndGet);
    tcase_add_test(tc, namespaceDefault_explicitOverrides);
    tcase_add_test(tc, namespaceDefault_effectiveFallback);
    tcase_add_test(tc, namespaceDefault_noRoleMatchDenied);
    tcase_add_test(tc, namespaceDefault_perNamespaceIsolation);
    tcase_add_test(tc, namespaceDefault_invalidNamespaceIndex);
    tcase_add_test(tc, namespaceDefault_explicitEmptyDenies);
    suite_add_tcase(s, tc);
    return s;
}

/**********************************/
/* Part 18 §5.2 UserManagement    */
/**********************************/

#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)

/* A minimal in-memory UserManagement provider. Real implementations own
 * password hashing, persistence and rate limiting; this one only has to be
 * observable so the core's argument checking, policy validation and
 * self-reference guards can be tested. */
#define UM_MAX_USERS 8
typedef struct {
    UA_String userName;
    UA_String password;
    UA_UserConfigurationMask configuration;
    UA_String description;
} UMUser;

static UMUser umUsers[UM_MAX_USERS];
static size_t umUsersSize;
static UA_PasswordOptionsMask umOptions;
static UA_StatusCode umForcedStatus;
/* Records the arguments the core forwarded, so the tests can assert that the
 * Method arguments reach the provider unchanged and in the right order. */
static UA_String umLastPassword;
static UA_String umLastOldPassword;
static UA_Boolean umChangePasswordCalled;

static void umReset(void) {
    for(size_t i = 0; i < umUsersSize; i++) {
        UA_String_clear(&umUsers[i].userName);
        UA_String_clear(&umUsers[i].password);
        UA_String_clear(&umUsers[i].description);
    }
    umUsersSize = 0;
    UA_String_clear(&umLastPassword);
    UA_String_clear(&umLastOldPassword);
    umChangePasswordCalled = false;
    umForcedStatus = UA_STATUSCODE_GOOD;
    umOptions = UA_PASSWORDOPTIONSMASK_SUPPORTINITIALPASSWORDCHANGE |
                UA_PASSWORDOPTIONSMASK_SUPPORTDISABLEUSER |
                UA_PASSWORDOPTIONSMASK_SUPPORTDISABLEDELETEFORUSER |
                UA_PASSWORDOPTIONSMASK_SUPPORTNOCHANGEFORUSER;
}

static UMUser *umFind(const UA_String *userName) {
    for(size_t i = 0; i < umUsersSize; i++) {
        if(UA_String_equal(&umUsers[i].userName, userName))
            return &umUsers[i];
    }
    return NULL;
}

static UA_StatusCode
umGetUsers(UA_Server *s, UA_AccessControl *ac,
           UA_UserManagementDataType **users, size_t *usersSize) {
    if(umForcedStatus != UA_STATUSCODE_GOOD)
        return umForcedStatus;
    *users = NULL;
    *usersSize = 0;
    if(umUsersSize == 0)
        return UA_STATUSCODE_GOOD;
    UA_UserManagementDataType *out = (UA_UserManagementDataType*)
        UA_Array_new(umUsersSize, &UA_TYPES[UA_TYPES_USERMANAGEMENTDATATYPE]);
    if(!out)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(size_t i = 0; i < umUsersSize; i++) {
        UA_String_copy(&umUsers[i].userName, &out[i].userName);
        UA_String_copy(&umUsers[i].description, &out[i].description);
        out[i].userConfiguration = umUsers[i].configuration;
    }
    *users = out;
    *usersSize = umUsersSize;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
umGetPasswordPolicy(UA_Server *s, UA_AccessControl *ac, UA_Range *length,
                    UA_PasswordOptionsMask *options,
                    UA_LocalizedText *restrictions) {
    length->low = 8.0;
    length->high = 64.0;
    *options = umOptions;
    *restrictions = UA_LOCALIZEDTEXT_ALLOC("en-US", "at least eight characters");
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
umGetUserConfiguration(UA_Server *s, UA_AccessControl *ac,
                       const UA_String *userName,
                       UA_UserConfigurationMask *configuration) {
    UMUser *u = umFind(userName);
    if(!u)
        return UA_STATUSCODE_BADNOTFOUND;
    *configuration = u->configuration;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
umAddUser(UA_Server *s, UA_AccessControl *ac, const UA_String *userName,
          const UA_String *password, UA_UserConfigurationMask configuration,
          const UA_String *description) {
    if(umForcedStatus != UA_STATUSCODE_GOOD)
        return umForcedStatus;
    if(umFind(userName))
        return UA_STATUSCODE_BADALREADYEXISTS;
    if(umUsersSize >= UM_MAX_USERS)
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;
    UMUser *u = &umUsers[umUsersSize++];
    UA_String_copy(userName, &u->userName);
    UA_String_copy(password, &u->password);
    UA_String_copy(description, &u->description);
    u->configuration = configuration;
    UA_String_clear(&umLastPassword);
    UA_String_copy(password, &umLastPassword);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
umModifyUser(UA_Server *s, UA_AccessControl *ac, const UA_String *userName,
             UA_Boolean modifyPassword, const UA_String *password,
             UA_Boolean modifyConfiguration,
             UA_UserConfigurationMask configuration,
             UA_Boolean modifyDescription, const UA_String *description) {
    UMUser *u = umFind(userName);
    if(!u)
        return UA_STATUSCODE_BADNOTFOUND;
    if(modifyPassword) {
        UA_String_clear(&u->password);
        UA_String_copy(password, &u->password);
    }
    if(modifyConfiguration)
        u->configuration = configuration;
    if(modifyDescription) {
        UA_String_clear(&u->description);
        UA_String_copy(description, &u->description);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
umRemoveUser(UA_Server *s, UA_AccessControl *ac, const UA_String *userName) {
    UMUser *u = umFind(userName);
    if(!u)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_String_clear(&u->userName);
    UA_String_clear(&u->password);
    UA_String_clear(&u->description);
    size_t idx = (size_t)(u - umUsers);
    for(size_t i = idx; i + 1 < umUsersSize; i++)
        umUsers[i] = umUsers[i + 1];
    umUsersSize--;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
umChangePassword(UA_Server *s, UA_AccessControl *ac, const UA_String *userName,
                 const UA_String *oldPassword, const UA_String *newPassword) {
    umChangePasswordCalled = true;
    UA_String_clear(&umLastOldPassword);
    UA_String_copy(oldPassword, &umLastOldPassword);
    UMUser *u = umFind(userName);
    if(!u)
        return UA_STATUSCODE_BADNOTFOUND;
    if(!UA_String_equal(&u->password, oldPassword))
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    UA_String_clear(&u->password);
    return UA_String_copy(newPassword, &u->password);
}

/* initNS0RBAC runs from UA_Server_init, so the provider has to be part of the
 * configuration handed to UA_Server_newWithConfig. Installing the callbacks on
 * UA_Server_getConfig() after the Server exists is too late to wire the
 * Object - the tests below would then silently exercise nothing. */
static void setupUserManagement(void) {
    umReset();
    UA_ServerConfig sc;
    memset(&sc, 0, sizeof(UA_ServerConfig));
    sc.logging = UA_Log_Stdout_new(UA_LOGLEVEL_ERROR);
    UA_ServerConfig_setMinimal(&sc, 4840, NULL);
    sc.accessControl.getUsers = umGetUsers;
    sc.accessControl.getPasswordPolicy = umGetPasswordPolicy;
    sc.accessControl.getUserConfiguration = umGetUserConfiguration;
    sc.accessControl.addUser = umAddUser;
    sc.accessControl.modifyUser = umModifyUser;
    sc.accessControl.removeUser = umRemoveUser;
    sc.accessControl.changePassword = umChangePassword;
    server = UA_Server_newWithConfig(&sc);
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardownUserManagement(void) {
    teardown();
    umReset();
}

/* A server without a provider must leave the UserManagement Object inert. */
static void setupNoUserManagement(void) {
    umReset();
    setup();
}

static UA_CallMethodResult
callUserMethod(UA_UInt32 methodId, size_t inputSize, UA_Variant *input) {
    UA_CallMethodRequest req;
    UA_CallMethodRequest_init(&req);
    req.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_USERMANAGEMENT);
    req.methodId = UA_NODEID_NUMERIC(0, methodId);
    req.inputArguments = input;
    req.inputArgumentsSize = inputSize;
    return UA_Server_call(server, &req);
}

static void
setUserArgs(UA_Variant *v, UA_String *name, UA_String *password,
            UA_UserConfigurationMask *cfg, UA_String *description) {
    UA_Variant_setScalar(&v[0], name, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&v[1], password, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&v[2], cfg, &UA_TYPES[UA_TYPES_USERCONFIGURATIONMASK]);
    UA_Variant_setScalar(&v[3], description, &UA_TYPES[UA_TYPES_STRING]);
}

/* AddUser reaches the provider with its arguments intact, and the Users
 * Property reports what the provider holds. */
START_TEST(userManagement_addUserReachesProvider) {
    UA_String name = UA_STRING("alice");
    UA_String password = UA_STRING("s3cret-password");
    UA_String description = UA_STRING("plant operator");
    UA_UserConfigurationMask cfg = UA_USERCONFIGURATIONMASK_NONE;
    UA_Variant in[4];
    setUserArgs(in, &name, &password, &cfg, &description);

    UA_CallMethodResult res = callUserMethod(UA_NS0ID_USERMANAGEMENT_ADDUSER,
                                             4, in);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);

    ck_assert_uint_eq(umUsersSize, 1);
    ck_assert(UA_String_equal(&umUsers[0].userName, &name));
    ck_assert(UA_String_equal(&umLastPassword, &password));
    ck_assert(UA_String_equal(&umUsers[0].description, &description));

    /* The Users Property is backed by getUsers */
    UA_Variant users;
    ck_assert_uint_eq(UA_Server_readValue(server,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_USERMANAGEMENT_USERS),
                          &users), UA_STATUSCODE_GOOD);
    ck_assert(users.type == &UA_TYPES[UA_TYPES_USERMANAGEMENTDATATYPE]);
    ck_assert_uint_eq(users.arrayLength, 1);
    UA_UserManagementDataType *list = (UA_UserManagementDataType*)users.data;
    ck_assert(UA_String_equal(&list[0].userName, &name));
    UA_Variant_clear(&users);

    /* A provider failure is reported rather than swallowed */
    umForcedStatus = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
    UA_Variant unused;
    ck_assert_uint_eq(UA_Server_readValue(server,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_USERMANAGEMENT_USERS),
                          &unused), UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    umForcedStatus = UA_STATUSCODE_GOOD;
}
END_TEST

/* The password policy Properties are served from getPasswordPolicy. */
START_TEST(userManagement_passwordPolicyReadable) {
    UA_Variant v;
    ck_assert_uint_eq(UA_Server_readValue(server,
                UA_NODEID_NUMERIC(0, UA_NS0ID_USERMANAGEMENT_PASSWORDLENGTH),
                &v), UA_STATUSCODE_GOOD);
    ck_assert(v.type == &UA_TYPES[UA_TYPES_RANGE]);
    ck_assert(((UA_Range*)v.data)->low == 8.0);
    ck_assert(((UA_Range*)v.data)->high == 64.0);
    UA_Variant_clear(&v);

    ck_assert_uint_eq(UA_Server_readValue(server,
                UA_NODEID_NUMERIC(0, UA_NS0ID_USERMANAGEMENT_PASSWORDOPTIONS),
                &v), UA_STATUSCODE_GOOD);
    ck_assert(v.type == &UA_TYPES[UA_TYPES_PASSWORDOPTIONSMASK]);
    ck_assert_uint_eq(*(UA_PasswordOptionsMask*)v.data, umOptions);
    UA_Variant_clear(&v);

    ck_assert_uint_eq(UA_Server_readValue(server,
            UA_NODEID_NUMERIC(0, UA_NS0ID_USERMANAGEMENT_PASSWORDRESTRICTIONS),
            &v), UA_STATUSCODE_GOOD);
    ck_assert(v.type == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_Variant_clear(&v);
}
END_TEST

/* Malformed arguments are rejected without reaching the provider. The Call
 * service validates arity and types against the declared InputArguments, so it
 * usually answers first (Bad_ArgumentsMissing, Bad_InvalidArgument or
 * Bad_TypeMismatch); the callback's own userMethodInputs check backs that up
 * for callers that reach it directly. What matters here is that no combination
 * reaches the provider. */
START_TEST(userManagement_rejectsMalformedArguments) {
    UA_String name = UA_STRING("bob");
    UA_UserConfigurationMask cfg = UA_USERCONFIGURATIONMASK_NONE;

    /* Too few arguments */
    UA_Variant one;
    UA_Variant_setScalar(&one, &name, &UA_TYPES[UA_TYPES_STRING]);
    UA_CallMethodResult res =
        callUserMethod(UA_NS0ID_USERMANAGEMENT_ADDUSER, 1, &one);
    ck_assert_uint_ne(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);

    /* Right arity, wrong type in the middle */
    UA_Variant bad[4];
    UA_Variant_setScalar(&bad[0], &name, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&bad[1], &cfg,
                         &UA_TYPES[UA_TYPES_USERCONFIGURATIONMASK]);
    UA_Variant_setScalar(&bad[2], &cfg,
                         &UA_TYPES[UA_TYPES_USERCONFIGURATIONMASK]);
    UA_Variant_setScalar(&bad[3], &name, &UA_TYPES[UA_TYPES_STRING]);
    res = callUserMethod(UA_NS0ID_USERMANAGEMENT_ADDUSER, 4, bad);
    ck_assert_uint_ne(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);

    /* An array where a scalar is required */
    UA_String names[2] = {UA_STRING("a"), UA_STRING("b")};
    UA_Variant arr;
    UA_Variant_setArray(&arr, names, 2, &UA_TYPES[UA_TYPES_STRING]);
    res = callUserMethod(UA_NS0ID_USERMANAGEMENT_REMOVEUSER, 1, &arr);
    ck_assert_uint_ne(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);

    /* ModifyUser takes seven arguments, not four */
    UA_String password = UA_STRING("pw");
    UA_String description = UA_STRING("d");
    UA_Variant four[4];
    setUserArgs(four, &name, &password, &cfg, &description);
    res = callUserMethod(UA_NS0ID_USERMANAGEMENT_MODIFYUSER, 4, four);
    ck_assert_uint_ne(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);

    ck_assert_uint_eq(umUsersSize, 0);
    ck_assert(!umChangePasswordCalled);
}
END_TEST

/* The requested UserConfiguration is validated against the provider's
 * PasswordOptions before the provider is asked to apply it. */
START_TEST(userManagement_configurationValidatedAgainstPolicy) {
    UA_String name = UA_STRING("carol");
    UA_String password = UA_STRING("s3cret-password");
    UA_String description = UA_STRING("");
    UA_Variant in[4];

    /* NoChangeByUser and MustChangePassword contradict each other */
    UA_UserConfigurationMask cfg = UA_USERCONFIGURATIONMASK_NOCHANGEBYUSER |
                                   UA_USERCONFIGURATIONMASK_MUSTCHANGEPASSWORD;
    setUserArgs(in, &name, &password, &cfg, &description);
    UA_CallMethodResult res =
        callUserMethod(UA_NS0ID_USERMANAGEMENT_ADDUSER, 4, in);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADCONFIGURATIONERROR);
    UA_CallMethodResult_clear(&res);

    /* A flag the provider does not advertise is refused */
    umOptions = UA_PASSWORDOPTIONSMASK_NONE;
    cfg = UA_USERCONFIGURATIONMASK_DISABLED;
    setUserArgs(in, &name, &password, &cfg, &description);
    res = callUserMethod(UA_NS0ID_USERMANAGEMENT_ADDUSER, 4, in);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADNOTSUPPORTED);
    UA_CallMethodResult_clear(&res);

    /* Once advertised, the same request succeeds */
    umOptions = UA_PASSWORDOPTIONSMASK_SUPPORTDISABLEUSER;
    setUserArgs(in, &name, &password, &cfg, &description);
    res = callUserMethod(UA_NS0ID_USERMANAGEMENT_ADDUSER, 4, in);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);
    ck_assert_uint_eq(umUsersSize, 1);
    ck_assert_uint_eq(umUsers[0].configuration,
                      UA_USERCONFIGURATIONMASK_DISABLED);
}
END_TEST

/* ModifyUser forwards each modify flag and its value separately. */
START_TEST(userManagement_modifyUserAppliesSelectedFields) {
    UA_String name = UA_STRING("dave");
    UA_String password = UA_STRING("s3cret-password");
    UA_String description = UA_STRING("before");
    UA_UserConfigurationMask cfg = UA_USERCONFIGURATIONMASK_NONE;
    UA_Variant in[4];
    setUserArgs(in, &name, &password, &cfg, &description);
    UA_CallMethodResult res =
        callUserMethod(UA_NS0ID_USERMANAGEMENT_ADDUSER, 4, in);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);

    /* Change only the description; leave password and configuration alone */
    UA_Boolean no = false, yes = true;
    UA_String newPassword = UA_STRING("unused");
    UA_String newDescription = UA_STRING("after");
    UA_Variant mod[7];
    UA_Variant_setScalar(&mod[0], &name, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&mod[1], &no, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&mod[2], &newPassword, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&mod[3], &no, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&mod[4], &cfg,
                         &UA_TYPES[UA_TYPES_USERCONFIGURATIONMASK]);
    UA_Variant_setScalar(&mod[5], &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&mod[6], &newDescription, &UA_TYPES[UA_TYPES_STRING]);
    res = callUserMethod(UA_NS0ID_USERMANAGEMENT_MODIFYUSER, 7, mod);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);

    ck_assert(UA_String_equal(&umUsers[0].description, &newDescription));
    ck_assert(UA_String_equal(&umUsers[0].password, &password));

    /* An unsupported configuration is still refused on modify */
    umOptions = UA_PASSWORDOPTIONSMASK_NONE;
    UA_UserConfigurationMask disabled = UA_USERCONFIGURATIONMASK_DISABLED;
    UA_Variant_setScalar(&mod[3], &yes, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Variant_setScalar(&mod[4], &disabled,
                         &UA_TYPES[UA_TYPES_USERCONFIGURATIONMASK]);
    res = callUserMethod(UA_NS0ID_USERMANAGEMENT_MODIFYUSER, 7, mod);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADNOTSUPPORTED);
    UA_CallMethodResult_clear(&res);
}
END_TEST

/* RemoveUser forwards to the provider and surfaces its status. */
START_TEST(userManagement_removeUserReachesProvider) {
    UA_String name = UA_STRING("erin");
    UA_String password = UA_STRING("s3cret-password");
    UA_String description = UA_STRING("");
    UA_UserConfigurationMask cfg = UA_USERCONFIGURATIONMASK_NONE;
    UA_Variant in[4];
    setUserArgs(in, &name, &password, &cfg, &description);
    UA_CallMethodResult res =
        callUserMethod(UA_NS0ID_USERMANAGEMENT_ADDUSER, 4, in);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);

    UA_Variant rm;
    UA_Variant_setScalar(&rm, &name, &UA_TYPES[UA_TYPES_STRING]);
    res = callUserMethod(UA_NS0ID_USERMANAGEMENT_REMOVEUSER, 1, &rm);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);
    ck_assert_uint_eq(umUsersSize, 0);

    /* Removing it again is the provider's Bad_NotFound, passed through */
    res = callUserMethod(UA_NS0ID_USERMANAGEMENT_REMOVEUSER, 1, &rm);
    ck_assert_uint_eq(res.statusCode, UA_STATUSCODE_BADNOTFOUND);
    UA_CallMethodResult_clear(&res);
}
END_TEST

/* ChangePassword acts on the caller's own account over an encrypted channel.
 * The local admin Session has neither a channel nor a user name, so it cannot
 * reach the provider - a privileged caller must not change a password for
 * someone else through this Method. */
START_TEST(userManagement_changePasswordNeedsOwnEncryptedSession) {
    UA_String oldPassword = UA_STRING("s3cret-password");
    UA_String newPassword = UA_STRING("even-more-s3cret");
    UA_Variant in[2];
    UA_Variant_setScalar(&in[0], &oldPassword, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&in[1], &newPassword, &UA_TYPES[UA_TYPES_STRING]);

    UA_CallMethodResult res =
        callUserMethod(UA_NS0ID_USERMANAGEMENT_CHANGEPASSWORD, 2, in);
    ck_assert_uint_eq(res.statusCode,
                      UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT);
    UA_CallMethodResult_clear(&res);
    ck_assert(!umChangePasswordCalled);
}
END_TEST

/* Without a complete provider the Object stays inert: no DataSource behind the
 * Properties and no callbacks on the Methods. */
START_TEST(userManagement_inertWithoutProvider) {
    UA_String name = UA_STRING("frank");
    UA_String password = UA_STRING("s3cret-password");
    UA_String description = UA_STRING("");
    UA_UserConfigurationMask cfg = UA_USERCONFIGURATIONMASK_NONE;
    UA_Variant in[4];
    setUserArgs(in, &name, &password, &cfg, &description);

    UA_CallMethodResult res = callUserMethod(UA_NS0ID_USERMANAGEMENT_ADDUSER,
                                             4, in);
    ck_assert_uint_ne(res.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&res);
    ck_assert_uint_eq(umUsersSize, 0);
}
END_TEST

static Suite *testSuite_UserManagement(void) {
    Suite *s = suite_create("RBAC User Management");

    TCase *tc = tcase_create("UserManagement");
    tcase_add_checked_fixture(tc, setupUserManagement, teardownUserManagement);
    tcase_add_test(tc, userManagement_addUserReachesProvider);
    tcase_add_test(tc, userManagement_passwordPolicyReadable);
    tcase_add_test(tc, userManagement_rejectsMalformedArguments);
    tcase_add_test(tc, userManagement_configurationValidatedAgainstPolicy);
    tcase_add_test(tc, userManagement_modifyUserAppliesSelectedFields);
    tcase_add_test(tc, userManagement_removeUserReachesProvider);
    tcase_add_test(tc, userManagement_changePasswordNeedsOwnEncryptedSession);
    suite_add_tcase(s, tc);

    TCase *tc_none = tcase_create("NoProvider");
    tcase_add_checked_fixture(tc_none, setupNoUserManagement,
                              teardownUserManagement);
    tcase_add_test(tc_none, userManagement_inertWithoutProvider);
    suite_add_tcase(s, tc_none);
    return s;
}
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL && UA_ENABLE_METHODCALLS */

static Suite *testSuite_InformationModel(void) {
    Suite *s = suite_create("RBAC Information Model");
    TCase *tc = tcase_create("NS0");
    tcase_add_unchecked_fixture(tc, setup, teardown);
#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
    tcase_add_test(tc, roleSetExists);
    tcase_add_test(tc, standardRolesWithCorrectIds);
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */
    tcase_add_test(tc, getAllRoles_includesWellKnown);
    tcase_add_test(tc, updateRole_identifiedByEitherKey);
    tcase_add_test(tc, protectMandatoryRoles);
    tcase_add_test(tc, allowModifyingOptionalRoles);
    tcase_add_test(tc, identityMapping_wellKnownRoles);
#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
    tcase_add_test(tc, wellKnownRoles_nodeFields);
    tcase_add_test(tc, wellKnownRoles_identitiesFromRegistry);
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */
    tcase_add_test(tc, addedRole_ns0NodeFields);
#ifdef UA_GENERATED_NAMESPACE_ZERO_FULL
    tcase_add_test(tc, addRole_cApiPublishesRoleObject);
    tcase_add_test(tc, customConfiguration_propertyReadable);
    tcase_add_test(tc, excludeProperties_readWriteRegistry);
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL */
#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
    tcase_add_test(tc, addRemoveRoleMethod_updatesAddressSpace);
    tcase_add_test(tc, roleSetMethods_restrictedToAdmin);
#endif /* UA_GENERATED_NAMESPACE_ZERO_FULL && UA_ENABLE_METHODCALLS */
    suite_add_tcase(s, tc);

    TCase *tc_session = tcase_create("SessionRoles");
    tcase_add_unchecked_fixture(tc_session, setup, teardown);
    tcase_add_test(tc_session, sessionRoleManagement);
    tcase_add_test(tc_session, addSessionRole);
    tcase_add_test(tc_session, sessionRoleNames);
    tcase_add_test(tc_session, trustedApplication_roleRegistered);
    tcase_add_test(tc_session, trustedApplication_assignedWhenTrusted);
    tcase_add_test(tc_session, anonymousRole_alwaysAssigned);
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
    srunner_add_suite(sr, testSuite_NamespaceDefaults());
    srunner_add_suite(sr, testSuite_InformationModel());
#if defined(UA_GENERATED_NAMESPACE_ZERO_FULL) && defined(UA_ENABLE_METHODCALLS)
    srunner_add_suite(sr, testSuite_UserManagement());
#endif
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
