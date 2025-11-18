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
    const UA_Role *role = UA_Server_getRoleById(server, roleId);
    
    ck_assert_ptr_nonnull(role);
    ck_assert(UA_NodeId_equal(&role->roleId, &roleId));
}
END_TEST

START_TEST(Server_rbacGetRolesByName) {
    UA_String roleName = UA_STRING("Anonymous");
    UA_String namespaceUri = UA_STRING_NULL;
    const UA_Role *role = UA_Server_getRoleByName(server, roleName, namespaceUri);
    
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
    
    const UA_Role *role = UA_Server_getRoleById(server, newRoleId);
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
    
    const UA_Role *roleBeforeRemove = UA_Server_getRoleById(server, newRoleId);
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
    
    const UA_Role *role = UA_Server_getRoleById(server, newRoleId);
    ck_assert_ptr_nonnull(role);
    size_t initialSize = role->imrtSize;
    
    retval = UA_Server_addRoleIdentity(server, newRoleId, UA_IDENTITYCRITERIATYPE_ANONYMOUS);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role = UA_Server_getRoleById(server, newRoleId);
    ck_assert_uint_eq(role->imrtSize, initialSize + 1);
    
    retval = UA_Server_removeRoleIdentity(server, newRoleId, UA_IDENTITYCRITERIATYPE_ANONYMOUS);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role = UA_Server_getRoleById(server, newRoleId);
    ck_assert_uint_eq(role->imrtSize, initialSize);
    
    UA_NodeId_clear(&newRoleId);
}
END_TEST

START_TEST(Server_rbacApplicationManagement) {
    UA_NodeId newRoleId = UA_NODEID_NULL;
    UA_StatusCode retval = UA_Server_addRole(server, UA_STRING("TestRoleApp"),
                                              UA_STRING_NULL, NULL, &newRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role = UA_Server_getRoleById(server, newRoleId);
    ck_assert_ptr_nonnull(role);
    size_t initialSize = role->applicationsSize;
    
    UA_String appUri = UA_STRING("urn:test:application");
    retval = UA_Server_addRoleApplication(server, newRoleId, appUri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role = UA_Server_getRoleById(server, newRoleId);
    ck_assert_uint_eq(role->applicationsSize, initialSize + 1);
    
    retval = UA_Server_removeRoleApplication(server, newRoleId, appUri);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role = UA_Server_getRoleById(server, newRoleId);
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
    
    const UA_Role *role1 = UA_Server_getRoleById(server, roleId1);
    ck_assert_ptr_nonnull(role1);
    ck_assert_uint_eq(role1->roleName.namespaceIndex, 1);
    
    const UA_Role *foundRole1 = UA_Server_getRoleByName(server, UA_STRING("CustomRole1"), UA_STRING_NULL);
    ck_assert_ptr_nonnull(foundRole1);
    ck_assert(UA_NodeId_equal(&foundRole1->roleId, &roleId1));
    
    UA_String customNsUri = UA_STRING("http://example.com/CustomNamespace");
    UA_UInt16 customNsIdx = UA_Server_addNamespace(server, "http://example.com/CustomNamespace");
    ck_assert(customNsIdx >= 2);
    
    retval = UA_Server_addRole(server, UA_STRING("CustomRole2"), customNsUri, NULL, &roleId2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role2 = UA_Server_getRoleById(server, roleId2);
    ck_assert_ptr_nonnull(role2);
    ck_assert_uint_eq(role2->roleName.namespaceIndex, customNsIdx);
    
    const UA_Role *foundRole2 = UA_Server_getRoleByName(server, UA_STRING("CustomRole2"), customNsUri);
    ck_assert_ptr_nonnull(foundRole2);
    ck_assert(UA_NodeId_equal(&foundRole2->roleId, &roleId2));
    
    const UA_Role *notFound = UA_Server_getRoleByName(server, UA_STRING("CustomRole2"), UA_STRING_NULL);
    ck_assert_ptr_null(notFound);
    
    retval = UA_Server_addRole(server, UA_STRING("CustomRole3"), customNsUri, NULL, &roleId3);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role3 = UA_Server_getRoleById(server, roleId3);
    ck_assert_uint_eq(role3->roleName.namespaceIndex, role2->roleName.namespaceIndex);
    
    UA_NodeId duplicateRoleId;
    retval = UA_Server_addRole(server, UA_STRING("CustomRole2"), customNsUri, NULL, &duplicateRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDEXISTS);
    
    UA_String differentNsUri = UA_STRING("http://example.com/AnotherNamespace");
    UA_UInt16 differentNsIdx = UA_Server_addNamespace(server, "http://example.com/AnotherNamespace");
    ck_assert(differentNsIdx >= 2);
    
    retval = UA_Server_addRole(server, UA_STRING("CustomRole2"), differentNsUri, NULL, &roleId4);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_Role *role4 = UA_Server_getRoleById(server, roleId4);
    ck_assert_uint_eq(role4->roleName.namespaceIndex, differentNsIdx);
    ck_assert(role4->roleName.namespaceIndex != role2->roleName.namespaceIndex);
    
    const UA_Role *foundInNs1 = UA_Server_getRoleByName(server, UA_STRING("CustomRole2"), customNsUri);
    const UA_Role *foundInNs2 = UA_Server_getRoleByName(server, UA_STRING("CustomRole2"), differentNsUri);
    ck_assert_ptr_nonnull(foundInNs1);
    ck_assert_ptr_nonnull(foundInNs2);
    ck_assert(!UA_NodeId_equal(&foundInNs1->roleId, &foundInNs2->roleId));
    
    UA_String nonExistentNsUri = UA_STRING("http://example.com/NonExistent");
    retval = UA_Server_addRole(server, UA_STRING("FailRole"), nonExistentNsUri, NULL, &roleId4);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADINVALIDARGUMENT);
    
    UA_QualifiedName browseName2;
    retval = UA_Server_readBrowseName(server, roleId2, &browseName2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    role2 = UA_Server_getRoleById(server, roleId2);
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
    
    const UA_Role *role1 = UA_Server_getRoleById(server, roleId1);
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
    
    const UA_Role *role2 = UA_Server_getRoleById(server, roleId2);
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
    const UA_Role *anonymousRole = UA_Server_getRoleById(server, anonymousRoleId);
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
    
    const UA_Role *notFound = UA_Server_getRoleById(server, roleId);
    ck_assert_ptr_null(notFound);
    
    UA_NodeId_clear(&roleId);
}
END_TEST

START_TEST(Server_rbacSessionRoleManagement) {
    /* Use the adminSession which is always available */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, 
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    
    /* Initially, admin session should have no roles */
    size_t rolesSize = 0;
    UA_NodeId *roles = NULL;
    UA_StatusCode retval = UA_Server_getSessionRoles(server, &adminSessionId, &rolesSize, &roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, 0);
    ck_assert_ptr_null(roles);
    
    /* Set two roles for the session */
    UA_NodeId rolesToSet[2];
    rolesToSet[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    rolesToSet[1] = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 2, rolesToSet);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify roles were set correctly */
    retval = UA_Server_getSessionRoles(server, &adminSessionId, &rolesSize, &roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, 2);
    ck_assert_ptr_nonnull(roles);
    
    /* Check that the roles match */
    bool foundObserver = false;
    bool foundOperator = false;
    for(size_t i = 0; i < rolesSize; i++) {
        if(UA_NodeId_equal(&roles[i], &rolesToSet[0]))
            foundObserver = true;
        if(UA_NodeId_equal(&roles[i], &rolesToSet[1]))
            foundOperator = true;
    }
    ck_assert(foundObserver);
    ck_assert(foundOperator);
    
    UA_Array_delete(roles, rolesSize, &UA_TYPES[UA_TYPES_NODEID]);
    
    /* Update to a different set of roles */
    UA_NodeId newRoles[1];
    newRoles[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 1, newRoles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify the update */
    retval = UA_Server_getSessionRoles(server, &adminSessionId, &rolesSize, &roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, 1);
    ck_assert(UA_NodeId_equal(&roles[0], &newRoles[0]));
    
    UA_Array_delete(roles, rolesSize, &UA_TYPES[UA_TYPES_NODEID]);
    
    /* Clear all roles by setting empty array */
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 0, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify roles were cleared */
    retval = UA_Server_getSessionRoles(server, &adminSessionId, &rolesSize, &roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, 0);
    ck_assert_ptr_null(roles);
    
    /* Test error handling: invalid session ID */
    UA_NodeId invalidSessionId = UA_NODEID_NUMERIC(0, 999999);
    retval = UA_Server_getSessionRoles(server, &invalidSessionId, &rolesSize, &roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADSESSIONIDINVALID);
    
    /* Test error handling: invalid role ID */
    UA_NodeId invalidRole = UA_NODEID_NUMERIC(0, 999999);
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 1, &invalidRole);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDUNKNOWN);
}
END_TEST

START_TEST(Server_rbacAddSessionRole) {
    /* Use the admin session */
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, 
        (UA_Guid){1, 0, 0, {0,0,0,0,0,0,0,0}});
    
    /* Initially, session should have no roles */
    size_t rolesSize = 0;
    UA_NodeId *roles = NULL;
    UA_StatusCode retval = UA_Server_getSessionRoles(server, &adminSessionId, &rolesSize, &roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, 0);
    
    /* Add first role */
    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    retval = UA_Server_addSessionRole(server, &adminSessionId, observerRole);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify first role was added */
    retval = UA_Server_getSessionRoles(server, &adminSessionId, &rolesSize, &roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, 1);
    ck_assert(UA_NodeId_equal(&roles[0], &observerRole));
    UA_Array_delete(roles, rolesSize, &UA_TYPES[UA_TYPES_NODEID]);
    
    /* Add second role */
    UA_NodeId operatorRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    retval = UA_Server_addSessionRole(server, &adminSessionId, operatorRole);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify both roles are present */
    retval = UA_Server_getSessionRoles(server, &adminSessionId, &rolesSize, &roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, 2);
    
    bool foundObserver = false;
    bool foundOperator = false;
    for(size_t i = 0; i < rolesSize; i++) {
        if(UA_NodeId_equal(&roles[i], &observerRole))
            foundObserver = true;
        if(UA_NodeId_equal(&roles[i], &operatorRole))
            foundOperator = true;
    }
    ck_assert(foundObserver);
    ck_assert(foundOperator);
    UA_Array_delete(roles, rolesSize, &UA_TYPES[UA_TYPES_NODEID]);
    
    /* Try to add the same role again (should be idempotent) */
    retval = UA_Server_addSessionRole(server, &adminSessionId, observerRole);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify still only 2 roles */
    retval = UA_Server_getSessionRoles(server, &adminSessionId, &rolesSize, &roles);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rolesSize, 2);
    UA_Array_delete(roles, rolesSize, &UA_TYPES[UA_TYPES_NODEID]);
    
    /* Test error handling: invalid role */
    UA_NodeId invalidRole = UA_NODEID_NUMERIC(0, 999999);
    retval = UA_Server_addSessionRole(server, &adminSessionId, invalidRole);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDUNKNOWN);
    
    /* Cleanup */
    retval = UA_Server_setSessionRoles(server, &adminSessionId, 0, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(Server_rbacNodePermissionsBasic) {
    /* Create a test variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 42;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId;
    UA_StatusCode retval = UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                      UA_QUALIFIEDNAME(1, "TestVariable"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                      attr, NULL, &testNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Initially, node should have invalid permission index */
    UA_PermissionIndex permIdx;
    retval = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    /* Add permissions for Observer role */
    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    
    retval = UA_Server_addRolePermissions(server, testNodeId, observerRole, permissions, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify permission index was set */
    retval = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(permIdx != UA_PERMISSION_INDEX_INVALID);
    
    /* Verify the permission config contains the role */
    const UA_RolePermissions *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert(UA_NodeId_equal(&rp->entries[0].roleId, &observerRole));
    ck_assert_uint_eq(rp->entries[0].permissions, permissions);
    
    /* Cleanup */
    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(Server_rbacNodePermissionsMultipleRoles) {
    /* Create a test node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "MultiRoleVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 123;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId;
    UA_StatusCode retval = UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                      UA_QUALIFIEDNAME(1, "MultiRoleVariable"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                      attr, NULL, &testNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add permissions for Observer role */
    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    retval = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add permissions for Operator role */
    UA_NodeId operatorRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    retval = UA_Server_addRolePermissions(server, testNodeId, operatorRole,
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify both roles are in the permission config */
    UA_PermissionIndex permIdx;
    retval = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_RolePermissions *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 2);
    
    /* Find both roles in the config */
    bool foundObserver = false;
    bool foundOperator = false;
    for(size_t i = 0; i < rp->entriesSize; i++) {
        if(UA_NodeId_equal(&rp->entries[i].roleId, &observerRole)) {
            foundObserver = true;
            ck_assert_uint_eq(rp->entries[i].permissions,
                            UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
        }
        if(UA_NodeId_equal(&rp->entries[i].roleId, &operatorRole)) {
            foundOperator = true;
            ck_assert_uint_eq(rp->entries[i].permissions,
                            UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE);
        }
    }
    ck_assert(foundObserver);
    ck_assert(foundOperator);
    
    /* Cleanup */
    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(Server_rbacNodePermissionsUpdate) {
    /* Create a test node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "UpdateVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 456;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId;
    UA_StatusCode retval = UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                      UA_QUALIFIEDNAME(1, "UpdateVariable"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                      attr, NULL, &testNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add initial permissions for Observer role */
    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    retval = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                          UA_PERMISSIONTYPE_BROWSE,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get initial permissions */
    UA_PermissionIndex permIdx;
    retval = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_RolePermissions *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert_uint_eq(rp->entries[0].permissions, UA_PERMISSIONTYPE_BROWSE);
    
    /* Update permissions for the same role (should add permissions, not replace) */
    retval = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                          UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_READROLEPERMISSIONS,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify permissions were updated (ORed together) */
    retval = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert_uint_eq(rp->entries[0].permissions,
                    UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_READROLEPERMISSIONS);
    
    /* Cleanup */
    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(Server_rbacNodePermissionsInvalidRole) {
    /* Create a test node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "ErrorTestVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 789;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId;
    UA_StatusCode retval = UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                      UA_QUALIFIEDNAME(1, "ErrorTestVariable"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                      attr, NULL, &testNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Try to add permissions with invalid role */
    UA_NodeId invalidRole = UA_NODEID_NUMERIC(0, 999999);
    retval = UA_Server_addRolePermissions(server, testNodeId, invalidRole,
                                          UA_PERMISSIONTYPE_BROWSE,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDUNKNOWN);
    
    /* Verify node still has invalid permission index */
    UA_PermissionIndex permIdx;
    retval = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    /* Try to add permissions to invalid node */
    UA_NodeId invalidNode = UA_NODEID_NUMERIC(0, 999998);
    UA_NodeId validRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    retval = UA_Server_addRolePermissions(server, invalidNode, validRole,
                                          UA_PERMISSIONTYPE_BROWSE,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODEIDUNKNOWN);
    
    /* Cleanup */
    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(Server_rbacNodePermissionsSharedConfig) {
    /* Create two test nodes */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 100;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SharedNode1");
    UA_NodeId node1;
    UA_StatusCode retval = UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                      UA_QUALIFIEDNAME(1, "SharedNode1"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                      attr, NULL, &node1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SharedNode2");
    UA_NodeId node2;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        UA_QUALIFIEDNAME(1, "SharedNode2"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                        attr, NULL, &node2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add same permissions to both nodes */
    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    
    retval = UA_Server_addRolePermissions(server, node1, observerRole, permissions, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    retval = UA_Server_addRolePermissions(server, node2, observerRole, permissions, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get permission indices */
    UA_PermissionIndex idx1, idx2;
    retval = UA_Server_getNodePermissionIndex(server, node1, &idx1);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    retval = UA_Server_getNodePermissionIndex(server, node2, &idx2);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Both nodes should have valid indices */
    ck_assert(idx1 != UA_PERMISSION_INDEX_INVALID);
    ck_assert(idx2 != UA_PERMISSION_INDEX_INVALID);
    
    /* Verify both configs are correct */
    const UA_RolePermissions *rp1 = UA_Server_getRolePermissionConfig(server, idx1);
    const UA_RolePermissions *rp2 = UA_Server_getRolePermissionConfig(server, idx2);
    ck_assert_ptr_nonnull(rp1);
    ck_assert_ptr_nonnull(rp2);
    
    ck_assert_uint_eq(rp1->entriesSize, 1);
    ck_assert_uint_eq(rp2->entriesSize, 1);
    ck_assert(UA_NodeId_equal(&rp1->entries[0].roleId, &observerRole));
    ck_assert(UA_NodeId_equal(&rp2->entries[0].roleId, &observerRole));
    
    /* Cleanup */
    UA_Server_deleteNode(server, node1, true);
    UA_Server_deleteNode(server, node2, true);
    UA_NodeId_clear(&node1);
    UA_NodeId_clear(&node2);
}
END_TEST

START_TEST(Server_rbacNodePermissionsOverwrite) {
    /* Create a test node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "OverwriteVariable");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Int32 value = 999;
    UA_Variant_setScalar(&attr.value, &value, &UA_TYPES[UA_TYPES_INT32]);
    
    UA_NodeId testNodeId;
    UA_StatusCode retval = UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                      UA_QUALIFIEDNAME(1, "OverwriteVariable"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                      attr, NULL, &testNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add initial permissions for Observer role */
    UA_NodeId observerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    retval = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Get initial permissions */
    UA_PermissionIndex permIdx;
    retval = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    const UA_RolePermissions *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert_uint_eq(rp->entries[0].permissions,
                    UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE);
    
    /* Overwrite permissions with just BROWSE (overwriteExisting=true) */
    retval = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                          UA_PERMISSIONTYPE_BROWSE,
                                          true, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify permissions were replaced, not merged */
    retval = UA_Server_getNodePermissionIndex(server, testNodeId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert_uint_eq(rp->entries[0].permissions, UA_PERMISSIONTYPE_BROWSE);
    
    /* Now merge additional permissions (overwriteExisting=false) */
    retval = UA_Server_addRolePermissions(server, testNodeId, observerRole,
                                          UA_PERMISSIONTYPE_READ,
                                          false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify permissions were merged */
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert_uint_eq(rp->entries[0].permissions, 
                    UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
    
    /* Cleanup */
    UA_Server_deleteNode(server, testNodeId, true);
    UA_NodeId_clear(&testNodeId);
}
END_TEST

START_TEST(Server_rbacNodePermissionsRecursive) {
    /* Create a hierarchical node structure:
     *   ParentObject
     *     |-- Child1 (Organizes)
     *     |-- Child2 (Organizes)
     *          |-- GrandChild1 (HasComponent)
     */
    
    /* Create parent object */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ParentObject");
    UA_NodeId parentId;
    UA_StatusCode retval = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                   UA_QUALIFIEDNAME(1, "ParentObject"),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                   oAttr, NULL, &parentId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create Child1 */
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Child1");
    UA_NodeId child1Id;
    retval = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                     parentId,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                     UA_QUALIFIEDNAME(1, "Child1"),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                     oAttr, NULL, &child1Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create Child2 */
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Child2");
    UA_NodeId child2Id;
    retval = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                     parentId,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                     UA_QUALIFIEDNAME(1, "Child2"),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                     oAttr, NULL, &child2Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create GrandChild1 under Child2 */
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "GrandChild1");
    UA_NodeId grandChild1Id;
    retval = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                     child2Id,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_QUALIFIEDNAME(1, "GrandChild1"),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                     oAttr, NULL, &grandChild1Id);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Apply permissions recursively to parent - should cascade to all children */
    UA_NodeId operatorRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    retval = UA_Server_addRolePermissions(server, parentId, operatorRole, 
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ,
                                          false, true); /* recursive = true */
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify parent has permissions */
    UA_PermissionIndex permIdx;
    retval = UA_Server_getNodePermissionIndex(server, parentId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    const UA_RolePermissions *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert(UA_NodeId_equal(&rp->entries[0].roleId, &operatorRole));
    ck_assert_uint_eq(rp->entries[0].permissions, 
                     UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
    
    /* Verify Child1 has permissions */
    retval = UA_Server_getNodePermissionIndex(server, child1Id, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert(UA_NodeId_equal(&rp->entries[0].roleId, &operatorRole));
    ck_assert_uint_eq(rp->entries[0].permissions, 
                     UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
    
    /* Verify Child2 has permissions */
    retval = UA_Server_getNodePermissionIndex(server, child2Id, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert(UA_NodeId_equal(&rp->entries[0].roleId, &operatorRole));
    
    /* Verify GrandChild1 has permissions */
    retval = UA_Server_getNodePermissionIndex(server, grandChild1Id, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert(UA_NodeId_equal(&rp->entries[0].roleId, &operatorRole));
    
    /* Cleanup */
    UA_Server_deleteNode(server, parentId, true);
    UA_NodeId_clear(&parentId);
    UA_NodeId_clear(&child1Id);
    UA_NodeId_clear(&child2Id);
    UA_NodeId_clear(&grandChild1Id);
}
END_TEST

START_TEST(Server_rbacRemovePermissionsRecursive) {
    /* Create a hierarchical structure */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ParentNode");
    UA_NodeId parentId;
    UA_StatusCode retval = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                   UA_QUALIFIEDNAME(1, "ParentNode"),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                   oAttr, NULL, &parentId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ChildNode");
    UA_NodeId childId;
    retval = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                     parentId,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_QUALIFIEDNAME(1, "ChildNode"),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                     oAttr, NULL, &childId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add permissions recursively */
    UA_NodeId engineerRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    retval = UA_Server_addRolePermissions(server, parentId, engineerRole,
                                          UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE,
                                          false, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify both nodes have permissions */
    UA_PermissionIndex permIdx;
    retval = UA_Server_getNodePermissionIndex(server, parentId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    retval = UA_Server_getNodePermissionIndex(server, childId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    /* Remove WRITE permission recursively */
    retval = UA_Server_removeRolePermissions(server, parentId, engineerRole,
                                             UA_PERMISSIONTYPE_WRITE, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify parent has reduced permissions */
    retval = UA_Server_getNodePermissionIndex(server, parentId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    const UA_RolePermissions *rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert_uint_eq(rp->entries[0].permissions,
                     UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
    
    /* Verify child has reduced permissions */
    retval = UA_Server_getNodePermissionIndex(server, childId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 1);
    ck_assert_uint_eq(rp->entries[0].permissions,
                     UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ);
    
    /* Remove all remaining permissions */
    retval = UA_Server_removeRolePermissions(server, parentId, engineerRole,
                                             UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify parent has no role entry (it was removed) */
    retval = UA_Server_getNodePermissionIndex(server, parentId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 0);
    
    /* Verify child has no role entry */
    retval = UA_Server_getNodePermissionIndex(server, childId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    rp = UA_Server_getRolePermissionConfig(server, permIdx);
    ck_assert_ptr_nonnull(rp);
    ck_assert_uint_eq(rp->entriesSize, 0);
    
    /* Cleanup */
    UA_Server_deleteNode(server, parentId, true);
    UA_NodeId_clear(&parentId);
    UA_NodeId_clear(&childId);
}
END_TEST

START_TEST(Server_rbacSetPermissionIndexRecursive) {
    /* Create hierarchical structure */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "RootNode");
    UA_NodeId rootId;
    UA_StatusCode retval = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                   UA_QUALIFIEDNAME(1, "RootNode"),
                                                   UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                   oAttr, NULL, &rootId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "SubNode");
    UA_NodeId subId;
    retval = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                     rootId,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                     UA_QUALIFIEDNAME(1, "SubNode"),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                     oAttr, NULL, &subId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Create a permission configuration */
    UA_NodeId supervisorRole = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    UA_RolePermissionEntry entry;
    retval = UA_NodeId_copy(&supervisorRole, &entry.roleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    entry.permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | UA_PERMISSIONTYPE_WRITE;
    
    UA_PermissionIndex configIdx;
    retval = UA_Server_addRolePermissionConfig(server, 1, &entry, &configIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_NodeId_clear(&entry.roleId);
    
    /* Set permission index recursively */
    retval = UA_Server_setNodePermissionIndex(server, rootId, configIdx, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify root has the index */
    UA_PermissionIndex permIdx;
    retval = UA_Server_getNodePermissionIndex(server, rootId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, configIdx);
    
    /* Verify sub node has the index */
    retval = UA_Server_getNodePermissionIndex(server, subId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, configIdx);
    
    /* Clear permissions recursively */
    retval = UA_Server_setNodePermissionIndex(server, rootId, UA_PERMISSION_INDEX_INVALID, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify both nodes have invalid index */
    retval = UA_Server_getNodePermissionIndex(server, rootId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    retval = UA_Server_getNodePermissionIndex(server, subId, &permIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(permIdx, UA_PERMISSION_INDEX_INVALID);
    
    /* Cleanup */
    UA_Server_deleteNode(server, rootId, true);
    UA_NodeId_clear(&rootId);
    UA_NodeId_clear(&subId);
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
    tcase_add_test(tc_rbac, Server_rbacSessionRoleManagement);
    tcase_add_test(tc_rbac, Server_rbacAddSessionRole);
    tcase_add_test(tc_rbac, Server_rbacNodePermissionsBasic);
    tcase_add_test(tc_rbac, Server_rbacNodePermissionsMultipleRoles);
    tcase_add_test(tc_rbac, Server_rbacNodePermissionsUpdate);
    tcase_add_test(tc_rbac, Server_rbacNodePermissionsInvalidRole);
    tcase_add_test(tc_rbac, Server_rbacNodePermissionsSharedConfig);
    tcase_add_test(tc_rbac, Server_rbacNodePermissionsOverwrite);
    tcase_add_test(tc_rbac, Server_rbacNodePermissionsRecursive);
    tcase_add_test(tc_rbac, Server_rbacRemovePermissionsRecursive);
    tcase_add_test(tc_rbac, Server_rbacSetPermissionIndexRecursive);
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
