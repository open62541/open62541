/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Andreas Ebner
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_RBAC

#define UA_ENABLE_RBAC_INFORMATIONMODEL

#ifdef UA_ENABLE_RBAC_INFORMATIONMODEL
UA_StatusCode addRoleRepresentation(UA_Server *server, UA_Role *role);
UA_StatusCode removeRoleRepresentation(UA_Server *server, const UA_NodeId *roleId);
#endif

struct ApplyToHierarchicalChildrenContext {
    UA_Server *server;
    const UA_ReferenceTypeSet *hierarchRefsSet;
    void *callbackContext;
    UA_StatusCode (*applyCallback)(UA_Server *server, const UA_NodeId *nodeId, void *context);
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
    }
    
    UA_NODESTORE_RELEASE(ctx->server, childNode);
    return NULL;
}


static UA_StatusCode
applyToHierarchicalChildren(UA_Server *server, const UA_NodeId *nodeId,
                            UA_StatusCode (*applyCallback)(UA_Server *server, 
                                                           const UA_NodeId *nodeId, 
                                                           void *context),
                            void *callbackContext) {
    UA_ReferenceTypeSet hierarchRefsSet;
    UA_NodeId hr = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    UA_StatusCode res = referenceTypeIndices(server, &hr, &hierarchRefsSet, true);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    
    struct ApplyToHierarchicalChildrenContext ctx;
    ctx.server = server;
    ctx.hierarchRefsSet = &hierarchRefsSet;
    ctx.callbackContext = callbackContext;
    ctx.applyCallback = applyCallback;
    ctx.status = UA_STATUSCODE_GOOD;
    
    for(size_t i = 0; i < node->head.referencesSize && ctx.status == UA_STATUSCODE_GOOD; i++) {
        UA_NodeReferenceKind *rk = &node->head.references[i];
        
        if(rk->isInverse)
            continue;
        if(!UA_ReferenceTypeSet_contains(&hierarchRefsSet, rk->referenceTypeIndex))
            continue;
        
        UA_NodeReferenceKind_iterate(rk, applyToHierarchicalChildrenIterator, &ctx);
    }
    
    UA_NODESTORE_RELEASE(server, node);
    return ctx.status;
}

/* UA_Role Type functions */

void UA_EXPORT
UA_RolePermissions_init(UA_RolePermissions *rp) {
    if(!rp)
        return;
    rp->entriesSize = 0;
    rp->entries = NULL;
    rp->refCount = 0;
}

void UA_EXPORT
UA_RolePermissions_clear(UA_RolePermissions *rp) {
    if(!rp)
        return;
    if(rp->entries) {
        for(size_t i = 0; i < rp->entriesSize; i++) {
            UA_NodeId_clear(&rp->entries[i].roleId);
        }
        UA_free(rp->entries);
        rp->entries = NULL;
    }
    rp->entriesSize = 0;
}

/* Called when a node is being deleted to decrement the refCount of its
 * rolePermissions entry. This ensures the refCount accurately reflects the
 * number of nodes referencing each entry. */
void
UA_Server_decrementRolePermissionsRefCount(UA_Server *server,
                                           UA_PermissionIndex permissionIndex) {
    if(permissionIndex == UA_PERMISSION_INDEX_INVALID)
        return;
    if(permissionIndex >= server->config.rolePermissionsSize)
        return;
    
    UA_RolePermissions *rp = &server->config.rolePermissions[permissionIndex];
    if(rp->refCount > 0)
        rp->refCount--;
}

UA_StatusCode UA_EXPORT
UA_RolePermissions_copy(const UA_RolePermissions *src, UA_RolePermissions *dst) {
    if(!src || !dst)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_RolePermissions_init(dst);
    
    if(src->entriesSize > 0 && src->entries) {
        dst->entries = (UA_RolePermissionEntry*)
            UA_malloc(src->entriesSize * sizeof(UA_RolePermissionEntry));
        if(!dst->entries)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        
        dst->entriesSize = src->entriesSize;
        for(size_t i = 0; i < src->entriesSize; i++) {
            UA_StatusCode res = UA_NodeId_copy(&src->entries[i].roleId, 
                                               &dst->entries[i].roleId);
            if(res != UA_STATUSCODE_GOOD) {
                /* Cleanup on error */
                for(size_t j = 0; j < i; j++) {
                    UA_NodeId_clear(&dst->entries[j].roleId);
                }
                UA_free(dst->entries);
                dst->entries = NULL;
                dst->entriesSize = 0;
                return res;
            }
            dst->entries[i].permissions = src->entries[i].permissions;
        }
    }
    
    return UA_STATUSCODE_GOOD;
}

void UA_EXPORT
UA_Role_init(UA_Role *role) {
    if(!role)
        return;
        
    UA_NodeId_init(&role->roleId);
    UA_QualifiedName_init(&role->roleName);
    role->imrtSize = 0;
    role->imrt = NULL;
    role->applicationsExclude = false;
    role->applicationsSize = 0;
    role->applications = NULL;
    role->endpointsExclude = false;
    role->endpointsSize = 0;
    role->endpoints = NULL;
    role->customConfiguration = false;
}

void UA_EXPORT
UA_Role_clear(UA_Role *role) {
    if(!role)
        return;
        
    UA_NodeId_clear(&role->roleId);
    UA_QualifiedName_clear(&role->roleName);
    
    if(role->imrt) {
        for(size_t i = 0; i < role->imrtSize; i++) {
            UA_IdentityMappingRuleType_clear(&role->imrt[i]);
        }
        UA_free(role->imrt);
    }
    
    if(role->applications) {
        for(size_t i = 0; i < role->applicationsSize; i++) {
            UA_String_clear(&role->applications[i]);
        }
        UA_free(role->applications);
    }

    if(role->endpoints) {
        for(size_t i = 0; i < role->endpointsSize; i++) {
            UA_EndpointType_clear(&role->endpoints[i]);
        }
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
        UA_Role_clear(dst);
        return res;
    }
    
    if(src->imrtSize > 0) {
        dst->imrt = (UA_IdentityMappingRuleType*)
            UA_malloc(src->imrtSize * sizeof(UA_IdentityMappingRuleType));
        if(!dst->imrt) {
            UA_Role_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        
        dst->imrtSize = src->imrtSize;
        for(size_t i = 0; i < src->imrtSize; i++) {
            res = UA_IdentityMappingRuleType_copy(&src->imrt[i], &dst->imrt[i]);
            if(res != UA_STATUSCODE_GOOD) {
                for(size_t j = 0; j < i; j++) {
                    UA_IdentityMappingRuleType_clear(&dst->imrt[j]);
                }
                UA_Role_clear(dst);
                return res;
            }
        }
    }
    
    dst->applicationsExclude = src->applicationsExclude;
    if(src->applicationsSize > 0) {
        dst->applications = (UA_String*)
            UA_malloc(src->applicationsSize * sizeof(UA_String));
        if(!dst->applications) {
            UA_Role_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        
        for(size_t i = 0; i < src->applicationsSize; i++) {
            res = UA_String_copy(&src->applications[i], &dst->applications[i]);
            if(res != UA_STATUSCODE_GOOD) {
                for(size_t j = 0; j < i; j++) {
                    UA_String_clear(&dst->applications[j]);
                }
                UA_free(dst->applications);
                dst->applications = NULL;
                dst->applicationsSize = 0;
                UA_Role_clear(dst);
                return res;
            }
        }
        dst->applicationsSize = src->applicationsSize;
    }
    
    dst->endpointsExclude = src->endpointsExclude;
    if(src->endpointsSize > 0) {
        dst->endpoints = (UA_EndpointType*)
            UA_malloc(src->endpointsSize * sizeof(UA_EndpointType));
        if(!dst->endpoints) {
            UA_Role_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        
        for(size_t i = 0; i < src->endpointsSize; i++) {
            res = UA_EndpointType_copy(&src->endpoints[i], &dst->endpoints[i]);
            if(res != UA_STATUSCODE_GOOD) {
                for(size_t j = 0; j < i; j++) {
                    UA_EndpointType_clear(&dst->endpoints[j]);
                }
                UA_free(dst->endpoints);
                dst->endpoints = NULL;
                dst->endpointsSize = 0;
                UA_Role_clear(dst);
                return res;
            }
        }
        dst->endpointsSize = src->endpointsSize;
    }
    
    dst->customConfiguration = src->customConfiguration;
    
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
        
    if(r1->imrtSize != r2->imrtSize)
        return false;
        
    for(size_t i = 0; i < r1->imrtSize; i++) {
        if(!UA_IdentityMappingRuleType_equal(&r1->imrt[i], &r2->imrt[i]))
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
    
    if(r1->customConfiguration != r2->customConfiguration)
        return false;
        
    return true;
}

/* RBAC Management */

/* Initialize OPC UA well known roles according to Part 18 v1.05 Section 4.3 */
static UA_StatusCode
initializeStandardRoles(UA_Server *server) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_NodeId roleId;
    
    /* Anonymous Role */
    UA_Role anonymousRole;
    UA_Role_init(&anonymousRole);
    anonymousRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    anonymousRole.imrt = (UA_IdentityMappingRuleType*)UA_malloc(2 * sizeof(UA_IdentityMappingRuleType));
    if(!anonymousRole.imrt) 
        return UA_STATUSCODE_BADOUTOFMEMORY;
    anonymousRole.imrtSize = 2;
    UA_IdentityMappingRuleType_init(&anonymousRole.imrt[0]);
    anonymousRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_ANONYMOUS;
    anonymousRole.imrt[0].criteria = UA_STRING_NULL;
    UA_IdentityMappingRuleType_init(&anonymousRole.imrt[1]);
    anonymousRole.imrt[1].criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    anonymousRole.imrt[1].criteria = UA_STRING_NULL;
    
    res = UA_Server_addRole(server, UA_STRING("Anonymous"), UA_STRING_NULL, &anonymousRole, &roleId);
    UA_Role_clear(&anonymousRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* AuthenticatedUser Role */
    UA_Role authenticatedUserRole;
    UA_Role_init(&authenticatedUserRole);
    authenticatedUserRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    authenticatedUserRole.imrt = (UA_IdentityMappingRuleType*)UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!authenticatedUserRole.imrt) 
        return UA_STATUSCODE_BADOUTOFMEMORY;
    authenticatedUserRole.imrtSize = 1;
    UA_IdentityMappingRuleType_init(&authenticatedUserRole.imrt[0]);
    authenticatedUserRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    authenticatedUserRole.imrt[0].criteria = UA_STRING_NULL;
    
    res = UA_Server_addRole(server, UA_STRING("AuthenticatedUser"), UA_STRING_NULL, &authenticatedUserRole, &roleId);
    UA_Role_clear(&authenticatedUserRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* Observer Role - no default identity mapping per OPC UA Part 18
     * Administrator must explicitly configure who gets this role */
    UA_Role observerRole;
    UA_Role_init(&observerRole);
    observerRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    /* Empty identity mapping - role cannot be granted until configured */
    
    res = UA_Server_addRole(server, UA_STRING("Observer"), UA_STRING_NULL, &observerRole, &roleId);
    UA_Role_clear(&observerRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* Operator Role - no default identity mapping per OPC UA Part 18 */
    UA_Role operatorRole;
    UA_Role_init(&operatorRole);
    operatorRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    /* Empty identity mapping - role cannot be granted until configured */
    
    res = UA_Server_addRole(server, UA_STRING("Operator"), UA_STRING_NULL, &operatorRole, &roleId);
    UA_Role_clear(&operatorRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* Engineer Role - no default identity mapping per OPC UA Part 18 */
    UA_Role engineerRole;
    UA_Role_init(&engineerRole);
    engineerRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    /* Empty identity mapping - role cannot be granted until configured */
    
    res = UA_Server_addRole(server, UA_STRING("Engineer"), UA_STRING_NULL, &engineerRole, &roleId);
    UA_Role_clear(&engineerRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* Supervisor Role - no default identity mapping per OPC UA Part 18 */
    UA_Role supervisorRole;
    UA_Role_init(&supervisorRole);
    supervisorRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    /* Empty identity mapping - role cannot be granted until configured */
    
    res = UA_Server_addRole(server, UA_STRING("Supervisor"), UA_STRING_NULL, &supervisorRole, &roleId);
    UA_Role_clear(&supervisorRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* ConfigureAdmin Role - no default identity mapping per OPC UA Part 18 */
    UA_Role configureAdminRole;
    UA_Role_init(&configureAdminRole);
    configureAdminRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    /* Empty identity mapping - role cannot be granted until configured */
    
    res = UA_Server_addRole(server, UA_STRING("ConfigureAdmin"), UA_STRING_NULL, &configureAdminRole, &roleId);
    UA_Role_clear(&configureAdminRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* SecurityAdmin Role - no default identity mapping per OPC UA Part 18 */
    UA_Role securityAdminRole;
    UA_Role_init(&securityAdminRole);
    securityAdminRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    /* Empty identity mapping - role cannot be granted until configured */
    
    res = UA_Server_addRole(server, UA_STRING("SecurityAdmin"), UA_STRING_NULL, &securityAdminRole, &roleId);
    UA_Role_clear(&securityAdminRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    return UA_STATUSCODE_GOOD;
}

/* Initialize roles from configuration during server startup */
static UA_StatusCode
initializeRolesFromConfig(UA_Server *server) {
    UA_StatusCode res = initializeStandardRoles(server);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    if(server->config.rolesSize == 0)
        return UA_STATUSCODE_GOOD;

    for(size_t i = 0; i < server->config.rolesSize; i++) {
        UA_NodeId roleId;
        UA_String roleName = server->config.roles[i].roleName.name;
        UA_String namespaceUri = UA_STRING_NULL;
        
        res = UA_Server_addRole(server, roleName, namespaceUri, 
                               &server->config.roles[i], &roleId);
        if(res != UA_STATUSCODE_GOOD) {
            return res;
        }
        UA_NodeId_clear(&roleId);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_Role*
findRoleById(UA_Server *server, const UA_NodeId *roleId) {
    for(size_t i = 0; i < server->rolesSize; i++) {
        if(UA_NodeId_equal(&server->roles[i].roleId, roleId))
            return &server->roles[i];
    }
    return NULL;
}

static UA_Role*
findRoleByName(UA_Server *server, const UA_String *roleName, 
               const UA_String *namespaceUri) {
    UA_QualifiedName searchName;
    searchName.name = *roleName;
    
    if(namespaceUri && namespaceUri->length > 0) {
        /* Specific namespace requested */
        size_t tempNsIdx = 0;
        UA_StatusCode res = UA_Server_getNamespaceByName(server, *namespaceUri, &tempNsIdx);
        if(res != UA_STATUSCODE_GOOD)
            return NULL;
        searchName.namespaceIndex = (UA_UInt16)tempNsIdx;
        
        for(size_t i = 0; i < server->rolesSize; i++) {
            if(UA_QualifiedName_equal(&server->roles[i].roleName, &searchName))
                return &server->roles[i];
        }
        return NULL;
    } else {
        /* No specific namespace - search in NS0 (well-known roles) and NS1 (default server namespace)
         * According to OPC UA Part 18 v1.05 Section 4.2.2:
         * "If namespaceUri is null or empty then the resulting BrowseName will be qualified by the Server's NamespaceUri"
         * This means default is NS1, but we also check NS0 for well-known roles */
        searchName.namespaceIndex = 0;
        for(size_t i = 0; i < server->rolesSize; i++) {
            if(UA_QualifiedName_equal(&server->roles[i].roleName, &searchName))
                return &server->roles[i];
        }
        
        /* Not found in NS0, try NS1 (default server namespace) */
        searchName.namespaceIndex = 1;
        for(size_t i = 0; i < server->rolesSize; i++) {
            if(UA_QualifiedName_equal(&server->roles[i].roleName, &searchName))
                return &server->roles[i];
        }
        return NULL;
    }
}

static UA_Boolean
isWellKnownRole(const UA_NodeId *roleId) {
    if(roleId->namespaceIndex != 0 || roleId->identifierType != UA_NODEIDTYPE_NUMERIC)
        return false;
        
    UA_UInt32 standardRoleIds[] = {
        UA_NS0ID_WELLKNOWNROLE_ANONYMOUS,
        UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER,
        UA_NS0ID_WELLKNOWNROLE_OBSERVER,
        UA_NS0ID_WELLKNOWNROLE_OPERATOR,
        UA_NS0ID_WELLKNOWNROLE_ENGINEER,
        UA_NS0ID_WELLKNOWNROLE_SUPERVISOR,
        UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN,
        UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN,
        UA_NS0ID_WELLKNOWNROLE_SECURITYKEYSERVERADMIN,
        UA_NS0ID_WELLKNOWNROLE_SECURITYKEYSERVERPUSH,
        UA_NS0ID_WELLKNOWNROLE_SECURITYKEYSERVERACCESS
    };
    
    for(size_t i = 0; i < 11; i++) {
        if(roleId->identifier.numeric == standardRoleIds[i])
            return true;
    }
    return false;
}

/* Check if a role is one of the mandatory well-known roles that cannot be modified
 * According to OPC UA Part 18 v1.05 Section 4.3:
 * "A Server shall not allow changes to the Roles Anonymous, AuthenticatedUser and TrustedApplication" */
static UA_Boolean
isMandatoryWellKnownRole(const UA_NodeId *roleId) {
    if(roleId->namespaceIndex != 0 || roleId->identifierType != UA_NODEIDTYPE_NUMERIC)
        return false;
        
    /* Only Anonymous and AuthenticatedUser are mandatory and protected
     * Note: TrustedApplication is mentioned in spec but not present in current NodeSet */
    return (roleId->identifier.numeric == UA_NS0ID_WELLKNOWNROLE_ANONYMOUS ||
            roleId->identifier.numeric == UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
}

UA_StatusCode UA_EXPORT
UA_Server_addRole(UA_Server *server, UA_String roleName, 
                  UA_String namespaceUri, const UA_Role *role,
                  UA_NodeId *outNewRoleId) {
    if(!server || !outNewRoleId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_Role *newRole = NULL;
    const UA_NodeId *targetNodeId = NULL;
    UA_NodeId generatedNodeId = UA_NODEID_NULL;
    UA_Boolean roleAdded = false;
    UA_UInt16 browseNameNsIdx = 0;
    size_t tempNsIdx = 0;

    /* Determine BrowseName namespace index from NamespaceUri parameter
     * According to OPC UA Part 18 v1.05 Section 4.2.2:
     * "The combination of the NamespaceUri and RoleName parameters are used to 
     *  construct the BrowseName for the new Node.
     *  If this value is null or empty then the resulting BrowseName will be 
     *  qualified by the Server's NamespaceUri." */
    if(namespaceUri.length == 0) {
        browseNameNsIdx = 1;
    } else {
        res = UA_Server_getNamespaceByName(server, namespaceUri, &tempNsIdx);
        if(res != UA_STATUSCODE_GOOD) {
#if UA_MULTITHREADING >= 100
            UA_UNLOCK(&server->serviceMutex);
#endif
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        }
        browseNameNsIdx = (UA_UInt16) tempNsIdx;
    }

    UA_Role *existing = findRoleByName(server, &roleName, &namespaceUri);
    if(existing) {
        res = UA_STATUSCODE_BADNODEIDEXISTS;
        goto cleanup;
    }

    /* Determine the NodeId for the role
     * - Well-known roles: use their predefined NodeId
     * - Custom roles: generate a unique NodeId in namespace 0
     * Note: All roles (well-known and custom) MUST have a NodeId in namespace 0 */
    if(role && !UA_NodeId_isNull(&role->roleId)) {
        /* Role has a specific NodeId (typically well-known role) */
        const UA_Node *existingNode = UA_NODESTORE_GET(server, &role->roleId);
        if(existingNode) {
            UA_NODESTORE_RELEASE(server, existingNode);
            
            /* Only well-known roles are allowed to have existing NodeIds in NS0 */
            if(!isWellKnownRole(&role->roleId)) {
                res = UA_STATUSCODE_BADNODEIDEXISTS;
                goto cleanup;
            }
        }
        targetNodeId = &role->roleId;
    } else {
        /* Generate a unique NodeId for custom role (reused in NS0 representation) */
        UA_UInt32 identifier;
        do {
            identifier = UA_UInt32_random();
            generatedNodeId = UA_NODEID_NUMERIC(0, identifier);
            const UA_Node *existingNode = UA_NODESTORE_GET(server, &generatedNodeId);
            if(!existingNode)
                break; 
            UA_NODESTORE_RELEASE(server, existingNode);
        } while(true);
        
        targetNodeId = &generatedNodeId;
    }

    UA_Role *newRoles = (UA_Role*)UA_realloc(server->roles, 
                                             (server->rolesSize + 1) * sizeof(UA_Role));
    if(!newRoles) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    server->roles = newRoles;
    newRole = &server->roles[server->rolesSize];
    
    if(role) {
        res = UA_Role_copy(role, newRole);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        
        UA_QualifiedName_clear(&newRole->roleName);
        UA_QualifiedName_init(&newRole->roleName);
    } else {
        UA_Role_init(newRole);
    }
    
    UA_NodeId_clear(&newRole->roleId);
    res = UA_NodeId_copy(targetNodeId, &newRole->roleId);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    res = UA_String_copy(&roleName, &newRole->roleName.name);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Set BrowseName namespace index
     * - Well-known roles: always in NS0 (namespace 0)
     * - Custom roles: from NamespaceUri parameter */
    if(isWellKnownRole(targetNodeId)) {
        newRole->roleName.namespaceIndex = 0;
    } else {
        newRole->roleName.namespaceIndex = browseNameNsIdx;
    }

    server->rolesSize++;
    roleAdded = true;

#ifdef UA_ENABLE_RBAC_INFORMATIONMODEL
    /* Add NS0 representation only for custom roles (not well-known roles)
     * Note: newRole->roleId is already set (either predefined or generated above) */
    if(!isWellKnownRole(&newRole->roleId)) {
        res = addRoleRepresentation(server, newRole);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }
#endif
    
    res = UA_NodeId_copy(&newRole->roleId, outNewRoleId);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

cleanup:
    if(res != UA_STATUSCODE_GOOD) {
        if(roleAdded) {
            server->rolesSize--;
        }
        if(newRole) {
            UA_Role_clear(newRole);
        }
    }
    UA_NodeId_clear(&generatedNodeId);

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode UA_EXPORT
UA_Server_removeRole(UA_Server *server, UA_NodeId roleId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* Find the role */
    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }

    /* Don't allow removal of well-known roles */
    if(isWellKnownRole(&roleId)) {
        res = UA_STATUSCODE_BADUSERACCESSDENIED;
        goto cleanup;
    }

#ifdef UA_ENABLE_RBAC_INFORMATIONMODEL
    /* Remove NS0 representation first */
    res = removeRoleRepresentation(server, &roleId);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
#endif

    size_t roleIndex = role - server->roles;
    UA_Role_clear(&server->roles[roleIndex]);

    if(roleIndex < server->rolesSize - 1) {
        memmove(&server->roles[roleIndex], 
                &server->roles[roleIndex + 1],
                (server->rolesSize - roleIndex - 1) * sizeof(UA_Role));
    }

    server->rolesSize--;

    if(server->rolesSize > 0) {
        UA_Role *newRoles = (UA_Role*)
            UA_realloc(server->roles, 
                       server->rolesSize * sizeof(UA_Role));
        if(newRoles)
            server->roles = newRoles;
    } else {
        UA_free(server->roles);
        server->roles = NULL;
    }

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode UA_EXPORT
UA_Server_addRoleApplication(UA_Server *server, UA_NodeId roleId, UA_String uri) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }

    /* Don't allow modification of mandatory well-known roles */
    if(isMandatoryWellKnownRole(&roleId)) {
        res = UA_STATUSCODE_BADUSERACCESSDENIED;
        goto cleanup;
    }

    UA_String *newApps = (UA_String*)
        UA_realloc(role->applications, 
                   (role->applicationsSize + 1) * sizeof(UA_String));
    if(!newApps) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    role->applications = newApps;
    
    res = UA_String_copy(&uri, &role->applications[role->applicationsSize]);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    role->applicationsSize++;

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode UA_EXPORT
UA_Server_removeRoleApplication(UA_Server *server, UA_NodeId roleId, UA_String uri) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }

    /* Don't allow modification of mandatory well-known roles */
    if(isMandatoryWellKnownRole(&roleId)) {
        res = UA_STATUSCODE_BADUSERACCESSDENIED;
        goto cleanup;
    }

    size_t appIndex = SIZE_MAX;
    for(size_t i = 0; i < role->applicationsSize; i++) {
        if(UA_String_equal(&role->applications[i], &uri)) {
            appIndex = i;
            break;
        }
    }

    if(appIndex == SIZE_MAX) {
        res = UA_STATUSCODE_BADNOTFOUND;
        goto cleanup;
    }

    UA_String_clear(&role->applications[appIndex]);

    if(appIndex < role->applicationsSize - 1) {
        memmove(&role->applications[appIndex], 
                &role->applications[appIndex + 1],
                (role->applicationsSize - appIndex - 1) * sizeof(UA_String));
    }

    role->applicationsSize--;

    if(role->applicationsSize > 0) {
        UA_String *newApps = (UA_String*)
            UA_realloc(role->applications, 
                       role->applicationsSize * sizeof(UA_String));
        if(newApps)
            role->applications = newApps;
    } else {
        UA_free(role->applications);
        role->applications = NULL;
    }

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode UA_EXPORT
UA_Server_addRoleEndpoint(UA_Server *server, UA_NodeId roleId, UA_EndpointType endpoint) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }

    /* Don't allow modification of mandatory well-known roles */
    if(isMandatoryWellKnownRole(&roleId)) {
        res = UA_STATUSCODE_BADUSERACCESSDENIED;
        goto cleanup;
    }

    UA_EndpointType *newEndpoints = (UA_EndpointType*)
        UA_realloc(role->endpoints, 
                   (role->endpointsSize + 1) * sizeof(UA_EndpointType));
    if(!newEndpoints) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    role->endpoints = newEndpoints;
    
    res = UA_EndpointType_copy(&endpoint, &role->endpoints[role->endpointsSize]);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    role->endpointsSize++;

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode UA_EXPORT
UA_Server_removeRoleEndpoint(UA_Server *server, UA_NodeId roleId, UA_EndpointType endpoint) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }

    /* Don't allow modification of mandatory well-known roles */
    if(isMandatoryWellKnownRole(&roleId)) {
        res = UA_STATUSCODE_BADUSERACCESSDENIED;
        goto cleanup;
    }

    size_t endpointIndex = SIZE_MAX;
    for(size_t i = 0; i < role->endpointsSize; i++) {
        if(UA_EndpointType_equal(&role->endpoints[i], &endpoint)) {
            endpointIndex = i;
            break;
        }
    }

    if(endpointIndex == SIZE_MAX) {
        res = UA_STATUSCODE_BADNOTFOUND;
        goto cleanup;
    }

    UA_EndpointType_clear(&role->endpoints[endpointIndex]);

    if(endpointIndex < role->endpointsSize - 1) {
        memmove(&role->endpoints[endpointIndex], 
                &role->endpoints[endpointIndex + 1],
                (role->endpointsSize - endpointIndex - 1) * sizeof(UA_EndpointType));
    }

    role->endpointsSize--;

    if(role->endpointsSize > 0) {
        UA_EndpointType *newEndpoints = (UA_EndpointType*)
            UA_realloc(role->endpoints, 
                       role->endpointsSize * sizeof(UA_EndpointType));
        if(newEndpoints)
            role->endpoints = newEndpoints;
    } else {
        UA_free(role->endpoints);
        role->endpoints = NULL;
    }

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode UA_EXPORT
UA_Server_addRoleIdentity(UA_Server *server, UA_NodeId roleId,
                          UA_IdentityCriteriaType criteriaType, UA_String criteria) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }

    /* Don't allow modification of mandatory well-known roles */
    if(isMandatoryWellKnownRole(&roleId)) {
        res = UA_STATUSCODE_BADUSERACCESSDENIED;
        goto cleanup;
    }

    /* Reallocate identity mapping rules array */
    UA_IdentityMappingRuleType *newImrt = (UA_IdentityMappingRuleType*)
        UA_realloc(role->imrt, 
                   (role->imrtSize + 1) * sizeof(UA_IdentityMappingRuleType));
    if(!newImrt) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    role->imrt = newImrt;
    
    /* Initialize and set identity criteria */
    UA_IdentityMappingRuleType_init(&role->imrt[role->imrtSize]);
    role->imrt[role->imrtSize].criteriaType = criteriaType;
    
    /* Copy criteria string (can be empty for Anonymous/AuthenticatedUser) */
    if(criteria.length > 0) {
        res = UA_String_copy(&criteria, &role->imrt[role->imrtSize].criteria);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    role->imrtSize++;

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode UA_EXPORT
UA_Server_removeRoleIdentity(UA_Server *server, UA_NodeId roleId, UA_IdentityCriteriaType ict) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }

    /* Don't allow modification of mandatory well-known roles */
    if(isMandatoryWellKnownRole(&roleId)) {
        res = UA_STATUSCODE_BADUSERACCESSDENIED;
        goto cleanup;
    }

    size_t imrtIndex = SIZE_MAX;
    for(size_t i = 0; i < role->imrtSize; i++) {
        if(role->imrt[i].criteriaType == ict) {
            imrtIndex = i;
            break;
        }
    }

    if(imrtIndex == SIZE_MAX) {
        res = UA_STATUSCODE_BADNOTFOUND;
        goto cleanup;
    }

    UA_IdentityMappingRuleType_clear(&role->imrt[imrtIndex]);

    if(imrtIndex < role->imrtSize - 1) {
        memmove(&role->imrt[imrtIndex], 
                &role->imrt[imrtIndex + 1],
                (role->imrtSize - imrtIndex - 1) * sizeof(UA_IdentityMappingRuleType));
    }

    role->imrtSize--;

    if(role->imrtSize > 0) {
        UA_IdentityMappingRuleType *newImrt = (UA_IdentityMappingRuleType*)
            UA_realloc(role->imrt, 
                       role->imrtSize * sizeof(UA_IdentityMappingRuleType));
        if(newImrt)
            role->imrt = newImrt;
    } else {
        UA_free(role->imrt);
        role->imrt = NULL;
    }

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}


/* RBAC Init */

UA_StatusCode
UA_Server_initializeRBAC(UA_Server *server) {
    return initializeRolesFromConfig(server);
}

void
UA_Server_cleanupRBAC(UA_Server *server) {
    if(!server->roles)
        return;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    for(size_t i = 0; i < server->rolesSize; i++) {
        UA_Role_clear(&server->roles[i]);
    }
    
    UA_free(server->roles);
    server->roles = NULL;
    server->rolesSize = 0;

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif
}

/* RBAC Query Functions */

UA_EXPORT const UA_Role *
UA_Server_getRoleById(UA_Server *server, UA_NodeId roleId) {
    if(!server)
        return NULL;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_Role *role = findRoleById(server, &roleId);

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return role;
}

UA_EXPORT const UA_Role *
UA_Server_getRoleByName(UA_Server *server, UA_String roleName, UA_String namespaceUri) {
    if(!server)
        return NULL;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_Role *role = findRoleByName(server, &roleName, &namespaceUri);

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return role;
}

UA_StatusCode UA_EXPORT
UA_Server_getRoles(UA_Server *server, size_t *rolesSize, UA_NodeId **roleIds) {
    if(!server || !rolesSize || !roleIds)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    *rolesSize = 0;
    *roleIds = NULL;

    if(server->rolesSize == 0) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_GOOD;
    }

    /* Allocate array for role NodeIds */
    *roleIds = (UA_NodeId*)UA_malloc(server->rolesSize * sizeof(UA_NodeId));
    if(!*roleIds) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Copy all role NodeIds */
    for(size_t i = 0; i < server->rolesSize; i++) {
        UA_StatusCode res = UA_NodeId_copy(&server->roles[i].roleId, &(*roleIds)[i]);
        if(res != UA_STATUSCODE_GOOD) {
            /* Clean up on error */
            for(size_t j = 0; j < i; j++) {
                UA_NodeId_clear(&(*roleIds)[j]);
            }
            UA_free(*roleIds);
            *roleIds = NULL;
#if UA_MULTITHREADING >= 100
            UA_UNLOCK(&server->serviceMutex);
#endif
            return res;
        }
    }

    *rolesSize = server->rolesSize;

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return UA_STATUSCODE_GOOD;
}

/*******************************/
/* Node RolePermissions API    */
/*******************************/

/* Helper: Check if two RolePermissions have identical entries 
 * (same roles and same permissions) */
static UA_Boolean
rolePermissionsEqual(const UA_RolePermissions *a, const UA_RolePermissions *b) {
    if(a->entriesSize != b->entriesSize)
        return false;
    
    /* Check if every entry in 'a' has a matching entry in 'b' */
    for(size_t i = 0; i < a->entriesSize; i++) {
        UA_Boolean found = false;
        for(size_t j = 0; j < b->entriesSize; j++) {
            if(UA_NodeId_equal(&a->entries[i].roleId, &b->entries[j].roleId) &&
               a->entries[i].permissions == b->entries[j].permissions) {
                found = true;
                break;
            }
        }
        if(!found)
            return false;
    }
    return true;
}

/* Helper: Find an existing RolePermissions entry that matches the given entries.
 * Returns UA_PERMISSION_INDEX_INVALID if not found. 
 * Called with mutex already locked. */
static UA_PermissionIndex
findMatchingRolePermissions(UA_Server *server, 
                            size_t entriesSize, const UA_RolePermissionEntry *entries) {
    UA_RolePermissions temp;
    temp.entriesSize = entriesSize;
    temp.entries = (UA_RolePermissionEntry*)(uintptr_t)entries; /* Cast away const for comparison only */
    temp.refCount = 0;
    
    for(size_t i = 0; i < server->config.rolePermissionsSize; i++) {
        if(rolePermissionsEqual(&server->config.rolePermissions[i], &temp))
            return (UA_PermissionIndex)i;
    }
    return UA_PERMISSION_INDEX_INVALID;
}

/* Helper: Find a free slot (refCount == 0) in the rolePermissions array.
 * Returns UA_PERMISSION_INDEX_INVALID if no free slot found.
 * Called with mutex already locked. */
static UA_PermissionIndex
findFreeRolePermissionsSlot(UA_Server *server) {
    for(size_t i = 0; i < server->config.rolePermissionsSize; i++) {
        if(server->config.rolePermissions[i].refCount == 0)
            return (UA_PermissionIndex)i;
    }
    return UA_PERMISSION_INDEX_INVALID;
}

/* Helper: Create a new RolePermissions entry or reuse a free slot.
 * Called with mutex already locked. 
 * Does NOT update refCount - caller must do that. */
static UA_StatusCode
createOrReuseRolePermissionsSlot(UA_Server *server,
                                 size_t entriesSize, const UA_RolePermissionEntry *entries,
                                 UA_PermissionIndex *outIndex) {
    /* First, try to find an existing entry with matching permissions */
    UA_PermissionIndex existingIdx = findMatchingRolePermissions(server, entriesSize, entries);
    if(existingIdx != UA_PERMISSION_INDEX_INVALID) {
        *outIndex = existingIdx;
        return UA_STATUSCODE_GOOD;
    }
    
    /* Try to find a free slot (refCount == 0) */
    UA_PermissionIndex freeIdx = findFreeRolePermissionsSlot(server);
    
    if(freeIdx != UA_PERMISSION_INDEX_INVALID) {
        /* Reuse the free slot - clear old data first */
        UA_RolePermissions *slot = &server->config.rolePermissions[freeIdx];
        UA_RolePermissions_clear(slot);
        UA_RolePermissions_init(slot);
        
        if(entriesSize > 0) {
            slot->entries = (UA_RolePermissionEntry*)
                UA_malloc(entriesSize * sizeof(UA_RolePermissionEntry));
            if(!slot->entries)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            
            slot->entriesSize = entriesSize;
            for(size_t i = 0; i < entriesSize; i++) {
                UA_StatusCode res = UA_NodeId_copy(&entries[i].roleId, &slot->entries[i].roleId);
                if(res != UA_STATUSCODE_GOOD) {
                    for(size_t j = 0; j < i; j++)
                        UA_NodeId_clear(&slot->entries[j].roleId);
                    UA_free(slot->entries);
                    slot->entries = NULL;
                    slot->entriesSize = 0;
                    return res;
                }
                slot->entries[i].permissions = entries[i].permissions;
            }
        }
        *outIndex = freeIdx;
        return UA_STATUSCODE_GOOD;
    }
    
    /* No free slot - need to append to array */
    if(server->config.rolePermissionsSize >= UA_PERMISSION_INDEX_INVALID)
        return UA_STATUSCODE_BADOUTOFRANGE;
    
    UA_RolePermissions *newArray = (UA_RolePermissions*)
        UA_realloc(server->config.rolePermissions,
                   (server->config.rolePermissionsSize + 1) * sizeof(UA_RolePermissions));
    if(!newArray)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    
    server->config.rolePermissions = newArray;
    UA_PermissionIndex newIndex = (UA_PermissionIndex)server->config.rolePermissionsSize;
    
    UA_RolePermissions *entry = &server->config.rolePermissions[newIndex];
    UA_RolePermissions_init(entry);
    
    if(entriesSize > 0) {
        entry->entries = (UA_RolePermissionEntry*)
            UA_malloc(entriesSize * sizeof(UA_RolePermissionEntry));
        if(!entry->entries)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        
        entry->entriesSize = entriesSize;
        for(size_t i = 0; i < entriesSize; i++) {
            UA_StatusCode res = UA_NodeId_copy(&entries[i].roleId, &entry->entries[i].roleId);
            if(res != UA_STATUSCODE_GOOD) {
                for(size_t j = 0; j < i; j++)
                    UA_NodeId_clear(&entry->entries[j].roleId);
                UA_free(entry->entries);
                entry->entries = NULL;
                entry->entriesSize = 0;
                return res;
            }
            entry->entries[i].permissions = entries[i].permissions;
        }
    }
    
    server->config.rolePermissionsSize++;
    *outIndex = newIndex;
    return UA_STATUSCODE_GOOD;
}

/* Internal helper function for adding role permissions to a single node
 * Called with mutex already locked.
 * 
 * IMPORTANT: RolePermissions entries are IMMUTABLE once created with refCount > 0.
 * When modifying permissions for a node, we:
 * 1. Decrement refCount of the old entry (if any)
 * 2. Build the new desired permission set
 * 3. Find or create a matching entry
 * 4. Increment refCount of the new entry
 * 5. Update the node's permissionIndex */
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
    UA_RolePermissionEntry *newEntries = NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    if(currentIndex == UA_PERMISSION_INDEX_INVALID) {
        /* No existing permissions - create new with just this role */
        newEntriesSize = 1;
        newEntries = (UA_RolePermissionEntry*)UA_malloc(sizeof(UA_RolePermissionEntry));
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
        UA_RolePermissions *oldRp = &server->config.rolePermissions[currentIndex];
        
        /* Find if this role already exists in current permissions */
        size_t existingRoleIdx = SIZE_MAX;
        for(size_t i = 0; i < oldRp->entriesSize; i++) {
            if(UA_NodeId_equal(&oldRp->entries[i].roleId, roleId)) {
                existingRoleIdx = i;
                break;
            }
        }
        
        if(existingRoleIdx != SIZE_MAX) {
            /* Role exists - copy all and modify the role's permissions */
            newEntriesSize = oldRp->entriesSize;
            newEntries = (UA_RolePermissionEntry*)
                UA_malloc(newEntriesSize * sizeof(UA_RolePermissionEntry));
            if(!newEntries)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            
            for(size_t i = 0; i < oldRp->entriesSize; i++) {
                res = UA_NodeId_copy(&oldRp->entries[i].roleId, &newEntries[i].roleId);
                if(res != UA_STATUSCODE_GOOD) {
                    for(size_t j = 0; j < i; j++)
                        UA_NodeId_clear(&newEntries[j].roleId);
                    UA_free(newEntries);
                    return res;
                }
                if(i == existingRoleIdx) {
                    /* Apply the permission change */
                    if(overwriteExisting)
                        newEntries[i].permissions = permissionType;
                    else
                        newEntries[i].permissions = oldRp->entries[i].permissions | permissionType;
                } else {
                    newEntries[i].permissions = oldRp->entries[i].permissions;
                }
            }
        } else {
            /* Role doesn't exist - copy all and add new entry */
            newEntriesSize = oldRp->entriesSize + 1;
            newEntries = (UA_RolePermissionEntry*)
                UA_malloc(newEntriesSize * sizeof(UA_RolePermissionEntry));
            if(!newEntries)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            
            for(size_t i = 0; i < oldRp->entriesSize; i++) {
                res = UA_NodeId_copy(&oldRp->entries[i].roleId, &newEntries[i].roleId);
                if(res != UA_STATUSCODE_GOOD) {
                    for(size_t j = 0; j < i; j++)
                        UA_NodeId_clear(&newEntries[j].roleId);
                    UA_free(newEntries);
                    return res;
                }
                newEntries[i].permissions = oldRp->entries[i].permissions;
            }
            /* Add new role entry at the end */
            res = UA_NodeId_copy(roleId, &newEntries[oldRp->entriesSize].roleId);
            if(res != UA_STATUSCODE_GOOD) {
                for(size_t j = 0; j < oldRp->entriesSize; j++)
                    UA_NodeId_clear(&newEntries[j].roleId);
                UA_free(newEntries);
                return res;
            }
            newEntries[oldRp->entriesSize].permissions = permissionType;
        }
    }
    
    /* Find or create a slot for the new permissions */
    UA_PermissionIndex targetIndex;
    res = createOrReuseRolePermissionsSlot(server, newEntriesSize, newEntries, &targetIndex);
    
    /* Clean up temporary entries */
    for(size_t i = 0; i < newEntriesSize; i++)
        UA_NodeId_clear(&newEntries[i].roleId);
    UA_free(newEntries);
    
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    /* Update refcounts and node's permissionIndex if changed */
    if(targetIndex != currentIndex) {
        /* Decrement old refCount */
        if(currentIndex != UA_PERMISSION_INDEX_INVALID) {
            UA_RolePermissions *oldRp = &server->config.rolePermissions[currentIndex];
            if(oldRp->refCount > 0)
                oldRp->refCount--;
        }
        
        /* Increment new refCount */
        server->config.rolePermissions[targetIndex].refCount++;
        
        /* Update node's permission index */
        UA_Node *editNode = UA_NODESTORE_GET_EDIT(server, nodeId);
        if(!editNode) {
            /* Rollback refcount changes */
            server->config.rolePermissions[targetIndex].refCount--;
            if(currentIndex != UA_PERMISSION_INDEX_INVALID)
                server->config.rolePermissions[currentIndex].refCount++;
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
        }
        editNode->head.permissionIndex = targetIndex;
        UA_NODESTORE_RELEASE(server, editNode);
    }
    /* If targetIndex == currentIndex, the permissions might already match 
     * (found existing matching entry) - no refcount change needed */
    
    return UA_STATUSCODE_GOOD;
}

/* Callback context for recursive addRolePermissions */
struct AddRolePermissionsContext {
    const UA_NodeId *roleId;
    UA_PermissionType permissionType;
    UA_Boolean overwriteExisting;
};

/* Callback for recursive application - called without mutex (mutex already held) */
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

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    const UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }
    
    /* Use internal helper for the main node */
    res = addRolePermissionsInternal(server, &nodeId, &roleId, permissionType, overwriteExisting);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    
    /* Handle recursive application */
    if(recursive) {
        struct AddRolePermissionsContext ctx;
        ctx.roleId = &roleId;
        ctx.permissionType = permissionType;
        ctx.overwriteExisting = overwriteExisting;
        
        res = applyToHierarchicalChildren(server, &nodeId, addRolePermissionsCallback, &ctx);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

/* Internal helper function for removing role permissions from a single node
 * Called with mutex already locked.
 * 
 * IMPORTANT: RolePermissions entries are IMMUTABLE once created with refCount > 0.
 * When removing permissions for a node, we:
 * 1. Decrement refCount of the old entry
 * 2. Build the new desired permission set (with specified permissions removed)
 * 3. Find or create a matching entry (or set to INVALID if no permissions remain)
 * 4. Increment refCount of the new entry
 * 5. Update the node's permissionIndex */
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
    
    UA_RolePermissions *oldRp = &server->config.rolePermissions[currentIndex];
    
    /* Find the role in current permissions */
    size_t roleEntryIdx = SIZE_MAX;
    for(size_t i = 0; i < oldRp->entriesSize; i++) {
        if(UA_NodeId_equal(&oldRp->entries[i].roleId, roleId)) {
            roleEntryIdx = i;
            break;
        }
    }
    
    /* Role not found in permissions - nothing to remove */
    if(roleEntryIdx == SIZE_MAX)
        return UA_STATUSCODE_GOOD;
    
    /* Calculate new permissions for this role */
    UA_PermissionType newPerms = oldRp->entries[roleEntryIdx].permissions & ~permissionType;
    
    /* Build new entries array */
    size_t newEntriesSize = 0;
    UA_RolePermissionEntry *newEntries = NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    if(newPerms == 0) {
        /* Role has no permissions left - exclude it from new entries */
        newEntriesSize = oldRp->entriesSize - 1;
    } else {
        /* Role still has some permissions - keep it with modified permissions */
        newEntriesSize = oldRp->entriesSize;
    }
    
    UA_PermissionIndex targetIndex = UA_PERMISSION_INDEX_INVALID;
    
    if(newEntriesSize > 0) {
        newEntries = (UA_RolePermissionEntry*)
            UA_malloc(newEntriesSize * sizeof(UA_RolePermissionEntry));
        if(!newEntries)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        
        size_t j = 0;
        for(size_t i = 0; i < oldRp->entriesSize; i++) {
            if(i == roleEntryIdx) {
                if(newPerms != 0) {
                    /* Keep this role with modified permissions */
                    res = UA_NodeId_copy(&oldRp->entries[i].roleId, &newEntries[j].roleId);
                    if(res != UA_STATUSCODE_GOOD) {
                        for(size_t k = 0; k < j; k++)
                            UA_NodeId_clear(&newEntries[k].roleId);
                        UA_free(newEntries);
                        return res;
                    }
                    newEntries[j].permissions = newPerms;
                    j++;
                }
                /* If newPerms == 0, skip this entry (don't copy it) */
            } else {
                /* Copy other entries unchanged */
                res = UA_NodeId_copy(&oldRp->entries[i].roleId, &newEntries[j].roleId);
                if(res != UA_STATUSCODE_GOOD) {
                    for(size_t k = 0; k < j; k++)
                        UA_NodeId_clear(&newEntries[k].roleId);
                    UA_free(newEntries);
                    return res;
                }
                newEntries[j].permissions = oldRp->entries[i].permissions;
                j++;
            }
        }
        
        /* Find or create a slot for the new permissions */
        res = createOrReuseRolePermissionsSlot(server, newEntriesSize, newEntries, &targetIndex);
        
        /* Clean up temporary entries */
        for(size_t i = 0; i < newEntriesSize; i++)
            UA_NodeId_clear(&newEntries[i].roleId);
        UA_free(newEntries);
        
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    /* If newEntriesSize == 0, targetIndex stays UA_PERMISSION_INDEX_INVALID */
    
    /* Update refcounts and node's permissionIndex if changed */
    if(targetIndex != currentIndex) {
        /* Decrement old refCount */
        if(oldRp->refCount > 0)
            oldRp->refCount--;
        
        /* Increment new refCount (if not INVALID) */
        if(targetIndex != UA_PERMISSION_INDEX_INVALID)
            server->config.rolePermissions[targetIndex].refCount++;
        
        /* Update node's permission index */
        UA_Node *editNode = UA_NODESTORE_GET_EDIT(server, nodeId);
        if(!editNode) {
            /* Rollback refcount changes */
            if(targetIndex != UA_PERMISSION_INDEX_INVALID)
                server->config.rolePermissions[targetIndex].refCount--;
            oldRp->refCount++;
            return UA_STATUSCODE_BADNODEIDUNKNOWN;
        }
        editNode->head.permissionIndex = targetIndex;
        UA_NODESTORE_RELEASE(server, editNode);
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

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    const UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }
    
    res = removeRolePermissionsInternal(server, &nodeId, &roleId, permissionType);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    
    if(recursive) {
        struct RemoveRolePermissionsContext ctx;
        ctx.roleId = &roleId;
        ctx.permissionType = permissionType;
        
        res = applyToHierarchicalChildren(server, &nodeId, removeRolePermissionsCallback, &ctx);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

cleanup:

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

/* Session Roles Management */

UA_StatusCode
UA_Server_getSessionRoles(UA_Server *server, const UA_NodeId *sessionId,
                          size_t *rolesSize, UA_NodeId **roleIds) {
    if(!server || !sessionId || !rolesSize || !roleIds)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_Session *session = getSessionById(server, sessionId);
    if(!session) {
        res = UA_STATUSCODE_BADSESSIONIDINVALID;
        goto cleanup;
    }

    /* Deep copy the roles array */
    if(session->rolesSize > 0) {
        res = UA_Array_copy(session->roles, session->rolesSize,
                           (void**)roleIds, &UA_TYPES[UA_TYPES_NODEID]);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        *rolesSize = session->rolesSize;
    } else {
        *rolesSize = 0;
        *roleIds = NULL;
    }

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode
UA_Server_setSessionRoles(UA_Server *server, const UA_NodeId *sessionId,
                          size_t rolesSize, const UA_NodeId *roleIds) {
    if(!server || !sessionId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(rolesSize > 0 && !roleIds)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_Session *session = getSessionById(server, sessionId);
    if(!session) {
        res = UA_STATUSCODE_BADSESSIONIDINVALID;
        goto cleanup;
    }

    /* Validate that all role IDs exist */
    for(size_t i = 0; i < rolesSize; i++) {
        const UA_Role *role = findRoleById(server, &roleIds[i]);
        if(!role) {
            res = UA_STATUSCODE_BADNODEIDUNKNOWN;
            goto cleanup;
        }
    }

    UA_Array_delete(session->roles, session->rolesSize, &UA_TYPES[UA_TYPES_NODEID]);
    session->roles = NULL;
    session->rolesSize = 0;

    if(rolesSize > 0) {
        res = UA_Array_copy(roleIds, rolesSize,
                           (void**)&session->roles, &UA_TYPES[UA_TYPES_NODEID]);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
        session->rolesSize = rolesSize;
    }

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode
UA_Server_addSessionRole(UA_Server *server, const UA_NodeId *sessionId,
                         const UA_NodeId roleId) {
    if(!server || !sessionId)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;

    UA_Session *session = getSessionById(server, sessionId);
    if(!session) {
        res = UA_STATUSCODE_BADSESSIONIDINVALID;
        goto cleanup;
    }

    /* Validate that the role exists */
    const UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }

    /* Check if role is already assigned */
    for(size_t i = 0; i < session->rolesSize; i++) {
        if(UA_NodeId_equal(&session->roles[i], &roleId)) {
            goto cleanup;
        }
    }

    UA_NodeId *newRoles = (UA_NodeId*)
        UA_realloc(session->roles, (session->rolesSize + 1) * sizeof(UA_NodeId));
    if(!newRoles) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    session->roles = newRoles;

    res = UA_NodeId_copy(&roleId, &session->roles[session->rolesSize]);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    session->rolesSize++;

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

/* Role Permission Configuration Management */

UA_StatusCode
UA_Server_addRolePermissionConfig(UA_Server *server,
                                  size_t entriesSize, const UA_RolePermissionEntry *entries,
                                  UA_PermissionIndex *outIndex) {
    if(!server || (entriesSize > 0 && !entries))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    if(server->config.rolePermissionsSize >= UA_PERMISSION_INDEX_INVALID) {
        res = UA_STATUSCODE_BADOUTOFRANGE;
        goto cleanup;
    }

    for(size_t i = 0; i < entriesSize; i++) {
        const UA_Role *role = findRoleById(server, &entries[i].roleId);
        if(!role) {
            res = UA_STATUSCODE_BADNODEIDUNKNOWN;
            goto cleanup;
        }
    }

    UA_RolePermissions *newArray = (UA_RolePermissions*)
        UA_realloc(server->config.rolePermissions,
                   (server->config.rolePermissionsSize + 1) * sizeof(UA_RolePermissions));
    if(!newArray) {
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    server->config.rolePermissions = newArray;
    UA_PermissionIndex newIndex = (UA_PermissionIndex)server->config.rolePermissionsSize;
    
    UA_RolePermissions *entry = &server->config.rolePermissions[newIndex];
    UA_RolePermissions_init(entry);
    
    if(entriesSize > 0) {
        entry->entries = (UA_RolePermissionEntry*)
            UA_malloc(entriesSize * sizeof(UA_RolePermissionEntry));
        if(!entry->entries) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        
        entry->entriesSize = entriesSize;
        for(size_t i = 0; i < entriesSize; i++) {
            res = UA_NodeId_copy(&entries[i].roleId, &entry->entries[i].roleId);
            if(res != UA_STATUSCODE_GOOD) {
                for(size_t j = 0; j < i; j++) {
                    UA_NodeId_clear(&entry->entries[j].roleId);
                }
                UA_free(entry->entries);
                entry->entries = NULL;
                entry->entriesSize = 0;
                goto cleanup;
            }
            entry->entries[i].permissions = entries[i].permissions;
        }
    }
    
    server->config.rolePermissionsSize++;
    
    if(outIndex)
        *outIndex = newIndex;

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

const UA_RolePermissions *
UA_Server_getRolePermissionConfig(UA_Server *server, UA_PermissionIndex index) {
    if(!server || index >= server->config.rolePermissionsSize)
        return NULL;
    
    return &server->config.rolePermissions[index];
}

UA_StatusCode
UA_Server_updateRolePermissionConfig(UA_Server *server, UA_PermissionIndex index,
                                     size_t entriesSize, const UA_RolePermissionEntry *entries) {
    if(!server || (entriesSize > 0 && !entries))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    if(index >= server->config.rolePermissionsSize)
        return UA_STATUSCODE_BADOUTOFRANGE;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    UA_RolePermissions *config = &server->config.rolePermissions[index];
    
    /* Cannot modify entries that are still referenced by nodes.
     * This ensures immutability - nodes sharing this entry would be affected. */
    if(config->refCount > 0) {
        res = UA_STATUSCODE_BADINVALIDSTATE;
        goto cleanup;
    }
    
    /* Validate that all role IDs exist */
    for(size_t i = 0; i < entriesSize; i++) {
        const UA_Role *role = findRoleById(server, &entries[i].roleId);
        if(!role) {
            res = UA_STATUSCODE_BADNODEIDUNKNOWN;
            goto cleanup;
        }
    }

    /* Clear old entries */
    if(config->entries) {
        for(size_t i = 0; i < config->entriesSize; i++) {
            UA_NodeId_clear(&config->entries[i].roleId);
        }
        UA_free(config->entries);
        config->entries = NULL;
        config->entriesSize = 0;
    }
    
    /* Copy new entries */
    if(entriesSize > 0) {
        config->entries = (UA_RolePermissionEntry*)
            UA_malloc(entriesSize * sizeof(UA_RolePermissionEntry));
        if(!config->entries) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        
        config->entriesSize = entriesSize;
        for(size_t i = 0; i < entriesSize; i++) {
            res = UA_NodeId_copy(&entries[i].roleId, &config->entries[i].roleId);
            if(res != UA_STATUSCODE_GOOD) {
                /* Cleanup on error */
                for(size_t j = 0; j < i; j++) {
                    UA_NodeId_clear(&config->entries[j].roleId);
                }
                UA_free(config->entries);
                config->entries = NULL;
                config->entriesSize = 0;
                goto cleanup;
            }
            config->entries[i].permissions = entries[i].permissions;
        }
    }

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}


/* Internal helper function for setting a node's permission index.
 * Called with mutex already locked.
 * Properly manages refCounts when changing a node's permission index. */
static UA_StatusCode
setNodePermissionIndexInternal(UA_Server *server, const UA_NodeId *nodeId,
                               UA_PermissionIndex permissionIndex) {
    if(permissionIndex != UA_PERMISSION_INDEX_INVALID &&
       permissionIndex >= server->config.rolePermissionsSize)
        return UA_STATUSCODE_BADOUTOFRANGE;
    
    /* Get current permission index */
    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    
    UA_PermissionIndex currentIndex = node->head.permissionIndex;
    UA_NODESTORE_RELEASE(server, node);
    
    /* No change needed */
    if(currentIndex == permissionIndex)
        return UA_STATUSCODE_GOOD;
    
    /* Decrement old refCount */
    if(currentIndex != UA_PERMISSION_INDEX_INVALID) {
        UA_RolePermissions *oldRp = &server->config.rolePermissions[currentIndex];
        if(oldRp->refCount > 0)
            oldRp->refCount--;
    }
    
    /* Increment new refCount */
    if(permissionIndex != UA_PERMISSION_INDEX_INVALID)
        server->config.rolePermissions[permissionIndex].refCount++;
    
    /* Update node's permission index */
    UA_Node *editNode = UA_NODESTORE_GET_EDIT(server, nodeId);
    if(!editNode) {
        /* Rollback refcount changes */
        if(permissionIndex != UA_PERMISSION_INDEX_INVALID)
            server->config.rolePermissions[permissionIndex].refCount--;
        if(currentIndex != UA_PERMISSION_INDEX_INVALID)
            server->config.rolePermissions[currentIndex].refCount++;
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    
    editNode->head.permissionIndex = permissionIndex;
    UA_NODESTORE_RELEASE(server, editNode);
    
    return UA_STATUSCODE_GOOD;
}

struct SetNodePermissionIndexContext {
    UA_PermissionIndex permissionIndex;
};

static UA_StatusCode
setNodePermissionIndexCallback(UA_Server *server, const UA_NodeId *nodeId, void *context) {
    struct SetNodePermissionIndexContext *ctx = (struct SetNodePermissionIndexContext*)context;
    return setNodePermissionIndexInternal(server, nodeId, ctx->permissionIndex);
}

UA_StatusCode
UA_Server_setNodePermissionIndex(UA_Server *server, const UA_NodeId nodeId,
                                 UA_PermissionIndex permissionIndex,
                                 UA_Boolean recursive) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    res = setNodePermissionIndexInternal(server, &nodeId, permissionIndex);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    
    /* Handle recursive application */
    if(recursive) {
        struct SetNodePermissionIndexContext ctx;
        ctx.permissionIndex = permissionIndex;
        
        res = applyToHierarchicalChildren(server, &nodeId, setNodePermissionIndexCallback, &ctx);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

UA_StatusCode
UA_Server_getNodePermissionIndex(UA_Server *server, const UA_NodeId nodeId,
                                 UA_PermissionIndex *permissionIndex) {
    if(!server || !permissionIndex)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    const UA_Node *node = UA_NODESTORE_GET(server, &nodeId);
    if(!node) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }
    
    *permissionIndex = node->head.permissionIndex;
    
    UA_NODESTORE_RELEASE(server, node);

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

/* Helper function to compute effective permissions for a set of roles on a node.
 * Must be called with server mutex held. */
static UA_PermissionType
computeEffectivePermissions(UA_Server *server, const UA_Node *node,
                            size_t rolesSize, const UA_NodeId *roles) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    
    /* If node has no permission configuration, return full permissions
     * (default permissive behavior) */
    if(node->head.permissionIndex == UA_PERMISSION_INDEX_INVALID)
        return 0xFFFFFFFF;
    
    if(node->head.permissionIndex >= server->config.rolePermissionsSize)
        return 0xFFFFFFFF;
    
    const UA_RolePermissions *rp = &server->config.rolePermissions[node->head.permissionIndex];
    if(!rp->entries || rp->entriesSize == 0)
        return 0xFFFFFFFF;
    
    /* Compute logical OR of permissions for all session roles */
    UA_PermissionType effectivePerms = 0;
    
    for(size_t i = 0; i < rolesSize; i++) {
        for(size_t j = 0; j < rp->entriesSize; j++) {
            if(UA_NodeId_equal(&roles[i], &rp->entries[j].roleId)) {
                effectivePerms |= rp->entries[j].permissions;
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

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    /* Get the node */
    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
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
    
    /* Compute effective permissions */
    *effectivePermissions = computeEffectivePermissions(server, node, rolesSize, roles);
    
    UA_NODESTORE_RELEASE(server, node);

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
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

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    
    /* Get the node */
    const UA_Node *node = UA_NODESTORE_GET(server, nodeId);
    if(!node) {
        res = UA_STATUSCODE_BADNODEIDUNKNOWN;
        goto cleanup;
    }
    
    /* If node has no permission configuration, return empty array */
    if(node->head.permissionIndex == UA_PERMISSION_INDEX_INVALID ||
       node->head.permissionIndex >= server->config.rolePermissionsSize) {
        UA_NODESTORE_RELEASE(server, node);
        goto cleanup;
    }
    
    const UA_RolePermissions *rp = &server->config.rolePermissions[node->head.permissionIndex];
    if(!rp->entries || rp->entriesSize == 0) {
        UA_NODESTORE_RELEASE(server, node);
        goto cleanup;
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
        for(size_t j = 0; j < rp->entriesSize; j++) {
            if(UA_NodeId_equal(&roles[i], &rp->entries[j].roleId)) {
                matchCount++;
                break;
            }
        }
    }
    
    if(matchCount == 0) {
        UA_NODESTORE_RELEASE(server, node);
        goto cleanup;
    }
    
    /* Allocate result array */
    UA_RolePermissionType *result = (UA_RolePermissionType*)
        UA_Array_new(matchCount, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    if(!result) {
        UA_NODESTORE_RELEASE(server, node);
        res = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }
    
    /* Fill result array with permissions for session's roles */
    size_t resultIdx = 0;
    for(size_t i = 0; i < rolesSize && resultIdx < matchCount; i++) {
        for(size_t j = 0; j < rp->entriesSize; j++) {
            if(UA_NodeId_equal(&roles[i], &rp->entries[j].roleId)) {
                res = UA_NodeId_copy(&roles[i], &result[resultIdx].roleId);
                if(res != UA_STATUSCODE_GOOD) {
                    UA_Array_delete(result, resultIdx, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
                    UA_NODESTORE_RELEASE(server, node);
                    goto cleanup;
                }
                result[resultIdx].permissions = rp->entries[j].permissions;
                resultIdx++;
                break;
            }
        }
    }
    
    *entriesSize = matchCount;
    *entries = result;
    
    UA_NODESTORE_RELEASE(server, node);

cleanup:
#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return res;
}

#endif /* UA_ENABLE_RBAC */