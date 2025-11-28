/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */


 /*
 * This example demonstrates how to configure roles in an OPC UA server using:
 * 1. Initial role configuration via ServerConfig
 * 2. Runtime role management via Server API
 */

#include <open62541/plugin/accesscontrol.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

int main(void) {

    /* Step 1: Configure an initial custom role via ServerConfig
     * This role will be configured BEFORE the server is created */
    
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setDefault(&config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to set default config: %s", UA_StatusCode_name(retval));
        return EXIT_FAILURE;
    }
    
    UA_Role configRole;
    UA_Role_init(&configRole);
    
    /* Set role name (only the name, namespace will be set automatically) */
    UA_String maintenanceRoleName = UA_STRING("MaintenanceRole");
    UA_String_copy(&maintenanceRoleName, &configRole.roleName.name);
    
    /* Define identity mapping: Only authenticated users */
    configRole.imrt = (UA_IdentityMappingRuleType*)
        UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!configRole.imrt) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to allocate identity mapping rule");
        UA_Role_clear(&configRole);
        UA_ServerConfig_clear(&config);
        return EXIT_FAILURE;
    }
    configRole.imrtSize = 1;
    UA_IdentityMappingRuleType_init(&configRole.imrt[0]);
    configRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    configRole.imrt[0].criteria = UA_STRING_NULL;

    /* Add role to server configuration
     * Note: The server will take ownership of the role during initialization */
    config.rolesSize = 1;
    config.roles = (UA_Role*)UA_malloc(sizeof(UA_Role));
    if(!config.roles) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to allocate roles array");
        UA_Role_clear(&configRole);
        UA_ServerConfig_clear(&config);
        return EXIT_FAILURE;
    }
    config.roles[0] = configRole;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "MaintenanceRole configured via ServerConfig");

    /* Create a new server instance with the configured roles
     * 
     * IMPORTANT: 
     * - UA_Server_newWithConfig takes ownership of the config and initializes the server
     * - During initialization, UA_Server_initializeRBAC() is called, which processes 
     *   all roles from config.roles
     * - Therefore, roles must be added to the config BEFORE creating the server
     * ---- Alternative: Use UA_Server_new() and add roles later via UA_Server_addRole() API
     */
    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to create server");
        return EXIT_FAILURE;
    }

    /* Add another role at runtime via API
     * This demonstrates dynamic role management after server initialization */
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
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                "OperatorRole added via API with NodeId ns=%u;i=%u",
                operatorRoleId.namespaceIndex, operatorRoleId.identifier.numeric);

    /* Demonstrate RolePermissions API
     * Set specific permissions for roles on nodes */
    
    /* a: Grant multiple permissions using bitmask (BROWSE | READ) 
     * on ServerStatus variable for OperatorRole */
    UA_NodeId serverStatusId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS);
    UA_UInt32 permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
    retval = UA_Server_addRolePermissions(server, serverStatusId, operatorRoleId, 
                                          permissions, false, false);
    retval = UA_Server_addRolePermissions(server, serverStatusId, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER), 
                                          permissions, false, false);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                    "Added BROWSE|READ permissions for OperatorRole on ServerStatus");
    } else {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                       "Failed to add permissions: %s (API not yet implemented)", 
                       UA_StatusCode_name(retval));
    }
    
    /* b: Remove a specific permission (demonstrates removal API) */
    retval = UA_Server_removeRolePermissions(server, serverStatusId, operatorRoleId, 
                                             UA_PERMISSIONTYPE_WRITE, false);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                    "Removed WRITE permission for OperatorRole on ServerStatus");
    } else {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                       "Failed to remove WRITE permission: %s (API not yet implemented)", 
                       UA_StatusCode_name(retval));
    }

    /* Demonstrate Session Roles Management API
     * Assign roles dynamically to the admin session */
    
    UA_NodeId adminSessionId = UA_NODEID_GUID(0, (UA_Guid){.data1 = 1});
    
    /* Prepare roles to assign to the session */
    UA_NodeId rolesToSet[2];
    size_t rolesToSetSize = 0;
    
    /* Add the OperatorRole we created in Step 2 */
    rolesToSet[rolesToSetSize++] = operatorRoleId;
    
    /* Add the MaintenanceRole from Step 1 */
    const UA_Role *maintenanceRole = UA_Server_getRoleByName(server, 
                                                              UA_STRING("MaintenanceRole"), 
                                                              UA_STRING_NULL);
    if(maintenanceRole) {
        retval = UA_NodeId_copy(&maintenanceRole->roleId, &rolesToSet[rolesToSetSize]);
        if(retval == UA_STATUSCODE_GOOD) {
            rolesToSetSize++;
        }
    }
    
    /* Assign the roles to the admin session */
    retval = UA_Server_setSessionRoles(server, &adminSessionId, rolesToSetSize, rolesToSet);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                    "Successfully assigned %zu role(s) to admin session", rolesToSetSize);
    } else {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                       "Failed to assign session roles: %s", UA_StatusCode_name(retval));
    }
    
    /* Cleanup: Clear the copied MaintenanceRole ID */
    if(rolesToSetSize > 1) {
        UA_NodeId_clear(&rolesToSet[1]);
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Server is running...");

    /* Run the server until interrupted */
    UA_Server_runUntilInterrupt(server);

    UA_NodeId_clear(&operatorRoleId);
    retval = UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
