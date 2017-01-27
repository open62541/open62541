/* This Source Code Form is subject to the terms of the Mozilla Public 
* License, v. 2.0. If a copy of the MPL was not distributed with this 
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */ 
/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "ua_accesscontrol_default.h"

/* We allow login anonymous and with the following username / password. The
 * access rights are maximally permissive in this example plugin. */

#define ANONYMOUS_POLICY "open62541-anonymous-policy"
#define USERNAME_POLICY "open62541-username-policy"

#define UA_STRING_STATIC(s) {sizeof(s)-1, (UA_Byte*)s}
const UA_String anonymous_policy = UA_STRING_STATIC(ANONYMOUS_POLICY);
const UA_String username_policy = UA_STRING_STATIC(USERNAME_POLICY);

UA_StatusCode
activateSession_default(const UA_NodeId *sessionId, const UA_ExtensionObject *userIdentityToken,
                        void **sessionHandle) {
    /* Could the token be decoded? */
    if(userIdentityToken->encoding < UA_EXTENSIONOBJECT_DECODED)
        return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

    /* anonymous login */
    if(enableAnonymousLogin &&
       userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]) {
        const UA_AnonymousIdentityToken *token = (UA_AnonymousIdentityToken *)userIdentityToken->content.decoded.data;

        /* Compatibility notice: Siemens OPC Scout v10 provides an empty
         * policyId. This is not compliant. For compatibility we will assume
         * that empty policyId == ANONYMOUS_POLICY */
        if(token->policyId.data && !UA_String_equal(&token->policyId, &anonymous_policy))
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        *sessionHandle = NULL;
        return UA_STATUSCODE_GOOD;
    }

    /* username and password */
    if(enableUsernamePasswordLogin &&
       userIdentityToken->content.decoded.type == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        const UA_UserNameIdentityToken *token = (UA_UserNameIdentityToken *)userIdentityToken->content.decoded.data;
        if(!UA_String_equal(&token->policyId, &username_policy))
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* empty username and password */
        if(token->userName.length == 0 && token->password.length == 0)
            return UA_STATUSCODE_BADIDENTITYTOKENINVALID;

        /* trying to match pw/username */
        UA_Boolean match = false;
        for(size_t i = 0; i < usernamePasswordsSize; i++) {
            const UA_String *user = &usernamePasswords[i].username;
            const UA_String *pw = &usernamePasswords[i].password;
            if(UA_String_equal(&token->userName, user) && UA_String_equal(&token->password, pw)) {
                match = true;
                break;
            }
        }
        if(!match)
            return UA_STATUSCODE_BADUSERACCESSDENIED;

        *sessionHandle = NULL;
        return UA_STATUSCODE_GOOD;
    }

    /* Unsupported token type */
    return UA_STATUSCODE_BADIDENTITYTOKENINVALID;
}

void
closeSession_default(const UA_NodeId *sessionId, void *sessionHandle) {
    /* no handle to clean up */
}

UA_UInt32
getUserRightsMask_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_NodeId *nodeId) {
    return 0xFFFFFFFF;
}

UA_Byte
getUserAccessLevel_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_NodeId *nodeId) {
    return 0xFF;
}

UA_Boolean
getUserExecutable_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_NodeId *nodeId) {
    return true;
}

UA_Boolean
getUserExecutableOnObject_default(const UA_NodeId *sessionId, void *sessionHandle,
                                  const UA_NodeId *methodId, const UA_NodeId *objectId) {
    return true;
}

UA_Boolean
allowAddNode_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_AddNodesItem *item) {
    return true;
}

UA_Boolean
allowAddReference_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_AddReferencesItem *item) {
    return true;
}

UA_Boolean
allowDeleteNode_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_DeleteNodesItem *item) {
    return true;
}
      
UA_Boolean
allowDeleteReference_default(const UA_NodeId *sessionId, void *sessionHandle, const UA_DeleteReferencesItem *item) {
    return true;
}
