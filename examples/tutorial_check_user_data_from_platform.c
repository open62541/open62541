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
typedef struct __attribute__((__packed__)) user_data_buffer {
    bool userAvailable;
    UA_String username;
    UA_String password;
    UA_String role;
    uint64_t GroupID;
    uint64_t userID;
} userDatabase;

int checkUser (userDatabase* userData);
UA_Boolean checkUserDatabase(const UA_UserNameIdentityToken *userToken, UA_String *roleName);
UA_StatusCode addNewNamespaceandSetDefaultPermission(UA_Server *server);
UA_StatusCode createNewRoleMethodCall(UA_Server *server);
//UA_StatusCode removeRoleMethodCall(UA_Server *server);
UA_StatusCode checkTheRoleSessionLoggedIn(UA_Server *server);
//void  checkTheRoleSessionLoggedIn(UA_Server *server, void *data);

static UA_StatusCode
roleIdentificationMethodCallback(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *methodId, void *methodContext,
                                 const UA_NodeId *objectId, void *objectContext,
                                 size_t inputSize, const UA_Variant *input,
                                 size_t outputSize, UA_Variant *output) {
    //UA_ServerConfig *config = UA_Server_getConfig(server);
    if (sessionContext == NULL){
        UA_String userRoleInfo = UA_STRING("Anonymous");
        UA_Variant_setScalarCopy(output, &userRoleInfo, &UA_TYPES[UA_TYPES_STRING]);
        return UA_STATUSCODE_GOOD;
    }
#ifdef UA_ENABLE_ROLE_PERMISSION
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
    printf("\nSessionUsername:%s\n", userAndRoleInfo->username->data);
    printf("\nSessionRolename:%s\n", userAndRoleInfo->rolename->data);
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

UA_StatusCode addNewNamespaceandSetDefaultPermission(UA_Server *server){
 UA_UInt16 nsIdx = UA_Server_addNamespace(server, "http://yourorganisation.org/test/");
    printf("\nnameSpaceIndex:%d\n", nsIdx);
    UA_NodeId outNewNodeId;
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "http://yourorganisation.org/test/");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "http://yourorganisation.org/test/");
    UA_Server_addObjectNode(server, UA_NODEID_STRING(nsIdx, "http://yourorganisation.org/test/"), 
                            UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_NAMESPACES), UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(nsIdx, "http://yourorganisation.org/test/"), UA_NODEID_NUMERIC(0, UA_NS0ID_NAMESPACEMETADATATYPE),
                            attr, NULL, &outNewNodeId);

    UA_VariableAttributes attributes = UA_VariableAttributes_default;
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "TestRoleNode");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_UInt16 value = 15;
    UA_Variant_setScalar(&attributes.value, &value, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(
        server, UA_NODEID_NUMERIC(1, 1000), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "TestRoleNode"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attributes, NULL, NULL);

    attributes = UA_VariableAttributes_default;
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
    rolePermission[0].permissions = 0xFFFF;
    rolePermission[1].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER);
    rolePermission[1].permissions = 0xFFFF;
    rolePermission[2].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_CONFIGUREADMIN);
    rolePermission[2].permissions = 0xFFFF;
    rolePermission[3].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ENGINEER);
    rolePermission[3].permissions = 0xFFFF;
    rolePermission[4].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OBSERVER);
    rolePermission[4].permissions = 0xFFFF;
    rolePermission[5].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_OPERATOR);
    rolePermission[5].permissions = 0xFFFF;
    rolePermission[6].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    rolePermission[6].permissions = 0xFFFF;
    rolePermission[7].roleId = UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SUPERVISOR);
    rolePermission[7].permissions = 0xFFFF;
    UA_Variant_setArray(&attributes.value, rolePermission, 8, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_addVariableNode(
        server, UA_NODEID_STRING(nsIdx, "DefaultRolePermissions"), outNewNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
        UA_QUALIFIEDNAME(nsIdx, "DefaultRolePermissions"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attributes, NULL, NULL);
    attributes.displayName = UA_LOCALIZEDTEXT("en-US", "DefaultUserRolePermissions");
    attributes.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attributes.dataType = UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE].typeId;
    //UA_Variant_setScalar(&attributes.value, &permissionValue, &UA_TYPES[UA_TYPES_ROLEPERMISSIONTYPE]);
    UA_Server_addVariableNode(
        server, UA_NODEID_STRING(nsIdx, "DefaultUserRolePermissions"), outNewNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
        UA_QUALIFIEDNAME(nsIdx, "DefaultUserRolePermissions"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), attributes, NULL, NULL);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode createNewRoleMethodCall(UA_Server *server){
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
        printf("\nMethodCall Failed\n");
    
    printf("\nOutputArgumentSize:%ld\n", result.outputArgumentsSize);
    UA_NodeId *roleNodeId = (UA_NodeId *)result.outputArguments[0].data;
    printf("\nroleNodeId:%s\n", roleNodeId->identifier.string.data);
    printf("\nroleNodeIdNmaespaceIndex:%d\n", roleNodeId->namespaceIndex);

    return result.statusCode;
}

/*UA_StatusCode removeRoleMethodCall(UA_Server *server){
    UA_Variant inputArguments[1];
    UA_Variant_init(&inputArguments[0]);
    UA_NodeId removeRole;
    removeRole.identifier.string = UA_STRING("KalycitoWorker");
    removeRole.identifierType = UA_NODEIDTYPE_STRING;
    removeRole.namespaceIndex = 2;
    UA_Variant_setScalarCopy(&inputArguments[0], &removeRole, &UA_TYPES[UA_TYPES_NODEID]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;         // 1 would be correct
    callMethodRequest.inputArguments = (UA_Variant*)&inputArguments;
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_ROLESETTYPE_REMOVEROLE);
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERCAPABILITIES_ROLESET);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    if (result.statusCode != UA_STATUSCODE_GOOD)
        printf("\nMethodCall Failed\n");

    return result.statusCode;
}*/

//void  checkTheRoleSessionLoggedIn(UA_Server *server, void *data){
/*UA_StatusCode checkTheRoleSessionLoggedIn(UA_Server *server){
    printf("\ncheckTheRoleSessionLoggedIn\n");
    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.methodId = UA_NODEID_STRING(1, "SessionRoleIdentification");
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    if (result.statusCode != UA_STATUSCODE_GOOD)
        printf("\nMethodCall Failed\n");
    else{
    printf("\ncheckTheRoleSessionStartedEnd\n");
    printf("\nOutputArgumentSize:%ld\n", result.outputArgumentsSize);
    UA_String *roleNameSessionLoggedIn = (UA_String *)result.outputArguments[0].data;
    printf("\nroleNameSessionLoggedIn:%s\n", roleNameSessionLoggedIn->data);
    }

    return result.statusCode;
}*/

UA_Boolean checkUserDatabase(const UA_UserNameIdentityToken *userToken, UA_String *roleName){
    userDatabase userData;
    userData.userAvailable = false;
    UA_String_copy(&userToken->userName, &userData.username);
    UA_String_copy(&userToken->password, &userData.password);
    checkUser(&userData);
    if (userData.userAvailable != true)
       return false;

    printf("\nPlatformUseravailale:%d\n", userData.userAvailable);
    printf("\nPlatformUserNmae:%s\n", userData.username.data);
    printf("\nPlatformPassword:%s\n", userData.password.data);
    printf("\nPlatformRole:%s\n", userData.role.data);
    printf("\nPlatformLength:%ld\n", userData.role.length);
    printf("\nPlatformGroupID:%ld\n", userData.GroupID);
    printf("\nPlatformUserID:%ld\n", userData.userID);
    UA_String_copy(&userData.role, roleName);

    if (userData.username.data != NULL)
        free(userData.username.data);

    if (userData.password.data != NULL)
        free(userData.password.data );

    if (userData.password.data != NULL)
        free(userData.role.data );

    //checkTheRoleSessionStarted(server);

    return true;
}

/** It follows the main server code, making use of the above definitions. */

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
    //UA_UInt64 callbackId;
    addNewNamespaceandSetDefaultPermission(server);
    addSessionRoleIdentificationMethodCall(server);
    createNewRoleMethodCall(server);
   // UA_Server_addRepeatedCallback(server, checkTheRoleSessionLoggedIn,NULL,5000, &callbackId);
   // removeRoleMethodCall(server);
    //checkTheRoleSessionStarted(server);
    config->accessControl.checkUserDatabase = checkUserDatabase;
  //  config->accessControl.checkTheRoleSessionLoggedIn = checkTheRoleSessionLoggedIn;

    /* Disable anonymous logins, enable two user/password logins */
    config->accessControl.clear(&config->accessControl);
    UA_StatusCode retval = UA_AccessControl_custom(config, true, NULL,
             &config->securityPolicies[config->securityPoliciesSize-1].policyUri, 0, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        printf("\nAccessControlDefaultFailed\n");

    retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
