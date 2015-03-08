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
    response->responseHeader.serviceResult = UA_SessionManager_createSession(&server->sessionManager, channel, &newSession);
	if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
		return;

    //TODO get maxResponseMessageSize internally
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = newSession->timeout;
    response->authenticationToken = newSession->authenticationToken;
    response->responseHeader.serviceResult = UA_String_copy(&request->sessionName, &newSession->sessionName);
    response->responseHeader.serviceResult |= UA_ByteString_copy(&server->serverCertificate, &response->serverCertificate);
    if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_SessionManager_removeSession(&server->sessionManager, &newSession->sessionId);
         return;
    }
}

void Service_ActivateSession(UA_Server *server,UA_SecureChannel *channel,
                             const UA_ActivateSessionRequest *request,
                             UA_ActivateSessionResponse *response) {
    // make the channel know about the session
	UA_Session *foundSession;
	UA_SessionManager_getSessionByToken(&server->sessionManager,
                                        (const UA_NodeId*)&request->requestHeader.authenticationToken,
                                        &foundSession);

	if(foundSession == UA_NULL)
        response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
    else
        channel->session = foundSession;
}

void Service_CloseSession(UA_Server *server, const UA_CloseSessionRequest *request,
                              UA_CloseSessionResponse *response) {
	UA_Session *foundSession;
	UA_SessionManager_getSessionByToken(&server->sessionManager,
			(const UA_NodeId*)&request->requestHeader.authenticationToken, &foundSession);

	if(foundSession == UA_NULL){
		response->responseHeader.serviceResult = UA_STATUSCODE_BADIDENTITYTOKENINVALID;
		return;
	}

	if(UA_SessionManager_removeSession(&server->sessionManager, &foundSession->sessionId) == UA_STATUSCODE_GOOD)
		response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;
	else
		response->responseHeader.serviceResult = UA_STATUSCODE_BADSECURECHANNELIDINVALID;
}
