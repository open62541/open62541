#include "ua_services.h"
#include "ua_application.h"

Session sessionMockup = {
		(UA_Int32) 0,
		&appMockup
};

UA_Int32 Service_CreateSession(UA_Session session, const UA_CreateSessionRequest *request, UA_CreateSessionResponse *response) {
	UA_String_printf("CreateSession Service - endpointUrl=", &(request->endpointUrl));
	// FIXME: create session
	UA_Session *newSession;

	UA_Session_new(newSession);
	UA_Session_init(*newSession, &request->sessionName,request->requestedSessionTimeout);

	UA_SessionManager_addSession(session);
	UA_Session_getId(*newSession, &response->sessionId);

	return UA_SUCCESS;
}

UA_Int32 Service_ActivateSession(UA_Session session, const UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response) {

	// FIXME: activate session
	UA_NodeId_printf("ActivateSession - authToken=", &(request->requestHeader.authenticationToken));
	// 321 == AnonymousIdentityToken_Encoding_DefaultBinary
	UA_NodeId_printf("ActivateSession - uIdToken.type=", &(request->userIdentityToken.typeId));
	UA_ByteString_printx_hex("ActivateSession - uIdToken.body=", &(request->userIdentityToken.body));

	// FIXME: channel->session->application = <Application Ptr>
	//FIXME channel->session = &sessionMockup;
	return UA_SUCCESS;
}

UA_Int32 Service_CloseSession(UA_Session session, const UA_CloseSessionRequest *request, UA_CloseSessionResponse *response) {
	//FIXME channel->session = UA_NULL;
	// FIXME: set response
	return UA_SUCCESS;
}
