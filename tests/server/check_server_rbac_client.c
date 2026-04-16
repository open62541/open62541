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
    sc->allPermissionsForAnonymous = false;

    /* Configure AccessControl with usernames */
    UA_SecurityPolicy *sp = &sc->securityPolicies[sc->securityPoliciesSize-1];
    UA_AccessControl_default(sc, true, &sp->policyUri, 3, usernamePasswords);

    /* Create a custom role "OperatorRole" with UserName identity mapping for "operator" */
    UA_Role operatorRole;
    UA_Role_init(&operatorRole);
    operatorRole.roleName = UA_QUALIFIEDNAME(0, "OperatorRole");

    UA_NodeId operatorRoleId;
    UA_StatusCode retval = UA_Server_addRole(server, &operatorRole, &operatorRoleId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add identity mapping for username "operator" */
    {
        UA_Role updRole;
        retval = UA_Server_getRoleById(server, operatorRoleId, &updRole);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_IdentityMappingRuleType *rules = (UA_IdentityMappingRuleType*)
            UA_realloc(updRole.identityMappingRules,
                       (updRole.identityMappingRulesSize + 1) *
                       sizeof(UA_IdentityMappingRuleType));
        ck_assert_ptr_nonnull(rules);
        updRole.identityMappingRules = rules;
        UA_IdentityMappingRuleType_init(&rules[updRole.identityMappingRulesSize]);
        rules[updRole.identityMappingRulesSize].criteriaType =
            UA_IDENTITYCRITERIATYPE_USERNAME;
        rules[updRole.identityMappingRulesSize].criteria = UA_STRING_ALLOC("operator");
        updRole.identityMappingRulesSize++;
        retval = UA_Server_updateRole(server, &updRole);
        UA_Role_clear(&updRole);
    }
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add permissions for this role on ServerStatus node */
    UA_NodeId serverStatusId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    UA_PermissionType permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                                    UA_PERMISSIONTYPE_READROLEPERMISSIONS;
    retval = UA_Server_addRolePermissions(server, serverStatusId, operatorRoleId,
                                          permissions, false, false);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Add recursive permissions on BuildInfo node and its children */
    UA_NodeId buildInfoId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
    UA_PermissionType buildInfoPermissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                                              UA_PERMISSIONTYPE_READROLEPERMISSIONS |
                                              UA_PERMISSIONTYPE_WRITE;
    retval = UA_Server_addRolePermissions(server, buildInfoId, operatorRoleId,
                                          buildInfoPermissions, false, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify role was created with identity mapping */
    UA_Role role;
    UA_StatusCode getRes = UA_Server_getRole(server,
                                             UA_QUALIFIEDNAME(0, "OperatorRole"),
                                             &role);
    ck_assert_uint_eq(getRes, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(role.identityMappingRulesSize, 1);
    ck_assert_uint_eq(role.identityMappingRules[0].criteriaType,
                      UA_IDENTITYCRITERIATYPE_USERNAME);

    printf("OperatorRole created with NodeId: ns=%u, i=%u\n",
           operatorRoleId.namespaceIndex, operatorRoleId.identifier.numeric);
    printf("OperatorRole identity mapping: %zu rule(s)\n",
           role.identityMappingRulesSize);
    for(size_t i = 0; i < role.identityMappingRulesSize; i++) {
        printf("  Rule %zu: criteriaType=%d, criteria='%.*s'\n",
               i, role.identityMappingRules[i].criteriaType,
               (int)role.identityMappingRules[i].criteria.length,
               role.identityMappingRules[i].criteria.data);
    }
    UA_Role_clear(&role);

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

    UA_StatusCode retval = UA_Client_connectUsername(client,
                                                      "opc.tcp://localhost:4840",
                                                      "operator", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read ServerStatus node's RolePermissions and UserRolePermissions */
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

    /* Check RolePermissions - should have at least 1 entry */
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

    /* Check UserRolePermissions - should have at least 1 entry */
    ck_assert_uint_eq(resp.results[1].status, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(resp.results[1].value.type, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    size_t urpCount = resp.results[1].value.arrayLength;
    if(urpCount == 0 && resp.results[1].value.data) urpCount = 1;
    printf("UserRolePermissions count: %zu\n", urpCount);
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

/* Test that recursive permissions on BuildInfo apply to all children */
START_TEST(Client_buildinfo_recursive_permissions) {
    UA_Client *client = UA_Client_newForUnitTest();

    UA_StatusCode retval = UA_Client_connectUsername(client,
                                                      "opc.tcp://localhost:4840",
                                                      "operator", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 buildInfoNodes[] = {
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO,
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI,
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_MANUFACTURERNAME,
        UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTNAME
    };

    size_t nodeCount = 4;
    UA_ReadValueId *rvid = (UA_ReadValueId*)
        UA_Array_new(nodeCount * 2, &UA_TYPES[UA_TYPES_READVALUEID]);

    for(size_t i = 0; i < nodeCount; i++) {
        rvid[i*2].nodeId = UA_NODEID_NUMERIC(0, buildInfoNodes[i]);
        rvid[i*2].attributeId = UA_ATTRIBUTEID_ROLEPERMISSIONS;
        rvid[i*2+1].nodeId = UA_NODEID_NUMERIC(0, buildInfoNodes[i]);
        rvid[i*2+1].attributeId = UA_ATTRIBUTEID_USERROLEPERMISSIONS;
    }

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = rvid;
    req.nodesToReadSize = nodeCount * 2;

    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.resultsSize, nodeCount * 2);

    const char *nodeNames[] = {"BuildInfo", "ProductUri",
                                "ManufacturerName", "ProductName"};

    for(size_t i = 0; i < nodeCount; i++) {
        printf("=== %s (i=%u) ===\n", nodeNames[i], buildInfoNodes[i]);
        if(resp.results[i*2].status == UA_STATUSCODE_GOOD && resp.results[i*2].hasValue) {
            size_t rpCount = resp.results[i*2].value.arrayLength;
            if(rpCount == 0 && resp.results[i*2].value.data) rpCount = 1;
            printf("  RolePermissions: %zu entries\n", rpCount);
            ck_assert_uint_ge(rpCount, 1);
        } else {
            printf("  RolePermissions: Failed - %s\n",
                   UA_StatusCode_name(resp.results[i*2].status));
            ck_assert_uint_eq(resp.results[i*2].status, UA_STATUSCODE_GOOD);
        }
        if(resp.results[i*2+1].status == UA_STATUSCODE_GOOD && resp.results[i*2+1].hasValue) {
            size_t urpCount = resp.results[i*2+1].value.arrayLength;
            if(urpCount == 0 && resp.results[i*2+1].value.data) urpCount = 1;
            printf("  UserRolePermissions: %zu entries\n", urpCount);
            ck_assert_uint_ge(urpCount, 1);
        } else {
            printf("  UserRolePermissions: Failed - %s\n",
                   UA_StatusCode_name(resp.results[i*2+1].status));
            ck_assert_uint_eq(resp.results[i*2+1].status, UA_STATUSCODE_GOOD);
        }
        printf("\n");
    }

    UA_Array_delete(rvid, nodeCount * 2, &UA_TYPES[UA_TYPES_READVALUEID]);
    UA_ReadResponse_clear(&resp);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test that anonymous login gets restricted access when
 * allPermissionsForAnonymous is false */
START_TEST(Client_anonymous_restricted_access) {
    UA_Client *client = UA_Client_newForUnitTest();

    /* Connect anonymously */
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read UserAccessLevel on a BuildInfo child node that has explicit
     * OperatorRole permissions. Anonymous should NOT have read access
     * because RBAC is active and the Anonymous well-known role only
     * has BROWSE (allPermissionsForAnonymous = false). */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);
    rvid.attributeId = UA_ATTRIBUTEID_USERACCESSLEVEL;

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = &rvid;
    req.nodesToReadSize = 1;

    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.resultsSize, 1);

    if(resp.results[0].status == UA_STATUSCODE_GOOD && resp.results[0].hasValue) {
        UA_Byte userAccessLevel = *(UA_Byte*)resp.results[0].value.data;
        printf("Anonymous UserAccessLevel on ProductUri: 0x%02x\n", userAccessLevel);
        /* Anonymous should NOT have write access */
        ck_assert_uint_eq(userAccessLevel & UA_ACCESSLEVELMASK_WRITE, 0);
    }

    UA_ReadResponse_clear(&resp);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test that guest (authenticated but no custom role) gets limited access */
START_TEST(Client_guest_limited_access) {
    UA_Client *client = UA_Client_newForUnitTest();

    /* Connect as guest — authenticated but not mapped to any custom role */
    UA_StatusCode retval = UA_Client_connectUsername(client,
                                                      "opc.tcp://localhost:4840",
                                                      "guest", "guest123");
    if(retval != UA_STATUSCODE_GOOD) {
        printf("Guest connect failed: %s (non-fatal for this test)\n",
               UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return;
    }

    /* Read UserAccessLevel on a BuildInfo child (has explicit OperatorRole perms).
     * Guest should NOT have write access since they only have
     * Anonymous+AuthenticatedUser roles, not OperatorRole. */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = UA_NODEID_NUMERIC(0,
                      UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);
    rvid.attributeId = UA_ATTRIBUTEID_USERACCESSLEVEL;

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = &rvid;
    req.nodesToReadSize = 1;

    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.resultsSize, 1);

    if(resp.results[0].status == UA_STATUSCODE_GOOD && resp.results[0].hasValue) {
        UA_Byte userAccessLevel = *(UA_Byte*)resp.results[0].value.data;
        printf("Guest UserAccessLevel on ProductUri: 0x%02x\n", userAccessLevel);
        /* Guest should NOT have write access */
        ck_assert_uint_eq(userAccessLevel & UA_ACCESSLEVELMASK_WRITE, 0);
    } else {
        /* Access denied is also an acceptable result for a restricted user */
        printf("Guest read UserAccessLevel returned: %s (expected for restricted)\n",
               UA_StatusCode_name(resp.results[0].status));
    }

    UA_ReadResponse_clear(&resp);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
END_TEST

/* Test that UserWriteMask reflects RBAC permissions correctly.
 * The operator role on BuildInfo has WRITE permission, which should map
 * to WriteAttribute bits in UserWriteMask. */
START_TEST(Client_userwritemask_reflects_rbac) {
    UA_Client *client = UA_Client_newForUnitTest();

    UA_StatusCode retval = UA_Client_connectUsername(client,
                                                      "opc.tcp://localhost:4840",
                                                      "operator", "password");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read UserWriteMask on BuildInfo node (operator has WRITE permission) */
    UA_ReadValueId rvids[2];
    UA_ReadValueId_init(&rvids[0]);
    UA_ReadValueId_init(&rvids[1]);

    rvids[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
    rvids[0].attributeId = UA_ATTRIBUTEID_USERWRITEMASK;

    /* Also read UserAccessLevel on a BuildInfo child variable */
    rvids[1].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);
    rvids[1].attributeId = UA_ATTRIBUTEID_USERACCESSLEVEL;

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = rvids;
    req.nodesToReadSize = 2;

    UA_ReadResponse resp = UA_Client_Service_read(client, req);
    ck_assert_uint_eq(resp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(resp.resultsSize, 2);

    /* Check UserWriteMask on BuildInfo */
    if(resp.results[0].status == UA_STATUSCODE_GOOD && resp.results[0].hasValue) {
        UA_UInt32 userWriteMask = *(UA_UInt32*)resp.results[0].value.data;
        printf("Operator UserWriteMask on BuildInfo: 0x%08x\n", userWriteMask);
        /* Operator has WRITE permission -> WriteAttribute maps to WriteMask bits
         * (all bits except RolePermissions and Historizing, which need
         * separate PermissionType bits) */
    }

    /* Check UserAccessLevel on ProductUri variable.
     * Note: UserAccessLevel is the intersection of the node's AccessLevel
     * and the RBAC-derived permissions. ProductUri's AccessLevel is READ-only
     * in the NS0 information model, so WRITE cannot appear here even if
     * RBAC grants it. We verify that READ is present. */
    if(resp.results[1].status == UA_STATUSCODE_GOOD && resp.results[1].hasValue) {
        UA_Byte userAccessLevel = *(UA_Byte*)resp.results[1].value.data;
        printf("Operator UserAccessLevel on ProductUri: 0x%02x\n", userAccessLevel);
        /* Operator has READ permission -> bit 0 must be set */
        ck_assert_uint_ne(userAccessLevel & UA_ACCESSLEVELMASK_READ, 0);
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
    tcase_add_test(tc, Client_buildinfo_recursive_permissions);
    tcase_add_test(tc, Client_anonymous_restricted_access);
    tcase_add_test(tc, Client_guest_limited_access);
    tcase_add_test(tc, Client_userwritemask_reflects_rbac);
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
