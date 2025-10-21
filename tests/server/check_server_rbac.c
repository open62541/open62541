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
}

static void teardown(void) {
    if(server) {
        UA_Server_delete(server);
        server = NULL;
    }
}

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

START_TEST(Server_rbacStandardRolesWithCorrectIds) {
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
        UA_StatusCode retval = UA_Server_readBrowseName(server, roleId, &browseName);
        
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_String expectedName = UA_STRING((char*)(uintptr_t)roles[i].name);
        ck_assert(UA_String_equal(&browseName.name, &expectedName));
        
        UA_QualifiedName_clear(&browseName);
    }
}
END_TEST

START_TEST(Server_rbacGetRolesById) {
    UA_NodeId roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    const UA_Role *role = UA_Server_getRolesById(server, roleId);
    
    ck_assert_ptr_nonnull(role);
    ck_assert(UA_NodeId_equal(&role->roleId, &roleId));
}
END_TEST

START_TEST(Server_rbacGetRolesByName) {
    UA_String roleName = UA_STRING("Anonymous");
    UA_String namespaceUri = UA_STRING_NULL;
    const UA_Role *role = UA_Server_getRolesByName(server, roleName, namespaceUri);
    
    ck_assert_ptr_nonnull(role);
    ck_assert(UA_String_equal(&role->roleName.name, &roleName));
}
END_TEST

START_TEST(Server_rbacGetAllRoles) {
    size_t rolesSize = 0;
    UA_NodeId *roleIds = NULL;
    UA_StatusCode retval = UA_Server_getRoles(server, &rolesSize, &roleIds);
    
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ge(rolesSize, 8);
    ck_assert_ptr_nonnull(roleIds);
    
    for(size_t i = 0; i < rolesSize; i++) {
        UA_NodeId_clear(&roleIds[i]);
    }
    UA_free(roleIds);
}
END_TEST

START_TEST(Server_rbacAddRoleViaAPI) {
    UA_String roleName = UA_STRING("TestRole");
    UA_String namespaceUri = UA_STRING_NULL;
    UA_NodeId newRoleId = UA_NODEID_NULL;
    
    UA_StatusCode retval = UA_Server_addRole(server, roleName, namespaceUri, NULL, &newRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&newRoleId));
    
    const UA_Role *role = UA_Server_getRolesById(server, newRoleId);
    ck_assert_ptr_nonnull(role);
    ck_assert(UA_String_equal(&role->roleName.name, &roleName));
    
    UA_NodeId_clear(&newRoleId);
}
END_TEST

START_TEST(Server_rbacRemoveRoleViaAPI) {
    UA_String roleName = UA_STRING("TestRoleToRemove");
    UA_String namespaceUri = UA_STRING_NULL;
    UA_NodeId newRoleId = UA_NODEID_NULL;
    
    size_t rolesSize = 0;
    UA_NodeId *roleIds = NULL;
    UA_StatusCode retval = UA_Server_getRoles(server, &rolesSize, &roleIds);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    size_t initialRolesSize = rolesSize;
    for(size_t i = 0; i < rolesSize; i++) {
        UA_NodeId_clear(&roleIds[i]);
    }
    UA_free(roleIds);
    
    retval = UA_Server_addRole(server, roleName, namespaceUri, NULL, &newRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *roleBeforeRemove = UA_Server_getRolesById(server, newRoleId);
    ck_assert_ptr_nonnull(roleBeforeRemove);
    
    retval = UA_Server_removeRole(server, newRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_getRoles(server, &rolesSize, &roleIds);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, initialRolesSize);
    
    for(size_t i = 0; i < rolesSize; i++) {
        UA_NodeId_clear(&roleIds[i]);
    }
    UA_free(roleIds);
    
    UA_NodeId_clear(&newRoleId);
}
END_TEST

START_TEST(Server_rbacIdentityManagement) {
    UA_NodeId newRoleId = UA_NODEID_NULL;
    UA_StatusCode retval = UA_Server_addRole(server, UA_STRING("TestRoleIdentity"),
                                              UA_STRING_NULL, NULL, &newRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role = UA_Server_getRolesById(server, newRoleId);
    ck_assert_ptr_nonnull(role);
    size_t initialSize = role->imrtSize;
    
    retval = UA_Server_addRoleIdentity(server, newRoleId, UA_IDENTITYCRITERIATYPE_ANONYMOUS);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role = UA_Server_getRolesById(server, newRoleId);
    ck_assert_uint_eq(role->imrtSize, initialSize + 1);
    
    retval = UA_Server_removeRoleIdentity(server, newRoleId, UA_IDENTITYCRITERIATYPE_ANONYMOUS);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role = UA_Server_getRolesById(server, newRoleId);
    ck_assert_uint_eq(role->imrtSize, initialSize);
    
    UA_NodeId_clear(&newRoleId);
}
END_TEST

START_TEST(Server_rbacApplicationManagement) {
    UA_NodeId newRoleId = UA_NODEID_NULL;
    UA_StatusCode retval = UA_Server_addRole(server, UA_STRING("TestRoleApp"),
                                              UA_STRING_NULL, NULL, &newRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role = UA_Server_getRolesById(server, newRoleId);
    ck_assert_ptr_nonnull(role);
    size_t initialSize = role->applicationsSize;
    
    UA_String appUri = UA_STRING("urn:test:application");
    retval = UA_Server_addRoleApplication(server, newRoleId, appUri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role = UA_Server_getRolesById(server, newRoleId);
    ck_assert_uint_eq(role->applicationsSize, initialSize + 1);
    
    retval = UA_Server_removeRoleApplication(server, newRoleId, appUri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role = UA_Server_getRolesById(server, newRoleId);
    ck_assert_uint_eq(role->applicationsSize, initialSize);
    
    UA_NodeId_clear(&newRoleId);
}
END_TEST

START_TEST(Server_rbacProtectMandatoryRoles) {
    UA_NodeId anonymousRoleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    UA_StatusCode retval;
    
    retval = UA_Server_addRoleIdentity(server, anonymousRoleId, UA_IDENTITYCRITERIATYPE_USERNAME);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    
    retval = UA_Server_removeRoleIdentity(server, anonymousRoleId, UA_IDENTITYCRITERIATYPE_ANONYMOUS);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    
    UA_String appUri = UA_STRING("http://example.com/testapp");
    retval = UA_Server_addRoleApplication(server, anonymousRoleId, appUri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    
    UA_EndpointType endpoint;
    UA_EndpointType_init(&endpoint);
    UA_String transportUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");
    UA_String_copy(&transportUri, &endpoint.transportProfileUri);
    retval = UA_Server_addRoleEndpoint(server, anonymousRoleId, endpoint);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_EndpointType_clear(&endpoint);
    
    retval = UA_Server_removeRole(server, anonymousRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    
    UA_NodeId authenticatedUserRoleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    retval = UA_Server_addRoleIdentity(server, authenticatedUserRoleId, UA_IDENTITYCRITERIATYPE_USERNAME);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
    
    retval = UA_Server_removeRole(server, authenticatedUserRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADUSERACCESSDENIED);
}
END_TEST

START_TEST(Server_rbacAllowModifyingOptionalRoles) {
    UA_NodeId observerRoleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    UA_StatusCode retval;
    
    retval = UA_Server_addRoleIdentity(server, observerRoleId, UA_IDENTITYCRITERIATYPE_USERNAME);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_removeRoleIdentity(server, observerRoleId, UA_IDENTITYCRITERIATYPE_USERNAME);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    UA_String appUri = UA_STRING("http://example.com/testapp");
    retval = UA_Server_addRoleApplication(server, observerRoleId, appUri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_removeRoleApplication(server, observerRoleId, appUri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_rbacNamespaceHandling) {
    UA_NodeId roleId1, roleId2, roleId3, roleId4;
    UA_StatusCode retval;
    
    retval = UA_Server_addRole(server, UA_STRING("CustomRole1"), UA_STRING_NULL, NULL, &roleId1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role1 = UA_Server_getRolesById(server, roleId1);
    ck_assert_ptr_nonnull(role1);
    ck_assert_uint_eq(role1->roleName.namespaceIndex, 1);
    
    const UA_Role *foundRole1 = UA_Server_getRolesByName(server, UA_STRING("CustomRole1"), UA_STRING_NULL);
    ck_assert_ptr_nonnull(foundRole1);
    ck_assert(UA_NodeId_equal(&foundRole1->roleId, &roleId1));
    
    UA_String customNsUri = UA_STRING("http://example.com/CustomNamespace");
    UA_UInt16 customNsIdx = UA_Server_addNamespace(server, "http://example.com/CustomNamespace");
    ck_assert(customNsIdx >= 2);
    
    retval = UA_Server_addRole(server, UA_STRING("CustomRole2"), customNsUri, NULL, &roleId2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role2 = UA_Server_getRolesById(server, roleId2);
    ck_assert_ptr_nonnull(role2);
    ck_assert_uint_eq(role2->roleName.namespaceIndex, customNsIdx);
    
    const UA_Role *foundRole2 = UA_Server_getRolesByName(server, UA_STRING("CustomRole2"), customNsUri);
    ck_assert_ptr_nonnull(foundRole2);
    ck_assert(UA_NodeId_equal(&foundRole2->roleId, &roleId2));
    
    const UA_Role *notFound = UA_Server_getRolesByName(server, UA_STRING("CustomRole2"), UA_STRING_NULL);
    ck_assert_ptr_null(notFound);
    
    retval = UA_Server_addRole(server, UA_STRING("CustomRole3"), customNsUri, NULL, &roleId3);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role3 = UA_Server_getRolesById(server, roleId3);
    ck_assert_uint_eq(role3->roleName.namespaceIndex, role2->roleName.namespaceIndex);
    
    UA_NodeId duplicateRoleId;
    retval = UA_Server_addRole(server, UA_STRING("CustomRole2"), customNsUri, NULL, &duplicateRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDEXISTS);
    
    UA_String differentNsUri = UA_STRING("http://example.com/AnotherNamespace");
    UA_UInt16 differentNsIdx = UA_Server_addNamespace(server, "http://example.com/AnotherNamespace");
    ck_assert(differentNsIdx >= 2);
    
    retval = UA_Server_addRole(server, UA_STRING("CustomRole2"), differentNsUri, NULL, &roleId4);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role4 = UA_Server_getRolesById(server, roleId4);
    ck_assert_uint_eq(role4->roleName.namespaceIndex, differentNsIdx);
    ck_assert(role4->roleName.namespaceIndex != role2->roleName.namespaceIndex);
    
    const UA_Role *foundInNs1 = UA_Server_getRolesByName(server, UA_STRING("CustomRole2"), customNsUri);
    const UA_Role *foundInNs2 = UA_Server_getRolesByName(server, UA_STRING("CustomRole2"), differentNsUri);
    ck_assert_ptr_nonnull(foundInNs1);
    ck_assert_ptr_nonnull(foundInNs2);
    ck_assert(!UA_NodeId_equal(&foundInNs1->roleId, &foundInNs2->roleId));
    
    UA_String nonExistentNsUri = UA_STRING("http://example.com/NonExistent");
    retval = UA_Server_addRole(server, UA_STRING("FailRole"), nonExistentNsUri, NULL, &roleId4);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    
    UA_QualifiedName browseName2;
    retval = UA_Server_readBrowseName(server, roleId2, &browseName2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role2 = UA_Server_getRolesById(server, roleId2);
    ck_assert_uint_eq(browseName2.namespaceIndex, role2->roleName.namespaceIndex);
    UA_QualifiedName_clear(&browseName2);
    
    UA_Server_removeRole(server, roleId1);
    UA_Server_removeRole(server, roleId2);
    UA_Server_removeRole(server, roleId3);
    UA_Server_removeRole(server, roleId4);
    
    UA_NodeId_clear(&roleId1);
    UA_NodeId_clear(&roleId2);
    UA_NodeId_clear(&roleId3);
    UA_NodeId_clear(&roleId4);
}
END_TEST

START_TEST(Server_rbacBrowseNameNamespaceMatches) {
    /* Test that BrowseName namespace in NS0 matches the role's BrowseName namespace
     * This is critical: the user specifies the namespace via NamespaceUri parameter,
     * and the NS0 representation must preserve this namespace */
    
    /* Test 1: Role in default namespace (nsIdx=1) */
    UA_NodeId roleId1;
    UA_StatusCode retval = UA_Server_addRole(server, UA_STRING("TestRole1"), 
                                              UA_STRING_NULL, NULL, &roleId1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role1 = UA_Server_getRolesById(server, roleId1);
    ck_assert_ptr_nonnull(role1);
    ck_assert_uint_eq(role1->roleName.namespaceIndex, 1); /* Default namespace */
    
    /* Read BrowseName from NS0 node */
    UA_QualifiedName browseName1;
    retval = UA_Server_readBrowseName(server, roleId1, &browseName1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify: NS0 BrowseName namespace MUST match role's BrowseName namespace */
    ck_assert_uint_eq(browseName1.namespaceIndex, role1->roleName.namespaceIndex);
    ck_assert_uint_eq(browseName1.namespaceIndex, 1);
    ck_assert(UA_String_equal(&browseName1.name, &role1->roleName.name));
    UA_QualifiedName_clear(&browseName1);
    
    /* Test 2: Role in custom namespace */
    UA_String customNsUri = UA_STRING("http://example.com/TestNamespace");
    UA_UInt16 customNsIdx = UA_Server_addNamespace(server, "http://example.com/TestNamespace");
    ck_assert(customNsIdx >= 2);
    
    UA_NodeId roleId2;
    retval = UA_Server_addRole(server, UA_STRING("TestRole2"), 
                                customNsUri, NULL, &roleId2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role2 = UA_Server_getRolesById(server, roleId2);
    ck_assert_ptr_nonnull(role2);
    ck_assert_uint_eq(role2->roleName.namespaceIndex, customNsIdx);
    
    /* Read BrowseName from NS0 node */
    UA_QualifiedName browseName2;
    retval = UA_Server_readBrowseName(server, roleId2, &browseName2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify: NS0 BrowseName namespace MUST match role's BrowseName namespace */
    ck_assert_uint_eq(browseName2.namespaceIndex, role2->roleName.namespaceIndex);
    ck_assert_uint_eq(browseName2.namespaceIndex, customNsIdx);
    ck_assert(UA_String_equal(&browseName2.name, &role2->roleName.name));
    UA_QualifiedName_clear(&browseName2);
    
    /* Test 3: Well-known role in NS0 */
    UA_NodeId anonymousRoleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    const UA_Role *anonymousRole = UA_Server_getRolesById(server, anonymousRoleId);
    ck_assert_ptr_nonnull(anonymousRole);
    
    UA_QualifiedName browseNameAnonymous;
    retval = UA_Server_readBrowseName(server, anonymousRoleId, &browseNameAnonymous);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Well-known roles should have BrowseName in NS0 */
    ck_assert_uint_eq(browseNameAnonymous.namespaceIndex, 0);
    ck_assert_uint_eq(anonymousRole->roleName.namespaceIndex, 0);
    UA_QualifiedName_clear(&browseNameAnonymous);
    
    /* Cleanup */
    UA_Server_removeRole(server, roleId1);
    UA_Server_removeRole(server, roleId2);
    UA_NodeId_clear(&roleId1);
    UA_NodeId_clear(&roleId2);
}
END_TEST

START_TEST(Server_rbacNamespaceRemoval) {
    UA_String customNsUri = UA_STRING("http://test.com/Namespace1");
    UA_UInt16 customNsIdx = UA_Server_addNamespace(server, "http://test.com/Namespace1");
    ck_assert(customNsIdx >= 2);
    
    UA_NodeId roleId;
    UA_StatusCode retval = UA_Server_addRole(server, UA_STRING("TestRole"), customNsUri, NULL, &roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    UA_QualifiedName browseName;
    retval = UA_Server_readBrowseName(server, roleId, &browseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(browseName.namespaceIndex, customNsIdx);
    UA_QualifiedName_clear(&browseName);
    
    retval = UA_Server_removeRole(server, roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_readBrowseName(server, roleId, &browseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDUNKNOWN);
    
    const UA_Role *notFound = UA_Server_getRolesById(server, roleId);
    ck_assert_ptr_null(notFound);
    
    UA_NodeId_clear(&roleId);
}
END_TEST

static Suite *testSuite_Server_RBAC(void) {
    Suite *s = suite_create("Server RBAC");
    TCase *tc_rbac = tcase_create("RBAC Information Model");
    tcase_add_unchecked_fixture(tc_rbac, setup, teardown);
    tcase_add_test(tc_rbac, Server_rbacRoleSetExists);
    tcase_add_test(tc_rbac, Server_rbacStandardRolesWithCorrectIds);
    tcase_add_test(tc_rbac, Server_rbacGetRolesById);
    tcase_add_test(tc_rbac, Server_rbacGetRolesByName);
    tcase_add_test(tc_rbac, Server_rbacGetAllRoles);
    tcase_add_test(tc_rbac, Server_rbacAddRoleViaAPI);
    tcase_add_test(tc_rbac, Server_rbacRemoveRoleViaAPI);
    tcase_add_test(tc_rbac, Server_rbacIdentityManagement);
    tcase_add_test(tc_rbac, Server_rbacApplicationManagement);
    tcase_add_test(tc_rbac, Server_rbacProtectMandatoryRoles);
    tcase_add_test(tc_rbac, Server_rbacAllowModifyingOptionalRoles);
    tcase_add_test(tc_rbac, Server_rbacNamespaceHandling);
    tcase_add_test(tc_rbac, Server_rbacBrowseNameNamespaceMatches);
    tcase_add_test(tc_rbac, Server_rbacNamespaceRemoval);
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
