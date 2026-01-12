/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* RBAC Client-Server Integration Tests
 * 
 * This file tests the full client-server flow for RBAC:
 * - Username login with automatic role assignment
 * - Role-based access control enforcement
 * - UserRolePermissions attribute visibility
 */

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/accesscontrol_default.h>

#include "test_helpers.h"
#include "thread_wrapper.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

static UA_Server *server;
static UA_Boolean running;
static THREAD_HANDLE server_thread;

/* Users for authentication */
static UA_UsernamePasswordLogin usernamePasswords[3] = {
    {UA_STRING_STATIC("operator"), UA_STRING_STATIC("password")},
    {UA_STRING_STATIC("admin"), UA_STRING_STATIC("admin123")},
    {UA_STRING_STATIC("guest"), UA_STRING_STATIC("guest123")}
};

THREAD_CALLBACK(serverloop) {
    while(running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->allowNonePolicyPassword = true;

    /* Configure AccessControl with usernames */
    UA_SecurityPolicy *sp = &sc->securityPolicies[sc->securityPoliciesSize-1];
    UA_AccessControl_default(sc, true, &sp->policyUri, 3, usernamePasswords);

    /* Create a custom role "OperatorRole" with UserName identity mapping for "operator" */
    UA_NodeId operatorRoleId;
    UA_StatusCode retval = UA_Server_addRole(server, UA_STRING("OperatorRole"), 
                                              UA_STRING_NULL, NULL, &operatorRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add identity mapping for username "operator" */
    retval = UA_Server_addRoleIdentity(server, operatorRoleId,
                                       UA_IDENTITYCRITERIATYPE_USERNAME,
                                       UA_STRING("operator"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Add permissions for this role on ServerStatus node 
     * Include ReadRolePermissions so the role can read the RolePermissions attribute */
    UA_NodeId serverStatusId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ | 
                                    UA_PERMISSIONTYPE_READROLEPERMISSIONS;
    retval = UA_Server_addRolePermissions(server, serverStatusId, operatorRoleId, 
                                          permissions, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Verify role was created with identity mapping */
    const UA_Role *role = UA_Server_getRoleById(server, operatorRoleId);
    ck_assert_ptr_nonnull(role);
    ck_assert_uint_eq(role->imrtSize, 1);
    ck_assert_uint_eq(role->imrt[0].criteriaType, UA_IDENTITYCRITERIATYPE_USERNAME);
    
    /* Print role info for debugging */
    printf("OperatorRole created with NodeId: ns=%u, i=%u\n", 
           operatorRoleId.namespaceIndex, operatorRoleId.identifier.numeric);
    printf("OperatorRole identity mapping: %zu rule(s)\n", role->imrtSize);
    for(size_t i = 0; i < role->imrtSize; i++) {
        printf("  Rule %zu: criteriaType=%d, criteria='%.*s'\n", 
               i, role->imrt[i].criteriaType,
               (int)role->imrt[i].criteria.length, role->imrt[i].criteria.data);
    }

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
    
    UA_NodeId_clear(&operatorRoleId);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* Test that username login triggers automatic role assignment */
START_TEST(Client_login_assigns_roles) {
    UA_Client *client = UA_Client_newForUnitTest();
    
    /* Connect with username "operator" */
    UA_StatusCode retval = UA_Client_connectUsername(client, 
                                                      "opc.tcp://localhost:4840", 
                                                      "operator", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    
    /* Read ServerStatus node's RolePermissions and UserRolePermissions attributes */
    UA_ReadValueId rvid[2];
    UA_ReadValueId_init(&rvid[0]);
    UA_ReadValueId_init(&rvid[1]);
    
    rvid[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    rvid[0].attributeId = UA_ATTRIBUTEID_ROLEPERMISSIONS;
    
    rvid[1].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    rvid[1].attributeId = UA_ATTRIBUTEID_USERROLEPERMISSIONS;
    
    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = rvid;
    req.nodesToReadSize = 2;
    
    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.resultsSize, 2);
    
    /* Check RolePermissions - should have at least 1 entry (for OperatorRole) */
    ck_assert_uint_eq(resp.results[0].status, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(resp.results[0].value.type, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    
    size_t rpCount = resp.results[0].value.arrayLength;
    if(rpCount == 0 && resp.results[0].value.data) rpCount = 1;
    printf("RolePermissions count: %zu\n", rpCount);
    ck_assert_uint_ge(rpCount, 1);
    
    UA_RolePermissionType *rp = (UA_RolePermissionType*)resp.results[0].value.data;
    for(size_t i = 0; i < rpCount; i++) {
        printf("  RolePermission[%zu]: roleId=ns=%u;i=%u, permissions=0x%08x\n",
               i, rp[i].roleId.namespaceIndex, rp[i].roleId.identifier.numeric, 
               rp[i].permissions);
    }
    
    /* Check UserRolePermissions - should have 1 entry (for the operator's role) */
    ck_assert_uint_eq(resp.results[1].status, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(resp.results[1].value.type, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    
    size_t urpCount = resp.results[1].value.arrayLength;
    if(urpCount == 0 && resp.results[1].value.data) urpCount = 1;
    printf("UserRolePermissions count: %zu\n", urpCount);
    
    /* This is the key assertion - the operator user should see their role's permissions */
    ck_assert_uint_ge(urpCount, 1);
    
    UA_RolePermissionType *urp = (UA_RolePermissionType*)resp.results[1].value.data;
    for(size_t i = 0; i < urpCount; i++) {
        printf("  UserRolePermission[%zu]: roleId=ns=%u;i=%u, permissions=0x%08x\n",
               i, urp[i].roleId.namespaceIndex, urp[i].roleId.identifier.numeric,
               urp[i].permissions);
    }
    
    UA_ReadResponse_clear(&resp);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

static Suite *testSuite_Server_RBAC_Client(void) {
    Suite *s = suite_create("Server RBAC Client Integration");
    TCase *tc = tcase_create("Client Login and Role Assignment");
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, Client_login_assigns_roles);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = testSuite_Server_RBAC_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
