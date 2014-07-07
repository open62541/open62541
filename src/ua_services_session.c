#include "ua_services.h"
#include "ua_application.h"

Session sessionMockup = {
		(UA_Int32) 0,
		&appMockup
};

UA_Int32 Service_CreateSession(SL_Channel *channel, const UA_CreateSessionRequest *request, UA_CreateSessionResponse *response) {
#ifdef DEBUG
	UA_String_printf("CreateSession Service - endpointUrl=", &(request->endpointUrl));
#endif
	// FIXME: create session

	response->sessionId.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	response->sessionId.namespace = 1;
	response->sessionId.identifier.numeric = 666;
	return UA_SUCCESS;
}

UA_Int32 Service_ActivateSession(SL_Channel *channel, const UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response) {
	// FIXME: activate session
#ifdef DEBUG
	UA_NodeId_printf("ActivateSession - authToken=", &(request->requestHeader.authenticationToken));
	// 321 == AnonymousIdentityToken_Encoding_DefaultBinary
	UA_NodeId_printf("ActivateSession - uIdToken.type=", &(request->userIdentityToken.typeId));
	UA_ByteString_printx_hex("ActivateSession - uIdToken.body=", &(request->userIdentityToken.body));
#endif

	// FIXME: channel->session->application = <Application Ptr>
	channel->session = &sessionMockup;
	return UA_SUCCESS;
}

UA_Int32 Service_CloseSession(SL_Channel *channel, const UA_CloseSessionRequest *request, UA_CloseSessionResponse *response) {
	channel->session = UA_NULL;
	// FIXME: set response
	return UA_SUCCESS;
}
