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

UA_StatusCode
initNS0RBAC(UA_Server *server) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    /* Initialize RBAC information model nodes 
     * According to OPC UA Part 18 specification, the RoleSet Object should be
     * present under Server.ServerCapabilities.RoleSet (i=15606) with the following
     * well-known roles:
     * - Anonymous (i=15644)
     * - AuthenticatedUser (i=15656) 
     * - Observer (i=15668)
     * - Operator (i=15680)
     * - Engineer (i=16036)
     * - Supervisor (i=15692)
     * - ConfigureAdmin (i=15716)
     * - SecurityAdmin (i=15704)
     * - SecurityKeyServerAdmin (i=25565)
     * - SecurityKeyServerAccess (i=25603)
     * - SecurityKeyServerPush (i=25584)
     */
    
    /* Add the RoleSet Object to Server.ServerCapabilities */
    UA_ObjectAttributes roleSetAttr = UA_ObjectAttributes_default;
    roleSetAttr.displayName = UA_LOCALIZEDTEXT("", "RoleSet");
    roleSetAttr.description = UA_LOCALIZEDTEXT("", "Contains the roles supported by the server");

    retval = UA_Server_addObjectNode(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET),  // RoleSet Object ID
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),   // ServerCapabilities parent
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),     // HasComponent reference
        UA_QUALIFIEDNAME(0, "RoleSet"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE),  // RoleSetType
        roleSetAttr,
        NULL, NULL);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Failed to add RoleSet Object: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RoleSet Object added successfully to Server.ServerCapabilities");
    
    retval = UA_Server_setMethodNode_callback(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_ADDROLE), 
        addRoleMethodCallback);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Failed to set AddRole method callback: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    retval = UA_Server_setMethodNode_callback(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_REMOVEROLE),
        removeRoleMethodCallback);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Failed to set RemoveRole method callback: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RBAC method callbacks (AddRole, RemoveRole) attached successfully");
    
    /* TODO: Implement other RBAC method callbacks */
    /* - AddUser, ModifyUser, RemoveUser methods */
    /* - ChangePassword method */
    /* - AddIdentity, RemoveIdentity methods */
    /* - AddApplication, RemoveApplication methods */
    /* - AddEndpoint, RemoveEndpoint methods */
    
    return retval;
}

#endif /* UA_ENABLE_RBAC */
