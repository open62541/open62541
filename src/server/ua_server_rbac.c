/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "ua_server_internal.h"
#include "ua_server_rbac.h"

#ifdef UA_ENABLE_RBAC

/* RBAC implementation. Permission configurations are deduplicated internally;
 * nodes sharing the same role permissions reference a shared entry via a
 * compact permission index in the node head. */

/*********************************/
/* UA_RolePermissionSet Type API */
/*********************************/

void UA_EXPORT
UA_RolePermissionSet_init(UA_RolePermissionSet *rps) {
    if(!rps)
        return;
    rps->rolePermissionsSize = 0;
    rps->rolePermissions = NULL;
}

void UA_EXPORT
UA_RolePermissionSet_clear(UA_RolePermissionSet *rps) {
    if(!rps)
        return;
    if(rps->rolePermissions) {
        for(size_t i = 0; i < rps->rolePermissionsSize; i++)
            UA_NodeId_clear(&rps->rolePermissions[i].roleId);
        UA_free(rps->rolePermissions);
    }
    rps->rolePermissionsSize = 0;
    rps->rolePermissions = NULL;
}

UA_StatusCode UA_EXPORT
UA_RolePermissionSet_copy(const UA_RolePermissionSet *src,
                          UA_RolePermissionSet *dst) {
    if(!src || !dst)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_RolePermissionSet_init(dst);

    if(src->rolePermissionsSize > 0 && src->rolePermissions) {
        dst->rolePermissions = (UA_RolePermission*)
            UA_calloc(src->rolePermissionsSize, sizeof(UA_RolePermission));
        if(!dst->rolePermissions)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dst->rolePermissionsSize = src->rolePermissionsSize;

        for(size_t i = 0; i < src->rolePermissionsSize; i++) {
            UA_StatusCode res = UA_NodeId_copy(&src->rolePermissions[i].roleId,
                                               &dst->rolePermissions[i].roleId);
            if(res != UA_STATUSCODE_GOOD) {
                UA_RolePermissionSet_clear(dst);
                return res;
            }
            dst->rolePermissions[i].permissions = src->rolePermissions[i].permissions;
        }
    }

    return UA_STATUSCODE_GOOD;
}

/**********************/
/* UA_Role Type API   */
/**********************/

void UA_EXPORT
UA_Role_init(UA_Role *role) {
    if(!role)
        return;
    UA_NodeId_init(&role->roleId);
    UA_QualifiedName_init(&role->roleName);
    role->identityMappingRulesSize = 0;
    role->identityMappingRules = NULL;
    role->applicationsExclude = false;
    role->applicationsSize = 0;
    role->applications = NULL;
    role->endpointsExclude = false;
    role->endpointsSize = 0;
    role->endpoints = NULL;
}

void UA_EXPORT
UA_Role_clear(UA_Role *role) {
    if(!role)
        return;
    UA_NodeId_clear(&role->roleId);
    UA_QualifiedName_clear(&role->roleName);

    if(role->identityMappingRules) {
        for(size_t i = 0; i < role->identityMappingRulesSize; i++)
            UA_IdentityMappingRuleType_clear(&role->identityMappingRules[i]);
        UA_free(role->identityMappingRules);
    }

    if(role->applications) {
        for(size_t i = 0; i < role->applicationsSize; i++)
            UA_String_clear(&role->applications[i]);
        UA_free(role->applications);
    }

    if(role->endpoints) {
        for(size_t i = 0; i < role->endpointsSize; i++)
            UA_EndpointType_clear(&role->endpoints[i]);
        UA_free(role->endpoints);
    }
    UA_Role_init(role);
}

UA_StatusCode UA_EXPORT
UA_Role_copy(const UA_Role *src, UA_Role *dst) {
    if(!src || !dst)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Role_init(dst);

    UA_StatusCode res = UA_NodeId_copy(&src->roleId, &dst->roleId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = UA_QualifiedName_copy(&src->roleName, &dst->roleName);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&dst->roleId);
        return res;
    }

    if(src->identityMappingRulesSize > 0) {
        dst->identityMappingRules = (UA_IdentityMappingRuleType*)
            UA_calloc(src->identityMappingRulesSize, sizeof(UA_IdentityMappingRuleType));
        if(!dst->identityMappingRules) {
            UA_Role_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->identityMappingRulesSize = src->identityMappingRulesSize;
        for(size_t i = 0; i < src->identityMappingRulesSize; i++) {
            res = UA_IdentityMappingRuleType_copy(&src->identityMappingRules[i],
                                                  &dst->identityMappingRules[i]);
            if(res != UA_STATUSCODE_GOOD) {
                UA_Role_clear(dst);
                return res;
            }
        }
    }

    dst->applicationsExclude = src->applicationsExclude;
    if(src->applicationsSize > 0) {
        dst->applications = (UA_String*)
            UA_calloc(src->applicationsSize, sizeof(UA_String));
        if(!dst->applications) {
            UA_Role_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->applicationsSize = src->applicationsSize;
        for(size_t i = 0; i < src->applicationsSize; i++) {
            res = UA_String_copy(&src->applications[i], &dst->applications[i]);
            if(res != UA_STATUSCODE_GOOD) {
                UA_Role_clear(dst);
                return res;
            }
        }
    }

    dst->endpointsExclude = src->endpointsExclude;
    if(src->endpointsSize > 0) {
        dst->endpoints = (UA_EndpointType*)
            UA_calloc(src->endpointsSize, sizeof(UA_EndpointType));
        if(!dst->endpoints) {
            UA_Role_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->endpointsSize = src->endpointsSize;
        for(size_t i = 0; i < src->endpointsSize; i++) {
            res = UA_EndpointType_copy(&src->endpoints[i], &dst->endpoints[i]);
            if(res != UA_STATUSCODE_GOOD) {
                UA_Role_clear(dst);
                return res;
            }
        }
    }

    return UA_STATUSCODE_GOOD;
}

UA_Boolean UA_EXPORT
UA_Role_equal(const UA_Role *r1, const UA_Role *r2) {
    if(!r1 || !r2)
        return false;
    if(!UA_NodeId_equal(&r1->roleId, &r2->roleId))
        return false;
    if(!UA_QualifiedName_equal(&r1->roleName, &r2->roleName))
        return false;
    if(r1->identityMappingRulesSize != r2->identityMappingRulesSize)
        return false;
    for(size_t i = 0; i < r1->identityMappingRulesSize; i++) {
        if(!UA_IdentityMappingRuleType_equal(&r1->identityMappingRules[i],
                                             &r2->identityMappingRules[i]))
            return false;
    }
    if(r1->applicationsExclude != r2->applicationsExclude)
        return false;
    if(r1->applicationsSize != r2->applicationsSize)
        return false;
    for(size_t i = 0; i < r1->applicationsSize; i++) {
        if(!UA_String_equal(&r1->applications[i], &r2->applications[i]))
            return false;
    }
    if(r1->endpointsExclude != r2->endpointsExclude)
        return false;
    if(r1->endpointsSize != r2->endpointsSize)
        return false;
    for(size_t i = 0; i < r1->endpointsSize; i++) {
        if(!UA_EndpointType_equal(&r1->endpoints[i], &r2->endpoints[i]))
            return false;
    }
    return true;
}

/*****************************************/
/* Internal UA_RolePermissionEntry Helpers */
/*****************************************/

static void
rolePermissionEntry_init(UA_RolePermissionEntry *rp) {
    rp->rolePermissionsSize = 0;
    rp->rolePermissions = NULL;
    rp->refCount = 0;
}

static void
rolePermissionEntry_clear(UA_RolePermissionEntry *rp) {
    if(rp->rolePermissions) {
        for(size_t i = 0; i < rp->rolePermissionsSize; i++)
            UA_NodeId_clear(&rp->rolePermissions[i].roleId);
        UA_free(rp->rolePermissions);
    }
    rolePermissionEntry_init(rp);
}

/* Copy an array of UA_RolePermission into a new allocation */
static UA_StatusCode
copyRolePermissionArray(size_t srcSize, const UA_RolePermission *src,
                        size_t *dstSize, UA_RolePermission **dst) {
    if(srcSize == 0 || !src) {
        *dstSize = 0;
        *dst = NULL;
        return UA_STATUSCODE_GOOD;
    }

    UA_RolePermission *arr = (UA_RolePermission*)
        UA_calloc(srcSize, sizeof(UA_RolePermission));
    if(!arr)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    for(size_t i = 0; i < srcSize; i++) {
        UA_StatusCode res = UA_NodeId_copy(&src[i].roleId, &arr[i].roleId);
        if(res != UA_STATUSCODE_GOOD) {
            for(size_t j = 0; j < i; j++)
                UA_NodeId_clear(&arr[j].roleId);
            UA_free(arr);
            return res;
        }
        arr[i].permissions = src[i].permissions;
    }

    *dstSize = srcSize;
    *dst = arr;
    return UA_STATUSCODE_GOOD;
}

/* Compare two role permission arrays for equality */
static UA_Boolean
compareRolePermissions(size_t size1, const UA_RolePermission *rp1,
                       size_t size2, const UA_RolePermission *rp2) {
    if(size1 != size2)
        return false;
    for(size_t i = 0; i < size1; i++) {
        if(!UA_NodeId_equal(&rp1[i].roleId, &rp2[i].roleId) ||
           rp1[i].permissions != rp2[i].permissions)
            return false;
    }
    return true;
}

/* Find or create a role-permission entry in the server's internal array.
 * Returns the index of the matching or new entry.
 * Must be called with the server lock held. */
static UA_StatusCode
findOrCreateRolePermissions(UA_Server *server,
                            size_t rpSize, const UA_RolePermission *rp,
                            UA_PermissionIndex *outIndex) {
    /* Check for an existing identical entry */
    for(size_t i = 0; i < server->rolePermissionsSize; i++) {
        const UA_RolePermissionEntry *existing = &server->rolePermissions[i];
        if(compareRolePermissions(rpSize, rp,
                                  existing->rolePermissionsSize,
                                  existing->rolePermissions)) {
            *outIndex = (UA_PermissionIndex)i;
            return UA_STATUSCODE_GOOD;
        }
    }

    /* Try to reuse a slot whose refCount has dropped to zero
     * (skip protected entries from the server config). */
    for(size_t i = 0; i < server->rolePermissionsSize; i++) {
        UA_RolePermissionEntry *candidate = &server->rolePermissions[i];
        if(candidate->refCount == 0 &&
           candidate->refCount != UA_ROLEPERMISSIONS_REFCOUNT_PROTECTED) {
            rolePermissionEntry_clear(candidate);
            rolePermissionEntry_init(candidate);
            UA_StatusCode res = copyRolePermissionArray(rpSize, rp,
                                                        &candidate->rolePermissionsSize,
                                                        &candidate->rolePermissions);
            if(res != UA_STATUSCODE_GOOD)
                return res;
            *outIndex = (UA_PermissionIndex)i;
            return UA_STATUSCODE_GOOD;
        }
    }

    /* Bounds check */
    if(server->rolePermissionsSize >= UA_PERMISSION_INDEX_INVALID)
        return UA_STATUSCODE_BADOUTOFRANGE;

    /* Append a new entry */
    UA_RolePermissionEntry *newArray = (UA_RolePermissionEntry*)
        UA_realloc(server->rolePermissions,
                   (server->rolePermissionsSize + 1) * sizeof(UA_RolePermissionEntry));
    if(!newArray)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    server->rolePermissions = newArray;
    UA_PermissionIndex newIndex = (UA_PermissionIndex)server->rolePermissionsSize;

    UA_RolePermissionEntry *entry = &server->rolePermissions[newIndex];
    rolePermissionEntry_init(entry);

    UA_StatusCode res = copyRolePermissionArray(rpSize, rp,
                                                &entry->rolePermissionsSize,
                                                &entry->rolePermissions);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    server->rolePermissionsSize++;
    *outIndex = newIndex;
    return UA_STATUSCODE_GOOD;
}

/* Decrement refCount for a permission index. Must be called with lock held. */
static void
decrementRefCount(UA_Server *server, UA_PermissionIndex index) {
    if(index == UA_PERMISSION_INDEX_INVALID ||
       index >= server->rolePermissionsSize)
        return;
    UA_RolePermissionEntry *rp = &server->rolePermissions[index];
    if(rp->refCount != UA_ROLEPERMISSIONS_REFCOUNT_PROTECTED &&
       rp->refCount > 0)
        rp->refCount--;
}

/* Increment refCount for a permission index. Must be called with lock held. */
static void
incrementRefCount(UA_Server *server, UA_PermissionIndex index) {
    if(index == UA_PERMISSION_INDEX_INVALID ||
       index >= server->rolePermissionsSize)
        return;
    UA_RolePermissionEntry *rp = &server->rolePermissions[index];
    if(rp->refCount != UA_ROLEPERMISSIONS_REFCOUNT_PROTECTED)
        rp->refCount++;
}

/**********************/
/* RBAC Init/Cleanup  */
/**********************/

/* Initialize well-known roles per specification */
static UA_StatusCode
initializeStandardRoles(UA_Server *server) {
    struct {
        UA_UInt32 id;
        const char *name;
        UA_IdentityCriteriaType identities[2];
        size_t identitiesSize;
    } stdRoles[] = {
        {UA_NS0ID_WELLKNOWNROLE_ANONYMOUS, "Anonymous",
         {UA_IDENTITYCRITERIATYPE_ANONYMOUS,
          UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER}, 2},
        {UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER, "AuthenticatedUser",
         {UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER, 0}, 1},
        {UA_NS0ID_WELLKNOWNROLE_OBSERVER, "Observer", {0, 0}, 0},
        {UA_NS0ID_WELLKNOWNROLE_OPERATOR, "Operator", {0, 0}, 0},
        {UA_NS0ID_WELLKNOWNROLE_ENGINEER, "Engineer", {0, 0}, 0},
        {UA_NS0ID_WELLKNOWNROLE_SUPERVISOR, "Supervisor", {0, 0}, 0},
        {UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN, "ConfigureAdmin", {0, 0}, 0},
        {UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN, "SecurityAdmin", {0, 0}, 0}
    };
    size_t count = sizeof(stdRoles) / sizeof(stdRoles[0]);

    for(size_t idx = 0; idx < count; idx++) {
        UA_Role role;
        UA_Role_init(&role);
        role.roleId = UA_NODEID_NUMERIC(0, stdRoles[idx].id);
        role.roleName =
            UA_QUALIFIEDNAME(0, (char*)(uintptr_t)stdRoles[idx].name);

        if(stdRoles[idx].identitiesSize > 0) {
            role.identityMappingRules = (UA_IdentityMappingRuleType*)
                UA_calloc(stdRoles[idx].identitiesSize,
                           sizeof(UA_IdentityMappingRuleType));
            if(!role.identityMappingRules)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            role.identityMappingRulesSize = stdRoles[idx].identitiesSize;
            for(size_t j = 0; j < stdRoles[idx].identitiesSize; j++) {
                role.identityMappingRules[j].criteriaType =
                    stdRoles[idx].identities[j];
                role.identityMappingRules[j].criteria = UA_STRING_NULL;
            }
        }

        UA_NodeId outId;
        UA_StatusCode res = UA_Server_addRole(server, &role, &outId);
        /* Clean up allocated identity array since addRole copies */
        UA_free(role.identityMappingRules);
        if(res != UA_STATUSCODE_GOOD)
            return res;

        /* Mark the well-known role as protected (cannot be removed) */
        if(server->rolesSize > 0)
            server->rolesProtected[server->rolesSize - 1] = true;
    }

    return UA_STATUSCODE_GOOD;
}

/* Apply config roles into server's internal role registry via UA_Server_addRole.
 * Config roles are marked as protected (cannot be removed at runtime).
 * Duplicate names or NodeIds are skipped with a warning. */
static UA_StatusCode
initializeRolesFromConfig(UA_Server *server) {
    UA_ServerConfig *config = &server->config;
    if(config->rolesSize == 0 || !config->roles)
        return UA_STATUSCODE_GOOD;

    for(size_t i = 0; i < config->rolesSize; i++) {
        UA_NodeId outId;
        UA_StatusCode res = UA_Server_addRole(server, &config->roles[i], &outId);
        if(res == UA_STATUSCODE_BADALREADYEXISTS) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "RBAC: Config role '%.*s' (ns=%u) skipped - "
                           "a role with the same name or NodeId already exists",
                           (int)config->roles[i].roleName.name.length,
                           config->roles[i].roleName.name.data,
                           config->roles[i].roleName.namespaceIndex);
            continue;
        }
        if(res != UA_STATUSCODE_GOOD)
            return res;

        /* Mark as protected (cannot be removed at runtime) */
        if(server->rolesSize > 0)
            server->rolesProtected[server->rolesSize - 1] = true;
        UA_NodeId_clear(&outId);
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_initRBAC(UA_Server *server) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    server->rolePermissionsSize = 0;
    server->rolePermissions = NULL;

    /* Copy presets from config into the internal array.
     * Mark them with UA_ROLEPERMISSIONS_REFCOUNT_PROTECTED so they
     * can never be removed during server runtime. */
    UA_ServerConfig *config = &server->config;
    if(config->rolePermissionPresetsSize > 0 && config->rolePermissionPresets) {
        server->rolePermissions = (UA_RolePermissionEntry*)
            UA_calloc(config->rolePermissionPresetsSize, sizeof(UA_RolePermissionEntry));
        if(!server->rolePermissions)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        for(size_t i = 0; i < config->rolePermissionPresetsSize; i++) {
            UA_RolePermissionEntry *entry = &server->rolePermissions[i];
            rolePermissionEntry_init(entry);

            const UA_RolePermissionSet *preset = &config->rolePermissionPresets[i];
            UA_StatusCode res =
                copyRolePermissionArray(preset->rolePermissionsSize,
                                        preset->rolePermissions,
                                        &entry->rolePermissionsSize,
                                        &entry->rolePermissions);
            if(res != UA_STATUSCODE_GOOD) {
                /* Clean up already copied entries */
                for(size_t j = 0; j < i; j++)
                    rolePermissionEntry_clear(&server->rolePermissions[j]);
                UA_free(server->rolePermissions);
                server->rolePermissions = NULL;
                return res;
            }
            entry->refCount = UA_ROLEPERMISSIONS_REFCOUNT_PROTECTED;
        }
        server->rolePermissionsSize = config->rolePermissionPresetsSize;
    }

    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "RBAC support enabled (EXPERIMENTAL - not for production use). "
                "%zu preset(s) loaded.",
                server->rolePermissionsSize);

    /* Register the OPC UA well-known roles in the internal registry */
    UA_StatusCode stdRes = initializeStandardRoles(server);
    if(stdRes != UA_STATUSCODE_GOOD)
        return stdRes;

    /* Copy config roles into the internal role registry */
    UA_StatusCode initRes = initializeRolesFromConfig(server);
    if(initRes != UA_STATUSCODE_GOOD)
        return initRes;

    if(server->rolesSize > 0)
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "RBAC: %zu role(s) loaded from config (protected).",
                    server->rolesSize);

    return UA_STATUSCODE_GOOD;
}

void
UA_Server_cleanupRBAC(UA_Server *server) {
    if(!server)
        return;

    /* Clean up role-permission entries */
    for(size_t i = 0; i < server->rolePermissionsSize; i++)
        rolePermissionEntry_clear(&server->rolePermissions[i]);
    UA_free(server->rolePermissions);
    server->rolePermissions = NULL;
    server->rolePermissionsSize = 0;

    /* Clean up role registry */
    for(size_t i = 0; i < server->rolesSize; i++)
        UA_Role_clear(&server->roles[i]);
    UA_free(server->roles);
    server->roles = NULL;
    UA_free(server->rolesProtected);
    server->rolesProtected = NULL;
    server->rolesSize = 0;
}

/************************************/
/* Internal Helpers: Role Registry  */
/************************************/

static UA_Role *
findRoleByName(UA_Server *server, const UA_QualifiedName *roleName) {
    for(size_t i = 0; i < server->rolesSize; i++) {
        if(UA_QualifiedName_equal(&server->roles[i].roleName, roleName))
            return &server->roles[i];
    }
    return NULL;
}

static UA_Role*
findRoleById(UA_Server *server, const UA_NodeId *roleId) {
    for(size_t i = 0; i < server->rolesSize; i++) {
        if(UA_NodeId_equal(&server->roles[i].roleId, roleId))
            return &server->roles[i];
    }
    return NULL;
}

/************************************/
/* Public API: Role Management      */
/************************************/

UA_StatusCode
UA_Server_addRole(UA_Server *server, const UA_Role *role,
                  UA_NodeId *outRoleNodeId) {
    if(!server || !role)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    /* Check for duplicate roleName (BrowseName uniqueness per Part 18) */
    if(findRoleByName(server, &role->roleName)) {
        unlockServer(server);
        return UA_STATUSCODE_BADALREADYEXISTS;
    }

    /* Check for duplicate roleId (if the caller provided one) */
    if(!UA_NodeId_isNull(&role->roleId) &&
       findRoleById(server, &role->roleId)) {
        unlockServer(server);
        return UA_STATUSCODE_BADALREADYEXISTS;
    }

    /* Grow the arrays */
    UA_Role *newRoles = (UA_Role*)
        UA_realloc(server->roles,
                   (server->rolesSize + 1) * sizeof(UA_Role));
    if(!newRoles) {
        unlockServer(server);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    server->roles = newRoles;

    UA_Boolean *newProtected = (UA_Boolean*)
        UA_realloc(server->rolesProtected,
                   (server->rolesSize + 1) * sizeof(UA_Boolean));
    if(!newProtected) {
        unlockServer(server);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    server->rolesProtected = newProtected;

    /* Deep-copy the role */
    UA_Role *newRole = &server->roles[server->rolesSize];
    UA_StatusCode res = UA_Role_copy(role, newRole);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    /* Auto-assign a numeric roleId if the caller passed a null NodeId */
    if(UA_NodeId_isNull(&newRole->roleId))
        newRole->roleId = UA_NODEID_NUMERIC(0, UA_UInt32_random());

    server->rolesProtected[server->rolesSize] = false;
    server->rolesSize++;

    /* Return the assigned roleId */
    if(outRoleNodeId) {
        res = UA_NodeId_copy(&newRole->roleId, outRoleNodeId);
        if(res != UA_STATUSCODE_GOOD) {
            /* Rollback */
            server->rolesSize--;
            UA_Role_clear(newRole);
            unlockServer(server);
            return res;
        }
    }

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeRole(UA_Server *server,
                     const UA_QualifiedName roleName) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_Role *role = findRoleByName(server, &roleName);
    if(!role) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    size_t roleIndex = (size_t)(role - server->roles);

    /* Protected roles (from config) cannot be removed */
    if(server->rolesProtected[roleIndex]) {
        unlockServer(server);
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    }

    UA_Role_clear(&server->roles[roleIndex]);

    /* Shift remaining entries */
    if(roleIndex < server->rolesSize - 1) {
        memmove(&server->roles[roleIndex],
                &server->roles[roleIndex + 1],
                (server->rolesSize - roleIndex - 1) * sizeof(UA_Role));
        memmove(&server->rolesProtected[roleIndex],
                &server->rolesProtected[roleIndex + 1],
                (server->rolesSize - roleIndex - 1) * sizeof(UA_Boolean));
    }

    server->rolesSize--;

    if(server->rolesSize > 0) {
        UA_Role *r = (UA_Role*)
            UA_realloc(server->roles,
                       server->rolesSize * sizeof(UA_Role));
        if(r)
            server->roles = r;
        UA_Boolean *p = (UA_Boolean*)
            UA_realloc(server->rolesProtected,
                       server->rolesSize * sizeof(UA_Boolean));
        if(p)
            server->rolesProtected = p;
    } else {
        UA_free(server->roles);
        server->roles = NULL;
        UA_free(server->rolesProtected);
        server->rolesProtected = NULL;
    }

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getRole(UA_Server *server,
                  const UA_QualifiedName roleName,
                  UA_Role *outRole) {
    if(!server || !outRole)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_Role *role = findRoleByName(server, &roleName);
    if(!role) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_StatusCode res = UA_Role_copy(role, outRole);

    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getRoles(UA_Server *server, size_t *rolesSize,
                   UA_QualifiedName **roleNames) {
    if(!server || !rolesSize || !roleNames)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    *rolesSize = 0;
    *roleNames = NULL;

    if(server->rolesSize == 0) {
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }

    *roleNames = (UA_QualifiedName*)
        UA_malloc(server->rolesSize * sizeof(UA_QualifiedName));
    if(!*roleNames) {
        unlockServer(server);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(size_t i = 0; i < server->rolesSize; i++) {
        UA_StatusCode res = UA_QualifiedName_copy(&server->roles[i].roleName,
                                                   &(*roleNames)[i]);
        if(res != UA_STATUSCODE_GOOD) {
            for(size_t j = 0; j < i; j++)
                UA_QualifiedName_clear(&(*roleNames)[j]);
            UA_free(*roleNames);
            *roleNames = NULL;
            unlockServer(server);
            return res;
        }
    }
    *rolesSize = server->rolesSize;

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

/*****************************************/
/* Public API: Node Role Permissions     */
/*****************************************/

/* Internal helper: set permissionIndex on a single node.
 * Must be called with the server lock held. */
static UA_StatusCode
setNodePermissionIndexLocked(UA_Server *server, const UA_NodeId *nodeId,
                             UA_PermissionIndex newIndex) {
    /* Get the editable node */
    UA_Node *editNode = UA_NODESTORE_GET_EDIT(server, nodeId);
    if(!editNode)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    UA_PermissionIndex currentIndex = editNode->head.permissionIndex;

    /* No change needed */
    if(currentIndex == newIndex) {
        UA_NODESTORE_RELEASE(server, (const UA_Node*)editNode);
        return UA_STATUSCODE_GOOD;
    }

    /* Update refCounts */
    decrementRefCount(server, currentIndex);
    incrementRefCount(server, newIndex);

    /* Set the new index */
    editNode->head.permissionIndex = newIndex;
    UA_NODESTORE_RELEASE(server, (const UA_Node*)editNode);

    return UA_STATUSCODE_GOOD;
}

/* Internal helper: recursively set permissionIndex on a node and its children.
 * The server lock is released for the browse call and re-acquired for
 * each node edit. */
static UA_StatusCode
setPermissionIndexRecursive(UA_Server *server, const UA_NodeId *nodeId,
                            UA_PermissionIndex index) {
    /* Set on this node (already locked by caller) */
    UA_StatusCode res = setNodePermissionIndexLocked(server, nodeId, index);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Browse for hierarchical children - need to release lock since
     * UA_Server_browse does its own locking */
    unlockServer(server);

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *nodeId;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NS0ID(HIERARCHICALREFERENCES);
    bd.includeSubtypes = true;
    bd.resultMask = UA_BROWSERESULTMASK_TARGETINFO;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);

    lockServer(server);

    if(br.statusCode != UA_STATUSCODE_GOOD) {
        res = br.statusCode;
        UA_BrowseResult_clear(&br);
        return res;
    }

    for(size_t i = 0; i < br.referencesSize; i++) {
        UA_ReferenceDescription *ref = &br.references[i];
        if(!UA_ExpandedNodeId_isLocal(&ref->nodeId))
            continue;
        /* Recurse - ignore errors on individual children */
        setPermissionIndexRecursive(server, &ref->nodeId.nodeId, index);
    }

    UA_BrowseResult_clear(&br);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setNodeRolePermissions(UA_Server *server,
                                 const UA_NodeId nodeId,
                                 size_t rolePermissionsSize,
                                 const UA_RolePermission *rolePermissions,
                                 UA_Boolean recursive) {
    if(!server || (rolePermissionsSize > 0 && !rolePermissions))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    /* Find or create the internal entry (deduplication) */
    UA_PermissionIndex index;
    UA_StatusCode res =
        findOrCreateRolePermissions(server, rolePermissionsSize,
                                    rolePermissions, &index);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    /* Set the index on the node (and optionally its children) */
    if(recursive)
        res = setPermissionIndexRecursive(server, &nodeId, index);
    else
        res = setNodePermissionIndexLocked(server, &nodeId, index);

    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getNodeRolePermissions(UA_Server *server,
                                 const UA_NodeId nodeId,
                                 size_t *rolePermissionsSize,
                                 UA_RolePermission **rolePermissions) {
    if(!server || !rolePermissionsSize || !rolePermissions)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    *rolePermissionsSize = 0;
    *rolePermissions = NULL;

    lockServer(server);

    const UA_Node *node = UA_NODESTORE_GET(server, &nodeId);
    if(!node) {
        unlockServer(server);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    UA_PermissionIndex index = node->head.permissionIndex;
    UA_NODESTORE_RELEASE(server, node);

    if(index == UA_PERMISSION_INDEX_INVALID ||
       index >= server->rolePermissionsSize) {
        unlockServer(server);
        return UA_STATUSCODE_GOOD; /* No role permissions set */
    }

    const UA_RolePermissionEntry *rp = &server->rolePermissions[index];
    UA_StatusCode res = copyRolePermissionArray(rp->rolePermissionsSize,
                                                rp->rolePermissions,
                                                rolePermissionsSize,
                                                rolePermissions);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_removeNodeRolePermissions(UA_Server *server,
                                    const UA_NodeId nodeId,
                                    UA_Boolean recursive) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_StatusCode res;
    if(recursive)
        res = setPermissionIndexRecursive(server, &nodeId,
                                          UA_PERMISSION_INDEX_INVALID);
    else
        res = setNodePermissionIndexLocked(server, &nodeId,
                                           UA_PERMISSION_INDEX_INVALID);

    unlockServer(server);
    return res;
}

/*****************************************/
/* Internal Helpers: Role Lookups        */
/*****************************************/

/* Mandatory well-known roles cannot be modified */
static UA_Boolean
isMandatoryWellKnownRole(const UA_NodeId *roleId) {
    if(roleId->namespaceIndex != 0 ||
       roleId->identifierType != UA_NODEIDTYPE_NUMERIC)
        return false;
    return (roleId->identifier.numeric == UA_NS0ID_WELLKNOWNROLE_ANONYMOUS ||
            roleId->identifier.numeric == UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
}

/************************************/
/* Public API: Role Queries         */
/************************************/

UA_StatusCode
UA_Server_getRoleById(UA_Server *server, UA_NodeId roleId,
                      UA_Role *outRole) {
    if(!server || !outRole)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_Role_copy(role, outRole);
    unlockServer(server);
    return res;
}

/************************************/
/* Public API: Role Update          */
/************************************/

UA_StatusCode UA_EXPORT
UA_Server_updateRole(UA_Server *server, const UA_Role *role) {
    if(!server || !role)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_Boolean hasId = !UA_NodeId_isNull(&role->roleId);
    UA_Boolean hasName = (role->roleName.name.length > 0);
    if(!hasId && !hasName)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_Role *existing = NULL;
    if(hasId && hasName) {
        existing = findRoleById(server, &role->roleId);
        if(!existing ||
           !UA_QualifiedName_equal(&existing->roleName, &role->roleName)) {
            unlockServer(server);
            return UA_STATUSCODE_BADNOTFOUND;
        }
    } else if(hasId) {
        existing = findRoleById(server, &role->roleId);
    } else {
        existing = findRoleByName(server, &role->roleName);
    }
    if(!existing) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    if(isMandatoryWellKnownRole(&existing->roleId)) {
        unlockServer(server);
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    }

    /* Deep copy the incoming role */
    UA_Role copy;
    UA_StatusCode res = UA_Role_copy(role, &copy);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    /* Clear old mutable fields */
    for(size_t i = 0; i < existing->identityMappingRulesSize; i++)
        UA_IdentityMappingRuleType_clear(&existing->identityMappingRules[i]);
    UA_free(existing->identityMappingRules);

    for(size_t i = 0; i < existing->applicationsSize; i++)
        UA_String_clear(&existing->applications[i]);
    UA_free(existing->applications);

    for(size_t i = 0; i < existing->endpointsSize; i++)
        UA_EndpointType_clear(&existing->endpoints[i]);
    UA_free(existing->endpoints);

    /* Move mutable fields from copy to existing (ownership transfer) */
    existing->identityMappingRulesSize = copy.identityMappingRulesSize;
    existing->identityMappingRules = copy.identityMappingRules;
    existing->applicationsExclude = copy.applicationsExclude;
    existing->applicationsSize = copy.applicationsSize;
    existing->applications = copy.applications;
    existing->endpointsExclude = copy.endpointsExclude;
    existing->endpointsSize = copy.endpointsSize;
    existing->endpoints = copy.endpoints;

    /* Null out moved fields before clearing the rest */
    copy.identityMappingRulesSize = 0;
    copy.identityMappingRules = NULL;
    copy.applicationsSize = 0;
    copy.applications = NULL;
    copy.endpointsSize = 0;
    copy.endpoints = NULL;
    UA_Role_clear(&copy);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

/************************************/
/* Internal Session Role Helper     */
/************************************/

/* Set roles on a session. Validates all role IDs against the server registry.
 * Must be called with the server lock held. */
UA_StatusCode
UA_Session_setRoles(UA_Server *server, UA_Session *session,
                    const UA_NodeId *roleIds, size_t rolesSize) {
    for(size_t i = 0; i < rolesSize; i++) {
        if(!findRoleById(server, &roleIds[i]))
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    UA_Array_delete(session->roles, session->rolesSize, &UA_TYPES[UA_TYPES_NODEID]);
    session->roles = NULL;
    session->rolesSize = 0;

    if(rolesSize > 0) {
        UA_StatusCode res = UA_Array_copy(roleIds, rolesSize,
                                          (void**)&session->roles,
                                          &UA_TYPES[UA_TYPES_NODEID]);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        session->rolesSize = rolesSize;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getSessionRoleNames(UA_Server *server, const UA_NodeId *sessionId,
                              size_t *outSize, UA_QualifiedName **outRoleNames) {
    if(!server || !sessionId || !outSize || !outRoleNames)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    *outSize = 0;
    *outRoleNames = NULL;

    lockServer(server);

    UA_Session *session = getSessionById(server, sessionId);
    if(!session) {
        unlockServer(server);
        return UA_STATUSCODE_BADSESSIONIDINVALID;
    }

    if(session->rolesSize == 0) {
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }

    UA_QualifiedName *names = (UA_QualifiedName*)
        UA_calloc(session->rolesSize, sizeof(UA_QualifiedName));
    if(!names) {
        unlockServer(server);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    size_t count = 0;
    for(size_t i = 0; i < session->rolesSize; i++) {
        const UA_Role *role = findRoleById(server, &session->roles[i]);
        if(!role)
            continue;
        UA_StatusCode res = UA_QualifiedName_copy(&role->roleName, &names[count]);
        if(res != UA_STATUSCODE_GOOD) {
            for(size_t j = 0; j < count; j++)
                UA_QualifiedName_clear(&names[j]);
            UA_free(names);
            unlockServer(server);
            return res;
        }
        count++;
    }

    *outRoleNames = names;
    *outSize = count;
    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

/*****************************************/
/* Internal Helpers: Hierarchy Traversal */
/*****************************************/

/* Context for recursive hierarchy traversal using internal reference iterators */
struct ApplyToHierarchicalChildrenContext {
    UA_Server *server;
    const UA_ReferenceTypeSet *hierarchRefsSet;
    void *callbackContext;
    UA_StatusCode (*applyCallback)(UA_Server *server, const UA_NodeId *nodeId,
                                   void *context);
    UA_StatusCode status;
};

static void *
applyToHierarchicalChildrenIterator(void *context, UA_ReferenceTarget *t) {
    struct ApplyToHierarchicalChildrenContext *ctx =
        (struct ApplyToHierarchicalChildrenContext*)context;

    if(!UA_NodePointer_isLocal(t->targetId))
        return NULL;

    UA_NodeId childId = UA_NodePointer_toNodeId(t->targetId);

    ctx->status = ctx->applyCallback(ctx->server, &childId, ctx->callbackContext);
    if(ctx->status != UA_STATUSCODE_GOOD)
        return NULL;

    const UA_Node *childNode = UA_NODESTORE_GET(ctx->server, &childId);
    if(!childNode)
        return NULL;

    for(size_t i = 0; i < childNode->head.referencesSize; i++) {
        UA_NodeReferenceKind *rk = &childNode->head.references[i];

        if(rk->isInverse)
            continue;
        if(!UA_ReferenceTypeSet_contains(ctx->hierarchRefsSet, rk->referenceTypeIndex))
            continue;

        void *res = UA_NodeReferenceKind_iterate(rk, applyToHierarchicalChildrenIterator, ctx);
        if(res != NULL) {
            UA_NODESTORE_RELEASE(ctx->server, childNode);
            return res;
        }

        if(ctx->status != UA_STATUSCODE_GOOD) {
            UA_NODESTORE_RELEASE(ctx->server, childNode);
            return NULL;
        }
    }

    UA_NODESTORE_RELEASE(ctx->server, childNode);
    return NULL;
}

/* Apply a callback to all hierarchical children of a node.
 * Must be called with the server lock held. */
static UA_StatusCode
applyToHierarchicalChildren(UA_Server *server, const UA_NodeId *nodeId,
                            UA_StatusCode (*callback)(UA_Server *server,
                                                      const UA_NodeId *nodeId,
                                                      void *context),
                            void *callbackContext) {
    UA_ReferenceTypeSet hierarchRefsSet;
    UA_ReferenceTypeSet_init(&hierarchRefsSet);
    UA_NodeId hierarchRefTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    UA_StatusCode res = referenceTypeIndices(server, &hierarchRefTypeId,
                                             &hierarchRefsSet, true);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    struct ApplyToHierarchicalChildrenContext ctx;
    ctx.server = server;
    ctx.hierarchRefsSet = &hierarchRefsSet;
    ctx.callbackContext = callbackContext;
    ctx.applyCallback = callback;
    ctx.status = UA_STATUSCODE_GOOD;

    for(size_t i = 0; i < node->head.referencesSize; i++) {
        UA_NodeReferenceKind *rk = &node->head.references[i];

        if(rk->isInverse)
            continue;
        if(!UA_ReferenceTypeSet_contains(&hierarchRefsSet, rk->referenceTypeIndex))
            continue;

        UA_NodeReferenceKind_iterate(rk, applyToHierarchicalChildrenIterator, &ctx);
        if(ctx.status != UA_STATUSCODE_GOOD)
            break;
    }

    UA_NODESTORE_RELEASE(server, node);
    return ctx.status;
}

/*******************************/
/* Per-Role Node Permissions   */
/*******************************/

/* Internal helper for adding role permissions to a single node.
 * Must be called with the server lock held.
 *
 * IMPORTANT: RolePermission entries are IMMUTABLE once created with refCount > 0.
 * When modifying permissions for a node, we:
 * 1. Get the node's current permissionIndex
 * 2. Build the new desired permission set
 * 3. Find or create a matching entry
 * 4. Update refcounts and the node's permissionIndex */
static UA_StatusCode
addRolePermissionsInternal(UA_Server *server, const UA_NodeId *nodeId,
                           const UA_NodeId *roleId, UA_PermissionType permissionType,
                           UA_Boolean overwriteExisting) {
    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    UA_PermissionIndex currentIndex = node->head.permissionIndex;
    UA_NODESTORE_RELEASE(server, node);

    /* Build the new desired permission entries array */
    size_t newEntriesSize = 0;
    UA_RolePermission *newEntries = NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    if(currentIndex == UA_PERMISSION_INDEX_INVALID) {
        /* No existing permissions - create new with just this role */
        newEntriesSize = 1;
        newEntries = (UA_RolePermission*)UA_malloc(sizeof(UA_RolePermission));
        if(!newEntries)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        res = UA_NodeId_copy(roleId, &newEntries[0].roleId);
        if(res != UA_STATUSCODE_GOOD) {
            UA_free(newEntries);
            return res;
        }
        newEntries[0].permissions = permissionType;
    } else {
        /* Copy existing entries and modify/add the role entry */
        UA_RolePermissionEntry *oldRp = &server->rolePermissions[currentIndex];

        /* Find if this role already exists in current permissions */
        size_t existingRoleIdx = SIZE_MAX;
        for(size_t i = 0; i < oldRp->rolePermissionsSize; i++) {
            if(UA_NodeId_equal(&oldRp->rolePermissions[i].roleId, roleId)) {
                existingRoleIdx = i;
                break;
            }
        }

        if(existingRoleIdx != SIZE_MAX) {
            /* Role exists - copy all and modify the role's permissions */
            newEntriesSize = oldRp->rolePermissionsSize;
            newEntries = (UA_RolePermission*)
                UA_malloc(newEntriesSize * sizeof(UA_RolePermission));
            if(!newEntries)
                return UA_STATUSCODE_BADOUTOFMEMORY;

            for(size_t i = 0; i < oldRp->rolePermissionsSize; i++) {
                res = UA_NodeId_copy(&oldRp->rolePermissions[i].roleId, &newEntries[i].roleId);
                if(res != UA_STATUSCODE_GOOD) {
                    for(size_t j = 0; j < i; j++)
                        UA_NodeId_clear(&newEntries[j].roleId);
                    UA_free(newEntries);
                    return res;
                }
                if(i == existingRoleIdx) {
                    if(overwriteExisting)
                        newEntries[i].permissions = permissionType;
                    else
                        newEntries[i].permissions = oldRp->rolePermissions[i].permissions | permissionType;
                } else {
                    newEntries[i].permissions = oldRp->rolePermissions[i].permissions;
                }
            }
        } else {
            /* Role doesn't exist - copy all and add new entry */
            newEntriesSize = oldRp->rolePermissionsSize + 1;
            newEntries = (UA_RolePermission*)
                UA_malloc(newEntriesSize * sizeof(UA_RolePermission));
            if(!newEntries)
                return UA_STATUSCODE_BADOUTOFMEMORY;

            for(size_t i = 0; i < oldRp->rolePermissionsSize; i++) {
                res = UA_NodeId_copy(&oldRp->rolePermissions[i].roleId, &newEntries[i].roleId);
                if(res != UA_STATUSCODE_GOOD) {
                    for(size_t j = 0; j < i; j++)
                        UA_NodeId_clear(&newEntries[j].roleId);
                    UA_free(newEntries);
                    return res;
                }
                newEntries[i].permissions = oldRp->rolePermissions[i].permissions;
            }
            res = UA_NodeId_copy(roleId, &newEntries[oldRp->rolePermissionsSize].roleId);
            if(res != UA_STATUSCODE_GOOD) {
                for(size_t j = 0; j < oldRp->rolePermissionsSize; j++)
                    UA_NodeId_clear(&newEntries[j].roleId);
                UA_free(newEntries);
                return res;
            }
            newEntries[oldRp->rolePermissionsSize].permissions = permissionType;
        }
    }

    /* Find or create a slot for the new permissions */
    UA_PermissionIndex targetIndex;
    res = findOrCreateRolePermissions(server, newEntriesSize, newEntries, &targetIndex);

    /* Clean up temporary entries */
    for(size_t i = 0; i < newEntriesSize; i++)
        UA_NodeId_clear(&newEntries[i].roleId);
    UA_free(newEntries);

    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Update refcounts and node's permissionIndex if changed */
    if(targetIndex != currentIndex) {
        decrementRefCount(server, currentIndex);
        incrementRefCount(server, targetIndex);

        /* Update node's permission index */
        UA_Node *editNode = UA_NODESTORE_GET_EDIT(server, nodeId);
        if(!editNode) {
            /* Rollback refcount changes */
            decrementRefCount(server, targetIndex);
            incrementRefCount(server, currentIndex);
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
        }
        editNode->head.permissionIndex = targetIndex;
        UA_NODESTORE_RELEASE(server, (const UA_Node*)editNode);
    }

    return UA_STATUSCODE_GOOD;
}

/* Callback context for recursive addRolePermissions */
struct AddRolePermissionsContext {
    const UA_NodeId *roleId;
    UA_PermissionType permissionType;
    UA_Boolean overwriteExisting;
};

static UA_StatusCode
addRolePermissionsCallback(UA_Server *server, const UA_NodeId *nodeId, void *context) {
    struct AddRolePermissionsContext *ctx = (struct AddRolePermissionsContext*)context;
    return addRolePermissionsInternal(server, nodeId, ctx->roleId,
                                     ctx->permissionType, ctx->overwriteExisting);
}

UA_StatusCode
UA_Server_addRolePermissions(UA_Server *server, const UA_NodeId nodeId,
                             const UA_NodeId roleId, UA_PermissionType permissionType,
                             UA_Boolean overwriteExisting, UA_Boolean recursive) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    const UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        unlockServer(server);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    UA_StatusCode res = addRolePermissionsInternal(server, &nodeId, &roleId,
                                                   permissionType, overwriteExisting);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    if(recursive) {
        struct AddRolePermissionsContext ctx;
        ctx.roleId = &roleId;
        ctx.permissionType = permissionType;
        ctx.overwriteExisting = overwriteExisting;
        res = applyToHierarchicalChildren(server, &nodeId, addRolePermissionsCallback, &ctx);
    }

    unlockServer(server);
    return res;
}

/* Internal helper for removing role permissions from a single node.
 * Must be called with the server lock held. */
static UA_StatusCode
removeRolePermissionsInternal(UA_Server *server, const UA_NodeId *nodeId,
                              const UA_NodeId *roleId, UA_PermissionType permissionType) {
    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    UA_PermissionIndex currentIndex = node->head.permissionIndex;
    UA_NODESTORE_RELEASE(server, node);

    if(currentIndex == UA_PERMISSION_INDEX_INVALID)
        return UA_STATUSCODE_GOOD;

    UA_RolePermissionEntry *oldRp = &server->rolePermissions[currentIndex];

    /* Find the role in current permissions */
    size_t roleEntryIdx = SIZE_MAX;
    for(size_t i = 0; i < oldRp->rolePermissionsSize; i++) {
        if(UA_NodeId_equal(&oldRp->rolePermissions[i].roleId, roleId)) {
            roleEntryIdx = i;
            break;
        }
    }

    if(roleEntryIdx == SIZE_MAX)
        return UA_STATUSCODE_GOOD; /* Role not found, nothing to remove */

    /* Calculate new permissions for this role */
    UA_PermissionType newPerms = oldRp->rolePermissions[roleEntryIdx].permissions & ~permissionType;

    /* Build new entries array */
    size_t newEntriesSize = (newPerms == 0) ?
        oldRp->rolePermissionsSize - 1 : oldRp->rolePermissionsSize;

    UA_PermissionIndex targetIndex = UA_PERMISSION_INDEX_INVALID;
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    if(newEntriesSize > 0) {
        UA_RolePermission *newEntries = (UA_RolePermission*)
            UA_malloc(newEntriesSize * sizeof(UA_RolePermission));
        if(!newEntries)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        size_t j = 0;
        for(size_t i = 0; i < oldRp->rolePermissionsSize; i++) {
            if(i == roleEntryIdx) {
                if(newPerms != 0) {
                    res = UA_NodeId_copy(&oldRp->rolePermissions[i].roleId, &newEntries[j].roleId);
                    if(res != UA_STATUSCODE_GOOD) {
                        for(size_t k = 0; k < j; k++)
                            UA_NodeId_clear(&newEntries[k].roleId);
                        UA_free(newEntries);
                        return res;
                    }
                    newEntries[j].permissions = newPerms;
                    j++;
                }
            } else {
                res = UA_NodeId_copy(&oldRp->rolePermissions[i].roleId, &newEntries[j].roleId);
                if(res != UA_STATUSCODE_GOOD) {
                    for(size_t k = 0; k < j; k++)
                        UA_NodeId_clear(&newEntries[k].roleId);
                    UA_free(newEntries);
                    return res;
                }
                newEntries[j].permissions = oldRp->rolePermissions[i].permissions;
                j++;
            }
        }

        res = findOrCreateRolePermissions(server, newEntriesSize, newEntries, &targetIndex);

        for(size_t i = 0; i < newEntriesSize; i++)
            UA_NodeId_clear(&newEntries[i].roleId);
        UA_free(newEntries);

        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    /* Update refcounts and node's permissionIndex if changed */
    if(targetIndex != currentIndex) {
        decrementRefCount(server, currentIndex);
        if(targetIndex != UA_PERMISSION_INDEX_INVALID)
            incrementRefCount(server, targetIndex);

        UA_Node *editNode = UA_NODESTORE_GET_EDIT(server, nodeId);
        if(!editNode) {
            if(targetIndex != UA_PERMISSION_INDEX_INVALID)
                decrementRefCount(server, targetIndex);
            incrementRefCount(server, currentIndex);
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
        }
        editNode->head.permissionIndex = targetIndex;
        UA_NODESTORE_RELEASE(server, (const UA_Node*)editNode);
    }

    return UA_STATUSCODE_GOOD;
}

struct RemoveRolePermissionsContext {
    const UA_NodeId *roleId;
    UA_PermissionType permissionType;
};

static UA_StatusCode
removeRolePermissionsCallback(UA_Server *server, const UA_NodeId *nodeId, void *context) {
    struct RemoveRolePermissionsContext *ctx = (struct RemoveRolePermissionsContext*)context;
    return removeRolePermissionsInternal(server, nodeId, ctx->roleId, ctx->permissionType);
}

UA_StatusCode
UA_Server_removeRolePermissions(UA_Server *server, const UA_NodeId nodeId,
                                const UA_NodeId roleId, UA_PermissionType permissionType,
                                UA_Boolean recursive) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    const UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        unlockServer(server);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    UA_StatusCode res = removeRolePermissionsInternal(server, &nodeId, &roleId, permissionType);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    if(recursive) {
        struct RemoveRolePermissionsContext ctx;
        ctx.roleId = &roleId;
        ctx.permissionType = permissionType;
        res = applyToHierarchicalChildren(server, &nodeId, removeRolePermissionsCallback, &ctx);
    }

    unlockServer(server);
    return res;
}

/************************************/
/* Permission Index Management      */
/************************************/

/* Internal helper for setting a node's permission index directly.
 * Must be called with the server lock held. */
static UA_StatusCode
setNodePermissionIndexDirect(UA_Server *server, const UA_NodeId *nodeId,
                             UA_PermissionIndex permissionIndex) {
    if(permissionIndex != UA_PERMISSION_INDEX_INVALID &&
       permissionIndex >= server->rolePermissionsSize)
        return UA_STATUSCODE_BADOUTOFRANGE;

    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    UA_PermissionIndex currentIndex = node->head.permissionIndex;
    UA_NODESTORE_RELEASE(server, node);

    if(currentIndex == permissionIndex)
        return UA_STATUSCODE_GOOD;

    decrementRefCount(server, currentIndex);
    if(permissionIndex != UA_PERMISSION_INDEX_INVALID)
        incrementRefCount(server, permissionIndex);

    UA_Node *editNode = UA_NODESTORE_GET_EDIT(server, nodeId);
    if(!editNode) {
        if(permissionIndex != UA_PERMISSION_INDEX_INVALID)
            decrementRefCount(server, permissionIndex);
        incrementRefCount(server, currentIndex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    editNode->head.permissionIndex = permissionIndex;
    UA_NODESTORE_RELEASE(server, (const UA_Node*)editNode);

    return UA_STATUSCODE_GOOD;
}

struct SetNodePermissionIndexContext {
    UA_PermissionIndex permissionIndex;
};

static UA_StatusCode
setNodePermissionIndexCallback(UA_Server *server, const UA_NodeId *nodeId, void *context) {
    struct SetNodePermissionIndexContext *ctx = (struct SetNodePermissionIndexContext*)context;
    return setNodePermissionIndexDirect(server, nodeId, ctx->permissionIndex);
}

UA_StatusCode
UA_Server_setNodePermissionIndex(UA_Server *server, const UA_NodeId nodeId,
                                 UA_PermissionIndex permissionIndex,
                                 UA_Boolean recursive) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_StatusCode res = setNodePermissionIndexDirect(server, &nodeId, permissionIndex);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    if(recursive) {
        struct SetNodePermissionIndexContext ctx;
        ctx.permissionIndex = permissionIndex;
        res = applyToHierarchicalChildren(server, &nodeId, setNodePermissionIndexCallback, &ctx);
    }

    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getNodePermissionIndex(UA_Server *server, const UA_NodeId nodeId,
                                 UA_PermissionIndex *permissionIndex) {
    if(!server || !permissionIndex)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    const UA_Node *node = UA_NODESTORE_GET(server, &nodeId);
    if(!node) {
        unlockServer(server);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    *permissionIndex = node->head.permissionIndex;

    UA_NODESTORE_RELEASE(server, node);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

/************************************/
/* Role Permission Config Mgmt     */
/************************************/

UA_StatusCode
UA_Server_addRolePermissionConfig(UA_Server *server,
                                  size_t entriesSize,
                                  const UA_RolePermission *entries,
                                  UA_PermissionIndex *outIndex) {
    if(!server || (entriesSize > 0 && !entries))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    if(server->rolePermissionsSize >= UA_PERMISSION_INDEX_INVALID) {
        unlockServer(server);
        return UA_STATUSCODE_BADOUTOFRANGE;
    }

    /* Validate that all role IDs exist */
    for(size_t i = 0; i < entriesSize; i++) {
        const UA_Role *role = findRoleById(server, &entries[i].roleId);
        if(!role) {
            unlockServer(server);
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
        }
    }

    UA_RolePermissionEntry *newArray = (UA_RolePermissionEntry*)
        UA_realloc(server->rolePermissions,
                   (server->rolePermissionsSize + 1) * sizeof(UA_RolePermissionEntry));
    if(!newArray) {
        unlockServer(server);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    server->rolePermissions = newArray;
    UA_PermissionIndex newIndex = (UA_PermissionIndex)server->rolePermissionsSize;

    UA_RolePermissionEntry *entry = &server->rolePermissions[newIndex];
    rolePermissionEntry_init(entry);

    if(entriesSize > 0) {
        UA_StatusCode res = copyRolePermissionArray(entriesSize, entries,
                                                    &entry->rolePermissionsSize,
                                                    &entry->rolePermissions);
        if(res != UA_STATUSCODE_GOOD) {
            unlockServer(server);
            return res;
        }
    }

    server->rolePermissionsSize++;

    if(outIndex)
        *outIndex = newIndex;

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

const UA_RolePermissionSet *
UA_Server_getRolePermissionConfig(UA_Server *server, UA_PermissionIndex index) {
    if(!server || index >= server->rolePermissionsSize)
        return NULL;

    /* Cast UA_RolePermissionEntry to UA_RolePermissionSet — they have the same
     * layout for the first two fields (rolePermissionsSize, rolePermissions) */
    return (const UA_RolePermissionSet*)&server->rolePermissions[index];
}

UA_StatusCode
UA_Server_updateRolePermissionConfig(UA_Server *server, UA_PermissionIndex index,
                                     size_t entriesSize,
                                     const UA_RolePermission *entries) {
    if(!server || (entriesSize > 0 && !entries))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(index >= server->rolePermissionsSize)
        return UA_STATUSCODE_BADOUTOFRANGE;

    lockServer(server);

    UA_RolePermissionEntry *config = &server->rolePermissions[index];

    /* Cannot modify entries that are still referenced by nodes */
    if(config->refCount > 0 &&
       config->refCount != UA_ROLEPERMISSIONS_REFCOUNT_PROTECTED) {
        unlockServer(server);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    /* Validate that all role IDs exist */
    for(size_t i = 0; i < entriesSize; i++) {
        const UA_Role *role = findRoleById(server, &entries[i].roleId);
        if(!role) {
            unlockServer(server);
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
        }
    }

    /* Clear old entries */
    size_t savedRefCount = config->refCount;
    rolePermissionEntry_clear(config);

    /* Copy new entries */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(entriesSize > 0) {
        res = copyRolePermissionArray(entriesSize, entries,
                                      &config->rolePermissionsSize,
                                      &config->rolePermissions);
    }
    config->refCount = savedRefCount;

    unlockServer(server);
    return res;
}

/************************************/
/* Effective Permission Queries     */
/************************************/

/* Compute effective permissions for a set of roles on a node.
 * Falls back to namespace DefaultRolePermissions if none are set. */
static UA_PermissionType
computeEffectivePermissions(UA_Server *server, const UA_Node *node,
                            size_t rolesSize, const UA_NodeId *roles) {
    UA_PermissionIndex permIdx = node->head.permissionIndex;
    const UA_RolePermission *entries = NULL;
    size_t entriesSize = 0;

    /* If node has explicit permission configuration, use it */
    if(permIdx != UA_PERMISSION_INDEX_INVALID) {
        if(permIdx >= server->rolePermissionsSize)
            return 0;
        const UA_RolePermissionEntry *rp = &server->rolePermissions[permIdx];
        entries = rp->rolePermissions;
        entriesSize = rp->rolePermissionsSize;
    }

    /* If no permissions configured, check allPermissionsForAnonymousRole.
     * When true (the default), un-configured nodes are fully permissive. */
    if(!entries || entriesSize == 0) {
        if(server->config.allPermissionsForAnonymousRole)
            return 0xFFFFFFFF; /* All permissions granted */
        return 0; /* Strict: deny unless explicitly configured */
    }

    /* Compute logical OR of permissions for all session roles */
    UA_PermissionType effectivePerms = 0;

    for(size_t i = 0; i < rolesSize; i++) {
        for(size_t j = 0; j < entriesSize; j++) {
            if(UA_NodeId_equal(&roles[i], &entries[j].roleId)) {
                effectivePerms |= entries[j].permissions;
                break;
            }
        }
    }

    return effectivePerms;
}

UA_StatusCode
UA_Server_getEffectivePermissions(UA_Server *server, const UA_NodeId *sessionId,
                                  const UA_NodeId *nodeId,
                                  UA_PermissionType *effectivePermissions) {
    if(!server || !nodeId || !effectivePermissions)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node) {
        unlockServer(server);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Get session roles */
    size_t rolesSize = 0;
    UA_NodeId *roles = NULL;

    if(sessionId) {
        UA_Session *session = getSessionById(server, sessionId);
        if(session && session->rolesSize > 0) {
            rolesSize = session->rolesSize;
            roles = session->roles;
        }
    }

    *effectivePermissions = computeEffectivePermissions(server, node, rolesSize, roles);

    UA_NODESTORE_RELEASE(server, node);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getUserRolePermissions(UA_Server *server, const UA_NodeId *sessionId,
                                 const UA_NodeId *nodeId,
                                 size_t *entriesSize,
                                 UA_RolePermissionType **entries) {
    if(!server || !nodeId || !entriesSize || !entries)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    *entriesSize = 0;
    *entries = NULL;

    lockServer(server);

    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node) {
        unlockServer(server);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* If node has no permission configuration, return empty array */
    if(node->head.permissionIndex == UA_PERMISSION_INDEX_INVALID ||
       node->head.permissionIndex >= server->rolePermissionsSize) {
        UA_NODESTORE_RELEASE(server, node);
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }

    const UA_RolePermissionEntry *rp = &server->rolePermissions[node->head.permissionIndex];
    if(!rp->rolePermissions || rp->rolePermissionsSize == 0) {
        UA_NODESTORE_RELEASE(server, node);
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }

    /* Get session roles */
    size_t rolesSize = 0;
    UA_NodeId *roles = NULL;

    if(sessionId) {
        UA_Session *session = getSessionById(server, sessionId);
        if(session && session->rolesSize > 0) {
            rolesSize = session->rolesSize;
            roles = session->roles;
        }
    }

    /* Count how many roles the session has that also have permissions on this node */
    size_t matchCount = 0;
    for(size_t i = 0; i < rolesSize; i++) {
        for(size_t j = 0; j < rp->rolePermissionsSize; j++) {
            if(UA_NodeId_equal(&roles[i], &rp->rolePermissions[j].roleId)) {
                matchCount++;
                break;
            }
        }
    }

    if(matchCount == 0) {
        UA_NODESTORE_RELEASE(server, node);
        unlockServer(server);
        return UA_STATUSCODE_GOOD;
    }

    /* Allocate result array */
    UA_RolePermissionType *result = (UA_RolePermissionType*)
        UA_Array_new(matchCount, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    if(!result) {
        UA_NODESTORE_RELEASE(server, node);
        unlockServer(server);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Fill result array */
    size_t resultIdx = 0;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < rolesSize && resultIdx < matchCount; i++) {
        for(size_t j = 0; j < rp->rolePermissionsSize; j++) {
            if(UA_NodeId_equal(&roles[i], &rp->rolePermissions[j].roleId)) {
                res = UA_NodeId_copy(&roles[i], &result[resultIdx].roleId);
                if(res != UA_STATUSCODE_GOOD) {
                    UA_Array_delete(result, resultIdx, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
                    UA_NODESTORE_RELEASE(server, node);
                    unlockServer(server);
                    return res;
                }
                result[resultIdx].permissions = rp->rolePermissions[j].permissions;
                resultIdx++;
                break;
            }
        }
    }

    *entriesSize = matchCount;
    *entries = result;

    UA_NODESTORE_RELEASE(server, node);

    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

/************************************/
/* RefCount helper                  */
/************************************/

void
UA_Server_decrementRolePermissionsRefCount(UA_Server *server,
                                           UA_PermissionIndex index) {
    if(!server)
        return;
    lockServer(server);
    decrementRefCount(server, index);
    unlockServer(server);
}

#endif /* UA_ENABLE_RBAC */