/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 */


#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/namespace0_generated.h"
#include "open62541/nodeids.h"

#ifdef UA_ENABLE_RBAC

/* OPC UA Part 18 Role-Based Access Control Implementation
 * 
 * Reference: OPC 10000-18: UA Part 18: Role-Based Security
 * https://reference.opcfoundation.org/Core/Part18/v105/docs/
 */

/* TODO: Implement RBAC information model nodes */
/* TODO: Implement user management methods */
/* TODO: Implement identity mapping rules */
/* TODO: Implement endpoint role assignments */

/* Forward declarations */
UA_StatusCode initNS0RBAC(UA_Server *server);

static UA_StatusCode
addRoleMethodCallback(UA_Server *server,
                      const UA_NodeId *objectId, void *objectContext,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *inputType, void *inputContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output) {
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddRole method called");
    
    if(inputSize != 2) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddRole: Expected 2 input parameters, got %zu", inputSize);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_NodeId *roleId = (UA_NodeId*)input[0].data;
    UA_String *roleName = (UA_String*)input[1].data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddRole: RoleId=%s, RoleName=%.*s",
                UA_NodeId_print(roleId, NULL),
                (int)roleName->length, roleName->data);
    
    /* TODO: Implement actual role addition logic */
    /* - Validate roleId and roleName */
    /* - Check if role already exists */
    /* - Add role to internal role management */
    /* - Add role object to RoleSet */
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeRoleMethodCallback(UA_Server *server,
                         const UA_NodeId *objectId, void *objectContext,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *inputType, void *inputContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveRole method called");
    
    if(inputSize != 1) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveRole: Expected 1 input parameter, got %zu", inputSize);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_NodeId *roleId = (UA_NodeId*)input[0].data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveRole: RoleId=%s",
                UA_NodeId_print(roleId, NULL));
    
    /* TODO: Implement actual role removal logic */
    /* - Validate roleId */
    /* - Check if role exists */
    /* - Check if role is in use by any users */
    /* - Remove role from internal role management */
    /* - Remove role object from RoleSet */
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addIdentityMethodCallback(UA_Server *server,
                         const UA_NodeId *objectId, void *objectContext,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *inputType, void *inputContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddIdentity method called");
    
    if(inputSize != 1) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddIdentity: Expected 1 input parameter, got %zu", inputSize);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeIdentityMethodCallback(UA_Server *server,
                            const UA_NodeId *objectId, void *objectContext,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *inputType, void *inputContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveIdentity method called");
    
    if(inputSize != 1) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveIdentity: Expected 1 input parameter, got %zu", inputSize);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addApplicationMethodCallback(UA_Server *server,
                            const UA_NodeId *objectId, void *objectContext,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *inputType, void *inputContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddApplication method called");
    
    if(inputSize != 1) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddApplication: Expected 1 input parameter, got %zu", inputSize);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeApplicationMethodCallback(UA_Server *server,
                               const UA_NodeId *objectId, void *objectContext,
                               const UA_NodeId *methodId, void *methodContext,
                               const UA_NodeId *inputType, void *inputContext,
                               size_t inputSize, const UA_Variant *input,
                               size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveApplication method called");
    
    if(inputSize != 1) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveApplication: Expected 1 input parameter, got %zu", inputSize);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addEndpointMethodCallback(UA_Server *server,
                         const UA_NodeId *objectId, void *objectContext,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *inputType, void *inputContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddEndpoint method called");
    
    if(inputSize != 1) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddEndpoint: Expected 1 input parameter, got %zu", inputSize);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeEndpointMethodCallback(UA_Server *server,
                            const UA_NodeId *objectId, void *objectContext,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *inputType, void *inputContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveEndpoint method called");
    
    if(inputSize != 1) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveEndpoint: Expected 1 input parameter, got %zu", inputSize);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
initNS0RBAC(UA_Server *server) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RoleSet Object and well-known roles are available through namespace0_generated");
    
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_ADDROLE), addRoleMethodCallback);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_REMOVEROLE), removeRoleMethodCallback);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDIDENTITY), addIdentityMethodCallback);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEIDENTITY), removeIdentityMethodCallback);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDAPPLICATION), addApplicationMethodCallback);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEAPPLICATION), removeApplicationMethodCallback);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDENDPOINT), addEndpointMethodCallback);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEENDPOINT), removeEndpointMethodCallback);

    return retval;
}

#endif /* UA_ENABLE_RBAC */
