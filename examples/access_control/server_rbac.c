/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * This example demonstrates how to configure Role-Based Access Control (RBAC)
 * in an OPC UA server using:
 * 1. User authentication via username/password
 * 2. Role identity mapping (UserName criteria) for automatic role assignment
 * 3. Runtime role management via Server API
 *
 * The default access control plugin (ua_accesscontrol_default.c) implements
 * automatic role assignment by evaluating IdentityCriteriaType:
 * - Anonymous: Matches anonymous sessions
 * - AuthenticatedUser: Matches any authenticated (non-anonymous) session
 * - UserName: Matches if the username equals the criteria string
 */

#include <open62541/plugin/accesscontrol.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    /* Step 1: Configure the server with username/password authentication */
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setDefault(&config);

    /* Allow username/password authentication over unencrypted connection (for demo) */
    config.allowNonePolicyPassword = true;

    /* Configure RBAC mode: allPermissionsForAnonymous
     * This controls the default role permissions applied to namespace 0 (NS0)
     * per OPC UA Part 18.
     *
     * If FALSE (recommended for production):
     *   - Anonymous role: BROWSE only
     *   - AuthenticatedUser role: BROWSE | READ
     *   - ConfigureAdmin role: All permissions (BROWSE, READ, WRITE, CALL, etc.)
     *
     * If TRUE (INSECURE - for testing/development only):
     *   - Anonymous role: All permissions
     *
     * These defaults apply to all nodes in NS0 that don't have explicit RolePermissions.
     * Change this to 'true' for testing without authentication. */
    config.allPermissionsForAnonymous = false;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "RBAC Mode: allPermissionsForAnonymous = %s",
                config.allPermissionsForAnonymous ? "true (INSECURE)" : "false (secure)");

    /* Configure users for authentication */
    UA_UsernamePasswordLogin logins[3] = {
        {UA_STRING_STATIC("admin"), UA_STRING_STATIC("admin123")},
        {UA_STRING_STATIC("operator"), UA_STRING_STATIC("operator123")},
        {UA_STRING_STATIC("guest"), UA_STRING_STATIC("guest123")}
    };

    /* Setup access control with the users (allow anonymous too for demo) */
    retval = UA_AccessControl_default(&config, true, NULL, 3, logins);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to configure access control: %s", UA_StatusCode_name(retval));
        UA_ServerConfig_clear(&config);
        return EXIT_FAILURE;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Configured users: admin, operator, guest and anonymous");

    /* Step 2: Create a custom role via ServerConfig with UserName identity mapping.
     * This role will be automatically assigned to users named "admin". */
    UA_Role adminRole;
    UA_Role_init(&adminRole);

    adminRole.roleName = UA_QUALIFIEDNAME_ALLOC(0, "AdminRole");

    /* Define identity mapping: match username "admin" */
    adminRole.identityMappingRules = (UA_IdentityMappingRuleType*)
        UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!adminRole.identityMappingRules) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to allocate identity mapping rule");
        UA_Role_clear(&adminRole);
        UA_ServerConfig_clear(&config);
        return EXIT_FAILURE;
    }
    adminRole.identityMappingRulesSize = 1;
    UA_IdentityMappingRuleType_init(&adminRole.identityMappingRules[0]);
    adminRole.identityMappingRules[0].criteriaType = UA_IDENTITYCRITERIATYPE_USERNAME;
    adminRole.identityMappingRules[0].criteria = UA_STRING_ALLOC("admin");

    /* Add the role to the server configuration */
    config.rolesSize = 1;
    config.roles = (UA_Role*)UA_malloc(sizeof(UA_Role));
    if(!config.roles) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to allocate roles array");
        UA_Role_clear(&adminRole);
        UA_ServerConfig_clear(&config);
        return EXIT_FAILURE;
    }
    config.roles[0] = adminRole;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "AdminRole configured with UserName criteria for 'admin'");

    /* Create the server (roles are initialized during server creation) */
    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to create server");
        return EXIT_FAILURE;
    }

    /* Step 3: Add an OperatorRole at runtime with UserName identity mapping */
    UA_Role operatorRole;
    UA_Role_init(&operatorRole);
    operatorRole.roleName = UA_QUALIFIEDNAME(0, "OperatorRole");

    UA_NodeId operatorRoleId;
    retval = UA_Server_addRole(server, &operatorRole, &operatorRoleId);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to add OperatorRole: %s", UA_StatusCode_name(retval));
        UA_Server_run_shutdown(server);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    /* Add UserName identity mapping for "operator" user */
    {
        UA_Role updRole;
        retval = UA_Server_getRoleById(server, operatorRoleId, &updRole);
        if(retval == UA_STATUSCODE_GOOD) {
            UA_IdentityMappingRuleType *rules = (UA_IdentityMappingRuleType*)
                UA_realloc(updRole.identityMappingRules,
                           (updRole.identityMappingRulesSize + 1) *
                           sizeof(UA_IdentityMappingRuleType));
            if(rules) {
                updRole.identityMappingRules = rules;
                UA_IdentityMappingRuleType_init(&rules[updRole.identityMappingRulesSize]);
                rules[updRole.identityMappingRulesSize].criteriaType =
                    UA_IDENTITYCRITERIATYPE_USERNAME;
                rules[updRole.identityMappingRulesSize].criteria =
                    UA_STRING_ALLOC("operator");
                updRole.identityMappingRulesSize++;
                retval = UA_Server_updateRole(server, &updRole);
            } else {
                retval = UA_STATUSCODE_BADOUTOFMEMORY;
            }
            UA_Role_clear(&updRole);
        }
        if(retval == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "OperatorRole added with UserName criteria for 'operator'");
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                           "Failed to add identity rule: %s", UA_StatusCode_name(retval));
        }
    }

    /* Look up the AdminRole's NodeId (it was created via config) */
    UA_NodeId adminRoleId = UA_NODEID_NULL;
    {
        UA_Role tmpRole;
        retval = UA_Server_getRole(server, UA_QUALIFIEDNAME(0, "AdminRole"), &tmpRole);
        if(retval == UA_STATUSCODE_GOOD) {
            adminRoleId = tmpRole.roleId;
            tmpRole.roleId = UA_NODEID_NULL; /* prevent clear from freeing it */
            UA_Role_clear(&tmpRole);
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                         "Failed to look up AdminRole: %s", UA_StatusCode_name(retval));
        }
    }

    /* Step 4: Configure permissions for the roles on ServerStatus.
     * Once a node has explicit RolePermissions, ONLY listed roles get access.
     * So we must list both OperatorRole and AdminRole. */
    UA_NodeId serverStatusId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    UA_UInt32 permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                            UA_PERMISSIONTYPE_READROLEPERMISSIONS;

    /* Add permissions for OperatorRole on ServerStatus */
    retval = UA_Server_addRolePermissions(server, serverStatusId, operatorRoleId,
                                          permissions, false, false);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Added BROWSE|READ|READROLEPERMISSIONS permissions for "
                    "OperatorRole on ServerStatus");
    }

    /* Add all permissions for AdminRole on ServerStatus */
    if(!UA_NodeId_isNull(&adminRoleId)) {
        retval = UA_Server_addRolePermissions(server, serverStatusId, adminRoleId,
                                              0xFFFFFFFF, false, false);
        if(retval == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Added ALL permissions for AdminRole on ServerStatus");
        }
    }

    /* Step 5: Configure permissions on BuildInfo node with recursive flag */
    UA_NodeId buildInfoId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
    UA_UInt32 buildInfoPermissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ |
                                     UA_PERMISSIONTYPE_READROLEPERMISSIONS |
                                     UA_PERMISSIONTYPE_WRITE;

    /* Add permissions for OperatorRole on BuildInfo and all its children (recursive) */
    retval = UA_Server_addRolePermissions(server, buildInfoId, operatorRoleId,
                                          buildInfoPermissions, false, true);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Added BROWSE|READ|READROLEPERMISSIONS|WRITE permissions for "
                    "OperatorRole on BuildInfo (recursive)");
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to add recursive permissions for OperatorRole on BuildInfo: %s",
                     UA_StatusCode_name(retval));
    }

    /* Add all permissions for AdminRole on BuildInfo and children (recursive) */
    if(!UA_NodeId_isNull(&adminRoleId)) {
        retval = UA_Server_addRolePermissions(server, buildInfoId, adminRoleId,
                                              0xFFFFFFFF, false, true);
        if(retval == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Added ALL permissions for AdminRole on BuildInfo (recursive)");
        }
    }

    /* Verify one child node has permissions set (recursive example) */
    UA_NodeId productUriId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO_PRODUCTURI);
    size_t rpSize = 0;
    UA_RolePermission *rpArr = NULL;
    retval = UA_Server_getNodeRolePermissions(server, productUriId, &rpSize, &rpArr);
    if(retval == UA_STATUSCODE_GOOD && rpSize > 0) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "BuildInfo.ProductUri has %zu role permission entries (via recursive flag)",
                    rpSize);
        UA_Array_delete(rpArr, rpSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    }

    /* Print all available roles */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\n=== Available Roles ===");
    size_t allRolesSize = 0;
    UA_QualifiedName *allRoleNames = NULL;
    retval = UA_Server_getRoles(server, &allRolesSize, &allRoleNames);
    if(retval == UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < allRolesSize; i++) {
            UA_Role role;
            UA_StatusCode res = UA_Server_getRole(server, allRoleNames[i], &role);
            if(res == UA_STATUSCODE_GOOD) {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                            "  %.*s - %zu identity rule(s)",
                            (int)role.roleName.name.length, role.roleName.name.data,
                            role.identityMappingRulesSize);
                for(size_t j = 0; j < role.identityMappingRulesSize; j++) {
                    const char *criteriaTypeName = "Unknown";
                    switch(role.identityMappingRules[j].criteriaType) {
                        case UA_IDENTITYCRITERIATYPE_ANONYMOUS:
                            criteriaTypeName = "Anonymous"; break;
                        case UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER:
                            criteriaTypeName = "AuthenticatedUser"; break;
                        case UA_IDENTITYCRITERIATYPE_USERNAME:
                            criteriaTypeName = "UserName"; break;
                        case UA_IDENTITYCRITERIATYPE_THUMBPRINT:
                            criteriaTypeName = "Thumbprint"; break;
                        case UA_IDENTITYCRITERIATYPE_ROLE:
                            criteriaTypeName = "Role"; break;
                        case UA_IDENTITYCRITERIATYPE_GROUPID:
                            criteriaTypeName = "GroupId"; break;
                        case UA_IDENTITYCRITERIATYPE_APPLICATION:
                            criteriaTypeName = "Application"; break;
                        case UA_IDENTITYCRITERIATYPE_X509SUBJECT:
                            criteriaTypeName = "X509Subject"; break;
                        default: break;
                    }
                    if(role.identityMappingRules[j].criteria.length > 0) {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                    "      -> %s: '%.*s'", criteriaTypeName,
                                    (int)role.identityMappingRules[j].criteria.length,
                                    role.identityMappingRules[j].criteria.data);
                    } else {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                    "      -> %s", criteriaTypeName);
                    }
                }
                UA_Role_clear(&role);
            }
            UA_QualifiedName_clear(&allRoleNames[i]);
        }
        UA_free(allRoleNames);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "\n=== Role Assignment ===\n"
                "When clients connect, roles are automatically assigned based on:\n"
                "  - Anonymous login -> Anonymous role + any role with AuthenticatedUser criteria\n"
                "  - user 'admin'    -> AdminRole + any role with AuthenticatedUser criteria\n"
                "  - user 'operator' -> OperatorRole + any role with AuthenticatedUser criteria\n"
                "  - user 'guest'    -> Only roles with AuthenticatedUser criteria");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "\nServer is running...\n"
                "Connect with: opc.tcp://localhost:4840\n"
                "Try users: admin/admin123, operator/operator123, guest/guest123");

    /* Run the server until interrupted */
    UA_Server_runUntilInterrupt(server);

    UA_NodeId_clear(&operatorRoleId);
    UA_NodeId_clear(&adminRoleId);
    retval = UA_Server_run_shutdown(server);
    UA_Server_delete(server);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
