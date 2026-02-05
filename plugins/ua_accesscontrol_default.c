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
static const UA_String anonymous_policy = UA_STRING_STATIC(ANONYMOUS_POLICY);
static const UA_String certificate_policy = UA_STRING_STATIC(CERTIFICATE_POLICY);
static const UA_String username_policy = UA_STRING_STATIC(USERNAME_POLICY);

/************************/
/* Access Control Logic */
/************************/

static UA_StatusCode
activateSession_default(UA_Server *server, UA_AccessControl *ac,
                        const UA_EndpointDescription *endpointDescription,
                        const UA_ByteString *secureChannelRemoteCertificate,
                        const UA_NodeId *sessionId,
                        const UA_ExtensionObject *userIdentityToken,
                        void **sessionContext
#ifdef UA_ENABLE_RBAC
                        , size_t *rolesSize,
                        UA_NodeId **roleIds
#endif
                        ) {
    AccessControlContext *context = (AccessControlContext*)ac->context;
    UA_ServerConfig *config = UA_Server_getConfig(server);

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

        const UA_AnonymousIdentityToken *token = (UA_AnonymousIdentityToken*)
            userIdentityToken->content.decoded.data;

        /* Match the beginnig of the PolicyId.
         * Compatibility notice: Siemens OPC Scout v10 provides an empty
         * policyId. This is not compliant. For compatibility, assume that empty
         * policyId == ANONYMOUS_POLICY */
        if(token->policyId.data &&
           (token->policyId.length < anonymous_policy.length ||
            strncmp((const char*)token->policyId.data,
                    (const char*)anonymous_policy.data,
                    anonymous_policy.length) != 0)) {
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        }
    } else if(tokenType == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        /* Username and password */
        const UA_UserNameIdentityToken *userToken = (UA_UserNameIdentityToken*)
            userIdentityToken->content.decoded.data;

        /* Match the beginnig of the PolicyId */
        if(userToken->policyId.length < username_policy.length ||
           strncmp((const char*)userToken->policyId.data,
                   (const char*)username_policy.data,
                   username_policy.length) != 0) {
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        }

        /* The userToken has been decrypted by the server before forwarding
         * it to the plugin. This information can be used here. */
        /* if(userToken->encryptionAlgorithm.length > 0) {} */

        /* Empty username and password */
        if(userToken->userName.length == 0 && userToken->password.length == 0)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* Try to match username/pw */
        UA_Boolean match = false;
        if(context->loginCallback) {
            if(context->loginCallback(&userToken->userName, &userToken->password,
                                      context->usernamePasswordLoginSize, context->usernamePasswordLogin,
                                      sessionContext, context->loginContext) == UA_STATUSCODE_GOOD)
                match = true;
        } else {
            for(size_t i = 0; i < context->usernamePasswordLoginSize; i++) {
                if(UA_String_equal(&userToken->userName, &context->usernamePasswordLogin[i].username) &&
                   UA_ByteString_equal(&userToken->password, &context->usernamePasswordLogin[i].password)) {
                    match = true;
                    break;
                }
            }
        }
        if(!match)
            return UA_STATUSCODE_BADUSERACCESSDENIED;
    } else if(tokenType == &UA_TYPES[UA_TYPES_X509IDENTITYTOKEN]) {
        /* x509 certificate */
        const UA_X509IdentityToken *userToken = (UA_X509IdentityToken*)
            userIdentityToken->content.decoded.data;

        /* Match the beginnig of the PolicyId */
        if(userToken->policyId.length < certificate_policy.length ||
           strncmp((const char*)userToken->policyId.data,
                   (const char*)certificate_policy.data,
                   certificate_policy.length) != 0) {
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        }

        if(!config->sessionPKI.verifyCertificate)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

       UA_StatusCode res = config->sessionPKI.
            verifyCertificate(&config->sessionPKI, &userToken->certificateData);
        if(res != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADIDENTITYTOKENREJECTED;
    } else {
        /* Unsupported token type */
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }

#ifdef UA_ENABLE_RBAC
    /* Evaluate roles based on identity mapping rules according to OPC UA Part 18.
     * 
     * Role assignment is based on the Identities property of each Role which
     * contains an array of IdentityMappingRuleType. A role is granted if ANY
     * of its identity mapping rules match the current session's identity.
     * 
     * IdentityCriteriaType values (from Part 18):
     * - Anonymous (5): Matches when user provides no credentials
     * - AuthenticatedUser (6): Matches any non-anonymous user  
     * - UserName (1): Matches specific username in criteria field
     * - Thumbprint (2): Matches certificate thumbprint
     * - X509Subject (8): Matches X509 subject name
     * - Application (7): Matches ApplicationUri from client certificate
     * - TrustedApplication (9): Matches any trusted application
     * - Role (3): Matches role in Access Token (JWT)
     * - GroupId (4): Matches group in Access Token
     */
    if(rolesSize && roleIds) {
        *rolesSize = 0;
        *roleIds = NULL;
        
        /* Determine session identity characteristics */
        UA_Boolean isAnonymous = (tokenType == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]);
        UA_Boolean isAuthenticated = !isAnonymous;
        UA_String userName = UA_STRING_NULL;
        
        if(tokenType == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
            const UA_UserNameIdentityToken *userToken = 
                (UA_UserNameIdentityToken*)userIdentityToken->content.decoded.data;
            userName = userToken->userName;
        }
        /* TODO: For X509, extract thumbprint/subject for matching */
        
        /* Get all roles from the server */
        size_t allRolesSize = 0;
        UA_NodeId *allRoleIds = NULL;
        UA_StatusCode res = UA_Server_getRoles(server, &allRolesSize, &allRoleIds);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        
        /* Pre-allocate for maximum possible matches, we'll shrink later if needed */
        UA_NodeId *matchedRoles = (UA_NodeId*)UA_malloc(allRolesSize * sizeof(UA_NodeId));
        if(!matchedRoles) {
            UA_Array_delete(allRoleIds, allRolesSize, &UA_TYPES[UA_TYPES_NODEID]);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        
        size_t matchCount = 0;
        for(size_t i = 0; i < allRolesSize; i++) {
            const UA_Role *role = UA_Server_getRoleById(server, allRoleIds[i]);
            if(!role)
                continue;
                
            /* Check if any identity mapping rule matches */
            for(size_t j = 0; j < role->imrtSize; j++) {
                UA_Boolean match = false;
                UA_IdentityCriteriaType criteriaType = role->imrt[j].criteriaType;
                
                switch(criteriaType) {
                    case UA_IDENTITYCRITERIATYPE_ANONYMOUS:
                        /* Matches only anonymous sessions */
                        match = isAnonymous;
                        break;
                        
                    case UA_IDENTITYCRITERIATYPE_AUTHENTICATEDUSER:
                        /* Matches any authenticated (non-anonymous) session */
                        match = isAuthenticated;
                        break;
                        
                    case UA_IDENTITYCRITERIATYPE_USERNAME:
                        /* Matches if username equals the criteria string */
                        if(userName.length > 0 && role->imrt[j].criteria.length > 0) {
                            match = UA_String_equal(&userName, &role->imrt[j].criteria);
                        }
                        break;
                        
                    /* TODO: Implement additional criteria types */
                    case UA_IDENTITYCRITERIATYPE_THUMBPRINT:
                    case UA_IDENTITYCRITERIATYPE_ROLE:
                    case UA_IDENTITYCRITERIATYPE_GROUPID:
                    case UA_IDENTITYCRITERIATYPE_APPLICATION:
                    case UA_IDENTITYCRITERIATYPE_X509SUBJECT:
                    default:
                        break;
                }
                
                if(match) {
                    UA_NodeId_copy(&allRoleIds[i], &matchedRoles[matchCount]);
                    matchCount++;
                    break; /* One matching rule is enough */
                }
            }
        }
        
        UA_Array_delete(allRoleIds, allRolesSize, &UA_TYPES[UA_TYPES_NODEID]);
        
        if(matchCount > 0) {
            *roleIds = matchedRoles;
            *rolesSize = matchCount;
        } else {
            UA_free(matchedRoles);
        }
    }
#endif

    return UA_STATUSCODE_GOOD;
}

static void
closeSession_default(UA_Server *server, UA_AccessControl *ac,
                     const UA_NodeId *sessionId, void *sessionContext) {
}

/* RBAC-aware getUserRightsMask implementation
 * 
 * Per OPC UA Part 3 Section 8.55:
 * - WriteAttribute (Bit 2): affects UserWriteMask for general attributes
 * - WriteRolePermissions (Bit 3): affects UserWriteMask for RolePermissions
 * - WriteHistorizing (Bit 4): affects UserWriteMask for Historizing
 * 
 * The UserWriteMask is the intersection of:
 * 1. What's allowed by WriteMask (node capability)
 * 2. What the session's roles permissions allow */
static UA_UInt32
getUserRightsMask_default(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *nodeId, void *nodeContext) {
#ifdef UA_ENABLE_RBAC
    /* Get effective permissions for this session on this node */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return 0xFFFFFFFF; /* Default permissive on error */
    
    /* If no RBAC configuration, allow everything */
    if(effectivePerms == 0xFFFFFFFF)
        return 0xFFFFFFFF;
    
    /* Build UserWriteMask based on RBAC permissions */
    UA_UInt32 userWriteMask = 0;
    
    /* WriteAttribute permission allows writing most attributes */
    if(effectivePerms & UA_PERMISSIONTYPE_WRITEATTRIBUTE) {
        /* All attributes except Value, Historizing, and RolePermissions */
        userWriteMask = 0xFFFFFFFF;
        /* Clear bits that need specific permissions */
        userWriteMask &= ~UA_WRITEMASK_ROLEPERMISSIONS;
        userWriteMask &= ~UA_WRITEMASK_HISTORIZING;
    }
    
    /* WriteRolePermissions permission allows writing RolePermissions attribute */
    if(effectivePerms & UA_PERMISSIONTYPE_WRITEROLEPERMISSIONS) {
        userWriteMask |= UA_WRITEMASK_ROLEPERMISSIONS;
    }
    
    /* WriteHistorizing permission allows writing Historizing attribute */
    if(effectivePerms & UA_PERMISSIONTYPE_WRITEHISTORIZING) {
        userWriteMask |= UA_WRITEMASK_HISTORIZING;
    }
    
    return userWriteMask;
#else
    return 0xFFFFFFFF;
#endif
}

/* RBAC-aware getUserAccessLevel implementation
 * 
 * Per OPC UA Part 3 Section 8.55:
 * - Read (Bit 5): affects CurrentRead bit of UserAccessLevel
 * - Write (Bit 6): affects CurrentWrite bit of UserAccessLevel
 * - ReadHistory (Bit 7): affects HistoryRead bit of UserAccessLevel
 * - InsertHistory/ModifyHistory/DeleteHistory (Bits 8-10): affect HistoryWrite bit
 * 
 * The UserAccessLevel is the intersection of:
 * 1. What's allowed by AccessLevel (node capability)
 * 2. What the session's roles permissions allow */
static UA_Byte
getUserAccessLevel_default(UA_Server *server, UA_AccessControl *ac,
                           const UA_NodeId *sessionId, void *sessionContext,
                           const UA_NodeId *nodeId, void *nodeContext) {
#ifdef UA_ENABLE_RBAC
    /* Get effective permissions for this session on this node */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return 0xFF; /* Default permissive on error */
    
    /* If no RBAC configuration, allow everything */
    if(effectivePerms == 0xFFFFFFFF)
        return 0xFF;
    
    /* Build UserAccessLevel based on RBAC permissions */
    UA_Byte userAccessLevel = 0;
    
    /* Read permission -> CurrentRead bit */
    if(effectivePerms & UA_PERMISSIONTYPE_READ)
        userAccessLevel |= UA_ACCESSLEVELMASK_READ;
    
    /* Write permission -> CurrentWrite bit */
    if(effectivePerms & UA_PERMISSIONTYPE_WRITE)
        userAccessLevel |= UA_ACCESSLEVELMASK_WRITE;
    
    /* ReadHistory permission -> HistoryRead bit */
    if(effectivePerms & UA_PERMISSIONTYPE_READHISTORY)
        userAccessLevel |= UA_ACCESSLEVELMASK_HISTORYREAD;
    
    /* InsertHistory, ModifyHistory, or DeleteHistory -> HistoryWrite bit */
    if(effectivePerms & (UA_PERMISSIONTYPE_INSERTHISTORY | 
                         UA_PERMISSIONTYPE_MODIFYHISTORY | 
                         UA_PERMISSIONTYPE_DELETEHISTORY))
        userAccessLevel |= UA_ACCESSLEVELMASK_HISTORYWRITE;
    
    return userAccessLevel;
#else
    return 0xFF;
#endif
}

/* RBAC-aware getUserExecutable implementation
 * 
 * Per OPC UA Part 3 Section 8.55:
 * - Call (Bit 12): affects UserExecutable Attribute on Method nodes
 * 
 * A method is user-executable if the session has Call permission */
static UA_Boolean
getUserExecutable_default(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext) {
#ifdef UA_ENABLE_RBAC
    /* Get effective permissions for this session on this method */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, methodId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If no RBAC configuration, allow */
    if(effectivePerms == 0xFFFFFFFF)
        return true;
    
    /* Call permission required for method execution */
    return (effectivePerms & UA_PERMISSIONTYPE_CALL) != 0;
#else
    return true;
#endif
}

/* RBAC-aware getUserExecutableOnObject implementation
 * 
 * Per OPC UA Part 3 Section 8.55:
 * - Call (Bit 12): "The Client is allowed to call the Method if this bit is set
 *   on the Object or ObjectType Node passed in the Call request and the Method
 *   Instance associated with that Object or ObjectType."
 * 
 * Both the object and the method need Call permission */
static UA_Boolean
getUserExecutableOnObject_default(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext) {
#ifdef UA_ENABLE_RBAC
    /* Check Call permission on the object */
    UA_PermissionType objectPerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, objectId, &objectPerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If object has RBAC config and lacks Call permission, deny */
    if(objectPerms != 0xFFFFFFFF && !(objectPerms & UA_PERMISSIONTYPE_CALL))
        return false;
    
    /* Check Call permission on the method */
    UA_PermissionType methodPerms = 0;
    res = UA_Server_getEffectivePermissions(server, sessionId, methodId, &methodPerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If method has RBAC config and lacks Call permission, deny */
    if(methodPerms != 0xFFFFFFFF && !(methodPerms & UA_PERMISSIONTYPE_CALL))
        return false;
    
    return true;
#else
    return true;
#endif
}

/* RBAC-aware allowAddNode implementation
 * 
 * Per OPC UA Part 3 Section 8.55:
 * - AddNode (Bit 16): "The Client is allowed to add Nodes to the Namespace.
 *   This Permission is only used in the DefaultRolePermissions and
 *   DefaultUserRolePermissions Properties of a NamespaceMetadata Object" */
static UA_Boolean
allowAddNode_default(UA_Server *server, UA_AccessControl *ac,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_AddNodesItem *item) {
#ifdef UA_ENABLE_RBAC
    /* Check AddReference permission on the parent node */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, 
                                                          &item->parentNodeId.nodeId, 
                                                          &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If no RBAC configuration, allow */
    if(effectivePerms == 0xFFFFFFFF)
        return true;
    
    /* AddReference permission on parent required to add children */
    return (effectivePerms & UA_PERMISSIONTYPE_ADDREFERENCE) != 0;
#else
    return true;
#endif
}

/* RBAC-aware allowAddReference implementation */
static UA_Boolean
allowAddReference_default(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_AddReferencesItem *item) {
#ifdef UA_ENABLE_RBAC
    /* Check AddReference permission on the source node */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, 
                                                          &item->sourceNodeId, 
                                                          &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If no RBAC configuration, allow */
    if(effectivePerms == 0xFFFFFFFF)
        return true;
    
    return (effectivePerms & UA_PERMISSIONTYPE_ADDREFERENCE) != 0;
#else
    return true;
#endif
}

/* RBAC-aware allowDeleteNode implementation */
static UA_Boolean
allowDeleteNode_default(UA_Server *server, UA_AccessControl *ac,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_DeleteNodesItem *item) {
#ifdef UA_ENABLE_RBAC
    /* Check DeleteNode permission on the node being deleted */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, 
                                                          &item->nodeId, 
                                                          &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If no RBAC configuration, allow */
    if(effectivePerms == 0xFFFFFFFF)
        return true;
    
    return (effectivePerms & UA_PERMISSIONTYPE_DELETENODE) != 0;
#else
    return true;
#endif
}

/* RBAC-aware allowDeleteReference implementation */
static UA_Boolean
allowDeleteReference_default(UA_Server *server, UA_AccessControl *ac,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_DeleteReferencesItem *item) {
#ifdef UA_ENABLE_RBAC
    /* Check RemoveReference permission on the source node */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, 
                                                          &item->sourceNodeId, 
                                                          &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If no RBAC configuration, allow */
    if(effectivePerms == 0xFFFFFFFF)
        return true;
    
    return (effectivePerms & UA_PERMISSIONTYPE_REMOVEREFERENCE) != 0;
#else
    return true;
#endif
}

/* RBAC-aware allowBrowseNode implementation
 * 
 * Per OPC UA Part 3 Section 8.55:
 * - Browse (Bit 0): "The Client is allowed to see the references to and from
 *   the Node. This implies that the Client is able to Read Attributes other
 *   than the Value or the RolePermissions Attribute." */
static UA_Boolean
allowBrowseNode_default(UA_Server *server, UA_AccessControl *ac,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_NodeId *nodeId, void *nodeContext) {
#ifdef UA_ENABLE_RBAC
    /* Get effective permissions for this session on this node */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If no RBAC configuration, allow */
    if(effectivePerms == 0xFFFFFFFF)
        return true;
    
    /* Browse permission required */
    return (effectivePerms & UA_PERMISSIONTYPE_BROWSE) != 0;
#else
    return true;
#endif
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
static UA_Boolean
allowTransferSubscription_default(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *oldSessionId, void *oldSessionContext,
                                  const UA_NodeId *newSessionId, void *newSessionContext) {
    if(!oldSessionId)
        return true;
    /* Allow the transfer if the same user-id was used to activate both sessions */
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

    return (UA_order(&session1UserId, &session2UserId,
                     &UA_TYPES[UA_TYPES_VARIANT]) == UA_ORDER_EQ);
}
#endif

#ifdef UA_ENABLE_HISTORIZING
/* RBAC-aware allowHistoryUpdateUpdateData implementation
 * 
 * Per OPC UA Part 3 Section 8.55:
 * - InsertHistory (Bit 8): allows inserting historical values
 * - ModifyHistory (Bit 9): allows modifying historical values */
static UA_Boolean
allowHistoryUpdateUpdateData_default(UA_Server *server, UA_AccessControl *ac,
                                     const UA_NodeId *sessionId, void *sessionContext,
                                     const UA_NodeId *nodeId,
                                     UA_PerformUpdateType performInsertReplace,
                                     const UA_DataValue *value) {
#ifdef UA_ENABLE_RBAC
    /* Get effective permissions for this session on this node */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If no RBAC configuration, allow */
    if(effectivePerms == 0xFFFFFFFF)
        return true;
    
    /* Check appropriate permission based on operation type */
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

/* RBAC-aware allowHistoryUpdateDeleteRawModified implementation
 * 
 * Per OPC UA Part 3 Section 8.55:
 * - DeleteHistory (Bit 10): allows deleting historical values */
static UA_Boolean
allowHistoryUpdateDeleteRawModified_default(UA_Server *server, UA_AccessControl *ac,
                                            const UA_NodeId *sessionId, void *sessionContext,
                                            const UA_NodeId *nodeId,
                                            UA_DateTime startTimestamp,
                                            UA_DateTime endTimestamp,
                                            bool isDeleteModified) {
#ifdef UA_ENABLE_RBAC
    /* Get effective permissions for this session on this node */
    UA_PermissionType effectivePerms = 0;
    UA_StatusCode res = UA_Server_getEffectivePermissions(server, sessionId, nodeId, &effectivePerms);
    if(res != UA_STATUSCODE_GOOD)
        return true; /* Default permissive on error */
    
    /* If no RBAC configuration, allow */
    if(effectivePerms == 0xFFFFFFFF)
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

