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
 */

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
    role->customConfiguration = false;
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
            res = UA_IdentityMappingRuleType_copy(&src->identityMappingRules[i], &dst->identityMappingRules[i]);
            if(res != UA_STATUSCODE_GOOD) {
                UA_Role_clear(dst);
                return res;
            }
        }
    }
    
    dst->applicationsExclude = src->applicationsExclude;
    if(src->applicationsSize > 0) {
        dst->applications = (UA_String*)UA_calloc(src->applicationsSize, sizeof(UA_String));
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
        dst->endpoints = (UA_EndpointType*)UA_calloc(src->endpointsSize, sizeof(UA_EndpointType));
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
    if(r1->identityMappingRulesSize != r2->identityMappingRulesSize)
        return false;
    for(size_t i = 0; i < r1->identityMappingRulesSize; i++) {
        if(!UA_IdentityMappingRuleType_equal(&r1->identityMappingRules[i], &r2->identityMappingRules[i]))
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

/********************************/
/* UA_RolePermissions Type API  */
/********************************/

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
        for(size_t i = 0; i < rp->entriesSize; i++)
            UA_NodeId_clear(&rp->entries[i].roleId);
        UA_free(rp->entries);
    }
    rp->entriesSize = 0;
    rp->entries = NULL;
    rp->refCount = 0;
}

UA_StatusCode UA_EXPORT
UA_RolePermissions_copy(const UA_RolePermissions *src, UA_RolePermissions *dst) {
    if(!src || !dst)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_RolePermissions_init(dst);
    
    if(src->entriesSize > 0 && src->entries) {
        dst->entries = (UA_RolePermissionEntry*)
            UA_calloc(src->entriesSize, sizeof(UA_RolePermissionEntry));
        if(!dst->entries)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dst->entriesSize = src->entriesSize;
        
        for(size_t i = 0; i < src->entriesSize; i++) {
            UA_StatusCode res = UA_NodeId_copy(&src->entries[i].roleId, &dst->entries[i].roleId);
            if(res != UA_STATUSCODE_GOOD) {
                UA_RolePermissions_clear(dst);
                return res;
            }
            dst->entries[i].permissions = src->entries[i].permissions;
        }
    }
    
    return UA_STATUSCODE_GOOD;
}

/**********************/
/* RBAC Init/Cleanup  */
/**********************/

UA_StatusCode
UA_Server_initializeRBAC(UA_Server *server) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    /* RBAC initialization will be implemented in subsequent PRs.
     * This stub ensures the flag is properly defined and the
     * build system is configured correctly. */
    
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "RBAC support enabled (EXPERIMENTAL - not for production use)");
    
    return UA_STATUSCODE_GOOD;
}

void
UA_Server_cleanupRBAC(UA_Server *server) {
    if(!server)
        return;
    
    /* RBAC cleanup will be implemented in subsequent PRs */
}

#endif /* UA_ENABLE_RBAC */
