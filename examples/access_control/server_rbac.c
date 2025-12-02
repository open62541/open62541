/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/*
 * This example demonstrates how to configure Role-Based Access Control (RBAC) 
 * in an OPC UA server using:
 * 1. User authentication via username/password
 * 2. Role identity mapping (UserName criteria) for automatic role assignment
 * 3. Runtime role management via Server API
 * 
 * Per OPC UA Part 18, roles are automatically assigned to sessions during 
 * ActivateSession based on the IdentityMappingRuleType configured for each role.
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
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to set default config: %s", UA_StatusCode_name(retval));
        return EXIT_FAILURE;
    }
    
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
                "Configured users: admin, operator, guest (and anonymous)");

    /* Step 2: Create a custom role via ServerConfig with UserName identity mapping
     * This role will be automatically assigned to users named "admin" */
    UA_Role adminRole;
    UA_Role_init(&adminRole);
    
    UA_String adminRoleName = UA_STRING("AdminRole");
    UA_String_copy(&adminRoleName, &adminRole.roleName.name);
    
    /* Define identity mapping: match username "admin" */
    adminRole.imrt = (UA_IdentityMappingRuleType*)
        UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!adminRole.imrt) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to allocate identity mapping rule");
        UA_Role_clear(&adminRole);
        UA_ServerConfig_clear(&config);
        return EXIT_FAILURE;
    }
    adminRole.imrtSize = 1;
    UA_IdentityMappingRuleType_init(&adminRole.imrt[0]);
    adminRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_USERNAME;
    adminRole.imrt[0].criteria = UA_STRING_ALLOC("admin");

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
    UA_NodeId operatorRoleId;
    retval = UA_Server_addRole(server, 
                               UA_STRING("OperatorRole"), 
                               UA_STRING_NULL, 
                               NULL, 
                               &operatorRoleId);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to add OperatorRole: %s", UA_StatusCode_name(retval));
        UA_Server_run_shutdown(server);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    
    /* Add UserName identity mapping for "operator" user */
    retval = UA_Server_addRoleIdentity(server, operatorRoleId,
                                         UA_IDENTITYCRITERIATYPE_USERNAME,
                                         UA_STRING("operator"));
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                    "OperatorRole added with UserName criteria for 'operator'");
    } else {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                       "Failed to add identity rule: %s", UA_StatusCode_name(retval));
    }

    /* Step 4: Configure permissions for the roles */
    UA_NodeId serverStatusId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    UA_UInt32 permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    
    retval = UA_Server_addRolePermissions(server, serverStatusId, operatorRoleId, 
                                          permissions, false, false);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                    "Added BROWSE|READ permissions for OperatorRole on ServerStatus");
    }

    /* Print all available roles */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "=== Available Roles ===");
    size_t allRolesSize = 0;
    UA_NodeId *allRoleIds = NULL;
    retval = UA_Server_getRoles(server, &allRolesSize, &allRoleIds);
    if(retval == UA_STATUSCODE_GOOD) {
        for(size_t i = 0; i < allRolesSize; i++) {
            const UA_Role *role = UA_Server_getRoleById(server, allRoleIds[i]);
            if(role) {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                            "  %.*s - %zu identity rule(s)",
                            (int)role->roleName.name.length, role->roleName.name.data,
                            role->imrtSize);
                for(size_t j = 0; j < role->imrtSize; j++) {
                    const char *criteriaTypeName = "Unknown";
                    switch(role->imrt[j].criteriaType) {
                        case UA_IDENTITYCRITERIATYPE_ANONYMOUS: criteriaTypeName = "Anonymous"; break;
                        case UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER: criteriaTypeName = "AuthenticatedUser"; break;
                        case UA_IDENTITYCRITERIATYPE_USERNAME: criteriaTypeName = "UserName"; break;
                        case UA_IDENTITYCRITERIATYPE_THUMBPRINT: criteriaTypeName = "Thumbprint"; break;
                        case UA_IDENTITYCRITERIATYPE_ROLE: criteriaTypeName = "Role"; break;
                        case UA_IDENTITYCRITERIATYPE_GROUPID: criteriaTypeName = "GroupId"; break;
                        case UA_IDENTITYCRITERIATYPE_APPLICATION: criteriaTypeName = "Application"; break;
                        case UA_IDENTITYCRITERIATYPE_X509SUBJECT: criteriaTypeName = "X509Subject"; break;
                        default: break;
                    }
                    if(role->imrt[j].criteria.length > 0) {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                                    "      -> %s: '%.*s'", criteriaTypeName,
                                    (int)role->imrt[j].criteria.length, 
                                    role->imrt[j].criteria.data);
                    } else {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                                    "      -> %s", criteriaTypeName);
                    }
                }
            }
        }
        UA_Array_delete(allRoleIds, allRolesSize, &UA_TYPES[UA_TYPES_NODEID]);
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "=== Role Assignment ===");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "When clients connect, roles are automatically assigned based on:");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "  - Anonymous login -> Anonymous role + any role with AuthenticatedUser criteria");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "  - user 'admin'    -> AdminRole + any role with AuthenticatedUser criteria");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "  - user 'operator' -> OperatorRole + any role with AuthenticatedUser criteria");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "  - user 'guest'    -> Only roles with AuthenticatedUser criteria");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Server is running...");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Connect with: opc.tcp://localhost:4840");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "Try users: admin/admin123, operator/operator123, guest/guest123");

    /* Run the server until interrupted */
    UA_Server_runUntilInterrupt(server);

    UA_NodeId_clear(&operatorRoleId);
    retval = UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
