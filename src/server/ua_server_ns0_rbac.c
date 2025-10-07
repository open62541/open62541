/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 */


#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/accesscontrol.h>
#include "open62541/namespace0_generated.h"
#include "open62541/nodeids.h"

#ifdef UA_ENABLE_RBAC

/* OPC UA Part 18 Role-Based Access Control Implementation
 * 
 * Reference: OPC 10000-18: UA Part 18: Role-Based Security
 * https://reference.opcfoundation.org/Core/Part18/v105/docs/
 * 
 * This file implements the RBAC method callbacks for the OPC UA namespace 0:
 * - Role Management: AddRole, RemoveRole methods using UA_Server_addRole/removeRole API
 * - Identity Management: AddIdentity, RemoveIdentity methods using UA_Server_addRoleIdentity/removeRoleIdentity API
 * - Application Management: AddApplication, RemoveApplication methods using UA_Server_addRoleApplication/removeRoleApplication API
 * - Endpoint Management: AddEndpoint, RemoveEndpoint methods using UA_Server_addRoleEndpoint/removeRoleEndpoint API
 */

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
    
    if(input[0].type != &UA_TYPES[UA_TYPES_STRING] ||
       input[1].type != &UA_TYPES[UA_TYPES_STRING]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddRole: Invalid parameter types");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_String *roleName = (UA_String*)input[0].data;
    UA_String *namespaceUri = (UA_String*)input[1].data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddRole: RoleName=%.*s, NamespaceUri=%.*s",
                (int)roleName->length, roleName->data,
                (int)namespaceUri->length, namespaceUri->data);
    
    UA_Role role;
    UA_Role_init(&role);
    role.customConfiguration = true;
    
    UA_QualifiedName_init(&role.roleName);
    UA_String_copy(roleName, &role.roleName.name);
    
    UA_NodeId newRoleId = UA_NODEID_NULL;
    UA_StatusCode retval = UA_Server_addRole(server, *roleName, *namespaceUri, &role, &newRoleId);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddRole: Failed to add role, status: %s", UA_StatusCode_name(retval));
        UA_Role_clear(&role);
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddRole: Successfully added role with NodeId: %s",
                UA_NodeId_print(&newRoleId, NULL));
    
    /* Set output parameter with the new role NodeId */
    if(outputSize >= 1) {
        UA_Variant_setScalarCopy(&output[0], &newRoleId, &UA_TYPES[UA_TYPES_NODEID]);
    }
    
    UA_Role_clear(&role);
    UA_NodeId_clear(&newRoleId);
    
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
    
    if(input[0].type != &UA_TYPES[UA_TYPES_NODEID]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveRole: Invalid parameter type, expected NodeId");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_NodeId *roleId = (UA_NodeId*)input[0].data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveRole: RoleId=%s",
                UA_NodeId_print(roleId, NULL));
    
    UA_StatusCode retval = UA_Server_removeRole(server, *roleId);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveRole: Failed to remove role, status: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveRole: Successfully removed role with NodeId: %s",
                UA_NodeId_print(roleId, NULL));
    
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
    
    if(input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddIdentity: Invalid parameter type, expected IdentityMappingRuleType");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    
    if(!extObj->content.decoded.data || 
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddIdentity: Invalid IdentityMappingRuleType data");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_IdentityMappingRuleType *rule = (UA_IdentityMappingRuleType*)extObj->content.decoded.data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddIdentity: Rule CriteriaType=%u, Criteria=%.*s",
                rule->criteriaType, (int)rule->criteria.length, rule->criteria.data);
    
    UA_StatusCode retval = UA_Server_addRoleIdentity(server, *objectId, rule->criteriaType);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddIdentity: Failed to add identity, status: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddIdentity: Successfully added identity criteria");
    
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
    
    if(input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveIdentity: Invalid parameter type, expected IdentityMappingRuleType");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    
    if(!extObj->content.decoded.data || 
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveIdentity: Invalid IdentityMappingRuleType data");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_IdentityMappingRuleType *rule = (UA_IdentityMappingRuleType*)extObj->content.decoded.data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveIdentity: Rule CriteriaType=%u, Criteria=%.*s",
                rule->criteriaType, (int)rule->criteria.length, rule->criteria.data);
    
    UA_StatusCode retval = UA_Server_removeRoleIdentity(server, *objectId, rule->criteriaType);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveIdentity: Failed to remove identity, status: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveIdentity: Successfully removed identity criteria");
    
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
    
    if(input[0].type != &UA_TYPES[UA_TYPES_STRING]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddApplication: Invalid parameter type, expected String");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_String *applicationUri = (UA_String*)input[0].data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddApplication: ApplicationUri=%.*s",
                (int)applicationUri->length, applicationUri->data);
    
    UA_StatusCode retval = UA_Server_addRoleApplication(server, *objectId, *applicationUri);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddApplication: Failed to add application, status: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddApplication: Successfully added application");
    
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
    
    if(input[0].type != &UA_TYPES[UA_TYPES_STRING]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveApplication: Invalid parameter type, expected String");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_String *applicationUri = (UA_String*)input[0].data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveApplication: ApplicationUri=%.*s",
                (int)applicationUri->length, applicationUri->data);
    
    UA_StatusCode retval = UA_Server_removeRoleApplication(server, *objectId, *applicationUri);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveApplication: Failed to remove application, status: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveApplication: Successfully removed application");
    
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
    
    if(input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddEndpoint: Invalid parameter type, expected EndpointType");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    
    if(!extObj->content.decoded.data || 
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_ENDPOINTTYPE]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddEndpoint: Invalid EndpointType data");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_EndpointType *endpoint = (UA_EndpointType*)extObj->content.decoded.data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddEndpoint: EndpointUrl=%.*s",
                (int)endpoint->endpointUrl.length, endpoint->endpointUrl.data);
    
    UA_StatusCode retval = UA_Server_addRoleEndpoint(server, *objectId, *endpoint);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "AddEndpoint: Failed to add endpoint, status: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "AddEndpoint: Successfully added endpoint");
    
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
    
    if(input[0].type != &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveEndpoint: Invalid parameter type, expected EndpointType");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_ExtensionObject *extObj = (UA_ExtensionObject*)input[0].data;
    
    if(!extObj->content.decoded.data || 
       extObj->content.decoded.type != &UA_TYPES[UA_TYPES_ENDPOINTTYPE]) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveEndpoint: Invalid EndpointType data");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_EndpointType *endpoint = (UA_EndpointType*)extObj->content.decoded.data;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveEndpoint: EndpointUrl=%.*s",
                (int)endpoint->endpointUrl.length, endpoint->endpointUrl.data);
    
    UA_StatusCode retval = UA_Server_removeRoleEndpoint(server, *objectId, *endpoint);
    
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "RemoveEndpoint: Failed to remove endpoint, status: %s", UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RemoveEndpoint: Successfully removed endpoint");
    
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
initNS0RBAC(UA_Server *server) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_ADDROLE), addRoleMethodCallback);
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_REMOVEROLE), removeRoleMethodCallback);

    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDIDENTITY), addIdentityMethodCallback);
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEIDENTITY), removeIdentityMethodCallback);

    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDAPPLICATION), addApplicationMethodCallback);
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEAPPLICATION), removeApplicationMethodCallback);

    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDENDPOINT), addEndpointMethodCallback);
    retval |= UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEENDPOINT), removeEndpointMethodCallback);

    return retval;
}

#endif /* UA_ENABLE_RBAC */
