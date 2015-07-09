#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_session_manager.h"
#include "ua_statuscodes.h"
#include "ua_util.h"

void Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                           const UA_CreateSessionRequest *request,
                           UA_CreateSessionResponse *response) {
    response->responseHeader.serviceResult =
        UA_Array_copy(server->endpointDescriptions, (void**)&response->serverEndpoints,
                      &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION], server->endpointDescriptionsSize);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return;
    response->serverEndpointsSize = server->endpointDescriptionsSize;

	UA_Session *newSession;
    response->responseHeader.serviceResult = UA_SessionManager_createSession(&server->sessionManager,
                                                                             channel, request, &newSession);
	if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
		return;

    //TODO get maxResponseMessageSize internally
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = newSession->timeout;
    response->authenticationToken = newSession->authenticationToken;
    response->responseHeader.serviceResult = UA_String_copy(&request->sessionName, &newSession->sessionName);
    if(server->endpointDescriptions)
        response->responseHeader.serviceResult |=
            UA_ByteString_copy(&server->endpointDescriptions->serverCertificate, &response->serverCertificate);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager, server, &newSession->authenticationToken);
         return;
    }
}

void Service_ActivateSession(UA_Server *server, UA_SecureChannel *channel,
                             const UA_ActivateSessionRequest *request,
                             UA_ActivateSessionResponse *response) {
    // make the channel know about the session
	UA_Session *foundSession =
        UA_SessionManager_getSession(&server->sessionManager,
                                     (const UA_NodeId*)&request->requestHeader.authenticationToken);

	if(foundSession == UA_NULL) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        return;
	} else if(foundSession->validTill < UA_DateTime_now()) {
        response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
        return;
	}

    UA_UserIdentityToken token;
    UA_UserIdentityToken_init(&token);
    size_t offset = 0;
    UA_UserIdentityToken_decodeBinary(&request->userIdentityToken.body, &offset, &token);

    UA_UserNameIdentityToken username_token;
    UA_UserNameIdentityToken_init(&username_token);

    UA_String ap = UA_STRING(ANONYMOUS_POLICY);
    UA_String up = UA_STRING(USERNAME_POLICY);
    if(token.policyId.data == UA_NULL) {
        /* 1) no policy defined */
        response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    } else if(server->config.Login_enableAnonymous && UA_String_equal(&token.policyId, &ap)) {
        /* 2) anonymous logins */
        if(foundSession->channel && foundSession->channel != channel)
            UA_SecureChannel_detachSession(foundSession->channel, foundSession);
        UA_SecureChannel_attachSession(channel, foundSession);
        foundSession->activated = UA_TRUE;
        UA_Session_updateLifetime(foundSession);
    } else if(server->config.Login_enableUsernamePassword && UA_String_equal(&token.policyId, &up)) {
        /* 3) username logins */
        offset = 0;
        UA_UserNameIdentityToken_decodeBinary(&request->userIdentityToken.body, &offset, &username_token);
        if(username_token.encryptionAlgorithm.data != UA_NULL) {
            /* 3.1) we only support encryption */
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        } else  if(username_token.userName.length == -1 && username_token.password.length == -1){
            /* 3.2) empty username and password */
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        } else {
            /* 3.3) ok, trying to match the username */
            UA_UInt32 i = 0;
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
    UA_UserIdentityToken_deleteMembers(&token);
    UA_UserNameIdentityToken_deleteMembers(&username_token);
    return;
}

void Service_CloseSession(UA_Server *server, UA_Session *session, const UA_CloseSessionRequest *request,
                          UA_CloseSessionResponse *response) {
	UA_Session *foundSession =
        UA_SessionManager_getSession(&server->sessionManager,
		                             (const UA_NodeId*)&request->requestHeader.authenticationToken);
	if(foundSession == UA_NULL)
		response->responseHeader.serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
	else 
        response->responseHeader.serviceResult =
            UA_SessionManager_removeSession(&server->sessionManager, server, &session->authenticationToken);
}
