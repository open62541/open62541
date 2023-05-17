/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *    Copyright 2022 (c) Kalycito Infotech Private Limited
 */

#include "ua_server_internal.h"
#include "ua_session.h"
#include "ua_subscription.h"
#include "ua_server_role_access.h"

#ifdef UA_ENABLE_ROLE_PERMISSION
UA_Boolean checkUserAccess(const UA_Node *node, void *sessionContext, UA_UInt32 permissionBit) {
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
    if ((node->head.userRolePermissionsSize != 0)) {
        for (size_t index = 0; index < node->head.userRolePermissionsSize; index++) {
            if (UA_NodeId_equal(&node->head.userRolePermissions[index].roleId,  \
                                &userAndRoleInfo->accessControlSettings->role.roleId) == true) {
                if ((node->head.userRolePermissions[index].permissions & permissionBit) == permissionBit) {
                    return true;
                }
            }
        }
    }
    else{
        if ((userAndRoleInfo->accessControlSettings->role.permissions & permissionBit) \
            == permissionBit){
            return true;
        }
    }
    return false;
}

UA_NodeId
findRoleIdentityNodeID(UA_Server *server, UA_NodeId startingNode) {
    UA_NodeId resultNodeId;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = UA_QUALIFIEDNAME(0, "Identities");
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = startingNode;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    UA_BrowsePathResult bpr = translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return UA_NODEID_NULL;

    UA_StatusCode res = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &resultNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_BrowsePathResult_clear(&bpr);
        return UA_NODEID_NULL;
    }

    UA_BrowsePathResult_clear(&bpr);
    return resultNodeId;
}

UA_StatusCode
setUserRole_settings(UA_Server* server, UA_String roleName,
                     UA_AccessControlSettings* accessControlSettings) {
    const UA_String anonymous = UA_STRING_STATIC(ANONYMOUS_WELL_KNOWN_RULE);
    const UA_String authenticatedUser = UA_STRING_STATIC(AUTHENTICATEDUSER_WELL_KNOWN_RULE);
    const UA_String configureAdmin = UA_STRING_STATIC(CONFIGUREADMIN_WELL_KNOWN_RULE);
    const UA_String engineer = UA_STRING_STATIC(ENGINEER_WELL_KNOWN_RULE);
    const UA_String observer = UA_STRING_STATIC(OBSERVER_WELL_KNOWN_RULE);
    const UA_String operatorRole = UA_STRING_STATIC(OPERATOR_WELL_KNOWN_RULE);
    const UA_String securityAdmin = UA_STRING_STATIC(SECURITYADMIN_WELL_KNOWN_RULE);
    const UA_String supervisor = UA_STRING_STATIC(SUPERVISOR_WELL_KNOWN_RULE);

    UA_PermissionType setCustomRolePermission = 0x0;
    if (UA_String_equal(&roleName, &anonymous) == true) {
        accessControlSettings->accessControlGroup = UA_ANONYMOUS_WELL_KNOWN_RULE;
        accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ;
        accessControlSettings->methodAccessPermission = true;
        accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);

        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, accessControlSettings);
            accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_ANONYMOUS_WELL_KNOWN_RULE: %X", accessControlSettings->role.permissions);
        }
        else {
            accessControlSettings->role.permissions = 0x27;
        }

        const UA_IdentityMappingRuleType identityMappingRule = {UA_IDENTITYCRITERIATYPE_ANONYMOUS, anonymous};
        UA_IdentityMappingRuleType_copy(&identityMappingRule,
                                        &accessControlSettings->identityMappingRule);
    }

    else if (UA_String_equal(&roleName, &authenticatedUser) == true) {
        accessControlSettings->accessControlGroup = UA_AUTHENTICATEDUSER_WELL_KNOWN_RULE;
        accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        accessControlSettings->methodAccessPermission = true;
        accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, accessControlSettings);
            accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_AUTHENTICATEDUSER_WELL_KNOWN_RULE: %X", accessControlSettings->role.permissions);
        }
        else {
            accessControlSettings->role.permissions = 0x1FFFF;
        }
        const UA_IdentityMappingRuleType identityMappingRule = {UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER, authenticatedUser};
        UA_IdentityMappingRuleType_copy(&identityMappingRule,
                                        &accessControlSettings->identityMappingRule);
    }

    else if (UA_String_equal(&roleName, &configureAdmin) == true) {
        accessControlSettings->accessControlGroup = UA_CONFIGUREADMIN_WELL_KNOWN_RULE;
        accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;
        accessControlSettings->methodAccessPermission = true;
        accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, accessControlSettings);
            accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_CONFIGUREADMIN_WELL_KNOWN_RULE: %X", accessControlSettings->role.permissions);
        }
        else {
            accessControlSettings->role.permissions = 0x1FFFF;
        }
        const UA_IdentityMappingRuleType identityMappingRule = {UA_IDENTITYCRITERIATYPE_ROLE, configureAdmin};
        UA_IdentityMappingRuleType_copy(&identityMappingRule,
                                        &accessControlSettings->identityMappingRule);
    }

    else if (UA_String_equal(&roleName, &engineer) == true) {
        accessControlSettings->accessControlGroup = UA_ENGINEER_WELL_KNOWN_RULE;
        accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        accessControlSettings->methodAccessPermission = true;
        accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, accessControlSettings);
            accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_ENGINEER_WELL_KNOWN_RULE: %X", accessControlSettings->role.permissions);
        }
        else {
            accessControlSettings->role.permissions = 0x1FFFF;
        }
        const UA_IdentityMappingRuleType identityMappingRule = {UA_IDENTITYCRITERIATYPE_ROLE, engineer};
        UA_IdentityMappingRuleType_copy(&identityMappingRule,
                                        &accessControlSettings->identityMappingRule);
    }

    else if (UA_String_equal(&roleName, &observer) == true) {
        accessControlSettings->accessControlGroup = UA_OBSERVER_WELL_KNOWN_RULE;
        accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_HISTORYREAD;
        accessControlSettings->methodAccessPermission = false;
        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, accessControlSettings);
            accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_OBSERVER_WELL_KNOWN_RULE: %X", accessControlSettings->role.permissions);
        }
        else {
            accessControlSettings->role.permissions = 0x1FFFF;
        }
        accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
        const UA_IdentityMappingRuleType identityMappingRule = {UA_IDENTITYCRITERIATYPE_ROLE, observer};
        UA_IdentityMappingRuleType_copy(&identityMappingRule,
                                        &accessControlSettings->identityMappingRule);
    }

    else if (UA_String_equal(&roleName, &operatorRole) == true) {
        accessControlSettings->accessControlGroup = UA_OPERATOR_WELL_KNOWN_RULE;
        accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        accessControlSettings->methodAccessPermission = true;
        accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, accessControlSettings);
            accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_OPERATOR_WELL_KNOWN_RULE: %X", accessControlSettings->role.permissions);
        }
        else {
            accessControlSettings->role.permissions = 0x06;
        }
        const UA_IdentityMappingRuleType identityMappingRule = {UA_IDENTITYCRITERIATYPE_ROLE, operatorRole};
        UA_IdentityMappingRuleType_copy(&identityMappingRule,
                                        &accessControlSettings->identityMappingRule);
    }

    else if (UA_String_equal(&roleName, &securityAdmin) == true) {
        accessControlSettings->accessControlGroup = UA_SECURITYADMIN_WELL_KNOWN_RULE;
        accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;
        accessControlSettings->methodAccessPermission = true;
        accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, accessControlSettings);
            accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_SECURITYADMIN_WELL_KNOWN_RULE: %X", accessControlSettings->role.permissions);
        }
        else {
            accessControlSettings->role.permissions = 0x1FFFF;
        }
        const UA_IdentityMappingRuleType identityMappingRule = {UA_IDENTITYCRITERIATYPE_ROLE, securityAdmin};
        UA_IdentityMappingRuleType_copy(&identityMappingRule,
                                        &accessControlSettings->identityMappingRule);
    }

    else if (UA_String_equal(&roleName, &supervisor) == true) {
        accessControlSettings->accessControlGroup = UA_SUPERVISOR_WELL_KNOWN_RULE;
        accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        accessControlSettings->methodAccessPermission = false;
        accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, accessControlSettings);
            accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_SUPERVISOR_WELL_KNOWN_RULE: %X", accessControlSettings->role.permissions);
        }
        else {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Invalid Role provided");
            accessControlSettings->role.permissions = 0x1FFFF;
        }
        const UA_IdentityMappingRuleType identityMappingRule = {UA_IDENTITYCRITERIATYPE_ROLE, supervisor};
        UA_IdentityMappingRuleType_copy(&identityMappingRule,
                                        &accessControlSettings->identityMappingRule);
    }
    else {
        accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        accessControlSettings->methodAccessPermission = true;
        accessControlSettings->role.roleId = UA_NODEID_STRING(0, (char*)roleName.data);
        accessControlSettings->role.permissions = 0x1FFFF;
        const UA_IdentityMappingRuleType identityMappingRule = {UA_IDENTITYCRITERIATYPE_ROLE, roleName};
        UA_IdentityMappingRuleType_copy(&identityMappingRule,
                                        &accessControlSettings->identityMappingRule);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
setRuntimeUserPermission(UA_Server *server, UA_UsernameRoleInfo *userAndRoleInfo) {
    UA_PermissionType setCustomRolePermission = 0x0;
    if (userAndRoleInfo->accessControlSettings->identityMappingRule.criteriaType == UA_IDENTITYCRITERIATYPE_ANONYMOUS) {
        userAndRoleInfo->accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ;
        userAndRoleInfo->accessControlSettings->methodAccessPermission = true;
        userAndRoleInfo->accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, userAndRoleInfo->accessControlSettings);
            userAndRoleInfo->accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_SECURITYADMIN_WELL_KNOWN_RULE: %X", userAndRoleInfo->accessControlSettings->role.permissions);
        }
        else {
            userAndRoleInfo->accessControlSettings->role.permissions = 0x27;
        }

    }
    else if(userAndRoleInfo->accessControlSettings->identityMappingRule.criteriaType == UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER) {
        userAndRoleInfo->accessControlSettings->accessPermissions = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        userAndRoleInfo->accessControlSettings->methodAccessPermission = true;
        userAndRoleInfo->accessControlSettings->role.roleId = UA_NODEID_NUMERIC(0, UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER);
        /* Set the RolePermission for the node */
        if (server->config.accessControl.readUserDefinedRolePermission != NULL) {
            setCustomRolePermission = server->config.accessControl.readUserDefinedRolePermission(server, userAndRoleInfo->accessControlSettings);
            userAndRoleInfo->accessControlSettings->role.permissions = setCustomRolePermission;
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Provided userRolePermision UA_SECURITYADMIN_WELL_KNOWN_RULE: %X", userAndRoleInfo->accessControlSettings->role.permissions);
        }
        else {
            userAndRoleInfo->accessControlSettings->role.permissions = 0x1FFFF;
        }
    }
    else if(userAndRoleInfo->accessControlSettings->identityMappingRule.criteriaType == UA_IDENTITYCRITERIATYPE_ROLE) {
        UA_String roleName = userAndRoleInfo->accessControlSettings->identityMappingRule.criteria;
        setUserRole_settings(server, roleName, userAndRoleInfo->accessControlSettings);
    }
    else {
        /* Only 3 IdentityMappingRuleType is defined */
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addIdentityActionForWellKonwnRules(UA_Server *server,
                                             const UA_NodeId *sessionId, void *sessionHandle,
                                             const UA_NodeId *methodId, void *methodContext,
                                             const UA_NodeId *objectId, void *objectContext,
                                             size_t inputSize, const UA_Variant *input,
                                             size_t outputSize, UA_Variant *output) {
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionHandle;
    UA_IdentityMappingRuleType *rule = (UA_IdentityMappingRuleType *)input[0].data;
    UA_IdentityMappingRuleType_copy(rule,
                                    &userAndRoleInfo->accessControlSettings->identityMappingRule);
    setRuntimeUserPermission(server, userAndRoleInfo);

/*
    // Find IdentityNode and provide the value
    // Handle multiple client sessions
    UA_NodeId identityNode = findRoleIdentityNodeID(server, *objectId);
    UA_Variant value;
    UA_Variant_setArray(&value, rule, 1, &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]);

    UA_StatusCode retVal = UA_Server_writeValue(server, identityNode, value);
    if (retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Error in writing the RoleIdentity: %s",
                     UA_StatusCode_name(retVal));
    }
*/
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeIdentityActionForWellKonwnRules(UA_Server *server,
                                                const UA_NodeId *sessionId, void *sessionHandle,
                                                const UA_NodeId *methodId, void *methodContext,
                                                const UA_NodeId *objectId, void *objectContext,
                                                size_t inputSize, const UA_Variant *input,
                                                size_t outputSize, UA_Variant *output) {

    /* Find IdentityNode and provide the value */
    UA_NodeId identityNode = findRoleIdentityNodeID(server, *objectId);
    UA_Variant value;
    UA_StatusCode retVal = UA_Server_readValue(server, identityNode, &value);
    if (retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Error in Reading the RoleIdentity: %s",
                     UA_StatusCode_name(retVal));
    }

    UA_IdentityMappingRuleType *inputrule = (UA_IdentityMappingRuleType *)input[0].data;
    UA_IdentityMappingRuleType *ruleAlreadyAvailable = (UA_IdentityMappingRuleType *)value.data;
    if (((ruleAlreadyAvailable->criteriaType) == (inputrule->criteriaType)) && \
        (UA_String_equal(&ruleAlreadyAvailable->criteria, &inputrule->criteria) == true)) {
           UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionHandle;
           userAndRoleInfo->accessControlSettings->accessPermissions = 0;
           userAndRoleInfo->accessControlSettings->methodAccessPermission = false;
           UA_Variant emptyValue;
           UA_IdentityMappingRuleType *rule = NULL;
           UA_Variant_setArray(&emptyValue, rule, 0, &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]);
           retVal = UA_Server_writeValue(server, identityNode, emptyValue);
           if (retVal != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                             "Error in removing the IdentityMappingRule to the IdentityNode: %s",
                             UA_StatusCode_name(retVal));
           }
    }
    else {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "IdentityMappingRuleType is not matched");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addRoleAction(UA_Server *server,
                        const UA_NodeId *sessionId, void *sessionHandle,
                        const UA_NodeId *methodId, void *methodContext,
                        const UA_NodeId *objectId, void *objectContext,
                        size_t inputSize, const UA_Variant *input,
                        size_t outputSize, UA_Variant *output) {
    UA_NodeId outNewNodeId;
    size_t    namespaceIndex;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    UA_String *roleName = (UA_String*)input[0].data;
    UA_String *namespaceURI = (UA_String*)input[1].data;
    attr.description = UA_LOCALIZEDTEXT("en-US", (char *)roleName->data);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", (char *)roleName->data);
    UA_StatusCode retVal = UA_Server_getNamespaceByName(server, *namespaceURI, &namespaceIndex);
    if (retVal != UA_STATUSCODE_GOOD)
       return UA_STATUSCODE_BADINTERNALERROR;

    UA_Server_addObjectNode(server, UA_NODEID_STRING((UA_UInt16)namespaceIndex,(char*)roleName->data),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME((UA_UInt16)namespaceIndex, (char *)roleName->data), UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE),
                            attr, NULL, &outNewNodeId);
    UA_Variant_setScalarCopy(output, &outNewNodeId, &UA_TYPES[UA_TYPES_NODEID]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeRoleAction(UA_Server *server,
                           const UA_NodeId *sessionId, void *sessionHandle,
                           const UA_NodeId *methodId, void *methodContext,
                           const UA_NodeId *objectId, void *objectContext,
                           size_t inputSize, const UA_Variant *input,
                           size_t outputSize, UA_Variant *output) {
    UA_NodeId *roleNodeId = (UA_NodeId *)input[0].data;
    UA_Server_deleteNode(server, *roleNodeId, true);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addRole(UA_Server *server, UA_NodeId parentNodeId, UA_NodeId targetNodeId,
                  UA_ObjectAttributes attr, UA_QualifiedName browseName) {
    UA_NodeId mrNodeId = targetNodeId;
    const UA_Node *mrnode = UA_NODESTORE_GET(server, &mrNodeId);
    if (mrnode != NULL)
        deleteNode(server, targetNodeId, true);

    UA_NODESTORE_RELEASE(server, mrnode);
    addNode(server, UA_NODECLASS_OBJECT, targetNodeId,
            parentNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            browseName,
            UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE),
            &attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);

    UA_String anonymous = UA_STRING_STATIC("Anonymous");
    UA_String authenticatedUser = UA_STRING_STATIC("AuthenticatedUser");

    /* Find the Identity Node ID */
    UA_NodeId identityNode = findRoleIdentityNodeID(server, targetNodeId);

    /* Fill the IdentityMappingRule -> criteriaType and criteria as required */
    UA_IdentityMappingRuleType identityMappingRule;
    identityMappingRule.criteria = browseName.name;

    /* Set the IdentityMappingRule -> criteriaType based on the Roles */
    if (UA_String_equal(&browseName.name, &anonymous) == true)
        identityMappingRule.criteriaType = UA_IDENTITYCRITERIATYPE_ANONYMOUS;
    else if(UA_String_equal(&browseName.name, &authenticatedUser) == true)
        identityMappingRule.criteriaType = UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER;
    else
        identityMappingRule.criteriaType = UA_IDENTITYCRITERIATYPE_ROLE;

    /* Write the IdentityMappingRule value to the Identity Node */
    UA_Variant value;
    UA_Variant_setArray(&value, &identityMappingRule, 1, &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]);

    UA_StatusCode retVal = writeValueAttribute(server, identityNode, &value);
    if (retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Error in writing the IdentityMappingRule to the IdentityNode: %s",
                     UA_StatusCode_name(retVal));
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeRole(UA_Server *server, UA_NodeId targetNodeId) {
    deleteNode(server, targetNodeId, true);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_AddIdentity_method(UA_Server *server, UA_NodeId requested_node_id, UA_NodeId parent_node_id) {

    UA_MethodAttributes method_attr = UA_MethodAttributes_default;
    method_attr.executable = true;
    /* Method with Input Arguments */
    UA_Argument inputArguments;
    UA_Argument_init(&inputArguments);
    inputArguments.dataType = UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en-US", "The rule to add");
    inputArguments.name = UA_STRING("Rule");
    inputArguments.valueRank = UA_VALUERANK_SCALAR; /* scalar argument */
    addMethodNode(server, requested_node_id , parent_node_id,
                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(0, "AddIdentity"),
                  &method_attr, NULL, 1, &inputArguments, UA_NODEID_NULL, NULL, 0, NULL, UA_NODEID_NULL, NULL, NULL, NULL);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_RemoveIdentity_method(UA_Server *server, UA_NodeId requested_node_id, UA_NodeId parent_node_id) {

    UA_MethodAttributes method_attr = UA_MethodAttributes_default;
    method_attr.executable = true;
    /* Method with Input Arguments */
    UA_Argument inputArguments;
    UA_Argument_init(&inputArguments);
    inputArguments.dataType = UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en-US", "The rule to remove");
    inputArguments.name = UA_STRING("Rule");
    inputArguments.valueRank = UA_VALUERANK_SCALAR; /* scalar argument */

    addMethodNode(server, requested_node_id , parent_node_id,
                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(0, "RemoveIdentity"),
                  &method_attr, NULL, 1, &inputArguments, UA_NODEID_NULL, NULL, 0, NULL, UA_NODEID_NULL, NULL, NULL, NULL);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setDefaultRoles(UA_Server *server) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "RoleSet");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "RoleSet");
    deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET), true);
    UA_NodeId outNewNodeId;
    addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET),
                   UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES),  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                   UA_QUALIFIEDNAME(0, "RoleSet"),
                   UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE),
                   &attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, &outNewNodeId);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_ADDROLE),
                           UA_Server_addRoleAction);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_REMOVEROLE),
                           UA_Server_removeRoleAction);

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Anonymous");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Anonymous");
    UA_Server_addRole(server, outNewNodeId,
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS), attr, UA_QUALIFIEDNAME(0, "Anonymous"));
    UA_Server_AddIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS_ADDIDENTITY),
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS));
    UA_Server_RemoveIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS_REMOVEIDENTITY),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS));
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS_ADDIDENTITY),
                                    UA_Server_addIdentityActionForWellKonwnRules);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS_REMOVEIDENTITY),
                                    UA_Server_removeIdentityActionForWellKonwnRules);

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "AuthenticatedUser");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "AuthenticatedUser");
    UA_Server_addRole(server, outNewNodeId,
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER), attr, UA_QUALIFIEDNAME(0, "AuthenticatedUser"));
    UA_Server_AddIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER_ADDIDENTITY),
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER));
    UA_Server_RemoveIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER_REMOVEIDENTITY),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER));
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER_ADDIDENTITY),
                                    UA_Server_addIdentityActionForWellKonwnRules);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER_REMOVEIDENTITY),
                                    UA_Server_removeIdentityActionForWellKonwnRules);

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "ConfigureDomain");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "ConfigureDomain");
    UA_Server_addRole(server, outNewNodeId,
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN), attr, UA_QUALIFIEDNAME(0, "ConfigureDomain"));
    UA_Server_AddIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN_ADDIDENTITY),
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN));
    UA_Server_RemoveIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN_REMOVEIDENTITY),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN));
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN_ADDIDENTITY),
                                    UA_Server_addIdentityActionForWellKonwnRules);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN_REMOVEIDENTITY),
                                    UA_Server_removeIdentityActionForWellKonwnRules);

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Engineer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Engineer");
    UA_Server_addRole(server, outNewNodeId,
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER), attr, UA_QUALIFIEDNAME(0, "Engineer"));
    UA_Server_AddIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER_ADDIDENTITY),
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER));
    UA_Server_RemoveIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER_REMOVEIDENTITY),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER));
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER_ADDIDENTITY),
                                    UA_Server_addIdentityActionForWellKonwnRules);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER_REMOVEIDENTITY),
                                    UA_Server_removeIdentityActionForWellKonwnRules);

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Observer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Observer");
    UA_Server_addRole(server, outNewNodeId,
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER), attr, UA_QUALIFIEDNAME(0, "Observer"));
    UA_Server_AddIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER_ADDIDENTITY),
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER));
    UA_Server_RemoveIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER_REMOVEIDENTITY),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER));
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER_ADDIDENTITY),
                                    UA_Server_addIdentityActionForWellKonwnRules);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER_REMOVEIDENTITY),
                                    UA_Server_removeIdentityActionForWellKonwnRules);

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Operator");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Operator");
    UA_Server_addRole(server, outNewNodeId,
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR), attr, UA_QUALIFIEDNAME(0, "Operator"));
    UA_Server_AddIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR_ADDIDENTITY),
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR));
    UA_Server_RemoveIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR_REMOVEIDENTITY),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR));
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR_ADDIDENTITY),
                                    UA_Server_addIdentityActionForWellKonwnRules);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR_REMOVEIDENTITY),
                                    UA_Server_removeIdentityActionForWellKonwnRules);

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "SecurityAdmin");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "SecurityAdmin");
    UA_Server_addRole(server, outNewNodeId,
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN), attr, UA_QUALIFIEDNAME(0, "SecurityAdmin"));
    UA_Server_AddIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN_ADDIDENTITY),
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN));
    UA_Server_RemoveIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN_REMOVEIDENTITY),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN));
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN_ADDIDENTITY),
                                    UA_Server_addIdentityActionForWellKonwnRules);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN_REMOVEIDENTITY),
                                    UA_Server_removeIdentityActionForWellKonwnRules);

    attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Supervisor");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Supervisor");
    UA_Server_addRole(server, outNewNodeId,
                      UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR), attr, UA_QUALIFIEDNAME(0, "Supervisor"));
    UA_Server_AddIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR_ADDIDENTITY),
                                 UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR));
    UA_Server_RemoveIdentity_method(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR_REMOVEIDENTITY),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR));
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR_ADDIDENTITY),
                                    UA_Server_addIdentityActionForWellKonwnRules);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR_REMOVEIDENTITY),
                                    UA_Server_removeIdentityActionForWellKonwnRules);

    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDIDENTITY),
                                    UA_Server_addIdentityActionForWellKonwnRules);
    setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEIDENTITY),
                                    UA_Server_removeIdentityActionForWellKonwnRules);

    return UA_STATUSCODE_GOOD;

}
#endif