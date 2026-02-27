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

/* ---- Role Type API tests ---- */

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

/* ---- addRole / getRoles / getRole ---- */

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

/* ---- getRoles ---- */

START_TEST(getRoles_empty) {
    size_t rolesSize = 99;
    UA_QualifiedName *roleNames = NULL;
    UA_StatusCode res = UA_Server_getRoles(server, &rolesSize, &roleNames);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, 0);
    ck_assert_ptr_null(roleNames);
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
    ck_assert_uint_eq(rolesSize, 2);
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

/* ---- getRole ---- */

START_TEST(getRole_notFound) {
    UA_QualifiedName badName = UA_QUALIFIEDNAME(0, "NonExistentRole");
    UA_Role out;
    UA_StatusCode res = UA_Server_getRole(server, badName, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADNOTFOUND);
}
END_TEST

/* ---- removeRole ---- */

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

/* ---- Config roles (protected) ---- */

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
    ck_assert_uint_eq(rolesSize, 2);

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
    ck_assert_uint_eq(rolesSize, 2);

    for(size_t i = 0; i < rolesSize; i++)
        UA_QualifiedName_clear(&roleNames[i]);
    UA_free(roleNames);
}
END_TEST

/* ---- Test suite assembly ---- */

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

int main(void) {
    int number_failed = 0;
    SRunner *sr;

    sr = srunner_create(testSuite_RolTypeAPI());
    srunner_add_suite(sr, testSuite_RoleManagement());
    srunner_add_suite(sr, testSuite_ConfigRoles());
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
