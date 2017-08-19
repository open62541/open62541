/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "ua_accesscontrol_default.h"

/* Example access control management. Anonymous and username / password login.
 * The access rights are maximally permissive. */

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define USERNAME_POLICY "open62541-username-policy"

// TODO: There should be one definition of these strings in the endpoint.
// Put the endpoint definition in the access control struct?
#define UA_STRING_STATIC(s) {sizeof(s)-1, (UA_Byte*)s}
const UA_String anonymous_policy = UA_STRING_STATIC(ANONYMOUS_POLICY);
const UA_String username_policy = UA_STRING_STATIC(USERNAME_POLICY);

typedef struct {
    UA_String username;
    UA_String password;
} UA_UsernamePasswordLogin;

const size_t usernamePasswordsSize = 2;
UA_UsernamePasswordLogin usernamePasswords[2] = {
    { UA_STRING_STATIC("user1"), UA_STRING_STATIC("password") },
    { UA_STRING_STATIC("user2"), UA_STRING_STATIC("password1") } };

UA_StatusCode
activateSession_default(const UA_NodeId *sessionId,
                        const UA_ExtensionObject *userIdentityToken,
                        void **sessionContext) {
    /* Could the token be decoded? */
    if(userIdentityToken->encoding < UA_EXTENSIONOBJECT_DECODED)
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    /* Anonymous login */
    if(userIdentityToken->content.decoded.type ==
       &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]) {
        const UA_AnonymousIdentityToken *token =
            (UA_AnonymousIdentityToken*)userIdentityToken->content.decoded.data;

        /* Compatibility notice: Siemens OPC Scout v10 provides an empty
         * policyId. This is not compliant. For compatibility, assume that empty
         * policyId == ANONYMOUS_POLICY */
        if(token->policyId.data &&
           !UA_String_equal(&token->policyId, &anonymous_policy))
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* No userdata atm */
        *sessionContext = NULL;
        return UA_STATUSCODE_GOOD;
    }

    /* Username and password */
    if(userIdentityToken->content.decoded.type ==
       &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        const UA_UserNameIdentityToken *token =
            (UA_UserNameIdentityToken*)userIdentityToken->content.decoded.data;
        if(!UA_String_equal(&token->policyId, &username_policy) || token->encryptionAlgorithm.length > 0)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* Empty username and password */
        if(token->userName.length == 0 && token->password.length == 0)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* Try to match username/pw */
        UA_Boolean match = false;
        for(size_t i = 0; i < usernamePasswordsSize; i++) {
            const UA_String *user = &usernamePasswords[i].username;
            const UA_String *pw = &usernamePasswords[i].password;
            if(UA_String_equal(&token->userName, user) &&
               UA_String_equal(&token->password, pw)) {
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

void
closeSession_default(const UA_NodeId *sessionId, void *sessionContext) {
    /* no context to clean up */
}

UA_UInt32
getUserRightsMask_default(const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *nodeId, void *nodeContext) {
    return 0xFFFFFFFF;
}

UA_Byte
getUserAccessLevel_default(const UA_NodeId *sessionId, void *sessionContext,
                           const UA_NodeId *nodeId, void *nodeContext) {
    return 0xFF;
}

UA_Boolean
getUserExecutable_default(const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext) {
    return true;
}

UA_Boolean
getUserExecutableOnObject_default(const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext) {
    return true;
}

UA_Boolean
allowAddNode_default(const UA_NodeId *sessionId, void *sessionContext,
                     const UA_AddNodesItem *item) {
    return true;
}

UA_Boolean
allowAddReference_default(const UA_NodeId *sessionId, void *sessionContext,
                          const UA_AddReferencesItem *item) {
    return true;
}

UA_Boolean
allowDeleteNode_default(const UA_NodeId *sessionId, void *sessionContext,
                        const UA_DeleteNodesItem *item) {
    return true;
}
      
UA_Boolean
allowDeleteReference_default(const UA_NodeId *sessionId, void *sessionContext,
                             const UA_DeleteReferencesItem *item) {
    return true;
}
