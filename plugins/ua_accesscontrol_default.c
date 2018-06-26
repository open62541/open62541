/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_accesscontrol_default.h"

/* Example access control management. Anonymous and username / password login.
 * The access rights are maximally permissive. */

typedef struct {
    UA_Boolean allowAnonymous;
    size_t usernamePasswordLoginSize;
    UA_UsernamePasswordLogin *usernamePasswordLogin;
} AccessControlContext;

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define USERNAME_POLICY "open62541-username-policy"
const UA_String anonymous_policy = UA_STRING_STATIC(ANONYMOUS_POLICY);
const UA_String username_policy = UA_STRING_STATIC(USERNAME_POLICY);

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

        /* No userdata atm */
        *sessionContext = NULL;
        return UA_STATUSCODE_GOOD;
    }

    /* Username and password */
    if(userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        const UA_UserNameIdentityToken *userToken =
            (UA_UserNameIdentityToken*)userIdentityToken->content.decoded.data;

        if(!UA_String_equal(&userToken->policyId, &username_policy))
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* TODO: Support encrypted username/password over unencrypted SecureChannels */
        if(userToken->encryptionAlgorithm.length > 0)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* Empty username and password */
        if(userToken->userName.length == 0 && userToken->password.length == 0)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* Try to match username/pw */
        UA_Boolean match = false;
        for(size_t i = 0; i < context->usernamePasswordLoginSize; i++) {
            if(UA_String_equal(&userToken->userName, &context->usernamePasswordLogin[i].username) &&
               UA_String_equal(&userToken->password, &context->usernamePasswordLogin[i].password)) {
                match = true;
                break;
            }
        }
        if(!match)
            return UA_STATUSCODE_BADUSERACCESSDENIED;

        /* No userdata atm */
        *sessionContext = NULL;
        return UA_STATUSCODE_GOOD;
    }

    /* Unsupported token type */
    return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
}

static void
closeSession_default(UA_Server *server, UA_AccessControl *ac,
                     const UA_NodeId *sessionId, void *sessionContext) {
    /* no context to clean up */
}

static UA_UInt32
getUserRightsMask_default(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *nodeId, void *nodeContext) {
    return 0xFFFFFFFF;
}

static UA_Byte
getUserAccessLevel_default(UA_Server *server, UA_AccessControl *ac,
                           const UA_NodeId *sessionId, void *sessionContext,
                           const UA_NodeId *nodeId, void *nodeContext) {
    return 0xFF;
}

static UA_Boolean
getUserExecutable_default(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext) {
    return true;
}

static UA_Boolean
getUserExecutableOnObject_default(UA_Server *server, UA_AccessControl *ac,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext) {
    return true;
}

static UA_Boolean
allowAddNode_default(UA_Server *server, UA_AccessControl *ac,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_AddNodesItem *item) {
    return true;
}

static UA_Boolean
allowAddReference_default(UA_Server *server, UA_AccessControl *ac,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_AddReferencesItem *item) {
    return true;
}

static UA_Boolean
allowDeleteNode_default(UA_Server *server, UA_AccessControl *ac,
                        const UA_NodeId *sessionId, void *sessionContext,
                        const UA_DeleteNodesItem *item) {
    return true;
}

static UA_Boolean
allowDeleteReference_default(UA_Server *server, UA_AccessControl *ac,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_DeleteReferencesItem *item) {
    return true;
}

/***************************************/
/* Create Delete Access Control Plugin */
/***************************************/

static void deleteMembers_default(UA_AccessControl *ac) {
    UA_Array_delete((void*)(uintptr_t)ac->userTokenPolicies,
                    ac->userTokenPoliciesSize,
                    &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);

    AccessControlContext *context = (AccessControlContext*)ac->context;
    for(size_t i = 0; i < context->usernamePasswordLoginSize; i++) {
        UA_String_deleteMembers(&context->usernamePasswordLogin[i].username);
        UA_String_deleteMembers(&context->usernamePasswordLogin[i].password);
    }
    if(context->usernamePasswordLoginSize > 0)
        UA_free(context->usernamePasswordLogin);
    UA_free(ac->context);
}

UA_AccessControl
UA_AccessControl_default(UA_Boolean allowAnonymous, size_t usernamePasswordLoginSize,
                         const UA_UsernamePasswordLogin *usernamePasswordLogin) {
    AccessControlContext *context = (AccessControlContext*)
        UA_malloc(sizeof(AccessControlContext));
    memset(context, 0, sizeof(AccessControlContext));
    UA_AccessControl ac;
    memset(&ac, 0, sizeof(ac));
    ac.context = context;
    ac.deleteMembers = deleteMembers_default;
    ac.activateSession = activateSession_default;
    ac.closeSession = closeSession_default;
    ac.getUserRightsMask = getUserRightsMask_default;
    ac.getUserAccessLevel = getUserAccessLevel_default;
    ac.getUserExecutable = getUserExecutable_default;
    ac.getUserExecutableOnObject = getUserExecutableOnObject_default;
    ac.allowAddNode = allowAddNode_default;
    ac.allowAddReference = allowAddReference_default;
    ac.allowDeleteNode = allowDeleteNode_default;
    ac.allowDeleteReference = allowDeleteReference_default;

    /* Allow anonymous? */
    context->allowAnonymous = allowAnonymous;

    /* Copy username/password to the access control plugin */
    if(usernamePasswordLoginSize > 0) {
        context->usernamePasswordLogin = (UA_UsernamePasswordLogin*)
            UA_malloc(usernamePasswordLoginSize * sizeof(UA_UsernamePasswordLogin));
        if(!context->usernamePasswordLogin)
            return ac;
        context->usernamePasswordLoginSize = usernamePasswordLoginSize;
        for(size_t i = 0; i < usernamePasswordLoginSize; i++) {
            UA_String_copy(&usernamePasswordLogin[i].username, &context->usernamePasswordLogin[i].username);
            UA_String_copy(&usernamePasswordLogin[i].password, &context->usernamePasswordLogin[i].password);
        }
    }

    /* Set the allowed policies */
    size_t policies = 0;
    if(allowAnonymous)
        policies++;
    if(usernamePasswordLoginSize > 0)
        policies++;
    ac.userTokenPoliciesSize = 0;
    ac.userTokenPolicies = (UA_UserTokenPolicy *)
        UA_Array_new(policies, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    if(!ac.userTokenPolicies)
        return ac;
    ac.userTokenPoliciesSize = policies;

    policies = 0;
    if(allowAnonymous) {
        ac.userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
        ac.userTokenPolicies[policies].policyId = UA_STRING_ALLOC(ANONYMOUS_POLICY);
        policies++;
    }

    if(usernamePasswordLoginSize > 0) {
        ac.userTokenPolicies[policies].tokenType = UA_USERTOKENTYPE_USERNAME;
        ac.userTokenPolicies[policies].policyId = UA_STRING_ALLOC(USERNAME_POLICY);
        /* No encryption of username/password supported at the moment */
        ac.userTokenPolicies[policies].securityPolicyUri =
            UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
    }
    return ac;
}
