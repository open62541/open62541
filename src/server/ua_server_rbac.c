/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Andreas Ebner
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_RBAC

/* UA_Role Type functions */

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
    
    /* Observer Role */
    UA_Role observerRole;
    UA_Role_init(&observerRole);
    observerRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    observerRole.imrt = (UA_IdentityMappingRuleType*)UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!observerRole.imrt) 
        return UA_STATUSCODE_BADOUTOFMEMORY;
    observerRole.imrtSize = 1;
    UA_IdentityMappingRuleType_init(&observerRole.imrt[0]);
    observerRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    observerRole.imrt[0].criteria = UA_STRING_NULL;
    
    res = UA_Server_addRole(server, UA_STRING("Observer"), UA_STRING_NULL, &observerRole, &roleId);
    UA_Role_clear(&observerRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* Operator Role */
    UA_Role operatorRole;
    UA_Role_init(&operatorRole);
    operatorRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    operatorRole.imrt = (UA_IdentityMappingRuleType*)UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!operatorRole.imrt) 
        return UA_STATUSCODE_BADOUTOFMEMORY;
    operatorRole.imrtSize = 1;
    UA_IdentityMappingRuleType_init(&operatorRole.imrt[0]);
    operatorRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    operatorRole.imrt[0].criteria = UA_STRING_NULL;
    
    res = UA_Server_addRole(server, UA_STRING("Operator"), UA_STRING_NULL, &operatorRole, &roleId);
    UA_Role_clear(&operatorRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* Engineer Role */
    UA_Role engineerRole;
    UA_Role_init(&engineerRole);
    engineerRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    engineerRole.imrt = (UA_IdentityMappingRuleType*)UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!engineerRole.imrt) 
        return UA_STATUSCODE_BADOUTOFMEMORY;
    engineerRole.imrtSize = 1;
    UA_IdentityMappingRuleType_init(&engineerRole.imrt[0]);
    engineerRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    engineerRole.imrt[0].criteria = UA_STRING_NULL;
    
    res = UA_Server_addRole(server, UA_STRING("Engineer"), UA_STRING_NULL, &engineerRole, &roleId);
    UA_Role_clear(&engineerRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* Supervisor Role */
    UA_Role supervisorRole;
    UA_Role_init(&supervisorRole);
    supervisorRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    supervisorRole.imrt = (UA_IdentityMappingRuleType*)UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!supervisorRole.imrt) 
        return UA_STATUSCODE_BADOUTOFMEMORY;
    supervisorRole.imrtSize = 1;
    UA_IdentityMappingRuleType_init(&supervisorRole.imrt[0]);
    supervisorRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    supervisorRole.imrt[0].criteria = UA_STRING_NULL;
    
    res = UA_Server_addRole(server, UA_STRING("Supervisor"), UA_STRING_NULL, &supervisorRole, &roleId);
    UA_Role_clear(&supervisorRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* ConfigureAdmin Role */
    UA_Role configureAdminRole;
    UA_Role_init(&configureAdminRole);
    configureAdminRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    configureAdminRole.imrt = (UA_IdentityMappingRuleType*)UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!configureAdminRole.imrt) 
        return UA_STATUSCODE_BADOUTOFMEMORY;
    configureAdminRole.imrtSize = 1;
    UA_IdentityMappingRuleType_init(&configureAdminRole.imrt[0]);
    configureAdminRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    configureAdminRole.imrt[0].criteria = UA_STRING_NULL;
    
    res = UA_Server_addRole(server, UA_STRING("ConfigureAdmin"), UA_STRING_NULL, &configureAdminRole, &roleId);
    UA_Role_clear(&configureAdminRole);
    if(res != UA_STATUSCODE_GOOD) return res;
    
    /* SecurityAdmin Role */
    UA_Role securityAdminRole;
    UA_Role_init(&securityAdminRole);
    securityAdminRole.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    securityAdminRole.imrt = (UA_IdentityMappingRuleType*)UA_malloc(sizeof(UA_IdentityMappingRuleType));
    if(!securityAdminRole.imrt) 
        return UA_STATUSCODE_BADOUTOFMEMORY;
    securityAdminRole.imrtSize = 1;
    UA_IdentityMappingRuleType_init(&securityAdminRole.imrt[0]);
    securityAdminRole.imrt[0].criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    securityAdminRole.imrt[0].criteria = UA_STRING_NULL;
    
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
        /* TODO Check namespace handling */
        UA_String namespaceUri = UA_STRING_NULL;
        
        res = UA_Server_addRole(server, roleName, namespaceUri, 
                               &server->config.roles[i], &roleId);
        if(res != UA_STATUSCODE_GOOD) {
            return res;
        }
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
    for(size_t i = 0; i < server->rolesSize; i++) {
        if(UA_String_equal(&server->roles[i].roleName.name, roleName)) {
            /* TODO: check namespace URI handling */
            return &server->roles[i];
        }
    }
    return NULL;
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

//TODO: In the information model we handover name  and namespaceUri --> Why no QualifiedName?
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

    /* Check if role name already exists */
    UA_Role *existing = findRoleByName(server, &roleName, &namespaceUri);
    if(existing) {
        res = UA_STATUSCODE_BADNODEIDEXISTS;
        goto cleanup;
    }

    if(role && !UA_NodeId_isNull(&role->roleId)) {
        const UA_Node *existingNode = UA_NODESTORE_GET(server, &role->roleId);
        if(existingNode) {
            UA_NODESTORE_RELEASE(server, existingNode);
            
            /* well-known roles that exist in NS0 */
            if(!isWellKnownRole(&role->roleId)) {
                res = UA_STATUSCODE_BADNODEIDEXISTS;
                goto cleanup;
            }
        }
        targetNodeId = &role->roleId;
    } else {
        /* Generate new NodeId TODO more carefull handling of namespace*/
        generatedNodeId = UA_NODEID_NUMERIC(1, 0);
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

    /* Set namespace index from role NodeId */
    newRole->roleName.namespaceIndex = newRole->roleId.namespaceIndex;

    server->rolesSize++;
    
    res = UA_NodeId_copy(&newRole->roleId, outNewRoleId);
    if(res != UA_STATUSCODE_GOOD) {
        server->rolesSize--; /* Rollback size increment */
        goto cleanup;
    }

cleanup:
    if(res != UA_STATUSCODE_GOOD && newRole) {
        UA_Role_clear(newRole);
    }

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

    /* Find the role */
    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    size_t roleIndex = role - server->roles;
    UA_Role_clear(&server->roles[roleIndex]);

    /* We need to move the remaining roles. Check if a more advanced data structure e.g. List would be better. */
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

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_addRoleApplication(UA_Server *server, UA_NodeId roleId, UA_String uri) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    UA_String *newApps = (UA_String*)
        UA_realloc(role->applications, 
                   (role->applicationsSize + 1) * sizeof(UA_String));
    if(!newApps) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    role->applications = newApps;
    
    UA_StatusCode res = UA_String_copy(&uri, &role->applications[role->applicationsSize]);
    if(res != UA_STATUSCODE_GOOD) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return res;
    }

    role->applicationsSize++;

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_removeRoleApplication(UA_Server *server, UA_NodeId roleId, UA_String uri) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    size_t appIndex = SIZE_MAX;
    for(size_t i = 0; i < role->applicationsSize; i++) {
        if(UA_String_equal(&role->applications[i], &uri)) {
            appIndex = i;
            break;
        }
    }

    if(appIndex == SIZE_MAX) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNOTFOUND;
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

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_addRoleEndpoint(UA_Server *server, UA_NodeId roleId, UA_EndpointType endpoint) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    UA_EndpointType *newEndpoints = (UA_EndpointType*)
        UA_realloc(role->endpoints, 
                   (role->endpointsSize + 1) * sizeof(UA_EndpointType));
    if(!newEndpoints) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    role->endpoints = newEndpoints;
    
    UA_StatusCode res = UA_EndpointType_copy(&endpoint, &role->endpoints[role->endpointsSize]);
    if(res != UA_STATUSCODE_GOOD) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return res;
    }

    role->endpointsSize++;

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_removeRoleEndpoint(UA_Server *server, UA_NodeId roleId, UA_EndpointType endpoint) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    size_t endpointIndex = SIZE_MAX;
    for(size_t i = 0; i < role->endpointsSize; i++) {
        if(UA_EndpointType_equal(&role->endpoints[i], &endpoint)) {
            endpointIndex = i;
            break;
        }
    }

    if(endpointIndex == SIZE_MAX) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNOTFOUND;
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

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_addRoleIdentity(UA_Server *server, UA_NodeId roleId, UA_IdentityCriteriaType ict) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Reallocate identity mapping rules array */
    UA_IdentityMappingRuleType *newImrt = (UA_IdentityMappingRuleType*)
        UA_realloc(role->imrt, 
                   (role->imrtSize + 1) * sizeof(UA_IdentityMappingRuleType));
    if(!newImrt) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    role->imrt = newImrt;
    
    /* Initialize and set identity criteria */
    UA_IdentityMappingRuleType_init(&role->imrt[role->imrtSize]);
    role->imrt[role->imrtSize].criteriaType = ict;
    role->imrt[role->imrtSize].criteria = UA_STRING_NULL;

    role->imrtSize++;

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_removeRoleIdentity(UA_Server *server, UA_NodeId roleId, UA_IdentityCriteriaType ict) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

#if UA_MULTITHREADING >= 100
    UA_LOCK(&server->serviceMutex);
#endif

    UA_Role *role = findRoleById(server, &roleId);
    if(!role) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    size_t imrtIndex = SIZE_MAX;
    for(size_t i = 0; i < role->imrtSize; i++) {
        if(role->imrt[i].criteriaType == ict) {
            imrtIndex = i;
            break;
        }
    }

    if(imrtIndex == SIZE_MAX) {
#if UA_MULTITHREADING >= 100
        UA_UNLOCK(&server->serviceMutex);
#endif
        return UA_STATUSCODE_BADNOTFOUND;
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

#if UA_MULTITHREADING >= 100
    UA_UNLOCK(&server->serviceMutex);
#endif

    return UA_STATUSCODE_GOOD;
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
UA_Server_getRolesById(UA_Server *server, UA_NodeId roleId) {
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
UA_Server_getRolesByName(UA_Server *server, UA_String roleName, UA_String namespaceUri) {
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

#endif /* UA_ENABLE_RBAC */