#include "ua_services.h"

UA_Int32 service_createsession(UA_SL_Channel *channel, UA_CreateSessionRequest *request, UA_CreateSessionResponse *response) {
	UA_String_printf("CreateSession Service - endpointUrl=", &(request->endpointUrl));
	// FIXME: create session
	response->sessionId.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	response->sessionId.namespace = 1;
	response->sessionId.identifier.numeric = 666;
	return UA_SUCCESS;
}

UA_Int32 service_activatesession(UA_SL_Channel *channel, UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response) {
	// FIXME: activate session
	UA_NodeId_printf("ActivateSession - authToken=", &(request->requestHeader.authenticationToken));
	// 321 == AnonymousIdentityToken_Encoding_DefaultBinary
	UA_NodeId_printf("ActivateSession - uIdToken.type=", &(request->userIdentityToken.typeId));
	UA_ByteString_printx_hex("ActivateSession - uIdToken.body=", &(request->userIdentityToken.body));

	// FIXME: channel->application = <Application Ptr>
	
	return UA_SUCCESS;
}

UA_Int32 service_closesession(UA_SL_Channel *channel, UA_CloseSessionRequest *request, UA_CloseSessionResponse *response) {

	channel->session = UA_NULL;
	// FIXME: set response
	
	return UA_SUCCESS;
}
