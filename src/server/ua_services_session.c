#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_session_manager.h"
#include "ua_types_generated_encoding_binary.h"

void Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                           const UA_CreateSessionRequest *request, UA_CreateSessionResponse *response) {
    if(channel->securityToken.channelId == 0) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURECHANNELIDINVALID;
        return;
    }
    response->responseHeader.serviceResult =
        UA_Array_copy(server->endpointDescriptions, server->endpointDescriptionsSize,
                      (void**)&response->serverEndpoints, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;
    response->serverEndpointsSize = server->endpointDescriptionsSize;

    UA_Session *newSession;
    response->responseHeader.serviceResult =
        UA_SessionManager_createSession(&server->sessionManager, channel, request, &newSession);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Processing CreateSessionRequest failed");
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
            UA_ByteString_copy(&server->endpointDescriptions->serverCertificate,
                               &response->serverCertificate);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager, &newSession->authenticationToken);
         return;
    }
    UA_LOG_DEBUG_CHANNEL(server->config.logger, channel, "Session %i created",
                         newSession->sessionId.identifier.numeric);
}

void
Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                        const UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response) {
    if(session->validTill < UA_DateTime_now()) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "ActivateSession: SecureChannel %i wants "
                            "to activate, but the session has timed out", channel->securityToken.channelId);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        return;
    }

    if(request->userIdentityToken.encoding < UA_EXTENSIONOBJECT_DECODED ||
       (request->userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN] &&
        request->userIdentityToken.content.decoded.type != &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN])) {
        UA_LOG_INFO_SESSION(server->config.logger, session, "ActivateSession: SecureChannel %i wants "
                            "to activate, but the UserIdentify token is invalid",
                            channel->securityToken.channelId);
        response->responseHeader.serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }


    UA_String ap = UA_STRING(ANONYMOUS_POLICY);
    UA_String up = UA_STRING(USERNAME_POLICY);

    /* Compatibility notice: Siemens OPC Scout v10 provides an empty policyId,
       this is not okay For compatibility we will assume that empty policyId ==
       ANONYMOUS_POLICY
       if(token.policyId->data == NULL)
           response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    */

    /* anonymous login */
    if(server->config.enableAnonymousLogin &&
       request->userIdentityToken.content.decoded.type == &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN]) {
        const UA_AnonymousIdentityToken *token = request->userIdentityToken.content.decoded.data;
        if(token->policyId.data && !UA_String_equal(&token->policyId, &ap)) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
            return;
        }
        if(session->channel && session->channel != channel) {
            /* Close the old SecureChannel (this also detaches it) */
            UA_LOG_INFO_SESSION(server->config.logger, session,
                                "ActivateSession: Detach from old channel");
            UA_SecureChannelManager_close(&server->secureChannelManager,
                                          session->channel->securityToken.channelId);
        }
        UA_SecureChannel_attachSession(channel, session);
        session->activated = true;
        UA_Session_updateLifetime(session);
        UA_LOG_INFO_SESSION(server->config.logger, session, "ActivateSession: Session activated");
        return;
    }

    /* username login */
    if(server->config.enableUsernamePasswordLogin &&
       request->userIdentityToken.content.decoded.type == &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN]) {
        const UA_UserNameIdentityToken *token = request->userIdentityToken.content.decoded.data;
        if(!UA_String_equal(&token->policyId, &up)) {
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
            return;
        }
        if(token->encryptionAlgorithm.length > 0) {
            /* we don't support encryption */
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
            return;
        }
        /* ok, trying to match the username */
        for(size_t i = 0; i < server->config.usernamePasswordLoginsSize; i++) {
            UA_String *user = &server->config.usernamePasswordLogins[i].username;
            UA_String *pw = &server->config.usernamePasswordLogins[i].password;
            if(!UA_String_equal(&token->userName, user) || !UA_String_equal(&token->password, pw))
                continue;
            /* success - activate */
            if(session->channel && session->channel != channel) {
                UA_LOG_INFO_SESSION(server->config.logger, session,
                                    "ActivateSession: Detach from old channel");
                UA_SecureChannel_detachSession(session->channel, session);
            }
            UA_SecureChannel_attachSession(channel, session);
            session->activated = true;
            UA_Session_updateLifetime(session);
            UA_LOG_INFO_SESSION(server->config.logger, session, "ActivateSession: Session activated");
            return;
        }
        /* no match */
        UA_LOG_INFO_SESSION(server->config.logger, session,
                            "ActivateSession: Did not find matching username/password");
        response->responseHeader.serviceResult = UA_STATUSCODE_BADUSERACCESSDENIED;
        return;
    }
    response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
}

void
Service_CloseSession(UA_Server *server, UA_Session *session, const UA_CloseSessionRequest *request,
                     UA_CloseSessionResponse *response) {
    UA_LOG_INFO_SESSION(server->config.logger, session, "CloseSession");
    response->responseHeader.serviceResult =
        UA_SessionManager_removeSession(&server->sessionManager, &session->authenticationToken);
}
