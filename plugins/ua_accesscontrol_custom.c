/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <open62541/plugin/accesscontrol_custom.h>

/* Example access control management. Anonymous and username / password login.
 * The access rights are maximally permissive.
 *
 * FOR PRODUCTION USE, THIS EXAMPLE PLUGIN SHOULD BE REPLACED WITH LESS
 * PERMISSIVE ACCESS CONTROL.
 *
 * For TransferSubscriptions, we check whether the transfer happens between
 * Sessions for the same user. */

#ifdef UA_ENABLE_ROLE_PERMISSION
typedef struct {
    UA_Boolean allowAnonymous;
    size_t usernamePasswordLoginSize;
    UA_UsernamePasswordLogin *usernamePasswordLogin;
    UA_CertificateVerification verifyX509;
} AccessControlContext;

static UA_String anonymous_policy = UA_STRING_STATIC(ANONYMOUS_POLICY);
static UA_String certificate_policy = UA_STRING_STATIC(CERTIFICATE_POLICY);
static UA_String username_policy = UA_STRING_STATIC(USERNAME_POLICY);

/************************/
/* Access Control Logic */
/************************/

static UA_StatusCode
activateSession_custom(UA_Server *server, UA_AccessControl *ac,
                        const UA_EndpointDescription *endpointDescription,
                        const UA_ByteString *secureChannelRemoteCertificate,
                        const UA_NodeId *sessionId,
                        const UA_ExtensionObject *userIdentityToken,
                        void **sessionContext) {
    AccessControlContext *context = (AccessControlContext*)ac->context;
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* The empty token is interpreted as anonymous */
    if(userIdentityToken->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        if(!context->allowAnonymous)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* No userdata atm */
        *sessionContext = NULL;
        return UA_STATUSCODE_GOOD;
    }

    /* Could the token be decoded? */
    if(userIdentityToken->encoding < UA_EXTENSIONOBJECT_DECODED)
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    /* Anonymous login */
    if(userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]) {
        if(!context->allowAnonymous)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        const UA_AnonymousIdentityToken *token = (UA_AnonymousIdentityToken*)
            userIdentityToken->content.decoded.data;

        /* Compatibility notice: Siemens OPC Scout v10 provides an empty
         * policyId. This is not compliant. For compatibility, assume that empty
         * policyId == ANONYMOUS_POLICY */
        if(token->policyId.data && !UA_String_equal(&token->policyId, &anonymous_policy))
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        UA_UsernameRoleInfo *usernameAndRoleInfo = (UA_UsernameRoleInfo *)malloc(sizeof(UA_UsernameRoleInfo));
        UA_String anonymous = UA_STRING("Anonymous");
        UA_String roleName = UA_STRING("Anonymous");
        usernameAndRoleInfo->username = UA_String_new();
        if(usernameAndRoleInfo->username)
            UA_String_copy(&anonymous, usernameAndRoleInfo->username);
        usernameAndRoleInfo->rolename = UA_String_new();
        if(usernameAndRoleInfo->rolename)
            UA_String_copy(&roleName, usernameAndRoleInfo->rolename);

        usernameAndRoleInfo->accessControlSettings = (UA_AccessControlSettings*)malloc(sizeof(UA_AccessControlSettings));
        if (config->accessControl.setRoleAccessPermission != NULL) {
            config->accessControl.setRoleAccessPermission(roleName, usernameAndRoleInfo->accessControlSettings);
        }
        else {
            setUserRole_settings(server, roleName, usernameAndRoleInfo->accessControlSettings);
        }
        /* No userdata atm */
        *sessionContext = usernameAndRoleInfo;
        return UA_STATUSCODE_GOOD;
    }

    /* Username and password */
    if(userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        const UA_UserNameIdentityToken *userToken =
            (UA_UserNameIdentityToken*)userIdentityToken->content.decoded.data;

        if(!UA_String_equal(&userToken->policyId, &username_policy))
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* The userToken has been decrypted by the server before forwarding
         * it to the plugin. This information can be used here. */
        /* if(userToken->encryptionAlgorithm.length > 0) {} */

        /* Empty username and password */
        if(userToken->userName.length == 0 && userToken->password.length == 0)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* Try to match username/pw */
        UA_Boolean match = false;
        UA_String roleName = UA_STRING_NULL;
        if (config->accessControl.checkUserDatabase != NULL) {
            match = config->accessControl.checkUserDatabase(userToken, &roleName);
        }
        else {
            for(size_t i = 0; i < context->usernamePasswordLoginSize; i++) {
                if(UA_String_equal(&userToken->userName, &context->usernamePasswordLogin[i].username) &&
                UA_String_equal(&userToken->password, &context->usernamePasswordLogin[i].password)) {
                    match = true;
                    break;
                }
            }
        }

        if(!match)
            return UA_STATUSCODE_BADUSERACCESSDENIED;

        /* For the CTT, recognize whether two sessions are  */
         UA_UsernameRoleInfo *usernameAndRoleInfo = (UA_UsernameRoleInfo *)malloc(sizeof(UA_UsernameRoleInfo));
         usernameAndRoleInfo->username = UA_String_new();
         if(usernameAndRoleInfo->username)
             UA_String_copy(&userToken->userName, usernameAndRoleInfo->username);
         usernameAndRoleInfo->rolename = UA_String_new();
         if(usernameAndRoleInfo->rolename)
             UA_String_copy(&roleName, usernameAndRoleInfo->rolename);

         usernameAndRoleInfo->accessControlSettings = (UA_AccessControlSettings*)malloc(sizeof(UA_AccessControlSettings));
        if (config->accessControl.setRoleAccessPermission != NULL) {
            config->accessControl.setRoleAccessPermission(roleName, usernameAndRoleInfo->accessControlSettings);
        }
        else {
            setUserRole_settings(server, roleName, usernameAndRoleInfo->accessControlSettings);
        }

        *sessionContext = usernameAndRoleInfo;

        if(roleName.data != NULL)
            UA_free(roleName.data);

        return UA_STATUSCODE_GOOD;
    }

    /* x509 certificate */
    if(userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]) {
        const UA_X509IdentityToken *userToken = (UA_X509IdentityToken*)
            userIdentityToken->content.decoded.data;

        if(!UA_String_equal(&userToken->policyId, &certificate_policy))
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        if(!context->verifyX509.verifyCertificate)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        return context->verifyX509.
            verifyCertificate(context->verifyX509.context,
                              &userToken->certificateData);
    }

    /* Unsupported token type */
    return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
}

static void
closeSession_custom(UA_Server *server, UA_AccessControl *ac,
                     const UA_NodeId *sessionId, void *sessionContext) {

    if (sessionContext != NULL) {
        UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;

        if(userAndRoleInfo->accessControlSettings->identityMappingRule.criteria.data != NULL)
            UA_IdentityMappingRuleType_clear(&userAndRoleInfo->accessControlSettings->identityMappingRule);

        if(userAndRoleInfo->accessControlSettings != NULL)
            UA_free(userAndRoleInfo->accessControlSettings);

        if (userAndRoleInfo->rolename->data != NULL)
            UA_String_delete(userAndRoleInfo->rolename);

        if (userAndRoleInfo->username->data != NULL) {
            UA_String_delete(userAndRoleInfo->username);
        }

        UA_free(userAndRoleInfo);
    }

}

static UA_UInt32
getUserRightsMask_custom(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *nodeId, void *nodeContext) {
    return 0xFFFFFFFF;
}

static UA_Byte
getUserAccessLevel_custom(UA_Server *server, UA_AccessControl *ac,
                           const UA_NodeId *sessionId, void *sessionContext,
                           const UA_NodeId *nodeId, void *nodeContext) {
    UA_Byte outAccessLevel;
    UA_Server_readUserAccessLevel(server, *nodeId, &outAccessLevel);
    return outAccessLevel;
}

static UA_Boolean
getUserExecutable_custom(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext) {
    UA_Boolean outExecutable;
    UA_Server_readUserExecutable(server, *methodId, &outExecutable);
    return outExecutable;
}

static UA_Boolean
getUserExecutableOnObject_custom(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext) {
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
    if (sessionContext != NULL)
        return userAndRoleInfo->accessControlSettings->methodAccessPermission;
    else
        return true;
}

static UA_Boolean
allowAddNode_custom(UA_Server *server, UA_AccessControl *ac,
                    const UA_NodeId *sessionId, void *sessionContext,
                    const UA_AddNodesItem *item, UA_RolePermissionType *userRolePermission,size_t userRolePermissionSize) {
#ifdef UA_ENABLE_ROLE_PERMISSION
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
    if (userRolePermissionSize != 0){
        for (size_t index = 0; index < userRolePermissionSize; index++){
            if (UA_NodeId_equal(&userRolePermission[index].roleId,
                                &userAndRoleInfo->accessControlSettings->role.roleId) == true){
                if ((userRolePermission[index].permissions & UA_PERMISSIONTYPE_ADDNODE) ==
                        UA_PERMISSIONTYPE_ADDNODE){
                    return true;
                }
            }
        }
    }
    else{
        if ((userAndRoleInfo->accessControlSettings->role.permissions & UA_PERMISSIONTYPE_ADDNODE) \
            == UA_PERMISSIONTYPE_ADDNODE){
            return true;
        }
    }

    return false;
#else
    return true;
#endif
}

static UA_Boolean
allowAddReference_custom(UA_Server *server, UA_AccessControl *ac,
                         const UA_NodeId *sessionId, void *sessionContext,
                         const UA_AddReferencesItem *item,
                         UA_RolePermissionType *userRolePermission, size_t userRolePermissionSize) {
#ifdef UA_ENABLE_ROLE_PERMISSION
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
    if (userRolePermissionSize != 0) {
        for (size_t index = 0; index < userRolePermissionSize; index++) {
            if (UA_NodeId_equal(&userRolePermission[index].roleId,
                                &userAndRoleInfo->accessControlSettings->role.roleId) == true) {
                if ((userRolePermission[index].permissions & UA_PERMISSIONTYPE_ADDREFERENCE) ==
                        UA_PERMISSIONTYPE_ADDREFERENCE){
                    return true;
                }
            }
        }
    }
    else{
        if ((userAndRoleInfo->accessControlSettings->role.permissions & UA_PERMISSIONTYPE_ADDREFERENCE) \
            == UA_PERMISSIONTYPE_ADDREFERENCE){
            return true;
        }
    }

    return false;
#else
    return true;
#endif
}

static UA_Boolean
allowDeleteNode_custom(UA_Server *server, UA_AccessControl *ac,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_DeleteNodesItem *item, UA_RolePermissionType *userRolePermission, size_t userRolePermissionSize) {
#ifdef UA_ENABLE_ROLE_PERMISSION
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
    if (userRolePermissionSize != 0) {
        for (size_t index = 0; index < userRolePermissionSize; index++) {
            if (UA_NodeId_equal(&userRolePermission[index].roleId,
                                &userAndRoleInfo->accessControlSettings->role.roleId) == true) {
                if ((userRolePermission[index].permissions & UA_PERMISSIONTYPE_DELETENODE) \
                     == UA_PERMISSIONTYPE_DELETENODE){
                    return true;
                }
            }
        }
    }
    else{
        if ((userAndRoleInfo->accessControlSettings->role.permissions & UA_PERMISSIONTYPE_DELETENODE) \
            == UA_PERMISSIONTYPE_DELETENODE){
            return true;
        }
    }

    return false;
#else
    return true;
#endif
}

static UA_Boolean
allowDeleteReference_custom(UA_Server *server, UA_AccessControl *ac,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_DeleteReferencesItem *item,
                             UA_RolePermissionType *userRolePermission, size_t userRolePermissionSize) {
#ifdef UA_ENABLE_ROLE_PERMISSION
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
    if (userRolePermissionSize != 0){
        for (size_t index = 0; index < userRolePermissionSize; index++){
            if (UA_NodeId_equal(&userRolePermission[index].roleId,
                                &userAndRoleInfo->accessControlSettings->role.roleId) == true){
                if ((userRolePermission[index].permissions & UA_PERMISSIONTYPE_REMOVEREFERENCE) \
                    == UA_PERMISSIONTYPE_REMOVEREFERENCE){
                    return true;
                }
            }
        }
    }
    else{
        if ((userAndRoleInfo->accessControlSettings->role.permissions & UA_PERMISSIONTYPE_REMOVEREFERENCE) \
            == UA_PERMISSIONTYPE_REMOVEREFERENCE){
            return true;
        }
    }

    return false;
#else
    return true;
#endif
}

static UA_Boolean
allowBrowseNode_custom(UA_Server *server, UA_AccessControl *ac,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_NodeId *nodeId, void *nodeContext,
                        UA_RolePermissionType *userRolePermission, size_t userRolePermissionSize) {
#ifdef UA_ENABLE_ROLE_PERMISSION
    if (nodeId->namespaceIndex != 0) {
        UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
        if (userRolePermissionSize != 0) {
            for (size_t index = 0; index < userRolePermissionSize; index++){
                if (UA_NodeId_equal(&userRolePermission[index].roleId,
                                    &userAndRoleInfo->accessControlSettings->role.roleId) == true) {
                    if ((userRolePermission[index].permissions & UA_PERMISSIONTYPE_BROWSE) \
                        == UA_PERMISSIONTYPE_BROWSE){
                        return true;
                    }
                }
            }
        }
        else{
            if ((userAndRoleInfo->accessControlSettings->role.permissions & UA_PERMISSIONTYPE_BROWSE) \
                == UA_PERMISSIONTYPE_BROWSE){
                return true;
            }
        }

        return false;
    }

    return true;
#else
    return true;
#endif
}

static UA_Boolean
hasAccessToNode_custom(UA_Server *server, UA_AccessControl *ac,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *nodeId, void *nodeContext, UA_Byte *serviceAccessLevel) {
    UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
    UA_Byte outAccessLevel;
    UA_Server_readAccessLevel(server, *nodeId, &outAccessLevel);
    UA_Byte accessLevelPermission = (((UA_Byte)userAndRoleInfo->accessControlSettings->accessPermissions) & \
                                     outAccessLevel);
    if ((accessLevelPermission & *serviceAccessLevel) == *serviceAccessLevel) {
        *serviceAccessLevel = accessLevelPermission;
        return true;
    }
    else {
        *serviceAccessLevel = accessLevelPermission;
        return false;
    }
}

static UA_Boolean
hasAccessToMethod_custom(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext) {

    UA_Boolean outExecutable;
    UA_Server_readExecutable(server, *methodId, &outExecutable);
    if (outExecutable == false)
        return false;
    else {
        UA_UsernameRoleInfo *userAndRoleInfo = (UA_UsernameRoleInfo*)sessionContext;
        return userAndRoleInfo->accessControlSettings->methodAccessPermission;
    }
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
static UA_Boolean
allowTransferSubscription_custom(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *oldSessionId, void *oldSessionContext,
                                  const UA_NodeId *newSessionId, void *newSessionContext) {
    if(oldSessionContext == newSessionContext)
        return true;
    if(oldSessionContext && newSessionContext)
        return UA_ByteString_equal((UA_ByteString*)oldSessionContext,
                                   (UA_ByteString*)newSessionContext);
    return false;
}
#endif

#ifdef UA_ENABLE_HISTORIZING
static UA_Boolean
allowHistoryUpdateUpdateData_custom(UA_Server *server, UA_AccessControl *ac,
                                     const UA_NodeId *sessionId, void *sessionContext,
                                     const UA_NodeId *nodeId,
                                     UA_PerformUpdateType performInsertReplace,
                                     const UA_DataValue *value) {
    return true;
}

static UA_Boolean
allowHistoryUpdateDeleteRawModified_custom(UA_Server *server, UA_AccessControl *ac,
                                            const UA_NodeId *sessionId, void *sessionContext,
                                            const UA_NodeId *nodeId,
                                            UA_DateTime startTimestamp,
                                            UA_DateTime endTimestamp,
                                            bool isDeleteModified) {
    return true;
}
#endif

/***************************************/
/* Create Delete Access Control Plugin */
/***************************************/

static void clear_custom(UA_AccessControl *ac) {
    UA_Array_delete((void*)(uintptr_t)ac->userTokenPolicies,
                    ac->userTokenPoliciesSize,
                    &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    ac->userTokenPolicies = NULL;
    ac->userTokenPoliciesSize = 0;

    AccessControlContext *context = (AccessControlContext*)ac->context;

    if (context) {
        for(size_t i = 0; i < context->usernamePasswordLoginSize; i++) {
            UA_String_clear(&context->usernamePasswordLogin[i].username);
            UA_String_clear(&context->usernamePasswordLogin[i].password);
        }
        if(context->usernamePasswordLoginSize > 0)
            UA_free(context->usernamePasswordLogin);

        if(context->verifyX509.clear)
            context->verifyX509.clear(&context->verifyX509);

        UA_free(ac->context);
        ac->context = NULL;
    }
}

UA_StatusCode
UA_AccessControl_custom(UA_ServerConfig *config,
                         UA_Boolean allowAnonymous,
                         UA_CertificateVerification *verifyX509,
                         const UA_ByteString *userTokenPolicyUri,
                         size_t usernamePasswordLoginSize,
                         const UA_UsernamePasswordLogin *usernamePasswordLogin) {
    UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_SERVER,
                   "AccessControl: Unconfigured AccessControl. Users have all permissions.");
    UA_AccessControl *ac = &config->accessControl;

    if(ac->clear)
        clear_custom(ac);

    ac->clear = clear_custom;
    ac->activateSession = activateSession_custom;
    ac->closeSession = closeSession_custom;
    ac->getUserRightsMask = getUserRightsMask_custom;
    ac->getUserAccessLevel = getUserAccessLevel_custom;
    ac->getUserExecutable = getUserExecutable_custom;
    ac->getUserExecutableOnObject = getUserExecutableOnObject_custom;
    ac->allowAddNode = allowAddNode_custom;
    ac->allowAddReference = allowAddReference_custom;
    ac->allowBrowseNode = allowBrowseNode_custom;
    ac->hasAccessToNode  = hasAccessToNode_custom;
    ac->hasAccessToMethod = hasAccessToMethod_custom;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    ac->allowTransferSubscription = allowTransferSubscription_custom;
#endif

#ifdef UA_ENABLE_HISTORIZING
    ac->allowHistoryUpdateUpdateData = allowHistoryUpdateUpdateData_custom;
    ac->allowHistoryUpdateDeleteRawModified = allowHistoryUpdateDeleteRawModified_custom;
#endif

    ac->allowDeleteNode = allowDeleteNode_custom;
    ac->allowDeleteReference = allowDeleteReference_custom;

    AccessControlContext *context = (AccessControlContext*)
            UA_malloc(sizeof(AccessControlContext));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(context, 0, sizeof(AccessControlContext));
    ac->context = context;
    /* Allow anonymous? */
    context->allowAnonymous = allowAnonymous;
    if(allowAnonymous) {
        UA_LOG_INFO(&config->logger, UA_LOGCATEGORY_SERVER,
                    "AccessControl: Anonymous login is enabled");
    }

    /* Allow x509 certificates? Move the plugin over. */
    if(verifyX509) {
        context->verifyX509 = *verifyX509;
        memset(verifyX509, 0, sizeof(UA_CertificateVerification));
    } else {
        memset(&context->verifyX509, 0, sizeof(UA_CertificateVerification));
        UA_LOG_INFO(&config->logger, UA_LOGCATEGORY_SERVER,
                    "AccessControl: x509 certificate user authentication is enabled");
    }

    /* Copy username/password to the access control plugin */
    if(usernamePasswordLoginSize > 0) {
        context->usernamePasswordLogin = (UA_UsernamePasswordLogin*)
            UA_malloc(usernamePasswordLoginSize * sizeof(UA_UsernamePasswordLogin));
        if(!context->usernamePasswordLogin)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        context->usernamePasswordLoginSize = usernamePasswordLoginSize;
        for(size_t i = 0; i < usernamePasswordLoginSize; i++) {
            UA_String_copy(&usernamePasswordLogin[i].username,
                           &context->usernamePasswordLogin[i].username);
            UA_String_copy(&usernamePasswordLogin[i].password,
                           &context->usernamePasswordLogin[i].password);
        }
    }

    /* Set the allowed policies */
    size_t policies = 0;
    if(allowAnonymous)
        policies++;
    if(verifyX509)
        policies++;
    if(usernamePasswordLoginSize > 0)
        policies++;
    ac->userTokenPoliciesSize = 0;
    ac->userTokenPolicies = (UA_UserTokenPolicy *)
        UA_Array_new(policies, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    if(!ac->userTokenPolicies)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ac->userTokenPoliciesSize = policies;

    policies = 0;
    if(allowAnonymous) {
        ac->userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
        ac->userTokenPolicies[policies].policyId = UA_STRING_ALLOC(ANONYMOUS_POLICY);
        policies++;
    }

    if(verifyX509) {
        ac->userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_CERTIFICATE;
        ac->userTokenPolicies[policies].policyId = UA_STRING_ALLOC(CERTIFICATE_POLICY);
#if UA_LOGLEVEL <= 400
        if(UA_ByteString_equal(userTokenPolicyUri, &UA_SECURITY_POLICY_NONE_URI)) {
            UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_SERVER,
                           "x509 Certificate Authentication configured, "
                           "but no encrypting SecurityPolicy. "
                           "This can leak credentials on the network.");
        }
#endif
        UA_ByteString_copy(userTokenPolicyUri,
                           &ac->userTokenPolicies[policies].securityPolicyUri);
        policies++;
    }

    if(usernamePasswordLoginSize > 0) {
        ac->userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_USERNAME;
        ac->userTokenPolicies[policies].policyId = UA_STRING_ALLOC(USERNAME_POLICY);
#if UA_LOGLEVEL <= 400
        if(UA_ByteString_equal(userTokenPolicyUri, &UA_SECURITY_POLICY_NONE_URI)) {
            UA_LOG_WARNING(&config->logger, UA_LOGCATEGORY_SERVER,
                           "Username/Password Authentication configured, "
                           "but no encrypting SecurityPolicy. "
                           "This can leak credentials on the network.");
        }
#endif
        UA_ByteString_copy(userTokenPolicyUri,
                           &ac->userTokenPolicies[policies].securityPolicyUri);
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_SERVER_ROLE_ACCESS_H_ */
