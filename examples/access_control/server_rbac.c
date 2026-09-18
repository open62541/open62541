/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * This example demonstrates how to configure Role-Based Access Control (RBAC)
 * in an OPC UA server using:
 * 1. User authentication via username/password
 * 2. Identity mapping for the well-known roles from the server configuration
 * 3. Adding a custom role at runtime through the C API
 * 4. Namespace default role permissions (OPC UA Part 5, 6.3.13)
 * 5. Explicit per-node role permissions with recursive flag
 *
 * Roles are assigned to a Session by matching its identity against the
 * IdentityCriteriaType entries of every role (OPC UA Part 18, 4.4.2):
 * - Anonymous: Matches anonymous sessions
 * - AuthenticatedUser: Matches any authenticated (non-anonymous) session
 * - UserName: Matches if the username equals the criteria string
 *
 * The well-known roles are registered by the server itself, but - apart from
 * Anonymous, AuthenticatedUser and TrustedApplication - none of them has a
 * default identity mapping. Without configuring one, no Session is ever
 * granted ConfigureAdmin or SecurityAdmin. That is what Step 2 is for.
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

    /* When allPermissionsForAnonymous is false, access is denied for nodes
     * without explicit RolePermissions or namespace defaults.
     * Set to true for development/testing to skip permission checks. */
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

    /* Step 2: Give the well-known roles an identity mapping.
     *
     * config.wellKnownRoleMappings is applied to the roles the server registers
     * during startup, so the mapping is in place before the first Session is
     * activated. Each entry names its target by roleName (or roleId) and
     * replaces that role's mapping rules and filters.
     *
     * 'admin' is mapped to both ConfigureAdmin - whose permissions arrive
     * indirectly through the namespace defaults in Step 4 - and SecurityAdmin,
     * which is the role the RoleSet management Methods require. There is no
     * default SecurityAdmin, so without this no client could ever administer
     * the RoleSet over the wire.
     *
     * The mandatory Anonymous, AuthenticatedUser and TrustedApplication roles
     * cannot be remapped; an entry for one of them is rejected. */
    {
        const char *adminRoles[2] = {"ConfigureAdmin", "SecurityAdmin"};
        UA_Role *mappings = (UA_Role*)UA_calloc(2, sizeof(UA_Role));
        if(!mappings) {
            UA_ServerConfig_clear(&config);
            return EXIT_FAILURE;
        }
        for(size_t i = 0; i < 2; i++) {
            UA_Role_init(&mappings[i]);
            mappings[i].roleName = UA_QUALIFIEDNAME_ALLOC(0, adminRoles[i]);
            mappings[i].identityMappingRules = (UA_IdentityMappingRuleType*)
                UA_calloc(1, sizeof(UA_IdentityMappingRuleType));
            if(!mappings[i].identityMappingRules)
                continue;
            UA_IdentityMappingRuleType_init(&mappings[i].identityMappingRules[0]);
            mappings[i].identityMappingRules[0].criteriaType =
                UA_IDENTITYCRITERIATYPE_USERNAME;
            mappings[i].identityMappingRules[0].criteria = UA_STRING_ALLOC("admin");
            mappings[i].identityMappingRulesSize = 1;
        }
        /* Ownership moves to the config; UA_ServerConfig_clear frees it */
        config.wellKnownRoleMappings = mappings;
        config.wellKnownRoleMappingsSize = 2;
    }

    /* Create the server. The well-known roles are registered and the mappings
     * configured above are applied during creation. */
    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Failed to create server");
        return EXIT_FAILURE;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Mapped 'admin' to the ConfigureAdmin and SecurityAdmin roles");

    UA_NodeId configureAdminRoleId = UA_NODEID_NULL;
    {
        UA_Role confAdminRole;
        if(UA_Server_getRole(server, UA_QUALIFIEDNAME(0, "ConfigureAdmin"),
                             &confAdminRole) == UA_STATUSCODE_GOOD) {
            UA_NodeId_copy(&confAdminRole.roleId, &configureAdminRoleId);
            UA_Role_clear(&confAdminRole);
        }
    }

    /* Step 3: Add a custom role at runtime, the counterpart to Step 2.
     * Custom roles can also be defined up front in config.roles (or under the
     * "rbac" key of a JSON server configuration); those are protected and
     * cannot be removed again at runtime. Roles added here can. Either way the
     * identity mapping rules are what decide who is granted the role. */
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

    /* Step 4: Set namespace default permissions for NS0.
     * Per OPC UA Part 5 (6.3.13), if a node has no explicit RolePermissions,
     * the DefaultRolePermissions from the NamespaceMetadata apply.
     * This gives well-known roles their baseline permissions indirectly. */
    {
        UA_RolePermission nsDefaults[3];
        nsDefaults[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
        nsDefaults[0].permissions = UA_PERMISSIONTYPE_BROWSE;
        nsDefaults[1].roleId =
            UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
        nsDefaults[1].permissions = UA_PERMISSIONTYPE_BROWSE | UA_PERMISSIONTYPE_READ;
        nsDefaults[2].roleId =
            UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
        nsDefaults[2].permissions = UA_PERMISSIONTYPE_ALL;

        retval = UA_Server_setNamespaceDefaultRolePermissions(server, 0, 3, nsDefaults);
        if(retval == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "NS0 defaults set: Anonymous=BROWSE, "
                        "AuthenticatedUser=BROWSE|READ, ConfigureAdmin=ALL");
        }
    }

    /* Step 5: Configure explicit permissions on ServerStatus.
     * Explicit RolePermissions override namespace defaults for the node.
     * ALL roles that need access must be listed. */
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

    /* ConfigureAdmin needs explicit listing since node-level overrides defaults */
    if(!UA_NodeId_isNull(&configureAdminRoleId)) {
        retval = UA_Server_addRolePermissions(server, serverStatusId,
                                              configureAdminRoleId,
                                              UA_PERMISSIONTYPE_ALL, false, false);
        if(retval == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Added ALL permissions for ConfigureAdmin on ServerStatus");
        }
    }

    /* Step 6: Configure permissions on BuildInfo node with recursive flag */
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

    /* Add all permissions for ConfigureAdmin on BuildInfo and children (recursive) */
    if(!UA_NodeId_isNull(&configureAdminRoleId)) {
        retval = UA_Server_addRolePermissions(server, buildInfoId,
                                              configureAdminRoleId,
                                              UA_PERMISSIONTYPE_ALL, false, true);
        if(retval == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Added ALL permissions for ConfigureAdmin on BuildInfo "
                        "(recursive)");
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
                        case UA_IDENTITYCRITERIATYPE_TRUSTEDAPPLICATION:
                            criteriaTypeName = "TrustedApplication"; break;
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
                "  - Anonymous login -> Anonymous role\n"
                "  - user 'admin'    -> ConfigureAdmin + SecurityAdmin + "
                "AuthenticatedUser\n"
                "  - user 'operator' -> OperatorRole + AuthenticatedUser\n"
                "  - user 'guest'    -> AuthenticatedUser only\n"
                "Every Session additionally holds the Anonymous role.");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "\n=== Administering the RoleSet ===\n"
                "The RoleSet and its Methods (AddRole, AddIdentity, ...) require\n"
                "the SecurityAdmin role AND an encrypted SecureChannel. This\n"
                "example runs without encryption, so 'admin' can be seen to hold\n"
                "SecurityAdmin but the Methods still answer BadSecurityMode-\n"
                "Insufficient. Configure a SecurityPolicy with SignAndEncrypt to\n"
                "use them.");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "\nServer is running...\n"
                "Connect with: opc.tcp://localhost:4840\n"
                "Try users: admin/admin123, operator/operator123, guest/guest123");

    /* Run the server until interrupted */
    UA_Server_runUntilInterrupt(server);

    UA_NodeId_clear(&operatorRoleId);
    UA_NodeId_clear(&configureAdminRoleId);
    retval = UA_Server_run_shutdown(server);
    UA_Server_delete(server);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
