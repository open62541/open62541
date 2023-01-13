/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Adding Methods to Objects and check user database
 * -------------------------------------------------
 * This example will check login user is available in user database
 */
#include <open62541/plugin/accesscontrol_custom.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// structure for user Data Queue
typedef struct user_data_buffer {
    bool userAvailable;
    UA_String username;
    UA_String password;
    UA_String role;
    uint64_t GroupID;
    uint64_t userID;
} userDatabase;

UA_NodeId outNewNodeId;

int           checkUser (userDatabase* userData);
UA_Boolean    checkUserDatabase(const UA_UserNameIdentityToken *userToken, UA_String *roleName);
UA_PermissionType readUserProvidedRoles(UA_Server *server, UA_AccessControlSettings* accessControlSettings);
UA_StatusCode addNewNamespaceandSetDefaultPermission(UA_Server *server);
UA_StatusCode createNewRoleMethodCall(UA_Server *server);
UA_StatusCode checkTheRoleSessionLoggedIn(UA_Server *server);
UA_NodeId     findIdentityNodeID(UA_Server *server, UA_NodeId startingNode);
UA_StatusCode addNewTestNode(UA_Server *server);

UA_NodeId
findIdentityNodeID(UA_Server *server, UA_NodeId startingNode) {
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
    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return UA_NODEID_NULL;

    UA_StatusCode res = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &resultNodeId);
    if(res != UA_STATUSCODE_GOOD){
        UA_BrowsePathResult_clear(&bpr);
        return UA_NODEID_NULL;
    }

    UA_BrowsePathResult_clear(&bpr);
    return resultNodeId;
}

static UA_Boolean
addRoleBasedNodes(UA_Server *server,
                  const UA_NodeId *sessionId,
                  void *sessionContext,
                  const UA_NodeId *sourceNodeId,
                  const UA_NodeId *targetParentNodeId,
                  const UA_NodeId *referenceTypeId)
 {
    UA_NodeId addIdentityNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_ADDIDENTITY);
    UA_NodeId removeIdentityNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLETYPE_REMOVEIDENTITY);

    //  Add the AddIdentity Method and RemoveIdentity Method, when creating the RoleType Object
    if ((UA_NodeId_equal(sourceNodeId, &addIdentityNodeId)) || (UA_NodeId_equal(sourceNodeId, &removeIdentityNodeId)))
        return UA_TRUE;
    else
        return UA_FALSE;
}

static UA_StatusCode
roleIdentificationMethodCallback(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *methodId, void *methodContext,
                                 const UA_NodeId *objectId, void *objectContext,
                                 size_t inputSize, const UA_Variant *input,
                                 size_t outputSize, UA_Variant *output) {
    if (sessionContext == NULL) {
        UA_String userRoleInfo = UA_STRING("Anonymous");
        UA_Variant_setScalarCopy(output, &userRoleInfo, &UA_TYPES[UA_TYPES_STRING]);
        return UA_STATUSCODE_GOOD;
    }

#ifdef UA_ENABLE_ROLE_PERMISSION
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SessionUsername: %s", userAndRoleInfo->username->data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "SessionRolename: %s", userAndRoleInfo->rolename->data);
    UA_Variant_setScalarCopy(output, userAndRoleInfo->rolename, &UA_TYPES[UA_TYPES_STRING]);
#endif

    return UA_STATUSCODE_GOOD;
}

static void
addSessionRoleIdentificationMethodCall(UA_Server *server) {
    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    outputArgument.name = UA_STRING("Role");
    outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes roleIdentificationAttr = UA_MethodAttributes_default;
    roleIdentificationAttr.description = UA_LOCALIZEDTEXT("en-US","Identify the Role logged in session");
    roleIdentificationAttr.displayName = UA_LOCALIZEDTEXT("en-US","SessionRoleIdentification");
    roleIdentificationAttr.executable = true;
    roleIdentificationAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "SessionRoleIdentification"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Identify the Role logged in session"),
                            roleIdentificationAttr, &roleIdentificationMethodCallback,
                            0, NULL, 1, &outputArgument, NULL, NULL);
}

UA_StatusCode addNewNamespaceandSetDefaultPermission(UA_Server *server) {
    UA_UInt16 nsIdx = UA_Server_addNamespace(server, "http://yourorganisation.org/test/");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "NameSpaceIndex: %d", nsIdx);
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "http://yourorganisation.org/test/");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "http://yourorganisation.org/test/");
    UA_Server_addObjectNode(server, UA_NODEID_STRING(nsIdx, "http://yourorganisation.org/test/"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACES), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(nsIdx, "http://yourorganisation.org/test/"), UA_NODEID_NUMERIC(0, UA_NS0ID_NAMESPACEMETADATATYPE),
                            attr, NULL, &outNewNodeId);

    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "DefaultAccessRestrictions");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.dataType = UA_TYPES[UA_TYPES_ACCESSRESTRICTIONTYPE].typeId;
    UA_AccessRestrictionType restrictionValue = 15;
    UA_Variant_setScalar(&attributes.value, &restrictionValue, &UA_TYPES[UA_TYPES_ACCESSRESTRICTIONTYPE]);
    UA_Server_addVariableNode(
        server, UA_NODEID_STRING(nsIdx, "DefaultAccessRestrictions"), outNewNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
        UA_QUALIFIEDNAME(nsIdx, "DefaultAccessRestrictions"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "DefaultRolePermissions");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.dataType = UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE].typeId;
    UA_RolePermissionType rolePermission[8];
    rolePermission[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    rolePermission[0].permissions = 0x1FFFF;
    rolePermission[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    rolePermission[1].permissions = 0x1FFFF;
    rolePermission[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    rolePermission[2].permissions = 0x1FFFF;
    rolePermission[3].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    rolePermission[3].permissions = 0x1FFFF;
    rolePermission[4].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    rolePermission[4].permissions = 0x1FFFF;
    rolePermission[5].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    rolePermission[5].permissions = 0x1FFFF;
    rolePermission[6].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    rolePermission[6].permissions = 0x1FFFF;
    rolePermission[7].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    rolePermission[7].permissions = 0x1FFFF;
    UA_Variant_setArray(&attributes.value, rolePermission, 8, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_addVariableNode(server, UA_NODEID_STRING(nsIdx, "DefaultRolePermissions"), outNewNodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(nsIdx, "DefaultRolePermissions"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attributes, NULL, NULL);
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "DefaultUserRolePermissions");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.dataType = UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE].typeId;
    rolePermission[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    rolePermission[0].permissions = 0x23;
    rolePermission[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    rolePermission[1].permissions = 0x1FFFF;
    rolePermission[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    rolePermission[2].permissions = 0x1FFFF;
    rolePermission[3].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    rolePermission[3].permissions = 0x1FFFF;
    rolePermission[4].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    rolePermission[4].permissions = 0x1FFFF;
    rolePermission[5].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    rolePermission[5].permissions = 0x1FFFF;
    rolePermission[6].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    rolePermission[6].permissions = 0x1FFFF;
    rolePermission[7].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    rolePermission[7].permissions = 0x1FFFF;
    UA_Variant_setArray(&attributes.value, rolePermission, 8, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_addVariableNode(server, UA_NODEID_STRING(nsIdx, "DefaultUserRolePermissions"), outNewNodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(nsIdx, "DefaultUserRolePermissions"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attributes, NULL, NULL);

    return UA_STATUSCODE_GOOD;
}

UA_PermissionType readUserProvidedRoles(UA_Server *server, UA_AccessControlSettings* accessControlSettings)
{
    UA_UInt16 nsIdx = UA_Server_addNamespace(server, "http://yourorganisation.org/test/");
    UA_Variant dims;
    UA_Variant_init(&dims);
    UA_Server_readValue(server, UA_NODEID_STRING(nsIdx, "DefaultUserRolePermissions"), &dims);
    UA_RolePermissionType *p = (UA_RolePermissionType *)dims.data;

    switch(accessControlSettings->accessControlGroup)
    {
        case UA_ANONYMOUS_WELL_KNOWN_RULE: {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_ANONYMOUS_WELL_KNOWN_RULE 0x23 - %0X", p[0].permissions);
            return p[0].permissions;
            break;
        }
        case UA_AUTHENTICATEDUSER_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_AUTHENTICATEDUSER_WELL_KNOWN_RULE - %0X", p[1].permissions);
            return p[1].permissions;
            break;
        case UA_CONFIGUREADMIN_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_CONFIGUREADMIN_WELL_KNOWN_RULE - %0X", p[2].permissions);
            return p[2].permissions;
            break;
        case UA_ENGINEER_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_ENGINEER_WELL_KNOWN_RULE - %0X", p[3].permissions);
            return p[3].permissions;
            break;
        case UA_OBSERVER_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_OBSERVER_WELL_KNOWN_RULE - %0X", p[4].permissions);
            return p[4].permissions;
            break;
        case UA_OPERATOR_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_OPERATOR_WELL_KNOWN_RULE - %0X", p[5].permissions);
            return p[5].permissions;
            break;
        case UA_SECURITYADMIN_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_SECURITYADMIN_WELL_KNOWN_RULE - %0X", p[6].permissions);
            return p[6].permissions;
            break;
        case UA_SUPERVISOR_WELL_KNOWN_RULE:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UA_SUPERVISOR_WELL_KNOWN_RULE - %0X", p[7].permissions);
            return p[7].permissions;
            break;
        default:
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "UNKNOWN ROLE USED");
            return 0x0;
    }
    UA_Variant_clear(&dims);
}

/* Note: Uncomment the below codesection to add new Role node via the open62541 server */
/*
UA_StatusCode createNewRoleMethodCall(UA_Server *server) {
    UA_Variant inputArguments[2];
    UA_Variant_init(&inputArguments[0]);
    UA_String newRole = UA_STRING("KalycitoWorker");
    UA_Variant_setScalarCopy(&inputArguments[0], &newRole, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_init(&inputArguments[1]);
    UA_String namespaceUri = UA_STRING("http://yourorganisation.org/test/");
    UA_Variant_setScalarCopy(&inputArguments[1], &namespaceUri, &UA_TYPES[UA_TYPES_STRING]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 2;         // 1 would be correct
    callMethodRequest.inputArguments = (UA_Variant*)&inputArguments;
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_ADDROLE);
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    if (result.statusCode != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "MethodCall Failed to add KalycitoWorker Role Node");

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "OutputArgumentSize: %ld", result.outputArgumentsSize);

    UA_NodeId *roleNodeId = (UA_NodeId *)result.outputArguments[0].data;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RoleNodeId: %s", roleNodeId->identifier.string.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "RoleNodeIdNamespaceIndex: %d", roleNodeId->namespaceIndex);

    // Add IdentityMappingRule to the KalycitoWorker's Identity Node (Child node of the KalycitoWorker Role)
    UA_IdentityMappingRuleType identityMappingRule;
    identityMappingRule.criteria = UA_STRING("Engineer");
    identityMappingRule.criteriaType = UA_IDENTITYCRITERIATYPE_ROLE;

    UA_Variant value;
    UA_Variant_setArray(&value, &identityMappingRule, 1, &UA_TYPES[UA_TYPES_IDENTITYMAPPINGRULETYPE]);

    UA_NodeId identityChildNodeId = findIdentityNodeID(server, *roleNodeId);

    UA_StatusCode ret = UA_Server_writeValue(server, identityChildNodeId, value);
    if (ret != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error in writing the RoleIdentity: %s", UA_StatusCode_name(ret));

    UA_Variant_clear(&inputArguments[0]);
    UA_Variant_clear(&inputArguments[1]);
    callMethodRequest.inputArguments = NULL;
    callMethodRequest.inputArgumentsSize = 0;
    UA_CallMethodRequest_clear(&callMethodRequest);
    UA_CallMethodResult_clear(&result);

    return result.statusCode;
}
*/
UA_StatusCode addNewTestNodeObject(UA_Server *server);

UA_StatusCode addNewTestNodeObject(UA_Server *server) {
    /* Object one */
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Demo_Object");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Demo_Object");
    object_attr.rolePermissionsSize = 2;
    object_attr.rolePermissions = (UA_RolePermissionType *)UA_Array_new(object_attr.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    object_attr.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    object_attr.rolePermissions[0].permissions = 0x1FFFF;
    object_attr.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    object_attr.rolePermissions[1].permissions = 0x1FFFF;
    object_attr.userRolePermissionsSize = 2;
    object_attr.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(object_attr.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    object_attr.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    object_attr.userRolePermissions[0].permissions = 0x1FFFF;
    object_attr.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    object_attr.userRolePermissions[1].permissions = 0x1FFFF;
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 200000),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, "Demo_Object"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), object_attr, NULL, NULL);
    UA_Array_delete(object_attr.rolePermissions, object_attr.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Array_delete(object_attr.userRolePermissions, object_attr.userRolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    /* Variable one */
    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_1");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 2;
    attributes.rolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissionsSize = 2;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x1;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1;
    UA_UInt16 value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2000), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_1"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_2");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x3;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x3;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2002), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_2"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_3");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x7;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.userRolePermissions[1].permissions = 0x7;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2003), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_3"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_4");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x27;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x27;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2004), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_4"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_5");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x67;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x67;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x67;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2005), UA_NODEID_NUMERIC(1, 200000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_5"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode addNewTestNode(UA_Server *server) {
    /* Object one */
    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en-US", "Demo_Variables");
    object_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Demo_Variables");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 100000),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(1, "Demo_Variables"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), object_attr, NULL, NULL);

    /* Variable one */
    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_1");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 2;
    attributes.rolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF; //1ffff
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissionsSize = 2;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x1;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1;
    UA_UInt16 value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1000), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_1"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_2");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x3;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x3;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1002), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_2"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_3");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x7;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x7;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1003), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_3"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_4");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x27;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x27;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1004), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_4"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_5");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x67;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x67;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1005), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_5"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_6");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    attributes.userRolePermissions[0].permissions = 0x867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x867;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1006), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_6"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);


    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_7");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    attributes.userRolePermissions[0].permissions = 0x1867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1867;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1007), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_7"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_8");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    attributes.userRolePermissions[0].permissions = 0x3867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1008), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_8"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_9");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    attributes.userRolePermissions[0].permissions = 0x7867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1009), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_9"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_10");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    attributes.userRolePermissions[0].permissions = 0xF867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1010), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_10"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_11");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    attributes.userRolePermissions[0].permissions = 0x1F867;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x1FFFF;
    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1011), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_11"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    UA_Array_delete(attributes.rolePermissions, attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Array_delete(attributes.userRolePermissions, attributes.userRolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_12");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 3;
    attributes.rolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.rolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.rolePermissions[0].permissions = 0x3;
    attributes.rolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.rolePermissions[1].permissions = 0x1FFFF;
    attributes.rolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    attributes.rolePermissions[2].permissions = 0x1FFFF;

    attributes.userRolePermissionsSize = 3;
    attributes.userRolePermissions = (UA_RolePermissionType *)UA_Array_new(attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    attributes.userRolePermissions[0].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS);
    attributes.userRolePermissions[0].permissions = 0x3;
    attributes.userRolePermissions[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    attributes.userRolePermissions[1].permissions = 0x3;
    attributes.userRolePermissions[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    attributes.userRolePermissions[2].permissions = 0x3;

    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1012), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_12"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    UA_Array_delete(attributes.rolePermissions, attributes.rolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Array_delete(attributes.userRolePermissions, attributes.userRolePermissionsSize, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);

    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode_13");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.rolePermissionsSize = 1;
    attributes.userRolePermissionsSize = 1;
    attributes.rolePermissions = (UA_RolePermissionType *)calloc(attributes.rolePermissionsSize, sizeof(UA_RolePermissionType));
    attributes.userRolePermissions = (UA_RolePermissionType *)calloc(attributes.userRolePermissionsSize, sizeof(UA_RolePermissionType));
    UA_String newRole = UA_STRING("KalycitoWorker");
    attributes.rolePermissions[0].roleId = UA_NODEID_STRING(0, (char *)newRole.data);
    attributes.rolePermissions[0].permissions = 0x1FFFF;
    attributes.userRolePermissions[0].roleId = UA_NODEID_STRING(0, (char *)newRole.data);
    attributes.userRolePermissions[0].permissions = 0x1FFFF;

    value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1013), UA_NODEID_NUMERIC(1, 100000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                              UA_QUALIFIEDNAME(1, "TestRoleNode_13"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);
    UA_free(attributes.rolePermissions);
    UA_free(attributes.userRolePermissions);

    return UA_STATUSCODE_GOOD;
}

UA_Boolean checkUserDatabase(const UA_UserNameIdentityToken *userToken, UA_String *roleName) {
    userDatabase userData;
    userData.userAvailable = false;
    UA_String_copy(&userToken->userName, &userData.username);
    UA_String_copy(&userToken->password, &userData.password);
    checkUser(&userData);
    if (userData.userAvailable != true)
       return false;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PlatformUseravailale: %d", userData.userAvailable);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PlatformUserName: %s", userData.username.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PlatformPassword: %s", userData.password.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PlatformRole: %s", userData.role.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PlatformLength: %ld", userData.role.length);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PlatformGroupID: %ld", userData.GroupID);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PlatformUserID: %ld", userData.userID);
    UA_String_copy(&userData.role, roleName);

    if (userData.username.data != NULL)
        free(userData.username.data);

    if (userData.password.data != NULL)
        free(userData.password.data );

    if (userData.password.data != NULL)
        free(userData.role.data );

    return true;
}

/* It follows the main server code, making use of the above definitions. */
static volatile UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    config->nodeLifecycle.createOptionalChild = addRoleBasedNodes;

    addNewNamespaceandSetDefaultPermission(server);
    addSessionRoleIdentificationMethodCall(server);
    // Uncomment to add new Role Node via the open62541 server
    // createNewRoleMethodCall(server);

    addNewTestNode(server);
    addNewTestNodeObject(server);

    config->accessControl.checkUserDatabase = checkUserDatabase;
    config->accessControl.readUserDefinedRolePermission = readUserProvidedRoles;

    /* Disable anonymous logins, enable two user/password logins */
    config->accessControl.clear(&config->accessControl);
    UA_StatusCode retval = UA_AccessControl_custom(config, true, NULL,
                           &config->securityPolicies[config->securityPoliciesSize-1].policyUri, 0, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AccessControlDefaultFailed");

    retval = UA_Server_run(server, &running);

    UA_Server_deleteNode(server, outNewNodeId, true);
    UA_NodeId_clear(&outNewNodeId);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
