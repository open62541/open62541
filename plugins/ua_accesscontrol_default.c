/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2019 (c) HMS Industrial Networks AB (Author: Jonas Green)
 */

#include <open62541/plugin/accesscontrol_default.h>

/* Example access control management. Anonymous and username / password login.
 * The access rights are maximally permissive.
 *
 * FOR PRODUCTION USE, THIS EXAMPLE PLUGIN SHOULD BE REPLACED WITH LESS
 * PERMISSIVE ACCESS CONTROL.
 *
 * For TransferSubscriptions, we check whether the transfer happens between
 * Sessions for the same user. */

typedef struct {
    UA_Boolean allowAnonymous;
    size_t usernamePasswordLoginSize;
    UA_UsernamePasswordLogin *usernamePasswordLogin;
    UA_UsernamePasswordLoginCallback loginCallback;
    void *loginContext;
    UA_CertificateGroup verifyX509;
} AccessControlContext;

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define CERTIFICATE_POLICY "open62541-certificate-policy"
#define USERNAME_POLICY "open62541-username-policy"

/************************/
/* Access Control Logic */
/************************/

static UA_StatusCode
activateSession_default(UA_Server *server, UA_AccessControl *ac,
                        const UA_EndpointDescription *endpointDescription,
                        const UA_ByteString *secureChannelRemoteCertificate,
                        const UA_NodeId *sessionId,
                        const UA_ExtensionObject *userIdentityToken,
                        void **sessionContext) {
    AccessControlContext *context = (AccessControlContext*)ac->context;

    /* The empty token is interpreted as anonymous */
    UA_AnonymousIdentityToken anonToken;
    UA_ExtensionObject tmpIdentity;
    if(userIdentityToken->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        UA_AnonymousIdentityToken_init(&anonToken);
        UA_ExtensionObject_init(&tmpIdentity);
        UA_ExtensionObject_setValueNoDelete(&tmpIdentity,
                                            &anonToken,
                                            &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]);
        userIdentityToken = &tmpIdentity;
    }

    /* Could the token be decoded? */
    if(userIdentityToken->encoding < UA_EXTENSIONOBJECT_DECODED)
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    const UA_DataType *tokenType = userIdentityToken->content.decoded.type;
    if(tokenType == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]) {
        /* Anonymous login */
        if(!context->allowAnonymous)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    } else if(tokenType == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        /* Username and password */
        const UA_UserNameIdentityToken *userToken = (UA_UserNameIdentityToken*)
            userIdentityToken->content.decoded.data;

        /* Empty username and password */
        if(userToken->userName.length == 0 && userToken->password.length == 0)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* Try to match username/pw */
        if(context->loginCallback) {
            /* Configured callback */
            UA_StatusCode res =
                context->loginCallback(&userToken->userName, &userToken->password,
                                       context->usernamePasswordLoginSize,
                                       context->usernamePasswordLogin,
                                       sessionContext, context->loginContext);
            if(res != UA_STATUSCODE_GOOD)
                return UA_STATUSCODE_BADUSERACCESSDENIED;
        } else {
            /* Compare against the configured list  */
            UA_Boolean match = false;
            for(size_t i = 0; i < context->usernamePasswordLoginSize; i++) {
                UA_UsernamePasswordLogin *upl = &context->usernamePasswordLogin[i];
                if(!UA_String_equal(&userToken->userName, &upl->username))
                   continue;
                if(userToken->password.length != upl->password.length)
                    continue;
                if(!UA_constantTimeEqual(userToken->password.data,
                                         upl->password.data,
                                         upl->password.length))
                    continue;
                match = true;
                break;
            }
            if(!match)
                return UA_STATUSCODE_BADUSERACCESSDENIED;
        }
    } else if(tokenType == &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]) {
        /* x509 certificate was already validated against the sessionPKI in the
         * server */
    } else {
        /* Unsupported token type */
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }

    return UA_STATUSCODE_GOOD;
}

static void
closeSession_default(UA_Server *server, UA_AccessControl *ac,
                     const UA_NodeId *sessionId, void *sessionContext) {
}

static UA_UInt32
getUserRightsMask_default(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *nodeId, void *nodeContext) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    UA_UInt32 userWriteMask = 0;
    if(effectivePerms & UA_PERMISSIONTYPE_WRITEATTRIBUTE) {
        userWriteMask = 0xFFFFFFFF;
        userWriteMask &= ~UA_WRITEMASK_ROLEPERMISSIONS;
        userWriteMask &= ~UA_WRITEMASK_HISTORIZING;
    }
    if(effectivePerms & UA_PERMISSIONTYPE_WRITEROLEPERMISSIONS)
        userWriteMask |= UA_WRITEMASK_ROLEPERMISSIONS;
    if(effectivePerms & UA_PERMISSIONTYPE_WRITEHISTORIZING)
        userWriteMask |= UA_WRITEMASK_HISTORIZING;
    return userWriteMask;
#else
    return 0xFFFFFFFF;
#endif
}

static UA_Byte
getUserAccessLevel_default(UA_Server *server, UA_AccessControl *ac,
                           const UA_NodeId *sessionId, void *sessionContext,
                           const UA_NodeId *nodeId, void *nodeContext) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return 0xFF;
    UA_Byte userAccessLevel = 0;
    if(effectivePerms & UA_PERMISSIONTYPE_READ)
        userAccessLevel |= UA_ACCESSLEVELMASK_READ;
    if(effectivePerms & UA_PERMISSIONTYPE_WRITE)
        userAccessLevel |= UA_ACCESSLEVELMASK_WRITE;
    if(effectivePerms & UA_PERMISSIONTYPE_READHISTORY)
        userAccessLevel |= UA_ACCESSLEVELMASK_HISTORYREAD;
    if(effectivePerms & (UA_PERMISSIONTYPE_INSERTHISTORY |
                         UA_PERMISSIONTYPE_MODIFYHISTORY |
                         UA_PERMISSIONTYPE_DELETEHISTORY))
        userAccessLevel |= UA_ACCESSLEVELMASK_HISTORYWRITE;
    return userAccessLevel;
#else
    return 0xFF;
#endif
}

static UA_Boolean
getUserExecutable_default(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          methodId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return true;
    return (effectivePerms & UA_PERMISSIONTYPE_CALL) != 0;
#else
    return true;
#endif
}

static UA_Boolean
getUserExecutableOnObject_default(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType objectPerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          objectId, &objectPerms);
    if(res != UA_STATUSCODE_GOOD)
        return true;
    if(objectPerms != 0xFFFFFFFF && !(objectPerms & UA_PERMISSIONTYPE_CALL))
        return false;
    UA_PermissionType methodPerms = 0;
    res = UA_Server_getEffectivePermissions(server, sessionId,
                                            methodId, &methodPerms);
    if(res != UA_STATUSCODE_GOOD)
        return true;
    if(methodPerms != 0xFFFFFFFF && !(methodPerms & UA_PERMISSIONTYPE_CALL))
        return false;
    return true;
#else
    return true;
#endif
}

static UA_Boolean
allowAddNode_default(UA_Server *server, UA_AccessControl *ac,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_AddNodesItem *item) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          &item->parentNodeId.nodeId,
                                                          &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return true;
    return (effectivePerms & UA_PERMISSIONTYPE_ADDNODE) != 0;
#else
    return true;
#endif
}

static UA_Boolean
allowAddReference_default(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_AddReferencesItem *item) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          &item->sourceNodeId,
                                                          &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return true;
    return (effectivePerms & UA_PERMISSIONTYPE_ADDREFERENCE) != 0;
#else
    return true;
#endif
}

static UA_Boolean
allowDeleteNode_default(UA_Server *server, UA_AccessControl *ac,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_DeleteNodesItem *item) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          &item->nodeId,
                                                          &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return true;
    return (effectivePerms & UA_PERMISSIONTYPE_DELETENODE) != 0;
#else
    return true;
#endif
}

static UA_Boolean
allowDeleteReference_default(UA_Server *server, UA_AccessControl *ac,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_DeleteReferencesItem *item) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          &item->sourceNodeId,
                                                          &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return true;
    return (effectivePerms & UA_PERMISSIONTYPE_REMOVEREFERENCE) != 0;
#else
    return true;
#endif
}

static UA_Boolean
allowBrowseNode_default(UA_Server *server, UA_AccessControl *ac,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_NodeId *nodeId, void *nodeContext) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return true;
    return (effectivePerms & UA_PERMISSIONTYPE_BROWSE) != 0;
#else
    return true;
#endif
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
static UA_Boolean
allowCreateSubscription_default(UA_Server *server, UA_AccessControl *ac,
                                const UA_NodeId *sessionId, void *sessionContext) {
    return true;
}

static UA_Boolean
allowTransferSubscription_default(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *oldSessionId, void *oldSessionContext,
                                  const UA_NodeId *newSessionId, void *newSessionContext) {
    if(!oldSessionId)
        return true;
    
    /* Get clientUserId for both sessions */
    UA_Variant session1UserId;
    UA_Variant_init(&session1UserId);
    UA_Server_getSessionAttribute(server, oldSessionId,
                                  UA_QUALIFIEDNAME(0, "clientUserId"),
                                  &session1UserId);
    UA_Variant session2UserId;
    UA_Variant_init(&session2UserId);
    UA_Server_getSessionAttribute(server, newSessionId,
                                  UA_QUALIFIEDNAME(0, "clientUserId"),
                                  &session2UserId);

    /* clientUserId is always a String type */
    UA_Boolean result = false;
    if(session1UserId.type == &UA_TYPES[UA_TYPES_STRING] &&
       session2UserId.type == &UA_TYPES[UA_TYPES_STRING]) {
        UA_String *userId1 = (UA_String*)session1UserId.data;
        UA_String *userId2 = (UA_String*)session2UserId.data;
        
        /* Anonymous users have empty userId - reject immediately.
         * According to OPC UA CTT, anonymous users should not be
         * allowed to transfer subscriptions. */
        if(userId1->length == 0 || userId2->length == 0) {
            result = false;
        } else if(UA_String_equal(userId1, userId2)) {
            /* Same authenticated user - allow transfer */
            result = true;
        }
    }
    
    UA_Variant_clear(&session1UserId);
    UA_Variant_clear(&session2UserId);
    
    return result;
}
#endif

#ifdef UA_ENABLE_HISTORIZING
static UA_Boolean
allowHistoryUpdateUpdateData_default(UA_Server *server, UA_AccessControl *ac,
                                     const UA_NodeId *sessionId, void *sessionContext,
                                     const UA_NodeId *nodeId,
                                     UA_PerformUpdateType performInsertReplace,
                                     const UA_DataValue *value) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return true;
    if(performInsertReplace == UA_PERFORMUPDATETYPE_INSERT)
        return (effectivePerms & UA_PERMISSIONTYPE_INSERTHISTORY) != 0;
    else if(performInsertReplace == UA_PERFORMUPDATETYPE_REPLACE ||
            performInsertReplace == UA_PERFORMUPDATETYPE_UPDATE)
        return (effectivePerms & UA_PERMISSIONTYPE_MODIFYHISTORY) != 0;
    return true;
#else
    return true;
#endif
}

static UA_Boolean
allowHistoryUpdateDeleteRawModified_default(UA_Server *server, UA_AccessControl *ac,
                                            const UA_NodeId *sessionId, void *sessionContext,
                                            const UA_NodeId *nodeId,
                                            UA_DateTime startTimestamp,
                                            UA_DateTime endTimestamp,
                                            bool isDeleteModified) {
#ifdef UA_ENABLE_RBAC
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId,
                                                          nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD || effectivePerms == 0xFFFFFFFF)
        return true;
    return (effectivePerms & UA_PERMISSIONTYPE_DELETEHISTORY) != 0;
#else
    return true;
#endif
}
#endif

/***************************************/
/* Create Delete Access Control Plugin */
/***************************************/

static void clear_default(UA_AccessControl *ac) {
    UA_Array_delete((void*)(uintptr_t)ac->userTokenPolicies,
                    ac->userTokenPoliciesSize,
                    &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    ac->userTokenPolicies = NULL;
    ac->userTokenPoliciesSize = 0;

    AccessControlContext *context = (AccessControlContext*)ac->context;

    if (context) {
        for(size_t i = 0; i < context->usernamePasswordLoginSize; i++) {
            UA_String_clear(&context->usernamePasswordLogin[i].username);
            UA_ByteString_clear(&context->usernamePasswordLogin[i].password);
        }
        if(context->usernamePasswordLoginSize > 0)
            UA_free(context->usernamePasswordLogin);

        UA_free(ac->context);
        ac->context = NULL;
    }
}

UA_StatusCode
UA_AccessControl_default(UA_ServerConfig *config,
                         UA_Boolean allowAnonymous,
                         const UA_String *userTokenPolicyUri,
                         size_t usernamePasswordLoginSize,
                         const UA_UsernamePasswordLogin *usernamePasswordLogin) {
    UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                   "AccessControl: Unconfigured AccessControl. Users have all permissions.");
    UA_AccessControl *ac = &config->accessControl;

    if(ac->clear)
        ac->clear(ac);

    ac->clear = clear_default;
    ac->activateSession = activateSession_default;
    ac->closeSession = closeSession_default;
    ac->getUserRightsMask = getUserRightsMask_default;
    ac->getUserAccessLevel = getUserAccessLevel_default;
    ac->getUserExecutable = getUserExecutable_default;
    ac->getUserExecutableOnObject = getUserExecutableOnObject_default;
    ac->allowAddNode = allowAddNode_default;
    ac->allowAddReference = allowAddReference_default;
    ac->allowBrowseNode = allowBrowseNode_default;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    ac->allowCreateSubscription = allowCreateSubscription_default;
    ac->allowTransferSubscription = allowTransferSubscription_default;
#endif

#ifdef UA_ENABLE_HISTORIZING
    ac->allowHistoryUpdateUpdateData = allowHistoryUpdateUpdateData_default;
    ac->allowHistoryUpdateDeleteRawModified = allowHistoryUpdateDeleteRawModified_default;
#endif

    ac->allowDeleteNode = allowDeleteNode_default;
    ac->allowDeleteReference = allowDeleteReference_default;

    AccessControlContext *context = (AccessControlContext*)
            UA_malloc(sizeof(AccessControlContext));
    if(!context)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(context, 0, sizeof(AccessControlContext));
    ac->context = context;

    /* Allow anonymous? */
    context->allowAnonymous = allowAnonymous;
    if(allowAnonymous) {
        UA_LOG_INFO(config->logging, UA_LOGCATEGORY_SERVER,
                    "AccessControl: Anonymous login is enabled");
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
            UA_ByteString_copy(&usernamePasswordLogin[i].password,
                           &context->usernamePasswordLogin[i].password);
        }
    }

    size_t numOfPolcies = 1;
    if(!userTokenPolicyUri) {
        if(config->securityPoliciesSize > 0)
            numOfPolcies = config->securityPoliciesSize;
        else {
            UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                           "No security policies defined for the secure channel.");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }

    /* Set the allowed policies */
    size_t policies = 0;
    if(allowAnonymous)
        policies++;
    if(usernamePasswordLoginSize > 0)
        policies++;
    if(config->sessionPKI.verifyCertificate)
        policies++;
    ac->userTokenPoliciesSize = 0;
    ac->userTokenPolicies = (UA_UserTokenPolicy *)
        UA_Array_new(policies * numOfPolcies, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    if(!ac->userTokenPolicies)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ac->userTokenPoliciesSize = policies * numOfPolcies;

    if(policies == 0) {
        UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                       "No allowed policies set.");
        return UA_STATUSCODE_GOOD;
    }

    const UA_String *utpUri = NULL;
    policies = 0;
    for(size_t i = 0; i < numOfPolcies; i++) {
        if(userTokenPolicyUri) {
            utpUri = userTokenPolicyUri;
        } else {
            utpUri = &config->securityPolicies[i].policyUri;
        }
        if(allowAnonymous) {
            ac->userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
            ac->userTokenPolicies[policies].policyId = UA_STRING_ALLOC(ANONYMOUS_POLICY);
            UA_String_copy(utpUri,
                               &ac->userTokenPolicies[policies].securityPolicyUri);
            policies++;
        }

        if(config->sessionPKI.verifyCertificate) {
            ac->userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_CERTIFICATE;
            ac->userTokenPolicies[policies].policyId = UA_STRING_ALLOC(CERTIFICATE_POLICY);
#if UA_LOGLEVEL <= 400
            if(UA_String_equal(utpUri, &UA_SECURITY_POLICY_NONE_URI)) {
                UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                               "x509 Certificate Authentication configured, "
                               "but no encrypting SecurityPolicy. "
                               "This can leak credentials on the network.");
            }
#endif
            UA_String_copy(utpUri,
                               &ac->userTokenPolicies[policies].securityPolicyUri);
            policies++;
        }

        if(usernamePasswordLoginSize > 0) {
            ac->userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_USERNAME;
            ac->userTokenPolicies[policies].policyId = UA_STRING_ALLOC(USERNAME_POLICY);
#if UA_LOGLEVEL <= 400
            if(UA_String_equal(utpUri, &UA_SECURITY_POLICY_NONE_URI)) {
                UA_LOG_WARNING(config->logging, UA_LOGCATEGORY_SERVER,
                               "Username/Password Authentication configured, "
                               "but no encrypting SecurityPolicy. "
                               "This can leak credentials on the network.");
            }
#endif
            UA_String_copy(utpUri,
                               &ac->userTokenPolicies[policies].securityPolicyUri);
            policies++;
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_AccessControl_defaultWithLoginCallback(UA_ServerConfig *config,
                                          UA_Boolean allowAnonymous,
                                          const UA_String *userTokenPolicyUri,
                                          size_t usernamePasswordLoginSize,
                                          const UA_UsernamePasswordLogin *usernamePasswordLogin,
                                          UA_UsernamePasswordLoginCallback loginCallback,
                                          void *loginContext) {
    AccessControlContext *context;
    UA_StatusCode sc =
        UA_AccessControl_default(config, allowAnonymous, userTokenPolicyUri,
                                 usernamePasswordLoginSize, usernamePasswordLogin);
    if(sc != UA_STATUSCODE_GOOD)
        return sc;

    context = (AccessControlContext *)config->accessControl.context;
    context->loginCallback = loginCallback;
    context->loginContext = loginContext;

    return UA_STATUSCODE_GOOD;
}

