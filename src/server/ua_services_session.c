#include "ua_services.h"
#include "ua_server.h"
#include "ua_session_manager.h"

UA_Int32 Service_CreateSession(UA_Server *server, UA_SecureChannel *channel,
                               const UA_CreateSessionRequest *request,
                               UA_CreateSessionResponse *response) {
    // creates a session and adds a pointer to the channel. Only when the
    // session is activated will the channel point to the session as well
    UA_Int32 retval = UA_SUCCESS;
	UA_Session *newSession;
    retval |= UA_SessionManager_createSession(server->sessionManager, channel, &newSession);
	if(retval != UA_SUCCESS)
	{
		return retval;
	}
    //TODO get maxResponseMessageSize
    UA_String_copy(&request->sessionName, &newSession->sessionName);
    newSession->maxResponseMessageSize = request->maxResponseMessageSize;
    
    response->sessionId = newSession->sessionId;
    response->revisedSessionTimeout = newSession->timeout;
    response->authenticationToken = newSession->authenticationToken;
    //channel->session = newSession;
    return retval;
}

UA_Int32 Service_ActivateSession(UA_Server *server,UA_SecureChannel *channel,
                                 const UA_ActivateSessionRequest *request,
                                 UA_ActivateSessionResponse *response) {
    // make the channel know about the session
	UA_Session *foundSession;

	UA_SessionManager_getSessionByToken(server->sessionManager,(UA_NodeId*)&request->requestHeader.authenticationToken,&foundSession);

	if(foundSession == UA_NULL)
	{
		return UA_ERROR;
	}
	//channel at creation must be the same at activation
	if(foundSession->channel !=channel)
	{
		return UA_ERROR;
	}
    channel->session = foundSession;

    return UA_SUCCESS;
}

UA_Int32 Service_CloseSession(UA_Server *server, UA_Session *session,
                              const UA_CloseSessionRequest *request,
                              UA_CloseSessionResponse *response) {
    session->channel->session = UA_NULL;
    UA_SessionManager_removeSession(server->sessionManager, &session->sessionId);
    /* UA_NodeId sessionId; */
    /* UA_Session_getId(session,&sessionId); */

    /* UA_SessionManager_removeSession(&sessionId); */
    // FIXME: set response
    return UA_SUCCESS;
}
