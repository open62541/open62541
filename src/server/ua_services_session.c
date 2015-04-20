#include "ua_services.h"
#include "ua_server_internal.h"
#include "ua_session_manager.h"
#include "ua_statuscodes.h"
#include "ua_util.h"

void Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                           const UA_CreateSessionRequest *request,
                           UA_CreateSessionResponse *response) {

    response->serverEndpoints = UA_malloc(sizeof(UA_EndpointDescription));
    if(!response->serverEndpoints || (response->responseHeader.serviceResult =
        UA_EndpointDescription_copy(server->endpointDescriptions, response->serverEndpoints)) !=
       UA_STATUSCODE_GOOD)
        return;
    response->serverEndpointsSize = 1;

    // creates a session and adds a pointer to the channel. Only when the
    // session is activated will the channel point to the session as well
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
        UA_SessionManager_removeSession(&server->sessionManager, &newSession->sessionId);
         return;
    }
}

#ifdef RETURN
#undef RETURN
#endif
#define RETURN  UA_UserIdentityToken_deleteMembers(&token); \
                UA_UserNameIdentityToken_deleteMembers(&username_token); \
                return
void Service_ActivateSession(UA_Server *server,UA_SecureChannel *channel,
                             const UA_ActivateSessionRequest *request,
                             UA_ActivateSessionResponse *response) {
    // make the channel know about the session
	UA_Session *foundSession;
	UA_SessionManager_getSessionByToken(&server->sessionManager,
                                        (const UA_NodeId*)&request->requestHeader.authenticationToken,
                                        &foundSession);

	if(foundSession == UA_NULL){
        response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        return;
	}



    UA_UserIdentityToken token;
    UA_UserIdentityToken_init(&token);
    size_t offset = 0;
    UA_UserIdentityToken_decodeBinary(&request->userIdentityToken.body, &offset, &token);

    UA_UserNameIdentityToken username_token;
    UA_UserNameIdentityToken_init(&username_token);

    //check policies

    if(token.policyId.data == UA_NULL){ //user identity token is NULL
        response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        //todo cleanup session
        RETURN;
    }

    //anonymous logins
    if(!server->config.Login_enableAnonymous && UA_String_equalchars(&token.policyId, ANONYMOUS_POLICY)){
        response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
        UA_UserIdentityToken_deleteMembers(&token);
        //todo cleanup session
        RETURN;
    }

    //username logins
    if(UA_String_equalchars(&token.policyId, USERNAME_POLICY)){
        if(!server->config.Login_enableUsernamePassword){
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
            //todo cleanup session
            RETURN;
        }
        offset = 0;
        UA_UserNameIdentityToken_decodeBinary(&request->userIdentityToken.body, &offset, &username_token);
        if(username_token.encryptionAlgorithm.data != UA_NULL){
            //we only support encryption
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
            //todo cleanup session
            RETURN;
        }
        UA_Boolean matched = UA_FALSE;
        for(UA_UInt32 i=0;i<server->config.Login_loginsCount;++i){
            if(UA_String_equalchars(&username_token.userName, server->config.Login_usernames[i])
            && UA_String_equalchars(&username_token.password, server->config.Login_passwords[i])){
                matched = UA_TRUE;
                break;
            }
        }
        if(!matched){
            //no username/pass matched
            response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
            //todo cleanup session
            RETURN;
        }
   }

   //success - bind session to the channel
   channel->session = foundSession;

   RETURN;

}
#undef RETURN

void Service_CloseSession(UA_Server *server, UA_Session *session, const UA_CloseSessionRequest *request,
                          UA_CloseSessionResponse *response) {
	UA_Session *foundSession;
	UA_SessionManager_getSessionByToken(&server->sessionManager,
			(const UA_NodeId*)&request->requestHeader.authenticationToken, &foundSession);

	if(foundSession == UA_NULL){
		response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
		return;
	}

	response->responseHeader.serviceResult =
        UA_SessionManager_removeSession(&server->sessionManager, &session->sessionId);
}
