#include "ua_services.h"
#include "ua_server.h"

UA_Int32 Service_CreateSession(SL_Channel *channel, UA_Server *server, const UA_CreateSessionRequest *request, UA_CreateSessionResponse *response) {
#ifdef DEBUG
	UA_String_printf("CreateSession Service - endpointUrl=", &(request->endpointUrl));
#endif
	UA_Session *newSession;
	UA_Int64 timeout;

	UA_SessionManager_getSessionTimeout(&timeout);
	UA_Session_new(&newSession);
	//TODO get maxResponseMessageSize
	UA_Session_setApplicationPointer(newSession, server->application); // todo: select application according to the endpointurl in the request
	UA_Session_init(newSession, (UA_String*)&request->sessionName,
	request->requestedSessionTimeout,
	request->maxResponseMessageSize,
	9999,
	(UA_Session_idProvider)UA_SessionManager_generateSessionId,
	timeout);

	UA_SessionManager_addSession(newSession);
	UA_Session_getId(newSession, &response->sessionId);
	UA_Session_getToken(newSession, &(response->authenticationToken));
	response->revisedSessionTimeout = timeout;
	//TODO fill results
	return UA_SUCCESS;
}

UA_Int32 Service_ActivateSession(SL_Channel *channel, UA_Session *session, const UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response) {
	UA_Session_bind(session, channel);
#ifdef DEBUG
	UA_NodeId_printf("ActivateSession - authToken=", &(request->requestHeader.authenticationToken));
	// 321 == AnonymousIdentityToken_Encoding_DefaultBinary
	UA_NodeId_printf("ActivateSession - uIdToken.type=", &(request->userIdentityToken.typeId));
	UA_ByteString_printx_hex("ActivateSession - uIdToken.body=", &(request->userIdentityToken.body));
#endif
	//TODO fill results
	return UA_SUCCESS;
}

UA_Int32 Service_CloseSession(UA_Session *session, const UA_CloseSessionRequest *request, UA_CloseSessionResponse *response) {
	UA_NodeId sessionId;
	UA_Session_getId(session,&sessionId);

	UA_SessionManager_removeSession(&sessionId);
// FIXME: set response
return UA_SUCCESS;
}
