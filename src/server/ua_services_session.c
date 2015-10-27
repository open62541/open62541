#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_session_manager.h"
#include "ua_types_generated_encoding_binary.h"

void Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                           const UA_CreateSessionRequest *request,
                           UA_CreateSessionResponse *response) {
    response->responseHeader.serviceResult =
        UA_Array_copy(server->endpointDescriptions, server->endpointDescriptionsSize,
                      (void**)&response->serverEndpoints, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;
    response->serverEndpointsSize = server->endpointDescriptionsSize;

	UA_Session *newSession;
    response->responseHeader.serviceResult = UA_SessionManager_createSession(&server->sessionManager,
                                                                             channel, request, &newSession);
	if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
    UA_LOG_DEBUG(server->logger, UA_LOGCATEGORY_SESSION,
                 "Processing CreateSessionRequest on SecureChannel %i failed",
                 channel->securityToken.channelId);
		return;
    }

    //TODO get maxResponseMessageSize internally
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = (UA_Double)newSession->timeout;
    response->authenticationToken = newSession->authenticationToken;
    response->responseHeader.serviceResult = UA_String_copy(&request->sessionName, &newSession->sessionName);
    if(server->endpointDescriptions)
        response->responseHeader.serviceResult |=
            UA_ByteString_copy(&server->endpointDescriptions->serverCertificate, &response->serverCertificate);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager, server, &newSession->authenticationToken);
         return;
    }
    UA_LOG_DEBUG(server->logger, UA_LOGCATEGORY_SESSION,
                 "Processing CreateSessionRequest on SecureChannel %i succeeded, created Session (ns=%i,i=%i)",
                 channel->securityToken.channelId, response->sessionId.namespaceIndex,
                 response->sessionId.identifier.numeric);
}

void Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                             const UA_ActivateSessionRequest *request,
                             UA_ActivateSessionResponse *response) {
    // make the channel know about the session
	UA_Session *foundSession =
        UA_SessionManager_getSession(&server->sessionManager,
                                     (const UA_NodeId*)&request->requestHeader.authenticationToken);

	if(foundSession == NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        UA_LOG_DEBUG(server->logger, UA_LOGCATEGORY_SESSION,
                     "Processing ActivateSessionRequest on SecureChannel %i, but no session found for the authentication token",
                     channel->securityToken.channelId);
        return;
	} else if(foundSession->validTill < UA_DateTime_now()) {
        UA_LOG_DEBUG(server->logger, UA_LOGCATEGORY_SESSION,
                     "Processing ActivateSessionRequest on SecureChannel %i, but the session has timed out",
                     channel->securityToken.channelId);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        return;
	}
    UA_LOG_DEBUG(server->logger, UA_LOGCATEGORY_SESSION,
                 "Processing ActivateSessionRequest on SecureChannel %i for Session (ns=%i,i=%i)",
                 channel->securityToken.channelId, foundSession->sessionId.namespaceIndex,
                 foundSession->sessionId.identifier.numeric);

    if(request->userIdentityToken.encoding < UA_EXTENSIONOBJECT_DECODED ||
       (request->userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_USERIDENTITYTOKEN] &&
        request->userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN])) {
        UA_LOG_DEBUG(server->logger, UA_LOGCATEGORY_SESSION,
                     "Invalided UserIdentityToken on SecureChannel %i for Session (ns=%i,i=%i)",
                     channel->securityToken.channelId, foundSession->sessionId.namespaceIndex,
                     foundSession->sessionId.identifier.numeric);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }
    const UA_UserIdentityToken token = *(const UA_UserIdentityToken*)request->userIdentityToken.content.decoded.data;

    UA_String ap = UA_STRING(ANONYMOUS_POLICY);
    UA_String up = UA_STRING(USERNAME_POLICY);
    //(Compatibility notice)
    //Siemens OPC Scout v10 provides an empty policyId, this is not okay
    //For compatibility we will assume that empty policyId == ANONYMOUS_POLICY
    //if(token.policyId.data == NULL) {
    //    /* 1) no policy defined */
    //    response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    //} else
    //(End Compatibility notice)
    if(server->config.Login_enableAnonymous && (token.policyId.data == NULL || UA_String_equal(&token.policyId, &ap))) {
        /* 2) anonymous logins */
        if(foundSession->channel && foundSession->channel != channel)
            UA_SecureChannel_detachSession(foundSession->channel, foundSession);
        UA_SecureChannel_attachSession(channel, foundSession);
        foundSession->activated = UA_TRUE;
        UA_Session_updateLifetime(foundSession);
    } else if(server->config.Login_enableUsernamePassword && UA_String_equal(&token.policyId, &up) &&
              request->userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        /* 3) username logins */
        const UA_UserNameIdentityToken username_token =
            *(const UA_UserNameIdentityToken*)request->userIdentityToken.content.decoded.data;

        if(username_token.encryptionAlgorithm.data != NULL) {
            /* 3.1) we only support encryption */
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        } else  if(username_token.userName.data == NULL && username_token.password.data == NULL){
            /* 3.2) empty username and password */
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        } else {
            /* 3.3) ok, trying to match the username */
            size_t i = 0;
            for(; i < server->config.Login_loginsCount; ++i) {
                UA_String user = UA_STRING(server->config.Login_usernames[i]);
                UA_String pw = UA_STRING(server->config.Login_passwords[i]);
                if(UA_String_equal(&username_token.userName, &user) &&
                   UA_String_equal(&username_token.password, &pw)) {
                    /* success - activate */
                    if(foundSession->channel && foundSession->channel != channel)
                        UA_SecureChannel_detachSession(foundSession->channel, foundSession);
                    UA_SecureChannel_attachSession(channel, foundSession);
                    foundSession->activated = UA_TRUE;
                    UA_Session_updateLifetime(foundSession);
                    break;
                }
            }
            /* no username/pass matched */
            if(i >= server->config.Login_loginsCount)
                response->responseHeader.serviceResult = UA_STATUSCODE_BADUSERACCESSDENIED;
        }
    } else {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    }
    return;
}

void Service_CloseSession(UA_Server *server, UA_Session *session, const UA_CloseSessionRequest *request,
                          UA_CloseSessionResponse *response) {
    UA_LOG_DEBUG(server->logger, UA_LOGCATEGORY_SESSION,
                 "Processing CloseSessionRequest for Session (ns=%i,i=%i)",
                 session->sessionId.namespaceIndex, session->sessionId.identifier.numeric);
    response->responseHeader.serviceResult =
        UA_SessionManager_removeSession(&server->sessionManager, server, &session->authenticationToken);
}
