#include "ua_services.h"
#include "ua_stack_session_manager.h"
#include "ua_application.h"

Session sessionMockup = {
		(UA_Int32) 0,
		&appMockup
};

UA_Int32 Service_CreateSession(UA_Session session, const UA_CreateSessionRequest *request, UA_CreateSessionResponse *response) {
	UA_String_printf("CreateSession Service - endpointUrl=", &(request->endpointUrl));

	UA_Session *newSession;

	UA_Session_new(&newSession);
	//TODO get maxResponseMessageSize
	UA_Session_init(*newSession, (UA_String*)&request->sessionName,
			request->requestedSessionTimeout,
			request->maxResponseMessageSize,
			9999,
			(UA_Session_idProvider)UA_SessionManager_generateSessionId);

	UA_SessionManager_addSession(newSession);
	UA_Session_getId(*newSession, &response->sessionId);
	UA_Session_getToken(*newSession, &(response->authenticationToken));


	return UA_SUCCESS;
}

UA_Int32 Service_ActivateSession(UA_Session session, const UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response) {

	// FIXME: activate session
	UA_NodeId_printf("ActivateSession - authToken=", &(request->requestHeader.authenticationToken));
	// 321 == AnonymousIdentityToken_Encoding_DefaultBinary
	UA_NodeId_printf("ActivateSession - uIdToken.type=", &(request->userIdentityToken.typeId));
	UA_ByteString_printx_hex("ActivateSession - uIdToken.body=", &(request->userIdentityToken.body));
	UA_Session_setApplicationPointer(session,&appMockup);
	// FIXME: channel->session->application = <Application Ptr>
	//FIXME channel->session = &sessionMockup;
	return UA_SUCCESS;
}

UA_Int32 Service_CloseSession(UA_Session session, const UA_CloseSessionRequest *request, UA_CloseSessionResponse *response) {
	//FIXME channel->session = UA_NULL;
	// FIXME: set response
	return UA_SUCCESS;
}
