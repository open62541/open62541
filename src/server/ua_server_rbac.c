/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025-2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_RBAC

/* OPC UA Part 18 Role-Based Access Control Implementation
 * Reference: OPC 10000-18: UA Part 18: Role-Based Security
 *
 * The server internally manages permission configurations with deduplication.
 * Multiple nodes sharing the same set of role permissions will reference the
 * same internal entry via a compact permission index stored in the node head.
 * This index is fully transparent to the public API users. */

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

    /* Bounds check */
    if(server->rolePermissionsSize >= UA_PERMISSION_INDEX_INVALID)
        return UA_STATUSCODE_BADOUTOFRANGE;

    /* Create a new entry */
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

/* Copy config roles into server's internal role registry.
 * Config roles are marked as protected (cannot be removed at runtime). */
static UA_StatusCode
initializeRolesFromConfig(UA_Server *server) {
    UA_ServerConfig *config = &server->config;
    if(config->rolesSize == 0 || !config->roles)
        return UA_STATUSCODE_GOOD;

    server->roles = (UA_Role*)
        UA_calloc(config->rolesSize, sizeof(UA_Role));
    if(!server->roles)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    server->rolesProtected = (UA_Boolean*)
        UA_calloc(config->rolesSize, sizeof(UA_Boolean));
    if(!server->rolesProtected) {
        UA_free(server->roles);
        server->roles = NULL;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(size_t i = 0; i < config->rolesSize; i++) {
        UA_StatusCode res = UA_Role_copy(&config->roles[i],
                                         &server->roles[i]);
        if(res != UA_STATUSCODE_GOOD) {
            /* Clean up already copied entries */
            for(size_t j = 0; j < i; j++)
                UA_Role_clear(&server->roles[j]);
            UA_free(server->roles);
            server->roles = NULL;
            UA_free(server->rolesProtected);
            server->rolesProtected = NULL;
            return res;
        }
        server->rolesProtected[i] = true;
    }
    server->rolesSize = config->rolesSize;
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

#endif /* UA_ENABLE_RBAC */