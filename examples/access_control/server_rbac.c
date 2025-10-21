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

static volatile UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Received signal %d", sig);
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    /* Step 1: Configure an initial custom role via ServerConfig
     * This role will be configured BEFORE the server is created */
    
    /* First, create and configure the ServerConfig */
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setDefault(&config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to set default config: %s", UA_StatusCode_name(retval));
        return EXIT_FAILURE;
    }
    
    /* Now configure the MaintenanceRole */
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
     * - Alternative: Use UA_Server_new() and add roles later via UA_Server_addRole() API
     */
    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to create server");
        return EXIT_FAILURE;
    }

    /* Start the server (this initializes the namespace and loads RoleType) */
    retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, 
                     "Failed to start server: %s", UA_StatusCode_name(retval));
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    /* Step 2: Add another role at runtime via API
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
                "OperatorRole added via API with NodeId ns=%d;i=%d",
                operatorRoleId.namespaceIndex, operatorRoleId.identifier.numeric);

    if(!running)
        goto cleanup;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Server is running...");
    
    /* Run the server main loop */
    while(running)
        retval = UA_Server_run_iterate(server, true);

cleanup:
    UA_NodeId_clear(&operatorRoleId);
    retval = UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
